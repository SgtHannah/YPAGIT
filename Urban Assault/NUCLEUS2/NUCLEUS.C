/*
**  $Source: PRG:VFM/Nucleus2/nucleus.c,v $
**  $Revision: 38.14 $
**  $Date: 1998/01/06 12:43:50 $
**  $Locker: floh $
**  $Author: floh $
**
**  Der portable Teil von Nucleus... Alle system-spezifischen
**  Sachen sind durch LowLevel-Module abgeschirmt. Bereits ab
**  hier ist alles "One Source".
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <utility/tagitem.h>

#include <string.h>
#include <stdlib.h>

#include "modules.h"
#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "nucleus/class.h"
#include "nucleus/nucleusclass.h"

/*-------------------------------------------------------------------
**  Die NucleusBase
*/
struct NucleusBase NBase;

// HACK!

/*** aus windd.class ***/
void wdd_InitLog(void);
void winp_InitLog(void);

/*-------------------------------------------------------------------
**    ======================================
**  *** DIE GLOBAL ENTRY TABLE VON NUCLEUS ***
**    ======================================
*/
struct GET_Nucleus nuc_NUC_GET = {

    /*** Kernel Funktionen V2.0 ***/

    /* memory v2.0 */
    nc_AllocVec,
    nc_FreeVec,

    /* lists v2.0 */
    nc_NewList,
    nc_AddHead,
    nc_AddTail,
    nc_RemHead,
    nc_RemTail,
    nc_Remove,
    nc_FindName,

    /* io v2.0 */
    nc_FOpen,
    nc_FClose,
    nc_FSeek,
    nc_FRead,
    nc_FWrite,

    /* tags v2.0 */
    nc_FindTagItem,
    nc_GetTagData,

    /* iff v2.0 */
    nc_AllocIFF,
    nc_FreeIFF,
    nc_InitIFFasNucleus,
    nc_OpenIFF,
    nc_CloseIFF,
    nc_PushChunk,
    nc_PopChunk,
    nc_ReadChunkBytes,
    nc_WriteChunkBytes,
    nc_ParseIFF,
    nc_CurrentChunk,
    nc_SkipChunk,

    /* engines v2.0 */
    nc_OpenEngine,
    nc_CloseEngine,

    /* object nucleus v2.0 */
    nc_NewObject,
    nc_DisposeObject,
    nc_DoMethod,
    nc_DoMethodA,
    nc_DoSuperMethodA,

    /* persistent nucleus v2.0 */
    nc_SaveObject,
    nc_LoadObject,
    nc_SaveInnerObject,
    nc_LoadInnerObject,

    /* config handling v2.01 */
    nc_GetConfigItems,

    /* io v2.01 */
    nc_FGetS,

    /* log v2.02 */
    nc_SetLogMode,
    nc_LogMsg,

    /* sys path handling */
    nc_SetSysPath,
    nc_GetSysPath,

    /* io v2.1 */
    nc_FDelete,
    nc_FOpenDir,
    nc_FCloseDir,
    nc_FReadDir,
    nc_FMakeDir,
    nc_FRemDir,
};

/****** nucleus/_OpenNucleus ***************************************
*
*   NAME   
*       _OpenNucleus -- initialisiere Nucleus System
*
*   SYNOPSIS
*       entry = _OpenNucleus()
*
*       struct GET_Nucleus *_OpenNucleus(void)
*
*   FUNCTION
*       Initialisiert das gesamte Nucleus-System, muß aufgerufen
*       werden, bevor irgendeine weitere Nucleus Funktion benutzt
*       wird. Neben den universellen Initialisierungen wird
*       die interne Routine nc_SystemInit() aufgerufen, die
*       Plattform-spezifische Initialisierungen übernimmt.
*
*   INPUTS
*
*   RESULT
*       entry   - In dynamisch gelinkten Nucleus-Systemen wird
*                 ein Pointer auf die Nucleus-Einsprungtabelle
*                 zurückgeliefert, falls sich Nucleus nicht initialisieren
*                 konnte, wird NULL zurückgegeben.
*                 In statisch gelinkten Systemen wird bei erfolgreicher
*                 Initialisierung ein Wert ungleich NULL zurückgegeben.
*
*   EXAMPLE
*       struct GET_Nucleus *NUC_GET;
*       NUC_GET = _OpenNucleus();
*       if (NULL == NUC_GET) {
*           puts("Konnte Nucleus nicht initialisieren.");
*       };
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _CloseNucleus()
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1 und leicht
*                           modifiziert (nc_SystemInit()).
*       04-Nov-95   floh    + nc_InitLog
*       24-Jan-96   floh    + Sys-Path-Initialisierung
*       24-Feb-97   floh    +
*       03-Mar-98   floh    + wdd_InitLog()
*       23-Mar-98   floh    + AssignList wird initialisiert
*
*****************************************************************************
*/
struct GET_Nucleus *nc_OpenNucleus(void)
{
    /* NucleusBase initialisieren */
    memset(&NBase,0,sizeof(NBase));
    nc_NewList((struct List *)&(NBase.ClassList));
    nc_NewList((struct List *)&(NBase.AssignList));

    /* SysPaths mit Defaults initialisieren */
    nc_SetSysPath(SYSPATH_RESOURCES,NULL);
    nc_SetSysPath(SYSPATH_CLASSES,NULL);
    nc_SetSysPath(SYSPATH_ENGINES,NULL);

    /* Log-Initialisierung */
    nc_InitLog("ypa_log.txt");
    wdd_InitLog();
    winp_InitLog();

    /* systemspezifische Initialisierungen */
    if (!nc_SystemInit()) {
        nc_KillLog();
        return(NULL);
    };

    /* fertig */
    return(&nuc_NUC_GET);
}

/****** nucleus/_CloseNucleus **************************************
*
*   NAME   
*       _CloseNucleus - Nucleus Cleanup
*
*   SYNOPSIS
*       void _CloseNucleus(void)
*
*   FUNCTION
*      Gegenstück von _OpenNucleus(), nur aufrufen, wenn
*      _OpenNucleus() != NULL zurückkam!
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1 und leicht
*                           modifiziert
*       04-Nov-95   floh    + nc_KillLog()
*       23-Mar-98   floh    + Assign-Liste wird geleert
*
*****************************************************************************
*/
void nc_CloseNucleus(void)
{
    struct MinNode *nd;

    /* Assign-Liste leeren */
    while (nd = (struct MinNode *) _RemHead((struct List *)&(NBase.AssignList))) {
        _FreeVec(nd);
    };

    /* systemspezifischer Cleanup */
    nc_SystemCleanup();

    /* Log-System killen */
    nc_KillLog();

    /* und fertig */
}

/****i* nucleus/nc_MakeClass *****************************************
*
*   NAME   
*       nc_MakeClass - initialisiert Klasse
*
*   SYNOPSIS
*       class = nc_MakeClass(segName)
*
*       Class *nc_MakeClass(STRPTR)
*
*   FUNCTION
*       Lädt das durch "segName" definierten Class-Dispatcher-Segment
*       und erzeugt eine neue Klassen-Struktur, die dann in die
*       globale Klassen-Liste eingetragen wird.
*
*   INPUTS
*       segName - die ClassID (z.B. "nucleus.class")
*
*   RESULT
*       class   - Pointer auf initialisierte Klassen-Struktur
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       nc_FreeClass
*
*   HISTORY
*       28-Dec-94   floh    teilweise übernommen aus Nucleus1-Code,
*                           allerdings jetzt 100% portabel
*       19-Feb-96   floh    akzeptiert jetzt NULL-Returns von
*                           get->MakeExternClass()
*       24-Feb-97   floh    ClassList jetzt in NBase eingebettet
*       21-Oct-97   floh    + Klasse wurde nicht vor dem ersten
*                             nc_FreeClass() in die Klassenliste
*                             eingebettet -> Absturz, falls sich eine
*                             Klasse nicht initialisieren konnte
*
*****************************************************************************
*
*/
Class *nc_MakeClass(STRPTR clid)
{
    /* zuerst nachsehen, ob Class schon existiert */
    Class *cl = (Class *) nc_FindName((struct List *)&(NBase.ClassList), clid);

    if (cl) {

        /* wenn Klasse schon existiert, ist die Sache simpel */
        cl->classcount++;

    } else {

        ULONG i;
        STRPTR name;
        UBYTE seg_name[256];
        ULONG *origmd;
        struct GET_Class *get;
        struct MethodArrayEntry *ma;
        struct MethodArrayEntry *newmd;
        struct ClassInfo *clinfo;

        /* ansonsten eine neue Klassen-Struktur anlegen... */
        cl = (Class *) nc_AllocVec(sizeof(Class), MEMF_PUBLIC|MEMF_CLEAR);
        if (!cl) return(NULL);

        /* Method-Array erzeugen */
        ma = (struct MethodArrayEntry *)
            nc_AllocVec(sizeof(struct MethodArrayEntry) * NUCLEUS_NUMMETHODS,
                        MEMF_PUBLIC|MEMF_CLEAR);
        if (!ma) {
            nc_FreeVec(cl); return(NULL);
        };

        /* Class-Struktur so weit wie möglich init. */
        cl->methods      = ma;

        /* ClassID nach eigenen Puffer kopieren */
        name = (STRPTR) nc_AllocVec(strlen(clid)+1,MEMF_PUBLIC|MEMF_CLEAR);
        if (!name) { nc_FreeVec(ma); nc_FreeVec(cl); return(NULL); };
        strcpy(name, clid);
        cl->node.ln_Name = name;

        /***-------------------------------------***/
        /*** Class-Dispatcher-Code-Segment laden ***/
        /***-------------------------------------***/

        /* endgültigen Segment-Name zusammenbasteln */
        strcpy(seg_name, ASSIGN_classes);   /* "MC2classes:" */
        strcat(seg_name, name);

        /* Code-Segment laden */
        get = (struct GET_Class *) nc_OpenSegment(seg_name,&(cl->seg_handle));

        /* geklappt? */
        if (!get) {
            nc_FreeVec(name); nc_FreeVec(ma); nc_FreeVec(cl); return(NULL);
        };

        /* Klasse darf sich jetzt initialisieren... */
        clinfo = (struct ClassInfo *)
            get->MakeExternClass(MID_ENGINE_OUTPUT_VISUAL, NBase.VisOutEng_GET,
                                 MID_ENGINE_OUTPUT_AUDIO,  NBase.AudOutEng_GET,
                                 MID_ENGINE_INPUT,         NBase.InEng_GET,
                                 MID_ENGINE_TRANSFORM,     NBase.TransEng_GET,
                                 MID_NUCLEUS,              &nuc_NUC_GET,
                                 MID_NucleusBase,          &NBase,
                                 TAG_DONE);

        if (!clinfo) {
            nc_FreeVec(name); nc_FreeVec(ma); nc_FreeVec(cl); return(NULL);
        };

        /* die zurückgegebene ClassInfo-Struktur auswerten... */
        cl->instsize = clinfo->instsize;
        cl->flags    = clinfo->flags;

        /***--------------------------***/
        /*** Methoden-Array ausfüllen ***/
        /***--------------------------***/
        origmd = clinfo->methods;
        newmd  = cl->methods;
        for (i=0; i<NUCLEUS_NUMMETHODS; i++) {
            newmd->dispatcher = (void *) *origmd;
            newmd->trueclass  = cl;
            newmd++; origmd++;
        };

        /* Class an ClassList anhängen */
        nc_AddTail((struct List *)&(NBase.ClassList), (struct Node *) cl);

        /* was jetzt kommt, ist nur für SubClasses */
        if ((clinfo->superclassid) != 0) {

            struct MethodArrayEntry *md;
            struct MethodArrayEntry *smd;

            /* falls Superclass noch nicht exists... */
            cl->superclass = nc_MakeClass(clinfo->superclassid);
            if (!(cl->superclass)) { nc_FreeClass(cl); return(NULL); };

            /* SubClass-Count der Superclass imkrementieren */
            cl->superclass->subclasscount += 1;

            /* Instance-Offset auf Object-Anfang ermitteln */
            cl->instoffset = cl->superclass->instoffset +
                             cl->superclass->instsize;

            /* nicht unterstützte Methoden downloaden */
            md  = cl->methods;
            smd = cl->superclass->methods;
            for (i=0; i<NUCLEUS_NUMMETHODS; i++) {
                if ((md->dispatcher) == NULL) {
                    md->dispatcher = smd->dispatcher;
                    md->trueclass  = smd->trueclass;
                };
                md++; smd++;
            };
        };
    };
    return(cl);
}

/****i* nucleus/nc_FreeClass ***************************************
*
*   NAME   
*       nc_FreeClass    - Klassen-Cleanup
*
*   SYNOPSIS
*       succes = nc_FreeClass(class)
*
*       BOOL nc_FreeClass(Class *)
*
*   FUNCTION
*       Gibt eine mit MakeClass() erzeugte Klasse frei.
*       Die Klassen-Struktur und das dazugehörende Codesegment
*       (nur unter DYNAMIC_LINKING) werden komplett aus dem
*       Speicher entfernt, allerdings nur, wenn keine
*       Objekte dieser Klasse mehr existieren.
*       Jedes nc_MakeClass() muß mit einem korrespondierendem
*       nc_FreeClass() abgeschlossen sein.
*
*   INPUTS
*       class   - Pointer auf von nc_MakeClass() erzeugte 
*                 Klassen-Struktur
*
*   RESULT
*       succes  - TRUE, alles ok, oder FALSE, es existieren noch
*                 Objekte dieser Klasse
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
*   HISTORY
*       28-Dec-94   floh    erzeugt aus Nucleus1-Code
*
*****************************************************************************
*
*/
BOOL nc_FreeClass(Class *cl)
{
    if (cl->classcount != 0) {
        cl->classcount -= 1;
    } else {
        /* existieren noch Objects oder Subklassen ? */
        if ((cl->subclasscount != 0) ||
            (cl->objectcount   != 0)) {
                return(FALSE);
        };

        /* falls Superclass vorhanden, deren SubClassCount - 1    */
        /* und einen FreeClass(superclass) machen, um symmetrisch */
        /* zu MakeClass() zu bleiben */
        if (cl->superclass) {
            cl->superclass->subclasscount -= 1;
            nc_FreeClass(cl->superclass);
        };

        /* Klasse darf jetzt Cleanup machen... */
        ((struct GET_Class *)cl->seg_handle.GET)->FreeExternClass();

        /* Code-Segment unloaden... */
        nc_CloseSegment(&(cl->seg_handle));

        /* Class-Struktur aus Class-List entfernen und Speicher freigeben */
        nc_Remove((struct Node *) cl);
        if (cl->node.ln_Name) nc_FreeVec(cl->node.ln_Name);
        if (cl->methods)      nc_FreeVec(cl->methods);
        nc_FreeVec(cl);
    };
    return(TRUE);
}

/****** nucleus/_new ***********************************************
*
*   NAME   
*       _new    - erzeugt ein neues Objekt
*
*   SYNOPSIS
*       object = _new(class_id, attrs,...)
*
*       Object *_new(STRPTR, ULONG,...)
*
*   FUNCTION
*       Erzeugt ein neues Objekt einer gegebenen Klasse.
*       Falls nicht bereits ein Objekt dieser Klasse existiert,
*       wird zuerst die Klasse selbst initialisiert, dann
*       wird der OM_NEW Handler dieser Klasse beauftragt,
*       ein neues Objekt zu allokieren und zu initialisieren.
*       Der Anfangszustand des Objekts wird definiert durch eine
*       Attribut-Liste, die Bestandteil der Parameterliste ist.
*
*   INPUTS
*       class_id    - die gewünschte Klasse als C-String 
*                     (z.B. "nucleus.class")
*       attrs       - Liste von (I)-Attributen als normale Tag-Liste.
*                     Welche Attribute akzeptiert werden, ist der
*                     jeweiligen Klassen-Spezifikation zu entnehmen.
*
*   RESULT
*       object  - Object-Handle des neu erzeugten Objects, oder NULL,
*                 falls das Object nicht erzeugt werden konnte.
*
*   EXAMPLE
*       Object *car;
*       car = _new("car.class", CARA_NUMWHEELS, 4,
*                               CRRA_FRONTDRIVE, TRUE,
*                               TAG_DONE);
*       if (car) {
*           _method(car, CARM_DRIVETO, 200, 50, 150);
*           _dispose(car);
*       };
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _method(), _methoda(), _dispose(), _load(), _save()
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1
*
*****************************************************************************
*
*/
Object *nc_NewObject(STRPTR clid, ULONG attrs,...)
{
    /* Prototype, um eine Methode direkt anzuwenden */
    #ifdef AMIGA
        ULONG __asm (*dispatcher) (__a0 Object *o, __a1 Class *cl, __a2 void *msg);
    #else
        ULONG (*dispatcher) (Object *o, Class *cl, void *msg);
    #endif

    Object *o;
    Class *cl;
    Class *virtcl;

    /* zuerst Class-Pointer holen */
    cl = nc_MakeClass(clid);
    if (!cl) return(NULL);

    /* dann den richtigen OM_NEW-Dispatcher und die "echte" */
    /* Klasse besorgen (virtclass ist normalerweise == cl) */
    dispatcher = cl->methods[OM_NEW].dispatcher;
    virtcl     = cl->methods[OM_NEW].trueclass;

    /* die TrueClass wird im (sonst nutzlosen) Object-Pointer hochgereicht */
    o = (Object *) dispatcher((Object *)cl,virtcl,(void *)&attrs);

    if (!o) nc_FreeClass(cl);

    return(o);
}

/****** nucleus/_dispose *******************************************
*
*   NAME   
*       _dispose    - vernichtet Object nach Benutzung.
*
*   SYNOPSIS
*       succes = _dispose(object)
*
*       BOOL _dispose(Object *)
*
*   FUNCTION
*       Mit _dispose() wird ein Objekt angewiesen, sich selbst
*       aus dem Speicher zu entfernen. Dazu wird die
*       Methode OM_DISPOSE auf das Objekt angewendet. Falls
*       das Objekt das letzte seiner Klasse war, wird die
*       Klassen-Struktur und (auf DYNAMIC_LINKING Systemen)
*       das Code-Segment der Klasse entfernt.
*
*   INPUTS
*       object  - Object-Handle, wie von _new() oder _load()
*                 returniert
*
*   RESULT
*       success - TRUE, Erfolg, bei FALSE konnte sich das Objekt
*                 nicht entfernen
*
*   EXAMPLE
*       siehe _new()
*
*   NOTES
*
*   BUGS
*       Die Routine kommt IMMER mit TRUE zurück, das ist eher
*       ein Design-Fehler, als ein Implementierungs-Fehler.
*       Wahrscheinlich wird in Zukunft _dispose() überhaupt
*       keinen Return-Wert mehr besitzen.
*
*   SEE ALSO
*       _new(), _load(), _save()
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1
*
*****************************************************************************
*
*/
#ifdef AMIGA
BOOL __asm nc_DisposeObject(__a0 Object *o)
#else
BOOL nc_DisposeObject(Object *o)
#endif
{
    /* hole Pointer auf Class des Objects (this is DIRTY, man) */
    Class *cl = ((struct nucleusdata *)o)->o_Class;

    /* Object disposen */
    BOOL ok = nc_DoMethodA(o, OM_DISPOSE, NULL);

    /* Klasse freigeben */
    if (ok) nc_FreeClass(cl);

    return(ok);
}

/*--------------------------------------------------------------------*/
#ifndef AMIGA   /* that's why das was jetzt kommt aufm Amiga Asm ist! */

/****** nucleus/_method ********************************************
*
*   NAME   
*       _method() - wendet Methode auf Objekt an, Stack-basiert
*
*   SYNOPSIS
*       retval = _method(object, method_id, msg, ...)
*
*       ULONG = _method(Object *, ULONG, ULONG,...)
*
*   FUNCTION
*       Wendet eine Methode auf ein Objekt an. Das Objekt
*       (bzw. dessen Klasse) muß die Methode unterstützen
*       (die unterstützten Methoden sind Teil der Klassen-
*       Spezifikation).
*
*       Methoden-Parameter werden auf dem Stack übergeben.
*       Diese Methoden-Parameter (auch Message genannt)
*       sind spezifisch für die Methode und in der
*       Klassen-Spezifikation definiert.
*
*   INPUTS
*       object    - Objekt, auf das die Methode angewendet 
*                   werden soll
*       method_id - definiert Methode, die angewendet werden soll
*       msg       - Methoden-spezifische Parameter-Liste, die
*                   auf dem Stack erwartet wird.
*
*   RESULT
*       retval  - Rückgabewert der Methode (manche Methoden haben
*                 keinen definierten Rückgabewert).
*
*   EXAMPLE
*       siehe _new()
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       _methoda()
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1
*       31-Jul-96   floh    Debugging-Test, ob NULL-Object-Pointer
*
*****************************************************************************
*
*/
ULONG nc_DoMethod(Object *o, ULONG mid, ULONG args,...)
{
    register Class *cl;

    /* Prototype für Methoden-Dispatcher-Call */
    ULONG (*dispatcher) (Object *o, Class *cl, void *msg);

    /*** FIXME ***/
    if (o == NULL) {
        _LogMsg("ERROR: Method invocation on NULL Object!\n");
        return(0);
    };

    /* Pointer auf Klasse aus Object holen */
    cl = ((struct nucleusdata *)o)->o_Class;

    /* Adresse des Dispatcher ermitteln */
    dispatcher = cl->methods[mid].dispatcher;

    /* und rein... */
    return((*dispatcher) (o,cl->methods[mid].trueclass,&args));
}

/****** nucleus/_methoda *******************************************
*
*   NAME   
*       _methoda - wendet Methode auf Objekt an, Pointer-basiert
*
*   SYNOPSIS
*       retval = _methoda(object, method_id, msgptr)
*
*       ULONG = _methoda(Object *, ULONG, void *)
*
*   FUNCTION
*       Entspricht _method(), die Message wird aber nicht
*       auf dem Stack, sondern durch einen Pointer auf
*       eine initialisierte Message-Struktur übergeben.
*
*   INPUTS
*       object    - Objekt, auf das die Methode angewendet 
*                   werden soll
*       method_id - definiert Methode, die angewendet werden soll
*       msgptr    - Pointer auf Methoden-spezifische Message-Struktur
*
*   RESULT
*       retval  - Rückgabewert der Methode (manche Methoden haben
*                 keinen definierten Rückgabewert).
*
*   EXAMPLE
*
*   NOTES
*       (1) Falls Performance wichtig ist, sollte _methoda()
*           gegenüber _method() unbedingt bevorzugt werden.
*       (2) Manche Methoden schreiben Werte in die msg-Struktur
*           zurück, in diesem Fall *MUSS* _methoda() benutzt
*           werden, weil die zurückgeschriebenen Werte (auf dem Stack)
*           sonst verloren gehen.
*
*   BUGS
*
*   SEE ALSO
*       _method()
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1
*       31-Jul-96   floh    Debugging-Test, ob NULL-Object-Pointer
*
*****************************************************************************
*
*/
ULONG nc_DoMethodA(Object *o, ULONG mid, Msg *msg)
{
    register Class *cl;

    /* Prototype für Methoden-Dispatcher-Call */
    ULONG (*dispatcher) (Object *o, Class *cl, void *msg);

    /*** FIXME ***/
    if (o == NULL) {
        _LogMsg("ERROR: Method invocation on NULL Object!\n");
        return(0);
    };

    /* Pointer auf Klasse aus Object holen */
    cl = ((struct nucleusdata *)o)->o_Class;

    /* Adresse des Dispatcher ermitteln */
    dispatcher = cl->methods[mid].dispatcher;

    /* und rein... */
    return((*dispatcher) (o,cl->methods[mid].trueclass,msg));
}

/****** nucleus/_supermethoda **************************************
*
*   NAME   
*       _supermethoda - wendet Methode der Superklasse einer
*                       gegebenen Klasse auf ein Objekt an
*                       (nur für Klassen-Designer!)
*
*   SYNOPSIS
*       retval = _supermethoda(class, object, method_id, msgptr)
*
*       ULONG _supermethoda(Class *, Object *, ULONG, void *)
*
*   FUNCTION
*       Diese Methode darf nur innerhalb von Klassen-Dispatchern
*       verwendet werden. Sie dient dazu, aus einem Methoden-Handler
*       heraus die Methode an die Eltern-Klasse "hochzureichen" und
*       deren Rückgabewert nach unten weiterzugeben.
*
*   INPUTS
*       class     - Pointer auf Klassen-Struktur
*       object    - Pointer auf Objekt
*       method_id - ID der anzuwendenden Methode
*       msgptr    - Pointer auf Message
*
*   RESULT
*       retval    - Rückgabewert des Methoden-Handlers der Elternklasse
*
*   EXAMPLE
*
*   NOTES
*       *** NICHT IN NORMALEN NUCLEUS APPLIKATIONEN ANWENDEN ***
*
*   BUGS
*
*   SEE ALSO
*
*   HISTORY
*       28-Dec-94   floh    übernommen aus Nucleus1
*
*****************************************************************************
*
*/
ULONG nc_DoSuperMethodA(Class *cl, Object *o, ULONG mid, Msg *msg)
{
    /* Prototype für Methoden-Dispatcher-Call */
    ULONG (*dispatcher) (Object *o, Class *cl, void *msg);

    /* Superclass-Ptr besorgen */
    cl = cl->superclass;

    /* Adresse des Dispatcher ermitteln */
    dispatcher = cl->methods[mid].dispatcher;

    /* und rein... */
    return((*dispatcher) (o,cl->methods[mid].trueclass,msg));
}

#endif  /* ifndef AMIGA */

/*--- Nucleus ends here -------------------------------------------*/

