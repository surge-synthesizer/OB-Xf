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
#include "../PluginEditor.h"
//[/Headers]

#include "PresetBar.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PresetBar::PresetBar (ObxdAudioProcessorEditor &gui)
    : editor(gui)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    presetNameLb.reset (new CustomLabel ("new label",
                                         TRANS("---\n")));
    addAndMakeVisible (presetNameLb.get());

#ifdef JUCE_MAC
    presetNameLb->setFont (Font(
        FontOptions()
            .withName("Helvetica Neue")
            .withStyle("Bold")
            .withHeight(16.0f)));
#endif
            
#ifdef JUCE_WINDOWS
    presetNameLb->setFont (Font(
            FontOptions()
                .withName("Arial")
                .withStyle("Bold")
                .withHeight(16.0f)));
#endif

#ifdef JUCE_LINUX
    presetNameLb->setFont (Font(
        FontOptions()
            .withName("DejaVu Sans")
            .withStyle("Bold")
            .withHeight(16.0f)));
#endif

    presetNameLb->setJustificationType (juce::Justification::centred);
    presetNameLb->setEditable (false, false, false);
    presetNameLb->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    presetNameLb->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    presetNameLb->setBounds (24, 8, 368, 24);

    previousBtn.reset (new juce::ImageButton ("new button"));
    addAndMakeVisible (previousBtn.get());
    previousBtn->setButtonText (juce::String());
    previousBtn->addListener (this);

    previousBtn->setImages (false, true, true,
                            juce::Image(), 1.000f, juce::Colour (0x00000000),
                            juce::Image(), 1.000f, juce::Colour (0x00000000),
                            juce::Image(), 1.000f, juce::Colour (0x00000000));
    previousBtn->setBounds (407, 8, 20, 24);

    nextBtn.reset (new juce::ImageButton ("new button"));
    addAndMakeVisible (nextBtn.get());
    nextBtn->setButtonText (juce::String());
    nextBtn->addListener (this);

    nextBtn->setImages (false, true, true,
                        juce::Image(), 1.000f, juce::Colour (0x00000000),
                        juce::Image(), 1.000f, juce::Colour (0x00000000),
                        juce::Image(), 1.000f, juce::Colour (0x00000000));
    nextBtn->setBounds (435, 8, 20, 24);

    drawable1 = juce::Drawable::createFromImageData (presetnavigation_svg, presetnavigation_svgSize);

    //[UserPreSize]
    presetNameLb->leftClicked = [this](juce::Point<int> pos){
        this->leftClicked(pos);
    };
    //[/UserPreSize]

    setSize (471, 40);


    //[Constructor] You can add your own custom stuff here..
    startTimer(50);
    //[/Constructor]
}

PresetBar::~PresetBar()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    presetNameLb = nullptr;
    previousBtn = nullptr;
    nextBtn = nullptr;
    drawable1 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PresetBar::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colours::black);

    {
        int x = 0, y = 0, width = 471, height = 40;
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (juce::Colours::black);
        jassert (drawable1 != nullptr);
        if (drawable1 != nullptr)
            drawable1->drawWithin (g, juce::Rectangle<int> (x, y, width, height).toFloat(),
                                   juce::RectanglePlacement::centred, 1.000f);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PresetBar::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PresetBar::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == previousBtn.get())
    {
        //[UserButtonCode_previousBtn] -- add your button handler code here..
        editor.prevProgram();
        //update();
        //[/UserButtonCode_previousBtn]
    }
    else if (buttonThatWasClicked == nextBtn.get())
    {
        //[UserButtonCode_nextBtn] -- add your button handler code here..
        editor.nextProgram();
        //update();
        //[/UserButtonCode_nextBtn]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void PresetBar::timerCallback() {
    update();
}

void PresetBar::update(){
    presetNameLb->setText(editor.getCurrentProgramName(), NotificationType::dontSendNotification);
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PresetBar" componentName=""
                 parentClasses="public juce::Component, public Timer" constructorParams="ObxdAudioProcessorEditor &amp;gui"
                 variableInitialisers="editor(gui)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="471"
                 initialHeight="40">
  <BACKGROUND backgroundColour="ff000000">
    <IMAGE pos="0 0 471 40" resource="presetnavigation_svg" opacity="1.0"
           mode="1"/>
  </BACKGROUND>
  <LABEL name="new label" id="3debca48aa2294c8" memberName="presetNameLb"
         virtualName="" explicitFocusOrder="0" pos="24 8 368 24" edTextCol="ff000000"
         edBkgCol="0" labelText="---&#10;" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="36"/>
  <IMAGEBUTTON name="new button" id="ecc6403235eead2" memberName="previousBtn"
               virtualName="" explicitFocusOrder="0" pos="407 8 20 24" buttonText=""
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="" opacityNormal="1.0" colourNormal="0" resourceOver=""
               opacityOver="1.0" colourOver="0" resourceDown="" opacityDown="1.0"
               colourDown="0"/>
  <IMAGEBUTTON name="new button" id="fab648fbebd2d318" memberName="nextBtn"
               virtualName="" explicitFocusOrder="0" pos="435 8 20 24" buttonText=""
               connectedEdges="0" needsCallback="1" radioGroupId="0" keepProportions="1"
               resourceNormal="" opacityNormal="1.0" colourNormal="0" resourceOver=""
               opacityOver="1.0" colourOver="0" resourceDown="" opacityDown="1.0"
               colourDown="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: presetnavigation_svg, 2078, "../../../../GitHub/OB-Xd/Source/Images/presetnavigation.svg"
static const unsigned char resource_PresetBar_presetnavigation_svg[] = { 60,63,120,109,108,32,118,101,114,115,105,111,110,61,34,49,46,48,34,32,101,110,99,111,100,105,110,103,61,34,85,84,70,45,56,34,32,
115,116,97,110,100,97,108,111,110,101,61,34,110,111,34,63,62,10,60,33,68,79,67,84,89,80,69,32,115,118,103,32,80,85,66,76,73,67,32,34,45,47,47,87,51,67,47,47,68,84,68,32,83,86,71,32,49,46,49,47,47,69,78,
34,32,34,104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,47,71,114,97,112,104,105,99,115,47,83,86,71,47,49,46,49,47,68,84,68,47,115,118,103,49,49,46,100,116,100,34,62,10,60,115,118,103,32,
119,105,100,116,104,61,34,49,48,48,37,34,32,104,101,105,103,104,116,61,34,49,48,48,37,34,32,118,105,101,119,66,111,120,61,34,48,32,48,32,52,55,49,32,51,57,34,32,118,101,114,115,105,111,110,61,34,49,46,
49,34,32,120,109,108,110,115,61,34,104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,47,50,48,48,48,47,115,118,103,34,32,120,109,108,110,115,58,120,108,105,110,107,61,34,104,116,116,112,58,
47,47,119,119,119,46,119,51,46,111,114,103,47,49,57,57,57,47,120,108,105,110,107,34,32,120,109,108,58,115,112,97,99,101,61,34,112,114,101,115,101,114,118,101,34,32,120,109,108,110,115,58,115,101,114,105,
102,61,34,104,116,116,112,58,47,47,119,119,119,46,115,101,114,105,102,46,99,111,109,47,34,32,115,116,121,108,101,61,34,102,105,108,108,45,114,117,108,101,58,101,118,101,110,111,100,100,59,99,108,105,112,
45,114,117,108,101,58,101,118,101,110,111,100,100,59,115,116,114,111,107,101,45,108,105,110,101,99,97,112,58,114,111,117,110,100,59,115,116,114,111,107,101,45,108,105,110,101,106,111,105,110,58,114,111,
117,110,100,59,115,116,114,111,107,101,45,109,105,116,101,114,108,105,109,105,116,58,49,46,53,59,34,62,10,32,32,32,32,60,114,101,99,116,32,120,61,34,48,34,32,121,61,34,48,34,32,119,105,100,116,104,61,
34,52,55,49,34,32,104,101,105,103,104,116,61,34,51,57,34,47,62,10,32,32,32,32,60,112,97,116,104,32,100,61,34,77,51,57,56,44,57,46,50,48,53,67,51,57,56,44,55,46,52,51,54,32,51,57,54,46,53,54,52,44,54,32,
51,57,52,46,55,57,53,44,54,76,50,52,46,50,48,53,44,54,67,50,50,46,52,51,54,44,54,32,50,49,44,55,46,52,51,54,32,50,49,44,57,46,50,48,53,76,50,49,44,50,57,46,55,57,53,67,50,49,44,51,49,46,53,54,52,32,50,
50,46,52,51,54,44,51,51,32,50,52,46,50,48,53,44,51,51,76,51,57,52,46,55,57,53,44,51,51,67,51,57,54,46,53,54,52,44,51,51,32,51,57,56,44,51,49,46,53,54,52,32,51,57,56,44,50,57,46,55,57,53,76,51,57,56,44,
57,46,50,48,53,90,34,32,115,116,121,108,101,61,34,115,116,114,111,107,101,58,114,103,98,40,49,50,49,44,49,50,49,44,49,50,49,41,59,115,116,114,111,107,101,45,119,105,100,116,104,58,49,112,120,59,34,47,
62,10,32,32,32,32,60,103,32,116,114,97,110,115,102,111,114,109,61,34,109,97,116,114,105,120,40,49,44,48,44,48,44,49,44,52,46,50,50,50,54,54,44,49,49,46,48,50,52,53,41,34,62,10,32,32,32,32,32,32,32,32,
60,103,32,116,114,97,110,115,102,111,114,109,61,34,109,97,116,114,105,120,40,50,52,44,48,44,48,44,50,52,44,52,48,54,46,54,50,49,44,49,55,46,50,54,52,53,41,34,62,10,32,32,32,32,32,32,32,32,32,32,32,32,
60,112,97,116,104,32,100,61,34,77,48,46,52,50,53,44,45,48,46,49,53,54,67,48,46,52,51,53,44,45,48,46,49,52,55,32,48,46,52,51,57,44,45,48,46,49,51,53,32,48,46,52,51,57,44,45,48,46,49,50,50,67,48,46,52,51,
57,44,45,48,46,49,48,57,32,48,46,52,51,53,44,45,48,46,48,57,55,32,48,46,52,50,53,44,45,48,46,48,56,56,67,48,46,52,49,53,44,45,48,46,48,55,56,32,48,46,52,48,52,44,45,48,46,48,55,51,32,48,46,51,57,49,44,
45,48,46,48,55,51,67,48,46,51,55,55,44,45,48,46,48,55,51,32,48,46,51,54,54,44,45,48,46,48,55,56,32,48,46,51,53,54,44,45,48,46,48,56,55,76,48,46,49,49,50,44,45,48,46,51,51,50,67,48,46,49,48,50,44,45,48,
46,51,52,49,32,48,46,48,57,56,44,45,48,46,51,53,51,32,48,46,48,57,56,44,45,48,46,51,54,54,67,48,46,48,57,56,44,45,48,46,51,56,32,48,46,49,48,51,44,45,48,46,51,57,49,32,48,46,49,49,50,44,45,48,46,52,48,
49,76,48,46,51,53,54,44,45,48,46,54,52,53,67,48,46,51,54,54,44,45,48,46,54,53,52,32,48,46,51,55,55,44,45,48,46,54,53,57,32,48,46,51,57,49,44,45,48,46,54,53,57,67,48,46,52,48,52,44,45,48,46,54,53,57,32,
48,46,52,49,53,44,45,48,46,54,53,52,32,48,46,52,50,53,44,45,48,46,54,52,53,67,48,46,52,51,53,44,45,48,46,54,51,53,32,48,46,52,51,57,44,45,48,46,54,50,52,32,48,46,52,51,57,44,45,48,46,54,49,67,48,46,52,
51,57,44,45,48,46,53,57,55,32,48,46,52,51,53,44,45,48,46,53,56,53,32,48,46,52,50,53,44,45,48,46,53,55,54,76,48,46,50,49,53,44,45,48,46,51,54,54,76,48,46,52,50,53,44,45,48,46,49,53,54,90,34,32,115,116,
121,108,101,61,34,102,105,108,108,58,114,103,98,40,49,50,49,44,49,50,49,44,49,50,49,41,59,102,105,108,108,45,114,117,108,101,58,110,111,110,122,101,114,111,59,34,47,62,10,32,32,32,32,32,32,32,32,60,47,
103,62,10,32,32,32,32,60,47,103,62,10,32,32,32,32,60,103,32,116,114,97,110,115,102,111,114,109,61,34,109,97,116,114,105,120,40,49,44,48,44,48,44,49,44,51,49,46,50,50,50,55,44,49,49,46,48,50,52,53,41,34,
62,10,32,32,32,32,32,32,32,32,60,103,32,116,114,97,110,115,102,111,114,109,61,34,109,97,116,114,105,120,40,50,52,44,48,44,48,44,50,52,44,52,48,54,46,54,50,49,44,49,55,46,50,54,52,53,41,34,62,10,32,32,
32,32,32,32,32,32,32,32,32,32,60,112,97,116,104,32,100,61,34,77,48,46,51,50,50,44,45,48,46,51,54,54,76,48,46,49,49,50,44,45,48,46,53,55,54,67,48,46,49,48,51,44,45,48,46,53,56,53,32,48,46,48,57,56,44,45,
48,46,53,57,55,32,48,46,48,57,56,44,45,48,46,54,49,67,48,46,48,57,56,44,45,48,46,54,50,52,32,48,46,49,48,50,44,45,48,46,54,51,53,32,48,46,49,49,50,44,45,48,46,54,52,53,67,48,46,49,50,50,44,45,48,46,54,
53,52,32,48,46,49,51,51,44,45,48,46,54,53,57,32,48,46,49,52,54,44,45,48,46,54,53,57,67,48,46,49,54,44,45,48,46,54,53,57,32,48,46,49,55,49,44,45,48,46,54,53,52,32,48,46,49,56,49,44,45,48,46,54,52,53,76,
48,46,52,50,53,44,45,48,46,52,48,49,67,48,46,52,51,53,44,45,48,46,51,57,49,32,48,46,52,51,57,44,45,48,46,51,56,32,48,46,52,51,57,44,45,48,46,51,54,54,67,48,46,52,51,57,44,45,48,46,51,53,51,32,48,46,52,
51,53,44,45,48,46,51,52,49,32,48,46,52,50,53,44,45,48,46,51,51,50,76,48,46,49,56,49,44,45,48,46,48,56,55,67,48,46,49,55,49,44,45,48,46,48,55,56,32,48,46,49,54,44,45,48,46,48,55,51,32,48,46,49,52,54,44,
45,48,46,48,55,51,67,48,46,49,51,51,44,45,48,46,48,55,51,32,48,46,49,50,50,44,45,48,46,48,55,56,32,48,46,49,49,50,44,45,48,46,48,56,56,67,48,46,49,48,50,44,45,48,46,48,57,55,32,48,46,48,57,56,44,45,48,
46,49,48,57,32,48,46,48,57,56,44,45,48,46,49,50,50,67,48,46,48,57,56,44,45,48,46,49,51,53,32,48,46,49,48,51,44,45,48,46,49,52,55,32,48,46,49,49,50,44,45,48,46,49,53,54,76,48,46,51,50,50,44,45,48,46,51,
54,54,90,34,32,115,116,121,108,101,61,34,102,105,108,108,58,114,103,98,40,49,50,49,44,49,50,49,44,49,50,49,41,59,102,105,108,108,45,114,117,108,101,58,110,111,110,122,101,114,111,59,34,47,62,10,32,32,
32,32,32,32,32,32,60,47,103,62,10,32,32,32,32,60,47,103,62,10,60,47,115,118,103,62,10,0,0};

const char* PresetBar::presetnavigation_svg = (const char*) resource_PresetBar_presetnavigation_svg;
const int PresetBar::presetnavigation_svgSize = 2078;


//[EndFile] You can add extra defines here...
//[/EndFile]

