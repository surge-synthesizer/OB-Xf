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

#include "ObxdImporter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Constants.h"
#include "ParameterList.h"
#include "SynthParam.h"
#include "configuration.h"

namespace
{

// OB-Xd ObxdParameters indices, frozen as they are written to disk.
// Mirrors third_party/OB-Xd/Source/Engine/ParamsEnum.h at tag 2.19.
enum ObxdParam : int
{
    UNDEFINED = 0,
    MIDILEARN = 1,
    VOLUME = 2,
    VOICE_COUNT = 3,
    TUNE = 4,
    OCTAVE = 5,
    BENDRANGE = 6,
    BENDOSC2 = 7,
    LEGATOMODE = 8,
    BENDLFORATE = 9,
    VFLTENV = 10,
    VAMPENV = 11,
    ASPLAYEDALLOCATION = 12,
    PORTAMENTO = 13,
    UNISON = 14,
    UDET = 15,
    OSC2_DET = 16,
    LFOFREQ = 17,
    LFOSINWAVE = 18,
    LFOSQUAREWAVE = 19,
    LFOSHWAVE = 20,
    LFO1AMT = 21,
    LFO2AMT = 22,
    LFOOSC1 = 23,
    LFOOSC2 = 24,
    LFOFILTER = 25,
    LFOPW1 = 26,
    LFOPW2 = 27,
    OSC2HS = 28,
    XMOD = 29,
    OSC1P = 30,
    OSC2P = 31,
    OSCQuantize = 32,
    OSC1Saw = 33,
    OSC1Pul = 34,
    OSC2Saw = 35,
    OSC2Pul = 36,
    PW = 37,
    BRIGHTNESS = 38,
    ENVPITCH = 39,
    OSC1MIX = 40,
    OSC2MIX = 41,
    NOISEMIX = 42,
    FLT_KF = 43,
    CUTOFF = 44,
    RESONANCE = 45,
    MULTIMODE = 46,
    FILTER_WARM = 47,
    BANDPASS = 48,
    FOURPOLE = 49,
    ENVELOPE_AMT = 50,
    LATK = 51,
    LDEC = 52,
    LSUS = 53,
    LREL = 54,
    FATK = 55,
    FDEC = 56,
    FSUS = 57,
    FREL = 58,
    ENVDER = 59,
    FILTERDER = 60,
    PORTADER = 61,
    PAN1 = 62,
    PAN2 = 63,
    PAN3 = 64,
    PAN4 = 65,
    PAN5 = 66,
    PAN6 = 67,
    PAN7 = 68,
    PAN8 = 69,
    UNLEARN = 70,
    ECONOMY_MODE = 71,
    LFO_SYNC = 72,
    PW_ENV = 73,
    PW_ENV_BOTH = 74,
    ENV_PITCH_BOTH = 75,
    FENV_INVERT = 76,
    PW_OSC2_OFS = 77,
    LEVEL_DIF = 78,
    SELF_OSC_PUSH = 79,
    PARAM_COUNT = 80,
};

// Re-implementations of OB-Xd's logsc/linsc plus their inverses, used for
// rescaling normalized values across the two engines.
// Matches the OB-Xd definitions in Source/Engine/AudioUtils.h.
inline float xdLogsc(float p, float lo, float hi, float rolloff = 19.f)
{
    return ((std::exp(p * std::log(rolloff + 1.f)) - 1.f) / rolloff) * (hi - lo) + lo;
}

inline float xdInvLinsc(float y, float lo, float hi)
{
    if (hi == lo)
        return 0.f;
    return juce::jlimit(0.f, 1.f, (y - lo) / (hi - lo));
}

inline float xdInvLogsc(float y, float lo, float hi, float rolloff = 19.f)
{
    if (hi == lo)
        return 0.f;
    const float t = rolloff * (y - lo) / (hi - lo) + 1.f;
    if (t <= 0.f)
        return 0.f;
    return juce::jlimit(0.f, 1.f, std::log(t) / std::log(rolloff + 1.f));
}

// Read attribute Val_<k>, falling back to bare <k> for the legacy schema.
float readObxdAttribute(const juce::XmlElement &e, int k, float fallback = 0.f)
{
    const auto vKey = "Val_" + juce::String(k);
    if (e.hasAttribute(vKey))
        return static_cast<float>(e.getDoubleAttribute(vKey, fallback));
    const auto bareKey = juce::String(k);
    if (e.hasAttribute(bareKey))
        return static_cast<float>(e.getDoubleAttribute(bareKey, fallback));
    return fallback;
}

// Map OB-Xd LFO1 sync rate (9 buckets) to OB-Xf's 21-bucket synced table.
// Lookup via (int)(v*8) -> XF normalized rate. See LFOSync.md.
float mapLfoSyncedRate(float vXd)
{
    static constexpr int xdToXf[9] = {1, 4, 5, 7, 10, 11, 13, 15, 16};
    const int kXd = juce::jlimit(0, 8, static_cast<int>(vXd * 8.f));
    return static_cast<float>(xdToXf[kXd]) / static_cast<float>(syncedRatesCount - 1);
}

void seedDefaults(Program &program)
{
    program.values.clear();
    for (const auto &param : ParameterList)
    {
        program.values[param.ID] = param.meta.naturalToNormalized01(param.meta.defaultVal);
    }
}

void translateLfoFreq(float vXd, bool synced, Program &program)
{
    using namespace SynthParam;

    if (synced)
    {
        program.values[ID::LFO1Rate] = mapLfoSyncedRate(vXd);
    }
    else
    {
        const float hzXd = xdLogsc(vXd, 0.f, 50.f, 120.f);
        program.values[ID::LFO1Rate] = xdInvLogsc(hzXd, 0.f, 250.f, 3775.f);
    }
}

void translateAttackTime(float vXd, const juce::String &id, float lo, float hi, Program &program)
{
    const float msXd = xdLogsc(vXd, lo, hi, 900.f);
    const float msXf = msXd / 3.f;

    program.values[id] = xdInvLogsc(msXf, lo, hi, 900.f);
}

float lfoBoolToBlend(float v) { return v >= 0.5f ? 0.f : 0.5f; }

float lfoBoolToTriState(float v)
{
    // OB-Xf tri-state {Off, On, Inverted} stored as int 0..2 normalized.
    // 0 -> 0/2 = 0; 1 -> 1/2 = 0.5; 2 -> 1.0.
    return v >= 0.5f ? 0.5f : 0.f;
}

} // namespace

void ObxdImporter::translateProgramFromXml(const juce::XmlElement &e, Program &program,
                                           std::vector<std::string> &warnings)
{
    using namespace SynthParam;

    seedDefaults(program);

    // Override defaults that the plan calls out as not matching OB-Xd hard-wired behavior.
    program.values[ID::Osc2Keytrack] = 1.f;
    program.values[ID::EnvToPitchInvert] = 0.f;
    program.values[ID::EnvToPWInvert] = 0.f;
    program.values[ID::RingModVol] = 0.f;
    program.values[ID::NoiseColor] = 0.f;
    program.values[ID::Filter4PoleXpander] = 0.f;
    program.values[ID::FilterXpanderMode] = 0.f;
    program.values[ID::AmpEnvAttackCurve] = 0.f;
    program.values[ID::FilterEnvAttackCurve] = 0.f;
    program.values[ID::LFO1PW] = 0.f;
    program.values[ID::LFO1ToVolume] = 0.f;
    program.values[ID::VibratoWave] = 0.f;
    program.values[ID::NotePriority] = 0.f;
    program.values[ID::EnvLegatoMode] = 0.f;

    const bool newFormat = e.hasAttribute("voiceCount");

    // Pre-fetch values we need to consult for cross-parameter decisions.
    const bool lfoSync = readObxdAttribute(e, LFO_SYNC) > 0.5f;
    const bool oscStep = readObxdAttribute(e, OSCQuantize) > 0.5f;

    auto val = [&](int k) { return readObxdAttribute(e, k); };

    // ---- Master ----

    const auto transpose = static_cast<int>(std::round(val(OCTAVE) * 4.f) + 1);

    program.values[ID::Volume] = val(VOLUME);
    program.values[ID::Tune] = val(TUNE);
    program.values[ID::Transpose] = std::clamp(transpose, 0, 4) * 0.25f;

    warnings.emplace_back("OCTAVE: OB-Xd had the middle C frequency reference an octave too high; "
                          "OB-Xf compensates for this by adjusting the Transpose parameter.");

    // ---- Global ----

    {
        float vc = val(VOICE_COUNT);

        if (!newFormat)
        {
            vc *= 0.25f;
        }

        // OB-Xd: voices = 1 + round(vc * 31). OB-Xf engine: 1 + (int)(v * 32)
        // produces 33 at vc = 1, so we need to compensate
        // 0.000001f epsilon is there to avoid cases where OB-Xd patch has i.e. 0.25 stored,
        // which would end up as 9 voices in OB-Xf.
        int xdVoices =
            std::clamp(static_cast<int>(std::round(vc * 31.0f) - 0.000001f) + 1, 1, MAX_VOICES);
        program.values[ID::Polyphony] =
            (static_cast<float>(xdVoices - 1) + 0.5f) / static_cast<float>(MAX_VOICES);
    }

    program.values[ID::HQMode] = val(FILTER_WARM);

    {
        const float br = val(BENDRANGE);
        const int range = (br > 0.5f) ? 12 : 2;
        const float n = static_cast<float>(range) / static_cast<float>(MAX_BEND_RANGE);
        program.values[ID::BendUpRange] = n;
        program.values[ID::BendDownRange] = n;
    }

    program.values[ID::BendOsc2Only] = val(BENDOSC2);

    {
        // OB-Xd vibrato: setFrequency(logsc(v, 3, 10)) Hz
        // OB-Xf vibrato: setRate(linsc(v, 2, 12)) Hz
        const float hzXd = xdLogsc(val(BENDLFORATE), 3.f, 10.f);
        program.values[ID::VibratoRate] = xdInvLinsc(hzXd, 2.f, 12.f);
    }

    program.values[ID::Portamento] = val(PORTAMENTO);
    program.values[ID::Unison] = val(UNISON);

    {
        // OB-Xd UDET: logsc(v, 0.001, 0.90); OB-Xf UnisonDetune: logsc(v, 0.001, 1.0)
        const float dXd = xdLogsc(val(UDET), 0.001f, 0.90f);
        program.values[ID::UnisonDetune] = xdInvLogsc(dXd, 0.001f, 1.0f);
    }

    program.values[ID::EnvLegatoMode] = val(LEGATOMODE);
    program.values[ID::NotePriority] = val(ASPLAYEDALLOCATION) > 0.5f ? 0.f : 0.5;

    // ---- Oscillators ----

    auto osc1PitchRaw = val(OSC1P) * 48.f;
    auto osc2PitchRaw = val(OSC2P) * 48.f;

    if (osc1PitchRaw <= 36.f && osc2PitchRaw <= 36.f && transpose > 4.f)
    {
        osc1PitchRaw = osc1PitchRaw + 12.f;
        osc2PitchRaw = osc2PitchRaw + 12.f;

        warnings.emplace_back(
            "OCTAVE: The imported sound has already had the Transpose parameter set to maximum "
            "value."
            "OB-Xf compensates for this by adjusting the Osc 1 and Osc 2 Pitch parameters an "
            "octave upwards.");
    }

    const auto osc1Pitch = oscStep ? ((int)(osc1PitchRaw)) / 48.f : osc1PitchRaw / 48.f;
    const auto osc2Pitch = oscStep ? ((int)(osc2PitchRaw)) / 48.f : osc2PitchRaw / 48.f;

    program.values[ID::Osc1Pitch] = osc1Pitch;
    program.values[ID::Osc2Pitch] = osc2Pitch;
    program.values[ID::Osc2Detune] = val(OSC2_DET);
    program.values[ID::Osc1SawWave] = val(OSC1Saw);
    program.values[ID::Osc1PulseWave] = val(OSC1Pul);
    program.values[ID::Osc2SawWave] = val(OSC2Saw);
    program.values[ID::Osc2PulseWave] = val(OSC2Pul);
    program.values[ID::OscSync] = val(OSC2HS);

    // OB-Xd XMOD: v * 24 semis. OB-Xf OscCrossmod: v * 48.
    program.values[ID::OscCrossmod] = val(XMOD) * 0.5f;

    program.values[ID::OscPW] = val(PW);
    // OB-Xd PW_OSC2_OFS: linsc(v, 0, 0.75). OB-Xf Osc2PWOffset: linsc(v, 0, 0.95).
    program.values[ID::Osc2PWOffset] = val(PW_OSC2_OFS) * (0.75f / 0.95f);
    // OB-Xd PW_ENV: linsc(v, 0, 0.85); OB-Xf EnvToPWAmount: linsc(v, 0, 1.055555555).
    program.values[ID::EnvToPWAmount] = val(PW_ENV) * (0.85f / 1.0555555555f);
    program.values[ID::EnvToPWBothOscs] = val(PW_ENV_BOTH);

    // OB-Xd ENVPITCH: v * 36 semitones; OB-Xf EnvToPitchAmount: v * 40 (sustain compensation).
    program.values[ID::EnvToPitchAmount] = val(ENVPITCH) * (36.f / 40.f);
    program.values[ID::EnvToPitchBothOscs] = val(ENV_PITCH_BOTH);
    program.values[ID::OscBrightness] = val(BRIGHTNESS);

    // ---- Mixer ----

    program.values[ID::Osc1Vol] = val(OSC1MIX);
    program.values[ID::Osc2Vol] = val(OSC2MIX);

    // OB-Xd noise mix runs the value through logsc(v, 0, 1, 35) before the engine.
    program.values[ID::NoiseVol] = xdLogsc(val(NOISEMIX), 0.f, 1.f, 35.f);

    // ---- Filter ----

    program.values[ID::FilterCutoff] = val(CUTOFF);
    program.values[ID::FilterResonance] = val(RESONANCE);
    program.values[ID::FilterMode] = val(MULTIMODE);
    program.values[ID::Filter2PoleBPBlend] = val(BANDPASS);
    program.values[ID::Filter4PoleMode] = val(FOURPOLE);
    program.values[ID::Filter2PolePush] = val(SELF_OSC_PUSH);
    program.values[ID::FilterKeyTrack] = val(FLT_KF);
    program.values[ID::FilterEnvAmount] = val(ENVELOPE_AMT);
    program.values[ID::FilterEnvInvert] = val(FENV_INVERT);

    // ---- Envelopes ----

    translateAttackTime(val(LATK), ID::AmpEnvAttack, 4.f, 60000.f, program);
    program.values[ID::AmpEnvDecay] = val(LDEC);
    program.values[ID::AmpEnvSustain] = val(LSUS);
    program.values[ID::AmpEnvRelease] = val(LREL);

    program.values[ID::VelToAmpEnv] = val(VAMPENV);

    translateAttackTime(val(FATK), ID::FilterEnvAttack, 1.f, 60000.f, program);
    program.values[ID::FilterEnvDecay] = val(FDEC);
    program.values[ID::FilterEnvSustain] = val(FSUS);
    program.values[ID::FilterEnvRelease] = val(FREL);

    program.values[ID::VelToFilterEnv] = val(VFLTENV);

    // ---- LFO ----

    program.values[ID::LFO1TempoSync] = lfoSync ? 1.f : 0.f;
    translateLfoFreq(val(LFOFREQ), lfoSync, program);

    if (lfoSync)
    {
        warnings.emplace_back(
            "LFO_SYNC: OB-Xd's tempo sync was unreliable; OB-Xf will sync correctly so "
            "the patch may sound slightly different. Selected LFO rate is preserved!");
    }

    program.values[ID::LFO1Wave1] = lfoBoolToBlend(val(LFOSINWAVE));
    program.values[ID::LFO1Wave2] = lfoBoolToBlend(val(LFOSQUAREWAVE));
    program.values[ID::LFO1Wave3] = lfoBoolToBlend(val(LFOSHWAVE));

    program.values[ID::LFO1ModAmount1] = val(LFO1AMT);
    program.values[ID::LFO1ModAmount2] = val(LFO2AMT);

    program.values[ID::LFO1ToOsc1Pitch] = lfoBoolToTriState(val(LFOOSC1));
    program.values[ID::LFO1ToOsc2Pitch] = lfoBoolToTriState(val(LFOOSC2));
    program.values[ID::LFO1ToFilterCutoff] = lfoBoolToTriState(val(LFOFILTER));
    program.values[ID::LFO1ToOsc1PW] = lfoBoolToTriState(val(LFOPW1));
    program.values[ID::LFO1ToOsc2PW] = lfoBoolToTriState(val(LFOPW2));

    // ---- Voice variation / pan ----

    program.values[ID::PortamentoSlop] = val(PORTADER);
    program.values[ID::FilterSlop] = val(FILTERDER);
    program.values[ID::EnvelopeSlop] = val(ENVDER);
    program.values[ID::LevelSlop] = val(LEVEL_DIF);

    program.values[ID::PanVoice1] = val(PAN1);
    program.values[ID::PanVoice2] = val(PAN2);
    program.values[ID::PanVoice3] = val(PAN3);
    program.values[ID::PanVoice4] = val(PAN4);
    program.values[ID::PanVoice5] = val(PAN5);
    program.values[ID::PanVoice6] = val(PAN6);
    program.values[ID::PanVoice7] = val(PAN7);
    program.values[ID::PanVoice8] = val(PAN8);

    // ---- Metadata ----

    program.setName(e.getStringAttribute("programName", "Init"));
    program.setAuthor("");
    program.setLicense("");
    program.setCategory("None");
    program.setProject("");
}

namespace
{

// Strict header check: 'CcnK' and fxID == 'Obxd'.
const fxChunkSet *asObxdChunkSet(const void *data, size_t size)
{
    if (size < sizeof(fxChunkSet) - 8)
    {
        return nullptr;
    }

    const auto *set = static_cast<const fxChunkSet *>(data);

    if (!compareMagic(set->chunkMagic, "CcnK"))
    {
        return nullptr;
    }

    if (!compareMagic(set->fxID, "Obxd"))
    {
        return nullptr;
    }

    if (fxbSwap(set->version) > fxbVersionNum)
    {
        return nullptr;
    }

    return set;
}

// Single program (FPCh) chunk extraction.
bool readFPChChunk(const void *data, size_t size, const void *&outChunkData, int &outChunkSize)
{
    if (size < sizeof(fxProgramSet))
    {
        return false;
    }

    const auto set = static_cast<const fxProgramSet *>(data);
    const int32_t rawChunkSize = fxbSwap(set->chunkSize);

    if (rawChunkSize < 0)
    {
        return false;
    }

    const size_t chunkSize = static_cast<size_t>(rawChunkSize);

    if (chunkSize + sizeof(fxProgramSet) - 8 > size)
    {
        return false;
    }

    outChunkData = set->chunk;
    outChunkSize = static_cast<int>(chunkSize);

    return true;
}

// Bank (FBCh) chunk extraction.
bool readFBChChunk(const void *data, size_t size, const void *&outChunkData, int &outChunkSize)
{
    if (size < sizeof(fxChunkSet))
    {
        return false;
    }

    const auto *set = static_cast<const fxChunkSet *>(data);
    const int32_t rawChunkSize = fxbSwap(set->chunkSize);

    if (rawChunkSize < 0)
    {
        return false;
    }

    const size_t chunkSize = static_cast<size_t>(rawChunkSize);

    if (chunkSize + sizeof(fxChunkSet) - 8 > size)
    {
        return false;
    }

    outChunkData = set->chunk;
    outChunkSize = static_cast<int>(chunkSize);

    return true;
}

} // namespace

bool ObxdImporter::isOBXdData(const void *data, size_t size)
{
    return asObxdChunkSet(data, size) != nullptr;
}

bool ObxdImporter::visitPrograms(
    const void *data, size_t size,
    const std::function<void(int, bool, const juce::XmlElement &)> &visit)
{
    const auto *set = asObxdChunkSet(data, size);

    if (!set)
    {
        return false;
    }

    const void *chunk = nullptr;
    int chunkSize = 0;

    const bool isFPCh = compareMagic(set->fxMagic, "FPCh");
    const bool isFBCh = compareMagic(set->fxMagic, "FBCh");

    if (isFPCh)
    {
        if (!readFPChChunk(data, size, chunk, chunkSize))
        {
            return false;
        }
    }
    else if (isFBCh)
    {
        if (!readFBChChunk(data, size, chunk, chunkSize))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    std::unique_ptr<juce::XmlElement> xml;

    if (chunkSize >= 8 && std::memcmp(static_cast<const char *>(chunk), "VC2!", 4) == 0)
    {
        // Legacy OB-Xd wrapper: 'VC2!' + LE uint32 + raw UTF-8 XML
        uint32_t xmlLen;
        std::memcpy(&xmlLen, static_cast<const char *>(chunk) + 4, 4);
        const auto xmlStr =
            juce::String::fromUTF8(static_cast<const char *>(chunk) + 8,
                                   static_cast<int>(std::min<size_t>(xmlLen, chunkSize - 8)));
        xml = juce::XmlDocument::parse(xmlStr);
    }
    else
    {
        xml = juce::AudioProcessor::getXmlFromBinary(chunk, chunkSize);
    }

    if (!xml)
    {
        return false;
    }

    const auto knownRoot = xml->hasTagName("discoDSP") || xml->hasTagName("Datsounds");

    if (!knownRoot)
    {
        return false;
    }

    if (isFPCh)
    {
        visit(0, true, *xml);
        return true;
    }

    const int currentProgram = xml->getIntAttribute("currentProgram", 0);
    const auto *programsNode = xml->getChildByName("programs");

    if (!programsNode)
    {
        return false;
    }

    int idx = 0;
    bool sawAny = false;

    for (auto *programEl : programsNode->getChildIterator())
    {
        if (!programEl->hasTagName("program"))
        {
            continue;
        }

        visit(idx, idx == currentProgram, *programEl);
        sawAny = true;

        idx++;

        if (idx >= 128)
        {
            break;
        }
    }

    return sawAny;
}

bool ObxdImporter::importSingleOnto(const void *data, size_t size, Program &outProgram,
                                    std::vector<std::string> *outWarnings)
{
    bool wrote = false;
    std::vector<std::string> warnings;

    visitPrograms(data, size, [&](int /*idx*/, bool isCurrent, const juce::XmlElement &programEl) {
        if (!isCurrent || wrote)
        {
            return;
        }

        translateProgramFromXml(programEl, outProgram, warnings);

        wrote = true;
    });

    if (outWarnings)
    {
        for (auto &w : warnings)
        {
            outWarnings->emplace_back(std::move(w));
        }
    }

    return wrote;
}

ObxdImporter::BankImportResult ObxdImporter::importBankFromFxb(
    const void *data, size_t size, const juce::String &bankName, const juce::File &destFolder,
    const std::function<bool(const Program &, juce::MemoryBlock &)> &serializePatch)
{
    BankImportResult result;

    const juce::File bankFolder = destFolder.getChildFile(bankName);

    bool visitOk = visitPrograms(
        data, size, [&](int /*idx*/, bool /*isCurrent*/, const juce::XmlElement &programEl) {
            // Check for init patch by name before translating
            const juce::String programName = programEl.getStringAttribute("programName", "Default");

            if (programName.trim().equalsIgnoreCase("Default"))
            {
                ++result.skipped;
                return;
            }

            Program prog;
            std::vector<std::string> warnings;
            translateProgramFromXml(programEl, prog, warnings);

            // Override the project metadata with the bank name
            prog.setProject(bankName);

            juce::MemoryBlock mb;
            if (!serializePatch(prog, mb))
            {
                // serialization failure — treat as a skip but don't count as skipping an init patch
                return;
            }

            // Sanitize patch name for use as a filename.
            juce::String safeName = prog.getName().replaceCharacters("\\/:*?\"<>|", " ").trim();

            if (safeName.isEmpty())
            {
                safeName = juce::String("Patch ") + juce::String(result.imported + 1);
            }

            const juce::File patchFile = bankFolder.getChildFile(safeName + ".fxp");

            if (!bankFolder.createDirectory())
            {
                return;
            }

            if (patchFile.replaceWithData(mb.getData(), mb.getSize()))
            {
                ++result.imported;
            }
        });

    if (!visitOk)
    {
        result.parseError = true;
    }

    return result;
}
