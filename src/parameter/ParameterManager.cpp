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
#include "FreeLayoutHelper.h"

ParameterManager::ParameterManager(juce::AudioProcessor &audioProcessor,
                                   const juce::String &identifier,
                                   const std::vector<ParameterInfo> &_parameters)
    : apvts(audioProcessor, nullptr, identifier, createParameterLayout(_parameters)),
      parameters{_parameters}
{
}

ParameterManager::~ParameterManager()
{
    for (const auto &[fst, snd] : callbacks)
        apvts.removeParameterListener(fst, this);
}

bool ParameterManager::registerParameterCallback(const juce::String &ID, Callback cb)
{
    if (ID.isNotEmpty() && cb)
    {
        if (callbacks.find(ID) == callbacks.end())
        {
            apvts.addParameterListener(ID, this);
            callbacks[ID] = cb;
            return true;
        }
    }
    return false;
}

void ParameterManager::updateParameters(bool force)
{
    int processed = 0;
    juce::String processedParams;

    if (force)
    {
        std::for_each(callbacks.begin(), callbacks.end(), [this, &processedParams](auto &p) {
            if (auto *param = apvts.getParameter(p.first))
            {
                float value = param->getValue();
                processedParams += p.first + "=" + juce::String(value) + ", ";
                p.second(value, true);
            }
        });
        fifo.clear();
        DBG("Force updated all parameters: " + processedParams);
    }

    auto newParam = fifo.popParameter();
    while (newParam.first)
    {
        if (auto it = callbacks.find(newParam.second.parameterID); it != callbacks.end())
        {
            if (apvts.getParameter(newParam.second.parameterID))
            {
                const float normValue = juce::jlimit(0.0f, 1.0f, newParam.second.newValue);
                processedParams += juce::String(newParam.second.parameterID) + "=" +
                                   juce::String(normValue) + ", ";
                it->second(normValue, false);
                processed++;
            }
        }
        newParam = fifo.popParameter();
    }

    if (processed > 0)
    {
        DBG("Processed " + juce::String(processed) + " parameters from FIFO: " + processedParams);
    }
}

void ParameterManager::clearParameterQueue() { fifo.clear(); }

const std::vector<ParameterInfo> &ParameterManager::getParameters() const { return parameters; }

juce::AudioProcessorValueTreeState &ParameterManager::getAPVTS() { return apvts; }

void ParameterManager::parameterChanged(const juce::String &parameterID, float newValue)
{
    fifo.pushParameter(parameterID, newValue);
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
