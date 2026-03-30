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
 * Include SynthEngine.h first so the include chain resolves correctly:
 *   SynthEngine → Voice → AdsrEnvelope (first encounter, defined here)
 * Also pulls in Constants.h which defines the 'dc' global used in
 * ADSREnvelope::processSample().
 */
#include "SynthEngine.h"
#include "AdsrEnvelope.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch2.hpp>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

/* ==========================================================================
 * Helpers
 * ========================================================================== */

struct EnvConfig
{
    float attackMs{50.f};
    float decayMs{100.f};
    float sustain{0.7f};
    float releaseMs{200.f};
    float sampleRate{48000.f};
    float attackCurve{0.f}; /* 0 == exp, 1 == lin */
    float offsetFactor{1.f};
};

static ADSREnvelope makeEnv(const EnvConfig &cfg)
{
    ADSREnvelope env;
    env.setSampleRate(cfg.sampleRate);
    env.setEnvOffsets(cfg.offsetFactor);
    env.setAttackCurve(cfg.attackCurve);
    env.setAttack(cfg.attackMs);
    env.setDecay(cfg.decayMs);
    env.setSustain(cfg.sustain);
    env.setRelease(cfg.releaseMs);
    env.ResetEnvelopeState();
    return env;
}

/* ==========================================================================
 * Suite 1: Boundedness tests
 * ========================================================================== */

TEST_CASE("ADSREnvelope output stays in [0, 1] — fast attack, medium sustain",
          "[ADSREnvelope][bounds]")
{
    EnvConfig cfg;
    cfg.attackMs = 10.f;
    cfg.decayMs = 50.f;
    cfg.sustain = 0.5f;
    cfg.releaseMs = 80.f;
    cfg.sampleRate = 48000.f;

    auto env = makeEnv(cfg);
    env.triggerAttack();

    /* Run attack + decay + a sustain hold */
    const int holdSamples = static_cast<int>(cfg.sampleRate * 0.5f);
    for (int i = 0; i < holdSamples; ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (gated) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }

    env.triggerRelease();

    /* Run until silent, with a generous timeout */
    const int relTimeout = static_cast<int>(cfg.sampleRate * 2.f);
    for (int i = 0; i < relTimeout && env.isActive(); ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (release) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }
}

TEST_CASE("ADSREnvelope output stays in [0, 1] — slow attack, high sustain",
          "[ADSREnvelope][bounds]")
{
    EnvConfig cfg;
    cfg.attackMs = 500.f;
    cfg.decayMs = 200.f;
    cfg.sustain = 0.9f;
    cfg.releaseMs = 300.f;
    cfg.sampleRate = 48000.f;

    auto env = makeEnv(cfg);
    env.triggerAttack();

    const int holdSamples = static_cast<int>(cfg.sampleRate * 3.f);
    for (int i = 0; i < holdSamples; ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (gated) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }

    env.triggerRelease();

    const int relTimeout = static_cast<int>(cfg.sampleRate * 3.f);
    for (int i = 0; i < relTimeout && env.isActive(); ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (release) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }
}

TEST_CASE("ADSREnvelope output stays in [0, 1] — very fast attack, low sustain",
          "[ADSREnvelope][bounds]")
{
    EnvConfig cfg;
    cfg.attackMs = 1.f;
    cfg.decayMs = 20.f;
    cfg.sustain = 0.1f;
    cfg.releaseMs = 50.f;
    cfg.sampleRate = 48000.f;

    auto env = makeEnv(cfg);
    env.triggerAttack();

    const int holdSamples = static_cast<int>(cfg.sampleRate * 0.2f);
    for (int i = 0; i < holdSamples; ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (gated) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }

    env.triggerRelease();

    const int relTimeout = static_cast<int>(cfg.sampleRate * 1.f);
    for (int i = 0; i < relTimeout && env.isActive(); ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (release) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }
}

TEST_CASE("ADSREnvelope output stays in [0, 1] — linear attack curve", "[ADSREnvelope][bounds]")
{
    EnvConfig cfg;
    cfg.attackMs = 100.f;
    cfg.decayMs = 80.f;
    cfg.sustain = 0.6f;
    cfg.releaseMs = 150.f;
    cfg.sampleRate = 48000.f;
    cfg.attackCurve = 1.f; /* linear */

    auto env = makeEnv(cfg);
    env.triggerAttack();

    const int holdSamples = static_cast<int>(cfg.sampleRate * 1.f);
    for (int i = 0; i < holdSamples; ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (gated, lin) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }

    env.triggerRelease();

    const int relTimeout = static_cast<int>(cfg.sampleRate * 2.f);
    for (int i = 0; i < relTimeout && env.isActive(); ++i)
    {
        const float v = env.processSample();
        INFO("sample " << i << " (release, lin) = " << v);
        REQUIRE(v >= 0.0f);
        REQUIRE(v <= 1.0f);
    }
}

/* ==========================================================================
 * Suite 2: Stage timing tests
 * ========================================================================== */

/*
 * Attack: par.a = attackMs / atkTimeAdjustment = attackMs * 3.
 * The attack ends when output > 1 - atkValueEnd = 0.9.
 * We check that the output stops rising within 2x the adjusted attack time.
 *
 * Decay: we check that output reaches near-sustain within 2x decayMs.
 *
 * Release: we check that output drops below 0.001 within 2x releaseMs.
 */

TEST_CASE("ADSREnvelope attack completes within 2x expected duration", "[ADSREnvelope][timing]")
{
    const float attackMs = 50.f;
    const float sr = 48000.f;

    EnvConfig cfg;
    cfg.attackMs = attackMs;
    cfg.decayMs = 500.f;
    cfg.sustain = 0.8f;
    cfg.releaseMs = 200.f;
    cfg.sampleRate = sr;

    auto env = makeEnv(cfg);
    env.triggerAttack();

    /*
     * The attack phase ends when output > 1 - atkValueEnd (0.9).
     * par.a = attackMs / atkTimeAdjustment = attackMs * 3
     * So the actual RC time used internally is 3x the knob value.
     * We allow 2x that as our timing budget.
     */
    const float adjustedAttackMs = attackMs / ADSREnvelope::atkTimeAdjustment;
    const int maxAttackSamples =
        static_cast<int>(sr * adjustedAttackMs * ADSREnvelope::msToSec * 2.f);

    float prevOutput = 0.f;
    int attackCompletedAt = -1;
    for (int i = 0; i < maxAttackSamples * 4; ++i)
    {
        const float v = env.processSample();
        /* Attack is done when the output stops rising */
        if (attackCompletedAt < 0 && v < prevOutput && i > 0)
        {
            attackCompletedAt = i;
            break;
        }
        prevOutput = v;
    }

    INFO("Attack completed at sample " << attackCompletedAt
                                       << ", max allowed = " << maxAttackSamples);
    REQUIRE(attackCompletedAt > 0);
    REQUIRE(attackCompletedAt <= maxAttackSamples);
}

TEST_CASE("ADSREnvelope decay reaches near-sustain within 2x decay time", "[ADSREnvelope][timing]")
{
    const float decayMs = 100.f;
    const float sustainLevel = 0.7f;
    const float sr = 48000.f;

    EnvConfig cfg;
    cfg.attackMs = 1.f; /* very fast attack so decay starts quickly */
    cfg.decayMs = decayMs;
    cfg.sustain = sustainLevel;
    cfg.releaseMs = 500.f;
    cfg.sampleRate = sr;

    auto env = makeEnv(cfg);
    env.triggerAttack();

    /* Burn through the (very short) attack */
    const int attackBudget = static_cast<int>(sr * 0.1f);
    for (int i = 0; i < attackBudget; ++i)
        env.processSample();

    /* Now measure how long decay takes to reach within 5% of sustain */
    const float targetLevel = sustainLevel * 1.05f;
    const int maxDecaySamples = static_cast<int>(sr * decayMs * ADSREnvelope::msToSec * 2.f);

    int decayDoneSample = -1;
    for (int i = 0; i < maxDecaySamples; ++i)
    {
        const float v = env.processSample();
        if (v <= targetLevel)
        {
            decayDoneSample = i;
            break;
        }
    }

    INFO("Decay reached near-sustain at sample " << decayDoneSample
                                                 << ", max allowed = " << maxDecaySamples);
    REQUIRE(decayDoneSample >= 0);
    REQUIRE(decayDoneSample <= maxDecaySamples);
}

TEST_CASE("ADSREnvelope release reaches near-silent within 2x release time",
          "[ADSREnvelope][timing]")
{
    const float releaseMs = 200.f;
    const float sr = 48000.f;

    EnvConfig cfg;
    cfg.attackMs = 1.f;
    cfg.decayMs = 50.f;
    cfg.sustain = 0.7f;
    cfg.releaseMs = releaseMs;
    cfg.sampleRate = sr;

    auto env = makeEnv(cfg);
    env.triggerAttack();

    /* Run attack + decay + a bit of sustain */
    const int holdSamples = static_cast<int>(sr * 0.5f);
    for (int i = 0; i < holdSamples; ++i)
        env.processSample();

    env.triggerRelease();

    /* Measure how long until output drops below 0.001 */
    const int maxRelSamples = static_cast<int>(sr * releaseMs * ADSREnvelope::msToSec * 2.f);
    const float silenceThreshold = 0.001f;

    int silenceSample = -1;
    for (int i = 0; i < maxRelSamples; ++i)
    {
        const float v = env.processSample();
        if (v < silenceThreshold)
        {
            silenceSample = i;
            break;
        }
    }

    INFO("Release went near-silent at sample " << silenceSample
                                               << ", max allowed = " << maxRelSamples);
    REQUIRE(silenceSample >= 0);
    REQUIRE(silenceSample <= maxRelSamples);
}

/* ==========================================================================
 * Suite 3: Golden value tests
 *
 * These pin the exact floating-point output of ADSREnvelope so that
 * performance-only refactors can verify numeric stability to within 1e-5.
 *
 * To regenerate after an intentional numeric change:
 *   OBXF_PRINT_GOLDEN=1 ./src/tests/obxf-tests_artefacts/Debug/obxf-tests "[ADSREnvelope][golden]"
 * then paste the printed arrays back into the expected{} initialisers below.
 * ========================================================================== */

static constexpr float ENV_GOLDEN_SR = 48000.f;
/* ENV_GOLDEN_WARMUP is 0: envelope starts from silence, no warmup needed */
static constexpr int ENV_GOLDEN_RECORD = 50;
static constexpr float ENV_GOLDEN_TOL = 1e-5f;

static bool envGoldenPrintMode() { return std::getenv("OBXF_PRINT_GOLDEN") != nullptr; }

static void envGoldenCheckOrPrint(const char *label,
                                  const std::array<float, ENV_GOLDEN_RECORD> &got,
                                  const std::array<float, ENV_GOLDEN_RECORD> &expected)
{
    if (envGoldenPrintMode())
    {
        std::printf("// %s\n        {", label);
        for (int i = 0; i < ENV_GOLDEN_RECORD; ++i)
        {
            if (i > 0 && i % 5 == 0)
                std::printf("\n         ");
            std::printf("%.9ff%s", got[i], i + 1 < ENV_GOLDEN_RECORD ? ", " : "");
        }
        std::printf("};\n");
    }
    else
    {
        for (int i = 0; i < ENV_GOLDEN_RECORD; ++i)
        {
            INFO(label << " sample[" << i << "]: got=" << got[i] << " expected=" << expected[i]);
            REQUIRE(got[i] == Approx(expected[i]).margin(ENV_GOLDEN_TOL));
        }
    }
}

/* Standard ADSR parameters used for all golden tests */
static EnvConfig goldenCfg()
{
    EnvConfig cfg;
    cfg.attackMs = 50.f;
    cfg.decayMs = 100.f;
    cfg.sustain = 0.7f;
    cfg.releaseMs = 200.f;
    cfg.sampleRate = ENV_GOLDEN_SR;
    cfg.attackCurve = 0.f;
    cfg.offsetFactor = 1.f;
    return cfg;
}

/* --------------------------------------------------------------------------
 * Golden test 1: attack phase — first 50 samples from silence
 * -------------------------------------------------------------------------- */

TEST_CASE("ADSREnvelope golden — attack phase", "[ADSREnvelope][golden]")
{
    auto env = makeEnv(goldenCfg());
    env.triggerAttack();

    /* No warmup — record the first ENV_GOLDEN_RECORD samples of the attack */
    std::array<float, ENV_GOLDEN_RECORD> got;
    for (int i = 0; i < ENV_GOLDEN_RECORD; ++i)
        got[i] = env.processSample();

    /* clang-format off */
    static const std::array<float, ENV_GOLDEN_RECORD> expected{
        0.000995850f, 0.001990708f, 0.002984575f, 0.003977453f, 0.004969342f,
        0.005960243f, 0.006950158f, 0.007939086f, 0.008927030f, 0.009913989f,
        0.010899967f, 0.011884961f, 0.012868975f, 0.013852010f, 0.014834065f,
        0.015815143f, 0.016795242f, 0.017774366f, 0.018752515f, 0.019729691f,
        0.020705892f, 0.021681122f, 0.022655381f, 0.023628669f, 0.024600988f,
        0.025572339f, 0.026542723f, 0.027512141f, 0.028480593f, 0.029448081f,
        0.030414606f, 0.031380165f, 0.032344766f, 0.033308405f, 0.034271084f,
        0.035232805f, 0.036193568f, 0.037153374f, 0.038112227f, 0.039070122f,
        0.040027063f, 0.040983051f, 0.041938089f, 0.042892173f, 0.043845307f,
        0.044797495f, 0.045748733f, 0.046699025f, 0.047648370f, 0.048596770f};
    /* clang-format on */

    envGoldenCheckOrPrint("ADSREnvelope attack phase", got, expected);
}

/* --------------------------------------------------------------------------
 * Golden test 2: release phase — 50 samples after attack+decay complete,
 *                sustain held briefly, then release triggered
 * -------------------------------------------------------------------------- */

TEST_CASE("ADSREnvelope golden — release phase", "[ADSREnvelope][golden]")
{
    auto env = makeEnv(goldenCfg());
    env.triggerAttack();

    /*
     * Run enough samples for attack + decay to complete and sustain to
     * stabilise, then trigger release.
     * attack: ~50ms * 3 (atkTimeAdjustment) ≈ 150ms → ~7200 samples
     * decay:  ~100ms                               → ~4800 samples
     * extra sustain hold:                            ~4800 samples
     */
    const int preReleaseSamples = static_cast<int>(ENV_GOLDEN_SR * 0.5f); /* 500ms */
    for (int i = 0; i < preReleaseSamples; ++i)
        env.processSample();

    env.triggerRelease();

    /* Record ENV_GOLDEN_RECORD samples of the release tail */
    std::array<float, ENV_GOLDEN_RECORD> got;
    for (int i = 0; i < ENV_GOLDEN_RECORD; ++i)
        got[i] = env.processSample();

    /* clang-format off */
    static const std::array<float, ENV_GOLDEN_RECORD> expected{
        0.699186504f, 0.698373973f, 0.697562397f, 0.696751714f, 0.695941985f,
        0.695133209f, 0.694325387f, 0.693518519f, 0.692712545f, 0.691907525f,
        0.691103458f, 0.690300286f, 0.689498067f, 0.688696802f, 0.687896430f,
        0.687097013f, 0.686298549f, 0.685500979f, 0.684704363f, 0.683908641f,
        0.683113873f, 0.682319999f, 0.681527078f, 0.680735052f, 0.679943979f,
        0.679153800f, 0.678364515f, 0.677576184f, 0.676788747f, 0.676002264f,
        0.675216675f, 0.674431980f, 0.673648179f, 0.672865331f, 0.672083378f,
        0.671302319f, 0.670522153f, 0.669742942f, 0.668964624f, 0.668187201f,
        0.667410672f, 0.666635036f, 0.665860295f, 0.665086508f, 0.664313614f,
        0.663541615f, 0.662770510f, 0.662000299f, 0.661230981f, 0.660462558f};
    /* clang-format on */

    envGoldenCheckOrPrint("ADSREnvelope release phase", got, expected);
}
