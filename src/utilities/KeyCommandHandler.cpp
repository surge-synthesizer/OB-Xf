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

#include "KeyCommandHandler.h"

KeyCommandHandler::KeyCommandHandler() : nextProgramCallback([] {}), prevProgramCallback([] {}) {}

void KeyCommandHandler::getAllCommands(juce::Array<juce::CommandID> &commands)
{
    const juce::Array<juce::CommandID> ids{
        KeyPressCommandIDs::buttonNextProgram, KeyPressCommandIDs::buttonPrevProgram,
        KeyPressCommandIDs::buttonPadNextProgram, KeyPressCommandIDs::buttonPadPrevProgram,
        KeyPressCommandIDs::refreshTheme};

    commands.addArray(ids);
}

void KeyCommandHandler::getCommandInfo(const juce::CommandID commandID,
                                       juce::ApplicationCommandInfo &result)
{
    switch (commandID)
    {
    case KeyPressCommandIDs::buttonNextProgram:
        result.setInfo("Next Patch", "Load the next patch", "Navigation", 0);
        result.addDefaultKeypress('+', 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::buttonPrevProgram:
        result.setInfo("Previous Patch", "Load the previous patch", "Navigation", 0);
        result.addDefaultKeypress('-', 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::buttonPadNextProgram:
        result.setInfo("Next Patch", "Load the next patch", "Navigation", 0);
        result.addDefaultKeypress(juce::KeyPress::numberPadAdd, 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::buttonPadPrevProgram:
        result.setInfo("Previous Patch", "Load the previous patch", "Navigation", 0);
        result.addDefaultKeypress(juce::KeyPress::numberPadSubtract, 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::refreshTheme:
        result.setInfo("Refresh Theme", "Refreshes the currently loaded theme", "Developer", 0);
        result.addDefaultKeypress(juce::KeyPress::F5Key, 0);
        result.setActive(true);
        break;
    default:
        break;
    }
}

bool KeyCommandHandler::perform(const InvocationInfo &info)
{
    switch (info.commandID)
    {
    case KeyPressCommandIDs::buttonNextProgram:
    case KeyPressCommandIDs::buttonPadNextProgram:
        nextProgramCallback();
        return true;

    case KeyPressCommandIDs::buttonPrevProgram:
    case KeyPressCommandIDs::buttonPadPrevProgram:
        prevProgramCallback();
        return true;

    case KeyPressCommandIDs::refreshTheme:
        refreshThemeCallback();
        return true;

    default:
        return false;
    }
}