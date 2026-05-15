/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils.h"

#include "components/ScalingImageCache.h"

#include "gui/AboutScreen.h"
#include "gui/MPEMatrix.h"
#include "gui/SaveDialog.h"
#include "gui/MutatorMenu.h"
#include "gui/FocusDebugger.h"
#include "gui/FocusOrder.h"

#include "sst/plugininfra/misc_platform.h"

struct IdleTimer : juce::Timer
{
    ObxfAudioProcessorEditor *editor{nullptr};
    IdleTimer(ObxfAudioProcessorEditor *e) : editor(e) {}
    void timerCallback() override
    {
        if (editor)
            editor->idle();
    }
};

const std::unordered_map<std::string, ObxfAudioProcessorEditor::KnobDescriptor> &
ObxfAudioProcessorEditor::knobRegistry()
{
    using namespace SynthParam;

    static const KnobPostCreate pitchSnapCallbacks = [](Knob *k) {
        k->cmdDragCallback = [](const double value) {
            const auto semitoneValue = static_cast<int>(juce::jmap(value, -24.0, 24.0));
            return juce::jmap(static_cast<double>(semitoneValue), -24.0, 24.0, 0.0, 1.0);
        };
        k->altDragCallback = [](const double value) {
            const int values[13]{-24, -19, -17, -12, -7, -5, 0, 5, 7, 12, 17, 19, 24};
            const auto snapValue = static_cast<int>(juce::jmap(value, 0.0, 12.0));
            return juce::jmap(static_cast<double>(values[snapValue]), -24.0, 24.0, 0.0, 1.0);
        };
    };

    // clang-format off
    static const std::unordered_map<std::string, KnobDescriptor> reg = {
        // ---- FILTER ----
        {"filterCutoffKnob",          {ID::FilterCutoff,          Name::FilterCutoff,           1.f,  "knob"}},
        {"filterResonanceKnob",       {ID::FilterResonance,       Name::FilterResonance,        0.f,  "knob"}},
        {"filterEnvAmountKnob",       {ID::FilterEnvAmount,       Name::FilterEnvAmount,        0.f,  "knob"}},
        {"filterModeKnob",            {ID::FilterMode,            Name::FilterMode,             0.f,  "knob"}},
        {"filterKeyTrackKnob",        {ID::FilterKeyTrack,        Name::FilterKeyTrack,         0.f,  "knob"}},

        // ---- FILTER ENVELOPE ----
        {"filterEnvAttackKnob",       {ID::FilterEnvAttack,       Name::FilterEnvAttack,        0.f,  "knob"}},
        {"filterEnvDecayKnob",        {ID::FilterEnvDecay,        Name::FilterEnvDecay,         0.f,  "knob"}},
        {"filterEnvSustainKnob",      {ID::FilterEnvSustain,      Name::FilterEnvSustain,       1.f,  "knob"}},
        {"filterEnvReleaseKnob",      {ID::FilterEnvRelease,      Name::FilterEnvRelease,       0.f,  "knob"}},
        {"filterEnvAttackCurveSlider",{ID::FilterEnvAttackCurve,  Name::FilterEnvAttackCurve,   0.f,  "slider-h"}},
        {"velToFilterEnvSlider",      {ID::VelToFilterEnv,        Name::VelToFilterEnv,         0.f,  "slider-h"}},

        // ---- AMP ENVELOPE ----
        {"ampEnvAttackKnob",         {ID::AmpEnvAttack,          Name::AmpEnvAttack,           0.f,  "knob"}},
        {"ampEnvDecayKnob",          {ID::AmpEnvDecay,           Name::AmpEnvDecay,            0.f,  "knob"}},
        {"ampEnvSustainKnob",        {ID::AmpEnvSustain,         Name::AmpEnvSustain,          1.f,  "knob"}},
        {"ampEnvReleaseKnob",        {ID::AmpEnvRelease,         Name::AmpEnvRelease,          0.f,  "knob"}},
        {"ampEnvAttackCurveSlider",  {ID::AmpEnvAttackCurve,     Name::AmpEnvAttackCurve,      0.f,  "slider-h"}},
        {"velToAmpEnvSlider",        {ID::VelToAmpEnv,           Name::VelToAmpEnv,            0.f,  "slider-h"}},

        // ---- OSCILLATORS ----
        {"osc1PitchKnob",            {ID::Osc1Pitch,             Name::Osc1Pitch,             0.5f,  "knob", pitchSnapCallbacks}},
        {"osc2PitchKnob",            {ID::Osc2Pitch,             Name::Osc2Pitch,             0.5f,  "knob", pitchSnapCallbacks}},
        {"osc2DetuneKnob",           {ID::Osc2Detune,            Name::Osc2Detune,             0.f,  "knob"}},
        {"oscPWKnob",                {ID::OscPW,                 Name::OscPW,                  0.f,  "knob"}},
        {"osc2PWOffsetKnob",         {ID::Osc2PWOffset,          Name::Osc2PWOffset,           0.f,  "knob"}},
        {"oscCrossmodKnob",          {ID::OscCrossmod,           Name::OscCrossmod,            0.f,  "knob"}},
        {"oscBrightnessKnob",        {ID::OscBrightness,         Name::OscBrightness,          1.f,  "knob"}},

        {"envToPitchAmountKnob",     {ID::EnvToPitchAmount, Name::EnvToPitchAmount, 0.f, "knob",
            [](Knob *k) {
                k->cmdDragCallback = [](const double value) {
                    const auto semitoneValue = static_cast<int>(juce::jmap(value, 0.0, 36.0));
                    return juce::jmap(static_cast<double>(semitoneValue), 0.0, 36.0, 0.0, 1.0);
                };
                k->altDragCallback = [](const double value) {
                    const int values[10]{0, 5, 7, 12, 17, 19, 24, 29, 31, 36};
                    const auto snapValue = static_cast<int>(juce::jmap(value, 0.0, 9.0));
                    return juce::jmap(static_cast<double>(values[snapValue]), 0.0, 36.0, 0.0, 1.0);
                };
            }}},

        {"envToPWAmountKnob",        {ID::EnvToPWAmount,         Name::EnvToPWAmount,          0.f,  "knob"}},

        // ---- MIXER ----
        {"osc1VolKnob",              {ID::Osc1Vol,               Name::Osc1Vol,                1.f,  "knob"}},
        {"osc2VolKnob",              {ID::Osc2Vol,               Name::Osc2Vol,                1.f,  "knob"}},
        {"ringModVolKnob",           {ID::RingModVol,            Name::RingModVol,             0.f,  "knob"}},
        {"noiseVolKnob",             {ID::NoiseVol,              Name::NoiseVol,               0.f,  "knob"}},

        // ---- MASTER ----
        {"volumeKnob",               {ID::Volume,                Name::Volume,                 0.5f, "knob"}},
        {"tuneKnob",                 {ID::Tune,                  Name::Tune,                   0.5f, "knob"}},
        {"transposeKnob",            {ID::Transpose,             Name::Transpose,              0.5f, "knob",
            [](Knob *k) {
                k->altDragCallback = [](const double value) {
                    const auto octValue = static_cast<int>(juce::jmap(value, -2.0, 2.0));
                    return juce::jmap(static_cast<double>(octValue), -2.0, 2.0, 0.0, 1.0);
                };
            }}},

        // ---- GLOBAL ----
        {"portamentoKnob",           {ID::Portamento,            Name::Portamento,             0.f,  "knob"}},
        {"unisonDetuneKnob",         {ID::UnisonDetune,          Name::UnisonDetune,           0.25f,"knob"}},

        // ---- CONTROL ----
        {"vibratoRateKnob",          {ID::VibratoRate,           Name::VibratoRate,            0.3f, "knob"}},

        // ---- VOICE VARIATION ----
        {"filterSlopKnob",           {ID::FilterSlop,            Name::FilterSlop,             0.25f,"knob"}},
        {"portamentoSlopKnob",       {ID::PortamentoSlop,        Name::PortamentoSlop,         0.25f,"knob"}},
        {"envelopeSlopKnob",         {ID::EnvelopeSlop,          Name::EnvelopeSlop,           0.25f,"knob"}},
        {"levelSlopKnob",            {ID::LevelSlop,             Name::LevelSlop,              0.25f,"knob"}},

        // ---- LFO 1 ----
        {"lfo1RateKnob",             {ID::LFO1Rate,              Name::LFO1Rate,               0.5f, "knob"}},
        {"lfo1ModAmount1Knob",       {ID::LFO1ModAmount1,        Name::LFO1ModAmount1,         0.f,  "knob"}},
        {"lfo1ModAmount2Knob",       {ID::LFO1ModAmount2,        Name::LFO1ModAmount2,         0.f,  "knob"}},
        {"lfo1Wave1Knob",            {ID::LFO1Wave1,             Name::LFO1Wave1,              0.5f, "knob"}},
        {"lfo1Wave2Knob",            {ID::LFO1Wave2,             Name::LFO1Wave2,              0.5f, "knob"}},
        {"lfo1Wave3Knob",            {ID::LFO1Wave3,             Name::LFO1Wave3,              0.5f, "knob"}},
        {"lfo1PWSlider",             {ID::LFO1PW,                Name::LFO1PW,                 0.f,  "knob"}},

        // ---- LFO 2 ----
        {"lfo2RateKnob",             {ID::LFO2Rate,              Name::LFO2Rate,               0.5f, "knob"}},
        {"lfo2ModAmount1Knob",       {ID::LFO2ModAmount1,        Name::LFO2ModAmount1,         0.f,  "knob"}},
        {"lfo2ModAmount2Knob",       {ID::LFO2ModAmount2,        Name::LFO2ModAmount2,         0.f,  "knob"}},
        {"lfo2Wave1Knob",            {ID::LFO2Wave1,             Name::LFO2Wave1,              0.5f, "knob"}},
        {"lfo2Wave2Knob",            {ID::LFO2Wave2,             Name::LFO2Wave2,              0.5f, "knob"}},
        {"lfo2Wave3Knob",            {ID::LFO2Wave3,             Name::LFO2Wave3,              0.5f, "knob"}},
        {"lfo2PWSlider",             {ID::LFO2PW,                Name::LFO2PW,                 0.f,  "knob"}},
    };
    // clang-format on

    return reg;
}

const std::unordered_map<std::string, ObxfAudioProcessorEditor::ButtonDescriptor> &
ObxfAudioProcessorEditor::buttonRegistry()
{
    using namespace SynthParam;

    // clang-format off
    static const std::unordered_map<std::string, ButtonDescriptor> reg = {
        // ---- OSCILLATORS ----
        {"oscSyncButton",            {ID::OscSync,               Name::OscSync,               "button"}},
        {"osc1SawButton",            {ID::Osc1SawWave,           Name::Osc1SawWave,           "button"}},
        {"osc2SawButton",            {ID::Osc2SawWave,           Name::Osc2SawWave,           "button"}},
        {"osc1PulseButton",          {ID::Osc1PulseWave,         Name::Osc1PulseWave,         "button"}},
        {"osc2PulseButton",          {ID::Osc2PulseWave,         Name::Osc2PulseWave,         "button"}},
        {"osc2KeytrackButton",       {ID::Osc2Keytrack,          Name::Osc2Keytrack,          "button-keytrack"}},
        {"envToPitchInvertButton",   {ID::EnvToPitchInvert,      Name::EnvToPitchInvert,      "button-slim"}},
        {"envToPWInvertButton",      {ID::EnvToPWInvert,         Name::EnvToPWInvert,         "button-slim"}},
        {"envToPitchBothOscsButton", {ID::EnvToPitchBothOscs,    Name::EnvToPitchBothOscs,    "button-slim"}},
        {"envToPWBothOscsButton",    {ID::EnvToPWBothOscs,       Name::EnvToPWBothOscs,       "button-slim"}},

        // ---- FILTER ----
        {"filter2PoleBPBlendButton", {ID::Filter2PoleBPBlend,    Name::Filter2PoleBPBlend,    "button-slim"}},
        {"filter4PoleModeButton",    {ID::Filter4PoleMode,       Name::Filter4PoleMode,       "button-slim"}},
        {"filter2PolePushButton",    {ID::Filter2PolePush,       Name::Filter2PolePush,       "button-slim"}},
        {"filter4PoleXpanderButton", {ID::Filter4PoleXpander,    Name::Filter4PoleXpander,    "button"}},
        {"filterEnvInvertButton",    {ID::FilterEnvInvert,       Name::FilterEnvInvert,       "button-slim"}},

        // ---- GLOBAL ----
        {"unisonButton",             {ID::Unison,                Name::Unison,                "button"}},
        {"hqModeButton",             {ID::HQMode,                Name::HQMode,                "button"}},
        {"bendOsc2OnlyButton",       {ID::BendOsc2Only,          Name::BendOsc2Only,          "button"}},

        // ---- CONTROL ----
        {"vibratoWaveButton",        {ID::VibratoWave,           Name::VibratoWave,           "button-slim-vibrato-wave"}},

        // ---- LFO 1 ----
        {"lfo1TempoSyncButton",      {ID::LFO1TempoSync,         Name::LFO1TempoSync,         "button-slim"}},

        // ---- LFO 2 ----
        {"lfo2TempoSyncButton",      {ID::LFO2TempoSync,         Name::LFO2TempoSync,         "button-slim-alt"}},
    };
    // clang-format on

    return reg;
}

const std::unordered_map<std::string, ObxfAudioProcessorEditor::MultiStateDescriptor> &
ObxfAudioProcessorEditor::multiStateRegistry()
{
    using namespace SynthParam;

    // clang-format off
    static const std::unordered_map<std::string, MultiStateDescriptor> reg = {
        {"noiseColorButton",         {ID::NoiseColor,            Name::NoiseColor,            "button-slim-noise",  3}},

        // ---- LFO 1 routing ----
        {"lfo1ToOsc1PitchButton",    {ID::LFO1ToOsc1Pitch,       Name::LFO1ToOsc1Pitch,       "button-dual",     3}},
        {"lfo1ToOsc2PitchButton",    {ID::LFO1ToOsc2Pitch,       Name::LFO1ToOsc2Pitch,       "button-dual",     3}},
        {"lfo1ToFilterCutoffButton", {ID::LFO1ToFilterCutoff,    Name::LFO1ToFilterCutoff,    "button-dual",     3}},
        {"lfo1ToOsc1PWButton",       {ID::LFO1ToOsc1PW,          Name::LFO1ToOsc1PW,          "button-dual",     3}},
        {"lfo1ToOsc2PWButton",       {ID::LFO1ToOsc2PW,          Name::LFO1ToOsc2PW,          "button-dual",     3}},
        {"lfo1ToVolumeButton",       {ID::LFO1ToVolume,          Name::LFO1ToVolume,          "button-dual",     3}},

        // ---- LFO 2 routing ----
        {"lfo2ToOsc1PitchButton",    {ID::LFO2ToOsc1Pitch,       Name::LFO2ToOsc1Pitch,       "button-dual-alt", 3}},
        {"lfo2ToOsc2PitchButton",    {ID::LFO2ToOsc2Pitch,       Name::LFO2ToOsc2Pitch,       "button-dual-alt", 3}},
        {"lfo2ToFilterCutoffButton", {ID::LFO2ToFilterCutoff,    Name::LFO2ToFilterCutoff,    "button-dual-alt", 3}},
        {"lfo2ToOsc1PWButton",       {ID::LFO2ToOsc1PW,          Name::LFO2ToOsc1PW,          "button-dual-alt", 3}},
        {"lfo2ToOsc2PWButton",       {ID::LFO2ToOsc2PW,          Name::LFO2ToOsc2PW,          "button-dual-alt", 3}},
        {"lfo2ToVolumeButton",       {ID::LFO2ToVolume,          Name::LFO2ToVolume,          "button-dual-alt", 3}},
    };
    // clang-format on

    return reg;
}

const std::unordered_map<std::string, ObxfAudioProcessorEditor::ListDescriptor> &
ObxfAudioProcessorEditor::listRegistry()
{
    using namespace SynthParam;

    // clang-format off
    static const std::unordered_map<std::string, ListDescriptor> reg = {
        {"polyphonyMenu",        {ID::Polyphony,          Name::Polyphony,          "menu-poly"         }},
        {"unisonVoicesMenu",     {ID::UnisonVoices,       Name::UnisonVoices,       "menu-poly"         }},
        {"envLegatoModeMenu",    {ID::EnvLegatoMode,      Name::EnvLegatoMode,      "menu-legato"       }},
        {"notePriorityMenu",     {ID::NotePriority,       Name::NotePriority,       "menu-note-priority"}},
        {"bendUpRangeMenu",      {ID::BendUpRange,        Name::BendUpRange,        "menu-pitch-bend"   }},
        {"bendDownRangeMenu",    {ID::BendDownRange,      Name::BendDownRange,      "menu-pitch-bend"   }},
        {"filterXpanderModeMenu",{ID::FilterXpanderMode,  Name::FilterXpanderMode,  "menu-xpander"      }},
    };
    // clang-format on

    return reg;
}

std::map<std::string, ObxfAudioProcessorEditor::PanelGroup>
ObxfAudioProcessorEditor::panelGroupDefinitions()
{
    auto makePanel = [](std::string selector, std::vector<std::string> names,
                        std::unique_ptr<PanelGroup> child = nullptr) -> Panel {
        Panel p;
        p.selectorButtonName = std::move(selector);
        p.widgetNames = std::move(names);
        p.childGroup = std::move(child);
        return p;
    };

    // clang-format off
    std::map<std::string, PanelGroup> m;

    // ---- GLOBAL / MPE ----
    {
        auto &g = m["global"];

        g.panels.push_back(
            makePanel("globalSelectButton",
            {
                "portamentoKnob", "unisonDetuneKnob", "unisonButton",
                "unisonVoicesMenu", "hqModeButton", "lockHQButton",
                "polyphonyMenu", "envLegatoModeMenu", "notePriorityMenu",
                "globalBGLabel",
            }));

        auto mpeSubGroup = std::make_unique<PanelGroup>();

        mpeSubGroup->panels.push_back(
            makePanel("mpeStrikeSelectButton",
            {
                "mpeStrikeDestination1Menu", "mpeStrikeAmount1Knob",
                "mpeStrikeDestination2Menu", "mpeStrikeAmount2Knob",
            }));
        mpeSubGroup->panels.push_back(
            makePanel("mpeLiftSelectButton",
            {
            "mpeLiftDestination1Menu",   "mpeLiftAmount1Knob",
            "mpeLiftDestination2Menu",   "mpeLiftAmount2Knob",
            }));
        mpeSubGroup->panels.push_back(
            makePanel("mpePressSelectButton",
            {
                "mpePressDestination1Menu",  "mpePressAmount1Knob",
                "mpePressDestination2Menu",  "mpePressAmount2Knob",
            }));
        mpeSubGroup->panels.push_back(
            makePanel("mpeSlideSelectButton",
            {
                "mpeSlideDestination1Menu",  "mpeSlideAmount1Knob",
                "mpeSlideDestination2Menu",  "mpeSlideAmount2Knob",
            }));

        g.panels.push_back(
            makePanel("mpeSelectButton",
            {
                "mpeLinesLabel", "mpeSlideBipolarButton", "mpeGlideRangeMenu",
            },
            std::move(mpeSubGroup)));
    }

    // ---- LFO ----
    {
        auto &g = m["lfo"];
        g.panels.push_back(makePanel("lfo1SelectButton", {
            "lfo1RateKnob", "lfo1ModAmount1Knob", "lfo1ModAmount2Knob",
            "lfo1Wave1Knob", "lfo1Wave2Knob", "lfo1Wave3Knob", "lfo1PWSlider",
            "lfo1TempoSyncButton",
            "lfo1ToOsc1PitchButton", "lfo1ToOsc2PitchButton",
            "lfo1ToFilterCutoffButton", "lfo1ToOsc1PWButton",
            "lfo1ToOsc2PWButton", "lfo1ToVolumeButton",
            "lfo1Wave2Label",
        }));
        g.panels.push_back(makePanel("lfo2SelectButton", {
            "lfo2RateKnob", "lfo2ModAmount1Knob", "lfo2ModAmount2Knob",
            "lfo2Wave1Knob", "lfo2Wave2Knob", "lfo2Wave3Knob", "lfo2PWSlider",
            "lfo2TempoSyncButton",
            "lfo2ToOsc1PitchButton", "lfo2ToOsc2PitchButton",
            "lfo2ToFilterCutoffButton", "lfo2ToOsc1PWButton",
            "lfo2ToOsc2PWButton", "lfo2ToVolumeButton",
            "lfo2Wave2Label",
        }));
    }

    // clang-format on
    return m;
}

void ObxfAudioProcessorEditor::PanelGroup::resolve(
    const std::map<juce::String, std::unique_ptr<juce::Component>> &componentMap)
{
    for (auto &panel : panels)
    {
        panel.resolvedWidgets.clear();

        for (auto &name : panel.widgetNames)
        {
            if (auto it = componentMap.find(name); it != componentMap.end() && it->second)
            {
                panel.resolvedWidgets.push_back(it->second.get());
            }
        }

        if (panel.childGroup)
        {
            panel.childGroup->resolve(componentMap);
        }
    }
}

void ObxfAudioProcessorEditor::PanelGroup::showPanel(int index)
{
    activePanel = index;

    for (int i = 0; i < (int)panels.size(); i++)
    {
        const bool visible = (i == index);

        for (auto *c : panels[i].resolvedWidgets)
        {
            c->setVisible(visible);
        }

        if (panels[i].childGroup)
        {
            if (visible)
            {
                panels[i].childGroup->showPanel(panels[i].childGroup->activePanel);
            }
            else
            {
                panels[i].childGroup->hideAll();
            }
        }
    }
}

void ObxfAudioProcessorEditor::PanelGroup::hideAll()
{
    for (auto &panel : panels)
    {
        for (auto *c : panel.resolvedWidgets)
        {
            c->setVisible(false);
        }

        if (panel.childGroup)
        {
            panel.childGroup->hideAll();
        }
    }
}

template <typename T>
static T *storeWidget(std::map<juce::String, std::unique_ptr<juce::Component>> &map,
                      ObxfAudioProcessorEditor *editor, const juce::String &name,
                      std::unique_ptr<T> widget)
{
    if (!widget)
    {
        return nullptr;
    }

    auto *raw = widget.get();

    map[name] = std::move(widget);
    editor->addAndMakeVisible(*raw);

    return raw;
}

ObxfAudioProcessorEditor::ObxfAudioProcessorEditor(ObxfAudioProcessor &p)
    : AudioProcessorEditor(&p), processor(p), utils(p.getUtils()),
      paramCoordinator(p.getParamCoordinator()), imageCache(utils), midiStart(5000),
      sizeStart(4000), themeStart(1000), themes(utils.getThemeLocations())
{
    skinLoaded = false;
    updateProcessorImpliedScaleFactor = false;

    lookAndFeelPtr = std::make_unique<obxf::LookAndFeel>(this);
    setLookAndFeel(lookAndFeelPtr.get());

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        // We are by definition one process so it is safe to
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeelPtr.get());
    }

    keyCommandHandler = std::make_unique<KeyCommandHandler>();
    keyCommandHandler->setNextProgramCallback([this]() {
        nextProgram();
        grabKeyboardFocus();
    });
    keyCommandHandler->setPrevProgramCallback([this]() {
        prevProgram();
        grabKeyboardFocus();
    });
    keyCommandHandler->setRefreshThemeCallback([this]() {
        clean();
        loadTheme(processor);
    });

    commandManager.registerAllCommandsForTarget(keyCommandHandler.get());
    commandManager.setFirstCommandTarget(keyCommandHandler.get());
    commandManager.getKeyMappings()->resetToDefaultMappings();
    getTopLevelComponent()->addKeyListener(commandManager.getKeyMappings());

    aboutScreen = std::make_unique<AboutScreen>(*this);
    addChildComponent(*aboutScreen);

    saveDialog = std::make_unique<SaveDialog>(*this);
    addChildComponent(*saveDialog);

    mpeMatrixEditor = std::make_unique<MPEMatrixEditor>(processor);
    addChildComponent(*mpeMatrixEditor);

    const auto jersey = juce::Typeface::createSystemTypefaceFor(BinaryData::Jersey20_ttf,
                                                                BinaryData::Jersey20_ttfSize);
    const auto trek =
        juce::Typeface::createSystemTypefaceFor(BinaryData::Trek_ttf, BinaryData::Trek_ttfSize);

    patchNameFont = juce::FontOptions(jersey);
    midiLearnPopupFont = juce::FontOptions(trek);

    themeLocation = utils.getCurrentThemeLocation();

    loadTheme(processor);

    if (backgroundIsSVG)
    {
        const auto &bgi = imageCache.getSVGDrawable("background");

        initialWidth = bgi->getWidth();
        initialHeight = bgi->getHeight();
    }
    else
    {
        initialWidth = backgroundImage.getWidth();
        initialHeight = backgroundImage.getHeight();
    }

    if (noThemesAvailable)
    {
        initialWidth = 800;
        initialHeight = 400;
    }

    setSize(initialWidth, initialHeight);
    initialBounds = getLocalBounds();
    setResizable(true, false);

    constrainer = std::make_unique<juce::ComponentBoundsConstrainer>();
    constrainer->setMinimumSize(initialWidth * scaleFactors[0], initialHeight * scaleFactors[0]);
    constrainer->setFixedAspectRatio(static_cast<double>(initialWidth) / initialHeight);
    setConstrainer(constrainer.get());

    updateFromHost();

    idleTimer = std::make_unique<IdleTimer>(this);
    idleTimer->startTimer(1000 / 30);
    processor.uiState.editorAttached = true;

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    inspector = std::make_unique<melatonin::Inspector>(*this, false);
    inspector->setVisible(false);
#endif

    focusDebugger = std::make_unique<FocusDebugger>(*this);
    // focusDebugger->setDoFocusDebug(true);

    auto sf = utils.getDefaultZoomFactor();

    if (processor.lastImpliedScaleFactor != 1.f)
    {
        sf = processor.lastImpliedScaleFactor;
    }

    setSize(juce::roundToInt(static_cast<float>(initialWidth) * sf),
            juce::roundToInt(static_cast<float>(initialHeight) * sf));

    updateProcessorImpliedScaleFactor = true;

    resized();

    for (auto &[n, c] : componentMap)
    {
        if (auto hcf = dynamic_cast<HasScaleFactor *>(c.get()))
        {
            hcf->scaleFactorChanged();
        }
    }

    resizeOnNextIdle = 3;
    ignoreHostScale = false;
    dontParentMenusToEditor = false;

#if JUCE_LINUX
    if (juce::PluginHostType().isBitwigStudio())
        dontParentMenusToEditor = true;
#endif
}

void ObxfAudioProcessorEditor::parentHierarchyChanged()
{
#if JUCE_WINDOWS
    if (utils.getUseSoftwareRenderer())
    {
        if (auto peer = getPeer())
        {
            peer->setCurrentRenderingEngine(0); // 0 for software mode, 1 for Direct2D mode
        }
    }
#endif

    if (isShowing() && isVisible())
    {
        resized();
    }
}

void ObxfAudioProcessorEditor::resized()
{
    scaleFactorChanged();
    themeLocation = utils.getCurrentThemeLocation();

    if (cachedLayout.empty())
    {
        return;
    }

    static FocusOrder focusOrder;

    if (saveDialog)
    {
        saveDialog->resetState();
    }

    for (const auto &layout : cachedLayout)
    {
        const auto &name = layout.name;
        auto it = componentMap.find(name);

        if (it == componentMap.end() || !it->second)
        {
            continue;
        }

        Component *comp = it->second.get();

        auto addOrder = [&]() {
            auto tfo = focusOrder.getOrder(name.toStdString());

            if (tfo != 0 && comp->getTitle().isNotEmpty())
            {
                comp->setExplicitFocusOrder(tfo);
                comp->setWantsKeyboardFocus(true);
            }
        };

        if (auto *knob = dynamic_cast<Knob *>(comp))
        {
            if (layout.d > 0)
                knob->setBounds(transformBounds(layout.x, layout.y, layout.d, layout.d));
            else if (layout.w > 0 && layout.h > 0)
                knob->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
            else
                knob->setBounds(
                    transformBounds(layout.x, layout.y, defKnobDiameter, defKnobDiameter));

            knob->setPopupDisplayEnabled(true, true, knob->getParentComponent());
            addOrder();
        }
        else if (dynamic_cast<ButtonList *>(comp) || dynamic_cast<ImageMenu *>(comp) ||
                 dynamic_cast<MultiStateButton *>(comp))
        {
            comp->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
            addOrder();
        }
        else if (dynamic_cast<ToggleButton *>(comp))
        {
            comp->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
            addOrder();

            // Programmer select buttons have a paired label at the same bounds
            if (name.startsWith("select") && name.endsWith("Button"))
            {
                auto labelName = juce::String(name).replace("Button", "Label");
                if (auto lit = componentMap.find(labelName);
                    lit != componentMap.end() && lit->second)
                {
                    lit->second->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));
                }
            }
        }
        else
        {
            comp->setBounds(transformBounds(layout.x, layout.y, layout.w, layout.h));

            if (name.startsWith("voice") && name.endsWith("LED"))
            {
                auto bgName = juce::String(name).replace("LED", "BG");

                if (auto bgIt = componentMap.find(bgName);
                    bgIt != componentMap.end() && bgIt->second)
                {
                    bgIt->second->setBounds(
                        transformBounds(layout.x, layout.y, layout.w, layout.h));
                }
            }

            addOrder();
        }
    }

    if (saveDialog)
    {
        saveDialog->resized();
    }

    const float sf = impliedScaleFactor();

    for (auto &overlay : midiLearnOverlays)
    {
        if (overlay)
        {
            overlay->setScaleFactor(sf);
        }
    }

    if (aboutScreen)
    {
        aboutScreen->setBounds(getBounds());
    }

    if (saveDialog)
    {
        saveDialog->setBounds(getBounds());
    }

    if (mpeMatrixEditor)
    {
        const int w = MPEMatrixEditor::preferredWidth();
        const int h = MPEMatrixEditor::preferredHeight();

        mpeMatrixEditor->setBounds((getWidth() - w) / 2, (getHeight() - h) / 2, w, h);
    }

    if (updateProcessorImpliedScaleFactor)
    {
        processor.lastImpliedScaleFactor = impliedScaleFactor();
    }
}

void ObxfAudioProcessorEditor::idle()
{
    if (!skinLoaded)
    {
        return;
    }

    countTimer++;

    if (isShowing() && isVisible() && resizeOnNextIdle >= 0)
    {
        resizeOnNextIdle--;
        if (resizeOnNextIdle == 0)
        {
            resized();
            for (auto &[n, c] : componentMap)
                if (auto hcf = dynamic_cast<HasScaleFactor *>(c.get()))
                    hcf->scaleFactorChanged();
        }
    }

    if (countTimer == 4)
    {
        if (componentMap.count("patchNumberMenu"))
        {
            updatePatchNumberIfNeeded();
        }

        updateSelectButtonStates();
        countTimer = 0;
    }

    // MIDI learn state sync
    if (auto *midiLearnButton = getWidget<ToggleButton>("midiLearnButton"))
    {
        const auto mla = paramCoordinator.midiLearnAttachment.get();
        const auto mts = midiLearnButton->getToggleState();

        if (mla != mts)
        {
            if (!mla)
            {
                exitMidiLearnMode();
            }

            midiLearnButton->setToggleState(mla, juce::dontSendNotification);
        }

        const auto lup = processor.getMidiHandler().getLastUsedParameter();

        if (lup > 0)
        {
            for (auto &p : ParameterList)
            {
                if (p.meta.id == (uint32_t)lup && p.ID != midiLearnLastUsedPID)
                {
                    midiLearnLastUsedPID = p.ID;
                    repaint();
                }
            }
        }
        else if (midiLearnLastUsedPID.isNotEmpty())
        {
            midiLearnLastUsedPID = "";
            repaint();
        }
    }

    // Voice LEDs
    if (auto *polyphonyMenu = getWidget<ButtonList>("polyphonyMenu"))
    {
        const int curPoly = juce::jmin(polyphonyMenu->getSelectedId(), MAX_VOICES);

        for (int i = 0; i < MAX_VOICES; i++)
        {
            const auto ledName = fmt::format("voice{}LED", i + 1);
            const auto bgName = fmt::format("voice{}BG", i + 1);
            auto *led = getWidget<Label>(ledName);
            auto *bg = getWidget<Label>(bgName);

            if (!led || !bg)
            {
                continue;
            }

            if (i >= curPoly && bg->isVisible())
            {
                bg->setVisible(false);
                led->setVisible(false);
            }
            else if (i < curPoly && !bg->isVisible())
            {
                bg->setVisible(true);
                led->setVisible(true);
            }
        }

        for (int i = 0; i < curPoly; i++)
        {
            const auto state =
                std::cbrtf(static_cast<float>(processor.uiState.voiceStatusValue[i]));
            auto *led = getWidget<Label>(fmt::format("voice{}LED", i + 1));

            if (led && state != led->getAlpha())
            {
                led->setAlpha(state);
            }
        }

        for (int i = curPoly; i < MAX_VOICES; i++)
        {
            auto *led = getWidget<Label>(fmt::format("voice{}LED", i + 1));

            if (led && led->getAlpha() > 0.f)
            {
                led->setAlpha(0.f);
            }
        }
    }

    // Osc triangle labels
    {
        auto *osc1Saw = getWidget<ToggleButton>("osc1SawButton");
        auto *osc1Pulse = getWidget<ToggleButton>("osc1PulseButton");
        auto *tri1 = getWidget<Label>("osc1TriangleLabel");

        if (tri1 && osc1Saw && osc1Pulse)
        {
            const int state = !(osc1Saw->getToggleState() || osc1Pulse->getToggleState());

            if (tri1->getCurrentFrame() != state)
            {
                tri1->setCurrentFrame(state);
            }
        }
    }
    {
        auto *osc2Saw = getWidget<ToggleButton>("osc2SawButton");
        auto *osc2Pulse = getWidget<ToggleButton>("osc2PulseButton");
        auto *tri2 = getWidget<Label>("osc2TriangleLabel");

        if (tri2 && osc2Saw && osc2Pulse)
        {
            const int state = !(osc2Saw->getToggleState() || osc2Pulse->getToggleState());

            if (tri2->getCurrentFrame() != state)
            {
                tri2->setCurrentFrame(state);
            }
        }
    }

    // Pulsewidth labels
    {
        auto *pwKnob = getWidget<Knob>("oscPWKnob");
        auto *pw2Knob = getWidget<Knob>("osc2PWOffsetKnob");
        auto *pulse1Label = getWidget<Label>("osc1PulseLabel");
        auto *pulse2Label = getWidget<Label>("osc2PulseLabel");

        if (pulse1Label && pulse2Label && pwKnob && pw2Knob)
        {
            const auto pw1 = juce::roundToInt(pwKnob->getValue() * 46.f);
            const auto pw2 = juce::jmin(pw1 + juce::roundToInt(pw2Knob->getValue() * 46.f), 50);

            if (pulse1Label->getCurrentFrame() != pw1)
            {
                pulse1Label->setCurrentFrame(pw1);
            }

            if (pulse2Label->getCurrentFrame() != pw2)
            {
                pulse2Label->setCurrentFrame(pw2);
            }
        }
    }

    // LFO Wave 2 labels
    {
        auto *lfo1PW = getWidget<Knob>("lfo1PWSlider");
        auto *wave2Lbl1 = getWidget<Label>("lfo1Wave2Label");

        if (wave2Lbl1 && wave2Lbl1->isVisible() && lfo1PW)
        {
            const int state = juce::roundToInt(lfo1PW->getValue() * 24.f);

            if (wave2Lbl1->getCurrentFrame() != state)
            {
                wave2Lbl1->setCurrentFrame(state);
            }
        }
    }
    {
        auto *lfo2PW = getWidget<Knob>("lfo2PWSlider");
        auto *wave2Lbl2 = getWidget<Label>("lfo2Wave2Label");

        if (wave2Lbl2 && wave2Lbl2->isVisible() && lfo2PW)
        {
            const int state = juce::roundToInt(lfo2PW->getValue() * 24.f);

            if (wave2Lbl2->getCurrentFrame() != state)
            {
                wave2Lbl2->setCurrentFrame(state);
            }
        }
    }

    // Filter mode/options labels and button visibility
    {
        const auto fourPole = [&] {
            auto *b = getWidget<ToggleButton>("filter4PoleModeButton");
            return b && b->getToggleState();
        }();
        const auto xpanderMode = [&] {
            auto *b = getWidget<ToggleButton>("filter4PoleXpanderButton");
            return b && b->getToggleState();
        }();
        const auto bpBlend = [&] {
            auto *b = getWidget<ToggleButton>("filter2PoleBPBlendButton");
            return b && b->getToggleState();
        }();

        const int filterModeFrame = fourPole ? (xpanderMode ? 3 : 2) : (bpBlend ? 1 : 0);

        if (auto *filterModeLabel = getWidget<Label>("filterModeLabel"))
        {
            if (filterModeLabel->getCurrentFrame() != filterModeFrame)
            {
                filterModeLabel->setCurrentFrame(filterModeFrame);
            }
        }

        auto *filterOptionsLabel = getWidget<Label>("filterOptionsLabel");

        if (filterOptionsLabel && (int)fourPole != filterOptionsLabel->getCurrentFrame())
        {
            filterOptionsLabel->setCurrentFrame(fourPole);
        }
    }

    // Unison voices menu dimming
    {
        auto *unisonButton = getWidget<ToggleButton>("unisonButton");
        auto *unisonVoicesMenu = getWidget<ButtonList>("unisonVoicesMenu");

        if (unisonButton && unisonVoicesMenu)
        {
            const bool on = unisonButton->getToggleState();

            if (on && unisonVoicesMenu->getAlpha() < 1.f)
            {
                unisonVoicesMenu->setAlpha(1.f);
            }

            if (!on && unisonVoicesMenu->getAlpha() == 1.f)
            {
                unisonVoicesMenu->setAlpha(0.25f);
            }
        }
    }

    // Patch name display
    if (auto *patchNameLabel = getWidget<Display>("patchNameLabel"))
    {
        if (!patchNameLabel->isBeingEdited())
        {
            const auto oldName = processor.getActiveProgram().getName();

            if (patchNameLabel->getText() != oldName)
            {
                patchNameLabel->setText(oldName, juce::dontSendNotification);
            }
        }
    }
}

void ObxfAudioProcessorEditor::updateSelectButtonStates()
{
    auto *groupSelectButton = getWidget<ToggleButton>("groupSelectButton");
    auto lsp = processor.lastLoadedPatchNode.lock();

    auto selectButton = [&](int i) {
        return getWidget<ToggleButton>(fmt::format("select{}Button", i + 1));
    };
    auto selectLabel = [&](int i) { return getWidget<Label>(fmt::format("select{}Label", i + 1)); };

    if (lsp)
    {
        auto parentCount{MAX_PROGRAMS};
        const auto idx = lsp->indexInParent;

        if (auto lp = lsp->parent.lock())
        {
            parentCount = lp->nonFolderChildIndices.size();
        }

        const uint8_t curGroup = idx / NUM_PATCHES_PER_GROUP;
        const uint8_t curPatchInGroup = idx % NUM_PATCHES_PER_GROUP;

        currProgrammerGroup = curGroup;
        currProgrammerPatch = curPatchInGroup;

        for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
        {
            bool enabled{true};

            if (groupSelectButton)
            {
                if (groupSelectButton->getToggleState())
                {
                    enabled = i <= (parentCount / NUM_PATCHES_PER_GROUP);
                }
                else if (i + curGroup * NUM_PATCHES_PER_GROUP >= parentCount)
                {
                    enabled = false;
                }
            }

            uint8_t offset = 0;

            if (enabled && selectButton(i) && selectButton(i)->isDown())
            {
                offset += 1;
            }

            if (i == curGroup)
            {
                offset += 2;
            }
            if (i == curPatchInGroup)
            {
                offset += 4;
            }

            if (auto *lbl = selectLabel(i))
            {
                bool doRepaint = false;

                if (lbl->getCurrentFrame() != offset)
                {
                    lbl->setCurrentFrame(offset);
                    doRepaint = true;
                }
                if (lbl->isEnabled() != enabled)
                {
                    lbl->setEnabled(enabled);
                    doRepaint = true;
                }
                if (doRepaint)
                {
                    lbl->repaint();
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < NUM_PATCHES_PER_GROUP; i++)
        {
            if (auto *lbl = selectLabel(i))
            {
                if (lbl->getCurrentFrame() != 0)
                {
                    lbl->setCurrentFrame(0);
                    lbl->setEnabled(true);
                    lbl->repaint();
                }
            }
        }
    }
}

void ObxfAudioProcessorEditor::loadPatchFromProgrammer(int whichButton)
{
    int newIdx = whichButton;
    const auto lsp = processor.lastLoadedPatchNode.lock();
    const auto *gsb = getWidget<ToggleButton>("groupSelectButton");
    const bool gsOn = gsb && gsb->getToggleState();

    if (!lsp)
    {
        newIdx *= gsOn ? NUM_PATCHES_PER_GROUP : 1;

        if (newIdx >= 0 && newIdx < (int)utils.patchesAsLinearList.size())
        {
            utils.loadPatch(utils.patchesAsLinearList[newIdx]);
        }

        return;
    }

    const auto lspParent = lsp->parent.lock();

    if (!lspParent)
    {
        return;
    }

    newIdx = gsOn ? whichButton * NUM_PATCHES_PER_GROUP + currProgrammerPatch
                  : currProgrammerGroup * NUM_PATCHES_PER_GROUP + whichButton;

    if ((size_t)newIdx < lspParent->nonFolderChildIndices.size())
    {
        utils.loadPatch(utils.patchesAsLinearList[lspParent->nonFolderChildIndices[newIdx]]);
    }
}

void ObxfAudioProcessorEditor::updatePatchNumberIfNeeded()
{
    auto *patchNumberMenu = getWidget<DisplayDigits>("patchNumberMenu");

    if (!patchNumberMenu)
    {
        return;
    }

    auto fr = patchNumberMenu->getFrame();
    auto ppl = processor.lastLoadedPatchNode.lock();
    auto nextIndex = fr;

    if (ppl)
    {
        if (ppl->indexInParent + 1 != fr)
        {
            nextIndex = ppl->indexInParent + 1;
        }
    }
    else
    {
        nextIndex = 0;
    }

    if (nextIndex != fr)
    {
        patchNumberMenu->setFrame(nextIndex);
    }
}

void ObxfAudioProcessorEditor::loadTheme(ObxfAudioProcessor &ownerFilter)
{
    skinLoaded = false;

    if (!loadThemeFilesAndCheck())
    {
        return;
    }

    OBLOG(themes, "Setting theme to " << themeLocation.file.getFullPathName()
                                      << " (type=" << (int)themeLocation.locationType << ")");

    const auto parameterValues = saveComponentParameterValues();
    clearAndResetComponents(ownerFilter);

    if (themeLocation.locationType == Utils::EMBEDDED)
    {
        auto xml = juce::String(BinaryData::theme_xml, BinaryData::theme_xmlSize);
        cachedThemeXml = juce::XmlDocument(xml).getDocumentElement();
    }
    else
    {
        const juce::File theme = themeLocation.file.getChildFile("theme.xml");
        cachedThemeXml = juce::XmlDocument(theme).getDocumentElement();
    }

    if (!cachedThemeXml)
    {
        jassertfalse;
        return;
    }

    if (cachedThemeXml->getTagName() == "obxf-theme")
    {
        imageCache.clearCache();
        imageCache.embeddedMode = (themeLocation.locationType == Utils::EMBEDDED);
        imageCache.skinDir = imageCache.embeddedMode ? juce::File() : themeLocation.file;

        createComponentsFromXml(cachedThemeXml.get());
    }

    setupMenus();
    restoreComponentParameterValues(parameterValues);
    finalizeThemeLoad(ownerFilter);
    resized();
}

bool ObxfAudioProcessorEditor::loadThemeFilesAndCheck()
{
    themes = utils.getThemeLocations();

    if (themes.empty())
    {
        noThemesAvailable = true;
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "No Themes Found",
            juce::String() +
                "No themes were found in the Theme folder. The editor cannot be displayed. " +
                "Factory folder location is '" + utils.getDocumentFolder().getFullPathName() +
                "'.");
        repaint();
        return false;
    }
    noThemesAvailable = false;
    return true;
}

std::map<juce::String, float> ObxfAudioProcessorEditor::saveComponentParameterValues()
{
    std::map<juce::String, float> values;

    for (auto &[name, comp] : componentMap)
    {
        if (const auto *k = dynamic_cast<Knob *>(comp.get()))
        {
            values[name] = static_cast<float>(k->getValue());
        }
        else if (const auto *ms = dynamic_cast<MultiStateButton *>(comp.get()))
        {
            values[name] = static_cast<float>(ms->getValue());
        }
        else if (const auto *b = dynamic_cast<ToggleButton *>(comp.get()))
        {
            values[name] = b->getToggleState() ? 1.f : 0.f;
        }
    }

    return values;
}

void ObxfAudioProcessorEditor::clearAndResetComponents(ObxfAudioProcessor &ownerFilter)
{
    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    popupMenus.clear();
    cachedLayout.clear();
    componentByParamID.clear();
    panelGroups.clear();

    // Remove all child components that are in componentMap before clearing it
    for (auto &[name, comp] : componentMap)
    {
        if (comp)
        {
            removeChildComponent(comp.get());
        }
    }

    componentMap.clear();

    ownerFilter.removeChangeListener(this);

    themeLocation = utils.getCurrentThemeLocation();
}

bool ObxfAudioProcessorEditor::parseAndCreateComponentsFromTheme()
{
    const juce::File theme = themeLocation.file.getChildFile("theme.xml");

    if (!theme.existsAsFile())
    {
        storeWidget(componentMap, this, "mainMenu", addMenu(14, 25, 23, 35, "button-clear-red"));
        rebuildComponents(processor);

        return false;
    }

    juce::XmlDocument themeXml(theme);

    if (const auto doc = themeXml.getDocumentElement(); !doc)
    {
        notLoadTheme = true;
        setSize(1440, 486);
        return false;
    }
    else
    {
        if (doc->getTagName() == "obxf-theme")
        {
            createComponentsFromXml(doc.get());
        }

        resized();

        return true;
    }
}

void ObxfAudioProcessorEditor::createComponentsFromXml(const juce::XmlElement *doc)
{
    createParameterBoundWidgets(doc);
    createSpecialWidgets(doc);
    cacheLayoutFromXml(doc);

    panelGroups = panelGroupDefinitions();

    for (auto &[name, group] : panelGroups)
    {
        group.resolve(componentMap);
        group.showPanel(group.activePanel);
    }

    auto switchPanel = [this](PanelGroup &group, int index, std::function<void()> existingClickCb) {
        group.showPanel(index);

        if (existingClickCb)
        {
            existingClickCb();
        }

        if (auto *mlb = getWidget<ToggleButton>("midiLearnButton"); mlb && mlb->getToggleState())
        {
            enterMidiLearnMode();
        }
    };

    auto updateFilterVisibility = [this](std::function<void()> existingClickCb = nullptr) {
        const bool fourPole = getWidget<ToggleButton>("filter4PoleModeButton") &&
                              getWidget<ToggleButton>("filter4PoleModeButton")->getToggleState();
        const bool xpander = getWidget<ToggleButton>("filter4PoleXpanderButton") &&
                             getWidget<ToggleButton>("filter4PoleXpanderButton")->getToggleState();

        if (existingClickCb)
        {
            existingClickCb();
        }

        if (auto *b = getWidget<ToggleButton>("filter2PoleBPBlendButton"))
            b->setVisible(!fourPole);
        if (auto *b = getWidget<ToggleButton>("filter2PolePushButton"))
            b->setVisible(!fourPole);
        if (auto *b = getWidget<ToggleButton>("filter4PoleXpanderButton"))
            b->setVisible(fourPole);
        if (auto *k = getWidget<Knob>("filterModeKnob"))
            k->setVisible(!(fourPole && xpander));
        if (auto *m = getWidget<ButtonList>("filterXpanderModeMenu"))
            m->setVisible(fourPole && xpander);
    };

    for (auto &[groupName, group] : panelGroups)
    {
        for (int i = 0; i < (int)group.panels.size(); i++)
        {
            if (auto *b = getWidget<ToggleButton>(group.panels[i].selectorButtonName))
            {
                auto existingClickCb = b->onClick;

                if (b->getRadioGroupId() != 0)
                    b->onClick = [&group, i, existingClickCb, switchPanel]() {
                        switchPanel(group, i, existingClickCb);
                    };
                else
                    b->onClick = [&group, b, existingClickCb, switchPanel]() {
                        switchPanel(group, b->getToggleState() ? 1 : 0, existingClickCb);
                    };
            }
        }
    }

    if (auto *b = getWidget<ToggleButton>("filter4PoleModeButton"))
    {
        auto existingClickCb = b->onClick;
        b->onClick = [existingClickCb, updateFilterVisibility]() {
            updateFilterVisibility(existingClickCb);
        };
    }

    if (auto *b = getWidget<ToggleButton>("filter4PoleXpanderButton"))
    {
        auto existingClickCb = b->onClick;
        b->onClick = [existingClickCb, updateFilterVisibility]() {
            updateFilterVisibility(existingClickCb);
        };
    }

    componentByParamID.clear();

    for (auto &[n, c] : componentMap)
    {
        if (auto *dcp = dynamic_cast<HasParameterWithID *>(c.get()))
        {
            if (dcp->getParameterWithID())
            {
                componentByParamID[dcp->getParameterWithID()->getParameterID()] = c.get();
            }
        }
    }

    updateFilterVisibility();
}

void ObxfAudioProcessorEditor::createParameterBoundWidgets(const juce::XmlElement *doc)
{
    const auto &knobs = knobRegistry();
    const auto &buttons = buttonRegistry();
    const auto &multiState = multiStateRegistry();
    const auto &lists = listRegistry();

    for (const auto *child : doc->getChildWithTagNameIterator("widget"))
    {
        const std::string nameStd = child->getStringAttribute("name").toStdString();
        const juce::String name = child->getStringAttribute("name");
        const auto x = child->getIntAttribute("x");
        const auto y = child->getIntAttribute("y");
        const auto w = child->getIntAttribute("w");
        const auto h = child->getIntAttribute("h");
        const auto d = child->getIntAttribute("d");
        const auto fh = child->getIntAttribute("fh");
        const auto pic = child->getStringAttribute("pic");

        if (auto it = knobs.find(nameStd); it != knobs.end())
        {
            const auto &desc = it->second;
            auto knob = addKnob(x, y, w, h, d, fh, juce::String(desc.paramId), desc.defaultVal,
                                juce::String(desc.displayName),
                                useAssetOrDefault(pic, juce::String(desc.defaultAsset)));
            if (!knob)
            {
                continue;
            }

            if (desc.postCreate)
            {
                desc.postCreate(knob.get());
            }

            storeWidget(componentMap, this, name, std::move(knob));

            continue;
        }

        if (auto it = buttons.find(nameStd); it != buttons.end())
        {
            const auto &desc = it->second;
            auto button =
                addButton(x, y, w, h, juce::String(desc.paramId), juce::String(desc.displayName),
                          useAssetOrDefault(pic, juce::String(desc.defaultAsset)));

            if (!button)
            {
                continue;
            }

            if (desc.postCreate)
            {
                desc.postCreate(button.get());
            }

            storeWidget(componentMap, this, name, std::move(button));

            continue;
        }

        if (auto it = multiState.find(nameStd); it != multiState.end())
        {
            const auto &desc = it->second;
            auto btn = addMultiStateButton(
                x, y, w, h, juce::String(desc.paramId), juce::String(desc.displayName),
                useAssetOrDefault(pic, juce::String(desc.defaultAsset)), desc.numStates);
            if (!btn)
            {
                continue;
            }

            storeWidget(componentMap, this, name, std::move(btn));

            continue;
        }

        if (auto it = lists.find(nameStd); it != lists.end())
        {
            const auto &desc = it->second;
            auto list =
                addList(x, y, w, h, juce::String(desc.paramId), juce::String(desc.displayName),
                        useAssetOrDefault(pic, juce::String(desc.defaultAsset)));
            if (!list)
            {
                continue;
            }

            storeWidget(componentMap, this, name, std::move(list));

            continue;
        }

        if (name.startsWith("pan") && name.endsWith("Knob"))
        {
            const auto which = name.retainCharacters("12345678").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx >= 0 && whichIdx < MAX_PANNINGS)
            {
                auto knob =
                    addKnob(x, y, w, h, d, fh, fmt::format("PanVoice{}", which), 0.5f,
                            fmt::format("Pan Voice {}", which), useAssetOrDefault(pic, "knob"));
                if (knob)
                {
                    storeWidget(componentMap, this, name, std::move(knob));
                }
            }
        }
    }
}

void ObxfAudioProcessorEditor::createSpecialWidgets(const juce::XmlElement *doc)
{
    using namespace SynthParam;

    for (const auto *child : doc->getChildWithTagNameIterator("widget"))
    {
        const juce::String name = child->getStringAttribute("name");
        const auto x = child->getIntAttribute("x");
        const auto y = child->getIntAttribute("y");
        const auto w = child->getIntAttribute("w");
        const auto h = child->getIntAttribute("h");
        const auto pic = child->getStringAttribute("pic");
        const auto color =
            juce::Colour(child->getStringAttribute("color", "FFFF0000").getHexValue32());

        // Skip anything already handled by the parameter-bound pass
        if (componentMap.count(name))
        {
            continue;
        }

        if (name == "patchNameLabel")
        {
            auto label =
                std::make_unique<Display>("Patch Name", [this]() { return impliedScaleFactor(); });

            label->setBounds(transformBounds(x, y, w, h));
            label->setJustificationType(juce::Justification::centred);
            label->setMinimumHorizontalScale(1.f);
            label->setFont(patchNameFont.withHeight(18));
            label->setColour(juce::Label::textColourId, color);
            label->setColour(juce::Label::textWhenEditingColourId, color);
            label->setColour(juce::Label::outlineWhenEditingColourId,
                             juce::Colours::transparentBlack);
            label->setColour(juce::TextEditor::textColourId, color);
            label->setColour(juce::TextEditor::highlightedTextColourId, color);
            label->setColour(juce::TextEditor::highlightColourId, juce::Colour(0x20FFFFFF));
            label->setColour(juce::CaretComponent::caretColourId, color);
            lookAndFeelPtr->textInputColour = color;

            auto *raw = storeWidget(componentMap, this, name, std::move(label));
            auto *display = static_cast<Display *>(raw);

            display->onTextChange = [this, display]() {
                processor.getActiveProgram().setName(display->getText());
            };

            continue;
        }

        if (name.startsWith("voice") && name.endsWith("LED"))
        {
            const auto which = name.retainCharacters("0123456789").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx < 0 || whichIdx >= MAX_VOICES)
            {
                continue;
            }

            const auto assetName =
                useAssetOrDefault(pic, fmt::format("label-led{}", which / MAX_PANNINGS));

            if (auto bg = addLabel(x, y, w, h, h, fmt::format("Voice {} BG", which), assetName))
            {
                storeWidget(componentMap, this, name.replace("LED", "BG"), std::move(bg));
            }

            if (auto led = addLabel(x, y, w, h, h, fmt::format("Voice {} LED", which), assetName))
            {
                auto *raw = storeWidget(componentMap, this, name, std::move(led));
                static_cast<Label *>(raw)->setCurrentFrame(1);
            }

            continue;
        }

        struct
        {
            const char *widgetName;
            const char *displayName;
            const char *asset;
            bool toBack;
            bool isMPE;
            bool noMouseInteraction;
        } labelDefs[] = {
            {"masterBGLabel", "Master Section Background", "label-bg-master", true, false, true},
            {"globalBGLabel", "Global Section Background", "label-bg-global", true, false, true},
            {"mpeLinesLabel", "MPE Dimension Select Label", "label-mpe-lines", false, true, true},
            {"filterModeLabel", "Filter Mode Label", "label-filter-mode", true, false, false},
            {"filterOptionsLabel", "Filter Options Label", "label-filter-options", true, false,
             false},
            {"osc1TriangleLabel", "Osc 1 Triangle Icon", "label-osc-triangle", false, false, false},
            {"osc1PulseLabel", "Osc 1 Pulse Icon", "label-osc-pulse", false, false, false},
            {"osc2TriangleLabel", "Osc 2 Triangle Icon", "label-osc-triangle", false, false, false},
            {"osc2PulseLabel", "Osc 2 Pulse Icon", "label-osc-pulse", false, false, false},
            {"lfo1Wave2Label", "LFO 1 Wave 2 Icons", "label-lfo-wave2", true, false, false},
            {"lfo2Wave2Label", "LFO 2 Wave 2 Icons", "label-lfo-wave2", true, false, false},
        };

        bool handled = false;

        for (auto &def : labelDefs)
        {
            if (name != def.widgetName)
            {
                continue;
            }

            if (auto lbl = addLabel(x, y, w, h, h, def.displayName, def.asset))
            {
                auto *raw = storeWidget(componentMap, this, name, std::move(lbl));

                if (def.toBack)
                {
                    raw->toBack();
                }

                if (def.noMouseInteraction)
                {
                    raw->setInterceptsMouseClicks(false, false);
                }
            }

            handled = true;

            break;
        }

        if (handled)
        {
            continue;
        }

        if (name == "mainMenu")
        {
            if (auto menu = addMenu(x, y, w, h, useAssetOrDefault(pic, "button-clear-red")))
            {
                auto *raw = storeWidget(componentMap, this, name, std::move(menu));

                raw->setTitle("Main Menu");

                juce::Timer::callAfterDelay(30, [w = juce::Component::SafePointer(this)]() {
                    if (w)
                    {
                        w->keyboardFocusMainMenu();
                    }
                });
            }

            continue;
        }

        if (name == "midiLearnButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MidiLearn,
                                 useAssetOrDefault(pic, "button"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->onStateChange = [this, tb]() {
                const bool state = tb->getToggleState();
                paramCoordinator.midiLearnAttachment.set(state);

                if (state)
                {
                    enterMidiLearnMode();
                }
                else
                {
                    exitMidiLearnMode();
                }
            };

            continue;
        }

        if (name == "mpeButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::MPEMode,
                                 useAssetOrDefault(pic, "button"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.getMidiHandler().mpeEnabled.load(),
                               juce::dontSendNotification);

            tb->onClick = [this]() {
                const auto mpeStatus = processor.getSynth().getMotherboard()->mpeEnabled;
                processor.setMpeEnabled(!mpeStatus);
            };

            continue;
        }

        if (name == "lockHQButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::LockHQ,
                                 useAssetOrDefault(pic, "button-lock"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.lockHighQuality.load(), juce::dontSendNotification);

            tb->onClick = [this, tb]() {
                const bool locked = tb->getToggleState();
                auto &ph = processor.getParamCoordinator().getParameterUpdateHandler();
                const auto hq = ph.getParameter(ID::HQMode);

                processor.lockHighQuality.store(locked);

                if (locked)
                {
                    processor.lockedHQ = hq->convertFrom0to1(hq->getValue());
                }
            };

            continue;
        }

        if (name == "lockBendRangeButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::LockPitchBend,
                                 useAssetOrDefault(pic, "button-lock"));
            auto *raw = storeWidget(componentMap, this, name, std::move(btn));
            auto *tb = static_cast<ToggleButton *>(raw);

            tb->setToggleState(processor.lockPitchBend.load(), juce::dontSendNotification);

            tb->onClick = [this, tb]() {
                const bool locked = tb->getToggleState();
                auto &ph = processor.getParamCoordinator().getParameterUpdateHandler();
                const auto pbDown = ph.getParameter(ID::BendDownRange);
                const auto pbUp = ph.getParameter(ID::BendUpRange);

                processor.lockPitchBend.store(locked);

                if (locked)
                {
                    processor.lockedPBDownRange =
                        static_cast<int>(pbDown->convertFrom0to1(pbDown->getValue()));
                    processor.lockedPBUpRange =
                        static_cast<int>(pbUp->convertFrom0to1(pbUp->getValue()));
                }
            };

            continue;
        }

        if (name == "patchNumberMenu")
        {
            auto digits = std::make_unique<DisplayDigits>("menu-patch", h, imageCache);
            auto *raw = storeWidget(componentMap, this, name, std::move(digits));
            auto *dd = static_cast<DisplayDigits *>(raw);

            raw->setBounds(transformBounds(x, y, w, h));

            dd->onClick = [this]() {
                juce::PopupMenu m;
                createPatchList(m);
                m.showMenuAsync(obxf::defaultPopupMenuOptions(this), [this](int i) {
                    if (i)
                        MenuActionCallback(i);
                });
            };

            continue;
        }

        if (name == "prevPatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::PrevPatch,
                                 useAssetOrDefault(pic, "button-clear"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { prevProgram(); };

            continue;
        }

        if (name == "nextPatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::NextPatch,
                                 useAssetOrDefault(pic, "button-clear"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { nextProgram(); };

            continue;
        }

        if (name == "savePatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::SavePatch,
                                 useAssetOrDefault(pic, "button-clear-red"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() {
                if (const auto peer = getPeer())
                {
                    const auto mod = peer->getCurrentModifiersRealtime();
                    MenuActionCallback(mod.isCommandDown() ? MenuAction::QuickSavePatch
                                                           : MenuAction::SavePatch);
                }
                else
                {
                    MenuActionCallback(MenuAction::SavePatch);
                }
            };

            continue;
        }

        if (name == "undoPatchButton")
        {
            auto btn =
                addButton(x, y, w, h, juce::String{}, Name::Undo, useAssetOrDefault(pic, "button"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() {
                processor.getParamCoordinator().getParameterUpdateHandler().undo();
            };

            continue;
        }

        if (name == "initPatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::InitializePatch,
                                 useAssetOrDefault(pic, "button-clear-red"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { MenuActionCallback(MenuAction::InitializePatch); };

            continue;
        }

        if (name == "randomizePatchButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::RandomizePatch,
                                 useAssetOrDefault(pic, "button-clear-white"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onClick = [this]() { mutateCallback(); };

            tb->onRightClick([this]() { showMutatorMenu(); });

            continue;
        }

        if (name == "groupSelectButton")
        {
            auto btn = addButton(x, y, w, h, juce::String{}, Name::PatchGroupSelect,
                                 useAssetOrDefault(pic, "button-alt"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->onStateChange = [this]() { updateSelectButtonStates(); };

            continue;
        }

        if (name.startsWith("select") && name.endsWith("Button"))
        {
            const auto which = name.retainCharacters("0123456789").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx < 0 || whichIdx >= NUM_PATCHES_PER_GROUP)
            {
                continue;
            }

            const juce::String labelName = juce::String(name).replace("Button", "Label");

            if (auto lbl =
                    addLabel(x, y, w, h, h, fmt::format("Select Group/Patch {} Label", which),
                             "button-group-patch"))
            {
                storeWidget(componentMap, this, labelName, std::move(lbl));
            }

            auto btn = addButton(x, y, w, h, juce::String{},
                                 fmt::format("Select Group/Patch {}", which), "");
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->setTriggeredOnMouseDown(true);
            tb->setClickingTogglesState(false);

            tb->onStateChange = [this, whichIdx]() {
                if (auto *b = getWidget<ToggleButton>(fmt::format("select{}Button", whichIdx + 1));
                    b && b->isDown())
                {
                    loadPatchFromProgrammer(whichIdx);
                }
                updateSelectButtonStates();
            };

            continue;
        }

        if (name.startsWith("lfo") && name.endsWith("SelectButton"))
        {
            const auto which = name.retainCharacters("0123456789").getIntValue();
            const auto whichIdx = which - 1;

            if (whichIdx < 0 || whichIdx >= NUM_LFOS)
            {
                continue;
            }

            auto btn = addButton(x, y, w, h, juce::String{}, fmt::format("Select LFO {}", which),
                                 useAssetOrDefault(pic, "button-slim"));
            auto *tb =
                static_cast<ToggleButton *>(storeWidget(componentMap, this, name, std::move(btn)));

            tb->setRadioGroupId(1);

            tb->onClick = [this, whichIdx]() { processor.selectedLFOIndex = whichIdx; };

            // Trigger the initial selection once all LFO buttons have been created.
            // We do this after the loop via a deferred check on the last LFO index.
            if (whichIdx == NUM_LFOS - 1)
            {
                const int sel = processor.selectedLFOIndex;

                if (sel >= 0 && sel < NUM_LFOS)
                {
                    if (auto *selBtn =
                            getWidget<ToggleButton>(fmt::format("lfo{}SelectButton", sel + 1)))
                    {
                        selBtn->setToggleState(true, juce::sendNotification);
                        selBtn->triggerClick();
                    }
                }
            }

            continue;
        }

        if (name == "aboutPage")
        {
            auto btn =
                addButton(x, y, w, h, juce::String{}, "About Page", useAssetOrDefault(pic, ""));
            if (btn)
            {
                auto *tb = static_cast<ToggleButton *>(
                    storeWidget(componentMap, this, name, std::move(btn)));

                tb->setWantsKeyboardFocus(false);
                tb->setAccessible(false);

                tb->onClick = [this]() { aboutScreen->showOver(this); };
            }

            continue;
        }

        if (name.startsWith("savePatch") && saveDialog)
        {
            saveDialog->boundsMap[name.toStdString()] = juce::Rectangle<int>(x, y, w, h);

            if (child->hasAttribute("color"))
            {
                saveDialog->colorMap[name.toStdString()] =
                    juce::Colour(child->getStringAttribute("color").getHexValue32());
            }
        }
    }
}

void ObxfAudioProcessorEditor::cacheLayoutFromXml(const juce::XmlElement *doc)
{
    cachedLayout.clear();

    for (const auto *child : doc->getChildWithTagNameIterator("widget"))
    {
        cachedLayout.push_back({child->getStringAttribute("name"), child->getIntAttribute("x"),
                                child->getIntAttribute("y"), child->getIntAttribute("w"),
                                child->getIntAttribute("h"), child->getIntAttribute("d")});
    }
}

void ObxfAudioProcessorEditor::setupPolyphonyMenu() const
{
    if (auto *m = getWidget<ButtonList>("polyphonyMenu"))
    {
        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(2);

        if (const auto *p = paramCoordinator.getParameter(ID::Polyphony))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupUnisonVoicesMenu() const
{
    if (auto *m = getWidget<ButtonList>("unisonVoicesMenu"))
    {
        for (int i = 1; i <= MAX_VOICES; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(2);

        if (const auto *p = paramCoordinator.getParameter(ID::UnisonVoices))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupEnvLegatoModeMenu() const
{
    using namespace sst::plugininfra::misc_platform;

    if (auto *m = getWidget<ButtonList>("envLegatoModeMenu"))
    {
        m->addChoice(toOSCase("Both Envelopes"));
        m->addChoice(toOSCase("Filter Envelope Only"));
        m->addChoice(toOSCase("Amplifier Envelope Only"));
        m->addChoice(toOSCase("Always Retrigger"));

        if (const auto *p = paramCoordinator.getParameter(ID::EnvLegatoMode))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupNotePriorityMenu() const
{
    if (auto *m = getWidget<ButtonList>("notePriorityMenu"))
    {
        m->addChoice("Last");
        m->addChoice("Low");
        m->addChoice("High");

        if (const auto *p = paramCoordinator.getParameter(ID::NotePriority))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupBendUpRangeMenu() const
{
    if (auto *m = getWidget<ButtonList>("bendUpRangeMenu"))
    {
        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(4);

        if (const auto *p = paramCoordinator.getParameter(ID::BendUpRange))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupBendDownRangeMenu() const
{
    if (auto *m = getWidget<ButtonList>("bendDownRangeMenu"))
    {
        for (int i = 0; i <= MAX_BEND_RANGE; ++i)
        {
            m->addChoice(juce::String(i));
        }

        m->setNumColumns(4);

        if (const auto *p = paramCoordinator.getParameter(ID::BendDownRange))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupFilterXpanderModeMenu() const
{
    if (auto *m = getWidget<ButtonList>("filterXpanderModeMenu"))
    {
        for (const auto *choice : {"LP4", "LP3", "LP2", "LP1", "HP3", "HP2", "HP1", "BP4", "BP2",
                                   "N2", "PH3", "HP2+LP1", "HP3+LP1", "N2+LP1", "PH3+LP1"})
        {
            m->addChoice(choice);
        }

        if (const auto *p = paramCoordinator.getParameter(ID::FilterXpanderMode))
        {
            m->setScrollWheelEnabled(true);
            m->setValue(p->getValue(), juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::setupMenus()
{
    setupPolyphonyMenu();
    setupUnisonVoicesMenu();
    setupEnvLegatoModeMenu();
    setupNotePriorityMenu();
    setupBendUpRangeMenu();
    setupBendDownRangeMenu();
    setupFilterXpanderModeMenu();
    createMenu();
}

void ObxfAudioProcessorEditor::restoreComponentParameterValues(
    const std::map<juce::String, float> &parameterValues)
{
    for (auto &[name, val] : parameterValues)
    {
        auto it = componentMap.find(name);

        if (it == componentMap.end() || !it->second)
        {
            continue;
        }

        auto *comp = it->second.get();

        if (auto *k = dynamic_cast<Knob *>(comp))
        {
            k->setValue(val, juce::dontSendNotification);
        }
        else if (auto *ms = dynamic_cast<MultiStateButton *>(comp))
        {
            ms->setValue(val, juce::dontSendNotification);
        }
        else if (auto *b = dynamic_cast<ToggleButton *>(comp))
        {
            b->setToggleState(val > 0.5f, juce::dontSendNotification);
        }
    }
}

void ObxfAudioProcessorEditor::finalizeThemeLoad(ObxfAudioProcessor &ownerFilter)
{
    ownerFilter.addChangeListener(this);
    skinLoaded = true;
    scaleFactorChanged();
    repaint();
}

void ObxfAudioProcessorEditor::keyboardFocusMainMenu()
{
    if (auto *mm = getWidget<ImageMenu>("mainMenu"))
    {
        if (isShowing() && mm->isShowing())
        {
            mm->setWantsKeyboardFocus(true);
            juce::Timer::callAfterDelay(50, [w = juce::Component::SafePointer(mm)]() {
                if (w)
                    w->grabKeyboardFocus();
            });
        }
        else
        {
            juce::Timer::callAfterDelay(30, [w = juce::Component::SafePointer(this)]() {
                if (w)
                    w->keyboardFocusMainMenu();
            });
        }
    }
}

ObxfAudioProcessorEditor::~ObxfAudioProcessorEditor()
{
#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    if (inspector)
    {
        inspector.reset();
    }
#endif

    focusDebugger.reset();

    processor.uiState.editorAttached = false;
    idleTimer->stopTimer();
    processor.removeChangeListener(this);
    setLookAndFeel(nullptr);

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    knobAttachments.clear();
    buttonListAttachments.clear();
    toggleAttachments.clear();
    multiStateAttachments.clear();
    popupMenus.clear();
    componentMap.clear();

    juce::PopupMenu::dismissAllActiveMenus();
}

void ObxfAudioProcessorEditor::scaleFactorChanged()
{
    if (imageCache.isSVG("background"))
    {
        backgroundIsSVG = true;
    }
    else
    {
        backgroundIsSVG = false;
        backgroundImage = imageCache.getImageFor("background", getWidth(), getHeight());
    }
    repaint();
}

std::unique_ptr<Label> ObxfAudioProcessorEditor::addLabel(const int x, const int y, const int w,
                                                          const int h, int fh,
                                                          const juce::String &name,
                                                          const juce::String &assetName)
{
    if (fh == 0 && h > 0)
    {
        fh = h;
    }

    auto *label = new Label(assetName, fh, imageCache);

    label->setBounds(transformBounds(x, y, w, h));
    label->setName(name);

    addAndMakeVisible(label);

    return std::unique_ptr<Label>(label);
}

std::unique_ptr<Knob> ObxfAudioProcessorEditor::addKnob(int x, int y, int w, int h, int d, int fh,
                                                        const juce::String &paramId, float defval,
                                                        const juce::String &name,
                                                        const juce::String &assetName)
{
    std::unique_ptr<Knob> knob;

    if (fh == 0)
    {
        int frameHeight = defKnobDiameter;

        if (d > 0)
            frameHeight = d;
        else if (fh > 0)
            frameHeight = fh;

        knob = std::make_unique<Knob>(assetName, frameHeight, &processor, imageCache);
    }
    else
    {
        knob = std::make_unique<Knob>(assetName, fh, &processor, imageCache);
        if (w > h)
            knob->svgTranslationMode = Knob::HORIZONTAL;
        else
            knob->svgTranslationMode = Knob::VERTICAL;
    }

    if (!paramId.isEmpty())
    {
        if (auto *param = paramCoordinator.getParameter(paramId); param != nullptr)
        {
            knob->setParameter(param);
            knob->setValue(param->getValue(), juce::dontSendNotification);
            knobAttachments.emplace_back(
                new KnobAttachment(paramCoordinator.getParameterUpdateHandler(), param, *knob));
        }

        knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);

        if (d > 0)
        {
            knob->setBounds(transformBounds(x, y, d, d));
            if (w > 0 && h > 0 && w > h)
            {
                knob->setSliderStyle(juce::Slider::RotaryHorizontalDrag);
            }
        }
        else if (w > 0 && h > 0)
        {
            knob->setBounds(transformBounds(x, y, w, h));

            if (w > h)
            {
                knob->setSliderStyle(juce::Slider::RotaryHorizontalDrag);
            }
        }
        else
        {
            knob->setBounds(transformBounds(x, y, defKnobDiameter, defKnobDiameter));
        }

        knob->setTextBoxStyle(Knob::NoTextBox, true, 0, 0);
        knob->setRange(0, 1);
        knob->setTextBoxIsEditable(false);
        knob->setDoubleClickReturnValue(true, defval, juce::ModifierKeys::noModifiers);
        knob->setTitle(name);
    }

    addAndMakeVisible(*knob);
    return knob;
}

std::unique_ptr<ToggleButton> ObxfAudioProcessorEditor::addButton(const int x, const int y,
                                                                  const int w, const int h,
                                                                  const juce::String &paramId,
                                                                  const juce::String &name,
                                                                  const juce::String &assetName)
{
    auto *button = new ToggleButton(assetName, h, imageCache, &processor);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramCoordinator.getParameter(paramId))
        {
            button->setParameter(param);
            button->setToggleState(param->getValue() > 0.5f, juce::dontSendNotification);
            toggleAttachments.emplace_back(new ButtonAttachment(
                paramCoordinator.getParameterUpdateHandler(), param, *button,
                [](ToggleButton &b, float v) {
                    b.setToggleState(v > 0.5f, juce::dontSendNotification);
                },
                [](const ToggleButton &b) { return b.getToggleState() ? 1.0f : 0.0f; }));
        }
    }
    else
    {
        button->addListener(this);
        button->setToggleState(false, juce::dontSendNotification);
    }

    button->setBounds(transformBounds(x, y, w, h));
    button->setButtonText(name);
    button->setTitle(name);
    button->setTriggeredOnMouseDown(true);

    addAndMakeVisible(button);

    return std::unique_ptr<ToggleButton>(button);
}

std::unique_ptr<MultiStateButton> ObxfAudioProcessorEditor::addMultiStateButton(
    const int x, const int y, const int w, const int h, const juce::String &paramId,
    const juce::String &name, const juce::String &assetName, const uint8_t numStates)
{
    auto *button = new MultiStateButton(assetName, imageCache, &processor, numStates);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramCoordinator.getParameter(paramId); param != nullptr)
        {
            button->setOptionalParameter(param);
            button->setValue(param->getValue(), juce::dontSendNotification);
            multiStateAttachments.emplace_back(new MultiStateAttachment(
                paramCoordinator.getParameterUpdateHandler(), param, *button));
        }

        button->setBounds(transformBounds(x, y, w, h));
        button->setTitle(name);
    }

    addAndMakeVisible(button);

    return std::unique_ptr<MultiStateButton>(button);
}

std::unique_ptr<ButtonList> ObxfAudioProcessorEditor::addList(const int x, const int y, const int w,
                                                              const int h,
                                                              const juce::String &paramId,
                                                              const juce::String &name,
                                                              const juce::String &assetName)
{
    auto *list = new ButtonList(assetName, h, imageCache, &processor);

    if (!paramId.isEmpty())
    {
        if (auto *param = paramCoordinator.getParameter(paramId); param != nullptr)
        {
            list->setParameter(param);
            list->setValue(param->getValue(), juce::dontSendNotification);

            buttonListAttachments.emplace_back(new ButtonListAttachment(
                paramCoordinator.getParameterUpdateHandler(), param, *list));
        }
    }

    list->setBounds(transformBounds(x, y, w, h));
    list->setName(name);
    list->setTitle(name);

    addAndMakeVisible(list);

    return std::unique_ptr<ButtonList>(list);
}

std::unique_ptr<ImageMenu> ObxfAudioProcessorEditor::addMenu(const int x, const int y, const int w,
                                                             const int h,
                                                             const juce::String &assetName)
{
    auto *menu = new ImageMenu(assetName, imageCache);

    menu->setBounds(transformBounds(x, y, w, h));
    menu->setName("Menu");

    auto safeThis = SafePointer(this);
    menu->onClick = [safeThis]() {
        if (!safeThis)
        {
            return;
        }

        if (auto *mm = safeThis->getWidget<ImageMenu>("mainMenu"))
        {
            auto x = mm->getScreenX();
            auto y = mm->getScreenY();
            auto dx = mm->getWidth();
            auto pos = juce::Point<int>(x, y + dx);
            safeThis->resultFromMenu(pos);
        }
    };

    addAndMakeVisible(menu);

    return std::unique_ptr<ImageMenu>(menu);
}

void ObxfAudioProcessorEditor::actionListenerCallback(const juce::String & /*message*/) {}

juce::Rectangle<int> ObxfAudioProcessorEditor::transformBounds(int x, int y, int w, int h) const
{
    if (initialBounds.isEmpty())
        return {x, y, w, h};

    auto aspectRatio = 1.f * initialBounds.getWidth() / initialBounds.getHeight();
    auto currentAspectRatio = 1.f * getWidth() / getHeight();
    float scale{1.f};

    if (currentAspectRatio > aspectRatio)
    {
        // this means the width is too big
        scale = static_cast<float>(getHeight()) / static_cast<float>(initialBounds.getHeight());
    }
    else
    {
        // this means the height is too big
        scale = static_cast<float>(getWidth()) / static_cast<float>(initialBounds.getWidth());
    }

    return {juce::roundToInt(static_cast<float>(x) * scale),
            juce::roundToInt(static_cast<float>(y) * scale),
            juce::roundToInt(static_cast<float>(w) * scale),
            juce::roundToInt(static_cast<float>(h) * scale)};
}

juce::String ObxfAudioProcessorEditor::useAssetOrDefault(const juce::String &assetName,
                                                         const juce::String &defaultAssetName) const
{
    if (assetName.isNotEmpty())
        return assetName;
    else
        return defaultAssetName;
}

void ObxfAudioProcessorEditor::clean()
{
    this->removeAllChildren();

    if (aboutScreen)
    {
        addChildComponent(*aboutScreen);
    }
    if (saveDialog)
    {
        addChildComponent(*saveDialog);
    }
    if (mpeMatrixEditor)
    {
        addChildComponent(*mpeMatrixEditor);
    }
}

void ObxfAudioProcessorEditor::rebuildComponents(ObxfAudioProcessor &ownerFilter)
{
    themeLocation = utils.getCurrentThemeLocation();

    ownerFilter.removeChangeListener(this);

    setSize(1440, 450);

    ownerFilter.addChangeListener(this);
    repaint();
}

juce::PopupMenu ObxfAudioProcessorEditor::createPatchList(juce::PopupMenu &menu) const
{
    auto lsi = processor.lastLoadedProgram;

    auto raddTo = [that = this, lsi](juce::PopupMenu &m, const Utils::PatchTreeNode::ptr_t &node,
                                     auto &&self) -> void {
        for (auto &child : node->children)
        {
            auto checked = lsi >= child->childRange.first && lsi <= child->childRange.second;
            if (child->isFolder)
            {
                if (!child->children.empty())
                {
                    juce::PopupMenu subMenu;
                    self(subMenu, child, self);
                    m.addSubMenu(child->displayName, subMenu, true, juce::Image(), checked);
                }
            }
            else
            {
                m.addItem(child->displayName, true, checked,
                          [ch = std::weak_ptr(child), w = that]() {
                              if (auto sp = ch.lock())
                              {
                                  w->utils.loadPatch(sp);
                              }
                          });
            }
        }
    };

    for (auto i = 0U; i < utils.patchRoot->children.size(); i++)
    {
        auto &ch = utils.patchRoot->children[i];

        if (!ch->children.empty())
        {
            menu.addSectionHeader(Utils::toString(ch->locationType));
            menu.addSeparator();

            raddTo(menu, ch, raddTo);

            if (i < utils.patchRoot->children.size() - 1)
            {
                menu.addColumnBreak();
            }
        }
    }

    using namespace sst::plugininfra::misc_platform;

    menu.addColumnBreak();
    menu.addSectionHeader("Functions");
    menu.addSeparator();

    bool enablePasteOption = utils.isPatchInClipboard();

    menu.addItem(static_cast<int>(MenuAction::InitializePatch), toOSCase("Initialize Patch"), true,
                 false);

    menu.addSeparator();

    menu.addItem(MenuAction::LoadPatch,
#if JUCE_IOS
                 toOSCase("Import Patch..."),
#else
                 toOSCase("Load Patch..."),
#endif
                 true, false);
    menu.addItem(MenuAction::SavePatch, toOSCase("Save Patch..."), true, false);

    menu.addSeparator();

    menu.addItem(MenuAction::DeletePatch, toOSCase("Delete Patch"), true, false);

    menu.addSeparator();

    menu.addItem(MenuAction::CopyPatch, toOSCase("Copy Patch"), true, false);
    menu.addItem(MenuAction::PastePatch, toOSCase("Paste Patch"), enablePasteOption, false);

    menu.addSeparator();

    /*     menu.addItem(MenuAction::SetDefaultPatch, toOSCase("Set Current Patch as Default"), true,
                     false); */

    menu.addItem(MenuAction::RefreshBrowser, toOSCase("Refresh Patch Browser"), true, false);

    return menu;
}

void ObxfAudioProcessorEditor::createMenu()
{
    using namespace sst::plugininfra::misc_platform;

    popupMenus.clear();
    auto *menu = new juce::PopupMenu();
    juce::PopupMenu midiMenu;
    themes = utils.getThemeLocations();

    createMidiMapMenu(static_cast<int>(midiStart), midiMenu);

    menu->addSubMenu(toOSCase("MIDI Mapping"), midiMenu);

    {
        juce::PopupMenu mpeMenu;
        auto &midiHandler = processor.getMidiHandler();

        bool mpeOn = midiHandler.mpeEnabled.load();
        mpeMenu.addItem(toOSCase("MPE Enabled"), true, mpeOn,
                        [w = juce::Component::SafePointer(this), mpeOn]() {
                            if (!w)
                                return;

                            w->processor.setMpeEnabled(!mpeOn);

                            if (auto *mb = w->getWidget<ToggleButton>("mpeButton"))
                            {
                                mb->setToggleState(!mpeOn, juce::dontSendNotification);
                            }
                        });

        mpeMenu.addSeparator();
        mpeMenu.addSectionHeader(toOSCase("Glide Range"));

        int curPBR = midiHandler.mpePitchBendRange.load();
        for (int pbr : {2, 12, 24, 48})
        {
            mpeMenu.addItem(fmt::format("{} semitones", pbr), true, curPBR == pbr,
                            [w = juce::Component::SafePointer(this), pbr]() {
                                if (!w)
                                    return;
                                w->processor.setMpePitchBendRange(pbr);
                            });
        }

        mpeMenu.addSeparator();
        mpeMenu.addItem(toOSCase("MPE Assignments..."), [w = juce::Component::SafePointer(this)]() {
            if (!w || !w->mpeMatrixEditor)
                return;
            w->mpeMatrixEditor->refresh();
            w->mpeMatrixEditor->setVisible(true);
            w->mpeMatrixEditor->toFront(true);
        });

        juce::PopupMenu mpePresetMenu;

        menu->addSubMenu(toOSCase("MPE"), mpeMenu);
    }

    {
        juce::PopupMenu themeMenu;
        auto ll = Utils::LocationType::SYSTEM_FACTORY;

        if (themes.size() && themes[0].locationType != Utils::LocationType::USER)
        {
            themeMenu.addSectionHeader("Factory");
        }

        for (size_t i = 0; i < themes.size(); ++i)
        {
            auto theme = themes[i];

            if (theme.locationType != ll && theme.locationType == Utils::LocationType::USER)
            {
                if (i != 0)
                    themeMenu.addSeparator();
                themeMenu.addSectionHeader("User");
            }

            ll = theme.locationType;

            themeMenu.addItem(static_cast<int>(i + themeStart + 1), theme.file.getFileName(), true,
                              theme == themeLocation);
        }

#if JUCE_WINDOWS
        themeMenu.addSeparator();
        auto usw = utils.getUseSoftwareRenderer();
        themeMenu.addItem(toOSCase("Use Software Renderer"), true, usw,
                          [w = juce::Component::SafePointer(this)]() {
                              if (!w)
                                  return;
                              auto usw = w->utils.getUseSoftwareRenderer();
                              w->utils.setUseSoftwareRenderer(!usw);
                              juce::AlertWindow::showMessageBoxAsync(
                                  juce::AlertWindow::WarningIcon, "Software Renderer Change",
                                  "A software renderer change is only active "
                                  "once you restart/reload the plugin.");
                          });
#endif

        menu->addSubMenu("Themes", themeMenu);
    }

    {
        juce::PopupMenu sizeMenu;

        bool isCurZoomAmongScaleFactors = false;
        auto curZoom = impliedScaleFactor();
        auto pd = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

        juce::Rectangle<float> dispArea{0, 0, 100000, 100000};
        if (pd)
        {
            // when would this code not have a primary display?
            // when the juce LV2 lets-print-our-state-at-build-time
            // nonsense runs ahd hits load theme because it makes an
            // editor without an xvfb. And then crashes our pipelines.
            dispArea = pd->userArea.toFloat();
        }

        for (int i = 0; i < numScaleFactors; i++)
        {
            auto scaledArea = initialBounds.toFloat().transformedBy(
                juce::AffineTransform().scaled(scaleFactors[i]));

            bool isActive = dispArea.getWidth() >= scaledArea.getWidth() &&
                            dispArea.getHeight() >= scaledArea.getHeight();

            bool isTicked = std::fabs(scaleFactors[i] - curZoom) * 100 < 0.5f;

            if (isTicked && !isCurZoomAmongScaleFactors)
            {
                isCurZoomAmongScaleFactors = true;
            }

            sizeMenu.addItem(static_cast<int>(sizeStart) + i,
                             fmt::format("{:.0f}%", scaleFactors[i] * 100.f), isActive, isTicked);
        }

        sizeMenu.addSeparator();

        if (!isCurZoomAmongScaleFactors)
        {
            sizeMenu.addItem(
                -1, toOSCase(fmt::format("Custom Zoom Level: {:.{}f}%", curZoom * 100.f, 1)), false,
                true);
            sizeMenu.addSeparator();
        }

        // used to be an if - change to show it non-enabled
        // if (utils.getDefaultZoomFactor() != curZoom)
        {
            auto disp = (utils.getDefaultZoomFactor() != curZoom);
            auto dispZoom = curZoom;

            if (isCurZoomAmongScaleFactors)
            {
                dispZoom = std::round(curZoom * 100.f) / 100.f;
            }

            sizeMenu.addItem(toOSCase(fmt::format("Zoom to Default Level ({:.{}f}%)",
                                                  utils.getDefaultZoomFactor() * 100.f, 0)),
                             disp, false, [this]() {
                                 auto zf = utils.getDefaultZoomFactor();
                                 const int newWidth =
                                     juce::roundToInt(static_cast<float>(initialWidth) * zf);
                                 const int newHeight =
                                     juce::roundToInt(static_cast<float>(initialHeight) * zf);

                                 setSize(newWidth, newHeight);
                                 resized();
                             });

            sizeMenu.addSeparator();

            sizeMenu.addItem(
                toOSCase(fmt::format("Set {:.{}f}% as Default Zoom Level", dispZoom * 100.f,
                                     isCurZoomAmongScaleFactors ? 0 : 1)),
                disp, false, [this, curZoom]() { utils.setDefaultZoomFactor(curZoom); });

            sizeMenu.addSeparator();

            juce::PopupMenu menuZoomMenu;
            auto ms = utils.getMenuScaleMode();
            menuZoomMenu.addItem("Disabled", true, ms == Utils::MenuScaleMode::DONT,
                                 [w = juce::Component::SafePointer(this)]() {
                                     if (w)
                                         w->utils.setMenuScaleMode(Utils::MenuScaleMode::DONT);
                                 });

            const auto plugin =
                (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                    ? ""
                    : " (Plugin)";
            menuZoomMenu.addItem(
                fmt::format("Enabled{}", plugin), true, ms == Utils::MenuScaleMode::WITH_PLUGIN,
                [w = juce::Component::SafePointer(this)]() {
                    if (w)
                        w->utils.setMenuScaleMode(Utils::MenuScaleMode::WITH_PLUGIN);
                });
            sizeMenu.addSubMenu(toOSCase("Menu Scaling"), menuZoomMenu);

#ifndef JUCE_MAC
            const auto host =
                (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                    ? "Operating System"
                    : "Host";
            menuZoomMenu.addItem(fmt::format("Enabled ({})", host), true,
                                 ms == Utils::MenuScaleMode::WITH_OS,
                                 [w = juce::Component::SafePointer(this)]() {
                                     if (w)
                                         w->utils.setMenuScaleMode(Utils::MenuScaleMode::WITH_OS);
                                 });
#endif
        }

        menu->addSubMenu("Zoom", sizeMenu);
    }

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    juce::PopupMenu debugMenu;

    debugMenu.addSeparator();

    const bool isInspectorVisible = inspector && inspector->isVisible();

    debugMenu.addItem(MenuAction::Inspector, toOSCase("GUI Inspector..."), true,
                      isInspectorVisible);
    debugMenu.addItem(toOSCase("Toggle Focus Debugger"),
                      [w = juce::Component::SafePointer(this)]() {
                          if (w)
                              w->focusDebugger->setDoFocusDebug(!w->focusDebugger->doFocusDebug);
                      });

    menu->addSubMenu("Developer", debugMenu);
#endif

    menu->addSeparator();
#if !JUCE_IOS
    menu->addItem(MenuAction::RevealUserDirectory, toOSCase("Open User Data Folder..."), true,
                  false);
#endif
    menu->addItem(toOSCase("Open Manual..."), []() {
        juce::URL("https://surge-synth-team.org/ob-xf/manual/getting-started/")
            .launchInDefaultBrowser();
    });

    menu->addSeparator();

    menu->addItem("About OB-Xf", [w = SafePointer(this)]() {
        if (!w)
            return;
        w->aboutScreen->showOver(w.getComponent());
    });

    popupMenus.emplace_back(menu);
}

void ObxfAudioProcessorEditor::createMidiMapMenu(int menuNo, juce::PopupMenu &menuMidi)
{
    using namespace sst::plugininfra::misc_platform;

    const juce::File midi_dir = utils.getMidiFolderFor(Utils::LocationType::USER);

    menuMidi.addItem(toOSCase("Clear MIDI Mapping"), true, false,
                     [this]() { processor.getMidiMap().reset(); });

    menuMidi.addItem(toOSCase("Save MIDI Mapping..."), true, false, [this, midi_dir]() {
        fileChooser =
            std::make_unique<juce::FileChooser>("Save MIDI Mapping", midi_dir, "*.xml", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                {
                    processor.getMidiHandler().saveBindingsTo(result);
                }
            });
    });

    menuMidi.addSeparator();

    juce::StringArray list;

    for (const auto &entry : juce::RangedDirectoryIterator(midi_dir, true, "*.xml"))
    {
        list.add(entry.getFile().getFullPathName());
    }

    list.sort(true);

    for (const auto &i : list)
    {
        juce::File f(i);
        auto name = f.getFileNameWithoutExtension();
        bool isCurrent = (processor.getCurrentMidiPath() == f.getFullPathName());

        menuMidi.addItem(menuNo++, name, true, isCurrent);
        midiFiles.emplace_back(
            Utils::MidiLocation{Utils::LocationType::USER, midi_dir.getFileName(), f});
    }
}

void ObxfAudioProcessorEditor::resultFromMenu(const juce::Point<int> pos)
{
    createMenu();

    popupMenus[0]->showMenuAsync(obxf::defaultPopupMenuOptions(this), [this](size_t result) {
        if (result >= (themeStart + 1) && result <= (themeStart + themes.size()))
        {
            result -= 1;
            result -= themeStart;

            utils.setCurrentThemeLocation(themes[result]);
            themeLocation = themes[result];
            clean();
            loadTheme(processor);
        }
        else if (result >= sizeStart && result < (sizeStart + numScaleFactors))
        {
            size_t index = result - sizeStart;
            const int newWidth =
                juce::roundToInt(static_cast<float>(initialWidth) * scaleFactors[index]);
            const int newHeight =
                juce::roundToInt(static_cast<float>(initialHeight) * scaleFactors[index]);

            setSize(newWidth, newHeight);
            resized();
        }
        else if (result < midiStart)
        {
            MenuActionCallback(static_cast<int>(result));
        }
        else if (result >= midiStart)
        {
            if (const auto selected_idx = result - midiStart; selected_idx < midiFiles.size())
            {
                const auto &midiLoc = midiFiles[selected_idx];

                if (juce::File f = midiLoc.file; f.exists())
                {
                    processor.getCurrentMidiPath() = f.getFullPathName();
                    processor.getMidiMap().loadFile(f);
                }
            }
        }
    });
}

void ObxfAudioProcessorEditor::MenuActionCallback(int action)
{
    if (action == MenuAction::InitializePatch)
    {
        utils.initializePatch();
        processor.processActiveProgramChanged();
    }

    if (action == MenuAction::LoadPatch)
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Load Patch", juce::File(), "*.fxp", true);

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser &chooser) {
                if (const juce::File result = chooser.getResult(); result != juce::File())
                {
                    utils.loadPatch(result);
                }
            });
    }

    if (action == MenuAction::SavePatch)
    {
        saveDialog->showOver(this);
    }

    if (action == MenuAction::QuickSavePatch)
    {
        saveDialog->doQuickSave();
    }

    if (action == MenuAction::DeletePatch)
    {
        auto llp = processor.lastLoadedPatchNode.lock()->index;

        if (llp >= utils.lastFactoryPatch)
        {
            auto &curPatch = utils.patchesAsLinearList[llp];
            const auto success = curPatch->file.deleteFile();

            if (success)
            {
                utils.rescanPatchTree();

                llp -= 1;

                utils.loadPatch(utils.patchesAsLinearList[llp]);
            }
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Cannot Delete Patch",
                "Factory patches, their unsaved modifications and patches dropped from outside "
                "cannot be deleted!");
        }
    }

    if (action == MenuAction::CopyPatch)
    {
        utils.copyPatch();
    }

    if (action == MenuAction::PastePatch)
    {
        utils.pastePatch();
    }

    if (action == MenuAction::SetDefaultPatch)
    {
        utils.setDefaultPatch(processor.getActiveProgram().getName());
    }

    if (action == MenuAction::RefreshBrowser)
    {
        utils.rescanPatchTree();

        const auto pn = processor.getActiveProgram().getName().toStdString();

        processor.resetLastLoadedProgramByName(pn);
    }

    if (action == MenuAction::RevealUserDirectory)
    {
#if !JUCE_IOS
        utils.getDocumentFolder().revealToUser();
#endif
    }

#if (defined(DEBUG) || defined(_DEBUG)) && !JUCE_IOS
    // Open Melatonin inspector
    if (action == MenuAction::Inspector)
    {
        this->inspector->setVisible(!this->inspector->isVisible());
        this->inspector->toggle(this->inspector->isVisible());
    }
#endif
}

void ObxfAudioProcessorEditor::nextProgram()
{
    auto llp = processor.lastLoadedProgram;
    auto nlp = llp + 1;
    if ((size_t)nlp >= utils.patchesAsLinearList.size())
    {
        nlp = -1;
    }
    if (nlp < 0)
    {
        utils.initializePatch();
        processor.processActiveProgramChanged();
    }
    else
    {
        utils.loadPatch(utils.patchesAsLinearList[nlp]);
    }
}

void ObxfAudioProcessorEditor::prevProgram()
{
    auto llp = processor.lastLoadedProgram;
    auto nlp = llp - 1;
    if (nlp < -1)
    {
        nlp = utils.patchesAsLinearList.size() - 1;
    }
    if (nlp == -1)
    {
        utils.initializePatch();
        processor.processActiveProgramChanged();
    }
    else
    {
        utils.loadPatch(utils.patchesAsLinearList[nlp]);
    }
}

void ObxfAudioProcessorEditor::updateFromHost()
{
    for (const auto &knobAttachment : knobAttachments)
    {
        knobAttachment->updateToControl();
    }
    for (const auto &buttonListAttachment : buttonListAttachments)
    {
        buttonListAttachment->updateToControl();
    }
    for (const auto &toggleAttachment : toggleAttachments)
    {
        toggleAttachment->updateToControl();
    }
    for (const auto &multiStateAttachment : multiStateAttachments)
    {
        multiStateAttachment->updateToControl();
    }

    updatePatchNumberIfNeeded();
    updateSelectButtonStates();

    if (auto *b = getWidget<ToggleButton>("lockBendRangeButton"))
    {
        b->setToggleState(processor.lockPitchBend.load(), juce::dontSendNotification);
    }

    if (auto *b = getWidget<ToggleButton>("lockHQButton"))
    {
        b->setToggleState(processor.lockHighQuality.load(), juce::dontSendNotification);
    }

    auto &midiHandler = processor.getMidiHandler();

    if (auto *b = getWidget<ToggleButton>("mpeButton"))
    {
        b->setToggleState(midiHandler.mpeEnabled.load(), juce::dontSendNotification);
    }

    repaint();
}

void ObxfAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster * /*source*/)
{
    updateFromHost();
}

void ObxfAudioProcessorEditor::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
    {
        resultFromMenu(e.getMouseDownScreenPosition());
    }
}

void ObxfAudioProcessorEditor::handleAsyncUpdate()
{
    scaleFactorChanged();
    repaint();
}

void ObxfAudioProcessorEditor::paintMissingAssets(juce::Graphics &g)
{
    g.fillAll(juce::Colour(30, 30, 50));
    auto h{20}, p{4};
    g.setFont(juce::FontOptions(h));
    g.setColour(juce::Colours::white);
    auto b = getLocalBounds().reduced(5).withHeight(h);

    auto write = [&](auto m) {
        g.drawText(m, b, juce::Justification::centred, true);
        b = b.translated(0, h + p);
    };

    write("Missing Assets");
    write("Please check your installation.");
    write("");
    write("Asset Directory List is");
    g.setFont(juce::FontOptions(15));
    write(utils.getSystemFactoryFolder().getFullPathName());
    write(utils.getLocalFactoryFolder().getFullPathName());
    write(utils.getDocumentFolder().getFullPathName());
    g.setFont(juce::FontOptions(h));
    write("");
    write("Current Theme Directory is");
    g.setFont(juce::FontOptions(15));
    write(themeLocation.file.getFullPathName());
    g.setFont(juce::FontOptions(h));
    write("");
    write("You can get the assets from our GitHub repo to place there.");
    write("If you run the macOS/Windows installer you shouldn't see this any more. Linux soon!");
}

void ObxfAudioProcessorEditor::paint(juce::Graphics &g)
{
    if (noThemesAvailable)
    {
        paintMissingAssets(g);
        return;
    }

    const float newPhysicalPixelScaleFactor = g.getInternalContext().getPhysicalPixelScaleFactor();

    if (newPhysicalPixelScaleFactor != utils.getPixelScaleFactor())
    {
        utils.setPixelScaleFactor(newPhysicalPixelScaleFactor);
        scaleFactorChanged();
    }

    g.fillAll(juce::Colours::black.brighter(0.1f));

    // background gui
    if (backgroundIsSVG)
    {
        const auto &bgi = imageCache.getSVGDrawable("background");
        if (bgi)
        {
            auto aspectRatio = 1.f * initialBounds.getWidth() / initialBounds.getHeight();
            auto currentAspectRatio = 1.f * getWidth() / getHeight();
            float scale{1.f};

            if (currentAspectRatio > aspectRatio)
            {
                // this means the width is too big
                scale =
                    static_cast<float>(getHeight()) / static_cast<float>(initialBounds.getHeight());
            }
            else
            {
                // this means the height is too big
                scale =
                    static_cast<float>(getWidth()) / static_cast<float>(initialBounds.getWidth());
            }
            auto tf = juce::AffineTransform().scaled(scale);
            bgi->draw(g, 1.f, tf);
        }
        else
        {
            OBLOG(themes, "Background SVG is not present");
            g.fillAll(juce::Colour(50, 0, 0));
        }
    }
    else
    {
        auto aspectRatio = 1.f * initialBounds.getWidth() / initialBounds.getHeight();
        auto currentAspectRatio = 1.f * getWidth() / getHeight();

        auto w = getWidth();
        auto h = getHeight();
        if (currentAspectRatio > aspectRatio)
        {
            // this means the width is too big
            w = getHeight() * aspectRatio;
        }
        else
        {
            // this means the height is too big
            h = getWidth() / aspectRatio;
        }
        g.drawImage(backgroundImage, 0, 0, w, h, 0, 0, backgroundImage.getWidth(),
                    backgroundImage.getHeight());
    }
}

#if !JUCE_IOS
bool ObxfAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    if (files.size() == 1)
    {
        const auto file = juce::File(files[0]);
        const juce::String ext = file.getFileExtension().toLowerCase();
        return file.existsAsFile() && (ext == ".fxp" || ext == ".fxb");
    }

    return std::any_of(files.begin(), files.end(), [](const juce::String &q) {
        const juce::File file(q);
        const juce::String ext = file.getFileExtension().toLowerCase();
        return ext == ".fxb" || ext == ".fxp";
    });
}

void ObxfAudioProcessorEditor::filesDropped(const juce::StringArray &files, int /*x*/, int /*y*/)
{
    // we will take only the first file from the drop
    if (files.size() > 0)
    {
        const auto file = juce::File(files[0]);

        if (const juce::String ext = file.getFileExtension().toLowerCase(); ext == ".fxp")
        {
            utils.loadPatch(file);
        }
    }
}
#endif

float ObxfAudioProcessorEditor::impliedScaleFactor() const
{
    if (initialHeight == 0 || initialWidth == 0)
        return 1.f;

    auto xf = getWidth() / static_cast<float>(initialWidth);
    auto yf = getHeight() / static_cast<float>(initialHeight);

    return std::max(xf, yf);
}

float ObxfAudioProcessorEditor::menuScaleFactor() const
{
    auto ms = utils.getMenuScaleMode();
    switch (ms)
    {
    case Utils::DONT:
        return 1.f;
    case Utils::WITH_OS:
        return utils.getPluginAPIScale();
    case Utils::WITH_PLUGIN:
    {
        auto psf = std::max(1.f, utils.getPluginAPIScale());
        auto rs = impliedScaleFactor() / psf;
        if (rs <= 1)
            return rs;

        return std::sqrt(rs); // slow it down a bit
    }
    }
    return 1.f;
}

void ObxfAudioProcessorEditor::setScaleFactor(float newScale)
{
    if (ignoreHostScale)
        newScale = 1.f;
    utils.setPluginAPIScale(newScale);
    // this line causes the crash with bitmap assets we've been chasing
    // Why? We kinda need it I think...
    // AudioProcessorEditor::setScaleFactor(newScale);
}

void ObxfAudioProcessorEditor::showMutatorMenu()
{
    juce::PopupMenu m;

    m.addSectionHeader("Patch Mutator");
    m.addSeparator();

    auto custom = std::make_unique<MutatorMenu>(
        processor.mutateSections,
        [this](const int index, bool value) { this->processor.mutateSections[index] = value; });

    m.addCustomItem(1, std::move(custom));

    m.showMenuAsync(obxf::defaultPopupMenuOptions(this));
}

void ObxfAudioProcessorEditor::mutateCallback() { processor.mutatePatch(); }

void ObxfAudioProcessorEditor::randomizeCallback() { processor.randomizePatch(); }

void ObxfAudioProcessorEditor::enterMidiLearnMode()
{
    midiLearnMode = true;

    auto getCC = [this](Component *c) -> int {
        OBLOGONCE(midiLearn, "Make this less scan-every-every");

        auto dcp = dynamic_cast<HasParameterWithID *>(c);
        auto isLastUsed{false};

        if (dcp && dcp->getParameterWithID())
        {
            auto id = dcp->getParameterWithID()->getParameterID();

            if (id == midiLearnLastUsedPID)
                isLastUsed = true;

            auto &mm = processor.getMidiMap();

            for (int i = 0; i < NUM_MIDI_CC; i++)
            {
                if (mm.controllerParamID[i] == id)
                {
                    return isLastUsed ? -i : i;
                }
            }
        }

        return isLastUsed ? -MidiLearnOverlay::noCCSentinel : MidiLearnOverlay::noCCSentinel;
    };

    midiLearnOverlays.clear();

    for (auto &[pid, comp] : componentByParamID)
    {
        AnchorPosition position = determineAnchorPosition(comp, pid);
        auto overlay = std::make_unique<MidiLearnOverlay>(comp, getCC, position);

        overlay->setPopupFont(midiLearnPopupFont);
        overlay->setScaleFactor(impliedScaleFactor());

        overlay->onSelectionCallback = [this, pcopy = pid](MidiLearnOverlay *selected) {
            midiLearnLastUsedPID = pcopy;
            processor.getMidiHandler().setLastUsedParameter(pcopy);
            repaint();
        };

        overlay->onClearCallback = [this, pid](Component *comp) {
            processor.getMidiMap().clearBindingByParamID(pid);
            repaint();
        };

        if (comp->isVisible())
        {
            addAndMakeVisible(*overlay);
            midiLearnOverlays.push_back(std::move(overlay));
        }
    }
}

int ObxfAudioProcessorEditor::patchesInCurrentFolder() const
{
    const auto lsp = processor.lastLoadedPatchNode.lock();

    if (lsp)
    {
        auto lspParent = lsp->parent.lock();

        if (lspParent)
        {
            return lspParent->nonFolderChildIndices.size();
        }
    }

    return 0;
}

AnchorPosition ObxfAudioProcessorEditor::determineAnchorPosition(Component *comp,
                                                                 const juce::String &paramId)
{
    static const std::unordered_map<juce::String, AnchorPosition> paramPositions = {
        {ID::VibratoWave, AnchorPosition::Below},
        {ID::EnvToPitchBothOscs, AnchorPosition::Left},
        {ID::EnvToPitchInvert, AnchorPosition::Right},
        {ID::EnvToPWBothOscs, AnchorPosition::Right},
        {ID::EnvToPWInvert, AnchorPosition::Left},
        {ID::NoiseColor, AnchorPosition::Below},
        {ID::Filter4PoleMode, AnchorPosition::Left},
        {ID::FilterEnvInvert, AnchorPosition::Left},
        {ID::Filter2PoleBPBlend, AnchorPosition::Above},
        {ID::Filter2PolePush, AnchorPosition::Below},
        {ID::LFO1TempoSync, AnchorPosition::Left},
        {ID::LFO1PW, AnchorPosition::Left},
        {ID::LFO2TempoSync, AnchorPosition::Left},
        {ID::LFO2PW, AnchorPosition::Left}};

    if (const auto it = paramPositions.find(paramId); it != paramPositions.end())
        return it->second;

    if (dynamic_cast<Knob *>(comp))
        return AnchorPosition::Overlay;

    return AnchorPosition::Overlay;
}

void ObxfAudioProcessorEditor::exitMidiLearnMode()
{
    midiLearnMode = false;
    midiLearnOverlays.clear();
    midiLearnLastUsedPID = "";
    processor.getMidiHandler().clearLastUsedParameter();
    repaint();
}
