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

#ifndef OBXF_SRC_ENGINE_PARAMETERS_H
#define OBXF_SRC_ENGINE_PARAMETERS_H

#include "Voice.h"
#include "ParameterTags.h"

class Parameters
{
  public:
    std::atomic<float> values[PARAM_COUNT]{};
    std::atomic<juce::String *> namePtr{new juce::String("Default")};

    Parameters() { setDefaultValues(); }
    ~Parameters() { delete namePtr.load(); }

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
            value = 0.f;
        }

        values[VOICE_COUNT] = 0.2258f;     // 8 voices
        values[PITCH_BEND_UP] = 0.0417f;   // 2 semitones
        values[PITCH_BEND_DOWN] = 0.0417f; // 2 semitones
        values[BRIGHTNESS] = 1.f;
        values[LSUS] = 1.f;
        values[CUTOFF] = 1.f;
        values[VOLUME] = 0.5f;
        values[OSC1MIX] = 1.f;
        values[OSC2MIX] = 1.f;
        values[OSC1Saw] = 1.f;
        values[OSC2Saw] = 1.f;
        values[BENDLFORATE] = 0.2f; // 4 Hz
        values[LFOFREQ] = 0.5f;     // 4 Hz
        values[LFOSQUAREWAVE] = 0.5f;
        values[LFOSHWAVE] = 0.5f;
        values[ECONOMY_MODE] = 1.f;
        values[ENVDER] = 0.25f;
        values[FILTERDER] = 0.25f;
        values[LEVEL_DIF] = 0.25f;
        values[PORTADER] = 0.25f;
        values[UDET] = 0.25f;
    }

    // JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
};

#endif // OBXF_SRC_ENGINE_PARAMETERS_H
