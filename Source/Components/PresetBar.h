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
class ObxdAudioProcessorEditor;
class CustomLabel: public juce::Label {
public:
    CustomLabel (const String& componentName = String(),
                 const String& labelText = String()): juce::Label(componentName, labelText) {
        
    }
    std::function<void(juce::Point<int> pos)> leftClicked;
    void mouseDown (const juce::MouseEvent& event) override {
        if (this->getBounds().contains(event.getMouseDownPosition()) && event.mods.isLeftButtonDown()){
            leftClicked(event.getScreenPosition());
        }
    };
};
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PresetBar  : public juce::Component,
                   public Timer,
                   public juce::Button::Listener
{
public:
    //==============================================================================
    PresetBar (ObxdAudioProcessorEditor &gui);
    ~PresetBar() override;

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void timerCallback() override;
    void update();
    
    
    std::function<void(juce::Point<int> &pos)> leftClicked;
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;

    // Binary resources:
    static const char* presetnavigation_svg;
    static const int presetnavigation_svgSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    ObxdAudioProcessorEditor &editor;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<CustomLabel> presetNameLb;
    std::unique_ptr<juce::ImageButton> previousBtn;
    std::unique_ptr<juce::ImageButton> nextBtn;
    std::unique_ptr<juce::Drawable> drawable1;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

