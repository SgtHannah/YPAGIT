#ifndef AUDIO_MILESENGINE_H
#define AUDIO_MILESENGINE_H
/*
**  $Source: PRG:VFM/Include/audio/milesengine.h,v $
**  $Revision: 38.5 $
**  $Date: 1997/02/19 23:45:34 $
**  $Locker:  $
**  $Author: floh $
**
**  Auf das Miles-AIL-Treibersystem aufbauende VFM-Audio-Engine.
**
**  (C) Copyright 1996 by A.Weissflog
*/

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

#ifndef AUDIO_SOUNDKIT_H
#include "audio/soundkit.h"
#endif

/*-----------------------------------------------------------------*/

#define MILES_MAXCHANNELS   (16)
#define MILES_NUMPALFX      (8) // max. Anzahl paralleler Paletten-FX
#define MILES_NUMSHAKEFX    (4) // max. Anzahl paralleler Shake-FX
#define MILES_RNDTABLE_ENTRIES  (64)

struct Channel {
    snd_Channel *handle;      // Sample-Handle
    struct SoundSource *src;  // momentan abgespielte SoundSource
};

struct MilesBase {

    snd_Sound *driver;      // der aktive Digital-Sound-Driver

    ULONG master_volume;    // die globale Lautstärke
    ULONG num_channels;     // Anzahl <channel>'s (<= MILES_MAXCHANNELS)
    ULONG num_palfx;        // Anzahl Paletten-FX-Channels
    ULONG num_shakefx;      // Anzahl Shake-FX-Channels

    struct Channel ch[MILES_MAXCHANNELS];   // die "echten" Sound-Kanäle

    struct SoundSource *top_n[MILES_MAXCHANNELS];   // Top-N Sounds je Frame
    struct SoundSource *top_palfx[MILES_NUMPALFX];  // Top-N Pal-FX je Frame
    struct SoundSource *top_shakefx[MILES_NUMSHAKEFX];  // Top-N Shake-FX je Frame

    ULONG rev_stereo;           // Stereo invertiert
    ULONG palfx_clean;          // gesetzt, wenn momentane Palette "clean"
    ULONG time_stamp;           // time_stamp des aktuellen Frames

    ULONG act_rnd;
    FLOAT rnd_table[MILES_RNDTABLE_ENTRIES];

    struct flt_triple vwr_pos;  // Viewer-Position
    struct flt_triple vwr_vec;  // Viewer-Geschwindigkeit
    struct flt_m3x3   vwr_mx;   // Viewer-Rotations-Matrix (wird von Shake-FX modifiziert!)
    struct flt_m3x3   shk_mx;   // Shake-Rotations-Matrix
};

/*-----------------------------------------------------------------*/
#endif



