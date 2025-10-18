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

#include "ParameterManager.h"
#include "SynthParam.h"
#include "PluginProcessor.h"

ParameterManager::ParameterManager(ObxfAudioProcessor &audioProcessor,
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

ParameterManager::~ParameterManager()
{
    std::lock_guard<std::mutex> cblg(callbackMutex);

    paramMap.clear();
    callbacks.clear();
}

void ParameterManager::parameterValueChanged(int parameterIndex, float newValue)
{
    const auto paramID = parameters[parameterIndex].ID;
    queueParameterChange(paramID, newValue);
}

bool ParameterManager::addParameterCallback(const juce::String &ID, const juce::String &purpose,
                                            const Callback &cb)
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

bool ParameterManager::removeParameterCallback(const juce::String &ID, const juce::String &purpose)
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

#define DEBUG_PARAM_SETS 0
void ParameterManager::updateParameters(const bool force)
{
#if DEBUG_PARAM_SETS
    int processed = 0;
    juce::String processedParams;
#endif

    if (force)
    {
        std::lock_guard<std::mutex> cblg(callbackMutex);

        std::for_each(callbacks.begin(), callbacks.end(),
#if DEBUG_PARAM_SETS
                      [this, &processedParams](auto &p)
#else
            [this](auto &p)
#endif
                      {
                          if (auto *param = getParameter(p.first))
                          {
                              float value = param->getValue();
#if DEBUG_PARAM_SETS
                              processedParams += p.first + "=" + juce::String(value) + ", ";
#endif
                              for (auto &[_, cb] : p.second)
                                  cb(value, true);
                          }
                      });
        fifo.clear();
        // DBG("Force updated all parameters: " + processedParams);
    }

    auto newParam = fifo.popParameter();
    while (newParam.first)
    {
        std::lock_guard<std::mutex> cblg(callbackMutex);

        if (auto it = callbacks.find(newParam.second.parameterID); it != callbacks.end())
        {
            if (/* auto par = */ getParameter(newParam.second.parameterID))
            {
#if DEBUG_PARAM_SETS
                processedParams += juce::String(newParam.second.parameterID) + "=" +
                                   juce::String(newParam.second.newValue) + ", ";
#endif
                for (auto &[_, cb] : it->second)
                    cb(newParam.second.newValue, false);
#if DEBUG_PARAM_SETS
                processed++;
#endif
            }
        }
        newParam = fifo.popParameter();
    }

#if DEBUG_PARAM_SETS
    if (processed > 0)
    {
        DBG("Processed " + juce::String(processed) + " parameters from FIFO: " + processedParams);
    }
#endif
}

void ParameterManager::forceSingleParameterCallback(const juce::String &paramID, float newValue)
{
    if (auto it = callbacks.find(paramID); it != callbacks.end())
        for (auto &[_, cb] : it->second)
            cb(newValue, true);
}

void ParameterManager::clearParameterQueue() { fifo.clear(); }

const std::vector<ParameterInfo> &ParameterManager::getParameters() const { return parameters; }

juce::RangedAudioParameter *ParameterManager::getParameter(const juce::String &paramID) const
{
    if (const auto it = paramMap.find(paramID); it != paramMap.end())
        return it->second;
    return nullptr;
}

void ParameterManager::queueParameterChange(const juce::String &paramID, float newValue)
{
    fifo.pushParameter(paramID, newValue);
}

void ParameterManager::addParameter(const juce::String &paramID, juce::RangedAudioParameter *param)
{
    paramMap[paramID] = param;
}

void ParameterManager::flushParameterQueue()
{
    auto newParam = fifo.popParameter();
    int processed = 0;
    juce::String processedParams;

    while (newParam.first)
    {
        std::lock_guard<std::mutex> cblg(callbackMutex);

        if (auto it = callbacks.find(newParam.second.parameterID); it != callbacks.end())
        {
            processedParams += juce::String(newParam.second.parameterID) + "=" +
                               juce::String(newParam.second.newValue) + ", ";
            for (auto &[_, cb] : it->second)
                cb(newParam.second.newValue, false);
            processed++;
        }
        newParam = fifo.popParameter();
    }

    if (processed > 0)
    {
        DBG("Flushed " + juce::String(processed) + " parameters from FIFO: " + processedParams);
    }
}

void ParameterManager::parameterGestureChanged(int, bool)
{
    if (!supressGestureToDirty)
    {
        audioProcessor.setCurrentProgramDirtyState(true);
    }
}
