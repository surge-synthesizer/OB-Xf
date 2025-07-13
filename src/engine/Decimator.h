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

#ifndef OBXF_SRC_ENGINE_DECIMATOR_H
#define OBXF_SRC_ENGINE_DECIMATOR_H

// MusicDsp
// T.Rochebois
// still in dev
class Decimator17
{
  private:
    const float h0{0.5f};
    const float h1{0.314356238f};
    const float h3{-0.0947515890f};
    const float h5{0.0463142134f};
    const float h7{-0.0240881704f};
    const float h9{0.0120250406f};
    const float h11{-0.00543170841f};
    const float h13{0.00207426259f};
    const float h15{-0.000572688237f};
    const float h17{5.18944944E-5f};

    float R1{0.f};
    float R2{0.f};
    float R3{0.f};
    float R4{0.f};
    float R5{0.f};
    float R6{0.f};
    float R7{0.f};
    float R8{0.f};
    float R9{0.f};
    float R10{0.f};
    float R11{0.f};
    float R12{0.f};
    float R13{0.f};
    float R14{0.f};
    float R15{0.f};
    float R16{0.f};
    float R17{0.f};

  public:
    Decimator17() {}

    void resetDecimator()
    {
        R1 = R2 = R3 = R4 = R5 = R6 = R7 = R8 = R9 = R10 = R11 = R12 = R13 = R14 = R15 = R16 = R17 =
            0.f;
    }
    float decimate(const float x0, const float x1)
    {
        float h17x0 = h17 * x0;
        float h15x0 = h15 * x0;
        float h13x0 = h13 * x0;
        float h11x0 = h11 * x0;
        float h9x0 = h9 * x0;
        float h7x0 = h7 * x0;
        float h5x0 = h5 * x0;
        float h3x0 = h3 * x0;
        float h1x0 = h1 * x0;
        float R18 = R17 + h17x0;

        R17 = R16 + h15x0;
        R16 = R15 + h13x0;
        R15 = R14 + h11x0;
        R14 = R13 + h9x0;
        R13 = R12 + h7x0;
        R12 = R11 + h5x0;
        R11 = R10 + h3x0;
        R10 = R9 + h1x0;
        R9 = R8 + h1x0 + h0 * x1;
        R8 = R7 + h3x0;
        R7 = R6 + h5x0;
        R6 = R5 + h7x0;
        R5 = R4 + h9x0;
        R4 = R3 + h11x0;
        R3 = R2 + h13x0;
        R2 = R1 + h15x0;
        R1 = h17x0;

        return R18;
    }
};

class Decimator9
{
  private:
    const float h0{8192.f / 16384.0f};
    const float h1{5042.f / 16384.f};
    const float h3{-1277.f / 16384.f};
    const float h5{429.f / 16384.f};
    const float h7{-116.f / 16384.f};
    const float h9{18.f / 16384.f};

    float R1{0.f};
    float R2{0.f};
    float R3{0.f};
    float R4{0.f};
    float R5{0.f};
    float R6{0.f};
    float R7{0.f};
    float R8{0.f};
    float R9{0.f};

  public:
    Decimator9() {}

    inline float decimate(const float x0, const float x1)
    {
        float h9x0 = h9 * x0;
        float h7x0 = h7 * x0;
        float h5x0 = h5 * x0;
        float h3x0 = h3 * x0;
        float h1x0 = h1 * x0;
        float R10 = R9 + h9x0;

        R9 = R8 + h7x0;
        R8 = R7 + h5x0;
        R7 = R6 + h3x0;
        R6 = R5 + h1x0;
        R5 = R4 + h1x0 + h0 * x1;
        R4 = R3 + h3x0;
        R3 = R2 + h5x0;
        R2 = R1 + h7x0;
        R1 = h9x0;

        return R10;
    }
};

#endif // OBXF_SRC_ENGINE_DECIMATOR_H
