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

#ifndef OBXF_SRC_ENGINE_NOISE_H
#define OBXF_SRC_ENGINE_NOISE_H

#include <array>
#include <math.h>
#include <cstdint>

class Noise
{
  public:
    void setSampleRate(float sr, uint8_t numPinkNoiseGenerators = 10u)
    {
        white.volComp = (4.6567E-10f * 0.5f) * sqrt(sr / 44100.f);
        setPinkNoiseGen(numPinkNoiseGenerators);
    };

    void seedWhiteNoise(int32_t seed = 0) { white.state = seed; };

    void setPinkNoiseGen(uint8_t numGenerators = 10u)
    {
        pink.index = 0;
        pink.indexMask = (1 << numGenerators) - 1;

        // calculate maximum possible signed random value
        int32_t pmax = (numGenerators + 1) * (1 << (randomBits - 1));
        pink.scale = 1.0f / pmax;

        // initialize rows
        for (uint8_t i = 0; i < numGenerators; i++)
        {
            pink.rows[i] = 0;
        }

        pink.runningSum = 0;
    };

    inline int32_t getRandomValue()
    {
        // we're using unsigned arithmetic here to avoid overflow UB
        return white.state = int32_t(uint32_t(white.state) * 1103515245u + 12345u);
    };

    inline float getWhiteNoiseSample() { return getRandomValue() * white.volComp; };

    // Adapted from Phil Burk's copyleft code:
    // https://www.firstpr.com.au/dsp/pink-noise/phil_burk_19990905_patest_pink.c
    inline float getPinkNoiseSample()
    {
        // increment and mask index
        pink.index = (pink.index + 1) & pink.indexMask;

        // if index is zero, don't update any random values
        if (pink.index != 0)
        {
            int numZeros = 0;
            int n = pink.index;

            // determine how many trailing zeroes there are in index
            // this algorithm will hang if n == 0, so test that first
            while ((n & 1) == 0)
            {
                n = n >> 1;
                numZeros++;
            }

            const int32_t randomValue = getRandomValue() >> randomShift;

            // replace the indexed row's random value
            // subtract and add back to running sum instead of adding all the random values together
            // only one changes each time
            pink.runningSum -= pink.rows[numZeros];
            pink.runningSum += randomValue;
            pink.rows[numZeros] = randomValue;
        }

        // add extra white noise value
        const int32_t randomValue = getRandomValue() >> randomShift;

        return pink.scale * (pink.runningSum + randomValue);
    };

    inline float getRedNoiseSample()
    {
        red.state += getWhiteNoiseSample() * 0.05f;

        if (red.state > 1.f)
            red.state = 2.f - red.state;
        else if (red.state < -1.f)
            red.state = -2.f - red.state;

        return red.state;
    };

  private:
    static constexpr uint8_t maxRandomRows = 30;
    static constexpr uint8_t randomBits = 24;
    static constexpr uint8_t randomShift = (sizeof(int32_t) * 8) - randomBits;

    struct WhiteNoise
    {
        int32_t state{0};
        // compensates volume at higher sample rates, because noise bandwidth becomes wider
        // (=louder) with increased Nyquist frequency
        float volComp{1.f};
    } white;

    struct PinkNoise
    {
        std::array<int32_t, maxRandomRows> rows;
        int32_t runningSum; // used to optimize summing of generators
        int32_t index;      // incremented each sample
        int32_t indexMask;  // index wrapped by ANDing with this mask
        float scale;        // used to scale within [-1.f, 1.f]
    } pink;

    struct RedNoise
    {
        float state;
    } red;
};

#endif // OBXF_SRC_ENGINE_NOISE_H
