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

#ifndef OBXF_SRC_MIDI_MIDIHANDLER_H
#define OBXF_SRC_MIDI_MIDIHANDLER_H

#include "MidiMap.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include "Utils.h"
#include "ParameterAdapter.h"

class SynthEngine;
class MidiMap;

class MidiHandler
{
  public:
    MidiHandler(SynthEngine &s, MidiMap &b, ParameterManagerAdapter &pm, Utils &utils);

    void prepareToPlay();

    void processMidiPerSample(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer,
                              int samplePos);

    bool getNextEvent(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer,
                      int samplePos);

    void initMidi();

    void updateMidiConfig() const;

    void setMidiControlledParamSet(const bool value) { midiControlledParamSet = value; }
    void setLastMovedController(const int value) { lastMovedController = value; }
    void setLastUsedParameter(const juce::String &paramId);

    [[nodiscard]] int getLastMovedController() const { return lastMovedController; }
    [[nodiscard]] bool getMidiControlledParamSet() const { return midiControlledParamSet; }

    [[nodiscard]] juce::String getCurrentMidiPath() const { return currentMidiPath; }

    std::function<void(int)> onProgramChangeCallback;

    int getLastUsedParameter() const { return lastUsedParameter; }

    void saveBindingsTo(const juce::File &f) const { bindings.saveFile(f); }

  private:
    Utils &utils;
    SynthEngine &synth;
    MidiMap &bindings;
    ParameterManagerAdapter &paramManager;

    std::unique_ptr<juce::MidiMessage> nextMidi;
    std::unique_ptr<juce::MidiMessage> midiMsg;

    std::atomic<bool> midiControlledParamSet{false};
    int lastMovedController{0};
    std::atomic<int> lastUsedParameter{0};
    int midiEventPos{0};
    uint8_t bankSelectMSB{0};

    juce::String currentMidiPath;
};

#endif // OBXF_SRC_MIDI_MIDIHANDLER_H
