/*
**  $Source: PRG:VFM/Classes/_IDevClass/id_main.c,v $
**  $Revision: 38.2 $
**  $Date: 1996/06/15 18:22:19 $
**  $Locker:  $
**  $Author: floh $
**
**  Die idev.class ist vollständig "virtuell", sämtliche
**  Funktionalität wird erst von den Subklassen
**  implementiert.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>

#include "input/idevclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(void, idev_IDEVM_RESET, struct idev_reset_msg *);

/*-------------------------------------------------------------------
**  Class Header
*/
#ifdef AMIGA
__far ULONG idev_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG idev_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo idev_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeIDevClass(ULONG id,...);
__geta4 BOOL FreeIDevClass(void);
#else
struct ClassInfo *MakeIDevClass(ULONG id,...);
BOOL FreeIDevClass(void);
#endif

struct GET_Class idev_GET = {
    &MakeIDevClass,      /* MakeExternClass() */
    &FreeIDevClass,      /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&idev_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *idev_Entry(void)
{
    return(&idev_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo idev_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:idev.class"   /* der Segment-Name */
    },
    idev_Entry,                 /* Entry()-Adresse */
};
#endif

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeIDevClass(ULONG id,...)
#else
struct ClassInfo *MakeIDevClass(ULONG id,...)
#endif
/*
**  CHANGES
**      19-Feb-96   floh    created
*/
{
    memset(idev_Methods,0,sizeof(idev_Methods));

    idev_Methods[IDEVM_RESET]   = (ULONG) idev_IDEVM_RESET;

    idev_clinfo.superclassid = NUCLEUSCLASSID;
    idev_clinfo.methods      = idev_Methods;
    idev_clinfo.instsize     = 0;   // keine eigene Instance Data
    idev_clinfo.flags        = 0;

    /* und das war's */
    return(&idev_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeIDevClass(void)
#else
BOOL FreeIDevClass(void)
#endif
/*
**  CHANGED
**      19-Feb-96   floh    created
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, idev_IDEVM_RESET, struct idev_reset_msg *msg)
/*
**  FUNCTION
**      Fängt IDEVM_RESET Anwendungen ab, wenn eine Subklasse
**      die Methode ignoriert.
**
**  CHANGED
**      15-Jun-96   floh    created
*/
{ }
