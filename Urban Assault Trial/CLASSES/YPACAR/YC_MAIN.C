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
#include "ypa/ypacarclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** yb_main.c ***/
_dispatcher(Object *, yc_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, yc_OM_DISPOSE, void *ignored);
_dispatcher(void, yc_OM_SET, struct TagItem *attrs);
_dispatcher(void, yc_OM_GET, struct TagItem *attrs);


/*** yc_intelligence.c ***/
_dispatcher(void, yc_YBM_HANDLEINPUT, struct trigger_logic_msg *msg);

/*** yc_move.c ***/
_dispatcher(void, yc_YTM_ALIGNVEHICLE_A, struct alignvehicle_msg *avm);
_dispatcher(void, yc_YTM_ALIGNVEHICLE_U, struct alignvehicle_msg *avm);


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL yc_initAttrs(Object *, struct ypacar_data *, struct TagItem *);
void yc_setAttrs(Object *, struct ypacar_data *, struct TagItem *);
void yc_getAttrs(Object *, struct ypacar_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_audio_engine

#ifdef AMIGA
__far ULONG yc_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG yc_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo yc_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeYCClass(ULONG id,...);
__geta4 BOOL FreeYCClass(void);
#else
struct ClassInfo *MakeYCClass(ULONG id,...);
BOOL FreeYCClass(void);
#endif

struct GET_Class yc_GET = {
    &MakeYCClass,                 /* MakeExternClass() */
    &FreeYCClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&yc_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *yc_Entry(void)
{
    return(&yc_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo yc_class_seg = {
    { NULL, NULL,                       /* ln_Succ, ln_Pred */
      0, 0,                             /* ln_Type, ln_Pri  */
      "MC2classes:ypacar.class"         /* der Segment-Name */
    },
    yc_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeYCClass(ULONG id,...)
#else
struct ClassInfo *MakeYCClass(ULONG id,...)
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
    memset(yc_Methods,0,sizeof(yc_Methods));

    /*** Nucleus ***/
    yc_Methods[OM_NEW]              = (ULONG) yc_OM_NEW;
    yc_Methods[OM_DISPOSE]          = (ULONG) yc_OM_DISPOSE;
    yc_Methods[OM_SET]              = (ULONG) yc_OM_SET;
    yc_Methods[OM_GET]              = (ULONG) yc_OM_GET;

    /*** Bact ***/
    yc_Methods[YBM_HANDLEINPUT]     = (ULONG) yc_YBM_HANDLEINPUT;
    
    /*** Tank ***/
    yc_Methods[YTM_ALIGNVEHICLE_A]  = (ULONG) yc_YTM_ALIGNVEHICLE_A;
    yc_Methods[YTM_ALIGNVEHICLE_U]  = (ULONG) yc_YTM_ALIGNVEHICLE_U;


    /* ClassInfo-Struktur ausfüllen */
    yc_clinfo.superclassid = YPATANK_CLASSID;
    yc_clinfo.methods      = yc_Methods;
    yc_clinfo.instsize     = sizeof(struct ypacar_data);
    yc_clinfo.flags        = 0;

    /* fertig */
    return(&yc_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeYCClass(void)
#else
BOOL FreeYCClass(void)
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
_dispatcher(Object *, yc_OM_NEW, struct TagItem *attrs)
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
    struct ypacar_data *ycd;
    struct Bacterium *bact;


    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    ycd = INST_DATA(cl,newo);

    /*** Standardwerte ***/

    /*** den BactPointer ***/
    _get( newo, YBA_Bacterium, &bact );
    ycd->bact = bact;
    ycd->bact->BactClassID = BCLID_YPACAR;

    /*** die Attribute ***/
    if (!yc_initAttrs(newo,ycd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** einige Initialisierungen ***/

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, yc_OM_DISPOSE, void *ignored)
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
_dispatcher(void, yc_OM_SET, struct TagItem *attrs)
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
    yc_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yc_OM_GET, struct TagItem *attrs)
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
    yc_getAttrs(o,INST_DATA(cl,o),attrs);
}








