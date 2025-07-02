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

#ifndef OBXF_SRC_GUI_LABEL_H
#define OBXF_SRC_GUI_LABEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../components/ScaleComponent.h"

class Label final : public juce::Drawable, public ScalableComponent
{

  public:
    Label(const juce::String &name, int fh, ObxfAudioProcessor *owner_)
        : ScalableComponent(owner_), img_name(std::move(name)), frameHeight(fh), currentFrame(0),
          owner(owner_)
    {
        scaleFactorChanged();

        if (frameHeight > 0 && label.isValid())
            totalFrames = label.getHeight() / frameHeight;
        else
            totalFrames = 1;
    }

    void scaleFactorChanged() override
    {
        label = getScaledImageFromCache(img_name);
        repaint();
    }

    void setCurrentFrame(int frameIndex)
    {
        currentFrame = juce::jlimit(0, totalFrames - 1, frameIndex);
    }

    int getCurrentFrame() const { return currentFrame; }

    void paint(juce::Graphics &g) override
    {
        if (!label.isValid() || frameHeight <= 0)
            return;

        juce::Rectangle<int> sourceRect(0, currentFrame * frameHeight, label.getWidth(),
                                        frameHeight);

        g.drawImage(label, bounds.toFloat(), juce::RectanglePlacement::centred, false);
    }

    juce::Rectangle<float> getDrawableBounds() const override { return bounds.toFloat(); }

    void setDrawableBounds(juce::Rectangle<int> newBounds) { bounds = newBounds; }

    juce::Path getOutlineAsPath() const override
    {
        juce::Path path;
        path.addRectangle(bounds);
        return path;
    }

    std::unique_ptr<juce::Drawable> createCopy() const override
    {
        auto copy = std::make_unique<Label>(img_name, frameHeight, nullptr);
        copy->setCurrentFrame(currentFrame);
        return std::move(copy);
    }

  private:
    juce::String img_name;
    juce::Image label;
    int frameHeight;
    int currentFrame;
    int totalFrames;
    juce::Rectangle<int> bounds;
    juce::AudioProcessor *owner{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Label)
};

#endif // OBXF_SRC_GUI_LABEL_H