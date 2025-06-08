/*
        ==============================================================================
        This file is part of Obxd synthesizer.

        Copyright � 2013-2014 Filatov Vadim

        Contact author via email :
        justdat_@_e1.ru

        This file may be licensed under the terms of of the
        GNU General Public License Version 2 (the ``GPL'').

        Software distributed under the License is distributed
        on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
        express or implied. See the GPL for the specific language
        governing rights and limitations.

        You should have received a copy of the GPL along with this
        program. If not, go to http://www.gnu.org/licenses/gpl.html
        or write to the Free Software Foundation, Inc.,
        51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
        ==============================================================================
 */
#pragma once
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
