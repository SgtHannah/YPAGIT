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
#include "input/clickbox.h"
#include "requester/buttonclass.h"


/*-------------------------------------------------------------------
**  Methoden-Prototypen
*/

/*** bt_main.c ***/
_dispatcher(Object *, bt_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, bt_OM_DISPOSE, void *ignored);
_dispatcher(void, bt_OM_SET, struct TagItem *attrs);
_dispatcher(void, bt_OM_GET, struct TagItem *attrs);


/*** bt_build.c ***/
_dispatcher(BOOL, bt_BTM_NEWBUTTON, struct newbutton_msg *nb);
_dispatcher(BOOL, bt_BTM_REMOVEBUTTON, struct selectbutton_msg *sb);
_dispatcher(BOOL, bt_BTM_ENABLEBUTTON, struct selectbutton_msg *sb);
_dispatcher(BOOL, bt_BTM_DISABLEBUTTON, struct selectbutton_msg *sb);
_dispatcher(BOOL, bt_BTM_SETSTRING, struct setstring_msg *ss);
_dispatcher(BOOL, bt_BTM_SETSTATE, struct setstate_msg *ss);
_dispatcher(void, bt_BTM_REFRESH, struct selectbutton_msg *sb);
_dispatcher(BOOL, bt_BTM_SETBUTTONPOS, struct setbuttonpos_msg *sbp);


/*** bt_publish.c ***/
_dispatcher( void, bt_BTM_PUBLISH, void *nix );
_dispatcher( void, bt_BTM_SWITCHPUBLISH, void *nix );


/*** bt_input.c ***/
_dispatcher( LONG, bt_BTM_HANDLEINPUT, struct VFMInput *input );
_dispatcher( LONG, bt_BTM_GETOFFSET, struct selectbutton_msg *sb );
_dispatcher( LONG *, bt_BTM_GETSPECIALINFO, struct selectbutton_msg *sb);


/*-------------------------------------------------------------------
**  externe Prototypes
*/
BOOL bt_initAttrs(Object *, struct button_data *, struct TagItem *);
void bt_setAttrs(Object *, struct button_data *, struct TagItem *);
void bt_getAttrs(Object *, struct button_data *, struct TagItem *);
void bt_FreeFont(struct VFMFont *fnt);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_input_engine
_use_ov_engine

#ifdef AMIGA
__far ULONG bt_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG bt_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo bt_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeBTClass(ULONG id,...);
__geta4 BOOL FreeBTClass(void);
#else
struct ClassInfo *MakeBTClass(ULONG id,...);
BOOL FreeBTClass(void);
#endif

struct GET_Class bt_GET = {
    &MakeBTClass,                 /* MakeExternClass() */
    &FreeBTClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&bt_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *bt_Entry(void)
{
    return(&bt_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo bt_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:button.class"     /* der Segment-Name */
    },
    bt_Entry,                       /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeBTClass(ULONG id,...)
#else
struct ClassInfo *MakeBTClass(ULONG id,...)
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
    memset(bt_Methods,0,sizeof(bt_Methods));

    /***  nucleus.class ***/
    bt_Methods[OM_NEW]                  = (ULONG) bt_OM_NEW;
    bt_Methods[OM_DISPOSE]              = (ULONG) bt_OM_DISPOSE;
    bt_Methods[OM_SET]                  = (ULONG) bt_OM_SET;
    bt_Methods[OM_GET]                  = (ULONG) bt_OM_GET;

    /*** button.class ***/
    bt_Methods[BTM_NEWBUTTON]           = (ULONG) bt_BTM_NEWBUTTON;
    bt_Methods[BTM_REMOVEBUTTON]        = (ULONG) bt_BTM_REMOVEBUTTON;
    bt_Methods[BTM_ENABLEBUTTON]        = (ULONG) bt_BTM_ENABLEBUTTON;
    bt_Methods[BTM_DISABLEBUTTON]       = (ULONG) bt_BTM_DISABLEBUTTON;
    bt_Methods[BTM_SWITCHPUBLISH]       = (ULONG) bt_BTM_SWITCHPUBLISH;
    bt_Methods[BTM_PUBLISH]             = (ULONG) bt_BTM_PUBLISH;
    bt_Methods[BTM_HANDLEINPUT]         = (ULONG) bt_BTM_HANDLEINPUT;
    bt_Methods[BTM_SETSTRING]           = (ULONG) bt_BTM_SETSTRING;
    bt_Methods[BTM_SETSTATE]            = (ULONG) bt_BTM_SETSTATE;
    bt_Methods[BTM_GETOFFSET]           = (ULONG) bt_BTM_GETOFFSET;
    bt_Methods[BTM_GETSPECIALINFO]      = (ULONG) bt_BTM_GETSPECIALINFO;
    bt_Methods[BTM_REFRESH]             = (ULONG) bt_BTM_REFRESH;
    bt_Methods[BTM_SETBUTTONPOS]        = (ULONG) bt_BTM_SETBUTTONPOS;


    /*** ClassInfo-Struktur ausfüllen ***/
    bt_clinfo.superclassid = NUCLEUSCLASSID;
    bt_clinfo.methods      = bt_Methods;
    bt_clinfo.instsize     = sizeof(struct button_data);
    bt_clinfo.flags        = 0;


    /* fertig */
    return(&bt_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeBTClass(void)
#else
BOOL FreeBTClass(void)
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
_dispatcher(Object *, bt_OM_NEW, struct TagItem *attrs)
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
    struct button_data *btd;
    LONG   xres, yres;

    /*** Methode an Superclass ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /*** LID-Pointer ***/
    btd = INST_DATA(cl,newo);


    /*** (I)-Attribute auswerten ***/
    if (!bt_initAttrs(newo,btd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };

    /*** Wurde eine korrekte Ausdehnung übergeben? ***/
    if( (btd->click.rect.w <= 0) || (btd->click.rect.h <= 0) ) {
        _methoda(newo, OM_DISPOSE, NULL);
        return( NULL );
        }

    /*** Ausdehnung holen ***/
    _OVE_GetAttrs( OVET_XRes, &xres, OVET_YRes, &yres, TAG_DONE );
    btd->screen_x = (WORD) xres;
    btd->screen_y = (WORD) yres;


    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, bt_OM_DISPOSE, void *ignored)
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
    struct button_data *btd;
    WORD i;

    btd = INST_DATA( cl, o);

    /*** Freigabe der allozierten Strukturen ***/
    for( i = 0; i < MAXNUM_CLICKBUTTONS; i++ ) {

        if( btd->button[i] ) {

            if( btd->button[ i ]->modus == BM_SLIDER )
                _FreeVec( btd->button[ i ]->specialinfo );

            _FreeVec( btd->button[i] );
            }

        if( btd->click.buttons[i] )
            _FreeVec( btd->click.buttons[i] );
        }


    /*** OM_DISPOSE nach oben geben ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bt_OM_SET, struct TagItem *attrs)
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
    bt_setAttrs(o,INST_DATA(cl,o),attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bt_OM_GET, struct TagItem *attrs)
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
    bt_getAttrs(o,INST_DATA(cl,o),attrs);
}








