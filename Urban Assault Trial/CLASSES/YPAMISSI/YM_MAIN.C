/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_main.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/08 23:12:41 $
**  $Locker: floh $
**  $Author: floh $
**
**  Main-Modul der yparocket.class.
**  Die zweite "Luftklasse"
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "engine/engine.h"
#include "types.h"
#include "modules.h"

#include "transform/te.h"

#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypamissileclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** ym_main.c ***/
_dispatcher(Object *, ym_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, ym_OM_DISPOSE, void *ignored);
_dispatcher(void, ym_OM_SET, struct TagItem *attrs);
_dispatcher(void, ym_OM_GET, struct TagItem *attrs);


/*** ym_intelligence.c ***/
_dispatcher(void, ym_YBM_AI_LEVEL1, struct trigger_logic_msg *msg);
_dispatcher(void, ym_YBM_AI_LEVEL2, struct trigger_logic_msg *msg);
_dispatcher(void, ym_YBM_AI_LEVEL3, struct trigger_logic_msg *msg);
_dispatcher(void, ym_YBM_MOVE, struct move_msg *move);
_dispatcher(void, ym_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);
_dispatcher(void, ym_YMM_ALIGNMISSILE_S, void *nix);
_dispatcher(void, ym_YMM_ALIGNMISSILE_V, struct alignmissile_msg *am);

/*** ym_support.c ***/
_dispatcher( void, ym_YMM_RESETVIEWER, void *nix );
_dispatcher( void, ym_YMM_DOIMPULSE, void *nix );
_dispatcher( void, ym_YBM_REINCARNATE, void *nix );

/*** ym_condition.c ***/
_dispatcher( void, ym_YBM_SETSTATE, struct setstate_msg *state );
_dispatcher( void, ym_YBM_SETSTATE_I, struct setstate_msg *state );


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL ym_initAttrs(Object *, struct ypamissile_data *, struct TagItem *);
void ym_setAttrs(Object *, struct ypamissile_data *, struct TagItem *);
void ym_getAttrs(Object *, struct ypamissile_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_audio_engine

#ifdef AMIGA
__far ULONG ym_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG ym_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo ym_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYMClass(ULONG id,...);
__geta4 BOOL FreeYMClass(void);
#else
struct ClassInfo *MakeYMClass(ULONG id,...);
BOOL FreeYMClass(void);
#endif

struct GET_Class ym_GET = {
    &MakeYMClass,                 /* MakeExternClass() */
    &FreeYMClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&ym_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *ym_Entry(void)
{
    return(&ym_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo ym_class_seg = {
    { NULL, NULL,                       /* ln_Succ, ln_Pred */
      0, 0,                             /* ln_Type, ln_Pri  */
      "MC2classes:ypamissile.class"        /* der Segment-Name */
    },
    ym_Entry,                       /* Entry()-Point */
};
#endif

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
/*-------------------------------------------------------------------
**  Logischerweise kann der NUC_GET-Pointer nicht mit einem
**  _GetTagData() aus dem Nucleus-Kernel ermittelt werden,
**  weil NUC_GET noch nicht initialisiert ist! Deshalb hier eine
**  handgestrickte Routine.
*/
struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)
{
    register ULONG act_tag;

    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {
        switch (act_tag) {
            case TAG_DONE:  return(NULL); break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;
            case TAG_SKIP:  tagList += tagList->ti_Data; break;
        };
        tagList++;
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYMClass(ULONG id,...)
#else
struct ClassInfo *MakeYMClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet ypazepp.class im Nucleus-System an.
**
**  INPUTS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      17-Aug-95   8100000C    created
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    /* Methoden-Array initialisieren */
    memset(ym_Methods,0,sizeof(ym_Methods));

    ym_Methods[OM_NEW]              = (ULONG) ym_OM_NEW;
    ym_Methods[OM_DISPOSE]          = (ULONG) ym_OM_DISPOSE;
    ym_Methods[OM_SET]              = (ULONG) ym_OM_SET;
    ym_Methods[OM_GET]              = (ULONG) ym_OM_GET;

    ym_Methods[YBM_AI_LEVEL1]       = (ULONG) ym_YBM_AI_LEVEL1;
    ym_Methods[YBM_AI_LEVEL2]       = (ULONG) ym_YBM_AI_LEVEL2;
    ym_Methods[YBM_AI_LEVEL3]       = (ULONG) ym_YBM_AI_LEVEL3;
    ym_Methods[YBM_MOVE]            = (ULONG) ym_YBM_MOVE;
    ym_Methods[YBM_HANDLEINPUT]     = (ULONG) ym_YBM_HANDLEINPUT;
    ym_Methods[YBM_REINCARNATE]     = (ULONG) ym_YBM_REINCARNATE;
    ym_Methods[YBM_SETSTATE]        = (ULONG) ym_YBM_SETSTATE;
    ym_Methods[YBM_SETSTATE_I]      = (ULONG) ym_YBM_SETSTATE_I;

    ym_Methods[YMM_RESETVIEWER]     = (ULONG) ym_YMM_RESETVIEWER;
    ym_Methods[YMM_DOIMPULSE]       = (ULONG) ym_YMM_DOIMPULSE;
    ym_Methods[YMM_ALIGNMISSILE_S]  = (ULONG) ym_YMM_ALIGNMISSILE_S;
    ym_Methods[YMM_ALIGNMISSILE_V]  = (ULONG) ym_YMM_ALIGNMISSILE_V;


    /* ClassInfo-Struktur ausfüllen */
    ym_clinfo.superclassid = YPABACT_CLASSID;
    ym_clinfo.methods      = ym_Methods;
    ym_clinfo.instsize     = sizeof(struct ypamissile_data);
    ym_clinfo.flags        = 0;

    /* fertig */
    return(&ym_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYMClass(void)
#else
BOOL FreeYMClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      08-Jun-95   8100000C    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  METHODEN DISPATCHER                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ym_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
**      25-Sep-95   floh        Bacterium-BactClassID wird ausgefüllt
*/
{
    Object *newo;
    struct ypamissile_data *ymd;
    struct Bacterium *bact;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    ymd = INST_DATA(cl,newo);

    /*** den BactPointer ***/
    _get( newo, YBA_Bacterium, &bact );
    ymd->bact = bact;
    ymd->auto_node.bact = bact;
    ymd->auto_node.o    = newo;
    ymd->bact->BactClassID = BCLID_YPAMISSY;

    /*** einige Initialisierungen ***/

    /*** die Attribute ***/
    if (!ym_initAttrs(newo,ymd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, ym_OM_DISPOSE, void *ignored)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jun-95   8100000C    created
*/
{

    /*** OM_DISPOSE nach oben geben ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ym_OM_SET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**      ---
**
**  CHANGED
**      09-Jun-95   floh    created
*/
{

    /* und die Superclass auch... */
    _supermethoda(cl,o,OM_SET,(Msg *)attrs);
    
    /* Attribute setzen */
    ym_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ym_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      09-Jun-95   floh    created
*/
{

    /* Methode an Superclass */
    _supermethoda(cl,o,OM_GET,(Msg *)attrs);
    
    /* Attribute getten */
    ym_getAttrs(o,INST_DATA(cl,o),attrs);
}








