#ifndef AUDIO_NOSOUNDENGINE_H
#define AUDIO_NOSOUNDENGINE_H
/*
**  $Source: PRG:VFM/Include/audio/nosoundengine.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/11/26 01:50:51 $
**  $Locker:  $
**  $Author: floh $
**
**  Dummy-Sound-Engine, ohne Audio, aber mit Shake- und
**  Paletten-FX.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

/*-----------------------------------------------------------------*/

#define NOSND_MAXCHANNELS   (16)
#define NOSND_NUMPALFX      (8) // max. Anzahl paralleler Paletten-FX
#define NOSND_NUMSHAKEFX    (4) // max. Anzahl paralleler Shake-FX
#define NOSND_RNDTABLE_ENTRIES  (64)

struct NoSoundBase {

    ULONG num_palfx;        // Anzahl Paletten-FX-Channels
    ULONG num_shakefx;      // Anzahl Shake-FX-Channels

    struct SoundSource *top_palfx[NOSND_NUMPALFX];  // Top-N Pal-FX je Frame
    struct SoundSource *top_shakefx[NOSND_NUMSHAKEFX];  // Top-N Shake-FX je Frame

    ULONG palfx_clean;          // gesetzt, wenn momentane Palette "clean"
    ULONG time_stamp;           // time_stamp des aktuellen Frames

    ULONG act_rnd;
    FLOAT rnd_table[NOSND_RNDTABLE_ENTRIES];

    struct flt_triple vwr_pos;  // Viewer-Position
    struct flt_triple vwr_vec;  // Viewer-Geschwindigkeit
    struct flt_m3x3   vwr_mx;   // Viewer-Rotations-Matrix (wird von Shake-FX modifiziert!)
    struct flt_m3x3   shk_mx;   // Shake-Rotations-Matrix
};

/*-----------------------------------------------------------------*/
#endif



