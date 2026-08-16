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

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>

#include <juce_core/juce_core.h>

#include "configuration.h"
#include "ParamScales.h"
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
 * 1. MatrixTarget enum      — add your target, and update matrixTargetToString() and
 *    matrixTargetFromString() as well.
 * 2. matrixTargetScaling()  — add a case giving the same normalized-to-native curve
 *    that SynthEngine::process* applies to the matching parameter, plus its native
 *    min/max. Leave clampToRange true unless the target is linear and wants to swing
 *    outside its own range (see the pitch targets).
 * 3. isValidMatrixTarget()  — add the SynthParam::ID string to the set.
 * 4. SynthEngine::process*  — mirror the normalized value into the bases with
 *    setMatrixBase(MatrixTarget::Foo, val), so the matrix modulates the patch value.
 * 5. Voice::ProcessSample() — consume it. Almost always this is just
 *      foo = matrixAdjustments.nativeOr(MatrixTarget::Foo, foo);
 *    which leaves unmodulated voices untouched. Targets injected into a modulation
 *    bus rather than read from a parameter field (pitch, and anything else that is
 *    unclamped) instead add a native delta:
 *      bus += matrixAdjustments.modFor(MatrixTarget::Foo) *
 *             matrixTargetScaling(MatrixTarget::Foo).span();
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
    Count // sentinel; not a target. Enum values are never streamed, strings are.
};

inline constexpr size_t MatrixTargetCount{static_cast<size_t>(MatrixTarget::Count)};

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
 * MatrixTargetScaling: how a target converts normalized (0..1) parameter values to
 * native units. These curves must match the ones SynthEngine::process* applies to the
 * matching parameter — that is the whole point: a depth of 1.0 moves the target by the
 * full width of its own range, following its own curve, rather than adding a fixed
 * number of native units on top of an already-scaled value.
 *
 * clampToRange false means modulation is allowed to push the target outside its natural
 * range. That is only meaningful for linear curves, where the scaling extrapolates
 * sensibly; it exists for the pitch targets, whose musical zero is 0 semitones rather
 * than the parameter minimum, so that a bipolar source swings +/- the full range.
 */
struct MatrixTargetScaling
{
    float (*toNative)(float norm){nullptr};
    float nativeMin{0.f};
    float nativeMax{1.f};
    bool clampToRange{true};

    constexpr float span() const { return nativeMax - nativeMin; }

    float apply(float norm) const
    {
        return toNative(clampToRange ? std::clamp(norm, 0.f, 1.f) : norm);
    }
};

/*
 * Keep each entry's curve identical to the corresponding SynthEngine::process* call.
 * FilterCutoff and FilterResonance are listed for completeness, but their base value is
 * smoothed per sample rather than stored in VoiceMatrixBases, so recalculateMatrix does
 * not precompute a native value for them — see the notes there.
 */
inline constexpr MatrixTargetScaling matrixTargetScaling(MatrixTarget t)
{
    switch (t)
    {
    case MatrixTarget::FilterCutoff:
        return {+[](float v) { return linsc(v, 0.f, 120.f); }, 0.f, 120.f, true};
    case MatrixTarget::FilterResonance:
        return {+[](float v) { return 0.991f - logsc(1.f - v, 0.f, 0.991f, 40.f); }, 0.f, 0.991f,
                true};
    // Pitch is bipolar around the patch value, so it is deliberately not clamped
    case MatrixTarget::Osc1Pitch:
    case MatrixTarget::Osc2Pitch:
    case MatrixTarget::OscPitch:
        return {+[](float v) { return v * 48.f; }, 0.f, 48.f, false};
    case MatrixTarget::Osc2Detune:
        return {+[](float v) { return logsc(v, 0.001f, 0.6f); }, 0.001f, 0.6f, true};
    case MatrixTarget::Osc2PWOffset:
        return {+[](float v) { return linsc(v, 0.f, 0.95f); }, 0.f, 0.95f, true};
    case MatrixTarget::Osc1Vol:
    case MatrixTarget::Osc2Vol:
    case MatrixTarget::NoiseVol:
    case MatrixTarget::RingModVol:
        return {+[](float v) { return v; }, 0.f, 1.f, true};
    case MatrixTarget::UnisonDetune:
        return {+[](float v) { return logsc(v, 0.001f, 1.f); }, 0.001f, 1.f, true};
    case MatrixTarget::OscPW:
        return {+[](float v) { return linsc(v, 0.f, 0.95f); }, 0.f, 0.95f, true};
    case MatrixTarget::OscCrossmod:
        return {+[](float v) { return v * 48.f; }, 0.f, 48.f, true};
    case MatrixTarget::LFO1ModAmount1:
    case MatrixTarget::LFO2ModAmount1:
        return {+[](float v) { return logsc(logsc(v, 0.f, 1.f, 60.f), 0.f, 60.f, 10.f); }, 0.f,
                60.f, true};
    case MatrixTarget::LFO1ModAmount2:
    case MatrixTarget::LFO2ModAmount2:
        return {+[](float v) { return linsc(v, 0.f, 0.7f); }, 0.f, 0.7f, true};
    case MatrixTarget::LFO2Rate:
        return {+[](float v) { return logsc(v, 0.f, 250.f, 3775.f); }, 0.f, 250.f, true};
    case MatrixTarget::FilterEnvAttack:
    case MatrixTarget::FilterEnvRelease:
        return {+[](float v) { return logsc(v, 1.f, 60000.f, 900.f); }, 1.f, 60000.f, true};
    case MatrixTarget::AmpEnvAttack:
        return {+[](float v) { return logsc(v, 4.f, 60000.f, 900.f); }, 4.f, 60000.f, true};
    case MatrixTarget::AmpEnvRelease:
        return {+[](float v) { return logsc(v, 8.f, 60000.f, 900.f); }, 8.f, 60000.f, true};
    case MatrixTarget::None:
    case MatrixTarget::Count:
    default:
        return {+[](float v) { return v; }, 0.f, 1.f, true};
    }
}

/*
 * VoiceMatrixBases: normalized (0..1) parameter value per target, mirrored from the
 * parameters by SynthEngine::process*. Global rather than per-voice, since these are the
 * patch values the per-voice modulation is applied on top of.
 *
 * FilterCutoff and FilterResonance entries are unused — those two are smoothed per
 * sample, so their base only exists inside the smoother.
 */
struct VoiceMatrixBases
{
    std::array<float, MatrixTargetCount> value{};

    void set(MatrixTarget t, float norm) { value[static_cast<size_t>(t)] = norm; }
    float get(MatrixTarget t) const { return value[static_cast<size_t>(t)]; }
};

/*
 * VoiceMatrixAdjustments: per-voice modulation result.
 *
 * mod[] is the raw normalized sum of (source * depth) over the active rows. It is what
 * the two smoothed targets use, and what unclamped linear targets (pitch) use to form a
 * native-space delta at their injection point.
 *
 * native[]/active[] carry the final native value for every other target, computed once
 * per source change in recalculateMatrix rather than per sample. When a target is not
 * active, the voice keeps using the engine's own unmodulated value, so voices with no
 * modulation are bit-identical to the unmodulated path.
 */
struct VoiceMatrixAdjustments
{
    std::array<float, MatrixTargetCount> mod{};
    std::array<float, MatrixTargetCount> native{};
    std::array<bool, MatrixTargetCount> active{};

    void clear()
    {
        mod.fill(0.f);
        native.fill(0.f);
        active.fill(false);
    }

    float modFor(MatrixTarget t) const { return mod[static_cast<size_t>(t)]; }
    bool isActive(MatrixTarget t) const { return active[static_cast<size_t>(t)]; }

    /* Final native value if this target is modulated on this voice, else the caller's
     * own unmodulated value. */
    float nativeOr(MatrixTarget t, float unmodulated) const
    {
        const auto i = static_cast<size_t>(t);
        return active[i] ? native[i] : unmodulated;
    }
};

using namespace SynthParam;

// ---------------------------------------------------------------------------
// Valid modulation targets — keep in sync with the MatrixTarget enum above
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
// Call after any source value changes, and after any base value changes, to avoid
// accumulation. Runs at event rate (note on/off, MPE message, parameter change) and
// never per sample, which is why it is the right place to evaluate the scaling curves.
// ---------------------------------------------------------------------------
inline void recalculateMatrix(const VoiceMatrix &matrix, const VoiceMatrixBases &bases,
                              const VoiceMatrixSourceValues &srcVals, VoiceMatrixAdjustments &adj)
{
    adj.clear();

    bool anyActive{false};

    for (const auto &row : matrix.rows)
    {
        if (!row.isActive())
        {
            continue;
        }

        adj.mod[static_cast<size_t>(row.target)] += srcVals.get(row.source) * row.depth;
        anyActive = true;
    }

    if (!anyActive)
    {
        return;
    }

    // OscPitch is a pseudo-target: it drives both oscillators alongside their own rows
    const float bothPitch = adj.modFor(MatrixTarget::OscPitch);

    for (size_t i = 1; i < MatrixTargetCount; ++i) // 0 is None
    {
        const auto t = static_cast<MatrixTarget>(i);

        /* Cutoff and resonance have no stored base — it lives in a per-sample smoother —
         * so they are applied from mod[] at their consumption site instead. Leaving
         * active[] false here keeps a stray nativeOr() call safe. */
        if (t == MatrixTarget::FilterCutoff || t == MatrixTarget::FilterResonance)
        {
            continue;
        }

        float m = adj.mod[i];

        if (t == MatrixTarget::Osc1Pitch || t == MatrixTarget::Osc2Pitch)
        {
            m += bothPitch;
        }

        if (m == 0.f)
        {
            continue;
        }

        adj.active[i] = true;
        adj.native[i] = matrixTargetScaling(t).apply(bases.get(t) + m);
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
