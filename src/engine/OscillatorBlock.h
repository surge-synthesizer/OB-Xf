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

#ifndef OBXF_SRC_ENGINE_OSCILLATORBLOCK_H
#define OBXF_SRC_ENGINE_OSCILLATORBLOCK_H

#include "Voice.h"
#include "SynthEngine.h"
#include "AudioUtils.h"
#include "BlepData.h"
#include "DelayLine.h"
#include "Noise.h"
#include "SawOsc.h"
#include "PulseOsc.h"
#include "TriangleOsc.h"

class OscillatorBlock
{
  private:
    float sampleRate;
    float pitch1;
    float pitch2;
    float sampleRateInv;

    float x1, x2;

    float osc1Factor;
    float osc2Factor;

    float pw1w, pw2w;

    // delay line implements fixed sample delay
    DelayLine<Samples> del1, del2;
    DelayLine<Samples> xmodd;
    DelayLineBoolean<Samples> syncd;
    DelayLine<Samples> syncFracd;
    DelayLine<Samples> cvd;
    Noise noise;
    SawOsc o1s, o2s;
    PulseOsc o1p, o2p;
    TriangleOsc o1t, o2t;

  public:
    float tune; // +/- 1
    int oct;

    float dirt;

    float notePlaying;

    float totalDetune;

    float osc2Det;
    float pulseWidth;
    float pw1, pw2;

    bool penvinv, pwenvinv;

    float osc1Mix, osc2Mix;
    float ringModMix, noiseMix, noiseColor;
    float pto1, pto2;

    float osc1Saw, osc2Saw, osc1Pul, osc2Pul;

    float osc1p, osc2p;
    bool hardSync;
    float xmod;

    OscillatorBlock() : o1s(), o2s(), o1p(), o2p(), o1t(), o2t()
    {
        dirt = 0.1f;
        totalDetune = 0.f;
        ringModMix = noiseMix = noiseColor = 0.f;
        oct = 0;
        tune = 0.f;
        pw1w = pw2w = 0.f;
        pto1 = pto2 = 0.f;
        pw1 = pw2 = 0.f;
        xmod = 0.f;
        hardSync = false;
        osc1p = osc2p = 10.f;
        osc1Saw = osc2Saw = osc1Pul = osc2Pul = false;
        osc2Det = 0.f;
        notePlaying = 30.f;
        pulseWidth = 0.f;
        osc1Mix = osc2Mix = 0.f;
        x1 = x2 = 0.f;
    }

    ~OscillatorBlock() {}

    void setDecimation()
    {
        o1p.setDecimation();
        o1t.setDecimation();
        o1s.setDecimation();
        o2p.setDecimation();
        o2t.setDecimation();
        o2s.setDecimation();
    }

    void removeDecimation()
    {
        o1p.removeDecimation();
        o1t.removeDecimation();
        o1s.removeDecimation();
        o2p.removeDecimation();
        o2t.removeDecimation();
        o2s.removeDecimation();
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.f / sampleRate;

        noise.setSampleRate(sampleRate);
        noise.seedWhiteNoise(std::rand());

        osc1Factor = noise.getWhiteNoiseSample() * 0.5f;
        osc2Factor = noise.getWhiteNoiseSample() * 0.5f;

        x1 = (noise.getWhiteNoiseSample() * 0.5f) + 0.5f;
        x2 = (noise.getWhiteNoiseSample() * 0.5f) + 0.5f;
    }

    inline float ProcessSample()
    {
        float noiseGen = noise.getWhiteNoiseSample();

        pitch1 = getPitch(dirt * noiseGen + notePlaying + osc1p + pto1 + tune + oct +
                          totalDetune * osc1Factor);
        bool hsr = false;
        float hsfrac = 0.f;
        float fs = juce::jmin(pitch1 * (sampleRateInv), 0.45f);

        x1 += fs;
        hsfrac = 0.f;

        float osc1out = 0.f;
        float pwcalc = juce::jlimit<float>(0.1f, 1.f, (pulseWidth + pw1) * 0.5f + 0.5f);

        if (osc1Pul)
        {
            o1p.processLeader(x1, fs, pwcalc, pw1w);
        }

        if (osc1Saw)
        {
            o1s.processLeader(x1, fs);
        }
        else if (!osc1Pul)
        {
            o1t.processLeader(x1, fs);
        }

        if (x1 >= 1.f)
        {
            x1 -= 1.f;
            hsfrac = x1 / fs;
            hsr = true;
        }

        pw1w = pwcalc;
        hsr &= hardSync;

        // Delaying our hardsync gate signal and frac
        hsr = syncd.feedReturn(hsr) != 0.f;
        hsfrac = syncFracd.feedReturn(hsfrac);

        if (osc1Pul)
        {
            osc1out += o1p.getValue(x1, pwcalc) + o1p.aliasReduction();
        }

        if (osc1Saw)
        {
            osc1out += o1s.getValue(x1) + o1s.aliasReduction();
        }
        else if (!osc1Pul)
        {
            osc1out = o1t.getValue(x1) + o1t.aliasReduction();
        }

        // pitch control needs additional delay buffer to compensate
        // this will give us less aliasing on crossmod
        // hard sync gate signal delayed too
        noiseGen = noise.getWhiteNoiseSample();

        pitch2 = getPitch(cvd.feedReturn(dirt * noiseGen + notePlaying + osc2Det + osc2p + pto2 +
                                         osc1out * xmod + tune + oct + totalDetune * osc2Factor));

        fs = juce::jmin(pitch2 * (sampleRateInv), 0.45f);

        pwcalc = juce::jlimit<float>(0.1f, 1.f, (pulseWidth + pw2) * 0.5f + 0.5f);

        float osc2out = 0.f;

        x2 += fs;

        if (osc2Pul)
        {
            o2p.processFollower(x2, fs, hsr, hsfrac, pwcalc, pw2w);
        }

        if (osc2Saw)
        {
            o2s.processFollower(x2, fs, hsr, hsfrac);
        }
        else if (!osc2Pul)
        {
            o2t.processFollower(x2, fs, hsr, hsfrac);
        }

        if (x2 >= 1.f)
        {
            x2 -= 1.f;
        }

        pw2w = pwcalc;

        // On hard sync reset slave phase is affected that way
        if (hsr)
        {
            float fracMaster = (fs * hsfrac);
            x2 = fracMaster;
        }

        // Delaying osc1 signal and getting delayed back
        osc1out = xmodd.feedReturn(osc1out);

        if (osc2Pul)
        {
            osc2out += o2p.getValue(x2, pwcalc) + o2p.aliasReduction();
        }

        if (osc2Saw)
        {
            osc2out += o2s.getValue(x2) + o2s.aliasReduction();
        }
        else if (!osc2Pul)
        {
            osc2out = o2t.getValue(x2) + o2t.aliasReduction();
        }

        float rmOut = osc1out * osc2out;

        if (noiseColor < 0.333333333333f)
        {
            noiseGen = noise.getWhiteNoiseSample();
        }
        else if (noiseColor < 0.666666666666f)
        {
            noiseGen = noise.getPinkNoiseSample();
        }
        else
        {
            noiseGen = noise.getRedNoiseSample();
        }

        // mixing
        float res = (osc1out * osc1Mix) + (osc2out * osc2Mix) + (noiseGen * (noiseMix + 0.0006f)) +
                    (rmOut * ringModMix);

        return res * 3.f;
    }
};

#endif // OBXF_SRC_ENGINE_OSCILLATORBLOCK_H
