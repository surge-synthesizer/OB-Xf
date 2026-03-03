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

#include "LookAndFeel.h"
#include "PluginEditor.h"

namespace obxf
{
juce::PopupMenu::Options LookAndFeel::defaultPopupMenuOptions()
{
    return juce::PopupMenu::Options().withParentComponent(editor);
}

juce::Font LookAndFeel::getPopupMenuFont()
{
    auto f = LookAndFeel_V4::getPopupMenuFont();
    auto fh = f.getHeight();
    return f.withHeight(fh * editor->menuScaleFactor());
}

juce::Font LookAndFeel::getSliderPopupFont(juce::Slider &s)
{
    auto f = LookAndFeel_V4::getSliderPopupFont(s);
    auto fh = f.getHeight() * 1.1;
    return f.withHeight(fh * editor->impliedScaleFactor() / editor->utils.getPluginAPIScale());
}

void LookAndFeel::getIdealPopupMenuItemSizeWithOptions(const juce::String &text, bool isSeparator,
                                                       int standardMenuItemHeight, int &idealWidth,
                                                       int &idealHeight,
                                                       const juce::PopupMenu::Options &o)
{
    juce::LookAndFeel_V4::getIdealPopupMenuItemSizeWithOptions(
        text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight, o);
    idealHeight *= editor->menuScaleFactor();
    idealWidth *= editor->menuScaleFactor();
}

void LookAndFeel::getIdealPopupMenuSectionHeaderSizeWithOptions(const juce::String &text,
                                                                int standardMenuItemHeight,
                                                                int &idealWidth, int &idealHeight,
                                                                const juce::PopupMenu::Options &o)
{
    getIdealPopupMenuItemSizeWithOptions(text, false, standardMenuItemHeight, idealWidth,
                                         idealHeight, o);
    // This is just a heuristic.
    auto ems = editor->menuScaleFactor();
    auto ha = 5;
    if (ems < 1.0f)
    {
        ha += 1;
    }
    else
    {
        auto d = (ems - 1) * 8;
        ha = std::max(0, (int)(ha - d));
    }
    idealHeight += ha;
}

float LookAndFeel::menuScaleFactor() const { return editor->menuScaleFactor(); }

} // namespace obxf