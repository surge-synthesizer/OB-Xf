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

#ifndef OBXF_SRC_ENGINE_OSCILLATORQUAD_H
#define OBXF_SRC_ENGINE_OSCILLATORQUAD_H

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

#include "GroupRows.h"
#include "NoiseQuad.h"
#include "SimdMath.h"
#include "Voice.h"

/*
 * Voice::processOscillators for up to four voices at once, one voice per lane. With
 * exactExp the results match the scalar oscillators bit for bit; without it the
 * two pitch exps are a vector approximation, well under a cent out.
 *
 * A group holds its voices' oscillator state across blocks, the BLEP rings and
 * delay lines rotated so the lanes share one position. A voice's state goes in
 * when it joins the group and back to the voice when it leaves, or whenever the
 * group can't run its lanes together. Noise runs first per voice, in its
 * original order. A BLEP lands only around a waveform edge, so the edge logic is
 * the scalar code run for just the lanes that need it. When a voice stops before
 * the end of the block its lane runs on with its state held, and its state goes
 * back to it as of the sample it stopped.
 */
struct OscillatorQuad
{
    static constexpr int lanes{4};

    // libm exp per lane matches the scalar oscillator pitch bit for bit, at a cost
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

    void process(GroupRows &r, Voice *const v[lanes], int laneCount)
    {
        if (!compatible(v, laneCount))
        {
            releaseAll();

            for (int l = 0; l < laneCount; ++l)
            {
                auto &voice = *v[l];
                auto &b = voice.block;
                const int k = voice.blockSubSamples;

                GroupRows::toVoice(r.notePlaying, l, b.notePlaying, k);
                GroupRows::toVoice(r.osc1PWMod, l, b.osc1PWMod, k);
                GroupRows::toVoice(r.osc2PWMod, l, b.osc2PWMod, k);
                GroupRows::toVoice(r.osc1PitchMod, l, b.osc1PitchMod, k);
                GroupRows::toVoice(r.osc2PitchMod, l, b.osc2PitchMod, k);
                voice.processOscillators();
                GroupRows::fromVoice(r.out, l, b.out, k);
            }
            return;
        }

        if (!configured || !sameConfig(v[0]->oscs))
        {
            releaseAll();
            takeConfig(v[0]->oscs);
        }

        int shortest = v[0]->blockSubSamples, longest = shortest;
        alignas(16) float played[lanes]{};

        for (int l = 0; l < lanes; ++l)
        {
            // spare lanes borrow the first voice's settings and run on silence
            setLaneConstants(*v[std::min(l, laneCount - 1)], l);

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

            if (voice)
            {
                played[l] = static_cast<float>(voice->blockSubSamples);
                shortest = std::min(shortest, voice->blockSubSamples);
                longest = std::max(longest, voice->blockSubSamples);
            }
        }

        fillNoise(v, laneCount, longest);

        if (shortest == longest)
        {
            run<false>(r, longest, SIMD_MM(load_ps)(played));
            return;
        }

        run<true>(r, longest, SIMD_MM(load_ps)(played));

        // a lane that stopped early hands back its state as of where it stopped
        for (int l = 0; l < laneCount; ++l)
        {
            if (v[l]->blockSubSamples < longest)
            {
                unloadLane(l, longest - v[l]->blockSubSamples);
            }
        }
    }

  private:
    static constexpr int maxSubs = Voice::maxBlockSubSamples;
    static constexpr int ringSize = B_SAMPLESx2;
    static constexpr int ringMask = ringSize - 1;
    static constexpr int delaySize = B_SAMPLES;
    static constexpr int delayMask = delaySize - 1;

    enum Generator
    {
        Saw1,
        Saw2,
        Pulse1,
        Pulse2,
        Tri1,
        Tri2,
        generatorCount
    };

    // the waveform switches and tables, which every lane shares
    bool configured{false};
    bool saw1{false}, pulse1{false}, tri1{false}, saw2{false}, pulse2{false}, tri2{false};
    const float *blepTable[generatorCount]{}, *blampTable[generatorCount]{};

    // the voice whose state each lane holds
    Voice *held[lanes]{};

    // per lane constants, named after the scalar expressions they stand for
    struct alignas(16) LaneConstants
    {
        float oscPitchNoise[lanes], pitch1[lanes], pitch2[lanes], tune[lanes], transpose[lanes],
            unisonDetune[lanes], slop1[lanes], slop2[lanes], detune[lanes], keytrack[lanes],
            notKeytrack[lanes], crossmod[lanes], pw[lanes], sync[lanes], mix1[lanes], mix2[lanes],
            mixNoise[lanes], ringMod[lanes], level[lanes], sampleRateInv[lanes];
    } lc;

    // held state; rings and delay lines are read relative to blepPos and delayPos
    struct alignas(16) State
    {
        float phase1[lanes], phase2[lanes], pw1[lanes], pw2[lanes], pitch1[lanes], pitch2[lanes];
        float pw1t[2][lanes];
        float blep[generatorCount][ringSize][lanes];
        float genDelay[generatorCount][delaySize][lanes];
        float sync[delaySize][lanes], syncFrac[delaySize][lanes], crossmod[delaySize][lanes],
            pitch[delaySize][lanes];
    } st{};

    struct alignas(16) Rows
    {
        float noise1[maxSubs][lanes], noise2[maxSubs][lanes], noiseColor[maxSubs][lanes];
    } noiseRows;

    int blepPos{0}, delayPos{0};

    template <typename F> static void withGenerator(OscillatorBlock &o, int g, F &&f)
    {
        switch (g)
        {
        case Saw1:
            f(o.gen.osc1Saw);
            break;
        case Saw2:
            f(o.gen.osc2Saw);
            break;
        case Pulse1:
            f(o.gen.osc1Pulse);
            break;
        case Pulse2:
            f(o.gen.osc2Pulse);
            break;
        case Tri1:
            f(o.gen.osc1Triangle);
            break;
        case Tri2:
            f(o.gen.osc2Triangle);
            break;
        default:
            break;
        }
    }

    bool active(int g) const
    {
        const bool on[generatorCount] = {saw1, saw2, pulse1, pulse2, tri1, tri2};
        return on[g];
    }

    static bool compatible(Voice *const v[lanes], int laneCount)
    {
        const auto &a = v[0]->oscs;

        for (int l = 1; l < laneCount; ++l)
        {
            const auto &b = v[l]->oscs;

            if (b.par.osc.saw1 != a.par.osc.saw1 || b.par.osc.saw2 != a.par.osc.saw2 ||
                b.par.osc.pulse1 != a.par.osc.pulse1 || b.par.osc.pulse2 != a.par.osc.pulse2 ||
                b.gen.osc1Saw.blepPtr != a.gen.osc1Saw.blepPtr ||
                b.gen.osc2Saw.blepPtr != a.gen.osc2Saw.blepPtr ||
                b.gen.osc1Pulse.blepPtr != a.gen.osc1Pulse.blepPtr ||
                b.gen.osc2Pulse.blepPtr != a.gen.osc2Pulse.blepPtr ||
                b.gen.osc1Triangle.blepPtr != a.gen.osc1Triangle.blepPtr ||
                b.gen.osc2Triangle.blepPtr != a.gen.osc2Triangle.blepPtr ||
                b.gen.osc1Triangle.blampPtr != a.gen.osc1Triangle.blampPtr ||
                b.gen.osc2Triangle.blampPtr != a.gen.osc2Triangle.blampPtr)
            {
                return false;
            }
        }

        return true;
    }

    bool sameConfig(OscillatorBlock &o) const
    {
        if (o.par.osc.saw1 != saw1 || o.par.osc.pulse1 != pulse1 || o.par.osc.saw2 != saw2 ||
            o.par.osc.pulse2 != pulse2 || o.gen.osc1Triangle.blampPtr != blampTable[Tri1] ||
            o.gen.osc2Triangle.blampPtr != blampTable[Tri2])
        {
            return false;
        }

        bool same = true;

        for (int g = 0; g < generatorCount; ++g)
        {
            withGenerator(o, g, [&](auto &gen) { same = same && gen.blepPtr == blepTable[g]; });
        }

        return same;
    }

    void takeConfig(OscillatorBlock &o)
    {
        saw1 = o.par.osc.saw1;
        pulse1 = o.par.osc.pulse1;
        tri1 = !saw1 && !pulse1;
        saw2 = o.par.osc.saw2;
        pulse2 = o.par.osc.pulse2;
        tri2 = !saw2 && !pulse2;

        for (int g = 0; g < generatorCount; ++g)
        {
            withGenerator(o, g, [&](auto &gen) { blepTable[g] = gen.blepPtr; });
        }

        blampTable[Tri1] = o.gen.osc1Triangle.blampPtr;
        blampTable[Tri2] = o.gen.osc2Triangle.blampPtr;

        configured = true;
    }

    // a delay line's next read is the sample fed delaySize - 1 steps before it
    static int delayAt(int pos, int age) { return (pos - age) & delayMask; }

    void loadLane(Voice &voice, int l)
    {
        auto &o = voice.oscs;

        st.phase1[l] = o.osc1.phase;
        st.phase2[l] = o.osc2.phase;
        st.pw1[l] = o.osc1.pw;
        st.pw2[l] = o.osc2.pw;
        st.pitch1[l] = o.osc1.pitch;
        st.pitch2[l] = o.osc2.pitch;
        st.pw1t[0][l] = o.gen.osc1Pulse.pw1t ? 1.f : 0.f;
        st.pw1t[1][l] = o.gen.osc2Pulse.pw1t ? 1.f : 0.f;

        for (int age = 1; age < delaySize; ++age)
        {
            const int at = delayAt(delayPos, age);
            st.sync[at][l] = o.delay.sync.history(age) ? 1.f : 0.f;
            st.syncFrac[at][l] = o.delay.syncFrac.history(age);
            st.crossmod[at][l] = o.delay.crossmod.history(age);
            st.pitch[at][l] = o.delay.pitch.history(age);
        }

        for (int g = 0; g < generatorCount; ++g)
        {
            if (!active(g))
            {
                continue;
            }

            withGenerator(o, g, [&](auto &gen) {
                for (int j = 0; j < ringSize; ++j)
                {
                    st.blep[g][(blepPos + j) & ringMask][l] =
                        gen.buffer[(gen.bufferPos + j) & ringMask];
                }

                for (int age = 1; age < delaySize; ++age)
                {
                    st.genDelay[g][delayAt(delayPos, age)][l] = gen.delay.history(age);
                }
            });
        }

        held[l] = &voice;
    }

    // back is how many samples before the group's position this lane stopped
    void unloadLane(int l, int back = 0)
    {
        auto &o = held[l]->oscs;
        const int bpos = blepPos - back, dpos = delayPos - back;

        o.osc1.phase = st.phase1[l];
        o.osc2.phase = st.phase2[l];
        o.osc1.pw = st.pw1[l];
        o.osc2.pw = st.pw2[l];
        o.osc1.pitch = st.pitch1[l];
        o.osc2.pitch = st.pitch2[l];
        o.gen.osc1Pulse.pw1t = st.pw1t[0][l] != 0.f;
        o.gen.osc2Pulse.pw1t = st.pw1t[1][l] != 0.f;

        for (int age = 1; age < delaySize; ++age)
        {
            const int at = delayAt(dpos, age);
            o.delay.sync.setHistory(age, st.sync[at][l] != 0.f);
            o.delay.syncFrac.setHistory(age, st.syncFrac[at][l]);
            o.delay.crossmod.setHistory(age, st.crossmod[at][l]);
            o.delay.pitch.setHistory(age, st.pitch[at][l]);
        }

        for (int g = 0; g < generatorCount; ++g)
        {
            if (!active(g))
            {
                continue;
            }

            withGenerator(o, g, [&](auto &gen) {
                for (int j = 0; j < ringSize; ++j)
                {
                    gen.buffer[j] = st.blep[g][(bpos + j) & ringMask][l];
                }
                gen.bufferPos = 0;

                for (int age = 1; age < delaySize; ++age)
                {
                    gen.delay.setHistory(age, st.genDelay[g][delayAt(dpos, age)][l]);
                }
            });
        }

        held[l] = nullptr;
        clearLane(l);
    }

    void clearLane(int l)
    {
        st.phase1[l] = st.phase2[l] = st.pw1[l] = st.pw2[l] = st.pitch1[l] = st.pitch2[l] = 0.f;
        st.pw1t[0][l] = st.pw1t[1][l] = 0.f;

        for (int j = 0; j < delaySize; ++j)
        {
            st.sync[j][l] = st.syncFrac[j][l] = st.crossmod[j][l] = st.pitch[j][l] = 0.f;
        }

        for (int g = 0; g < generatorCount; ++g)
        {
            for (int j = 0; j < ringSize; ++j)
            {
                st.blep[g][j][l] = 0.f;
            }

            for (int j = 0; j < delaySize; ++j)
            {
                st.genDelay[g][j][l] = 0.f;
            }
        }
    }

    void setLaneConstants(const Voice &voice, int l)
    {
        const auto &o = voice.oscs;
        const auto &adj = voice.matrixAdjustments;

        lc.oscPitchNoise[l] = o.par.mod.oscPitchNoise;
        lc.pitch1[l] = o.par.osc.pitch1;
        lc.pitch2[l] = o.par.osc.pitch2;
        lc.tune[l] = o.par.pitch.tune;
        lc.transpose[l] = static_cast<float>(o.par.pitch.transpose);
        lc.unisonDetune[l] = adj.nativeOr(MatrixTarget::UnisonDetune, o.par.pitch.unisonDetune);
        lc.slop1[l] = o.osc1.tuningSlop;
        lc.slop2[l] = o.osc2.tuningSlop;
        lc.detune[l] = adj.nativeOr(MatrixTarget::Osc2Detune, o.par.osc.detune);
        lc.keytrack[l] = static_cast<float>(o.par.osc.keytrack2);
        lc.notKeytrack[l] = 1.f - o.par.osc.keytrack2;
        lc.crossmod[l] = adj.nativeOr(MatrixTarget::OscCrossmod, o.par.osc.crossmod);
        lc.pw[l] = adj.nativeOr(MatrixTarget::OscPW, o.par.osc.pw);
        lc.sync[l] = o.par.osc.sync ? obxf_simd::allBits() : 0.f;
        lc.mix1[l] = adj.nativeOr(MatrixTarget::Osc1Vol, o.par.mix.osc1);
        lc.mix2[l] = adj.nativeOr(MatrixTarget::Osc2Vol, o.par.mix.osc2);
        lc.mixNoise[l] = adj.nativeOr(MatrixTarget::NoiseVol, o.par.mix.noise) + 0.0006f;
        lc.ringMod[l] = adj.nativeOr(MatrixTarget::RingModVol, o.par.mix.ringMod);
        lc.level[l] = 1 - voice.par.slop.level * voice.slop.level;
        lc.sampleRateInv[l] = o.sampleRateInv;
    }

    void fillNoise(Voice *const v[lanes], int laneCount, int n)
    {
        const int color = v[0]->oscs.par.mix.noiseColor;
        bool together = color != OscillatorBlock::Pink;
        Noise *noise[lanes]{};
        alignas(16) float played[lanes]{};

        for (int l = 0; l < laneCount; ++l)
        {
            together = together && v[l]->oscs.par.mix.noiseColor == color;
            noise[l] = &v[l]->oscs.gen.noise;
            played[l] = static_cast<float>(v[l]->blockSubSamples);
        }

        if (!together)
        {
            fillNoiseByLane(v, laneCount, n);
            return;
        }

        // white and red noise run four voices at a time, each as its own Noise would
        NoiseQuad nq;
        nq.load(noise, laneCount);

        const auto playedV = SIMD_MM(load_ps)(played);

        for (int s = 0; s < n; ++s)
        {
            const auto active = SIMD_MM(cmplt_ps)(SIMD_MM(set1_ps)(static_cast<float>(s)), playedV);

            SIMD_M128 w1, w2, w3;
            nq.whiteThree(active, w1, w2, w3);

            SIMD_MM(store_ps)(noiseRows.noise1[s], w1);
            SIMD_MM(store_ps)(noiseRows.noise2[s], w2);
            SIMD_MM(store_ps)(noiseRows.noiseColor[s],
                              color == OscillatorBlock::White ? w3 : nq.redFrom(w3, active));
        }

        nq.store(noise, laneCount);
    }

    // samples outside, lanes inside, so the lanes' serial generators overlap
    void fillNoiseByLane(Voice *const v[lanes], int laneCount, int n)
    {
        for (int s = 0; s < n; ++s)
        {
            for (int l = 0; l < lanes; ++l)
            {
                if (l >= laneCount || s >= v[l]->blockSubSamples)
                {
                    noiseRows.noise1[s][l] = noiseRows.noise2[s][l] = noiseRows.noiseColor[s][l] =
                        0.f;
                    continue;
                }

                auto &o = v[l]->oscs;
                const auto color = OscillatorBlock::noiseColorFns[o.par.mix.noiseColor];

                noiseRows.noise1[s][l] = o.gen.noise.getWhite();
                noiseRows.noise2[s][l] = o.gen.noise.getWhite();
                noiseRows.noiseColor[s][l] = (o.gen.noise.*color)();
            }
        }
    }

    // the generators' mixIn functions, writing one lane of the shared ring
    void mixInImpulseCenter(int g, int l, float offset, float scale)
    {
        int lpIn = static_cast<int>(B_OVERSAMPLING * offset);
        if (lpIn >= B_OVERSAMPLING)
            lpIn = B_OVERSAMPLING - 1;

        const float frac = offset * B_OVERSAMPLING - static_cast<float>(lpIn);
        const float f1s = (1.0f - frac) * scale;
        const float fracs = frac * scale;

        const float *rowA = blepTable[g] + lpIn * B_SAMPLESx2;
        const float *rowB = rowA + B_SAMPLESx2;

        auto &ring = st.blep[g];

        for (int i = 0; i < B_SAMPLES; i++)
            ring[(blepPos + i) & ringMask][l] += rowA[i] * f1s + rowB[i] * fracs;

        for (int i = B_SAMPLES; i < B_SAMPLESx2; i++)
            ring[(blepPos + i) & ringMask][l] -= rowA[i] * f1s + rowB[i] * fracs;
    }

    void mixInBlampCenter(int g, int l, float offset, float scale)
    {
        int lpIn = static_cast<int>(B_OVERSAMPLING * offset);
        if (lpIn >= B_OVERSAMPLING)
            lpIn = B_OVERSAMPLING - 1;

        const float frac = offset * B_OVERSAMPLING - static_cast<float>(lpIn);
        const float f1s = (1.0f - frac) * scale;
        const float fracs = frac * scale;

        const float *rowA = blampTable[g] + lpIn * B_SAMPLESx2;
        const float *rowB = rowA + B_SAMPLESx2;

        auto &ring = st.blep[g];

        for (int i = 0; i < B_SAMPLESx2; i++)
            ring[(blepPos + i) & ringMask][l] += rowA[i] * f1s + rowB[i] * fracs;
    }

    // the generators' edge logic for one lane, as SawOsc, PulseOsc and TriangleOsc have it
    void sawLeader(int g, int l, float x, float delta)
    {
        if (x >= 1.0f)
        {
            x -= 1.0f;
            mixInImpulseCenter(g, l, x / delta, 1);
        }
    }

    void sawFollower(int g, int l, float x, float delta, bool hardSyncReset, float hardSyncFrac)
    {
        if (x >= 1.0f)
        {
            x -= 1.0f;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                mixInImpulseCenter(g, l, x / delta, 1);
            }
            else
            {
                x += 1;
            }
        }

        if (hardSyncReset)
        {
            float fracMaster = (delta * hardSyncFrac);
            float trans = (x - fracMaster);

            mixInImpulseCenter(g, l, hardSyncFrac, trans);
        }
    }

    void pulseLeader(int g, int l, float x, float delta, float pulseWidth, float pulseWidthWas,
                     bool &pw1t)
    {
        float sum = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;
            mixInImpulseCenter(g, l, x / delta, 1);
            pw1t = false;
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - sum <= pulseWidth))
        {
            float frac = (x - pulseWidth) / sum;
            pw1t = true;
            mixInImpulseCenter(g, l, frac, -1.f);
        }

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;
            mixInImpulseCenter(g, l, x / delta, 1.f);
            pw1t = false;
        }
    }

    void pulseFollower(int g, int l, float x, float delta, bool hardSyncReset, float hardSyncFrac,
                       float pulseWidth, float pulseWidthWas, bool &pw1t)
    {
        float sum = delta - (pulseWidth - pulseWidthWas);

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                mixInImpulseCenter(g, l, x / delta, 1.f);
                pw1t = false;
            }
            else
            {
                x += 1.f;
            }
        }

        if ((!pw1t) && (x >= pulseWidth) && (x - sum <= pulseWidth))
        {
            float frac = (x - pulseWidth) / sum;

            pw1t = true;

            if (((!hardSyncReset) || (frac > hardSyncFrac)))
            {
                mixInImpulseCenter(g, l, frac, -1.f);
            }
            else
            {
                pw1t = false;
            }
        }

        if ((pw1t) && x >= 1.f)
        {
            x -= 1.f;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                mixInImpulseCenter(g, l, x / delta, 1.f);
                pw1t = false;
            }
            else
            {
                x += 1.f;
            }
        }

        if (hardSyncReset)
        {
            float trans = (pw1t ? 1.f : 0.f);
            mixInImpulseCenter(g, l, hardSyncFrac, trans);
            pw1t = false;
        }
    }

    void triLeader(int g, int l, float x, float delta)
    {
        if (x >= 1.0)
        {
            x -= 1.0;
            mixInBlampCenter(g, l, x / delta, -4 * B_SAMPLES * delta);
        }

        if (x >= 0.5 && x - delta < 0.5)
        {
            mixInBlampCenter(g, l, (x - 0.5) / delta, 4 * B_SAMPLES * delta);
        }

        if (x >= 1.0)
        {
            x -= 1.0;
            mixInBlampCenter(g, l, x / delta, -4 * B_SAMPLES * delta);
        }
    }

    void triFollower(int g, int l, float x, float delta, bool hardSyncReset, float hardSyncFrac)
    {
        bool hspass = true;

        if (x >= 1.0)
        {
            x -= 1.0;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                mixInBlampCenter(g, l, x / delta, -4 * B_SAMPLES * delta);
            }
            else
            {
                x += 1;
                hspass = false;
            }
        }

        if (x >= 0.5 && x - delta < 0.5 && hspass)
        {
            float frac = (x - 0.5) / delta;

            if (((!hardSyncReset) || (frac > hardSyncFrac)))
            {
                mixInBlampCenter(g, l, frac, 4 * B_SAMPLES * delta);
            }
        }

        if (x >= 1.0 && hspass)
        {
            x -= 1.0;

            if (((!hardSyncReset) || (x / delta > hardSyncFrac)))
            {
                mixInBlampCenter(g, l, x / delta, -4 * B_SAMPLES * delta);
            }
            else
            {
                x += 1;
            }
        }

        if (hardSyncReset)
        {
            float fracMaster = (delta * hardSyncFrac);
            float trans = (x - fracMaster);
            float mix = 0.5f - 2.f * std::fabs(trans - 0.5f);

            if (trans > 0.5)
            {
                mixInBlampCenter(g, l, hardSyncFrac, -4 * B_SAMPLES * delta);
            }

            mixInImpulseCenter(g, l, hardSyncFrac, mix + 0.5);
        }
    }

    struct EdgeArgs
    {
        alignas(16) float x[lanes], delta[lanes], pw[lanes], pwWas[lanes], hsr[lanes], hsf[lanes];
    };

    // calls f(lane) for each lane set in mask
    template <typename F> static void forLanes(SIMD_M128 mask, F &&f)
    {
        int bits = SIMD_MM(movemask_ps)(mask);

        while (bits)
        {
            const int l = std::countr_zero(static_cast<unsigned>(bits));
            f(l);
            bits &= bits - 1;
        }
    }

    // masked runs lanes of different lengths, each lane held still once it has played
    template <bool masked> void run(GroupRows &r, int n, SIMD_M128 playedV)
    {
        using namespace obxf_simd;

        auto ld = [](const float *x) { return SIMD_MM(load_ps)(x); };
        auto set1 = [](float f) { return SIMD_MM(set1_ps)(f); };
        auto add = [](SIMD_M128 a, SIMD_M128 b) { return SIMD_MM(add_ps)(a, b); };
        auto sub = [](SIMD_M128 a, SIMD_M128 b) { return SIMD_MM(sub_ps)(a, b); };
        auto mul = [](SIMD_M128 a, SIMD_M128 b) { return SIMD_MM(mul_ps)(a, b); };

        const auto zero = set1(0.f), one = set1(1.f), half = set1(0.5f), two = set1(2.f),
                   three = set1(3.f), tenth = set1(0.1f), maxFs = set1(0.45f),
                   minus33 = set1(-33.f), signBit = set1(-0.f), a440 = set1(440.f),
                   pitchMult = set1(mult), twelfth = set1(1.f / 12.f);

        const auto oscPitchNoiseV = ld(lc.oscPitchNoise), pitch1V = ld(lc.pitch1),
                   pitch2V = ld(lc.pitch2), tuneV = ld(lc.tune), transposeV = ld(lc.transpose),
                   unisonDetuneV = ld(lc.unisonDetune), slop1V = ld(lc.slop1),
                   slop2V = ld(lc.slop2), detuneV = ld(lc.detune), keytrackV = ld(lc.keytrack),
                   notKeytrackV = ld(lc.notKeytrack), crossmodV = ld(lc.crossmod), pwV = ld(lc.pw),
                   syncV = ld(lc.sync), mix1V = ld(lc.mix1), mix2V = ld(lc.mix2),
                   mixNoiseV = ld(lc.mixNoise), ringModV = ld(lc.ringMod), levelV = ld(lc.level),
                   sampleRateInvV = ld(lc.sampleRateInv);

        auto phase1 = ld(st.phase1), phase2 = ld(st.phase2), pw1Was = ld(st.pw1),
             pw2Was = ld(st.pw2), pitch1 = ld(st.pitch1), pitch2 = ld(st.pitch2);

        // libm exp per lane keeps getPitch exactly as the scalar oscillators have it
        auto getPitch = [&](SIMD_M128 index) {
            if constexpr (exactExp)
            {
                alignas(16) float xs[lanes];
                SIMD_MM(store_ps)(xs, mul(pitchMult, index));
                for (auto &x : xs)
                {
                    x = std::exp(x);
                }
                return mul(a440, ld(xs));
            }
            else
            {
                return mul(a440, obxf_simd::exp2(mul(index, twelfth)));
            }
        };

        auto active = SIMD_MM(cmpeq_ps)(zero, zero);

        auto keep = [&](SIMD_M128 was, SIMD_M128 now) {
            if constexpr (masked)
                return blend(was, now, active);
            else
                return now;
        };

        auto onlyActive = [&](SIMD_M128 mask) {
            if constexpr (masked)
                return SIMD_MM(and_ps)(mask, active);
            else
                return mask;
        };

        auto feed = [&](float (*ring)[lanes], SIMD_M128 x) {
            SIMD_MM(store_ps)(ring[delayPos], keep(ld(ring[delayPos]), x));
            return ld(ring[(delayPos + 1) & delayMask]);
        };

        auto aliasReduction = [&](int g) {
            SIMD_MM(store_ps)(st.blep[g][blepPos], keep(ld(st.blep[g][blepPos]), zero));
            return SIMD_MM(xor_ps)(ld(st.blep[g][(blepPos + 1) & ringMask]), signBit);
        };

        auto pulseValue = [&](int g, SIMD_M128 x, SIMD_M128 pw) {
            const auto high = SIMD_MM(and_ps)(SIMD_MM(cmpge_ps)(x, pw), one);
            return feed(st.genDelay[g], add(sub(pw, one), high));
        };

        auto sawValue = [&](int g, SIMD_M128 x) { return feed(st.genDelay[g], sub(x, half)); };

        auto triValue = [&](int g, SIMD_M128 x) {
            const auto ax = SIMD_MM(andnot_ps)(signBit, sub(x, half));
            return feed(st.genDelay[g], negMulAdd(two, ax, half));
        };

        auto wrapped = [&](SIMD_M128 x) { return SIMD_MM(cmpge_ps)(x, one); };

        // lanes where a triangle may cross its midpoint this sample
        auto triEdge = [&](SIMD_M128 x, SIMD_M128 delta) {
            return SIMD_MM(or_ps)(wrapped(x),
                                  SIMD_MM(and_ps)(SIMD_MM(cmpge_ps)(x, half),
                                                  SIMD_MM(cmplt_ps)(sub(x, delta), half)));
        };

        // lanes where a pulse may change level this sample
        auto pulseEdge = [&](SIMD_M128 x, SIMD_M128 delta, SIMD_M128 pw, SIMD_M128 pwWas) {
            const auto sum = sub(delta, sub(pw, pwWas));
            return SIMD_MM(or_ps)(wrapped(x), SIMD_MM(and_ps)(SIMD_MM(cmpge_ps)(x, pw),
                                                              SIMD_MM(cmple_ps)(sub(x, sum), pw)));
        };

        EdgeArgs e;

        auto storeEdge = [&](SIMD_M128 x, SIMD_M128 delta, SIMD_M128 pw, SIMD_M128 pwWas,
                             SIMD_M128 hsr, SIMD_M128 hsf) {
            SIMD_MM(store_ps)(e.x, x);
            SIMD_MM(store_ps)(e.delta, delta);
            SIMD_MM(store_ps)(e.pw, pw);
            SIMD_MM(store_ps)(e.pwWas, pwWas);
            SIMD_MM(store_ps)(e.hsr, hsr);
            SIMD_MM(store_ps)(e.hsf, hsf);
        };

        auto syncAt = [&](int l) {
            uint32_t bits;
            std::memcpy(&bits, &e.hsr[l], sizeof(bits));
            return bits != 0;
        };

        auto withPw1t = [&](int which, int l, auto &&f) {
            bool pw1t = st.pw1t[which][l] != 0.f;
            f(pw1t);
            st.pw1t[which][l] = pw1t ? 1.f : 0.f;
        };

        for (int s = 0; s < n; ++s)
        {
            if constexpr (masked)
                active = SIMD_MM(cmplt_ps)(set1(static_cast<float>(s)), playedV);

            const auto note = ld(r.notePlaying[s]), pw1Mod = ld(r.osc1PWMod[s]),
                       pw2Mod = ld(r.osc2PWMod[s]), pitch1Mod = ld(r.osc1PitchMod[s]),
                       pitch2Mod = ld(r.osc2PitchMod[s]);

            // oscillator 1
            auto p1 = mulAdd(oscPitchNoiseV, ld(noiseRows.noise1[s]), note);
            p1 = add(p1, pitch1V);
            p1 = add(p1, pitch1Mod);
            p1 = add(p1, tuneV);
            p1 = add(p1, transposeV);
            p1 = mulAdd(unisonDetuneV, slop1V, p1);
            const auto pitch1Now = getPitch(p1);

            const auto fs1 = SIMD_MM(min_ps)(mul(pitch1Now, sampleRateInvV), maxFs);
            auto ph1 = add(phase1, fs1);

            const auto pwcalc1 =
                SIMD_MM(max_ps)(tenth, SIMD_MM(min_ps)(one, mulAdd(add(pwV, pw1Mod), half, half)));

            if (pulse1)
            {
                const auto edge = onlyActive(pulseEdge(ph1, fs1, pwcalc1, pw1Was));
                if (SIMD_MM(movemask_ps)(edge))
                {
                    storeEdge(ph1, fs1, pwcalc1, pw1Was, zero, zero);
                    forLanes(edge, [&](int l) {
                        withPw1t(0, l, [&](bool &pw1t) {
                            pulseLeader(Pulse1, l, e.x[l], e.delta[l], e.pw[l], e.pwWas[l], pw1t);
                        });
                    });
                }
            }

            if (saw1)
            {
                const auto edge = onlyActive(wrapped(ph1));
                if (SIMD_MM(movemask_ps)(edge))
                {
                    storeEdge(ph1, fs1, zero, zero, zero, zero);
                    forLanes(edge, [&](int l) { sawLeader(Saw1, l, e.x[l], e.delta[l]); });
                }
            }
            else if (tri1)
            {
                const auto edge = onlyActive(triEdge(ph1, fs1));
                if (SIMD_MM(movemask_ps)(edge))
                {
                    storeEdge(ph1, fs1, zero, zero, zero, zero);
                    forLanes(edge, [&](int l) { triLeader(Tri1, l, e.x[l], e.delta[l]); });
                }
            }

            const auto wrap1 = wrapped(ph1);
            ph1 = blend(ph1, sub(ph1, one), wrap1);
            auto syncFrac = SIMD_MM(and_ps)(SIMD_MM(div_ps)(ph1, fs1), wrap1);

            // delaying the hard sync gate and frac
            auto syncReset = SIMD_MM(and_ps)(SIMD_MM(and_ps)(wrap1, syncV), one);
            syncReset = SIMD_MM(cmpneq_ps)(feed(st.sync, syncReset), zero);
            syncFrac = feed(st.syncFrac, syncFrac);

            auto osc1out = zero;

            if (pulse1)
            {
                osc1out =
                    add(osc1out, add(pulseValue(Pulse1, ph1, pwcalc1), aliasReduction(Pulse1)));
            }

            if (saw1)
            {
                osc1out = add(osc1out, add(sawValue(Saw1, ph1), aliasReduction(Saw1)));
            }
            else if (tri1)
            {
                osc1out = add(triValue(Tri1, ph1), aliasReduction(Tri1));
            }

            // oscillator 2, its pitch delayed to line up with the crossmod
            auto p2 = mulAdd(oscPitchNoiseV, ld(noiseRows.noise2[s]), mul(note, keytrackV));
            p2 = mulAdd(minus33, notKeytrackV, p2);
            p2 = add(p2, detuneV);
            p2 = add(p2, pitch2V);
            p2 = add(p2, pitch2Mod);
            p2 = mulAdd(osc1out, crossmodV, p2);
            p2 = add(p2, tuneV);
            p2 = add(p2, transposeV);
            p2 = mulAdd(unisonDetuneV, slop2V, p2);
            const auto pitch2Now = getPitch(feed(st.pitch, p2));

            const auto fs2 = SIMD_MM(min_ps)(mul(pitch2Now, sampleRateInvV), maxFs);
            const auto pwcalc2 =
                SIMD_MM(max_ps)(tenth, SIMD_MM(min_ps)(one, mulAdd(add(pwV, pw2Mod), half, half)));

            auto ph2 = add(phase2, fs2);

            if (pulse2)
            {
                const auto edge =
                    onlyActive(SIMD_MM(or_ps)(pulseEdge(ph2, fs2, pwcalc2, pw2Was), syncReset));
                if (SIMD_MM(movemask_ps)(edge))
                {
                    storeEdge(ph2, fs2, pwcalc2, pw2Was, syncReset, syncFrac);
                    forLanes(edge, [&](int l) {
                        withPw1t(1, l, [&](bool &pw1t) {
                            pulseFollower(Pulse2, l, e.x[l], e.delta[l], syncAt(l), e.hsf[l],
                                          e.pw[l], e.pwWas[l], pw1t);
                        });
                    });
                }
            }

            if (saw2)
            {
                const auto edge = onlyActive(SIMD_MM(or_ps)(wrapped(ph2), syncReset));
                if (SIMD_MM(movemask_ps)(edge))
                {
                    storeEdge(ph2, fs2, zero, zero, syncReset, syncFrac);
                    forLanes(edge, [&](int l) {
                        sawFollower(Saw2, l, e.x[l], e.delta[l], syncAt(l), e.hsf[l]);
                    });
                }
            }
            else if (tri2)
            {
                const auto edge = onlyActive(SIMD_MM(or_ps)(triEdge(ph2, fs2), syncReset));
                if (SIMD_MM(movemask_ps)(edge))
                {
                    storeEdge(ph2, fs2, zero, zero, syncReset, syncFrac);
                    forLanes(edge, [&](int l) {
                        triFollower(Tri2, l, e.x[l], e.delta[l], syncAt(l), e.hsf[l]);
                    });
                }
            }

            ph2 = blend(ph2, sub(ph2, one), wrapped(ph2));

            // on hard sync reset, follower phase is affected that way
            ph2 = blend(ph2, mul(fs2, syncFrac), syncReset);

            osc1out = feed(st.crossmod, osc1out);

            auto osc2out = zero;

            if (pulse2)
            {
                osc2out =
                    add(osc2out, add(pulseValue(Pulse2, ph2, pwcalc2), aliasReduction(Pulse2)));
            }

            if (saw2)
            {
                osc2out = add(osc2out, add(sawValue(Saw2, ph2), aliasReduction(Saw2)));
            }
            else if (tri2)
            {
                osc2out = add(triValue(Tri2, ph2), aliasReduction(Tri2));
            }

            const auto rmOut = mul(osc1out, osc2out);

            auto mixed = mulAdd(osc1out, mix1V, mul(osc2out, mix2V));
            mixed = mulAdd(ld(noiseRows.noiseColor[s]), mixNoiseV, mixed);
            mixed = mulAdd(rmOut, ringModV, mixed);

            SIMD_MM(store_ps)(r.out[s], mul(mul(mixed, three), levelV));

            phase1 = keep(phase1, ph1);
            phase2 = keep(phase2, ph2);
            pw1Was = keep(pw1Was, pwcalc1);
            pw2Was = keep(pw2Was, pwcalc2);
            pitch1 = keep(pitch1, pitch1Now);
            pitch2 = keep(pitch2, pitch2Now);

            delayPos = (delayPos + 1) & delayMask;
            blepPos = (blepPos + 1) & ringMask;
        }

        SIMD_MM(store_ps)(st.phase1, phase1);
        SIMD_MM(store_ps)(st.phase2, phase2);
        SIMD_MM(store_ps)(st.pw1, pw1Was);
        SIMD_MM(store_ps)(st.pw2, pw2Was);
        SIMD_MM(store_ps)(st.pitch1, pitch1);
        SIMD_MM(store_ps)(st.pitch2, pitch2);
    }
};

#endif // OBXF_SRC_ENGINE_OSCILLATORQUAD_H
