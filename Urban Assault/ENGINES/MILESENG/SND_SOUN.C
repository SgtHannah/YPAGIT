/*
**  $Source: PRG:VFM/Engines/MilesEngine/snd_soundkit.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 16:40:35 $
**  $Locker: floh $
**  $Author: floh $
**
**  SoundKit, kapselt die DOS-/Win-Version von AIL in ein
**  einheitliches Interface.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include "thirdparty/mss.h"

#ifndef NULL
#define NULL (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#include "audio/soundkit.h"

/*-----------------------------------------------------------------*/
snd_Sound *snd_InitAudio(long num_channels)
/*
**  FUNCTION
**      Initialisierung des Soundkits sollte so vor sich gehen:
**
**      snd_InitAudio() -> loop snd_AllocChannel() -> snd_SetMasterVolume()
**
**  INPUTS
**      num_channels - Anzahl gewünschter Mixerkanäle (<16)
**
**  RESULTS
**      snd_Sound   - abstraktes SoundKit-Handle
**
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    snd_Sound *snd = NULL;

    AIL_startup();

    #ifdef IS_WIN32
    {
        static PCMWAVEFORMAT sPCMWF;
        DWORD r;

        /*** WINDOWS Initialisierung (benutzt immer DirectSound) ***/
        AIL_set_preference(DIG_MIXER_CHANNELS, num_channels);
        AIL_set_preference(DIG_USE_WAVEOUT,NO);

        sPCMWF.wf.wFormatTag      = WAVE_FORMAT_PCM;
        sPCMWF.wf.nChannels       = 2;
        sPCMWF.wf.nSamplesPerSec  = SND_FREQ;
        sPCMWF.wf.nAvgBytesPerSec = SND_FREQ * 2;
        sPCMWF.wf.nBlockAlign     = 2;
        sPCMWF.wBitsPerSample     = SND_BITS;
        r = AIL_waveOutOpen((HDIGDRIVER FAR *) &snd,
                            NULL,
                            WAVE_MAPPER,
                            (LPWAVEFORMAT) &sPCMWF);
        /*** Fehler, wenn (r != 0) ***/
    };
    #else
    {
        /*** DOS Initialisierung ***/
        char old_path[256];

        AIL_set_preference(DIG_MIXER_CHANNELS, num_channels);
        AIL_set_preference(DIG_USE_16_BITS, NO);
        AIL_set_preference(DIG_USE_STEREO,  YES);
        AIL_set_preference(DIG_HARDWARE_SAMPLE_RATE, NOM_VAL);

        /*** DOS-Treiber installieren ***/
        getcwd(old_path,sizeof(old_path));
        chdir("DRIVERS");
        if (AIL_install_DIG_INI((HDIGDRIVER) &snd) != AIL_INIT_SUCCESS) {
            /*** hat nicht geklappt, kein Sound... ***/
            chdir(old_path);
            return(TRUE);
        };
        chdir(old_path);
    };
    #endif

    return(snd);
}

/*-----------------------------------------------------------------*/
void snd_KillAudio(snd_Sound *snd)
/*
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    if (snd) {
        #ifdef IS_WIN32
        {
            AIL_shutdown();
        };
        #else
        {
            AIL_shutdown();
        };
        #endif
    };
}

/*-----------------------------------------------------------------*/
snd_Sound *snd_ModifyNumChannels(snd_Sound *snd, long num_channels)
/*
**  FUNCTION
**      Wegen dem DOS-Timer-Service der milestimer.class
**      kann ich die AIL nicht einfach downshutten und
**      wieder initialisieren (dann ist der Timer-
**      Service im A*sch). Die App muß nach Aufruf von
**      snd_ModifyNumChannels ihre Channel-Handles neu
**      allokieren und das MasterVolume neu setzen!
**
**      Das alte SoundHandle wird ungültig, und muß
**      durch das zurückgelieferte ersetzt werden!
**
**  CHANGED
**      19-Feb-97   floh    created
*/
{
    HDIGDRIVER old_hd  = (HDIGDRIVER) snd;
    HDIGDRIVER *new_hd = NULL;

    #ifdef IS_WIN32
    {
        /*** WINDOWS ***/
        static PCMWAVEFORMAT sPCMWF;
        DWORD r;

        /*** WINDOWS Initialisierung (benutzt immer DirectSound) ***/
        AIL_waveOutClose(old_hd);

        AIL_set_preference(DIG_MIXER_CHANNELS, num_channels);
        AIL_set_preference(DIG_USE_WAVEOUT,NO);

        sPCMWF.wf.wFormatTag      = WAVE_FORMAT_PCM;
        sPCMWF.wf.nChannels       = 2;
        sPCMWF.wf.nSamplesPerSec  = SND_FREQ;
        sPCMWF.wf.nAvgBytesPerSec = SND_FREQ * 2;
        sPCMWF.wf.nBlockAlign     = 2;
        sPCMWF.wBitsPerSample     = SND_BITS;
        r = AIL_waveOutOpen((HDIGDRIVER FAR *) &new_hd,
                            NULL,
                            WAVE_MAPPER,
                            (LPWAVEFORMAT) &sPCMWF);
        /*** Fehler, wenn (r != 0) ***/
    };
    #else
    {
        /*** DOS ***/
        char old_path[256];

        /*** Driver muß neu initialisiert werden ***/
        AIL_uninstall_DIG_driver(old_hd);
        AIL_set_preference(DIG_MIXER_CHANNELS, num_channels);
        AIL_set_preference(DIG_USE_16_BITS, NO);
        AIL_set_preference(DIG_USE_STEREO,  YES);
        AIL_set_preference(DIG_HARDWARE_SAMPLE_RATE, MIN_VAL);
        AIL_set_preference(DIG_MIXER_CHANNELS, num_channels);
        getcwd(old_path,sizeof(old_path));
        chdir("DRIVERS");
        AIL_install_DIG_INI(&(new_hd));
        chdir(old_path);
    };
    #endif

    return((snd_Sound *)new_hd);
}

/*-----------------------------------------------------------------*/
void snd_SetMasterVolume(snd_Sound *snd, long vol)
/*
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    if (snd) {
        HDIGDRIVER h = (HDIGDRIVER) snd;
        AIL_set_digital_master_volume(h,vol);
    };
}

/*-----------------------------------------------------------------*/
snd_Channel *snd_AllocChannel(snd_Sound *snd)
/*
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    HSAMPLE s = NULL;
    if (snd) {
        HDIGDRIVER h = (HDIGDRIVER) snd;
        s = AIL_allocate_sample_handle(snd);
        if (s) AIL_init_sample(s);
    };
    return((snd_Channel *)s);
}

/*-----------------------------------------------------------------*/
void snd_FreeChannel(snd_Sound *snd, snd_Channel *c)
/*
**  CHANGED
**      18-Feb-97   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
void AILCALLBACK snd_EOS_CallBack(HSAMPLE channel)
/*
**  FUNCTION
**      EndOfSample Callback-Routine, ruft einfach die
**      externe "Custom-Callback-Routine" auf.
**
**  CHANGED
**      19-Feb-97   floh    created
*/
{
    /*** user_data[0] enthält Pointer auf Callback-Routine ***/
    if (channel->user_data[0]) {
        void (*eos_callback) (snd_Channel *) = channel->user_data[0];
        eos_callback((snd_Channel *)channel);
    };
}

/*-----------------------------------------------------------------*/
void snd_StartChannel(snd_Sound *snd, snd_Channel *c,
                      void (*eos_callback) (snd_Channel *),
                      void *address,
                      long len,
                      long pitch,
                      long volume,
                      long pan,
                      long loop_count,
                      long offset)
/*
**  FUNCTION
**      Startet ein neues Sample auf dem übergebenen
**      Kanal.
**
**  INPUTS
**      snd_Sound       - SoundKit-Handle
**      snd_Channel     - Channel-Handle
**      eos_callback    - EndOfSample Callback-Funktion
**      address         - Startadresse des Samples
**      len             - Länge des Samples
**      pitch           - Pitch des Samples
**      volume          - Lautstärke des Samples
**      pan             - Stereo-Position des Samples
**      loop_count      - Loopcount des Samples
**      offset          - Start-Offset ins Samples
**
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    if (snd) {
        HSAMPLE s = (HSAMPLE) c;
        s->user_data[0] = (S32) eos_callback;
        AIL_init_sample(s);
        AIL_register_EOS_callback(s,(AILSAMPLECB)snd_EOS_CallBack);
        AIL_set_sample_address(s,address,len);
        AIL_set_sample_type(s,DIG_F_MONO_8,0);
        AIL_set_sample_playback_rate(s,pitch);
        AIL_set_sample_volume(s,volume);
        AIL_set_sample_pan(s,pan);
        AIL_set_sample_loop_count(s,loop_count);
        AIL_start_sample(s);
        if (offset > 0) AIL_set_sample_position(s,offset);
    };
}

/*-----------------------------------------------------------------*/
void snd_EndChannel(snd_Sound *snd, snd_Channel *c)
/*
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    if (snd) {
        HSAMPLE s = (HSAMPLE) c;
        AIL_end_sample(s);
    };
}

/*-----------------------------------------------------------------*/
void snd_ModifyChannel(snd_Sound *snd, snd_Channel *c,
                       long pitch,
                       long volume,
                       long pan)
/*
**  CHANGED
**      18-Feb-97   floh    created
*/
{
    if (snd) {
        HSAMPLE s = (HSAMPLE) c;
        AIL_set_sample_playback_rate(s,pitch);
        AIL_set_sample_volume(s,volume);
        AIL_set_sample_pan(s,pan);
    };
}

