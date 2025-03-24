#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
class ObxdAudioProcessor;


class ScalableComponent {
public:
    virtual ~ScalableComponent();

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

    juce::Image getScaledImageFromCache(const juce::String &imageName) const;
    juce::Rectangle<int> originalBounds;

private:
    ObxdAudioProcessor *processor;
};

