/*
**  $Source: PRG:VFM/Classes/_RsrcClass/rsrcclass.c,v $
**  $Revision: 38.4 $
**  $Date: 1996/01/22 22:20:06 $
**  $Locker:  $
**  $Author: floh $
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "nucleus/class.h"
#include "resources/rsrcclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(Object *, rs_OM_NEW, struct TagItem *);
_dispatcher(BOOL, rs_OM_DISPOSE, void *);
_dispatcher(void, rs_OM_GET, struct TagItem *);
_dispatcher(struct RsrcNode *, rs_RSM_CREATE, struct TagItem *);
_dispatcher(void, rs_RSM_FREE, struct rsrc_free_msg *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG rs_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG rs_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo rs_clinfo;

/*-------------------------------------------------------------------
**  Die Dispatcher-globale Resource-Liste, in der alle
**  zur Zeit verwalteten Resourcen durch eine Node
**  repräsentiert sind.
*/
struct List SharedList;
struct List ExclusiveList;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeRsrcClass(ULONG id,...);
__geta4 BOOL FreeRsrcClass(void);
#else
struct ClassInfo *MakeRsrcClass(ULONG id,...);
BOOL FreeRsrcClass(void);
#endif

struct GET_Class rsrc_GET = {
    &MakeRsrcClass,
    &FreeRsrcClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&rsrc_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
struct GET_Class *rsrc_Entry(void)
{
    return(&rsrc_GET);
};

struct SegInfo rsrc_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:rsrc.class"
    },
    rsrc_Entry,
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
__geta4 struct ClassInfo *MakeRsrcClass(ULONG id,...)
#else
struct ClassInfo *MakeRsrcClass(ULONG id,...)
#endif
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Nov-95   floh    created
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *) &id))) return(NULL);
    #endif

    /* Resource-Listen initialisieren */
    _NewList(&SharedList);
    _NewList(&ExclusiveList);

    /* Methoden-Array initialisieren */
    memset(rs_Methods,0,sizeof(rs_Methods));

    rs_Methods[OM_NEW]        = (ULONG) rs_OM_NEW;
    rs_Methods[OM_DISPOSE]    = (ULONG) rs_OM_DISPOSE;
    rs_Methods[OM_GET]        = (ULONG) rs_OM_GET;
    rs_Methods[RSM_CREATE]    = (ULONG) rs_RSM_CREATE;
    rs_Methods[RSM_FREE]      = (ULONG) rs_RSM_FREE;

    /* ClassInfo-Struktur ausfüllen */
    rs_clinfo.superclassid = NUCLEUSCLASSID;
    rs_clinfo.methods      = rs_Methods;
    rs_clinfo.instsize     = sizeof(struct rsrc_data);
    rs_clinfo.flags        = 0;

    /* Ende */
    return(&rs_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeRsrcClass(void)
#else
BOOL FreeRsrcClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      14-Nov-95   floh    created
*/
{
    return(TRUE);
}

/*******************************************************************/
/***    AB HIER METHODEN DISPATCHER    *****************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, rs_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**      Der rsrc.class Konstruktor erwartet folgende
**      (I)-Attribute:
**
**      RSA_Name      - (zwingend) Resource-ID der Resource, die
**                      am Objekt hängt
**      RSA_IFFHandle - (optional) offenes IFFHandle, wenn die Resource
**                      nicht aus einem Standalone File, sondern aus einem
**                      Collection-File geladen werden soll.
**
**      Es wird zuerst in der Resource-Liste nachgeguckt, ob
**      die Resource bereits einmal geladen wurde. Ist dem so,
**      wird einfach der OpenCount inkrementiert und fertig...
**
**      Ansonsten wird eine neue Resource mittels RSM_CREATE
**      erzeugt (was dabei genau geschieht, ist Sache der
**      Subklasse).
**
**  CHANGED
**      10-Jan-96   floh    created
*/
{
    Object *newo;
    struct rsrc_data *rd;
    UBYTE *name;
    ULONG access;
    BOOL dont_copy;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    rd = INST_DATA(cl,newo);

    /*** existiert die Resource schon? ***/
    name   = (UBYTE *) _GetTagData(RSA_Name, NULL, attrs);
    access = _GetTagData(RSA_Access, ACCESS_SHARED, attrs);

    if (name) {

        struct RsrcNode *rnode = NULL;

        if (access == ACCESS_SHARED) {
            rnode = (struct RsrcNode *) 
                    _FindName((struct List *)&SharedList, name);
        };

        if (!rnode) {

            /*** Resource existiert noch nicht, also erzeugen ***/
            rnode = (struct RsrcNode *) _methoda(newo, RSM_CREATE, attrs);
            if (!rnode) {
                _methoda(newo, OM_DISPOSE, 0);
                return(NULL);
            };
        };

        /*** als neuen Client anmelden ***/
        rnode->OpenCount++;
        rd->rnode  = rnode;
        rd->handle = rnode->Handle;

    } else {
        /*** kein Name - kein Objekt... ***/
        _methoda(newo, OM_DISPOSE, 0);
        return(NULL);
    };

    /*** Status von RSA_DontCopy sichern ***/
    dont_copy = _GetTagData(RSA_DontCopy, FALSE, attrs);
    if (dont_copy) rd->flags |= RSF_DontCopy;

    /*** fertig! ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, rs_OM_DISPOSE, void *ignored)
/*
**  FUNCTION
**      Falls eine Resource am Objekt hängt, wird deren
**      OpenCount dekrementiert. Falls dieser dann
**      auf 0 steht, wird ein RSM_FREE angewendet.
**
**  CHANGED
**      10-Jan-96   floh    created
*/
{
    struct rsrc_data *rd = INST_DATA(cl,o);

    if (rd->rnode) {

        rd->rnode->OpenCount--;

        /*** wenn OpenCount auf 0, Resource freigeben ***/
        if (rd->rnode->OpenCount == 0) {
            _method(o, RSM_FREE, (ULONG) rd->rnode);
        };
    };

    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rs_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      10-Jan-96   floh    created
**      21-Jan-96   floh    + RSA_SharedRsrcList
**                          + RSA_ExclusiveRsrcList
*/
{
    struct rsrc_data *rd = INST_DATA(cl,o);
    struct TagItem *stored_attrs = attrs;

    ULONG *value;
    ULONG tag;

    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        value = (ULONG *) attrs++->ti_Data;
        switch(tag) {
            /* die System-Tags */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            switch(tag) {
                case RSA_Name:
                    if (rd->rnode) {
                        *value = (ULONG) rd->rnode->Node.ln_Name;
                    } else {
                        *value = NULL;
                    };
                    break;

                case RSA_Handle:
                    *value = (ULONG) rd->handle;
                    break;

                case RSA_Access:
                    if (rd->rnode) {
                        *value = (ULONG) rd->rnode->Access;
                    } else {
                        *value = ACCESS_EXCLUSIVE;
                    };
                    break;

                case RSA_DontCopy:
                    if (rd->flags & RSF_DontCopy) *value = TRUE;
                    else                          *value = FALSE;
                    break;

                case RSA_SharedRsrcList:
                    *value = (ULONG) &SharedList;
                    break;

                case RSA_ExclusiveRsrcList:
                    *value = (ULONG) &ExclusiveList;
                    break;
            };
        };
    };

    _supermethoda(cl,o,OM_GET,stored_attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, rs_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**      In der rsrc.class kommt RSM_CREATE nicht die Funktion
**      zu, konkrete Resourcen zu erzeugen, sondern es leistet
**      nur einige Vorarbeit bevor konkrete Subklassen loslegen
**      können.
**
**      Dazu gehört:
**
**          - allokieren und initialisieren einer RsrcNode
**          - einklinken der Resource-Node in die Shared-
**            oder Exclusive-Liste
**
**      Für den Fall, das die Resource aus einem Collection-File
**      erzeugt werden soll, muß außerhalb rs_RSM_CREATE bereits
**      folgende Vorarbeit geleistet worden sein:
**
**          - Klasse und Resourcen-Name muß aus dem RSRC-FORM
**            gelesen worden sein
**          - das neue Objekt wird als Instanz der gelesenen Klasse
**            und den gültigen Attributen RSA_Name und RSA_IFFHandle
**            erzeugt
**
**      Die initialisierte Resource-Node wird zurückgegeben und
**      muß von einer Subklasse innerhalb ihres RSM_CREATE
**      noch konkret ausgefüllt werden.
**
**  INPUTS
**      RSA_Name      - (immer) eindeutiger Name der Resource
**      RSA_IFFHandle - (optional) Pointer auf offenes IFFHandle,
**                      aus dem gelesen wird. Dem Objekt steht
**                      frei, diesen IFF-Stream zu benutzen oder
**                      "eigene Wege zu gehen".
**      RSA_Access    - ACCESS_SHARED oder ACCESS_EXCLUSIVE, Default
**                      ist ACCESS_SHARED
**
**  RESULTS
**      Gültiger <struct RsrcNode *> oder NULL, falls was
**      schiefging.
**
**  CHANGED
**      10-Jan-96   floh    created
**      22-Jan-96   floh    + RSA_Weight (für Quasi-Einsortierung
**                            von Resourcen, die selbst Resourcen
**                            besitzen.
*/
{
    UBYTE *name;
    ULONG access;
    ULONG weight;
    struct RsrcNode *rnode = NULL;

    /*** ein Name ist zwingend ***/
    name   = (UBYTE *) _GetTagData(RSA_Name, NULL, tlist);
    access = (ULONG)   _GetTagData(RSA_Access, ACCESS_SHARED, tlist);
    weight = (ULONG)   _GetTagData(RSA_Weight, 0, tlist);
    if (name) {

        /*** allokiere RsrcNode ***/
        rnode = (struct RsrcNode *) _AllocVec(sizeof(struct RsrcNode), MEMF_PUBLIC);
        if (rnode) {

            struct List *use_list;

            strncpy(rnode->Name, name, RSRC_NAME_CHARS);
            rnode->Node.ln_Name = rnode->Name;
            rnode->OpenCount    = 0;
            rnode->Handle       = NULL;
            rnode->Access       = access;

            _get(o, OA_ClassName, &(rnode->HandlerClass));


            /*** in Shared- oder Exclusive-Liste einklinken ***/
            if (ACCESS_SHARED == access) use_list = &SharedList;
            else                         use_list = &ExclusiveList;

            /*** von vorn oder von hinten? ***/
            if (weight > 0) _AddTail(use_list, (struct Node *)rnode);
            else            _AddHead(use_list, (struct Node *)rnode);
        };
    };

    return(rnode);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rs_RSM_FREE, struct rsrc_free_msg *msg)
/*
**  FUNCTION
**      Gibt die übergebene Resource-Node inklusive dem Resource-
**      Handle frei. Darf nur aufgerufen werden, wenn
**      der OpenCount wieder auf 0 steht!
**
**      Die Resource-Node MUSS bereits aus der Resourcen-Liste
**      removed worden sein!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      10-Jan-96   floh    created
*/
{
    _Remove((struct Node *) msg->rnode);
    if (msg->rnode) _FreeVec(msg->rnode);
}

