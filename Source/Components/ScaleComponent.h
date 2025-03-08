/*
  ==============================================================================

    ScaleComponent.h
    Created: 27 Aug 2021 1:26:08pm
    Author:  discoDSP

  ==============================================================================
*/

#pragma once

//==============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
class ObxdAudioProcessor;




//==============================================================================
class ScalableComponent
{
public:
    virtual ~ScalableComponent();
    int getScaleInt();
    void setScaleFactor(float newScaleFactor, bool newIsHighResolutionDisplay);
    float getScaleImage();
    float getScaleFactor() const;
	bool getIsHighResolutionDisplay() const;

    virtual void scaleFactorChanged();

protected:
    ScalableComponent(ObxdAudioProcessor* owner_);

    Image getScaledImageFromCache(const String& imageName, float scaleFactor, bool isHighResolutionDisplay);

private:
    ObxdAudioProcessor* processor;
    float scaleFactor;
	bool isHighResolutionDisplay;
    
};


//==============================================================================
class CustomLookAndFeel : public LookAndFeel_V4,
                             public ScalableComponent
{
public:
    CustomLookAndFeel(ObxdAudioProcessor* owner_):LookAndFeel_V4(), ScalableComponent(owner_) {
        this->setColour(PopupMenu::backgroundColourId, Colour(20, 20, 20));
        this->setColour(PopupMenu::textColourId, Colour(245, 245, 245));
        this->setColour(PopupMenu::highlightedBackgroundColourId, Colour(60, 60, 60));
        this->setColour(Label::textColourId, Colour(245, 245, 245));

    };
    

    PopupMenu::Options getOptionsForComboBoxPopupMenu (ComboBox& box, Label& label) override
    {
        PopupMenu::Options option = LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label);
        return option.withStandardItemHeight (label.getHeight()/ getScaleFactor());
    };
    Font getPopupMenuFont () override
    {

        float scaleFactor = getScaleFactor();
        DBG("getPopupMenuFont::scaleFactor " << scaleFactor);
        if (scaleFactor > 1.0) scaleFactor *= 0.85;

        
        #ifdef JUCE_MAC
        return {
            FontOptions()
                .withName("Helvetica Neue")
                .withStyle("Regular")
                .withHeight(18.0f * scaleFactor)};
        #endif
            
        #ifdef JUCE_WINDOWS
        return {
        FontOptions()
            .withName("Arial")
            .withStyle("Regular")
            .withHeight(18.0f * scaleFactor)};
        #endif

        #ifdef JUCE_LINUX
        return {
            FontOptions()
                .withName("DejaVu Sans")
                .withStyle("Regular")
                .withHeight(18.0f * scaleFactor)};
        #endif
    }
 
};
