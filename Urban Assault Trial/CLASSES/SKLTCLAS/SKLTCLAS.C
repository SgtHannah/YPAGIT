/*
**  $Source: PRG:VFM/Classes/_SKLTClass/skltclass.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/01/20 17:08:21 $
**  $Locker:  $
**  $Author: floh $
**
**  Die sklt.class ist eine primitive LoaderClass für das
**  SKLT-Fileformat. Sie ist Subklasse der skeleton.class.
**
**  (C) Copyright 1994,1995,1996 by A.Weissflog
*/
#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "skeleton/skltclass.h"

/*-------------------------------------------------------------------
**  Prototypen und Importe
*/
_dispatcher(Object *, sklt_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, sklt_OM_SAVETOIFF, struct iff_msg *iffmsg);
_dispatcher(struct RsrcNode *, sklt_RSM_CREATE, struct TagItem *tlist);
_dispatcher(ULONG, sklt_RSM_SAVE, struct rsrc_save_msg *msg);

extern struct IFFHandle *sklt_OpenIff(UBYTE *, ULONG);
extern struct RsrcNode *sklt_CreateSkeleton(Object *, Class *, struct TagItem *, struct IFFHandle *);
extern void sklt_CloseIff(struct IFFHandle *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG sklt_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG sklt_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo sklt_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeSKLTClass(ULONG id,...);
__geta4 BOOL FreeSKLTClass(void);
#else
struct ClassInfo *MakeSKLTClass(ULONG id,...);
BOOL FreeSKLTClass(void);
#endif

struct GET_Class sklt_GET = {
    &MakeSKLTClass,
    &FreeSKLTClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&sklt_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *sklt_Entry(void)
{
    return(&sklt_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo sklt_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:sklt.class"   /* der Segment-Name */
    },
    sklt_Entry,                 /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeSKLTClass(ULONG id,...)
#else
struct ClassInfo *MakeSKLTClass(ULONG id,...)
#endif
/*
**  CHANGED
**      26-Mar-94   floh    created
**      13-Nov-94   floh    sklt.class jetzt im Small Data Model,
**                          deshalb alle öffentlichen Routinen mit
**                          __geta4 Prefix.
**      15-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated (so ein Zufall).
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Methoden-Array initialisieren */
    memset(sklt_Methods,0,sizeof(sklt_Methods));

    sklt_Methods[OM_NEWFROMIFF] = (ULONG) sklt_OM_NEWFROMIFF;
    sklt_Methods[OM_SAVETOIFF]  = (ULONG) sklt_OM_SAVETOIFF;
    sklt_Methods[RSM_CREATE]    = (ULONG) sklt_RSM_CREATE;
    sklt_Methods[RSM_SAVE]      = (ULONG) sklt_RSM_SAVE;

    /* ClassInfo-Struktur ausfüllen */
    sklt_clinfo.superclassid = SKELETON_CLASSID;
    sklt_clinfo.methods      = sklt_Methods;
    sklt_clinfo.instsize     = NULL;     /* KEINE EIGENE INSTANCE-DATA ! */
    sklt_clinfo.flags        = 0;

    /* Ende */
    return(&sklt_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeSKLTClass(void)
#else
BOOL FreeSKLTClass(void)
#endif
/*
**  CHANGED
**      26-Mar-94   floh    created
**      13-Nov-94   floh    sklt.class jetzt im Small Data Model,
**                          deshalb alle öffentlichen Routinen mit
**                          __geta4 Prefix.
**      15-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    return(TRUE);
}

/*******************************************************************/
/***    AB HIER METHODEN DISPATCHER    *****************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, sklt_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct RsrcNode *rnode = NULL;
    UBYTE *name;

    /*** RSA_Name ***MUSS*** vorhanden sein ***/
    name = (UBYTE *) _GetTagData(RSA_Name, NULL, tlist);
    if (name) {

        struct IFFHandle *iff;
        BOOL stand_alone = FALSE;

        /*** falls kein IFFHandle, Standalone-File ***/
        iff = (struct IFFHandle *) _GetTagData(RSA_IFFHandle, NULL, tlist);
        if (!iff) {
            iff = sklt_OpenIff(name, IFFF_READ);
            if (!iff) return(NULL);
            stand_alone = TRUE;
        };

        /*** Skeleton erzeugen und ausfüllen ***/
        rnode = sklt_CreateSkeleton(o,cl,tlist,iff);

        /*** falls Standalone-File, IFF-Stream wieder zumachen ***/
        if (stand_alone) sklt_CloseIff(iff);
    };

    /*** das war's ***/
    return(rnode);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, sklt_RSM_SAVE, struct rsrc_save_msg *msg)
/*
**  FUNCTION
**      Sichert die eingebettete Skeleton-Resource als
**      SKLT IFF file in einen Standalone File oder
**      eine Resource-Collection.
**
**  INPUTS
**      msg->name       - Filename relativ zu mc2resources:
**                        Nur, wenn (type == RSRC_SAVE_STANDALONE)
**      msg->iff        - Offener IFF-Stream.
**                        Nur, wenn (type == RSRC_SAVE_COLLECTION)
**      msg->type       - Gibt an, ob sich das Objekt in einen eigenen
**                        File sichern soll, oder in eine Collection
**                        (RSRC_SAVE_STANDALONE oder RSRC_SAVE_COLLECTION)
**
**  RESULTS
**      RSRC_SAVE_STANDALONE, RSRC_SAVE_COLLECTION oder RSRC_SAVE_ERROR
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct IFFHandle *iff;
    struct Skeleton *sklt;

    /*** Ptr auf Skeleton holen ***/
    _get(o, RSA_Handle, &sklt);
    if (!sklt) return(FALSE);

    /*** sicherstellen, daß offenes IFFHandle da ist ***/
    if (RSRC_SAVE_STANDALONE == msg->type) {
        /*** neuer Standalone-IFF-File ***/
        if (msg->name) {
            iff = sklt_OpenIff(msg->name, IFFF_WRITE);
            if (!iff) return(RSRC_SAVE_ERROR);
        } else return(RSRC_SAVE_ERROR);
    } else {
        iff = msg->iff;
        if (!iff) return(RSRC_SAVE_ERROR);
    };

    /*** FORM SKLT ***/
    if (_PushChunk(iff, SKLTIFF_FORMID, ID_FORM, IFFSIZE_UNKNOWN)!=0)
        return(RSRC_SAVE_ERROR);

    /*** SKLTIFF_NEWPOOL Chunk ***/
    if (sklt->Pool) {

        ULONG i;
        ULONG size;
        struct sklt_NewPoolPoint npp;

        size = sklt->NumPoolPoints * sizeof(npp);

        _PushChunk(iff, 0, SKLTIFF_NEWPOOL, size);
        for (i=0; i<sklt->NumPoolPoints; i++) {

            npp.x = sklt->Pool[i].x;
            npp.y = sklt->Pool[i].y;
            npp.z = sklt->Pool[i].z;

            n2vl(&(npp.x));
            n2vl(&(npp.y));
            n2vl(&(npp.z));

            _WriteChunkBytes(iff, &npp, sizeof(npp));
        };
        _PopChunk(iff);
    };

    /*** SKLTIFF_NEWSENSOR Chunk ***/
    if (sklt->SensorPool) {

        ULONG i;
        ULONG size;
        struct sklt_NewPoolPoint npp;

        size = sklt->NumSensorPoints * sizeof(npp);

        _PushChunk(iff, 0, SKLTIFF_NEWSENSOR, size);
        for (i=0; i<sklt->NumSensorPoints; i++) {

            npp.x = sklt->SensorPool[i].x;
            npp.y = sklt->SensorPool[i].y;
            npp.z = sklt->SensorPool[i].z;

            n2vl(&(npp.x));
            n2vl(&(npp.y));
            n2vl(&(npp.z));

            _WriteChunkBytes(iff, &npp, sizeof(npp));
        };
        _PopChunk(iff);
    };

    /*** SKLTIFF_NEWPOLY Chunk ***/
    if (sklt->Areas) {

        ULONG i;
        ULONG num_polys;
        WORD poly_buf[64];

        _PushChunk(iff, 0, SKLTIFF_NEWPOLY, IFFSIZE_UNKNOWN);

        /*** schreibe num_polys ***/
        num_polys = sklt->NumAreas;
        n2vl(&num_polys);
        _WriteChunkBytes(iff, &num_polys, sizeof(num_polys));

        /*** dann ein Polygon nach dem anderen ***/
        for (i=0; i<sklt->NumAreas; i++) {

            WORD n  = sklt->Areas[i][0];    // Anzahl Eckpunkte
            WORD j;

            /*** schreibe + endian-konvertiere Anzahl Eckpunkte ***/
            poly_buf[0] = n;
            n2vw(&poly_buf[0]);

            /*** schreibe + endian-konvertiere den Rest ***/
            for (j=0; j<n; j++) {
                poly_buf[j+1] = sklt->Areas[i][j+1];
                n2vw(&poly_buf[j+1]);
            };
            _WriteChunkBytes(iff, poly_buf, (n+1)*sizeof(WORD));
        };

        /*** NEWPOLY-Chunk fertig ***/
        _PopChunk(iff);
    };

    /*** FORM SKLT poppen ***/
    if (_PopChunk(iff) != 0) return(RSRC_SAVE_ERROR);

    /*** falls Standalone-File, IFF-Stream schließen ***/
    if (RSRC_SAVE_STANDALONE == msg->type) sklt_CloseIff(iff);

    /*** Ende ***/
    return(msg->type);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, sklt_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      26-Mar-94   floh    created
**      28-Mar-94   floh    DisposeObject() ersetzt durch OM_DISPOSE
**      13-Nov-94   floh    sklt.class jetzt im Small Data Model,
**                          deshalb alle öffentlichen Routinen mit
**                          __geta4 Prefix.
**      15-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    /* Pointer auf IFFHandle aus Msg holen */
    struct IFFHandle *iff = iffmsg->iffhandle;
    UBYTE name[255];
    BOOL name_exists = FALSE;
    LONG ifferror;
    struct ContextNode *cn;
    Object *newo = NULL;


    while ((ifferror = _ParseIFF(iff,IFFPARSE_RAWSTEP)) != IFFERR_EOC) {

        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror != 0) return(NULL);

        cn = _CurrentChunk(iff);

        /* Name-Chunk? */
        if (cn->cn_ID == SKLTCLIFF_NAME) {

            /* Name lesen... */
            _ReadChunkBytes(iff,name,sizeof(name));
            name_exists = TRUE;

            /* EndOfChunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);
    };

    /*** erzeuge ein sklt.class Objekt ***/
    if (name_exists) {

        struct TagItem new_attrs[3];

        new_attrs[0].ti_Tag  = RSA_Name;
        new_attrs[0].ti_Data = (ULONG) name;
        new_attrs[1].ti_Tag  = RSA_Access;
        new_attrs[1].ti_Data = ACCESS_SHARED;
        new_attrs[2].ti_Tag  = TAG_DONE;

        /*** siehe ilbm.class für eine Erklärung des folgenden ***/
        newo = (Object *) _supermethoda(cl,o,OM_NEW,new_attrs);
    };

    /* Ende */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, sklt_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      26-Mar-94   floh    created
**      13-Nov-94   floh    sklt.class jetzt im Small Data Model,
**                          deshalb alle öffentlichen Routinen mit
**                          __geta4 Prefix.
**      15-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    UBYTE *name;
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* eigenen FORM schreiben */
    if (_PushChunk(iff,SKLTCLIFF_FORMID,ID_FORM,IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /* Methode NICHT an SuperClass hochreichen */

    /* SKLTCLIFF_Name Chunk schreiben */
    _PushChunk(iff,0,SKLTCLIFF_NAME, IFFSIZE_UNKNOWN);

    /* Resource-Name ermitteln */
    _get(o, RSA_Name, &name);

    /* Name nach Chunk und diesen poppen */
    _WriteChunkBytes(iff,name,strlen(name)+1);
    _PopChunk(iff);

    /* FORM-Chunk poppen */
    if (_PopChunk(iff) != 0) return(FALSE);

    /* Ende */
    return(TRUE);
}

