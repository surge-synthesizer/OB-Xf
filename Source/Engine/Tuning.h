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

#ifndef OBXF_SRC_ENGINE_TUNING_H
#define OBXF_SRC_ENGINE_TUNING_H

#include "libMTSClient.h"

class Tuning
{
  private:
    MTSClient *mts_client{nullptr};

    enum Mode
    {
        MTS_ESP,
        TWELVE_TET
    } mode{TWELVE_TET};

  public:
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

        mode = hasMTSMaster() ? MTS_ESP : TWELVE_TET;
    }

    double midiNoteFromMTS(int midiIndex)
    {
        return midiIndex + MTS_RetuningInSemitones(mts_client, midiIndex, -1);
    }

    double tunedMidiNote(int midiIndex)
    {
        switch (mode)
        {
        case TWELVE_TET:
            return midiIndex;
            break;
        case MTS_ESP:
            return midiNoteFromMTS(midiIndex);
            break;
        }
        return midiIndex;
    }

    /*
        These methods can be later be used for implementing other steps in the MTS-ESP guide:
        https://github.com/ODDSound/MTS-ESP/blob/main/Client/libMTSClient.h
     */

    bool hasMTSMaster() { return MTS_HasMaster(mts_client); }

    const char *getMTSScale() { return MTS_GetScaleName(mts_client); }
};

#endif // OBXF_SRC_ENGINE_TUNING_H
