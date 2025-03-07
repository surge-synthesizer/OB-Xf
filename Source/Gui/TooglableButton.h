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
#include "../Source/Engine/SynthEngine.h"
#include "../Components/ScaleComponent.h"
class ObxdAudioProcessor;

class TooglableButton  : public ImageButton, public ScalableComponent
{
    juce::String img_name;
public:
	TooglableButton (juce::String name, ObxdAudioProcessor *owner) : ImageButton(),ScalableComponent(owner), img_name(name)
	{
		//this->setImages
		//kni = k;
        scaleFactorChanged();
		//toogled = false;
		width = kni.getWidth();
		height = kni.getHeight();
		w2 = width;
		h2 = height / 2;
		this->setClickingTogglesState (true);
	}
    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name, getScaleFactor(), getIsHighResolutionDisplay());
        /*
        backgroundImage =
            allImage.getClippedImage(Rectangle<int>(0,
                                                    allImage.getHeight() / 2,
                                                    allImage.getWidth(),
                                                    allImage.getHeight() / 2));
         */
        repaint();
    }
    ~TooglableButton() override{
        
    };
// Source: https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
public:
    class ToggleAttachment  : public juce::AudioProcessorValueTreeState::ButtonAttachment
    {
        RangedAudioParameter* parameter = nullptr;
        TooglableButton* buttonToControl = nullptr;
    public:
        ToggleAttachment (juce::AudioProcessorValueTreeState& stateToControl,
                          const juce::String& parameterID,
                          TooglableButton& buttonToControl) : AudioProcessorValueTreeState::ButtonAttachment (stateToControl, parameterID, buttonToControl), buttonToControl(&buttonToControl)
        {
            parameter = stateToControl.getParameter (parameterID);
            //buttonToControl.setParameter (parameter);
        }
        /*
        ToggleAttachment (juce::AudioProcessorValueTreeState& stateToControl,
                          const juce::String& parameterID,
                          Button& buttonToControl) : AudioProcessorValueTreeState::ButtonAttachment (stateToControl, parameterID, buttonToControl)
        {
        }*/
        void updateToSlider(){
            float val = parameter->getValue();
            //buttonToControl->setValue(parameter->convertFrom0to1(val0to1), NotificationType::dontSendNotification);
            DBG("Toggle Parameter: " << parameter->name << " Val: " << val);
            buttonToControl->setToggleState(val, NotificationType::dontSendNotification);
        }
        virtual ~ToggleAttachment() = default;
    };
    /*
    void setParameter (const AudioProcessorParameter* p)
    {
        if (parameter == p)
            return;
        
        parameter = p;
        repaint();
    }*/
    /*
	void clicked() override
	{
		toogled = ! toogled;
		//this->setColour(1,Colours::blue);
		//if(toogled)
		//	this->setColour(TextButton::ColourIds::buttonColourId,Colours::lightgreen);
		//else
		//	this->removeColour(TextButton::ColourIds::buttonColourId);
		//this->setColour(DrawableButton::ColourIds::backgroundColourId,Colours::lightpink);
		Button::clicked();

	};*/
    
	void paintButton (Graphics& g, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
	{
        int offset = 0;
        
        //if (toogled)
        if (getToggleState())
        {
            offset = 1;
        }
        
		g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, offset * h2 * getScaleInt() , w2 * getScaleInt(), h2 * getScaleInt());
	}
    /*
	void setValue (float state, int notify)
	{
		if (state > 0.5)
            toogled = true;
		else toogled = false;
        
		repaint();
	}*/
    
	/*float getValue()
	{
		if (toogled)
           return 1;
		else return 0;
	}*/
	//void paint(Graphics& g)
	//{
	//	g.drawImageTransformed(kni,AffineTransform::rotation(((getValue() - getMinimum())/(getMaximum() - getMinimum()))*float_Pi - float_Pi*2));
	//}
    
    //bool toogled;
    
private:
	Image kni;
	int width, height, w2, h2;
    //const AudioProcessorParameter* parameter;
};
