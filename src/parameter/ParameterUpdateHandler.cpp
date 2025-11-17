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

#include "ParameterUpdateHandler.h"
#include "SynthParam.h"
#include "PluginProcessor.h"

ParameterUpdateHandler::ParameterUpdateHandler(ObxfAudioProcessor &audioProcessor,
                                   const std::vector<ParameterInfo> &_parameters)
    : parameters{_parameters}, audioProcessor{audioProcessor}
{
    for (const auto &info : parameters)
    {
        juce::RangedAudioParameter *param = nullptr;
        using Type = sst::basic_blocks::params::ParamMetaData::Type;

        switch (const auto &meta = info.meta; meta.type)
        {
        case Type::FLOAT:
        case Type::BOOL:
        case Type::INT:
            param = new ObxfParameterFloat(
                juce::ParameterID{info.ID, info.versionHint},
                juce::NormalisableRange(meta.minVal, meta.maxVal, 0.00001f, 1.f), 0, meta);
            break;
        case Type::NONE:
        default:
            continue;
        }

        audioProcessor.addParameter(param);
        paramMap[info.ID] = param;
        param->addListener(this);
    }

    for (auto &p : paramMap)
    {
        if (p.first.toStdString() == SynthParam::ID::LFO1Rate)
        {
            auto *op = dynamic_cast<ObxfParameterFloat *>(p.second);
            op->setTempoSyncToggleParam(paramMap[juce::String{SynthParam::ID::LFO1TempoSync}]);
        }
        if (p.first.toStdString() == SynthParam::ID::LFO2Rate)
        {
            auto *op = dynamic_cast<ObxfParameterFloat *>(p.second);
            op->setTempoSyncToggleParam(paramMap[juce::String{SynthParam::ID::LFO2TempoSync}]);
        }
    }
}

ParameterUpdateHandler::~ParameterUpdateHandler()
{
    std::lock_guard<std::mutex> cblg(callbackMutex);

    paramMap.clear();
    callbacks.clear();
}

void ParameterUpdateHandler::parameterValueChanged(int parameterIndex, float newValue)
{
    const auto paramID = parameters[parameterIndex].ID;
    queueParameterChange(paramID, newValue);
}

bool ParameterUpdateHandler::addParameterCallback(const juce::String &ID, const juce::String &purpose,
                                            const callbackFn_t &cb)
{
    if (ID.isNotEmpty() && cb)
    {
        auto usePurpose = purpose;
        if (purpose.isEmpty())
        {
            usePurpose = "unk";
        }
        {
            std::lock_guard<std::mutex> cblg(callbackMutex);
            callbacks[ID][purpose] = cb;
        }
        return true;
    }
    jassertfalse;
    return false;
}

bool ParameterUpdateHandler::removeParameterCallback(const juce::String &ID, const juce::String &purpose)
{
    std::lock_guard<std::mutex> cblg(callbackMutex);

    auto cbit = callbacks.find(ID);
    if (cbit != callbacks.end())
    {
        auto purpit = cbit->second.find(purpose);
        if (purpit != cbit->second.end())
        {
            cbit->second.erase(purpit);
        }
    }
    return true;
}

void ParameterUpdateHandler::updateParameters(const bool force)
{
    if (force)
    {
        std::lock_guard<std::mutex> cblg(callbackMutex);

        std::for_each(callbacks.begin(), callbacks.end(), [this](auto &p) {
            if (auto *param = getParameter(p.first))
            {
                float value = param->getValue();

                OBLOG(paramSet, "FORCE: " << p.first << "=" << value);
                for (auto &[_, cb] : p.second)
                    cb(value, true);
            }
        });
    }

    auto newParam = fifo.popParameter();
    while (newParam.first)
    {
        std::lock_guard<std::mutex> cblg(callbackMutex);

        if (auto it = callbacks.find(newParam.second.parameterID); it != callbacks.end())
        {
            if (/* auto par = */ getParameter(newParam.second.parameterID))
            {
                OBLOG(paramSet, "UPDATE " << newParam.second.parameterID << " = "
                                          << newParam.second.newValue);
                for (auto &[_, cb] : it->second)
                    cb(newParam.second.newValue, false);
            }
        }
        newParam = fifo.popParameter();
    }
}

void ParameterUpdateHandler::forceSingleParameterCallback(const juce::String &paramID, float newValue)
{
    if (auto it = callbacks.find(paramID); it != callbacks.end())
        for (auto &[_, cb] : it->second)
            cb(newValue, true);
}

juce::RangedAudioParameter *ParameterUpdateHandler::getParameter(const juce::String &paramID) const
{
    if (const auto it = paramMap.find(paramID); it != paramMap.end())
        return it->second;
    return nullptr;
}

void ParameterUpdateHandler::queueParameterChange(const juce::String &paramID, float newValue)
{
    fifo.pushParameter(paramID, newValue);
}

void ParameterUpdateHandler::addParameter(const juce::String &paramID, juce::RangedAudioParameter *param)
{
    OBLOG(params, "Adding param to param map : " << paramID);
    paramMap[paramID] = param;
}

void ParameterUpdateHandler::parameterGestureChanged(int idx, bool b)
{
    if (!supressGestureToUndo)
    {
        OBLOG(undo,
              "Parameter index " << idx << " " << (b ? "started" : "ended") << " gesture for undo")
    }
}

void ParameterUpdateHandler::clearFIFO()
{
    if (!isFIFOClear())
        OBLOG(params, "Clearing non-empty FIFO");
}

bool ParameterUpdateHandler::isFIFOClear() { return fifo.isClear(); }
