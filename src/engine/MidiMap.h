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

class MidiMap
{
  public:
    int controllers[255]{};
    int controllers_default[255]{};

    std::map<juce::String, int> mapping;

    bool loaded = false;
    MidiMap()
    {
        reset();
        set_default();
    }

    void reset()
    {
        for (int i = 0; i < 255; i++)
        {
            controllers[i] = -1;
            controllers_default[i] = -1;
        }
    }

    void set_default()
    {
        auto getParaIdByName = [](const std::string &name) -> int {
            for (const auto &i : ParameterList)
                if (i.meta.name == name)
                    return i.meta.id;
            return -1;
        };

        struct MappingEntry
        {
            int cc;
            const std::string &name;
        };
        const MappingEntry entries[] = {{71, "Volume"},
                                        {15, "Polyphony"},
                                        {33, "Tune"},
                                        {17, "Transpose"},
                                        {118, "Pitch Bend Up"},
                                        {34, "Osc 2 Detune"},
                                        {35, "Legato Mode"},
                                        {75, "LFO Rate"},
                                        {76, "Filter Envelope Amount"},
                                        {20, "Velocity to Amp"},
                                        {21, "Note Priority"},
                                        {23, "Portamento"},
                                        {16, "Unison"},
                                        {24, "Unison Detune"},
                                        {43, "Osc 2 Detune"},
                                        {19, "LFO Rate"},
                                        {44, "LFO Wave 1"},
                                        {45, "LFO Wave 2"},
                                        {46, "LFO Wave 3"},
                                        {22, "LFO Mod 1 Amount"},
                                        {25, "LFO Mod 2 Amount"},
                                        {47, "LFO to Osc 1 Pitch"},
                                        {48, "LFO to Osc 2 Pitch"},
                                        {49, "LFO to Filter"},
                                        {50, "LFO to Osc 1 PW"},
                                        {51, "LFO to Osc 2 PW"},
                                        {52, "Osc 2 Hard Sync"},
                                        {53, "Cross Modulation"},
                                        {54, "Osc 1 Pitch"},
                                        {55, "Osc 2 Pitch"},
                                        {56, "Invert Filter Env to Pitch"},
                                        {57, "Osc 1 Saw"},
                                        {58, "Osc 1 Pulse"},
                                        {59, "Osc 2 Saw"},
                                        {60, "Osc 2 Pulse"},
                                        {61, "Pulsewidth"},
                                        {62, "Brightness"},
                                        {63, "Filter Env to Pitch"},
                                        {77, "Osc 1 Mix"},
                                        {78, "Osc 2 Mix"},
                                        {102, "Noise Mix"},
                                        {103, "Filter Keyfollow"},
                                        {74, "Filter Cutoff"},
                                        {42, "Filter Resonance"},
                                        {104, "Filter Mode"},
                                        {18, "Filter Slop"},
                                        {105, "Filter Bandpass Blend"},
                                        {106, "Filter 4-Pole Mode"},
                                        {107, "Filter Envelope Amount"},
                                        {73, "Amp Envelope Attack"},
                                        {36, "Amp Envelope Decay"},
                                        {37, "Amp Envelope Sustain"},
                                        {72, "Amp Envelope Release"},
                                        {38, "Filter Envelope Attack"},
                                        {39, "Filter Envelope Decay"},
                                        {40, "Filter Envelope Sustain"},
                                        {41, "Filter Envelope Release"},
                                        {108, "Envelope Slop"},
                                        {109, "Filter Slop"},
                                        {110, "Portamento Slop"},
                                        {81, "Pan Voice 1"},
                                        {82, "Pan Voice 2"},
                                        {83, "Pan Voice 3"},
                                        {84, "Pan Voice 4"},
                                        {85, "Pan Voice 5"},
                                        {86, "Pan Voice 6"},
                                        {87, "Pan Voice 7"},
                                        {88, "Pan Voice 8"},
                                        {113, "Filter Envelope to PW"},
                                        {114, "Filter Envelope to Osc 1+2 PW"},
                                        {115, "Filter Envelope to Osc 1+2 Pitch"},
                                        {116, "Filter Envelope Invert"},
                                        {117, "Osc 2 PW Offset"},
                                        {118, "Level Slop"},
                                        {119, "Filter Self-Oscillation Push"}};

        for (const auto &entry : entries)
        {
            int paraId = getParaIdByName(entry.name);
            controllers[entry.cc] = controllers_default[entry.cc] = paraId;
            mapping[entry.name] = paraId;
        }
    }

    int &operator[](int index)
    {
        if (index >= 255)
        {
            exit(0);
        }
        return controllers[index];
    }

    void setXml(juce::XmlElement &xml)
    {
        for (int i = 0; i < 255; ++i)
        {
            if (controllers[i] != -1)
            {
                xml.setAttribute("MIDI_" + juce::String(i), getTag(controllers[i]));
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
        return "undefine";
    }

    int getParaId(juce::String tagName) { return mapping[tagName]; }

    void getXml(juce::XmlElement &xml)
    {
        for (int i = 0; i < 255; ++i)
        {
            juce::String tmp = xml.getStringAttribute("MIDI_" + juce::String(i), "undefine");
            if (tmp != "undefine")
            {
                controllers[i] = getParaId(tmp);
            }
        }

        // Backward keys
        if (controllers[100] > 0)
        {
            controllers[77] = controllers_default[77];
            controllers[100] = 0;
        }
        if (controllers[101] > 0)
        {
            controllers[78] = controllers_default[78];
            controllers[101] = 0;
        }
    }

    bool loadFile(juce::File &xml)
    {
        reset();
        set_default();
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
        for (int i = 0; i < 255; i++)
        {
            if (controllers[i] == idx_para)
            {
                controllers[i] = -1;
            }
        }
        controllers[midiCC] = idx_para;
    }

    void saveFile(juce::File &xml)
    {
        juce::XmlElement ele("Data");
        this->setXml(ele);

        if (auto outStream = xml.createOutputStream())
            ele.writeTo(*outStream);
    }

    void clean() { reset(); }
};

#endif // OBXF_SRC_ENGINE_MIDIMAP_H
