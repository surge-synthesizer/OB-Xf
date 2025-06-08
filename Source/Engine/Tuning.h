//
//  Tuning.h
//  OB-Xd
//
//  Created by Natalia Escalera on 12/28/21.
//  Copyright © 2021 discoDSP. All rights reserved.
//
#pragma once

#include "MTS/libMTSClient.h"

#pragma once

class Tuning
{
private:
    MTSClient* mts_client{nullptr};
    enum Mode {
        MTS_ESP,
        TWELVE_TET
    } mode{TWELVE_TET};
    
public:
    Tuning()
    {
 
    }
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
        switch(mode)
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
    
    bool hasMTSMaster()
    {
        return MTS_HasMaster(mts_client);
    }
    
    const char *getMTSScale()
    {
        return MTS_GetScaleName(mts_client);
    }
    
};
