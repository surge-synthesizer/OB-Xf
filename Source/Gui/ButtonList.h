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

#ifndef OBXF_SRC_GUI_BUTTONLIST_H
#define OBXF_SRC_GUI_BUTTONLIST_H
#include <utility>

#include "../Source/Engine/SynthEngine.h"
class ObxdAudioProcessor;

class ButtonList final : public juce::ComboBox, public ScalableComponent
{
    juce::String img_name;

  public:
    ButtonList(juce::String nameImg, const int fh, ObxdAudioProcessor *owner)
        : ComboBox("cb"), ScalableComponent(owner), img_name(std::move(nameImg))
    {
        ButtonList::scaleFactorChanged();
        count = 0;
        h2 = fh;
        w2 = kni.getWidth();
    }

    void scaleFactorChanged() override
    {
        kni = getScaledImageFromCache(img_name);
        repaint();
    }
    // Source:
    // https://git.iem.at/audioplugins/IEMPluginSuite/-/blob/master/resources/customComponents/ReverseSlider.h
  public:
    class ButtonListAttachment final : public juce::AudioProcessorValueTreeState::ComboBoxAttachment
    {
        juce::RangedAudioParameter *parameter = nullptr;
        ButtonList *buttonListToControl = nullptr;

      public:
        ButtonListAttachment(juce::AudioProcessorValueTreeState &stateToControl,
                             const juce::String &parameterID, ButtonList &buttonListToControl)
            : juce::AudioProcessorValueTreeState::ComboBoxAttachment(stateToControl, parameterID,
                                                                     buttonListToControl),
              buttonListToControl(&buttonListToControl)
        {
            parameter = stateToControl.getParameter(parameterID);
            buttonListToControl.setParameter(parameter);
        }

        void updateToSlider() const
        {
            const float val = parameter->getValue();
            buttonListToControl->setValue(val, juce::NotificationType::dontSendNotification);
        }

        virtual ~ButtonListAttachment() = default;
    };

    void setParameter(const juce::AudioProcessorParameter *p)
    {
        if (parameter == p)
            return;

        parameter = p;
        repaint();
    }

    void addChoice(const juce::String &name) { addItem(name, ++count); }

    float getValue() const { return ((getSelectedId() - 1) / static_cast<float>(count - 1)); }

    void setValue(const float val, const juce::NotificationType notify)
    {
        setSelectedId(static_cast<int>(val * (count - 1) + 1), notify);
    }

    void paintOverChildren(juce::Graphics &g) override
    {
        int ofs = getSelectedId() - 1;
        g.drawImage(kni, 0, 0, getWidth(), getHeight(), 0, h2 * ofs, w2, h2);
    }

  private:
    int count;
    juce::Image kni;
    int w2, h2;
    const juce::AudioProcessorParameter *parameter{nullptr};
};

#endif // OBXF_SRC_GUI_BUTTONLIST_H
