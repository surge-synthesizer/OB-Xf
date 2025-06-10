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

#ifndef OBXF_SRC_ENGINE_MOTHERBOARD_H
#define OBXF_SRC_ENGINE_MOTHERBOARD_H

#include <climits>
#include "VoiceQueue.h"
#include "SynthEngine.h"
#include "Lfo.h"
#include "Tuning.h"

class Motherboard
{
  private:
    VoiceQueue vq;
    int totalvc;
    bool wasUni;
    bool awaitingkeys[129];
    int priorities[129];

    Decimator17 left, right;
    int asPlayedCounter;
    float lkl, lkr;
    float sampleRate, sampleRateInv;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Motherboard)

  public:
    Tuning tuning;
    bool asPlayedMode;
    Lfo mlfo, vibratoLfo;
    float vibratoAmount;
    bool vibratoEnabled;

    float Volume;
    const static int MAX_VOICES = 32;
    const static int MAX_PANNINGS = 8;
    float pannings[MAX_PANNINGS];
    ObxfVoice voices[MAX_VOICES];
    bool uni;
    bool Oversample;
    bool economyMode;

    Motherboard() : left(), right()
    {
        economyMode = true;
        lkl = lkr = 0;
        vibratoEnabled = true;
        asPlayedMode = false;
        asPlayedCounter = 0;
        for (int i = 0; i < 129; i++)
        {
            awaitingkeys[i] = false;
            priorities[i] = 0;
        }
        vibratoAmount = 0;
        Oversample = false;
        mlfo = Lfo();
        vibratoLfo = Lfo();
        vibratoLfo.waveForm = 1;
        uni = false;
        wasUni = false;
        Volume = 0;
        //	voices = new ObxfVoice* [MAX_VOICES];
        //	pannings = new float[MAX_VOICES];
        totalvc = MAX_VOICES;
        vq = VoiceQueue(MAX_VOICES, voices);
        for (int i = 0; i < MAX_PANNINGS; ++i)
        {
            pannings[i] = 0.5;
        }
    }

    ~Motherboard() {}

    void setVoiceCount(int count)
    {
        for (int i = count; i < MAX_VOICES; i++)
        {
            voices[i].NoteOff();
            voices[i].ResetEnvelope();
        }
        vq.reInit(count);
        totalvc = count;
    }

    void unisonOn()
    {
        // for(int i = 0 ; i < 110;i++)
        //	awaitingkeys[i] = false;
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1 / sampleRate;
        mlfo.setSampleRate(sr);
        vibratoLfo.setSampleRate(sr);
        for (int i = 0; i < MAX_VOICES; ++i)
        {
            voices[i].setSampleRate(sr);
        }
        SetOversample(Oversample);
    }

    void sustainOn()
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            ObxfVoice *p = vq.getNext();
            p->sustOn();
        }
    }

    void sustainOff()
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            ObxfVoice *p = vq.getNext();
            p->sustOff();
        }
    }

    void setNoteOn(int noteNo, float velocity)
    {
        asPlayedCounter++;
        priorities[noteNo] = asPlayedCounter;
        bool processed = false;
        if (wasUni != uni)
            unisonOn();
        if (uni)
        {
            if (!asPlayedMode)
            {
                int minmidi = 129;
                for (int i = 0; i < totalvc; i++)
                {
                    ObxfVoice *p = vq.getNext();
                    if (p->midiIndx < minmidi && p->Active)
                    {
                        minmidi = p->midiIndx;
                    }
                }
                if (minmidi < noteNo)
                {
                    awaitingkeys[noteNo] = true;
                }
                else
                {
                    for (int i = 0; i < totalvc; i++)
                    {
                        ObxfVoice *p = vq.getNext();
                        if (p->midiIndx > noteNo && p->Active)
                        {
                            awaitingkeys[p->midiIndx] = true;
                            p->NoteOn(noteNo, -0.5);
                        }
                        else
                        {
                            p->NoteOn(noteNo, velocity);
                        }
                    }
                }
                processed = true;
            }
            else
            {
                for (int i = 0; i < totalvc; i++)
                {
                    ObxfVoice *p = vq.getNext();
                    if (p->Active)
                    {
                        awaitingkeys[p->midiIndx] = true;
                        p->NoteOn(noteNo, -0.5);
                    }
                    else
                    {
                        p->NoteOn(noteNo, velocity);
                    }
                }
                processed = true;
            }
        }
        else
        {
            for (int i = 0; i < totalvc && !processed; i++)
            {
                ObxfVoice *p = vq.getNext();
                if (!p->Active)
                {
                    p->NoteOn(noteNo, velocity);
                    processed = true;
                }
            }
        }
        // if voice steal occured
        if (!processed)
        {
            //
            if (!asPlayedMode)
            {
                int maxmidi = 0;
                ObxfVoice *highestVoiceAvalible = NULL;
                for (int i = 0; i < totalvc; i++)
                {
                    ObxfVoice *p = vq.getNext();
                    if (p->midiIndx > maxmidi)
                    {
                        maxmidi = p->midiIndx;
                        highestVoiceAvalible = p;
                    }
                }
                if (maxmidi < noteNo)
                {
                    awaitingkeys[noteNo] = true;
                }
                else
                {
                    highestVoiceAvalible->NoteOn(noteNo, -0.5);
                    awaitingkeys[maxmidi] = true;
                }
            }
            else
            {
                int minPriority = INT_MAX;
                ObxfVoice *minPriorityVoice = NULL;
                for (int i = 0; i < totalvc; i++)
                {
                    ObxfVoice *p = vq.getNext();
                    if (priorities[p->midiIndx] < minPriority)
                    {
                        minPriority = priorities[p->midiIndx];
                        minPriorityVoice = p;
                    }
                }
                awaitingkeys[minPriorityVoice->midiIndx] = true;
                minPriorityVoice->NoteOn(noteNo, -0.5);
            }
        }
        wasUni = uni;
    }

    void setNoteOff(int noteNo)
    {
        awaitingkeys[noteNo] = false;
        int reallocKey = 0;
        // Voice release case
        if (!asPlayedMode)
        {
            while (reallocKey < 129 && (!awaitingkeys[reallocKey]))
            {
                reallocKey++;
            }
        }
        else
        {
            reallocKey = 129;
            int maxPriority = INT_MIN;
            for (int i = 0; i < 129; i++)
            {
                if (awaitingkeys[i] && (maxPriority < priorities[i]))
                {
                    reallocKey = i;
                    maxPriority = priorities[i];
                }
            }
        }
        if (reallocKey != 129)
        {
            for (int i = 0; i < totalvc; i++)
            {
                ObxfVoice *p = vq.getNext();
                if ((p->midiIndx == noteNo) && (p->Active))
                {
                    p->NoteOn(reallocKey, -0.5);
                    awaitingkeys[reallocKey] = false;
                }
            }
        }
        else
        // No realloc
        {
            for (int i = 0; i < totalvc; i++)
            {
                ObxfVoice *n = vq.getNext();
                if (n->midiIndx == noteNo && n->Active)
                {
                    n->NoteOff();
                }
            }
        }
    }

    void SetOversample(bool over)
    {
        if (over == true)
        {
            mlfo.setSampleRate(sampleRate * 2);
            vibratoLfo.setSampleRate(sampleRate * 2);
        }
        else
        {
            mlfo.setSampleRate(sampleRate);
            vibratoLfo.setSampleRate(sampleRate);
        }
        for (int i = 0; i < MAX_VOICES; i++)
        {
            voices[i].setHQ(over);
            if (over)
                voices[i].setSampleRate(sampleRate * 2);
            else
                voices[i].setSampleRate(sampleRate);
        }
        Oversample = over;
    }

    inline float processSynthVoice(ObxfVoice &b, float lfoIn, float vibIn)
    {
        if (economyMode)
            b.checkAdsrState();
        if (b.shouldProcessed || (!economyMode))
        {
            b.lfoIn = lfoIn;
            b.lfoVibratoIn = vibIn;
            return b.ProcessSample();
        }
        return 0;
    }

    void processSample(float *sm1, float *sm2)
    {
        tuning.updateMTSESPStatus();
        mlfo.update();
        vibratoLfo.update();
        float vl = 0, vr = 0;
        float vlo = 0, vro = 0;
        float lfovalue = mlfo.getVal();
        float viblfo = vibratoEnabled ? (vibratoLfo.getVal() * vibratoAmount) : 0;
        float lfovalue2 = 0, viblfo2 = 0;
        if (Oversample)
        {
            mlfo.update();
            vibratoLfo.update();
            lfovalue2 = mlfo.getVal();
            viblfo2 = vibratoEnabled ? (vibratoLfo.getVal() * vibratoAmount) : 0;
        }

        for (int i = 0; i < totalvc; i++)
        {
            voices[i].initTuning(&tuning);
            float x1 = processSynthVoice(voices[i], lfovalue, viblfo);
            if (Oversample)
            {
                float x2 = processSynthVoice(voices[i], lfovalue2, viblfo2);
                vlo += x2 * (1 - pannings[i % MAX_PANNINGS]);
                vro += x2 * (pannings[i % MAX_PANNINGS]);
            }
            vl += x1 * (1 - pannings[i % MAX_PANNINGS]);
            vr += x1 * (pannings[i % MAX_PANNINGS]);
        }
        if (Oversample)
        {
            vl = left.Calc(vl, vlo);
            vr = right.Calc(vr, vro);
        }
        *sm1 = vl * Volume;
        *sm2 = vr * Volume;
    }
};

#endif // OBXF_SRC_ENGINE_MOTHERBOARD_H
