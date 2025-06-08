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

#include "KeyCommandHandler.h"

KeyCommandHandler::KeyCommandHandler() : nextProgramCallback([] {}), prevProgramCallback([] {}) {}

void KeyCommandHandler::getAllCommands(juce::Array<juce::CommandID> &commands)
{
    const juce::Array<juce::CommandID> ids{
        KeyPressCommandIDs::buttonNextProgram, KeyPressCommandIDs::buttonPrevProgram,
        KeyPressCommandIDs::buttonPadNextProgram, KeyPressCommandIDs::buttonPadPrevProgram};

    commands.addArray(ids);
}

void KeyCommandHandler::getCommandInfo(const juce::CommandID commandID,
                                       juce::ApplicationCommandInfo &result)
{
    switch (commandID)
    {
    case KeyPressCommandIDs::buttonNextProgram:
        result.setInfo("Move up", "Move the button + ", "Button", 0);
        result.addDefaultKeypress('+', 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::buttonPrevProgram:
        result.setInfo("Move right", "Move the button - ", "Button", 0);
        result.addDefaultKeypress('-', 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::buttonPadNextProgram:
        result.setInfo("Move down", "Move the button Pad + ", "Button", 0);
        result.addDefaultKeypress(juce::KeyPress::numberPadAdd, 0);
        result.setActive(true);
        break;
    case KeyPressCommandIDs::buttonPadPrevProgram:
        result.setInfo("Move left", "Move the button Pad -", "Button", 0);
        result.addDefaultKeypress(juce::KeyPress::numberPadSubtract, 0);
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

    default:
        return false;
    }
}