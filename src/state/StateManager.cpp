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

void StateManager::getPluginStateInformation(juce::MemoryBlock &destData) const
{
    OBLOG(state, "GetStateInformation");

    auto xmlState = juce::XmlElement("OB-Xf");

    xmlState.setAttribute(S("ob-xf_version"), humanReadableVersion(currentStreamingVersion));
    xmlState.setAttribute(S("single-program-format"), 1);
    {
        auto *xpr = new juce::XmlElement("program");
        getActiveProgramStateOnto(*xpr);
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

    getActiveProgramStateOnto(xmlState);
    juce::AudioProcessor::copyXmlToBinary(xmlState, destData);
}

void StateManager::getActiveProgramStateOnto(juce::XmlElement &xmlState) const
{
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
    xmlState.setAttribute(S("author"), prog.getAuthor());
    xmlState.setAttribute(S("license"), prog.getLicense());
}

void StateManager::setPluginStateInformation(const void *data, int sizeInBytes)
{
    OBLOG(state, "setStateInformation");
    const std::unique_ptr<juce::XmlElement> xmlState =
        ObxfAudioProcessor::getXmlFromBinary(data, sizeInBytes);

    if (xmlState)
    {
        auto ver = xmlState->getStringAttribute("ob-xf_version");
        auto verNo = fromHumanReadableVersion(ver.toStdString());
        OBLOG(state, "Streaming version: " << ver << " (" << verNo << ")");

        auto pnode = xmlState->getChildByName("program");
        if (!pnode)
        {
            OBLOG(state, "No program element found!");
            return;
        }

        setActiveProgramStateFrom(*pnode, verNo);

        if (auto desp = xmlState->getChildByName(S("dawExtraState")))
        {
            dawExtraState.fromElement(desp->getFirstChildElement());
        }

        audioProcessor->processActiveProgramChanged();
        audioProcessor->sendChangeMessage();
    }
}

void StateManager::setProgramStateInformation(const void *data, const int sizeInBytes)
{
    if (const std::unique_ptr<juce::XmlElement> e =
            juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes))
    {
        auto ver = e->getStringAttribute("ob-xf_version");
        auto verNo = fromHumanReadableVersion(ver.toStdString());

        setActiveProgramStateFrom(*e, verNo);
        audioProcessor->processActiveProgramChanged();
        audioProcessor->sendChangeMessage();
    }
}

void StateManager::setActiveProgramStateFrom(const juce::XmlElement &pnode, uint64_t versionNumber)
{
    const bool newFormat = pnode.hasAttribute("voiceCount");

    auto &program = audioProcessor->getActiveProgram();
    program.setToDefaultPatch();

    for (auto *param : ObxfAudioProcessor::ObxfParams(*audioProcessor))
    {
        const auto &paramId = param->paramID;
        float value = 0.0f;
        if (pnode.hasAttribute(paramId))
            value = static_cast<float>(pnode.getDoubleAttribute(paramId, program.values[paramId]));
        else
            value = program.values[paramId];

        if (!newFormat && paramId == "POLYPHONY")
            value *= 0.25f;

        program.values[paramId] = value;
    }

    program.setName(pnode.getStringAttribute(S("programName"), S(INIT_PATCH_NAME)));
    program.setAuthor(pnode.getStringAttribute(S("author"), S("")));
    program.setLicense(pnode.getStringAttribute(S("license"), S("")));
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
        setProgramStateInformation(cset->chunk, fxbSwap(cset->chunkSize));
    }
    else
    {
        OBLOG(state, "Support for format " << set->fxMagic << " not available");
        return false;
    }

    return true;
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
