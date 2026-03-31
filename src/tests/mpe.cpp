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

/*
 * Include SynthEngine.h first so the include chain resolves correctly —
 * same pattern as osc.cpp and filt.cpp.
 */
#include "SynthEngine.h"

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch2.hpp>

#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

/* Approximate equality. */
static bool approxEq(float a, float b, float tol = 1e-5f) { return std::abs(a - b) <= tol; }

/* Minimal engine set up for MPE testing. Polyphony=8, mpeEnabled=true. */
struct MpeEngine
{
    SynthEngine eng;
    Motherboard *mb;

    MpeEngine()
    {
        eng.setSampleRate(44100.f);
        mb = eng.getMotherboard();
        mb->setPolyphony(8);
        mb->mpeEnabled = true;
    }

    /* First gated voice on the given channel, or nullptr. */
    Voice *gatedOnChannel(int8_t ch) const
    {
        for (int i = 0; i < MAX_VOICES; ++i)
            if (mb->voices[i].isGated() && mb->voices[i].channel == ch)
                return &mb->voices[i];
        return nullptr;
    }

    /* Total number of currently gated voices. */
    int gatedCount() const
    {
        int n = 0;
        for (int i = 0; i < MAX_VOICES; ++i)
            if (mb->voices[i].isGated())
                ++n;
        return n;
    }
};

// ===========================================================================
// Layer 1: Pure VoiceMatrix unit tests (no engine required)
// ===========================================================================

TEST_CASE("VoiceMatrix: single row Velocity->FilterCutoff depth 0.5, source 1.0", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 0.5f, 0));

    VoiceMatrixSourceValues src;
    src.velocity = 1.0f;
    VoiceMatrixAdjustments adj;
    recalculateMatrix(vm, src, adj);

    /* contribution = 1.0 * 0.5 = 0.5; range scaling happens at point of use in ProcessSample */
    REQUIRE(approxEq(adj.filterCutoff, 0.5f));
    REQUIRE(approxEq(adj.filterResonance, 0.f));
    REQUIRE(approxEq(adj.osc1Pitch, 0.f));
}

TEST_CASE("VoiceMatrix: two rows targeting the same param accumulate", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 0.3f, 0));
    REQUIRE(vm.setModulation("Timbre", SynthParam::ID::FilterCutoff, 0.4f, 1));

    VoiceMatrixSourceValues src;
    src.velocity = 1.0f;
    src.timbre = 1.0f;
    VoiceMatrixAdjustments adj;
    recalculateMatrix(vm, src, adj);

    REQUIRE(approxEq(adj.filterCutoff, 0.7f)); /* 0.3 + 0.4 */
}

TEST_CASE("VoiceMatrix: depth zero contributes nothing", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 0.f, 0));

    VoiceMatrixSourceValues src;
    src.velocity = 1.0f;
    VoiceMatrixAdjustments adj;
    recalculateMatrix(vm, src, adj);

    REQUIRE(approxEq(adj.filterCutoff, 0.f));
}

TEST_CASE("VoiceMatrix: source value zero contributes nothing", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 1.0f, 0));

    VoiceMatrixSourceValues src; /* velocity defaults to 0 */
    VoiceMatrixAdjustments adj;
    recalculateMatrix(vm, src, adj);

    REQUIRE(approxEq(adj.filterCutoff, 0.f));
}

TEST_CASE("VoiceMatrix: negative depth inverts the adjustment", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, -0.5f, 0));

    VoiceMatrixSourceValues src;
    src.velocity = 1.0f;
    VoiceMatrixAdjustments adj;
    recalculateMatrix(vm, src, adj);

    REQUIRE(approxEq(adj.filterCutoff, -0.5f));
}

TEST_CASE("VoiceMatrix: setModulation rejects unknown source string", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE_FALSE(vm.setModulation("Aftertouch", SynthParam::ID::FilterCutoff, 1.f, 0));
    REQUIRE_FALSE(vm.rows[0].isActive());
}

TEST_CASE("VoiceMatrix: setModulation rejects unknown target string", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE_FALSE(vm.setModulation("Velocity", "NonExistentParam", 1.f, 0));
    REQUIRE_FALSE(vm.rows[0].isActive());
}

TEST_CASE("VoiceMatrix: setModulation rejects out-of-range index", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE_FALSE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 1.f, numMatrixRows));
    REQUIRE_FALSE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 1.f, -1));
}

TEST_CASE("VoiceMatrix: isLFO2Bound cache tracks LFO2 row presence", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE_FALSE(vm.isLFO2Bound);

    REQUIRE(vm.setModulation("LFO2", SynthParam::ID::FilterCutoff, 1.f, 0));
    REQUIRE(vm.isLFO2Bound);

    /* Adding a non-LFO2 row: still bound */
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterResonance, 0.5f, 1));
    REQUIRE(vm.isLFO2Bound);

    /* Clearing the LFO2 row: no longer bound */
    vm.clearRow(0);
    REQUIRE_FALSE(vm.isLFO2Bound);
}

TEST_CASE("VoiceMatrix: non-LFO2 row does not set isLFO2Bound", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 1.f, 0));
    REQUIRE_FALSE(vm.isLFO2Bound);
}

TEST_CASE("VoiceMatrix: XML round-trip preserves all active rows", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 0.5f, 0));
    REQUIRE(vm.setModulation("Timbre", SynthParam::ID::Osc1Pitch, -0.3f, 1));
    REQUIRE(vm.setModulation("ChannelPressure", SynthParam::ID::FilterResonance, 0.7f, 2));

    auto el = vm.toElement();

    VoiceMatrix vm2;
    vm2.fromElement(el.get());

    REQUIRE(vm2.rows[0].source == MatrixSource::Velocity);
    REQUIRE(vm2.rows[0].target == SynthParam::ID::FilterCutoff);
    REQUIRE(approxEq(vm2.rows[0].depth, 0.5f));

    REQUIRE(vm2.rows[1].source == MatrixSource::Timbre);
    REQUIRE(vm2.rows[1].target == SynthParam::ID::Osc1Pitch);
    REQUIRE(approxEq(vm2.rows[1].depth, -0.3f));

    REQUIRE(vm2.rows[2].source == MatrixSource::ChannelPressure);
    REQUIRE(vm2.rows[2].target == SynthParam::ID::FilterResonance);
    REQUIRE(approxEq(vm2.rows[2].depth, 0.7f));

    /* Unused rows remain inactive */
    for (int i = 3; i < numMatrixRows; ++i)
        REQUIRE_FALSE(vm2.rows[i].isActive());
}

TEST_CASE("VoiceMatrix: fromElement with nullptr clears all rows", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("Velocity", SynthParam::ID::FilterCutoff, 1.f, 0));

    vm.fromElement(nullptr);

    for (int i = 0; i < numMatrixRows; ++i)
        REQUIRE_FALSE(vm.rows[i].isActive());
    REQUIRE_FALSE(vm.isLFO2Bound);
}

TEST_CASE("VoiceMatrix: clear() resets rows and cache", "[VoiceMatrix]")
{
    VoiceMatrix vm;
    REQUIRE(vm.setModulation("LFO2", SynthParam::ID::FilterCutoff, 1.f, 0));
    REQUIRE(vm.isLFO2Bound);

    vm.clear();

    REQUIRE_FALSE(vm.isLFO2Bound);
    for (int i = 0; i < numMatrixRows; ++i)
        REQUIRE_FALSE(vm.rows[i].isActive());
}

TEST_CASE("VoiceMatrix: VoiceMatrixSourceValues get/set round-trips all sources", "[VoiceMatrix]")
{
    VoiceMatrixSourceValues src;
    src.set(MatrixSource::VoiceBend, 0.1f);
    src.set(MatrixSource::ChannelPressure, 0.2f);
    src.set(MatrixSource::Timbre, 0.3f);
    src.set(MatrixSource::Velocity, 0.4f);
    src.set(MatrixSource::ReleaseVelocity, 0.5f);
    src.set(MatrixSource::LFO2, 0.6f);

    REQUIRE(approxEq(src.get(MatrixSource::VoiceBend), 0.1f));
    REQUIRE(approxEq(src.get(MatrixSource::ChannelPressure), 0.2f));
    REQUIRE(approxEq(src.get(MatrixSource::Timbre), 0.3f));
    REQUIRE(approxEq(src.get(MatrixSource::Velocity), 0.4f));
    REQUIRE(approxEq(src.get(MatrixSource::ReleaseVelocity), 0.5f));
    REQUIRE(approxEq(src.get(MatrixSource::LFO2), 0.6f));
    REQUIRE(approxEq(src.get(MatrixSource::None), 0.f));
}

// ===========================================================================
// Layer 2: Motherboard integration tests
// ===========================================================================

/*
 * NOTE: NoteOn does not check channel during voice allocation — two notes on
 * the same pitch number but different channels will retrigger the same voice.
 * The tests below use different note numbers where channel isolation matters.
 */

TEST_CASE("MPE: channel is recorded on the voice at NoteOn", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.eng.processNoteOn(60, 0.7f, 5);

    Voice *v = e.gatedOnChannel(5);
    REQUIRE(v != nullptr);
    REQUIRE(v->midiNote == 60);
    REQUIRE(v->channel == 5);
}

TEST_CASE("MPE: NoteOff on channel releases only that channel's voice", "[MPE][Motherboard]")
{
    MpeEngine e;
    /* Different notes so NoteOn does not retrigger the same voice */
    e.eng.processNoteOn(60, 0.8f, 2);
    e.eng.processNoteOn(62, 0.8f, 3);

    REQUIRE(e.gatedCount() == 2);

    e.eng.processNoteOff(60, 0.f, 2);

    REQUIRE(e.gatedOnChannel(2) == nullptr);
    REQUIRE(e.gatedOnChannel(3) != nullptr);
}

TEST_CASE("MPE: processMPEPitch sets voiceBend on the matching channel only", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.eng.processNoteOn(60, 0.5f, 2);
    e.eng.processNoteOn(62, 0.5f, 3);

    e.mb->processMPEPitch(2, 1.0f);

    Voice *v2 = e.gatedOnChannel(2);
    Voice *v3 = e.gatedOnChannel(3);
    REQUIRE(v2 != nullptr);
    REQUIRE(v3 != nullptr);

    REQUIRE(approxEq(v2->matrixSourceValues.voiceBend, 1.0f));
    REQUIRE(approxEq(v3->matrixSourceValues.voiceBend, 0.f)); /* untouched */
}

TEST_CASE("MPE: processMPEPitch scales mpeBend by mpePitchBendRange", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.mb->mpePitchBendRange = 48;
    e.eng.processNoteOn(60, 0.5f, 2);

    e.mb->processMPEPitch(2, 1.0f);

    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);
    REQUIRE(approxEq(v->mpeBend, 48.f));
}

TEST_CASE("MPE: processMPEPitch negative bend produces negative mpeBend", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.mb->mpePitchBendRange = 48;
    e.eng.processNoteOn(60, 0.5f, 2);

    e.mb->processMPEPitch(2, -1.0f);

    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);
    REQUIRE(approxEq(v->mpeBend, -48.f));
}

TEST_CASE("MPE: processMPETimbre normalises 0..1 to -1..1", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.eng.processNoteOn(60, 0.5f, 4);
    Voice *v = e.gatedOnChannel(4);
    REQUIRE(v != nullptr);

    e.mb->processMPETimbre(4, 0.f);
    REQUIRE(approxEq(v->matrixSourceValues.timbre, -1.f));

    e.mb->processMPETimbre(4, 0.5f);
    REQUIRE(approxEq(v->matrixSourceValues.timbre, 0.f));

    e.mb->processMPETimbre(4, 1.f);
    REQUIRE(approxEq(v->matrixSourceValues.timbre, 1.f));
}

TEST_CASE("MPE: processMPEChannelPressure normalises 0..1 to -1..1", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.eng.processNoteOn(60, 0.5f, 6);
    Voice *v = e.gatedOnChannel(6);
    REQUIRE(v != nullptr);

    e.mb->processMPEChannelPressure(6, 0.f);
    REQUIRE(approxEq(v->matrixSourceValues.channelPressure, -1.f));

    e.mb->processMPEChannelPressure(6, 0.5f);
    REQUIRE(approxEq(v->matrixSourceValues.channelPressure, 0.f));

    e.mb->processMPEChannelPressure(6, 1.f);
    REQUIRE(approxEq(v->matrixSourceValues.channelPressure, 1.f));
}

TEST_CASE("MPE: velocity is stored in matrixSourceValues at NoteOn", "[MPE][Motherboard]")
{
    MpeEngine e;

    e.eng.processNoteOn(60, 1.0f, 2);
    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);
    REQUIRE(approxEq(v->matrixSourceValues.velocity, 1.f));
    e.eng.processNoteOff(60, 0.f, 2);

    e.eng.processNoteOn(60, 0.5f, 2);
    Voice *v2 = e.gatedOnChannel(2);
    REQUIRE(v2 != nullptr);
    REQUIRE(approxEq(v2->matrixSourceValues.velocity, 0.5f));
}

TEST_CASE("MPE: release velocity is stored in matrixSourceValues at NoteOff", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.eng.processNoteOn(60, 0.8f, 3);
    Voice *v = e.gatedOnChannel(3);
    REQUIRE(v != nullptr);

    /* Capture before NoteOff since the voice ungates */
    e.eng.processNoteOff(60, 0.5f, 3);

    REQUIRE(approxEq(v->matrixSourceValues.releaseVelocity, 0.5f));
}

TEST_CASE("MPE: matrix adjustment is computed from velocity immediately at NoteOn",
          "[MPE][Motherboard]")
{
    MpeEngine e;
    e.mb->voiceMatrix.setModulation("Velocity", SynthParam::ID::FilterCutoff, 1.f, 0);

    e.eng.processNoteOn(60, 1.0f, 2);
    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);

    /* adj = velocity(1.0) * depth(1.0) = 1.0 */
    REQUIRE(approxEq(v->matrixAdjustments.filterCutoff, 1.f));
}

TEST_CASE("MPE: matrix adjustment updates when an MPE message is received", "[MPE][Motherboard]")
{
    MpeEngine e;
    e.mb->voiceMatrix.setModulation("Timbre", SynthParam::ID::FilterCutoff, 1.f, 0);

    e.eng.processNoteOn(60, 0.5f, 2);
    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);

    /* Full timbre: normalised = 2*1.0 - 1 = 1.0 → adj = 1.0 * 1.0 = 1.0 */
    e.mb->processMPETimbre(2, 1.f);
    REQUIRE(approxEq(v->matrixAdjustments.filterCutoff, 1.f));

    /* Zero timbre: normalised = 2*0.0 - 1 = -1.0 → adj = -1.0 * 1.0 = -1.0 */
    e.mb->processMPETimbre(2, 0.f);
    REQUIRE(approxEq(v->matrixAdjustments.filterCutoff, -1.f));
}

TEST_CASE("MPE: mpeBend is reset to zero on NoteOn (no bleed from previous note)",
          "[MPE][Motherboard]")
{
    MpeEngine e;
    e.eng.processNoteOn(60, 0.5f, 2);

    /* Apply a non-zero bend */
    e.mb->processMPEPitch(2, 1.0f);
    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);
    REQUIRE(v->mpeBend != 0.f);

    /* Release, then retrigger same channel — NoteOn must zero mpeBend */
    e.eng.processNoteOff(60, 0.f, 2);
    e.eng.processNoteOn(60, 0.5f, 2);

    Voice *v2 = e.gatedOnChannel(2);
    REQUIRE(v2 != nullptr);
    REQUIRE(approxEq(v2->mpeBend, 0.f));
}

TEST_CASE("MPE: matrix adjustments are cleared on NoteOn", "[MPE][Motherboard]")
{
    MpeEngine e;
    /* Wire Timbre → FilterCutoff so adjustments are nonzero after timbre message */
    e.mb->voiceMatrix.setModulation("Timbre", SynthParam::ID::FilterCutoff, 1.f, 0);

    e.eng.processNoteOn(60, 0.5f, 2);
    e.mb->processMPETimbre(2, 1.f); /* adj.filterCutoff becomes 1.0 */

    e.eng.processNoteOff(60, 0.f, 2);
    e.eng.processNoteOn(60, 0.5f, 2); /* fresh note: timbre source cleared → adj reset */

    Voice *v = e.gatedOnChannel(2);
    REQUIRE(v != nullptr);
    /* Timbre source value was cleared; only Velocity contributes, not Timbre */
    REQUIRE(approxEq(v->matrixSourceValues.timbre, 0.f));
    REQUIRE(approxEq(v->matrixAdjustments.filterCutoff, 0.f));
}
