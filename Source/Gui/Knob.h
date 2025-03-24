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
#include <utility>

#include "../Source/Engine/SynthEngine.h"
#include "../Components/ScaleComponent.h"
class ObxdAudioProcessor;

class KnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel()
    {
        setColour(juce::BubbleComponent::ColourIds::backgroundColourId, juce::Colours::white.withAlpha(0.8f));
        setColour(juce::BubbleComponent::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        setColour(juce::TooltipWindow::textColourId, juce::Colours::black);
    }
    int getSliderPopupPlacement(juce::Slider&) override
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
	Knob (juce::String name, const int fh, ObxdAudioProcessor* owner_) : Slider("Knob"), ScalableComponent(owner_), img_name(std::move(name))
	{
        scaleFactorChanged();
        
		h2 = fh;
        w2 = kni.getWidth();
		numFr = kni.getHeight() / h2;
        setLookAndFeel(&lookAndFeel);
        setVelocityModeParameters(1.0, 1, 0.0, true, juce::ModifierKeys::ctrlModifier);
	}

    ~Knob() override
    {
        setLookAndFeel(nullptr);
    }

    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isShiftDown())
        {
            if (shouldResetOnShiftClick)
            {
                sendActionMessage(resetActionMessage);
            }
        }
        Slider::mouseDown(event);
    }

    void mouseDrag(const juce::MouseEvent& event) override
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

// Source: https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
public:
    class KnobAttachment final : public juce::AudioProcessorValueTreeState::SliderAttachment
    {
        juce::RangedAudioParameter* parameter = nullptr;
        Knob* sliderToControl = nullptr;
    public:
        KnobAttachment (juce::AudioProcessorValueTreeState& stateToControl,
                        const juce::String& parameterID,
                        Knob& sliderToControl) : juce::AudioProcessorValueTreeState::SliderAttachment (stateToControl, parameterID, sliderToControl), sliderToControl(&sliderToControl)
        {
            parameter = stateToControl.getParameter (parameterID);
            sliderToControl.setParameter (parameter);
        }

        void updateToSlider() const {
            const float val = parameter->getValue();
            sliderToControl->setValue(val, juce::NotificationType::dontSendNotification);
        }

        ~KnobAttachment() = default;
    };
    
    void setParameter (juce::AudioProcessorParameter* p)
    {
        if (parameter == p)
            return;
        
        parameter = p;
        updateText();
        repaint();
    }

	void paint (juce::Graphics& g) override
	{
        const int ofs = static_cast<int>((getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (numFr - 1));
        g.drawImage (kni, 0, 0, getWidth(), getHeight(), 0, h2 * ofs, w2, h2);
	}

    void resetOnShiftClick(const bool value, const juce::String& identifier)
    {
        shouldResetOnShiftClick = value;
        resetActionMessage = identifier;
    }

    std::function<double(double)> shiftDragCallback, altDragCallback, alternativeValueMapCallback;
private:
	juce::Image kni;
	int numFr;
	int w2, h2;
    bool shouldResetOnShiftClick{ false };
    juce::String resetActionMessage{};
    juce::AudioProcessorParameter* parameter {nullptr};
    KnobLookAndFeel lookAndFeel;
};
