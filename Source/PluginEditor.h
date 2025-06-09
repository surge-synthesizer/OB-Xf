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

#ifndef OBXF_SRC_PLUGINEDITOR_H
#define OBXF_SRC_PLUGINEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Gui/Knob.h"
#include "Gui/TooglableButton.h"
#include "Gui/ButtonList.h"
#include "Components/SetPresetNameWindow.h"
#include "Components/PresetBar.h"
#include "Components/ScaleComponent.h"
#include "Constants.h"
#include "Utils.h"
#include "Contrainer.h"
#include "KeyCommandHandler.h"

#include "Gui/MidiKeyboard.h"

#if defined(DEBUG) || defined(_DEBUG)
#include "melatonin_inspector/melatonin_inspector.h"
#endif

class ObxdAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                       public juce::AsyncUpdater,
                                       public juce::ChangeListener,
                                       public juce::Button::Listener,
                                       public juce::ActionListener,
                                       public juce::Timer,
                                       public juce::FileDragAndDropTarget,
                                       public ScalableComponent
{
  public:
    explicit ObxdAudioProcessorEditor(ObxdAudioProcessor &ownerFilter);

    ~ObxdAudioProcessorEditor() override;

    bool isInterestedInFileDrag(const juce::StringArray &files) override;

    void filesDropped(const juce::StringArray &files, int x, int y) override;

    void scaleFactorChanged() override;

    void mouseUp(const juce::MouseEvent &e) override;

    void paint(juce::Graphics &g) override;

    void updateFromHost();

    void handleAsyncUpdate() override;

    juce::String getCurrentProgramName() const
    {
        return processor.getProgramName(processor.getCurrentProgram());
    }

    void updatePresetBar(bool resize = true);

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void buttonClicked(juce::Button *) override;

    void timerCallback() override
    {
        countTimer++;
        if (countTimer == 4 && needNotifytoHost)
        {
            countTimer = 0;
            needNotifytoHost = false;
            processor.updateHostDisplay();
        }

        countTimerForLed++;
        if (midiUnlearnButton && midiUnlearnButton->getToggleState() && countTimerForLed > 3)
        {
            midiUnlearnButton->setToggleState(false, juce::NotificationType::sendNotification);
            countTimerForLed = 0;
        }
    }

    void nextProgram();

    void prevProgram();

    void MenuActionCallback(int action);

    void resized() override;

    bool isHighResolutionDisplay() const { return utils.getPixelScaleFactor() > 1.0; }

    void actionListenerCallback(const juce::String &message) override;

  private:
    juce::Rectangle<int> transformBounds(int x, int y, int w, int h) const;

    std::unique_ptr<Knob> addKnob(int x, int y, int d, ObxdAudioProcessor &filter, int parameter,
                                  const juce::String &name, float defval);

    void placeLabel(int x, int y, const juce::String &text);

    std::unique_ptr<TooglableButton> addButton(int x, int y, int w, int h,
                                               ObxdAudioProcessor &filter, int parameter,
                                               const juce::String &name);

    std::unique_ptr<ButtonList> addList(int x, int y, int w, int h, ObxdAudioProcessor &filter,
                                        int parameter, const juce::String &name,
                                        const juce::String &nameImg);

    juce::ImageButton *addMenuButton(int x, int y, int d, const juce::String &imgName);

    MidiKeyboard *addMidiKeyboard(int x, int y, int w, int h);

    void createMenu();

    void createMidi(int, juce::PopupMenu &);

    void resultFromMenu(juce::Point<int>);

    void clean();

    void rebuildComponents(ObxdAudioProcessor &);

    void loadSkin(ObxdAudioProcessor &);

    std::unique_ptr<AspectRatioDownscaleConstrainer> constrainer;
    ObxdAudioProcessor &processor;
    Utils &utils;
    ParameterManagerAdaptor &paramManager;
    std::unique_ptr<KeyCommandHandler> keyCommandHandler;
#if defined(DEBUG) || defined(_DEBUG)
    melatonin::Inspector inspector{*this};
#endif
    juce::Image backgroundImage;
    std::map<juce::String, Component *> mappingComps;
    //==============================================================================

    std::unique_ptr<Knob> cutoffKnob, resonanceKnob, osc1PitchKnob, osc2PitchKnob, osc2DetuneKnob,
        volumeKnob, portamentoKnob, voiceDetuneKnob, filterEnvelopeAmtKnob, filterKeyFollowKnob,
        pulseWidthKnob, xmodKnob, multimodeKnob, attackKnob, decayKnob, sustainKnob, releaseKnob,
        fattackKnob, fdecayKnob, fsustainKnob, freleaseKnob, osc1MixKnob, osc2MixKnob, noiseMixKnob,
        filterDetuneKnob, envelopeDetuneKnob, portamentoDetuneKnob, tuneKnob, lfoFrequencyKnob,
        lfoAmt1Knob, lfoAmt2Knob, pan1Knob, pan2Knob, pan3Knob, pan4Knob, pan5Knob, pan6Knob,
        pan7Knob, pan8Knob, brightnessKnob, envPitchModKnob, bendLfoRateKnob, veloAmpEnvKnob,
        veloFltEnvKnob, transposeKnob, pwEnvKnob, pwOffsetKnob;

    std::unique_ptr<TooglableButton> hardSyncButton, osc1SawButton, osc2SawButton, osc1PulButton,
        osc2PulButton, unisonButton, pitchQuantButton, filterHQButton, filterBPBlendButton,
        lfoSinButton, lfoSquareButton, lfoSHButton, lfoOsc1Button, lfoOsc2Button, lfoFilterButton,
        lfoPwm1Button, lfoPwm2Button, bendRangeButton, bendOsc2OnlyButton, fourPoleButton,
        asPlayedAllocButton, midiLearnButton, midiUnlearnButton, lfoSyncButton, pwEnvBothButton,
        envPitchBothButton, fenvInvertButton, selfOscPushButton;

    std::unique_ptr<ButtonList> voiceSwitch, legatoSwitch;

    std::unique_ptr<MidiKeyboard> midiKeyboard;
    juce::MidiKeyboardState keyboardState;

    juce::File skinFolder;

    //==============================================================================
    juce::OwnedArray<Knob::KnobAttachment> knobAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachments;
    juce::OwnedArray<ButtonList::ButtonListAttachment> buttonListAttachments;

    juce::OwnedArray<juce::ImageButton> imageButtons;

    juce::OwnedArray<juce::PopupMenu> popupMenus;

    bool notLoadSkin = false;
    int progStart{};
    int bankStart{};
    int skinStart{};

    juce::Array<juce::File> skins;
    juce::Array<juce::File> banks;
    std::unique_ptr<SetPresetNameWindow> setPresetNameWindow;
    std::unique_ptr<PresetBar> presetBar;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::ApplicationCommandManager commandManager;
    int countTimer = 0;
    bool needNotifytoHost = false;

    juce::Array<juce::String> midiFiles;
    int menuMidiNum{};
    int menuScaleNum{};
    int countTimerForLed = 0;

    struct Action
    {
        static const juce::String panReset;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxdAudioProcessorEditor)
};

#endif // OBXF_SRC_PLUGINEDITOR_H
