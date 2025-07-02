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
static const std::string SelfOscPush{"SelfOscPush"};
static const std::string EnvPitchBoth{"EnvPitchBoth"};
static const std::string FenvInvert{"FenvInvert"};
static const std::string PwOsc2Ofs{"PWOfs"};
static const std::string LevelDetune{"LevelSlop"};
static const std::string PwEnvBoth{"PWEnvBoth"};
static const std::string PwEnv{"PWEnv"};
static const std::string LfoSync{"LfoSync"};
static const std::string EconomyMode{"EconomyMode"};
static const std::string VAmpFactor{"VAmpFactor"};
static const std::string VFltFactor{"VFltFactor"};
static const std::string AsPlayedAllocation{"AsPlayedAllocation"};
static const std::string VibratoRate{"VibratoRate"};
static const std::string FourPole{"FourPole"};
static const std::string LegatoMode{"LegatoMode"};
static const std::string EnvelopeToPitch{"EnvelopeToPitch"};
static const std::string EnvelopeToPitchInv{"EnvelopeToPitchInv"};
static const std::string EnvelopeToPWInv{"EnvelopeToPWInv"};
static const std::string Polyphony{"Polyphony"};
static const std::string UnisonVoices{"UnisonVoices"};
static const std::string BandpassBlend{"BandpassBlend"};
static const std::string FilterWarm{"HQMode"};
static const std::string PitchBendUpRange{"PitchBendUp"};
static const std::string PitchBendDownRange{"PitchBendDown"};
static const std::string BendOsc2Only{"BendOsc2Only"};
static const std::string Transpose{"Octave"};
static const std::string Tune{"Tune"};
static const std::string Brightness{"Brightness"};
static const std::string RingModMix{"RingModMix"};
static const std::string NoiseMix{"NoiseMix"};
static const std::string NoiseColor{"NoiseColor"};
static const std::string Osc1Mix{"Osc1Mix"};
static const std::string Osc2Mix{"Osc2Mix"};
static const std::string Multimode{"Multimode"};
static const std::string LfoSineWave{"LfoWave1"};
static const std::string LfoSquareWave{"LfoWave2"};
static const std::string LfoSampleHoldWave{"LfoWave3"};
static const std::string LfoAmount1{"LfoAmount1"};
static const std::string LfoAmount2{"LfoAmount2"};
static const std::string LfoFilter{"LfoFilter"};
static const std::string LfoOsc1{"LfoOsc1"};
static const std::string LfoOsc2{"LfoOsc2"};
static const std::string LfoFrequency{"LfoFrequency"};
static const std::string LfoPulsewidth{"LfoPW"};
static const std::string LfoPw1{"LfoPW1"};
static const std::string LfoPw2{"LfoPW2"};
static const std::string LfoVolume{"LfoVolume"};
static const std::string PortamentoDetune{"PortamentoDetune"};
static const std::string FilterDetune{"FilterDetune"};
static const std::string EnvelopeDetune{"EnvelopeDetune"};
static const std::string Pan1{"Pan1"};
static const std::string Pan2{"Pan2"};
static const std::string Pan3{"Pan3"};
static const std::string Pan4{"Pan4"};
static const std::string Pan5{"Pan5"};
static const std::string Pan6{"Pan6"};
static const std::string Pan7{"Pan7"};
static const std::string Pan8{"Pan8"};
static const std::string Xmod{"Xmod"};
static const std::string Osc2HardSync{"Osc2HardSync"};
static const std::string Osc1Pitch{"Osc1Pitch"};
static const std::string Osc2Pitch{"Osc2Pitch"};
static const std::string Portamento{"Portamento"};
static const std::string Unison{"Unison"};
static const std::string FilterKeyFollow{"FilterKeyFollow"};
static const std::string PulseWidth{"PulseWidth"};
static const std::string Osc2Saw{"Osc2Saw"};
static const std::string Osc1Saw{"Osc1Saw"};
static const std::string Osc1Pulse{"Osc1Pulse"};
static const std::string Osc2Pulse{"Osc2Pulse"};
static const std::string Volume{"Volume"};
static const std::string VoiceDetune{"VoiceDetune"};
static const std::string Oscillator2Detune{"Oscillator2Detune"};
static const std::string Cutoff{"Cutoff"};
static const std::string Resonance{"Resonance"};
static const std::string FilterEnvAmount{"FilterEnvAmount"};
static const std::string Attack{"Attack"};
static const std::string Decay{"Decay"};
static const std::string Sustain{"Sustain"};
static const std::string Release{"Release"};
static const std::string FilterAttack{"FilterAttack"};
static const std::string FilterDecay{"FilterDecay"};
static const std::string FilterSustain{"FilterSustain"};
static const std::string FilterRelease{"FilterRelease"};
} // namespace ID

namespace Name
{
static const std::string SelfOscPush{"Filter Self-Oscillation Push"};
static const std::string EnvPitchBoth{"Filter Envelope to Osc 1+2 Pitch"};
static const std::string FenvInvert{"Filter Envelope Invert"};
static const std::string PwOsc2Ofs{"Osc 2 PW Offset"};
static const std::string LevelDetune{"Level Slop"};
static const std::string PwEnvBoth{"Filter Envelope to Osc 1+2 PW"};
static const std::string PwEnv{"Filter Envelope to PW"};
static const std::string LfoSync{"LFO Tempo Sync"};
static const std::string EconomyMode{"Eco Mode"};
static const std::string MidiLearn{"MIDI Learn"};
static const std::string MidiUnlearn{"Clear MIDI Learn"};
static const std::string VAmpFactor{"Velocity to Amp"};
static const std::string VFltFactor{"Velocity to Filter"};
static const std::string AsPlayedAllocation{"As Played Allocation"};
static const std::string VibratoRate{"Vibrato Rate"};
static const std::string FourPole{"Filter 4-Pole Mode"};
static const std::string LegatoMode{"Legato Mode"};
static const std::string EnvelopeToPitch{"Filter Env to Pitch"};
static const std::string EnvelopeToPitchInv{"Invert Filter Env to Pitch"};
static const std::string EnvelopeToPWInv{"Invert Filter Env to PW"};
static const std::string Polyphony{"Polyphony"};
static const std::string UnisonVoices{"Unison Voices (unimplemented)"};
static const std::string BandpassBlend{"Filter Bandpass Blend"};
static const std::string FilterWarm{"High Quality Mode"};
static const std::string PitchBendUpRange{"Pitch Bend Up"};
static const std::string PitchBendDownRange{"Pitch Bend Down"};
static const std::string BendOsc2Only{"Bend Osc 2 Only"};
static const std::string Transpose{"Transpose"};
static const std::string Tune{"Tune"};
static const std::string Brightness{"Brightness"};
static const std::string Osc1Mix{"Osc 1 Mix"};
static const std::string Osc2Mix{"Osc 2 Mix"};
static const std::string RingModMix{"Ring Mod Mix"};
static const std::string NoiseMix{"Noise Mix"};
static const std::string NoiseColor{"Noise Color"};
static const std::string Multimode{"Filter Morph"};
static const std::string LfoSampleHoldWave{"LFO Wave 3"};
static const std::string LfoSineWave{"LFO Wave 1"};
static const std::string LfoSquareWave{"LFO Wave 2"};
static const std::string LfoAmount1{"LFO Mod 1 Amount"};
static const std::string LfoAmount2{"LFO Mod 2 Amount"};
static const std::string LfoFilter{"LFO to Filter"};
static const std::string LfoOsc1{"LFO to Osc 1 Pitch"};
static const std::string LfoOsc2{"LFO to Osc 2 Pitch"};
static const std::string LfoFrequency{"LFO Rate"};
static const std::string LfoPulsewidth{"LFO PW"};
static const std::string LfoPw1{"LFO to Osc 1 PW"};
static const std::string LfoPw2{"LFO to Osc 2 PW"};
static const std::string LfoVolume{"LFO to Volume"};
static const std::string PortamentoDetune{"Portamento Slop"};
static const std::string FilterDetune{"Filter Slop"};
static const std::string EnvelopeDetune{"Envelope Slop"};
static const std::string Pan1{"Pan Voice 1"};
static const std::string Pan2{"Pan Voice 2"};
static const std::string Pan3{"Pan Voice 3"};
static const std::string Pan4{"Pan Voice 4"};
static const std::string Pan5{"Pan Voice 5"};
static const std::string Pan6{"Pan Voice 6"};
static const std::string Pan7{"Pan Voice 7"};
static const std::string Pan8{"Pan Voice 8"};
static const std::string Xmod{"Cross Modulation"};
static const std::string Osc2HardSync{"Osc 2 Hard Sync"};
static const std::string Osc1Pitch{"Osc 1 Pitch"};
static const std::string Osc2Pitch{"Osc 2 Pitch"};
static const std::string Portamento{"Portamento"};
static const std::string Unison{"Unison"};
static const std::string FilterKeyFollow{"Filter Keyfollow"};
static const std::string PulseWidth{"Pulsewidth"};
static const std::string Osc2Saw{"Osc 2 Saw"};
static const std::string Osc1Saw{"Osc 1 Saw"};
static const std::string Osc1Pulse{"Osc 1 Pulse"};
static const std::string Osc2Pulse{"Osc 2 Pulse"};
static const std::string Volume{"Volume"};
static const std::string VoiceDetune{"Unison Detune"};
static const std::string Oscillator2Detune{"Osc 2 Detune"};
static const std::string Cutoff{"Filter Cutoff"};
static const std::string Resonance{"Filter Resonance"};
static const std::string FilterEnvAmount{"Filter Envelope Amount"};
static const std::string Attack{"Amp Envelope Attack"};
static const std::string Decay{"Amp Envelope Decay"};
static const std::string Sustain{"Amp Envelope Sustain"};
static const std::string Release{"Amp Envelope Release"};
static const std::string FilterAttack{"Filter Envelope Attack"};
static const std::string FilterDecay{"Filter Envelope Decay"};
static const std::string FilterSustain{"Filter Envelope Sustain"};
static const std::string FilterRelease{"Filter Envelope Release"};
static const std::string PrevPatch{"Previous Patch"};
static const std::string NextPatch{"Next Patch"};
static const std::string InitPatch{"Initialize Patch"};
static const std::string RandomizePatch{"Randomize Patch"};
static const std::string PatchGroupSelect{"Patch Group Select"};
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