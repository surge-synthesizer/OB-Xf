#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Gui/Knob.h"
#include "Gui/TooglableButton.h"
#include "Gui/ButtonList.h"
#include "Components/SetPresetNameWindow.h"
#include "Components/PresetBar.h"
#include "Components/ScaleComponent.h"
#include "Constants.h"
#include "Utils.h"
#if defined(DEBUG) || defined(_DEBUG)
#include "melatonin_inspector/melatonin_inspector.h"
#endif

class ObxdAudioProcessorEditor final : public juce::AudioProcessorEditor
                                       , public juce::AsyncUpdater
                                       , public juce::ChangeListener
                                       , public juce::Button::Listener
                                       , public juce::ActionListener
                                       , public juce::ApplicationCommandTarget
                                       , public juce::Timer
                                       , public juce::FileDragAndDropTarget
                                       , public ScalableComponent {
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

    juce::String getCurrentProgramName() const {
        return processor.getProgramName(processor.getCurrentProgram());
    }

    void updatePresetBar(bool resize = true);

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void buttonClicked(juce::Button *) override;

    void timerCallback() override {
        countTimer++;
        if (countTimer == 4 && needNotifytoHost) {
            countTimer = 0;
            needNotifytoHost = false;
            processor.updateHostDisplay();
        }

        countTimerForLed++;
        if (midiUnlearnButton && midiUnlearnButton->getToggleState() && countTimerForLed > 3) {
            midiUnlearnButton->setToggleState(false, juce::NotificationType::sendNotification);
            countTimerForLed = 0;
        }
    }

    ApplicationCommandTarget *getNextCommandTarget() override {
        return nullptr;
    };

    void getAllCommands(juce::Array<juce::CommandID> &commands) override {
        const juce::Array<juce::CommandID> ids{
            buttonNextProgram, buttonPrevProgram,
            buttonPadNextProgram, buttonPadPrevProgram
        };

        commands.addArray(ids);
    };

    void getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo &result) override {
        switch (commandID) {
            case buttonNextProgram:
                result.setInfo("Move up", "Move the button + ", "Button", 0);
                result.addDefaultKeypress('+', 0);
                result.setActive(true);
                break;
            case buttonPrevProgram:
                result.setInfo("Move right", "Move the button - ", "Button", 0);
                result.addDefaultKeypress('-', 0);
                result.setActive(true);
                break;
            case buttonPadNextProgram:
                result.setInfo("Move down", "Move the button Pad + ", "Button", 0);
                result.addDefaultKeypress(juce::KeyPress::numberPadAdd, 0);
                result.setActive(true);
                break;
            case buttonPadPrevProgram:
                result.setInfo("Move left", "Move the button Pad -", "Button", 0);
                result.addDefaultKeypress(juce::KeyPress::numberPadSubtract, 0);
                result.setActive(true);
                break;
            default:
                break;
        }
    };

    bool perform(const InvocationInfo &info) override {
        switch (info.commandID) {
            case buttonNextProgram:
            case buttonPadNextProgram:
                nextProgram();
                grabKeyboardFocus();
                break;

            case buttonPrevProgram:
            case buttonPadPrevProgram:
                prevProgram();
                grabKeyboardFocus();
                break;
            default:
                return false;
        }
        return true;
    };

    void nextProgram();

    void prevProgram();

    void MenuActionCallback(int action);

    void resized() override;

    bool isHighResolutionDisplay() const {
        return utils.getPixelScaleFactor() > 1.0;
    }

    void actionListenerCallback(const juce::String &message) override;

private:
    juce::Rectangle<int> transformBounds(int x, int y, int w, int h) const;

    std::unique_ptr<Knob> addKnob(int x, int y, int d, ObxdAudioProcessor &filter, int parameter,
                                  const juce::String &name, float defval);

    void placeLabel(int x, int y, const juce::String &text);

    std::unique_ptr<TooglableButton> addButton(int x, int y, int w, int h, ObxdAudioProcessor &filter, int parameter,
                                               const juce::String &name);

    std::unique_ptr<ButtonList> addList(int x, int y, int w, int h, ObxdAudioProcessor &filter, int parameter,
                                        const juce::String &name, const juce::String &nameImg);

    juce::ImageButton *addMenuButton(int x, int y, int d, const juce::String &imgName);

    void createMenu();

    void createMidi(int, juce::PopupMenu &);

    void resultFromMenu(juce::Point<int>);

    void clean();

    void rebuildComponents(ObxdAudioProcessor &);

    void loadSkin(ObxdAudioProcessor &);

public:
    ObxdAudioProcessor &processor;

private:
    Utils &utils;
#if defined(DEBUG) || defined(_DEBUG)
    melatonin::Inspector inspector{*this};
#endif
    juce::Image backgroundImage;
    std::map<juce::String, Component *> mappingComps;
    //==============================================================================

    std::unique_ptr<Knob> cutoffKnob,
            resonanceKnob,
            osc1PitchKnob,
            osc2PitchKnob,
            osc2DetuneKnob,
            volumeKnob,
            portamentoKnob,
            voiceDetuneKnob,
            filterEnvelopeAmtKnob,
            pulseWidthKnob,
            xmodKnob,
            multimodeKnob,
            attackKnob,
            decayKnob,
            sustainKnob,
            releaseKnob,
            fattackKnob,
            fdecayKnob,
            fsustainKnob,
            freleaseKnob,
            osc1MixKnob,
            osc2MixKnob,
            noiseMixKnob,
            filterDetuneKnob,
            envelopeDetuneKnob,
            portamentoDetuneKnob,
            tuneKnob,
            lfoFrequencyKnob,
            lfoAmt1Knob,
            lfoAmt2Knob,
            pan1Knob,
            pan2Knob,
            pan3Knob,
            pan4Knob,
            pan5Knob,
            pan6Knob,
            pan7Knob,
            pan8Knob,
            brightnessKnob,
            envPitchModKnob,
            bendLfoRateKnob,
            veloAmpEnvKnob,
            veloFltEnvKnob,
            transposeKnob;

    std::unique_ptr<TooglableButton> hardSyncButton,
            osc1SawButton,
            osc2SawButton,
            osc1PulButton,
            osc2PulButton,
            filterKeyFollowButton,
            unisonButton,
            pitchQuantButton,
            filterHQButton,
            filterBPBlendButton,
            lfoSinButton,
            lfoSquareButton,
            lfoSHButton,
            lfoOsc1Button,
            lfoOsc2Button,
            lfoFilterButton,
            lfoPwm1Button,
            lfoPwm2Button,
            bendRangeButton,
            bendOsc2OnlyButton,
            fourPoleButton,
            asPlayedAllocButton,
            midiLearnButton,
            midiUnlearnButton;

    std::unique_ptr<ButtonList> voiceSwitch, legatoSwitch;

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

    struct Action {
        static const juce::String panReset;
    };
};
