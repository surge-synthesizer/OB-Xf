#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Engine/SynthEngine.h"

#include "IParameterState.h"
#include "IProgramState.h"
#include "SynthParam.h"
#include "ParameterManager.h"

static const std::vector<ParameterInfo> Parameters{
    {SynthParam::ID::Undefined, SynthParam::Name::Undefined, SynthParam::Units::Percent,
     SynthParam::Defaults::Undefined, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::SelfOscPush, SynthParam::Name::SelfOscPush, SynthParam::Units::Percent,
     SynthParam::Defaults::SelfOscPush, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::EnvPitchBoth, SynthParam::Name::EnvPitchBoth, SynthParam::Units::Percent,
     SynthParam::Defaults::EnvPitchBoth, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FenvInvert, SynthParam::Name::FenvInvert, SynthParam::Units::Percent,
     SynthParam::Defaults::FenvInvert, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::PwOsc2Ofs, SynthParam::Name::PwOsc2Ofs, SynthParam::Units::Percent,
     SynthParam::Defaults::PwOsc2Ofs, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LevelDif, SynthParam::Name::LevelDif, SynthParam::Units::Percent,
     SynthParam::Defaults::LevelDif, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::PwEnvBoth, SynthParam::Name::PwEnvBoth, SynthParam::Units::Percent,
     SynthParam::Defaults::PwEnvBoth, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::PwEnv, SynthParam::Name::PwEnv, SynthParam::Units::Percent,
     SynthParam::Defaults::PwEnv, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoSync, SynthParam::Name::LfoSync, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoSync, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::EconomyMode, SynthParam::Name::EconomyMode, SynthParam::Units::Percent,
     SynthParam::Defaults::EconomyMode, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::MidiUnlearn, SynthParam::Name::MidiUnlearn, SynthParam::Units::Percent,
     SynthParam::Defaults::MidiUnlearn, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::MidiLearn, SynthParam::Name::MidiLearn, SynthParam::Units::Percent,
     SynthParam::Defaults::MidiLearn, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::VAmpFactor, SynthParam::Name::VAmpFactor, SynthParam::Units::Percent,
     SynthParam::Defaults::VAmpFactor, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::VFltFactor, SynthParam::Name::VFltFactor, SynthParam::Units::Percent,
     SynthParam::Defaults::VFltFactor, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::AsPlayedAllocation, SynthParam::Name::AsPlayedAllocation,
     SynthParam::Units::Percent, SynthParam::Defaults::AsPlayedAllocation,
     SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::VibratoRate, SynthParam::Name::VibratoRate, SynthParam::Units::Hz,
     SynthParam::Defaults::VibratoRate, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FourPole, SynthParam::Name::FourPole, SynthParam::Units::Percent,
     SynthParam::Defaults::FourPole, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LegatoMode, SynthParam::Name::LegatoMode, SynthParam::Units::Percent,
     SynthParam::Defaults::LegatoMode, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::EnvelopeToPitch, SynthParam::Name::EnvelopeToPitch, SynthParam::Units::Percent,
     SynthParam::Defaults::EnvelopeToPitch, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::PitchQuant, SynthParam::Name::PitchQuant, SynthParam::Units::Percent,
     SynthParam::Defaults::PitchQuant, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::VoiceCount, SynthParam::Name::VoiceCount, SynthParam::Units::Percent,
     SynthParam::Defaults::VoiceCount, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::BandpassBlend, SynthParam::Name::BandpassBlend, SynthParam::Units::Percent,
     SynthParam::Defaults::BandpassBlend, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterWarm, SynthParam::Name::FilterWarm, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterWarm, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::BendRange, SynthParam::Name::BendRange, SynthParam::Units::Percent,
     SynthParam::Defaults::BendRange, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::BendOsc2Only, SynthParam::Name::BendOsc2Only, SynthParam::Units::Percent,
     SynthParam::Defaults::BendOsc2Only, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Octave, SynthParam::Name::Octave, SynthParam::Units::Percent,
     SynthParam::Defaults::Octave, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Tune, SynthParam::Name::Tune, SynthParam::Units::Percent,
     SynthParam::Defaults::Tune, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Brightness, SynthParam::Name::Brightness, SynthParam::Units::Percent,
     SynthParam::Defaults::Brightness, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::NoiseMix, SynthParam::Name::NoiseMix, SynthParam::Units::Percent,
     SynthParam::Defaults::NoiseMix, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc2Mix, SynthParam::Name::Osc2Mix, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc2Mix, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc1Mix, SynthParam::Name::Osc1Mix, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc1Mix, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Multimode, SynthParam::Name::Multimode, SynthParam::Units::Percent,
     SynthParam::Defaults::Multimode, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoSampleHoldWave, SynthParam::Name::LfoSampleHoldWave,
     SynthParam::Units::Percent, SynthParam::Defaults::LfoSampleHoldWave,
     SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoSineWave, SynthParam::Name::LfoSineWave, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoSineWave, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoSquareWave, SynthParam::Name::LfoSquareWave, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoSquareWave, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoAmount1, SynthParam::Name::LfoAmount1, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoAmount1, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoAmount2, SynthParam::Name::LfoAmount2, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoAmount2, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoFilter, SynthParam::Name::LfoFilter, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoFilter, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoOsc1, SynthParam::Name::LfoOsc1, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoOsc1, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoOsc2, SynthParam::Name::LfoOsc2, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoOsc2, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoFrequency, SynthParam::Name::LfoFrequency, SynthParam::Units::Hz,
     SynthParam::Defaults::LfoFrequency, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoPw1, SynthParam::Name::LfoPw1, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoPw1, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::LfoPw2, SynthParam::Name::LfoPw2, SynthParam::Units::Percent,
     SynthParam::Defaults::LfoPw2, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::PortamentoDetune, SynthParam::Name::PortamentoDetune,
     SynthParam::Units::Percent, SynthParam::Defaults::PortamentoDetune,
     SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterDetune, SynthParam::Name::FilterDetune, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterDetune, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultSkew,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::EnvelopeDetune, SynthParam::Name::EnvelopeDetune, SynthParam::Units::Percent,
     SynthParam::Defaults::EnvelopeDetune, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan1, SynthParam::Name::Pan1, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan1, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan2, SynthParam::Name::Pan2, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan2, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan3, SynthParam::Name::Pan3, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan3, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan4, SynthParam::Name::Pan4, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan4, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan5, SynthParam::Name::Pan5, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan5, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan6, SynthParam::Name::Pan6, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan6, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan7, SynthParam::Name::Pan7, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan7, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Pan8, SynthParam::Name::Pan8, SynthParam::Units::Percent,
     SynthParam::Defaults::Pan8, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Xmod, SynthParam::Name::Xmod, SynthParam::Units::Percent,
     SynthParam::Defaults::Xmod, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc2HardSync, SynthParam::Name::Osc2HardSync, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc2HardSync, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc1Pitch, SynthParam::Name::Osc1Pitch, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc1Pitch, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc2Pitch, SynthParam::Name::Osc2Pitch, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc2Pitch, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Portamento, SynthParam::Name::Portamento, SynthParam::Units::Percent,
     SynthParam::Defaults::Portamento, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Unison, SynthParam::Name::Unison, SynthParam::Units::Percent,
     SynthParam::Defaults::Unison, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterKeyFollow, SynthParam::Name::FilterKeyFollow, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterKeyFollow, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::PulseWidth, SynthParam::Name::PulseWidth, SynthParam::Units::Percent,
     SynthParam::Defaults::PulseWidth, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc1Saw, SynthParam::Name::Osc1Saw, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc1Saw, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc2Saw, SynthParam::Name::Osc2Saw, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc2Saw, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc1Pulse, SynthParam::Name::Osc1Pulse, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc1Pulse, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Osc2Pulse, SynthParam::Name::Osc2Pulse, SynthParam::Units::Percent,
     SynthParam::Defaults::Osc2Pulse, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Volume, SynthParam::Name::Volume, SynthParam::Units::Percent,
     SynthParam::Defaults::Volume, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::VoiceDetune, SynthParam::Name::VoiceDetune, SynthParam::Units::Percent,
     SynthParam::Defaults::VoiceDetune, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Oscillator2detune, SynthParam::Name::Oscillator2detune,
     SynthParam::Units::Percent, SynthParam::Defaults::Oscillator2detune,
     SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Cutoff, SynthParam::Name::Cutoff, SynthParam::Units::Percent,
     SynthParam::Defaults::Cutoff, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Resonance, SynthParam::Name::Resonance, SynthParam::Units::Percent,
     SynthParam::Defaults::Resonance, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterEnvAmount, SynthParam::Name::FilterEnvAmount, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterEnvAmount, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Attack, SynthParam::Name::Attack, SynthParam::Units::Percent,
     SynthParam::Defaults::Attack, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Decay, SynthParam::Name::Decay, SynthParam::Units::Percent,
     SynthParam::Defaults::Decay, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Sustain, SynthParam::Name::Sustain, SynthParam::Units::Percent,
     SynthParam::Defaults::Sustain, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::Release, SynthParam::Name::Release, SynthParam::Units::Percent,
     SynthParam::Defaults::Release, SynthParam::Ranges::DefaultMin, SynthParam::Ranges::DefaultMax,
     SynthParam::Ranges::DefaultIncrement, SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterAttack, SynthParam::Name::FilterAttack, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterAttack, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterDecay, SynthParam::Name::FilterDecay, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterDecay, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterSustain, SynthParam::Name::FilterSustain, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterSustain, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
    {SynthParam::ID::FilterRelease, SynthParam::Name::FilterRelease, SynthParam::Units::Percent,
     SynthParam::Defaults::FilterRelease, SynthParam::Ranges::DefaultMin,
     SynthParam::Ranges::DefaultMax, SynthParam::Ranges::DefaultIncrement,
     SynthParam::Ranges::DefaultSkew},
};

class ParameterManagerAdaptor
{
  public:
    ParameterManagerAdaptor(IParameterState &paramState, IProgramState &progState,
                            juce::AudioProcessor &processor)
        : parameterState(paramState), programState(progState),
          paramManager(processor, "SynthParams", Parameters)
    {
        setupParameterCallbacks();
    }

    juce::String getEngineParameterId(const size_t index)
    {
        switch (index)
        {
        case SELF_OSC_PUSH:
            return SynthParam::ID::SelfOscPush;
        case ENV_PITCH_BOTH:
            return SynthParam::ID::EnvPitchBoth;
        case FENV_INVERT:
            return SynthParam::ID::FenvInvert;
        case PW_OSC2_OFS:
            return SynthParam::ID::PwOsc2Ofs;
        case LEVEL_DIF:
            return SynthParam::ID::LevelDif;
        case PW_ENV_BOTH:
            return SynthParam::ID::PwEnvBoth;
        case PW_ENV:
            return SynthParam::ID::PwEnv;
        case LFO_SYNC:
            return SynthParam::ID::LfoSync;
        case ECONOMY_MODE:
            return SynthParam::ID::EconomyMode;
        case UNLEARN:
            return SynthParam::ID::MidiUnlearn;
        case MIDILEARN:
            return SynthParam::ID::MidiLearn;
        case VAMPENV:
            return SynthParam::ID::VAmpFactor;
        case VFLTENV:
            return SynthParam::ID::VFltFactor;
        case ASPLAYEDALLOCATION:
            return SynthParam::ID::AsPlayedAllocation;
        case BENDLFORATE:
            return SynthParam::ID::VibratoRate;
        case FOURPOLE:
            return SynthParam::ID::FourPole;
        case LEGATOMODE:
            return SynthParam::ID::LegatoMode;
        case ENVPITCH:
            return SynthParam::ID::EnvelopeToPitch;
        case OSCQuantize:
            return SynthParam::ID::PitchQuant;
        case VOICE_COUNT:
            return SynthParam::ID::VoiceCount;
        case BANDPASS:
            return SynthParam::ID::BandpassBlend;
        case FILTER_WARM:
            return SynthParam::ID::FilterWarm;
        case BENDRANGE:
            return SynthParam::ID::BendRange;
        case BENDOSC2:
            return SynthParam::ID::BendOsc2Only;
        case OCTAVE:
            return SynthParam::ID::Octave;
        case TUNE:
            return SynthParam::ID::Tune;
        case BRIGHTNESS:
            return SynthParam::ID::Brightness;
        case NOISEMIX:
            return SynthParam::ID::NoiseMix;
        case OSC1MIX:
            return SynthParam::ID::Osc1Mix;
        case OSC2MIX:
            return SynthParam::ID::Osc2Mix;
        case MULTIMODE:
            return SynthParam::ID::Multimode;
        case LFOSHWAVE:
            return SynthParam::ID::LfoSampleHoldWave;
        case LFOSINWAVE:
            return SynthParam::ID::LfoSineWave;
        case LFOSQUAREWAVE:
            return SynthParam::ID::LfoSquareWave;
        case LFO1AMT:
            return SynthParam::ID::LfoAmount1;
        case LFO2AMT:
            return SynthParam::ID::LfoAmount2;
        case LFOFILTER:
            return SynthParam::ID::LfoFilter;
        case LFOOSC1:
            return SynthParam::ID::LfoOsc1;
        case LFOOSC2:
            return SynthParam::ID::LfoOsc2;
        case LFOFREQ:
            return SynthParam::ID::LfoFrequency;
        case LFOPW1:
            return SynthParam::ID::LfoPw1;
        case LFOPW2:
            return SynthParam::ID::LfoPw2;
        case PORTADER:
            return SynthParam::ID::PortamentoDetune;
        case FILTERDER:
            return SynthParam::ID::FilterDetune;
        case ENVDER:
            return SynthParam::ID::EnvelopeDetune;
        case PAN1:
            return SynthParam::ID::Pan1;
        case PAN2:
            return SynthParam::ID::Pan2;
        case PAN3:
            return SynthParam::ID::Pan3;
        case PAN4:
            return SynthParam::ID::Pan4;
        case PAN5:
            return SynthParam::ID::Pan5;
        case PAN6:
            return SynthParam::ID::Pan6;
        case PAN7:
            return SynthParam::ID::Pan7;
        case PAN8:
            return SynthParam::ID::Pan8;
        case XMOD:
            return SynthParam::ID::Xmod;
        case OSC2HS:
            return SynthParam::ID::Osc2HardSync;
        case OSC1P:
            return SynthParam::ID::Osc1Pitch;
        case OSC2P:
            return SynthParam::ID::Osc2Pitch;
        case PORTAMENTO:
            return SynthParam::ID::Portamento;
        case UNISON:
            return SynthParam::ID::Unison;
        case FLT_KF:
            return SynthParam::ID::FilterKeyFollow;
        case PW:
            return SynthParam::ID::PulseWidth;
        case OSC2Saw:
            return SynthParam::ID::Osc2Saw;
        case OSC1Saw:
            return SynthParam::ID::Osc1Saw;
        case OSC1Pul:
            return SynthParam::ID::Osc1Pulse;
        case OSC2Pul:
            return SynthParam::ID::Osc2Pulse;
        case VOLUME:
            return SynthParam::ID::Volume;
        case UDET:
            return SynthParam::ID::VoiceDetune;
        case OSC2_DET:
            return SynthParam::ID::Oscillator2detune;
        case CUTOFF:
            return SynthParam::ID::Cutoff;
        case RESONANCE:
            return SynthParam::ID::Resonance;
        case ENVELOPE_AMT:
            return SynthParam::ID::FilterEnvAmount;
        case LATK:
            return SynthParam::ID::Attack;
        case LDEC:
            return SynthParam::ID::Decay;
        case LSUS:
            return SynthParam::ID::Sustain;
        case LREL:
            return SynthParam::ID::Release;
        case FATK:
            return SynthParam::ID::FilterAttack;
        case FDEC:
            return SynthParam::ID::FilterDecay;
        case FSUS:
            return SynthParam::ID::FilterSustain;
        case FREL:
            return SynthParam::ID::FilterRelease;

        default:
            break;
        }

        return "Undefined";
    }

    int getParameterIndexFromId(const juce::String &paramId)
    {
        for (size_t i = 0; i < PARAM_COUNT; ++i)
        {
            if (paramId.compare(getEngineParameterId(i)) == 0)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    void setEngineParameterValue(SynthEngine &synth, const int index, const float newValue,
                                 const bool notifyToHost = false)
    {
        if (!parameterState.getMidiControlledParamSet() || index == MIDILEARN || index == UNLEARN)
            programState.updateProgramValue(index, newValue);

        if (engine != &synth)
        {
            setEngine(synth);
        }

        const juce::String paramId = getEngineParameterId(index);
        auto *param = paramManager.getAPVTS().getParameter(paramId);
        if (param == nullptr)
        {
            return;
        }

        if (notifyToHost)
            param->setValueNotifyingHost(newValue);
        else
            param->setValue(newValue);

        if (parameterState.getIsHostAutomatedChange())
            parameterState.sendChangeMessage();
    }

    juce::AudioProcessorValueTreeState &getValueTreeState() { return paramManager.getAPVTS(); }

    void updateParameters(bool force = false) { paramManager.updateParameters(force); }

    void setEngine(SynthEngine &synth)
    {
        engine = &synth;
        setupParameterCallbacks();
    }

    void flushFIFO() { paramManager.flushParameterQueue(); }

    void clearFIFO() { paramManager.clearFiFO(); }

  private:
    void setupParameterCallbacks()
    {
        for (size_t i = 0; i < PARAM_COUNT; ++i)
        {
            const juce::String paramId = getEngineParameterId(i);
            const auto index = static_cast<int>(i);

            paramManager.registerParameterCallback(
                paramId, [this, index](const float newValue, bool /*forced*/) {
                    if (this->engine)
                    {
                        processParameterChange(*this->engine, index, newValue);
                        this->programState.updateProgramValue(index, newValue);
                    }
                });
        }
    }

    void processParameterChange(SynthEngine &synth, const int index, const float newValue)
    {
        switch (index)
        {
        case SELF_OSC_PUSH:
            synth.processSelfOscPush(newValue);
            break;
        case PW_ENV_BOTH:
            synth.processPwEnvBoth(newValue);
            break;
        case PW_OSC2_OFS:
            synth.processPwOfs(newValue);
            break;
        case ENV_PITCH_BOTH:
            synth.processPitchModBoth(newValue);
            break;
        case FENV_INVERT:
            synth.processInvertFenv(newValue);
            break;
        case LEVEL_DIF:
            synth.processLoudnessDetune(newValue);
            break;
        case PW_ENV:
            synth.processPwEnv(newValue);
            break;
        case LFO_SYNC:
            synth.procLfoSync(newValue);
            break;
        case ECONOMY_MODE:
            synth.procEconomyMode(newValue);
            break;
        case VAMPENV:
            synth.procAmpVelocityAmount(newValue);
            break;
        case VFLTENV:
            synth.procFltVelocityAmount(newValue);
            break;
        case ASPLAYEDALLOCATION:
            synth.procAsPlayedAlloc(newValue);
            break;
        case BENDLFORATE:
            synth.procModWheelFrequency(newValue);
            break;
        case FOURPOLE:
            synth.processFourPole(newValue);
            break;
        case LEGATOMODE:
            synth.processLegatoMode(newValue);
            break;
        case ENVPITCH:
            synth.processEnvelopeToPitch(newValue);
            break;
        case OSCQuantize:
            synth.processPitchQuantization(newValue);
            break;
        case VOICE_COUNT:
            synth.setVoiceCount(newValue);
            break;
        case BANDPASS:
            synth.processBandpassSw(newValue);
            break;
        case FILTER_WARM:
            synth.processOversampling(newValue);
            break;
        case BENDOSC2:
            synth.procPitchWheelOsc2Only(newValue);
            break;
        case BENDRANGE:
            synth.procPitchWheelAmount(newValue);
            break;
        case NOISEMIX:
            synth.processNoiseMix(newValue);
            break;
        case OCTAVE:
            synth.processOctave(newValue);
            break;
        case TUNE:
            synth.processTune(newValue);
            break;
        case BRIGHTNESS:
            synth.processBrightness(newValue);
            break;
        case MULTIMODE:
            synth.processMultimode(newValue);
            break;
        case LFOFREQ:
            synth.processLfoFrequency(newValue);
            break;
        case LFO1AMT:
            synth.processLfoAmt1(newValue);
            break;
        case LFO2AMT:
            synth.processLfoAmt2(newValue);
            break;
        case LFOSINWAVE:
            synth.processLfoSine(newValue);
            break;
        case LFOSQUAREWAVE:
            synth.processLfoSquare(newValue);
            break;
        case LFOSHWAVE:
            synth.processLfoSH(newValue);
            break;
        case LFOFILTER:
            synth.processLfoFilter(newValue);
            break;
        case LFOOSC1:
            synth.processLfoOsc1(newValue);
            break;
        case LFOOSC2:
            synth.processLfoOsc2(newValue);
            break;
        case LFOPW1:
            synth.processLfoPw1(newValue);
            break;
        case LFOPW2:
            synth.processLfoPw2(newValue);
            break;
        case PORTADER:
            synth.processPortamentoDetune(newValue);
            break;
        case FILTERDER:
            synth.processFilterDetune(newValue);
            break;
        case ENVDER:
            synth.processEnvelopeDetune(newValue);
            break;
        case XMOD:
            synth.processOsc2Xmod(newValue);
            break;
        case OSC2HS:
            synth.processOsc2HardSync(newValue);
            break;
        case OSC2P:
            synth.processOsc2Pitch(newValue);
            break;
        case OSC1P:
            synth.processOsc1Pitch(newValue);
            break;
        case PORTAMENTO:
            synth.processPortamento(newValue);
            break;
        case UNISON:
            synth.processUnison(newValue);
            break;
        case FLT_KF:
            synth.processFilterKeyFollow(newValue);
            break;
        case OSC1MIX:
            synth.processOsc1Mix(newValue);
            break;
        case OSC2MIX:
            synth.processOsc2Mix(newValue);
            break;
        case PW:
            synth.processPulseWidth(newValue);
            break;
        case OSC1Saw:
            synth.processOsc1Saw(newValue);
            break;
        case OSC2Saw:
            synth.processOsc2Saw(newValue);
            break;
        case OSC1Pul:
            synth.processOsc1Pulse(newValue);
            break;
        case OSC2Pul:
            synth.processOsc2Pulse(newValue);
            break;
        case VOLUME:
            synth.processVolume(newValue);
            break;
        case UDET:
            synth.processDetune(newValue);
            break;
        case OSC2_DET:
            synth.processOsc2Det(newValue);
            break;
        case CUTOFF:
            synth.processCutoff(newValue);
            break;
        case RESONANCE:
            synth.processResonance(newValue);
            break;
        case ENVELOPE_AMT:
            synth.processFilterEnvelopeAmt(newValue);
            break;
        case LATK:
            synth.processLoudnessEnvelopeAttack(newValue);
            break;
        case LDEC:
            synth.processLoudnessEnvelopeDecay(newValue);
            break;
        case LSUS:
            synth.processLoudnessEnvelopeSustain(newValue);
            break;
        case LREL:
            synth.processLoudnessEnvelopeRelease(newValue);
            break;
        case FATK:
            synth.processFilterEnvelopeAttack(newValue);
            break;
        case FDEC:
            synth.processFilterEnvelopeDecay(newValue);
            break;
        case FSUS:
            synth.processFilterEnvelopeSustain(newValue);
            break;
        case FREL:
            synth.processFilterEnvelopeRelease(newValue);
            break;
        case PAN1:
            synth.processPan(newValue, 1);
            break;
        case PAN2:
            synth.processPan(newValue, 2);
            break;
        case PAN3:
            synth.processPan(newValue, 3);
            break;
        case PAN4:
            synth.processPan(newValue, 4);
            break;
        case PAN5:
            synth.processPan(newValue, 5);
            break;
        case PAN6:
            synth.processPan(newValue, 6);
            break;
        case PAN7:
            synth.processPan(newValue, 7);
            break;
        case PAN8:
            synth.processPan(newValue, 8);
            break;
        default:
            break;
        }
    }

    IParameterState &parameterState;
    IProgramState &programState;
    ParameterManager paramManager;
    SynthEngine *engine = nullptr;
};
