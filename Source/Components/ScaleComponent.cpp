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
ScalableComponent::ScalableComponent(ObxdAudioProcessor* owner_)
    : processor(owner_),
	  scaleFactor(0.0f),
        isHighResolutionDisplay(false)
{
    setScaleFactor(1.0f, false);
}

ScalableComponent::~ScalableComponent()
{
}

void ScalableComponent::setScaleFactor(float newScaleFactor, bool newIsHighResolutionDisplay)
{
    // until we get a freely scalable editor !!!
    jassert(newScaleFactor == 1.0f || newScaleFactor == 1.5f || newScaleFactor == 2.0f);

    if (scaleFactor != newScaleFactor || isHighResolutionDisplay != newIsHighResolutionDisplay)
    {
        scaleFactor = newScaleFactor;
		isHighResolutionDisplay = newIsHighResolutionDisplay;

        scaleFactorChanged();
    }
}

float ScalableComponent::getScaleImage(){
    float scale = 1.0;
    if (!isHighResolutionDisplay)
    {
        if (getScaleFactor() == 1.5f)
        {
            scale= 0.75f;
        }
        else if (getScaleFactor() == 2.0f)
        {
            scale= 0.5f;
        }
    } else {
        if (getScaleFactor() == 1.0f) //2x image
        {
            scale= 0.5f;
        }
        else if (getScaleFactor() == 1.5f) //4x images =>150%
        {
            scale= (0.25f + 0.125f);
        }
        else if (getScaleFactor() == 2.0f) //4x images =>200x
        {
            scale= 0.5f;
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

int ScalableComponent::getScaleInt(){
    int scaleFactorInt = 1;
    if (scaleFactor == 1.5f)
        scaleFactorInt = 2;
    if (scaleFactor == 2.0f)
        scaleFactorInt = 4;

    if (isHighResolutionDisplay){
        scaleFactorInt *= 2;
    }
    if (scaleFactorInt> 4){
        scaleFactorInt=4;
    }
    return scaleFactorInt;
}

juce::Image ScalableComponent::getScaledImageFromCache(const juce::String& imageName,
													   float /*scaleFactor*/,
													   bool isHighResolutionDisplay)
{
    jassert(scaleFactor == 1.0f || scaleFactor == 1.5f || scaleFactor == 2.0f);
    this->isHighResolutionDisplay = isHighResolutionDisplay;
    int scaleFactorInt = getScaleInt();
    juce::String resourceName = imageName + "_png";
    if (scaleFactorInt != 1){
        resourceName = imageName + juce::String::formatted("%dx_png", scaleFactorInt);
    }


    int size = 0;
    juce::File skin;
    if (processor){
        juce::File f(utils.getCurrentSkinFolder());
        if (f.isDirectory()){
            skin=f;
        }
    }

    const char* data = nullptr;
    juce::String image_file = imageName;
    if (scaleFactorInt ==1)
        image_file +=  ".png";
    else
        image_file += juce::String::formatted("@%dx.png", scaleFactorInt);
    DBG(" Loaf image: " << image_file);
    juce::File file = skin.getChildFile(image_file);
    if (file.exists()){
        return juce::ImageCache::getFromFile(file);
    } else {
        data = BinaryData::getNamedResource((const char*)resourceName.toUTF8(), size);
        DBG(" Image: " << resourceName);
        return juce::ImageCache::getFromMemory(data, size);
    }
}

void ScalableComponent::scaleFactorChanged()
{
}
