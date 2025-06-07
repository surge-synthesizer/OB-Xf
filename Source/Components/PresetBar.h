#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class ObxdAudioProcessorEditor;

class CustomLabel final : public juce::Label {
public:
    explicit CustomLabel(const juce::String &componentName = juce::String(),
                         const juce::String &labelText = juce::String())
        : Label(componentName, labelText) {
    }

    std::function<void(juce::Point<int> pos)> leftClicked;

    void mouseDown(const juce::MouseEvent &event) override {
        if (this->getBounds().contains(event.getMouseDownPosition()) && event.mods.isLeftButtonDown()) {
            if (leftClicked)
                leftClicked(event.getScreenPosition());
        }
    }
};

class PresetBar final : public juce::Component,
                        public juce::Timer,
                        public juce::Button::Listener {
public:
    explicit PresetBar(ObxdAudioProcessorEditor &gui);

    ~PresetBar() override;

    void timerCallback() override;

    void update() const;

    std::function<void(juce::Point<int> &pos)> leftClicked;

    void paint(juce::Graphics &g) override;

    void resized() override;

    void buttonClicked(juce::Button *buttonThatWasClicked) override;

private:
    ObxdAudioProcessorEditor &editor;

    std::unique_ptr<CustomLabel> presetNameLb;
    std::unique_ptr<juce::ImageButton> previousBtn;
    std::unique_ptr<juce::ImageButton> nextBtn;
    std::unique_ptr<juce::Drawable> drawable1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBar)
};
