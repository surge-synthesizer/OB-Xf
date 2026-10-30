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

#include "libMTSClient.h"

class Tuning
{
  private:
    MTSClient *mts_client{nullptr};

    enum Mode
    {
        RETUNED,
        TWELVE_TET
    } mode{TWELVE_TET};

    // each key's pitch in semitones, while retuned
    std::array<double, 128> tunedKeys{};

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

    void updateMTSESPStatus()
    {
        if (mts_client == nullptr)
        {
            mts_client = MTS_RegisterClient();
        }

        if (!hasMTSMaster())
        {
            mode = TWELVE_TET;
            return;
        }

        for (int k = 0; k < 128; ++k)
        {
            tunedKeys[k] = k + MTS_RetuningInSemitones(mts_client, static_cast<char>(k), -1);
        }

        mode = RETUNED;
    }

    // a fixed scale in place of MTS-ESP, which the next updateMTSESPStatus replaces
    void setRetuning(const std::array<double, 128> &semitonesFromKey)
    {
        for (int k = 0; k < 128; ++k)
        {
            tunedKeys[k] = k + semitonesFromKey[k];
        }

        mode = RETUNED;
    }

    double tunedMidiNote(int midiIndex) const
    {
        return mode == RETUNED ? tunedKeys[midiIndex & 127] : midiIndex;
    }

    // a fractional key, between the tuned keys either side and in 12-TET off the keyboard
    float tunedKeyAt(float key) const
    {
        if (mode != RETUNED)
        {
            return key;
        }

        const float at = std::clamp(key, 0.f, 127.f);
        const int lo = std::min(static_cast<int>(at), 126);
        const auto lower = static_cast<float>(tunedKeys[lo]);
        const auto upper = static_cast<float>(tunedKeys[lo + 1]);

        return lower + (at - lo) * (upper - lower) + (key - at);
    }

    bool bendsFollowScale() const
    {
        return mode == RETUNED && (scaleAwareWheel.load(std::memory_order_relaxed) ||
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
