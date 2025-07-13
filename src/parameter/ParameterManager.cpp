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

ParameterManager::ParameterManager(juce::AudioProcessor &audioProcessor,
                                   const std::vector<ParameterInfo> &_parameters)
    : parameters{_parameters}
{
    for (const auto &info : parameters)
    {
        juce::RangedAudioParameter *param = nullptr;
        using Type = sst::basic_blocks::params::ParamMetaData::Type;

        switch (const auto &meta = info.meta; meta.type)
        {
        case Type::FLOAT:
            param = new ObxfParameterFloat(
                juce::ParameterID{info.ID, 1},
                juce::NormalisableRange(meta.minVal, meta.maxVal, 0.00001f, 1.f), 0, meta);
            break;
        case Type::BOOL:
            param = new juce::AudioParameterBool(juce::ParameterID{info.ID, 1}, meta.name,
                                                 meta.defaultVal > 0.5f);
            break;
        case Type::NONE:
        default:
            continue;
        }

        audioProcessor.addParameter(param);
        paramMap[info.ID] = param;
        param->addListener(this);
    }
}

ParameterManager::~ParameterManager()
{
    paramMap.clear();
    callbacks.clear();
}

void ParameterManager::parameterValueChanged(int parameterIndex, float newValue)
{
    const auto paramID = parameters[parameterIndex].ID;
    queueParameterChange(paramID, newValue);
}

bool ParameterManager::registerParameterCallback(const juce::String &ID, const Callback &cb)
{
    if (ID.isNotEmpty() && cb)
    {
        if (callbacks.find(ID) == callbacks.end())
        {
            callbacks[ID] = cb;
            return true;
        }
    }
    return false;
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
                              p.second(value, true);
                          }
                      });
        fifo.clear();
        // DBG("Force updated all parameters: " + processedParams);
    }

    auto newParam = fifo.popParameter();
    while (newParam.first)
    {
        if (auto it = callbacks.find(newParam.second.parameterID); it != callbacks.end())
        {
            if (auto par = getParameter(newParam.second.parameterID))
            {
                // the set methods *shoudl* be idempotent but in the event
                // they aren't we don't want to clobber them with no change
                if (par->getValue() != newParam.second.newValue)
                {
#if DEBUG_PARAM_SETS
                    processedParams += juce::String(newParam.second.parameterID) + "=" +
                                       juce::String(newParam.second.newValue) + ", ";
#endif
                    it->second(newParam.second.newValue, false);
                }
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
        if (auto it = callbacks.find(newParam.second.parameterID); it != callbacks.end())
        {
            processedParams += juce::String(newParam.second.parameterID) + "=" +
                               juce::String(newParam.second.newValue) + ", ";
            it->second(newParam.second.newValue, true);
            processed++;
        }
        newParam = fifo.popParameter();
    }

    if (processed > 0)
    {
        DBG("Flushed " + juce::String(processed) + " parameters from FIFO: " + processedParams);
    }
}
