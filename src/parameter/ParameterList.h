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

#ifndef OBXF_SRC_PARAMETER_PARAMETERLIST_H
#define OBXF_SRC_PARAMETER_PARAMETERLIST_H

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

inline pmd tristateLFOTo()
{
    return pmd()
        .asInt()
        .withRange(0.f, 2.f)
        .withDefault(0.f)
        .withDecimalPlaces(0)
        .withUnorderedMapFormatting({{0, "Off"}, {1, "On"}, {2, "Inverted"}});
}

/*
 * A note about adding to this list.
 *
 * All our formats now support adding parameters, but AUv2 has a tweak where
 * you need to use the JUCE "versionHints" mechanism to group new ones in a version.
 *
 * What this means de-facto is if after OB-Xf 1.0 we add new parameters for
 * 1.1 or what not we need a verison hint, which is the third argument to
 * ParameterInfo constructor.
 *
 * So to add post 1.0 a new param you would do
 *
 * { ID::NewStuff, pmd().asFloat().withName("New").asPercent(), 2 }
 *
 * and that '2' matters a lot. Probably what we will do after 1.0 is
 * add constants here like "obxf_11_versionhint" which is 2, then add a
 * 12_versionhint and so on.
 *
 * For now the version hint defaults to 1 and is consumed in the ParameterManager
 * where the ObxfParameterFloat is constructed.
 */
// clang-format off
static const std::vector<ParameterInfo> ParameterList{
    // <-- MASTER -->
    {ID::Volume, pmd().asFloat().withName(Name::Volume).withRange(0.f, 1.f).asPercent().withDefault(0.5f).withDecimalPlaces(1).withID(1008)},
    {ID::Transpose, pmd().asFloat().withName(Name::Transpose).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(0).withID(2112)},
    {ID::Tune, pmd().asFloat().withName(Name::Tune).withRange(-100.f, 100.f).withLinearScaleFormatting("cents").withDecimalPlaces(1).withID(5150)},

    // <-- GLOBAL -->
    {ID::Polyphony, pmd().asInt().withLinearScaleFormatting("Voices").withName(Name::Polyphony).withRange(1, MAX_VOICES).withDefault(8).withID(8675309)},
    {ID::HQMode, pmd().asOnOffBool().withName(Name::HQMode).withID(90210)},

    {ID::UnisonVoices, pmd().asInt().withLinearScaleFormatting("Voices").withName(Name::UnisonVoices).withRange(1, MAX_VOICES).withDefault(8).withID(0101101)},

    {ID::Portamento, pmd().asFloat().withName(Name::Portamento).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(1979)},
    {ID::Unison, pmd().asOnOffBool().withName(Name::Unison).withID(8)},
    {ID::UnisonDetune, pmd().asFloat().withName(Name::UnisonDetune).withRange(0.f, 1.f).asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(9846)},

    {ID::EnvLegatoMode, pmd().asInt().withName(Name::EnvLegatoMode).withRange(0, 3).withID(12340)
                             .withUnorderedMapFormatting({{0, "Both"}, {1, "Filter"}, {2, "Amp"}, {3, "Retrigger"}})},
    {ID::NotePriority, pmd().asInt().withName(Name::NotePriority).withRange(0, 2).withID(153251)
                            .withUnorderedMapFormatting({{0, "Last"}, {1, "Low"}, {2, "High"}})},

    // <-- OSCILLATORS -->
    {ID::Osc1Pitch, pmd().asFloat().withName(Name::Osc1Pitch).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2).withID(12352)},
    {ID::Osc2Detune, pmd().asFloat().withName(Name::Osc2Detune).withRange(0.f, 1.f)
                          .withOBXFLogScale(0.1f, 100.f, 0.001f, "cents").withDecimalPlaces(1).withID(1345443)},
    {ID::Osc2Pitch, pmd().asFloat().withName(Name::Osc2Pitch).asSemitoneRange(-24.f, 24.f).withDecimalPlaces(2).withID(124)},

    {ID::Osc1SawWave, pmd().asOnOffBool().withName(Name::Osc1SawWave).withDefault(1.f).withID(122235)},
    {ID::Osc1PulseWave, pmd().asOnOffBool().withName(Name::Osc1PulseWave).withID(323116)},

    {ID::Osc2SawWave, pmd().asOnOffBool().withName(Name::Osc2SawWave).withDefault(1.f).withID(4357)},
    {ID::Osc2PulseWave, pmd().asOnOffBool().withName(Name::Osc2PulseWave).withID(76818)},

    {ID::OscPW, pmd().asFloat().withName(Name::OscPW).withRange(0.f, 1.f).withLinearScaleFormatting("%", 47.5f, 50.f)
                     .withDecimalPlaces(1).withID(9859834)},
    {ID::Osc2PWOffset, pmd().asFloat().withName(Name::Osc2PWOffset).withRange(0.f, 1.f)
                            .withLinearScaleFormatting("%", 47.5f).withDecimalPlaces(1).withID(232240)},

    {ID::EnvToPitchAmount, pmd().asFloat().withName(Name::EnvToPitchAmount).asSemitoneRange(0.f, 36.f).withDecimalPlaces(2).withID(7878921)},
    {ID::EnvToPitchBothOscs, pmd().asOnOffBool().withName(Name::EnvToPitchBothOscs).withDefault(1.f).withID(222232)},
    {ID::EnvToPitchInvert, pmd().asOnOffBool().withName(Name::EnvToPitchInvert).withID(23678)},

    {ID::EnvToPWAmount, pmd().asFloat().withName(Name::EnvToPWAmount).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(7824)},
    {ID::EnvToPWBothOscs, pmd().asOnOffBool().withName(Name::EnvToPWBothOscs).withDefault(1.f).withID(22235)},
    {ID::EnvToPWInvert, pmd().asOnOffBool().withName(Name::EnvToPWInvert).withID(9926)},

    {ID::OscCrossmod, pmd().asFloat().withName(Name::OscCrossmod).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(298647)},
    {ID::OscSync, pmd().asOnOffBool().withName(Name::OscSync).withID(28778979)},
    {ID::OscBrightness, pmd().asFloat().withName(Name::OscBrightness).withRange(0.f, 1.f).asPercent().withDefault(1.f).withDecimalPlaces(1).withID(255779)},

    // <-- MIXER -->
    {ID::Osc1Vol, pmd().asCubicDecibelAttenuation().withName(Name::Osc1Vol).withDefault(1.f).withDecimalPlaces(1).withID(465630)},
    {ID::Osc2Vol, pmd().asCubicDecibelAttenuation().withName(Name::Osc2Vol).withDefault(0.f).withDecimalPlaces(1).withID(35461)},
    {ID::RingModVol, pmd().asCubicDecibelAttenuation().withName(Name::RingModVol).withDefault(0.f).withDecimalPlaces(1).withID(378662)},
    {ID::NoiseVol, pmd().asCubicDecibelAttenuation().withName(Name::NoiseVol).withDefault(0.f).withDecimalPlaces(1).withID(76833)},
    {ID::NoiseColor, pmd().asInt().withName(Name::NoiseColor).withRange(0, 2).withID(667834)
                          .withUnorderedMapFormatting({{0, "White"}, {1, "Pink"}, {2, "Red"}})},

    // <-- CONTROL -->
    {ID::BendUpRange, pmd().asInt().withName(Name::BendUpRange).withRange(0, MAX_BEND_RANGE).withDefault(2).withID(3121235)
                           .withLinearScaleFormatting("Semitones")},
    {ID::BendDownRange, pmd().asInt().withName(Name::BendDownRange).withRange(0, MAX_BEND_RANGE).withDefault(2).withID(9800936)
                             .withLinearScaleFormatting("Semitones")},
    {ID::BendOsc2Only, pmd().asOnOffBool().withName(Name::BendOsc2Only).withID(979737)},

    {ID::VibratoWave, pmd().asBool().withName(Name::VibratoWave).withID(938).withUnorderedMapFormatting({{0, "Sine"}, {1, "Square"}})},
    {ID::VibratoRate, pmd().asFloat().withName(Name::VibratoRate).withRange(0.f, 1.f)
                           .withLinearScaleFormatting("Hz", 10.f, 2.f).withDefault(0.3f).withDecimalPlaces(2).withID(13239)},

    // <-- FILTER -->
    {ID::Filter4PoleMode, pmd().asOnOffBool().withName(Name::Filter4PoleMode).withID(402)},

    {ID::FilterCutoff, pmd().asFloat().withName(Name::FilterCutoff).withRange(-45.f, 75.f)
                            .withATwoToTheBFormatting(440.f, 1.f / 12.f, "Hz").withDefault(75.f).withDecimalPlaces(1).withID(4341)},
    {ID::FilterResonance, pmd().asFloat().withName(Name::FilterResonance).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(44562)},
    {ID::FilterEnvAmount, pmd().asFloat().withName(Name::FilterEnvAmount).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(12343)},

    {ID::FilterKeyTrack, pmd().asFloat().withName(Name::FilterKeyTrack).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(21244467)},
    {ID::FilterMode, pmd().asFloat().withName(Name::FilterMode).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(433455)},

    {ID::Filter2PoleBPBlend, pmd().asOnOffBool().withName(Name::Filter2PoleBPBlend).withID(456889)},
    {ID::Filter2PolePush, pmd().asOnOffBool().withName(Name::Filter2PolePush).withID(7747)},

    {ID::Filter4PoleXpander, pmd().asOnOffBool().withName(Name::Filter4PoleXpander).withID(999666)},
    {ID::FilterXpanderMode, pmd().asInt().withName(Name::FilterXpanderMode).withRange(0, NUM_XPANDER_MODES - 1).withID(666999)
                                 .withUnorderedMapFormatting({
                                                                { 0, "LP4"    }, { 1, "LP3"   }, { 2, "LP2"    }, { 3, "LP1"    },
                                                                { 4, "HP3"    }, { 5, "HP2"   }, { 6, "HP1"    }, { 7, "BP4"    },
                                                                { 8, "BP2"    }, { 9, "N2"    }, {10, "PH3"    }, {11, "HP2+LP1"},
                                                                {12, "HP3+LP1"}, {13, "N2+LP1"}, {14, "PH3+LP1"},
                                                             })},

    // <-- LFO 1 -->
    {ID::LFO1TempoSync, pmd().asOnOffBool().withName(Name::LFO1TempoSync).withID(9948)},

    {ID::LFO1Rate, pmd().withName(Name::LFO1Rate).withRange(0.f, 1.f).temposyncable(true)
                        .withOBXFLogScale(0, 250, 3775.f, "Hz").withDefault(0.5f).withDecimalPlaces(2).withID(45649)},
    {ID::LFO1ModAmount1, pmd().asFloat().withName(Name::LFO1ModAmount1).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(45650)},
    {ID::LFO1ModAmount2, pmd().asFloat().withName(Name::LFO1ModAmount2).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(56751)},

    {ID::LFO1Wave1, customLFOWave("Sine", "Triangle").withName(Name::LFO1Wave1).withDefault(-1.f).withID(512232)},
    {ID::LFO1Wave2, customLFOWave("Pulse", "Saw").withName(Name::LFO1Wave2).withID(456853)},
    {ID::LFO1Wave3, customLFOWave("Sample&Hold", "Sample&Glide").withName(Name::LFO1Wave3).withID(2454)},

    {ID::LFO1PW, pmd().asFloat().withName(Name::LFO1PW).withRange(0.f, 1.f)
                      .withLinearScaleFormatting("%", 45.f, 50.f).withDecimalPlaces(1).withID(56755)},

    {ID::LFO1ToOsc1Pitch, tristateLFOTo().withName(Name::LFO1ToOsc1Pitch).withID(546756)},
    {ID::LFO1ToOsc2Pitch, tristateLFOTo().withName(Name::LFO1ToOsc2Pitch).withID(45657)},
    {ID::LFO1ToFilterCutoff, tristateLFOTo().withName(Name::LFO1ToFilterCutoff).withID(645658)},

    {ID::LFO1ToOsc1PW, tristateLFOTo().withName(Name::LFO1ToOsc1PW).withID(768759)},
    {ID::LFO1ToOsc2PW, tristateLFOTo().withName(Name::LFO1ToOsc2PW).withID(67860)},
    {ID::LFO1ToVolume, tristateLFOTo().withName(Name::LFO1ToVolume).withID(667761)},

    // <-- LFO 2 -->
    {ID::LFO2TempoSync, pmd().asOnOffBool().withName(Name::LFO2TempoSync).withID(7245678)},

    {ID::LFO2Rate, pmd().withName(Name::LFO2Rate).withRange(0.f, 1.f).temposyncable(true)
                        .withOBXFLogScale(0, 250, 3775.f, "Hz").withDefault(0.5f).withDecimalPlaces(2).withID(236345)},
    {ID::LFO2ModAmount1, pmd().asFloat().withName(Name::LFO2ModAmount1).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(375638)},
    {ID::LFO2ModAmount2, pmd().asFloat().withName(Name::LFO2ModAmount2).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(8975649)},

    {ID::LFO2Wave1, customLFOWave("Sine", "Triangle").withName(Name::LFO2Wave1).withDefault(-1.f).withID(5568357)},
    {ID::LFO2Wave2, customLFOWave("Pulse", "Saw").withName(Name::LFO2Wave2).withID(32893957)},
    {ID::LFO2Wave3, customLFOWave("Sample&Hold", "Sample&Glide").withName(Name::LFO2Wave3).withID(5789009)},

    {ID::LFO2PW, pmd().asFloat().withName(Name::LFO2PW).withRange(0.f, 1.f)
                      .withLinearScaleFormatting("%", 45.f, 50.f).withDecimalPlaces(1).withID(45678765)},

    {ID::LFO2ToOsc1Pitch, tristateLFOTo().withName(Name::LFO2ToOsc1Pitch).withID(1010696)},
    {ID::LFO2ToOsc2Pitch, tristateLFOTo().withName(Name::LFO2ToOsc2Pitch).withID(2049961)},
    {ID::LFO2ToFilterCutoff, tristateLFOTo().withName(Name::LFO2ToFilterCutoff).withID(95890497)},

    {ID::LFO2ToOsc1PW, tristateLFOTo().withName(Name::LFO2ToOsc1PW).withID(51034956)},
    {ID::LFO2ToOsc2PW, tristateLFOTo().withName(Name::LFO2ToOsc2PW).withID(1058774325)},
    {ID::LFO2ToVolume, tristateLFOTo().withName(Name::LFO2ToVolume).withID(984477567)},

    // <-- FILTER ENVELOPE -->
    {ID::FilterEnvInvert, pmd().asOnOffBool().withName(Name::FilterEnvInvert).withID(2262)},

    {ID::FilterEnvAttack, pmd().asFloat().withName(Name::FilterEnvAttack).withRange(0.f, 1.f)
                               .withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(33563)},
    {ID::FilterEnvDecay, pmd().asFloat().withName(Name::FilterEnvDecay).withRange(0.f, 1.f)
                              .withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(62344)},
    {ID::FilterEnvSustain, pmd().asFloat().withName(Name::FilterEnvSustain).withRange(0.f, 1.f)
                                .asPercent().withDefault(1.f).withDecimalPlaces(1).withID(129965)},
    {ID::FilterEnvRelease, pmd().asFloat().withName(Name::FilterEnvRelease).withRange(0.f, 1.f)
                                .withOBXFLogScale(1.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(77442366)},

    {ID::FilterEnvAttackCurve, pmd().asFloat().withName(Name::FilterEnvAttackCurve).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(38356520)},
    {ID::VelToFilterEnv, pmd().asFloat().withName(Name::VelToFilterEnv).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(232347)},

    // <-- AMPLIFIER ENVELOPE -->
    {ID::AmpEnvAttack, pmd().asFloat().withName(Name::AmpEnvAttack).withRange(0.f, 1.f)
                            .withOBXFLogScale(4.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(678968)},
    {ID::AmpEnvDecay, pmd().asFloat().withName(Name::AmpEnvDecay).withRange(0.f, 1.f)
                           .withOBXFLogScale(4.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(9878769)},
    {ID::AmpEnvSustain, pmd().asFloat().withName(Name::AmpEnvSustain).withRange(0.f, 1.f)
                             .asPercent().withDefault(1.f).withDecimalPlaces(1).withID(23470)},
    {ID::AmpEnvRelease, pmd().asFloat().withName(Name::AmpEnvRelease).withRange(0.f, 1.f)
                             .withOBXFLogScale(8.f, 60000.f, 900.f, "ms").withDisplayRescalingAbove(1000.f, 0.001f, "s").withID(12371)},

    {ID::AmpEnvAttackCurve, pmd().asFloat().withName(Name::AmpEnvAttackCurve).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(918546732)},
    {ID::VelToAmpEnv, pmd().asFloat().withName(Name::VelToAmpEnv).withRange(0.f, 1.f).asPercent().withDecimalPlaces(1).withID(79872)},

    // <-- VOICE VARIATION -->
    {ID::PortamentoSlop, pmd().asFloat().withName(Name::PortamentoSlop).withRange(0.f, 1.f)
                              .asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(8773)},
    {ID::FilterSlop, pmd().asFloat().withName(Name::FilterSlop).withRange(0.f, 1.f)
                          .asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(7664)},
    {ID::EnvelopeSlop, pmd().asFloat().withName(Name::EnvelopeSlop).withRange(0.f, 1.f)
                            .asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(55455)},
    {ID::LevelSlop, pmd().asFloat().withName(Name::LevelSlop).withRange(0.f, 1.f)
                         .asPercent().withDefault(0.25f).withDecimalPlaces(1).withID(9176)},

    {ID::PanVoice1, customPan().withName(Name::PanVoice1).withID(34577)},
    {ID::PanVoice2, customPan().withName(Name::PanVoice2).withID(36578)},
    {ID::PanVoice3, customPan().withName(Name::PanVoice3).withID(12311279)},
    {ID::PanVoice4, customPan().withName(Name::PanVoice4).withID(453680)},
    {ID::PanVoice5, customPan().withName(Name::PanVoice5).withID(1231281)},
    {ID::PanVoice6, customPan().withName(Name::PanVoice6).withID(435382)},
    {ID::PanVoice7, customPan().withName(Name::PanVoice7).withID(123321283)},
    {ID::PanVoice8, customPan().withName(Name::PanVoice8).withID(63584)},
};
// clang-format on

#endif // OBXF_SRC_PARAMETER_PARAMETERLIST_H
