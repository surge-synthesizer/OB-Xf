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

#ifndef OBXF_SRC_ENGINE_TUNING_H
#define OBXF_SRC_ENGINE_TUNING_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

#include "libMTSClient.h"

class Tuning
{
  public:
    // each key's pitch in semitones
    using KeyTable = std::array<double, 128>;

  private:
    MTSClient *mts_client{nullptr};

    enum Mode
    {
        MTS_ESP,
        FIXED,
        TWELVE_TET
    } mode{TWELVE_TET};

    static constexpr KeyTable equalKeys = [] {
        KeyTable t{};
        for (int k = 0; k < 128; ++k)
        {
            t[k] = k;
        }
        return t;
    }();

    /*
     * One table per MIDI channel, asked of MTS-ESP the first time its channel plays
     * in a host buffer. Under MPE every note shares the first, asked without a channel.
     */
    std::array<KeyTable, 16> tables{};
    uint32_t fresh{0};
    bool mpe{false};

  public:
    // bends that move along the scale from the note rather than in 12-TET above it
    std::atomic<bool> scaleAwareWheel{true}, scaleAwareMPE{true};

    Tuning() {}

    ~Tuning()
    {
        if (mts_client != nullptr)
        {
            MTS_DeregisterClient(mts_client);
            mts_client = nullptr;
        }
    }

    // once per host buffer, from the audio thread
    void updateMTSESPStatus(bool mpeEnabled)
    {
        if (mts_client == nullptr)
        {
            mts_client = MTS_RegisterClient();
        }

        usesMPE(mpeEnabled);

        if (mode != FIXED)
        {
            mode = hasMTSMaster() ? MTS_ESP : TWELVE_TET;
        }
    }

    void usesMPE(bool mpeEnabled)
    {
        mpe = mpeEnabled;
        fresh = 0;
    }

    // a fixed scale in place of MTS-ESP, for one channel or all of them
    void setRetuning(const KeyTable &semitonesFromKey, int channel = -1)
    {
        for (int c = 0; c < 16; ++c)
        {
            if (channel < 0 || channel == c)
            {
                for (int k = 0; k < 128; ++k)
                {
                    tables[c][k] = k + semitonesFromKey[k];
                }
            }
        }

        mode = FIXED;
    }

    const KeyTable &keysFor(int channel)
    {
        if (mode == TWELVE_TET)
        {
            return equalKeys;
        }

        const bool anyChannel = mpe || channel < 0;
        const int t = anyChannel ? 0 : channel & 15;

        if (mode == MTS_ESP && !(fresh & (1u << t)))
        {
            const auto asked = static_cast<char>(anyChannel ? -1 : t);

            for (int k = 0; k < 128; ++k)
            {
                tables[t][k] = k + MTS_RetuningInSemitones(mts_client, static_cast<char>(k), asked);
            }

            fresh |= 1u << t;
        }

        return tables[t];
    }

    double tunedMidiNote(int midiIndex, int channel) { return keysFor(channel)[midiIndex & 127]; }

    // a fractional key, between the tuned keys either side and in 12-TET off the keyboard
    static float tunedKeyAt(const KeyTable &keys, float key)
    {
        const float at = std::clamp(key, 0.f, 127.f);
        const int lo = std::min(static_cast<int>(at), 126);
        const auto lower = static_cast<float>(keys[lo]);
        const auto upper = static_cast<float>(keys[lo + 1]);

        return lower + (at - lo) * (upper - lower) + (key - at);
    }

    float tunedKeyAt(float key, int channel) { return tunedKeyAt(keysFor(channel), key); }

    bool bendsFollowScale() const
    {
        return mode != TWELVE_TET && (scaleAwareWheel.load(std::memory_order_relaxed) ||
                                      scaleAwareMPE.load(std::memory_order_relaxed));
    }

    /*
        These methods can be later be used for implementing other steps in the MTS-ESP guide:
        https://github.com/ODDSound/MTS-ESP/blob/main/Client/libMTSClient.h
     */

    bool hasMTSMaster() { return MTS_HasMaster(mts_client); }

    const char *getMTSScale() { return MTS_GetScaleName(mts_client); }
};

#endif // OBXF_SRC_ENGINE_TUNING_H
