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

#ifndef OBXF_SRC_ENGINE_PARAMS_H
#define OBXF_SRC_ENGINE_PARAMS_H
#include "ObxdVoice.h"
#include "ParamsEnum.h"
class ObxdParams
{
  public:
    std::atomic<float> values[PARAM_COUNT]{};
    std::atomic<juce::String *> namePtr{new juce::String("Default")};

    ObxdParams() { setDefaultValues(); }
    ~ObxdParams() { delete namePtr.load(); }

    void setName(const juce::String &newName)
    {
        auto *newStr = new juce::String(newName);
        const auto *oldStr = namePtr.exchange(newStr);
        delete oldStr;
    }

    juce::String getName() const
    {
        juce::String *ptr = namePtr.load();
        return ptr ? *ptr : juce::String();
    }
    void setDefaultValues()
    {
        for (auto &value : values)
        {
            value = 0.0f;
        }
        values[VOICE_COUNT] = 0.2f;
        values[BRIGHTNESS] = 1.0f;
        values[OCTAVE] = 0.5;
        values[TUNE] = 0.5f;
        values[OSC2_DET] = 0.4;
        values[LSUS] = 1.0f;
        values[CUTOFF] = 1.0f;
        values[VOLUME] = 0.5f;
        values[OSC1MIX] = 1;
        values[OSC2MIX] = 1;
        values[OSC1Saw] = 1;
        values[OSC2Saw] = 1;
        values[BENDLFORATE] = 0.6;

        //		values[FILTER_DRIVE]= 0.01;
        values[PAN1] = 0.5;
        values[PAN2] = 0.5;
        values[PAN3] = 0.5;
        values[PAN4] = 0.5;
        values[PAN5] = 0.5;
        values[PAN6] = 0.5;
        values[PAN7] = 0.5;
        values[PAN8] = 0.5;
        values[ECONOMY_MODE] = 1;
        values[ENVDER] = 0.3;
        values[FILTERDER] = 0.3;
        values[LEVEL_DIF] = 0.3;
        values[PORTADER] = 0.3;
        values[UDET] = 0.2;
    }
    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxdParams)
};

#endif // OBXF_SRC_ENGINE_PARAMS_H
