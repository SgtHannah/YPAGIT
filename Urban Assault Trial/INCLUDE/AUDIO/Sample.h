#ifndef AUDIO_SAMPLE_H
#define AUDIO_SAMPLE_H
/*
**  $Source: PRG:VFM/Include/audio/sample.h,v $
**  $Revision: 38.2 $
**  $Date: 1996/04/18 23:34:39 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition der Storage-Struktur für Audio-Sample-Data.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-----------------------------------------------------------------*/
struct VFMSample {
    APTR Data;              // Ptr auf Sample-Buffer
    ULONG Length;           // Länge des Sample-Buffer in Byte
    ULONG Type;             // Format des Sample-Puffers
    ULONG SamplesPerSec;    // Recording-Sample-Rate des Samples
    ULONG Flags;            // siehe unten
};

/*** VFMSample.Type ***/
#define SMPT_8BIT_MONO    (1) // 8 Bit Mono Sample
#define SMPT_8BIT_STEREO  (2) // 8 Bit Stereo (noch nicht unterstützt)
#define SMPT_16BIT_MONO   (3) // 16 Bit Mono (noch nicht unterstützt)
#define SMPT_16BIT_STEREO (4) // 16 Bit Stereo (noch nicht unterstützt)

#define VSMPF_AlienData   (1<<0)    // VFMSample.Data nicht von mir allokiert

/*-----------------------------------------------------------------*/
#endif


