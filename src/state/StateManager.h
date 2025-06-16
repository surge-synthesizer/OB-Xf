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
#include "ObxfBank.h"

class ObxfAudioProcessor;

class StateManager final : public juce::ChangeBroadcaster
{
  public:
    explicit StateManager(ObxfAudioProcessor *processor) : audioProcessor(processor) {}

    ~StateManager() override;

    StateManager(const StateManager &) = delete;

    StateManager &operator=(const StateManager &) = delete;

    bool isMemoryBlockAPatch(const juce::MemoryBlock &mb);

    bool loadFromMemoryBlock(juce::MemoryBlock &mb);

    bool restoreProgramSettings(const fxProgram *prog) const;

    void getStateInformation(juce::MemoryBlock &destData) const;

    void getCurrentProgramStateInformation(juce::MemoryBlock &destData) const;

    void setStateInformation(const void *data, int sizeInBytes, bool restoreCurrentProgram);

    void setCurrentProgramStateInformation(const void *data, int sizeInBytes);

  private:
    ObxfAudioProcessor *audioProcessor{nullptr};
};

#endif // OBXF_SRC_STATE_STATEMANAGER_H
