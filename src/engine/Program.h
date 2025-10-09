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

#ifndef OBXF_SRC_ENGINE_PARAMETERS_H
#define OBXF_SRC_ENGINE_PARAMETERS_H

#include "ParameterList.h"

class Program
{
  public:
    Program() : namePtr(new juce::String(INIT_PATCH_NAME)) { setDefaultValues(); }

    // Copy constructor
    Program(const Program &other) : namePtr(new juce::String(*other.namePtr.load()))
    {
        for (const auto &kv : other.values)
        {
            values[kv.first].store(kv.second.load());
        }
    }

    // Assignment operator
    Program &operator=(const Program &other)
    {
        if (this != &other)
        {
            // Copy values
            values.clear();
            for (const auto &kv : other.values)
            {
                values[kv.first].store(kv.second.load());
            }

            // Copy name
            auto *newStr = new juce::String(*other.namePtr.load());
            const auto *oldStr = namePtr.exchange(newStr);
            delete oldStr;
        }
        return *this;
    }

    ~Program() { delete namePtr.load(); }

    void setDefaultValues()
    {
        values.clear();
        for (const auto &param : ParameterList)
        {
            values[param.ID] = param.meta.naturalToNormalized01(param.meta.defaultVal);
        }
    }

    float getValueById(const juce::String &id) const
    {
        auto it = values.find(id);
        return it != values.end() ? it->second.load() : 0.0f;
    }

    void setValueById(const juce::String &id, float v) { values[id].store(v); }

    void setName(const juce::String &newName)
    {
        auto *newStr = new juce::String(newName);
        const auto *oldStr = namePtr.exchange(newStr);
        delete oldStr;
    }

    juce::String getName() const
    {
        juce::String *ptr = namePtr.load();
        return ptr ? *ptr : juce::String();
    }

    std::unordered_map<juce::String, std::atomic<float>> values;
    std::atomic<juce::String *> namePtr;
};

#endif // OBXF_SRC_ENGINE_PARAMETERS_H