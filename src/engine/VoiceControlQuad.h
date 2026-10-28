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

#ifndef OBXF_SRC_ENGINE_VOICECONTROLQUAD_H
#define OBXF_SRC_ENGINE_VOICECONTROLQUAD_H

#include <algorithm>
#include <cmath>

#include "AdsrEnvelopeQuad.h"
#include "GroupRows.h"
#include "NoiseQuad.h"
#include "SimdMath.h"
#include "Voice.h"

/*
 * The voice control stage for up to four voices at once, one voice per lane:
 * portamento, the filter cutoff, and the oscillators' pitch and PW modulation.
 * With exactExp the results match the per-voice scalar control bit for bit;
 * without it the cutoff's exp is a vector approximation, well under a cent out.
 *
 * The per-sample inputs go into one row of four lanes per sample first: the
 * engine's values as rows, each voice's LFO 2 waveform in scalar code, its cutoff
 * noise, LFO 2 smoothing and delay lines four voices at a time. The portamento filter
 * and all the modulation sums then run four voices at a time straight from those rows, and leave
 * theirs in the group's rows for the oscillators and filter.
 *
 * The group's envelopes run first, into its own rows, since how long each voice
 * plays decides the block's shape. A group holds its voices' delay lines across
 * blocks, as the oscillators do, and hands a voice's back when it leaves the group
 * or stops sounding.
 */
struct VoiceControlQuad
{
    static constexpr int lanes{4};

    // libm exp per lane matches the scalar cutoff pitch bit for bit, at a cost
    static constexpr bool exactExp{false};

    // hands back lanes whose voice moved or stopped, so no other group loads it stale
    void releaseLanes(Voice *const v[lanes], int laneCount)
    {
        for (int l = 0; l < lanes; ++l)
        {
            if (held[l] && held[l] != (l < laneCount ? v[l] : nullptr))
            {
                unloadLane(l);
            }
        }
    }

    void releaseAll()
    {
        for (int l = 0; l < lanes; ++l)
        {
            if (held[l])
            {
                unloadLane(l);
            }
        }
    }

    // the voices' envelopes, before any group's control runs; returns the most samples any played
    int processEnvelopes(Voice *const v[lanes], int laneCount, int subSamples)
    {
        using namespace obxf_simd;

        for (int l = 0; l < lanes; ++l)
        {
            Voice *voice = l < laneCount ? v[l] : nullptr;

            if (held[l] != voice)
            {
                if (held[l])
                {
                    unloadLane(l);
                }

                if (voice)
                {
                    loadLane(*voice, l);
                }
            }
        }

        ADSREnvelope *amp[lanes]{}, *filter[lanes]{};
        int played[lanes]{};

        for (int l = 0; l < laneCount; ++l)
        {
            amp[l] = &v[l]->ampEnv;
            filter[l] = &v[l]->filterEnv;
        }

        ADSREnvelopeQuad::process(amp, filter, ampEnvRows, modEnvRows, laneCount, subSamples,
                                  played);

        alignas(16) float invert[lanes]{}, filterScale[lanes]{}, ampScale[lanes]{},
            playedF[lanes]{};
        longest = 0;

        for (int l = 0; l < laneCount; ++l)
        {
            const auto &voice = *v[l];
            invert[l] = voice.par.filter.invertEnvScale;
            filterScale[l] = voice.filterEnvScale();
            ampScale[l] = voice.ampEnvScale();
            playedF[l] = static_cast<float>(played[l]);
            longest = std::max(longest, played[l]);
        }

        auto ld = [](const float *x) { return SIMD_MM(load_ps)(x); };
        const auto invertV = ld(invert), filterScaleV = ld(filterScale), ampScaleV = ld(ampScale),
                   playedV = ld(playedF);
        auto &line = delays[AmpEnvDelay];

        // stopped lanes hold zero, which scales to zero
        for (int s = 0; s < longest; ++s)
        {
            const auto active = SIMD_MM(cmplt_ps)(SIMD_MM(set1_ps)(static_cast<float>(s)), playedV);
            const int at = (delayPos + s) & delayMask, next = (at + 1) & delayMask;

            SIMD_MM(store_ps)(
                modEnvRows[s],
                SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(invertV, ld(modEnvRows[s])), filterScaleV));

            const auto in = SIMD_MM(mul_ps)(ld(ampEnvRows[s]), ampScaleV);
            SIMD_MM(store_ps)(line[at], blend(ld(line[at]), in, active));
            SIMD_MM(store_ps)(ampEnvRows[s], SIMD_MM(and_ps)(ld(line[next]), active));
        }

        for (int l = 0; l < laneCount; ++l)
        {
            v[l]->finishEnvelopes(played[l], subSamples,
                                  played[l] > 0 ? ampEnvRows[played[l] - 1][l] : 0.f);
        }

        return longest;
    }

    void process(GroupRows &groupRows, Voice *const v[lanes], int laneCount, const float *lfo1In,
                 const float *vibratoLFOIn, const float *cutoff, const float *bend,
                 int subPerSample)
    {
        rows = &groupRows;

        const int padded = (longest + 3) / 4 * 4;

        fillRows(v, laneCount, longest, padded, lfo1In, cutoff, bend, subPerSample);
        delayPos = (delayPos + longest) & delayMask;

        // a stopped voice may be reset by its next note, so it goes back as of where it stopped
        for (int l = 0; l < laneCount; ++l)
        {
            if (!v[l]->sounding)
            {
                unloadLane(l, longest - v[l]->blockSubSamples);
            }
        }

        for (int l = 0; l < lanes; ++l)
        {
            // spare lanes borrow the first voice's settings and are never stored
            setLaneConstants(*v[std::min(l, laneCount - 1)], l,
                             l < laneCount ? v[l]->blockSubSamples : 0);
        }

        runLanes(v, laneCount, padded, lfo1In, vibratoLFOIn);
    }

  private:
    static constexpr int maxSubs = Voice::maxBlockSubSamples;
    static constexpr int delaySize = B_SAMPLES * OVERSAMPLE_FACTOR;
    static constexpr int delayMask = delaySize - 1;

    static_assert((delaySize & delayMask) == 0);

    enum Delay
    {
        LFO1Delay,
        LFO2Delay,
        FilterEnvDelay,
        AmpEnvDelay,
        delayCount
    };

    GroupRows *rows{nullptr};

    // the voice whose delay lines each lane holds
    Voice *held[lanes]{};

    // held delay lines, read relative to delayPos
    alignas(16) float delays[delayCount][delaySize][lanes]{};
    int delayPos{0};

    // envelope rows from processEnvelopes, and how far the longest lane played
    alignas(16) float modEnvRows[maxSubs][lanes], ampEnvRows[maxSubs][lanes];
    int longest{0};

    static auto &delayLine(Voice &voice, int d)
    {
        switch (d)
        {
        case LFO1Delay:
            return voice.lfo1Delayed;
        case LFO2Delay:
            return voice.lfo2Delayed;
        case FilterEnvDelay:
            return voice.filterEnvDelayed;
        default:
            return voice.ampEnvDelayed;
        }
    }

    // a delay line's next read is the sample fed delaySize - 1 steps before it
    static int delayAt(int pos, int age) { return (pos - age) & delayMask; }

    void loadLane(Voice &voice, int l)
    {
        for (int d = 0; d < delayCount; ++d)
        {
            auto &line = delayLine(voice, d);

            for (int age = 1; age < delaySize; ++age)
            {
                delays[d][delayAt(delayPos, age)][l] = line.history(age);
            }
        }

        held[l] = &voice;
    }

    // back is how many samples before the group's position this lane stopped
    void unloadLane(int l, int back = 0)
    {
        const int pos = delayPos - back;

        for (int d = 0; d < delayCount; ++d)
        {
            auto &line = delayLine(*held[l], d);

            for (int age = 1; age < delaySize; ++age)
            {
                line.setHistory(age, delays[d][delayAt(pos, age)][l]);
            }

            for (int j = 0; j < delaySize; ++j)
            {
                delays[d][j][l] = 0.f;
            }
        }

        held[l] = nullptr;
    }

    // each lane's LFO 2 smoother target, sample by sample
    alignas(16) float lfo2Target[maxSubs][lanes];

    // per lane constants, named after the scalar expressions they stand for
    struct alignas(16) LaneConstants
    {
        float played[lanes], portaIn[lanes], portaScale[lanes], portaState[lanes], pbDown[lanes],
            pbUp[lanes], mpeBend[lanes], cutoffMod[lanes], lfo1Cut[lanes], lfo2Cut[lanes],
            lfo1Amt1[lanes], lfo1Amt2[lanes], lfo2Amt1[lanes], lfo2Amt2[lanes], slopCut[lanes],
            slopCutPar[lanes], envAmt[lanes], keytrack[lanes], maxCut[lanes], pushOn[lanes],
            pushCut[lanes], pwSign[lanes], pitchSign[lanes], l1o1pw[lanes], l2o1pw[lanes],
            l1o2pw[lanes], l2o2pw[lanes], envPWBoth[lanes], envPWAmt[lanes], pwOffset[lanes],
            pbBoth[lanes], l1o1p[lanes], l2o1p[lanes], l1o2p[lanes], l2o2p[lanes],
            envPitchBoth[lanes], envPitchAmt[lanes], osc1Mod[lanes], osc2Mod[lanes], bothMod[lanes];
    } lc;

    void fillRows(Voice *const v[lanes], int laneCount, int longest, int padded,
                  const float *lfo1In, const float *cutoff, const float *bend, int subPerSample)
    {
        using namespace obxf_simd;

        auto &r = *rows;
        int played[lanes]{};
        alignas(16) float playedF[lanes]{}, cutoffWas[lanes]{}, bendWas[lanes]{}, sounding[lanes]{};
        Noise *noise[lanes]{};

        for (int l = 0; l < laneCount; ++l)
        {
            auto &voice = *v[l];
            played[l] = voice.blockSubSamples;
            playedF[l] = static_cast<float>(played[l]);
            cutoffWas[l] = voice.par.filter.cutoff;
            bendWas[l] = voice.pitchBend;
            sounding[l] = voice.soundingAtBlockStart ? allBits() : 0.f;
            noise[l] = &voice.noiseGen;
        }

        const auto playedV = SIMD_MM(load_ps)(playedF);

        // every voice takes the engine's values as each output sample starts, bar a voice
        // starting this block, which keeps its own through the first one
        const auto soundingV = SIMD_MM(load_ps)(sounding);
        const auto cutoffWasV = SIMD_MM(load_ps)(cutoffWas), bendWasV = SIMD_MM(load_ps)(bendWas);

        for (int s = 0; s < padded; ++s)
        {
            const int t = std::min(s, longest - 1) / subPerSample;
            auto c = SIMD_MM(set1_ps)(cutoff[t]), b = SIMD_MM(set1_ps)(bend[t]);

            if (t == 0)
            {
                c = blend(cutoffWasV, c, soundingV);
                b = blend(bendWasV, b, soundingV);
            }

            SIMD_MM(store_ps)(r.cutoffPar[s], c);
            SIMD_MM(store_ps)(r.bend[s], b);
        }

        // LFO 2 only steps a counter between the samples its waveform moves on
        for (int l = 0; l < laneCount; ++l)
        {
            auto &voice = *v[l];
            auto &lfo = voice.lfo2;
            const int k = played[l];

            voice.par.filter.cutoff = r.cutoffPar[k - 1][l];
            voice.pitchBend = r.bend[k - 1][l];

            lfo.setRate(juce::jmax(0.01f, voice.matrixAdjustments.nativeOr(MatrixTarget::LFO2Rate,
                                                                           voice.lfo2BaseRate)));

            for (int s = 0; s < k;)
            {
                lfo.update();

                const float target = lfo.state.blockTarget;
                const int run = std::min(k - s, LFO::blockFactor - lfo.state.blockPos);

                for (int i = 0; i < run; ++i)
                {
                    lfo2Target[s + i][l] = target;
                }

                lfo.state.blockPos += run - 1;
                s += run;
            }

            for (int s = k; s < padded; ++s)
            {
                lfo2Target[s][l] = 0.f;
            }
        }

        for (int l = laneCount; l < lanes; ++l)
        {
            for (int s = 0; s < padded; ++s)
            {
                lfo2Target[s][l] = 0.f;
            }
        }

        std::copy(&ampEnvRows[0][0], &ampEnvRows[0][0] + padded * lanes, &r.ampEnv[0][0]);

        // with juce::Random this was swinging ~[-1.75, 1.75]
        // but our Noise class swings ~[-0.52, 0.52], so a factor of 3.365 retains it
        NoiseQuad cutoffNoise;
        cutoffNoise.load(noise, laneCount);

        for (int s = 0; s < padded; ++s)
        {
            const auto active = SIMD_MM(cmplt_ps)(SIMD_MM(set1_ps)(static_cast<float>(s)), playedV);
            SIMD_MM(store_ps)(r.noise[s],
                              SIMD_MM(mul_ps)(cutoffNoise.white(active), SIMD_MM(set1_ps)(3.365f)));
        }

        cutoffNoise.store(noise, laneCount);

        smoothLFO2(v, laneCount, padded, played);
        runDelays(longest, padded, playedV, lfo1In);
    }

    // DelayLine::feedReturn for every lane at once, a lane holding still once it has played
    void runDelays(int longest, int padded, SIMD_M128 playedV, const float *lfo1In)
    {
        using namespace obxf_simd;

        auto &r = *rows;
        auto ld = [](const float *x) { return SIMD_MM(load_ps)(x); };

        for (int s = 0; s < longest; ++s)
        {
            const auto active = SIMD_MM(cmplt_ps)(SIMD_MM(set1_ps)(static_cast<float>(s)), playedV);
            const int at = (delayPos + s) & delayMask, next = (at + 1) & delayMask;

            auto feed = [&](int d, SIMD_M128 in, float *out) {
                auto &line = delays[d];
                SIMD_MM(store_ps)(line[at], blend(ld(line[at]), in, active));
                SIMD_MM(store_ps)(out, SIMD_MM(and_ps)(ld(line[next]), active));
            };

            feed(LFO1Delay, SIMD_MM(set1_ps)(lfo1In[s]), r.lfo1Mod[s]);
            feed(LFO2Delay, ld(r.lfo2[s]), r.lfo2Mod[s]);
            feed(FilterEnvDelay, ld(modEnvRows[s]), r.envDelayed[s]);
        }

        for (int s = longest; s < padded; ++s)
        {
            const auto zero = SIMD_MM(setzero_ps)();
            SIMD_MM(store_ps)(r.lfo1Mod[s], zero);
            SIMD_MM(store_ps)(r.lfo2Mod[s], zero);
            SIMD_MM(store_ps)(r.envDelayed[s], zero);
        }
    }

    // LFO::getVal for every lane at once, a lane holding still once it has played
    void smoothLFO2(Voice *const v[lanes], int laneCount, int padded, const int played[lanes])
    {
        auto &r = *rows;
        alignas(16) float smoothed[lanes]{}, scale[lanes]{}, playedF[lanes]{};

        for (int l = 0; l < laneCount; ++l)
        {
            const auto &lfo = v[l]->lfo2;
            smoothed[l] = lfo.state.smoothedOutput;
            scale[l] = lfo.smoothScale;
            playedF[l] = static_cast<float>(played[l]);
        }

        const auto scaleV = SIMD_MM(load_ps)(scale), playedV = SIMD_MM(load_ps)(playedF);
        auto st = SIMD_MM(load_ps)(smoothed);

        // as tpt_lp_unwarped
        for (int s = 0; s < padded; ++s)
        {
            const auto active = SIMD_MM(cmplt_ps)(SIMD_MM(set1_ps)(static_cast<float>(s)), playedV);
            const auto vv =
                SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(SIMD_MM(load_ps)(lfo2Target[s]), st), scaleV);

            SIMD_MM(store_ps)(r.lfo2[s], SIMD_MM(add_ps)(vv, st));
            st = obxf_simd::blend(st, SIMD_MM(add_ps)(st, SIMD_MM(add_ps)(vv, vv)), active);
        }

        SIMD_MM(store_ps)(smoothed, st);

        for (int l = 0; l < laneCount; ++l)
        {
            v[l]->lfo2.state.smoothedOutput = smoothed[l];
        }
    }

    void setLaneConstants(const Voice &voice, int l, int played)
    {
        constexpr auto pitchSpan = matrixTargetScaling(MatrixTarget::OscPitch).span();

        const auto &adj = voice.matrixAdjustments;
        const auto &p = voice.par;

        lc.played[l] = static_cast<float>(played);

        // why -93? beats me!
        lc.portaIn[l] = static_cast<float>(voice.tuning->tunedMidiNote(voice.midiNote) - 93);

        // tpt_lp_unwarped's scale, from the portamento time
        const float portaTime = p.osc.portamento * (1 + voice.slop.portamento * p.slop.portamento);
        lc.portaScale[l] = tpt_unwarped_scale(portaTime, voice.sampleRateInv);
        lc.portaState[l] = voice.state.portamento;

        lc.pbDown[l] = p.extmod.pbDown;
        lc.pbUp[l] = p.extmod.pbUp;
        lc.mpeBend[l] = voice.mpeBend;

        lc.cutoffMod[l] = adj.modFor(MatrixTarget::FilterCutoff);
        lc.lfo1Cut[l] = p.lfo1.cutoff;
        lc.lfo2Cut[l] = p.lfo2.cutoff;
        lc.lfo1Amt1[l] = adj.nativeOr(MatrixTarget::LFO1ModAmount1, p.lfo1.amt1);
        lc.lfo1Amt2[l] = adj.nativeOr(MatrixTarget::LFO1ModAmount2, p.lfo1.amt2);
        lc.lfo2Amt1[l] = adj.nativeOr(MatrixTarget::LFO2ModAmount1, p.lfo2.amt1);
        lc.lfo2Amt2[l] = adj.nativeOr(MatrixTarget::LFO2ModAmount2, p.lfo2.amt2);
        lc.slopCut[l] = voice.slop.cutoff;
        lc.slopCutPar[l] = p.slop.cutoff;
        lc.envAmt[l] = p.filter.envAmt;
        lc.keytrack[l] = p.filter.keytrack;
        lc.maxCut[l] = (voice.sampleRate * 0.5f - 120.0f);
        lc.pushOn[l] = p.filter.push2Pole ? obxf_simd::allBits() : 0.f;
        lc.pushCut[l] = 19000.f + (5000.f * p.oversample);

        lc.pwSign[l] = static_cast<float>(voice.oscs.par.mod.envToPWInvert ? -1 : 1);
        lc.pitchSign[l] = static_cast<float>(voice.oscs.par.mod.envToPitchInvert ? -1 : 1);

        lc.l1o1pw[l] = p.lfo1.osc1PW;
        lc.l2o1pw[l] = p.lfo2.osc1PW;
        lc.l1o2pw[l] = p.lfo1.osc2PW;
        lc.l2o2pw[l] = p.lfo2.osc2PW;
        lc.envPWBoth[l] = p.osc.envPWBothOscs ? obxf_simd::allBits() : 0.f;
        lc.envPWAmt[l] = p.osc.envPWAmt;
        lc.pwOffset[l] = adj.nativeOr(MatrixTarget::Osc2PWOffset, p.osc.pwOsc2Offset);

        lc.pbBoth[l] = !p.extmod.pbOsc2Only ? obxf_simd::allBits() : 0.f;
        lc.l1o1p[l] = p.lfo1.osc1Pitch;
        lc.l2o1p[l] = p.lfo2.osc1Pitch;
        lc.l1o2p[l] = p.lfo1.osc2Pitch;
        lc.l2o2p[l] = p.lfo2.osc2Pitch;
        lc.envPitchBoth[l] = p.osc.envPitchBothOscs ? obxf_simd::allBits() : 0.f;
        lc.envPitchAmt[l] = p.osc.envPitchAmt;
        lc.osc1Mod[l] = adj.modFor(MatrixTarget::Osc1Pitch);
        lc.osc2Mod[l] = adj.modFor(MatrixTarget::Osc2Pitch);
        lc.bothMod[l] = adj.modFor(MatrixTarget::OscPitch) * pitchSpan;
    }

    void runLanes(Voice *const v[lanes], int laneCount, int padded, const float *lfo1In,
                  const float *vibratoLFOIn)
    {
        using namespace obxf_simd;

        constexpr auto cutoffScale = matrixTargetScaling(MatrixTarget::FilterCutoff);
        constexpr auto pitchSpan = matrixTargetScaling(MatrixTarget::OscPitch).span();

        auto ld = [](const float *x) { return SIMD_MM(load_ps)(x); };
        auto set1 = [](float f) { return SIMD_MM(set1_ps)(f); };

        const auto zero = set1(0.f), forty = set1(40.f), fortyFive = set1(45.f),
                   span = set1(cutoffScale.span()), cutMin = set1(cutoffScale.nativeMin),
                   cutMax = set1(cutoffScale.nativeMax), spanPitch = set1(pitchSpan),
                   pitchMult = set1(mult), a440 = set1(440.f), twelfth = set1(1.f / 12.f);

        const auto playedV = ld(lc.played), portaInV = ld(lc.portaIn),
                   portaScaleV = ld(lc.portaScale), pbDownV = ld(lc.pbDown), pbUpV = ld(lc.pbUp),
                   mpeBendV = ld(lc.mpeBend), cutoffModV = ld(lc.cutoffMod),
                   lfo1CutV = ld(lc.lfo1Cut), lfo2CutV = ld(lc.lfo2Cut),
                   lfo1Amt1V = ld(lc.lfo1Amt1), lfo1Amt2V = ld(lc.lfo1Amt2),
                   lfo2Amt1V = ld(lc.lfo2Amt1), lfo2Amt2V = ld(lc.lfo2Amt2),
                   slopCutV = ld(lc.slopCut), slopCutParV = ld(lc.slopCutPar),
                   envAmtV = ld(lc.envAmt), keytrackV = ld(lc.keytrack), maxCutV = ld(lc.maxCut),
                   pushOnV = ld(lc.pushOn), pushCutV = ld(lc.pushCut), pwSignV = ld(lc.pwSign),
                   pitchSignV = ld(lc.pitchSign), l1o1pwV = ld(lc.l1o1pw), l2o1pwV = ld(lc.l2o1pw),
                   l1o2pwV = ld(lc.l1o2pw), l2o2pwV = ld(lc.l2o2pw), envPWBothV = ld(lc.envPWBoth),
                   envPWAmtV = ld(lc.envPWAmt), pwOffsetV = ld(lc.pwOffset),
                   pbBothV = ld(lc.pbBoth), l1o1pV = ld(lc.l1o1p), l2o1pV = ld(lc.l2o1p),
                   l1o2pV = ld(lc.l1o2p), l2o2pV = ld(lc.l2o2p),
                   envPitchBothV = ld(lc.envPitchBoth), envPitchAmtV = ld(lc.envPitchAmt),
                   osc1ModV = ld(lc.osc1Mod), osc2ModV = ld(lc.osc2Mod), bothModV = ld(lc.bothMod);

        auto portaV = ld(lc.portaState);

        // libm exp per lane keeps getPitch exactly as the scalar control has it
        auto expPerLane = [](SIMD_M128 x, SIMD_M128 mask) {
            alignas(16) float xs[lanes], ms[lanes];
            SIMD_MM(store_ps)(xs, x);
            SIMD_MM(store_ps)(ms, mask);
            for (int l = 0; l < lanes; ++l)
            {
                uint32_t bits;
                std::memcpy(&bits, &ms[l], sizeof(bits));
                xs[l] = bits ? std::exp(xs[l]) : 0.f;
            }
            return SIMD_MM(load_ps)(xs);
        };

        auto &r = *rows;
        auto st = [](float *row, SIMD_M128 x) { SIMD_MM(store_ps)(row, x); };

        for (int s = 0; s < padded; ++s)
        {
            const auto active = SIMD_MM(cmplt_ps)(set1(static_cast<float>(s)), playedV);

            const auto cutoffParV = ld(r.cutoffPar[s]), bendV = ld(r.bend[s]),
                       f1 = ld(r.lfo1Mod[s]), f2 = ld(r.lfo2Mod[s]), noiseV = ld(r.noise[s]),
                       envDel = ld(r.envDelayed[s]), lfo2V = ld(r.lfo2[s]),
                       modEnv = ld(modEnvRows[s]);
            const auto lfo1V = set1(lfo1In[s]);
            const auto vibV = set1(vibratoLFOIn[s]);

            // portamento processing (implements RC circuit), as tpt_lp_unwarped
            const auto pv = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(portaInV, portaV), portaScaleV);
            const auto porta = SIMD_MM(add_ps)(pv, portaV);
            portaV = blend(portaV, SIMD_MM(add_ps)(SIMD_MM(add_ps)(pv, pv), portaV), active);
            st(r.notePlaying[s], porta);

            // rescale pitch bend
            const auto pbs = SIMD_MM(add_ps)(blend(SIMD_MM(mul_ps)(bendV, pbUpV),
                                                   SIMD_MM(mul_ps)(bendV, pbDownV),
                                                   SIMD_MM(cmplt_ps)(bendV, zero)),
                                             mpeBendV);

            const auto modulatedCutoff = SIMD_MM(max_ps)(
                cutMin, SIMD_MM(min_ps)(cutMax, mulAdd(cutoffModV, span, cutoffParV)));

            auto arg = mulAdd(SIMD_MM(mul_ps)(lfo1CutV, f1), lfo1Amt1V,
                              SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(lfo2CutV, f2), lfo2Amt1V));
            arg = SIMD_MM(add_ps)(arg, modulatedCutoff);
            arg = mulAdd(slopCutV, slopCutParV, arg);
            arg = mulAdd(envAmtV, envDel, arg);
            arg = SIMD_MM(sub_ps)(arg, fortyFive);
            arg = mulAdd(keytrackV, SIMD_MM(add_ps)(SIMD_MM(add_ps)(pbs, porta), forty), arg);

            // getPitch, which is 440 * 2^(arg / 12)
            const auto cutoffPitch =
                SIMD_MM(mul_ps)(a440, exactExp ? expPerLane(SIMD_MM(mul_ps)(pitchMult, arg), active)
                                               : obxf_simd::exp2(SIMD_MM(mul_ps)(arg, twelfth)));

            // limit max cutoff for numerical stability, and on self-oscillation
            auto cutoffcalc = SIMD_MM(min_ps)(SIMD_MM(add_ps)(cutoffPitch, noiseV), maxCutV);
            cutoffcalc = blend(cutoffcalc, SIMD_MM(min_ps)(cutoffcalc, pushCutV), pushOnV);
            st(r.cutoff[s], cutoffcalc);

            // pulse width modulation
            const auto pwenv = SIMD_MM(mul_ps)(modEnv, pwSignV);

            st(r.osc1PWMod[s],
               SIMD_MM(add_ps)(mulAdd(SIMD_MM(mul_ps)(l1o1pwV, lfo1V), lfo1Amt2V,
                                      SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(l2o1pwV, lfo2V), lfo2Amt2V)),
                               blend(zero, SIMD_MM(mul_ps)(envPWAmtV, pwenv), envPWBothV)));

            st(r.osc2PWMod[s],
               SIMD_MM(add_ps)(
                   mulAdd(envPWAmtV, pwenv,
                          mulAdd(SIMD_MM(mul_ps)(l1o2pwV, lfo1V), lfo1Amt2V,
                                 SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(l2o2pwV, lfo2V), lfo2Amt2V))),
                   pwOffsetV));

            // pitch modulation
            const auto pitchEnv = SIMD_MM(mul_ps)(modEnv, pitchSignV);

            auto p1 = mulAdd(SIMD_MM(mul_ps)(l1o1pV, lfo1V), lfo1Amt1V, blend(zero, pbs, pbBothV));
            p1 = mulAdd(SIMD_MM(mul_ps)(l2o1pV, lfo2V), lfo2Amt1V, p1);
            p1 = SIMD_MM(add_ps)(
                p1, blend(zero, SIMD_MM(mul_ps)(envPitchAmtV, pitchEnv), envPitchBothV));
            p1 = SIMD_MM(add_ps)(p1, vibV);
            p1 = mulAdd(osc1ModV, spanPitch, p1);
            st(r.osc1PitchMod[s], SIMD_MM(add_ps)(p1, bothModV));

            auto p2 = mulAdd(SIMD_MM(mul_ps)(l1o2pV, lfo1V), lfo1Amt1V, pbs);
            p2 = mulAdd(SIMD_MM(mul_ps)(l2o2pV, lfo2V), lfo2Amt1V, p2);
            p2 = mulAdd(envPitchAmtV, pitchEnv, p2);
            p2 = SIMD_MM(add_ps)(p2, vibV);
            p2 = mulAdd(osc2ModV, spanPitch, p2);
            st(r.osc2PitchMod[s], SIMD_MM(add_ps)(p2, bothModV));
        }

        SIMD_MM(store_ps)(lc.portaState, portaV);
        for (int l = 0; l < laneCount; ++l)
        {
            v[l]->state.portamento = lc.portaState[l];
        }
    }
};

#endif // OBXF_SRC_ENGINE_VOICECONTROLQUAD_H
