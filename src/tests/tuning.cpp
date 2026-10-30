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
 * Bends under a retuning: a held note's pitch, measured from the rendered
 * audio, before and after a pitch wheel or MPE bend.
 */
#include "SynthEngine.h"
#include "ParameterCoordinator.h"

#include <catch2/catch2.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
constexpr float sampleRate{48000.f};
constexpr int heldKey{69};

// 34 equal divisions of the octave, with key 69 left at 440Hz
std::array<double, 128> ed2_34()
{
    std::array<double, 128> res{};
    for (int k = 0; k < 128; ++k)
    {
        res[k] = (k - heldKey) * (12.0 / 34.0 - 1.0);
    }
    return res;
}

// steps alternating half a semitone and a semitone and a half, up and down from key 69
std::array<double, 128> uneven()
{
    std::array<double, 128> res{};
    for (int k = 0; k < 128; ++k)
    {
        const int d = k - heldKey;
        const int pairs = d >= 0 ? d / 2 : (d - 1) / 2;
        const double tuned = 2.0 * pairs + ((d - 2 * pairs) == 1 ? 0.5 : 0.0);
        res[k] = tuned - d;
    }
    return res;
}

struct BendRig
{
    std::unique_ptr<SynthEngine> engine{std::make_unique<SynthEngine>()};
    Motherboard *mb{engine->getMotherboard()};

    // a bare saw through an open filter, with nothing else moving its pitch
    BendRig()
    {
        std::srand(1);
        engine->setSampleRate(sampleRate);

        std::unordered_map<std::string, float> natural;
        for (const auto &p : ParameterList)
        {
            natural[p.ID.toStdString()] = p.meta.defaultVal;
        }

        namespace ID = SynthParam::ID;
        for (const auto &[id, v] :
             std::vector<std::pair<std::string, float>>{{ID::Osc1SawWave, 1.f},
                                                        {ID::Osc1PulseWave, 0.f},
                                                        {ID::Osc1Vol, 1.f},
                                                        {ID::Osc2Vol, 0.f},
                                                        {ID::RingModVol, 0.f},
                                                        {ID::NoiseVol, 0.f},
                                                        {ID::FilterCutoff, 75.f},
                                                        {ID::FilterResonance, 0.f},
                                                        {ID::FilterEnvAmount, 0.f},
                                                        {ID::EnvToPitchAmount, 0.f},
                                                        {ID::LFO1ModAmount1, 0.f},
                                                        {ID::LFO2ModAmount1, 0.f},
                                                        {ID::Portamento, 0.f},
                                                        {ID::Unison, 0.f},
                                                        {ID::BendUpRange, 2.f},
                                                        {ID::BendDownRange, 2.f},
                                                        {ID::BendOsc2Only, 0.f},
                                                        {ID::AmpEnvAttack, 0.f},
                                                        {ID::AmpEnvSustain, 1.f}})
        {
            REQUIRE(natural.count(id));
            natural[id] = v;
        }

        const auto &handlers = obxf::getHandlerMap();
        for (const auto &p : ParameterList)
        {
            const auto id = p.ID.toStdString();
            if (auto h = handlers.find(id); h != handlers.end())
            {
                h->second(*engine, p.meta.naturalToNormalized01(natural[id]));
            }
        }
    }

    void render(float seconds, std::vector<float> *into = nullptr)
    {
        const int n = static_cast<int>(seconds * sampleRate);
        std::vector<float> left(n), right(n);

        for (int pos = 0; pos < n; pos += 64)
        {
            engine->processBlock(left.data() + pos, right.data() + pos, std::min(64, n - pos));
        }

        if (into)
        {
            *into = left;
        }
    }

    // the held note's pitch in cents above A440, from its rising zero crossings
    double centsAboveA440()
    {
        std::vector<float> x;
        render(0.5f, &x);

        double mean{0};
        for (auto s : x)
        {
            mean += s;
        }
        mean /= x.size();

        double first{-1}, last{-1};
        int crossings{0};

        for (size_t i = 1; i < x.size(); ++i)
        {
            const double a = x[i - 1] - mean, b = x[i] - mean;
            if (a < 0 && b >= 0)
            {
                const double at = (i - 1) + a / (a - b);
                if (first < 0)
                {
                    first = at;
                }
                last = at;
                ++crossings;
            }
        }

        REQUIRE(crossings > 100);

        const double hz = (crossings - 1) * sampleRate / (last - first);
        return 1200.0 * std::log2(hz / 440.0);
    }

    // how far a bend moves the held note, in cents
    template <typename Bend> double bendCents(Bend &&bend, int8_t channel = 1)
    {
        engine->processNoteOn(heldKey, 0.8f, channel);
        render(0.2f);
        const auto before = centsAboveA440();

        bend();
        render(0.2f);

        return centsAboveA440() - before;
    }
};

constexpr double cents34{1200.0 / 34.0};
constexpr double tolerance{0.5};
} // namespace

TEST_CASE("Tuning: fractional keys interpolate between the retuned keys", "[tuning]")
{
    Tuning t;
    REQUIRE(t.tunedKeyAt(61.25f, 0) == 61.25f);

    t.setRetuning(ed2_34());
    REQUIRE(t.tunedMidiNote(heldKey, 0) == heldKey);
    REQUIRE(t.tunedKeyAt(71.f, 0) == Approx(heldKey + 2 * 12.0 / 34.0));
    REQUIRE(t.tunedKeyAt(69.5f, 0) == Approx(heldKey + 0.5 * 12.0 / 34.0));

    // off the keyboard the scale gives way to 12-TET
    REQUIRE(t.tunedKeyAt(130.f, 0) == Approx(t.tunedKeyAt(127.f, 0) + 3.f));
    REQUIRE(t.tunedKeyAt(-2.f, 0) == Approx(t.tunedKeyAt(0.f, 0) - 2.f));
}

TEST_CASE("Tuning: each channel has its own retuning, bar under MPE", "[tuning]")
{
    Tuning t;
    t.setRetuning({});
    t.setRetuning(ed2_34(), 3);

    const auto tuned71 = Approx(heldKey + 2 * 12.0 / 34.0);

    REQUIRE(t.tunedMidiNote(71, 3) == tuned71);
    REQUIRE(t.tunedMidiNote(71, 4) == 71);
    REQUIRE(t.tunedMidiNote(71, 0) == 71);

    // under MPE every note shares the first channel's
    t.usesMPE(true);
    REQUIRE(t.tunedMidiNote(71, 3) == 71);

    t.setRetuning(ed2_34(), 0);
    REQUIRE(t.tunedMidiNote(71, 4) == tuned71);

    t.usesMPE(false);
    REQUIRE(t.tunedMidiNote(71, 4) == 71);
}

TEST_CASE("Tuning: the pitch wheel follows the scale on the note's channel", "[tuning]")
{
    BendRig rig;
    rig.mb->tuning.setRetuning({});
    rig.mb->tuning.setRetuning(ed2_34(), 5);

    const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(1.f); }, 5);
    REQUIRE(c == Approx(2 * cents34).margin(tolerance));
}

TEST_CASE("Tuning: the pitch wheel bends along an ED2-34 scale", "[tuning]")
{
    BendRig rig;
    rig.mb->tuning.setRetuning(ed2_34());

    SECTION("a full bend up moves two keys")
    {
        const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(1.f); });
        REQUIRE(c == Approx(2 * cents34).margin(tolerance));
    }

    SECTION("a full bend down moves two keys")
    {
        const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(-1.f); });
        REQUIRE(c == Approx(-2 * cents34).margin(tolerance));
    }

    SECTION("a quarter bend moves half a key")
    {
        const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(0.25f); });
        REQUIRE(c == Approx(0.5 * cents34).margin(tolerance));
    }

    SECTION("turned off, the wheel bends in 12-TET")
    {
        rig.mb->tuning.scaleAwareWheel = false;
        const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(1.f); });
        REQUIRE(c == Approx(200.0).margin(tolerance));
    }
}

TEST_CASE("Tuning: the MPE glide bends along an ED2-34 scale", "[tuning]")
{
    BendRig rig;
    rig.mb->tuning.setRetuning(ed2_34());
    rig.mb->mpeEnabled = true;

    // two keys of the 48 semitone glide range
    const auto twoKeys = 2.f / rig.mb->mpePitchBendRange;

    SECTION("scale-aware")
    {
        const auto c = rig.bendCents([&] { rig.engine->processMPEPitch(2, twoKeys); }, 2);
        REQUIRE(c == Approx(2 * cents34).margin(tolerance));
    }

    SECTION("turned off, it bends in 12-TET")
    {
        rig.mb->tuning.scaleAwareMPE = false;
        const auto c = rig.bendCents([&] { rig.engine->processMPEPitch(2, twoKeys); }, 2);
        REQUIRE(c == Approx(200.0).margin(tolerance));
    }
}

TEST_CASE("Tuning: a bend between keys of an uneven scale lands between them", "[tuning]")
{
    BendRig rig;
    rig.mb->tuning.setRetuning(uneven());

    // 1.5 keys: the 50 cents to key 70, then half its 150 cent step to 71
    const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(0.75f); });
    REQUIRE(c == Approx(50.0 + 75.0).margin(tolerance));
}

TEST_CASE("Tuning: without a retuning the wheel bends in 12-TET", "[tuning]")
{
    BendRig rig;
    const auto c = rig.bendCents([&] { rig.engine->processPitchWheel(1.f); });
    REQUIRE(c == Approx(200.0).margin(tolerance));
}
