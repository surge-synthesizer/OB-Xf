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

#ifndef OBXF_SRC_PARAMETER_SYNTHPARAM_H
#define OBXF_SRC_PARAMETER_SYNTHPARAM_H

#include <juce_audio_basics/juce_audio_basics.h>
#include <sst/basic-blocks/params/ParamMetadata.h>

namespace SynthParam
{
namespace ID
{
// MASTER
static const std::string Volume{"Volume"};
static const std::string Transpose{"Transpose"};
static const std::string Tune{"Tune"};

// GLOBAL
static const std::string Polyphony{"Polyphony"};
static const std::string HQMode{"HQMode"};
static const std::string UnisonVoices{"UnisonVoices"};

static const std::string Portamento{"Portamento"};
static const std::string Unison{"Unison"};
static const std::string UnisonDetune{"UnisonDetune"};

static const std::string EnvLegatoMode{"EnvLegatoMode"};
static const std::string NotePriority{"NotePriority"};

// OSCILLATORS
static const std::string Osc1Pitch{"Osc1Pitch"};
static const std::string Osc2Detune{"Osc2Detune"};
static const std::string Osc2Pitch{"Osc2Pitch"};

static const std::string Osc1SawWave{"Osc1SawWave"};
static const std::string Osc2SawWave{"Osc2SawWave"};

static const std::string Osc1PulseWave{"Osc1PulseWave"};
static const std::string Osc2PulseWave{"Osc2PulseWave"};

static const std::string OscPW{"OscPW"};
static const std::string Osc2PWOffset{"Osc2PWOffset"};

static const std::string EnvToPitchAmount{"EnvToPitchAmount"};
static const std::string EnvToPitchBothOscs{"EnvToPitchBothOscs"};
static const std::string EnvToPitchInvert{"EnvToPitchInvert"};

static const std::string EnvToPWAmount{"EnvToPWAmount"};
static const std::string EnvToPWBothOscs{"EnvToPWBothOscs"};
static const std::string EnvToPWInvert{"EnvToPWInvert"};

static const std::string OscCrossmod{"OscCrossmod"};
static const std::string OscSync{"OscSync"};
static const std::string OscBrightness{"OscBrightness"};

// MIXER
static const std::string Osc1Mix{"Osc1Mix"};
static const std::string Osc2Mix{"Osc2Mix"};
static const std::string RingModMix{"RingModMix"};
static const std::string NoiseMix{"NoiseMix"};
static const std::string NoiseColor{"NoiseColor"};

// CONTROL
static const std::string BendUpRange{"PitchBendUp"};
static const std::string BendDownRange{"PitchBendDown"};
static const std::string BendOsc2Only{"BendOsc2Only"};

static const std::string VibratoWave{"VibratoWave"};
static const std::string VibratoRate{"VibratoRate"};

// FILTER
static const std::string Filter4PoleMode{"Filter4PoleMode"};

static const std::string FilterCutoff{"FilterCutoff"};
static const std::string FilterResonance{"FilterResonance"};
static const std::string FilterEnvAmount{"FilterEnvAmount"};

static const std::string FilterKeyFollow{"FilterKeyFollow"};
static const std::string FilterMode{"FilterMode"};

static const std::string Filter2PoleBPBlend{"Filter2PoleBPBlend"};
static const std::string Filter2PolePush{"Filter2PolePush"};

static const std::string Filter4PoleXpander{"Filter4PoleXpander"};
static const std::string FilterXpanderMode{"FilterXpanderMode"};

// LFO 1
static const std::string LFO1TempoSync{"LFO1TempoSync"};

static const std::string LFO1Rate{"LFO1Rate"};
static const std::string LFO1ModAmount1{"LFO1ModAmount1"};
static const std::string LFO1ModAmount2{"LFO1ModAmount2"};

static const std::string LFO1Wave1{"LFO1Wave1"};
static const std::string LFO1Wave2{"LFO1Wave2"};
static const std::string LFO1Wave3{"LFO1Wave3"};

static const std::string LFO1PW{"LFO1PW"};

static const std::string LFO1ToOsc1Pitch{"LFO1ToOsc1Pitch"};
static const std::string LFO1ToOsc2Pitch{"LFO1ToOsc2Pitch"};
static const std::string LFO1ToFilterCutoff{"LFO1ToFilterCutoff"};

static const std::string LFO1ToOsc1PW{"LFO1ToOsc1PW"};
static const std::string LFO1ToOsc2PW{"LFO1ToOsc2PW"};
static const std::string LFO1ToVolume{"LFO1ToVolume"};

// LFO 2
static const std::string LFO2TempoSync{"LFO2TempoSync"};

static const std::string LFO2Wave1{"LFO2Wave1"};
static const std::string LFO2Wave2{"LFO2Wave2"};
static const std::string LFO2Wave3{"LFO2Wave3"};

static const std::string LFO2PW{"LFO2PW"};

static const std::string LFO2Rate{"LFO2Rate"};
static const std::string LFO2ModAmount1{"LFO2ModAmount1"};
static const std::string LFO2ModAmount2{"LFO2ModAmount2"};

static const std::string LFO2ToOsc1Pitch{"LFO2ToOsc1Pitch"};
static const std::string LFO2ToOsc2Pitch{"LFO2ToOsc2Pitch"};
static const std::string LFO2ToFilterCutoff{"LFO2ToFilterCutoff"};

static const std::string LFO2ToOsc1PW{"LFO2ToOsc1PW"};
static const std::string LFO2ToOsc2PW{"LFO2ToOsc2PW"};
static const std::string LFO2ToVolume{"LFO2ToVolume"};

// FILTER ENVELOPE
static const std::string FilterEnvInvert{"FilterEnvInvert"};

static const std::string FilterEnvAttack{"FilterEnvAttack"};
static const std::string FilterEnvDecay{"FilterEnvDecay"};
static const std::string FilterEnvSustain{"FilterEnvSustain"};
static const std::string FilterEnvRelease{"FilterEnvRelease"};

static const std::string FilterEnvAttackCurve{"FilterEnvAttackCurve"};
static const std::string VelToFilterEnv{"VelToFilterEnv"};

// AMPLIFIER ENVELOPE
static const std::string AmpEnvAttack{"AmpEnvAttack"};
static const std::string AmpEnvDecay{"AmpEnvDecay"};
static const std::string AmpEnvSustain{"AmpEnvSustain"};
static const std::string AmpEnvRelease{"AmpEnvRelease"};

static const std::string AmpEnvAttackCurve{"AmpEnvAttackCurve"};
static const std::string VelToAmpEnv{"VelToAmpEnv"};

// VOICE VARIATION
static const std::string PortamentoSlop{"PortamentoSlop"};
static const std::string FilterSlop{"FilterSlop"};
static const std::string EnvelopeSlop{"EnvelopeSlop"};
static const std::string LevelSlop{"LevelSlop"};

static const std::string PanVoice1{"PanVoice1"};
static const std::string PanVoice2{"PanVoice2"};
static const std::string PanVoice3{"PanVoice3"};
static const std::string PanVoice4{"PanVoice4"};

static const std::string PanVoice5{"PanVoice5"};
static const std::string PanVoice6{"PanVoice6"};
static const std::string PanVoice7{"PanVoice7"};
static const std::string PanVoice8{"PanVoice8"};

// OTHER/HIDDEN
static const std::string EcoMode{"EcoMode"};
} // namespace ID

namespace Name
{
// MASTER
static const std::string Volume{"Volume"};
static const std::string Transpose{"Transpose"};
static const std::string Tune{"Tune"};

// GLOBAL
static const std::string Polyphony{"Polyphony"};
static const std::string HQMode{"High Quality Mode"};
static const std::string UnisonVoices{"Unison Voices"};

static const std::string Portamento{"Portamento"};
static const std::string Unison{"Unison"};
static const std::string UnisonDetune{"Unison Detune"};

static const std::string EnvLegatoMode{"Envelope Legato Mode"};
static const std::string NotePriority{"Note Priority"};

static const std::string MidiLearn{"MIDI Learn"};
static const std::string MidiUnlearn{"Clear MIDI Learn"};

// OSCILLATORS
static const std::string Osc1Pitch{"Osc 1 Pitch"};
static const std::string Osc2Detune{"Osc 2 Detune"};
static const std::string Osc2Pitch{"Osc 2 Pitch"};

static const std::string Osc1SawWave{"Osc 1 Saw Wave"};
static const std::string Osc1PulseWave{"Osc 1 Pulse Wave"};

static const std::string Osc2SawWave{"Osc 2 Saw Wave"};
static const std::string Osc2PulseWave{"Osc 2 Pulse Wave"};

static const std::string OscPW{"Osc Pulsewidth"};
static const std::string Osc2PWOffset{"Osc 2 PW Offset"};

static const std::string EnvToPitchAmount{"Filter Env to Pitch"};
static const std::string EnvToPitchBothOscs{"Filter Env to Both Osc Pitch"};
static const std::string EnvToPitchInvert{"Invert Filter Env to Pitch"};

static const std::string EnvToPWAmount{"Filter Env to PW"};
static const std::string EnvToPWBothOscs{"Filter Env to Both Osc PW"};
static const std::string EnvToPWInvert{"Invert Filter Env to PW"};

static const std::string OscCrossmod{"Cross Modulation"};
static const std::string OscSync{"Osc Sync"};
static const std::string OscBrightness{"Osc Brightness"};

// MIXER
static const std::string Osc1Mix{"Osc 1 Mix"};
static const std::string Osc2Mix{"Osc 2 Mix"};
static const std::string RingModMix{"Ring Mod Mix"};
static const std::string NoiseMix{"Noise Mix"};
static const std::string NoiseColor{"Noise Color"};

// CONTROL
static const std::string BendUpRange{"Pitch Bend Up"};
static const std::string BendDownRange{"Pitch Bend Down"};
static const std::string BendOsc2Only{"Pitch Bend Osc 2 Only"};

static const std::string VibratoWave{"Vibrato Wave"};
static const std::string VibratoRate{"Vibrato Rate"};

// FILTER
static const std::string Filter4PoleMode{"Filter 4-Pole Mode"};

static const std::string FilterCutoff{"Filter Cutoff"};
static const std::string FilterResonance{"Filter Resonance"};
static const std::string FilterEnvAmount{"Filter Env Amount"};

static const std::string FilterKeyFollow{"Filter Keyfollow"};
static const std::string FilterMode{"Filter Mode"};

static const std::string Filter2PoleBPBlend{"Filter 2-Pole BP Blend"};
static const std::string Filter2PolePush{"Filter 2-Pole Push"};

static const std::string Filter4PoleXpander{"Filter 4-Pole Xpander"};
static const std::string FilterXpanderMode{"Filter Xpander Mode"};

// LFO 1
static const std::string LFO1TempoSync{"LFO 1 Tempo Sync"};

static const std::string LFO1Rate{"LFO 1 Rate"};
static const std::string LFO1ModAmount1{"LFO 1 Mod 1 Amount"};
static const std::string LFO1ModAmount2{"LFO 1 Mod 2 Amount"};

static const std::string LFO1Wave1{"LFO 1 Wave 1"};
static const std::string LFO1Wave2{"LFO 1 Wave 2"};
static const std::string LFO1Wave3{"LFO 1 Wave 3"};

static const std::string LFO1PW{"LFO 1 Pulsewidth"};

static const std::string LFO1ToOsc1Pitch{"LFO 1 to Osc 1 Pitch"};
static const std::string LFO1ToOsc2Pitch{"LFO 1 to Osc 2 Pitch"};
static const std::string LFO1ToFilterCutoff{"LFO 1 to Filter Cutoff"};

static const std::string LFO1ToOsc1PW{"LFO 1 to Osc 1 PW"};
static const std::string LFO1ToOsc2PW{"LFO 1 to Osc 2 PW"};
static const std::string LFO1ToVolume{"LFO 1 to Volume"};

// LFO 2
static const std::string LFO2TempoSync{"LFO 2 Tempo Sync"};

static const std::string LFO2Rate{"LFO 2 Rate"};
static const std::string LFO2ModAmount1{"LFO 2 Mod 1 Amount"};
static const std::string LFO2ModAmount2{"LFO 2 Mod 2 Amount"};

static const std::string LFO2Wave1{"LFO 2 Wave 1"};
static const std::string LFO2Wave2{"LFO 2 Wave 2"};
static const std::string LFO2Wave3{"LFO 2 Wave 3"};

static const std::string LFO2PW{"LFO 2 Pulsewidth"};

static const std::string LFO2ToOsc1Pitch{"LFO 2 to Osc 1 Pitch"};
static const std::string LFO2ToOsc2Pitch{"LFO 2 to Osc 2 Pitch"};
static const std::string LFO2ToFilterCutoff{"LFO 2 to Filter Cutoff"};

static const std::string LFO2ToOsc1PW{"LFO 2 to Osc 1 PW"};
static const std::string LFO2ToOsc2PW{"LFO 2 to Osc 2 PW"};
static const std::string LFO2ToVolume{"LFO 2 to Volume"};

// VOICE VARIATION
static const std::string PortamentoSlop{"Portamento Slop"};
static const std::string FilterSlop{"Filter Slop"};
static const std::string EnvelopeSlop{"Envelope Slop"};
static const std::string LevelSlop{"Level Slop"};

static const std::string PanVoice1{"Pan Voice 1"};
static const std::string PanVoice2{"Pan Voice 2"};
static const std::string PanVoice3{"Pan Voice 3"};
static const std::string PanVoice4{"Pan Voice 4"};

static const std::string PanVoice5{"Pan Voice 5"};
static const std::string PanVoice6{"Pan Voice 6"};
static const std::string PanVoice7{"Pan Voice 7"};
static const std::string PanVoice8{"Pan Voice 8"};

// FILTER ENVELOPE
static const std::string FilterEnvInvert{"Filter Env Invert"};

static const std::string FilterEnvAttack{"Filter Env Attack"};
static const std::string FilterEnvDecay{"Filter Env Decay"};
static const std::string FilterEnvSustain{"Filter Env Sustain"};
static const std::string FilterEnvRelease{"Filter Env Release"};

static const std::string FilterEnvAttackCurve{"Filter Env Attack Curve"};
static const std::string VelToFilterEnv{"Velocity to Filter Env"};

// AMPLIFIER ENVELOPE
static const std::string AmpEnvAttack{"Amp Env Attack"};
static const std::string AmpEnvDecay{"Amp Env Decay"};
static const std::string AmpEnvSustain{"Amp Env Sustain"};
static const std::string AmpEnvRelease{"Amp Env Release"};

static const std::string AmpEnvAttackCurve{"Amp Env Attack Curve"};
static const std::string VelToAmpEnv{"Velocity to Amp Env"};

// PROGRAMMER
static const std::string PatchList{"Patch List"};
static const std::string PrevPatch{"Previous Patch"};
static const std::string NextPatch{"Next Patch"};

static const std::string InitializePatch{"Initialize Patch"};
static const std::string RandomizePatch{"Randomize Patch"};

static const std::string PatchGroupSelect{"Select Patch or Group"};

// OTHER/HIDDEN
static const std::string EcoMode{"Eco Mode"};
} // namespace Name
} // namespace SynthParam

struct ObxfParameterFloat : juce::AudioParameterFloat
{
    ObxfParameterFloat(const juce::ParameterID &parameterID,
                       juce::NormalisableRange<float> normalisableRange, size_t paramIndexIn,
                       const sst::basic_blocks::params::ParamMetaData &md)
        : juce::AudioParameterFloat(
              parameterID, md.name, normalisableRange, md.defaultVal,
              juce::AudioParameterFloatAttributes()
                  .withLabel(md.unit)
                  .withCategory(juce::AudioProcessorParameter::genericParameter)
                  .withStringFromValueFunction(
                      [this](auto v, auto s) { return this->stringFromValue(v, s); })
                  .withValueFromStringFunction(
                      [this](auto v) { return this->valueFromString(v.toStdString()); })),
          meta(md), paramIndex(paramIndexIn)
    {
    }
    sst::basic_blocks::params::ParamMetaData meta;
    size_t paramIndex{0};

    std::string stringFromValue(float value, int)
    {
        float denormalizedValue = juce::jmap(value, 0.0f, 1.0f, meta.minVal, meta.maxVal);
        sst::basic_blocks::params::ParamMetaData::FeatureState fs;
        fs.isExtended = true;
        auto res = meta.valueToString(denormalizedValue, fs);
        if (res.has_value())
            return *res;
        else
            return "-error--";
    }
    float valueFromString(const std::string &s)
    {
        std::string em;
        auto res = meta.valueFromString(s, em);
        if (res.has_value())
            return *res;
        else
            return 0.f;
    }
};

#endif // OBXF_SRC_PARAMETER_SYNTHPARAM_H