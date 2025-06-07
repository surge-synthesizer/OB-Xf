#include "StateManager.h"
#include "PluginProcessor.h"

template <typename T>
juce::String S(const T &text) { return juce::String(text); }

StateManager::~StateManager() = default;

void StateManager::getStateInformation(juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("discoDSP");
    xmlState.setAttribute(S("currentProgram"), audioProcessor->getPrograms().currentProgram);

    auto *xprogs = new juce::XmlElement("programs");
    for (auto &program : audioProcessor->getPrograms().programs)
    {
        auto *xpr = new juce::XmlElement("program");
        xpr->setAttribute(S("programName"), program.name);
        xpr->setAttribute(S("voiceCount"), Motherboard::MAX_VOICES);

        for (int k = 0; k < PARAM_COUNT; ++k)
        {
            xpr->setAttribute("Val_" + juce::String(k), program.values[k]);
        }

        xprogs->addChildElement(xpr);
    }

    xmlState.addChildElement(xprogs);

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}


void StateManager::getCurrentProgramStateInformation(juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("discoDSP");

    if (const ObxdParams* prog = audioProcessor->getPrograms().currentProgramPtr.load())
    {
        for (int k = 0; k < PARAM_COUNT; ++k)
        {
            xmlState.setAttribute("Val_" + juce::String(k), prog->values[k]);
        }

        xmlState.setAttribute(S("voiceCount"), Motherboard::MAX_VOICES);
        xmlState.setAttribute(S("programName"), prog->name);
    }

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::setStateInformation(const void *data, int sizeInBytes)
{

    const std::unique_ptr<juce::XmlElement> xmlState = ObxdAudioProcessor::getXmlFromBinary(
        data, sizeInBytes);

    DBG(" XML:" << xmlState->toString());
    if (xmlState)
    {
        if (const juce::XmlElement *xprogs = xmlState->getFirstChildElement();
            xprogs && xprogs->hasTagName(S("programs")))
        {
            int i = 0;
            for (const auto *e : xprogs->getChildIterator())
            {
                const bool newFormat = e->hasAttribute("voiceCount");
                audioProcessor->getPrograms().programs[i].setDefaultValues();

                for (int k = 0; k < PARAM_COUNT; ++k)
                {
                    float value = 0.0;
                    if (e->hasAttribute("Val_" + juce::String(k)))
                    {
                        value = static_cast<float>(e->getDoubleAttribute("Val_" + juce::String(k),
                            audioProcessor->getPrograms().programs[i].values[k]));
                    }
                    else
                    {
                        value = static_cast<float>(e->getDoubleAttribute(juce::String(k),
                            audioProcessor->getPrograms().programs[i].values[k]));
                    }

                    if (!newFormat && k == VOICE_COUNT)
                        value *= 0.25f;
                    audioProcessor->getPrograms().programs[i].values[k] = value;
                }

                audioProcessor->getPrograms().programs[i].name = e->getStringAttribute(
                    S("programName"), S("Default"));

                ++i;
            }
        }

        audioProcessor->setCurrentProgram(xmlState->getIntAttribute(S("currentProgram"), 0));

        sendChangeMessage();
    }
}

void StateManager::setCurrentProgramStateInformation(const void *data, const int sizeInBytes)
{
    if (const std::unique_ptr<juce::XmlElement> e = juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
    {
        if (ObxdParams* prog = audioProcessor->getPrograms().currentProgramPtr.load())
        {
            prog->setDefaultValues();

            const bool newFormat = e->hasAttribute("voiceCount");
            for (int k = 0; k < PARAM_COUNT; ++k)
            {
                float value = 0.0f;
                if (e->hasAttribute("Val_" + juce::String(k)))
                {
                    value = static_cast<float>(e->getDoubleAttribute("Val_" + juce::String(k), prog->values[k]));
                }
                else
                {
                    value = static_cast<float>(e->getDoubleAttribute(juce::String(k), prog->values[k]));
                }

                if (!newFormat && k == VOICE_COUNT)
                    value *= 0.25f;
                prog->values[k] = value;
            }

            prog->name = e->getStringAttribute(S("programName"), S("Default"));
        }

        audioProcessor->setCurrentProgram(audioProcessor->getPrograms().currentProgram);
        sendChangeMessage();
    }
}

bool StateManager::loadFromMemoryBlock(juce::MemoryBlock &mb)
{
    const void *const data = mb.getData();
    const size_t dataSize = mb.getSize();

    if (dataSize < 28)
        return false;

    const auto set = static_cast<const fxSet *>(data);

    if ((!compareMagic(set->chunkMagic, "CcnK")) || fxbSwap(set->version) > fxbVersionNum)
        return false;

    if (compareMagic(set->fxMagic, "FxBk"))
    {
        // bank of programs
        if (fxbSwap(set->numPrograms) >= 0)
        {
            const int oldProg = audioProcessor->getCurrentProgram();
            const int numParams = fxbSwap(static_cast<const fxProgram *>(set->programs)->numParams);
            const int progLen = static_cast<int>(sizeof(fxProgram)) + (numParams - 1) * static_cast<
                                    int>(sizeof(float));

            for (int i = 0; i < fxbSwap(set->numPrograms); ++i)
            {
                if (i != oldProg)
                {
                    const auto *const prog = (const fxProgram *)(
                        ((const char *)(set->programs)) + i * progLen);
                    if (((const char *)prog) - ((const char *)set) >= static_cast<std::ptrdiff_t>(
                            dataSize))
                        return false;

                    if (fxbSwap(set->numPrograms) > 0)
                        audioProcessor->setCurrentProgram(i);

                    if (!restoreProgramSettings(prog))
                        return false;
                }
            }

            if (fxbSwap(set->numPrograms) > 0)
                audioProcessor->setCurrentProgram(oldProg);

            const auto *const prog = (const fxProgram *)(
                ((const char *)(set->programs)) + oldProg * progLen);
            if (((const char *)prog) - ((const char *)set) >= static_cast<std::ptrdiff_t>(dataSize))
                return false;

            if (!restoreProgramSettings(prog))
                return false;
        }
    }
    else if (compareMagic(set->fxMagic, "FxCk"))
    {
        // single program
        const auto *const prog = static_cast<const fxProgram *>(data);

        if (!compareMagic(prog->chunkMagic, "CcnK"))
            return false;

        audioProcessor->changeProgramName(audioProcessor->getCurrentProgram(), prog->prgName);

        for (int i = 0; i < fxbSwap(prog->numParams); ++i)
            audioProcessor->setEngineParameterValue(i, fxbSwapFloat(prog->params[i]));
    }
    else if (compareMagic(set->fxMagic, "FBCh"))
    {
        // non-preset chunk
        const auto *const cset = static_cast<const fxChunkSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxChunkSet) - 8 > static_cast<
                size_t>(dataSize))
            return false;

        setStateInformation(cset->chunk, fxbSwap(cset->chunkSize));
        audioProcessor->setCurrentProgram(0); // Set to first preset position
    }
    else if (compareMagic(set->fxMagic, "FPCh"))
    {
        // preset chunk
        const auto *const cset = static_cast<const fxProgramSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxProgramSet) - 8 > static_cast<
                size_t>(dataSize))
            return false;

        setCurrentProgramStateInformation(cset->chunk, fxbSwap(cset->chunkSize));

        audioProcessor->changeProgramName(audioProcessor->getCurrentProgram(), cset->name);
    }
    else
    {
        return false;
    }

    return true;
}

bool StateManager::restoreProgramSettings(const fxProgram *const prog) const
{
    if (compareMagic(prog->chunkMagic, "CcnK")
        && compareMagic(prog->fxMagic, "FxCk"))
    {
        audioProcessor->changeProgramName(audioProcessor->getCurrentProgram(), prog->prgName);

        for (int i = 0; i < fxbSwap(prog->numParams); ++i)
            audioProcessor->setEngineParameterValue(i, fxbSwapFloat(prog->params[i]));

        return true;
    }

    return false;
}