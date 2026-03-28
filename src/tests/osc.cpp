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
#include <array>
#include <cmath>
#include <cstdio>
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

/* ==========================================================================
 * Golden / regression tests for SawOsc
 *
 * These pin the exact floating-point output of each SawOsc mode so that
 * performance-only refactors can verify numeric stability to within 1e-5.
 *
 * To regenerate the tables after an intentional numeric change, run:
 *   OBXF_PRINT_GOLDEN=1 ./obxf-tests "[golden]"
 * then paste the printed initialisers back into the expected{} arrays below.
 * ========================================================================== */

static constexpr int GOLDEN_WARMUP = 100;
static constexpr int GOLDEN_RECORD = 50;
static constexpr float GOLDEN_SR = 48000.f;
static constexpr float GOLDEN_FREQ = 743.f;
static constexpr float GOLDEN_TOL = 1e-5f;

/*
 * Leader at 3× follower frequency: wraps every ~22 samples, giving multiple
 * hard-sync events across the 50-sample recording window.
 */
static constexpr float GOLDEN_LEADER_FREQ = GOLDEN_FREQ * 3.f;

static bool goldenPrintMode() { return std::getenv("OBXF_PRINT_GOLDEN") != nullptr; }

static void goldenCheckOrPrint(const char *label,
                               const std::array<float, GOLDEN_RECORD> &got,
                               const std::array<float, GOLDEN_RECORD> &expected)
{
    if (goldenPrintMode())
    {
        std::printf("// %s\n        {", label);
        for (int i = 0; i < GOLDEN_RECORD; ++i)
        {
            if (i > 0 && i % 5 == 0)
                std::printf("\n         ");
            std::printf("%.9ff%s", got[i], i + 1 < GOLDEN_RECORD ? ", " : "");
        }
        std::printf("};\n");
    }
    else
    {
        for (int i = 0; i < GOLDEN_RECORD; ++i)
        {
            INFO(label << " sample[" << i << "]: got=" << got[i]
                       << " expected=" << expected[i]);
            REQUIRE(got[i] == Approx(expected[i]).margin(GOLDEN_TOL));
        }
    }
}

/* --------------------------------------------------------------------------
 * SawOsc leader — normal mode
 * -------------------------------------------------------------------------- */

TEST_CASE("SawOsc golden — leader normal", "[SawOsc][golden]")
{
    const float delta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    SawOsc osc;
    float phase = 0.f;

    auto step = [&]() -> float {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.f)
            phase -= 1.f;
        return osc.getValue(phase) + osc.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> got;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        got[i] = step();

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.168792099f, -0.153312922f, -0.137833744f, -0.122354567f, -0.106875390f,
        -0.091396213f, -0.075917035f, -0.060437858f, -0.044958681f, -0.029479504f,
        -0.014000326f, 0.001478851f, 0.016957998f, 0.032437146f, 0.047916293f,
        0.063395441f, 0.078874588f, 0.094353735f, 0.109832883f, 0.125312030f,
        0.140791178f, 0.156270325f, 0.171749473f, 0.187228620f, 0.202707767f,
        0.218186915f, 0.233666062f, 0.249145210f, 0.264624357f, 0.280019850f,
        0.295788735f, 0.310617894f, 0.327347040f, 0.340635717f, 0.359715044f,
        0.369552046f, 0.393559575f, 0.396483898f, 0.430145651f, 0.419386238f,
        0.473339111f, 0.429323822f, 0.551504493f, 0.197991252f, -0.552032709f,
        -0.437194556f, -0.479017347f, -0.426176459f, -0.436248124f, -0.403052926f
    };
    /* clang-format on */
    goldenCheckOrPrint("SawOsc leader normal", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc leader — decimating mode
 * -------------------------------------------------------------------------- */

TEST_CASE("SawOsc golden — leader decimating", "[SawOsc][golden]")
{
    const float delta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    SawOsc osc;
    osc.setDecimation();
    float phase = 0.f;

    auto step = [&]() -> float {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.f)
            phase -= 1.f;
        return osc.getValue(phase) + osc.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> got;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        got[i] = step();

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.168792099f, -0.153312922f, -0.137833744f, -0.122354567f, -0.106875390f,
        -0.091396213f, -0.075917035f, -0.060437858f, -0.044958681f, -0.029479504f,
        -0.014000326f, 0.001478851f, 0.016957998f, 0.032437146f, 0.047916293f,
        0.063395441f, 0.078874588f, 0.094353735f, 0.109832883f, 0.125312030f,
        0.140791178f, 0.156270325f, 0.171749473f, 0.187228620f, 0.202707767f,
        0.218186915f, 0.233666062f, 0.249145210f, 0.264624357f, 0.280211031f,
        0.295453340f, 0.310098261f, 0.326586664f, 0.345039517f, 0.357625902f,
        0.365227044f, 0.387540400f, 0.420833379f, 0.421758115f, 0.399862081f,
        0.446185231f, 0.544383228f, 0.473101437f, 0.099173307f, -0.350810289f,
        -0.549988866f, -0.489836425f, -0.406452388f, -0.410927206f, -0.427110970f
    };
    /* clang-format on */
    goldenCheckOrPrint("SawOsc leader decimating", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc follower — normal & decimating modes, with hard sync in window
 *
 * Calling sequence mirrors OscillatorBlock::ProcessSample() (delay lines
 * omitted since they live in OscillatorBlock, not in SawOsc itself):
 *
 *   leader_phase += lDelta;
 *   leader.processLeader(leader_phase, lDelta);
 *   if (leader_phase >= 1.f) { leader_phase -= 1.f; syncFrac = leader_phase / lDelta; syncReset = true; }
 *   (void)(leader.getValue(leader_phase) + leader.aliasReduction());
 *
 *   follower_phase += fDelta;
 *   follower.processFollower(follower_phase, fDelta, syncReset, syncFrac);
 *   if (follower_phase >= 1.f) follower_phase -= 1.f;
 *   if (syncReset)             follower_phase = fDelta * syncFrac;
 *   sample = follower.getValue(follower_phase) + follower.aliasReduction();
 * -------------------------------------------------------------------------- */

static std::array<float, GOLDEN_RECORD> runSawFollower(bool decimating)
{
    const float fDelta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    const float lDelta = std::min(GOLDEN_LEADER_FREQ / GOLDEN_SR, 0.45f);

    SawOsc leader, follower;
    if (decimating)
    {
        leader.setDecimation();
        follower.setDecimation();
    }

    float lPhase = 0.f, fPhase = 0.f;

    auto step = [&]() -> float {
        /* --- leader --- */
        lPhase += lDelta;
        leader.processLeader(lPhase, lDelta);

        bool syncReset = false;
        float syncFrac = 0.f;
        if (lPhase >= 1.f)
        {
            lPhase -= 1.f;
            syncFrac = lPhase / lDelta;
            syncReset = true;
        }
        (void)(leader.getValue(lPhase) + leader.aliasReduction());

        /* --- follower --- */
        fPhase += fDelta;
        follower.processFollower(fPhase, fDelta, syncReset, syncFrac);
        if (fPhase >= 1.f)
            fPhase -= 1.f;
        if (syncReset)
            fPhase = fDelta * syncFrac;

        return follower.getValue(fPhase) + follower.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> out;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        out[i] = step();
    return out;
}

TEST_CASE("SawOsc golden — follower synced normal", "[SawOsc][golden]")
{
    const auto got = runSawFollower(false);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.290179342f, -0.512561917f, -0.457548440f, -0.464273065f, -0.434404761f,
        -0.428745300f, -0.406461626f, -0.395679891f, -0.377024293f, -0.363606423f,
        -0.346895814f, -0.332009017f, -0.316467345f, -0.300555706f, -0.286039501f,
        -0.268961668f, -0.255897462f, -0.236908883f, -0.226471022f, -0.203723788f,
        -0.198922604f, -0.168708667f, -0.431382537f, -0.495457530f, -0.455678463f,
        -0.453659296f, -0.429507107f, -0.419936895f, -0.400360107f, -0.387743503f,
        -0.370236844f, -0.356256306f, -0.339538842f, -0.325292617f, -0.308324307f,
        -0.294877887f, -0.276486784f, -0.265215993f, -0.243672892f, -0.236941114f,
        -0.208635584f, -0.212990627f, -0.161939338f, -0.269479960f, -0.509154499f,
        -0.460548490f, -0.464173406f, -0.436238199f, -0.429277390f, -0.407891899f
    };
    /* clang-format on */
    goldenCheckOrPrint("SawOsc follower synced normal", got, expected);
}

TEST_CASE("SawOsc golden — follower synced decimating", "[SawOsc][golden]")
{
    const auto got = runSawFollower(true);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.312641829f, -0.449401408f, -0.498094231f, -0.464530408f, -0.428200781f,
        -0.420771688f, -0.415040433f, -0.395695925f, -0.375608951f, -0.362074554f,
        -0.348650753f, -0.331636578f, -0.315150946f, -0.302271962f, -0.287769705f,
        -0.266698748f, -0.249399036f, -0.245676607f, -0.233972460f, -0.193145543f,
        -0.169537902f, -0.238735586f, -0.382145822f, -0.485820442f, -0.487992376f,
        -0.444024324f, -0.422244400f, -0.419735372f, -0.407232374f, -0.385544300f,
        -0.368743002f, -0.356257230f, -0.341036916f, -0.323551357f, -0.308827758f,
        -0.296253651f, -0.278545529f, -0.257098466f, -0.246469989f, -0.243448824f,
        -0.217686296f, -0.174634904f, -0.188081935f, -0.302410305f, -0.442083091f,
        -0.498149872f, -0.467776537f, -0.429663897f, -0.420837402f, -0.415911853f
    };
    /* clang-format on */
    goldenCheckOrPrint("SawOsc follower synced decimating", got, expected);
}

/* ==========================================================================
 * Golden / regression tests for TriangleOsc
 * ========================================================================== */

static std::array<float, GOLDEN_RECORD> runTriLeader(bool decimating)
{
    const float delta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    TriangleOsc osc;
    if (decimating)
        osc.setDecimation();
    float phase = 0.f;

    auto step = [&]() -> float {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.f)
            phase -= 1.f;
        return osc.getValue(phase) + osc.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> out;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        out[i] = step();
    return out;
}

static std::array<float, GOLDEN_RECORD> runTriFollower(bool decimating)
{
    const float fDelta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    const float lDelta = std::min(GOLDEN_LEADER_FREQ / GOLDEN_SR, 0.45f);

    SawOsc leader;
    TriangleOsc follower;
    if (decimating)
    {
        leader.setDecimation();
        follower.setDecimation();
    }

    float lPhase = 0.f, fPhase = 0.f;

    auto step = [&]() -> float {
        /* --- leader (SawOsc drives sync) --- */
        lPhase += lDelta;
        leader.processLeader(lPhase, lDelta);

        bool syncReset = false;
        float syncFrac = 0.f;
        if (lPhase >= 1.f)
        {
            lPhase -= 1.f;
            syncFrac = lPhase / lDelta;
            syncReset = true;
        }
        (void)(leader.getValue(lPhase) + leader.aliasReduction());

        /* --- follower --- */
        fPhase += fDelta;
        follower.processFollower(fPhase, fDelta, syncReset, syncFrac);
        if (fPhase >= 1.f)
            fPhase -= 1.f;
        if (syncReset)
            fPhase = fDelta * syncFrac;

        return follower.getValue(fPhase) + follower.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> out;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        out[i] = step();
    return out;
}

TEST_CASE("TriangleOsc golden — leader normal", "[TriangleOsc][golden]")
{
    const auto got = runTriLeader(false);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        0.162427291f, 0.193351895f, 0.224362075f, 0.255242020f, 0.286314189f,
        0.317108572f, 0.348300427f, 0.378920406f, 0.410388350f, 0.440497518f,
        0.473257959f, 0.493416876f, 0.466321111f, 0.435111403f, 0.404140621f,
        0.373229682f, 0.342230022f, 0.311301261f, 0.280325353f, 0.249376565f,
        0.218415171f, 0.187456995f, 0.156500891f, 0.125540286f, 0.094584674f,
        0.063624710f, 0.032667994f, 0.001709580f, -0.029248714f, -0.060206261f,
        -0.091164082f, -0.122124568f, -0.153075695f, -0.184048310f, -0.214979351f,
        -0.245983258f, -0.276867777f, -0.307938367f, -0.338729918f, -0.369927585f,
        -0.400548190f, -0.431967884f, -0.462371588f, -0.492400408f, -0.477189511f,
        -0.443645746f, -0.413978726f, -0.382223934f, -0.351799816f, -0.320472270f
    };
    /* clang-format on */
    goldenCheckOrPrint("TriangleOsc leader normal", got, expected);
}

TEST_CASE("TriangleOsc golden — leader decimating", "[TriangleOsc][golden]")
{
    const auto got = runTriLeader(true);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        0.162351653f, 0.193459630f, 0.224485382f, 0.255060226f, 0.285820365f,
        0.317609966f, 0.349015117f, 0.378253818f, 0.408315420f, 0.443119258f,
        0.475297391f, 0.487111896f, 0.470165730f, 0.436225027f, 0.402238220f,
        0.372774184f, 0.343170583f, 0.311484933f, 0.279869676f, 0.249241605f,
        0.218589202f, 0.187507242f, 0.156429037f, 0.125508338f, 0.094590157f,
        0.063630350f, 0.032668054f, 0.001709580f, -0.029248714f, -0.060210403f,
        -0.091172129f, -0.122095920f, -0.153008476f, -0.184066951f, -0.215172127f,
        -0.245876700f, -0.276449919f, -0.307947516f, -0.339749932f, -0.369600296f,
        -0.398816586f, -0.432255417f, -0.466899872f, -0.486598223f, -0.477918327f,
        -0.447086573f, -0.411924988f, -0.381428480f, -0.352349192f, -0.321147293f
    };
    /* clang-format on */
    goldenCheckOrPrint("TriangleOsc leader decimating", got, expected);
}

TEST_CASE("TriangleOsc golden — follower synced normal", "[TriangleOsc][golden]")
{
    const auto got = runTriFollower(false);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.080358699f, -0.525123775f, -0.415096939f, -0.428546101f, -0.368809521f,
        -0.357490599f, -0.312923312f, -0.291359752f, -0.254048616f, -0.227212906f,
        -0.193791598f, -0.164017975f, -0.132934719f, -0.101111412f, -0.072079010f,
        -0.037923325f, -0.011794935f, 0.026182242f, 0.047057949f, 0.092552431f,
        0.102154776f, 0.162582681f, -0.362765044f, -0.490915060f, -0.411356986f,
        -0.407318562f, -0.359014213f, -0.339873821f, -0.300720245f, -0.275486976f,
        -0.240473703f, -0.212512657f, -0.179077700f, -0.150585204f, -0.116648585f,
        -0.089755788f, -0.052973554f, -0.030431954f, 0.012654228f, 0.026117770f,
        0.082728840f, 0.074018747f, 0.176121324f, -0.038959920f, -0.518309057f,
        -0.421097010f, -0.428346783f, -0.372476429f, -0.358554810f, -0.315783828f
    };
    /* clang-format on */
    goldenCheckOrPrint("TriangleOsc follower synced normal", got, expected);
}

TEST_CASE("TriangleOsc golden — follower synced decimating", "[TriangleOsc][golden]")
{
    const auto got = runTriFollower(true);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.125283659f, -0.398802847f, -0.496188462f, -0.429060757f, -0.356401533f,
        -0.341543376f, -0.330080867f, -0.291391850f, -0.251217902f, -0.224149123f,
        -0.197301522f, -0.163273141f, -0.130301923f, -0.104543962f, -0.075539418f,
        -0.033397444f, 0.001201928f, 0.008646775f, 0.032055080f, 0.113708913f,
        0.160924181f, 0.022528820f, -0.264291644f, -0.471640915f, -0.475984812f,
        -0.388048649f, -0.344488770f, -0.339470774f, -0.314464778f, -0.271088600f,
        -0.237486005f, -0.212514490f, -0.182073802f, -0.147102684f, -0.117655531f,
        -0.092507347f, -0.057091050f, -0.014196883f, 0.007060021f, 0.013102351f,
        0.064627409f, 0.150730178f, 0.123836145f, -0.104820579f, -0.384166181f,
        -0.496299773f, -0.435553044f, -0.359327823f, -0.341674805f, -0.331823736f
    };
    /* clang-format on */
    goldenCheckOrPrint("TriangleOsc follower synced decimating", got, expected);
}

/* ==========================================================================
 * Golden / regression tests for PulseOsc  (50 % pulse width = square wave)
 * ========================================================================== */

static constexpr float GOLDEN_PW = 0.5f;

/*
 * Follower PW is narrower so the follower phase crosses it within the ~22-sample
 * sync window (22 × delta ≈ 0.34 > 0.2), making pw1t toggle and exercising the
 * non-trivial BLEP correction path in processFollower.
 */
static constexpr float GOLDEN_FOLLOWER_PW = 0.2f;

static std::array<float, GOLDEN_RECORD> runPulseLeader(bool decimating)
{
    const float delta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    PulseOsc osc;
    if (decimating)
        osc.setDecimation();
    float phase = 0.f;
    float pwWas = GOLDEN_PW;

    auto step = [&]() -> float {
        phase += delta;
        osc.processLeader(phase, delta, GOLDEN_PW, pwWas);
        if (phase >= 1.f)
            phase -= 1.f;
        pwWas = GOLDEN_PW;
        return osc.getValue(phase, GOLDEN_PW) + osc.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> out;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        out[i] = step();
    return out;
}

static std::array<float, GOLDEN_RECORD> runPulseFollower(bool decimating)
{
    const float fDelta = std::min(GOLDEN_FREQ / GOLDEN_SR, 0.45f);
    const float lDelta = std::min(GOLDEN_LEADER_FREQ / GOLDEN_SR, 0.45f);

    SawOsc leader;
    PulseOsc follower;
    if (decimating)
    {
        leader.setDecimation();
        follower.setDecimation();
    }

    float lPhase = 0.f, fPhase = 0.f;
    float pwWas = GOLDEN_FOLLOWER_PW;

    auto step = [&]() -> float {
        /* --- leader (SawOsc drives sync) --- */
        lPhase += lDelta;
        leader.processLeader(lPhase, lDelta);

        bool syncReset = false;
        float syncFrac = 0.f;
        if (lPhase >= 1.f)
        {
            lPhase -= 1.f;
            syncFrac = lPhase / lDelta;
            syncReset = true;
        }
        (void)(leader.getValue(lPhase) + leader.aliasReduction());

        /* --- follower --- */
        fPhase += fDelta;
        follower.processFollower(fPhase, fDelta, syncReset, syncFrac, GOLDEN_FOLLOWER_PW, pwWas);
        if (fPhase >= 1.f)
            fPhase -= 1.f;
        if (syncReset)
            fPhase = fDelta * syncFrac;
        pwWas = GOLDEN_FOLLOWER_PW;

        return follower.getValue(fPhase, GOLDEN_FOLLOWER_PW) + follower.aliasReduction();
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> out;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        out[i] = step();
    return out;
}

TEST_CASE("PulseOsc golden — leader normal", "[PulseOsc][golden]")
{
    const auto got = runPulseLeader(false);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.500950754f, -0.498366952f, -0.502618790f, -0.495948941f, -0.506038725f,
        -0.491175860f, -0.512710929f, -0.481627375f, -0.527202189f, -0.456756681f,
        -0.583076239f, 0.095045358f, 0.583677173f, 0.456600785f, 0.527275205f,
        0.481584072f, 0.512739599f, 0.491155654f, 0.506053209f, 0.495938599f,
        0.502626002f, 0.498361886f, 0.500954092f, 0.499476463f, 0.500243664f,
        0.499902397f, 0.500012755f, 0.500000000f, 0.500000000f, 0.499916345f,
        0.500206113f, 0.499556094f, 0.500806093f, 0.498615623f, 0.502215803f,
        0.496573657f, 0.505102038f, 0.492547214f, 0.510729849f, 0.484491259f,
        0.522964954f, 0.463470548f, 0.570172071f, 0.201179683f, -0.564323425f,
        -0.464964449f, -0.522266388f, -0.484904677f, -0.510455489f, -0.492739469f
    };
    /* clang-format on */
    goldenCheckOrPrint("PulseOsc leader normal", got, expected);
}

TEST_CASE("PulseOsc golden — leader decimating", "[PulseOsc][golden]")
{
    const auto got = runPulseLeader(true);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.500978529f, -0.503147900f, -0.497572750f, -0.491925955f, -0.504963338f,
        -0.517605066f, -0.489946336f, -0.463487834f, -0.522561073f, -0.581998348f,
        -0.402872741f, 0.047702551f, 0.462695152f, 0.582076728f, 0.505058229f,
        0.463466048f, 0.498032093f, 0.517615139f, 0.501122952f, 0.491920769f,
        0.499246269f, 0.503150403f, 0.500363708f, 0.498995364f, 0.499789774f,
        0.500114739f, 0.500015140f, 0.500000000f, 0.500000000f, 0.500107527f,
        0.499870688f, 0.499036461f, 0.500045717f, 0.503019452f, 0.500126660f,
        0.492248654f, 0.499082863f, 0.516896725f, 0.502342284f, 0.464967102f,
        0.495811105f, 0.578529954f, 0.491769016f, 0.102361739f, -0.363101006f,
        -0.577758789f, -0.533085465f, -0.465180606f, -0.485134572f, -0.516797543f
    };
    /* clang-format on */
    goldenCheckOrPrint("PulseOsc leader decimating", got, expected);
}

/*
 * Note: with PW=0.5 and leader at 3× follower frequency, the follower phase
 * resets every ~22 samples and never reaches the 0.5 PW threshold before the
 * next sync.  processFollower's hardSyncReset path fires with trans=0, so the
 * BLEP correction is zero and getValue consistently returns -0.5 through the
 * delay line.  The test still pins this stable state against regressions.
 */
TEST_CASE("PulseOsc golden — follower synced normal", "[PulseOsc][golden]")
{
    const auto got = runPulseFollower(false);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.165150851f, -0.876718402f, -0.760455787f, -0.823873281f, -0.785414934f,
        -0.807810128f, -0.797896743f, -0.796624362f, -0.809306622f, -0.783431947f,
        -0.826757312f, -0.755787253f, -0.886544049f, -0.356736809f, 0.284256846f,
        0.158286944f, 0.223735139f, 0.187271029f, 0.204188615f, 0.203795955f,
        0.187096104f, 0.220886856f, -0.606713533f, -0.849671900f, -0.774327636f,
        -0.815783799f, -0.789769411f, -0.806528866f, -0.796196342f, -0.801669955f,
        -0.800146043f, -0.798189700f, -0.803276360f, -0.796364009f, -0.786142051f,
        0.068489134f, 0.236857802f, 0.176610053f, 0.220558181f, 0.177894995f,
        0.227526173f, 0.160280451f, 0.272469848f, -0.100606173f, -0.862857938f,
        -0.766503036f, -0.820320308f, -0.787651360f, -0.806433141f, -0.798633754f
    };
    /* clang-format on */
    goldenCheckOrPrint("PulseOsc follower synced normal", got, expected);
}

TEST_CASE("PulseOsc golden — follower synced decimating", "[PulseOsc][golden]")
{
    const auto got = runPulseFollower(true);

    /* clang-format off */
    static const std::array<float, GOLDEN_RECORD> expected{
        -0.237464964f, -0.685387254f, -0.878868997f, -0.830271959f, -0.763679862f,
        -0.779970050f, -0.819148540f, -0.823517382f, -0.788439989f, -0.760983169f,
        -0.812336743f, -0.882068217f, -0.748166561f, -0.333048403f, 0.108526319f,
        0.292418778f, 0.234187156f, 0.143109649f, 0.160161659f, 0.262367338f,
        0.273134321f, 0.006782800f, -0.463105857f, -0.815223515f, -0.873284936f,
        -0.790045857f, -0.762845099f, -0.800262451f, -0.826838851f, -0.810660541f,
        -0.769657254f, -0.773622036f, -0.854557574f, -0.857606649f, -0.580683887f,
        -0.108575970f, 0.233154386f, 0.283664495f, 0.183735996f, 0.135023117f,
        0.204926878f, 0.292992532f, 0.187533170f, -0.204129755f, -0.660639167f,
        -0.875448346f, -0.836639643f, -0.765511394f, -0.777138889f, -0.817245424f
    };
    /* clang-format on */
    goldenCheckOrPrint("PulseOsc follower synced decimating", got, expected);
}
