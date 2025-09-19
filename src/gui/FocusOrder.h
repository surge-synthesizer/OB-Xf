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

#ifndef OB_XF_SRC_GUI_FOCUSORDER_H
#define OB_XF_SRC_GUI_FOCUSORDER_H

#include <vector>
#include <map>
#include <string>

struct FocusOrder
{
    FocusOrder()
    {
        order = {"volumeKnob",
                 "transposeKnob",
                 "tuneKnob",
                 "polyphonyMenu",
                 "hqModeButton",
                 "unisonVoicesMenu",
                 "portamentoKnob",
                 "unisonButton",
                 "unisonDetuneKnob",
                 "envLegatoModeMenu",
                 "notePriorityMenu",
                 "mainMenu",
                 "midiLearnButton",
                 "midiUnlearnButton",
                 "osc1PitchKnob",
                 "osc2DetuneKnob",
                 "osc2PitchKnob",
                 "osc1SawButton",
                 "osc1PulseButton",
                 "osc1TriangleLabel",
                 "osc1PulseLabel",
                 "osc2SawButton",
                 "osc2PulseButton",
                 "osc2TriangleLabel",
                 "osc2PulseLabel",
                 "oscPWKnob",
                 "osc2PWOffsetKnob",
                 "envToPitchAmountKnob",
                 "envToPitchBothOscsButton",
                 "envToPitchInvertButton",
                 "envToPWAmountKnob",
                 "envToPWBothOscsButton",
                 "envToPWInvertButton",
                 "oscCrossmodKnob",
                 "oscSyncButton",
                 "oscBrightnessKnob",
                 "osc1MixKnob",
                 "osc2MixKnob",
                 "ringModMixKnob",
                 "noiseMixKnob",
                 "noiseColorButton",
                 "bendDownRangeMenu",
                 "bendUpRangeMenu",
                 "bendOsc2OnlyButton",
                 "vibratoWaveButton",
                 "vibratoRateKnob",
                 "filter4PoleModeButton",
                 "filterCutoffKnob",
                 "filterResonanceKnob",
                 "filterEnvAmountKnob",
                 "filterKeyFollowKnob",
                 "filterModeKnob",
                 "filter2PoleBPBlendButton",
                 "filter2PolePushButton",
                 "filter4PoleXpanderButton",
                 "filterXpanderModeMenu",
                 "filterModeLabel",
                 "filterOptionsLabel",
                 "lfo1SelectButton",
                 "lfo2SelectButton",
                 "lfo1TempoSyncButton",
                 "lfo1RateKnob",
                 "lfo1ModAmount1Knob",
                 "lfo1ModAmount2Knob",
                 "lfo1Wave1Knob",
                 "lfo1Wave2Knob",
                 "lfo1Wave3Knob",
                 "lfo1PWSlider",
                 "lfo1Wave2Label",
                 "lfo1ToOsc1PitchButton",
                 "lfo1ToOsc2PitchButton",
                 "lfo1ToFilterCutoffButton",
                 "lfo1ToOsc1PWButton",
                 "lfo1ToOsc2PWButton",
                 "lfo1ToVolumeButton",
                 "lfo2TempoSyncButton",
                 "lfo2RateKnob",
                 "lfo2ModAmount1Knob",
                 "lfo2ModAmount2Knob",
                 "lfo2Wave1Knob",
                 "lfo2Wave2Knob",
                 "lfo2Wave3Knob",
                 "lfo2PWSlider",
                 "lfo2Wave2Label",
                 "lfo2ToOsc1PitchButton",
                 "lfo2ToOsc2PitchButton",
                 "lfo2ToFilterCutoffButton",
                 "lfo2ToOsc1PWButton",
                 "lfo2ToOsc2PWButton",
                 "lfo2ToVolumeButton",
                 "filterEnvInvertButton",
                 "filterEnvAttackKnob",
                 "filterEnvDecayKnob",
                 "filterEnvSustainKnob",
                 "filterEnvReleaseKnob",
                 "filterEnvAttackCurveSlider",
                 "velToFilterEnvSlider",
                 "ampEnvAttackKnob",
                 "ampEnvDecayKnob",
                 "ampEnvSustainKnob",
                 "ampEnvReleaseKnob",
                 "ampEnvAttackCurveSlider",
                 "velToAmpEnvSlider",
                 "portamentoSlopKnob",
                 "filterSlopKnob",
                 "envelopeSlopKnob",
                 "levelSlopKnob",
                 "pan1Knob",
                 "pan2Knob",
                 "pan3Knob",
                 "pan4Knob",
                 "pan5Knob",
                 "pan6Knob",
                 "pan7Knob",
                 "pan8Knob",

                 "patchNumberMenu",
                 "patchNameLabel",
                 "prevPatchButton",
                 "nextPatchButton",
                 "initPatchButton",
                 "randomizePatchButton",
                 "groupSelectButton",
                 "select1Button",
                 "select2Button",
                 "select3Button",
                 "select4Button",
                 "select5Button",
                 "select6Button",
                 "select7Button",
                 "select8Button",
                 "select9Button",
                 "select10Button",
                 "select11Button",
                 "select12Button",
                 "select13Button",
                 "select14Button",
                 "select15Button",
                 "select16Button"};
        rebuildMap();
    }

    int getOrder(const std::string &name) const
    {
        auto op = toOrder.find(name);
        if (op == toOrder.end())
        {
            if (name.find("voice") != 0)
                DBG("Tab Order not provided for " << name);
            return 0;
        }
        return op->second;
        ;
    }

  private:
    static constexpr int baseOrder = 100;
    std::vector<std::string> order;
    std::map<std::string, int> toOrder;
    void rebuildMap()
    {
        for (size_t i = 0; i < order.size(); i++)
        {
            toOrder[order[i]] = i * 3 + baseOrder;
        }
    }
};
#endif // OB_XF_FOCUSORDER_H
