
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

        g.setColour(juce::Colour(0xFF767C89));
        g.fillRect(boxRect);
        g.setColour(juce::Colour(0xFF191919));
        g.drawRect(boxRect, 2.f);

        const auto contentRect = getContentRect();

        auto drawContent = [&](juce::Graphics &gRef, const juce::Rectangle<float> &rect) {
            gRef.setColour(cc < 0 ? juce::Colours::red : juce::Colours::white);

            if (std::abs(cc) == noCCSentinel)
            {
                const auto dashY = rect.getCentreY();

                gRef.drawLine(rect.getX() + 12.0f, dashY, rect.getRight() - 12.0f, dashY, 2.0f);
            }
            else
            {
                const float w = rect.getWidth();
                const float h = rect.getHeight();
                const float yCenter = rect.getCentreY();

                const float xBoxSize = h * 0.55f;
                const float gap = w * 0.13f;
                const float sepWidth = 2.f;
                const float fontSize = h * 0.75f;
                const float ccGap = gap * 0.5f;

                const float totalWidth = xBoxSize + gap + sepWidth + ccGap + fontSize * 2.2f;
                const float startX = rect.getX() + (w - totalWidth) * 0.5f;

                // x
                const juce::Rectangle xRect(startX, yCenter - xBoxSize * 0.5f, xBoxSize, xBoxSize);
                gRef.drawLine(xRect.getX(), xRect.getY(), xRect.getRight(), xRect.getBottom(), 2.f);
                gRef.drawLine(xRect.getRight(), xRect.getY(), xRect.getX(), xRect.getBottom(), 2.f);

                // sep
                const float sepX = xRect.getRight() + gap + sepWidth * 0.5f;
                gRef.setColour(juce::Colour(0xFF191919));
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

  private:
    std::function<int(Component *)> getCCForComp;
    AnchorPosition anchorPosition;

    static constexpr int logicalDefaultOverlayHeight{18};
    static constexpr int logicalDefaultOverlayWidth{36};
    static constexpr int logicalMinOverlayHeight{9};
    static constexpr int logicalMinOverlayWidth{18};

    int midiLearnRectH{18};
    int midiLearnRectW{36};
    float scaleFactor{1.0f};

    void computeScaledSizes(const juce::Rectangle<int> & /*anchorBounds*/)
    {

        midiLearnRectH = juce::jmax(logicalMinOverlayHeight,
                                    juce::roundToInt((logicalDefaultOverlayHeight)*scaleFactor));

        midiLearnRectW = juce::jmax(logicalMinOverlayWidth,
                                    juce::roundToInt((logicalDefaultOverlayWidth)*scaleFactor));
    }

    juce::Rectangle<float> getContentRect() const
    {
        const auto boxRect = getLocalBounds().toFloat();
        const auto horizontalPadding = (midiLearnRectW * 0.08f);
        const auto verticalPadding = midiLearnRectH * 0.08f;

        switch (anchorPosition)
        {
        case AnchorPosition::Below:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Above:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Left:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Right:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        default:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        }
    }
};

#endif // MIDILEARNOVERLAY_H