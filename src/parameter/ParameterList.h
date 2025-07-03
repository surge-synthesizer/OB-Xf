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

#ifndef PARAMETERLIST_H
#define PARAMETERLIST_H

#include "SynthParam.h"
#include "ParameterInfo.h"

using namespace SynthParam;

using pmd = sst::basic_blocks::params::ParamMetaData;

enum ObxfParamFeatures : uint64_t
{
    IS_PAN = (uint64_t)pmd::Features::USER_FEATURE_0,
    // F2 = IS_PAN << 1
    // F3 = IS_PAN << 2
};

inline pmd customPan()
{
    // This basically sets up the pan knobs showing stuff like 37 L, 92 R and Center
    return pmd()
        .asFloat()
        .withRange(-1.f, 1.f)
        .withLinearScaleFormatting("R", 100)
        .withDisplayRescalingBelow(0, -1, "L")
        .withDefault(0.f)
        .withFeature((uint64_t)IS_PAN)
        .withDecimalPlaces(0)
        .withCustomDefaultDisplay("Center");
}

inline pmd customLFOWave(std::string leftWave, std::string rightWave)
{
    return pmd()
        .asFloat()
        .withRange(-1.f, 1.f)
        .withUnitSeparator("")
        .withLinearScaleFormatting("% " + rightWave, 100)
        .withDisplayRescalingBelow(0, -1, "% " + leftWave)
        .withDefault(0.f)
        .withDecimalPlaces(0)
        .withCustomDefaultDisplay("DC");
}

// clang-format off
static const std::vector<ParameterInfo> ParameterList{
    // <-- MASTER -->
    {ID::Volume, pmd().asFloat().withName(Name::Volume).withRange(0.f, 1.f).asPercent().withDefault(0.5f).withDecimalPlaces(1).withID(1008)},
    {ID::Transpose, pmd().asFloat().withName(Name::Transpose).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2).withID(2112)},
    {ID::Tune, pmd().asFloat().withName(Name::Tune).withRange(-100.f, 100.f).withLinearScaleFormatting("cents").withDecimalPlaces(1).withID(5150)},

    // <-- GLOBAL -->
    {ID::Polyphony, pmd().asFloat().withName(Name::Polyphony).withRange(0.f, 1.f).withDefault(0.1129f).withID(8675309)},
    {ID::HQMode, pmd().asBool().withName(Name::HQMode).withID(90210)},
    {ID::UnisonVoices, pmd().asFloat().withName(Name::UnisonVoices).withDefault(1.f).withRange(0.f, 1.f).withID(0101101)},

    {ID::Portamento, pmd().asFloat().withName(Name::Portamento).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(1979)},
    {ID::Unison, pmd().asBool().withName(Name::Unison).withID(8)},
    {ID::VoiceDetune, pmd().asFloat().withName(Name::VoiceDetune).withRange(0.f, 1.f).asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(9846)},

    {ID::LegatoMode, pmd().asFloat().withName(Name::LegatoMode).withRange(0.f, 1.f).withID(12340)},
    {ID::NotePriority, pmd().asBool().withName(Name::NotePriority).withID(153251)},

    // <-- OSCILLATORS -->
    {ID::Osc1Pitch, pmd().asFloat().withName(Name::Osc1Pitch).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2).withID(12352)},
    {ID::Oscillator2Detune, pmd().asFloat().withName(Name::Oscillator2Detune).withRange(0.f, 1.f).withOBXFLogScale(0.1f, 100.f, 0.001f, "cents").withDecimalPlaces(1).withID(1345443)},
    {ID::Osc2Pitch, pmd().asFloat().withName(Name::Osc2Pitch).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2).withID(124)},

    {ID::Osc1Saw, pmd().asBool().withName(Name::Osc1Saw).withDefault(1.f).withID(122235)},
    {ID::Osc1Pulse, pmd().asBool().withName(Name::Osc1Pulse).withID(323116)},

    {ID::Osc2Saw, pmd().asBool().withName(Name::Osc2Saw).withID(4357)},
    {ID::Osc2Pulse, pmd().asBool().withName(Name::Osc2Pulse).withID(76818)},

    {ID::PulseWidth, pmd().asFloat().withName(Name::PulseWidth).withRange(0.f, 1.f).withExtendFactors(47.5f, 50.f).withLinearScaleFormatting("%").withDefault(0.f).withDecimalPlaces(1).withID(9859834)},
    {ID::PwOsc2Ofs, pmd().asFloat().withName(Name::PwOsc2Ofs).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(232240)},

    {ID::EnvelopeToPitch, pmd().asFloat().withName(Name::EnvelopeToPitch).asSemitoneRange(0.f, 36.f).withDecimalPlaces(2).withID(7878921)},
    {ID::EnvPitchBoth, pmd().asBool().withName(Name::EnvPitchBoth).withID(222232)},
    {ID::EnvelopeToPitchInv, pmd().asBool().withName(Name::EnvelopeToPitchInv).withID(23678)},

    {ID::PwEnv, pmd().asFloat().withName(Name::PwEnv).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(7824)},
    {ID::PwEnvBoth, pmd().asBool().withName(Name::PwEnvBoth).withID(22235)},
    {ID::EnvelopeToPWInv, pmd().asBool().withName(Name::EnvelopeToPWInv).withID(9926)},

    {ID::Xmod, pmd().asFloat().withName(Name::Xmod).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(298647)},
    {ID::Osc2HardSync, pmd().asBool().withName(Name::Osc2HardSync).withID(28778979)},
    {ID::Brightness, pmd().asFloat().withName(Name::Brightness).withRange(0.f, 1.f).asPercent().withDefault(1.f).withDecimalPlaces(1).withID(255779)},


    // <-- MIXER -->
    {ID::Osc1Mix, pmd().asCubicDecibelAttenuation().withName(Name::Osc1Mix).withDefault(1.f).withDecimalPlaces(1).withID(465630)},
    {ID::Osc2Mix, pmd().asCubicDecibelAttenuation().withName(Name::Osc2Mix).withDefault(0.f).withDecimalPlaces(1).withID(35461)},
    {ID::RingModMix, pmd().asCubicDecibelAttenuation().withName(Name::RingModMix).withDefault(0.f).withDecimalPlaces(1).withID(378662)},
    {ID::NoiseMix, pmd().asCubicDecibelAttenuation().withName(Name::NoiseMix).withDefault(0.f).withDecimalPlaces(1).withID(76833)},
    {ID::NoiseColor, pmd().asCubicDecibelAttenuation().withName(Name::NoiseColor).withDefault(0.f).withDecimalPlaces(1).withID(667834)},


    // <-- CONTROL -->
    {ID::PitchBendUpRange, pmd().asFloat().withName(Name::PitchBendUpRange).withDefault(0.0417f).withRange(0.f, 1.f).withID(3121235)},
    {ID::PitchBendDownRange, pmd().asFloat().withName(Name::PitchBendDownRange).withDefault(0.0417f).withRange(0.f, 1.f).withID(9800936)},
    {ID::BendOsc2Only, pmd().asBool().withName(Name::BendOsc2Only).withID(979737)},

    {ID::VibratoWave, pmd().asBool().withName(Name::VibratoWave).withID(938)},
    {ID::VibratoRate, pmd().asFloat().withName(Name::VibratoRate).withRange(0.f, 1.f).withExtendFactors(10.f, 2.f).withLinearScaleFormatting("Hz").withDefault(0.2f).withDecimalPlaces(2).withID(13239)},

    // <-- FILTER -->
    {ID::FourPole, pmd().asBool().withName(Name::FourPole).withID(402)},

    {ID::Cutoff, pmd().asFloat().withName(Name::Cutoff).withRange(-45.f, 75.f).withATwoToTheBFormatting(440.f, 1.f / 12.f, "Hz").withDefault(75.f).withDecimalPlaces(1).withID(4341)},
    {ID::Resonance, pmd().asFloat().withName(Name::Resonance).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(44562)},
    {ID::FilterEnvAmount, pmd().asFloat().withName(Name::FilterEnvAmount).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(12343)},

    {ID::FilterKeyFollow, pmd().asFloat().withName(Name::FilterKeyFollow).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(21244467)},
    {ID::Multimode, pmd().asFloat().withName(Name::Multimode).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(433455)},

    {ID::BandpassBlend, pmd().asBool().withName(Name::BandpassBlend).withID(456889)},
    {ID::SelfOscPush, pmd().asBool().withName(Name::SelfOscPush).withID(7747)},

    // <-- LFO -->
    {ID::LfoSync, pmd().asBool().withName(Name::LfoSync).withID(9948)},

    {ID::LfoFrequency, pmd().withName(Name::LfoFrequency).withRange(0.f, 1.f).withOBXFLogScale(0, 250, 3775.f, "Hz").withDefault(0.5f).withDecimalPlaces(2).withID(45649)},
    {ID::LfoAmount1, pmd().asFloat().withName(Name::LfoAmount1).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(45650)},
    {ID::LfoAmount2, pmd().asFloat().withName(Name::LfoAmount2).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(56751)},

    {ID::LfoSineWave, customLFOWave("Sine", "Triangle").withName(Name::LfoSineWave).withDefault(-1.f).withID(512232)},
    {ID::LfoSquareWave, customLFOWave("Pulse", "Saw").withName(Name::LfoSquareWave).withID(456853)},
    {ID::LfoSampleHoldWave, customLFOWave("Sample&Hold", "Sample&Glide").withName(Name::LfoSampleHoldWave).withID(2454)},

    {ID::LfoPulsewidth, pmd().asFloat().withName(Name::LfoPulsewidth).withRange(0.f, 1.f).withExtendFactors(45.f, 50.f).withLinearScaleFormatting("%").withDecimalPlaces(1).withID(56755)},

    {ID::LfoOsc1, pmd().asBool().withName(Name::LfoOsc1).withID(546756)},
    {ID::LfoOsc2, pmd().asBool().withName(Name::LfoOsc2).withID(45657)},
    {ID::LfoFilter, pmd().asBool().withName(Name::LfoFilter).withID(645658)},

    {ID::LfoPw1, pmd().asBool().withName(Name::LfoPw1).withID(768759)},
    {ID::LfoPw2, pmd().asBool().withName(Name::LfoPw2).withID(67860)},
    {ID::LfoVolume, pmd().asBool().withName(Name::LfoVolume).withID(667761)},

    // <-- FILTER ENVELOPE -->
    {ID::FenvInvert, pmd().asBool().withName(Name::FenvInvert).withID(2262)},

    {ID::FilterAttack, pmd().asFloat().withName(Name::FilterAttack).withRange(0.f, 1.f).withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(33563)},
    {ID::FilterDecay, pmd().asFloat().withName(Name::FilterDecay).withRange(0.f, 1.f).withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(62344)},
    {ID::FilterSustain, pmd().asFloat().withName(Name::FilterSustain).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(129965)},
    {ID::FilterRelease, pmd().asFloat().withName(Name::FilterRelease).withRange(0.f, 1.f).withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(77442366)},

    {ID::VFltFactor, pmd().asFloat().withName(Name::VFltFactor).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(232347)},

    // <-- AMPLIFIER ENVELOPE -->
    {ID::Attack, pmd().asFloat().withName(Name::Attack).withRange(0.f, 1.f).withOBXFLogScale(4.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(678968)},
    {ID::Decay, pmd().asFloat().withName(Name::Decay).withRange(0.f, 1.f).withOBXFLogScale(4.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(9878769)},
    {ID::Sustain, pmd().asFloat().withName(Name::Sustain).withRange(0.f, 1.f).asPercent().withDefault(1.f).withDecimalPlaces(1).withID(23470)},
    {ID::Release, pmd().asFloat().withName(Name::Release).withRange(0.f, 1.f).withOBXFLogScale(8.f, 60000.f, 900.f, "s").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(12371)},

    {ID::VAmpFactor, pmd().asFloat().withName(Name::VAmpFactor).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(79872)},


    // <-- VOICE VARIATION -->
    {ID::PortamentoDetune, pmd().asFloat().withName(Name::PortamentoDetune).withRange(0.f, 1.f).asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(8773)},
    {ID::FilterDetune, pmd().asFloat().withName(Name::FilterDetune).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(7664)},
    {ID::EnvelopeDetune, pmd().asFloat().withName(Name::EnvelopeDetune).withRange(0.f, 1.f).asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(55455)},
    {ID::LevelDetune, pmd().asFloat().withName(Name::LevelDetune).withRange(0.f, 1.f).asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(9176)},

    {ID::Pan1, customPan().withName(Name::Pan1).withID(34577)},
    {ID::Pan2, customPan().withName(Name::Pan2).withID(36578)},
    {ID::Pan3, customPan().withName(Name::Pan3).withID(12311279)},
    {ID::Pan4, customPan().withName(Name::Pan4).withID(453680)},
    {ID::Pan5, customPan().withName(Name::Pan5).withID(1231281)},
    {ID::Pan6, customPan().withName(Name::Pan6).withID(435382)},
    {ID::Pan7, customPan().withName(Name::Pan7).withID(123321283)},
    {ID::Pan8, customPan().withName(Name::Pan8).withID(63584)},

    // <! -- OTHER -->
    {ID::EconomyMode, pmd().asBool().withName(Name::EconomyMode).withDefault(1.0f).withID(46485) },
};
// clang-format on

#endif // PARAMETERLIST_H
