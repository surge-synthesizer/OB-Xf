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

class ButtonListLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    ButtonListLookAndFeel()
    {
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(32, 32, 32));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(64, 64, 64));
        setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonListLookAndFeel)
};

class ButtonList final : public juce::ComboBox, public HasScaleFactor
{
    juce::String img_name;
    ScalingImageCache &imageCache;
    bool isSVG{false};

  public:
    ButtonList(juce::String assetName, const int fh, ScalingImageCache &cache,
               ObxfAudioProcessor *owner_)
        : ComboBox("cb"), img_name(std::move(assetName)), imageCache(cache), owner(owner_)
    {
        ButtonList::scaleFactorChanged();
        h2 = fh;
        w2 = kni.getWidth();
        setLookAndFeel(&lookAndFeel);
    }

    ~ButtonList() override { setLookAndFeel(nullptr); }

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

    void setParameter(const juce::AudioProcessorParameterWithID *p)
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
        if (isSVG)
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
        ComboBox::mouseDown(event);
    }

  private:
    int count{0};
    juce::Image kni;
    int w2, h2;
    const juce::AudioProcessorParameterWithID *parameter{nullptr};
    juce::AudioProcessor *owner{nullptr};
    ButtonListLookAndFeel lookAndFeel;
};

#endif // OBXF_SRC_GUI_BUTTONLIST_H
