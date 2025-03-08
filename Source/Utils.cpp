#include "Utils.h"
#include "juce_gui_basics/juce_gui_basics.h"

void openInPdf(const juce::File& file)
{
    if (file.existsAsFile())
    {
        // Use getFullPathName to ensure we have an absolute path
        juce::File absoluteFile = file.getFullPathName();

        // Use system command on Windows to open PDF with default viewer
#if JUCE_WINDOWS
        const juce::String command = R"(cmd /c start "" ")" + absoluteFile.getFullPathName() + "\"";
        system(command.toRawUTF8());
#else
        absoluteFile.startAsProcess();
#endif
    }
    else
    {
        juce::NativeMessageBox::showMessageBox(
            juce::AlertWindow::WarningIcon,
            "Error",
            "OB-Xd Manual.pdf not found."
        );
    }
}
