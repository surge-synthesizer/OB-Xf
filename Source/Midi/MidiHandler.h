/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_MIDI_MIDIHANDLER_H
#define OBXF_SRC_MIDI_MIDIHANDLER_H
#include <juce_audio_basics/juce_audio_basics.h>
class SynthEngine;
class MidiMap;
class ObxdBank;
#include "Utils.h"
#include "inplace_function.h"
#include "ParameterAdaptor.h"

class MidiHandler
{
  public:
    MidiHandler(SynthEngine &s, MidiMap &b, ObxdBank &p, ParameterManagerAdaptor &pm, Utils &utils);

    void prepareToPlay();

    void processMidiPerSample(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer,
                              int samplePos);

    bool getNextEvent(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer,
                      int samplePos);

    void initMidi();

    void updateMidiConfig() const;

    void setMidiControlledParamSet(const bool value) { midiControlledParamSet = value; }
    void setLastMovedController(const int value) { lastMovedController = value; }
    void setLastUsedParameter(const int value) { lastUsedParameter = value; }

    [[nodiscard]] int getLastMovedController() const { return lastMovedController; }
    [[nodiscard]] bool getMidiControlledParamSet() const { return midiControlledParamSet; }

    [[nodiscard]] juce::String getCurrentMidiPath() const { return currentMidiPath; }

    // maybe change this to juce::FixedSizeFunction
    stdext::inplace_function<void(int), 32> onProgramChangeCallback;

  private:
    Utils &utils;
    SynthEngine &synth;
    MidiMap &bindings;
    ObxdBank &programs;
    ParameterManagerAdaptor &paramManager;

    std::unique_ptr<juce::MidiMessage> nextMidi;
    std::unique_ptr<juce::MidiMessage> midiMsg;

    std::atomic<bool> midiControlledParamSet{false};
    int lastMovedController{0};
    int lastUsedParameter{0};
    int midiEventPos{0};

    juce::String currentMidiPath;
};

#endif // OBXF_SRC_MIDI_MIDIHANDLER_H
