/*
**  $Source: PRG:VFM/Classes/_AMeshClass/ameshclass.c,v $
**  $Revision: 38.9 $
**  $Date: 1996/11/10 21:19:19 $
**  $Locker:  $
**  $Author: floh $
**
**  Die amesh.class ist Subklasse der area.class und faßt eine
**  beliebige Anzahl Flächen mit identischen Attributen in ein
**  einziges ADE zusammen.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include "string.h"

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "engine/engine.h"

#include "visualstuff/ov_engine.h"
#include "transform/te.h"

#include "ade/ameshclass.h"
#include "bitmap/bitmapclass.h"
#include "skeleton/skeletonclass.h"

/*-------------------------------------------------------------------
**  Moduleigene Prototypen
*/
_dispatcher(Object *, amesh_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, amesh_OM_DISPOSE, void *ignored);
_dispatcher(void, amesh_OM_SET, struct TagItem *attrs);
_dispatcher(void, amesh_OM_GET, struct TagItem *attrs);
_dispatcher(Object *, amesh_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, amesh_OM_SAVETOIFF, struct iff_msg *iffmsg);

_dispatcher(void, amesh_ADEM_PUBLISH, struct publish_msg *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_ov_engine
_use_tform_engine

#ifdef AMIGA
__far ULONG amesh_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG amesh_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo amesh_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeAMeshClass(ULONG id,...);
__geta4 BOOL FreeAMeshClass(void);
#else
struct ClassInfo *MakeAMeshClass(ULONG id,...);
BOOL FreeAMeshClass(void);
#endif

struct GET_Class amesh_GET = {
    &MakeAMeshClass,             /* MakeExternClass() */
    &FreeAMeshClass,             /* FreeVASAClass()   */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&amesh_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *amesh_Entry(void)
{
    return(&amesh_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo amesh_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:amesh.class"       /* der Segment-Name */
    },
    amesh_Entry,  /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeAMeshClass(ULONG id,...)
#else
struct ClassInfo *MakeAMeshClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet die amesh.class im Nucleus-System an.
**
**  INPUTS
**      Folgende Tags werden ausgefiltert:
**          MID_ENGINE_OUTPUT_VISUAL
**          MID_ENGINE_TRANSFORM
**          MID_NUCLEUS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      14-Mar-95   floh    created
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray fischen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    _get_ov_engine(tlist);
    _get_tform_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(amesh_Methods,0,sizeof(amesh_Methods));

    amesh_Methods[OM_NEW]        = (ULONG) amesh_OM_NEW;
    amesh_Methods[OM_DISPOSE]    = (ULONG) amesh_OM_DISPOSE;
    amesh_Methods[OM_SET]        = (ULONG) amesh_OM_SET;
    amesh_Methods[OM_GET]        = (ULONG) amesh_OM_GET;
    amesh_Methods[OM_NEWFROMIFF] = (ULONG) amesh_OM_NEWFROMIFF;
    amesh_Methods[OM_SAVETOIFF]  = (ULONG) amesh_OM_SAVETOIFF;
    amesh_Methods[ADEM_PUBLISH]  = (ULONG) amesh_ADEM_PUBLISH;

    /* ClassInfo Struktur ausfüllen */
    amesh_clinfo.superclassid = AREA_CLASSID;
    amesh_clinfo.methods      = amesh_Methods;
    amesh_clinfo.instsize     = sizeof(struct amesh_data);
    amesh_clinfo.flags        = 0;

    /* und das war alles... */
    return(&amesh_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeAMeshClass(void)
#else
BOOL FreeAMeshClass(void)
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
**      14-Mar-95   floh    created
*/
{
    return(TRUE);
}

/*===================================================================
**  SUPPORT ROUTINEN
*/

/*-----------------------------------------------------------------*/
BOOL add_OlPool(struct amesh_data *amd, struct VFMOutline **ol_pool)
/*
**  FUNCTION
**      Hängt neuen Outline-Pool an Object. Dazu wird der evtl.
**      vorhandene Outline-Pool gekillt, die Größe des neuen
**      ermittelt, und dieser in einen eigenen Speicher-Block
**      kopiert.
**
**      amd->NumPolygons MUSS GÜLTIG SEIN!
**
**  INPUTS
**      amd     -> Pointer auf LID des amesh.class Objects
**      ol_pool -> Pointer auf Outline-Pool, wie in AMESHA_OutlinePool
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> zu wenig Speicher
**
**  CHANGED
**      15-Mar-95   floh    created
**      08-Jul-95   floh    jetzt mit <struct VFMOutline>
**      17-Jan-96   floh    revised & updated
*/
{
    ULONG i,num_elm,size;
    struct VFMOutline *elm_ptr;

    /* alten Pool freigeben */
    if (amd->OutlinePool) {
        _FreeVec(amd->OutlinePool);
        amd->OutlinePool = NULL;
    };

    /* Gesamt-Anzahl <struct VFMOutline>s im OutlinePool (inkl. Ende-Elms) */
    num_elm = 0;
    for (i=0; i<amd->NumPolygons; i++) {
        struct VFMOutline *act_elm = ol_pool[i];
        while (act_elm++->x >= 0.0) num_elm++;
        num_elm++;
    };

    /* Gesamt-Größe aus Anzahl Elementen und Pool-Pointer */
    size = num_elm*sizeof(struct VFMOutline) + 
           amd->NumPolygons*sizeof(struct VFMOutline *);
    amd->OutlinePool = (struct VFMOutline **) _AllocVec(size,MEMF_PUBLIC);
    if (!amd->OutlinePool) return(FALSE);

    /* Pointer auf 1. Element zuweisen */
    elm_ptr = (struct VFMOutline *) 
              (((ULONG *)amd->OutlinePool) + amd->NumPolygons);

    /* Outline-Pointer-Array und Outline-Pool gleichzeitig füllen */
    for (i=0; i<amd->NumPolygons; i++) {

        struct VFMOutline *source = ol_pool[i];

        /* Pool-Pointer */
        amd->OutlinePool[i] = elm_ptr;

        /* zugehörige Outline kopieren (inkl. Ende-Element) */
        do *elm_ptr++ = *source++; while(source->x >= 0.0);
        *elm_ptr++    = *source;       /* ---> Ende-Element */
    };

    /* Ende */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL add_AttrsPool(struct amesh_data *amd, struct amesh_attrs *attrs_pool)
/*
**  FUNCTION
**      Hängt neuen Attributes-Pool an Object. Ein evtl. vorhandener
**      wird vorher gekillt.
**
**      amd->NumPolygons MUSS GÜLTIG SEIN!
**
**  INPUTS
**      amd        -> Pointer auf LID des amesh.class Objects
**      attr_pool -> Pointer auf Attributes-Pool, wie in AMESHA_AttrsPool
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> zu wenig Speicher
**
**  CHANGED
**      15-Mar-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    ULONG size;

    /* alten Pool freigeben */
    if (amd->AttrsPool) {
        _FreeVec(amd->AttrsPool);
        amd->AttrsPool = NULL;
    };

    /* Speicher-Block für neuen Pool allokieren */
    size = amd->NumPolygons * sizeof(struct amesh_attrs);
    amd->AttrsPool = _AllocVec(size,MEMF_PUBLIC);
    if (!amd->AttrsPool) return(FALSE);

    /* Attributes-Pool in neuen Speicherblock kopieren */
    memcpy(amd->AttrsPool, attrs_pool, size);

    /* Ende */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL amesh_initAttrs(struct amesh_data *amd, struct TagItem *attrs)
/*
**  FUNCTION
**      Wertet (I)-Attribute aus.
**
**  INPUTS
**      amd   -> Pointer auf LID
**      attrs -> Pointer auf Attribut-Liste
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> ernsthafter Fehler
**
**  CHANGED
**      15-Mar-95   floh    created
**      08-Jul-95   floh    AMESHA_OutlinePool jetzt mit <struct VFMOutline>
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* ade.class Attribute */
                switch(tag) {
                    case ADEA_DepthFade:
                        if (data)  amd->Flags |= AMESHF_DepthFade;
                        else       amd->Flags &= ~AMESHF_DepthFade;
                        break;
                };

                /* area.class Attribute */
                switch(tag) {

                    case AREAA_TxtBitmap:
                        amd->TxtBitmap   = (Object *) data;
                        break;

                    case AREAA_TracyBitmap:
                        amd->TracyBitmap = (Object *) data;
                        break;
                };

                /* amesh.class Attribute */
                switch(tag) {
                    case AMESHA_NumPolygons:
                        amd->NumPolygons = data;
                        break;

                    case AMESHA_AttrsPool:
                        if (!add_AttrsPool(amd,(struct amesh_attrs *)data))
                            return(FALSE);
                        break;

                    case AMESHA_OutlinePool:
                        if (!add_OlPool(amd,(struct VFMOutline **)data))
                            return(FALSE);
                        break;
                };
        };
    }; /* while(attrs->ti_Tag) */

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void amesh_setAttrs(struct amesh_data *amd, struct TagItem *attrs)
/*
**  FUNCTION
**      Wertet (S)-Attribute aus.
**
**  INPUTS
**      amd   -> Pointer auf LID
**      attrs -> Pointer auf Attribut-Liste
**
**  RESULTS
**      ---
**
**  CHANGED
**      15-Mar-95   floh    created
**      18-Mar-95   floh    AMESHA_OutlinePool und AMESHA_AttrsPool
**                          jetzt settable - allerdings OHNE Feedback,
**                          ob was schiefgegangen ist (Out Of Mem),
**                          also Vorsicht!
**      08-Jul-95   floh    AMESHA_OutlinePool jetzt mit <struct VFMOutline>
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* ade.class Attribute */
                switch(tag) {
                    case ADEA_DepthFade:
                        if (data)  amd->Flags |= AMESHF_DepthFade;
                        else       amd->Flags &= ~AMESHF_DepthFade;
                        break;
                };

                /* area.class Attribute */
                switch(tag) {
                    case AREAA_TxtBitmap:
                        amd->TxtBitmap   = (Object *) data;
                        break;

                    case AREAA_TracyBitmap:
                        amd->TracyBitmap = (Object *) data;
                        break;
                };

                /* amesh.class Attribute */
                switch(tag) {
                    case AMESHA_AttrsPool:
                        add_AttrsPool(amd,(struct amesh_attrs *)data);
                        break;

                    case AMESHA_OutlinePool:
                        add_OlPool(amd,(struct VFMOutline **)data);
                        break;
                };

        };
    }; /* while (attrs->ti_Tag) */

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void amesh_getAttrs(struct amesh_data *amd, struct TagItem *attrs)
/*
**  FUNCTION
**      Wertet (G)-Attribute aus.
**
**  INPUTS
**      amd   -> Pointer auf LID
**      attrs -> Pointer auf Attribut-Liste
**
**  RESULTS
**      ---
**
**  CHANGED
**      15-Mar-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* amesh.class Attribute */
                switch(tag) {
                    case AMESHA_NumPolygons:
                        *value = (ULONG) amd->NumPolygons;
                        break;
                };
        };
    };

    /*** Ende ***/
}

/*=================================================================**
**  METHODEN DISPATCHER                                            **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, amesh_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      15-Mar-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    Object *newo;
    struct amesh_data *amd;

    /* Object instanzieren ;-) */
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /* Pointer auf LID */
    amd = INST_DATA(cl,newo);

    /* (I) Attribute auswerten */
    if (!amesh_initAttrs(amd,attrs)) {
        _methoda(newo,OM_DISPOSE,0);
        return(NULL);
    };

    /* Pointer auf PolygonInfo des area.class Fragments ermitteln */
    _get(newo,AREAA_PolygonInfo,&(amd->PolyInfo));

    /*** fertig ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, amesh_OM_DISPOSE, void *ignored)
/*
**  CHANGED
**      15-Mar-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct amesh_data *amd = INST_DATA(cl,o);

    if (amd->AttrsPool)   _FreeVec(amd->AttrsPool);
    if (amd->OutlinePool) _FreeVec(amd->OutlinePool);

    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, amesh_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      15-Mar-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct amesh_data *amd = INST_DATA(cl,o);
    amesh_setAttrs(amd,attrs);
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, amesh_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      15-Mar-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct amesh_data *amd = INST_DATA(cl,o);
    amesh_getAttrs(amd,attrs);
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, amesh_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      15-Mar-95   floh    created
**      18-Mar-95   floh    amd->PolyInfo wurde nicht initialisiert
**      08-Jul-95   floh    Outline-Pool jetzt im <struct VFMOutline>-Format
**      17-Jan-96   floh    revised & updated
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;

    Object *newo           = NULL;
    struct amesh_data *amd = NULL;
    ULONG ifferror;
    struct ContextNode *cn;

    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        if (ifferror) {
            if (newo) _methoda(newo,OM_DISPOSE,0);
            return(NULL);
        };

        cn = _CurrentChunk(iff);

        /*** FORM der SuperClass? ***/
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == AREAIFF_FORMID)) {

            newo = (Object *) _supermethoda(cl,o,OM_NEWFROMIFF,iffmsg);
            if (!newo) return(NULL);

            amd = INST_DATA(cl,newo);

            /* Pointer auf PolygonInfo des area.class Fragments ermitteln */
            _get(newo,AREAA_PolygonInfo,&(amd->PolyInfo));

            continue;
        };

        /*** Attribute-Pool-Chunk? ***/
        if (cn->cn_ID == AMESHIFF_Attrs) {
            if (newo) {

                WORD i;
                LONG size;

                /*** Attrs-Pool allokieren, einlesen, in LID eintragen ***/
                size = cn->cn_Size;
                amd->NumPolygons = size / sizeof(struct amesh_attrs);
                amd->AttrsPool   = _AllocVec(size,MEMF_PUBLIC);
                if (!amd->AttrsPool) {
                    _methoda(newo,OM_DISPOSE,0); 
                    return(NULL);
                };
                _ReadChunkBytes(iff,amd->AttrsPool,size);

                #ifdef _LittleEndian_
                /*** Endian-Konvertierung ***/
                for (i=0; i<amd->NumPolygons; i++) {
                    v2nw(&(amd->AttrsPool[i].poly_num));
                };
                #endif
            };
            /*** EndOfChunk holen ***/
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /*** Outline-Pool-Chunk? ***/
        if (cn->cn_ID == AMESHIFF_Outlines) {
            if (newo) {

                LONG size,num_olps,i;
                struct VFMOutline *elm_ptr;

                /*** Gesamt-Größe OL-Pool ermitteln und allokieren ***/
                /*** -> Die Num-Elements-Words am Anfang jeder     ***/
                /*** einzelnen Outline ist "Ersatz" für das Ende-  ***/
                /*** Element in jeder Outline!!!                   ***/
                /***-----------------------------------------------***/
                num_olps = cn->cn_Size / sizeof(WORD);
                size     = (amd->NumPolygons * sizeof(struct VFMOutline *)) +
                           (num_olps * sizeof(struct VFMOutline));
                amd->OutlinePool = (struct VFMOutline **) 
                                   _AllocVec(size,MEMF_PUBLIC);
                if (!amd->OutlinePool) {
                    _methoda(newo,OM_DISPOSE,0);
                    return(NULL);
                };

                /*** Pointer auf 1. Element im gesamten Pool ***/
                elm_ptr = (struct VFMOutline *) 
                          (((ULONG *)amd->OutlinePool) + amd->NumPolygons);

                /*** Outlines reinziehen und nach OL-Pool konvertieren ***/
                for (i=0; i<amd->NumPolygons; i++) {

                    WORD num_elm, j;

                    /* Pointer auf akt. Outline nach OL-Pool */
                    amd->OutlinePool[i] = elm_ptr;

                    /* Anzahl Elements in aktueller Outline lesen */
                    _ReadChunkBytes(iff,&num_elm,sizeof(WORD));
                    v2nw(&num_elm);

                    /* Outline-Punkte einzeln lesen und nach Pool */
                    for (j=0; j<num_elm; j++) {

                        struct amesh_ol_atom xy;

                        _ReadChunkBytes(iff,&xy,sizeof(xy));
                        elm_ptr->x = ((FLOAT)xy.x) / 256.0;
                        elm_ptr->y = ((FLOAT)xy.y) / 256.0;
                        elm_ptr++;
                    };

                    /* Ende-Element aktuelle Outline */
                    elm_ptr->x = -1.0;
                    elm_ptr->y = -1.0;
                    elm_ptr++;

                };  /* nächste Outline im Chunk */
            }; /* if (newo) */

            /*** End Of Chunk holen ***/
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /*** unbekannte Chunks ignorieren ***/
        _SkipChunk(iff);

        /*** Ende der Parse-Schleife ***/
    };

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, amesh_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      15-Mar-95   floh    created
**      16-Mar-95   floh    Bug: neue Pixel2D's wurden falsch gelöscht.
**      08-Jul-95   floh    Outline-Pool jetzt im <struct VFMOutline>-Format
**      17-Jan-96   floh    revised & updated
*/
{
    UWORD i,size;
    struct amesh_data *amd = INST_DATA(cl,o);
    struct IFFHandle *iff  = iffmsg->iffhandle;

    /*** eigene FORM schreiben ***/
    if (_PushChunk(iff,AMESHIFF_FORMID,ID_FORM,IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /* Methode an SuperClass hochreichen */
    if (!_supermethoda(cl,o,OM_SAVETOIFF,iffmsg)) return(FALSE);

    /*** Attribut-Pool-Chunk erzeugen ***/
    _PushChunk(iff,0,AMESHIFF_Attrs,IFFSIZE_UNKNOWN);

    /*** Endian-Konvertierung ***/
    #ifdef _LittleEndian_
    for (i=0; i<amd->NumPolygons; i++) n2vw(&(amd->AttrsPool[i].poly_num));
    #endif

    size = amd->NumPolygons * sizeof(struct amesh_attrs);
    _WriteChunkBytes(iff, amd->AttrsPool, size);
    _PopChunk(iff);

    /*** Endian-Konvertierung rückgängig machen ***/
    #ifdef _LittleEndian_
    for (i=0; i<amd->NumPolygons; i++) v2nw(&(amd->AttrsPool[i].poly_num));
    #endif

    /*** OutlinePool-Chunk schreiben (falls vorhanden) ***/
    if (amd->OutlinePool) {

        _PushChunk(iff,0,AMESHIFF_Outlines,IFFSIZE_UNKNOWN);

        /* für jede Outline... */
        for (i=0; i<amd->NumPolygons; i++) {

            struct VFMOutline *act_elm;
            WORD num_elm, j;

            /* Anzahl Punkte in Outline ermitteln (OHNE Ende-Element) */
            act_elm = amd->OutlinePool[i];
            num_elm = 0;
            while (act_elm++->x >= 0.0) num_elm++;

            /* Anzahl Punkte nach Chunk */
            n2vw(&num_elm);
            _WriteChunkBytes(iff, &num_elm, sizeof(WORD));
            v2nw(&num_elm);

            /* aktuelle Outline Punkt für Punkt nach Chunk */
            act_elm = amd->OutlinePool[i];
            for (j=0; j<num_elm; j++) {

                struct amesh_ol_atom xy;
                xy.x = (BYTE) (act_elm->x * 256.0);
                xy.y = (BYTE) (act_elm->y * 256.0);
                _WriteChunkBytes(iff, &xy, sizeof(xy));
                act_elm++;
            };
        };
        /* Outline-Chunk poppen */
        _PopChunk(iff);
    };

    /*** AMESHIFF FORM ID poppen ***/
    _PopChunk(iff);

    /*** Ende ***/
    return(TRUE);
}

