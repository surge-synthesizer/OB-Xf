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

#ifndef OBXF_SRC_ENGINE_VOICEMATRIX_H
#define OBXF_SRC_ENGINE_VOICEMATRIX_H

#include <array>
#include <string>
#include <unordered_set>

#include <juce_core/juce_core.h>

#include "configuration.h"
#include "SynthParam.h"

/*
 * VoiceMatrix: per-synth modulation routing from MPE/voice sources to per-voice targets.
 * Sources are enumerated below; targets are SynthParam ID strings.
 * The matrix has NUM_MATRIX_ROWS rows, each with a source, target, and depth.
 */

/*
 * HOW TO ADD A NEW MODULATION SOURCE
 * ------------------------------------
 * 1. MatrixSource enum      — add your entry (e.g. Aftertouch).
 * 2. matrixSourceToString() — add a case returning a stable string literal.
 * 3. matrixSourceFromString() — add the matching reverse case.
 * 4. VoiceMatrixSourceValues — add a float field (e.g. aftertouch{0.f}).
 * 5. VoiceMatrixSourceValues::get() — add a case returning the new field.
 * 6. VoiceMatrixSourceValues::set() — add a case writing the new field.
 * 7. Wire it up — call setMatrixSource() + recalculateMatrix() from wherever
 *    the source value arrives:
 *      - Per-voice at note start: Voice::NoteOn() sets the field after
 *        matrixSourceValues.clear(); Motherboard calls recalculateMatrix().
 *      - Per-voice at note end: Voice::NoteOff() sets the field;
 *        Motherboard calls recalculateMatrix().
 *      - Per-channel real-time: Motherboard::processMPE*() follows the
 *        same pattern as processMPETimbre / processMPEChannelPressure.
 * 8. MPEMatrixEditor (src/gui/MPEMatrix.h) — add the item to the source
 *    combo box and update sourceToId() / idToSource().
 *
 * HOW TO ADD A NEW MODULATION TARGET
 * ------------------------------------
 * 1. VoiceMatrixAdjustments — add a float field (e.g. osc1PWOffset{0.f})
 *    and clear it in clear().
 * 2. VoiceMatrixRanges      — add a static constexpr float for the natural
 *    scale: +/-1 source maps to +/- that many native units.
 * 3. isValidMatrixTarget()  — add the SynthParam::ID string to the set.
 * 4. recalculateMatrix()    — add an else-if branch that accumulates
 *    contribution into the new field.
 * 5. Voice::ProcessSample() — consume the adjustment at the right point
 *    in the signal path, e.g.:
 *      foo = bar + matrixAdjustments.osc1PWOffset * VoiceMatrixRanges::osc1PWOffset;
 * 6. MPEMatrixEditor (src/gui/MPEMatrix.h) — add the item to the target
 *    combo box and update targetToId() / idToTarget().
 */

// ---------------------------------------------------------------------------
// Source enum — use string conversion for stable streaming (not int values)
// ---------------------------------------------------------------------------
enum class MatrixSource
{
    Strike,
    Lift,
    Press,
    Slide,
    Glide,
    None
};

inline std::string matrixSourceToString(MatrixSource src)
{
    switch (src)
    {
    case MatrixSource::Strike:
        return "Strike";
    case MatrixSource::Lift:
        return "Lift";
    case MatrixSource::Press:
        return "Press";
    case MatrixSource::Slide:
        return "Slide";
    case MatrixSource::Glide:
        return "Glide";
    case MatrixSource::None:
    default:
        return "None";
    }
}

inline MatrixSource matrixSourceFromString(const std::string &s)
{
    if (s == "Strike")
        return MatrixSource::Strike;
    if (s == "Lift")
        return MatrixSource::Lift;
    if (s == "Press")
        return MatrixSource::Press;
    if (s == "Slide")
        return MatrixSource::Slide;
    if (s == "Glide")
        return MatrixSource::Glide;
    return MatrixSource::None;
}

/*
 * VoiceMatrixAdjustments: per-voice modulation accumulator.
 * Contains one float per valid matrix target — keep this in sync with
 * isValidMatrixTarget() below.
 */
struct VoiceMatrixAdjustments
{
    // Keep in sync with isValidMatrixTarget()
    float filterCutoff{0.f};
    float filterResonance{0.f};
    float osc1Pitch{0.f};
    float osc2Pitch{0.f};
    float osc2Detune{0.f};
    float osc2PWOffset{0.f};
    float osc1Vol{0.f};
    float osc2Vol{0.f};
    float noiseVol{0.f};
    float ringModVol{0.f};
    float oscPitch{0.f}; // both Osc1 and Osc2 pitch together
    float unisonDetune{0.f};
    float oscPW{0.f}; // shared pulse width
    float crossmod{0.f};
    float lfo1Mod1{0.f};
    float lfo1Mod2{0.f};
    float lfo2Rate{0.f};
    float lfo2Mod1{0.f};
    float lfo2Mod2{0.f};
    float filterEnvAttack{0.f};
    float filterEnvRelease{0.f};
    float ampEnvAttack{0.f};
    float ampEnvRelease{0.f};

    void clear()
    {
        filterCutoff = 0.f;
        filterResonance = 0.f;
        osc1Pitch = 0.f;
        osc2Pitch = 0.f;
        osc2Detune = 0.f;
        osc2PWOffset = 0.f;
        osc1Vol = 0.f;
        osc2Vol = 0.f;
        noiseVol = 0.f;
        ringModVol = 0.f;
        oscPitch = 0.f;
        unisonDetune = 0.f;
        oscPW = 0.f;
        crossmod = 0.f;
        lfo1Mod1 = 0.f;
        lfo1Mod2 = 0.f;
        lfo2Rate = 0.f;
        lfo2Mod1 = 0.f;
        lfo2Mod2 = 0.f;
        filterEnvAttack = 0.f;
        filterEnvRelease = 0.f;
        ampEnvAttack = 0.f;
        ampEnvRelease = 0.f;
    }
};

/*
 * VoiceMatrixRanges: natural scale for each modulation target.
 * +/-1 source value maps to +/- the range value in the target's native units.
 * Keep in sync with VoiceMatrixAdjustments above and isValidMatrixTarget() below.
 */
struct VoiceMatrixRanges
{
    // Pitch targets: +/-1 = +/-48 semitones
    static constexpr float filterCutoff{96.f};
    static constexpr float osc1Pitch{48.f};
    static constexpr float osc2Pitch{48.f};
    // All other targets: +/-1 = +/-1 (full range)
    static constexpr float filterResonance{1.f};
    static constexpr float osc2Detune{1.f};
    static constexpr float osc2PWOffset{1.f};
    static constexpr float osc1Vol{1.f};
    static constexpr float osc2Vol{1.f};
    static constexpr float noiseVol{1.f};
    static constexpr float ringModVol{1.f};
    // Both-osc pitch: same scale as individual pitches
    static constexpr float oscPitch{48.f};
    // Unison detune: +/-1 = +/-1 in log-scaled unison-detune space
    static constexpr float unisonDetune{1.f};
    // Osc PW: +/-1 = +/-0.95 (full PW range)
    static constexpr float oscPW{0.95f};
    // Crossmod: +/-1 = +/-48 (matching processCrossmod scale)
    static constexpr float crossmod{48.f};
    // LFO mod amounts: +/-1 = full processed range
    static constexpr float lfo1Mod1{60.f};  // amt1 max after logsc chain
    static constexpr float lfo1Mod2{0.7f};  // amt2 max (linsc 0..0.7)
    static constexpr float lfo2Rate{250.f}; // Hz, full LFO2 rate range
    static constexpr float lfo2Mod1{60.f};
    static constexpr float lfo2Mod2{0.7f};
    // Envelope times: +/-1 = +/-10 seconds (ms)
    static constexpr float filterEnvAttack{20000.f};
    static constexpr float filterEnvRelease{20000.f};
    static constexpr float ampEnvAttack{20000.f};
    static constexpr float ampEnvRelease{20000.f};
};

// ---------------------------------------------------------------------------
// Valid modulation targets — keep in sync with VoiceMatrixAdjustments above
// ---------------------------------------------------------------------------
inline bool isValidMatrixTarget(const std::string &tgt)
{
    static const std::unordered_set<std::string> validTargets = {
        SynthParam::ID::FilterCutoff,
        SynthParam::ID::FilterResonance,
        SynthParam::ID::Osc1Pitch,
        SynthParam::ID::Osc2Pitch,
        SynthParam::ID::Osc2Detune,
        SynthParam::ID::Osc2PWOffset,
        SynthParam::ID::Osc1Vol,
        SynthParam::ID::Osc2Vol,
        SynthParam::ID::NoiseVol,
        SynthParam::ID::RingModVol,
        SynthParam::ID::OscPitch,
        SynthParam::ID::UnisonDetune,
        SynthParam::ID::OscPW,
        SynthParam::ID::OscCrossmod,
        SynthParam::ID::LFO1ModAmount1,
        SynthParam::ID::LFO1ModAmount2,
        SynthParam::ID::LFO2Rate,
        SynthParam::ID::LFO2ModAmount1,
        SynthParam::ID::LFO2ModAmount2,
        SynthParam::ID::FilterEnvAttack,
        SynthParam::ID::FilterEnvRelease,
        SynthParam::ID::AmpEnvAttack,
        SynthParam::ID::AmpEnvRelease,
    };
    return validTargets.count(tgt) > 0;
}

// ---------------------------------------------------------------------------
// A single matrix row
// ---------------------------------------------------------------------------
struct MatrixRow
{
    MatrixSource source{MatrixSource::None};
    std::string target{}; // SynthParam::ID string, empty = unassigned
    float depth{0.f};     // -1..1

    bool isActive() const { return source != MatrixSource::None && !target.empty(); }
};

// ---------------------------------------------------------------------------
// The matrix itself — lives on the synth, streams in the patch XML
// ---------------------------------------------------------------------------
struct VoiceMatrix
{
    std::array<MatrixRow, NUM_MATRIX_ROWS> rows{};

    /* Returns false if source/target are invalid or idx is out of range */
    bool setModulation(const std::string &src, const std::string &tgt, float depth, int idx)
    {
        if (idx < 0 || idx >= NUM_MATRIX_ROWS)
            return false;

        auto s = matrixSourceFromString(src);
        if (s == MatrixSource::None)
            return false;

        if (!isValidMatrixTarget(tgt))
            return false;

        rows[idx].source = s;
        rows[idx].target = tgt;
        rows[idx].depth = depth;
        return true;
    }

    void clearRow(int idx)
    {
        if (idx >= 0 && idx < NUM_MATRIX_ROWS)
        {
            rows[idx] = MatrixRow{};
        }
    }

    void clear() { rows.fill(MatrixRow{}); }

    // -----------------------------------------------------------------------
    // XML streaming — call toElement / fromElement from patch save/load
    // -----------------------------------------------------------------------
    std::unique_ptr<juce::XmlElement> toElement() const
    {
        auto el = std::make_unique<juce::XmlElement>("VoiceMatrix");

        for (int i = 0; i < NUM_MATRIX_ROWS; ++i)
        {
            const auto &row = rows[i];

            if (!row.isActive())
            {
                continue;
            }

            auto *rowEl = new juce::XmlElement("row");

            rowEl->setAttribute("idx", i);
            rowEl->setAttribute("source", matrixSourceToString(row.source));
            rowEl->setAttribute("target", row.target);
            rowEl->setAttribute("depth", row.depth);

            el->addChildElement(rowEl);
        }
        return el;
    }

    void fromElement(const juce::XmlElement *el)
    {
        rows.fill(MatrixRow{});

        if (el)
        {
            for (auto *rowEl : el->getChildIterator())
            {
                int idx = rowEl->getIntAttribute("idx", -1);

                if (idx < 0 || idx >= NUM_MATRIX_ROWS)
                {
                    continue;
                }

                rows[idx].source =
                    matrixSourceFromString(rowEl->getStringAttribute("source").toStdString());
                rows[idx].target = rowEl->getStringAttribute("target").toStdString();
                rows[idx].depth = static_cast<float>(rowEl->getDoubleAttribute("depth", 0.0));
            }
        }
    }
};

/*
 * VoiceMatrixSourceValues: stores the last received value for each matrix source,
 * normalised to -1..1. Lives on each Voice so recalculateMatrix can recompute
 * adjustments from scratch without accumulation.
 */
struct VoiceMatrixSourceValues
{
    float voiceBend{0.f};
    float channelPressure{0.f};
    float timbre{0.f};
    float velocity{0.f};
    float releaseVelocity{0.f};

    void clear()
    {
        voiceBend = 0.f;
        channelPressure = 0.f;
        timbre = 0.f;
        velocity = 0.f;
        releaseVelocity = 0.f;
    }

    float get(MatrixSource src) const
    {
        switch (src)
        {
        case MatrixSource::Glide:
            return voiceBend;
        case MatrixSource::Press:
            return channelPressure;
        case MatrixSource::Slide:
            return timbre;
        case MatrixSource::Strike:
            return velocity;
        case MatrixSource::Lift:
            return releaseVelocity;
        default:
            return 0.f;
        }
    }

    void set(MatrixSource src, float value)
    {
        switch (src)
        {
        case MatrixSource::Glide:
            voiceBend = value;
            break;
        case MatrixSource::Press:
            channelPressure = value;
            break;
        case MatrixSource::Slide:
            timbre = value;
            break;
        case MatrixSource::Strike:
            velocity = value;
            break;
        case MatrixSource::Lift:
            releaseVelocity = value;
            break;
            break;
        default:
            break;
        }
    }
};

// ---------------------------------------------------------------------------
// setMatrixTarget: store a source value on a voice, then call recalculateMatrix.
// value should be normalised to -1..1 before calling.
// ---------------------------------------------------------------------------
inline void setMatrixSource(VoiceMatrixSourceValues &srcVals, MatrixSource src, float value)
{
    srcVals.set(src, value);
}

// ---------------------------------------------------------------------------
// recalculateMatrix: zero adjustments and recompute from stored source values.
// Call after any source value changes to avoid accumulation.
// ---------------------------------------------------------------------------
inline void recalculateMatrix(const VoiceMatrix &matrix, const VoiceMatrixSourceValues &srcVals,
                              VoiceMatrixAdjustments &adj)
{
    adj.clear();

    for (const auto &row : matrix.rows)
    {
        if (!row.isActive())
        {
            continue;
        }

        const float contribution = srcVals.get(row.source) * row.depth;

        if (row.target == SynthParam::ID::FilterCutoff)
            adj.filterCutoff += contribution;
        else if (row.target == SynthParam::ID::FilterResonance)
            adj.filterResonance += contribution;
        else if (row.target == SynthParam::ID::Osc1Pitch)
            adj.osc1Pitch += contribution;
        else if (row.target == SynthParam::ID::Osc2Pitch)
            adj.osc2Pitch += contribution;
        else if (row.target == SynthParam::ID::Osc2Detune)
            adj.osc2Detune += contribution;
        else if (row.target == SynthParam::ID::Osc2PWOffset)
            adj.osc2PWOffset += contribution;
        else if (row.target == SynthParam::ID::Osc1Vol)
            adj.osc1Vol += contribution;
        else if (row.target == SynthParam::ID::Osc2Vol)
            adj.osc2Vol += contribution;
        else if (row.target == SynthParam::ID::NoiseVol)
            adj.noiseVol += contribution;
        else if (row.target == SynthParam::ID::RingModVol)
            adj.ringModVol += contribution;
        else if (row.target == SynthParam::ID::OscPitch)
            adj.oscPitch += contribution;
        else if (row.target == SynthParam::ID::UnisonDetune)
            adj.unisonDetune += contribution;
        else if (row.target == SynthParam::ID::OscPW)
            adj.oscPW += contribution;
        else if (row.target == SynthParam::ID::OscCrossmod)
            adj.crossmod += contribution;
        else if (row.target == SynthParam::ID::LFO1ModAmount1)
            adj.lfo1Mod1 += contribution;
        else if (row.target == SynthParam::ID::LFO1ModAmount2)
            adj.lfo1Mod2 += contribution;
        else if (row.target == SynthParam::ID::LFO2Rate)
            adj.lfo2Rate += contribution;
        else if (row.target == SynthParam::ID::LFO2ModAmount1)
            adj.lfo2Mod1 += contribution;
        else if (row.target == SynthParam::ID::LFO2ModAmount2)
            adj.lfo2Mod2 += contribution;
        else if (row.target == SynthParam::ID::FilterEnvAttack)
            adj.filterEnvAttack += contribution;
        else if (row.target == SynthParam::ID::FilterEnvRelease)
            adj.filterEnvRelease += contribution;
        else if (row.target == SynthParam::ID::AmpEnvAttack)
            adj.ampEnvAttack += contribution;
        else if (row.target == SynthParam::ID::AmpEnvRelease)
            adj.ampEnvRelease += contribution;
    }
}

// ---------------------------------------------------------------------------
// Thread-safe UI→audio FIFO for matrix row updates
// ---------------------------------------------------------------------------
struct MatrixRowUpdate
{
    int index{-1};
    MatrixRow row{};
};

/*
 * MatrixUpdateFifo: single-producer (UI), single-consumer (audio) FIFO for
 * pushing row edits from the message thread to processBlock.
 */
template <int Capacity> class MatrixUpdateFifo
{
  public:
    MatrixUpdateFifo() : abstractFifo(Capacity) {}

    bool push(int index, const MatrixRow &row)
    {
        if (abstractFifo.getFreeSpace() == 0)
            return false;
        auto scope = abstractFifo.write(1);
        if (scope.blockSize1 > 0)
            buffer[scope.startIndex1] = {index, row};
        else if (scope.blockSize2 > 0)
            buffer[scope.startIndex2] = {index, row};
        return true;
    }

    bool hasElement() const { return abstractFifo.getNumReady() > 0; }

    /* Call only after hasElement() returns true */
    MatrixRowUpdate pop()
    {
        auto scope = abstractFifo.read(1);
        if (scope.blockSize1 > 0)
            return buffer[scope.startIndex1];
        return buffer[scope.startIndex2];
    }

  private:
    juce::AbstractFifo abstractFifo;
    std::array<MatrixRowUpdate, Capacity> buffer{};

    JUCE_DECLARE_NON_COPYABLE(MatrixUpdateFifo)
    JUCE_DECLARE_NON_MOVEABLE(MatrixUpdateFifo)
};

#endif // OBXF_SRC_ENGINE_VOICEMATRIX_H
