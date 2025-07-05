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

#ifndef OBXF_SRC_GUI_MULTISTATEBUTTON_H
#define OBXF_SRC_GUI_MULTISTATEBUTTON_H

#include <utility>

#include "../src/engine/SynthEngine.h"
#include "../components/ScalableComponent.h"

class ObxfAudioProcessor;

class MultiStateButton final : public juce::Slider, public ScalableComponent
{
    juce::String img_name;

  public:
    MultiStateButton(juce::String name, ObxfAudioProcessor *owner, uint8_t states = 3)
        : Slider(), ScalableComponent(owner), img_name(std::move(name)), numStates(states)
    {
        numFrames = numStates * 2;

        scaleFactorChanged();

        width = kni.getWidth();
        height = kni.getHeight();
        h2 = height / numFrames;
        stepSize = 1.f / static_cast<float>(numStates - 1);

        setRange(0.f, 1.f, stepSize);
    }

    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name);
        repaint();
    }

    ~MultiStateButton() override = default;

    // Source:
    // https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
  public:
    class MultiStateAttachment final : public juce::AudioProcessorValueTreeState::SliderAttachment
    {
        juce::RangedAudioParameter *parameter = nullptr;
        MultiStateButton *buttonToControl = nullptr;

      public:
        MultiStateAttachment(juce::AudioProcessorValueTreeState &stateToControl,
                             const juce::String &parameterID, MultiStateButton &buttonToControl)
            : juce::AudioProcessorValueTreeState::SliderAttachment(stateToControl, parameterID,
                                                                   buttonToControl),
              buttonToControl(&buttonToControl)
        {
            parameter = stateToControl.getParameter(parameterID);
            buttonToControl.setParameter(parameter);
        }

        void updateToSlider() const
        {
            const float val = parameter->getValue();
            buttonToControl->setValue(val, juce::dontSendNotification);
        }

        ~MultiStateAttachment() = default;
    };

    void mouseDrag(const juce::MouseEvent & /*event*/) override { return; }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (event.mods.isLeftButtonDown())
        {
            counter = (counter + 1) % numStates;
            setValue((double)counter / (numStates - 1));
            repaint();
            isMousePressed = true;
        }
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        if (isMousePressed && event.mods.isLeftButtonDown())
        {
            isMousePressed = false;
            repaint();
        }
    }

    void setParameter(juce::AudioProcessorParameterWithID *p)
    {
        if (parameter == p)
            return;

        parameter = p;
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
        g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, ((counter * 2) + isMousePressed) * h2,
                    width, h2);
    }

  private:
    juce::AudioProcessorParameterWithID *parameter{nullptr};
    juce::Image kni;
    bool isMousePressed{false};
    int counter{0};
    int numStates{3}, numFrames{6};
    int width, height, h2;
    float stepSize{0.5};
};

#endif // OBXF_SRC_GUI_MULTISTATEBUTTON_H
