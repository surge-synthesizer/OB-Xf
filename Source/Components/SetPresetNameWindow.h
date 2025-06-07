#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class SetPresetNameWindow final : public juce::Component,
                                  public juce::Button::Listener
{
public:
    SetPresetNameWindow();
    ~SetPresetNameWindow() override;

    std::function<void(int, juce::String)> callback;

    void setText(const juce::String& txt) const {
        nameTextEditor->setText(txt);
    }

    void grabTextEditorFocus() const;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* buttonThatWasClicked) override;

private:
    std::unique_ptr<juce::TextEditor> nameTextEditor;
    std::unique_ptr<juce::TextButton> cancel;
    std::unique_ptr<juce::TextButton> Ok;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetPresetNameWindow)
};