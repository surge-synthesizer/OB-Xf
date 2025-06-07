#include "KeyCommandHandler.h"

KeyCommandHandler::KeyCommandHandler()
    : nextProgramCallback([]{}),
      prevProgramCallback([]{})
{
}

void KeyCommandHandler::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    const juce::Array<juce::CommandID> ids{
        KeyPressCommandIDs::buttonNextProgram,
        KeyPressCommandIDs::buttonPrevProgram,
        KeyPressCommandIDs::buttonPadNextProgram,
        KeyPressCommandIDs::buttonPadPrevProgram
    };

    commands.addArray(ids);
}

void KeyCommandHandler::getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID) {
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

bool KeyCommandHandler::perform(const InvocationInfo& info)
{
    switch (info.commandID) {
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