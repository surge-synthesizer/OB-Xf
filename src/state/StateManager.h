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

#ifndef OBXF_SRC_STATE_STATEMANAGER_H
#define OBXF_SRC_STATE_STATEMANAGER_H

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Constants.h"
#include "SynthEngine.h"
#include "Bank.h"

class ObxfAudioProcessor;

class StateManager final : public juce::ChangeBroadcaster
{
  public:
    explicit StateManager(ObxfAudioProcessor *processor) : audioProcessor(processor) {}

    ~StateManager() override;

    StateManager(const StateManager &) = delete;

    StateManager &operator=(const StateManager &) = delete;

    bool loadFromMemoryBlock(juce::MemoryBlock &mb, const int index = -1);

    void getStateInformation(juce::MemoryBlock &destData) const;
    void setStateInformation(const void *data, int sizeInBytes, bool restoreCurrentProgram);

    void getProgramStateInformation(const int index, juce::MemoryBlock &destData) const;
    void setProgramStateInformation(const void *data, int sizeInBytes, const int index = -1);

    bool restoreProgramSettings(const fxProgram *prog) const;

    /*
     * The DAW Extra State mechanism works as follows
     *
     * - On saving to the DAW (Processor:;getStateInformation) before
     * the stream we call collectDAWExtraStateFromInstance before stream.
     * That will let you put whatever you want on the instance here.
     *
     * Then in the resulting XML we put the result of dawExtraState::toElement
     *
     * On unstream it is the opposite (xml -> fromElement and then after unstream
     * we do a applyDAWExtraStateToInstance)
     *
     * that means if you add something to DES you have five steps
     *
     * - add a data member to DAWExtraState
     * - Populate it from the Processor* in collectDAWExtraStateFromInstance
     * - stream it in toElement
     * - unstream it in fromElement - but unstream it onto the DAWExtraState object
     * - apply the dawExtraState new member to the Processor in applyDAWExtraStateToInstance
     *
     * Basically the DAWExtraState object acts as a little buffer of
     * stuff we want to collect which is not stored in a patch.
     */
    void collectDAWExtraStateFromInstance();
    void applyDAWExtraStateToInstance();

  private:
    struct DAWExtraState
    {
        static constexpr int desVersion{1};

        // midi map state
        std::array<int, 128> controllers{};

        uint8_t selectedLFOIndex{0};

        std::unique_ptr<juce::XmlElement> toElement() const;
        void fromElement(const juce::XmlElement *e);
    } dawExtraState;

    ObxfAudioProcessor *audioProcessor{nullptr};
};

#endif // OBXF_SRC_STATE_STATEMANAGER_H
