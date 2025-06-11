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

#ifndef OBXF_SRC_GUI_KNOB_H
#define OBXF_SRC_GUI_KNOB_H

#include <utility>

#include "../Source/Engine/SynthEngine.h"
#include "../Components/ScaleComponent.h"

class ObxfAudioProcessor;

class KnobLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    KnobLookAndFeel()
    {
        setColour(juce::BubbleComponent::ColourIds::backgroundColourId,
                  juce::Colours::white.withAlpha(0.8f));
        setColour(juce::BubbleComponent::ColourIds::outlineColourId,
                  juce::Colours::transparentBlack);
        setColour(juce::TooltipWindow::textColourId, juce::Colours::black);
    }

    int getSliderPopupPlacement(juce::Slider &) override
    {
        return juce::BubbleComponent::BubblePlacement::above;
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobLookAndFeel)
};

class Knob final : public juce::Slider, public ScalableComponent, public juce::ActionBroadcaster
{
    juce::String img_name;

  public:
    Knob(juce::String name, const int fh, ObxfAudioProcessor *owner_)
        : Slider("Knob"), ScalableComponent(owner_), img_name(std::move(name))
    {
        scaleFactorChanged();

        h2 = fh;
        w2 = kni.getWidth();
        numFr = kni.getHeight() / h2;
        setLookAndFeel(&lookAndFeel);
        setVelocityModeParameters(1.0, 1, 0.0, true, juce::ModifierKeys::ctrlModifier);
    }

    ~Knob() override { setLookAndFeel(nullptr); }

    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent &event) override
    {
        Slider::mouseDrag(event);
        if (event.mods.isShiftDown())
        {
            if (shiftDragCallback)
            {
                setValue(shiftDragCallback(getValue()), juce::sendNotificationAsync);
            }
        }
        if (event.mods.isAltDown())
        {
            if (altDragCallback)
            {
                setValue(altDragCallback(getValue()), juce::sendNotificationAsync);
            }
        }
        if (alternativeValueMapCallback)
        {
            setValue(alternativeValueMapCallback(getValue()), juce::sendNotificationAsync);
        }
    }

    // Source:
    // https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
  public:
    class KnobAttachment final : public juce::AudioProcessorValueTreeState::SliderAttachment
    {
        juce::RangedAudioParameter *parameter = nullptr;
        Knob *sliderToControl = nullptr;

      public:
        KnobAttachment(juce::AudioProcessorValueTreeState &stateToControl,
                       const juce::String &parameterID, Knob &sliderToControl)
            : juce::AudioProcessorValueTreeState::SliderAttachment(stateToControl, parameterID,
                                                                   sliderToControl),
              sliderToControl(&sliderToControl)
        {
            parameter = stateToControl.getParameter(parameterID);
            sliderToControl.setParameter(parameter);
        }

        void updateToSlider() const
        {
            const float val = parameter->getValue();
            sliderToControl->setValue(val, juce::NotificationType::dontSendNotification);
        }

        ~KnobAttachment() = default;
    };

    void setParameter(juce::AudioProcessorParameter *p)
    {
        if (parameter == p)
            return;

        parameter = p;
        updateText();
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
        const int ofs = static_cast<int>((getValue() - getMinimum()) /
                                         (getMaximum() - getMinimum()) * (numFr - 1));
        g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, h2 * ofs, w2, h2);
    }

    std::function<double(double)> shiftDragCallback, altDragCallback, alternativeValueMapCallback;

  private:
    juce::Image kni;
    int numFr;
    int w2, h2;
    juce::AudioProcessorParameter *parameter{nullptr};
    KnobLookAndFeel lookAndFeel;
};

#endif // OBXF_SRC_GUI_KNOB_H
