#ifndef AUDIO_SOUNDKIT_H
#define AUDIO_SOUNDKIT_H
/*
**  $Source: PRG:VFM/Include/audio/soundkit.h,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 12:51:12 $
**  $Locker:  $
**  $Author: floh $
**
**  Wegen den Typkollisions-Kram und einigen Detail-Unterschieden
**  zwischen der DOS- und Windows-Version des "Miles Sound System"
**  packe ich das Zeug in ein abstraktes Modul, namens
**  soundkit.c.
**
**  (C) Copyright 1997 by A.Weissflog
*/

/*** Typedefs ***/
typedef void snd_Sound;
typedef void snd_Channel;

/*** Konstanten ***/
#define SND_FREQ (22050)    // Hardware-Sample-Frequenz
#define SND_BITS (8)        // Bits Pro Sample

/*** Prototypes ***/
snd_Sound *snd_InitAudio(long num_channels);
void snd_KillAudio(snd_Sound *);
snd_Sound *snd_ModifyNumChannels(snd_Sound *, long num_channels);
void snd_SetMasterVolume(snd_Sound *, long);
snd_Channel *snd_AllocChannel(snd_Sound *);
void snd_FreeChannel(snd_Sound *, snd_Channel *);
void snd_StartChannel(snd_Sound *, snd_Channel *,
                      void (*eos_callback) (snd_Channel *),
                      void *address,
                      long len,
                      long pitch,
                      long volume,
                      long pan,
                      long loop_count,
                      long offset);
void snd_EndChannel(snd_Sound *, snd_Channel *);
void snd_ModifyChannel(snd_Sound *, snd_Channel *,
                       long pitch,
                       long volume,
                       long pan);

/*-----------------------------------------------------------------*/
#endif

