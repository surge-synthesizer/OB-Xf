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
    OBLOG(state, "GetStateInformation");

    auto xmlState = juce::XmlElement("OB-Xf");

    xmlState.setAttribute(S("ob-xf_version"), humanReadableVersion(currentStreamingVersion));
    xmlState.setAttribute(S("single-program-format"), 1);
    {
        const auto &program = audioProcessor->getActiveProgram();
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

        xmlState.addChildElement(xpr);
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

void StateManager::getProgramStateInformation(juce::MemoryBlock &destData) const
{
    auto xmlState = juce::XmlElement("OB-Xf");
    xmlState.setAttribute(S("ob-xf_version"), humanReadableVersion(currentStreamingVersion));

    const Program &prog = audioProcessor->getActiveProgram();

    for (const auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
    {
        const auto &paramId = param->paramID;
        auto it = prog.values.find(paramId);
        const float value = (it != prog.values.end()) ? it->second.load() : 0.0f;
        xmlState.setAttribute(paramId, value);
    }

    xmlState.setAttribute(S("voiceCount"), MAX_VOICES);
    xmlState.setAttribute(S("programName"), prog.getName());

    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::setStateInformation(const void *data, int sizeInBytes,
                                       bool restoreCurrentProgram)
{
    OBLOG(state, "setStateInformation");
    const std::unique_ptr<juce::XmlElement> xmlState =
        ObxfAudioProcessor::getXmlFromBinary(data, sizeInBytes);

    // DBG(" XML:" << (xmlState ? xmlState->toString() : juce::String("null")));
    if (xmlState)
    {
        auto ver = xmlState->getStringAttribute("ob-xf_version");
        auto verNo = fromHumanReadableVersion(ver.toStdString());
        OBLOG(state, "Streaming version: " << ver << " (" << verNo << ")");
        // this order matters!

        auto e = xmlState->getChildByName("program");
        if (!e)
        {
            OBLOG(state, "No program element found!");
            return;
        }

        const bool newFormat = e->hasAttribute("voiceCount");

        auto &program = audioProcessor->getActiveProgram();
        program.setDefaultValues();

        for (auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
        {
            const auto &paramId = param->paramID;
            float value = 0.0f;
            if (e->hasAttribute(paramId))
                value = static_cast<float>(e->getDoubleAttribute(paramId, program.values[paramId]));
            else
                value = program.values[paramId];

            if (!newFormat && paramId == "POLYPHONY")
                value *= 0.25f;

            program.values[paramId] = value;

            /*
             * We are updating all the programs in a bank but only one
             * of them is current so only one needs to notify the host
             * of the param change. But only do this for the front program
             */
            param->beginChangeGesture();
            param->setValueNotifyingHost(value);
            param->endChangeGesture();
        }

        program.setName(e->getStringAttribute(S("programName"), S(INIT_PATCH_NAME)));

        auto desp = xmlState->getChildByName(S("dawExtraState"));

        if (desp)
        {
            dawExtraState.fromElement(desp->getFirstChildElement());
        }

        sendChangeMessage();
    }
}

void StateManager::setProgramStateInformation(const void *data, const int sizeInBytes)
{
    if (const std::unique_ptr<juce::XmlElement> e =
            juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
    {
        OBLOG(patches, std::string((char *)data, (char *)data + sizeInBytes));
        Program &prog = audioProcessor->getActiveProgram();
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

        audioProcessor->processActiveProgramChanged();
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

    if (compareMagic(set->fxMagic, "FPCh")) // patch memory chunk
    {
        const auto *const cset = static_cast<const fxProgramSet *>(data);

        if (static_cast<size_t>(fxbSwap(cset->chunkSize)) + sizeof(fxProgramSet) - 8 >
            static_cast<size_t>(dataSize))
            return false;
        OBLOG(state, "Reading FPCh block size " << dataSize);
        setProgramStateInformation(cset->chunk, fxbSwap(cset->chunkSize));
    }
    else
    {
        OBLOG(state, "Support for format " << set->fxMagic << " not available");
        return false;
    }

    return true;
}

bool StateManager::restoreProgramSettings(const fxProgram *const prog) const
{
#if 0
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
#endif
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
