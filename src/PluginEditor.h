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

#include "PluginProcessor.h"
#include "gui/Knob.h"
#include "gui/ToggleButton.h"
#include "gui/MultiStateButton.h"
#include "gui/ButtonList.h"
#include "gui/ImageMenu.h"
#include "gui/Display.h"
#include "gui/Label.h"
#include "components/ScalingImageCache.h"
#include "Constants.h"
#include "Utils.h"
#include "KeyCommandHandler.h"

#include "gui/LookAndFeel.h"

#include "parameter/ParameterAttachment.h"

#if defined(DEBUG) || defined(_DEBUG)
#include "melatonin_inspector/melatonin_inspector.h"
#endif

struct AboutScreen;

using KnobAttachment = Attachment<Knob, true, void (*)(Knob &, float), float (*)(const Knob &)>;
using ButtonAttachment = Attachment<ToggleButton, false, void (*)(ToggleButton &, float),
                                    float (*)(const ToggleButton &)>;
using ButtonListAttachment =
    Attachment<ButtonList, false, void (*)(ButtonList &, float), float (*)(const ButtonList &)>;
using MultiStateAttachment =
    Attachment<MultiStateButton, false, void (*)(MultiStateButton &, float),
               float (*)(const MultiStateButton &)>;

class ObxfAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                       public juce::AsyncUpdater,
                                       public juce::ChangeListener,
                                       public juce::Button::Listener,
                                       public juce::ActionListener,
                                       public juce::FileDragAndDropTarget
{
  public:
    static constexpr int defKnobDiameter = 40;

    explicit ObxfAudioProcessorEditor(ObxfAudioProcessor &ownerFilter);

    ~ObxfAudioProcessorEditor() override;

    bool isInterestedInFileDrag(const juce::StringArray &files) override;

    void filesDropped(const juce::StringArray &files, int x, int y) override;

    void scaleFactorChanged();

    void mouseUp(const juce::MouseEvent &e) override;

    void paint(juce::Graphics &g) override;
    void paintMissingAssets(juce::Graphics &g);

    void updateFromHost();

    void handleAsyncUpdate() override;

    juce::String getCurrentProgramName() const
    {
        return processor.getProgramName(processor.getCurrentProgram());
    }

    int getCurrentProgramIndex() const { return processor.getCurrentProgram(); }

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void buttonClicked(juce::Button *) override;

    void nextProgram();

    void prevProgram();

    void MenuActionCallback(int action);

    void resized() override;

    bool isHighResolutionDisplay() const { return utils.getPixelScaleFactor() > 1.0; }

    void actionListenerCallback(const juce::String &message) override;

  private:
    juce::Rectangle<int> transformBounds(int x, int y, int w, int h) const;

    juce::String useAssetOrDefault(const juce::String &assetName,
                                   const juce::String &defaultAssetName) const;

    std::unique_ptr<Label> addLabel(int x, int y, int w, int h, int fh, const juce::String &name,
                                    const juce::String &assetName);

    std::unique_ptr<Knob> addKnob(int x, int y, int w, int h, int d, int fh,
                                  const juce::String &paramId, float defval,
                                  const juce::String &name, const juce::String &assetName);

    std::unique_ptr<ToggleButton> addButton(int x, int y, int w, int h, const juce::String &paramId,
                                            const juce::String &name,
                                            const juce::String &assetName);

    std::unique_ptr<MultiStateButton> addMultiStateButton(int x, int y, int w, int h,
                                                          const juce::String &paramId,
                                                          const juce::String &name,
                                                          const juce::String &assetName,
                                                          const uint8_t numStates = 3);

    std::unique_ptr<ButtonList> addList(int x, int y, int w, int h, const juce::String &paramId,
                                        const juce::String &name, const juce::String &assetName);

    std::unique_ptr<ImageMenu> addMenu(int x, int y, int w, int h, const juce::String &assetName);

    juce::PopupMenu createPatchList(juce::PopupMenu &menu, const int itemIdxStart) const;

    void createMenu();

    void createMidi(int, juce::PopupMenu &);

    void resultFromMenu(juce::Point<int>);

    void clean();

    void rebuildComponents(ObxfAudioProcessor &);

    void updateSelectButtonStates() const;

    void loadTheme(ObxfAudioProcessor &);
    bool loadThemeFilesAndCheck();
    std::map<juce::String, float> saveComponentParameterValues();
    void clearAndResetComponents(ObxfAudioProcessor &ownerFilter);
    bool parseAndCreateComponentsFromTheme();
    void setupMenus();
    void restoreComponentParameterValues(const std::map<juce::String, float> &parameterValues);
    void finalizeThemeLoad(ObxfAudioProcessor &ownerFilter);
    void createComponentsFromXml(const juce::XmlElement *doc);
    void setupPolyphonyMenu() const;
    void setupUnisonVoicesMenu() const;
    void setupEnvLegatoModeMenu() const;
    void setupNotePriorityMenu() const;
    void setupBendUpRangeMenu() const;
    void setupBendDownRangeMenu() const;
    void setupFilterXpanderModeMenu() const;
    void setupPatchNumberMenu() const;

  public:
    void idle();
    // The various caches are a bit off with zoom in constructor
    // so just call resize again first time round the idle loop if
    // this is set. Or remove this and fix that bug I couldn't find.
    bool resizeOnNextIdle = false;

  private:
    std::unique_ptr<juce::Timer> idleTimer;
    std::unique_ptr<juce::XmlElement> cachedThemeXml;

    std::unique_ptr<juce::ComponentBoundsConstrainer> constrainer;
    ObxfAudioProcessor &processor;
    Utils &utils;
    ParameterManagerAdapter &paramAdapter;
    std::unique_ptr<KeyCommandHandler> keyCommandHandler;
#if defined(DEBUG) || defined(_DEBUG)
    std::unique_ptr<melatonin::Inspector> inspector{};
#endif

    bool backgroundIsSVG{false};
    juce::Image backgroundImage;
    std::map<juce::String, Component *> componentMap;
    ScalingImageCache imageCache;
    //==============================================================================

    std::unique_ptr<Label> osc1TriangleLabel, osc1PulseLabel, osc2TriangleLabel, osc2PulseLabel,
        filterModeLabel, filterOptionsLabel, lfo1Wave2Label, lfo2Wave2Label;
    std::unique_ptr<Display> patchNameLabel;
    std::unique_ptr<MultiStateButton> noiseColorButton, lfo1ToOsc1PitchButton,
        lfo1ToOsc2PitchButton, lfo1ToFilterCutoffButton, lfo1ToOsc1PWButton, lfo1ToOsc2PWButton,
        lfo1ToVolumeButton, lfo2ToOsc1PitchButton, lfo2ToOsc2PitchButton, lfo2ToFilterCutoffButton,
        lfo2ToOsc1PWButton, lfo2ToOsc2PWButton, lfo2ToVolumeButton;
    std::unique_ptr<Knob> filterCutoffKnob, filterResonanceKnob, osc1PitchKnob, osc2PitchKnob,
        osc2DetuneKnob, volumeKnob, portamentoKnob, unisonDetuneKnob, filterEnvAmountKnob,
        filterKeyFollowKnob, oscPWKnob, oscCrossmodKnob, filterModeKnob, ampEnvAttackCurveSlider,
        ampEnvAttackKnob, ampEnvDecayKnob, ampEnvSustainKnob, ampEnvReleaseKnob,
        filterEnvAttackCurveSlider, filterEnvAttackKnob, filterEnvDecayKnob, filterEnvSustainKnob,
        filterEnvReleaseKnob, osc1MixKnob, osc2MixKnob, noiseMixKnob, ringModMixKnob,
        filterSlopKnob, envelopeSlopKnob, portamentoSlopKnob, levelSlopKnob, tuneKnob, lfo1RateKnob,
        lfo1ModAmount1Knob, lfo1ModAmount2Knob, lfo1Wave1Knob, lfo1Wave2Knob, lfo1Wave3Knob,
        lfo1PWSlider, lfo2RateKnob, lfo2ModAmount1Knob, lfo2ModAmount2Knob, lfo2Wave1Knob,
        lfo2Wave2Knob, lfo2Wave3Knob, lfo2PWSlider, oscBrightnessKnob, envToPitchAmountKnob,
        vibratoRateKnob, velToAmpEnvSlider, velToFilterEnvSlider, transposeKnob, envToPWAmountKnob,
        osc2PWOffsetKnob;
    std::unique_ptr<ToggleButton> oscSyncButton, osc1SawButton, osc2SawButton, osc1PulseButton,
        osc2PulseButton, unisonButton, envToPitchInvertButton, envToPWInvertButton, hqModeButton,
        filter2PoleBPBlendButton, lfo1TempoSyncButton, lfo2TempoSyncButton, bendOsc2OnlyButton,
        vibratoWaveButton, filter4PoleModeButton, filter4PoleXpanderButton, midiLearnButton,
        midiUnlearnButton, envToPWBothOscsButton, envToPitchBothOscsButton, filterEnvInvertButton,
        filter2PolePushButton, prevPatchButton, nextPatchButton, initPatchButton,
        randomizePatchButton, groupSelectButton, aboutPageButton;
    std::unique_ptr<ButtonList> polyphonyMenu, unisonVoicesMenu, envLegatoModeMenu,
        notePriorityMenu, bendUpRangeMenu, bendDownRangeMenu, filterXpanderModeMenu,
        patchNumberMenu;
    std::unique_ptr<ImageMenu> mainMenu;

    std::array<std::unique_ptr<Knob>, MAX_PANNINGS> panKnobs;
    std::array<std::unique_ptr<ToggleButton>, NUM_PATCHES_PER_GROUP> selectButtons;
    std::array<std::unique_ptr<ToggleButton>, NUM_LFOS> selectLFOButtons;
    std::array<std::unique_ptr<Label>, NUM_PATCHES_PER_GROUP> selectLabels;
    std::array<std::unique_ptr<Label>, MAX_VOICES> voiceLEDs, voiceBGs;
    std::array<std::vector<juce::Component *>, NUM_LFOS> lfoControls;

    Utils::ThemeLocation themeLocation;

    std::vector<std::unique_ptr<KnobAttachment>> knobAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> toggleAttachments;
    std::vector<std::unique_ptr<ButtonListAttachment>> buttonListAttachments;
    std::vector<std::unique_ptr<MultiStateAttachment>> multiStateAttachments;

    std::vector<std::unique_ptr<juce::PopupMenu>> popupMenus;

    std::unique_ptr<AboutScreen> aboutScreen;
    friend struct AboutScreen;

    bool notLoadTheme{false};
    bool skinLoaded{false};
    size_t midiStart{};
    size_t sizeStart{};
    size_t presetStart{};
    size_t bankStart{};
    size_t themeStart{};

    static constexpr int numScaleFactors = 7;
    static constexpr float scaleFactors[] = {.5f, .75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f};

    float impliedScaleFactor() const;

    std::vector<Utils::ThemeLocation> themes;
    std::vector<Utils::BankLocation> banks;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::FontOptions patchNameFont;
    juce::ApplicationCommandManager commandManager;

    int countTimer = 0;
    bool needNotifytoHost = false;

    std::vector<juce::String> midiFiles;
    int countTimerForLed = 0;

    std::shared_ptr<obxf::LookAndFeel> lookAndFeelPtr;

    bool noThemesAvailable = false;
    juce::Rectangle<int> initialBounds;

    int initialWidth = 0;
    int initialHeight = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObxfAudioProcessorEditor)
};

#endif // OBXF_SRC_PLUGINEDITOR_H
