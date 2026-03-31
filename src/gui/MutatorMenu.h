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

#ifndef OBXF_SRC_GUI_MUTATORMENU_H
#define OBXF_SRC_GUI_MUTATORMENU_H

#include "LookAndFeel.h"
#include "configuration.h"

class MutatorMenu : public juce::PopupMenu::CustomComponent
{
  public:
    struct MutatorLookAndFeel : public juce::LookAndFeel_V4
    {
        MutatorLookAndFeel()
        {
            using namespace juce;

            setColour(PopupMenu::highlightedBackgroundColourId, Colour(64, 64, 64));
        }

        int getPopupMenuBorderSize() override { return 1; }
    };

    static constexpr int mutateMenuID = 1234;
    static constexpr int mutateMenuWidth = 300;
    static constexpr int mutateMenuHeight = 89;

    using maskChangedCb = std::function<void(int index, bool value)>;
    using mutateCb = std::function<void(const MutatorMenu &)>;

    MutatorMenu(MutateMask initialMask, maskChangedCb maskCallback, mutateCb mutateCallback)
        : CustomComponent(false), onMaskChanged(std::move(maskCallback)),
          onMutate(std::move(mutateCallback))
    {
        auto setupToggle = [this](juce::ToggleButton &b, const juce::String &text,
                                  const bool initValue, const int index) {
            b.setButtonText(text);
            b.setTriggeredOnMouseDown(true);
            b.setToggleState(initValue, juce::dontSendNotification);

            b.onClick = [this, &b, index] {
                if (onMaskChanged)
                    onMaskChanged(index, b.getToggleState());
                repaint();
            };

            addAndMakeVisible(b);
        };

        setupToggle(oscButton, "Oscillators", initialMask[0], 0);
        setupToggle(mixerButton, "Mixer", initialMask[1], 1);
        setupToggle(filterButton, "Filter", initialMask[2], 2);
        setupToggle(lfoButton, "LFOs", initialMask[3], 3);
        setupToggle(envButton, "Envelopes", initialMask[4], 4);
        setupToggle(voiceButton, "Voice", initialMask[5], 5);

        setSize(mutateMenuWidth, mutateMenuHeight);
    }

    bool anySectionActive()
    {
        return oscButton.getToggleState() || mixerButton.getToggleState() ||
               filterButton.getToggleState() || lfoButton.getToggleState() ||
               envButton.getToggleState() || voiceButton.getToggleState();
    }

    void parentHierarchyChanged() override
    {
        if (auto *menuWindow = getParentComponent())
        {
            menuWindow->setLookAndFeel(&layoutHandler);
            setBounds(menuWindow->getLocalBounds());
        }
    }

    void getIdealSize(int &width, int &height) override
    {
        width = mutateMenuWidth;
        height = mutateMenuHeight;
    }

    void paint(juce::Graphics &g) override
    {
        const auto area = getLocalBounds().removeFromBottom(22);

        // separator
        g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.3f));
        g.fillRect(area.getX() + 4, area.getY() - 6, area.getWidth() - 8, 1);

        getLookAndFeel().drawPopupMenuItem(g, mutateClickArea, false, anySectionActive(),
                                           isMouseOverMutate, false, false, "Mutate!", "", nullptr,
                                           nullptr);
    }

    void mouseDown(const juce::MouseEvent &e) override
    {
        if (isMouseOverMutate && anySectionActive())
        {
            // Left click = close the menu after mutating
            if (e.mods.isLeftButtonDown())
            {
                if (onMutate)
                    onMutate(*this);

                if (auto *modal = juce::Component::getCurrentlyModalComponent())
                {
                    modal->exitModalState(mutateMenuID);
                }
            }
            // Right click = keep the menu open after mutating
            else if (e.mods.isRightButtonDown())
            {
                if (onMutate)
                    onMutate(*this);

                isMutateRightClicked = true;
                layoutHandler.setColour(juce::PopupMenu::highlightedTextColourId,
                                        juce::Colours::red);
                repaint();
            }
        }
    }

    void mouseUp(const juce::MouseEvent &e) override
    {
        if (isMouseOverMutate)
        {
            if (isMutateRightClicked)
            {
                layoutHandler.setColour(juce::PopupMenu::highlightedTextColourId,
                                        juce::Colours::white);
                isMutateRightClicked = false;
                repaint();
            }
        }
    }

    void mouseMove(const juce::MouseEvent &e) override
    {
        const bool nowOver = mutateClickArea.contains(e.getPosition());

        if (nowOver != isMouseOverMutate)
        {
            isMouseOverMutate = nowOver;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent &e) override
    {
        isMouseOverMutate = false;
        repaint();
    }

    void resized() override
    {
        auto area = getLocalBounds();

        mutateClickArea = area.removeFromBottom(22).expanded(2, 0);

        area.removeFromTop(2);
        area.removeFromLeft(12);
        area.removeFromRight(8);

        const auto rowHeight = 26;
        auto gridArea = area.removeFromTop(rowHeight * 2);
        auto row1 = gridArea.removeFromTop(rowHeight);
        auto row2 = gridArea.removeFromTop(rowHeight);

        const auto colWidth = getWidth() / 3;

        oscButton.setBounds(row1.removeFromLeft(colWidth));
        mixerButton.setBounds(row1.removeFromLeft(colWidth));
        filterButton.setBounds(row1);

        lfoButton.setBounds(row2.removeFromLeft(colWidth));
        envButton.setBounds(row2.removeFromLeft(colWidth));
        voiceButton.setBounds(row2);
    }

  private:
    maskChangedCb onMaskChanged;
    mutateCb onMutate;

    juce::ToggleButton oscButton, mixerButton, filterButton, lfoButton, envButton, voiceButton;

    bool isMouseOverMutate = false;
    bool isMutateRightClicked = false;

    juce::Rectangle<int> mutateClickArea;
    MutatorLookAndFeel layoutHandler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MutatorMenu)
};

#endif // OBXF_SRC_GUI_MUTATORMENU_H