/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Main-Modul der network.class.
**
**  (C) Copyright 1997 by  Andreas Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nucleusclass.h"
#include "modules.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "types.h"
#include "modules.h"
#include "network/networkclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** nw_main.c ***/
_dispatcher(Object *, nw_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, nw_OM_DISPOSE, void *ignored);
_dispatcher(void, nw_OM_SET, struct TagItem *attrs);
_dispatcher(void, nw_OM_GET, struct TagItem *attrs);


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL nw_initAttrs(Object *, struct network_data *, struct TagItem *);
void nw_setAttrs(Object *, struct network_data *, struct TagItem *);
void nw_getAttrs(Object *, struct network_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_input_engine
_use_ov_engine

#ifdef AMIGA
__far ULONG nw_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG nw_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo nw_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeNWClass(ULONG id,...);
__geta4 BOOL FreeNWClass(void);
#else
struct ClassInfo *MakeNWClass(ULONG id,...);
BOOL FreeNWClass(void);
#endif

struct GET_Class nw_GET = {
    &MakeNWClass,                 /* MakeExternClass() */
    &FreeNWClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&nw_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *nw_Entry(void)
{
    return(&nw_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo nw_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:network.class"    /* der Segment-Name */
    },
    nw_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeNWClass(ULONG id,...)
#else
struct ClassInfo *MakeNWClass(ULONG id,...)
#endif

/*
**  FUNCTION
**      Meldet network.class im Nucleus-System an.
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
    
    _get_ov_engine(tlist);
    _get_input_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(nw_Methods,0,sizeof(nw_Methods));

    /***  nucleus.class ***/
    nw_Methods[OM_NEW]                  = (ULONG) nw_OM_NEW;
    nw_Methods[OM_DISPOSE]              = (ULONG) nw_OM_DISPOSE;
    nw_Methods[OM_SET]                  = (ULONG) nw_OM_SET;
    nw_Methods[OM_GET]                  = (ULONG) nw_OM_GET;

    /*** network.class ***/
    
    /*** ClassInfo-Struktur ausfüllen ***/
    nw_clinfo.superclassid = NUCLEUSCLASSID;
    nw_clinfo.methods      = nw_Methods;
    nw_clinfo.instsize     = sizeof(struct network_data);
    nw_clinfo.flags        = 0;


    /* fertig */
    return(&nw_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeNWClass(void)
#else
BOOL FreeNWClass(void)
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
_dispatcher(Object *, nw_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
*/
{
    Object *newo;
    struct network_data *nwd;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    nwd = INST_DATA(cl,newo);


    /*** (I)-Attribute auswerten ***/
    if (!nw_initAttrs(newo,nwd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, nw_OM_DISPOSE, void *ignored)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
*/
{
    struct network_data *nwd;

    nwd = INST_DATA( cl, o);

    /*** OM_DISPOSE nach oben geben ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, nw_OM_SET, struct TagItem *attrs)
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
    nw_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, nw_OM_GET, struct TagItem *attrs)
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
    nw_getAttrs(o,INST_DATA(cl,o),attrs);
}








