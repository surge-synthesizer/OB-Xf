/*
Copyright (C) 2021 by ODDSound Ltd. info@oddsound.com

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

#ifndef libMTSClient_h
#define libMTSClient_h

#ifdef __cplusplus
extern "C" {
#endif
    
    /*
     Steps for using the MTS-ESP client API to add microtuning support to a plug-in.
     Steps 1 and 2 are required, however it is recommended to include further steps when
     integrating:
     
     
     1. REQUIRED: Register and de-register a plug-in instance as a client with MTS-ESP.
     On startup in the plug-in constructor call:

        MTSClient *client = MTS_RegisterClient();

     Store the returned MTSClient pointer to supply when calling other MTS-ESP client API
     functions. On shutdown in the plug-in destructor call:

        MTS_DeregisterClient(client);

     
     2. REQUIRED: Query retuning when a note-on message is received and adjust tuning accordingly.
     When given a note call:

        double freq = MTS_NoteToFrequency(client, midinote, midichannel);
     OR
        double retune_semitones = MTS_RetuningInSemitones(client, midinote, midichannel);
     OR
        double retune_ratio = MTS_RetuningAsRatio(client, midinote, midichannel);

     MIDI channel arguments should use the range [0,15] however if you don’t know the MIDI
     channel, use -1 (see step 6 for more on MIDI channels).

     
     3. RECOMMENDED: Continuously query retuning whilst a note is held, allowing tuning to change
     along the flight of a note. Do this if you can and as often as possible, ideally at the same
     time as processing any other pitch modulation sources (envelopes, MIDI controllers, LFOs etc.).

     
     4. RECOMMENDED: Provide an option to the user to select whether tuning is queried at note-on
     only, as in step 2, or continuously, as in step 3. There are creative and practical
     advantages to both, depending on the use case, and offering an option to the user will
     provide the most useful MTS-ESP integration. If not offering such an option, continuous
     retuning should be preferred over note-on only retuning.
     
     
     5. RECOMMENDED: Query whether a note should be sounded when a note-on message is received.
     The Scala .kbm keyboard mapping format allows for MIDI keys to be unmapped i.e. no frequency
     is specified for them, and the MTS-ESP library supports this too. You can query whether a note
     is unmapped and should be ignored with:
     
        bool should_ignore_note = MTS_ShouldFilterNote(client, midinote, midichannel);
     
     If this returns true, ignore the note-on and don’t play anything. Calling this function is
     recommended but optional and a valid value for frequency/retuning will be returned for an
     unmapped note. MIDI channel arguments should use the range [0,15] however if you don’t
     know the MIDI channel, use -1.
     
     
     6. RECOMMENDED: Always supply a MIDI channel when querying retuning or note filtering. Doing
     so allows your plug-in to multi-channel tuning tables, useful for microtonal MIDI controllers
     with more than 128 keys or working with large scales. Even if multi-channel tables are not
     in use, a master may still make use of channel-specific note filtering for functions such as
     key switches to change tunings. If your plug-in supports MPE and has a switch for enabling MPE
     support, it is recommended to NOT supply a MIDI channel if MPE is enabled.
     
     
     7. RECOMMENDED: If you are adding MTS-ESP support to a plug-in that already has some kind
     of microtuning support, e.g. loading .scl or .tun files, let the local tuning automatically
     override MTS-ESP, or provide an option for MTS-ESP retuning to be explicitly disabled.
     This affords a user the option to use a different tuning to the global MTS-ESP table
     for a specific plug-in instance.
     
     
     8. OPTIONAL: Add support for MIDI Tuning Standard (or MTS, from the MIDI specification) SysEx
     messages to your plug-in. When not connected to an MTS-ESP master plug-in, these can be used
     to retune it instead, providing microtuing support even when MTS-ESP is not in use.
     When a SysEx message is received, call:
     
        MTS_ParseMIDIData(client, buffer, len); // if buffer is signed char *
     OR
        MTS_ParseMIDIDataU(client, buffer, len); // if buffer is unsigned char *
     
     These will update a local tuning table which is used when querying retuning as in steps 2
     and 3.
     
     
     9. OPTIONAL: If you want to display to the user whether the plug-in is "connnected" to an
     MTS-ESP master plug-in, call:
     
        bool has_master = MTS_HasMaster(client);
     
     
     10: OPTIONAL: It is possible to query the name of the current scale. This function is necessarily
     supplied for the case where a client is sending MTS SysEx messages, however it can be used
     to display the current scale name to the user on your UI too:
     
        const char *name = MTS_GetScaleName(client);
     
     
     11: EXTRAS: Helper functions are available which return the MIDI note whose pitch is nearest
     a given frequency. The MIDI note returned is guaranteed to be mapped. If you intend to
     generate a note-on message using the returned note number, you may already know which MIDI
     channel it will be sent on, in which case you must specify this in the call, else the client
     library can prescribe a channel for you. This is done so that multi-channel mapping
     and note filtering can be respected. See below for further details.
     */
    
    // Opaque datatype for MTSClient.
    typedef struct MTSClient MTSClient;

    // Register/deregister as a client. Call from the plugin constructor and destructor.
    extern MTSClient *MTS_RegisterClient();
    extern void MTS_DeregisterClient(MTSClient *client);

    // Check if the client is currently connected to a master plugin.
    extern bool MTS_HasMaster(MTSClient *client);

    // Returns true if note should not be played. MIDI channel argument should be included if possible (0-15), else set to -1.
    extern bool MTS_ShouldFilterNote(MTSClient *client, char midinote, char midichannel);

    // Retuning a midi note. Pick the version that makes your life easiest! MIDI channel argument should be included if possible (0-15), else set to -1.
    extern double MTS_NoteToFrequency(MTSClient *client, char midinote, char midichannel);
    extern double MTS_RetuningInSemitones(MTSClient *client, char midinote, char midichannel);
    extern double MTS_RetuningAsRatio(MTSClient *client, char midinote, char midichannel);
    
    // MTS_FrequencyToNote() is a helper function returning the note number whose pitch is closest to the supplied frequency. Two versions are provided:
    // The first is for the simplest case: supply a frequency and get a note number back.
    // If you intend to use the returned note number to generate a note-on message on a specific, pre-determined MIDI channel, set the midichannel argument to the destination channel (0-15), else set to -1.
    // If a MIDI channel is supplied, the corresponding multi-channel tuning table will be queried if in use, else multi-channel tables are ignored.
    extern char MTS_FrequencyToNote(MTSClient *client, double freq, char midichannel);
    // Use the second version if you intend to use the returned note number to generate a note-on message and where you have the possibility to send it on any MIDI channel.
    // The midichannel argument is a pointer to a char which will receive the MIDI channel on which the note message should be sent (0-15).
    // Multi-channel tuning tables are queried if in use.
    extern char MTS_FrequencyToNoteAndChannel(MTSClient *client, double freq, char *midichannel);
    
    // Returns the name of the current scale.
    extern const char *MTS_GetScaleName(MTSClient *client);

    // Parse incoming MIDI data to update local retuning. All formats of MTS sysex message accepted.
    extern void MTS_ParseMIDIDataU(MTSClient *client, const unsigned char *buffer, int len);
    extern void MTS_ParseMIDIData(MTSClient *client, const char *buffer, int len);

#ifdef __cplusplus
}
#endif

#endif

