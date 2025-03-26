#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct KeyPressCommandIDs {
    static constexpr int buttonNextProgram = 1;
    static constexpr int buttonPrevProgram = 2;
    static constexpr int buttonPadNextProgram = 3;
    static constexpr int buttonPadPrevProgram = 4;
};

struct MenuAction {
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

    void setNextProgramCallback(ProgramChangeCallback callback) { nextProgramCallback = std::move(callback); }
    void setPrevProgramCallback(ProgramChangeCallback callback) { prevProgramCallback = std::move(callback); }

    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }

    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    ProgramChangeCallback nextProgramCallback;
    ProgramChangeCallback prevProgramCallback;
};