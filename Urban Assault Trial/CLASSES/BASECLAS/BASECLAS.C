/*
**  $Source: PRG:VFM/Classes/_BaseClass/baseclass.c,v $
**  $Revision: 38.24 $
**  $Date: 1996/11/10 21:00:54 $
**  $Locker:  $
**  $Author: floh $
**
**  Main-Modul der base.class
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "types.h"
#include "modules.h"
#include "engine/engine.h"

#include "visualstuff/ov_engine.h"
#include "transform/te.h"

#include "baseclass/baseclass.h"
#include "ade/ade_class.h"
#include "skeleton/skeletonclass.h"

/*===================================================================
**  globaler ID-Counter, wird mit jedem OM_NEW incrementiert
*/
ULONG base_Id;

/*-------------------------------------------------------------------
**  Moduleigene Prototypen
*/
_dispatcher(Object *, base_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, base_OM_DISPOSE, void *ignored);
_dispatcher(void, base_OM_SET, struct TagItem *attrs);
_dispatcher(void, base_OM_GET, struct TagItem *attrs);

/*-------------------------------------------------------------------
**  externe Prototypes
*/
_dispatcher(BOOL, base_OM_SAVETOIFF, struct iff_msg *iffmsg);
_dispatcher(Object *, base_OM_NEWFROMIFF, struct iff_msg *iffmsg);

_dispatcher(void, base_BSM_POSITION, struct flt_vector_msg *msg);
_dispatcher(void, base_BSM_VECTOR, struct flt_vector_msg *msg);
_dispatcher(void, base_BSM_SCALE, struct flt_vector_msg *msg);
_dispatcher(void, base_BSM_ANGLE, struct lng_vector_msg *msg);
_dispatcher(void, base_BSM_ROTATION, struct lng_vector_msg *msg);

_dispatcher(void, base_BSM_ADDCHILD, struct addchild_msg *msg);
_dispatcher(void, base_BSM_NEWMOTHER, struct newmother_msg *msg);
_dispatcher(void, base_BSM_MAIN, struct main_msg *msg);
_dispatcher(Object *, base_BSM_SETMAINOBJECT, struct setmainobject_msg *msg);

_dispatcher(void, base_BSM_MOTION, struct trigger_msg *msg);
_dispatcher(ULONG, base_BSM_PUBLISH, struct trigger_msg *msg);
_dispatcher(void, base_BSM_TRIGGER, struct trigger_msg *msg);
_dispatcher(void, base_BSM_HANDLEINPUT, struct trigger_msg *msg);

BOOL base_initAttributes(Object *o, struct base_data *bd, struct TagItem *attrs);
void base_setAttributes(Object *o, struct base_data *bd, struct TagItem *attrs);
void base_getAttributes(Object *o, struct base_data *bd, struct TagItem *attrs);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_tform_engine
_use_ov_engine
_use_input_engine

#ifdef AMIGA
__far ULONG base_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG base_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo base_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeBaseClass(ULONG id,...);
__geta4 BOOL FreeBaseClass(void);
#else
struct ClassInfo *MakeBaseClass(ULONG id,...);
BOOL FreeBaseClass(void);
#endif

struct GET_Class base_GET = {
    &MakeBaseClass,                 /* MakeExternClass() */
    &FreeBaseClass,                 /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&base_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Entry-Point */
struct GET_Class *base_Entry(void)
{
    return(&base_GET);
};

/* und die SegInfo-Struktur */
struct SegInfo base_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:base.class"   /* der Segment-Name */
    },
    base_Entry,                 /* Entry()-Point */
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
__geta4 struct ClassInfo *MakeBaseClass(ULONG id,...)
#else
struct ClassInfo *MakeBaseClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet base.class im Nucleus-System an.
**
**  INPUTS
**      Folgende Tags werden akzeptiert:
**          MID_ENGINE_OUTPUT_VISUAL
**          MID_ENGINE_TRANSFORM
**          MID_NUCLEUS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      10-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      28-Nov-94   floh    neu: BSM_SCALE
**      21-Dec-94   floh    neu: BSM_SETMAINOBJECT
**      19-Jan-95   floh    Nucleus2-Revision
**      12-Sep-95   floh    globale Variable base_ID wird auf 1 initialisiert
**      24-Sep-95   floh    - BSM_DOCOLLISION
**                          - BSM_CHECKCOLLISION
**                          - BSM_HANDLECOLLISION
**      04-Mar-96   floh    + input.engine Benutzung (nur wenn
**                            BSM_TRIGGER angewendet wird)
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    _get_tform_engine(tlist);
    _get_ov_engine(tlist);
    _get_input_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(base_Methods,0,sizeof(base_Methods));

    base_Methods[OM_NEW]                 = (ULONG) base_OM_NEW;
    base_Methods[OM_DISPOSE]             = (ULONG) base_OM_DISPOSE;
    base_Methods[OM_SET]                 = (ULONG) base_OM_SET;
    base_Methods[OM_GET]                 = (ULONG) base_OM_GET;
    base_Methods[OM_NEWFROMIFF]          = (ULONG) base_OM_NEWFROMIFF;
    base_Methods[OM_SAVETOIFF]           = (ULONG) base_OM_SAVETOIFF;

    base_Methods[BSM_POSITION]           = (ULONG) base_BSM_POSITION;
    base_Methods[BSM_VECTOR]             = (ULONG) base_BSM_VECTOR;
    base_Methods[BSM_ANGLE]              = (ULONG) base_BSM_ANGLE;
    base_Methods[BSM_ROTATION]           = (ULONG) base_BSM_ROTATION;
    base_Methods[BSM_SCALE]              = (ULONG) base_BSM_SCALE;

    base_Methods[BSM_ADDCHILD]           = (ULONG) base_BSM_ADDCHILD;
    base_Methods[BSM_NEWMOTHER]          = (ULONG) base_BSM_NEWMOTHER;
    base_Methods[BSM_MAIN]               = (ULONG) base_BSM_MAIN;

    base_Methods[BSM_MOTION]             = (ULONG) base_BSM_MOTION;
    base_Methods[BSM_PUBLISH]            = (ULONG) base_BSM_PUBLISH;
    base_Methods[BSM_HANDLEINPUT]        = (ULONG) base_BSM_HANDLEINPUT;
    base_Methods[BSM_SETMAINOBJECT]      = (ULONG) base_BSM_SETMAINOBJECT;
    base_Methods[BSM_TRIGGER]            = (ULONG) base_BSM_TRIGGER;

    /*** Globals init. ***/
    base_Id = 1;

    /* ClassInfo Struktur ausfüllen */
    base_clinfo.superclassid = NUCLEUSCLASSID;
    base_clinfo.methods      = base_Methods;
    base_clinfo.instsize     = sizeof(struct base_data);
    base_clinfo.flags        = 0;

    /* das war's */
    return(&base_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeBaseClass(void)
#else
BOOL FreeBaseClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**      ---
**
**  RESULTS
**      TRUE -> alles OK
**      FALSE-> Klasse konnte nicht freigegeben werden (sollte eigentlich
**              nie auftreten).
**
**  CHANGED
**      10-Nov-94   floh    created
**      17-Nov-94   floh    serial debug code
**      19-Jan-95   floh    Nucleus2-Revision
*/
{
    return(TRUE);
}

/*=================================================================**
**  METHODEN DISPATCHER                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, base_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**      Pointer auf base.class Object oder NULL bei Mißerfolg.
**
**  CHANGED
**      11-Nov-94   floh    created
**      14-Nov-94   floh    BSA_FollowMode -> BSA_FollowMother
**      17-Nov-94   floh    serial debug code
**      20-Nov-94   floh    Ooops, hatte ganz vergessen, den
**                          Object-Pointer in die eigene ChildNode
**                          einzutragen.
**      28-Nov-94   floh    (1) BSA_VisLimit, BSA_AmbientLight
**                          (2) die Skalierer der TForm werden auf
**                              1.0 gesetzt
**                          (3) BSA_TriggerAll
**      01-Dec-94   floh    (1) BSA_TriggerAll -> BSA_PublishAll
**                          (2) BSA_TerminateCollision
**      03-Dec-94   floh    neues TForm-Format...fixed
**      19-Jan-95   floh    Nucleus2-Revision
**      12-Sep-95   floh    Jetzt mit globalem ID-Counter, der nach
**                          base_data.Id geschrieben wird.
**      24-Sep-95   floh    der gesamte Kollisions-Komplex existiert
**                          nicht mehr
*/
{
    Object *newo;
    struct base_data *bd;

    /* Methode an Superclass */
    newo = (Object *) _supermethoda(cl,o,OM_NEW,(Msg *)attrs);
    if (!newo) return(NULL);

    /* LID Pointer */
    bd = INST_DATA(cl,newo);

    /*===================================*/
    /* ein paar Sachen initialisieren... */
    /*===================================*/

    /* Allgemein */
    bd->Id = base_Id++;

    /* Listen... */
    _NewList((struct List *) &(bd->ADElist));
    _NewList((struct List *) &(bd->ChildList));

    /* Object-Node(s) */
    bd->ChildNode.Object = newo;

    /* TForm */
    bd->TForm.scl.x = 1.0;
    bd->TForm.scl.y = 1.0;
    bd->TForm.scl.z = 1.0;

    /* daraus die Rot/Scale-Matrix ermitteln lassen... */
    #ifdef DYNAMIC_LINKING
        if (TE_GET_SPEC) _RefreshTForm(&(bd->TForm));
    #else
        _RefreshTForm(&(bd->TForm));
    #endif

    /* (I)-Attribute auswerten */
    if (!base_initAttributes(newo,bd,attrs)) {
        _methoda(newo, OM_DISPOSE, NULL);
        return(NULL);
    };
    /* das war's dann... */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, base_OM_DISPOSE, void *ignored)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      14-Nov-94   floh    oops, hatte vergessen, mich aus der ChildList
**                          meiner Mother zu Remove()n
**      17-Nov-94   floh    (1) serial debug code
**                          (2) Ooops, hatte ganz vergessen, daß Skeleton
**                              zu disposen
**      20-Nov-94   floh    Remove()ing aus Mother's ChildList war Schrott.
**      19-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
**      22-Jan-96   floh    _dispose()'d bei Bedarf das optional
**                          von OM_NEWFROMIFF erzeugte embed.class
**                          Object in <bd->EmbedObj>
*/
{
    struct base_data *bd = INST_DATA(cl,o);

    /* Skeleton-Object killen, falls vorhanden */
    if (bd->Skeleton) _dispose(bd->Skeleton);

    /* alle angehängten ADEs killen */
    while (bd->ADElist.mlh_Head->mln_Succ) {
        Object *ade = ((struct ObjectNode *) bd->ADElist.mlh_Head)->Object;
        _dispose(ade);
    };

    /* wenn ich eine Mother habe, mich aus deren ChildList Removen() */
    if (bd->Mother) _Remove((struct Node *) &(bd->ChildNode));

    /* alle Children killen */
    while (bd->ChildList.mlh_Head->mln_Succ) {
        Object *child = ((struct ObjectNode *) bd->ChildList.mlh_Head)->Object;
        _dispose(child);
    };

    /* optionales embed.class Object killen */
    if (bd->EmbedObj) _dispose(bd->EmbedObj);

    /* OM_DISPOSE hochreichen */
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,(Msg *)ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_OM_SET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      14-Nov-94   floh    BSA_FollowMode -> BSA_FollowMother
**      17-Nov-94   floh    serial debug code
**      21-Nov-94   floh    ADEs bekommen jetzt nur noch das Attribut
**                          ADEA_ExtSkeleton mitgeteilt, durch Einführung
**                          der <publish_msg> wurden ADEA_Viewer3DPool und
**                          ADEA_Global3DPool ungültig.
**      28-Nov-94   floh    BSA_VisLimit, BSA_AmbientLight, BSA_TriggerAll
**      01-Dec-94   floh    (1) BSA_TriggerAll -> BSA_PublishAll
**                          (2) BSA_TerminateCollision
**      19-Jan-95   floh    Nucleus2-Revision
*/
{
    /* Attribute setzen... */
    base_setAttributes(o,INST_DATA(cl,o),attrs);

    /* und die SuperClass soll auch zu ihrem Recht kommen... */
    _supermethoda(cl,o,OM_SET,(Msg *)attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, base_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      12-Nov-94   floh    created
**      14-Nov-94   floh    BSA_FollowMode -> FollowMother
**      17-Nov-94   floh    (1) serial debug code
**                          (2) die ganzen Winkel-Values waren noch
**                              auf FLOAT gecastet, obwohl die jetzt
**                              im IO-Protokoll einfache LONGs sind...
**      28-Nov-94   floh    BSA_SX, BSA_SY, BSA_SZ
**                          BSA_VisLimit
**                          BSA_AmbientLight
**                          BSA_TriggerAll
**      01-Dec-94   floh    (1) BSA_TriggerAll -> BSA_PublishAll
**                          (2) BSA_TerminateCollision
**      03-Dec-94   floh    neues TForm-Format...fixed
**      19-Jan-95   floh    Nucleus2-Revision
*/
{
    /* Attribute getten */
    base_getAttributes(o,INST_DATA(cl,o),attrs);

    /* Methode an SuperClass */
    _supermethoda(cl,o,OM_GET,(Msg *)attrs);
}
