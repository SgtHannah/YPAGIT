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
#include "ypa/ypaflyerclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** yf_main.c ***/
_dispatcher(Object *, yf_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yf_OM_DISPOSE, void *ignored);
_dispatcher(void, yf_OM_SET, struct TagItem *attrs);
_dispatcher(void, yf_OM_GET, struct TagItem *attrs);


/*** yf_intelligence.c ***/
_dispatcher(void, yf_YBM_AI_LEVEL3, struct trigger_logic_msg *msg);
_dispatcher(void, yf_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);
_dispatcher(void, yf_YBM_REINCARNATE, void *nischt);

/*** yf_move.c ***/
_dispatcher(void, yf_YBM_MOVE, struct move_msg *move);
_dispatcher(void, yf_YBM_STOPMACHINE, struct trigger_logic_msg *msg );

/*** yf_position.c ***/
_dispatcher(void, yf_YBM_SETPOSITION, struct setposition_msg *pos);


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL yf_initAttrs(Object *, struct ypaflyer_data *, struct TagItem *);
void yf_setAttrs(Object *, struct ypaflyer_data *, struct TagItem *);
void yf_getAttrs(Object *, struct ypaflyer_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_audio_engine

#ifdef AMIGA
__far ULONG yf_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yf_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yf_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYFClass(ULONG id,...);
__geta4 BOOL FreeYFClass(void);
#else
struct ClassInfo *MakeYFClass(ULONG id,...);
BOOL FreeYFClass(void);
#endif

struct GET_Class yf_GET = {
    &MakeYFClass,                 /* MakeExternClass() */
    &FreeYFClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yf_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yf_Entry(void)
{
    return(&yf_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yf_class_seg = {
    { NULL, NULL,                       /* ln_Succ, ln_Pred */
      0, 0,                             /* ln_Type, ln_Pri  */
      "MC2classes:ypaflyer.class"    /* der Segment-Name */
    },
    yf_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeYFClass(ULONG id,...)
#else
struct ClassInfo *MakeYFClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet ypaflyer.class im Nucleus-System an.
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
    memset(yf_Methods,0,sizeof(yf_Methods));

    yf_Methods[OM_NEW]              = (ULONG) yf_OM_NEW;
    yf_Methods[OM_DISPOSE]          = (ULONG) yf_OM_DISPOSE;
    yf_Methods[OM_SET]              = (ULONG) yf_OM_SET;
    yf_Methods[OM_GET]              = (ULONG) yf_OM_GET;

    yf_Methods[YBM_AI_LEVEL3]       = (ULONG) yf_YBM_AI_LEVEL3;
    yf_Methods[YBM_MOVE]            = (ULONG) yf_YBM_MOVE;
    yf_Methods[YBM_HANDLEINPUT]     = (ULONG) yf_YBM_HANDLEINPUT;
    yf_Methods[YBM_SETPOSITION]     = (ULONG) yf_YBM_SETPOSITION;
    yf_Methods[YBM_REINCARNATE]     = (ULONG) yf_YBM_REINCARNATE;
    yf_Methods[YBM_STOPMACHINE]     = (ULONG) yf_YBM_STOPMACHINE;


    /* ClassInfo-Struktur ausfüllen */
    yf_clinfo.superclassid = YPABACT_CLASSID;
    yf_clinfo.methods      = yf_Methods;
    yf_clinfo.instsize     = sizeof(struct ypaflyer_data);
    yf_clinfo.flags        = 0;

    /* fertig */
    return(&yf_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYFClass(void)
#else
BOOL FreeYFClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      08-Jun-95   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  METHODEN DISPATCHER                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yf_OM_NEW, struct TagItem *attrs)
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
    struct ypaflyer_data *yfd;
    struct Bacterium *bact;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    yfd = INST_DATA(cl,newo);
    
    
    /*** die Attribute ***/
    if (!yf_initAttrs(newo,yfd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };


    /*** einige Initialisierungen ***/
    _get( newo, YBA_Bacterium, &bact );
    yfd->bact              = bact;
    yfd->bact->BactClassID = BCLID_YPAFLYER;
    yfd->buoyancy          = 0.0;
    yfd->flight_type       = 0;

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yf_OM_DISPOSE, void *ignored)
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
_dispatcher(void, yf_OM_SET, struct TagItem *attrs)
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
    yf_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yf_OM_GET, struct TagItem *attrs)
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
    yf_getAttrs(o,INST_DATA(cl,o),attrs);
}








