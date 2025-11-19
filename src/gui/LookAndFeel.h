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

#ifndef OBXF_SRC_GUI_LOOKANDFEEL_H
#define OBXF_SRC_GUI_LOOKANDFEEL_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "BinaryData.h"

namespace obxf
{

class LookAndFeel_DocumentWindowButton final : public juce::Button
{
  public:
    LookAndFeel_DocumentWindowButton(const juce::String &name, const juce::Colour c,
                                     const juce::Path &normal, const juce::Path &toggled)
        : Button(name), colour(c), normalShape(normal), toggledShape(toggled)
    {
    }

    void paintButton(juce::Graphics &g, const bool shouldDrawButtonAsHighlighted,
                     bool /*shouldDrawButtonAsDown*/) override
    {
        if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(juce::Colour(96, 96, 96));
            g.fillAll();
        }

        g.setColour(colour);
        const auto &p = getToggleState() ? toggledShape : normalShape;

        const auto reducedRect =
            juce::Justification(juce::Justification::centred)
                .appliedToRectangle(juce::Rectangle(getHeight(), getHeight()), getLocalBounds())
                .toFloat()
                .reduced(static_cast<float>(getHeight()) * 0.25f);

        g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
    }

  private:
    juce::Colour colour;
    juce::Path normalShape, toggledShape;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LookAndFeel_DocumentWindowButton)
};

class LookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    LookAndFeel()
    {
        loadSvgIcon();

        using namespace juce;

        setColour(DocumentWindow::backgroundColourId, Colour(48, 48, 48));
        setColour(TextButton::buttonColourId, Colour(32, 32, 32));
        setColour(TextEditor::backgroundColourId, Colour(32, 32, 32));
        setColour(TextEditor::highlightColourId, Colour(96, 96, 96));
        setColour(ListBox::backgroundColourId, Colour(32, 32, 32));
        setColour(ScrollBar::thumbColourId, Colour(212, 212, 212));
        setColour(ScrollBar::trackColourId, Colour(128, 128, 128));
        setColour(Slider::thumbColourId, Colour(212, 212, 212));
        setColour(Slider::trackColourId, Colour(128, 128, 128));
        setColour(Slider::backgroundColourId, Colour((uint8)255, 255, 255, 20.f));
        setColour(ComboBox::backgroundColourId, Colour(32, 32, 32));
        setColour(PopupMenu::backgroundColourId, Colour(32, 32, 32));
        setColour(PopupMenu::highlightedBackgroundColourId, Colour(64, 64, 64));
        setColour(AlertWindow::backgroundColourId, Colour(48, 48, 48));
        setColour(AlertWindow::outlineColourId, Colour(96, 96, 96));
        setColour(AlertWindow::textColourId, Colours::white);
    }

    void drawDocumentWindowTitleBar(juce::DocumentWindow &window, juce::Graphics &g, const int w,
                                    const int h, int, int, const juce::Image *, bool) override
    {
        g.fillAll(juce::Colour(48, 48, 48));
        g.setColour(juce::Colours::white);

        const auto wt = window.getName();

        juce::String obxfLabel = "OB-Xf";
        juce::String obxfVersion = OBXF_VERSION_STR;

#ifndef NDEBUG
        obxfVersion = OBXF_VERSION_STR;
        obxfVersion += " DEBUG";
#else
        obxfVersion = "";
#endif

        if (wt != "OB-Xf")
        {
            obxfLabel = "OB-Xf -";
            obxfVersion = wt;
        }

        const juce::Font fontLabel{juce::FontOptions(18.0f, juce::Font::bold)};
        const juce::Font fontVersion{juce::FontOptions(14.0f)};

        juce::GlyphArrangement labelArrangement;
        labelArrangement.addLineOfText(fontLabel, obxfLabel, 0, 0);
        const float sw = labelArrangement.getBoundingBox(0, obxfLabel.length(), true).getWidth();

        juce::GlyphArrangement versionArrangement;
        versionArrangement.addLineOfText(fontVersion, obxfVersion, 0, 0);
        const float vw =
            versionArrangement.getBoundingBox(0, obxfVersion.length(), true).getWidth();

        const auto titleCenter = w / 2;
        int textMargin = 0;
#ifndef NDEBUG
        textMargin = 5;
#endif
        const auto titleTextWidth = sw + vw + static_cast<float>(textMargin);

        if (svgIcon)
        {
            const juce::Rectangle iconBounds(static_cast<float>(titleCenter) -
                                                 (titleTextWidth / 2) - 14 -
                                                 static_cast<float>(textMargin),
                                             static_cast<float>(h) / 2 - 7, 12.0f, 14.0f);
            svgIcon->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
        }

        const auto boxSurge = juce::Rectangle<float>(
            static_cast<float>(titleCenter) - (titleTextWidth / 2), 0, sw, static_cast<float>(h));
        g.drawText(obxfLabel, boxSurge, juce::Justification::centredLeft);

        const auto boxVersion =
            juce::Rectangle<float>(static_cast<float>(titleCenter) - (titleTextWidth / 2) + sw +
                                       static_cast<float>(textMargin),
                                   0, vw, static_cast<float>(h));
        g.drawText(obxfVersion, boxVersion, juce::Justification::centredLeft);
    }

    juce::Button *createDocumentWindowButton(const int buttonType) override
    {
        juce::Path shape;
        constexpr auto crossThickness = 0.25f;

        if (buttonType == juce::DocumentWindow::closeButton)
        {
            shape.addLineSegment({0.0f, 0.0f, 1.0f, 1.0f}, crossThickness);
            shape.addLineSegment({1.0f, 0.0f, 0.0f, 1.0f}, crossThickness);
            return new LookAndFeel_DocumentWindowButton("close", juce::Colour(212, 64, 64), shape,
                                                        shape);
        }

        if (buttonType == juce::DocumentWindow::minimiseButton)
        {
            shape.addLineSegment({0.0f, 0.0f, 1.0f, 0.0f}, 0.0f);
            shape.addLineSegment({0.0f, 0.9f, 1.0f, 0.9f}, crossThickness);
            return new LookAndFeel_DocumentWindowButton("minimize", juce::Colour(255, 212, 32),
                                                        shape, shape);
        }

        return nullptr;
    }

    void drawPopupMenuBackgroundWithOptions(juce::Graphics &g, int width, int height,
                                            const juce::PopupMenu::Options &o) override
    {
        auto background = findColour(juce::PopupMenu::backgroundColourId);

        g.fillAll(background);

        g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.6f));
        g.drawRect(0, 0, width, height);
    }

  private:
    std::unique_ptr<juce::Drawable> svgIcon;
    void loadSvgIcon()
    {
        const juce::String svgString =
            juce::String::fromUTF8(BinaryData::icon_svg, BinaryData::icon_svgSize);
        if (svgString.isEmpty())
        {
            return;
        }
        if (const auto xml = juce::XmlDocument::parse(svgString))
        {
            svgIcon = juce::Drawable::createFromSVG(*xml);
        }
    }
};
#endif // OBXF_SRC_GUI_LOOKANDFEEL_H

} // namespace obxf