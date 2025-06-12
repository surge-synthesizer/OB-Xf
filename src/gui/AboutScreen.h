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

#ifndef OBXF_SRC_GUI_ABOUTSCREEN_H
#define OBXF_SRC_GUI_ABOUTSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>

struct AboutScreen final : juce::Component
{
    void mouseUp(const juce::MouseEvent & /*event*/) override { setVisible(false); }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.4f));

        const auto bxBnd = getLocalBounds().reduced(70).withTrimmedBottom(80);
        g.setColour(juce::Colours::black);
        g.fillRect(bxBnd);
        g.setColour(juce::Colours::white);
        g.drawRect(bxBnd);

        g.setColour(juce::Colour(0xFF, 0x90, 0x00));
        auto txRec = bxBnd.reduced(8, 4);
        g.drawText("OB-Xf", txRec, juce::Justification::centredTop);
        txRec = txRec.withTrimmedTop(33);

        g.setColour(juce::Colour(0xE0, 0xE0, 0xE0));
        g.drawText(std::string("Version : ") + OBXF_VERSION_STR, txRec,
                   juce::Justification::centredTop);
        txRec = txRec.withTrimmedTop(50);

        std::vector<std::string> msg = {
            "A continuation of the last open source version of OB-Xd.",
        };
        for (const auto &m : msg)
        {
            g.drawText(m, txRec, juce::Justification::topLeft);
            txRec = txRec.withTrimmedTop(14);
        }

        txRec = txRec.withTrimmedTop(50);
        g.setColour(juce::Colour(0xE0, 0xE0, 0xE0));
        g.drawText("Click anywhere to close", txRec, juce::Justification::centredTop);
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());
        setVisible(true);
        toFront(true);
    }
};

#endif // OBXF_SRC_GUI_ABOUTSCREEN_H
