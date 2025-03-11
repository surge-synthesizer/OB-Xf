// MidiProcessor.h
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
class SynthEngine;
class MidiMap;
class ObxdBank;
class ParameterManager;
#include "ProgramChangeCallback.h"


class MidiHandler {
public:
    MidiHandler(SynthEngine &s, MidiMap &b, ObxdBank &p, ParameterManager &pm, ProgramChangeCallback &pcCallback);

    void prepareToPlay();

    void processMidiPerSample(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer, int samplePos);

    bool getNextEvent(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer, int samplePos);

    void initMidi();

    void updateMidiConfig() const;

    void setMidiControlledParamSet(const bool value) { midiControlledParamSet = value; }
    void setLastMovedController(const int value) { lastMovedController = value; }
    void setLastUsedParameter(const int value) { lastUsedParameter = value; }

    [[nodiscard]] int getLastMovedController() const { return lastMovedController; }
    [[nodiscard]] bool getMidiControlledParamSet() const { return midiControlledParamSet; }

    [[nodiscard]] juce::String getCurrentMidiPath() const { return currentMidiPath; }

private:
    SynthEngine &synth;
    MidiMap &bindings;
    ObxdBank &programs;
    ParameterManager &paramManager;

    std::unique_ptr<juce::MidiMessage> nextMidi;
    std::unique_ptr<juce::MidiMessage> midiMsg;

    bool midiControlledParamSet{false};
    int lastMovedController{0};
    int lastUsedParameter{0};
    int midiEventPos{0};

    juce::String currentMidiPath;

    ProgramChangeCallback &programChangeCallback;
};
