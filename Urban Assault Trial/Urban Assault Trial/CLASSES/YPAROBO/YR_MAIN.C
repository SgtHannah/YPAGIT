/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Main-Modul der yparobo.class.
**
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
#include "ypa/yparoboclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** yr_main.c ***/
_dispatcher(Object *, yr_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yr_OM_DISPOSE, void *ignored);
_dispatcher(void, yr_OM_SET, struct TagItem *attrs);
_dispatcher(void, yr_OM_GET, struct TagItem *attrs);


/*** yr_intelligence.c ***/
_dispatcher(void, yr_YBM_AI_LEVEL1, struct trigger_logic_msg *msg);
_dispatcher(void, yr_YBM_AI_LEVEL3, struct trigger_logic_msg *msg);
_dispatcher(void, yr_YBM_MOVE, struct move_msg *move);
_dispatcher(void, yr_YBM_STOPMACHINE, struct trigger_logic_msg *msg);
_dispatcher(void, yr_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);
_dispatcher(void, yr_YBM_DOWHILEDEATH, struct trigger_logic_msg *msg);

/*** Energy ***/
_dispatcher(void, yr_YBM_GENERALENERGY, struct trigger_logic_msg *msg);


/*** yr_die.c ***/
_dispatcher(void, yr_YBM_DIE, void *nix );
_dispatcher(void, yr_YBM_REINCARNATE, void *nix );
_dispatcher(void, yr_YBM_RELEASEVEHICLE, Object *ob );


/*** yr_support.c ***/
_dispatcher(void, yr_YRM_GETCOMMANDER, struct getcommander_msg *gc);
_dispatcher(void, yr_YRM_ALLOWVTYPES,  ULONG *types );
_dispatcher(void, yr_YRM_FORBIDVTYPES, ULONG *types );
_dispatcher(BOOL, yr_YRM_SEARCHROBO, struct searchrobo_msg *sr);
_dispatcher(BOOL, yr_YRM_GETENEMY, struct settarget_msg *msg);
_dispatcher(BOOL, yr_YRM_EXTERNMAKECOMMAND, struct externmakecommand_msg *msg);

/*** yr_position.c ***/
_dispatcher(void, yr_YBM_SETPOSITION, struct setposition_msg *pos );
_dispatcher(void, yr_YBM_CHECKPOSITION, void *nix );

/*** yb_message.c ***/
_dispatcher(BOOL, yr_YRM_LOGMSG, struct bact_message *bm);

extern ULONG YPA_CommandCount;

/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL yr_initAttrs(Object *, struct yparobo_data *, struct TagItem *);
void yr_setAttrs(Object *, struct yparobo_data *, struct TagItem *);
void yr_getAttrs(Object *, struct yparobo_data *, struct TagItem *);
struct Cell *yr_FirstSector( struct yparobo_data *yrd );

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_audio_engine

UBYTE **RoboLocaleHandle;

#ifdef AMIGA
__far ULONG yr_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yr_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yr_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYRClass(ULONG id,...);
__geta4 BOOL FreeYRClass(void);
#else
struct ClassInfo *MakeYRClass(ULONG id,...);
BOOL FreeYRClass(void);
#endif

struct GET_Class yr_GET = {
    &MakeYRClass,                 /* MakeExternClass() */
    &FreeYRClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yr_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yr_Entry(void)
{
    return(&yr_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yr_class_seg = {
    { NULL, NULL,                       /* ln_Succ, ln_Pred */
      0, 0,                             /* ln_Type, ln_Pri  */
      "MC2classes:yparobo.class"        /* der Segment-Name */
    },
    yr_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeYRClass(ULONG id,...)
#else
struct ClassInfo *MakeYRClass(ULONG id,...)
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
    memset(yr_Methods,0,sizeof(yr_Methods));

    yr_Methods[OM_NEW]                = (ULONG) yr_OM_NEW;
    yr_Methods[OM_DISPOSE]            = (ULONG) yr_OM_DISPOSE;
    yr_Methods[OM_SET]                = (ULONG) yr_OM_SET;
    yr_Methods[OM_GET]                = (ULONG) yr_OM_GET;

    yr_Methods[YBM_AI_LEVEL1]         = (ULONG) yr_YBM_AI_LEVEL1;
    yr_Methods[YBM_AI_LEVEL3]         = (ULONG) yr_YBM_AI_LEVEL3;
    yr_Methods[YBM_MOVE]              = (ULONG) yr_YBM_MOVE;
    yr_Methods[YBM_HANDLEINPUT]       = (ULONG) yr_YBM_HANDLEINPUT;
    yr_Methods[YBM_DIE]               = (ULONG) yr_YBM_DIE;
    yr_Methods[YBM_SETPOSITION]       = (ULONG) yr_YBM_SETPOSITION;
    yr_Methods[YBM_REINCARNATE]       = (ULONG) yr_YBM_REINCARNATE;
    yr_Methods[YBM_STOPMACHINE]       = (ULONG) yr_YBM_STOPMACHINE;
    yr_Methods[YBM_GENERALENERGY]     = (ULONG) yr_YBM_GENERALENERGY;
    yr_Methods[YBM_CHECKPOSITION]     = (ULONG) yr_YBM_CHECKPOSITION;
    yr_Methods[YBM_DOWHILEDEATH]      = (ULONG) yr_YBM_DOWHILEDEATH;

    /*** die neuen ***/
    yr_Methods[YRM_GETCOMMANDER]      = (ULONG) yr_YRM_GETCOMMANDER;
    yr_Methods[YRM_ALLOWVTYPES]       = (ULONG) yr_YRM_ALLOWVTYPES;
    yr_Methods[YRM_FORBIDVTYPES]      = (ULONG) yr_YRM_FORBIDVTYPES;
    yr_Methods[YRM_SEARCHROBO]        = (ULONG) yr_YRM_SEARCHROBO;
    yr_Methods[YRM_GETENEMY]          = (ULONG) yr_YRM_GETENEMY;
    yr_Methods[YRM_EXTERNMAKECOMMAND] = (ULONG) yr_YRM_EXTERNMAKECOMMAND;
    yr_Methods[YRM_LOGMSG]            = (ULONG) yr_YRM_LOGMSG;


    /* ClassInfo-Struktur ausfüllen */
    yr_clinfo.superclassid = YPABACT_CLASSID;
    yr_clinfo.methods      = yr_Methods;
    yr_clinfo.instsize     = sizeof(struct yparobo_data);
    yr_clinfo.flags        = 0;

    /*** CommandCount initialisieren ***/
    YPA_CommandCount       = 1; // 0 ist kein Geschwader!

    /* fertig */
    return(&yr_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYRClass(void)
#else
BOOL FreeYRClass(void)
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
_dispatcher(Object *, yr_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/
{
    Object *newo;
    struct yparobo_data *yrd;
    struct Bacterium *bact;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    yrd = INST_DATA(cl,newo);

    /*** BakterienPointer ***/
    _get( newo, YBA_Bacterium, &bact);
    yrd->bact = bact;

    /*** Attribute initialisieren ***/
    yr_initAttrs( newo,INST_DATA(cl,newo),attrs);
    yrd->bact->BactClassID = BCLID_YPAROBO;

    yrd->FirstSector = yr_FirstSector( yrd );

    /*** Handle holen ***/
    _get( yrd->world, YWA_LocaleHandle, &RoboLocaleHandle );

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yr_OM_DISPOSE, void *ignored)
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
    struct yparobo_data *yrd;
    struct Bacterium *vbact;

    yrd = INST_DATA(cl, o );

    /*** OM_DISPOSE nach oben geben ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yr_OM_SET, struct TagItem *attrs)
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
    yr_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yr_OM_GET, struct TagItem *attrs)
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
    yr_getAttrs(o,INST_DATA(cl,o),attrs);
}








