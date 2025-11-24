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
        name = INIT_PATCH_NAME;
        setToDefaultPatch();
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
        setLicense("");
        setAuthor("");
        setCategory("None");
        setProject("");
    }

    float getValueById(const juce::String &id) const
    {
        auto it = values.find(id);
        return it != values.end() ? it->second.load() : 0.0f;
    }

    void setValueById(const juce::String &id, float v) { values[id].store(v); }

    void setName(const juce::String &newName) { name = newName; }
    juce::String getName() const { return name; }

    void setAuthor(const juce::String &newName) { author = newName; }
    juce::String getAuthor() const { return author; }

    void setLicense(const juce::String &newName) { license = newName; }
    juce::String getLicense() const { return license; }

    void setCategory(const juce::String &newName) { category = newName; }
    juce::String getCategory() const { return category; }

    void setProject(const juce::String &newName) { project = newName; }
    juce::String getProject() const { return project; }

    static std::vector<juce::String> availableCategories()
    {
        return {"Bass",  "Brass",      "Bells", "Drums", "FX",         "Keys",    "Lead",  "Mallet",
                "Organ", "Percussion", "Pad",   "Pluck", "Soundscape", "Strings", "Vocal", "Winds"};
    }
    std::unordered_map<juce::String, std::atomic<float>> values;

  private:
    juce::String name, author, license, category, project;
};

#endif // OBXF_SRC_ENGINE_PARAMETERS_H