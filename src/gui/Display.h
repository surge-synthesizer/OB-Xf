/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_GUI_DISPLAY_H
#define OBXF_SRC_GUI_DISPLAY_H

#include <functional>
#include <configuration.h>
#include <juce_gui_basics/juce_gui_basics.h>

struct TextBubble : public juce::BubbleComponent
{
    juce::TextLayout layout;

    static constexpr int padding = 8;

    TextBubble(const juce::StringArray &lines, const juce::Font &titleFont,
               const juce::Font &rowFont, float parentWidth)
    {
        static constexpr float lineSpacing = 1.15f;

        juce::AttributedString as;
        as.setWordWrap(juce::AttributedString::byWord);
        as.setJustification(juce::Justification::left);
        as.setLineSpacing(rowFont.getHeight() * (lineSpacing - 1.f));

        auto maxWidth = parentWidth * 1.5f;

        for (int i = 0; i < lines.size(); ++i)
        {
            const auto &font = (i == 0) ? titleFont : rowFont;

            as.append(lines[i], font, juce::Colours::white);

            if (i + 1 < lines.size())
            {
                as.append("\n", font, juce::Colours::white);

                maxWidth = std::max(maxWidth,
                                    juce::GlyphArrangement::getStringWidth(rowFont, lines[i + 1]) +
                                        padding);
            }
        }

        layout.createLayout(as, maxWidth);
    }

    void getContentSize(int &w, int &h) override
    {
        w = static_cast<int>(std::ceil(layout.getWidth())) + padding;
        h = static_cast<int>(std::ceil(layout.getHeight())) + padding;
    }

    void paintContent(juce::Graphics &g, int w, int h) override
    {
        g.setColour(findColour(juce::TooltipWindow::textColourId));
        layout.draw(
            g, juce::Rectangle<float>(padding * 0.5f, padding * 0.5f, w - padding, h - padding));
    }
};

class Display final : public juce::Label, private juce::Timer
{
  public:
    Display(const juce::String &name, std::function<float()> gs)
    {
        getScale = gs;
        setName(name);
        setTitle(name);
        setBorderSize(juce::BorderSize(0));
        setEditable(true);
    }

    ~Display() override { dismissBubble(); }

    std::function<juce::StringArray()> getExtraRows{nullptr};

    int bubbleDelay{0};

    void mouseEnter(const juce::MouseEvent &) override
    {
        if (isTextClipped || !extraRows().isEmpty())
        {
            if (bubbleDelay > 0)
            {
                startTimer(bubbleDelay);
            }
            else
            {
                showBubble();
            }
        }
    }

    void mouseExit(const juce::MouseEvent &) override { dismissBubble(); }

    void setText(const juce::String &newText, juce::NotificationType notification)
    {
        dismissBubble();
        juce::Label::setText(newText, notification);
    }

    void editorShown(juce::TextEditor *editor) override
    {
        // a pending bubble would otherwise pop up over the editor
        dismissBubble();

        // sigh, JUCE, you could've fixed this for the label's editor in the past 10 years...
        editor->setJustification(getJustificationType());
        // and let's not get me started on inconsistent indents...
        editor->setIndents(2, -1);
        auto f = juce::Font(editor->getFont());
        f = f.withHeight(f.getHeight() * getScale());
        editor->setFont(f);
        editor->applyFontToAllText(f);
    }

    void paint(juce::Graphics &g) override
    {
        if (isBeingEdited())
        {
            return;
        }

        auto sf = getScale();

        juce::Graphics::ScopedSaveState ss(g);

        g.addTransform(juce::AffineTransform().scaled(sf));

        auto font(getLookAndFeel().getLabelFont(*this));
        auto textArea = getLocalBounds().transformedBy(juce::AffineTransform().scaled(1.0 / sf));
        auto w = juce::GlyphArrangement::getStringWidth(font, getText());
        auto r = std::min(1.f * (textArea.getWidth() - 4) / w, 1.5f);

        isTextClipped = false;

        if (r >= 0.9f)
        {
            font = font.withHeight(font.getHeight() * r);
            w = juce::GlyphArrangement::getStringWidth(font, getText());
            int maxIt{0};

            // 4 above and 8 here? The juce string algo needs a little pad at the margin
            while (w > textArea.getWidth() - 8 && maxIt++ < 5)
            {
                font = font.withHeight(font.getHeight() * 0.97);
                w = juce::GlyphArrangement::getStringWidth(font, getText());
            }

            g.setColour(findColour(Label::textColourId));
            g.setFont(font);
            g.drawText(getText(), textArea.reduced(4, 0), juce::Justification::centred);
        }
        else
        {
            font = font.withHeight(font.getHeight() * 0.9);

            juce::Graphics::ScopedSaveState gs(g);

            g.reduceClipRegion(textArea.reduced(2));
            g.setColour(findColour(Label::textColourId));
            g.setFont(font);

            juce::GlyphArrangement ga;

            ga.addJustifiedText(font, getText(), textArea.getX() + 2,
                                textArea.getY() + font.getHeight() - font.getDescent() + 2,
                                textArea.getWidth() - 4, juce::Justification::centred, -3.f);
            ga.draw(g);

            isTextClipped = ga.getBoundingBox(0, -1, true).getBottom() > textArea.getBottom();
        }
    }

  private:
    std::function<float()> getScale = []() { return 1.0; };
    bool isTextClipped{false};

    juce::Component::SafePointer<TextBubble> bubble;

    juce::StringArray extraRows() const
    {
        return getExtraRows ? getExtraRows() : juce::StringArray{};
    }

    void showBubble()
    {
        dismissBubble();

        auto *topLevel = getTopLevelComponent();

        if (!topLevel)
        {
            return;
        }

        const auto lf = obxf::obxfLookAndFeel(this);
        const auto sf = lf ? lf->editorScaleFactor() : 1.0f;
        const auto font = withDefaultMetrics(juce::FontOptions(15.0f, juce::Font::bold));
        const auto rowFont = withDefaultMetrics(juce::FontOptions(15.0f, juce::Font::plain));
        auto fh = font.getHeight() * 0.75f;

        juce::StringArray lines;

        lines.add(getText());
        lines.addArray(extraRows());

        auto *b = new TextBubble(lines, font.withHeight(fh * sf), rowFont.withHeight(fh * sf),
                                 static_cast<float>(getWidth()));
        bubble = b;

        topLevel->addAndMakeVisible(b);

        b->setAllowedPlacement(juce::BubbleComponent::above);
        b->setPosition(topLevel->getLocalArea(nullptr, getScreenBounds()));
    }

    void timerCallback() override
    {
        stopTimer();
        showBubble();
    }

    void dismissBubble()
    {
        stopTimer();

        if (bubble != nullptr)
        {
            bubble->setVisible(false);

            // Safe even if already deleted; SafePointer will be null.
            delete bubble.getComponent();

            bubble = nullptr;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Display)
};

class DisplayDigits : public juce::Component, HasScaleFactor
{
    juce::String img_name;
    ScalingImageCache &imageCache;
    bool isSVG{false};

  public:
    DisplayDigits(juce::String assetName, const int fh, ScalingImageCache &cache)
        : img_name(std::move(assetName)), imageCache(cache)
    {
        scaleFactorChanged();
        h2 = fh;
        w2 = kni.getWidth();
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

    void setFrame(const float val)
    {
        frame = val;
        repaint();
    }

    int getFrame() const { return frame; }

    void resized() override { scaleFactorChanged(); }

    std::function<void()> onClick{nullptr};

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (onClick)
            onClick();
    }

    void paint(juce::Graphics &g) override
    {
        if (isSVG)
        {
            auto &svgi = imageCache.getSVGDrawable(img_name.toStdString());
            const float scale = getWidth() * 1.0 / svgi->getWidth();
            auto tf = juce::AffineTransform().scaled(scale).translated(0, -scale * h2 * frame);
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
            const int srcY = h2 * frame * scale;

            g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, srcY, srcW, srcH);
        }
    }

  private:
    int frame{0};
    juce::Image kni;
    int w2, h2;
};

#endif // OBXF_SRC_GUI_DISPLAY_H