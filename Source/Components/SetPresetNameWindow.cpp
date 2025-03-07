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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "SetPresetNameWindow.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SetPresetNameWindow::SetPresetNameWindow ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    nameTextEditor.reset (new juce::TextEditor ("nameTextEditor"));
    addAndMakeVisible (nameTextEditor.get());
    nameTextEditor->setMultiLine (false);
    nameTextEditor->setReturnKeyStartsNewLine (false);
    nameTextEditor->setReadOnly (false);
    nameTextEditor->setScrollbarsShown (true);
    nameTextEditor->setCaretVisible (true);
    nameTextEditor->setPopupMenuEnabled (false);
    nameTextEditor->setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);
    nameTextEditor->setColour (juce::CaretComponent::caretColourId, juce::Colours::white);
    nameTextEditor->setText (juce::String());

    cancel.reset (new juce::TextButton ("cancel"));
    addAndMakeVisible (cancel.get());
    cancel->setButtonText (TRANS("Cancel"));
    cancel->addListener (this);
    cancel->setColour (juce::TextButton::buttonColourId, juce::Colours::black);

    Ok.reset (new juce::TextButton ("Ok"));
    addAndMakeVisible (Ok.get());
    Ok->setButtonText (TRANS("OK"));
    Ok->addListener (this);
    Ok->setColour (juce::TextButton::buttonColourId, juce::Colours::black);


    //[UserPreSize]
    cancel->setColour (juce::ComboBox::ColourIds::outlineColourId, juce::Colours::white);
    Ok->setColour (juce::ComboBox::ColourIds::outlineColourId, juce::Colours::white);
    //[/UserPreSize]

    setSize (300, 150);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

SetPresetNameWindow::~SetPresetNameWindow()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    nameTextEditor = nullptr;
    cancel = nullptr;
    Ok = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SetPresetNameWindow::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colours::black);

    {
        int x = 0, y = proportionOfHeight (0.0000f), width = proportionOfWidth (1.0000f), height = proportionOfHeight (1.0000f);
        juce::Colour fillColour = juce::Colours::black;
        juce::Colour strokeColour = juce::Colour (0xff666666);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRect (x, y, width, height);
        g.setColour (strokeColour);
        g.drawRect (x, y, width, height, 1);

    }

    {
        int x = proportionOfWidth (0.0000f), y = proportionOfHeight (0.1000f), width = proportionOfWidth (1.0000f), height = proportionOfHeight (0.2000f);
        juce::String text (TRANS("Preset Name"));
        juce::Colour fillColour = juce::Colours::white;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont(juce::Font(
            juce::FontOptions()
                .withName("Arial")
                .withStyle("Regular")
                .withHeight(15.0f)));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SetPresetNameWindow::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    nameTextEditor->setBounds (proportionOfWidth (0.1500f), proportionOfHeight (0.3467f), proportionOfWidth (0.7000f), proportionOfHeight (0.1733f));
    cancel->setBounds (proportionOfWidth (0.2000f), proportionOfHeight (0.7000f), proportionOfWidth (0.2500f), proportionOfHeight (0.1600f));
    Ok->setBounds (proportionOfWidth (0.5500f), proportionOfHeight (0.7000f), proportionOfWidth (0.2500f), proportionOfHeight (0.1600f));
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SetPresetNameWindow::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == cancel.get())
    {
        //[UserButtonCode_cancel] -- add your button handler code here..
        callback(0, nameTextEditor->getText());
        //[/UserButtonCode_cancel]
    }
    else if (buttonThatWasClicked == Ok.get())
    {
        //[UserButtonCode_Ok] -- add your button handler code here..
        callback(1, nameTextEditor->getText());
        //[/UserButtonCode_Ok]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void SetPresetNameWindow::grabTextEditorFocus()
{
    nameTextEditor->grabKeyboardFocus();
};
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SetPresetNameWindow" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="300" initialHeight="150">
  <BACKGROUND backgroundColour="ff000000">
    <RECT pos="0 0% 100% 100%" fill="solid: ff000000" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: ff666666"/>
    <TEXT pos="0% 10% 100% 20%" fill="solid: ffffffff" hasStroke="0" text="Preset Name"
          fontname="Default font" fontsize="15.0" kerning="0.0" bold="0"
          italic="0" justification="36"/>
  </BACKGROUND>
  <TEXTEDITOR name="nameTextEditor" id="13e287a1045d7d6d" memberName="nameTextEditor"
              virtualName="" explicitFocusOrder="0" pos="15% 34.667% 70% 17.333%"
              bkgcol="ff000000" caretcol="ffffffff" initialText="" multiline="0"
              retKeyStartsLine="0" readonly="0" scrollbars="1" caret="1" popupmenu="0"/>
  <TEXTBUTTON name="cancel" id="873979f2630a3992" memberName="cancel" virtualName=""
              explicitFocusOrder="0" pos="20% 70% 25% 16%" bgColOff="ff000000"
              buttonText="Cancel" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="Ok" id="2874357d53dac91e" memberName="Ok" virtualName=""
              explicitFocusOrder="0" pos="55% 70% 25% 16%" bgColOff="ff000000"
              buttonText="OK" connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

