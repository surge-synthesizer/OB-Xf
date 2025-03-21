#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AspectRatioDownscaleConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    AspectRatioDownscaleConstrainer(const int origWidth, const int origHeight)
        : originalWidth(origWidth), originalHeight(origHeight),
          aspectRatio(static_cast<float>(origWidth) / static_cast<float>(origHeight))
    {}

    void checkBounds(juce::Rectangle<int>& bounds,
                    const juce::Rectangle<int>& /*previousBounds*/,
                    const juce::Rectangle<int>& /*limits*/,
                    bool /*isStretchingTop*/,
                    bool /*isStretchingLeft*/,
                    bool /*isStretchingBottom*/,
                    bool /*isStretchingRight*/) override
    {
        bounds.setWidth(juce::jmin(bounds.getWidth(), originalWidth));
        bounds.setHeight(juce::jmin(bounds.getHeight(), originalHeight));
        
        const int newWidth = bounds.getWidth();

        if (const int newHeight = bounds.getHeight(); static_cast<float>(newWidth) / static_cast<float>(newHeight) > aspectRatio)
        {
            bounds.setWidth(juce::roundToInt(newHeight * aspectRatio));
        }
        else if (static_cast<float>(newWidth) / static_cast<float>(newHeight) < aspectRatio)
        {
            bounds.setHeight(juce::roundToInt(newWidth / aspectRatio));
        }
    }

private:
    int originalWidth, originalHeight;
    float aspectRatio;
};