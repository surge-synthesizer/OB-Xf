/*
Copyright (C) 2021 by ODDSound Ltd. info@oddsound.com

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

#include "libMTSClient.h"
#include <math.h>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__TOS_WIN__) || defined(_MSC_VER)
#define MTS_ESP_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HRESULT (WINAPI* SHGetKnownFolderPathFunc) (const GUID*,DWORD,HANDLE,PWSTR*);
typedef void (WINAPI* CoTaskMemFreeFunc) (LPVOID);
#else
#include <dlfcn.h>
#endif

const static double ln2=0.693147180559945309417;
const static double ratioToSemitones=17.31234049066756088832; // 12./log(2.)
typedef void (*mts_void)(void);
typedef bool (*mts_bool)(void);
typedef bool (*mts_bcc)(char,char);
typedef const double *(*mts_cd)(void);
typedef const double *(*mts_cdc)(char);
typedef bool (*mts_bc)(char);
typedef const char *(*mts_pcc)(void);

struct mtsclientglobal
{
    mtsclientglobal() : RegisterClient(0), DeregisterClient(0), HasMaster(0), ShouldFilterNote(0), ShouldFilterNoteMultiChannel(0), GetTuning(0), GetMultiChannelTuning(0), UseMultiChannelTuning(0), GetScaleName(0), esp_retuning(0), handle(0)
    {
        for (int i=0;i<128;i++) iet[i]=1./(440.*pow(2.,(i-69.)/12.));
        load_lib();
        if (GetTuning) esp_retuning=GetTuning();
        for (int i=0;i<16;i++) multi_channel_esp_retuning[i]=GetMultiChannelTuning?GetMultiChannelTuning(static_cast<char>(i)):0;
    }
    virtual inline bool isOnline() const {return esp_retuning && HasMaster && HasMaster();}
    
    mts_void RegisterClient,DeregisterClient;mts_bool HasMaster;mts_bcc ShouldFilterNote,ShouldFilterNoteMultiChannel;mts_cd GetTuning;mts_cdc GetMultiChannelTuning;mts_bc UseMultiChannelTuning;mts_pcc GetScaleName; // Interface to lib
    double iet[128];const double *esp_retuning;const double *multi_channel_esp_retuning[16]; // tuning tables
    
#ifdef MTS_ESP_WIN
    virtual void load_lib()
	{
        SHGetKnownFolderPathFunc SHGetKnownFolderPath=0;
        CoTaskMemFreeFunc CoTaskMemFree=0;
        HMODULE shell32Module=GetModuleHandleW(L"Shell32.dll");
        HMODULE ole32Module=GetModuleHandleW(L"Ole32.dll");
        if (shell32Module) SHGetKnownFolderPath=(SHGetKnownFolderPathFunc)GetProcAddress(shell32Module,"SHGetKnownFolderPath");
        if (ole32Module) CoTaskMemFree=(CoTaskMemFreeFunc)GetProcAddress(ole32Module,"CoTaskMemFree");
        if (SHGetKnownFolderPath && CoTaskMemFree)
        {
            const GUID FOLDERID_ProgramFilesCommonGUID = {0xF7F1ED05, 0x9F6D, 0x47A2, {0xAA, 0xAE, 0x29, 0xD3, 0x17, 0xC6, 0xF0, 0x66}};
            PWSTR cf=NULL;
            if (SHGetKnownFolderPath(&FOLDERID_ProgramFilesCommonGUID,0,0,&cf)>=0)
            {
                WCHAR buffer[MAX_PATH];buffer[0]=L'\0';
                if (cf) {
#if defined(_MSC_VER)
                    wcsncpy_s(buffer, MAX_PATH, cf, _TRUNCATE);
#else
                    wcsncpy(buffer, cf, MAX_PATH);
                    buffer[MAX_PATH - 1] = L'\0';
#endif
                }
                CoTaskMemFree(cf);
                buffer[MAX_PATH-1]=L'\0';
                const WCHAR *libpath=L"\\MTS-ESP\\LIBMTS.dll";
                DWORD cfLen=wcslen(buffer);
#if defined(_MSC_VER)
                wcsncat_s(buffer, MAX_PATH, libpath, MAX_PATH-cfLen-1);
#else
                wcsncat(buffer, libpath, MAX_PATH-cfLen-1);
                buffer[MAX_PATH - 1] = L'\0';
#endif
                if (!(handle=LoadLibraryW(buffer))) return;
            }
            else {CoTaskMemFree(cf);return;}
        }
        else return;
        RegisterClient                  =(mts_void) GetProcAddress(handle,"MTS_RegisterClient");
        DeregisterClient                =(mts_void) GetProcAddress(handle,"MTS_DeregisterClient");
        HasMaster                       =(mts_bool) GetProcAddress(handle,"MTS_HasMaster");
        ShouldFilterNote                =(mts_bcc)  GetProcAddress(handle,"MTS_ShouldFilterNote");
        ShouldFilterNoteMultiChannel    =(mts_bcc)  GetProcAddress(handle,"MTS_ShouldFilterNoteMultiChannel");
        GetTuning                       =(mts_cd)   GetProcAddress(handle,"MTS_GetTuningTable");
        GetMultiChannelTuning           =(mts_cdc)  GetProcAddress(handle,"MTS_GetMultiChannelTuningTable");
        UseMultiChannelTuning           =(mts_bc)   GetProcAddress(handle,"MTS_UseMultiChannelTuning");
        GetScaleName                    =(mts_pcc)  GetProcAddress(handle,"MTS_GetScaleName");
    }
    virtual ~mtsclientglobal() {if (handle) FreeLibrary(handle);}
    HINSTANCE handle;
#else
    virtual void load_lib()
    {
        if (!(handle=dlopen("/Library/Application Support/MTS-ESP/libMTS.dylib",RTLD_NOW)) &&
            !(handle=dlopen("/usr/local/lib/libMTS.so",RTLD_NOW))) return;
        RegisterClient                  =(mts_void) dlsym(handle,"MTS_RegisterClient");
        DeregisterClient                =(mts_void) dlsym(handle,"MTS_DeregisterClient");
        HasMaster                       =(mts_bool) dlsym(handle,"MTS_HasMaster");
        ShouldFilterNote                =(mts_bcc)  dlsym(handle,"MTS_ShouldFilterNote");
        ShouldFilterNoteMultiChannel    =(mts_bcc)  dlsym(handle,"MTS_ShouldFilterNoteMultiChannel");
        GetTuning                       =(mts_cd)   dlsym(handle,"MTS_GetTuningTable");
        GetMultiChannelTuning           =(mts_cdc)  dlsym(handle,"MTS_GetMultiChannelTuningTable");
        UseMultiChannelTuning           =(mts_bc)   dlsym(handle,"MTS_UseMultiChannelTuning");
        GetScaleName                    =(mts_pcc)  dlsym(handle,"MTS_GetScaleName");
    }
    virtual ~mtsclientglobal() {if (handle) dlclose(handle);}
    void *handle;
#endif
};

static mtsclientglobal global;

struct MTSClient
{
    MTSClient() : tuningName("12-TET"), supportsNoteFiltering(false), supportsMultiChannelNoteFiltering(false), supportsMultiChannelTuning(false), freqRequestReceived(false), supportsMTSSysex(false)
    {
        for (int i=0;i<128;i++) retuning[i]=440.*pow(2.,(i-69.)/12.);
        if (global.RegisterClient) global.RegisterClient();
    }
    virtual ~MTSClient() {if (global.DeregisterClient) global.DeregisterClient();}
    bool hasMaster() {return global.isOnline();}
    inline double freq(char midinote,char midichannel)
    {
        freqRequestReceived=true;
        supportsMultiChannelTuning=!(midichannel&~15);
        if (!global.isOnline()) return retuning[midinote&127];
        if ((!supportsNoteFiltering || supportsMultiChannelNoteFiltering) && supportsMultiChannelTuning && global.UseMultiChannelTuning && global.UseMultiChannelTuning(midichannel) && global.multi_channel_esp_retuning[midichannel&15])
        {
            return global.multi_channel_esp_retuning[midichannel&15][midinote&127];
        }
        return global.esp_retuning[midinote&127];
    }
    inline double ratio(char midinote,char midichannel)
    {
        freqRequestReceived=true;
        supportsMultiChannelTuning=!(midichannel&~15);
        if (!global.isOnline()) return supportsMTSSysex?retuning[midinote&127]*global.iet[midinote&127]:1.;
        if ((!supportsNoteFiltering || supportsMultiChannelNoteFiltering) && supportsMultiChannelTuning && global.UseMultiChannelTuning && global.UseMultiChannelTuning(midichannel) && global.multi_channel_esp_retuning[midichannel&15])
        {
            return global.multi_channel_esp_retuning[midichannel&15][midinote&127]*global.iet[midinote&127];
        }
        return global.esp_retuning[midinote&127]*global.iet[midinote&127];
    }
    inline double semitones(char midinote,char midichannel)
    {
        freqRequestReceived=true;
        supportsMultiChannelTuning=!(midichannel&~15);
        if (!global.isOnline()) return supportsMTSSysex?ratioToSemitones*log(retuning[midinote&127]*global.iet[midinote&127]):0.;
        if ((!supportsNoteFiltering || supportsMultiChannelNoteFiltering) && supportsMultiChannelTuning && global.UseMultiChannelTuning && global.UseMultiChannelTuning(midichannel) && global.multi_channel_esp_retuning[midichannel&15])
        {
            return ratioToSemitones*log(global.multi_channel_esp_retuning[midichannel&15][midinote&127]*global.iet[midinote&127]);
        }
        return ratioToSemitones*log(global.esp_retuning[midinote&127]*global.iet[midinote&127]);
    }
    inline bool shouldFilterNote(char midinote,char midichannel)
    {
        supportsNoteFiltering=true;
        supportsMultiChannelNoteFiltering=!(midichannel&~15);
        if (!freqRequestReceived) supportsMultiChannelTuning=supportsMultiChannelNoteFiltering; // assume it supports multi channel tuning until a request is received for a frequency and can verify
        if (!global.isOnline()) return false;
        if (supportsMultiChannelNoteFiltering && supportsMultiChannelTuning && global.UseMultiChannelTuning && global.UseMultiChannelTuning(midichannel))
        {
            return global.ShouldFilterNoteMultiChannel?global.ShouldFilterNoteMultiChannel(midinote&127,midichannel):false;
        }
        return global.ShouldFilterNote?global.ShouldFilterNote(midinote&127,midichannel):false;
    }
    inline char freqToNote(double freq,char midichannel)
    {
        bool online=global.isOnline(),multiChannel=false;
        const double *freqs=online?global.esp_retuning:retuning;
        if (online && !(midichannel&~15) && global.UseMultiChannelTuning && global.UseMultiChannelTuning(midichannel) && global.multi_channel_esp_retuning[midichannel & 0x0F])
        {
            freqs=global.multi_channel_esp_retuning[midichannel & 0x0F];
            multiChannel=true;
        }
        int iLower,iUpper;iLower=0;iUpper=0;
        double dLower,dUpper;dLower=0;dUpper=0;
        for (int i=0;i<128;i++)
        {
            if (online)
            {
                if (multiChannel && global.ShouldFilterNoteMultiChannel && global.ShouldFilterNoteMultiChannel(static_cast<char>(i),midichannel)) continue;
                else if (global.ShouldFilterNote && global.ShouldFilterNote(static_cast<char>(i),midichannel)) continue;
            }
            double d=freqs[i]-freq;
            if (d==0.) return static_cast<char>(i);
            if (d<0) {if (dLower==0. || d>dLower) {dLower=d;iLower=i;}}
            else if (dUpper==0. || d<dUpper) {dUpper=d;iUpper=i;}
        }
        if (dLower==0.) return static_cast<char>(iUpper);
        if (dUpper==0. || iLower==iUpper) return static_cast<char>(iLower);
        double fmid=freqs[iLower]*pow(2.,0.5*(log(freqs[iUpper]/freqs[iLower])/ln2));
        return freq<fmid?static_cast<char>(iLower):static_cast<char>(iUpper);
    }
    inline char freqToNote(double freq,char *midichannel)
    {
        if (!midichannel) return freqToNote(freq,static_cast<char>(-1));
        if (global.isOnline() && global.UseMultiChannelTuning)
        {
            int channelsInUse[16];int nMultiChannels=0;
            for (int i=0;i<16;i++) if (global.UseMultiChannelTuning(i) && global.multi_channel_esp_retuning[i]) channelsInUse[nMultiChannels++]=i;
            if (nMultiChannels)
            {
                const int nFreqs=128*nMultiChannels;
                int iLower,iUpper,channel,note;iLower=0;iUpper=0;channel=0;note=0;
                double dLower,dUpper;dLower=0;dUpper=0;
                for (int i=0;i<nFreqs;i++)
                {
                    channel=channelsInUse[i>>7];note=i&127;
                    if (global.ShouldFilterNoteMultiChannel && global.ShouldFilterNoteMultiChannel(static_cast<char>(note),static_cast<char>(channel))) continue;
                    double d=global.multi_channel_esp_retuning[channel][note]-freq;
                    if (d==0.) {*midichannel=static_cast<char>(channel);return static_cast<char>(note);}
                    if (d<0) {if (dLower==0. || d>dLower) {dLower=d;iLower=i;}}
                    else if (dUpper==0. || d<dUpper) {dUpper=d;iUpper=i;}
                }
                if (dLower==0.) {*midichannel=static_cast<char>(channelsInUse[iUpper>>7]);return static_cast<char>(iUpper&127);}
                if (dUpper==0. || iLower==iUpper) {*midichannel=static_cast<char>(channelsInUse[iLower>>7]);return static_cast<char>(iLower&127);}
                double fLower=global.multi_channel_esp_retuning[channelsInUse[iLower>>7]][iLower&127];
                double fUpper=global.multi_channel_esp_retuning[channelsInUse[iUpper>>7]][iUpper&127];
                double fmid=fLower*pow(2.,0.5*(log(fUpper/fLower)/ln2));
                if (freq<fmid) {*midichannel=static_cast<char>(channelsInUse[iLower>>7]);return static_cast<char>(iLower&127);}
                *midichannel=static_cast<char>(channelsInUse[iUpper>>7]);return static_cast<char>(iUpper&127);
            }
        }
        *midichannel=static_cast<char>(0);
        return freqToNote(freq,static_cast<char>(0));
    }
    inline void parseMIDIData(const unsigned char *buffer,int len)
    {
        supportsMTSSysex=true;
        int sysex_ctr=0,sysex_value=0,note=0,numTunings=0;
        /*int bank=-1,prog=0,checksum=0,deviceID=0;short int channelBitmap=0;bool realtime=false;*/ // unused for now
        eSysexState state=eIgnoring;eMTSFormat format=eBulk;
        for (int i=0;i<len;i++)
        {
            unsigned char b=buffer[i];
            if (b==0xF7) {state=eIgnoring;continue;}
            if (b>0x7F && b!=0xF0) continue;
            switch (state)
            {
                case eIgnoring:
                    if (b==0xF0) state=eMatchingSysex;
                    break;
                case eMatchingSysex:
                    sysex_ctr=0;
                    if (b==0x7E) state=eSysexValid;
                    else if (b==0x7F) {/*realtime=true;*/state=eSysexValid;}
                    else state=eIgnoring;
                    break;
                case eSysexValid:
                    switch (sysex_ctr++) // handle device ID
                    {
                        case 0: /*deviceID=b;*/ break;
                        case 1: if (b==0x08) state=eMatchingMTS; break;
                        default: state=eIgnoring; break; // it's not an MTS message
                    }
                    break;
                case eMatchingMTS:
                    sysex_ctr=0;
                    switch (b)
                    {
                        case 0: format=eRequest;state=eMatchingProg; break;
                        case 1: format=eBulk;state=eMatchingProg; break;
                        case 2: format=eSingle;state=eMatchingProg; break;
                        case 3: format=eRequest;state=eMatchingBank; break;
                        case 4: format=eBulk;state=eMatchingBank; break;
                        case 5: format=eScaleOctOneByte;state=eMatchingBank; break;
                        case 6: format=eScaleOctTwoByte;state=eMatchingBank; break;
                        case 7: format=eSingle;state=eMatchingBank; break;
                        case 8: format=eScaleOctOneByteExt;state=eMatchingChannel; break;
                        case 9: format=eScaleOctTwoByteExt;state=eMatchingChannel; break;
                        default: state=eIgnoring; break; // it's not a valid MTS format
                    }
                    break;
                case eMatchingBank:
                    /*bank=b;*/
                    state=eMatchingProg;
                    break;
                case eMatchingProg:
                    /*prog=b;*/
                    if (format==eSingle) state=eNumTunings;else {state=eTuningName;tuningName[0]='\0';}
                    break;
                case eTuningName:
                    tuningName[sysex_ctr]=static_cast<char>(b);
                    if (++sysex_ctr>=16) {tuningName[16]='\0';sysex_ctr=0;state=eTuningData;}
                    break;
                case eNumTunings:
                    numTunings=b;sysex_ctr=0;state=eTuningData;
                    break;
                case eMatchingChannel:
                    switch (sysex_ctr++)
                    {
                        case 0: /*for (int j=14;j<16;j++) channelBitmap|=(1<<j);*/ break;
                        case 1: /*for (int j=7;j<14;j++) channelBitmap|=(1<<j);*/ break;
                        case 2: /*for (int j=0;j<7;j++) channelBitmap|=(1<<j);*/sysex_ctr=0;state=eTuningData; break;
                    }
                    break;
                case eTuningData:
                    switch (format)
                    {
                        case eBulk:
                            sysex_value=(sysex_value<<7)|b;
                            sysex_ctr++;
                            if ((sysex_ctr&3)==3)
                            {
                                if (!(note==0x7F && sysex_value==16383)) updateTuning(note,(sysex_value>>14)&127,(sysex_value&16383)/16383.);
                                sysex_value=0;sysex_ctr++;
                                if (++note>=128) state=eCheckSum;
                            }
                            break;
                        case eSingle:
                            sysex_value=(sysex_value<<7)|b;
                            sysex_ctr++;
                            if (!(sysex_ctr&3))
                            {
                                if (!(note==0x7F && sysex_value==16383)) updateTuning((sysex_value>>21)&127,(sysex_value>>14)&127,(sysex_value&16383)/16383.);
                                sysex_value=0;
                                if (++note>=numTunings) state=eIgnoring;
                            }
                            break;
                        case eScaleOctOneByte: case eScaleOctOneByteExt:
                            for (int j=sysex_ctr;j<128;j+=12) updateTuning(j,j,(static_cast<double>(b)-64.)*0.01);
                            if (++sysex_ctr>=12) state=format==eScaleOctOneByte?eCheckSum:eIgnoring;
                            break;
                        case eScaleOctTwoByte: case eScaleOctTwoByteExt:
                            sysex_value=(sysex_value<<7)|b;
                            sysex_ctr++;
                            if (!(sysex_ctr&1))
                            {
                                double detune=(static_cast<double>(sysex_value&16383)-8192.)/(sysex_value>8192?8191.:8192.);
                                for (int j=note;j<128;j+=12) updateTuning(j,j,detune);
                                if (++note>=12) state=format==eScaleOctTwoByte?eCheckSum:eIgnoring;
                            }
                            break;
                        default: state=eIgnoring; break;
                    }
                    break;
                case eCheckSum:
                    /*checksum=b;*/
                    state=eIgnoring;
                    break;
            }
        }
    }
    inline void updateTuning(int note,int retuneNote,double detune)
    {
        if (note<0 || note>127 || retuneNote<0 || retuneNote>127) return;
        retuning[note]=440.*pow(2.,((retuneNote+detune)-69.)/12.);
    }
    const char *getScaleName() {return global.isOnline() && global.GetScaleName?global.GetScaleName():tuningName;}
    
    enum eSysexState {eIgnoring=0,eMatchingSysex,eSysexValid,eMatchingMTS,eMatchingBank,eMatchingProg,eMatchingChannel,eTuningName,eNumTunings,eTuningData,eCheckSum};
    enum eMTSFormat {eRequest=0,eBulk,eSingle,eScaleOctOneByte,eScaleOctTwoByte,eScaleOctOneByteExt,eScaleOctTwoByteExt};

    double retuning[128];
    char tuningName[17];
    bool supportsNoteFiltering,supportsMultiChannelNoteFiltering,supportsMultiChannelTuning,freqRequestReceived,supportsMTSSysex;
};

static char freqToNoteET(double freq)
{
    static double freqs[128];static bool init=false;
    if (!init) {for (int i=0;i<128;i++) freqs[i]=440.*pow(2.,(i-69.)/12.);init=true;}
    if (freq<=freqs[0]) return 0;
    if (freq>=freqs[127]) return 127;
    int mid=0;int n=-1;int n2=-1;
    for (int first=0,last=127;freq!=freqs[(mid=first+(last-first)/2)];(freq<freqs[mid])?last=mid-1:first=mid+1) if (first>last)
    {
        if (!mid) {n=mid;break;}
        if (mid>127) mid=127;
        n=mid-((freq-freqs[mid-1])<(freqs[mid]-freq));
        break;
    }
    if (n==-1) {if (freq==freqs[mid]) n=mid;else return 60;}
    if (!n) n2=1;
    else if (n==127) n2=126;
    else n2=n+1*(fabs(freqs[n-1]-freq)<fabs(freqs[n+1]-freq)?-1:1);
    if (n2<n) {int t=n;n=n2;n2=t;}
    double fmid=freqs[n]*pow(2.,0.5*(log(freqs[n2]/freqs[n])/ln2));
    return freq<fmid?static_cast<char>(n):static_cast<char>(n2);
}

// Exported functions:
MTSClient* MTS_RegisterClient()                                                 {return new MTSClient;}
void MTS_DeregisterClient(MTSClient* c)                                         {delete c;}
bool MTS_HasMaster(MTSClient* c)                                                {return c?c->hasMaster():false;}
bool MTS_ShouldFilterNote(MTSClient* c,char midinote,char midichannel)          {return c?c->shouldFilterNote(midinote&127,midichannel):false;}
double MTS_NoteToFrequency(MTSClient* c,char midinote,char midichannel)         {return c?c->freq(midinote,midichannel):(1./global.iet[midinote&127]);}
double MTS_RetuningAsRatio(MTSClient* c,char midinote,char midichannel)         {return c?c->ratio(midinote,midichannel):1.;}
double MTS_RetuningInSemitones(MTSClient* c,char midinote,char midichannel)     {return c?c->semitones(midinote,midichannel):0.;}
char MTS_FrequencyToNote(MTSClient *c,double freq,char midichannel)             {return c?c->freqToNote(freq,midichannel):freqToNoteET(freq);}
char MTS_FrequencyToNoteAndChannel(MTSClient *c,double freq,char *midichannel)  {if (c) return c->freqToNote(freq,midichannel);if (midichannel) *midichannel=0;return freqToNoteET(freq);}
const char *MTS_GetScaleName(MTSClient *c)                                      {return c?c->getScaleName():"";}
void MTS_ParseMIDIDataU(MTSClient *c,const unsigned char *buffer,int len)       {if (c) c->parseMIDIData(buffer,len);}
void MTS_ParseMIDIData(MTSClient *c,const char *buffer,int len)                 {if (c) c->parseMIDIData(reinterpret_cast<const unsigned char*>(buffer),len);}

