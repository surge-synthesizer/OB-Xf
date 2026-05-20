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

#include "../ObxfEditor.h"

void ObxfAudioProcessorEditor::enterMidiLearnMode()
{
    rebuildMidiLearnCCMap();

    auto getCC = [this](Component *c) -> int {
        auto dcp = dynamic_cast<HasParameterWithID *>(c);
        auto isLastUsed{false};

        if (dcp && dcp->getParameterWithID())
        {
            auto id = dcp->getParameterWithID()->getParameterID();

            if (id == midiLearnLastUsedPID)
            {
                isLastUsed = true;
            }

            if (const auto it = midiLearnCCByParamID.find(id); it != midiLearnCCByParamID.end())
            {
                return isLastUsed ? -it->second : it->second;
            }
        }

        return isLastUsed ? -MidiLearnOverlay::noCCSentinel : MidiLearnOverlay::noCCSentinel;
    };

    midiLearnOverlays.clear();

    for (auto &[pid, comp] : componentByParamID)
    {
        AnchorPosition position = determineAnchorPosition(comp, pid);
        auto overlay = std::make_unique<MidiLearnOverlay>(comp, getCC, position);

        overlay->setPopupFont(midiLearnPopupFont);
        overlay->setScaleFactor(impliedScaleFactor());

        overlay->onSelectionCallback = [this, pcopy = pid](MidiLearnOverlay *selected) {
            midiLearnLastUsedPID = pcopy;
            processor.getMidiHandler().setLastUsedParameter(pcopy);
            repaint();
        };

        overlay->onClearCallback = [this, pid, ov = overlay.get()](Component *comp) {
            processor.getMidiMap().clearBindingByParamID(pid);
            rebuildMidiLearnCCMap();
            repaint();
        };

        if (comp->isVisible())
        {
            addAndMakeVisible(*overlay);
            midiLearnOverlays.push_back(std::move(overlay));
        }
    }
}

void ObxfAudioProcessorEditor::exitMidiLearnMode()
{
    midiLearnOverlays.clear();
    midiLearnLastUsedPID = "";
    processor.getMidiHandler().clearLastUsedParameter();
    repaint();
}

void ObxfAudioProcessorEditor::rebuildMidiLearnCCMap()
{
    midiLearnCCByParamID.clear();
    const auto &mm = processor.getMidiMap();

    for (int i = 0; i < NUM_MIDI_CC; i++)
    {
        if (!mm.controllerParamID[i].isEmpty())
        {
            midiLearnCCByParamID[mm.controllerParamID[i]] = i;
        }
    }
}

AnchorPosition ObxfAudioProcessorEditor::determineAnchorPosition(Component *comp,
                                                                 const juce::String &paramId)
{
    static const std::unordered_map<juce::String, AnchorPosition> paramPositions = {
        {ID::VibratoWave, AnchorPosition::Below},
        {ID::EnvToPitchBothOscs, AnchorPosition::Left},
        {ID::EnvToPitchInvert, AnchorPosition::Right},
        {ID::EnvToPWBothOscs, AnchorPosition::Right},
        {ID::EnvToPWInvert, AnchorPosition::Left},
        {ID::NoiseColor, AnchorPosition::Below},
        {ID::Filter4PoleMode, AnchorPosition::Left},
        {ID::FilterEnvInvert, AnchorPosition::Left},
        {ID::Filter2PoleBPBlend, AnchorPosition::Above},
        {ID::Filter2PolePush, AnchorPosition::Below},
        {ID::LFO1TempoSync, AnchorPosition::Left},
        {ID::LFO1PW, AnchorPosition::Left},
        {ID::LFO2TempoSync, AnchorPosition::Left},
        {ID::LFO2PW, AnchorPosition::Left}};

    if (const auto it = paramPositions.find(paramId); it != paramPositions.end())
        return it->second;

    if (dynamic_cast<Knob *>(comp))
        return AnchorPosition::Overlay;

    return AnchorPosition::Overlay;
}