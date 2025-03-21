/*
  ==============================================================================

    ScaleComponent.cpp
    Created: 27 Aug 2021 1:26:08pm
    Author:  discoDSP

  ==============================================================================
*/
#include "../PluginProcessor.h"
#include "ScaleComponent.h"
#include "BinaryData.h"


//==============================================================================
ScalableComponent::ScalableComponent(ObxdAudioProcessor *owner_)
    : processor(owner_),
      scaleFactor(0.0f),
      isHighResolutionDisplay(false)
{
    setCustomScaleFactor(1.0f, false);
}

ScalableComponent::~ScalableComponent()
= default;

void ScalableComponent::setCustomScaleFactor(float newScaleFactor, bool newIsHighResolutionDisplay)
{
    if (scaleFactor != newScaleFactor || isHighResolutionDisplay != newIsHighResolutionDisplay)
    {
        scaleFactor = newScaleFactor;
        isHighResolutionDisplay = newIsHighResolutionDisplay;

        scaleFactorChanged();
    }
}

float ScalableComponent::getScaleImage() const
{
    float scale = 1.0;
    if (!isHighResolutionDisplay)
    {
        if (getScaleFactor() == 1.5f)
        {
            scale = 0.75f;
        }
        else if (getScaleFactor() == 2.0f)
        {
            scale = 0.5f;
        }
    }
    else
    {
        if (getScaleFactor() == 1.0f) //2x image
        {
            scale = 0.5f;
        }
        else if (getScaleFactor() == 1.5f) //4x images =>150%
        {
            scale = (0.25f + 0.125f);
        }
        else if (getScaleFactor() == 2.0f) //4x images =>200x
        {
            scale = 0.5f;
        }
    }
    return scale;
};

float ScalableComponent::getScaleFactor() const
{
    return scaleFactor;
}

bool ScalableComponent::getIsHighResolutionDisplay() const
{
    return isHighResolutionDisplay;
}

int ScalableComponent::getScaleInt() const
{
    int scaleFactorInt = 1;
    if (scaleFactor == 1.5f)
        scaleFactorInt = 2;
    if (scaleFactor == 2.0f)
        scaleFactorInt = 4;

    if (isHighResolutionDisplay)
    {
        scaleFactorInt *= 2;
    }
    if (scaleFactorInt > 4)
    {
        scaleFactorInt = 4;
    }
    return scaleFactorInt;
}

juce::Image ScalableComponent::getScaledImageFromCache(const juce::String &imageName,
                                                       float /*scaleFactor*/,
                                                       bool isHighResolutionDisplay)
{
    this->isHighResolutionDisplay = isHighResolutionDisplay;
    const int scaleFactorInt = getScaleInt();
    juce::String resourceName = imageName + "_png";
    if (scaleFactorInt != 1)
    {
        resourceName = imageName + juce::String::formatted("%dx_png", scaleFactorInt);
    }

    juce::File skin;
    if (processor)
    {
        if (const juce::File f(processor->getUtils().getCurrentSkinFolder()); f.isDirectory())
        {
            skin = f;
        }
    }

    const char *data = nullptr;
    juce::String image_file = imageName;
    if (scaleFactorInt == 1)
        image_file += ".png";
    else
        image_file += juce::String::formatted("@%dx.png", scaleFactorInt);
    if (const juce::File file = skin.getChildFile(image_file); file.exists())
    {
        return juce::ImageCache::getFromFile(file);
    }
    int size = 0;
    data = BinaryData::getNamedResource(resourceName.toUTF8(), size);
    return juce::ImageCache::getFromMemory(data, size);
}

void ScalableComponent::scaleFactorChanged()
{
}