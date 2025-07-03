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
    int stolenVoicesOnMIDIKey[129];
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
            stolenVoicesOnMIDIKey[i] = 0;
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

    void unisonChanged() { resetVoiceQueueCount(); }

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
            if (stolenVoicesOnMIDIKey[i])
                oss << i << "->" << stolenVoicesOnMIDIKey[i] << " ";
        DBG(oss.str());
#endif
    }

    /*
     * THe voice allocator schedule is pretty easy
     * voicePerKey = (uni pressed ? voicesPerKey : 1)
     * each key press triggers min(poly, voicesPerKey) voices stealing from playing
     * But on lowest / highest mode we only play if voices are avaiable and we are
     * lower/higher than them
     */

    int voicesPerKey() const { return std::min(uni ? univc : 1, totalvc); }

    int voicesUsed()
    {
        int va{0};
        for (int i = 0; i < totalvc; i++)
        {
            Voice *p = vq.getNext();
            if (p->Active)
                va++;
        }
        return va;
    }

    int voicesAvailable() { return totalvc - voicesUsed(); }

    Voice *nextVoiceToBeStolen()
    {
        Voice *res{nullptr};

        // Latest
        int minPriority = INT_MAX;
        for (int i = 0; i < totalvc; i++)
        {
            Voice *p = vq.getNext();
            if (p->Active && voiceAgeForPriority[p->midiIndx] < minPriority)
            {
                minPriority = voiceAgeForPriority[p->midiIndx];
                res = p;
            }
        }
        return res;
    }

    int nextMidiKeyToRealloc()
    {
        int res{-1};
        // Latest
        int minPriority = INT_MAX;
        for (int i = 0; i < 129; i++)
        {
            if (stolenVoicesOnMIDIKey[i] > 0 && voiceAgeForPriority[i] < minPriority)
            {
                minPriority = voiceAgeForPriority[i];
                res = i;
            }
        }
        return res;
    }

    bool shouldGivenKeySteal(int /* noteNo */)
    {
        // Latest
        return true;
    }

    void setNoteOn(int noteNo, float velocity, int8_t /* channel */)
    {
        // This played note has the highest as-played priority
        voiceAgeForPriority[noteNo] = asPlayedCounter++;

        // And toggle on unison if it was off
        if (wasUni != uni)
            unisonChanged();

        auto voicesNeeded = voicesPerKey();
        auto vavail = voicesAvailable();
        bool should = shouldGivenKeySteal(noteNo);

        // Go do some stealing
        while (should && voicesNeeded > vavail)
        {
            auto voicesToSteal = voicesNeeded;
            /*
             * Doing this as multiple passes is a bit time inefficient but it
             * helps a lot in the partial steal by oldest case etc
             */
            for (int i = 0; i < voicesToSteal; i++)
            {
                auto v = nextVoiceToBeStolen();
                stolenVoicesOnMIDIKey[v->midiIndx]++;
                v->NoteOn(noteNo, velocity);
                voicesNeeded--;
                break;
            }
        }

        if (voicesNeeded <= vavail)
        {
            // Super simple - just start the voices if they are there.
            // If there aren't enough we just wont start them
            for (int i = 0; i < totalvc; i++)
            {
                Voice *v = vq.getNext();

                if (!v->Active)
                {
                    v->NoteOn(noteNo, velocity);
                    voicesNeeded--;
                    if (voicesNeeded == 0)
                        break;
                }
            }
        }

        dumpVoiceStatus();
    }

    void setNoteOff(int noteNo, float /* velocity */, int8_t /* channel */)
    {
        auto newVoices = voicesPerKey();
        // Start by reallocing voices
        auto mk = nextMidiKeyToRealloc();
        while (newVoices > 0 && mk != -1)
        {
            for (int i = 0; i < totalvc; i++)
            {
                Voice *p = vq.getNext();
                if (p->midiIndx == noteNo && p->Active)
                {
                    p->NoteOn(mk, 0.5); // fixme - retain velocity
                    stolenVoicesOnMIDIKey[mk]--;
                    break;
                }
            }
            mk = nextMidiKeyToRealloc();
            newVoices--;
            dumpVoiceStatus();
        }

        // We've released this key so if we do have stolen voices we dont want to rebring them
        stolenVoicesOnMIDIKey[noteNo] = 0;

        // And if anything is still sounding on this key after the steal, kill it
        for (int i = 0; i < totalvc; i++)
        {
            Voice *v = vq.getNext();
            if (v->midiIndx == noteNo)
            {
                v->NoteOff();
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
