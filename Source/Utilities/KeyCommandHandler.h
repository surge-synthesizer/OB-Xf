/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_UTILITIES_KEYCOMMANDHANDLER_H
#define OBXF_SRC_UTILITIES_KEYCOMMANDHANDLER_H

#include <juce_gui_basics/juce_gui_basics.h>

struct KeyPressCommandIDs
{
    static constexpr int buttonNextProgram = 1;
    static constexpr int buttonPrevProgram = 2;
    static constexpr int buttonPadNextProgram = 3;
    static constexpr int buttonPadPrevProgram = 4;
};

struct MenuAction
{
    static constexpr int Cancel = 0;
    static constexpr int ToggleMidiKeyboard = 1;
    static constexpr int ImportPreset = 2;
    static constexpr int ImportBank = 3;
    static constexpr int ExportBank = 4;
    static constexpr int ExportPreset = 5;
    static constexpr int SavePreset = 6;
    static constexpr int NewPreset = 7;
    static constexpr int RenamePreset = 8;
    static constexpr int DeletePreset = 9;
    static constexpr int DeleteBank = 10;
    static constexpr int ShowBanks = 11;
    static constexpr int CopyPreset = 12;
    static constexpr int PastePreset = 13;
    static constexpr int LoadBank = 14;
};

class KeyCommandHandler final : public juce::ApplicationCommandTarget
{
  public:
    KeyCommandHandler();

    using ProgramChangeCallback = std::function<void()>;

    void setNextProgramCallback(ProgramChangeCallback callback)
    {
        nextProgramCallback = std::move(callback);
    }
    void setPrevProgramCallback(ProgramChangeCallback callback)
    {
        prevProgramCallback = std::move(callback);
    }

    ApplicationCommandTarget *getNextCommandTarget() override { return nullptr; }

    void getAllCommands(juce::Array<juce::CommandID> &commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result) override;
    bool perform(const InvocationInfo &info) override;

  private:
    ProgramChangeCallback nextProgramCallback;
    ProgramChangeCallback prevProgramCallback;
};
#endif // OBXF_SRC_UTILITIES_KEYCOMMANDHANDLER_H
