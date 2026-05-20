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

void enterMidiLearnMode();
void exitMidiLearnMode();
void rebuildMidiLearnCCMap();

AnchorPosition determineAnchorPosition(Component *comp, const juce::String &paramId);

// Members
juce::String midiLearnLastUsedPID;
std::vector<std::unique_ptr<MidiLearnOverlay>> midiLearnOverlays;
std::unordered_map<juce::String, int> midiLearnCCByParamID;