/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_main.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/08 23:12:41 $
**  $Locker: floh $
**  $Author: floh $
**
**  Main-Modul der ypabact.class.
**  Die ypabact.class ist Superklasse aller Vehikel im Spiel.
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
#include "ypa/ypatankclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** yb_main.c ***/
_dispatcher(Object *, yt_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yt_OM_DISPOSE, void *ignored);
_dispatcher(void, yt_OM_SET, struct TagItem *attrs);
_dispatcher(void, yt_OM_GET, struct TagItem *attrs);


/*** yb_intelligence.c ***/
_dispatcher(void, yt_YBM_AI_LEVEL3, struct trigger_logic_msg *msg);
_dispatcher(void, yt_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);
_dispatcher(void, yt_YBM_REINCARNATE, void *nix);
_dispatcher(BOOL, yt_YBM_TESTSECTARGET, struct Bacterium *b);

/*** yt_move.c ***/
_dispatcher(void, yt_YBM_MOVE, struct move_msg *move);
_dispatcher(void, yt_YBM_IMPULSE, struct impulse_msg *imp );
_dispatcher(void, yt_YTM_ALIGNVEHICLE_A, struct alignvehicle_msg *avm );
_dispatcher(BOOL, yt_YTM_ALIGNVEHICLE_U, struct alignvehicle_msg *avm );
_dispatcher(BOOL, yt_YBM_RECOIL, struct recoil_msg *rec );

/*** yt_setposition.c ***/
_dispatcher(void, yt_YBM_SETPOSITION, struct setposition_msg *setpos);
_dispatcher(void, yt_YBM_CHECKPOSITION, void *nix);

/*** yt_bactcollision.c ***/
_dispatcher( BOOL, yt_YBM_BACTCOLLISION, struct bactcollision_msg *bcoll);

/*** yt_fight.c ***/
_dispatcher( BOOL, yt_YBM_CHECKAUTOFIREPOS, struct checkautofirepos_msg *cafp );


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL yt_initAttrs(Object *, struct ypatank_data *, struct TagItem *);
void yt_setAttrs(Object *, struct ypatank_data *, struct TagItem *);
void yt_getAttrs(Object *, struct ypatank_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_audio_engine

#ifdef AMIGA
__far ULONG yt_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yt_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yt_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYTClass(ULONG id,...);
__geta4 BOOL FreeYTClass(void);
#else
struct ClassInfo *MakeYTClass(ULONG id,...);
BOOL FreeYTClass(void);
#endif

struct GET_Class yt_GET = {
    &MakeYTClass,                 /* MakeExternClass() */
    &FreeYTClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yt_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yt_Entry(void)
{
    return(&yt_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yt_class_seg = {
    { NULL, NULL,                       /* ln_Succ, ln_Pred */
      0, 0,                             /* ln_Type, ln_Pri  */
      "MC2classes:ypatank.class"        /* der Segment-Name */
    },
    yt_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeYTClass(ULONG id,...)
#else
struct ClassInfo *MakeYTClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet ypaworld.class im Nucleus-System an.
**
**  INPUTS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      08-Jun-95   floh    created
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif


    /* Methoden-Array initialisieren */
    memset(yt_Methods,0,sizeof(yt_Methods));

    /*** Nucleus ***/
    yt_Methods[OM_NEW]              = (ULONG) yt_OM_NEW;
    yt_Methods[OM_DISPOSE]          = (ULONG) yt_OM_DISPOSE;
    yt_Methods[OM_SET]              = (ULONG) yt_OM_SET;
    yt_Methods[OM_GET]              = (ULONG) yt_OM_GET;

    /*** Bact ***/
    yt_Methods[YBM_AI_LEVEL3]       = (ULONG) yt_YBM_AI_LEVEL3;
    yt_Methods[YBM_MOVE]            = (ULONG) yt_YBM_MOVE;
    yt_Methods[YBM_HANDLEINPUT]     = (ULONG) yt_YBM_HANDLEINPUT;
    yt_Methods[YBM_SETPOSITION]     = (ULONG) yt_YBM_SETPOSITION;
    yt_Methods[YBM_BACTCOLLISION]   = (ULONG) yt_YBM_BACTCOLLISION;
    yt_Methods[YBM_IMPULSE]         = (ULONG) yt_YBM_IMPULSE;
    yt_Methods[YBM_REINCARNATE]     = (ULONG) yt_YBM_REINCARNATE;
    yt_Methods[YBM_CHECKAUTOFIREPOS]= (ULONG) yt_YBM_CHECKAUTOFIREPOS;
    yt_Methods[YBM_CHECKPOSITION]   = (ULONG) yt_YBM_CHECKPOSITION;
    yt_Methods[YBM_RECOIL]          = (ULONG) yt_YBM_RECOIL;
    yt_Methods[YBM_TESTSECTARGET]   = (ULONG) yt_YBM_TESTSECTARGET;

    /*** Tank ***/
    yt_Methods[YTM_ALIGNVEHICLE_A]  = (ULONG) yt_YTM_ALIGNVEHICLE_A;
    yt_Methods[YTM_ALIGNVEHICLE_U]  = (ULONG) yt_YTM_ALIGNVEHICLE_U;


    /* ClassInfo-Struktur ausfüllen */
    yt_clinfo.superclassid = YPABACT_CLASSID;
    yt_clinfo.methods      = yt_Methods;
    yt_clinfo.instsize     = sizeof(struct ypatank_data);
    yt_clinfo.flags        = 0;

    /* fertig */
    return(&yt_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYTClass(void)
#else
BOOL FreeYTClass(void)
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
_dispatcher(Object *, yt_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
**      25-Sep-95   floh    Bacterium-BactClassID wird ausgefüllt
*/
{
    Object *newo;
    struct ypatank_data *ytd;
    struct Bacterium *bact;


    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    ytd = INST_DATA(cl,newo);

    /*** Standardwerte ***/

    /*** den BactPointer ***/
    _get( newo, YBA_Bacterium, &bact );
    ytd->bact = bact;
    ytd->bact->BactClassID = BCLID_YPATANK;

    /*** die Attribute ***/
    if (!yt_initAttrs(newo,ytd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** einige Initialisierungen ***/

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yt_OM_DISPOSE, void *ignored)
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
_dispatcher(void, yt_OM_SET, struct TagItem *attrs)
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

    /* Attribute setzen, nach supermethod, da wir die Position überlagern */
    yt_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yt_OM_GET, struct TagItem *attrs)
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
    yt_getAttrs(o,INST_DATA(cl,o),attrs);
}








