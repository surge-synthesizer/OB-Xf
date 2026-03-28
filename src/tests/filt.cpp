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
 * Include SynthEngine.h first so that the include chain resolves correctly:
 *   SynthEngine → Voice → Filter (first encounter, defined here)
 * Also pulls in AudioUtils.h (tpt_process_scaled_cutoff) and Utils.h (pi).
 */
#include "SynthEngine.h"
#include "AudioUtils.h"
#include "Filter.h"
#include "SawOsc.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch2.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>

/* ==========================================================================
 * Shared noise source
 *
 * A simple LCG produces a deterministic, flat-spectrum excitation signal.
 * ========================================================================== */

static float lcgSample(uint32_t &rng)
{
    rng = rng * 1664525u + 1013904223u;
    return static_cast<float>(static_cast<int32_t>(rng)) / 2147483648.f;
}

/* ==========================================================================
 * 2-pole filter runner
 * ========================================================================== */

struct TwoPoleConfig
{
    float cutoffHz{1000.f};
    float resonance{0.5f};
    float multimode{0.f};
    bool bpBlend{false};
    bool push{false};
};

static std::vector<float> run2Pole(float sampleRate, const TwoPoleConfig &cfg, int numSamples)
{
    Filter filt;
    filt.setSampleRate(sampleRate);
    filt.setResonance(cfg.resonance);
    filt.setMultimode(cfg.multimode);
    filt.par.bpBlend2Pole = cfg.bpBlend;
    filt.par.push2Pole = cfg.push;

    uint32_t rng = 0x12345678u;
    std::vector<float> output(numSamples);
    for (int i = 0; i < numSamples; ++i)
        output[i] = filt.apply2Pole(lcgSample(rng), cfg.cutoffHz);
    return output;
}

/* ==========================================================================
 * 4-pole filter runner
 * ========================================================================== */

struct FourPoleConfig
{
    float cutoffHz{1000.f};
    float resonance{0.5f};
    float multimode{0.f};
    bool xpander{false};
    uint8_t xpanderMode{0};
};

static std::vector<float> run4Pole(float sampleRate, const FourPoleConfig &cfg, int numSamples)
{
    Filter filt;
    filt.setSampleRate(sampleRate);
    filt.setResonance(cfg.resonance);
    filt.setMultimode(cfg.multimode);
    filt.par.xpander4Pole = cfg.xpander;
    filt.par.xpanderMode = cfg.xpanderMode;

    uint32_t rng = 0x87654321u;
    std::vector<float> output(numSamples);
    for (int i = 0; i < numSamples; ++i)
        output[i] = filt.apply4Pole(lcgSample(rng), cfg.cutoffHz);
    return output;
}

/* ==========================================================================
 * Bounds helpers
 * ========================================================================== */

static void checkFiniteAndBounded(const std::vector<float> &samples, float bound = 20.f)
{
    for (int i = 0; i < static_cast<int>(samples.size()); ++i)
    {
        INFO("sample[" << i << "] = " << samples[i]);
        REQUIRE(std::isfinite(samples[i]));
        REQUIRE(samples[i] >= -bound);
        REQUIRE(samples[i] <= bound);
    }
}

/* ==========================================================================
 * 2-pole bounds tests
 * ========================================================================== */

TEST_CASE("Filter 2-pole LP output is finite and bounded", "[Filter][2pole]")
{
    /* Low-pass, no resonance boost, flat-spectrum input */
    const auto samples = run2Pole(48000.f, {1000.f, 0.0f, 0.f, false, false}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 2-pole LP with high resonance is finite", "[Filter][2pole]")
{
    /* Near self-oscillation — output may be large, but must remain finite */
    const auto samples = run2Pole(48000.f, {1000.f, 0.9f, 0.f, false, false}, 4800);
    checkFiniteAndBounded(samples, 100.f);
}

TEST_CASE("Filter 2-pole BP blend output is finite and bounded", "[Filter][2pole]")
{
    /* BP blend at midpoint (multimode=0.5 → blend between LP and BP) */
    const auto samples = run2Pole(48000.f, {800.f, 0.4f, 0.5f, true, false}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 2-pole push mode output is finite and bounded", "[Filter][2pole]")
{
    const auto samples = run2Pole(48000.f, {1200.f, 0.3f, 0.f, false, true}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 2-pole HP (multimode=1) output is finite and bounded", "[Filter][2pole]")
{
    /* multimode=1.0 routes fully to the HP output in standard mode */
    const auto samples = run2Pole(48000.f, {1000.f, 0.4f, 1.f, false, false}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 2-pole reset clears state", "[Filter][2pole]")
{
    Filter filt;
    filt.setSampleRate(48000.f);
    filt.setResonance(0.5f);
    filt.setMultimode(0.f);

    /* Drive the filter into a non-zero state */
    uint32_t rng = 0xDEADBEEFu;
    for (int i = 0; i < 100; ++i)
        filt.apply2Pole(lcgSample(rng), 1000.f);

    filt.reset();

    /* After reset, feeding zero should yield zero (no stored energy) */
    const float out = filt.apply2Pole(0.f, 1000.f);
    REQUIRE(out == Approx(0.f).margin(1e-6f));
}

/* ==========================================================================
 * 4-pole bounds tests
 * ========================================================================== */

TEST_CASE("Filter 4-pole LP4 output is finite and bounded", "[Filter][4pole]")
{
    const auto samples = run4Pole(48000.f, {1000.f, 0.0f, 0.f, false, 0}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 4-pole LP4 with high resonance is finite", "[Filter][4pole]")
{
    const auto samples = run4Pole(48000.f, {1000.f, 0.9f, 0.f, false, 0}, 4800);
    checkFiniteAndBounded(samples, 100.f);
}

TEST_CASE("Filter 4-pole multimode sweep output is finite and bounded", "[Filter][4pole]")
{
    /* multimode=0.5 → midpoint between LP4 and LP3 poles */
    const auto samples = run4Pole(48000.f, {800.f, 0.4f, 0.5f, false, 0}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 4-pole xpander LP4 mode output is finite and bounded", "[Filter][4pole]")
{
    /* xpanderMode=0 → LP4 */
    const auto samples = run4Pole(48000.f, {1000.f, 0.4f, 0.f, true, 0}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 4-pole xpander HP2 mode output is finite and bounded", "[Filter][4pole]")
{
    /* xpanderMode=5 → HP2 */
    const auto samples = run4Pole(48000.f, {1000.f, 0.4f, 0.f, true, 5}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 4-pole xpander BP4 mode output is finite and bounded", "[Filter][4pole]")
{
    /* xpanderMode=7 → BP4 */
    const auto samples = run4Pole(48000.f, {1000.f, 0.3f, 0.f, true, 7}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 4-pole xpander notch (N2) mode output is finite and bounded", "[Filter][4pole]")
{
    /* xpanderMode=9 → N2 */
    const auto samples = run4Pole(48000.f, {1000.f, 0.3f, 0.f, true, 9}, 4800);
    checkFiniteAndBounded(samples);
}

TEST_CASE("Filter 4-pole reset clears state", "[Filter][4pole]")
{
    Filter filt;
    filt.setSampleRate(48000.f);
    filt.setResonance(0.5f);
    filt.setMultimode(0.f);

    uint32_t rng = 0xCAFEBABEu;
    for (int i = 0; i < 100; ++i)
        filt.apply4Pole(lcgSample(rng), 1000.f);

    filt.reset();

    const float out = filt.apply4Pole(0.f, 1000.f);
    REQUIRE(out == Approx(0.f).margin(1e-6f));
}

/* ==========================================================================
 * Golden / regression tests
 *
 * These pin the exact floating-point output of each filter configuration so
 * that performance-only refactors can verify numeric stability to within 1e-5.
 *
 * To regenerate after an intentional numeric change:
 *   OBXF_PRINT_GOLDEN=1 ./obxf-tests "[golden]"
 * then paste the printed arrays back into the expected{} initialisers below.
 * ========================================================================== */

static constexpr int FILT_GOLDEN_WARMUP = 100;
static constexpr int FILT_GOLDEN_RECORD = 50;
static constexpr float FILT_GOLDEN_SR = 48000.f;
static constexpr float FILT_GOLDEN_CUTOFF = 1200.f;
static constexpr float FILT_GOLDEN_TOL = 1e-5f;

static bool filtGoldenPrintMode() { return std::getenv("OBXF_PRINT_GOLDEN") != nullptr; }

static void filtGoldenCheckOrPrint(const char *label,
                                   const std::array<float, FILT_GOLDEN_RECORD> &got,
                                   const std::array<float, FILT_GOLDEN_RECORD> &expected)
{
    if (filtGoldenPrintMode())
    {
        std::printf("// %s\n        {", label);
        for (int i = 0; i < FILT_GOLDEN_RECORD; ++i)
        {
            if (i > 0 && i % 5 == 0)
                std::printf("\n         ");
            std::printf("%.9ff%s", got[i], i + 1 < FILT_GOLDEN_RECORD ? ", " : "");
        }
        std::printf("};\n");
    }
    else
    {
        for (int i = 0; i < FILT_GOLDEN_RECORD; ++i)
        {
            INFO(label << " sample[" << i << "]: got=" << got[i] << " expected=" << expected[i]);
            REQUIRE(got[i] == Approx(expected[i]).margin(FILT_GOLDEN_TOL));
        }
    }
}

/* --------------------------------------------------------------------------
 * Golden runner helpers
 * -------------------------------------------------------------------------- */

static std::array<float, FILT_GOLDEN_RECORD> golden2Pole(const TwoPoleConfig &cfg)
{
    Filter filt;
    filt.setSampleRate(FILT_GOLDEN_SR);
    filt.setResonance(cfg.resonance);
    filt.setMultimode(cfg.multimode);
    filt.par.bpBlend2Pole = cfg.bpBlend;
    filt.par.push2Pole = cfg.push;

    uint32_t rng = 0x12345678u;
    auto step = [&]() -> float { return filt.apply2Pole(lcgSample(rng), cfg.cutoffHz); };

    for (int i = 0; i < FILT_GOLDEN_WARMUP; ++i)
        step();

    std::array<float, FILT_GOLDEN_RECORD> got;
    for (int i = 0; i < FILT_GOLDEN_RECORD; ++i)
        got[i] = step();
    return got;
}

static std::array<float, FILT_GOLDEN_RECORD> golden4Pole(const FourPoleConfig &cfg)
{
    Filter filt;
    filt.setSampleRate(FILT_GOLDEN_SR);
    filt.setResonance(cfg.resonance);
    filt.setMultimode(cfg.multimode);
    filt.par.xpander4Pole = cfg.xpander;
    filt.par.xpanderMode = cfg.xpanderMode;

    uint32_t rng = 0x87654321u;
    auto step = [&]() -> float { return filt.apply4Pole(lcgSample(rng), cfg.cutoffHz); };

    for (int i = 0; i < FILT_GOLDEN_WARMUP; ++i)
        step();

    std::array<float, FILT_GOLDEN_RECORD> got;
    for (int i = 0; i < FILT_GOLDEN_RECORD; ++i)
        got[i] = step();
    return got;
}

/* --------------------------------------------------------------------------
 * 2-pole golden: LP (multimode=0)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole LP", "[Filter][golden]")
{
    const auto got = golden2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, false, false});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.051525276f, 0.005595362f, -0.043107074f, -0.082160167f, -0.109034821f,
        -0.124730922f, -0.134663641f, -0.149091974f, -0.165740475f, -0.170837730f,
        -0.160444096f, -0.146284789f, -0.140820578f, -0.140169010f, -0.128359318f,
        -0.098538093f, -0.062001936f, -0.038407188f, -0.028176486f, -0.014056245f,
        0.013102971f, 0.047938880f, 0.084015377f, 0.119411275f, 0.145658642f,
        0.157091185f, 0.160091504f, 0.168078065f, 0.185626030f, 0.208521396f,
        0.233890906f, 0.257073939f, 0.275764406f, 0.290888190f, 0.307852298f,
        0.330116093f, 0.348891169f, 0.357251972f, 0.360415280f, 0.358302534f,
        0.350992113f, 0.342828810f, 0.323034942f, 0.285421282f, 0.242119059f,
        0.203147799f, 0.174847752f, 0.162514701f, 0.158990026f, 0.152640283f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole LP", got, expected);
}

/* --------------------------------------------------------------------------
 * 2-pole golden: HP (multimode=1)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole HP", "[Filter][golden]")
{
    const auto got = golden2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 1.f, false, false});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        -0.522519350f, -0.365700424f, 0.806302130f, 0.310959905f, 0.537958920f,
        0.417873204f, -0.443221539f, -0.257235616f, 0.599250972f, 0.923655033f,
        0.054405581f, -0.424508840f, -0.609189034f, 0.865898192f, 0.678846002f,
        0.684326828f, -0.963389874f, -0.846906722f, 0.499609858f, 0.475644052f,
        0.654213667f, -0.544687212f, 0.635449827f, -0.836093307f, -0.440272927f,
        -0.675177932f, 0.429266363f, 0.621661723f, -0.128923655f, 0.499511719f,
        -0.470655322f, 0.088796452f, -0.432251364f, 0.199876398f, 0.329614460f,
        -0.003485337f, -0.885887027f, 0.093900055f, -0.141037896f, -0.663628638f,
        0.629141688f, -0.732351184f, -1.042165399f, -0.060282584f, 0.244326159f,
        0.270854801f, 0.936805725f, 0.433367342f, -0.381448299f, -0.126572102f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole HP", got, expected);
}

/* --------------------------------------------------------------------------
 * 2-pole golden: BP blend
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole BP blend", "[Filter][golden]")
{
    const auto got = golden2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.5f, true, false});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        -0.256845206f, -0.326749623f, -0.292073488f, -0.204143062f, -0.137331709f,
        -0.062106084f, -0.064101040f, -0.119228214f, -0.092311017f, 0.027544294f,
        0.104519337f, 0.075391576f, -0.005962217f, 0.014241233f, 0.135815248f,
        0.243099287f, 0.221136555f, 0.078663118f, 0.051330261f, 0.128084406f,
        0.217006132f, 0.225626051f, 0.232769236f, 0.216978252f, 0.116526037f,
        0.028738141f, 0.009384478f, 0.092094317f, 0.130873650f, 0.160039559f,
        0.162310600f, 0.132257655f, 0.105227172f, 0.086938865f, 0.128610700f,
        0.154277623f, 0.084282495f, 0.021951765f, 0.018241936f, -0.045086697f,
        -0.047800880f, -0.055923644f, -0.195581138f, -0.282345682f, -0.267861158f,
        -0.227315530f, -0.132270575f, -0.024435608f, -0.020349490f, -0.060331564f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole BP blend", got, expected);
}

/* --------------------------------------------------------------------------
 * 2-pole golden: push mode
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole push", "[Filter][golden]")
{
    const auto got = golden2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, false, true});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.065901190f, 0.018503925f, -0.032247249f, -0.073769793f, -0.103309371f,
        -0.121658675f, -0.134086728f, -0.150810018f, -0.169561967f, -0.176501155f,
        -0.167542711f, -0.154335499f, -0.149409115f, -0.148994014f, -0.137122557f,
        -0.106834427f, -0.069365956f, -0.044490002f, -0.032840043f, -0.017261980f,
        0.011441155f, 0.047965314f, 0.085861295f, 0.123151116f, 0.151263177f,
        0.164370418f, 0.168724582f, 0.177739188f, 0.196084186f, 0.219634622f,
        0.245561361f, 0.269205749f, 0.288230091f, 0.303528845f, 0.320517778f,
        0.342712522f, 0.361348271f, 0.369441241f, 0.372152090f, 0.369384229f,
        0.361205012f, 0.351977229f, 0.330910385f, 0.291722059f, 0.246500134f,
        0.205347866f, 0.174767584f, 0.160257146f, 0.154820889f, 0.146863952f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole push", got, expected);
}

/* --------------------------------------------------------------------------
 * 4-pole golden: LP4 (xpander mode 0)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole LP4", "[Filter][golden]")
{
    const auto got = golden4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, true, 0});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.233919173f, 0.206081569f, 0.175936356f, 0.144565374f, 0.113034457f,
        0.082272299f, 0.053131297f, 0.026350224f, 0.002306603f, -0.019036386f,
        -0.037966084f, -0.054909274f, -0.070289105f, -0.084306255f, -0.096778162f,
        -0.107198462f, -0.115112968f, -0.120434448f, -0.123325594f, -0.123966090f,
        -0.122556858f, -0.119417146f, -0.115006492f, -0.109931357f, -0.104879014f,
        -0.100380883f, -0.096670039f, -0.093814746f, -0.091842294f, -0.090652786f,
        -0.089917041f, -0.089182176f, -0.088107273f, -0.086647332f, -0.085117601f,
        -0.084070273f, -0.083959252f, -0.084832318f, -0.086289853f, -0.087658606f,
        -0.088312618f, -0.088001482f, -0.086872920f, -0.085162021f, -0.082892321f,
        -0.079824746f, -0.075553834f, -0.069631152f, -0.061694853f, -0.051559616f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole LP4", got, expected);
}

/* --------------------------------------------------------------------------
 * 4-pole golden: HP2 (xpander mode 5)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole HP2", "[Filter][golden]")
{
    const auto got = golden4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, true, 5});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.477763236f, 0.712693632f, 0.992122531f, -0.520238042f, 0.400516212f,
        1.321195960f, -0.796962738f, -1.225629568f, 0.628523648f, -0.994840562f,
        -0.248699769f, 0.646734476f, 0.147059873f, 1.805916429f, 0.412714928f,
        -1.405308127f, -0.327479601f, 0.499718577f, -0.014908395f, -0.504451811f,
        -0.229044184f, -0.410877526f, -0.706634462f, -0.693840921f, 1.889320016f,
        -1.083149195f, 0.807372570f, -1.014859319f, 1.787305355f, -0.141942456f,
        0.362791061f, -0.572458267f, -0.787882745f, -0.680358708f, -0.932320178f,
        1.806174994f, 0.017749183f, 1.490020037f, 0.181979910f, -0.344105363f,
        -1.545138836f, 0.233718485f, 0.133641869f, 1.254431248f, -0.347120821f,
        1.227347493f, -0.455026627f, -0.002801929f, 0.300301462f, -1.676498771f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole HP2", got, expected);
}

/* --------------------------------------------------------------------------
 * 4-pole golden: BP4 (xpander mode 7)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole BP4", "[Filter][golden]")
{
    const auto got = golden4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, true, 7});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        -0.266336113f, -0.189819098f, -0.099137396f, -0.007704493f, 0.062905006f,
        0.130127221f, 0.200302824f, 0.231275901f, 0.221055076f, 0.198635921f,
        0.160913944f, 0.120970123f, 0.101945959f, 0.115140885f, 0.166726306f,
        0.213858142f, 0.214669928f, 0.194077313f, 0.181918621f, 0.168808058f,
        0.142312080f, 0.105332546f, 0.057404839f, -0.005585493f, -0.053597327f,
        -0.066168293f, -0.068282619f, -0.073515855f, -0.069753103f, -0.039782509f,
        0.002794831f, 0.033912532f, 0.039174788f, 0.012065456f, -0.040766545f,
        -0.086302184f, -0.088959105f, -0.053533044f, 0.007301040f, 0.067595705f,
        0.088299163f, 0.067446306f, 0.040748313f, 0.039094467f, 0.061492838f,
        0.095552512f, 0.135956004f, 0.165881231f, 0.182470769f, 0.179201722f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole BP4", got, expected);
}

/* --------------------------------------------------------------------------
 * 4-pole golden: multimode (non-xpander, pole blend)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole multimode 0.5", "[Filter][golden]")
{
    const auto got = golden4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.5f, false, 0});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        -0.084485069f, -0.120118588f, -0.144649401f, -0.159468651f, -0.170088917f,
        -0.172651529f, -0.164744571f, -0.158307880f, -0.158207297f, -0.160382286f,
        -0.167519584f, -0.177810177f, -0.184788138f, -0.182692721f, -0.165630698f,
        -0.141803578f, -0.124220699f, -0.110563509f, -0.094300665f, -0.077516690f,
        -0.064367205f, -0.055854827f, -0.053820301f, -0.061434042f, -0.071878001f,
        -0.077591918f, -0.082345799f, -0.089168653f, -0.094712161f, -0.092495486f,
        -0.083298661f, -0.072617665f, -0.065913141f, -0.068206012f, -0.081578381f,
        -0.099415354f, -0.110313594f, -0.110740922f, -0.099718846f, -0.081593074f,
        -0.067869313f, -0.063578315f, -0.062737934f, -0.056727663f, -0.042921096f,
        -0.022068797f, 0.005968080f, 0.037188396f, 0.069834009f, 0.100500152f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole multimode 0.5", got, expected);
}

/* ==========================================================================
 * Golden / regression tests — SawOsc input
 *
 * Same configurations as the LCG-noise golden tests above, but driven by a
 * 432 Hz SawOsc leader at 48 kHz.  This exercises the filter on a realistic
 * harmonic signal rather than flat-spectrum noise.
 *
 * Calling sequence mirrors OscillatorBlock::ProcessSample() exactly.
 * ========================================================================== */

static constexpr float FILT_GOLDEN_OSC_FREQ = 432.f;

static std::array<float, FILT_GOLDEN_RECORD> goldenSaw2Pole(const TwoPoleConfig &cfg)
{
    SawOsc osc;
    Filter filt;
    filt.setSampleRate(FILT_GOLDEN_SR);
    filt.setResonance(cfg.resonance);
    filt.setMultimode(cfg.multimode);
    filt.par.bpBlend2Pole = cfg.bpBlend;
    filt.par.push2Pole = cfg.push;

    const float delta = FILT_GOLDEN_OSC_FREQ / FILT_GOLDEN_SR;
    float phase = 0.f;

    auto step = [&]() -> float {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.f)
            phase -= 1.f;
        const float in = osc.getValue(phase) + osc.aliasReduction();
        return filt.apply2Pole(in, cfg.cutoffHz);
    };

    for (int i = 0; i < FILT_GOLDEN_WARMUP; ++i)
        step();

    std::array<float, FILT_GOLDEN_RECORD> got;
    for (int i = 0; i < FILT_GOLDEN_RECORD; ++i)
        got[i] = step();
    return got;
}

static std::array<float, FILT_GOLDEN_RECORD> goldenSaw4Pole(const FourPoleConfig &cfg)
{
    SawOsc osc;
    Filter filt;
    filt.setSampleRate(FILT_GOLDEN_SR);
    filt.setResonance(cfg.resonance);
    filt.setMultimode(cfg.multimode);
    filt.par.xpander4Pole = cfg.xpander;
    filt.par.xpanderMode = cfg.xpanderMode;

    const float delta = FILT_GOLDEN_OSC_FREQ / FILT_GOLDEN_SR;
    float phase = 0.f;

    auto step = [&]() -> float {
        phase += delta;
        osc.processLeader(phase, delta);
        if (phase >= 1.f)
            phase -= 1.f;
        const float in = osc.getValue(phase) + osc.aliasReduction();
        return filt.apply4Pole(in, cfg.cutoffHz);
    };

    for (int i = 0; i < FILT_GOLDEN_WARMUP; ++i)
        step();

    std::array<float, FILT_GOLDEN_RECORD> got;
    for (int i = 0; i < FILT_GOLDEN_RECORD; ++i)
        got[i] = step();
    return got;
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 2-pole golden: LP (multimode=0)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole LP, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, false, false});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.216958910f, 0.226035967f, 0.235096291f, 0.244140923f, 0.253171116f,
        0.262188286f, 0.271193951f, 0.280189663f, 0.289176941f, 0.298157305f,
        0.307132214f, 0.316102475f, 0.325070262f, 0.334035635f, 0.343001336f,
        0.351965845f, 0.360933274f, 0.369899720f, 0.378871739f, 0.387841523f,
        0.396820784f, 0.405792445f, 0.414786130f, 0.423740089f, 0.432837337f,
        0.440390408f, 0.438032180f, 0.417573601f, 0.379378766f, 0.327263653f,
        0.264577091f, 0.194510147f, 0.119917482f, 0.043371920f, -0.032898434f,
        -0.106979810f, -0.177294254f, -0.242570221f, -0.301840276f, -0.354410201f,
        -0.399842173f, -0.437923372f, -0.468641430f, -0.492154092f, -0.508761585f,
        -0.518879056f, -0.523010314f, -0.521723509f, -0.515628576f, -0.505357087f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole LP SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 2-pole golden: HP (multimode=1)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole HP, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 1.f, false, false});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        -0.000710532f, -0.000678173f, -0.000635047f, -0.000583679f, -0.000526420f,
        -0.000465438f, -0.000402698f, -0.000339876f, -0.000278455f, -0.000219534f,
        -0.000164238f, -0.000201761f, 0.000168963f, -0.000523645f, 0.000932854f,
        -0.001535638f, 0.002609383f, -0.003841980f, 0.005972960f, -0.008462490f,
        0.012483469f, -0.017735589f, 0.026541268f, -0.041757051f, 0.080106378f,
        -0.367763311f, -0.944735587f, -0.665022492f, -0.588692248f, -0.404990196f,
        -0.308062047f, -0.170430079f, -0.081744745f, 0.018628635f, 0.088919550f,
        0.156937733f, 0.205365762f, 0.245781749f, 0.272713989f, 0.290509075f,
        0.298670650f, 0.298915207f, 0.292258590f, 0.279861867f, 0.262840539f,
        0.242256522f, 0.219100595f, 0.194279850f, 0.168608278f, 0.142801031f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole HP SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 2-pole golden: BP blend
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole BP blend, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.5f, true, false});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.057722095f, 0.057612803f, 0.057509452f, 0.057413537f, 0.057326172f,
        0.057248112f, 0.057179786f, 0.057121344f, 0.057072680f, 0.057033487f,
        0.057003282f, 0.056974478f, 0.056971900f, 0.056943987f, 0.056976192f,
        0.056928754f, 0.057013262f, 0.056916256f, 0.057083968f, 0.056888040f,
        0.057204500f, 0.056791149f, 0.057484172f, 0.056286667f, 0.059304826f,
        0.036665734f, -0.066630177f, -0.193320885f, -0.291990370f, -0.370194882f,
        -0.426313341f, -0.463971496f, -0.483818084f, -0.488785446f, -0.480321229f,
        -0.460971832f, -0.432457924f, -0.396951854f, -0.356145352f, -0.311818719f,
        -0.265449256f, -0.218418226f, -0.171891838f, -0.126864970f, -0.084153362f,
        -0.044401359f, -0.008091765f, 0.024441984f, 0.053001899f, 0.077510349f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole BP blend SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 2-pole golden: push mode
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 2-pole push, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw2Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, false, true});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.221375734f, 0.230459273f, 0.239516243f, 0.248548940f, 0.257559925f,
        0.266551852f, 0.275527507f, 0.284489691f, 0.293441087f, 0.302384257f,
        0.311321616f, 0.320254862f, 0.329186916f, 0.338118464f, 0.347052753f,
        0.355988622f, 0.364930481f, 0.373874605f, 0.382827640f, 0.391781807f,
        0.400748760f, 0.409711212f, 0.418698668f, 0.427649081f, 0.436745673f,
        0.444293410f, 0.441867471f, 0.421145856f, 0.382342160f, 0.329171330f,
        0.264941603f, 0.192853406f, 0.115811348f, 0.036469005f, -0.042843100f,
        -0.120095760f, -0.193590298f, -0.261935890f, -0.324052930f, -0.379146874f,
        -0.426694691f, -0.466415912f, -0.498249561f, -0.522324085f, -0.538929701f,
        -0.548489511f, -0.551531732f, -0.548663378f, -0.540545404f, -0.527870059f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 2-pole push SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 4-pole golden: LP4 (xpander mode 0)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole LP4, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, true, 0});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.100028597f, 0.108910926f, 0.117970936f, 0.127144977f, 0.136371031f,
        0.145589873f, 0.154745609f, 0.163786709f, 0.172666401f, 0.181343332f,
        0.189781815f, 0.197952241f, 0.205831096f, 0.213401079f, 0.220651045f,
        0.227575839f, 0.234176010f, 0.240457416f, 0.246431068f, 0.252112389f,
        0.257520765f, 0.262679130f, 0.267613262f, 0.272351056f, 0.276922822f,
        0.281348169f, 0.285537541f, 0.289102018f, 0.291265219f, 0.291006655f,
        0.287292033f, 0.279235870f, 0.266186476f, 0.247759640f, 0.223839253f,
        0.194557518f, 0.160263896f, 0.121488437f, 0.078903444f, 0.033286154f,
        -0.014516427f, -0.063619450f, -0.113128692f, -0.162165299f, -0.209887102f,
        -0.255506217f, -0.298303574f, -0.337640405f, -0.372966588f, -0.403826803f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole LP4 SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 4-pole golden: HP2 (xpander mode 5)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole HP2, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, true, 5});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        -0.024027020f, -0.024743989f, -0.025007125f, -0.024837801f, -0.024263479f,
        -0.023316789f, -0.022034621f, -0.020457566f, -0.018628590f, -0.016592655f,
        -0.014395385f, -0.012230266f, -0.009288206f, -0.008155677f, -0.003280096f,
        -0.005317949f, 0.004163766f, -0.005086750f, 0.014139495f, -0.009511095f,
        0.029002845f, -0.022873702f, 0.056279048f, -0.062782265f, 0.150768712f,
        -0.613452971f, -1.494987488f, -0.852576911f, -0.660159707f, -0.327034563f,
        -0.187599331f, 0.004065919f, 0.096335068f, 0.207335666f, 0.266838998f,
        0.329099506f, 0.364719689f, 0.395790249f, 0.412379175f, 0.421921700f,
        0.422510147f, 0.416084677f, 0.403134614f, 0.384398639f, 0.360574663f,
        0.332361400f, 0.300465047f, 0.265599936f, 0.228483230f, 0.189828783f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole HP2 SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 4-pole golden: BP4 (xpander mode 7)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole BP4, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.f, true, 7});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.019547714f, 0.014329036f, 0.009168025f, 0.004147464f, -0.000656839f,
        -0.005176913f, -0.009353623f, -0.013136736f, -0.016485551f, -0.019368773f,
        -0.021764692f, -0.023662683f, -0.025056481f, -0.025954667f, -0.026365260f,
        -0.026316889f, -0.025825458f, -0.024939710f, -0.023676533f, -0.022105552f,
        -0.020232679f, -0.018163396f, -0.015846772f, -0.013533663f, -0.010691141f,
        -0.012379799f, -0.040721171f, -0.107967630f, -0.195819452f, -0.282348543f,
        -0.355443060f, -0.408624589f, -0.439598799f, -0.448525578f, -0.437198848f,
        -0.408226401f, -0.364659637f, -0.309622258f, -0.246171713f, -0.177149758f,
        -0.105148345f, -0.032461602f, 0.038907804f, 0.107251815f, 0.171141922f,
        0.229414523f, 0.281154037f, 0.325674266f, 0.362503201f, 0.391363770f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole BP4 SawOsc", got, expected);
}

/* --------------------------------------------------------------------------
 * SawOsc-driven 4-pole golden: multimode 0.5 (non-xpander pole blend)
 * -------------------------------------------------------------------------- */

TEST_CASE("Filter golden — 4-pole multimode 0.5, SawOsc input", "[Filter][golden][saw]")
{
    const auto got = goldenSaw4Pole({FILT_GOLDEN_CUTOFF, 0.5f, 0.5f, false, 0});

    /* clang-format off */
    static const std::array<float, FILT_GOLDEN_RECORD> expected{
        0.188561246f, 0.198138535f, 0.207295254f, 0.216000095f, 0.224231139f,
        0.231975570f, 0.239229456f, 0.245997250f, 0.252291292f, 0.258131057f,
        0.263542593f, 0.268557101f, 0.273211807f, 0.277546227f, 0.281605333f,
        0.285432547f, 0.289077759f, 0.292584151f, 0.296003997f, 0.299375653f,
        0.302753180f, 0.306162506f, 0.309668273f, 0.313250154f, 0.317102641f,
        0.319744051f, 0.313713789f, 0.291690081f, 0.253958851f, 0.203843623f,
        0.144208863f, 0.077757232f, 0.006896834f, -0.066184364f, -0.139554128f,
        -0.211495057f, -0.280517608f, -0.345334083f, -0.404862881f, -0.458211809f,
        -0.504676998f, -0.543730974f, -0.575017333f, -0.598340750f, -0.613657475f,
        -0.621065021f, -0.620790601f, -0.613178432f, -0.598676920f, -0.577824056f
    };
    /* clang-format on */
    filtGoldenCheckOrPrint("Filter 4-pole multimode 0.5 SawOsc", got, expected);
}