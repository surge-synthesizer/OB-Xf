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

#ifndef OBXF_SRC_ENGINE_ADSRENVELOPEQUAD_H
#define OBXF_SRC_ENGINE_ADSRENVELOPEQUAD_H

#include <algorithm>

#include "SimdMath.h"
#include "AdsrEnvelope.h"

/*
 * Steps the amp and filter envelopes of up to four voices together, one voice
 * per lane, with the same results as ADSREnvelope::processSample bit for bit.
 *
 * Lane state is loaded from the envelopes, stepped, and stored back. A lane
 * runs while its amp envelope is active at the start of a sub-sample, and once
 * it stops both its envelopes are left exactly as they were.
 */
struct ADSREnvelopeQuad
{
    static constexpr int lanes{4};

    // writes one row of lanes per sample, zero once a lane has stopped, through the
    // last four-sample chunk any lane plays in
    static void process(ADSREnvelope *const amp[lanes], ADSREnvelope *const filter[lanes],
                        float (*ampOut)[lanes], float (*filterOut)[lanes], int laneCount,
                        int subSamples, int run[lanes])
    {
        Lanes ampLanes, filterLanes;
        ampLanes.load(amp, laneCount);
        filterLanes.load(filter, laneCount);

        alignas(16) float laneOn[lanes]{};
        for (int l = 0; l < laneCount; ++l)
        {
            laneOn[l] = obxf_simd::allBits();
        }

        const auto inUse = SIMD_MM(load_ps)(laneOn);
        const auto silent = SIMD_MM(set1_ps)(static_cast<float>(ADSREnvelope::State::Silent));
        const auto one = SIMD_MM(set1_ps)(1.f);

        // sustain holds for the whole block, so a held envelope's output is constant
        const bool ampHeld = ampLanes.holdsSustain(inUse);
        const bool filterHeld = filterLanes.holdsSustain(inUse);

        auto runV = SIMD_MM(setzero_ps)();
        int s = 0;

        for (; s < subSamples; ++s)
        {
            const auto active = SIMD_MM(and_ps)(inUse, SIMD_MM(cmpneq_ps)(ampLanes.state, silent));

            if (SIMD_MM(movemask_ps)(active) == 0)
            {
                break;
            }

            const auto filterRes = filterHeld ? filterLanes.output : filterLanes.step(active);
            const auto ampRes = ampHeld ? ampLanes.output : ampLanes.step(active);

            SIMD_MM(store_ps)(filterOut[s], SIMD_MM(and_ps)(filterRes, active));
            SIMD_MM(store_ps)(ampOut[s], SIMD_MM(and_ps)(ampRes, active));
            runV = SIMD_MM(add_ps)(runV, SIMD_MM(and_ps)(one, active));
        }

        for (const int padded = (s + 3) / 4 * 4; s < padded; ++s)
        {
            SIMD_MM(store_ps)(filterOut[s], SIMD_MM(setzero_ps)());
            SIMD_MM(store_ps)(ampOut[s], SIMD_MM(setzero_ps)());
        }

        alignas(16) float runs[lanes];
        SIMD_MM(store_ps)(runs, runV);

        for (int l = 0; l < lanes; ++l)
        {
            run[l] = static_cast<int>(runs[l]);
        }

        ampLanes.store(amp, laneCount);
        filterLanes.store(filter, laneCount);
    }

  private:
    struct Lanes
    {
        SIMD_M128 state, output, outputLin, coef, coefLin, attackCurve, oneMinusCurve, sustain,
            decayCoef;

        void load(ADSREnvelope *const env[lanes], int laneCount)
        {
            alignas(16) float st[lanes]{}, out[lanes]{}, outLin[lanes]{}, c[lanes]{}, cLin[lanes]{},
                curve[lanes]{}, sus[lanes]{}, dCoef[lanes]{};

            for (int l = 0; l < lanes; ++l)
            {
                st[l] = static_cast<float>(ADSREnvelope::State::Silent);
            }

            for (int l = 0; l < laneCount; ++l)
            {
                const auto &e = *env[l];
                st[l] = static_cast<float>(e.state);
                out[l] = e.output;
                outLin[l] = e.outputLin;
                c[l] = e.coef;
                cLin[l] = e.coefLin;
                curve[l] = e.attackCurve;
                sus[l] = e.par.s;
                dCoef[l] = e.decayCoefficient();
            }

            state = SIMD_MM(load_ps)(st);
            output = SIMD_MM(load_ps)(out);
            outputLin = SIMD_MM(load_ps)(outLin);
            coef = SIMD_MM(load_ps)(c);
            coefLin = SIMD_MM(load_ps)(cLin);
            attackCurve = SIMD_MM(load_ps)(curve);
            oneMinusCurve = SIMD_MM(sub_ps)(SIMD_MM(set1_ps)(1.f), attackCurve);
            sustain = SIMD_MM(load_ps)(sus);
            decayCoef = SIMD_MM(load_ps)(dCoef);
        }

        void store(ADSREnvelope *const env[lanes], int laneCount) const
        {
            alignas(16) float st[lanes], out[lanes], outLin[lanes], c[lanes];
            SIMD_MM(store_ps)(st, state);
            SIMD_MM(store_ps)(out, output);
            SIMD_MM(store_ps)(outLin, outputLin);
            SIMD_MM(store_ps)(c, coef);

            for (int l = 0; l < laneCount; ++l)
            {
                auto &e = *env[l];
                e.state = static_cast<ADSREnvelope::State>(static_cast<int>(st[l]));
                e.output = out[l];
                e.outputLin = outLin[l];
                e.coef = c[l];
            }
        }

        // whether every lane in use sustains, and if so steps them all to the sustain level
        bool holdsSustain(SIMD_M128 inUse)
        {
            const auto sustaining = SIMD_MM(cmpeq_ps)(
                state, SIMD_MM(set1_ps)(static_cast<float>(ADSREnvelope::State::Sustain)));

            if (SIMD_MM(movemask_ps)(SIMD_MM(andnot_ps)(sustaining, inUse)) != 0)
            {
                return false;
            }

            output = SIMD_MM(min_ps)(sustain, SIMD_MM(set1_ps)(0.9f));
            return true;
        }

        // ADSREnvelope::processSample for the lanes in mask, the rest untouched
        SIMD_M128 step(SIMD_M128 mask)
        {
            using namespace obxf_simd;
            using S = ADSREnvelope::State;
            auto stateIs = [this, mask](S st) {
                return SIMD_MM(and_ps)(
                    mask, SIMD_MM(cmpeq_ps)(state, SIMD_MM(set1_ps)(static_cast<float>(st))));
            };

            const auto one = SIMD_MM(set1_ps)(1.f);
            const auto zero = SIMD_MM(setzero_ps)();

            const auto isAttack = stateIs(S::Attack);
            const auto isDecay = stateIs(S::Decay);
            const auto isSustain = stateIs(S::Sustain);
            const auto isRelease = stateIs(S::Release);
            const auto isSilent = stateIs(S::Silent);

            // attack, and the lanes where it hands over to decay this sample
            const auto attackEnds = SIMD_MM(and_ps)(
                isAttack, SIMD_MM(cmpgt_ps)(SIMD_MM(sub_ps)(output, one),
                                            SIMD_MM(set1_ps)(-ADSREnvelope::atkValueEnd)));
            const auto attackRuns = SIMD_MM(andnot_ps)(attackEnds, isAttack);

            const auto handover = SIMD_MM(min_ps)(
                mulAdd(oneMinusCurve, output, SIMD_MM(mul_ps)(attackCurve, outputLin)),
                SIMD_MM(set1_ps)(0.99f));

            const auto attackOut = negMulAdd(SIMD_MM(sub_ps)(one, output), coef, output);
            const auto attackOutLin = SIMD_MM(add_ps)(outputLin, coefLin);
            const auto attackResult =
                mulAdd(oneMinusCurve, attackOut, SIMD_MM(mul_ps)(attackOutLin, attackCurve));

            // decay, including lanes arriving from attack
            const auto inDecay = SIMD_MM(or_ps)(isDecay, attackEnds);
            const auto decayIn = blend(output, handover, attackEnds);
            const auto decayCoefNow = blend(coef, decayCoef, attackEnds);

            const auto toSustain =
                SIMD_MM(and_ps)(inDecay, SIMD_MM(cmplt_ps)(SIMD_MM(sub_ps)(decayIn, sustain),
                                                           SIMD_MM(set1_ps)(10e-6f)));
            const auto decayRuns = SIMD_MM(andnot_ps)(toSustain, inDecay);
            const auto decayOut = mulAdd(decayIn, decayCoefNow, decayIn);

            const auto sustainOut = SIMD_MM(min_ps)(sustain, SIMD_MM(set1_ps)(0.9f));

            const auto releaseRuns =
                SIMD_MM(and_ps)(isRelease, SIMD_MM(cmpgt_ps)(output, SIMD_MM(set1_ps)(20e-6f)));
            const auto releaseEnds = SIMD_MM(andnot_ps)(releaseRuns, isRelease);
            const auto releaseOut =
                SIMD_MM(add_ps)(mulAdd(output, coef, output), SIMD_MM(set1_ps)(dc));

            // results keep the incoming output where processSample does
            auto result = output;
            result = blend(result, attackResult, attackRuns);
            result = blend(result, decayOut, decayRuns);
            result = blend(result, sustainOut, isSustain);
            result = blend(result, releaseOut, releaseRuns);
            result = blend(result, zero, isSilent);

            auto out = output;
            out = blend(out, attackOut, attackRuns);
            out = blend(out, decayIn, toSustain);
            out = blend(out, decayOut, decayRuns);
            out = blend(out, sustainOut, isSustain);
            out = blend(out, releaseOut, releaseRuns);
            out = blend(out, zero, isSilent);
            output = out;

            outputLin = blend(outputLin, attackOutLin, attackRuns);
            coef = blend(coef, decayCoefNow, attackEnds);

            state = blend(state, SIMD_MM(set1_ps)(static_cast<float>(S::Decay)), attackEnds);
            state = blend(state, SIMD_MM(set1_ps)(static_cast<float>(S::Sustain)), toSustain);
            state = blend(state, SIMD_MM(set1_ps)(static_cast<float>(S::Silent)), releaseEnds);

            return result;
        }
    };
};

#endif // OBXF_SRC_ENGINE_ADSRENVELOPEQUAD_H
