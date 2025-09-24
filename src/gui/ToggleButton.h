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

#ifndef OBXF_SRC_GUI_TOGGLEBUTTON_H
#define OBXF_SRC_GUI_TOGGLEBUTTON_H

#include <utility>

#include "../src/engine/SynthEngine.h"
#include "../components/ScalingImageCache.h"
#include "HasScaleFactor.h"

class ToggleButton final : public juce::ImageButton, public HasScaleFactor
{
    juce::String img_name;
    ScalingImageCache &imageCache;
    bool isSVG{false};

  public:
    ToggleButton(juce::String name, const int fh, ScalingImageCache &cache,
                 ObxfAudioProcessor *owner_)
        : img_name(std::move(name)), imageCache(cache), owner(owner_)
    {
        scaleFactorChanged();

        if (!isSVG)
        {
            width = kni.getWidth();
            height = kni.getHeight();
        }
        else
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            width = svgi->getWidth();
            height = svgi->getHeight();
        }
        h2 = fh;
        w2 = width;
        if (h2 > 0)
            numFr = height / h2;
        else
            numFr = 1;

        setClickingTogglesState(true);
    }

    void scaleFactorChanged() override
    {
        if (imageCache.isSVG(img_name.toStdString()))
        {
            isSVG = true;
        }
        else
        {
            isSVG = false;
            kni = imageCache.getImageFor(img_name.toStdString(), getWidth(), h2);
        }

        repaint();
    }

    void resized() override { scaleFactorChanged(); }

    ~ToggleButton() override = default;

    void paintButton(juce::Graphics &g, bool /*isMouseOverButton*/, bool isButtonDown) override
    {
        int offset = isButtonDown ? 1 : 0;
        if (getToggleState() && numFr > 2)
            offset += 2;

        if (isSVG)
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            const float scale = getWidth() * 1.0 / svgi->getWidth();
            auto tf = juce::AffineTransform().scaled(scale).translated(0, -scale * h2 * offset);
            svgi->draw(g, 1.f, tf);
        }
        else
        {
            const int zoomLevel =
                imageCache.zoomLevelFor(img_name.toStdString(), getWidth(), getHeight());
            constexpr int baseZoomLevel = 100;
            const float scale = static_cast<float>(zoomLevel) / static_cast<float>(baseZoomLevel);

            const int srcW = w2 * scale;
            const int srcH = h2 * scale;
            const int srcY = offset * h2 * scale;

            g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, srcY, srcW, srcH);
        }
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (owner != nullptr && parameter != nullptr)
        {
            if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
                obxf->setLastUsedParameter(parameter->paramID);
        }
        ImageButton::mouseDown(event);
    }

    void setParameter(juce::AudioProcessorParameterWithID *p) { parameter = p; }

  private:
    juce::Image kni;
    int width, height, numFr, w2, h2;
    juce::AudioProcessor *owner{nullptr};
    juce::AudioProcessorParameterWithID *parameter{nullptr};
};

#endif // OBXF_SRC_GUI_TOGGLEBUTTON_H
