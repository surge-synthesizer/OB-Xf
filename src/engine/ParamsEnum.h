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

#ifndef OBXF_SRC_ENGINE_PARAMSENUM_H
#define OBXF_SRC_ENGINE_PARAMSENUM_H

#include "ObxfVoice.h"

enum ObxfParameters
{
    UNDEFINED,
    MIDILEARN,
    VOLUME,
    VOICE_COUNT,
    TUNE,
    OCTAVE,
    BENDRANGE,
    BENDOSC2,
    LEGATOMODE,
    BENDLFORATE,
    VFLTENV,
    VAMPENV,

    ASPLAYEDALLOCATION,
    PORTAMENTO,
    UNISON,
    UDET,
    OSC2_DET,
    LFOFREQ,
    LFOSINWAVE,
    LFOSQUAREWAVE,
    LFOSHWAVE,
    LFO1AMT,
    LFO2AMT,
    LFOOSC1,
    LFOOSC2,
    LFOFILTER,
    LFOPW1,
    LFOPW2,
    OSC2HS,
    XMOD,
    OSC1P,
    OSC2P,
    ENVPITCHINV,
    OSC1Saw,
    OSC1Pul,
    OSC2Saw,
    OSC2Pul,
    PW,
    BRIGHTNESS,
    ENVPITCH,
    OSC1MIX,
    OSC2MIX,
    NOISEMIX,
    FLT_KF,
    CUTOFF,
    RESONANCE,
    MULTIMODE,
    FILTER_WARM,
    BANDPASS,
    FOURPOLE,
    ENVELOPE_AMT,
    LATK,
    LDEC,
    LSUS,
    LREL,
    FATK,
    FDEC,
    FSUS,
    FREL,
    ENVDER,
    FILTERDER,
    PORTADER,
    PAN1,
    PAN2,
    PAN3,
    PAN4,
    PAN5,
    PAN6,
    PAN7,
    PAN8,
    UNLEARN,
    ECONOMY_MODE,
    LFO_SYNC,
    PW_ENV,
    PW_ENV_BOTH,
    ENV_PITCH_BOTH,
    FENV_INVERT,
    PW_OSC2_OFS,
    LEVEL_DIF,
    SELF_OSC_PUSH,
    PARAM_COUNT,
};

#endif // OBXF_SRC_ENGINE_PARAMSENUM_H
