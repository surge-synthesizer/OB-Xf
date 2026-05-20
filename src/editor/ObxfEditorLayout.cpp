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

#include "../ObxfEditor.h"

// Panel group types for tab-switching
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

ObxfAudioProcessorEditor::Panel *
ObxfAudioProcessorEditor::PanelGroup::findPanel(const std::string &selectorWidget)
{
    for (auto &panel : panels)
    {
        if (panel.selectorWidget == selectorWidget)
        {
            return &panel;
        }
    }

    return nullptr;
}

// Static registries and panel group definitions
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
        // MASTER
        {"volumeKnob",               {ID::Volume,                Name::Volume,                 0.5f, "knob"}},
        {"transposeKnob",            {ID::Transpose,             Name::Transpose,              0.5f, "knob",
            [](Knob *k) {
                k->altDragCallback = [](const double value) {
                    const auto octValue = static_cast<int>(juce::jmap(value, -2.0, 2.0));
                    return juce::jmap(static_cast<double>(octValue), -2.0, 2.0, 0.0, 1.0);
                };
            }}},
        {"tuneKnob",                 {ID::Tune,                  Name::Tune,                   0.5f, "knob"}},

        // GLOBAL
        {"portamentoKnob",           {ID::Portamento,            Name::Portamento,             0.f,  "knob"}},
        {"unisonDetuneKnob",         {ID::UnisonDetune,          Name::UnisonDetune,           0.25f,"knob"}},

        // OSCILLATORS
        {"osc1PitchKnob",            {ID::Osc1Pitch,             Name::Osc1Pitch,             0.5f,  "knob", pitchSnapCallbacks}},
        {"osc2PitchKnob",            {ID::Osc2Pitch,             Name::Osc2Pitch,             0.5f,  "knob", pitchSnapCallbacks}},
        {"osc2DetuneKnob",           {ID::Osc2Detune,            Name::Osc2Detune,             0.f,  "knob"}},
        {"oscPWKnob",                {ID::OscPW,                 Name::OscPW,                  0.f,  "knob"}},
        {"osc2PWOffsetKnob",         {ID::Osc2PWOffset,          Name::Osc2PWOffset,           0.f,  "knob"}},
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
        {"oscCrossmodKnob",          {ID::OscCrossmod,           Name::OscCrossmod,            0.f,  "knob"}},
        {"oscBrightnessKnob",        {ID::OscBrightness,         Name::OscBrightness,          1.f,  "knob"}},

        // MIXER
        {"osc1VolKnob",              {ID::Osc1Vol,               Name::Osc1Vol,                1.f,  "knob"}},
        {"osc2VolKnob",              {ID::Osc2Vol,               Name::Osc2Vol,                1.f,  "knob"}},
        {"ringModVolKnob",           {ID::RingModVol,            Name::RingModVol,             0.f,  "knob"}},
        {"noiseVolKnob",             {ID::NoiseVol,              Name::NoiseVol,               0.f,  "knob"}},

        // CONTROL
        {"vibratoRateKnob",          {ID::VibratoRate,           Name::VibratoRate,            0.3f, "knob"}},

        // FILTER
        {"filterCutoffKnob",          {ID::FilterCutoff,          Name::FilterCutoff,           1.f,  "knob"}},
        {"filterResonanceKnob",       {ID::FilterResonance,       Name::FilterResonance,        0.f,  "knob"}},
        {"filterEnvAmountKnob",       {ID::FilterEnvAmount,       Name::FilterEnvAmount,        0.f,  "knob"}},
        {"filterKeyTrackKnob",        {ID::FilterKeyTrack,        Name::FilterKeyTrack,         0.f,  "knob"}},
        {"filterModeKnob",            {ID::FilterMode,            Name::FilterMode,             0.f,  "knob"}},

        // LFO 1
        {"lfo1RateKnob",             {ID::LFO1Rate,              Name::LFO1Rate,               0.5f, "knob"}},
        {"lfo1ModAmount1Knob",       {ID::LFO1ModAmount1,        Name::LFO1ModAmount1,         0.f,  "knob"}},
        {"lfo1ModAmount2Knob",       {ID::LFO1ModAmount2,        Name::LFO1ModAmount2,         0.f,  "knob"}},
        {"lfo1Wave1Knob",            {ID::LFO1Wave1,             Name::LFO1Wave1,              0.5f, "knob"}},
        {"lfo1Wave2Knob",            {ID::LFO1Wave2,             Name::LFO1Wave2,              0.5f, "knob"}},
        {"lfo1Wave3Knob",            {ID::LFO1Wave3,             Name::LFO1Wave3,              0.5f, "knob"}},
        {"lfo1PWSlider",             {ID::LFO1PW,                Name::LFO1PW,                 0.f,  "knob"}},

        // LFO 2
        {"lfo2RateKnob",             {ID::LFO2Rate,              Name::LFO2Rate,               0.5f, "knob"}},
        {"lfo2ModAmount1Knob",       {ID::LFO2ModAmount1,        Name::LFO2ModAmount1,         0.f,  "knob"}},
        {"lfo2ModAmount2Knob",       {ID::LFO2ModAmount2,        Name::LFO2ModAmount2,         0.f,  "knob"}},
        {"lfo2Wave1Knob",            {ID::LFO2Wave1,             Name::LFO2Wave1,              0.5f, "knob"}},
        {"lfo2Wave2Knob",            {ID::LFO2Wave2,             Name::LFO2Wave2,              0.5f, "knob"}},
        {"lfo2Wave3Knob",            {ID::LFO2Wave3,             Name::LFO2Wave3,              0.5f, "knob"}},
        {"lfo2PWSlider",             {ID::LFO2PW,                Name::LFO2PW,                 0.f,  "knob"}},

        // FILTER ENVELOPE
        {"filterEnvAttackKnob",       {ID::FilterEnvAttack,       Name::FilterEnvAttack,        0.f,  "knob"}},
        {"filterEnvDecayKnob",        {ID::FilterEnvDecay,        Name::FilterEnvDecay,         0.f,  "knob"}},
        {"filterEnvSustainKnob",      {ID::FilterEnvSustain,      Name::FilterEnvSustain,       1.f,  "knob"}},
        {"filterEnvReleaseKnob",      {ID::FilterEnvRelease,      Name::FilterEnvRelease,       0.f,  "knob"}},
        {"filterEnvAttackCurveSlider",{ID::FilterEnvAttackCurve,  Name::FilterEnvAttackCurve,   0.f,  "slider-h"}},
        {"velToFilterEnvSlider",      {ID::VelToFilterEnv,        Name::VelToFilterEnv,         0.f,  "slider-h"}},

        // AMP ENVELOPE
        {"ampEnvAttackKnob",         {ID::AmpEnvAttack,          Name::AmpEnvAttack,           0.f,  "knob"}},
        {"ampEnvDecayKnob",          {ID::AmpEnvDecay,           Name::AmpEnvDecay,            0.f,  "knob"}},
        {"ampEnvSustainKnob",        {ID::AmpEnvSustain,         Name::AmpEnvSustain,          1.f,  "knob"}},
        {"ampEnvReleaseKnob",        {ID::AmpEnvRelease,         Name::AmpEnvRelease,          0.f,  "knob"}},
        {"ampEnvAttackCurveSlider",  {ID::AmpEnvAttackCurve,     Name::AmpEnvAttackCurve,      0.f,  "slider-h"}},
        {"velToAmpEnvSlider",        {ID::VelToAmpEnv,           Name::VelToAmpEnv,            0.f,  "slider-h"}},

        // VOICE VARIATION
        {"portamentoSlopKnob",       {ID::PortamentoSlop,        Name::PortamentoSlop,         0.25f,"knob"}},
        {"filterSlopKnob",           {ID::FilterSlop,            Name::FilterSlop,             0.25f,"knob"}},
        {"envelopeSlopKnob",         {ID::EnvelopeSlop,          Name::EnvelopeSlop,           0.25f,"knob"}},
        {"levelSlopKnob",            {ID::LevelSlop,             Name::LevelSlop,              0.25f,"knob"}},
        {"pan1Knob",                 {ID::PanVoice1,             Name::PanVoice1,               0.5f,"knob"}},
        {"pan2Knob",                 {ID::PanVoice2,             Name::PanVoice2,               0.5f,"knob"}},
        {"pan3Knob",                 {ID::PanVoice3,             Name::PanVoice3,               0.5f,"knob"}},
        {"pan4Knob",                 {ID::PanVoice4,             Name::PanVoice4,               0.5f,"knob"}},
        {"pan5Knob",                 {ID::PanVoice5,             Name::PanVoice5,               0.5f,"knob"}},
        {"pan6Knob",                 {ID::PanVoice6,             Name::PanVoice6,               0.5f,"knob"}},
        {"pan7Knob",                 {ID::PanVoice7,             Name::PanVoice7,               0.5f,"knob"}},
        {"pan8Knob",                 {ID::PanVoice8,             Name::PanVoice8,               0.5f,"knob"}},
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
        // GLOBAL
        {"hqModeButton",             {ID::HQMode,                Name::HQMode,                "button"}},
        {"unisonButton",             {ID::Unison,                Name::Unison,                "button"}},

        // OSCILLATORS
        {"osc1SawButton",            {ID::Osc1SawWave,           Name::Osc1SawWave,           "button"}},
        {"osc1PulseButton",          {ID::Osc1PulseWave,         Name::Osc1PulseWave,         "button"}},
        {"osc2SawButton",            {ID::Osc2SawWave,           Name::Osc2SawWave,           "button"}},
        {"osc2PulseButton",          {ID::Osc2PulseWave,         Name::Osc2PulseWave,         "button"}},
        {"osc2KeytrackButton",       {ID::Osc2Keytrack,          Name::Osc2Keytrack,          "button-keytrack"}},
        {"envToPitchInvertButton",   {ID::EnvToPitchInvert,      Name::EnvToPitchInvert,      "button-slim"}},
        {"envToPitchBothOscsButton", {ID::EnvToPitchBothOscs,    Name::EnvToPitchBothOscs,    "button-slim"}},
        {"envToPWInvertButton",      {ID::EnvToPWInvert,         Name::EnvToPWInvert,         "button-slim"}},
        {"envToPWBothOscsButton",    {ID::EnvToPWBothOscs,       Name::EnvToPWBothOscs,       "button-slim"}},
        {"oscSyncButton",            {ID::OscSync,               Name::OscSync,               "button"}},

        // CONTROL
        {"bendOsc2OnlyButton",       {ID::BendOsc2Only,          Name::BendOsc2Only,          "button"}},
        {"vibratoWaveButton",        {ID::VibratoWave,           Name::VibratoWave,           "button-slim-vibrato-wave"}},

        // FILTER
        {"filter4PoleModeButton",    {ID::Filter4PoleMode,       Name::Filter4PoleMode,       "button-slim"}},
        {"filter2PoleBPBlendButton", {ID::Filter2PoleBPBlend,    Name::Filter2PoleBPBlend,    "button-slim"}},
        {"filter2PolePushButton",    {ID::Filter2PolePush,       Name::Filter2PolePush,       "button-slim"}},
        {"filter4PoleXpanderButton", {ID::Filter4PoleXpander,    Name::Filter4PoleXpander,    "button"}},

        // LFO 1
        {"lfo1TempoSyncButton",      {ID::LFO1TempoSync,         Name::LFO1TempoSync,         "button-slim"}},

        // LFO 2
        {"lfo2TempoSyncButton",      {ID::LFO2TempoSync,         Name::LFO2TempoSync,         "button-slim-alt"}},

        // FILTER ENVELOPE
        {"filterEnvInvertButton",    {ID::FilterEnvInvert,       Name::FilterEnvInvert,       "button-slim"}},
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
        // MIXER
        {"noiseColorButton",         {ID::NoiseColor,            Name::NoiseColor,            "button-slim-noise",  3}},

        // LFO 1
        {"lfo1ToOsc1PitchButton",    {ID::LFO1ToOsc1Pitch,       Name::LFO1ToOsc1Pitch,       "button-dual",     3}},
        {"lfo1ToOsc2PitchButton",    {ID::LFO1ToOsc2Pitch,       Name::LFO1ToOsc2Pitch,       "button-dual",     3}},
        {"lfo1ToFilterCutoffButton", {ID::LFO1ToFilterCutoff,    Name::LFO1ToFilterCutoff,    "button-dual",     3}},
        {"lfo1ToOsc1PWButton",       {ID::LFO1ToOsc1PW,          Name::LFO1ToOsc1PW,          "button-dual",     3}},
        {"lfo1ToOsc2PWButton",       {ID::LFO1ToOsc2PW,          Name::LFO1ToOsc2PW,          "button-dual",     3}},
        {"lfo1ToVolumeButton",       {ID::LFO1ToVolume,          Name::LFO1ToVolume,          "button-dual",     3}},

        // LFO 2
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
        // GLOBAL
        {"polyphonyMenu",        {ID::Polyphony,          Name::Polyphony,          "menu-poly"         }},
        {"unisonVoicesMenu",     {ID::UnisonVoices,       Name::UnisonVoices,       "menu-poly"         }},
        {"envLegatoModeMenu",    {ID::EnvLegatoMode,      Name::EnvLegatoMode,      "menu-legato"       }},
        {"notePriorityMenu",     {ID::NotePriority,       Name::NotePriority,       "menu-note-priority"}},

        // CONTROL
        {"bendUpRangeMenu",      {ID::BendUpRange,        Name::BendUpRange,        "menu-pitch-bend"   }},
        {"bendDownRangeMenu",    {ID::BendDownRange,      Name::BendDownRange,      "menu-pitch-bend"   }},

        // FILTER
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
        p.selectorWidget = std::move(selector);
        p.widgetNames = std::move(names);
        p.childGroup = std::move(child);
        return p;
    };

    // clang-format off
    std::map<std::string, PanelGroup> m;

    // ---- MASTER / MTS-ESP ----
    {
        auto &g = m["master"];

        g.panels.push_back(
            makePanel("",
            {
                "volumeKnob", "transposeKnob", "tuneKnob",
            }));

        g.panels.push_back(
            makePanel("mtsSettingsButton",
            {
                "mtsInfoDisplay", "mtsDynamicButton",
            }));
    }

    // ---- GLOBAL / MPE ----
    {

        auto &g = m["global"];

        g.panels.push_back(
            makePanel("",
            {
                "polyphonyMenu",     "hqModeButton", "lockHQButton",     "unisonVoicesMenu",
                "portamentoKnob",    "unisonButton",                     "unisonDetuneKnob",
                "envLegatoModeMenu",                 "notePriorityMenu",
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
            makePanel("mpeSettingsButton",
            {
                "mpeLinesLabel",
                "mpeStrikeSelectButton", "mpeLiftSelectButton",
                "mpePressSelectButton",  "mpeSlideSelectButton",
                "mpeSlideBipolarButton", "mpeGlideRangeMenu",
            },
            std::move(mpeSubGroup)));
    }

    // ---- LFO ----
    {
        auto &g = m["lfo"];

        g.panels.push_back(makePanel("lfo1SelectButton", {
            "lfo1TempoSyncButton",
            "lfo1RateKnob",          "lfo1ModAmount1Knob",    "lfo1ModAmount2Knob",
            "lfo1Wave1Knob",         "lfo1Wave2Knob",         "lfo1Wave3Knob",
            "lfo1PWSlider",          "lfo1Wave2Label",
            "lfo1ToOsc1PitchButton", "lfo1ToOsc2PitchButton", "lfo1ToFilterCutoffButton",
            "lfo1ToOsc1PWButton",    "lfo1ToOsc2PWButton",    "lfo1ToVolumeButton",
        }));

        g.panels.push_back(makePanel("lfo2SelectButton", {
            "lfo2TempoSyncButton",
            "lfo2RateKnob",          "lfo2ModAmount1Knob",    "lfo2ModAmount2Knob",
            "lfo2Wave1Knob",         "lfo2Wave2Knob",         "lfo2Wave3Knob",
            "lfo2PWSlider",          "lfo2Wave2Label",
            "lfo2ToOsc1PitchButton", "lfo2ToOsc2PitchButton", "lfo2ToFilterCutoffButton",
            "lfo2ToOsc1PWButton",    "lfo2ToOsc2PWButton",    "lfo2ToVolumeButton",
        }));
    }

    // clang-format on
    return m;
}
