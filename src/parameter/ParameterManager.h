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

#ifndef OBXF_SRC_PARAMETER_PARAMETERMANAGER_H
#define OBXF_SRC_PARAMETER_PARAMETERMANAGER_H
#define OBXF_SRC_PARAupdaMETER_PARAMETERMANAGER_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "ParameterInfo.h"
#include "FIFO.h"

#include <unordered_map>

class ObxfAudioProcessor;

/**
 * ParameterManager is an AudioProcessorParameter::Listener which deals with
 * all the necessary callbacks for parameters. It plays a few roles
 *
 * 1. It holds a FIFO which gets messages to the audio thread for callbacks
 * 2. It holds the callbacks by param ID to actually process parameter changes
 * 3. Since it is a listener, it holds the gesture changes which in the future will
 *    support UNDO
 * 4. It stores a copy of the ParameterInfo vector
 */
class ParameterManager : public juce::AudioProcessorParameter::Listener
{
  public:
    using callbackFn_t = std::function<void(float value, bool forced)>;

    ParameterManager(ObxfAudioProcessor &audioProcessor,
                     const std::vector<ParameterInfo> &parameters);
    ParameterManager() = delete;
    ~ParameterManager() override;

    // This is the listener API and should not be called directly
    // ValueChanged pushes onto the FIFO, GetstureChanged implements undo
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int /*parameterIndex */, bool /* gesture */) override;

    // Callbacks exist for a param and a purpose. Currently purposes are PROGRAM
    // and UI but its extensible.
    bool addParameterCallback(const juce::String &ID, const juce::String &purpose,
                              const callbackFn_t &cb);
    bool removeParameterCallback(const juce::String &ID, const juce::String &purpose);

    // This pushes a change onto the engine FIFO from the nonaudio thread
    void queueParameterChange(const juce::String &paramID, float newValue);

    void clearFiFO();
    bool isFiFOClear();

    // This drains the FIFO and applies them to all the parameters.
    void updateParameters(bool force = false);

    /**
     * This function is used only by the MIDI lag handler which runs on audio thread
     * so is a shortcut around the FIFO for that use case. Don't use it unless
     * you know why you want to use it.
     */
    void forceSingleParameterCallback(const juce::String &paramID, float newValue);

    juce::RangedAudioParameter *getParameter(const juce::String &paramID) const;
    void addParameter(const juce::String &paramID, juce::RangedAudioParameter *param);

    void setSupressGestureToUndo(bool state) { supressGestureToUndo = state; }

  private:
    FIFO<128> fifo;
    std::vector<ParameterInfo> parameters;
    ObxfAudioProcessor &audioProcessor;

    std::unordered_map<juce::String, std::unordered_map<juce::String, callbackFn_t>> callbacks;
    std::unordered_map<juce::String, juce::RangedAudioParameter *> paramMap;

    /*
     * Note we have a mutex to lock callbacks but it is almost never contested.
     * The only reason we need it is if editor closes during VST automation the closing
     * editor will modify the callback structure to remove its hooks
     */
    std::mutex callbackMutex;

    bool supressGestureToUndo{false};

    JUCE_DECLARE_NON_COPYABLE(ParameterManager)
    JUCE_DECLARE_NON_MOVEABLE(ParameterManager)
    JUCE_LEAK_DETECTOR(ParameterManager)
};

#endif // OBXF_SRC_PARAMETER_PARAMETERMANAGER_H
