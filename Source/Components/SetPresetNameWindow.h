/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.0.8

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <juce_gui_basics/juce_gui_basics.h>
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SetPresetNameWindow  : public juce::Component,
                             public juce::Button::Listener
{
public:
    //==============================================================================
    SetPresetNameWindow ();
    ~SetPresetNameWindow() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    std::function<void(int, juce::String)> callback;
    void setText(const juce::String &txt){
        nameTextEditor->setText(txt);
    }
    void grabTextEditorFocus();
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<juce::TextEditor> nameTextEditor;
    std::unique_ptr<juce::TextButton> cancel;
    std::unique_ptr<juce::TextButton> Ok;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SetPresetNameWindow)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

