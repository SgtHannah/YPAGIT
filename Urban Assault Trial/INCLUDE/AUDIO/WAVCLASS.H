#ifndef AUDIO_WAVCLASS_H
#define AUDIO_WAVCLASS_H
/*
**  $Source: PRG:VFM/Include/audio/wavclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1996/04/18 23:35:15 $
**  $Locker:  $
**  $Author: floh $
**
**  Wav-Loader-Klasse.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef AUDIO_SAMPLECLASS_H
#include "audio/sampleclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      wav.class - Loader Klasse für WAV Format
**
**  FUNCTION
**      Erzeugt ein Audio-Sample-Object aus einem WAV-File,
**      ist Subklasse der sample.class.
**
**  METHODS
**      keine
**
**      Die wav.class unterstützt NICHT RSM_SAVE!
**
**  ATTRIBUTES
**      keine
**
**-----------------------------------------------------------------*/

#define WAV_CLASSID "wav.class"

/*-----------------------------------------------------------------*/
#define WAVM_BASE       (SMPM_BASE+METHOD_DISTANCE)
/*-----------------------------------------------------------------*/
#define WAVA_BASE       (SMPA_BASE+ATTRIB_DISTANCE)
/*-------------------------------------------------------------------
**  Definitionen für WAV-Format
*/
#define WAV_FMT_ID  MAKE_ID('f','m','t',' ')
#define WAV_DATA_ID MAKE_ID('d','a','t','a')

struct RiffHeader {
    ULONG rh_id;      // 'RIFF'
    ULONG rh_len;     // Länge des Chunks ab "WAVE"
    ULONG rh_type;    // 'WAVE'
};

struct RiffChunk {
    ULONG rc_id;
    ULONG rc_len;
};

struct WavFormat {
    UWORD wFormatTag;       // 1 = Pulse Code Modulation
    UWORD nChannels;        // 1 = mone, 2 = stereo
    ULONG nSamplesPerSec;   // Playback frequency
    ULONG nAvgBytesPerSec;  // nChannels * nSamplesPerSec * (nBitsPerSample/8)
    UWORD nBlockAlign;
    UWORD FormatSpecific;
};

#define WAV_FORMAT_PCM  (1)

/*-----------------------------------------------------------------*/
#endif

