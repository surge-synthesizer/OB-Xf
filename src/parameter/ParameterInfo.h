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
    ParameterInfo(const juce::String &_ID, const pmd &_meta) : ID(_ID), meta(_meta) {}

    // Copy and move ctor
    ParameterInfo(const ParameterInfo &other) = default;
    ParameterInfo(ParameterInfo &&other) = default;

    // No default ctor
    ParameterInfo() = delete;

    const ParameterInfo &operator=(const ParameterInfo &) = delete;

    const ParameterInfo &operator=(ParameterInfo &) = delete;

    ~ParameterInfo() noexcept = default;

    const juce::String ID{};
    const pmd meta{};
};

#endif // OBXF_SRC_PARAMETER_PARAMETERINFO_H
