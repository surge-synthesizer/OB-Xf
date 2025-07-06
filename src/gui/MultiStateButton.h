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

    void mouseDrag(const juce::MouseEvent & /*event*/) override { return; }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (mouseButtonPressed == None)
        {
            const auto isLeft = event.mods.isLeftButtonDown();
            const auto isRight = event.mods.isRightButtonDown();

            if (isLeft || isRight)
            {
                counter = (counter + numStates + (isRight ? -1 : 1)) % numStates;

                setValue((double)counter / (numStates - 1));
                repaint();

                mouseButtonPressed = isRight ? Right : Left;
            }
        }
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        if ((mouseButtonPressed == Left && event.mods.isLeftButtonDown()) ||
            (mouseButtonPressed == Right && event.mods.isRightButtonDown()))
        {
            mouseButtonPressed = None;

            repaint();
        }
    }

    void paint(juce::Graphics &g) override
    {
        g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0,
                    ((counter * 2) + (mouseButtonPressed > None)) * h2, width, h2);
    }

  private:
    juce::Image kni;

    enum MouseButtonPressed
    {
        None,
        Left,
        Right
    } mouseButtonPressed{None};

    int counter{0};
    int numStates{3}, numFrames{6};
    int width, height, h2;
    float stepSize{0.5};
};

#endif // OBXF_SRC_GUI_MULTISTATEBUTTON_H
