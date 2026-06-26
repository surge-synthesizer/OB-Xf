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
#include <unordered_map>

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
 *
 * HOW TO ADD A NEW MODULATION TARGET
 * ------------------------------------
 * 1. VoiceMatrixAdjustments — add a float field (e.g. osc1PWOffset{0.f})
 *    and clear it in clear().
 * 2. VoiceMatrixRanges      — add a static constexpr float for the natural
 *    scale: +/-1 source maps to +/- that many native units.
 * 3. MatrixTargets enum  — add your target here, update matrixTargetToString() and
 *    matrixTargetFromString() methods as well.
 * 4. isValidMatrixTarget()  — add the SynthParam::ID string to the set.
 * 5. recalculateMatrix()    — add an else-if branch that accumulates
 *    contribution into the new field.
 * 6. Voice::ProcessSample() — consume the adjustment at the right point
 *    in the signal path, e.g.:
 *      foo = bar + matrixAdjustments.osc1PWOffset * VoiceMatrixRanges::osc1PWOffset;
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

using namespace SynthParam;

enum class MatrixTarget
{
    None,
    FilterCutoff,
    FilterResonance,
    Osc1Pitch,
    Osc2Pitch,
    Osc2Detune,
    Osc2PWOffset,
    Osc1Vol,
    Osc2Vol,
    NoiseVol,
    RingModVol,
    OscPitch,
    UnisonDetune,
    OscPW,
    OscCrossmod,
    LFO1ModAmount1,
    LFO1ModAmount2,
    LFO2Rate,
    LFO2ModAmount1,
    LFO2ModAmount2,
    FilterEnvAttack,
    FilterEnvRelease,
    AmpEnvAttack,
    AmpEnvRelease,
};

inline std::string matrixTargetToString(MatrixTarget target)
{
    switch (target)
    {
    case MatrixTarget::FilterCutoff:
        return ID::FilterCutoff;
    case MatrixTarget::FilterResonance:
        return ID::FilterResonance;
    case MatrixTarget::Osc1Pitch:
        return ID::Osc1Pitch;
    case MatrixTarget::Osc2Pitch:
        return ID::Osc2Pitch;
    case MatrixTarget::Osc2Detune:
        return ID::Osc2Detune;
    case MatrixTarget::Osc2PWOffset:
        return ID::Osc2PWOffset;
    case MatrixTarget::Osc1Vol:
        return ID::Osc1Vol;
    case MatrixTarget::Osc2Vol:
        return ID::Osc2Vol;
    case MatrixTarget::NoiseVol:
        return ID::NoiseVol;
    case MatrixTarget::RingModVol:
        return ID::RingModVol;
    case MatrixTarget::OscPitch:
        return ID::OscPitch;
    case MatrixTarget::UnisonDetune:
        return ID::UnisonDetune;
    case MatrixTarget::OscPW:
        return ID::OscPW;
    case MatrixTarget::OscCrossmod:
        return ID::OscCrossmod;
    case MatrixTarget::LFO1ModAmount1:
        return ID::LFO1ModAmount1;
    case MatrixTarget::LFO1ModAmount2:
        return ID::LFO1ModAmount2;
    case MatrixTarget::LFO2Rate:
        return ID::LFO2Rate;
    case MatrixTarget::LFO2ModAmount1:
        return ID::LFO2ModAmount1;
    case MatrixTarget::LFO2ModAmount2:
        return ID::LFO2ModAmount2;
    case MatrixTarget::FilterEnvAttack:
        return ID::FilterEnvAttack;
    case MatrixTarget::FilterEnvRelease:
        return ID::FilterEnvRelease;
    case MatrixTarget::AmpEnvAttack:
        return ID::AmpEnvAttack;
    case MatrixTarget::AmpEnvRelease:
        return ID::AmpEnvRelease;
    case MatrixTarget::None:
    default:
        return {};
    }
}

inline MatrixTarget matrixTargetFromString(const std::string &s)
{
    if (s == ID::FilterCutoff)
        return MatrixTarget::FilterCutoff;
    if (s == ID::FilterResonance)
        return MatrixTarget::FilterResonance;
    if (s == ID::Osc1Pitch)
        return MatrixTarget::Osc1Pitch;
    if (s == ID::Osc2Pitch)
        return MatrixTarget::Osc2Pitch;
    if (s == ID::Osc2Detune)
        return MatrixTarget::Osc2Detune;
    if (s == ID::Osc2PWOffset)
        return MatrixTarget::Osc2PWOffset;
    if (s == ID::Osc1Vol)
        return MatrixTarget::Osc1Vol;
    if (s == ID::Osc2Vol)
        return MatrixTarget::Osc2Vol;
    if (s == ID::NoiseVol)
        return MatrixTarget::NoiseVol;
    if (s == ID::RingModVol)
        return MatrixTarget::RingModVol;
    if (s == ID::OscPitch)
        return MatrixTarget::OscPitch;
    if (s == ID::UnisonDetune)
        return MatrixTarget::UnisonDetune;
    if (s == ID::OscPW)
        return MatrixTarget::OscPW;
    if (s == ID::OscCrossmod)
        return MatrixTarget::OscCrossmod;
    if (s == ID::LFO1ModAmount1)
        return MatrixTarget::LFO1ModAmount1;
    if (s == ID::LFO1ModAmount2)
        return MatrixTarget::LFO1ModAmount2;
    if (s == ID::LFO2Rate)
        return MatrixTarget::LFO2Rate;
    if (s == ID::LFO2ModAmount1)
        return MatrixTarget::LFO2ModAmount1;
    if (s == ID::LFO2ModAmount2)
        return MatrixTarget::LFO2ModAmount2;
    if (s == ID::FilterEnvAttack)
        return MatrixTarget::FilterEnvAttack;
    if (s == ID::FilterEnvRelease)
        return MatrixTarget::FilterEnvRelease;
    if (s == ID::AmpEnvAttack)
        return MatrixTarget::AmpEnvAttack;
    if (s == ID::AmpEnvRelease)
        return MatrixTarget::AmpEnvRelease;
    return MatrixTarget::None;
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

using namespace SynthParam;

// ---------------------------------------------------------------------------
// Valid modulation targets — keep in sync with VoiceMatrixAdjustments above
// ---------------------------------------------------------------------------
inline bool isValidMatrixTarget(const std::string &tgt)
{
    static const std::unordered_set<std::string> validTargets = {
        ID::FilterCutoff,   ID::FilterResonance, ID::Osc1Pitch,
        ID::Osc2Pitch,      ID::Osc2Detune,      ID::Osc2PWOffset,
        ID::Osc1Vol,        ID::Osc2Vol,         ID::NoiseVol,
        ID::RingModVol,     ID::OscPitch,        ID::UnisonDetune,
        ID::OscPW,          ID::OscCrossmod,     ID::LFO1ModAmount1,
        ID::LFO1ModAmount2, ID::LFO2Rate,        ID::LFO2ModAmount1,
        ID::LFO2ModAmount2, ID::FilterEnvAttack, ID::FilterEnvRelease,
        ID::AmpEnvAttack,   ID::AmpEnvRelease,
    };

    return validTargets.count(tgt) > 0;
}

// ---------------------------------------------------------------------------
// MPE matrix menu helpers — canonical target list and index<->ID conversion.
// Index 0 is always "None". Strike/Lift get extra envelope targets appended.
// ---------------------------------------------------------------------------
inline const std::vector<std::string> &matrixCommonTargets()
{
    static const std::vector<std::string> targets = {
        Name::OscPitch,     Name::Osc1Pitch,       Name::Osc2Pitch,      Name::Osc2Detune,
        Name::UnisonDetune, Name::OscPW,           Name::Osc2PWOffset,   Name::OscCrossmod,
        Name::Osc1Vol,      Name::Osc2Vol,         Name::RingModVol,     Name::NoiseVol,
        Name::FilterCutoff, Name::FilterResonance, Name::LFO1ModAmount1, Name::LFO1ModAmount2,
        Name::LFO2Rate,     Name::LFO2ModAmount1,  Name::LFO2ModAmount2,
    };
    return targets;
}

inline const std::vector<std::string> &matrixExtraTargets(MatrixSource src)
{
    static const std::vector<std::string> strikeExtras = {
        Name::FilterEnvAttack,
        Name::AmpEnvAttack,
    };

    static const std::vector<std::string> liftExtras = {
        Name::FilterEnvRelease,
        Name::AmpEnvRelease,
    };

    static const std::vector<std::string> empty;

    switch (src)
    {
    case MatrixSource::Strike:
        return strikeExtras;
    case MatrixSource::Lift:
        return liftExtras;
    default:
        return empty;
    }
}

inline MatrixTarget matrixTargetNameToEnum(const std::string &name)
{
    // clang-format off
    static const std::unordered_map<std::string, MatrixTarget> nameToEnum = {
        {Name::OscPitch,         MatrixTarget::OscPitch        },
        {Name::Osc1Pitch,        MatrixTarget::Osc1Pitch       },
        {Name::Osc2Pitch,        MatrixTarget::Osc2Pitch       },
        {Name::Osc2Detune,       MatrixTarget::Osc2Detune      },
        {Name::UnisonDetune,     MatrixTarget::UnisonDetune    },
        {Name::OscPW,            MatrixTarget::OscPW           },
        {Name::Osc2PWOffset,     MatrixTarget::Osc2PWOffset    },
        {Name::OscCrossmod,      MatrixTarget::OscCrossmod     },
        {Name::Osc1Vol,          MatrixTarget::Osc1Vol         },
        {Name::Osc2Vol,          MatrixTarget::Osc2Vol         },
        {Name::RingModVol,       MatrixTarget::RingModVol      },
        {Name::NoiseVol,         MatrixTarget::NoiseVol        },
        {Name::FilterCutoff,     MatrixTarget::FilterCutoff    },
        {Name::FilterResonance,  MatrixTarget::FilterResonance },
        {Name::LFO1ModAmount1,   MatrixTarget::LFO1ModAmount1  },
        {Name::LFO1ModAmount2,   MatrixTarget::LFO1ModAmount2  },
        {Name::LFO2Rate,         MatrixTarget::LFO2Rate        },
        {Name::LFO2ModAmount1,   MatrixTarget::LFO2ModAmount1  },
        {Name::LFO2ModAmount2,   MatrixTarget::LFO2ModAmount2  },
        {Name::FilterEnvAttack,  MatrixTarget::FilterEnvAttack },
        {Name::AmpEnvAttack,     MatrixTarget::AmpEnvAttack    },
        {Name::FilterEnvRelease, MatrixTarget::FilterEnvRelease},
        {Name::AmpEnvRelease,    MatrixTarget::AmpEnvRelease   },
    };
    // clang-format on

    auto it = nameToEnum.find(name);
    return it != nameToEnum.end() ? it->second : MatrixTarget::None;
}
// Returns 0 for None/unrecognised, 1..N for valid targets
inline int matrixTargetToMenuIndex(MatrixSource src, MatrixTarget target)
{
    if (target == MatrixTarget::None)
    {
        return 0;
    }

    const auto &common = matrixCommonTargets();

    for (int i = 0; i < (int)common.size(); ++i)
    {
        if (matrixTargetNameToEnum(common[i]) == target)
        {
            return i + 1; // +1 because index 0 is None
        }
    }

    const auto &extras = matrixExtraTargets(src);

    for (int i = 0; i < (int)extras.size(); ++i)
    {
        if (matrixTargetNameToEnum(extras[i]) == target)
        {
            return (int)common.size() + i + 1;
        }
    }

    return 0;
}

// Returns empty string for index 0 (None) or out of range
inline MatrixTarget matrixMenuIndexToTarget(MatrixSource src, int index)
{
    if (index <= 0)
    {
        return MatrixTarget::None;
    }

    const auto &common = matrixCommonTargets();

    if (index <= (int)common.size())
    {
        return matrixTargetNameToEnum(common[index - 1]);
    }

    const auto &extras = matrixExtraTargets(src);
    const int extraIndex = index - (int)common.size() - 1;

    if (extraIndex >= 0 && extraIndex < (int)extras.size())
    {
        return matrixTargetNameToEnum(extras[extraIndex]);
    }

    return MatrixTarget::None;
}

// ---------------------------------------------------------------------------
// A single matrix row
// ---------------------------------------------------------------------------
struct MatrixRow
{
    MatrixSource source{MatrixSource::None};
    MatrixTarget target{};
    float depth{0.f}; // -1..1

    bool isActive() const { return source != MatrixSource::None && target != MatrixTarget::None; }
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

        auto t = matrixTargetFromString(tgt);
        if (t == MatrixTarget::None)
            return false;

        rows[idx].source = s;
        rows[idx].target = t;
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
            rowEl->setAttribute("target", matrixTargetToString(row.target));
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
                rows[idx].target =
                    matrixTargetFromString(rowEl->getStringAttribute("target").toStdString());
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

        // clang-format off
        switch (row.target)
        {
        case MatrixTarget::FilterCutoff:    adj.filterCutoff    += contribution; break;
        case MatrixTarget::FilterResonance: adj.filterResonance += contribution; break;
        case MatrixTarget::Osc1Pitch:       adj.osc1Pitch       += contribution; break;
        case MatrixTarget::Osc2Pitch:       adj.osc2Pitch       += contribution; break;
        case MatrixTarget::Osc2Detune:      adj.osc2Detune      += contribution; break;
        case MatrixTarget::Osc2PWOffset:    adj.osc2PWOffset    += contribution; break;
        case MatrixTarget::Osc1Vol:         adj.osc1Vol         += contribution; break;
        case MatrixTarget::Osc2Vol:         adj.osc2Vol         += contribution; break;
        case MatrixTarget::NoiseVol:        adj.noiseVol        += contribution; break;
        case MatrixTarget::RingModVol:      adj.ringModVol      += contribution; break;
        case MatrixTarget::OscPitch:        adj.oscPitch        += contribution; break;
        case MatrixTarget::UnisonDetune:    adj.unisonDetune    += contribution; break;
        case MatrixTarget::OscPW:           adj.oscPW           += contribution; break;
        case MatrixTarget::OscCrossmod:     adj.crossmod        += contribution; break;
        case MatrixTarget::LFO1ModAmount1:  adj.lfo1Mod1        += contribution; break;
        case MatrixTarget::LFO1ModAmount2:  adj.lfo1Mod2        += contribution; break;
        case MatrixTarget::LFO2Rate:        adj.lfo2Rate        += contribution; break;
        case MatrixTarget::LFO2ModAmount1:  adj.lfo2Mod1        += contribution; break;
        case MatrixTarget::LFO2ModAmount2:  adj.lfo2Mod2        += contribution; break;
        case MatrixTarget::FilterEnvAttack: adj.filterEnvAttack += contribution; break;
        case MatrixTarget::FilterEnvRelease:adj.filterEnvRelease+= contribution; break;
        case MatrixTarget::AmpEnvAttack:    adj.ampEnvAttack    += contribution; break;
        case MatrixTarget::AmpEnvRelease:   adj.ampEnvRelease   += contribution; break;
        case MatrixTarget::None:
        default:                                                                 break;
        }
        // clang-format on
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
