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
#include <Constants.h>
#include "VoiceQueue.h"
#include "SynthEngine.h"
#include "Lfo.h"
#include "Tuning.h"

#define DEBUG_VOICE_MANAGER 0

class Motherboard
{
  private:
    VoiceQueue vq;
    int totalvc;
    int univc;
    bool wasUni;
    bool heldMIDIKeys[129];
    int voiceAgeForPriority[129];

    Decimator17 left, right;
    int asPlayedCounter;
    float lkl, lkr;
    float sampleRate, sampleRateInv;

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Motherboard)

  public:
    Tuning tuning;
    enum VoicePriority
    {
        LATEST,
        HIGHEST, // not implemented yet - defaults to latest
        LOWEST
    } voicePriorty;

    Lfo mlfo, vibratoLfo;
    float vibratoAmount;

    float Volume;
    float pannings[MAX_PANNINGS];
    Voice voices[MAX_VOICES];
    bool uni;
    bool oversample;
    bool economyMode;

    Motherboard() : left(), right()
    {
        economyMode = true;
        lkl = lkr = 0;
        voicePriorty = LATEST;
        asPlayedCounter = 0;
        for (int i = 0; i < 129; i++)
        {
            heldMIDIKeys[i] = false;
            voiceAgeForPriority[i] = 0;
        }
        vibratoAmount = 0;
        oversample = false;
        mlfo = Lfo();
        vibratoLfo = Lfo();
        vibratoLfo.wave1blend = -1.f; // pure sine wave
        vibratoLfo.unipolarPulse = true;
        uni = false;
        wasUni = false;
        Volume = 0;
        totalvc = MAX_VOICES;
        vq = VoiceQueue(MAX_VOICES, voices);
        for (int i = 0; i < MAX_PANNINGS; ++i)
        {
            pannings[i] = 0.5;
        }
    }

    ~Motherboard() {}

    void setPolyphony(int count)
    {
        totalvc = std::min(count, MAX_VOICES);
#if DEBUG_VOICE_MANAGER
        DBG("Setting voice count to " << totalvc);
#endif
        resetVoiceQueueCount();
    }

    void setUnisonVoices(int count)
    {
        univc = std::min(count, MAX_VOICES);
#if DEBUG_VOICE_MANAGER
        DBG("Setting unison voices count to " << totalvc << " (unused)");
#endif
        resetVoiceQueueCount();
    }

    void resetVoiceQueueCount()
    {
#if DEBUG_VOICE_MANAGER
        DBG("Setting voice count to " << totalvc << " (ignoring univc " << univc << ")");
#endif

        auto count = totalvc;
        count = std::min(count, MAX_VOICES);
#if DEBUG_VOICE_MANAGER
        DBG("Setting total voives to " << count << " in " << (uni ? "Uni" : "Poly") << " mode");
#endif
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
        resetVoiceQueueCount();
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
        SetOversample(oversample);
    }

    void sustainOn()
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            Voice *p = vq.getNext();
            p->sustOn();
        }
    }

    void sustainOff()
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            Voice *p = vq.getNext();
            p->sustOff();
        }
    }

    void dumpVoiceStatus()
    {
#if DEBUG_VOICE_MANAGER
        std::ostringstream vposs;
        vposs << "Voice State: mode=";
        switch (voicePriorty)
        {
        case LATEST:
            vposs << "latest";
            break;

        case HIGHEST:
            vposs << "highest";
            break;
        case LOWEST:
            vposs << "lowest";
            break;
        }
        DBG(vposs.str());
        for (int i = 0; i < totalvc; i++)
        {
            Voice *p = vq.getNext();
            if (p->Active)
            {
                DBG("  Active " << p->midiIndx << " prio=" << voiceAgeForPriority[p->midiIndx]);
            }
        }
        std::ostringstream oss;
        oss << "  Held Unsounding Keys: ";
        for (int i = 0; i < 129; i++)
            if (heldMIDIKeys[i])
                oss << i << " ";
        DBG(oss.str());
#endif
    }

    void setNoteOn(int noteNo, float velocity)
    {
        // This played note has the highest as-played priority
        asPlayedCounter++;
        voiceAgeForPriority[noteNo] = asPlayedCounter;

        // And toggle on unison if it was off
        if (wasUni != uni)
            unisonOn();

        bool processed = false;

        if (uni)
        {
            if (voicePriorty == LOWEST)
            {
                // Find the lowest playing note
                int minmidi = 129;
                for (int i = 0; i < totalvc; i++)
                {
                    Voice *p = vq.getNext();
                    if (p->midiIndx < minmidi && p->Active)
                    {
                        minmidi = p->midiIndx;
                    }
                }
                // if the lowest note is below the note i just played, put me in the
                // wait state
                if (minmidi < noteNo)
                {
                    heldMIDIKeys[noteNo] = true;
                }
                else
                {
                    // Otherwise I need to movve the notes to noteNo
                    for (int i = 0; i < totalvc; i++)
                    {
                        Voice *p = vq.getNext();
                        // if the found voice is higher than me and active
                        if (p->midiIndx > noteNo && p->Active)
                        {
                            // retune that voice to this lower note with velocity 0.5
                            heldMIDIKeys[p->midiIndx] = true;
                            p->NoteOn(noteNo, -0.5);
                        }
                        else
                        {
                            // otherwise put you to this note.
                            p->NoteOn(noteNo, velocity);
                        }
                    }
                }
                processed = true;
            }
            else
            {
                // Otherwise (not as-played is latest so just move everyone)
                for (int i = 0; i < totalvc; i++)
                {
                    Voice *p = vq.getNext();
                    if (p->Active)
                    {
                        // Set active note to note with velocity 0.5 and put it in awaiting
                        heldMIDIKeys[p->midiIndx] = true;
                        p->NoteOn(noteNo, -0.5);
                    }
                    else
                    {
                        // and inactive to the velocity and note
                        p->NoteOn(noteNo, velocity);
                    }
                }
                processed = true;
            }
        }
        else
        {
            // poly - just find a voice
            for (int i = 0; i < totalvc && !processed; i++)
            {
                Voice *p = vq.getNext();
                if (!p->Active)
                {
                    // and turn it on
                    p->NoteOn(noteNo, velocity);
                    processed = true;
                }
            }
        }
        // if voice steal occured
        if (!processed)
        {
            // If I am lowest prioprity pick the highest voice to steal
            if (voicePriorty == LOWEST)
            {
                int maxmidi = 0;
                Voice *highestVoiceAvalible = NULL;
                for (int i = 0; i < totalvc; i++)
                {
                    Voice *p = vq.getNext();
                    if (p->midiIndx > maxmidi)
                    {
                        maxmidi = p->midiIndx;
                        highestVoiceAvalible = p;
                    }
                }
                if (maxmidi < noteNo)
                {
                    heldMIDIKeys[noteNo] = true;
                }
                else
                {
                    highestVoiceAvalible->NoteOn(noteNo, -0.5);
                    heldMIDIKeys[maxmidi] = true;
                }
            }
            else
            {
                // Find the oldest
                int minPriority = INT_MAX;
                Voice *minPriorityVoice = NULL;
                for (int i = 0; i < totalvc; i++)
                {
                    Voice *p = vq.getNext();
                    if (voiceAgeForPriority[p->midiIndx] < minPriority)
                    {
                        minPriority = voiceAgeForPriority[p->midiIndx];
                        minPriorityVoice = p;
                    }
                }
                heldMIDIKeys[minPriorityVoice->midiIndx] = true;
                minPriorityVoice->NoteOn(noteNo, -0.5);
            }
        }
        wasUni = uni;
        dumpVoiceStatus();
    }

    void setNoteOff(int noteNo)
    {
        heldMIDIKeys[noteNo] = false; // i'm done thank you!
        int reallocKey = 0;
        // Voice release case - find the lowest note to re-fire
        if (voicePriorty == LOWEST)
        {
            while (reallocKey < 129 && (!heldMIDIKeys[reallocKey]))
            {
                reallocKey++;
            }
        }
        else
        {
            // or the newest
            reallocKey = 129;
            int maxPriority = INT_MIN;
            for (int i = 0; i < 129; i++)
            {
                if (heldMIDIKeys[i] && (maxPriority < voiceAgeForPriority[i]))
                {
                    reallocKey = i;
                    maxPriority = voiceAgeForPriority[i];
                }
            }
        }
        // If I have a voice to restart, do so by moving myself
        // (noteOn) to the new key (reallocKey)
        if (reallocKey != 129)
        {
            for (int i = 0; i < totalvc; i++)
            {
                Voice *p = vq.getNext();
                if ((p->midiIndx == noteNo) && (p->Active))
                {
                    p->NoteOn(reallocKey, -0.5);
                    heldMIDIKeys[reallocKey] = false;
                }
            }
        }
        else
        {
            // Just stop the note
            for (int i = 0; i < totalvc; i++)
            {
                Voice *n = vq.getNext();
                if (n->midiIndx == noteNo && n->Active)
                {
                    n->NoteOff();
                }
            }
        }
        dumpVoiceStatus();
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
        oversample = over;
    }

    inline float processSynthVoice(Voice &b, float lfoIn, float vibIn)
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
        float viblfo = vibratoLfo.getVal() * vibratoAmount * vibratoAmount * 4.f;
        float lfovalue2 = 0, viblfo2 = 0;
        if (oversample)
        {
            mlfo.update();
            vibratoLfo.update();
            lfovalue2 = mlfo.getVal();
            viblfo2 = vibratoLfo.getVal() * vibratoAmount * vibratoAmount * 4.f;
        }

        for (int i = 0; i < totalvc; i++)
        {
            voices[i].initTuning(&tuning);
            float x1 = processSynthVoice(voices[i], lfovalue, viblfo);
            if (oversample)
            {
                float x2 = processSynthVoice(voices[i], lfovalue2, viblfo2);
                vlo += x2 * (1 - pannings[i % MAX_PANNINGS]);
                vro += x2 * (pannings[i % MAX_PANNINGS]);
            }
            vl += x1 * (1 - pannings[i % MAX_PANNINGS]);
            vr += x1 * (pannings[i % MAX_PANNINGS]);
        }
        if (oversample)
        {
            vl = left.Calc(vl, vlo);
            vr = right.Calc(vr, vro);
        }
        *sm1 = vl * Volume;
        *sm2 = vr * Volume;
    }
};

#endif // OBXF_SRC_ENGINE_MOTHERBOARD_H
