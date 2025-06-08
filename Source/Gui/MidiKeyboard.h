/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_GUI_MIDIKEYBOARD_H
#define OBXF_SRC_GUI_MIDIKEYBOARD_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Components/ScaleComponent.h"

class ObxdAudioProcessor;

class MidiKeyboard final : public juce::MidiKeyboardComponent, public ScalableComponent
{
  public:
    MidiKeyboard(juce::MidiKeyboardState &state, const Orientation orientation,
                 ObxdAudioProcessor *owner)
        : MidiKeyboardComponent(state, orientation), ScalableComponent(owner)
    {
        setOpaque(false);
        Component::setVisible(true);

        setColour(whiteNoteColourId, juce::Colour(0xFFEEEEEE));
        setColour(blackNoteColourId, juce::Colour(0xFF333333));
        setColour(keySeparatorLineColourId, juce::Colour(0x66000000));
        setColour(mouseOverKeyOverlayColourId, juce::Colour(0x66FFFF00));
        setColour(keyDownOverlayColourId, juce::Colour(0xFF00AAFF));
        setColour(textLabelColourId, juce::Colour(0xFF000000));
    }

    void resized() override
    {
        constexpr int octaves = 6;
        constexpr float whiteKeysPerOctave = 7.0f;
        const float keyWidth = static_cast<float>(getWidth()) / (whiteKeysPerOctave * octaves);

        setKeyWidth(keyWidth);
        setAvailableRange(24, 127);
        MidiKeyboardComponent::resized();
    }

  private:
    class CustomKeyboardLookAndFeel final : public juce::LookAndFeel_V4
    {
      public:
        CustomKeyboardLookAndFeel() = default;
    };

    CustomKeyboardLookAndFeel lookAndFeel;
};

#endif // OBXF_SRC_GUI_MIDIKEYBOARD_H
