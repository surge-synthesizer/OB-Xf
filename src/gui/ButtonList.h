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

class ButtonList final : public juce::ComboBox
{
    juce::String img_name;
    ScalingImageCache &imageCache;

  public:
    ButtonList(juce::String assetName, const int fh, ScalingImageCache &cache)
        : ComboBox("cb"), img_name(std::move(assetName)), imageCache(cache)
    {
        ButtonList::scaleFactorChanged();
        h2 = fh;
        w2 = kni.getWidth();
        setLookAndFeel(&lookAndFeel);
    }

    ~ButtonList() override { setLookAndFeel(nullptr); }

    void scaleFactorChanged()
    {
        kni = imageCache.getImageFor(img_name.toStdString(), getWidth(), h2);
        repaint();
    }

    void resized() override { scaleFactorChanged(); }

    void setParameter(const juce::AudioProcessorParameter *p)
    {
        if (parameter == p)
            return;

        parameter = p;

        repaint();
    }

    void setRange(const int from, const int to)
    {
        min = from;
        max = to;
        range = abs(max - min);
    }

    void addChoice(const juce::String &name) { addItem(name, ++count); }

    float getValue() const
    {
        const auto curValNorm = getSelectedItemIndex();

        return juce::jmap((float)curValNorm, 0.f, (float)range, (float)min, (float)max);
    }

    void setValue(const float val, const juce::NotificationType notify)
    {
        auto newValNorm = juce::jlimit(min, max, static_cast<int>(val));

        setSelectedItemIndex(newValNorm, notify);
    }

    void paint(juce::Graphics &g) override
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

  private:
    int count{0};
    int range{0};
    int min{0};
    int max{0};
    juce::Image kni;
    int w2, h2;
    const juce::AudioProcessorParameter *parameter{nullptr};
    ButtonListLookAndFeel lookAndFeel;
};

#endif // OBXF_SRC_GUI_BUTTONLIST_H
