/*
 * OB-Xd was originally written by Vadim Filatov, and then a version
 * was released under the GPL3 at https://github.com/reales/OB-Xd.
 * Subsequently, the product was continued by DiscoDSP and the copyright
 * holders as an excellent closed source product.
 *
 * This repository is a successor to OB-Xd version 2.11.
 * Copyright 2013-2025 by the authors as indicated in the original release,
 * and subsequent authors as per GitHub transaction log.
 *
 * OB-Xf is released under the GNU General Public Licence v3 or later
 * (GPL-3.0-or-later). The license is found in the file "LICENSE"
 * in the root of this repository or at:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Source code is available at https://github.com/surge-synthesizer/OB-Xf
 */

#ifndef OBXF_SRC_MIDI_MPELAGS_H
#define OBXF_SRC_MIDI_MPELAGS_H

#include "SynthEngine.h"
#include "sst/basic-blocks/dsp/LagCollection.h"

// per-channel pressure and timbre, smoothed like the midi cc lags
struct MpeLags : sst::basic_blocks::dsp::LagCollectionBase<32, MpeLags>
{
    static constexpr int channels{16};

    // the values a voice's sources hold at note on
    static constexpr float pressureAtRest{0.f}, timbreAtRest{0.5f};

    SynthEngine &synth;
    explicit MpeLags(SynthEngine &s) : synth(s) {}

    // lane 0 drives every voice: the mpe master channel, or any channel without mpe
    static int lane(int midiChannel, bool mpe)
    {
        return mpe && midiChannel != 1 ? midiChannel - 1 : 0;
    }

    void setPressure(int midiChannel, bool mpe, float v)
    {
        setTargetValue(lane(midiChannel, mpe), v);
    }

    void setTimbre(int midiChannel, bool mpe, float v)
    {
        setTargetValue(channels + lane(midiChannel, mpe), v);
    }

    // a member channel's new note starts from rest, so its lags must too
    void noteOn(int midiChannel, bool mpe)
    {
        const auto l = lane(midiChannel, mpe);

        if (l == 0)
        {
            return;
        }

        lags[l].lag.snapTo(pressureAtRest);
        lags[channels + l].lag.snapTo(timbreAtRest);
        activeSet.removeFromActive(lags[l]);
        activeSet.removeFromActive(lags[channels + l]);
    }

    bool isActive() const { return activeSet.begin() != activeSet.end(); }

    void applyLag(size_t index)
    {
        const auto l = static_cast<int>(index % channels);
        const auto ch = static_cast<int8_t>(l == 0 ? -1 : l);
        const auto v = lags[index].lag.v;

        if (index < channels)
        {
            synth.processMPEChannelPressure(ch, v);
        }
        else
        {
            synth.processMPETimbre(ch, v);
        }
    }

    void lagCompleted(size_t) {}
};

#endif // OBXF_SRC_MIDI_MPELAGS_H
