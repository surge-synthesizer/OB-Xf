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

#ifndef OBXF_SRC_GUI_IMAGEMENU_H
#define OBXF_SRC_GUI_IMAGEMENU_H

#include <utility>

#include "../src/engine/SynthEngine.h"
#include "../components/ScalingImageCache.h"

class ImageMenu : public juce::ImageButton
{
    juce::String img_name;

  public:
    ImageMenu(juce::String assetName, ScalingImageCache &cache)
        : img_name(std::move(assetName)), imageCache(cache)
    {
        scaleFactorChanged();
        setOpaque(false);
        Component::setVisible(true);
    }

    void scaleFactorChanged()
    {
        img = imageCache.getImageFor(img_name.toStdString(), getWidth(), getHeight());
        width = img.getWidth();
        height = img.getHeight() / 2;

        const juce::Image normalImg =
            img.getClippedImage(juce::Rectangle<int>(0, 0, width, height));

        const juce::Image pressedImg =
            img.getClippedImage(juce::Rectangle<int>(0, height, width, height));

        setImages(false, // resizeButtonNowToFitThisImage
                  true,  // rescaleImagesWhenButtonSizeChanges
                  true,  // preserveImageProportions,
                  normalImg,
                  1.0f,           // imageOpacityWhenNormal
                  juce::Colour(), // overlayColourWhenNormal
                  normalImg,
                  1.0f,           // imageOpacityWhenOver
                  juce::Colour(), // overlayColourWhenOver
                  pressedImg,
                  1.0f,          // imageOpacityWhenDown
                  juce::Colour() // overlayColourWhenDown
        );

        repaint();
    }

    void resized() override { scaleFactorChanged(); }

  private:
    juce::Image img;
    int width, height;
    ScalingImageCache &imageCache;
};

#endif // OBXF_SRC_GUI_IMAGEMENU_H
