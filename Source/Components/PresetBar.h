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

#ifndef OBXF_SRC_COMPONENTS_PRESETBAR_H
#define OBXF_SRC_COMPONENTS_PRESETBAR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class ObxdAudioProcessorEditor;

class CustomLabel final : public juce::Label
{
  public:
    explicit CustomLabel(const juce::String &componentName = juce::String(),
                         const juce::String &labelText = juce::String())
        : Label(componentName, labelText)
    {
    }

    std::function<void(juce::Point<int> pos)> leftClicked;

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (this->getBounds().contains(event.getMouseDownPosition()) &&
            event.mods.isLeftButtonDown())
        {
            if (leftClicked)
                leftClicked(event.getScreenPosition());
        }
    }
};

class PresetBar final : public juce::Component, public juce::Timer, public juce::Button::Listener
{
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

#endif // OBXF_SRC_COMPONENTS_PRESETBAR_H
