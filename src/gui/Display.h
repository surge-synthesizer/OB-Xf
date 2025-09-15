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

#ifndef OBXF_SRC_GUI_DISPLAY_H
#define OBXF_SRC_GUI_DISPLAY_H

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class Display final : public juce::Label
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

    void editorShown(juce::TextEditor *editor) override
    {
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
            return;

        auto sf = getScale();
        juce::Graphics::ScopedSaveState ss(g);
        g.addTransform(juce::AffineTransform().scaled(sf));
        auto textArea = getLocalBounds().transformedBy(juce::AffineTransform().scaled(1.0 / sf));

        auto font(getLookAndFeel().getLabelFont(*this));

        auto w = juce::GlyphArrangement::getStringWidth(font, getText());
        auto r = std::min(1.f * getWidth() / w, 1.5f);
        if (r >= 0.9f)
        {
            font = font.withHeight(font.getHeight() * r);

            g.setColour(findColour(Label::textColourId));
            g.setFont(font);

            // In the near future this can be changd to address #266
            g.drawText(getText(), textArea, juce::Justification::centred);
        }
        else if (r > 0.45f)
        {
            g.setColour(findColour(Label::textColourId));
            font = font.withHeight(font.getHeight() * 0.9);

            g.setFont(font);

            g.drawMultiLineText(getText(), 0, font.getHeight() - font.getDescent() - 2, getWidth(),
                                juce::Justification::centred, -2.f);
        }
        else
        {
            g.setColour(findColour(Label::textColourId));
            font = font.withHeight(font.getHeight() * 0.55);
            g.setFont(font);
            g.drawMultiLineText(getText(), 0, font.getHeight() - font.getDescent() - 2, getWidth(),
                                juce::Justification::centred, -2.f);
        }
    }

  private:
    std::function<float()> getScale = []() { return 1.0; };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Display)
};

#endif // OBXF_SRC_GUI_DISPLAY_H