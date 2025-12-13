
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
    juce::FontOptions midiLearnPopupFont;

    MidiLearnOverlay(Component *anchor, std::function<int(Component *)> getCC,
                     const AnchorPosition position = AnchorPosition::Overlay)
        : anchorComp(anchor), getCCForComp(std::move(getCC)), anchorPosition(position)
    {
    }

    void setPopupFont(const juce::FontOptions f) { midiLearnPopupFont = f; };

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
            // hack for now, but works just fine really
            if (event.position.x < midiLearnRectH)
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
        const auto strokeW = 1.f * scaleFactor;

        g.setColour(juce::Colour(0xFF767C89));
        g.fillRect(boxRect);
        g.setColour(juce::Colour(0xFF191919));
        g.drawRect(boxRect, strokeW);

        auto drawContent = [&](juce::Graphics &gRef, const juce::Rectangle<float> &rect) {
            gRef.setColour(cc < 0 ? juce::Colours::red : juce::Colours::white);

            if (std::abs(cc) == noCCSentinel)
            {
                const auto dashY = rect.getCentreY();

                gRef.drawLine(rect.getX() + 12.0f, dashY, rect.getRight() - 12.0f, dashY, 2.0f);
            }
            else
            {
                const float h = rect.getHeight();
                const float xBoxSize = h;

                gRef.setFont(midiLearnPopupFont.withHeight(11.f * scaleFactor));

                // close button
                const juce::Rectangle xRect(rect.getX() + strokeW, rect.getY(), xBoxSize - strokeW,
                                            xBoxSize);
                gRef.setColour(cc < 0 ? juce::Colours::red : juce::Colours::white);
                gRef.drawText("X", xRect, juce::Justification::centred, false);

                // separator
                const float sepX = xRect.getRight();
                gRef.setColour(juce::Colour(0xFF191919));
                gRef.drawLine(sepX, rect.getY(), sepX, rect.getBottom(), strokeW);

                // CC number
                const juce::Rectangle ccRect(sepX + strokeW * 0.5f, rect.getY(),
                                             rect.getRight() - xBoxSize - strokeW * 1.5f, h);
                gRef.setColour(cc < 0 ? juce::Colours::red : juce::Colours::white);
                gRef.drawText(juce::String(std::abs(cc)), ccRect, juce::Justification::centred,
                              false);
            }
        };

        drawContent(g, boxRect.toFloat());
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

    static constexpr int logicalDefaultOverlayHeight{14};
    static constexpr int logicalDefaultOverlayWidth{38};
    static constexpr int logicalMinOverlayHeight{7};
    static constexpr int logicalMinOverlayWidth{19};

    int midiLearnRectH{14};
    int midiLearnRectW{38};
    float scaleFactor{1.0f};

    void computeScaledSizes(const juce::Rectangle<int> & /*anchorBounds*/)
    {
        midiLearnRectH = juce::jmax(logicalMinOverlayHeight,
                                    juce::roundToInt((logicalDefaultOverlayHeight)*scaleFactor));

        midiLearnRectW = juce::jmax(logicalMinOverlayWidth,
                                    juce::roundToInt((logicalDefaultOverlayWidth)*scaleFactor));
    }
};

#endif // MIDILEARNOVERLAY_H