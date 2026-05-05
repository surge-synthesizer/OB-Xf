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

#include "ObxdImporter.h"

#include "Constants.h"
#include "ParameterList.h"
#include "Program.h"
#include "SynthParam.h"
#include "configuration.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch2.hpp>

#include <cmath>
#include <cstring>
#include <vector>

namespace
{

// Mirrored OB-Xd ObxdParameters indices, kept terse for test brevity.
// Must match third_party/OB-Xd/Source/Engine/ParamsEnum.h tag 2.17.
enum
{
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
    PAN8 = 69,
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

// Mirror of OB-Xd's logsc/linsc, used by the test scaffolding to compute
// expected values without going through the importer.
inline float xLogsc(float p, float lo, float hi, float rolloff = 19.f)
{
    return ((std::exp(p * std::log(rolloff + 1.f)) - 1.f) / rolloff) * (hi - lo) + lo;
}

inline float xInvLogsc(float y, float lo, float hi, float rolloff = 19.f)
{
    const float t = rolloff * (y - lo) / (hi - lo) + 1.f;
    return std::log(t) / std::log(rolloff + 1.f);
}

inline float xInvLinsc(float y, float lo, float hi) { return (y - lo) / (hi - lo); }

inline float transformVoiceCount(float vc, int maxVoices)
{
    int xdVoices =
        std::clamp(static_cast<int>(std::round(vc * 31.0f - 0.000001f)) + 1, 1, maxVoices);

    return (static_cast<float>(xdVoices - 1) + 0.5f) / static_cast<float>(maxVoices);
}

inline int getEngineVoiceCount(float v) { return 1 + static_cast<int>(v * 32.0f); }

// Builds a `<discoDSP>` XML element for a single program. Caller writes
// individual Val_<k> attributes via setXdParam before serializing.
juce::XmlElement makeDiscoDSPElement(const juce::String &name = "Test")
{
    juce::XmlElement xml{"discoDSP"};
    xml.setAttribute("voiceCount", MAX_VOICES);
    xml.setAttribute("programName", name);
    for (int k = 0; k < PARAM_COUNT; ++k)
        xml.setAttribute("Val_" + juce::String(k), 0.0);
    return xml;
}

void setXdParam(juce::XmlElement &e, int k, float v)
{
    e.setAttribute("Val_" + juce::String(k), v);
}

// Wrap an XML element as an FPCh/FBCh chunk with the OB-Xd fxID.
juce::MemoryBlock wrapAsFPCh(const juce::XmlElement &xml, const char *programName = "Test")
{
    juce::MemoryBlock chunk;
    juce::AudioProcessor::copyXmlToBinary(xml, chunk);

    juce::MemoryBlock out;
    const size_t total = sizeof(fxProgramSet) + chunk.getSize() - 8;
    out.setSize(total, true);

    auto *set = static_cast<fxProgramSet *>(out.getData());
    set->chunkMagic = fxbName("CcnK");
    set->byteSize = 0;
    set->fxMagic = fxbName("FPCh");
    set->version = fxbSwap(fxbVersionNum);
    set->fxID = fxbName("Obxd");
    set->fxVersion = fxbSwap(fxbVersionNum);
    set->numPrograms = fxbSwap(1);
    std::memset(set->name, 0, sizeof(set->name));
    std::strncpy(set->name, programName, sizeof(set->name) - 1);
    set->chunkSize = fxbSwap(static_cast<int32_t>(chunk.getSize()));
    chunk.copyTo(set->chunk, 0, chunk.getSize());
    return out;
}

juce::MemoryBlock wrapAsFBCh(const juce::XmlElement &xml)
{
    juce::MemoryBlock chunk;
    juce::AudioProcessor::copyXmlToBinary(xml, chunk);

    juce::MemoryBlock out;
    const size_t total = sizeof(fxChunkSet) + chunk.getSize() - 8;
    out.setSize(total, true);

    auto *set = static_cast<fxChunkSet *>(out.getData());
    set->chunkMagic = fxbName("CcnK");
    set->byteSize = 0;
    set->fxMagic = fxbName("FBCh");
    set->version = fxbSwap(fxbVersionNum);
    set->fxID = fxbName("Obxd");
    set->fxVersion = fxbSwap(fxbVersionNum);
    set->numPrograms = fxbSwap(OBXD_BANK_SIZE);
    std::memset(set->future, 0, sizeof(set->future));
    set->chunkSize = fxbSwap(static_cast<int32_t>(chunk.getSize()));
    chunk.copyTo(set->chunk, 0, chunk.getSize());
    return out;
}

float get(const Program &p, const juce::String &id) { return p.getValueById(id); }

} // namespace

using namespace SynthParam;

TEST_CASE("ObxdImporter::isOBXdData detects the wrapper", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    auto fxp = wrapAsFPCh(xml);
    REQUIRE(ObxdImporter::isOBXdData(fxp.getData(), fxp.getSize()));

    SECTION("rejects an OB-Xf-format chunk")
    {
        // Same wrapper, different fxID
        auto *set = static_cast<fxProgramSet *>(fxp.getData());
        set->fxID = fxbName("OBXf");
        REQUIRE_FALSE(ObxdImporter::isOBXdData(fxp.getData(), fxp.getSize()));
    }

    SECTION("rejects garbage") { REQUIRE_FALSE(ObxdImporter::isOBXdData("not a fxp", 9)); }
}

TEST_CASE("ObxdImporter parses an FPCh single-program file", "[obxd]")
{
    auto xml = makeDiscoDSPElement("MyPatch");
    setXdParam(xml, VOLUME, 0.42f);
    setXdParam(xml, CUTOFF, 0.7f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    std::vector<std::string> warnings;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p, &warnings));

    REQUIRE(p.getName() == juce::String("MyPatch"));
    REQUIRE(get(p, ID::Volume) == Approx(0.42f));
    REQUIRE(get(p, ID::FilterCutoff) == Approx(0.7f));
}

TEST_CASE("ObxdImporter parses an FBCh bank and selects currentProgram", "[obxd]")
{
    juce::XmlElement root{"discoDSP"};
    root.setAttribute("currentProgram", 5);
    auto *programsNode = new juce::XmlElement("programs");
    for (int i = 0; i < OBXD_BANK_SIZE; ++i)
    {
        auto *progEl = new juce::XmlElement(makeDiscoDSPElement("Bank_" + juce::String(i)));
        progEl->setTagName("program");
        // Stamp Volume to a unique value per-program so we can verify selection.
        progEl->setAttribute("Val_" + juce::String(VOLUME), i / 128.0);
        programsNode->addChildElement(progEl);
    }
    root.addChildElement(programsNode);

    auto fxb = wrapAsFBCh(root);
    REQUIRE(ObxdImporter::isOBXdData(fxb.getData(), fxb.getSize()));

    int sawCount = 0;
    int currentSeen = -1;
    ObxdImporter::visitPrograms(fxb.getData(), fxb.getSize(),
                                [&](int idx, bool isCurrent, const juce::XmlElement &) {
                                    sawCount++;
                                    if (isCurrent)
                                        currentSeen = idx;
                                });
    REQUIRE(sawCount == OBXD_BANK_SIZE);
    REQUIRE(currentSeen == 5);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxb.getData(), fxb.getSize(), p));
    REQUIRE(p.getName() == juce::String("Bank_5"));
    REQUIRE(get(p, ID::Volume) == Approx(5.0 / 128.0));
}

TEST_CASE("ObxdImporter translates rescaled parameters", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    setXdParam(xml, XMOD, 0.5f);
    setXdParam(xml, PW_OSC2_OFS, 1.0f);
    setXdParam(xml, PW_ENV, 0.6f);
    setXdParam(xml, ENVPITCH, 0.8f);
    setXdParam(xml, NOISEMIX, 0.3f);
    setXdParam(xml, UDET, 0.4f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));

    // OscCrossmod: v -> v/2
    REQUIRE(get(p, ID::OscCrossmod) == Approx(0.25f));
    // Osc2PWOffset: v * 0.75/0.95
    REQUIRE(get(p, ID::Osc2PWOffset) == Approx(0.75f / 0.95f));
    // EnvToPWAmount: v * 0.85 / 1.0555…
    REQUIRE(get(p, ID::EnvToPWAmount) == Approx(0.6f * 0.85f / 1.0555555555f));
    // EnvToPitchAmount: v * 36/40
    REQUIRE(get(p, ID::EnvToPitchAmount) == Approx(0.8f * 0.9f));
    // NoiseVol: logsc(v, 0, 1, 35)
    REQUIRE(get(p, ID::NoiseVol) == Approx(xLogsc(0.3f, 0.f, 1.f, 35.f)));
    // UnisonDetune: invLogsc(logsc(v, 0.001, 0.90), 0.001, 1.0)
    {
        const float dXd = xLogsc(0.4f, 0.001f, 0.90f);
        REQUIRE(get(p, ID::UnisonDetune) == Approx(xInvLogsc(dXd, 0.001f, 1.0f)));
    }
}

TEST_CASE("ObxdImporter remaps BENDRANGE to dual ranges", "[obxd]")
{
    SECTION("low value -> 2 semitones")
    {
        auto xml = makeDiscoDSPElement();
        setXdParam(xml, BENDRANGE, 0.4f);
        auto fxp = wrapAsFPCh(xml);

        Program p;
        REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));
        REQUIRE(get(p, ID::BendUpRange) == Approx(2.f / MAX_BEND_RANGE));
        REQUIRE(get(p, ID::BendDownRange) == Approx(2.f / MAX_BEND_RANGE));
    }
    SECTION("high value -> 12 semitones")
    {
        auto xml = makeDiscoDSPElement();
        setXdParam(xml, BENDRANGE, 0.6f);
        auto fxp = wrapAsFPCh(xml);

        Program p;
        REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));
        REQUIRE(get(p, ID::BendUpRange) == Approx(12.f / MAX_BEND_RANGE));
        REQUIRE(get(p, ID::BendDownRange) == Approx(12.f / MAX_BEND_RANGE));
    }
}

TEST_CASE("ObxdImporter remaps LFO booleans", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    setXdParam(xml, LFOSINWAVE, 1.f);
    setXdParam(xml, LFOSQUAREWAVE, 0.f);
    setXdParam(xml, LFOSHWAVE, 1.f);
    setXdParam(xml, LFOOSC1, 1.f);
    setXdParam(xml, LFOFILTER, 0.f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));

    REQUIRE(get(p, ID::LFO1Wave1) == Approx(0.f));  // sine -> full left
    REQUIRE(get(p, ID::LFO1Wave2) == Approx(0.5f)); // off  -> DC
    REQUIRE(get(p, ID::LFO1Wave3) == Approx(0.f));  // S&H -> full left

    REQUIRE(get(p, ID::LFO1ToOsc1Pitch) == Approx(0.5f));   // tri-state On
    REQUIRE(get(p, ID::LFO1ToFilterCutoff) == Approx(0.f)); // tri-state Off
}

TEST_CASE("ObxdImporter LFO rate maps via Hz round-trip when unsynced", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    setXdParam(xml, LFO_SYNC, 0.f);
    setXdParam(xml, LFOFREQ, 0.5f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));

    const float hzXd = xLogsc(0.5f, 0.f, 50.f, 120.f);
    REQUIRE(get(p, ID::LFO1Rate) == Approx(xInvLogsc(hzXd, 0.f, 250.f, 3775.f)));
    REQUIRE(get(p, ID::LFO1TempoSync) == Approx(0.f));
}

TEST_CASE("ObxdImporter LFO rate uses sync table when synced", "[obxd]")
{
    // OB-Xd LFOFREQ buckets: (int)(v*8), 9 buckets indexed 0..8.
    // v=0.0  -> bucket 0 (1/8)  -> XF normalized 1/20
    // v=0.5  -> bucket 4 (1)    -> XF normalized 10/20
    // v=1.0  -> bucket 8 (4)    -> XF normalized 16/20

    struct Case
    {
        float vXd;
        float expectedXf;
    };

    for (auto c : {Case{0.0f, 1.f / 20.f}, Case{0.5f, 10.f / 20.f}, Case{1.0f, 16.f / 20.f}})
    {
        auto xml = makeDiscoDSPElement();
        setXdParam(xml, LFO_SYNC, 1.f);
        setXdParam(xml, LFOFREQ, c.vXd);
        auto fxp = wrapAsFPCh(xml);

        Program p;
        std::vector<std::string> warnings;
        REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p, &warnings));

        REQUIRE(get(p, ID::LFO1Rate) == Approx(c.expectedXf));
        REQUIRE(get(p, ID::LFO1TempoSync) == Approx(1.f));
        // Sync caveat warning should fire.
        bool hasSyncWarning = false;
        for (const auto &w : warnings)
            if (w.find("LFO_SYNC") != std::string::npos)
                hasSyncWarning = true;
        REQUIRE(hasSyncWarning);
    }
}

TEST_CASE("ObxdImporter VibratoRate maps via Hz cross-curve", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    setXdParam(xml, BENDLFORATE, 0.5f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));
    const float hzXd = xLogsc(0.5f, 3.f, 10.f);
    REQUIRE(get(p, ID::VibratoRate) == Approx(xInvLinsc(hzXd, 2.f, 12.f)));
}

TEST_CASE("ObxdImporter applies attack-time compensation by default", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    setXdParam(xml, LATK, 0.6f);
    setXdParam(xml, FATK, 0.4f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));

    const float msL = xLogsc(0.6f, 4.f, 60000.f, 900.f);
    const float msF = xLogsc(0.4f, 1.f, 60000.f, 900.f);
    REQUIRE(get(p, ID::AmpEnvAttack) == Approx(xInvLogsc(msL / 3.f, 4.f, 60000.f, 900.f)));
    REQUIRE(get(p, ID::FilterEnvAttack) == Approx(xInvLogsc(msF / 3.f, 1.f, 60000.f, 900.f)));
}

TEST_CASE("ObxdImporter sets OB-Xf-only defaults that mirror OB-Xd hard-wired behavior", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    auto fxp = wrapAsFPCh(xml);

    Program p;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p));

    REQUIRE(get(p, ID::Osc2Keytrack) == Approx(1.f));
    REQUIRE(get(p, ID::EnvToPitchInvert) == Approx(0.f));
    REQUIRE(get(p, ID::EnvToPWInvert) == Approx(0.f));
    REQUIRE(get(p, ID::RingModVol) == Approx(0.f));
    REQUIRE(get(p, ID::NoiseColor) == Approx(0.f));
    REQUIRE(get(p, ID::Filter4PoleXpander) == Approx(0.f));
    REQUIRE(get(p, ID::FilterXpanderMode) == Approx(0.f));
    REQUIRE(get(p, ID::AmpEnvAttackCurve) == Approx(0.f));
    REQUIRE(get(p, ID::FilterEnvAttackCurve) == Approx(0.f));
    REQUIRE(get(p, ID::LFO1PW) == Approx(0.f));
    REQUIRE(get(p, ID::LFO1ToVolume) == Approx(0.f));
    REQUIRE(get(p, ID::VibratoWave) == Approx(0.f));
    REQUIRE(get(p, ID::NotePriority) == Approx(0.f));
    REQUIRE(get(p, ID::EnvLegatoMode) == Approx(0.f));
}

TEST_CASE("OB-Xd to OB-Xf Polyphony Conversion", "[presets][polyphony]")
{
    SECTION("Full Range Accuracy Test (1-32 voices)")
    {
        for (int intendedVoices = 1; intendedVoices <= MAX_VOICES; ++intendedVoices)
        {
            // This is how OB-Xd generates the value stored in the preset
            float storedValue = static_cast<float>(intendedVoices - 1) / 31.0f;
            float resultValue = transformVoiceCount(storedValue, MAX_VOICES);
            int finalResult = getEngineVoiceCount(resultValue);

            INFO("Failed at Intended Voices: " << intendedVoices);
            INFO("Stored Value: " << storedValue);
            INFO("Midpoint Value: " << resultValue);
            REQUIRE(finalResult == intendedVoices);
        }
    }

    SECTION("Floating Point Boundary Safety")
    {
        // Test 8 voices specifically (0.25 case)
        // Even if the float is slightly off in the preset file, it should still work.
        float slightlyHigh = 0.2500001f;
        float slightlyLow = 0.2499999f;

        // Both should still result in 8 voices because of your logic
        REQUIRE(getEngineVoiceCount(transformVoiceCount(slightlyHigh, MAX_VOICES)) == 8);
        REQUIRE(getEngineVoiceCount(transformVoiceCount(slightlyLow, MAX_VOICES)) == 8);
    }
}

TEST_CASE("ObxdImporter records dropped-parameter warnings", "[obxd]")
{
    auto xml = makeDiscoDSPElement();
    setXdParam(xml, OCTAVE, 0.5f);
    auto fxp = wrapAsFPCh(xml);

    Program p;
    std::vector<std::string> warnings;
    REQUIRE(ObxdImporter::importSingleOnto(fxp.getData(), fxp.getSize(), p, &warnings));

    auto contains = [&](const char *needle) {
        for (const auto &w : warnings)
            if (w.find(needle) != std::string::npos)
                return true;
        return false;
    };
    REQUIRE(contains("OCTAVE"));
}
