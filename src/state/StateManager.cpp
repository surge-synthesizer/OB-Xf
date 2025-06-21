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

#include "StateManager.h"
#include "PluginProcessor.h"

template <typename T> juce::String S(const T &text) { return juce::String(text); }

StateManager::~StateManager() = default;

void StateManager::getStateInformation(juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("OB-Xf");
    xmlState.setAttribute(S("currentProgram"), audioProcessor->getPrograms().currentProgram);
    xmlState.setAttribute(S("ob-xf_version"), OBXF_VERSION_STR);

    auto *xprogs = new juce::XmlElement("programs");
    const auto& paramInfos = audioProcessor->getParamManager().getParameters();

    for (auto &program : audioProcessor->getPrograms().programs)
    {
        auto *xpr = new juce::XmlElement("program");
        xpr->setAttribute(S("programName"), program.getName());
        xpr->setAttribute(S("voiceCount"), MAX_VOICES);

        for (size_t k = 0; k < paramInfos.size(); ++k)
        {
            const auto& paramId = paramInfos[k].ID;
            xpr->setAttribute(paramId, program.values[k]);
        }

        xprogs->addChildElement(xpr);
    }

    xmlState.addChildElement(xprogs);

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::getCurrentProgramStateInformation(juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("OB-Xf");

    if (const Parameters *prog = audioProcessor->getPrograms().currentProgramPtr.load())
    {
        const auto& paramInfos = audioProcessor->getParamManager().getParameters();
        for (size_t k = 0; k < paramInfos.size(); ++k)
        {
            const auto& paramId = paramInfos[k].ID;
            xmlState.setAttribute(paramId, prog->values[k]);
        }

        xmlState.setAttribute(S("voiceCount"), MAX_VOICES);
        xmlState.setAttribute(S("programName"), prog->getName());
    }

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::setStateInformation(const void *data, int sizeInBytes,
                                       bool restoreCurrentProgram)
{

    const std::unique_ptr<juce::XmlElement> xmlState =
        ObxfAudioProcessor::getXmlFromBinary(data, sizeInBytes);

    DBG(" XML:" << xmlState->toString());
    if (xmlState)
    {
        if (const juce::XmlElement *xprogs = xmlState->getFirstChildElement();
            xprogs && xprogs->hasTagName(S("programs")))
        {
            int i = 0;
            const auto& paramInfos = audioProcessor->getParamManager().getParameters();

            for (const auto *e : xprogs->getChildIterator())
            {
                const bool newFormat = e->hasAttribute("voiceCount");
                audioProcessor->getPrograms().programs[i].setDefaultValues();

                for (size_t k = 0; k < paramInfos.size(); ++k)
                {
                    float value = 0.0;
                    const auto& paramId = paramInfos[k].ID;

                    if (e->hasAttribute(paramId))
                    {
                        value = static_cast<float>(e->getDoubleAttribute(
                            paramId, audioProcessor->getPrograms().programs[i].values[k]));
                    }

                    if (!newFormat && paramId == "VOICE_COUNT")
                        value *= 0.25f;
                    audioProcessor->getPrograms().programs[i].values[k] = value;
                }

                audioProcessor->getPrograms().programs[i].setName(
                    e->getStringAttribute(S("programName"), S("Default")));

                ++i;
            }
        }

        if (restoreCurrentProgram)
            audioProcessor->setCurrentProgram(audioProcessor->getPrograms().currentProgram);

        sendChangeMessage();
    }
}

void StateManager::setCurrentProgramStateInformation(const void *data, const int sizeInBytes)
{
    if (const std::unique_ptr<juce::XmlElement> e =
            juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
    {
        if (Parameters *prog = audioProcessor->getPrograms().currentProgramPtr.load())
        {
            prog->setDefaultValues();
            const auto& paramInfos = audioProcessor->getParamManager().getParameters();
            const bool newFormat = e->hasAttribute("voiceCount");

            for (size_t k = 0; k < paramInfos.size(); ++k)
            {
                float value = 0.0f;
                const auto& paramId = paramInfos[k].ID;

                if (e->hasAttribute(paramId))
                {
                    value = static_cast<float>(e->getDoubleAttribute(paramId, prog->values[k]));
                }

                if (!newFormat && paramId == "VOICE_COUNT")
                    value *= 0.25f;
                prog->values[k] = value;
            }

            prog->setName(e->getStringAttribute(S("programName"), S("Default")));
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
            const int progLen = static_cast<int>(sizeof(fxProgram)) +
                                (numParams - 1) * static_cast<int>(sizeof(float));

            for (int i = 0; i < fxbSwap(set->numPrograms); ++i)
            {
                if (i != oldProg)
                {
                    const auto *const prog =
                        (const fxProgram *)(((const char *)(set->programs)) + i * progLen);
                    if (((const char *)prog) - ((const char *)set) >=
                        static_cast<std::ptrdiff_t>(dataSize))
                        return false;

                    if (fxbSwap(set->numPrograms) > 0)
                        audioProcessor->setCurrentProgram(i);

                    if (!restoreProgramSettings(prog))
                        return false;
                }
            }

            if (fxbSwap(set->numPrograms) > 0)
                audioProcessor->setCurrentProgram(oldProg);

            const auto *const prog =
                (const fxProgram *)(((const char *)(set->programs)) + oldProg * progLen);
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
        // non-patch chunk
        const auto *const cset = static_cast<const fxChunkSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxChunkSet) - 8 >
            static_cast<size_t>(dataSize))
            return false;

        setStateInformation(cset->chunk, fxbSwap(cset->chunkSize), false);
        const int currentProg = audioProcessor->getCurrentProgram();
        audioProcessor->setCurrentProgram(currentProg, true);
    }
    else if (compareMagic(set->fxMagic, "FPCh"))
    {
        // patch chunk
        const auto *const cset = static_cast<const fxProgramSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxProgramSet) - 8 >
            static_cast<size_t>(dataSize))
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
    if (compareMagic(prog->chunkMagic, "CcnK") && compareMagic(prog->fxMagic, "FxCk"))
    {
        audioProcessor->changeProgramName(audioProcessor->getCurrentProgram(), prog->prgName);

        for (int i = 0; i < fxbSwap(prog->numParams); ++i)
            audioProcessor->setEngineParameterValue(i, fxbSwapFloat(prog->params[i]));

        return true;
    }

    return false;
}