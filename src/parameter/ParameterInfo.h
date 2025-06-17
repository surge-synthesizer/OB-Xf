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

#ifndef OBXF_SRC_PARAMETER_PARAMETERINFO_H
#define OBXF_SRC_PARAMETER_PARAMETERINFO_H

#include <optional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <sst/basic-blocks/params/ParamMetadata.h>

// Original Code
struct ParameterInfo
{
    enum Type : uint32_t
    {
        Float = 0,
        Choice,
        Bool
    };

    using pmd = sst::basic_blocks::params::ParamMetaData;
    ParameterInfo(const juce::String &_ID, const pmd &_meta) : ID(_ID), meta(_meta)
    {
    }

    // Float ctor
    ParameterInfo(const juce::String &_ID, const juce::String &_name, const juce::String &_unit,
                  float _def, float _min, float _max, float _inc, float _skw)
        : ID{_ID}, name{_name}, unit{_unit}, steps{}, type{Float}, def{_def}, min{_min}, max{_max},
          inc{_inc}, skw{_skw}
    {
        jassert(ID.isNotEmpty());
        jassert(min < max);
        // use std::cout to debug
        // if (!(_def <= _max && _def >= _min)) {
        //     std::cout << "Parameter assertion failure for: " << _ID << std::endl;
        //     std::cout << "def: " << _def << ", min: " << _min << ", max: " << _max << std::endl;
        // }

        jassert(def <= max && def >= min);
        jassert(inc >= 0.f);
        jassert(skw > 0.f);
    }

    // Choice ctor
    ParameterInfo(const juce::String &_ID, const juce::String &_name,
                  const juce::StringArray &_steps, unsigned int _def)
        : ID{_ID}, name{_name}, unit{}, steps{_steps}, type{Choice}, def{static_cast<float>(_def)},
          min{0.f}, max{static_cast<float>(_steps.size() - 1)}, inc{1.f}, skw{1.f}
    {
        jassert(ID.isNotEmpty());
        jassert(min < max);
        jassert(def <= max && def >= min);
        jassert(steps.size() == 0 || steps.size() == static_cast<int>(max + 1.f));
    }

    // Bool ctor
    ParameterInfo(const juce::String &_ID, const juce::String &_name,
                  const juce::String &offStepName, const juce::String &onStepName, bool _def)
        : ID{_ID}, name{_name}, unit{}, steps{offStepName, onStepName}, type{Bool},
          def{static_cast<float>(_def)}, min{0.f}, max{1.f}, inc{1.f}, skw{1.f}
    {
        jassert(ID.isNotEmpty());
    }

    // Generic ctor
    ParameterInfo(const juce::String &_ID, const juce::String &_name, const juce::String &_unit,
                  const juce::String &_steps, Type _type, float _def, float _min, float _max,
                  float _inc, float _skw)
        : ID{_ID}, name{_name}, unit{_unit}, steps{_steps}, type{_type}, def{_def}, min{_min},
          max{_max}, inc{_inc}, skw{_skw}
    {
        jassert(ID.isNotEmpty());
        jassert(min < max);
        jassert(def <= max && def >= min);
        jassert(inc >= 0.f);
        jassert(skw > 0.f);
    }

    // Copy and move ctor
    ParameterInfo(const ParameterInfo &other) = default;
    ParameterInfo(ParameterInfo &&other) = default;

    // No default ctor
    ParameterInfo() = delete;

    const ParameterInfo &operator=(const ParameterInfo &) = delete;

    const ParameterInfo &operator=(ParameterInfo &) = delete;

    ~ParameterInfo() noexcept = default;

    const juce::String ID{};
    const std::optional<pmd> meta{std::nullopt};

    const juce::String name{};
    const juce::String unit{};
    const juce::StringArray steps{};

    const Type type{Float};

    const float def{0};
    const float min{0};
    const float max{1};
    const float inc{0};
    const float skw{0};
};

#endif // OBXF_SRC_PARAMETER_PARAMETERINFO_H
