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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "ParameterInfo.h"
#include "FIFO.h"

#include <unordered_map>

class ParameterManager : public juce::AudioProcessorParameter::Listener
{
  public:
    using Callback = std::function<void(float value, bool forced)>;

    ParameterManager(juce::AudioProcessor &audioProcessor,
                     const std::vector<ParameterInfo> &parameters);

    ParameterManager() = delete;

    ~ParameterManager() override;

    bool registerParameterCallback(const juce::String &ID, const Callback &cb);

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/) override {}

    void flushParameterQueue();

    void clearFiFO() { fifo.clear(); }

    void updateParameters(bool force = false);

    void clearParameterQueue();

    const std::vector<ParameterInfo> &getParameters() const;

    juce::RangedAudioParameter *getParameter(const juce::String &paramID) const;

    void addParameter(const juce::String &paramID, juce::RangedAudioParameter *param);
    void queueParameterChange(const juce::String &paramID, float newValue);

  private:
    FIFO<128> fifo;
    std::vector<ParameterInfo> parameters;

    std::unordered_map<juce::String, Callback> callbacks;
    std::unordered_map<juce::String, juce::RangedAudioParameter *> paramMap;

    JUCE_DECLARE_NON_COPYABLE(ParameterManager)

    JUCE_DECLARE_NON_MOVEABLE(ParameterManager)

    JUCE_LEAK_DETECTOR(ParameterManager)
};

#endif // OBXF_SRC_PARAMETER_PARAMETERMANAGER_H
