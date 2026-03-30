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

#include "Noise.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch2.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>

/* ==========================================================================
 * Golden value infrastructure
 *
 * Run with OBXF_PRINT_GOLDEN=1 to regenerate expected arrays:
 *   OBXF_PRINT_GOLDEN=1 ./obxf-tests "[Noise][golden]"
 * ========================================================================== */

static constexpr int NOISE_GOLDEN_WARMUP = 100;
static constexpr int NOISE_GOLDEN_RECORD = 50;
static constexpr float NOISE_GOLDEN_SR = 48000.f;
static constexpr float NOISE_GOLDEN_TOL = 1e-5f;

static constexpr int32_t NOISE_SEED = 0x8675309;

static bool noiseGoldenPrintMode() { return std::getenv("OBXF_PRINT_GOLDEN") != nullptr; }

static void noiseGoldenCheckOrPrint(const char *label,
                                    const std::array<float, NOISE_GOLDEN_RECORD> &got,
                                    const std::array<float, NOISE_GOLDEN_RECORD> &expected)
{
    if (noiseGoldenPrintMode())
    {
        std::printf("// %s\n        {", label);
        for (int i = 0; i < NOISE_GOLDEN_RECORD; ++i)
        {
            if (i > 0 && i % 5 == 0)
                std::printf("\n         ");
            std::printf("%.9ff%s", got[i], i + 1 < NOISE_GOLDEN_RECORD ? ", " : "");
        }
        std::printf("};\n");
    }
    else
    {
        for (int i = 0; i < NOISE_GOLDEN_RECORD; ++i)
        {
            INFO(label << " sample[" << i << "]: got=" << got[i] << " expected=" << expected[i]);
            REQUIRE(got[i] == Approx(expected[i]).margin(NOISE_GOLDEN_TOL));
        }
    }
}

/* ==========================================================================
 * White noise golden test
 * ========================================================================== */

TEST_CASE("Noise white golden", "[Noise][golden]")
{
    Noise noise;
    noise.setSampleRate(NOISE_GOLDEN_SR);
    noise.seedWhiteNoise(NOISE_SEED);

    for (int i = 0; i < NOISE_GOLDEN_WARMUP; ++i)
        noise.getWhite();

    std::array<float, NOISE_GOLDEN_RECORD> got;
    for (int i = 0; i < NOISE_GOLDEN_RECORD; ++i)
        got[i] = noise.getWhite();

    static const std::array<float, NOISE_GOLDEN_RECORD> expected{
        0.303954214f,  -0.221340865f, -0.097927570f, 0.496008158f,  -0.099525258f, -0.400122881f,
        -0.027912056f, -0.428391129f, -0.378314167f, -0.352463752f, 0.486940116f,  0.056966770f,
        -0.019545054f, 0.199787766f,  0.221705377f,  -0.208931446f, 0.394272268f,  -0.186405912f,
        0.452840209f,  0.150847644f,  0.076933913f,  -0.172535107f, 0.221237034f,  0.322136551f,
        -0.153830156f, -0.322761446f, -0.389440477f, -0.181803823f, -0.059556946f, 0.092654914f,
        -0.256132215f, -0.421010524f, 0.153628528f,  -0.317378253f, 0.460325807f,  0.069946676f,
        -0.010949546f, 0.411969692f,  -0.497138917f, -0.245351583f, 0.446732998f,  -0.347155839f,
        -0.083277740f, 0.184927672f,  0.450490564f,  0.484687805f,  -0.244263217f, -0.304845661f,
        0.483369470f,  -0.331798255f};

    noiseGoldenCheckOrPrint("Noise white golden", got, expected);
}

/* ==========================================================================
 * Pink noise golden test
 * ========================================================================== */

TEST_CASE("Noise pink golden", "[Noise][golden]")
{
    Noise noise;
    noise.setSampleRate(NOISE_GOLDEN_SR);
    noise.seedWhiteNoise(NOISE_SEED);

    for (int i = 0; i < NOISE_GOLDEN_WARMUP; ++i)
        noise.getPink();

    std::array<float, NOISE_GOLDEN_RECORD> got;
    for (int i = 0; i < NOISE_GOLDEN_RECORD; ++i)
        got[i] = noise.getPink();

    static const std::array<float, NOISE_GOLDEN_RECORD> expected{
        0.013826685f,  0.008938941f,  0.031299982f,  -0.041910097f, -0.080919787f, 0.106543198f,
        0.286496073f,  0.206158131f,  0.006697882f,  -0.044845432f, -0.062933795f, 0.052067511f,
        0.157203794f,  0.020210885f,  -0.009778717f, -0.110478245f, 0.098732658f,  0.162257165f,
        0.013851036f,  0.016856574f,  -0.016683871f, -0.062336523f, 0.076446578f,  0.062874973f,
        0.042831760f,  0.069915056f,  0.130322248f,  0.181180671f,  0.104359031f,  0.053036433f,
        -0.018482903f, -0.162121922f, 0.062538505f,  0.075655058f,  0.034881148f,  -0.081128359f,
        -0.055742472f, 0.082846254f,  -0.004875053f, 0.161072418f,  0.115424976f,  -0.019933170f,
        0.073594116f,  0.039691538f,  0.125651017f,  0.202266917f,  0.105676547f,  0.022884142f,
        0.011186632f,  0.104897514f};

    noiseGoldenCheckOrPrint("Noise pink golden", got, expected);
}

/* ==========================================================================
 * Red noise golden test
 * ========================================================================== */

TEST_CASE("Noise red golden", "[Noise][golden]")
{
    Noise noise;
    noise.setSampleRate(NOISE_GOLDEN_SR);
    noise.seedWhiteNoise(NOISE_SEED);

    for (int i = 0; i < NOISE_GOLDEN_WARMUP; ++i)
        noise.getRed();

    std::array<float, NOISE_GOLDEN_RECORD> got;
    for (int i = 0; i < NOISE_GOLDEN_RECORD; ++i)
        got[i] = noise.getRed();

    static const std::array<float, NOISE_GOLDEN_RECORD> expected{
        -0.175322726f, -0.186389774f, -0.191286147f, -0.166485742f, -0.171461999f, -0.191468149f,
        -0.192863747f, -0.214283302f, -0.233199015f, -0.250822216f, -0.226475209f, -0.223626867f,
        -0.224604115f, -0.214614719f, -0.203529447f, -0.213976026f, -0.194262415f, -0.203582704f,
        -0.180940688f, -0.173398301f, -0.169551611f, -0.178178370f, -0.167116523f, -0.151009694f,
        -0.158701196f, -0.174839273f, -0.194311291f, -0.203401476f, -0.206379324f, -0.201746583f,
        -0.214553192f, -0.235603720f, -0.227922291f, -0.243791208f, -0.220774919f, -0.217277586f,
        -0.217825070f, -0.197226584f, -0.222083524f, -0.234351099f, -0.212014452f, -0.229372248f,
        -0.233536139f, -0.224289760f, -0.201765224f, -0.177530840f, -0.189743996f, -0.204986274f,
        -0.180817798f, -0.197407708f};

    noiseGoldenCheckOrPrint("Noise red golden", got, expected);
}
