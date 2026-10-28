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

/*
 * Whole-engine golden renders, for checking that a restructure of the
 * engine changes nothing. Factory patches play a fixed MIDI script through
 * SynthEngine::processBlock at irregular block sizes, with every random
 * source seeded.
 *
 * Hidden from the default run. To use, from the build directory:
 *   OBXF_ENGINE_GOLDEN=write OBXF_ENGINE_GOLDEN_DIR=/some/dir ./obxf-tests "[engine-golden]"
 * then after changing the engine
 *   OBXF_ENGINE_GOLDEN_DIR=/some/dir ./obxf-tests "[engine-golden]"
 * which requires every render to match bit for bit.
 *
 * OBXF_ENGINE_GOLDEN_TOLERANCE_DB=-70 instead passes renders whose peak error
 * is at least that far below the golden's peak, and OBXF_ENGINE_GOLDEN_REPORT
 * names a csv file to write the per-case error levels to.
 *
 * Goldens are only comparable on the machine and build that wrote them.
 */
#include "SynthEngine.h"
#include "ParameterCoordinator.h"

#include <catch2/catch2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr float goldenSampleRate{48000.f};
constexpr int goldenLength{static_cast<int>(goldenSampleRate * 3.5f)};

struct Event
{
    enum Kind
    {
        NoteOn,
        NoteOff,
        SustainOn,
        SustainOff,
        PitchWheel,
        ModWheel,
        MPEPitch,
        MPETimbre,
        MPEPressure
    };

    int at;
    Kind kind;
    int key{0};
    float value{0.f};
    int8_t channel{1};
};

std::vector<Event> polyScript()
{
    std::vector<Event> ev;
    auto on = [&](int at, int key, float vel) { ev.push_back({at, Event::NoteOn, key, vel}); };
    auto off = [&](int at, int key) { ev.push_back({at, Event::NoteOff, key, 0.5f}); };

    // a chord with onsets a few samples apart
    on(0, 48, 0.8f);
    on(3, 55, 0.7f);
    on(4, 60, 0.6f);
    on(129, 64, 0.9f);

    for (int i = 0; i < 40; ++i)
    {
        ev.push_back({6000 + i * 300, Event::PitchWheel, 0, std::sin(i * 0.4f)});
    }

    for (int i = 0; i < 20; ++i)
    {
        ev.push_back({9000 + i * 500, Event::ModWheel, 0, i / 19.f});
    }

    off(24000, 48);
    off(24001, 55);
    off(26000, 60);
    off(26050, 64);

    // sustain holds a released note
    ev.push_back({30000, Event::SustainOn});
    on(30500, 67, 0.75f);
    off(31000, 67);
    ev.push_back({40000, Event::SustainOff});

    // sustain released with nothing gated
    ev.push_back({42000, Event::SustainOn});
    ev.push_back({42100, Event::SustainOff});

    // more notes than voices
    for (int k = 0; k < 10; ++k)
    {
        on(50000 + k * 7, 40 + k * 3, 0.5f + 0.05f * k);
    }

    for (int k = 0; k < 10; ++k)
    {
        off(60000 + k * 997, 40 + k * 3);
    }

    // a one-sample note after silence
    on(100000, 72, 1.f);
    off(100001, 72);

    // retrigger while gated, then a quick repeat
    on(110000, 60, 0.8f);
    on(112000, 60, 0.4f);
    off(115000, 60);
    on(115100, 60, 0.6f);
    off(118000, 60);

    return ev;
}

std::vector<Event> mpeScript()
{
    std::vector<Event> ev;

    for (int k = 0; k < 4; ++k)
    {
        const int8_t ch = static_cast<int8_t>(2 + k);
        const int start = k * 1500;

        ev.push_back({start, Event::NoteOn, 50 + k * 5, 0.4f + 0.15f * k, ch});

        for (int i = 0; i < 30; ++i)
        {
            const int at = start + 2000 + i * 700;
            ev.push_back({at, Event::MPEPitch, 0, std::sin(i * 0.3f + k) * 0.2f, ch});
            ev.push_back({at + 100, Event::MPETimbre, 0, 0.5f + 0.5f * std::cos(i * 0.2f), ch});
            ev.push_back({at + 200, Event::MPEPressure, 0, (i % 10) / 9.f, ch});
        }

        ev.push_back({40000 + k * 3000, Event::NoteOff, 50 + k * 5, 0.2f * k, ch});
    }

    return ev;
}

struct GoldenCase
{
    std::string name;
    juce::File patch; // none plays the default patch
    bool forceHQ{false};
    bool matrixRows{false};
    bool mpe{false};
};

const juce::File &factoryPatchDir()
{
    static const juce::File dir{OBXF_FACTORY_PATCH_DIR};
    return dir;
}

std::vector<GoldenCase> goldenCases()
{
    auto all = factoryPatchDir().findChildFiles(juce::File::findFiles, true, "*.fxp");
    std::sort(all.begin(), all.end(), [](const auto &a, const auto &b) {
        return a.getFullPathName() < b.getFullPathName();
    });

    auto caseFor = [](const juce::File &f) {
        return GoldenCase{f.getParentDirectory().getFileName().toStdString() + "_" +
                              f.getFileNameWithoutExtension().toStdString(),
                          f};
    };

    std::vector<GoldenCase> res;
    res.push_back({"default"});

    for (int i = 0; i < all.size(); i += 8)
    {
        res.push_back(caseFor(all[i]));
    }

    // patches the profiling work leaned on
    for (auto *p :
         {"Strings/Uneasy Strings", "Pads/Sun Ray", "Atmospheres/Vibrores", "Keys/Accordion 3",
          "Organs/Klassika", "Brass/Trumpet", "Basses/Picked Bass", "Leads/Pulsapart"})
    {
        auto f = factoryPatchDir().getChildFile(juce::String(p) + ".fxp");
        if (f.existsAsFile())
        {
            res.push_back(caseFor(f));
        }
    }

    for (int i = 3; i < all.size(); i += 61)
    {
        auto c = caseFor(all[i]);
        c.name += "_hq";
        c.forceHQ = true;
        res.push_back(c);
    }

    for (int i = 5; i < all.size(); i += 97)
    {
        auto c = caseFor(all[i]);
        c.name += "_matrix";
        c.matrixRows = true;
        res.push_back(c);

        c.name += "_mpe";
        c.mpe = true;
        res.push_back(c);
    }

    return res;
}

bool applyPatch(SynthEngine &engine, const juce::File &patch)
{
    std::unordered_map<std::string, float> values;
    for (const auto &p : ParameterList)
    {
        values[p.ID.toStdString()] = p.meta.naturalToNormalized01(p.meta.defaultVal);
    }

    std::unique_ptr<juce::XmlElement> xml;

    if (patch != juce::File())
    {
        juce::MemoryBlock mb;
        if (!patch.loadFileAsData(mb) || mb.getSize() < sizeof(fxProgramSet))
        {
            return false;
        }

        const auto *set = static_cast<const fxProgramSet *>(mb.getData());
        if (!compareMagic(set->chunkMagic, "CcnK") || !compareMagic(set->fxMagic, "FPCh"))
        {
            return false;
        }

        xml = juce::AudioProcessor::getXmlFromBinary(set->chunk, fxbSwap(set->chunkSize));
        if (!xml)
        {
            return false;
        }

        const auto version =
            fromHumanReadableVersion(xml->getStringAttribute("ob-xf_version").toStdString());

        for (auto &[id, v] : values)
        {
            if (xml->hasAttribute(id))
            {
                v = static_cast<float>(xml->getDoubleAttribute(id, v));
            }

            // mirrors the legacy mapping in StateManager::populateProgramFromXml
            if (version < 0x2025'12'13 && id == SynthParam::ID::UnisonVoices)
            {
                const auto old = juce::jlimit(0, MAX_PANNINGS - 1,
                                              static_cast<int>(std::round(v * (MAX_PANNINGS - 1))));
                v = static_cast<float>(old) / static_cast<float>(MAX_VOICES - 1);
            }
        }
    }

    const auto &handlers = obxf::getHandlerMap();
    for (const auto &p : ParameterList)
    {
        const auto id = p.ID.toStdString();
        if (auto h = handlers.find(id); h != handlers.end())
        {
            h->second(engine, values[id]);
        }
    }

    auto *mb = engine.getMotherboard();
    mb->voiceMatrix.fromElement(xml ? xml->getChildByName("VoiceMatrix") : nullptr);
    mb->recalculateAllMatrices();

    return true;
}

void applyEvent(SynthEngine &engine, const Event &e)
{
    switch (e.kind)
    {
    case Event::NoteOn:
        engine.processNoteOn(e.key, e.value, e.channel);
        break;
    case Event::NoteOff:
        engine.processNoteOff(e.key, e.value, e.channel);
        break;
    case Event::SustainOn:
        engine.sustainOn();
        break;
    case Event::SustainOff:
        engine.sustainOff();
        break;
    case Event::PitchWheel:
        engine.processPitchWheel(e.value);
        break;
    case Event::ModWheel:
        engine.processModWheel(e.value);
        break;
    case Event::MPEPitch:
        engine.processMPEPitch(e.channel, e.value);
        break;
    case Event::MPETimbre:
        engine.processMPETimbre(e.channel, e.value);
        break;
    case Event::MPEPressure:
        engine.processMPEChannelPressure(e.channel, e.value);
        break;
    }
}

// interleaved stereo; the block walk mirrors ObxfAudioProcessor::processBlock
std::vector<float> renderCase(const GoldenCase &c)
{
    // noise generators are seeded from std::rand whenever the sample rate is set
    std::srand(1);

    auto engine = std::make_unique<SynthEngine>();
    auto *mb = engine->getMotherboard();

    mb->globalLFO.seedRandom(11);
    mb->vibratoLFO.seedRandom(12);
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        mb->voices[i].seedRandom(100 + 2 * i);
    }

    engine->setSampleRate(goldenSampleRate);
    REQUIRE(applyPatch(*engine, c.patch));

    if (c.forceHQ)
    {
        engine->processHQMode(1.f);
    }

    if (c.matrixRows)
    {
        auto &vm = mb->voiceMatrix;
        REQUIRE(vm.setModulation("Strike", SynthParam::ID::FilterCutoff, 0.4f, 0));
        REQUIRE(vm.setModulation("Lift", SynthParam::ID::AmpEnvRelease, -0.3f, 1));
        REQUIRE(vm.setModulation("Strike", SynthParam::ID::FilterEnvAttack, 0.5f, 2));
        REQUIRE(vm.setModulation("Press", SynthParam::ID::FilterResonance, 0.3f, 3));
        REQUIRE(vm.setModulation("Slide", SynthParam::ID::Osc2PWOffset, 0.25f, 4));
        REQUIRE(vm.setModulation("Glide", SynthParam::ID::LFO2Rate, 0.5f, 5));
        mb->recalculateAllMatrices();
    }

    mb->mpeEnabled = c.mpe;

    auto events = c.mpe ? mpeScript() : polyScript();
    std::stable_sort(events.begin(), events.end(),
                     [](const auto &a, const auto &b) { return a.at < b.at; });

    std::vector<float> left(goldenLength), right(goldenLength);

    uint32_t lcg{0x1234567u};
    double ppq{0.0};
    size_t nextEvent{0};
    int pos{0};

    while (pos < goldenLength)
    {
        lcg = lcg * 1664525u + 1013904223u;
        const int hostEnd = std::min(goldenLength, pos + 1 + static_cast<int>((lcg >> 8) % 512));

        engine->setPlayHead(120.f, static_cast<float>(ppq), pos == 0);
        ppq += (hostEnd - pos) * 120.0 / (60.0 * goldenSampleRate);

        while (pos < hostEnd)
        {
            while (nextEvent < events.size() && events[nextEvent].at <= pos)
            {
                applyEvent(*engine, events[nextEvent++]);
            }

            int blockEnd = hostEnd;
            if (nextEvent < events.size())
            {
                blockEnd = std::clamp(events[nextEvent].at, pos + 1, hostEnd);
            }

            engine->processBlock(left.data() + pos, right.data() + pos, blockEnd - pos);
            pos = blockEnd;
        }
    }

    std::vector<float> out(goldenLength * 2);
    for (int i = 0; i < goldenLength; ++i)
    {
        out[2 * i] = left[i];
        out[2 * i + 1] = right[i];
    }

    return out;
}

std::string goldenFileName(std::string name)
{
    for (auto &ch : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_' && ch != '-')
        {
            ch = '_';
        }
    }

    return name + ".f32";
}
} // namespace

TEST_CASE("Engine golden renders", "[.][engine-golden]")
{
    const auto *dirEnv = std::getenv("OBXF_ENGINE_GOLDEN_DIR");
    if (dirEnv == nullptr)
    {
        WARN("set OBXF_ENGINE_GOLDEN_DIR to write or compare engine goldens");
        return;
    }

    const auto *modeEnv = std::getenv("OBXF_ENGINE_GOLDEN");
    const bool writing = modeEnv != nullptr && std::string(modeEnv) == "write";

    const juce::File dir{juce::String(dirEnv)};
    REQUIRE(factoryPatchDir().isDirectory());

    if (writing)
    {
        REQUIRE(dir.createDirectory());
    }

    // a mismatch passes if its peak error sits this far below the golden's peak
    const auto *toleranceEnv = std::getenv("OBXF_ENGINE_GOLDEN_TOLERANCE_DB");
    const bool tolerant = toleranceEnv != nullptr;
    const double toleranceDb = tolerant ? std::atof(toleranceEnv) : 0.0;

    // one csv row per case, for comparing engine changes
    const auto *reportEnv = std::getenv("OBXF_ENGINE_GOLDEN_REPORT");
    std::string report = "case,first_frame,peak_error_db,rms_error_db\n";

    const auto cases = goldenCases();
    INFO(cases.size() << " cases");

    for (const auto &c : cases)
    {
        INFO("case " << c.name);

        const auto out = renderCase(c);
        const auto file = dir.getChildFile(goldenFileName(c.name));

        if (writing)
        {
            REQUIRE(file.replaceWithData(out.data(), out.size() * sizeof(float)));
            continue;
        }

        juce::MemoryBlock expected;
        REQUIRE(file.loadFileAsData(expected));
        REQUIRE(expected.getSize() == out.size() * sizeof(float));

        if (std::memcmp(expected.getData(), out.data(), expected.getSize()) == 0)
        {
            report += c.name + ",exact,,\n";
            continue;
        }

        const auto *exp = static_cast<const float *>(expected.getData());
        size_t first = out.size();
        double peakRef{0}, peakErr{0}, sumRef{0}, sumErr{0};

        for (size_t i = 0; i < out.size(); ++i)
        {
            const double err = static_cast<double>(out[i]) - exp[i];

            if (err != 0.0)
            {
                first = std::min(first, i);
            }

            peakRef = std::max(peakRef, std::fabs(static_cast<double>(exp[i])));
            peakErr = std::max(peakErr, std::fabs(err));
            sumRef += static_cast<double>(exp[i]) * exp[i];
            sumErr += err * err;
        }

        auto db = [](double num, double den) {
            return num <= 0.0 ? -300.0 : 20.0 * std::log10(num / std::max(den, 1e-30));
        };
        const double peakDb = db(peakErr, peakRef);
        const double rmsDb = db(std::sqrt(sumErr), std::sqrt(sumRef));

        report += c.name + "," + std::to_string(first / 2) + "," + std::to_string(peakDb) + "," +
                  std::to_string(rmsDb) + "\n";

        if (!tolerant || peakDb > toleranceDb)
        {
            FAIL_CHECK("differs from sample frame " << first / 2 << " (channel " << first % 2
                                                    << "), peak error " << peakDb
                                                    << " dB, rms error " << rmsDb << " dB");
        }
    }

    if (reportEnv != nullptr)
    {
        const juce::File reportFile{juce::String(reportEnv)};
        REQUIRE(reportFile.getParentDirectory().createDirectory());
        REQUIRE(reportFile.replaceWithText(report));
    }
}
