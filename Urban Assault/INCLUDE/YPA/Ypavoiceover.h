#ifndef YPA_YPAVOICEOVER_H
#define YPA_YPAVOICEOVER_H
/*
**  $Source: PRG:VFM/Include/ypa/ypavoiceover.h,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 14:31:43 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Voice-Over-Modul.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

/*-----------------------------------------------------------------*/
#define MAXNUM_VOICEOVERS   (1)     // max. parallel abgespielte Voiceovers

struct vo_Slot {
    LONG pri;                   // Priorität, -1 -> freier Slot
    struct SoundCarrier sc;
    Object *so;                 // das Sample-Object
    struct Bacterium *sender;
};

struct vo_VoiceOver {
    struct vo_Slot slot[MAXNUM_VOICEOVERS];
};

/*-----------------------------------------------------------------*/
#endif

