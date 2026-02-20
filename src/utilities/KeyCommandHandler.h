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

#ifndef OBXF_SRC_UTILITIES_KEYCOMMANDHANDLER_H
#define OBXF_SRC_UTILITIES_KEYCOMMANDHANDLER_H

#include <juce_gui_basics/juce_gui_basics.h>

struct KeyPressCommandIDs
{
    static constexpr int buttonNextProgram = 1;
    static constexpr int buttonPrevProgram = 2;
    static constexpr int buttonPadNextProgram = 3;
    static constexpr int buttonPadPrevProgram = 4;
    static constexpr int refreshTheme = 5;
};

struct MenuAction
{
    static constexpr int InitializePatch = 1;
    static constexpr int RandomizePatch = 2;

    static constexpr int LoadPatch = 3;
    static constexpr int SavePatch = 5;
    static constexpr int QuickSavePatch = 6;
    static constexpr int DeletePatch = 7;

    static constexpr int CopyPatch = 8;
    static constexpr int PastePatch = 9;

    static constexpr int RevealUserDirectory = 10;

    static constexpr int SetDefaultPatch = 11;
    static constexpr int RefreshBrowser = 12;

    static constexpr int Inspector = 100;
};

class KeyCommandHandler final : public juce::ApplicationCommandTarget
{
  public:
    KeyCommandHandler();

    using KeyCommandCallback = std::function<void()>;

    void setNextProgramCallback(KeyCommandCallback callback)
    {
        nextProgramCallback = std::move(callback);
    }
    void setPrevProgramCallback(KeyCommandCallback callback)
    {
        prevProgramCallback = std::move(callback);
    }
    void setRefreshThemeCallback(KeyCommandCallback callback)
    {
        refreshThemeCallback = std::move(callback);
    }

    ApplicationCommandTarget *getNextCommandTarget() override { return nullptr; }

    void getAllCommands(juce::Array<juce::CommandID> &commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result) override;
    bool perform(const InvocationInfo &info) override;

  private:
    KeyCommandCallback nextProgramCallback, prevProgramCallback, refreshThemeCallback;
};
#endif // OBXF_SRC_UTILITIES_KEYCOMMANDHANDLER_H
