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
class ScalableComponent {
public:
    virtual ~ScalableComponent();

    int getScaleInt() const;

    void setCustomScaleFactor(float newScaleFactor, bool newIsHighResolutionDisplay);

    float getScaleImage() const;

    float getScaleFactor() const;

    bool getIsHighResolutionDisplay() const;

    virtual void scaleFactorChanged();

    void setOriginalBounds(const juce::Rectangle<int>& bounds)
    {
        originalBounds = bounds;
    }

    juce::Rectangle<int> getOriginalBounds() const
    {
        return originalBounds;
    }

protected:
    explicit ScalableComponent(ObxdAudioProcessor *owner_);

    juce::Image getScaledImageFromCache(const juce::String &imageName, float scaleFactor, bool isHighResolutionDisplay);
    juce::Rectangle<int> originalBounds;

private:
    ObxdAudioProcessor *processor;
    float scaleFactor;
    bool isHighResolutionDisplay;
};

