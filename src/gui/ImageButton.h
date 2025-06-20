/*
 * OB-Xf - a continuation of the last open source version of OB-Xd.
 *
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product. For more info,
 * see "HISTORY.md" in the root of this repository.
 *
 * This repository is a successor to the last open source release,
 * a version marked as 2.11. Copyright 2013-2025 by the authors
 * as indicated in the original release, and subsequent authors
 * per the GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_GUI_IMAGEBUTTON_H
#define OBXF_SRC_GUI_IMAGEBUTTON_H

#include <utility>

#include "../src/engine/SynthEngine.h"

class ObxfAudioProcessor;

class ImageMenu : public juce::ImageButton, public ScalableComponent
{
    juce::String img_name;

  public:
    ImageMenu(juce::String nameImg, ObxfAudioProcessor *owner_)
        : ScalableComponent(owner_), img_name(std::move(nameImg))
    {
        ImageMenu::scaleFactorChanged();

        setOpaque(false);
        Component::setVisible(true);
    }

    void scaleFactorChanged() override
    {

        const juce::Image normalImage = getScaledImageFromCache(img_name);
        const juce::Image downImage = getScaledImageFromCache(img_name);

        constexpr bool resizeButtonNowToFitThisImage = false;
        constexpr bool rescaleImagesWhenButtonSizeChanges = true;
        constexpr bool preserveImageProportions = true;

        setImages(resizeButtonNowToFitThisImage, rescaleImagesWhenButtonSizeChanges,
                  preserveImageProportions, normalImage,
                  1.0f, // menu transparency
                  juce::Colour(), normalImage,
                  1.0f, // menu hover transparency
                  juce::Colour(), downImage,
                  0.3f, // menu click transparency
                  juce::Colour());

        repaint();
    }
};

#endif // OBXF_SRC_GUI_IMAGEBUTTON_H
