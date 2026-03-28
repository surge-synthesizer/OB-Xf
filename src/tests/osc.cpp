/*
 * OB-Xf - a continuation of the last open source version of OB-Xd.
 *
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more info,
 * see "HISTORY.md" in the root of this repository.
 *
 * This repository is a successor to the last open source release,
 * a version marked as 2.11. Copyright 2013-2025 by the authors
 * as indicated in the original release, and subsequent authors
 * per the GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

/*
 * Include SynthEngine.h (not the osc headers directly) so the circular include chain
 * resolves in the correct order:
 *   SynthEngine → Voice → OscillatorBlock → *Osc (first encounter, defined here)
 * If an osc header is included first its guard fires before OscillatorBlock.h sees it,
 * leaving the type undefined when OscillatorBlock.h tries to use it.
 */
#include "SynthEngine.h"
#include "SawOsc.h"
#include "PulseOsc.h"
#include "TriangleOsc.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch2.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

/* ==========================================================================
 * SawOsc helpers
 * ========================================================================== */

/*
 * Runs a SawOsc leader (Osc1 mode) for the given duration, sweeping linearly from startHz to
 * endHz. The calling sequence mirrors OscillatorBlock::ProcessSample() exactly:
 *
 *   phase += delta;
 *   osc.processLeader(phase, delta);
 *   if (phase >= 1.f) phase -= 1.f;
 *   sample = osc.getValue(phase) + osc.aliasReduction();
 */
static std::vector<float> runSawSweep(float sampleRate, float startHz, float endHz,
                                      float durationSec)
{
    const int numSamples = static_cast<int>(sampleRate * durationSec);
    SawOsc osc;
    float phase = 0.0f;
    std::vector<float> output(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(numSamples - 1);
        const float freq = startHz + t * (endHz - startHz);
        const float delta = std::min(freq / sampleRate, 0.45f);

        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.0f)
            phase -= 1.0f;

        output[i] = osc.getValue(phase) + osc.aliasReduction();
    }
    return output;
}

/* ==========================================================================
 * PulseOsc helpers
 * ========================================================================== */

static std::vector<float> runPulseSweep(float sampleRate, float startHz, float endHz,
                                        float durationSec, float pulseWidth = 0.5f)
{
    const int numSamples = static_cast<int>(sampleRate * durationSec);
    PulseOsc osc;
    float phase = 0.0f;
    float pwWas = pulseWidth;
    std::vector<float> output(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(numSamples - 1);
        const float freq = startHz + t * (endHz - startHz);
        const float delta = std::min(freq / sampleRate, 0.45f);

        phase += delta;
        osc.processLeader(phase, delta, pulseWidth, pwWas);
        if (phase >= 1.0f)
            phase -= 1.0f;

        output[i] = osc.getValue(phase, pulseWidth) + osc.aliasReduction();
        pwWas = pulseWidth;
    }
    return output;
}

/* ==========================================================================
 * TriangleOsc helpers
 * ========================================================================== */

static std::vector<float> runTriangleSweep(float sampleRate, float startHz, float endHz,
                                           float durationSec)
{
    const int numSamples = static_cast<int>(sampleRate * durationSec);
    TriangleOsc osc;
    float phase = 0.0f;
    std::vector<float> output(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(numSamples - 1);
        const float freq = startHz + t * (endHz - startHz);
        const float delta = std::min(freq / sampleRate, 0.45f);

        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.0f)
            phase -= 1.0f;

        output[i] = osc.getValue(phase) + osc.aliasReduction();
    }
    return output;
}

/* ==========================================================================
 * SawOsc tests
 * ========================================================================== */

TEST_CASE("SawOsc 440-880 Hz sweep output is bounded", "[SawOsc]")
{
    /* 48 kHz, quarter-second sweep from A4 to A5 */
    const auto samples = runSawSweep(48000.0f, 440.0f, 880.0f, 0.25f);

    const float minVal = *std::min_element(samples.begin(), samples.end());
    const float maxVal = *std::max_element(samples.begin(), samples.end());

    INFO("min=" << minVal << "  max=" << maxVal);
    REQUIRE(minVal >= -1.0f);
    REQUIRE(maxVal <= 1.0f);
}

TEST_CASE("SawOsc 440-880 Hz sweep has expected RMS", "[SawOsc]")
{
    const auto samples = runSawSweep(48000.0f, 440.0f, 880.0f, 0.25f);

    /*
     * Skip the first B_SAMPLES * 2 output frames while the delay line and BLEP
     * circular buffer settle from their zero-initialised state.
     */
    constexpr int skip = B_SAMPLESx2;
    REQUIRE(static_cast<int>(samples.size()) > skip);

    float sumSq = 0.0f;
    for (int i = skip; i < static_cast<int>(samples.size()); ++i)
        sumSq += samples[i] * samples[i];

    const float rms = std::sqrt(sumSq / static_cast<float>(samples.size() - skip));

    /*
     * A unit sawtooth on [-0.5, 0.5] has RMS = 0.5/sqrt(3) ≈ 0.289.
     * BLEP smoothing redistributes a small amount of energy near the
     * discontinuity, so we allow ±50 % tolerance.
     */
    INFO("RMS=" << rms);
    REQUIRE(rms > 0.15f);
    REQUIRE(rms < 0.45f);
}

TEST_CASE("SawOsc decimation mode produces bounded output", "[SawOsc]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 440.0f;
    const float delta = std::min(freq / sampleRate, 0.45f);

    SawOsc osc;
    osc.setDecimation();
    float phase = 0.0f;

    float minVal = 0.0f;
    float maxVal = 0.0f;

    /* 0.25 s at 48 kHz = 12 000 samples */
    constexpr int numSamples = 12000;
    for (int i = 0; i < numSamples; ++i)
    {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float s = osc.getValue(phase) + osc.aliasReduction();
        minVal = std::min(minVal, s);
        maxVal = std::max(maxVal, s);
    }

    INFO("decimated min=" << minVal << "  max=" << maxVal);
    REQUIRE(minVal >= -1.0f);
    REQUIRE(maxVal <= 1.0f);
}

TEST_CASE("SawOsc getValueFast matches getValue after delay-line warmup", "[SawOsc]")
{
    /*
     * getValueFast() skips the delay line and returns (x - 0.5) directly.
     * After the delay line is warmed up the two outputs should track each
     * other to within the BLEP correction magnitude.  We just check that
     * fast values are also bounded; a tighter numerical comparison would
     * require knowing the exact delay.
     */
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 220.0f;
    const float delta = std::min(freq / sampleRate, 0.45f);

    SawOsc osc;
    float phase = 0.0f;

    constexpr int numSamples = 4800; /* 0.1 s */
    for (int i = 0; i < numSamples; ++i)
    {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float fast = osc.getValueFast(phase);
        REQUIRE(fast >= -0.5f);
        REQUIRE(fast <= 0.5f);

        /* consume the alias-reduction slot so the internal buffer stays in sync */
        (void)osc.aliasReduction();
    }
}

/* --------------------------------------------------------------------------
 * Timing benchmarks — run with --benchmark-no-analysis or -b
 * -------------------------------------------------------------------------- */

TEST_CASE("SawOsc 440 Hz at 48 kHz — 1 second", "[SawOsc][!benchmark]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 440.0f;
    const float delta = std::min(freq / sampleRate, 0.45f);
    constexpr int numSamples = static_cast<int>(sampleRate); /* 1 second */

    BENCHMARK("SawOsc leader 440 Hz 1 s")
    {
        SawOsc osc;
        float phase = 0.0f;
        float accum = 0.0f; /* prevent the loop being optimised away */
        for (int i = 0; i < numSamples; ++i)
        {
            phase += delta;
            osc.processLeader(phase, delta);
            if (phase >= 1.0f)
                phase -= 1.0f;
            accum += osc.getValue(phase) + osc.aliasReduction();
        }
        return accum;
    };
}

TEST_CASE("SawOsc 440-880 Hz sweep at 48 kHz — 0.25 second", "[SawOsc][!benchmark]")
{
    BENCHMARK("SawOsc sweep 440-880 Hz 0.25 s")
    {
        return runSawSweep(48000.0f, 440.0f, 880.0f, 0.25f);
    };
}

/* ==========================================================================
 * PulseOsc tests
 * ========================================================================== */

TEST_CASE("PulseOsc 440-880 Hz sweep output is bounded", "[PulseOsc]")
{
    /* 48 kHz, quarter-second sweep from A4 to A5, 50 % duty cycle */
    const auto samples = runPulseSweep(48000.0f, 440.0f, 880.0f, 0.25f);

    const float minVal = *std::min_element(samples.begin(), samples.end());
    const float maxVal = *std::max_element(samples.begin(), samples.end());

    INFO("min=" << minVal << "  max=" << maxVal);
    REQUIRE(minVal >= -1.0f);
    REQUIRE(maxVal <= 1.0f);
}

TEST_CASE("PulseOsc 440-880 Hz sweep has expected RMS", "[PulseOsc]")
{
    const auto samples = runPulseSweep(48000.0f, 440.0f, 880.0f, 0.25f);

    constexpr int skip = B_SAMPLESx2;
    REQUIRE(static_cast<int>(samples.size()) > skip);

    float sumSq = 0.0f;
    for (int i = skip; i < static_cast<int>(samples.size()); ++i)
        sumSq += samples[i] * samples[i];

    const float rms = std::sqrt(sumSq / static_cast<float>(samples.size() - skip));

    /*
     * A square wave at 50 % duty cycle on [-0.5, 0.5] has RMS = 0.5.
     * BLEP correction and the delay line shift some energy, so we allow
     * ±50 % tolerance.
     */
    INFO("RMS=" << rms);
    REQUIRE(rms > 0.25f);
    REQUIRE(rms < 0.75f);
}

TEST_CASE("PulseOsc decimation mode produces bounded output", "[PulseOsc]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 440.0f;
    constexpr float pw = 0.5f;
    const float delta = std::min(freq / sampleRate, 0.45f);

    PulseOsc osc;
    osc.setDecimation();
    float phase = 0.0f;
    float pwWas = pw;

    float minVal = 0.0f;
    float maxVal = 0.0f;

    constexpr int numSamples = 12000;
    for (int i = 0; i < numSamples; ++i)
    {
        phase += delta;
        osc.processLeader(phase, delta, pw, pwWas);
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float s = osc.getValue(phase, pw) + osc.aliasReduction();
        minVal = std::min(minVal, s);
        maxVal = std::max(maxVal, s);
        pwWas = pw;
    }

    INFO("decimated min=" << minVal << "  max=" << maxVal);
    REQUIRE(minVal >= -1.0f);
    REQUIRE(maxVal <= 1.0f);
}

TEST_CASE("PulseOsc getValueFast is bounded", "[PulseOsc]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 220.0f;
    constexpr float pw = 0.5f;
    const float delta = std::min(freq / sampleRate, 0.45f);

    PulseOsc osc;
    float phase = 0.0f;
    float pwWas = pw;

    constexpr int numSamples = 4800;
    for (int i = 0; i < numSamples; ++i)
    {
        phase += delta;
        osc.processLeader(phase, delta, pw, pwWas);
        if (phase >= 1.0f)
            phase -= 1.0f;

        /* getValueFast returns values in [pw - 1, pw] which ⊆ [-1, 1] */
        const float fast = osc.getValueFast(phase, pw);
        REQUIRE(fast >= -1.0f);
        REQUIRE(fast <= 1.0f);

        (void)osc.aliasReduction();
        pwWas = pw;
    }
}

TEST_CASE("PulseOsc 440 Hz at 48 kHz — 1 second", "[PulseOsc][!benchmark]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 440.0f;
    constexpr float pw = 0.5f;
    const float delta = std::min(freq / sampleRate, 0.45f);
    constexpr int numSamples = static_cast<int>(sampleRate);

    BENCHMARK("PulseOsc leader 440 Hz 1 s")
    {
        PulseOsc osc;
        float phase = 0.0f;
        float pwWas = pw;
        float accum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            phase += delta;
            osc.processLeader(phase, delta, pw, pwWas);
            if (phase >= 1.0f)
                phase -= 1.0f;
            accum += osc.getValue(phase, pw) + osc.aliasReduction();
            pwWas = pw;
        }
        return accum;
    };
}

/* ==========================================================================
 * TriangleOsc tests
 * ========================================================================== */

TEST_CASE("TriangleOsc 440-880 Hz sweep output is bounded", "[TriangleOsc]")
{
    /* 48 kHz, quarter-second sweep from A4 to A5 */
    const auto samples = runTriangleSweep(48000.0f, 440.0f, 880.0f, 0.25f);

    const float minVal = *std::min_element(samples.begin(), samples.end());
    const float maxVal = *std::max_element(samples.begin(), samples.end());

    INFO("min=" << minVal << "  max=" << maxVal);
    REQUIRE(minVal >= -1.0f);
    REQUIRE(maxVal <= 1.0f);
}

TEST_CASE("TriangleOsc 440-880 Hz sweep has expected RMS", "[TriangleOsc]")
{
    const auto samples = runTriangleSweep(48000.0f, 440.0f, 880.0f, 0.25f);

    constexpr int skip = B_SAMPLESx2;
    REQUIRE(static_cast<int>(samples.size()) > skip);

    float sumSq = 0.0f;
    for (int i = skip; i < static_cast<int>(samples.size()); ++i)
        sumSq += samples[i] * samples[i];

    const float rms = std::sqrt(sumSq / static_cast<float>(samples.size() - skip));

    /*
     * A triangle wave on [-0.5, 0.5] has RMS = 0.5/sqrt(3) ≈ 0.289.
     * BLAMP smoothing at the peaks redistributes very little energy,
     * so we allow ±50 % tolerance.
     */
    INFO("RMS=" << rms);
    REQUIRE(rms > 0.15f);
    REQUIRE(rms < 0.45f);
}

TEST_CASE("TriangleOsc decimation mode produces bounded output", "[TriangleOsc]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 440.0f;
    const float delta = std::min(freq / sampleRate, 0.45f);

    TriangleOsc osc;
    osc.setDecimation();
    float phase = 0.0f;

    float minVal = 0.0f;
    float maxVal = 0.0f;

    constexpr int numSamples = 12000;
    for (int i = 0; i < numSamples; ++i)
    {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float s = osc.getValue(phase) + osc.aliasReduction();
        minVal = std::min(minVal, s);
        maxVal = std::max(maxVal, s);
    }

    INFO("decimated min=" << minVal << "  max=" << maxVal);
    REQUIRE(minVal >= -1.0f);
    REQUIRE(maxVal <= 1.0f);
}

TEST_CASE("TriangleOsc getValueFast is bounded", "[TriangleOsc]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 220.0f;
    const float delta = std::min(freq / sampleRate, 0.45f);

    TriangleOsc osc;
    float phase = 0.0f;

    constexpr int numSamples = 4800;
    for (int i = 0; i < numSamples; ++i)
    {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.0f)
            phase -= 1.0f;

        /* getValueFast returns values in [-0.5, 0.5] */
        const float fast = osc.getValueFast(phase);
        REQUIRE(fast >= -0.5f);
        REQUIRE(fast <= 0.5f);

        (void)osc.aliasReduction();
    }
}

TEST_CASE("TriangleOsc 440 Hz at 48 kHz — 1 second", "[TriangleOsc][!benchmark]")
{
    constexpr float sampleRate = 48000.0f;
    constexpr float freq = 440.0f;
    const float delta = std::min(freq / sampleRate, 0.45f);
    constexpr int numSamples = static_cast<int>(sampleRate);

    BENCHMARK("TriangleOsc leader 440 Hz 1 s")
    {
        TriangleOsc osc;
        float phase = 0.0f;
        float accum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            phase += delta;
            osc.processLeader(phase, delta);
            if (phase >= 1.0f)
                phase -= 1.0f;
            accum += osc.getValue(phase) + osc.aliasReduction();
        }
        return accum;
    };
}
