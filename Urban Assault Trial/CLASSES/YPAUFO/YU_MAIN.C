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
#include "ypa/ypaufoclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** yb_main.c ***/
_dispatcher(Object *, yu_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yu_OM_DISPOSE, void *ignored);
_dispatcher(void, yu_OM_SET, struct TagItem *attrs);
_dispatcher(void, yu_OM_GET, struct TagItem *attrs);


/*** yb_intelligence.c ***/
_dispatcher(void, yu_YBM_AI_LEVEL3, struct trigger_logic_msg *msg);
_dispatcher(void, yu_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);
_dispatcher(void, yu_YBM_REINCARNATE, void *nix);

/*** yu_move.c ***/
_dispatcher(void, yu_YBM_MOVE, struct move_msg *move);

/*** yu_setposition.c ***/
_dispatcher(void, yu_YBM_SETPOSITION, struct setposition_msg *setpos);

/*** yu_fight.c ***/
_dispatcher( void, yu_YBM_FIGHTBACT, struct fight_msg *msg);
_dispatcher( void, yu_YBM_FIGHTSECTOR, struct fight_msg *msg);




/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL yu_initAttrs(Object *, struct ypaufo_data *, struct TagItem *);
void yu_setAttrs(Object *, struct ypaufo_data *, struct TagItem *);
void yu_getAttrs(Object *, struct ypaufo_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_audio_engine

#ifdef AMIGA
__far ULONG yu_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yu_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yu_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYUClass(ULONG id,...);
__geta4 BOOL FreeYUClass(void);
#else
struct ClassInfo *MakeYUClass(ULONG id,...);
BOOL FreeYUClass(void);
#endif

struct GET_Class yu_GET = {
    &MakeYUClass,                 /* MakeExternClass() */
    &FreeYUClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yu_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yu_Entry(void)
{
    return(&yu_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yu_class_seg = {
    { NULL, NULL,                       /* ln_Succ, ln_Pred */
      0, 0,                             /* ln_Type, ln_Pri  */
      "MC2classes:ypaufo.class"         /* der Segment-Name */
    },
    yu_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeYUClass(ULONG id,...)
#else
struct ClassInfo *MakeYUClass(ULONG id,...)
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
    memset(yu_Methods,0,sizeof(yu_Methods));

    yu_Methods[OM_NEW]              = (ULONG) yu_OM_NEW;
    yu_Methods[OM_DISPOSE]          = (ULONG) yu_OM_DISPOSE;
    yu_Methods[OM_SET]              = (ULONG) yu_OM_SET;
    yu_Methods[OM_GET]              = (ULONG) yu_OM_GET;

    yu_Methods[YBM_AI_LEVEL3]       = (ULONG) yu_YBM_AI_LEVEL3;
    yu_Methods[YBM_MOVE]            = (ULONG) yu_YBM_MOVE;
    yu_Methods[YBM_HANDLEINPUT]     = (ULONG) yu_YBM_HANDLEINPUT;
    yu_Methods[YBM_SETPOSITION]     = (ULONG) yu_YBM_SETPOSITION;
    yu_Methods[YBM_REINCARNATE]     = (ULONG) yu_YBM_REINCARNATE;
    // yu_Methods[YBM_FIGHTBACT]       = (ULONG) yu_YBM_FIGHTBACT;
    // yu_Methods[YBM_FIGHTSECTOR]     = (ULONG) yu_YBM_FIGHTSECTOR;


    /* ClassInfo-Struktur ausfüllen */
    yu_clinfo.superclassid = YPABACT_CLASSID;
    yu_clinfo.methods      = yu_Methods;
    yu_clinfo.instsize     = sizeof(struct ypaufo_data);
    yu_clinfo.flags        = 0;

    /* fertig */
    return(&yu_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYUClass(void)
#else
BOOL FreeYUClass(void)
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
_dispatcher(Object *, yu_OM_NEW, struct TagItem *attrs)
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
    struct ypaufo_data *yud;
    struct Bacterium *bact;


    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    yud = INST_DATA(cl,newo);

    /*** Standardwerte ***/

    /*** den BactPointer ***/
    _get( newo, YBA_Bacterium, &bact );
    yud->bact = bact;
    yud->bact->BactClassID = BCLID_YPAUFO;

    /*** die Attribute ***/
    if (!yu_initAttrs(newo,yud,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** einige Initialisierungen ***/

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yu_OM_DISPOSE, void *ignored)
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
_dispatcher(void, yu_OM_SET, struct TagItem *attrs)
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
    yu_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yu_OM_GET, struct TagItem *attrs)
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
    yu_getAttrs(o,INST_DATA(cl,o),attrs);
}








