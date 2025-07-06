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

        using namespace SynthParam::Name;

        // clang-format off
        const MappingEntry entries[] =
        {
            {71, Volume},
            {15, Polyphony},
            {33, Tune},
            {17, Transpose},
            {118, PitchBendUpRange},
            {34, Osc2Detune},
            {35, EnvLegatoMode},
            {75, Lfo1Rate},
            {76, FilterEnvAmount},
            {20, VelToFilterEnv},
            {21, NotePriority},
            {23, Portamento},
            {16, Unison},
            {24, UnisonDetune},
            {43, Osc2Detune},
            {44, Lfo1Wave1},
            {45, Lfo1Wave2},
            {46, Lfo1Wave3},
            {22, Lfo1ModAmt1},
            {25, Lfo1ModAmt2},
            {47, Lfo1ToOsc1Pitch},
            {48, Lfo1ToOsc2Pitch},
            {49, Lfo1ToFilterCutoff},
            {50, Lfo1ToOsc1PW},
            {51, Lfo1ToOsc2PW},
            {52, OscSync},
            {53, Crossmod},
            {54, Osc1Pitch},
            {55, Osc2Pitch},
            {56, FilterEnvInvert},
            {57, Osc1SawWave},
            {58, Osc1PulseWave},
            {59, Osc2SawWave},
            {60, Osc2PulseWave},
            {61, OscPulseWidth},
            {62, Brightness},
            {63, EnvToPitch},
            {77, Osc1Mix},
            {78, Osc2Mix},
            {102, NoiseMix},
            {103, FilterKeyFollow},
            {74, FilterCutoff},
            {42, FilterResonance},
            {104, FilterMode},
            {18, FilterSlop},
            {105, Filter2PoleBPBlend},
            {106, Filter4PoleMode},
            {73, AmpEnvAttack},
            {36, AmpEnvDecay},
            {37, AmpEnvSustain},
            {72, AmpEnvRelease},
            {38, FilterEnvAttack},
            {39, FilterEnvDecay},
            {40, FilterEnvSustain},
            {41, FilterEnvRelease},
            {108, EnvelopeSlop},
            {109, FilterSlop},
            {110, PortamentoSlop},
            {81, PanVoice1},
            {82, PanVoice2},
            {83, PanVoice3},
            {84, PanVoice4},
            {85, PanVoice5},
            {86, PanVoice6},
            {87, PanVoice7},
            {88, PanVoice8},
            {113, EnvToPW},
            {114, EnvToPWBothOscs},
            {115, EnvToPitchBothOscs},
            {117, Osc2PWOffset},
            {118, LevelSlop},
            {119, Filter2PolePush}
        };
        // clang-format on

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
