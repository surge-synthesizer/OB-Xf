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

    float calculateScaleFactorFromSize(const juce::Rectangle<int>& currentBounds) const
    {
        if (originalBounds.getWidth() == 0 || originalBounds.getHeight() == 0)
            return 1.0f;

        const float widthRatio = currentBounds.getWidth() / (float)originalBounds.getWidth();
        const float heightRatio = currentBounds.getHeight() / (float)originalBounds.getHeight();

        return juce::jmin(widthRatio, heightRatio);
    }
private:
    ObxdAudioProcessor *processor;
    float scaleFactor;
    bool isHighResolutionDisplay;
};

//==============================================================================
class ScalableResizer
{
public:
    explicit ScalableResizer(juce::Component* componentToResize)
      : component(componentToResize)
    {
        resizer = std::make_unique<juce::ResizableCornerComponent>(
            component, &constraints);
        border = std::make_unique<juce::ResizableBorderComponent>(
            component, &constraints);

        component->addAndMakeVisible(resizer.get());
        component->addAndMakeVisible(border.get());

        constraints.setMinimumSize(400, 300);
        constraints.setMaximumSize(3000, 2000);
    }

    void resized() const {
        if (resizer)
            resizer->setBounds(component->getWidth() - 20, component->getHeight() - 20, 20, 20);
        if (border)
            border->setBounds(component->getLocalBounds());
    }

    void setFixedAspectRatio(const bool shouldMaintainRatio)
    {
        if (shouldMaintainRatio)
            constraints.setFixedAspectRatio(
                component->getWidth() / static_cast<float>(component->getHeight()));
        else
            constraints.setFixedAspectRatio(0.0);
    }

    juce::ComponentBoundsConstrainer& getConstrainer() { return constraints; }

private:
    juce::Component* component;
    std::unique_ptr<juce::ResizableCornerComponent> resizer;
    std::unique_ptr<juce::ResizableBorderComponent> border;
    juce::ComponentBoundsConstrainer constraints;
};