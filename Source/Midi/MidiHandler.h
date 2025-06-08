#pragma once
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
