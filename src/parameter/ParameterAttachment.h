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

#ifndef OBXF_SRC_PARAMETER_PARAMETERATTACHMENT_H
#define OBXF_SRC_PARAMETER_PARAMETERATTACHMENT_H

#include <juce_audio_processors/juce_audio_processors.h>

template <typename Control, bool beginEditFromDrag, typename SetValueFn, typename GetValueFn>
class Attachment
{
  public:
    Attachment(ParameterManager &pm, juce::RangedAudioParameter *param, Control &control,
               SetValueFn setValue, GetValueFn getValue)
        : parameter(param), controlRef(control), paramManager(pm), setValueFn(setValue),
          getValueFn(getValue)
    {
        setControlCallback(controlRef, [this]() {
            if (updating)
                return;
            updating = true;
            if (parameter)
            {
                if (!beginEditFromDrag)
                {
                    parameter->beginChangeGesture();
                }
                parameter->setValueNotifyingHost(getValueFn(controlRef));
                if (!beginEditFromDrag)
                {
                    parameter->endChangeGesture();
                }
                paramManager.queueParameterChange(parameter->paramID, getValueFn(controlRef));
            }
            updating = false;
        });

        if constexpr (beginEditFromDrag)
        {
            control.onDragStart = [this]() {
                isDragging = true;
                parameter->beginChangeGesture();
            };
            control.onDragEnd = [this]() {
                parameter->endChangeGesture();
                isDragging = false;
            };
        }

        paramCallback = [this](float value, bool) {
            if (updating)
                return;
            juce::MessageManager::callAsync([that = this, value]() {
                if (that->isDragging)
                {
                    return;
                }
                that->updating = true;
                that->setValueFn(that->controlRef, value);
                that->updating = false;
            });
        };

        paramManager.addParameterCallback(parameter->paramID, "UI", paramCallback);
    }

    ~Attachment() { paramManager.removeParameterCallback(parameter->paramID, "UI"); }

    void updateToControl()
    {
        if (parameter)
            setValueFn(controlRef, parameter->getValue());
    }

    template <typename C> void setControlCallback(C &control, std::function<void()> fn)
    {
        control.onValueChange = std::move(fn);
    }

    void setControlCallback(ToggleButton &control, std::function<void()> fn)
    {
        control.onClick = std::move(fn);
    }

    void setControlCallback(ButtonList &control, std::function<void()> fn)
    {
        control.onChange = std::move(fn);
    }

  private:
    juce::RangedAudioParameter *parameter;
    Control &controlRef;
    ParameterManager &paramManager;
    std::function<void(float, bool)> paramCallback;
    SetValueFn setValueFn;
    GetValueFn getValueFn;
    std::atomic<bool> updating = false;
    std::atomic<bool> isDragging = false;
};

#endif // OBXF_SRC_PARAMETER_PARAMETERATTACHMENT_H
