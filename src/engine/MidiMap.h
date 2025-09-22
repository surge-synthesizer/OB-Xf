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

#ifndef OBXF_SRC_ENGINE_MIDIMAP_H
#define OBXF_SRC_ENGINE_MIDIMAP_H

#include "SynthEngine.h"
#include "juce_audio_basics/juce_audio_basics.h"

static constexpr uint8_t NUM_MIDI_CC = 128;

class MidiMap
{
  public:
    std::array<int, NUM_MIDI_CC> controllers{};

    std::map<juce::String, int> mapping;

    bool loaded = false;

    MidiMap() { reset(); }

    void reset() { controllers.fill(-1); }

    int &operator[](int index)
    {
        if (index >= NUM_MIDI_CC)
        {
            exit(0);
        }

        return controllers[index];
    }

    void setXml(juce::XmlElement &xml)
    {
        for (auto c : controllers)
        {
            if (c > -1)
            {
                xml.setAttribute("CC" + juce::String(c), getTag(c));
            }
        }
    }

    juce::String getTag(int paraId)
    {
        for (std::map<juce::String, int>::iterator it = this->mapping.begin();
             it != this->mapping.end(); it++)
        {
            if (paraId == it->second)
            {
                return it->first;
            }
        }

        return "undefined";
    }

    int getParaId(juce::String tagName) { return mapping[tagName]; }

    void getXml(juce::XmlElement &xml)
    {
        for (auto &c : controllers)
        {
            juce::String tmp = xml.getStringAttribute("CC" + juce::String(c), "undefined");

            if (tmp != "undefined")
            {
                c = getParaId(tmp);
            }
        }
    }

    bool loadFile(juce::File &xml)
    {
        reset();

        if (xml.existsAsFile())
        {
            juce::XmlDocument xmlDoc(xml);
            this->getXml(*xmlDoc.getDocumentElement());
            return true;
        }

        return false;
    }

    void updateCC(int idx_para, int midiCC)
    {
        for (auto &c : controllers)
        {
            if (c == idx_para)
            {
                c = -1;
            }
        }

        controllers[midiCC] = idx_para;
    }

    void saveFile(juce::File &xml)
    {
        juce::XmlElement ele("Data");
        this->setXml(ele);

        if (auto outStream = xml.createOutputStream())
        {
            ele.writeTo(*outStream);
        }
    }
};

#endif // OBXF_SRC_ENGINE_MIDIMAP_H
