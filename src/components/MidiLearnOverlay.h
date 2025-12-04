
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

#ifndef OBXF_SRC_COMPONENTS_MIDILEARNOVERLAY_H
#define OBXF_SRC_COMPONENTS_MIDILEARNOVERLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <algorithm>

enum class AnchorPosition
{
    Below,
    Above,
    Overlay,
    Left,
    Right
};

class MidiLearnOverlay : public juce::Component
{
  public:
    static constexpr int noCCSentinel{1000};

    MidiLearnOverlay(Component *anchor, std::function<int(Component *)> getCC,
                     const AnchorPosition position = AnchorPosition::Overlay)
        : anchorComp(anchor), getCCForComp(std::move(getCC)), anchorPosition(position)
    {
    }

    void setScaleFactor(const float sf)
    {
        scaleFactor = std::max(0.01f, sf);
        updatePosition();
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (onSelectionCallback)
            onSelectionCallback(this);

        if (std::abs(getCCForComp(anchorComp)) != noCCSentinel)
        {
            // hack for now
            OBLOGONCE(midiLearn, "This mouse detection is bad");

            if (event.position.x < 15)
            {
                if (onClearCallback)
                    onClearCallback(anchorComp);
            }
        }
    }

    void paint(juce::Graphics &g) override
    {
        const int cc = getCCForComp(anchorComp);
        const auto boxRect = getLocalBounds();
        const int tailSize = midiLearnTailSize;

        juce::Path bubblePath;
        juce::Rectangle<float> bubbleRect;

        switch (anchorPosition)
        {
        case AnchorPosition::Below:
            bubbleRect = boxRect.toFloat().withTrimmedTop(tailSize);
            bubblePath.addRectangle(bubbleRect);
            bubblePath.addTriangle(bubbleRect.getCentreX() - tailSize / 2.0f, bubbleRect.getY(),
                                   bubbleRect.getCentreX() + tailSize / 2.0f, bubbleRect.getY(),
                                   bubbleRect.getCentreX(), bubbleRect.getY() - tailSize);
            break;
        case AnchorPosition::Above:
            bubbleRect = boxRect.toFloat().withTrimmedBottom(tailSize);
            bubblePath.addRectangle(bubbleRect);
            bubblePath.addTriangle(
                bubbleRect.getCentreX() - tailSize / 2.0f, bubbleRect.getBottom(),
                bubbleRect.getCentreX() + tailSize / 2.0f, bubbleRect.getBottom(),
                bubbleRect.getCentreX(), bubbleRect.getBottom() + tailSize);
            break;
        case AnchorPosition::Left:
            bubbleRect = boxRect.toFloat().withTrimmedRight(tailSize);
            bubblePath.addRectangle(bubbleRect);
            bubblePath.addTriangle(bubbleRect.getRight(), bubbleRect.getCentreY() - tailSize / 2.0f,
                                   bubbleRect.getRight(), bubbleRect.getCentreY() + tailSize / 2.0f,
                                   bubbleRect.getRight() + tailSize, bubbleRect.getCentreY());
            break;
        case AnchorPosition::Right:
            bubbleRect = boxRect.toFloat().withTrimmedLeft(tailSize);
            bubblePath.addRectangle(bubbleRect);
            bubblePath.addTriangle(bubbleRect.getX(), bubbleRect.getCentreY() - tailSize / 2.0f,
                                   bubbleRect.getX(), bubbleRect.getCentreY() + tailSize / 2.0f,
                                   bubbleRect.getX() - tailSize, bubbleRect.getCentreY());
            break;
        default:
            bubbleRect = boxRect.toFloat();
            bubblePath.addRectangle(bubbleRect);
            break;
        }

        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillPath(bubblePath);
        g.setColour(juce::Colour(0xFF444444));
        g.strokePath(bubblePath, juce::PathStrokeType(1.f));

        const auto contentRect = getContentRect();

        auto drawContent = [&](juce::Graphics &gRef, const juce::Rectangle<float> &rect) {
            gRef.setColour(cc < 0 ? juce::Colours::red : juce::Colours::white);

            if (std::abs(cc) == noCCSentinel)
            {
                const int dashY = static_cast<int>(rect.getCentreY());
                gRef.drawLine(rect.getX() + 4.0f, static_cast<float>(dashY), rect.getRight() - 4.0f,
                              static_cast<float>(dashY), std::max(1.0f, midiLearnTailSize * 0.18f));
            }
            else
            {
                const float w = rect.getWidth();
                const float h = rect.getHeight();
                const float yCenter = rect.getCentreY();

                const float xBoxSize = h * 0.55f;
                const float gap = w * 0.13f;
                const float sepWidth = w * 0.018f;
                const float fontSize = h * 0.75f;
                const float ccGap = gap * 0.5f;

                const float totalWidth = xBoxSize + gap + sepWidth + ccGap + fontSize * 2.2f;
                const float startX = rect.getX() + (w - totalWidth) * 0.5f;

                // x
                const juce::Rectangle xRect(startX, yCenter - xBoxSize * 0.5f, xBoxSize, xBoxSize);
                const float xStroke = std::max(1.0f, h * 0.12f);
                gRef.drawLine(xRect.getX(), xRect.getY(), xRect.getRight(), xRect.getBottom(),
                              xStroke);
                gRef.drawLine(xRect.getRight(), xRect.getY(), xRect.getX(), xRect.getBottom(),
                              xStroke);

                // sep
                const float sepX = xRect.getRight() + gap + sepWidth * 0.5f;
                gRef.setColour(juce::Colours::white);
                gRef.drawLine(sepX, rect.getY(), sepX, rect.getBottom(), sepWidth);

                // cc
                const juce::Rectangle ccRect(sepX + ccGap, rect.getY(), fontSize * 2.2f, h);
                gRef.setColour(cc < 0 ? juce::Colours::red : juce::Colours::white);
                gRef.setFont(fontSize);
                gRef.drawText(juce::String(std::abs(cc)), ccRect, juce::Justification::centredLeft,
                              false);
            }
        };

        drawContent(g, contentRect);
    }

    void parentHierarchyChanged() override { updatePosition(); }

    void updatePosition()
    {
        if (anchorComp && getParentComponent())
        {
            const auto anchorBounds = anchorComp->getBounds();
            computeScaledSizes(anchorBounds);

            switch (anchorPosition)
            {
            case AnchorPosition::Below:
            {
                const int w = midiLearnRectW;
                const int x = anchorBounds.getX() + (anchorBounds.getWidth() - w) / 2;
                const int y = anchorBounds.getBottom();
                setBounds(x, y, w, midiLearnRectH);
                break;
            }
            case AnchorPosition::Above:
            {
                const int w = midiLearnRectW;
                const int x = anchorBounds.getX() + (anchorBounds.getWidth() - w) / 2;
                const int y = anchorBounds.getY() - midiLearnRectH;
                setBounds(x, y, w, midiLearnRectH);
                break;
            }
            case AnchorPosition::Overlay:
            {
                const int w = midiLearnRectW;
                const int h = midiLearnRectH;
                const int x = anchorBounds.getX() + (anchorBounds.getWidth() - w) / 2;
                const int y = anchorBounds.getY() + (anchorBounds.getHeight() - h) / 2;
                setBounds(x, y, w, h);
                break;
            }
            case AnchorPosition::Left:
            {
                const int w = midiLearnRectW;
                const int h = midiLearnRectH;
                const int x = anchorBounds.getX() - w;
                const int y = anchorBounds.getY() + (anchorBounds.getHeight() - h) / 2;
                setBounds(x, y, w, h);
                break;
            }
            case AnchorPosition::Right:
            {
                const int w = midiLearnRectW;
                const int h = midiLearnRectH;
                const int x = anchorBounds.getRight();
                const int y = anchorBounds.getY() + (anchorBounds.getHeight() - h) / 2;
                setBounds(x, y, w, h);
                break;
            }
            }
        }
        repaint();
    }

    std::function<void(MidiLearnOverlay *)> onSelectionCallback;
    std::function<void(Component *)> onClearCallback;

    Component *anchorComp;

    void setMidiLearnActive(const bool active)
    {
        midiLearnActive = active;
        repaint();
    }
    void setComponentSelected(const bool selected)
    {
        componentSelected = selected;
        repaint();
    }

  private:
    bool midiLearnActive = false;
    bool componentSelected = false;

    std::function<int(Component *)> getCCForComp;
    AnchorPosition anchorPosition;

    static constexpr int logicalDefaultOverlayHeight{28};
    static constexpr int logicalDefaultOverlayWidth{45};
    static constexpr int logicalMinOverlayHeight{12};
    static constexpr int logicalMinOverlayWidth{36};

    int midiLearnRectH{28};
    int midiLearnRectW{45};
    int midiLearnTailSize{4};
    float scaleFactor{1.0f};

    void computeScaledSizes(const juce::Rectangle<int> & /*anchorBounds*/)
    {

        midiLearnRectH = juce::jmax(logicalMinOverlayHeight,
                                    juce::roundToInt((logicalDefaultOverlayHeight)*scaleFactor));

        midiLearnRectW = juce::jmax(logicalMinOverlayWidth,
                                    juce::roundToInt((logicalDefaultOverlayWidth)*scaleFactor));

        midiLearnTailSize = juce::jmax(4, juce::roundToInt(midiLearnRectH * 0.28f));
    }

    juce::Rectangle<float> getContentRect() const
    {
        const auto boxRect = getLocalBounds().toFloat();
        const auto tailSize = midiLearnTailSize;
        const auto horizontalPadding = (midiLearnRectH * 0.12f);
        const auto verticalPadding = midiLearnRectH * 0.08f;

        switch (anchorPosition)
        {
        case AnchorPosition::Below:
            return boxRect.withTrimmedTop(tailSize).reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Above:
            return boxRect.withTrimmedBottom(tailSize).reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Left:
            return boxRect.withTrimmedRight(tailSize).reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Right:
            return boxRect.withTrimmedLeft(tailSize).reduced(horizontalPadding, verticalPadding);
        default:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        }
    }
};

#endif // MIDILEARNOVERLAY_H