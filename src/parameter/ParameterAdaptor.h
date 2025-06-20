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
#include "ValueAttachment.h"

using namespace SynthParam;

using pmd = sst::basic_blocks::params::ParamMetaData;

inline pmd customPan()
{
    // This basically sets up the obxf pan rules of showing 37L and 92R and Center
    return pmd()
        .asFloat()
        .withRange(-1, 1)
        .withLinearScaleFormatting("R", 100)
        .withDisplayRescalingBelow(0, -1, "L")
        .withDefault(0)
        .withDecimalPlaces(0)
        .withCustomDefaultDisplay("Center");
}
// clang-format off
static const std::vector<ParameterInfo> ParameterList{

    // <-- MASTER -->
    {ID::Volume, pmd().asFloat().withName(Name::Volume.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::Transpose, pmd().asFloat().withName(Name::Transpose.toStdString()).withDefault(0.f).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2)},
    {ID::Tune, pmd().asFloat().withName(Name::Tune.toStdString()).withDefault(0.f).withRange(-100.f, 100.f).withLinearScaleFormatting("cents").withDecimalPlaces(2)},
    // <-- MASTER END -->

    // <-- GLOBAL -->
    {ID::Portamento, pmd().asFloat().withName(Name::Portamento.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::VoiceDetune, pmd().asFloat().withName(Name::VoiceDetune.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::Unison, pmd().asBool().withName(Name::Unison.toStdString())},
    {ID::VoiceCount, pmd().asFloat().withName(Name::VoiceCount.toStdString()).withDefault(0.f).withRange(0.f, 1.f)},
    {ID::LegatoMode, pmd().asFloat().withName(Name::LegatoMode.toStdString()).withDefault(0.f).withRange(0.f, 1.f)},
    {ID::AsPlayedAllocation, pmd().asBool().withName(Name::AsPlayedAllocation.toStdString())},
    // GLOBAL END

    // <-- OSCILLATORS -->
    {ID::EnvelopeToPitch, pmd().asFloat().withName(Name::EnvelopeToPitch.toStdString()).withDefault(0.f).asSemitoneRange(0.f, 36.f).withDecimalPlaces(2)},
    {ID::Brightness, pmd().asFloat().withName(Name::Brightness.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::Xmod, pmd().asFloat().withName(Name::Xmod.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::Osc1Saw, pmd().asBool().withName(Name::Osc1Saw.toStdString())},
    {ID::Osc2Saw, pmd().asBool().withName(Name::Osc2Saw.toStdString())},
    {ID::Osc1Pulse, pmd().asBool().withName(Name::Osc1Pulse.toStdString())},
    {ID::Osc2Pulse, pmd().asBool().withName(Name::Osc2Pulse.toStdString())},
    {ID::Osc2HardSync, pmd().asBool().withName(Name::Osc2HardSync.toStdString())},
    {ID::EnvelopeToPitchInv, pmd().asBool().withName(Name::EnvelopeToPitchInv.toStdString())},
    {ID::Osc1Pitch, pmd().asFloat().withName(Name::Osc1Pitch.toStdString()).withDefault(0.f).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2)},
    {ID::Osc2Pitch, pmd().asFloat().withName(Name::Osc2Pitch.toStdString()).withDefault(0.f).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2)},
    {ID::Oscillator2Detune, pmd().asFloat().withName(Name::Oscillator2Detune.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(0.1f, 100.f, 0.001f, "cents").withDecimalPlaces(1)},
    {ID::PulseWidth, pmd().asFloat().withName(Name::PulseWidth.toStdString()).withDefault(0.f).withRange(0.f, 1.f).withExtendFactors(47.5f, 50.f).withLinearScaleFormatting("%").withDecimalPlaces(1)},
    {ID::LfoVolume, pmd().asBool().withName(Name::PwEnvBoth.toStdString())},
    // <-- OSCILLATORS END -->

    // <-- MODULATION -->
    {ID::LfoFrequency, pmd().withName(Name::LfoFrequency.toStdString()).withDefault(0.f).withRange(0.f, 1.f).withOBXFLogScale(0, 250, 3775.f, "Hz").withDecimalPlaces(2)},
    {ID::LfoAmount1, pmd().asFloat().withName(Name::LfoAmount1.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::LfoAmount2, pmd().asFloat().withName(Name::LfoAmount2.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::LfoSampleHoldWave, pmd().asFloat().withName(Name::LfoSampleHoldWave.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::LfoSineWave, pmd().asFloat().withName(Name::LfoSineWave.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::LfoSquareWave, pmd().asFloat().withName(Name::LfoSquareWave.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::LfoPw1, pmd().asBool().withName(Name::LfoPw1.toStdString())},
    {ID::LfoPw2, pmd().asBool().withName(Name::LfoPw2.toStdString())},
    {ID::LfoOsc1, pmd().asBool().withName(Name::LfoOsc1.toStdString())},
    {ID::LfoOsc2, pmd().asBool().withName(Name::LfoOsc2.toStdString())},
    {ID::LfoFilter, pmd().asBool().withName(Name::LfoFilter.toStdString())},
    {ID::LfoPulsewidth, pmd().asFloat().withName(Name::LfoPulsewidth.toStdString()).withRange(0.f, 1.f).withExtendFactors(45.f, 50.f).withLinearScaleFormatting("%").withDecimalPlaces(1)},
    // <-- MODULATION END -->

    // <-- FILTER -->
    {ID::Resonance, pmd().asFloat().withName(Name::Resonance.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::FilterEnvAmount, pmd().asFloat().withName(Name::FilterEnvAmount.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::FilterKeyFollow, pmd().asFloat().withName(Name::FilterKeyFollow.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::Multimode, pmd().asFloat().withName(Name::Multimode.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::BandpassBlend, pmd().asBool().withName(Name::BandpassBlend.toStdString())},
    {ID::FilterWarm, pmd().asBool().withName(Name::FilterWarm.toStdString())},
    {ID::FourPole, pmd().asBool().withName(Name::FourPole.toStdString())},
    {ID::Cutoff, pmd().asFloat().withRange(-45.f, 75.f).withATwoToTheBFormatting(440.f, 1.f / 12.f, "Hz").withDecimalPlaces(1).withName("Cutoff")},
    // <-- FILTER END -->

    // <-- CONTROL -->
    {ID::PitchBendUpRange, pmd().asFloat().withName(Name::PitchBendUpRange.toStdString()).withDefault(0.f).withRange(0.f, 1.f)},
    {ID::PitchBendDownRange, pmd().asFloat().withName(Name::PitchBendDownRange.toStdString()).withDefault(0.f).withRange(0.f, 1.f)},
    {ID::VAmpFactor, pmd().asFloat().withName(Name::VAmpFactor.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::VFltFactor, pmd().asFloat().withName(Name::VFltFactor.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::BendOsc2Only, pmd().asBool().withName(Name::BendOsc2Only.toStdString())},
    {ID::VibratoRate, pmd().asFloat().withName(Name::VibratoRate.toStdString()).withDefault(0.f).withRange(0.f, 1.f).withExtendFactors(10.f, 2.f).withLinearScaleFormatting("Hz") .withDecimalPlaces(2)},
    // <-- CONTROL END -->

    // <-- VOICE VARIATION -->
    {ID::EnvelopeDetune, pmd().asFloat().withName(Name::EnvelopeDetune.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::PortamentoDetune, pmd().asFloat().withName(Name::PortamentoDetune.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::FilterDetune, pmd().asFloat().withName(Name::FilterDetune.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::LevelDetune, pmd().asFloat().withName(Name::LevelDetune.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},

    {ID::Pan1, customPan().withName(Name::Pan1.toStdString())},
    {ID::Pan2, customPan().withName(Name::Pan2.toStdString())},
    {ID::Pan3, customPan().withName(Name::Pan3.toStdString())},
    {ID::Pan4, customPan().withName(Name::Pan4.toStdString())},
    {ID::Pan5, customPan().withName(Name::Pan5.toStdString())},
    {ID::Pan6, customPan().withName(Name::Pan6.toStdString())},
    {ID::Pan7, customPan().withName(Name::Pan7.toStdString())},
    {ID::Pan8, customPan().withName(Name::Pan8.toStdString())},
    // <-- VOICE VARIATION END -->

    // <-- MIXER -->
    {ID::RingModMix, pmd().asCubicDecibelAttenuation().withCustomMinDisplay("-oo").withName(Name::RingModMix.toStdString())},
    {ID::NoiseColor, pmd().asCubicDecibelAttenuation().withCustomMinDisplay("-oo").withName(Name::NoiseColor.toStdString())},
    {ID::NoiseMix, pmd().asCubicDecibelAttenuation().withCustomMinDisplay("-oo").withName(Name::NoiseMix.toStdString())},
    {ID::Osc1Mix, pmd().asCubicDecibelAttenuation().withCustomMinDisplay("-oo").withName(Name::Osc1Mix.toStdString())},
    {ID::Osc2Mix, pmd().asCubicDecibelAttenuation().withCustomMinDisplay("-oo").withName(Name::Osc2Mix.toStdString())},
    // <-- MIXER END -->

    // <-- AMPLIFIER ENVELOPE -->
    {ID::Sustain, pmd().asFloat().withName(Name::Sustain.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::Attack, pmd().asFloat().withName(Name::Attack.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withDecimalPlaces(2)},
    {ID::Decay, pmd().asFloat().withName(Name::Decay.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withDecimalPlaces(2)},
    {ID::Release, pmd().asFloat().withName(Name::Release.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(1.f, 60000.f, 900.f, "s").withDisplayRescalingAbove(1000.f, 0.001f, "s").withDecimalPlaces(2)},
    // <-- AMPLIFIER ENVELOPE END -->

    // <-- FILTER ENVELOPE -->
    {ID::FilterSustain, pmd().asFloat().withName(Name::FilterSustain.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::FilterAttack, pmd().asFloat().withName(Name::FilterAttack.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(4.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withDecimalPlaces(2)},
    {ID::FilterDecay, pmd().asFloat().withName(Name::FilterDecay.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(4.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withDecimalPlaces(2)},
    {ID::FilterRelease, pmd().asFloat().withName(Name::FilterRelease.toStdString()).withRange(0.f, 1.f).withOBXFLogScale(8.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withDecimalPlaces(2)},
    // <-- FILTER ENVELOPE END -->

    // <! -- OTHER -->
    {ID::SelfOscPush, pmd().asBool().withName(Name::SelfOscPush.toStdString())},
    {ID::FenvInvert, pmd().asBool().withName(Name::FenvInvert.toStdString())},
    {ID::EnvPitchBoth, pmd().asBool().withName(Name::EnvPitchBoth.toStdString())},
    {ID::PwEnvBoth, pmd().asBool().withName(Name::PwEnvBoth.toStdString())},
    {ID::LfoSync, pmd().asBool().withName(Name::LfoSync.toStdString())},
    {ID::PwEnv, pmd().asFloat().withName(Name::PwEnv.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::PwOsc2Ofs, pmd().asFloat().withName(Name::PwOsc2Ofs.toStdString()).withDefault(0.f).withRange(0.f, 1.f).asPercent().withDecimalPlaces(2)},
    {ID::EnvelopeToPWInv, pmd().asBool().withName(Name::EnvelopeToPWInv.toStdString())},

    {ID::EconomyMode, pmd().asBool().withName(Name::EconomyMode.toStdString()).withDefault(Defaults::EconomyMode) },
    // <-- OTHER END -->
};
// clang-format on

class ParameterManagerAdaptor
{
  public:
    ValueAttachment<bool> midiLearnAttachment{};
    ValueAttachment<bool> midiUnlearnAttachment{};

    ParameterManagerAdaptor(IParameterState &paramState, IProgramState &progState,
                            juce::AudioProcessor &processor)
        : parameterState(paramState), programState(progState),
          paramManager(processor, "SynthParams", ParameterList)
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
            return ID::LevelDetune;
        case PW_ENV_BOTH:
            return ID::PwEnvBoth;
        case PW_ENV:
            return ID::PwEnv;
        case LFO_SYNC:
            return ID::LfoSync;
        case ECONOMY_MODE:
            return ID::EconomyMode;
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
        case ENVPWINV:
            return ID::EnvelopeToPWInv;
        case VOICE_COUNT:
            return ID::VoiceCount;
        case BANDPASS:
            return ID::BandpassBlend;
        case FILTER_WARM:
            return ID::FilterWarm;
        case PITCH_BEND_UP:
            return ID::PitchBendUpRange;
        case PITCH_BEND_DOWN:
            return ID::PitchBendDownRange;
        case BENDOSC2:
            return ID::BendOsc2Only;
        case OCTAVE:
            return ID::Transpose;
        case TUNE:
            return ID::Tune;
        case BRIGHTNESS:
            return ID::Brightness;
        case OSC1MIX:
            return ID::Osc1Mix;
        case OSC2MIX:
            return ID::Osc2Mix;
        case RINGMODMIX:
            return ID::RingModMix;
        case NOISEMIX:
            return ID::NoiseMix;
        case NOISE_COLOR:
            return ID::NoiseColor;
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
        case LFOVOLUME:
            return ID::LfoVolume;
        case LFOPW:
            return ID::LfoPulsewidth;
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
        if (!parameterState.getMidiControlledParamSet())
            parameterState.setLastUsedParameter(index);

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

    SynthEngine &getEngine() const { return *engine; }

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
        case ENVPWINV:
            synth.processEnvelopeToPWInvert(newValue);
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
        case PITCH_BEND_UP:
            synth.procPitchBendUpRange(newValue);
            break;
        case PITCH_BEND_DOWN:
            synth.procPitchBendDownRange(newValue);
            break;
        case RINGMODMIX:
            synth.processRingModMix(newValue);
            break;
        case NOISEMIX:
            synth.processNoiseMix(newValue);
            break;
        case NOISE_COLOR:
            synth.processNoiseColor(newValue);
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
        case LFOVOLUME:
            synth.processLfoVolume(newValue);
            break;
        case LFOPW:
            synth.processLfoPulsewidth(newValue);
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
