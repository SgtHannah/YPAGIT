/*
**  $Source: PRG:VFM/Classes/_WinTimerClass/wint_main.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 15:11:44 $
**  $Locker:  $
**  $Author: floh $
**
**  Timer unter Windows.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include "nucleus/nucleus2.h"
#include "input/wintimerclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, wit_ITIM_ELAPSEDTIME, void *);

extern ULONG wit_GetTickCount(void);

/*-----------------------------------------------------------------*/
ULONG wit_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo wit_clinfo;

struct ClassInfo *wit_MakeClass(ULONG id,...);
BOOL wit_FreeClass(void);

struct GET_Class wit_GET = {
    wit_MakeClass,
    wit_FreeClass,
};

struct GET_Class *wit_Entry(void)
{
    return(&wit_GET);
};

struct SegInfo wit_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/wintimer.class",
    },
    wit_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *wit_MakeClass(ULONG id,...)
/*
**  CHANGED
**      21-Nov-96   floh    created
*/
{
    /*** Klasse initialisieren ***/
    memset(wit_Methods,0,sizeof(wit_Methods));

    wit_Methods[ITIM_ELAPSEDTIME] = (ULONG) wit_ITIM_ELAPSEDTIME;

    wit_clinfo.superclassid = ITIMER_CLASSID;
    wit_clinfo.methods      = wit_Methods;
    wit_clinfo.instsize     = sizeof(struct wintimer_data);
    wit_clinfo.flags        = 0;

    return(&wit_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL wit_FreeClass(void)
/*
**  CHANGED
**      21-Nov-96   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  Methoden-Dispatcher                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, wit_ITIM_ELAPSEDTIME, void *ignore)
/*
**  CHANGED
**      21-Nov-96   floh    created
*/
{
    struct wintimer_data *wid = INST_DATA(cl,o);
    ULONG tick;
    LONG elapsed;

    /*** Windows-Gummizellen-Routine ***/
    tick = wit_GetTickCount();
    elapsed = tick - wid->old_tickcount;
    wid->old_tickcount = tick;

    /*** Overflow bzw. Sleep-Periode abfangen ***/
    if ((elapsed<0) || (elapsed>2000)) elapsed=1;

    /*** fini ***/
    return(elapsed);
}

