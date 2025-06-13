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

#ifndef OBXF_SRC_COMPONENTS_SETPRESETNAMEWINDOW_H
#define OBXF_SRC_COMPONENTS_SETPRESETNAMEWINDOW_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class SetPresetNameWindow final : public juce::Component,
                                  public juce::Button::Listener,
                                  public juce::TextEditor::Listener
{
  public:
    SetPresetNameWindow();
    ~SetPresetNameWindow() override;

    std::function<void(int, juce::String)> callback;

    void setText(const juce::String &txt) const { nameTextEditor->setText(txt); }

    void grabTextEditorFocus() const;
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void buttonClicked(juce::Button *buttonThatWasClicked) override;

  private:
    std::unique_ptr<juce::TextEditor> nameTextEditor;
    std::unique_ptr<juce::TextButton> cancel;
    std::unique_ptr<juce::TextButton> Ok;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetPresetNameWindow)
};
#endif // OBXF_SRC_COMPONENTS_SETPRESETNAMEWINDOW_H
