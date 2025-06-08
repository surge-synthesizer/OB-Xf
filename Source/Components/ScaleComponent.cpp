/*
 * OB-Xf - a continuation of the last open source version
 * of OB-Xd.
 *
 * OB-Xd was originally written by Filatov Vadim, and
 * then a version was released under the GPL3
 * at https://github.com/reales/OB-Xd. Subsequently
 * the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more
 * see "HISTORY.md" in the root of this repo.
 *
 * This repository is a successor to the last open source
 * release, a version marked as 2.11. Copyright
 * 2013-2025 by the authors as indicated in the original
 * release, and subsequent authors per the github transaction
 * log.
 *
 * OB-Xf is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source for OB-Xf is available at
 * https://github.com/surge-synthesizer/OB-Xf
 */

#include "../PluginProcessor.h"
#include "ScaleComponent.h"
#include "BinaryData.h"

ScalableComponent::ScalableComponent(ObxdAudioProcessor *owner_) : processor(owner_) {}

ScalableComponent::~ScalableComponent() = default;

juce::Image ScalableComponent::getScaledImageFromCache(const juce::String &imageName

) const
{
    juce::File skin;
    if (processor)
    {
        if (const juce::File f(processor->getUtils().getCurrentSkinFolder()); f.isDirectory())
            skin = f;
    }

    if (const juce::File file = skin.getChildFile(imageName + ".png"); file.exists())
        return juce::ImageCache::getFromFile(file);

    int size = 0;
    const char *data = BinaryData::getNamedResource((imageName + "_png").toUTF8(), size);
    return juce::ImageCache::getFromMemory(data, size);
}

void ScalableComponent::scaleFactorChanged() {}