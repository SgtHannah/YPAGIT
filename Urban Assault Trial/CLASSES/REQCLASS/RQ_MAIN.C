/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Main-Modul der button.class.
**
**  (C) Copyright 1995 by  Andreas Flemming
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
#include "visualstuff/ov_engine.h"
#include "engine/engine.h"
#include "types.h"
#include "modules.h"
#include "requester/requesterclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** rq_main.c ***/
_dispatcher(Object *, rq_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, rq_OM_DISPOSE, void *ignored);
_dispatcher(void, rq_OM_SET, struct TagItem *attrs);
_dispatcher(void, rq_OM_GET, struct TagItem *attrs);


/*** rq_publish.c ***/
_dispatcher( void, rq_RQM_PUBLISH, void *nix );
_dispatcher( void, rq_RQM_SWITCHPUBLISH, struct reqswitchpublish_msg *rsp );


/*** rq_input.c ***/
_dispatcher( LONG, rq_RQM_HANDLEINPUT, struct VFMInput *input );
_dispatcher( LONG, rq_RQM_GETOFFSET, struct selectbo_msg *t );
_dispatcher( LONG, rq_RQM_POSREQUESTER, struct movereq_msg *t );
_dispatcher( LONG, rq_RQM_MOVEREQUESTER, struct movereq_msg *t );


/*** rq_build.c ***/
_dispatcher( BOOL, rq_RQM_NEWBUTTONOBJECT, Object *ob );
_dispatcher( BOOL, rq_RQM_REMOVEBUTTONOBJECT, struct selectbo_msg *bo );


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL rq_initAttrs(Object *, struct requester_data *, struct TagItem *);
void rq_setAttrs(Object *, struct requester_data *, struct TagItem *);
void rq_getAttrs(Object *, struct requester_data *, struct TagItem *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_input_engine
_use_ov_engine

#ifdef AMIGA
__far ULONG rq_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG rq_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo rq_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeRQClass(ULONG id,...);
__geta4 BOOL FreeRQClass(void);
#else
struct ClassInfo *MakeRQClass(ULONG id,...);
BOOL FreeRQClass(void);
#endif

struct GET_Class rq_GET = {
    &MakeRQClass,                 /* MakeExternClass() */
    &FreeRQClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&rq_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *rq_Entry(void)
{
    return(&rq_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo rq_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:requester.class"  /* der Segment-Name */
    },
    rq_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeRQClass(ULONG id,...)
#else
struct ClassInfo *MakeRQClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet buttonlist im Nucleus-System an.
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
    memset(rq_Methods,0,sizeof(rq_Methods));

    /***  nucleus.class ***/
    rq_Methods[OM_NEW]                  = (ULONG) rq_OM_NEW;
    rq_Methods[OM_DISPOSE]              = (ULONG) rq_OM_DISPOSE;
    rq_Methods[OM_SET]                  = (ULONG) rq_OM_SET;
    rq_Methods[OM_GET]                  = (ULONG) rq_OM_GET;

    /*** button.class ***/
    rq_Methods[RQM_PUBLISH]             = (ULONG) rq_RQM_PUBLISH;
    rq_Methods[RQM_SWITCHPUBLISH]       = (ULONG) rq_RQM_SWITCHPUBLISH;
    rq_Methods[RQM_NEWBUTTONOBJECT]     = (ULONG) rq_RQM_NEWBUTTONOBJECT;
    rq_Methods[RQM_REMOVEBUTTONOBJECT]  = (ULONG) rq_RQM_REMOVEBUTTONOBJECT;
    rq_Methods[RQM_HANDLEINPUT]         = (ULONG) rq_RQM_HANDLEINPUT;
    rq_Methods[RQM_GETOFFSET]           = (ULONG) rq_RQM_GETOFFSET;
    rq_Methods[RQM_POSREQUESTER]        = (ULONG) rq_RQM_POSREQUESTER;
    rq_Methods[RQM_MOVEREQUESTER]       = (ULONG) rq_RQM_MOVEREQUESTER;


    /*** ClassInfo-Struktur ausfüllen ***/
    rq_clinfo.superclassid = NUCLEUSCLASSID;
    rq_clinfo.methods      = rq_Methods;
    rq_clinfo.instsize     = sizeof(struct requester_data);
    rq_clinfo.flags        = 0;


    /* fertig */
    return(&rq_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeRQClass(void)
#else
BOOL FreeRQClass(void)
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
_dispatcher(Object *, rq_OM_NEW, struct TagItem *attrs)
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
    struct requester_data *rqd;
    LONG   xres, yres;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    rqd = INST_DATA(cl,newo);


    /*** (I)-Attribute auswerten ***/
    if (!rq_initAttrs(newo,rqd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Wurden die notwendigen Objekte übergeben? ***/
    if( rqd->requester == NULL ) {
        _methoda(newo, OM_DISPOSE, NULL);
        return( NULL );
        }

    /*** Ausdehnung holen ***/
    _OVE_GetAttrs( OVET_XRes, &xres, OVET_YRes, &yres, TAG_DONE );
    rqd->screen_x = (WORD) xres;
    rqd->screen_y = (WORD) yres;

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, rq_OM_DISPOSE, void *ignored)
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
    struct requester_data *rqd;
    WORD i;

    rqd = INST_DATA( cl, o);

    /*** Freigabe der allozierten Strukturen ***/
    for( i = 0; i < NUM_BUTTONOBJECTS; i++ ) {

        if( rqd->button[i].o )
            _dispose( rqd->button[ i ].o);
        }

    if( rqd->icon )      _dispose( rqd->icon );
    if( rqd->requester ) _dispose( rqd->requester );

    /*** OM_DISPOSE nach oben geben ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rq_OM_SET, struct TagItem *attrs)
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
    rq_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rq_OM_GET, struct TagItem *attrs)
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
    rq_getAttrs(o,INST_DATA(cl,o),attrs);
}








