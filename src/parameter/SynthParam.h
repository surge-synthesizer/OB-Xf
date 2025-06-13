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

namespace SynthParam
{
namespace ID
{
static const juce::String Undefined{"Undefined"};
static const juce::String SelfOscPush{"SelfOscPush"};
static const juce::String EnvPitchBoth{"EnvPitchBoth"};
static const juce::String FenvInvert{"FenvInvert"};
static const juce::String PwOsc2Ofs{"PwOfs"};
static const juce::String LevelDif{"LevelDif"};
static const juce::String PwEnvBoth{"PwEnvBoth"};
static const juce::String PwEnv{"PwEnv"};
static const juce::String LfoSync{"LfoSync"};
static const juce::String EconomyMode{"EconomyMode"};
static const juce::String MidiUnlearn{"MidiUnlearn"};
static const juce::String MidiLearn{"MidiLearn"};
static const juce::String VAmpFactor{"VAmpFactor"};
static const juce::String VFltFactor{"VFltFactor"};
static const juce::String AsPlayedAllocation{"AsPlayedAllocation"};
static const juce::String VibratoRate{"VibratoRate"};
static const juce::String FourPole{"FourPole"};
static const juce::String LegatoMode{"LegatoMode"};
static const juce::String EnvelopeToPitch{"EnvelopeToPitch"};
static const juce::String EnvelopeToPitchInv{"EnvelopeToPitchInv"};
static const juce::String VoiceCount{"VoiceCount"};
static const juce::String BandpassBlend{"BandpassBlend"};
static const juce::String FilterWarm{"Filter_Warm"};
static const juce::String PitchBendUpRange{"PitchBendUp"};
static const juce::String PitchBendDownRange{"PitchBendDown"};
static const juce::String BendOsc2Only{"BendOsc2Only"};
static const juce::String Transpose{"Octave"};
static const juce::String Tune{"Tune"};
static const juce::String Brightness{"Brightness"};
static const juce::String NoiseMix{"NoiseMix"};
static const juce::String Osc1Mix{"Osc1Mix"};
static const juce::String Osc2Mix{"Osc2Mix"};
static const juce::String Multimode{"Multimode"};
static const juce::String LfoSampleHoldWave{"LfoSampleHoldWave"};
static const juce::String LfoSineWave{"LfoSineWave"};
static const juce::String LfoSquareWave{"LfoSquareWave"};
static const juce::String LfoAmount1{"LfoAmount1"};
static const juce::String LfoAmount2{"LfoAmount2"};
static const juce::String LfoFilter{"LfoFilter"};
static const juce::String LfoOsc1{"LfoOsc1"};
static const juce::String LfoOsc2{"LfoOsc2"};
static const juce::String LfoFrequency{"LfoFrequency"};
static const juce::String LfoPw1{"LfoPw1"};
static const juce::String LfoPw2{"LfoPw2"};
static const juce::String PortamentoDetune{"PortamentoDetune"};
static const juce::String FilterDetune{"FilterDetune"};
static const juce::String EnvelopeDetune{"EnvelopeDetune"};
static const juce::String Pan1{"Pan1"};
static const juce::String Pan2{"Pan2"};
static const juce::String Pan3{"Pan3"};
static const juce::String Pan4{"Pan4"};
static const juce::String Pan5{"Pan5"};
static const juce::String Pan6{"Pan6"};
static const juce::String Pan7{"Pan7"};
static const juce::String Pan8{"Pan8"};
static const juce::String Xmod{"Xmod"};
static const juce::String Osc2HardSync{"Osc2HardSync"};
static const juce::String Osc1Pitch{"Osc1Pitch"};
static const juce::String Osc2Pitch{"Osc2Pitch"};
static const juce::String Portamento{"Portamento"};
static const juce::String Unison{"Unison"};
static const juce::String FilterKeyFollow{"FilterKeyFollow"};
static const juce::String PulseWidth{"PulseWidth"};
static const juce::String Osc2Saw{"Osc2Saw"};
static const juce::String Osc1Saw{"Osc1Saw"};
static const juce::String Osc1Pulse{"Osc1Pulse"};
static const juce::String Osc2Pulse{"Osc2Pulse"};
static const juce::String Volume{"Volume"};
static const juce::String VoiceDetune{"VoiceDetune"};
static const juce::String Oscillator2Detune{"Oscillator2Detune"};
static const juce::String Cutoff{"Cutoff"};
static const juce::String Resonance{"Resonance"};
static const juce::String FilterEnvAmount{"FilterEnvAmount"};
static const juce::String Attack{"Attack"};
static const juce::String Decay{"Decay"};
static const juce::String Sustain{"Sustain"};
static const juce::String Release{"Release"};
static const juce::String FilterAttack{"FilterAttack"};
static const juce::String FilterDecay{"FilterDecay"};
static const juce::String FilterSustain{"FilterSustain"};
static const juce::String FilterRelease{"FilterRelease"};
} // namespace ID

namespace Name
{
static const juce::String Undefined{"Undefined"};
static const juce::String SelfOscPush{"Filter Self-Oscillation Push"};
static const juce::String EnvPitchBoth{"Filter Envelope to Osc 1+2 Pitch"};
static const juce::String FenvInvert{"Filter Envelope Invert"};
static const juce::String PwOsc2Ofs{"Osc 2 PW Offset"};
static const juce::String LevelDif{"Level Difference"};
static const juce::String PwEnvBoth{"Filter Envelope to Osc 1+2 PW"};
static const juce::String PwEnv{"Filter Envelope to PW"};
static const juce::String LfoSync{"LFO Tempo Sync"};
static const juce::String EconomyMode{"Eco Mode"};
static const juce::String MidiUnlearn{"MIDI Unlearn"};
static const juce::String MidiLearn{"MIDI Learn"};
static const juce::String VAmpFactor{"Velocity to Amp"};
static const juce::String VFltFactor{"Velocity to Filter"};
static const juce::String AsPlayedAllocation{"As Played Allocation"};
static const juce::String VibratoRate{"Vibrato Rate"};
static const juce::String FourPole{"Filter 4-Pole Mode"};
static const juce::String LegatoMode{"Legato Mode"};
static const juce::String EnvelopeToPitch{"Filter Env to Pitch"};
static const juce::String EnvelopeToPitchInv{"Invert Filter Env to Pitch"};
static const juce::String VoiceCount{"Polyphony"};
static const juce::String BandpassBlend{"Filter Bandpass Blend"};
static const juce::String FilterWarm{"High Quality Mode"};
static const juce::String PitchBendUpRange{"Pitch Bend Up"};
static const juce::String PitchBendDownRange{"Pitch Bend Down"};
static const juce::String BendOsc2Only{"Bend Osc 2 Only"};
static const juce::String Transpose{"Transpose"};
static const juce::String Tune{"Tune"};
static const juce::String Brightness{"Brightness"};
static const juce::String NoiseMix{"Noise Mix"};
static const juce::String Osc1Mix{"Osc 1 Mix"};
static const juce::String Osc2Mix{"Osc 2 Mix"};
static const juce::String Multimode{"Filter Morph"};
static const juce::String LfoSampleHoldWave{"LFO S&H Wave"};
static const juce::String LfoSineWave{"LFO Sine Wave"};
static const juce::String LfoSquareWave{"LFO Square Wave"};
static const juce::String LfoAmount1{"LFO Mod 1 Amount"};
static const juce::String LfoAmount2{"LFO Mod 2 Amount"};
static const juce::String LfoFilter{"LFO to Filter"};
static const juce::String LfoOsc1{"LFO to Osc 1 Pitch"};
static const juce::String LfoOsc2{"LFO to Osc 2 Pitch"};
static const juce::String LfoFrequency{"LFO Rate"};
static const juce::String LfoPw1{"LFO to Osc 1 PW"};
static const juce::String LfoPw2{"LFO to Osc 2 PW"};
static const juce::String PortamentoDetune{"Portamento Slop"};
static const juce::String FilterDetune{"Filter Slop"};
static const juce::String EnvelopeDetune{"Envelope Slop"};
static const juce::String Pan1{"Pan Voice 1"};
static const juce::String Pan2{"Pan Voice 2"};
static const juce::String Pan3{"Pan Voice 3"};
static const juce::String Pan4{"Pan Voice 4"};
static const juce::String Pan5{"Pan Voice 5"};
static const juce::String Pan6{"Pan Voice 6"};
static const juce::String Pan7{"Pan Voice 7"};
static const juce::String Pan8{"Pan Voice 8"};
static const juce::String Xmod{"Cross Modulation"};
static const juce::String Osc2HardSync{"Osc 2 Hard Sync"};
static const juce::String Osc1Pitch{"Osc 1 Pitch"};
static const juce::String Osc2Pitch{"Osc 2 Pitch"};
static const juce::String Portamento{"Portamento"};
static const juce::String Unison{"Unison"};
static const juce::String FilterKeyFollow{"Filter Keyfollow"};
static const juce::String PulseWidth{"Pulsewidth"};
static const juce::String Osc2Saw{"Osc 2 Saw"};
static const juce::String Osc1Saw{"Osc 1 Saw"};
static const juce::String Osc1Pulse{"Osc 1 Pulse"};
static const juce::String Osc2Pulse{"Osc 2 Pulse"};
static const juce::String Volume{"Volume"};
static const juce::String VoiceDetune{"Unison Detune"};
static const juce::String Oscillator2Detune{"Osc 2 Detune"};
static const juce::String Cutoff{"Filter Cutoff"};
static const juce::String Resonance{"Filter Resonance"};
static const juce::String FilterEnvAmount{"Filter Envelope Amount"};
static const juce::String Attack{"Amp Envelope Attack"};
static const juce::String Decay{"Amp Envelope Decay"};
static const juce::String Sustain{"Amp Envelope Sustain"};
static const juce::String Release{"Amp Envelope Release"};
static const juce::String FilterAttack{"Filter Envelope Attack"};
static const juce::String FilterDecay{"Filter Envelope Decay"};
static const juce::String FilterSustain{"Filter Envelope Sustain"};
static const juce::String FilterRelease{"Filter Envelope Release"};
} // namespace Name

namespace Units
{
static const juce::String None{""};
static const juce::String Hz{" Hz"};
static const juce::String Octaves{" oct"};
static const juce::String Semitones{" semi"};
static const juce::String Cents{" cents"};
static const juce::String Decibels{" dB"};
static const juce::String Ms{" ms"};
static const juce::String Sec{" s"};
static const juce::String Percent{" %"};
} // namespace Units

namespace Ranges
{
static constexpr float DefaultMin{0.0f};
static constexpr float DefaultMax{1.0f};
static constexpr float DefaultSkew{1.0f};
static constexpr float DefaultIncrement{0.0001f};
static constexpr float OscTuneIncrement{1.f / 4800.f};
static constexpr float Continuous{0.f};
} // namespace Ranges

namespace Type
{
static constexpr int Bool{0};
static constexpr int Float{1};
static constexpr int Choice{2};
} // namespace Type

namespace Defaults
{
static constexpr float Undefined{0.0f};
static constexpr float SelfOscPush{0.0f};
static constexpr float EnvPitchBoth{0.0f};
static constexpr float FenvInvert{0.0f};
static constexpr float PwOsc2Ofs{0.0f};
static constexpr float LevelDif{0.0f};
static constexpr float PwEnvBoth{0.0f};
static constexpr float PwEnv{0.0f};
static constexpr float LfoSync{0.0f};
static constexpr float EconomyMode{0.0f};
static constexpr float MidiUnlearn{0.0f};
static constexpr float MidiLearn{0.0f};
static constexpr float VAmpFactor{0.0f};
static constexpr float VFltFactor{0.0f};
static constexpr float AsPlayedAllocation{0.0f};
static constexpr float VibratoRate{0.1f};
static constexpr float FourPole{0.0f};
static constexpr float LegatoMode{0.0f};
static constexpr float EnvelopeToPitch{0.0f};
static constexpr float EnvelopeToPitchInv{0.0f};
static constexpr float VoiceCount{0.2f};
static constexpr float BandpassBlend{0.0f};
static constexpr float FilterWarm{0.0f};
static constexpr float PitchBendRange{0.5f};
static constexpr float BendOsc2Only{0.0f};
static constexpr float Transpose{0.5f};
static constexpr float Tune{0.5f};
static constexpr float Brightness{0.0f};
static constexpr float NoiseMix{0.0f};
static constexpr float Osc1Mix{1.0f};
static constexpr float Osc2Mix{1.0f};
static constexpr float Multimode{0.0f};
static constexpr float LfoSampleHoldWave{0.0f};
static constexpr float LfoSineWave{1.0f};
static constexpr float LfoSquareWave{0.0f};
static constexpr float LfoAmount1{0.0f};
static constexpr float LfoAmount2{0.0f};
static constexpr float LfoFilter{0.0f};
static constexpr float LfoOsc1{0.0f};
static constexpr float LfoOsc2{0.0f};
static constexpr float LfoFrequency{0.25f};
static constexpr float LfoPw1{0.0f};
static constexpr float LfoPw2{0.0f};
static constexpr float PortamentoDetune{0.0f};
static constexpr float FilterDetune{0.0f};
static constexpr float EnvelopeDetune{0.0f};
static constexpr float Pan1{0.5f};
static constexpr float Pan2{0.5f};
static constexpr float Pan3{0.5f};
static constexpr float Pan4{0.5f};
static constexpr float Pan5{0.5f};
static constexpr float Pan6{0.5f};
static constexpr float Pan7{0.5f};
static constexpr float Pan8{0.5f};
static constexpr float Xmod{0.0f};
static constexpr float Osc2HardSync{0.0f};
static constexpr float Osc1Pitch{0.5f};
static constexpr float Osc2Pitch{0.5f};
static constexpr float Portamento{0.0f};
static constexpr float Unison{0.0f};
static constexpr float FilterKeyFollow{0.0f};
static constexpr float PulseWidth{0.5f};
static constexpr float Osc2Saw{1.0f};
static constexpr float Osc1Saw{1.0f};
static constexpr float Osc1Pulse{0.0f};
static constexpr float Osc2Pulse{0.0f};
static constexpr float Volume{0.7f};
static constexpr float VoiceDetune{0.0f};
static constexpr float Oscillator2Detune{0.5f};
static constexpr float Cutoff{0.5f};
static constexpr float Resonance{0.0f};
static constexpr float FilterEnvAmount{0.0f};
static constexpr float Attack{0.0f};
static constexpr float Decay{0.2f};
static constexpr float Sustain{1.0f};
static constexpr float Release{0.2f};
static constexpr float FilterAttack{0.0f};
static constexpr float FilterDecay{0.2f};
static constexpr float FilterSustain{1.0f};
static constexpr float FilterRelease{0.2f};
} // namespace Defaults
} // namespace SynthParam

#endif // OBXF_SRC_PARAMETER_SYNTHPARAM_H
