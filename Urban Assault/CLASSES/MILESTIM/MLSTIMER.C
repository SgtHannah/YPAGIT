/*
**  $Source: PRG:VFM/Classes/_MilesTimer/mlstimerclass.c,v $
**  $Revision: 38.2 $
**  $Date: 1996/10/05 18:36:19 $
**  $Locker:  $
**  $Author: floh $
**
**  Miles-AIL-kompatible TimeSource-Klasse für DOS.
**  Benutzt die Timer-Callback-Functions des Miles-API.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>

#include "nucleus/nucleus2.h"
#include "input/milestimerclass.h"

/*-----------------------------------------------------------------*/
void __pascal mt_callback(U32);
_dispatcher(ULONG, mt_ITIM_ELAPSEDTIME, void *);

/*-----------------------------------------------------------------*/
ULONG mt_TimeStamp = 0;
HTIMER mt_TimerHandle = NULL;

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG mt_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo mt_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *MakeMilesTimerClass(ULONG id,...);
BOOL FreeMilesTimerClass(void);

struct GET_Class mt_GET = {
    &MakeMilesTimerClass,
    &FreeMilesTimerClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *mt_Entry(void)
{
    return(&mt_GET);
};

struct SegInfo milestimer_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/milestimer.class"
    },
    mt_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *MakeMilesTimerClass(ULONG id,...)
/*
**  CHANGED
**      19-Apr-96   floh    created
*/
{
    /*** Global Data initialisieren ***/
    mt_TimeStamp = 0;

    /*** Callback-Funktion einklinken, initialisieren und starten ***/
    mt_TimerHandle = AIL_register_timer(mt_callback);
    if (mt_TimerHandle != -1) {
        AIL_set_timer_frequency(mt_TimerHandle, 200);   // 200 Hz
        AIL_start_timer(mt_TimerHandle);
    } else {
        _LogMsg("milestimer.class: AIL_register_timer() failed!\n");
        return(NULL);
    };

    /*** Klasse initialisieren ***/
    memset(mt_Methods, 0, sizeof(mt_Methods));

    mt_Methods[ITIM_ELAPSEDTIME] = (ULONG) mt_ITIM_ELAPSEDTIME;

    mt_clinfo.superclassid = ITIMER_CLASSID;
    mt_clinfo.methods      = mt_Methods;
    mt_clinfo.instsize     = sizeof(struct milestimer_data);
    mt_clinfo.flags        = 0;

    return(&mt_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL FreeMilesTimerClass(void)
/*
**  CHANGED
**      19-Apr-96   floh    created
*/
{
    AIL_release_timer_handle(mt_TimerHandle);
    return(TRUE);
}

/*=================================================================**
**  SUPPORT FUNKTIONEN                                             **
**=================================================================*/

/*-----------------------------------------------------------------*/
void __pascal mt_callback(U32 user)
/*
**  FUNCTION
**      Callback-Funktion, die mit 200Hz aufgerufen wird.
**      Die mt_TimeStamp Variable wird einfach um 5
**      erhöht (5, wie 5 Millisekunden).
**
**  CHANGED
**      19-Apr-96   floh    created
*/
{
    mt_TimeStamp += 5;
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, mt_ITIM_ELAPSEDTIME, void *nul)
/*
**  CHANGED
**      19-Apr-96   floh    created
*/
{
    struct milestimer_data *md = INST_DATA(cl,o);
    ULONG elapsed;
    ULONG act_time;

    /*** lese aktuellen globalen TimeStamp ***/
    act_time = mt_TimeStamp;

    /*** Elapsed Time ermitteln (auf 1, falls 1.Aufruf) ***/
    if (0 == md->timer_store) elapsed = 1;
    else {
        elapsed = act_time - md->timer_store;
        if (0 == elapsed) elapsed = 1;
    };
    md->timer_store = act_time;
    return(elapsed);
}

