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

/**
 * Attachment hooks a widget up to a parameter. It has templtae params for the
 * control and on whether to eject begin/end gestures when grabbed (which allow
 * button lists to supress that on drag gestures). It can also take an optiona get/set
 * function if the control doesn't support standard setvalue / getvalue semantics.
 *
 * It also registeres the UI callback so that engine-side updates to params in their
 * callback processing up date the state.
 */
template <typename Control, bool beginEditFromDrag> class Attachment
{
  public:
    using GetValueFn = std::function<float(const Control &)>;
    using SetValueFn = std::function<void(Control &, float)>;

    Attachment(ParameterUpdateHandler &pm, juce::RangedAudioParameter *param, Control &control)
        : Attachment(
              pm, param, control,
              [](auto &c, auto f) { c.setValue(f, juce::dontSendNotification); },
              [](const auto &c) { return static_cast<float>(c.getValue()); })
    {
    }

    Attachment(ParameterUpdateHandler &pm, juce::RangedAudioParameter *param, Control &control,
               SetValueFn setValue, GetValueFn getValue)
        : parameter(param), controlRef(control), updateHandler(pm), setValueFn(setValue),
          getValueFn(getValue)
    {
        setControlCallback(controlRef, [this]() {
            if (updating)
                return;
            updating = true;
            if (parameter)
            {
                updateHandler.queueParameterChange(parameter->paramID, getValueFn(controlRef));
                if constexpr (!beginEditFromDrag)
                {
                    parameter->beginChangeGesture();
                }
                parameter->setValueNotifyingHost(getValueFn(controlRef));
                if constexpr (!beginEditFromDrag)
                {
                    parameter->endChangeGesture();
                }
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

        // this is for engine -> ui side communication. Basically if we
        // arent in a drag or update push the value
        paramCallback = [cwp = juce::Component::SafePointer(&controlRef), this](float value, bool) {
            if (updating)
                return;
            juce::MessageManager::callAsync([cwp, that = this, value]() {
                if (!cwp)
                {
                    // In between the message getting generated and now the c9ntrol has gone
                    // away on screen either from editor closing or from ui having some
                    // other transformation or rebuild.
                    return;
                }
                if (that->isDragging)
                {
                    return;
                }
                that->updating = true;
                that->setValueFn(that->controlRef, value);
                that->updating = false;
            });
        };

        updateHandler.addParameterCallback(parameter->paramID, "UI", paramCallback);
    }

    ~Attachment() { updateHandler.removeParameterCallback(parameter->paramID, "UI"); }

    // This forces the parameter onto the widget. It's called for instance when
    // we ahve a full update in Plugineditor
    void updateToControl()
    {
        if (parameter)
            setValueFn(controlRef, parameter->getValue());
    }

  private:
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

    juce::RangedAudioParameter *parameter;
    Control &controlRef;
    ParameterUpdateHandler &updateHandler;
    std::function<void(float, bool)> paramCallback;
    SetValueFn setValueFn;
    GetValueFn getValueFn;
    std::atomic<bool> updating = false;
    std::atomic<bool> isDragging = false;
};

#endif // OBXF_SRC_PARAMETER_PARAMETERATTACHMENT_H
