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
    Program()
    {
        namePtr.set(INIT_PATCH_NAME);
        setToDefaultPatch();
    }

    // Copy constructor
    Program(const Program &other)
        : namePtr(other.namePtr), authorPtr(other.authorPtr), licensePtr(other.licensePtr)
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
            namePtr.set(other.namePtr.get());
            authorPtr.set(other.authorPtr.get());
            licensePtr.set(other.licensePtr.get());
        }
        return *this;
    }

    ~Program() {}

    void setToDefaultPatch()
    {
        values.clear();
        for (const auto &param : ParameterList)
        {
            values[param.ID] = param.meta.naturalToNormalized01(param.meta.defaultVal);
        }
        setName(INIT_PATCH_NAME);
    }

    float getValueById(const juce::String &id) const
    {
        auto it = values.find(id);
        return it != values.end() ? it->second.load() : 0.0f;
    }

    void setValueById(const juce::String &id, float v) { values[id].store(v); }

    void setName(const juce::String &newName) { namePtr.set(newName); }

    juce::String getName() const { return namePtr.get(); }

    void setAuthor(const juce::String &newName) { authorPtr.set(newName); }

    juce::String getAuthor() const { return authorPtr.get(); }

    void setLicense(const juce::String &newName) { licensePtr.set(newName); }

    juce::String getLicense() const { return licensePtr.get(); }

    std::unordered_map<juce::String, std::atomic<float>> values;

  private:
    struct ASP
    {
        ASP() = default;

        ASP(const ASP &other) { set(other.get()); }
        ~ASP() { delete valPtr.load(); }
        void set(const juce::String &v)
        {
            auto *newStr = new juce::String(v);
            const auto *oldStr = valPtr.exchange(newStr);
            delete oldStr;
        }

        juce::String get() const
        {
            juce::String *ptr = valPtr.load();
            return ptr ? *ptr : juce::String();
        }

        std::atomic<juce::String *> valPtr{nullptr};
    };

    ASP namePtr, authorPtr, licensePtr;
};

#endif // OBXF_SRC_ENGINE_PARAMETERS_H