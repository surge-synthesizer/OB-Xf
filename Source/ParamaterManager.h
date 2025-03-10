#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Engine/SynthEngine.h"

class IProgramState
{
public:
    virtual ~IProgramState() = default;
    virtual void updateProgramValue(int index, float value) = 0;
    virtual juce::AudioProcessorValueTreeState& getValueTreeState() = 0;
};

class IParameterState : virtual public juce::ChangeBroadcaster
{
public:
    ~IParameterState() override = default;
    [[nodiscard]] virtual bool getMidiControlledParamSet() const = 0;
    virtual void setLastUsedParameter(int param) = 0;
    [[nodiscard]] virtual bool getIsHostAutomatedChange() const = 0;
};

class ParameterManager
{
public:

    ParameterManager(IParameterState& paramState, IProgramState& progState)
    : parameterState(paramState)
    , programState(progState)
    {}

    static juce::String getEngineParameterId(size_t index)
    {
    switch (index)
	{
        case SELF_OSC_PUSH:      return "SelfOscPush";
        case ENV_PITCH_BOTH:     return "EnvPitchBoth";
        case FENV_INVERT:        return "FenvInvert";
		case PW_OSC2_OFS:        return "PwOfs";
        case LEVEL_DIF:          return "LevelDif";
        case PW_ENV_BOTH:        return "PwEnvBoth";
		case PW_ENV:             return "PwEnv";
	    case LFO_SYNC:           return "LfoSync";
        case ECONOMY_MODE:       return "EconomyMode";
	    case UNLEARN:            return "MidiUnlearn";
        case MIDILEARN:          return "MidiLearn";
	    case VAMPENV:            return "VAmpFactor";
        case VFLTENV:            return "VFltFactor";
	    case ASPLAYEDALLOCATION: return "AsPlayedAllocation";
	    case BENDLFORATE:        return "VibratoRate";
	    case FOURPOLE:           return "FourPole";
	    case LEGATOMODE:         return "LegatoMode";
	    case ENVPITCH:           return "EnvelopeToPitch";
	    case OSCQuantize:        return "PitchQuant";
        case VOICE_COUNT:        return "VoiceCount";
	    case BANDPASS:           return "BandpassBlend";
	    case FILTER_WARM:        return "Filter_Warm";
	    case BENDRANGE:          return "BendRange";
        case BENDOSC2:           return "BendOsc2Only";
        case OCTAVE:             return "Octave";
        case TUNE:               return "Tune";
        case BRIGHTNESS:         return "Brightness";
        case NOISEMIX:           return "NoiseMix";
        case OSC1MIX:            return "Osc1Mix";
        case OSC2MIX:            return "Osc2Mix";
        case MULTIMODE:          return "Multimode";
        case LFOSHWAVE:          return "LfoSampleHoldWave";
        case LFOSINWAVE:         return "LfoSineWave";
        case LFOSQUAREWAVE:      return "LfoSquareWave";
        case LFO1AMT:            return "LfoAmount1";
        case LFO2AMT:            return "LfoAmount2";
        case LFOFILTER:          return "LfoFilter";
        case LFOOSC1:            return "LfoOsc1";
        case LFOOSC2:            return "LfoOsc2";
        case LFOFREQ:            return "LfoFrequency";
        case LFOPW1:             return "LfoPw1";
        case LFOPW2:             return "LfoPw2";
        case PORTADER:           return "PortamentoDetune";
        case FILTERDER:          return "FilterDetune";
        case ENVDER:             return "EnvelopeDetune";
        case PAN1:               return "Pan1";
        case PAN2:               return "Pan2";
        case PAN3:               return "Pan3";
        case PAN4:               return "Pan4";
        case PAN5:               return "Pan5";
        case PAN6:               return "Pan6";
        case PAN7:               return "Pan7";
        case PAN8:               return "Pan8";
        case XMOD:               return "Xmod";
        case OSC2HS:             return "Osc2HardSync";
        case OSC1P:              return "Osc1Pitch";
        case OSC2P:              return "Osc2Pitch";
        case PORTAMENTO:         return "Portamento";
        case UNISON:             return "Unison";
        case FLT_KF:             return "FilterKeyFollow";
        case PW:                 return "PulseWidth";
        case OSC2Saw:            return "Osc2Saw";
        case OSC1Saw:            return "Osc1Saw";
        case OSC1Pul:            return "Osc1Pulse";
        case OSC2Pul:            return "Osc2Pulse";
        case VOLUME:             return "Volume";
        case UDET:               return "VoiceDetune";
        case OSC2_DET:           return "Oscillator2detune";
        case CUTOFF:             return "Cutoff";
        case RESONANCE:          return "Resonance";
        case ENVELOPE_AMT:       return "FilterEnvAmount";
        case LATK:               return "Attack";
        case LDEC:               return "Decay";
        case LSUS:               return "Sustain";
        case LREL:               return "Release";
        case FATK:               return "FilterAttack";
        case FDEC:               return "FilterDecay";
        case FSUS:               return "FilterSustain";
        case FREL:               return "FilterRelease";

        default:
            break;
    }

    return "Undefined";
    }

    static juce::String getTrueParameterValueFromNormalizedRange(const size_t index,const float value)
    {
    switch (index)
    {
    case BENDLFORATE:        return juce::String{ logsc(value, 3, 10), 2 } + " Hz";
    case OCTAVE:             return juce::String{ (juce::roundToInt(value * 4) - 2) * 12.f, 0 } + " Semitones";
    case TUNE:               return juce::String{ value * 200 - 100, 1 } + " Cents";
    case NOISEMIX: {
        const auto decibels = juce::Decibels::gainToDecibels(logsc(value, 0, 1, 35));
        if (decibels < -80) return "-Inf";
        return juce::String{ decibels, 2 } + " dB";
    }
    case OSC1MIX:
    case OSC2MIX: {
        const auto decibels = juce::Decibels::gainToDecibels(value);
        if (decibels < -80) return "-Inf";
        return juce::String{ decibels, 2 } + " dB";
    }
    case LFOFREQ:            return juce::String{ logsc(value, 0, 50, 120), 2 } + " Hz";
    case PAN1:
    case PAN2:
    case PAN3:
    case PAN4:
    case PAN5:
    case PAN6:
    case PAN7:
    case PAN8: {
		const auto pan = value - 0.5f;
        if (pan < 0.f) return juce::String{ pan, 2 } + " (Left)";
        if (pan > 0.f) return juce::String{ pan, 2 } + " (Right)";
        return juce::String{ pan, 2 } + " (Center)";
    }
    case OSC1P:              return juce::String{ (float(value * 4) - 2) * 12.f, 1 } + " Semitones";
    case OSC2P:              return juce::String{ (float(value * 4) - 2) * 12.f, 1 } + " Semitones";


    default:
        break;
    }

    return juce::String{ static_cast<int>(juce::jmap(value, 0.f, 127.f)) };
    }

   static int getParameterIndexFromId (const juce::String& paramId)
    {
        for (size_t i = 0; i < PARAM_COUNT; ++i)
        {
            if (paramId.compare (getEngineParameterId (i)) == 0)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

void setEngineParameterValue(SynthEngine& synth, const int index, const float newValue, const bool notifyToHost = false) const {

        if (!parameterState.getMidiControlledParamSet() || index == MIDILEARN || index == UNLEARN)

    programState.updateProgramValue(index, newValue);

        const auto& apvtState = programState.getValueTreeState();
        if (notifyToHost)
            apvtState.getParameter(getEngineParameterId(index))->setValueNotifyingHost(newValue);
        else
            apvtState.getParameter(getEngineParameterId(index))->setValue(newValue);


    //DBG("Set Value Parameter: " << getEngineParameterId(index) << " Val: " << newValue);
    switch (index)
    {
        case SELF_OSC_PUSH:
            synth.processSelfOscPush (newValue);
            break;
        case PW_ENV_BOTH:
            synth.processPwEnvBoth (newValue);
            break;
        case PW_OSC2_OFS:
            synth.processPwOfs (newValue);
            break;
        case ENV_PITCH_BOTH:
            synth.processPitchModBoth (newValue);
            break;
        case FENV_INVERT:
            synth.processInvertFenv (newValue);
            break;
        case LEVEL_DIF:
            synth.processLoudnessDetune (newValue);
            break;
        case PW_ENV:
            synth.processPwEnv (newValue);
            break;
        case LFO_SYNC:
            synth.procLfoSync (newValue);
            break;
        case ECONOMY_MODE:
            synth.procEconomyMode (newValue);
            break;
        case VAMPENV:
            synth.procAmpVelocityAmount (newValue);
            break;
        case VFLTENV:
            synth.procFltVelocityAmount (newValue);
            break;
        case ASPLAYEDALLOCATION:
            synth.procAsPlayedAlloc (newValue);
            break;
        case BENDLFORATE:
            synth.procModWheelFrequency (newValue);
            break;
        case FOURPOLE:
            synth.processFourPole (newValue);
            break;
        case LEGATOMODE:
            synth.processLegatoMode (newValue);
            break;
        case ENVPITCH:
            synth.processEnvelopeToPitch (newValue);
            break;
        case OSCQuantize:
            synth.processPitchQuantization (newValue);
            break;
        case VOICE_COUNT:
            synth.setVoiceCount (newValue);
            break;
        case BANDPASS:
            synth.processBandpassSw (newValue);
            break;
        case FILTER_WARM:
            synth.processOversampling (newValue);
            break;
        case BENDOSC2:
            synth.procPitchWheelOsc2Only (newValue);
            break;
        case BENDRANGE:
            synth.procPitchWheelAmount (newValue);
            break;
        case NOISEMIX:
            synth.processNoiseMix (newValue);
            break;
        case OCTAVE:
            synth.processOctave (newValue);
            break;
        case TUNE:
            synth.processTune (newValue);
            break;
        case BRIGHTNESS:
            synth.processBrightness (newValue);
            break;
        case MULTIMODE:
            synth.processMultimode (newValue);
            break;
        case LFOFREQ:
            synth.processLfoFrequency (newValue);
            break;
        case LFO1AMT:
            synth.processLfoAmt1 (newValue);
            break;
        case LFO2AMT:
            synth.processLfoAmt2 (newValue);
            break;
        case LFOSINWAVE:
            synth.processLfoSine (newValue);
            break;
        case LFOSQUAREWAVE:
            synth.processLfoSquare (newValue);
            break;
        case LFOSHWAVE:
            synth.processLfoSH (newValue);
            break;
        case LFOFILTER:
            synth.processLfoFilter (newValue);
            break;
        case LFOOSC1:
            synth.processLfoOsc1 (newValue);
            break;
        case LFOOSC2:
            synth.processLfoOsc2 (newValue);
            break;
        case LFOPW1:
            synth.processLfoPw1 (newValue);
            break;
        case LFOPW2:
            synth.processLfoPw2 (newValue);
            break;
        case PORTADER:
            synth.processPortamentoDetune (newValue);
            break;
        case FILTERDER:
            synth.processFilterDetune (newValue);
            break;
        case ENVDER:
            synth.processEnvelopeDetune (newValue);
            break;
        case XMOD:
            synth.processOsc2Xmod (newValue);
            break;
        case OSC2HS:
            synth.processOsc2HardSync (newValue);
            break;
        case OSC2P:
            synth.processOsc2Pitch (newValue);
            break;
        case OSC1P:
            synth.processOsc1Pitch (newValue);
            break;
        case PORTAMENTO:
            synth.processPortamento (newValue);
            break;
        case UNISON:
            synth.processUnison (newValue);
            break;
        case FLT_KF:
            synth.processFilterKeyFollow (newValue);
            break;
        case OSC1MIX:
            synth.processOsc1Mix (newValue);
            break;
        case OSC2MIX:
            synth.processOsc2Mix (newValue);
            break;
        case PW:
            synth.processPulseWidth (newValue);
            break;
        case OSC1Saw:
            synth.processOsc1Saw (newValue);
            break;
        case OSC2Saw:
            synth.processOsc2Saw (newValue);
            break;
        case OSC1Pul:
            synth.processOsc1Pulse (newValue);
            break;
        case OSC2Pul:
            synth.processOsc2Pulse (newValue);
            break;
        case VOLUME:
            synth.processVolume (newValue);
            break;
        case UDET:
            synth.processDetune (newValue);
            break;
        case OSC2_DET:
            synth.processOsc2Det (newValue);
            break;
        case CUTOFF:
            synth.processCutoff (newValue);
            break;
        case RESONANCE:
            synth.processResonance (newValue);
            break;
        case ENVELOPE_AMT:
            synth.processFilterEnvelopeAmt (newValue);
            break;
        case LATK:
            synth.processLoudnessEnvelopeAttack (newValue);
            break;
        case LDEC:
            synth.processLoudnessEnvelopeDecay (newValue);
            break;
        case LSUS:
            synth.processLoudnessEnvelopeSustain (newValue);
            break;
        case LREL:
            synth.processLoudnessEnvelopeRelease (newValue);
            break;
        case FATK:
            synth.processFilterEnvelopeAttack (newValue);
            break;
        case FDEC:
            synth.processFilterEnvelopeDecay (newValue);
            break;
        case FSUS:
            synth.processFilterEnvelopeSustain (newValue);
            break;
        case FREL:
            synth.processFilterEnvelopeRelease (newValue);
            break;
        case PAN1:
            synth.processPan (newValue,1);
            break;
        case PAN2:
            synth.processPan (newValue,2);
            break;
        case PAN3:
            synth.processPan (newValue,3);
            break;
        case PAN4:
            synth.processPan (newValue,4);
            break;
        case PAN5:
            synth.processPan (newValue,5);
            break;
        case PAN6:
            synth.processPan (newValue,6);
            break;
        case PAN7:
            synth.processPan (newValue,7);
            break;
        case PAN8:
            synth.processPan (newValue,8);
            break;
        default: ;
    }

    //DIRTY HACK
    //This should be checked to avoid stalling on gui update
    //It is needed because some hosts do  wierd stuff
    if (parameterState.getIsHostAutomatedChange())
        parameterState.sendChangeMessage();
}


private:
    IParameterState& parameterState;
    IProgramState& programState;
};
