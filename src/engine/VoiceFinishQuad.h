/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_ENGINE_VOICEFINISHQUAD_H
#define OBXF_SRC_ENGINE_VOICEFINISHQUAD_H

#include <algorithm>
#include <cmath>

#include "sst/basic-blocks/dsp/FastMath.h"

#include "GroupRows.h"
#include "SimdMath.h"
#include "Voice.h"

/*
 * Voice::processFinish for up to four voices at once, one voice per lane: tone
 * shaping, the filter, LFO volume and the amp envelope. With exactTrig the
 * results match the scalar stage bit for bit; without it tan and atan are vector
 * approximations, a few parts in a million out. The voices must share their
 * filter topology, see compatible().
 *
 * The one-pole stages keep their state in double in the scalar code. Every sum
 * there is of two floats, which double holds exactly, so float arithmetic with a
 * single rounding gives the same values.
 */
struct VoiceFinishQuad
{
    static constexpr int lanes{4};

    // libm tan and atan per lane match the scalar filter bit for bit, at a cost
    static constexpr bool exactTrig{false};

    static bool compatible(Voice *const v[lanes], int laneCount)
    {
        const auto &f = v[0]->filter.par;

        for (int l = 1; l < laneCount; ++l)
        {
            const auto &g = v[l]->filter.par;

            if (v[l]->par.filter.fourPole != v[0]->par.filter.fourPole ||
                g.xpander4Pole != f.xpander4Pole || g.xpanderMode != f.xpanderMode ||
                g.bpBlend2Pole != f.bpBlend2Pole)
            {
                return false;
            }
        }

        return true;
    }

    using Row = float[Voice::maxBlockSubSamples][lanes];

    /*
     * Voice::setFilterEngineValues as each lane takes the engine's values, a row per
     * sample. Every lane shares them, bar a voice starting this block, which keeps its
     * own through the first output sample, and a voice with resonance modulation,
     * which applies its own curve.
     */
    static void fillFilterRows(Voice *const v[lanes], int laneCount, int longest,
                               const float *resonance, const float *resonanceUnmodulated,
                               const float *multimode, int subPerSample, Row &res2, Row &res4,
                               Row &mm, Row &mmPole, Row &mmXfade)
    {
        using namespace obxf_simd;

        constexpr auto resScale = matrixTargetScaling(MatrixTarget::FilterResonance);

        alignas(16) float res2Was[lanes]{}, res4Was[lanes]{}, mmWas[lanes]{}, poleWas[lanes]{},
            xfadeWas[lanes]{}, sounding[lanes]{}, resMod[lanes]{};
        bool anyMod = false;

        for (int l = 0; l < laneCount; ++l)
        {
            const auto &voice = *v[l];
            const auto &f = voice.filter;

            res2Was[l] = f.state.res2Pole;
            res4Was[l] = f.state.res4Pole;
            mmWas[l] = f.par.multimode;
            poleWas[l] = static_cast<float>(f.state.multimodePole);
            xfadeWas[l] = f.state.multimodeXfade;
            sounding[l] = voice.soundingAtBlockStart ? allBits() : 0.f;
            resMod[l] = voice.matrixAdjustments.modFor(MatrixTarget::FilterResonance);
            anyMod = anyMod || resMod[l] != 0.f;
        }

        const auto soundingV = SIMD_MM(load_ps)(sounding);
        const auto one = SIMD_MM(set1_ps)(1.f), three = SIMD_MM(set1_ps)(3.f);

        for (int s = 0; s < longest; ++s)
        {
            const int t = s / subPerSample;
            auto res = SIMD_MM(set1_ps)(resonanceUnmodulated[t]);

            if (anyMod)
            {
                alignas(16) float lane[lanes];
                SIMD_MM(store_ps)(lane, res);

                for (int l = 0; l < laneCount; ++l)
                {
                    if (resMod[l] != 0.f)
                    {
                        lane[l] = resScale.apply(resonance[t] + resMod[l]);
                    }
                }

                res = SIMD_MM(load_ps)(lane);
            }

            // Filter::setResonance and Filter::setMultimode
            auto r2 = SIMD_MM(sub_ps)(one, res);
            auto r4 = SIMD_MM(mul_ps)(SIMD_MM(set1_ps)(3.5f), res);
            auto m = SIMD_MM(set1_ps)(multimode[t]);
            auto pole = SIMD_MM(cvtepi32_ps)(SIMD_MM(cvttps_epi32)(SIMD_MM(mul_ps)(m, three)));
            auto xfade = mulAdd(m, three, SIMD_MM(xor_ps)(pole, SIMD_MM(set1_ps)(-0.f)));

            if (t == 0)
            {
                r2 = blend(SIMD_MM(load_ps)(res2Was), r2, soundingV);
                r4 = blend(SIMD_MM(load_ps)(res4Was), r4, soundingV);
                m = blend(SIMD_MM(load_ps)(mmWas), m, soundingV);
                pole = blend(SIMD_MM(load_ps)(poleWas), pole, soundingV);
                xfade = blend(SIMD_MM(load_ps)(xfadeWas), xfade, soundingV);
            }

            SIMD_MM(store_ps)(res2[s], r2);
            SIMD_MM(store_ps)(res4[s], r4);
            SIMD_MM(store_ps)(mm[s], m);
            SIMD_MM(store_ps)(mmPole[s], pole);
            SIMD_MM(store_ps)(mmXfade[s], xfade);
        }

        // padding past the end only ever reaches masked lanes
        for (int s = longest; s < Voice::maxBlockSubSamples; ++s)
        {
            for (int l = 0; l < lanes; ++l)
            {
                res2[s][l] = res4[s][l] = mm[s][l] = mmPole[s][l] = mmXfade[s][l] = 0.f;
            }
        }

        // each filter keeps the values as of its last sample
        for (int l = 0; l < laneCount; ++l)
        {
            auto &f = v[l]->filter;
            const int last = v[l]->blockSubSamples - 1;

            f.state.res2Pole = res2[last][l];
            f.state.res4Pole = res4[last][l];
            f.par.multimode = mm[last][l];
            f.state.multimodePole = static_cast<int>(mmPole[last][l]);
            f.state.multimodeXfade = mmXfade[last][l];
        }
    }

    static void process(const GroupRows &r, Voice *const v[lanes], int laneCount,
                        const float *lfo1In, const float *resonance,
                        const float *resonanceUnmodulated, const float *multimode, int subPerSample)
    {
        using namespace obxf_simd;

        constexpr int maxSubs = Voice::maxBlockSubSamples;

        // filter values as each lane's filter holds them, sample by sample
        alignas(16) float res2[maxSubs][lanes], res4[maxSubs][lanes], mm[maxSubs][lanes],
            mmPole[maxSubs][lanes], mmXfade[maxSubs][lanes];
        alignas(16) float scratchOut[maxSubs];

        float *out[lanes];

        alignas(16) float played[lanes]{}, dcScale[lanes]{}, brightScale[lanes]{},
            filterSrInv[lanes]{}, push[lanes]{}, vol1[lanes]{}, absVol1[lanes]{}, amt1[lanes]{},
            vol2[lanes]{}, absVol2[lanes]{}, amt2[lanes]{}, resCorr[lanes]{}, resCorrInv[lanes]{},
            oscBlock[lanes]{}, brightness[lanes]{}, pole1[lanes]{}, pole2[lanes]{}, pole3[lanes]{},
            pole4[lanes]{};

        int longest = 0;

        for (int l = 0; l < lanes; ++l)
        {
            if (l >= laneCount)
            {
                out[l] = scratchOut;
                continue;
            }

            auto &voice = *v[l];
            auto &f = voice.filter;
            const int k = voice.blockSubSamples;
            longest = std::max(longest, k);

            out[l] = voice.block.out;

            played[l] = static_cast<float>(k);

            dcScale[l] = voice.dcScale;
            brightScale[l] = voice.state.brightnessScale;
            filterSrInv[l] = f.sampleRateInv;
            push[l] = -1.f - (f.par.push2Pole * 0.035f);

            vol1[l] = voice.par.lfo1.volume;
            absVol1[l] = voice.par.lfo1.absVolume * 0.5f;
            amt1[l] = voice.matrixAdjustments.nativeOr(MatrixTarget::LFO1ModAmount2,
                                                       voice.par.lfo1.amt2) *
                      1.4285714285714286f;
            vol2[l] = voice.par.lfo2.volume;
            absVol2[l] = voice.par.lfo2.absVolume * 0.5f;
            amt2[l] = voice.matrixAdjustments.nativeOr(MatrixTarget::LFO2ModAmount2,
                                                       voice.par.lfo2.amt2) *
                      1.4285714285714286f;

            resCorr[l] = f.state.resCorrection;
            resCorrInv[l] = f.state.resCorrectionInv;

            oscBlock[l] = voice.state.oscBlock;
            brightness[l] = voice.state.brightness;
            pole1[l] = f.state.pole1;
            pole2[l] = f.state.pole2;
            pole3[l] = f.state.pole3;
            pole4[l] = f.state.pole4;
        }

        fillFilterRows(v, laneCount, longest, resonance, resonanceUnmodulated, multimode,
                       subPerSample, res2, res4, mm, mmPole, mmXfade);

        const auto &fp = v[0]->filter.par;
        const bool fourPole = v[0]->par.filter.fourPole;
        const bool xpander = fp.xpander4Pole;
        const bool bpBlend = fp.bpBlend2Pole;
        const auto *mix = Filter::poleMixFactors[fp.xpanderMode];

        auto ld = [](const float *p) { return SIMD_MM(load_ps)(p); };
        auto set1 = [](float f) { return SIMD_MM(set1_ps)(f); };

        const auto one = set1(1.f), two = set1(2.f), half = set1(0.5f), zero = set1(0.f);
        const auto piV = set1(pi);

        const auto playedV = ld(played), dcScaleV = ld(dcScale), brightScaleV = ld(brightScale),
                   srInvV = ld(filterSrInv), pushV = ld(push), vol1V = ld(vol1),
                   absVol1V = ld(absVol1), amt1V = ld(amt1), vol2V = ld(vol2),
                   absVol2V = ld(absVol2), amt2V = ld(amt2), resCorrV = ld(resCorr),
                   resCorrInvV = ld(resCorrInv);

        auto oscBlockV = ld(oscBlock), brightnessV = ld(brightness), p1 = ld(pole1), p2 = ld(pole2),
             p3 = ld(pole3), p4 = ld(pole4);

        // per-lane libm calls keep tan and atan exactly as the scalar filter has them
        auto perLane = [](SIMD_M128 x, SIMD_M128 mask, float (*fn)(float)) {
            alignas(16) float xs[lanes], ms[lanes];
            SIMD_MM(store_ps)(xs, x);
            SIMD_MM(store_ps)(ms, mask);
            for (int l = 0; l < lanes; ++l)
            {
                uint32_t bits;
                std::memcpy(&bits, &ms[l], sizeof(bits));
                xs[l] = bits ? fn(xs[l]) : 0.f;
            }
            return SIMD_MM(load_ps)(xs);
        };

        auto diode = [&](SIMD_M128 x) {
            // Taylor approximation of a slightly mismatched diode pair
            auto r = mulAdd(set1(0.0103592f), x, set1(0.00920833f));
            r = mulAdd(r, x, set1(0.185f));
            r = mulAdd(r, x, set1(0.05f));
            return mulAdd(r, x, one);
        };

        for (int s0 = 0; s0 < longest; s0 += 4)
        {
            SIMD_M128 res4Out[lanes];

            for (int j = 0; j < 4; ++j)
            {
                const int s = s0 + j;
                const auto active = SIMD_MM(cmplt_ps)(set1(static_cast<float>(s)), playedV);

                auto x = ld(r.out[s]);

                // process oscillator brightness, as Voice::processFinish has it
                {
                    const auto vv = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(x, oscBlockV), dcScaleV);
                    const auto lp = SIMD_MM(add_ps)(vv, oscBlockV);
                    oscBlockV = blend(oscBlockV,
                                      SIMD_MM(add_ps)(SIMD_MM(add_ps)(vv, vv), oscBlockV), active);
                    x = SIMD_MM(sub_ps)(x, lp);
                }
                {
                    const auto vv = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(x, brightnessV), brightScaleV);
                    const auto res = SIMD_MM(add_ps)(vv, brightnessV);
                    brightnessV = blend(
                        brightnessV, SIMD_MM(add_ps)(SIMD_MM(add_ps)(vv, vv), brightnessV), active);
                    x = res;
                }

                const auto prewarp = SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(ld(r.cutoff[s]), srInvV), piV);
                const auto g = exactTrig ? perLane(prewarp, active, tanf)
                                         : sst::basic_blocks::dsp::fasttanSSE(prewarp);

                if (fourPole)
                {
                    const auto onePlusG = SIMD_MM(add_ps)(one, g);
                    const auto lpc = SIMD_MM(div_ps)(g, onePlusG);
                    const auto ml = SIMD_MM(div_ps)(one, onePlusG);
                    const auto r4 = ld(res4[s]);

                    const auto S =
                        SIMD_MM(mul_ps)(mulAdd(lpc, mulAdd(lpc, mulAdd(lpc, p1, p2), p3), p4), ml);
                    const auto G =
                        SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(lpc, lpc), lpc), lpc);
                    const auto y0 = SIMD_MM(div_ps)(negMulAdd(r4, S, x), mulAdd(r4, G, one));

                    // first lowpass in the cascade, then damping
                    const auto vv = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(y0, p1), lpc);
                    const auto y1 = SIMD_MM(add_ps)(vv, p1);
                    const auto p1n = SIMD_MM(add_ps)(SIMD_MM(add_ps)(vv, vv), p1);
                    const auto damp = SIMD_MM(mul_ps)(p1n, resCorrV);
                    const auto damped =
                        exactTrig ? perLane(damp, active, atanf) : obxf_simd::atan(damp);
                    p1 = blend(p1, SIMD_MM(mul_ps)(damped, resCorrInvV), active);

                    auto stage = [&](SIMD_M128 &st, SIMD_M128 input) {
                        const auto w = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(input, st), lpc);
                        const auto r = SIMD_MM(add_ps)(w, st);
                        st = blend(st, SIMD_MM(add_ps)(SIMD_MM(add_ps)(w, w), st), active);
                        return r;
                    };

                    const auto y2 = stage(p2, y1);
                    const auto y3 = stage(p3, y2);
                    const auto y4 = stage(p4, y3);

                    SIMD_M128 o;

                    if (xpander)
                    {
                        o = mulAdd(y0, set1(mix[0]), SIMD_MM(mul_ps)(y1, set1(mix[1])));
                        o = mulAdd(y2, set1(mix[2]), o);
                        o = mulAdd(y3, set1(mix[3]), o);
                        o = mulAdd(y4, set1(mix[4]), o);
                    }
                    else
                    {
                        const auto xf = ld(mmXfade[s]);
                        const auto oneMinusXf = SIMD_MM(sub_ps)(one, xf);
                        const auto pole = ld(mmPole[s]);

                        const auto m0 = mulAdd(oneMinusXf, y4, SIMD_MM(mul_ps)(xf, y3));
                        const auto m1 = mulAdd(oneMinusXf, y3, SIMD_MM(mul_ps)(xf, y2));
                        const auto m2 = mulAdd(oneMinusXf, y2, SIMD_MM(mul_ps)(xf, y1));

                        o = zero;
                        o = blend(o, m0, SIMD_MM(cmpeq_ps)(pole, set1(0.f)));
                        o = blend(o, m1, SIMD_MM(cmpeq_ps)(pole, set1(1.f)));
                        o = blend(o, m2, SIMD_MM(cmpeq_ps)(pole, set1(2.f)));
                        o = blend(o, y1, SIMD_MM(cmpeq_ps)(pole, set1(3.f)));
                    }

                    // half volume compensation
                    x = SIMD_MM(mul_ps)(o, mulAdd(r4, set1(0.45f), one));
                }
                else
                {
                    const auto tCfb =
                        SIMD_MM(add_ps)(diode(SIMD_MM(mul_ps)(p1, set1(0.0876f))), pushV);
                    const auto rt = SIMD_MM(add_ps)(ld(res2[s]), tCfb);

                    const auto num = SIMD_MM(sub_ps)(
                        negMulAdd(g, p1, negMulAdd(two, SIMD_MM(mul_ps)(p1, rt), x)), p2);
                    const auto den = mulAdd(g, mulAdd(two, rt, g), one);
                    const auto vv = SIMD_MM(div_ps)(num, den);

                    const auto y1 = mulAdd(vv, g, p1);
                    p1 = blend(p1, mulAdd(vv, g, y1), active);
                    const auto y2 = mulAdd(y1, g, p2);
                    p2 = blend(p2, mulAdd(y1, g, y2), active);

                    const auto m = ld(mm[s]);

                    if (bpBlend)
                    {
                        const auto low =
                            mulAdd(SIMD_MM(sub_ps)(half, m), y2, SIMD_MM(mul_ps)(m, y1));
                        const auto high = mulAdd(SIMD_MM(sub_ps)(one, m), y1,
                                                 SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m, half), vv));
                        x = SIMD_MM(mul_ps)(two, blend(high, low, SIMD_MM(cmplt_ps)(m, half)));
                    }
                    else
                    {
                        x = mulAdd(SIMD_MM(sub_ps)(one, m), y2, SIMD_MM(mul_ps)(m, vv));
                    }
                }

                // LFO volume modulation, then the amp envelope
                x = SIMD_MM(mul_ps)(
                    x, negMulAdd(mulAdd(SIMD_MM(mul_ps)(vol1V, set1(lfo1In[s])), half, absVol1V),
                                 amt1V, one));
                x = SIMD_MM(mul_ps)(
                    x, negMulAdd(mulAdd(SIMD_MM(mul_ps)(vol2V, ld(r.lfo2[s])), half, absVol2V),
                                 amt2V, one));
                res4Out[j] = SIMD_MM(mul_ps)(x, ld(r.ampEnv[s]));
            }

            transpose(res4Out[0], res4Out[1], res4Out[2], res4Out[3]);

            for (int l = 0; l < lanes; ++l)
            {
                SIMD_MM(storeu_ps)(out[l] + s0, res4Out[l]);
            }
        }

        SIMD_MM(store_ps)(oscBlock, oscBlockV);
        SIMD_MM(store_ps)(brightness, brightnessV);
        SIMD_MM(store_ps)(pole1, p1);
        SIMD_MM(store_ps)(pole2, p2);
        SIMD_MM(store_ps)(pole3, p3);
        SIMD_MM(store_ps)(pole4, p4);

        for (int l = 0; l < laneCount; ++l)
        {
            auto &voice = *v[l];
            voice.state.oscBlock = oscBlock[l];
            voice.state.brightness = brightness[l];
            voice.filter.state.pole1 = pole1[l];
            voice.filter.state.pole2 = pole2[l];
            voice.filter.state.pole3 = pole3[l];
            voice.filter.state.pole4 = pole4[l];
        }
    }
};

#endif // OBXF_SRC_ENGINE_VOICEFINISHQUAD_H
