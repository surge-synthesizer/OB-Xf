#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AspectRatioDownscaleConstrainer final : public juce::ComponentBoundsConstrainer
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
        const int minWidth  = originalWidth  / 2;
        const int minHeight = originalHeight / 2;
        const int maxWidth  = originalWidth  * 2;
        const int maxHeight = originalHeight * 2;

        bounds.setWidth (juce::jmax (minWidth,  juce::jmin (bounds.getWidth(),  maxWidth)));
        bounds.setHeight(juce::jmax (minHeight, juce::jmin (bounds.getHeight(), maxHeight)));

        if (const float currentRatio = static_cast<float>(bounds.getWidth()) / static_cast<float>(bounds.getHeight()); currentRatio > aspectRatio)
            bounds.setWidth(juce::roundToInt(bounds.getHeight() * aspectRatio));
        else if (currentRatio < aspectRatio)
            bounds.setHeight(juce::roundToInt(bounds.getWidth() / aspectRatio));
    }

private:
    int originalWidth, originalHeight;
    float aspectRatio;
};