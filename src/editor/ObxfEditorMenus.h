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
void setupMPEMatrixMenus() const;
void setupMenus();

struct MPEMatrixWidgetDef
{
    const char *destWidget;
    const char *amountWidget;
    const char *destName;
    const char *amountName;
    MatrixSource source;
    int slotIndex;
};

// clang-format off
static constexpr std::array<MPEMatrixWidgetDef, NUM_MATRIX_ROWS> mpeMatrixWidgetDefs = {{
    {"mpeStrikeDestination1Menu", "mpeStrikeAmount1Knob", "MPE Strike Destination 1", "MPE Strike Amount 1", MatrixSource::Strike, 0},
    {"mpeStrikeDestination2Menu", "mpeStrikeAmount2Knob", "MPE Strike Destination 2", "MPE Strike Amount 2", MatrixSource::Strike, 1},
    {"mpeLiftDestination1Menu",   "mpeLiftAmount1Knob",   "MPE Lift Destination 1",   "MPE Lift Amount 1",   MatrixSource::Lift,   2},
    {"mpeLiftDestination2Menu",   "mpeLiftAmount2Knob",   "MPE Lift Destination 2",   "MPE Lift Amount 2",   MatrixSource::Lift,   3},
    {"mpePressDestination1Menu",  "mpePressAmount1Knob",  "MPE Press Destination 1",  "MPE Press Amount 1",  MatrixSource::Press,  4},
    {"mpePressDestination2Menu",  "mpePressAmount2Knob",  "MPE Press Destination 2",  "MPE Press Amount 2",  MatrixSource::Press,  5},
    {"mpeSlideDestination1Menu",  "mpeSlideAmount1Knob",  "MPE Slide Destination 1",  "MPE Slide Amount 1",  MatrixSource::Slide,  6},
    {"mpeSlideDestination2Menu",  "mpeSlideAmount2Knob",  "MPE Slide Destination 2",  "MPE Slide Amount 2",  MatrixSource::Slide,  7},
}};
// clang-format on

static std::string matrixTargetMenuAsset(MatrixSource src)
{
    std::string s;

    switch (src)
    {
    case MatrixSource::Strike:
        s = "menu-mpe-targets-strike";
        break;
    case MatrixSource::Lift:
        s = "menu-mpe-targets-lift";
        break;
    default:
        s = "menu-mpe-targets";
        break;
    }

    return s;
}

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
