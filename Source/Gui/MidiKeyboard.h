#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Components/ScaleComponent.h"


class ObxdAudioProcessor;

class MidiKeyboard final : public juce::MidiKeyboardComponent,
                           public ScalableComponent {
public:
    MidiKeyboard(juce::MidiKeyboardState &state,
                 const Orientation orientation,
                 ObxdAudioProcessor *owner
    )
        : MidiKeyboardComponent(state, orientation),
          ScalableComponent(owner) {
        setOpaque(false);
        Component::setVisible(true);

        setColour(whiteNoteColourId, juce::Colour(0xFFEEEEEE));
        setColour(blackNoteColourId, juce::Colour(0xFF333333));
        setColour(keySeparatorLineColourId, juce::Colour(0x66000000));
        setColour(mouseOverKeyOverlayColourId, juce::Colour(0x66FFFF00));
        setColour(keyDownOverlayColourId, juce::Colour(0xFF00AAFF));
        setColour(textLabelColourId, juce::Colour(0xFF000000));
    }

    void resized() override {
        constexpr int octaves = 6;
        constexpr float whiteKeysPerOctave = 7.0f;
        const float keyWidth = getWidth() / (whiteKeysPerOctave * octaves);

        setKeyWidth(keyWidth);
        setAvailableRange(24, 127);
        MidiKeyboardComponent::resized();
    }

private:
    class CustomKeyboardLookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        CustomKeyboardLookAndFeel() = default;
    };

    CustomKeyboardLookAndFeel lookAndFeel;
};
