/*
**  $Filename:  ade_class.c $
**  $Release:   2 $
**  $Revision: 38.22 $
**  $Date: 1996/01/20 16:49:11 $
**  $Locker:  $
**  $Author: floh $
**
**  Die ADE-Klasse ist die Stammklasse aller Atomic Data Elements.
**
**  (C) Copyright 1993 by A.Weissflog
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
#include "ade/ade_class.h"

/*-------------------------------------------------------------------
**  Prototypes für MakeADEClass()
*/
_dispatcher(Object *, ade_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, ade_OM_DISPOSE, void *ignored);
_dispatcher(void, ade_OM_SET, struct TagItem *attrs);
_dispatcher(void, ade_OM_GET, struct TagItem *attrs);
_dispatcher(Object *, ade_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, ade_OM_SAVETOIFF, struct iff_msg *iffmsg);

_dispatcher(void, ade_ADEM_CONNECT, struct adeconnect_msg *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG ade_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG ade_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo ade_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeADEClass(ULONG id,...);
__geta4 BOOL FreeADEClass(void);
#else
struct ClassInfo *MakeADEClass(ULONG id,...);
BOOL FreeADEClass(void);
#endif

struct GET_Class ade_GET = {
    &MakeADEClass,      /* MakeExternClass() */
    &FreeADEClass,      /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&ade_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *ade_Entry(void)
{
    return(&ade_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo ade_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:ade.class"    /* der Segment-Name */
    },
    ade_Entry,                  /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeADEClass(ULONG id,...)
#else
struct ClassInfo *MakeADEClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet die ade_class im Nucleus-System an.
**
**  INPUTS
**      Gesholt wird:
**          MID_NUCLEUS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGES
**      28-Sep-93   floh    created
**      08-Nov-93   floh    gedebugged, holt jetzt nicht mehr
**                          Pointer auf OVE_GET, braucht kein Schwein.
**      18-Dec-93   floh    entsprechend dem neuen Class Handling
**                          gibt jetzt eine ausgefüllte ClassInfo-Struktur
**                          zurück
**      03-Apr-94   floh    leichter FaceLift :-)
**      20-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      15-Jan-95   floh    Nucleus2-Revision
**      05-Jun-95   floh    Experimente mit SASC-Profiler. Negativ.
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Native Dispatchers in das Methods-Array eintragen */
    memset(ade_Methods,0,sizeof(ade_Methods));

    ade_Methods[OM_NEW]         = (ULONG) ade_OM_NEW;
    ade_Methods[OM_DISPOSE]     = (ULONG) ade_OM_DISPOSE;
    ade_Methods[OM_SET]         = (ULONG) ade_OM_SET;
    ade_Methods[OM_GET]         = (ULONG) ade_OM_GET;
    ade_Methods[OM_NEWFROMIFF]  = (ULONG) ade_OM_NEWFROMIFF;
    ade_Methods[OM_SAVETOIFF]   = (ULONG) ade_OM_SAVETOIFF;

    ade_Methods[ADEM_CONNECT]   = (ULONG) ade_ADEM_CONNECT;

    /* Class-Info-Struktur ausfüllen */
    ade_clinfo.superclassid = NUCLEUSCLASSID;
    ade_clinfo.methods      = ade_Methods;
    ade_clinfo.instsize     = sizeof(struct ade_data);
    ade_clinfo.flags        = 0;

    /* und das war's */
    return(&ade_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeADEClass(void)
#else
BOOL FreeADEClass(void)
#endif
/*
**  FUNCTION
**      Meldet ade_class wieder ab.
**
**  INPUTS
**      ---
**  OUPUTS
**      TRUE  -> alles OK
**      FALSE -> Klasse konnte nicht geschlossen werden
**               (still any object out there ???)
**
**  CHANGED
**      28-Sep-93   floh    created
**      18-Dec-93   floh    entsprechend dem neuen Class Handling
**                          wird die Class nicht mehr selbst
**                          freigegeben, das erledigt FreeClass()
**      20-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      15-Jan-95   floh    Nucleus2-Revision
**      05-Jun-95   floh    Experimente mit SASC-Profiler.
*/
{
    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN
**=================================================================*/

/*-----------------------------------------------------------------*/
void ade_initAttributes(Object *o, 
                        struct ade_data *aded, 
                        struct TagItem *attrs)
/*
**  FUNCTION
**      Initialisiert LID mit den Default-Attribut-Values
**      und scannt danach die übergebene TagList nach
**      "Overriders". Schneller als die alte Variante mit
**      GetTagData()
**
**  INPUTS
**      o       -> Pointer auf Object
**      aded    -> Pointer auf dessen LID
**      attr    -> Pointer auf (I) Attribut-Liste
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> ernsthafter Fehler aufgetreten
**
**  CHANGED
**      11-Dec-94   floh    created
**      15-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Default-Attribut-Values eintragen */
    if (ADEA_DepthFade_DEF) aded->ade_Flags |= ADEF_DepthFade;
    if (ADEA_BackCheck_DEF) aded->ade_Flags |= ADEF_BackCheck;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case ADEA_BackCheck:
                        if (data) aded->ade_Flags |= ADEF_BackCheck;
                        else      aded->ade_Flags &= ~ADEF_BackCheck;
                        break;

                    case ADEA_DepthFade:
                        if (data) aded->ade_Flags |= ADEF_DepthFade;
                        else      aded->ade_Flags &= ~ADEF_DepthFade;
                        break;

                    case ADEA_Point:
                        aded->ade_Point = data;
                        break;

                    case ADEA_Polygon:
                        aded->ade_Polygon = data;
                        break;

                };
        };
    };

    /* das war's */
}

/*-----------------------------------------------------------------*/
void ade_setAttributes(Object *o, 
                       struct ade_data *aded, 
                       struct TagItem *attrs)
/*
**  FUNCTION
**      Setzt (S)-Attribute.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Dec-94   floh    created
**      15-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case ADEA_BackCheck:
                        if (data) aded->ade_Flags |= ADEF_BackCheck;
                        else      aded->ade_Flags &= ~ADEF_BackCheck;
                        break;

                    case ADEA_DepthFade:
                        if (data) aded->ade_Flags |= ADEF_DepthFade;
                        else      aded->ade_Flags &= ~ADEF_DepthFade;
                        break;

                    case ADEA_Point:
                        aded->ade_Point = data;
                        break;

                    case ADEA_Polygon:
                        aded->ade_Polygon = data;
                        break;

                };
        };
    };

    /* das war's */
}

/*-----------------------------------------------------------------*/
void ade_getAttributes(Object *o, 
                       struct ade_data *aded, 
                       struct TagItem *attrs)
/*
**  FUNCTION
**      Abfrage von (G)-Attributen.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Dec-94   floh    created
**      15-Jan-95   floh    Nucleus2-Revision
**      10-Apr-95   floh    neu: ADEA_PrivateNode
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch (tag) {

            /* die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case ADEA_BackCheck:
                        if ((aded->ade_Flags) & ADEF_BackCheck) 
                            *value = TRUE;
                        else                                   
                            *value = FALSE;
                        break;

                    case ADEA_DepthFade:
                        if ((aded->ade_Flags) & ADEF_DepthFade) 
                            *value = TRUE;
                        else
                            *value = FALSE;
                        break;

                    case ADEA_Point:
                        *value = aded->ade_Point;
                        break;

                    case ADEA_Polygon:
                        *value = aded->ade_Polygon;
                        break;

                    case ADEA_PrivateNode:
                        *value = (ULONG) &(aded->ade_Private);
                        break;

                }; /* switch(tag) */
        }; /* switch(tag) */
    }; /* while(attrs->ti_Tag) */

    /* das war's */
}

/*=================================================================**
**  METHODEN DISPATCHER
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ade_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      29-Sep-93   floh    created
**      01-Oct-93   floh    ade_data-Struktur enthält jetzt ObjectNode,
**                          statt einfacher MinNodes. Dies ist die
**                          einzige legale Methode, beim Abarbeiten
**                          einer Object-Liste von der Node auf das
**                          zugehörige Object zu schließen. ade_OM_NEW()
**                          trägt das Object in die Nodes ein, ab dann
**                          sind die Felder aded->ade_Private.Object und
**                          aded->ade_Public.Object READONLY!
**      08-Nov-93   floh    ADEA_Connect wird jetzt nicht mehr
**                          als Init-Tag akzeptiert. In Fact existiert
**                          dieses Tag überhaupt nicht mehr, läuft jetzt
**                          alles über ADEM_CONNECT
**      03-Apr-94   floh    extensive Erweiterung des Init-Attribut-
**                          Handlings.
**      08-May-94   floh    neu: ADEA_DetailLimit und ADEA_Detail
**      20-Nov-94   floh    (1) serial debug code
**                          (2) obsoleter Attribut-Code entfernt
**                          (3) __geta4
**      11-Dec-94   floh    Init Attributes in externe Routine
**                          ausgelagert, die etwas effektiver
**                          vorgeht, als GetTagData().
**      15-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    Object *newo;
    struct ade_data *aded;

    /* zuerst wird die Methode an die Superclass weitergegeben */
    newo = (Object *) _supermethoda(cl,o,OM_NEW,(Msg *)attrs);
    if (!newo) return(NULL);

    /* Local Instance Data initialisieren */
    aded = INST_DATA(cl,newo);

    aded->ade_Private.Object  = newo;

    /* Init-Attribute auswerten */
    ade_initAttributes(newo,aded,attrs);

    /* das war alles... */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, ade_OM_DISPOSE, void *ignored)
/*
**  CHANGED
**      29-Sep-93   floh    created
**      01-Oct-93   floh    notwendige Änderungen wegen Einführung der
**                          ObjectNodes
**      20-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      15-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    struct ade_data *aded = INST_DATA(cl,o);

    /* falls noch in Private-List, daraus befreien */
    if ((aded->ade_Flags) & (ADEF_InListPrivate)) {
        _Remove((struct Node *) &(aded->ade_Private.Node));
    };

    /* Finally, Methode an Superclass hochgeben */
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,(Msg *)ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ade_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      03-Apr-94   floh    created
**      08-May-94   floh    neu: ADEA_Detail und ADEA_DetailLimit
**      20-Nov-94   floh    (1) serial debug code
**                          (2) obsoleten Attribut-Code entfernt
**                          (3) __geta4
**      11-Dec-94   floh    Attribute-Setting in effektivere
**                          Routine ausgelagert (welche ohne
**                          GetTagData() arbeitet).
**      15-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    ade_setAttributes(o,INST_DATA(cl,o),attrs);
    _supermethoda(cl,o,OM_SET,(Msg *)attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ade_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      03-Apr-94   floh    created
**      08-May-94   floh    neu: ADEA_DetailLimit
**      20-Nov-94   floh    (1) serial debug code
**                          (2) obsoleten Attribut-Code entfernt
**                          (3) __geta4
**      11-Dec-94   floh    Hauptarbeit wurd ein externe Routine
**                          ausgelagert, die ohne GetTagData()
**                          arbeitet und "ziemlich" fix ist.
**      15-Jan-95   floh    Nucleus2-Revision
**      10-Apr-95   floh    neu: ADEA_PrivateNode
**      17-Jan-96   floh    revised & updated
*/
{
    ade_getAttributes(o,INST_DATA(cl,o),attrs);
    _supermethoda(cl,o,OM_GET,(Msg *)attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ade_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      01-Oct-93   floh    created
**      03-Apr-94   floh    massiv geändert (siehe FUNCTION)
**      20-Oct-94   floh    erkennt jetzt nicht mehr den ATTR-Chunk,
**                          sondern liest vielmehr den STRC-Chunk mit
**                          seiner IFFAttr-Struktur ein und dekodiert
**                          diesen in eine TagList, damit werden
**                          Diskobjects endlich auch unabhängig von
**                          Attribut-Definitionen.
**                          --> ich habe mich doch noch entschlossen,
**                          die alten ATTR-Chunks emulativ abzuhandeln...
**                          + debugging...
**      20-Nov-94   floh    (1) Der VisLimit-Wert wird beim Lesen ignoriert,
**                              weil dieser jetzt Bestandteil der
**                              <struct publish_msg> ist!
**                          (2) __geta4
**      15-Jan-95   floh    (1) Nucleus2-Revision
**                          (2) Unterstützung für alten Attr-Chunk entfernt
**      17-Jan-96   floh    revised & updated
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;

    struct ade_data *aded;
    ULONG ifferror;
    struct ContextNode *cn;
    Object *newo = NULL;

    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror) {
            if (newo) { _methoda(newo,OM_DISPOSE,0); return(NULL); }
            else return(NULL);
        };

        cn = _CurrentChunk(iff);

        /* FORM der SuperClass ? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == OF_NUCLEUS)) {

            newo = (Object *) _supermethoda(cl,o,OM_NEWFROMIFF,(Msg *)iffmsg);
            if (!newo) return(NULL);

            /* LID init. */
            aded = INST_DATA(cl,newo);
            aded->ade_Private.Object = newo;
            continue;
        };

        /* Attributes-Chunk ? */
        if (cn->cn_ID == ADEIFF_IFFAttr) {

            if (newo) {
                /* IFFAttr-Struktur und zu initialisierende TagList */
                struct ade_IFFAttr iffattr;
                struct TagItem tags[10];    /* bei Erweiterung anpassen!!! */

                /* lese Chunk-Inhalt */
                _ReadChunkBytes(iff, &iffattr, sizeof(iffattr));

                /* Endian-Konvertierung */
                v2nw(&(iffattr.version));
                v2nw(&(iffattr.point_nr));
                v2nw(&(iffattr.polygon_nr));

                /* initialisiere TagList */
                if (iffattr.version >= 1) {
                    /* ADEIFF_VERSION == 1 */
                    tags[0].ti_Tag = ADEA_DepthFade;
                    tags[0].ti_Data = (ULONG) (iffattr.flags & ADEF_DepthFade) ? TRUE : FALSE;
                    tags[1].ti_Tag = ADEA_BackCheck;
                    tags[1].ti_Data = (ULONG) (iffattr.flags & ADEF_BackCheck) ? TRUE : FALSE;
                    tags[2].ti_Tag = ADEA_Point;
                    tags[2].ti_Data = (ULONG) iffattr.point_nr;
                    tags[3].ti_Tag = ADEA_Polygon;
                    tags[3].ti_Data = (ULONG) iffattr.polygon_nr;
                };
                /* hier dann für zukünftige IFFAttr-Version... */

                /* begrenze TagListe */
                tags[4].ti_Tag = TAG_DONE;

                /* OM_SET-Methode auf mich selbst anwenden... */
                _methoda(newo,OM_SET,(Msg *)tags);
            };
            /* EndOfChunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);

        /* Ende der Parse-Schleife */
    };

    /* ...und tschüß */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, ade_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      01-Oct-93   floh    created
**      04-Apr-94   floh    jetzt mit Attributes-Chunk.
**      08-May-94   floh    neu: schreibt jetzt zusätzlich 
**                          ADEA_DetailLimit in den Attributes-Chunk
**      20-Oct-94   floh    Unterstützung für ATTR-Chunk entfernt,
**                          benutzt jetzt ein STRC-Chunk (ADEIFF_IFFAttr),
**                          wo die Attribute in einer eigenen sehr
**                          platzsparenden Struktur abgelegt werden.
**      20-Nov-94   floh    (1) serial debug code
**                          (2) VisLimit wird jetzt ignoriert,
**                              dafür wird einfach NULL eingetragen, damit
**                              das Chunkformat noch stimmt.
**                          (3) __geta4
**      15-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    struct ade_IFFAttr iffattr;
    struct ade_data *aded = INST_DATA(cl,o);
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* eigenen FORM schreiben */
    if (_PushChunk(iff,ADEIFF_FORMID,ID_FORM,IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /* erstmal an SuperClass hochreichen */
    if (!(_supermethoda(cl,o,OM_SAVETOIFF,(Msg *)iffmsg)))
        return(FALSE);


    /* ADEIFF_IFFAttr-Chunk erzeugen */
    _PushChunk(iff,0,ADEIFF_IFFAttr,IFFSIZE_UNKNOWN);

    iffattr.version = ADEIFF_VERSION;
    iffattr.flags = (UBYTE) (aded->ade_Flags & (ADEF_DepthFade|ADEF_BackCheck));
    iffattr.detaillimit = 0; // OBSOLETE
    iffattr.point_nr    = aded->ade_Point;
    iffattr.polygon_nr  = aded->ade_Polygon;
    iffattr.vis_limit   = 0; // OBSOLETE

    /* Endian-Konvertierung */
    n2vw(&(iffattr.version));
    n2vw(&(iffattr.point_nr));
    n2vw(&(iffattr.polygon_nr));

    _WriteChunkBytes(iff,&iffattr,sizeof(struct ade_IFFAttr));
    _PopChunk(iff);


    /* ADE-FORM zumachen */
    if (_PopChunk(iff) != 0) return(FALSE);

    /* ...fertig */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ade_ADEM_CONNECT, struct adeconnect_msg *msg)
/*
**  FUNCTION
**      Bittet ein ADE-Object, sich an eine doppeltverkettete
**      Liste anzuhängen. Die Methode ist clever genug,
**      festzustellen, ob das Object schon in einer Liste
**      ist, wenn ja, wird es vorher korrekt aus dieser
**      entfernt.
**
**  INPUTS
**      msg -   Pointer auf adeconnect_msg-Struktur (definiert in
**              "ade/ade_class.h"), die einen Pointer auf
**              die Liste enthält, bei der sich das ADE-Object
**              an den Schwanz hängt.
**
**  RESULTS
**      keinä
**
**  CHANGED
**      29-Sep-93   floh    created
**      01-Oct-93   floh    Änderungen wegen Verwendung von ObjectNodes
**      04-Apr-94   floh    etwas redundanten Code entfernt
**      20-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      15-Jan-95   floh    Nucleus2-Revision
**      12-Apr-95   floh    (1) Statt AddTail() wird jetzt AddHead()
**                              zum Einklinken in die Liste verwendet.
**                          (2) Oh, ich Laie... AddHead() wieder
**                              ersetzt durch AddTail()
**      17-Jan-96   floh    revised & updated
*/
{
    struct ade_data *aded = INST_DATA(cl,o);

    /* zuerst checken, ob schon in einer Liste, wenn ja, removen */
    if ((aded->ade_Flags) & (ADEF_InListPrivate)) {
        _Remove((struct Node *) &(aded->ade_Private.Node));
    };

    /* dann an die neue Liste hängen */
    _AddTail(msg->list,(struct Node *) &(aded->ade_Private.Node));
    aded->ade_Flags |= ADEF_InListPrivate;
}

