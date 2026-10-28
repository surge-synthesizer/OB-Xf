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

#ifndef OBXF_SRC_ENGINE_MOTHERBOARD_H
#define OBXF_SRC_ENGINE_MOTHERBOARD_H

#include <climits>
#include <Constants.h>
#include "VoiceQueue.h"
#include "SynthEngine.h"
#include "AdsrEnvelopeQuad.h"
#include "GroupRows.h"
#include "VoiceControlQuad.h"
#include "OscillatorQuad.h"
#include "VoiceFinishQuad.h"
#include "Lfo.h"
#include "Tuning.h"
#include "VoiceMatrix.h"

class Motherboard
{
  private:
    Decimator17 left, right;

    VoiceQueue voiceQueue;
    int lastAllocatedIdx{-1};

    int totalVoiceCount{MAX_VOICES};
    int unisonVoiceCount{MAX_PANNINGS};
    bool wasUnisonSet{false};
    int stolenVoicesOnMIDIKey[129]{0};
    int8_t stolenVoicesChannelForMIDIKey[129]{0};
    int voiceAgeForPriority[129]{0};

    int asPlayedCounter{0};
    float sampleRate{1.f};
    float sampleRateInv{1.f};

    // scratch for the four-voice stages, too big for the audio thread's stack
    GroupRows groupRows{};

    static constexpr int oscGroupCount{(MAX_VOICES + OscillatorQuad::lanes - 1) /
                                       OscillatorQuad::lanes};
    VoiceControlQuad controlQuads[oscGroupCount];
    OscillatorQuad oscQuads[oscGroupCount];

    // playing voices in a stable order, so each four-voice group keeps its voices
    Voice *slots[MAX_VOICES]{};
    bool slotted[MAX_VOICES]{};
    int slotCount{0};

    void updateSlots(const bool plays[MAX_VOICES])
    {
        // a voice that stops gives its slot to the last voice
        for (int i = 0; i < slotCount;)
        {
            const auto index = slots[i] - voices;

            if (index < totalVoiceCount && plays[index])
            {
                ++i;
                continue;
            }

            slotted[index] = false;
            slots[i] = slots[--slotCount];
        }

        for (int i = 0; i < totalVoiceCount; ++i)
        {
            if (plays[i] && !slotted[i])
            {
                slotted[i] = true;
                slots[slotCount++] = &voices[i];
            }
        }
    }

    // hands every group's lanes back to their voices, before anything outside rewrites them
    void releaseLanes()
    {
        for (int g = 0; g < oscGroupCount; ++g)
        {
            controlQuads[g].releaseAll();
            oscQuads[g].releaseAll();
        }
    }

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Motherboard)

  public:
    Tuning tuning;
    Voice voices[MAX_VOICES];
    LFO globalLFO, vibratoLFO;

    inline int getTotalVoiceCount() const { return totalVoiceCount; }

    enum VoicePriority
    {
        LATEST,
        HIGHEST,
        LOWEST
    } voicePriority{LATEST};

    float vibratoAmount{0.f};
    float volume{0.f};
    float pannings[MAX_PANNINGS];
    bool anySounding{false};
    bool unison{false};
    bool oversample{false};
    bool reallocate{false};
    bool mpeEnabled{false};
    int mpePitchBendRange{48};

    VoiceMatrix voiceMatrix;
    VoiceMatrixBases matrixBases;

    /* Recompute every voice's matrix. Needed when the matrix rows or the base parameter
     * values change, as opposed to a single voice's sources. */
    void recalculateAllMatrices()
    {
        for (int i = 0; i < totalVoiceCount; i++)
        {
            recalculateMatrix(voiceMatrix, matrixBases, voices[i].matrixSourceValues,
                              voices[i].matrixAdjustments);
        }
    }

    std::array<int32_t, 128> debugNoteOn{}, debugNoteOff{};

    Motherboard() : left(), right()
    {
        for (int i = 0; i < 129; i++)
        {
            stolenVoicesOnMIDIKey[i] = 0;
            stolenVoicesChannelForMIDIKey[i] = 0;
            voiceAgeForPriority[i] = 0;
        }

        globalLFO = LFO();
        vibratoLFO = LFO();

        vibratoLFO.par.wave1blend = -1.f; // pure sine wave
        vibratoLFO.par.unipolarPulse = true;

        voiceQueue = VoiceQueue(MAX_VOICES, voices);

        for (int i = 0; i < MAX_PANNINGS; ++i)
        {
            pannings[i] = 0.5f;
        }

        for (int i = 0; i < MAX_VOICES; i++)
        {
            voices[i].initTuning(&tuning);
            voices[i].voiceIndex = i;
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

            lastAllocatedIdx = -1;
        }
    }

    void setUnisonVoices(int count)
    {
        auto newCount = std::min(count, MAX_VOICES);

        if (newCount != unisonVoiceCount)
        {
            unisonVoiceCount = newCount;

            resetVoiceQueueCount();

            lastAllocatedIdx = -1;
        }
    }

    void resetVoiceQueueCount()
    {
        auto count = std::min(totalVoiceCount, MAX_VOICES);

        for (int i = count; i < MAX_VOICES; i++)
        {
            voices[i].NoteOff(0.f);
            voices[i].ResetEnvelope();
        }

        voiceQueue.reInit(count);
        totalVoiceCount = count;
    }

    void unisonChanged() { resetVoiceQueueCount(); }

    void setSampleRate(float sr)
    {
        releaseLanes();

        sampleRate = sr;
        sampleRateInv = 1.f / sampleRate;

        globalLFO.setSampleRate(sr);
        vibratoLFO.setSampleRate(sr);

        for (int i = 0; i < MAX_VOICES; ++i)
        {
            voices[i].setSampleRate(sr);
        }

        // always execute this when setting SR for the motherboard
        // see GitHub issue #269
        SetHQMode(oversample, true);
    }

    bool isSustainOn{false};
    void sustainOn()
    {
        isSustainOn = true;
        for (int i = 0; i < MAX_VOICES; i++)
        {
            Voice *p = voiceQueue.getNext();

            p->sustOn();
        }
    }

    void sustainOff()
    {
        isSustainOn = false;
        for (int i = 0; i < MAX_VOICES; i++)
        {
            Voice *p = voiceQueue.getNext();

            p->sustOff();
        }
    }

    void dumpVoiceStatus(const std::string &reason)
    {
        OBLOG(voiceManager, "Dumping voice status: " << reason);
        if constexpr (obxf_log::voiceManager)
        {
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

            OBLOG(voiceManager, vposs.str());

            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *p = voiceQueue.getNext();

                if (p->isSounding())
                {
                    OBLOG(voiceManager,
                          "  idx=" << p->voiceIndex << " active " << p->midiNote
                                   << " prio=" << voiceAgeForPriority[p->midiNote]
                                   << " snd=" << p->isSounding() << " gt=" << p->isGated()
                                   << " sus=" << p->isGatedWithSustain() << " on/off "
                                   << debugNoteOn[p->midiNote] << "/" << debugNoteOff[p->midiNote]);
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

            OBLOG(voiceManager, oss.str());
        }
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

        OBLOG(voiceManager, "Voices used: " << va);
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
            int mkey{128};

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
            int maxPriority = INT_MIN;

            for (int i = 0; i < 129; i++)
            {
                if (stolenVoicesOnMIDIKey[i] > 0 && voiceAgeForPriority[i] > maxPriority)
                {
                    maxPriority = voiceAgeForPriority[i];
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

    void setNoteOn(int note, float velocity, int8_t channel)
    {
        anySounding = true;
        debugNoteOn[note]++;

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

        OBLOG(voiceManager, "NoteOn: " << note << " vAvail=" << vAvail << " voicesNeeded="
                                       << voicesNeeded << " shouldSteal=" << should);

        // First thing - am I actively playing on this key?
        for (int i = 0; i < totalVoiceCount; i++)
        {
            Voice *v = voiceQueue.getNext();

            if (v->midiNote == note && (!mpeEnabled || v->channel == channel) && v->isGated() &&
                voicesNeeded > 0)
            {
                v->NoteOn(note, velocity, channel);
                recalculateMatrix(voiceMatrix, matrixBases, v->matrixSourceValues,
                                  v->matrixAdjustments);
                voicesNeeded--;
            }
        }

        // reallocate voices played by same keys, as opposed to always round-robin
        if (reallocate && voicesNeeded > 0)
        {
            for (int i = 0; i < totalVoiceCount; i++)
            {
                if (voices[i].midiNote == note && voicesNeeded > 0)
                {
                    voices[i].NoteOn(note, velocity, channel);
                    recalculateMatrix(voiceMatrix, matrixBases, voices[i].matrixSourceValues,
                                      voices[i].matrixAdjustments);
                    lastAllocatedIdx = i;
                    voicesNeeded--;
                }
            }
        }

        // Go do some voice stealing!
        while (should && voicesNeeded > vAvail)
        {
            auto voicesToSteal = voicesNeeded;

            // Doing this as multiple passes is a bit time-inefficient,
            // but it helps a lot in the partial steal by oldest case etc
            for (int i = 0; i < voicesToSteal; i++)
            {
                auto v = nextVoiceToBeStolen();

                stolenVoicesOnMIDIKey[v->midiNote]++;
                stolenVoicesChannelForMIDIKey[v->midiNote] = v->channel;

                v->NoteOn(note, velocity, channel);
                recalculateMatrix(voiceMatrix, matrixBases, v->matrixSourceValues,
                                  v->matrixAdjustments);
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
            voiceQueue.setIdx(lastAllocatedIdx);

            // Super simple - just start the voices if they are there.
            // If there aren't enough, we just won't start them
            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *v = voiceQueue.getNext();

                if (!v->isGated())
                {
                    v->NoteOn(note, velocity, channel);
                    recalculateMatrix(voiceMatrix, matrixBases, v->matrixSourceValues,
                                      v->matrixAdjustments);
                    lastAllocatedIdx = v->voiceIndex;
                    voicesNeeded--;

                    if (voicesNeeded == 0)
                    {
                        break;
                    }
                }
            }
        }

        dumpVoiceStatus("NoteOn");
    }

    void setNoteOff(int note, float velocity, int8_t channel)
    {
        debugNoteOff[note]++;

        auto newVoices = voicesPerKey();

        // Start by reallocating voices
        auto mk = nextMidiKeyToRealloc();

        OBLOG(voiceManager, "NoteOff: " << note << " newv=" << newVoices << " nextMK=" << mk);

        // mk == note: this note is itself the top stolen key, release it directly
        // mk ==   -1: no stolen keys exist, nothing to realloc — release directly
        if (mk == note || mk == -1)
        {
            for (int i = 0; i < totalVoiceCount; i++)
            {
                Voice *v = voiceQueue.getNext();

                if (v->midiNote == note && (!mpeEnabled || v->channel == channel))
                {
                    v->NoteOff(velocity);
                    recalculateMatrix(voiceMatrix, matrixBases, v->matrixSourceValues,
                                      v->matrixAdjustments);
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
                    p->NoteOn(mk, Voice::reuseVelocitySentinel,
                              mpeEnabled ? stolenVoicesChannelForMIDIKey[mk] : p->channel);
                    recalculateMatrix(voiceMatrix, matrixBases, p->matrixSourceValues,
                                      p->matrixAdjustments);
                    stolenVoicesOnMIDIKey[mk]--;

                    break;
                }
            }

            mk = nextMidiKeyToRealloc();
            newVoices--;

            dumpVoiceStatus("Note Off");
        }

        // We've released this key so if we do have stolen voices we don't want to bring them back
        stolenVoicesOnMIDIKey[note] = 0;

        // And if anything is still sounding on this key after the steal, kill it
        for (int i = 0; i < totalVoiceCount; i++)
        {
            Voice *v = voiceQueue.getNext();

            if (v->midiNote == note && (!mpeEnabled || v->channel == channel))
            {
                v->NoteOff(velocity);
                recalculateMatrix(voiceMatrix, matrixBases, v->matrixSourceValues,
                                  v->matrixAdjustments);
            }
        }

        dumpVoiceStatus("Note Off (2)");
    }

    void processMPEPitch(int8_t channel, float pitchBendValue)
    {
        // pitchBendValue is -1..1 representing mpePitchBendRange. Scale to semitones with the range
        const float scaled = pitchBendValue * mpePitchBendRange;
        for (int i = 0; i < totalVoiceCount; i++)
        {
            // isGated not isSounding since long release can reuse channels
            if ((voices[i].channel == channel || channel == -1) && voices[i].isGated())
            {
                voices[i].mpeBend = scaled;
                setMatrixSource(voices[i].matrixSourceValues, MatrixSource::Glide, pitchBendValue);
                recalculateMatrix(voiceMatrix, matrixBases, voices[i].matrixSourceValues,
                                  voices[i].matrixAdjustments);
            }
        }
    }

    void processMPETimbre(int8_t channel, float timbreValue)
    {
        // timbreValue is 0..1 (CC74 / 127), normalized to -1..1 for the matrix
        const float normalised = timbreValue * 2.f - 1.f;
        for (int i = 0; i < totalVoiceCount; i++)
        {
            if ((voices[i].channel == channel || channel == -1) && voices[i].isGated())
            {
                setMatrixSource(voices[i].matrixSourceValues, MatrixSource::Slide, normalised);
                recalculateMatrix(voiceMatrix, matrixBases, voices[i].matrixSourceValues,
                                  voices[i].matrixAdjustments);
            }
        }
    }

    void processMPEChannelPressure(int8_t channel, float pressureValue)
    {
        for (int i = 0; i < totalVoiceCount; i++)
        {
            if ((voices[i].channel == channel || channel == -1) && voices[i].isGated())
            {
                setMatrixSource(voices[i].matrixSourceValues, MatrixSource::Press, pressureValue);
                recalculateMatrix(voiceMatrix, matrixBases, voices[i].matrixSourceValues,
                                  voices[i].matrixAdjustments);
            }
        }
    }

    void SetHQMode(bool over, bool force = false)
    {
        if (!force && over == oversample)
        {
            return;
        }

        releaseLanes();

        const auto factor = 1 + over;

        globalLFO.setSampleRate(sampleRate * factor);
        vibratoLFO.setSampleRate(sampleRate * factor);

        for (int i = 0; i < MAX_VOICES; i++)
        {
            voices[i].setSampleRate(sampleRate * factor);
            voices[i].setHQMode(over);
        }

        oversample = over;

        left.resetDecimator();
        right.resetDecimator();
    }

    /*
     * The per-output-sample values come from the engine's smoothers. numSamples is at
     * most Voice::blockSize, and nothing may change the engine during the block.
     */
    void processBlock(float *outL, float *outR, int numSamples, const float *cutoff,
                      const float *resonance, const float *resonanceUnmodulated,
                      const float *multimode, const float *bend, const float *vibratoAmounts)
    {
        const int subPerSample = oversample ? 2 : 1;
        const int subSamples = numSamples * subPerSample;

        vibratoAmount = vibratoAmounts[numSamples - 1];

        // with nothing sounding, just update the LFO phases
        auto idle = [&](int from) {
            for (int t = from; t < numSamples; ++t)
            {
                for (int u = 0; u < subPerSample; ++u)
                {
                    globalLFO.update(true);
                    vibratoLFO.update(true);
                }

                outL[t] = 0.f;
                outR[t] = 0.f;
            }
        };

        if (!anySounding)
        {
            idle(0);
            return;
        }

        bool plays[MAX_VOICES]{};

        for (int i = 0; i < totalVoiceCount; ++i)
        {
            plays[i] = voices[i].beginEnvelopes();

            if (!plays[i])
            {
                voices[i].finishEnvelopes(0, subSamples, 0.f);
            }
        }

        updateSlots(plays);

        // every group lets go of moved voices before any group takes them on
        for (int g = 0; g < oscGroupCount; ++g)
        {
            const int at = g * OscillatorQuad::lanes;
            const int count = std::clamp(slotCount - at, 0, OscillatorQuad::lanes);
            controlQuads[g].releaseLanes(slots + at, count);
            oscQuads[g].releaseLanes(slots + at, count);
        }

        int longestVoice = 0;

        for (int at = 0; at < slotCount; at += OscillatorQuad::lanes)
        {
            longestVoice = std::max(
                longestVoice,
                controlQuads[at / OscillatorQuad::lanes].processEnvelopes(
                    slots + at, std::min(OscillatorQuad::lanes, slotCount - at), subSamples));
        }

        // a sample runs in full while some voice sounded through the sample before it
        const int activeSamples = longestVoice == subSamples
                                      ? numSamples
                                      : std::min(numSamples, longestVoice / subPerSample + 1);

        float lfo1In[Voice::maxBlockSubSamples];
        float vibratoIn[Voice::maxBlockSubSamples];

        for (int t = 0; t < activeSamples; ++t)
        {
            for (int u = 0; u < subPerSample; ++u)
            {
                globalLFO.update();
                vibratoLFO.update();

                const auto s = t * subPerSample + u;
                lfo1In[s] = globalLFO.getVal();
                vibratoIn[s] = vibratoLFO.getVal() * vibratoAmounts[t] * vibratoAmounts[t] * 4.f;
            }
        }

        float mixL[Voice::maxBlockSubSamples]{};
        float mixR[Voice::maxBlockSubSamples]{};

        // each group runs every stage back to back on the same rows
        for (int at = 0; at < slotCount; at += OscillatorQuad::lanes)
        {
            auto *const group = slots + at;
            const int laneCount = std::min(OscillatorQuad::lanes, slotCount - at);

            controlQuads[at / OscillatorQuad::lanes].process(groupRows, group, laneCount, lfo1In,
                                                             vibratoIn, cutoff, bend, subPerSample);

            oscQuads[at / OscillatorQuad::lanes].process(groupRows, group, laneCount);

            if (VoiceFinishQuad::compatible(group, laneCount))
            {
                VoiceFinishQuad::process(groupRows, group, laneCount, lfo1In, resonance,
                                         resonanceUnmodulated, multimode, subPerSample);
                continue;
            }

            for (int l = 0; l < laneCount; ++l)
            {
                auto &voice = *group[l];
                const int k = voice.blockSubSamples;

                GroupRows::toVoice(groupRows.out, l, voice.block.out, k);
                GroupRows::toVoice(groupRows.cutoff, l, voice.block.cutoff, k);
                GroupRows::toVoice(groupRows.lfo2, l, voice.block.lfo2, k);
                GroupRows::toVoice(groupRows.ampEnv, l, voice.block.ampEnv, k);
                voice.processFinish(lfo1In, resonance, resonanceUnmodulated, multimode,
                                    subPerSample);
            }
        }

        for (int i = 0; i < totalVoiceCount; i++)
        {
            auto &v = voices[i];
            const float pan = pannings[i % MAX_PANNINGS];

            for (int s = 0; s < v.blockSubSamples; ++s)
            {
                mixL[s] += v.block.out[s] * (1 - pan);
                mixR[s] += v.block.out[s] * (pan);
            }

            v.takeTrailingEngineValues(numSamples, subPerSample, cutoff, bend, resonance,
                                       resonanceUnmodulated, multimode);
        }

        for (int t = 0; t < activeSamples; ++t)
        {
            float vl = mixL[t * subPerSample];
            float vr = mixR[t * subPerSample];

            if (oversample)
            {
                vl = left.decimate(vl, mixL[t * 2 + 1]);
                vr = right.decimate(vr, mixR[t * 2 + 1]);
            }

            outL[t] = vl * volume;
            outR[t] = vr * volume;
        }

        idle(activeSamples);

        anySounding = longestVoice == subSamples;
    }
};

#endif // OBXF_SRC_ENGINE_MOTHERBOARD_H
