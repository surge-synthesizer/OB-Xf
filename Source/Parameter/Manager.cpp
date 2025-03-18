#include "Manager.h"
#include "Helper.h"

ParameterManager::ParameterManager(juce::AudioProcessor &audioProcessor,
                                   const juce::String &identifier,
                                   const std::vector<ParameterInfo> &_parameters) : apvts(
        audioProcessor, nullptr, identifier, createParameterLayout(_parameters)),
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
            if (auto *raw{apvts.getRawParameterValue(p.first)})
            {
                float value = raw->load();
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
        auto it = callbacks.find(newParam.second.first);
        if (it != callbacks.end())
        {
            processedParams += newParam.second.first + "=" + juce::String(newParam.second.second) + ", ";
            it->second(newParam.second.second, false);
            processed++;
        }
        newParam = fifo.popParameter();
    }

    if (processed > 0)
        DBG("Processed " + juce::String(processed) + " parameters from FIFO: " + processedParams);
}

void ParameterManager::clearParameterQueue()
{
    fifo.clear();
}

const std::vector<ParameterInfo> &ParameterManager::getParameters() const
{
    return parameters;
}

juce::AudioProcessorValueTreeState &ParameterManager::getAPVTS()
{
    return apvts;
}

void ParameterManager::setStateInformation(const void *data, int sizeInBytes)
{
    juce::ValueTree newState{juce::ValueTree::readFromData(data, sizeInBytes)};
    apvts.replaceState(newState);
}

void ParameterManager::getStateInformation(juce::MemoryBlock &destData)
{
    juce::MemoryOutputStream mos(destData, false);
    apvts.state.writeToStream(mos);
}

void ParameterManager::parameterChanged(const juce::String &parameterID, float newValue)
{
    fifo.pushParameter(parameterID, newValue);
}

void ParameterManager::flushParameterQueue() {
    auto newParam = fifo.popParameter();
    int processed = 0;
    juce::String processedParams;

    while (newParam.first) {
        if (auto it = callbacks.find(newParam.second.first); it != callbacks.end()) {
            processedParams += newParam.second.first + "=" + juce::String(newParam.second.second) + ", ";
            it->second(newParam.second.second, true);
            processed++;
        }
        newParam = fifo.popParameter();
    }

    if (processed > 0) {
        DBG("Flushed " + juce::String(processed) + " parameters from FIFO: " + processedParams);
    }
}