/*
**  $Source: PRG:VFM/Classes/_ITimerClass/it_main.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/03/03 17:31:03 $
**  $Locker:  $
**  $Author: floh $
**
**  Die itimer.class ist vollständig virtuell, sämtliche
**  Funktionalität wird erst von den Subklassen implementiert.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>

#include "input/itimerclass.h"

/*-------------------------------------------------------------------
**  Class Header
*/
#ifdef AMIGA
__far ULONG itimer_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG itimer_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo itimer_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeITimerClass(ULONG id,...);
__geta4 BOOL FreeITimerClass(void);
#else
struct ClassInfo *MakeITimerClass(ULONG id,...);
BOOL FreeITimerClass(void);
#endif

struct GET_Class itimer_GET = {
    &MakeITimerClass,      /* MakeExternClass() */
    &FreeITimerClass,      /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&itimer_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *itimer_Entry(void)
{
    return(&itimer_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo itimer_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:itimer.class"
    },
    itimer_Entry,
};
#endif

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeITimerClass(ULONG id,...)
#else
struct ClassInfo *MakeITimerClass(ULONG id,...)
#endif
/*
**  CHANGES
**      19-Feb-96   floh    created
*/
{
    memset(itimer_Methods,0,sizeof(itimer_Methods));

    itimer_clinfo.superclassid = NUCLEUSCLASSID;
    itimer_clinfo.methods      = itimer_Methods;
    itimer_clinfo.instsize     = 0;   // keine eigene Instance Data
    itimer_clinfo.flags        = 0;

    /* und das war's */
    return(&itimer_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeITimerClass(void)
#else
BOOL FreeITimerClass(void)
#endif
/*
**  CHANGED
**      19-Feb-96   floh    created
*/
{
    return(TRUE);
}

