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

#ifndef OBXF_SRC_GUI_TOOGLABLEBUTTON_H
#define OBXF_SRC_GUI_TOOGLABLEBUTTON_H
#include <utility>

#include "../Source/Engine/SynthEngine.h"
#include "../Components/ScaleComponent.h"
class ObxdAudioProcessor;

class TooglableButton final : public juce::ImageButton, public ScalableComponent
{
    juce::String img_name;

  public:
    TooglableButton(juce::String name, ObxdAudioProcessor *owner)
        : ImageButton(), ScalableComponent(owner), img_name(std::move(name))
    {
        scaleFactorChanged();
        width = kni.getWidth();
        height = kni.getHeight();
        w2 = width;
        h2 = height / 2;
        this->setClickingTogglesState(true);
    }
    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name);
        repaint();
    }
    ~TooglableButton() override = default;
    // Source:
    // https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
  public:
    class ToggleAttachment final : public juce::AudioProcessorValueTreeState::ButtonAttachment
    {
        juce::RangedAudioParameter *parameter = nullptr;
        TooglableButton *buttonToControl = nullptr;

      public:
        ToggleAttachment(juce::AudioProcessorValueTreeState &stateToControl,
                         const juce::String &parameterID, TooglableButton &buttonToControl)
            : juce::AudioProcessorValueTreeState::ButtonAttachment(stateToControl, parameterID,
                                                                   buttonToControl),
              buttonToControl(&buttonToControl)
        {
            parameter = stateToControl.getParameter(parameterID);
        }

        void updateToSlider() const
        {
            const float val = parameter->getValue();
            //  DBG("Toggle Parameter: " << parameter->name << " Val: " << val);
            buttonToControl->setToggleState(val, juce::NotificationType::dontSendNotification);
        }

        ~ToggleAttachment() = default;
    };

    void paintButton(juce::Graphics &g, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        int offset = 0;

        // if (toogled)
        if (getToggleState())
        {
            offset = 1;
        }

        g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, offset * h2, w2, h2);
    }

  private:
    juce::Image kni;
    int width, height, w2, h2;
};

#endif // OBXF_SRC_GUI_TOOGLABLEBUTTON_H
