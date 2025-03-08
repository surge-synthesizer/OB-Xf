/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright � 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */
#pragma once
#include <utility>

#include "../Source/Engine/SynthEngine.h"
class ObxdAudioProcessor;


class ImageMenu : public ImageButton,
                                  public ScalableComponent
{
    juce::String img_name;
public:
    ImageMenu(juce::String nameImg, ObxdAudioProcessor* owner_)
        :  ScalableComponent(owner_), img_name(std::move(nameImg))
    {
	    ImageMenu::scaleFactorChanged();

        setOpaque(false);
	    Component::setVisible(true);
    }


    void scaleFactorChanged() override
    {
        const float scaleFactor = getScaleFactor();
        const bool isHighResolutionDisplay = getIsHighResolutionDisplay();

        const Image normalImage = getScaledImageFromCache(img_name, scaleFactor, isHighResolutionDisplay);
        const Image downImage = getScaledImageFromCache(img_name, scaleFactor, isHighResolutionDisplay);

        constexpr bool resizeButtonNowToFitThisImage = false;
        constexpr bool rescaleImagesWhenButtonSizeChanges = true;
        constexpr bool preserveImageProportions = true;

        setImages(resizeButtonNowToFitThisImage,
                  rescaleImagesWhenButtonSizeChanges,
                  preserveImageProportions,
                  normalImage,
                  1.0f, // menu transparency
                  Colour(),
                  normalImage,
                  1.0f, // menu hover transparency
                  Colour(),
                  downImage,
                  0.3f, // menu click transparency
                  Colour());

        repaint();
    }


protected:
};
