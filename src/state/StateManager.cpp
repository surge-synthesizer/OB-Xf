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

    xmlState.setAttribute(S("ob-xf_version"), humanReadableVersion(currentStreamingVersion));
    {
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
    }

    {
        auto *oprogs = new juce::XmlElement("originalPrograms");
        for (const auto &program : bank.originalPrograms)
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

            oprogs->addChildElement(xpr);
        }
        xmlState.addChildElement(oprogs);
    }

    {
        auto *dprogs = new juce::XmlElement("bankDirtyState");
        for (int i = 0; i < MAX_PROGRAMS; ++i)
        {
            auto *xpr = new juce::XmlElement("dirty");
            xpr->setAttribute("idx", i);
            xpr->setAttribute("is", bank.getIsProgramDirty(i));
            dprogs->addChildElement(xpr);
        }
        xmlState.addChildElement(dprogs);
    }

    auto des = dawExtraState.toElement();
    if (des)
    {
        auto *desp = new juce::XmlElement("dawExtraState");
        desp->addChildElement(des.release());
        xmlState.addChildElement(desp);
    }

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::getProgramStateInformation(const int index, juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("OB-Xf");

    const int idx = (index < 0) ? audioProcessor->getCurrentProgram() : index;

    if (const auto &bank = audioProcessor->getCurrentBank(); bank.hasProgram(idx))
    {
        const Program &prog = bank.programs[idx];

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
        // this order matters!
        for (auto progNode : {S("programs"), S("originalPrograms")})
        {
            if (const juce::XmlElement *xprogs = xmlState->getChildByName(progNode); xprogs)
            {
                int i = 0;
                auto &bank = audioProcessor->getCurrentBank();

                for (const auto *e : xprogs->getChildIterator())
                {
                    if (i >= MAX_PROGRAMS)
                        break;

                    const bool newFormat = e->hasAttribute("voiceCount");

                    Program program;
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

                        program.values[paramId] = value;

                        /*
                         * We are updating all the programs in a bank but only one
                         * of them is current so only one needs to notify the host
                         * of the param change.
                         */
                        if (i == audioProcessor->getCurrentBank().getCurrentProgramIndex())
                        {
                            param->beginChangeGesture();
                            param->setValueNotifyingHost(value);
                            param->endChangeGesture();
                        }
                    }

                    program.setName(e->getStringAttribute(S("programName"), S(INIT_PATCH_NAME)));

                    // We do programs *first* so use that to fill the back.
                    // Then if a session has original programs streamed,
                    // we will overwrite the second time.
                    if (progNode == S("programs"))
                    {
                        bank.programs[i] = program;
                        bank.originalPrograms[i] = program;
                    }
                    if (progNode == S("originalPrograms"))
                    {
                        bank.originalPrograms[i] = program;
                    }
                    ++i;
                }
            }
            else
            {
                // DBG("No prog node " << progNode);
            }
        }

        if (const juce::XmlElement *xprogs = xmlState->getChildByName("bankDirtyState"); xprogs)
        {
            auto &bank = audioProcessor->getCurrentBank();

            for (const auto *e : xprogs->getChildIterator())
            {
                auto idx = e->getIntAttribute("idx", -1);
                auto isd = e->getIntAttribute("is", -1);
                if (idx >= 0 && isd >= 0 && idx < MAX_PROGRAMS)
                {
                    bank.setProgramDirty(idx, isd);
                }
            }
        }
        else
        {
            // No dirty state saved. So just assume clean
            auto &bank = audioProcessor->getCurrentBank();

            for (int i = 0; i < MAX_PROGRAMS; ++i)
            {
                bank.setProgramDirty(i, false);
            }
        }

        if (restoreCurrentProgram)
        {
            audioProcessor->setCurrentProgram(
                audioProcessor->getCurrentBank().getCurrentProgramIndex());
        }

        auto desp = xmlState->getChildByName(S("dawExtraState"));

        if (desp)
        {
            dawExtraState.fromElement(desp->getFirstChildElement());
        }

        sendChangeMessage();
    }
}

void StateManager::setProgramStateInformation(const void *data, const int sizeInBytes,
                                              const int index)
{
    if (const std::unique_ptr<juce::XmlElement> e =
            juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
    {
        const int idx = (index < 0) ? audioProcessor->getCurrentProgram() : index;

        if (auto &bank = audioProcessor->getCurrentBank(); bank.hasProgram(idx))
        {
            Program &prog = bank.programs[idx];

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
                {
                    value *= 0.25f;
                }

                prog.values[paramId] = value;
            }

            prog.setName(e->getStringAttribute(S("programName"), S(INIT_PATCH_NAME)));
        }

        if (index < 0 || idx == audioProcessor->getCurrentProgram())
            audioProcessor->setCurrentProgram(
                audioProcessor->getCurrentBank().getCurrentProgramIndex());

        sendChangeMessage();
    }
}

bool StateManager::loadFromMemoryBlock(juce::MemoryBlock &mb, const int index)
{
    const void *const data = mb.getData();
    const size_t dataSize = mb.getSize();
    const int idx = (index < 0) ? audioProcessor->getCurrentProgram() : index;

    if (dataSize < 28)
        return false;

    const auto set = static_cast<const fxSet *>(data);

    if ((!compareMagic(set->chunkMagic, "CcnK")) || fxbSwap(set->version) > fxbVersionNum)
        return false;

    if (compareMagic(set->fxMagic, "FxBk")) // bank of programs
    {
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

        audioProcessor->saveAllFrontProgramsToBack();
    }
    else if (compareMagic(set->fxMagic, "FxCk")) // single program
    {
        const auto *const prog = static_cast<const fxProgram *>(data);

        if (!compareMagic(prog->chunkMagic, "CcnK"))
            return false;

        audioProcessor->changeProgramName(idx, prog->prgName);

        for (int i = 0; i < fxbSwap(prog->numParams); ++i)
        {
            const auto &paramId = ParameterList[i].ID;
            audioProcessor->setEngineParameterValue(paramId, fxbSwapFloat(prog->params[i]));
        }

        audioProcessor->setCurrentProgram(idx, true);
        audioProcessor->saveSpecificFrontProgramToBack(idx);
    }
    else if (compareMagic(set->fxMagic, "FBCh")) // bank memory chunk
    {
        const auto *const cset = static_cast<const fxChunkSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxChunkSet) - 8 >
            static_cast<size_t>(dataSize))
            return false;

        setStateInformation(cset->chunk, fxbSwap(cset->chunkSize), false);
        audioProcessor->saveAllFrontProgramsToBack();
        const int currentProg = audioProcessor->getCurrentProgram();
        audioProcessor->setCurrentProgram(currentProg, true);
    }
    else if (compareMagic(set->fxMagic, "FPCh")) // patch memory chunk
    {
        const auto *const cset = static_cast<const fxProgramSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxProgramSet) - 8 >
            static_cast<size_t>(dataSize))
            return false;

        setProgramStateInformation(cset->chunk, fxbSwap(cset->chunkSize), idx);

        auto &bn = audioProcessor->getCurrentBank().programs[idx];

        std::string progName(bn.getName().toStdString());
        std::string fxpName(cset->name); // caps at 28 chars

        if (fxpName.empty() || (!progName.empty() && progName != INIT_PATCH_NAME))
        {
            audioProcessor->changeProgramName(idx, progName.c_str());
        }
        else
        {
            audioProcessor->changeProgramName(idx, cset->name);
        }

        audioProcessor->saveSpecificFrontProgramToBack(idx);
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

void StateManager::collectDAWExtraStateFromInstance()
{
    static_assert(NUM_MIDI_CC == 128);

    auto &mmap = audioProcessor->getMidiHandler().getMidiMap();

    dawExtraState.controllers = mmap.controllers;
    dawExtraState.selectedLFOIndex = audioProcessor->selectedLFOIndex;
}

void StateManager::applyDAWExtraStateToInstance()
{
    static_assert(NUM_MIDI_CC == 128);

    auto &mmap = audioProcessor->getMidiHandler().getMidiMap();

    mmap.controllers = dawExtraState.controllers;
    mmap.resyncParamIDCache();

    audioProcessor->selectedLFOIndex = dawExtraState.selectedLFOIndex;
}

void StateManager::DAWExtraState::fromElement(const juce::XmlElement *e)
{
    auto sv = e->getIntAttribute("version", 1);

    if (sv != desVersion)
    {
        DBG("Streaming mismatch!");
    }

    for (int idx = 0; idx < 128; ++idx)
    {
        controllers[idx] = e->getIntAttribute("controllers_" + juce::String(idx), -1);
    }

    selectedLFOIndex = e->getIntAttribute("selectedLFOIndex", 0);
}

std::unique_ptr<juce::XmlElement> StateManager::DAWExtraState::toElement() const
{
    auto res = std::make_unique<juce::XmlElement>("DAWExtraState");

    res->setAttribute("version", desVersion);

    for (int idx = 0; idx < 128; ++idx)
    {
        if (controllers[idx] >= 0)
        {
            res->setAttribute("controllers_" + juce::String(idx), controllers[idx]);
        }
    }

    res->setAttribute("selectedLFOIndex", selectedLFOIndex);

    return res;
}
