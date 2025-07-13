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
    Decimator17 left, right;

    VoiceQueue voiceQueue;

    int totalVoiceCount{MAX_VOICES};
    int unisonVoiceCount{MAX_PANNINGS};
    bool wasUnisonSet{false};
    int stolenVoicesOnMIDIKey[129]{0};
    int voiceAgeForPriority[129]{0};

    int asPlayedCounter{0};
    float sampleRate{1.f};
    float sampleRateInv{1.f};

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Motherboard)

  public:
    Tuning tuning;
    Voice voices[MAX_VOICES];
    LFO globalLFO, vibratoLFO;

    enum VoicePriority
    {
        LATEST,
        HIGHEST,
        LOWEST
    } voicePriority{LATEST};

    float vibratoAmount{0.f};
    float volume{0.f};
    float pannings[MAX_PANNINGS];
    bool unison{false};
    bool oversample{false};
    bool ecoMode{true};

#if DEBUG_VOICE_MANAGER
    std::array<int32_t, 128> debugNoteOn{}, debugNoteOff{};
#endif

    Motherboard() : left(), right()
    {
        for (int i = 0; i < 129; i++)
        {
            stolenVoicesOnMIDIKey[i] = 0;
            voiceAgeForPriority[i] = 0;
        }

        globalLFO = LFO();
        vibratoLFO = LFO();

        vibratoLFO.wave1blend = -1.f; // pure sine wave
        vibratoLFO.unipolarPulse = true;

        voiceQueue = VoiceQueue(MAX_VOICES, voices);

        for (int i = 0; i < MAX_PANNINGS; ++i)
        {
            pannings[i] = 0.5f;
        }
    }

    ~Motherboard() {}

    void setPolyphony(int count)
    {
        auto newCount = std::min(count, MAX_VOICES);
        if (newCount != totalVoiceCount)
        {
            totalVoiceCount = newCount;

            resetVoiceQueueCount();
        }
    }

    void setUnisonVoices(int count)
    {
        auto newCount = std::min(count, MAX_PANNINGS);
        if (newCount != unisonVoiceCount)
        {
            unisonVoiceCount = newCount;

            resetVoiceQueueCount();
        }
    }

    void resetVoiceQueueCount()
    {
        auto count = std::min(totalVoiceCount, MAX_VOICES);

        for (int i = count; i < MAX_VOICES; i++)
        {
            voices[i].NoteOff();
            voices[i].ResetEnvelope();
        }

        voiceQueue.reInit(count);
        totalVoiceCount = count;
    }

    void unisonChanged() { resetVoiceQueueCount(); }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.f / sampleRate;

        globalLFO.setSampleRate(sr);
        vibratoLFO.setSampleRate(sr);

        for (int i = 0; i < MAX_VOICES; ++i)
        {
            voices[i].setSampleRate(sr);
        }

        SetHQMode(oversample);
    }

    void sustainOn()
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            Voice *p = voiceQueue.getNext();

            p->sustOn();
        }
    }

    void sustainOff()
    {
        for (int i = 0; i < MAX_VOICES; i++)
        {
            Voice *p = voiceQueue.getNext();

            p->sustOff();
        }
    }

    void dumpVoiceStatus()
    {
#if DEBUG_VOICE_MANAGER
        std::ostringstream vposs;

        vposs << "Voice State: mode=";
        switch (voicePriority)
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

        for (int i = 0; i < totalVoiceCount; i++)
        {
            Voice *p = voiceQueue.getNext();

            if (p->isGated())
            {
                DBG("  active " << p->midiNote << " prio=" << voiceAgeForPriority[p->midiNote]
                                << " on/off " << debugNoteOn[p->midiNote] << "/"
                                << debugNoteOff[p->midiNote]);
            }
        }

        std::ostringstream oss;

        oss << "  Held Unsounding Keys: ";

        for (int i = 0; i < 129; i++)
        {
            if (stolenVoicesOnMIDIKey[i])
            {
                oss << i << "->" << stolenVoicesOnMIDIKey[i] << " ";
            }
        }

        DBG(oss.str());
#endif
    }

    /*
     * THe voice allocator schedule is pretty easy
     * voicePerKey = (unison pressed ? voicesPerKey : 1)
     * each key press triggers min(poly, voicesPerKey) voices stealing from playing
     * But on lowest / highest mode we only play if voices are avaiable and we are
     * lower/higher than them
     */

    int voicesPerKey() const { return std::min(unison ? unisonVoiceCount : 1, totalVoiceCount); }

    int voicesUsed()
    {
        int va{0};

        for (int i = 0; i < totalVoiceCount; i++)
        {
            Voice *p = voiceQueue.getNext();

            if (p->isGated())
            {
                va++;
            }
        }

        return va;
    }

    int voicesAvailable() { return totalVoiceCount - voicesUsed(); }

    Voice *nextVoiceToBeStolen()
    {
        Voice *res{nullptr};

        switch (voicePriority)
        {
        case LATEST:
        {
            int minPriority = INT_MAX;

            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *p = voiceQueue.getNext();

                if (p->isGated() && voiceAgeForPriority[p->midiNote] < minPriority)
                {
                    minPriority = voiceAgeForPriority[p->midiNote];
                    res = p;
                }
            }
        }
        break;
        case LOWEST:
        {
            // Steal the highest playing voice
            int mkey{-1};

            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *p = voiceQueue.getNext();

                if (p->isGated() && p->midiNote > mkey)
                {
                    res = p;
                    mkey = p->midiNote;
                }
            }
        }
        break;
        case HIGHEST:
        {
            // Steal the lowest playing voice
            int mkey{120};

            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *p = voiceQueue.getNext();

                if (p->isGated() && p->midiNote < mkey)
                {
                    res = p;
                    mkey = p->midiNote;
                }
            }
        }
        break;
        }
        return res;
    }

    int nextMidiKeyToRealloc()
    {
        int res{-1};

        switch (voicePriority)
        {
        case LATEST:
        {
            int minPriority = INT_MAX;

            for (int i = 0; i < 129; i++)
            {
                if (stolenVoicesOnMIDIKey[i] > 0 && voiceAgeForPriority[i] < minPriority)
                {
                    minPriority = voiceAgeForPriority[i];
                    res = i;
                }
            }
        }
        break;
        case LOWEST:
        {
            // Find the lowest note with a stolen voice
            for (int i = 0; i < 129; i++)
            {
                if (stolenVoicesOnMIDIKey[i] > 0)
                {
                    return i;
                }
            }
        }
        break;
        case HIGHEST:
        {
            // Find the highest note with a stolen voice
            for (int i = 128; i >= 0; i--)
            {
                if (stolenVoicesOnMIDIKey[i] > 0)
                {
                    return i;
                }
            }
        }
        break;
        }
        return res;
    }

    bool shouldGivenKeySteal(int note)
    {
        switch (voicePriority)
        {
        case LATEST:
            return true;
        case LOWEST:
            // Am I lower than the lowest active note
            {
                auto shouldSteal{true};

                for (int i = 0; i < totalVoiceCount; i++)
                {
                    Voice *p = voiceQueue.getNext();

                    if (p->isGated())
                    {
                        shouldSteal = shouldSteal && note < p->midiNote;
                    }
                }

                return shouldSteal;
            }
            break;
        case HIGHEST:
            // Am I higher than the highest active note
            {
                auto shouldSteal{true};

                for (int i = 0; i < totalVoiceCount; i++)
                {
                    Voice *p = voiceQueue.getNext();

                    if (p->isGated())
                    {
                        shouldSteal = shouldSteal && note > p->midiNote;
                    }
                }

                return shouldSteal;
            }
            break;
        }
        return false;
    }

    void setNoteOn(int note, float velocity, int8_t /* channel */)
    {
#if DEBUG_VOICE_MANAGER
        debugNoteOn[note]++;
#endif
        // This played note has the highest as-played priority
        voiceAgeForPriority[note] = asPlayedCounter++;

        // And toggle on unison if it was off
        if (wasUnisonSet != unison)
        {
            unisonChanged();
        }

        auto voicesNeeded = voicesPerKey();
        auto vAvail = voicesAvailable();
        bool should = shouldGivenKeySteal(note);

        /*
         * First thing - am I actively playing on this key
         */
        for (int i = 0; i < totalVoiceCount; i++)
        {
            Voice *v = voiceQueue.getNext();
            if (v->midiNote == note && v->isGated() && voicesNeeded > 0)
            {
                v->NoteOn(note, velocity);
                voicesNeeded--;
            }
        }

        // Go do some stealing!
        while (should && voicesNeeded > vAvail)
        {
            auto voicesToSteal = voicesNeeded;

            // Doing this as multiple passes is a bit time-inefficient,
            // but it helps a lot in the partial steal by oldest case etc
            for (int i = 0; i < voicesToSteal; i++)
            {
                auto v = nextVoiceToBeStolen();

                stolenVoicesOnMIDIKey[v->midiNote]++;
                v->NoteOn(note, velocity);
                voicesNeeded--;

                break;
            }
        }

        if (!should && voicesNeeded > vAvail)
        {
            stolenVoicesOnMIDIKey[note] += voicesNeeded - vAvail;
            voicesNeeded = vAvail;
        }

        if (voicesNeeded && voicesNeeded <= vAvail)
        {
            // Super simple - just start the voices if they are there.
            // If there aren't enough, we just won't start them
            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *v = voiceQueue.getNext();

                if (!v->isGated())
                {
                    v->NoteOn(note, velocity);
                    voicesNeeded--;

                    if (voicesNeeded == 0)
                    {
                        break;
                    }
                }
            }
        }

        dumpVoiceStatus();
    }

    void setNoteOff(int note, float /* velocity */, int8_t /* channel */)
    {
#if DEBUG_VOICE_MANAGER
        debugNoteOff[note]++;
#endif
        auto newVoices = voicesPerKey();

        // Start by reallocating voices
        auto mk = nextMidiKeyToRealloc();
        if (mk == note) // OK I'm the next key to release so clear me out
        {
            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *v = voiceQueue.getNext();

                if (v->midiNote == note)
                {
                    v->NoteOff();
                }
            }
            stolenVoicesOnMIDIKey[note] = 0;
        }

        // and then find the next next key to release
        mk = nextMidiKeyToRealloc();

        while (newVoices > 0 && mk != -1) // don't realloc myself! just stop.
        {
            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *p = voiceQueue.getNext();

                if (p->midiNote == note && p->isGated())
                {
                    p->NoteOn(mk, Voice::reuseVelocitySentinel);
                    stolenVoicesOnMIDIKey[mk]--;

                    break;
                }
            }

            mk = nextMidiKeyToRealloc();
            newVoices--;

            dumpVoiceStatus();
        }

        // We've released this key so if we do have stolen voices we don't want to bring them back
        stolenVoicesOnMIDIKey[note] = 0;

        // And if anything is still sounding on this key after the steal, kill it
        for (int i = 0; i < totalVoiceCount; i++)
        {
            Voice *v = voiceQueue.getNext();

            if (v->midiNote == note)
            {
                v->NoteOff();
            }
        }

        dumpVoiceStatus();
    }

    void SetHQMode(bool over)
    {
        if (over == oversample)
            return;

        if (over == true)
        {
            globalLFO.setSampleRate(sampleRate * 2);
            vibratoLFO.setSampleRate(sampleRate * 2);
        }
        else
        {
            globalLFO.setSampleRate(sampleRate);
            vibratoLFO.setSampleRate(sampleRate);
        }

        for (int i = 0; i < MAX_VOICES; i++)
        {
            voices[i].setHQMode(over);
            voices[i].setSampleRate(sampleRate * (1 + over));
        }

        oversample = over;
        left.resetDecimator();
        right.resetDecimator();
    }

    inline float processSynthVoice(Voice &b, float lfoIn, float vibIn)
    {
        if (ecoMode)
        {
            b.updateSoundingState();
        }

        if (b.isSounding() || (!ecoMode))
        {
            b.lfoIn = lfoIn;
            b.lfoVibratoIn = vibIn;

            return b.ProcessSample();
        }

        return 0.f;
    }

    void processSample(float *sm1, float *sm2)
    {
        globalLFO.update();
        vibratoLFO.update();

        float vl = 0, vr = 0;
        float vlo = 0, vro = 0;
        float lfovalue = globalLFO.getVal();
        float viblfo = vibratoLFO.getVal() * vibratoAmount * vibratoAmount * 4.f;
        float lfovalue2 = 0, viblfo2 = 0;

        if (oversample)
        {
            globalLFO.update();
            vibratoLFO.update();
            lfovalue2 = globalLFO.getVal();
            viblfo2 = vibratoLFO.getVal() * vibratoAmount * vibratoAmount * 4.f;
        }

        for (int i = 0; i < totalVoiceCount; i++)
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
            vl = left.decimate(vl, vlo);
            vr = right.decimate(vr, vro);
        }

        *sm1 = vl * volume;
        *sm2 = vr * volume;
    }
};

#endif // OBXF_SRC_ENGINE_MOTHERBOARD_H
