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

#ifndef OBXF_SRC_PARAMETER_PARAMETERADAPTOR_H
#define OBXF_SRC_PARAMETER_PARAMETERADAPTOR_H

#include <juce_audio_basics/juce_audio_basics.h>
#include "engine/SynthEngine.h"

#include "IParameterState.h"
#include "IProgramState.h"
#include "SynthParam.h"
#include "ParameterManager.h"

using namespace SynthParam;

static const std::vector<ParameterInfo> Parameters{
    {ID::Undefined, Name::Undefined, Units::None, Defaults::Undefined, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::SelfOscPush, Name::SelfOscPush, Units::None, Defaults::SelfOscPush, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::EnvPitchBoth, Name::EnvPitchBoth, Units::None, Defaults::EnvPitchBoth, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::FenvInvert, Name::FenvInvert, Units::None, Defaults::FenvInvert, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::PwOsc2Ofs, Name::PwOsc2Ofs, Units::None, Defaults::PwOsc2Ofs, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LevelDif, Name::LevelDif, Units::Percent, Defaults::LevelDif, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::PwEnvBoth, Name::PwEnvBoth, Units::None, Defaults::PwEnvBoth, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::PwEnv, Name::PwEnv, Units::Percent, Defaults::PwEnv, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoSync, Name::LfoSync, Units::None, Defaults::LfoSync, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::EconomyMode, Name::EconomyMode, Units::None, Defaults::EconomyMode, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::MidiUnlearn, Name::MidiUnlearn, Units::None, Defaults::MidiUnlearn, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::MidiLearn, Name::MidiLearn, Units::None, Defaults::MidiLearn, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::VAmpFactor, Name::VAmpFactor, Units::Percent, Defaults::VAmpFactor, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::VFltFactor, Name::VFltFactor, Units::Percent, Defaults::VFltFactor, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::AsPlayedAllocation, Name::AsPlayedAllocation, Units::None, Defaults::AsPlayedAllocation,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::VibratoRate, Name::VibratoRate, Units::Hz, Defaults::VibratoRate, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::FourPole, Name::FourPole, Units::None, Defaults::FourPole, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LegatoMode, Name::LegatoMode, Units::None, Defaults::LegatoMode, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::EnvelopeToPitch, Name::EnvelopeToPitch, Units::Semitones, Defaults::EnvelopeToPitch,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::EnvelopeToPitchInv, Name::EnvelopeToPitchInv, Units::None, Defaults::EnvelopeToPitchInv,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::VoiceCount, Name::VoiceCount, Units::None, Defaults::VoiceCount, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::BandpassBlend, Name::BandpassBlend, Units::None, Defaults::BandpassBlend,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::FilterWarm, Name::FilterWarm, Units::None, Defaults::FilterWarm, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::BendRange, Name::BendRange, Units::Percent, Defaults::BendRange, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::BendOsc2Only, Name::BendOsc2Only, Units::None, Defaults::BendOsc2Only, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Transpose, Name::Transpose, Units::Percent, Defaults::Transpose, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::OscTuneIncrement, Ranges::DefaultSkew},
    {ID::Tune, Name::Tune, Units::Cents, Defaults::Tune, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Brightness, Name::Brightness, Units::Percent, Defaults::Brightness, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::NoiseMix, Name::NoiseMix, Units::Percent, Defaults::NoiseMix, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Osc2Mix, Name::Osc2Mix, Units::Percent, Defaults::Osc2Mix, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Osc1Mix, Name::Osc1Mix, Units::Percent, Defaults::Osc1Mix, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Multimode, Name::Multimode, Units::Percent, Defaults::Multimode, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::LfoSampleHoldWave, Name::LfoSampleHoldWave, Units::Percent, Defaults::LfoSampleHoldWave,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoSineWave, Name::LfoSineWave, Units::Percent, Defaults::LfoSineWave, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoSquareWave, Name::LfoSquareWave, Units::Percent, Defaults::LfoSquareWave,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoAmount1, Name::LfoAmount1, Units::Percent, Defaults::LfoAmount1, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::LfoAmount2, Name::LfoAmount2, Units::Percent, Defaults::LfoAmount2, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::LfoFilter, Name::LfoFilter, Units::Percent, Defaults::LfoFilter, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoOsc1, Name::LfoOsc1, Units::Percent, Defaults::LfoOsc1, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoOsc2, Name::LfoOsc2, Units::Percent, Defaults::LfoOsc2, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoFrequency, Name::LfoFrequency, Units::Hz, Defaults::LfoFrequency, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::LfoPw1, Name::LfoPw1, Units::Percent, Defaults::LfoPw1, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::LfoPw2, Name::LfoPw2, Units::Percent, Defaults::LfoPw2, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::PortamentoDetune, Name::PortamentoDetune, Units::Percent, Defaults::PortamentoDetune,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::FilterDetune, Name::FilterDetune, Units::Percent, Defaults::FilterDetune,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::EnvelopeDetune, Name::EnvelopeDetune, Units::Percent, Defaults::EnvelopeDetune,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Pan1, Name::Pan1, Units::Percent, Defaults::Pan1, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan2, Name::Pan2, Units::Percent, Defaults::Pan2, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan3, Name::Pan3, Units::Percent, Defaults::Pan3, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan4, Name::Pan4, Units::Percent, Defaults::Pan4, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan5, Name::Pan5, Units::Percent, Defaults::Pan5, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan6, Name::Pan6, Units::Percent, Defaults::Pan6, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan7, Name::Pan7, Units::Percent, Defaults::Pan7, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Pan8, Name::Pan8, Units::Percent, Defaults::Pan8, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Xmod, Name::Xmod, Units::Percent, Defaults::Xmod, Ranges::DefaultMin, Ranges::DefaultMax,
     Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Osc2HardSync, Name::Osc2HardSync, Units::Percent, Defaults::Osc2HardSync,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Osc1Pitch, Name::Osc1Pitch, Units::Percent, Defaults::Osc1Pitch, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::OscTuneIncrement, Ranges::DefaultSkew},
    {ID::Osc2Pitch, Name::Osc2Pitch, Units::Percent, Defaults::Osc2Pitch, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::OscTuneIncrement, Ranges::DefaultSkew},
    {ID::Portamento, Name::Portamento, Units::Percent, Defaults::Portamento, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Unison, Name::Unison, Units::Percent, Defaults::Unison, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::FilterKeyFollow, Name::FilterKeyFollow, Units::Percent, Defaults::FilterKeyFollow,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::PulseWidth, Name::PulseWidth, Units::Percent, Defaults::PulseWidth, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Osc1Saw, Name::Osc1Saw, Units::Percent, Defaults::Osc1Saw, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Osc2Saw, Name::Osc2Saw, Units::Percent, Defaults::Osc2Saw, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Osc1Pulse, Name::Osc1Pulse, Units::Percent, Defaults::Osc1Pulse, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Osc2Pulse, Name::Osc2Pulse, Units::Percent, Defaults::Osc2Pulse, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::DefaultIncrement, Ranges::DefaultSkew},
    {ID::Volume, Name::Volume, Units::Percent, Defaults::Volume, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::VoiceDetune, Name::VoiceDetune, Units::Percent, Defaults::VoiceDetune, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Oscillator2Detune, Name::Oscillator2Detune, Units::Cents, Defaults::Oscillator2Detune,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Cutoff, Name::Cutoff, Units::Percent, Defaults::Cutoff, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Resonance, Name::Resonance, Units::Percent, Defaults::Resonance, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::FilterEnvAmount, Name::FilterEnvAmount, Units::Percent, Defaults::FilterEnvAmount,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Attack, Name::Attack, Units::Percent, Defaults::Attack, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Decay, Name::Decay, Units::Percent, Defaults::Decay, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Sustain, Name::Sustain, Units::Percent, Defaults::Sustain, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::Release, Name::Release, Units::Percent, Defaults::Release, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::FilterAttack, Name::FilterAttack, Units::Percent, Defaults::FilterAttack,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::FilterDecay, Name::FilterDecay, Units::Percent, Defaults::FilterDecay, Ranges::DefaultMin,
     Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::FilterSustain, Name::FilterSustain, Units::Percent, Defaults::FilterSustain,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
    {ID::FilterRelease, Name::FilterRelease, Units::Percent, Defaults::FilterRelease,
     Ranges::DefaultMin, Ranges::DefaultMax, Ranges::Continuous, Ranges::DefaultSkew},
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
            return ID::SelfOscPush;
        case ENV_PITCH_BOTH:
            return ID::EnvPitchBoth;
        case FENV_INVERT:
            return ID::FenvInvert;
        case PW_OSC2_OFS:
            return ID::PwOsc2Ofs;
        case LEVEL_DIF:
            return ID::LevelDif;
        case PW_ENV_BOTH:
            return ID::PwEnvBoth;
        case PW_ENV:
            return ID::PwEnv;
        case LFO_SYNC:
            return ID::LfoSync;
        case ECONOMY_MODE:
            return ID::EconomyMode;
        case UNLEARN:
            return ID::MidiUnlearn;
        case MIDILEARN:
            return ID::MidiLearn;
        case VAMPENV:
            return ID::VAmpFactor;
        case VFLTENV:
            return ID::VFltFactor;
        case ASPLAYEDALLOCATION:
            return ID::AsPlayedAllocation;
        case BENDLFORATE:
            return ID::VibratoRate;
        case FOURPOLE:
            return ID::FourPole;
        case LEGATOMODE:
            return ID::LegatoMode;
        case ENVPITCH:
            return ID::EnvelopeToPitch;
        case ENVPITCHINV:
            return ID::EnvelopeToPitchInv;
        case VOICE_COUNT:
            return ID::VoiceCount;
        case BANDPASS:
            return ID::BandpassBlend;
        case FILTER_WARM:
            return ID::FilterWarm;
        case BENDRANGE:
            return ID::BendRange;
        case BENDOSC2:
            return ID::BendOsc2Only;
        case OCTAVE:
            return ID::Transpose;
        case TUNE:
            return ID::Tune;
        case BRIGHTNESS:
            return ID::Brightness;
        case NOISEMIX:
            return ID::NoiseMix;
        case OSC1MIX:
            return ID::Osc1Mix;
        case OSC2MIX:
            return ID::Osc2Mix;
        case MULTIMODE:
            return ID::Multimode;
        case LFOSHWAVE:
            return ID::LfoSampleHoldWave;
        case LFOSINWAVE:
            return ID::LfoSineWave;
        case LFOSQUAREWAVE:
            return ID::LfoSquareWave;
        case LFO1AMT:
            return ID::LfoAmount1;
        case LFO2AMT:
            return ID::LfoAmount2;
        case LFOFILTER:
            return ID::LfoFilter;
        case LFOOSC1:
            return ID::LfoOsc1;
        case LFOOSC2:
            return ID::LfoOsc2;
        case LFOFREQ:
            return ID::LfoFrequency;
        case LFOPW1:
            return ID::LfoPw1;
        case LFOPW2:
            return ID::LfoPw2;
        case PORTADER:
            return ID::PortamentoDetune;
        case FILTERDER:
            return ID::FilterDetune;
        case ENVDER:
            return ID::EnvelopeDetune;
        case PAN1:
            return ID::Pan1;
        case PAN2:
            return ID::Pan2;
        case PAN3:
            return ID::Pan3;
        case PAN4:
            return ID::Pan4;
        case PAN5:
            return ID::Pan5;
        case PAN6:
            return ID::Pan6;
        case PAN7:
            return ID::Pan7;
        case PAN8:
            return ID::Pan8;
        case XMOD:
            return ID::Xmod;
        case OSC2HS:
            return ID::Osc2HardSync;
        case OSC1P:
            return ID::Osc1Pitch;
        case OSC2P:
            return ID::Osc2Pitch;
        case PORTAMENTO:
            return ID::Portamento;
        case UNISON:
            return ID::Unison;
        case FLT_KF:
            return ID::FilterKeyFollow;
        case PW:
            return ID::PulseWidth;
        case OSC2Saw:
            return ID::Osc2Saw;
        case OSC1Saw:
            return ID::Osc1Saw;
        case OSC1Pul:
            return ID::Osc1Pulse;
        case OSC2Pul:
            return ID::Osc2Pulse;
        case VOLUME:
            return ID::Volume;
        case UDET:
            return ID::VoiceDetune;
        case OSC2_DET:
            return ID::Oscillator2Detune;
        case CUTOFF:
            return ID::Cutoff;
        case RESONANCE:
            return ID::Resonance;
        case ENVELOPE_AMT:
            return ID::FilterEnvAmount;
        case LATK:
            return ID::Attack;
        case LDEC:
            return ID::Decay;
        case LSUS:
            return ID::Sustain;
        case LREL:
            return ID::Release;
        case FATK:
            return ID::FilterAttack;
        case FDEC:
            return ID::FilterDecay;
        case FSUS:
            return ID::FilterSustain;
        case FREL:
            return ID::FilterRelease;

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
            synth.processLoudnessSlop(newValue);
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
        case ENVPITCHINV:
            synth.processEnvelopeToPitchInvert(newValue);
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
            synth.processPortamentoSlop(newValue);
            break;
        case FILTERDER:
            synth.processFilterSlop(newValue);
            break;
        case ENVDER:
            synth.processEnvelopeSlop(newValue);
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

#endif // OBXF_SRC_PARAMETER_PARAMETERADAPTOR_H
