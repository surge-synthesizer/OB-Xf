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

#ifndef OBXF_SRC_GUI_BUTTONLIST_H
#define OBXF_SRC_GUI_BUTTONLIST_H

#include <utility>

#include "../src/engine/SynthEngine.h"
#include "../components/ScalingImageCache.h"
#include "HasScaleFactor.h"
#include "LookAndFeel.h"

#include "sst/plugininfra/misc_platform.h"

class ButtonList final : public juce::ComboBox, public HasScaleFactor, public HasParameterWithID
{
    juce::String img_name;
    ScalingImageCache &imageCache;
    bool isSVG{false};
    bool forceEmbedded{false};

  public:
    ButtonList(juce::String assetName, const int fh, ScalingImageCache &cache,
               ObxfAudioProcessor *owner_, bool forceEmbedded_ = false)
        : juce::ComboBox("cb"), img_name(std::move(assetName)), imageCache(cache),
          forceEmbedded(forceEmbedded_), owner(owner_)
    {
        ButtonList::scaleFactorChanged();
        h2 = fh;
        w2 = kni.getWidth();
    }

    juce::AudioProcessorParameterWithID *getParameterWithID() override { return parameter; }

    ~ButtonList() override {}

    void scaleFactorChanged() override
    {
        if (forceEmbedded)
        {
            isSVG = true;
        }
        else if (imageCache.isSVG(img_name.toStdString()))
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

    void setParameter(juce::AudioProcessorParameterWithID *p)
    {
        if (parameter == p)
            return;

        parameter = p;

        repaint();
    }

    void addChoice(const juce::String &name) { addItem(name, ++count); }

    float getValue() const
    {
        if (count <= 1)
            return 0.f;
        const int curValNorm = getSelectedItemIndex();
        return static_cast<float>(curValNorm) / static_cast<float>(count - 1);
    }

    void setValue(const float val, const juce::NotificationType notify)
    {
        if (count <= 1)
            return;
        const int newValNorm =
            juce::jlimit(0, count - 1, static_cast<int>(std::round(val * (count - 1))));
        setSelectedItemIndex(newValNorm, notify);
    }

    void paint(juce::Graphics &g) override
    {
        if (forceEmbedded)
        {
            auto svgi = imageCache.getEmbeddedVectorDrawable(img_name.toStdString());
            const float scale = getWidth() * 1.0 / svgi->getWidth();
            auto tf = juce::AffineTransform().scaled(scale).translated(
                0, -scale * h2 * getSelectedItemIndex());
            svgi->draw(g, 1.f, tf);
        }
        else if (isSVG)
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            const float scale = getWidth() * 1.0 / svgi->getWidth();
            auto tf = juce::AffineTransform().scaled(scale).translated(
                0, -scale * h2 * getSelectedItemIndex());
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
            const int srcY = h2 * getSelectedItemIndex() * scale;

            g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, srcY, srcW, srcH);
        }
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (owner && parameter)
        {
            if (auto *obxf = dynamic_cast<ObxfAudioProcessor *>(owner))
            {
                obxf->setLastUsedParameter(parameter->paramID);
            }
        }

        if (event.mods.isRightButtonDown())
        {
            juce::ComboBox::showPopup();
        }
        else
        {
            juce::ComboBox::mouseDown(event);
        }
    }

    std::optional<sst::basic_blocks::params::ParamMetaData> getMetadata()
    {
        auto op = dynamic_cast<ObxfParameterFloat *>(parameter);

        if (op)
            return op->meta;
        else
            return std::nullopt;
    }

  private:
    int count{0};
    juce::Image kni;
    int w2, h2;
    juce::AudioProcessorParameterWithID *parameter{nullptr};
    juce::AudioProcessor *owner{nullptr};
};

#endif // OBXF_SRC_GUI_BUTTONLIST_H
