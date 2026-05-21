/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
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
        order = {
            "mainMenu",
            "mpeButton",

            "volumeKnob",
            "transposeKnob",
            "tuneKnob",

            "osc1PitchKnob",
            "osc1SawButton",
            "osc1PulseButton",
            "oscPWKnob",

            "osc2DetuneKnob",
            "osc2PitchKnob",
            "osc2SawButton",
            "osc2PulseButton",
            "osc2PWOffsetKnob",
            "osc2KeytrackButton",

            "envToPitchAmountKnob",
            "envToPitchBothOscsButton",
            "envToPitchInvertButton",

            "envToPWAmountKnob",
            "envToPWBothOscsButton",
            "envToPWInvertButton",

            "oscSyncButton",
            "oscCrossmodKnob",
            "oscBrightnessKnob",

            "osc1VolKnob",
            "osc2VolKnob",
            "ringModVolKnob",
            "noiseVolKnob",
            "noiseColorButton",

            "filterCutoffKnob",
            "filterResonanceKnob",
            "filterEnvAmountKnob",
            "filterKeyTrackKnob",

            "filter4PoleModeButton",
            "filterModeKnob",
            "filter2PoleBPBlendButton",
            "filter2PolePushButton",

            "filter4PoleXpanderButton",
            "filterXpanderModeMenu",

            "lfo1TempoSyncButton",
            "lfo1RateKnob",
            "lfo1ModAmount1Knob",
            "lfo1ToOsc1PitchButton",
            "lfo1ToOsc2PitchButton",
            "lfo1ToFilterCutoffButton",
            "lfo1ModAmount2Knob",
            "lfo1ToOsc1PWButton",
            "lfo1ToOsc2PWButton",
            "lfo1ToVolumeButton",
            "lfo1Wave1Knob",
            "lfo1Wave2Knob",
            "lfo1PWSlider",
            "lfo1Wave3Knob",

            "lfo2TempoSyncButton",
            "lfo2RateKnob",
            "lfo2ModAmount1Knob",
            "lfo2ToOsc1PitchButton",
            "lfo2ToOsc2PitchButton",
            "lfo2ToFilterCutoffButton",
            "lfo2ModAmount2Knob",
            "lfo2ToOsc1PWButton",
            "lfo2ToOsc2PWButton",
            "lfo2ToVolumeButton",
            "lfo2Wave1Knob",
            "lfo2Wave2Knob",
            "lfo2PWSlider",
            "lfo2Wave3Knob",

            "filterEnvAttackKnob",
            "filterEnvDecayKnob",
            "filterEnvSustainKnob",
            "filterEnvReleaseKnob",
            "filterEnvInvertButton",
            "filterEnvAttackCurveSlider",
            "velToFilterEnvSlider",

            "ampEnvAttackKnob",
            "ampEnvDecayKnob",
            "ampEnvSustainKnob",
            "ampEnvReleaseKnob",
            "ampEnvAttackCurveSlider",
            "velToAmpEnvSlider",

            "polyphonyMenu",
            "hqModeButton",
            "lockHQButton",
            "unisonVoicesMenu",

            "portamentoKnob",
            "unisonButton",
            "unisonDetuneKnob",

            "envLegatoModeMenu",
            "notePriorityMenu",

            "midiLearnButton",

            "bendDownRangeMenu",
            "bendUpRangeMenu",
            "bendOsc2OnlyButton",

            "vibratoWaveButton",
            "vibratoRateKnob",

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

            "savePatchButton",
            "undoPatchButton",

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
            "select16Button",

            "mtsStatusLabel",
            "mtsDynamicButton",
        };
        rebuildMap();
    }

    int getOrder(const std::string &name) const
    {
        auto op = toOrder.find(name);
        if (op == toOrder.end())
        {
            if (name.find("voice") != 0 && name != "aboutPage")
            {
                DBG("Tab Order not provided for " << name);
            }
            return 0;
        }
        return op->second;
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
