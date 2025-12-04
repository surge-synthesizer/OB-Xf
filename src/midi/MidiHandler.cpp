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
#include "sst/basic-blocks/dsp/LagCollection.h"

// we smooth midi into this many steps
static constexpr int midiBlock{4};

struct MidiHandler::LagHandler
    : sst::basic_blocks::dsp::LagCollectionBase<128, MidiHandler::LagHandler>
{
    const MidiHandler &handler;
    LagHandler(const MidiHandler &h) : handler(h) {}

    void setTarget(size_t index, float target)
    {
        assert(index < 128);
        this->setTargetValue(index, target);
    }

    void applyLag(size_t index)
    {
        // Force the value in the engine change
        handler.paramCoordinator.getParameterUpdateHandler().forceSingleParameterCallback(
            handler.bindings.getParamID(index), lags[index].lag.v);
    }

    void lagCompleted(size_t index)
    {
        // Notify host when done or when snapped
        auto &uh = handler.paramCoordinator.getParameterUpdateHandler();
        uh.setSuppressGestureToUndo(true);
        handler.paramCoordinator.setEngineParameterValue(
            handler.synth, handler.bindings.getParamID(index), lags[index].lag.v, true);
        uh.updateParameters(false);
        uh.setSuppressGestureToUndo(false);
    }
};

MidiHandler::MidiHandler(SynthEngine &s, MidiMap &b, ParameterCoordinator &pm, Utils &utils)
    : utils(utils), synth(s), bindings(b), paramCoordinator(pm)
{
    lagHandler = std::make_unique<LagHandler>(*this);
}

MidiHandler::~MidiHandler() {}

void MidiHandler::setLastUsedParameter(const juce::String &paramId)
{
    lastUsedParameter = 0;

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

        if (onMidiMessageCallback && midiMsg)
        {
            onMidiMessageCallback(*midiMsg);
        }

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
        if (midiMsg->isController())
        {
            bool dontLearn = false;

            switch (midiMsg->getControllerNumber())
            {
            case 0:
                bankSelectMSB = midiMsg->getControllerValue();
                dontLearn = true;
                break;
            case 1:
                synth.processModWheel(midiMsg->getControllerValue() / 127.0f);
                dontLearn = true;
                break;
            case 64:
            case 74:
            case 120:
            case 123:
                dontLearn = true;
                break;
            default:
                break;
            }

            if (!dontLearn)
            {
                lastMovedController = midiMsg->getControllerNumber();

                if (paramCoordinator.midiLearnAttachment.get() && lastUsedParameter > 0)
                {
                    midiControlledParamSet = true;

                    bindings.updateCC(lastUsedParameter, lastMovedController);
                }

                if (bindings.isBound(lastMovedController))
                {
                    auto val = bindings.ccTo01(lastMovedController, midiMsg->getControllerValue());

                    lagHandler->setTarget(lastMovedController, val);

                    lastMovedController = 0;
                    lastUsedParameter = 0;
                    midiControlledParamSet = false;
                }
            }
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

        if (midiMsg->isProgramChange()) // xC0
        {
            if (handleMIDIProgramChangeCallback)
            {
                bool upperBank = bankSelectMSB > 1 ? false : static_cast<bool>(bankSelectMSB);
                handleMIDIProgramChangeCallback(midiMsg->getProgramChangeNumber() +
                                                (128 * upperBank));
            }
        }
    }
}
void MidiHandler::processLags()
{
    if (lagPos == 0)
    {
        lagHandler->processAll();
    }
    lagPos = (lagPos + 1) & (midiBlock - 1);
}

bool MidiHandler::getNextEvent(juce::MidiBufferIterator *iter, const juce::MidiBuffer &midiBuffer,
                               const int samplePos)
{
    if (iter == nullptr || *iter == midiBuffer.end())
        return false;

    if (const auto metadata = **iter;
        metadata.samplePosition <= samplePos && metadata.getMessage().getRawDataSize() > 0)
    {
        *midiMsg = metadata.getMessage();
        midiEventPos = metadata.samplePosition;
        ++(*iter);
        return true;
    }
    return false;
}

void MidiHandler::initMidi()
{
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

void MidiHandler::setSampleRate(double sr)
{
    lagHandler->setRateInMilliseconds(30, sr, 1.0 / midiBlock);
}

void MidiHandler::snapLags() { lagHandler->snapAllActiveToTarget(); }
