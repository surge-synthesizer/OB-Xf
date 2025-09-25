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

#include <MidiHandler.h>
#include <Utils.h>
#include <engine/SynthEngine.h>
#include "MidiMap.h"

MidiHandler::MidiHandler(SynthEngine &s, MidiMap &b, ParameterManagerAdapter &pm, Utils &utils)
    : utils(utils), synth(s), bindings(b), paramManager(pm)
{
}

void MidiHandler::setLastUsedParameter(const juce::String &paramId)
{
    lastUsedParameter = -1;

    for (const auto &paramInfo : ParameterList)
    {
        if (paramInfo.ID == paramId)
        {
            lastUsedParameter = paramInfo.meta.id;
            break;
        }
    }
}

void MidiHandler::prepareToPlay()
{
    nextMidi = std::make_unique<juce::MidiMessage>(0xF0);
    midiMsg = std::make_unique<juce::MidiMessage>(0xF0);
}

void MidiHandler::processMidiPerSample(juce::MidiBufferIterator *iter,
                                       const juce::MidiBuffer &midiBuffer, const int samplePos)
{
    while (getNextEvent(iter, midiBuffer, samplePos))
    {
        if (!midiMsg)
            continue;

        const auto size = midiMsg->getRawDataSize();
        if (size < 1)
            continue;

        const auto *data = midiMsg->getRawData();
        if (!data)
            continue;

        const auto status = data[0] & 0xF0;
        if (status != 0x80 && status != 0x90 && status != 0xB0 && status != 0xC0 && status != 0xE0)
            continue;

        // DBG("Valid Message: " << (int)midiMsg->getChannel() << " "
        //     << (int)status << " "
        //     << (size > 1 ? (int)data[1] : 0) << " "
        //     << (size > 2 ? (int)data[2] : 0));

        if (midiMsg->isNoteOn())
        {
            synth.processNoteOn(midiMsg->getNoteNumber(), midiMsg->getFloatVelocity(),
                                midiMsg->getChannel() - 1);
        }
        else if (midiMsg->isNoteOff())
        {
            synth.processNoteOff(midiMsg->getNoteNumber(), midiMsg->getFloatVelocity(),
                                 midiMsg->getChannel() - 1);
        }
        if (midiMsg->isPitchWheel())
        {
            synth.processPitchWheel((midiMsg->getPitchWheelValue() - 8192) / 8192.0f);
        }
        if (midiMsg->isController() && midiMsg->getControllerNumber() == 1)
        {
            synth.processModWheel(midiMsg->getControllerValue() / 127.0f);
        }
        if (midiMsg->isSustainPedalOn())
        {
            synth.sustainOn();
        }
        if (midiMsg->isSustainPedalOff() || midiMsg->isAllNotesOff() || midiMsg->isAllSoundOff())
        {
            synth.sustainOff();
        }
        if (midiMsg->isAllNotesOff())
        {
            synth.allNotesOff();
        }
        if (midiMsg->isAllSoundOff())
        {
            synth.allSoundOff();
        }

        // DBG(" Message: " << midiMsg->getChannel() << " "
        //     << " " << ((size > 2) ? midiMsg->getRawData()[2] : 0));

        if (midiMsg->isProgramChange()) // xC0
        {
            if (onProgramChangeCallback)
                onProgramChangeCallback(midiMsg->getProgramChangeNumber());
        }
        else if (midiMsg->isController()) // xB0
        {
            lastMovedController = midiMsg->getControllerNumber();

            if (paramManager.midiLearnAttachment.get())
            {
                midiControlledParamSet = true;
                bindings.updateCC(lastUsedParameter, lastMovedController);

                // TODO: do this off-thread!
                juce::File midi_file =
                    utils.getMidiFolderFor(Utils::LocationType::USER).getChildFile("Custom.xml");
                bindings.saveFile(midi_file);
                currentMidiPath = midi_file.getFullPathName();

                paramManager.midiLearnAttachment.set(false);
            }

            if (bindings[lastMovedController] > 0)
            {
                for (const auto &paramInfo : ParameterList)
                {
                    if (paramInfo.meta.id == static_cast<uint32_t>(bindings[lastMovedController]))
                    {
                        paramManager.setEngineParameterValue(
                            synth, paramInfo.ID, midiMsg->getControllerValue() / 127.0f, true);
                        break;
                    }
                }

                paramManager.midiLearnAttachment.set(false);
                lastMovedController = 0;
                lastUsedParameter = 0;
                midiControlledParamSet = false;
            }
        }
    }
}

bool MidiHandler::getNextEvent(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer,
                               const int samplePos)
{
    if (iter == nullptr || *iter == midiBuffer.end())
        return false;

    while (*iter != midiBuffer.end())
    {
        if (const auto metadata = **iter;
            metadata.samplePosition >= samplePos && metadata.getMessage().getRawDataSize() > 0)
        {
            *midiMsg = metadata.getMessage();
            midiEventPos = metadata.samplePosition;
            ++(*iter);
            return true;
        }
        ++(*iter);
    }
    return false;
}

void MidiHandler::initMidi()
{
    // Documents > Obxf > MIDI > Default.xml
    if (juce::File default_file =
            utils.getMidiFolderFor(Utils::LocationType::USER).getChildFile("Default.xml");
        !default_file.exists())
    {
        bindings.saveFile(default_file);
    }

    const juce::File midi_config_file =
        utils.getMidiFolderFor(Utils::LocationType::USER).getChildFile("Config.xml");
    juce::XmlDocument xmlDoc(midi_config_file);

    if (const std::unique_ptr<juce::XmlElement> ele_file =
            xmlDoc.getDocumentElementIfTagMatches("File"))
    {
        const juce::String file_name = ele_file->getStringAttribute("name");
        // Midi cc loading
        if (juce::File midi_file =
                utils.getMidiFolderFor(Utils::LocationType::USER).getChildFile(file_name);
            bindings.loadFile(midi_file))
        {
            currentMidiPath = midi_file.getFullPathName();
        }
        else
        {
            if (juce::File xml =
                    utils.getMidiFolderFor(Utils::LocationType::USER).getChildFile("Default.xml");
                bindings.loadFile(xml))
            {
                currentMidiPath = xml.getFullPathName();
            }
        }
    }
}

void MidiHandler::updateMidiConfig() const
{
    const juce::File midi_config_file =
        utils.getMidiFolderFor(Utils::LocationType::USER).getChildFile("Config.xml");
    juce::XmlDocument xmlDoc(midi_config_file);
    if (const std::unique_ptr<juce::XmlElement> ele_file =
            xmlDoc.getDocumentElementIfTagMatches("File"))
    {
        const juce::File f(currentMidiPath);
        ele_file->setAttribute("name", f.getFileName());
        ele_file->writeTo(midi_config_file.getFullPathName());
    }
}