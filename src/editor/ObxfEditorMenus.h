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

// Menu setup
void setupPolyphonyMenu() const;
void setupUnisonVoicesMenu() const;
void setupEnvLegatoModeMenu() const;
void setupNotePriorityMenu() const;
void setupBendUpRangeMenu() const;
void setupBendDownRangeMenu() const;
void setupMPEGlideRangeMenu() const;
void setupFilterXpanderModeMenu() const;
void setupMenus();

// Main menu
void createMenu();
void createMidiMapMenu(int menuNo, juce::PopupMenu &menuMidi);
void resultFromMenu(juce::Point<int> pos);
void MenuActionCallback(int action);
void keyboardFocusMainMenu();
void importObxdBank(const juce::File &fxbFile);

// Patch list
juce::PopupMenu createPatchList(juce::PopupMenu &menu) const;
int patchesInCurrentFolder() const;

// Mutator
void showMutatorMenu();
void mutateCallback();
void randomizeCallback();

// Members
std::vector<std::unique_ptr<juce::PopupMenu>> popupMenus;

size_t midiStart{};
size_t sizeStart{};
size_t themeStart{};
