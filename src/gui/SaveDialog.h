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

#ifndef OB_XF_SRC_GUI_SAVEDIALOG_H
#define OB_XF_SRC_GUI_SAVEDIALOG_H


#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginEditor.h"

struct SaveDialog : juce::Component
{
    ObxfAudioProcessorEditor &editor;
    SaveDialog(ObxfAudioProcessorEditor &editor) : editor(editor) {}


    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.85f));
        auto sc = editor.impliedScaleFactor();
        auto bx = juce::Rectangle<int>(0,0,500 * sc,400 * sc).withCentre(getLocalBounds().getCentre());
        g.setColour(juce::Colour(20, 20, 60));
        g.fillRect(bx);
        g.setColour(juce::Colours::white);
        g.drawRect(bx);
    }

    void showOver(const Component *that)
    {
        setBounds(that->getBounds());
        setVisible(true);
        toFront(true);
    }
};

#endif // OB_XF_SAVEDIALOG_H
