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
#include "../components/ScalingImageCache.h"
#include "HasScaleFactor.h"

class Label final : public juce::Component, public HasScaleFactor
{
  public:
    Label(const juce::String &name, int fh, ScalingImageCache &cache)
        : img_name(std::move(name)), frameHeight(fh), currentFrame(0), imageCache(cache)
    {
        scaleFactorChanged();
        if (isSVG && frameHeight > 0)
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            totalFrames = svgi->getHeight() / frameHeight;
        }
        else if (frameHeight > 0 && label.isValid())
        {
            totalFrames = label.getHeight() / frameHeight;
        }
        else
        {
            totalFrames = 1;
        }
    }

    void scaleFactorChanged() override
    {
        if (imageCache.isSVG(img_name.toStdString()))
        {
            isSVG = true;
        }
        else
        {
            label = imageCache.getImageFor(img_name.toStdString(), getWidth(), frameHeight);
        }
        repaint();
    }

    void resized() override { scaleFactorChanged(); }

    void setCurrentFrame(int frameIndex)
    {
        auto nextFrame  = juce::jlimit(0, totalFrames - 1, frameIndex);
        if (nextFrame != currentFrame)
        {
            currentFrame = nextFrame;
            repaint();
        }
    }

    int getCurrentFrame() const { return currentFrame; }

    void paint(juce::Graphics &g) override
    {
        if (frameHeight <= 0)
        {
            return;
        }

        if (isSVG)
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            const float scale = getWidth() * 1.0 / svgi->getWidth();
            auto tf = juce::AffineTransform().scaled(scale).translated(0, -scale * frameHeight *
                                                                              currentFrame);

            juce::Graphics::ScopedSaveState ss(g);
            g.reduceClipRegion(getLocalBounds().withHeight(std::ceil(scale * frameHeight)));
            svgi->draw(g, isEnabled() ? 1.f : 0.5f, tf);
        }
        else
        {
            if (!label.isValid() || frameHeight <= 0)
                return;

            const int zoomLevel =
                imageCache.zoomLevelFor(img_name.toStdString(), getWidth(), getHeight());
            constexpr int baseZoomLevel = 100;
            const float scale = static_cast<float>(zoomLevel) / static_cast<float>(baseZoomLevel);

            const int srcY = static_cast<int>(currentFrame * frameHeight * scale);
            const int srcH = static_cast<int>(frameHeight * scale);

            juce::Image frame = label.getClippedImage({0, srcY, label.getWidth(), srcH});
            juce::Rectangle<float> targetRect(0, 0, static_cast<float>(getWidth()),
                                              static_cast<float>(getHeight()));

            {
                juce::Graphics::ScopedSaveState ss(g);
                if (!isEnabled())
                    g.setOpacity(0.5f);
                g.drawImage(frame, targetRect, juce::RectanglePlacement::stretchToFit, false);
            }
        }
    }

  private:
    juce::String img_name;
    juce::Image label;
    int frameHeight;
    int currentFrame;
    int totalFrames;
    ScalingImageCache &imageCache;
    bool isSVG{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Label)
};

#endif // OBXF_SRC_GUI_LABEL_H
