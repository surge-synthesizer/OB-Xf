/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright © 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

	This file may be licensed under the terms of of the
	GNU General Public License Version 2 (the ``GPL'').

	Software distributed under the License is distributed
	on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
	express or implied. See the GPL for the specific language
	governing rights and limitations.

	You should have received a copy of the GPL along with this
	program. If not, go to http://www.gnu.org/licenses/gpl.html
	or write to the Free Software Foundation, Inc.,  
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	==============================================================================
 */
#pragma once
#include "SynthEngine.h"
#include "ParamsEnum.h"
#include "juce_audio_basics/juce_audio_basics.h"
class MidiMap
{
public:
	int controllers[255];
    int controllers_default[255];
    
    std::map<juce::String, int> mapping;
    
    bool loaded = false;
	MidiMap()
	{
        reset();
        set_default();
	}
    void reset(){
        for(int i = 0 ; i < 255;i++){
            controllers[i] = -1;
            controllers_default[i] = -1;
        }
    }
    
    void set_default(){
        int midicc = 71;
        controllers[midicc] = controllers_default[midicc] = VOLUME;
        mapping["VOLUME"]=VOLUME;
        
        midicc = 15;
        controllers[midicc] = controllers_default[midicc] = VOICE_COUNT;
        mapping["VOICE_COUNT"]=VOICE_COUNT;
        
        midicc = 33;
        controllers[midicc] = controllers_default[midicc] = TUNE;
        mapping["TUNE"]=TUNE;
        
        
        midicc = 17;
        controllers[midicc] = controllers_default[midicc] = OCTAVE;
        mapping["OCTAVE"]=OCTAVE;
        
        midicc = 118;
        controllers[midicc] = controllers_default[midicc] = BENDRANGE;
        mapping["BENDRANGE"]=BENDRANGE;
        
        midicc = 34;
        controllers[midicc] = controllers_default[midicc] = BENDOSC2;
        mapping["BENDOSC2"]=BENDOSC2;
        
        midicc = 35;
        controllers[midicc] = controllers_default[midicc] = LEGATOMODE;
        mapping["LEGATOMODE"]=LEGATOMODE;
        
        midicc = 75;
        controllers[midicc] = controllers_default[midicc] = BENDLFORATE;
        mapping["BENDLFORATE"]=BENDLFORATE;
        
        midicc = 76;
        controllers[midicc] = controllers_default[midicc] = VFLTENV;
        mapping["VFLTENV"]=VFLTENV;
        
        midicc = 20;
        controllers[midicc] = controllers_default[midicc] = VAMPENV;
        mapping["VAMPENV"]=VAMPENV;
        
        midicc = 21;
        controllers[midicc] = controllers_default[midicc] = ASPLAYEDALLOCATION;
        mapping["ASPLAYEDALLOCATION"]=ASPLAYEDALLOCATION;
        
        midicc = 23;
        controllers[midicc] = controllers_default[midicc] = PORTAMENTO;
        mapping["PORTAMENTO"]=PORTAMENTO;
        
        midicc = 16;
        controllers[midicc] = controllers_default[midicc] = UNISON;
        mapping["UNISON"]=UNISON;
        
        midicc = 24;
        controllers[midicc] = controllers_default[midicc] = UDET;
        mapping["UDET"]=UDET;
        
        midicc = 43;
        controllers[midicc] = controllers_default[midicc] = OSC2_DET;
        mapping["OSC2_DET"]=OSC2_DET;
        
        midicc = 19;
        controllers[midicc] = controllers_default[midicc] = LFOFREQ;
        mapping["LFOFREQ"]=LFOFREQ;
        
        midicc = 44;
        controllers[midicc] = controllers_default[midicc] = LFOSINWAVE;
        mapping["LFOSINWAVE"]=LFOSINWAVE;
        
        midicc = 45;
        controllers[midicc] = controllers_default[midicc] = LFOSQUAREWAVE;
        mapping["LFOSQUAREWAVE"]=LFOSQUAREWAVE;
        
        midicc = 46;
        controllers[midicc] = controllers_default[midicc] = LFOSHWAVE;
        mapping["LFOSHWAVE"]=LFOSHWAVE;
        
        midicc = 22;
        controllers[midicc] = controllers_default[midicc] = LFO1AMT;
        mapping["LFO1AMT"]=LFO1AMT;
        
        midicc = 25;
        controllers[midicc] = controllers_default[midicc] = LFO2AMT;
        mapping["LFO2AMT"]=LFO2AMT;
        
        midicc = 47;
        controllers[midicc] = controllers_default[midicc] = LFOOSC1;
        mapping["LFOOSC1"]=LFOOSC1;
        
        midicc = 48;
        controllers[midicc] = controllers_default[midicc] = LFOOSC2;
        mapping["LFOOSC2"]=LFOOSC2;
        
        midicc = 49;
        controllers[midicc] = controllers_default[midicc] = LFOFILTER;
        mapping["LFOFILTER"]=LFOFILTER;
        
        midicc = 50;
        controllers[midicc] = controllers_default[midicc] = LFOPW1;
        mapping["LFOPW1"]=LFOPW1;
        
        midicc = 51;
        controllers[midicc] = controllers_default[midicc] = LFOPW2;
        mapping["LFOPW2"]=LFOPW2;
        
        midicc = 52;
        controllers[midicc] = controllers_default[midicc] = OSC2HS;
        mapping["OSC2HS"]=OSC2HS;
        
        midicc = 53;
        controllers[midicc] = controllers_default[midicc] = XMOD;
        mapping["XMOD"]=XMOD;
        
        midicc = 54;
        controllers[midicc] = controllers_default[midicc] = OSC1P;
        mapping["OSC1P"]=OSC1P;
        
        midicc = 55;
        controllers[midicc] = controllers_default[midicc] = OSC2P;
        mapping["OSC2P"]=OSC2P;
        
        midicc = 56;
        controllers[midicc] = controllers_default[midicc] = OSCQuantize;
        mapping["OSCQuantize"]=OSCQuantize;
        
        midicc = 57;
        controllers[midicc] = controllers_default[midicc] = OSC1Saw;
        mapping["OSC1Saw"]=OSC1Saw;
        
        midicc = 58;
        controllers[midicc] = controllers_default[midicc] = OSC1Pul;
        mapping["OSC1Pul"]=OSC1Pul;
        
        midicc = 59;
        controllers[midicc] = controllers_default[midicc] = OSC2Saw;
        mapping["OSC2Saw"]=OSC2Saw;
        
        midicc = 60;
        controllers[midicc] = controllers_default[midicc] = OSC2Pul;
        mapping["OSC2Pul"]=OSC2Pul;
        
        midicc = 61;
        controllers[midicc] = controllers_default[midicc] = PW;
        mapping["PW"]=PW;
        
        midicc = 62;
        controllers[midicc] = controllers_default[midicc] = BRIGHTNESS;
        mapping["BRIGHTNESS"]=BRIGHTNESS;
        
        midicc = 63;
        controllers[midicc] = controllers_default[midicc] = ENVPITCH;
        mapping["ENVPITCH"]=ENVPITCH;
        
        midicc = 77;
        controllers[midicc] = controllers_default[midicc] = OSC1MIX;
        mapping["OSC1MIX"]=OSC1MIX;
        
        midicc = 78;
        controllers[midicc] = controllers_default[midicc] = OSC2MIX;
        mapping["OSC2MIX"]=OSC2MIX;
        
        midicc = 102;
        controllers[midicc] = controllers_default[midicc] = NOISEMIX;
        mapping["NOISEMIX"]=NOISEMIX;
        
        midicc = 103;
        controllers[midicc] = controllers_default[midicc] = FLT_KF;
        mapping["FLT_KF"]=FLT_KF;
        
        midicc = 74;
        controllers[midicc] = controllers_default[midicc] = CUTOFF;
        mapping["CUTOFF"]=CUTOFF;
        
        midicc = 42;
        controllers[midicc] = controllers_default[midicc] = RESONANCE;
        mapping["RESONANCE"]=RESONANCE;
        
        midicc = 104;
        controllers[midicc] = controllers_default[midicc] = MULTIMODE;
        mapping["MULTIMODE"]=MULTIMODE;
        
        midicc = 18;
        controllers[midicc] = controllers_default[midicc] = FILTER_WARM;
        mapping["FILTER_WARM"]=FILTER_WARM;
        
        midicc = 105;
        controllers[midicc] = controllers_default[midicc] = BANDPASS;
        mapping["BANDPASS"]=BANDPASS;
        
        midicc = 106;
        controllers[midicc] = controllers_default[midicc] = FOURPOLE;
        mapping["FOURPOLE"]=FOURPOLE;
        
        midicc = 107;
        controllers[midicc] = controllers_default[midicc] = ENVELOPE_AMT;
        mapping["ENVELOPE_AMT"]=ENVELOPE_AMT;
        
        midicc = 73;
        controllers[midicc] = controllers_default[midicc] = LATK;
        mapping["LATK"]=LATK;
        
        midicc = 36;
        controllers[midicc] = controllers_default[midicc] = LDEC;
        mapping["LDEC"]=LDEC;
        
        midicc = 37;
        controllers[midicc] = controllers_default[midicc] = LSUS;
        mapping["LSUS"]=LSUS;
        
        midicc = 72;
        controllers[midicc] = controllers_default[midicc] = LREL;
        mapping["LREL"]=LREL;
        
        midicc = 38;
        controllers[midicc] = controllers_default[midicc] = FATK;
        mapping["FATK"]=FATK;
        
        midicc = 39;
        controllers[midicc] = controllers_default[midicc] = FDEC;
        mapping["FDEC"]=FDEC;
        
        midicc = 40;
        controllers[midicc] = controllers_default[midicc] = FSUS;
        mapping["FSUS"]=FSUS;
        
        midicc = 41;
        controllers[midicc] = controllers_default[midicc] = FREL;
        mapping["FREL"]=FREL;
        
        midicc = 108;
        controllers[midicc] = controllers_default[midicc] = ENVDER;
        mapping["ENVDER"]=ENVDER;
        
        midicc = 109;
        controllers[midicc] = controllers_default[midicc] = FILTERDER;
        mapping["FILTERDER"]=FILTERDER;
        
        midicc = 110;
        controllers[midicc] = controllers_default[midicc] = PORTADER;
        mapping["PORTADER"]=PORTADER;
        
        midicc = 81;
        controllers[midicc] = controllers_default[midicc] = PAN1;
        mapping["PAN1"]=PAN1;
        
        midicc = 82;
        controllers[midicc] = controllers_default[midicc] = PAN2;
        mapping["PAN2"]=PAN2;
        
        midicc = 83;
        controllers[midicc] = controllers_default[midicc] = PAN3;
        mapping["PAN3"]=PAN3;
        
        midicc = 84;
        controllers[midicc] = controllers_default[midicc] = PAN4;
        mapping["PAN4"]=PAN4;
        
        midicc = 85;
        controllers[midicc] = controllers_default[midicc] = PAN5;
        mapping["PAN5"]=PAN5;
        
        midicc = 86;
        controllers[midicc] = controllers_default[midicc] = PAN6;
        mapping["PAN6"]=PAN6;
        
        midicc = 87;
        controllers[midicc] = controllers_default[midicc] = PAN7;
        mapping["PAN7"]=PAN7;
        
        midicc = 88;
        controllers[midicc] = controllers_default[midicc] = PAN8;
        mapping["PAN8"]=PAN8;
        
        midicc = 111;
        controllers[midicc] = controllers_default[midicc] = ECONOMY_MODE;
        mapping["ECONOMY_MODE"]=ECONOMY_MODE;
        
        //midicc = 112;
        //controllers[midicc] = controllers_default[midicc] = LFO_SYNC;
        
        midicc = 113;
        controllers[midicc] = controllers_default[midicc] = PW_ENV;
        mapping["PW_ENV"]=PW_ENV;
        
        midicc = 114;
        controllers[midicc] = controllers_default[midicc] = PW_ENV_BOTH;
        mapping["PW_ENV_BOTH"]=PW_ENV_BOTH;
        
        midicc = 115;
        controllers[midicc] = controllers_default[midicc] = ENV_PITCH_BOTH;
        mapping["ENV_PITCH_BOTH"]=ENV_PITCH_BOTH;
        
        midicc = 116;
        controllers[midicc] = controllers_default[midicc] = FENV_INVERT;
        mapping["FENV_INVERT"]=FENV_INVERT;
        
        midicc = 117;
        controllers[midicc] = controllers_default[midicc] = PW_OSC2_OFS;
        mapping["PW_OSC2_OFS"]=PW_OSC2_OFS;
        
        midicc = 118;
        controllers[midicc] = controllers_default[midicc] = LEVEL_DIF;
        mapping["LEVEL_DIF"]=LEVEL_DIF;
        
        midicc = 119;
        controllers[midicc] = controllers_default[midicc] = SELF_OSC_PUSH;
        mapping["SELF_OSC_PUSH"]=SELF_OSC_PUSH;
        
    }
    
    
    int& operator[](int index)
    {
        if (index >= 255) {
            exit(0);
        }
        return controllers[index];
    }
    
    void setXml( juce::XmlElement &xml){
        for (int i = 0; i < 255; ++i)
        {
            if (controllers[i]!= -1){
                xml.setAttribute("MIDI_" +juce::String(i), getTag(controllers[i]));
            }
        }
    }
    juce::String getTag(int paraId){
        for (std::map<juce::String, int>::iterator it = this->mapping.begin(); it != this->mapping.end(); it ++){
            if (paraId == it->second){
                return it->first;
            }
        }
        return "undefine";
    }
    
    int getParaId(juce::String tagName){
        return mapping[tagName];
    }
    
    void getXml(juce::XmlElement &xml){
        for (int i = 0; i < 255; ++i)
        {
            juce::String tmp = xml.getStringAttribute("MIDI_" + juce::String(i), "undefine");
            if (tmp != "undefine"){
                controllers[i] = getParaId(tmp);
            }
        }
        
        // Backward keys
        if (controllers[100] > 0) {
            controllers[77] = controllers_default[77];
            controllers[100] = 0;
        }
        if ( controllers[101] > 0) {
            controllers[78] = controllers_default[78];
            controllers[101] = 0;
        }
    }
    
    bool loadFile(juce::File& xml){
        reset();
        set_default();
        if (xml.existsAsFile()){
            juce::XmlDocument xmlDoc (xml);
            this->getXml(*xmlDoc.getDocumentElement());
            return true;
        }
        
        return false;
    }
    
    void updateCC(int idx_para, int midiCC) {
        for (int i =0; i < 255; i++) {
            if (controllers[i] == idx_para){
                controllers[i] = -1;
            }
        }
        controllers[midiCC] = idx_para;
    }
    
    void saveFile(juce::File& xml)
	{
	    juce::XmlElement ele("Data");
	    this->setXml(ele);

	    if (auto outStream = xml.createOutputStream())
	        ele.writeTo(*outStream);
	}
    
    void clean(){
        reset();
    }
    
};
