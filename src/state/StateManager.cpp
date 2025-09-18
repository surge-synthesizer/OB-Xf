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
    const auto &bank = audioProcessor->getCurrentBank();
    xmlState.setAttribute(S("currentProgram"), bank.currentProgram);
    xmlState.setAttribute(S("ob-xf_version"), humanReadableVersion(currentStreamingVersion));

    xmlState.setAttribute(S("selectedLFOIndex"), audioProcessor->selectedLFOIndex);

    auto *xprogs = new juce::XmlElement("programs");

    for (const auto &program : bank.programs)
    {
        auto *xpr = new juce::XmlElement("program");
        xpr->setAttribute(S("programName"), program.getName());
        xpr->setAttribute(S("voiceCount"), MAX_VOICES);

        for (const auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
        {
            const auto &paramId = param->paramID;
            auto it = program.values.find(paramId);
            const float value = (it != program.values.end()) ? it->second.load() : 0.0f;
            xpr->setAttribute(paramId, value);
        }

        xprogs->addChildElement(xpr);
    }

    xmlState.addChildElement(xprogs);
    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::getCurrentProgramStateInformation(juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("OB-Xf");

    if (const auto &bank = audioProcessor->getCurrentBank(); bank.hasCurrentProgram())
    {
        const Parameters &prog = bank.getCurrentProgram();
        for (const auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
        {
            const auto &paramId = param->paramID;
            auto it = prog.values.find(paramId);
            const float value = (it != prog.values.end()) ? it->second.load() : 0.0f;
            xmlState.setAttribute(paramId, value);
        }
        xmlState.setAttribute(S("voiceCount"), MAX_VOICES);
        xmlState.setAttribute(S("programName"), prog.getName());
    }

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::setStateInformation(const void *data, int sizeInBytes,
                                       bool restoreCurrentProgram)
{
    const std::unique_ptr<juce::XmlElement> xmlState =
        ObxfAudioProcessor::getXmlFromBinary(data, sizeInBytes);

    // DBG(" XML:" << (xmlState ? xmlState->toString() : juce::String("null")));
    if (xmlState)
    {
        if (const juce::XmlElement *xprogs = xmlState->getFirstChildElement();
            xprogs && xprogs->hasTagName(S("programs")))
        {
            int i = 0;
            auto &bank = audioProcessor->getCurrentBank();

            for (const auto *e : xprogs->getChildIterator())
            {
                if (i >= MAX_PROGRAMS)
                    break;

                const bool newFormat = e->hasAttribute("voiceCount");
                auto &program = bank.programs[i];
                program.setDefaultValues();

                for (auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
                {
                    const auto &paramId = param->paramID;
                    float value = 0.0f;
                    if (e->hasAttribute(paramId))
                        value = static_cast<float>(
                            e->getDoubleAttribute(paramId, program.values[paramId]));
                    else
                        value = program.values[paramId];

                    if (!newFormat && paramId == "POLYPHONY")
                        value *= 0.25f;

                    param->beginChangeGesture();
                    param->setValueNotifyingHost(value);
                    param->endChangeGesture();

                    program.values[paramId] = value;
                }
                program.setName(e->getStringAttribute(S("programName"), S("Default")));
                ++i;
            }
        }
        if (xmlState->hasAttribute(S("currentProgram")))
        {
            const int currentProgram = xmlState->getIntAttribute(S("currentProgram"), 0);
            audioProcessor->getCurrentBank().currentProgram = currentProgram;
            if (restoreCurrentProgram)
                audioProcessor->setCurrentProgram(currentProgram);
        }
        else if (restoreCurrentProgram)
        {
            audioProcessor->setCurrentProgram(audioProcessor->getCurrentBank().currentProgram);
        }

        if (xmlState->hasAttribute(S("selectedLFOIndex")))
            audioProcessor->selectedLFOIndex = xmlState->getIntAttribute(S("selectedLFOIndex"), 0);

        sendChangeMessage();
    }
}

void StateManager::setCurrentProgramStateInformation(const void *data, const int sizeInBytes)
{
    if (const std::unique_ptr<juce::XmlElement> e =
            juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
    {
        if (auto &bank = audioProcessor->getCurrentBank(); bank.hasCurrentProgram())
        {
            Parameters &prog = bank.getCurrentProgram();
            prog.setDefaultValues();
            const bool newFormat = e->hasAttribute("voiceCount");

            for (const auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
            {
                const auto &paramId = param->paramID;
                float value = 0.0f;

                if (e->hasAttribute(paramId))
                {
                    auto it = prog.values.find(paramId);
                    const float defaultVal = (it != prog.values.end()) ? it->second.load() : 0.0f;
                    value = static_cast<float>(e->getDoubleAttribute(paramId, defaultVal));
                }

                if (!newFormat && paramId == "POLYPHONY")
                    value *= 0.25f;
                prog.values[paramId] = value;
            }

            prog.setName(e->getStringAttribute(S("programName"), S("Default")));
        }

        audioProcessor->setCurrentProgram(audioProcessor->getCurrentBank().currentProgram);
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
        {
            const auto &paramId = ParameterList[i].ID;
            audioProcessor->setEngineParameterValue(paramId, fxbSwapFloat(prog->params[i]));
        }
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

        int i = 0;
        for (const auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
        {
            if (i >= fxbSwap(prog->numParams))
                break;
            const auto &paramId = param->paramID;
            const float value = fxbSwapFloat(prog->params[i]);
            audioProcessor->setEngineParameterValue(paramId, value);
            ++i;
        }

        return true;
    }

    return false;
}