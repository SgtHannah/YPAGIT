/*
**  $Source: PRG:VFM/Classes/_ParticleClass/pl_main.c,v $
**  $Revision: 38.6 $
**  $Date: 1996/11/10 21:03:43 $
**  $Locker:  $
**  $Author: floh $
**
**  Das ultimative ADE-Partikel-System ;-)
**
**  (C) Copyright 1995-1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "engine/engine.h"
#include "transform/te.h"

#include "skeleton/skeletonclass.h"
#include "ade/particleclass.h"

/*-------------------------------------------------------------------
**  Prototypen
*/
_dispatcher(Object *, prtl_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, prtl_OM_DISPOSE, void *ignore);
_dispatcher(void, prtl_OM_SET, struct TagItem *attrs);
_dispatcher(void, prtl_OM_GET, struct TagItem *attrs);
_dispatcher(Object *, prtl_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, prtl_OM_SAVETOIFF, struct iff_msg *iffmsg);

_dispatcher(void, prtl_PTLM_ACCEL, struct ptl_vector_msg *msg);
_dispatcher(void, prtl_PTLM_GETACCEL, struct ptl_vector_msg *msg);
_dispatcher(void, prtl_PTLM_MAGNIFY, struct ptl_vector_msg *msg);
_dispatcher(void, prtl_PTLM_GETMAGNIFY, struct ptl_vector_msg *msg);

_dispatcher(void, prtl_ADEM_PUBLISH, struct publish_msg *msg);

_dispatcher(ULONG, prtl_PTLM_ADDADE, Object **msg);
_dispatcher(Object *, prtl_PTLM_REMADE, ULONG *msg);
_dispatcher(void, prtl_PTLM_ADEUP, ULONG *msg);
_dispatcher(void, prtl_PTLM_ADEDOWN, ULONG *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_tform_engine

#ifdef AMIGA
__far ULONG prtl_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG prtl_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo prtl_clinfo;

/*-------------------------------------------------------------------
**  Strukturen zur Klasseneinbindung
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeParticleClass(ULONG id,...);
__geta4 BOOL FreeParticleClass(void);
#else
struct ClassInfo *MakeParticleClass(ULONG id,...);
BOOL FreeParticleClass(void);
#endif

struct GET_Class prtl_GET = {
    &MakeParticleClass,             /* MakeExternClass() */
    &FreeParticleClass,             /* FreeVASAClass()   */
};

#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&prtl_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *prtl_Entry(void)
{
    return(&prtl_GET);
};

struct SegInfo prtl_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:particle.class"   /* der Segment-Name */
    },
    prtl_Entry,  /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeParticleClass(ULONG id,...)
#else
struct ClassInfo *MakeParticleClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet die particle.class im Nucleus-System an.
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
**      04-Sep-95   floh    created
**      12-Sep-95   floh    Ooops, hatte ich doch die particle.class
**                          als Superklasse der particle.class angegeben...
**      21-Sep-96   floh    + prtl_AllocSize, prtl_AllocMax Initialisierung
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray fischen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    _get_tform_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(prtl_Methods,0,sizeof(prtl_Methods));

    prtl_Methods[OM_NEW]        = (ULONG) prtl_OM_NEW;
    prtl_Methods[OM_DISPOSE]    = (ULONG) prtl_OM_DISPOSE;
    prtl_Methods[OM_SET]        = (ULONG) prtl_OM_SET;
    prtl_Methods[OM_GET]        = (ULONG) prtl_OM_GET;
    prtl_Methods[OM_NEWFROMIFF] = (ULONG) prtl_OM_NEWFROMIFF;
    prtl_Methods[OM_SAVETOIFF]  = (ULONG) prtl_OM_SAVETOIFF;

    prtl_Methods[PTLM_ACCEL]      = (ULONG) prtl_PTLM_ACCEL;
    prtl_Methods[PTLM_MAGNIFY]    = (ULONG) prtl_PTLM_MAGNIFY;
    prtl_Methods[PTLM_GETACCEL]   = (ULONG) prtl_PTLM_GETACCEL;
    prtl_Methods[PTLM_GETMAGNIFY] = (ULONG) prtl_PTLM_GETMAGNIFY;

    prtl_Methods[PTLM_ADDADE]     = (ULONG) prtl_PTLM_ADDADE;
    prtl_Methods[PTLM_REMADE]     = (ULONG) prtl_PTLM_REMADE;
    prtl_Methods[PTLM_ADEUP]      = (ULONG) prtl_PTLM_ADEUP;
    prtl_Methods[PTLM_ADEDOWN]    = (ULONG) prtl_PTLM_ADEDOWN;

    prtl_Methods[ADEM_PUBLISH]  = (ULONG) prtl_ADEM_PUBLISH;

    /* ClassInfo Struktur ausfüllen */
    prtl_clinfo.superclassid = ADE_CLASSID;
    prtl_clinfo.methods      = prtl_Methods;
    prtl_clinfo.instsize     = sizeof(struct particle_data);
    prtl_clinfo.flags        = 0;

    /* und das war alles... */
    return(&prtl_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeParticleClass(void)
#else
BOOL FreeParticleClass(void)
#endif
/*
**  CHANGED
**      04-Sep-95   floh    created
**      21-Sep-96   floh    prtl_AllocMax wird ausgegeben
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
APTR prtl_AllocVec(ULONG byteSize, ULONG attributes)
/*
**  FUNCTION
**      Verwaltet zusätzlich prtl_AllocSize und prtl_AllocMax.
**      IST DARAUF ANGEWIESEN, DASS DIE NUCLEUS-ALLOC-FUNKTION
**      IN DEN ERSTEN 4 BYTES DIE BLOCK-GRÖSSE MITSCHREIBT!
**
**  CHANGED
**      21-Sep-96   floh    created
*/
{
    APTR block = _AllocVec(byteSize,attributes);
    return(block);
}

/*-----------------------------------------------------------------*/
void prtl_FreeVec(APTR memoryBlock)
/*
**  FUNCTION
**      Verwaltet zusätzlich prtl_AllocSize und prtl_AllocMax.
**      IST DARAUF ANGEWIESEN, DASS DIE NUCLEUS-ALLOC-FUNKTION
**      IN DEN ERSTEN 4 BYTES DIE BLOCK-GRÖSSE MITSCHREIBT!
**
**  CHANGED
**      21-Sep-96   floh    created
*/
{
    _FreeVec(memoryBlock);
}

/*-----------------------------------------------------------------*/
BOOL prtl_MakeSkeleton(struct particle_data *pd)
/*
**  FUNCTION
**      Erzeugt ein "Fake Skeleton Object", als ExtSkeleton
**      für das eingebettete ADE.
**
**  CHANGED
**      04-Sep-95   floh    created
**      13-Sep-95   floh    der einzelne "Point" ist jetzt Bestandteil
**                          des 4-eckigen "Polygons", also nur noch 4 Punkte
**                          im Skeleton-Pool
**      14-Oct-95   floh    Ha... der Point-Pool wird ja überhaupt nicht
**                          benötigt... also wech damit.
**      17-Jan-96   floh    mit der neuen skeleton.class hat sich die
**                          On-The-Fly-Erzeugung eines Skeleton-Objects
**                          erheblich vereinfacht...
*/
{
    /*** ein "leeres" Skeleton-Objekt erzeugen ***/
    pd->Skeleton = _new("skeleton.class",
                        RSA_Name,   "particle_sklt",
                        RSA_Access, ACCESS_EXCLUSIVE,
                        SKLA_NumPoints,     5,
                        SKLA_NumPolygons,   1,
                        SKLA_NumPolyPoints, 4,
                        TAG_DONE);

    if (pd->Skeleton) {

        WORD *poly;

        /*** hole eingebettete Skeleton-Struktur ***/
        _get(pd->Skeleton, SKLA_Skeleton, &(pd->Sklt));

        /*** Polygon-Pool initialisieren ***/
        poly = pd->Sklt->Areas[0];
        poly[0] = 4;        // Anzahl Eckpunkte
        poly[1] = 1;        // Pool-Point 0 ist für Point-ADEs
        poly[2] = 2;
        poly[3] = 3;
        poly[4] = 4;

        /*** Success ***/
        return(TRUE);

    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL prtl_NewContexts(struct particle_data *pd)
/*
**  FUNCTION
**      Allokiert und initialisiert ein neues Context-Array,
**      entsprechend den in der LID bereits vorhandenen 
**      Informationen. Existierte vorher bereits ein Context-Array,
**      wird dieses zuerst korrekt freigegeben.
**
**  INPUTS
**      pd  - Pointer auf LID, soweit wie möglich initialisiert.
**            Zumindest folgende Attribute sollten bereits gesetzt
**            worden sein:
**                  PTLA_NumContexts
**                  PTLA_BirthRate
**                  PTLA_Lifetime
**                  PTLA_StartGeneration
**                  PTLA_StopGeneration
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> kein Speicher
**
**  CHANGED
**      04-Sep-95   floh    created
**      14-Sep-95   floh    Falls die Zeitspanne, innerhalb der Partikel
**                          generiert werden kleiner ist als die
**                          Lifetime eines Partikels, wird ersteres
**                          für die Berechnen der max. parallel existierenden
**                          Partikel-Anzahl verwendet. Das spart Platz
**                          für "Explosions-Partikel-Systeme", die eine enorme
**                          Ausstoßrate über einen sehr kurzen Zeitraum
**                          haben.
**      10-Oct-95   floh    Ooops, beim Freigeben der alten Kontexte
**                          wurde pd->NumContexts als Zähler verwendet,
**                          obwohl dieses schon auf die Anzahl der
**                          neuen Kontexte initialisiert ist!
**      17-Jan-96   floh    revised & updated
*/
{
    ULONG i;
    BOOL retval = FALSE;

    /*** existiert bereits ein Kontext-Array? ***/
    if (pd->CtxStart) {

        struct Context *ctx = pd->CtxStart;

        /* dann freigeben */
        while (ctx != pd->CtxEnd) {
            if (ctx->Start) prtl_FreeVec(ctx->Start);
            ctx++;
        };
        prtl_FreeVec(pd->CtxStart);
    };

    /*** neues Kontext-Array mit Partikel-Spaces allokieren ***/
    pd->CtxStart = (struct Context *)
        prtl_AllocVec(pd->NumContexts * sizeof(struct Context),
                      MEMF_PUBLIC|MEMF_CLEAR);

    if (pd->CtxStart) {

        ULONG num_prts;   // Anzahl Partikel in einem Kontext-Space
        LONG par_time;
        BOOL ok = TRUE;

        /*** die "richtige" Zeitspanne wählen, in die die Partikel ***/
        /*** max. Anzahl parallel existierender Partikel paßt      ***/
        if ((pd->Lifetime) < (pd->CtxStopGen-pd->CtxStartGen)) {
            par_time = pd->Lifetime;
        } else {
            par_time = pd->CtxStopGen - pd->CtxStartGen;
        };

        /*** max Anzahl Partikel = ParallelTime * Ausstoßrate (in sec!) ***/
        num_prts = ((par_time * pd->Birthrate)>>10) + 2;

        for (i=0; i<(pd->NumContexts); i++) {

            /*** einen Partikel-Space für jeden Kontext allokieren ***/
            struct Particle *prtl;
            struct Context *ctx = &(pd->CtxStart[i]);

            prtl = (struct Particle *)
            prtl_AllocVec(num_prts*sizeof(struct Particle),MEMF_PUBLIC|MEMF_CLEAR);
            if (prtl) {
                ctx->Start = prtl;
                ctx->End   = prtl + num_prts;
            } else {
                ok=FALSE;
            };
        };

        /*** Kontext-Ringpointer initialisieren ***/
        if (ok) {
            pd->CtxEnd  = pd->CtxStart + pd->NumContexts;
            pd->CtxNext = pd->CtxStart;
            retval = TRUE;
        };
    };

    /*** Ende ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
void prtl_RefreshVecs(struct particle_data *pd)
/*
**  FUNCTION
**      Berechnet alle resultierenden Parameter der
**      3D-Vektoren neu. Muß aufgerufen werden, wenn sich
**      einer der folgenden Komponenten ändert:
**
**          pd->StartAccel
**          pd->EndAccel
**          pd->StartMagnify
**          pd->EndMagnify
**          pd->CtxLifetime
**
**      Momentan existierende Kontexte und deren Partikel werden
**      nicht beeinflußt.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
*/
{
    FLOAT lft = (FLOAT) pd->CtxLifetime;

    pd->AddAccel.x = (pd->EndAccel.x - pd->StartAccel.x)/lft;
    pd->AddAccel.y = (pd->EndAccel.y - pd->StartAccel.y)/lft;
    pd->AddAccel.z = (pd->EndAccel.z - pd->StartAccel.z)/lft;

    pd->AddMagnify.x = (pd->EndMagnify.x - pd->StartMagnify.x)/lft;
    pd->AddMagnify.y = (pd->EndMagnify.y - pd->StartMagnify.y)/lft;
    pd->AddMagnify.z = (pd->EndMagnify.z - pd->StartMagnify.z)/lft;
}

/*-----------------------------------------------------------------*/
void prtl_RefreshParticles(struct particle_data *pd)
/*
**  FUNCTION
**      Berechnet alle resultierenden Parameter für
**      die Partikel neu. Muß aufgerufen werden, wenn
**      sich eine der folgenden komponenten geändert haben:
**
**          pd->Lifetime
**          pd->Birthrate
**          pd->StartSize
**          pd->EndSize
**
**      *** WICHTIG ***
**      Bitte beachten, daß eine Änderung von pd->Lifetime oder 
**      pd->BirthRate auch eine Neuallokierung der Context-Strukturen
**      mit den Partikel-Arrays erfordert. Dafür muß die Routine
**      NewContexts() extra bemüht werden!
**
**      Existierende Partikel-Spaces werden nicht beeinflußt, Änderungen
**      machen sich erst bei der Initialisierung eines neuen Contexts
**      bemerkbar.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
*/
{
    FLOAT lft = (FLOAT) pd->Lifetime;

    pd->Threshold   = 1024/(pd->Birthrate); // millisec<->sec!
    pd->AddSize     = (pd->EndSize - pd->StartSize)/lft;
}

/*-----------------------------------------------------------------*/
void prtl_RefreshADEs(struct particle_data *pd)
/*
**  FUNCTION
**      Updatet folgende Sachen:
**          pd->ADETimeDiff
**
**      Muß aufgerufen werden, wenn sich eine der folgenden
**      Sachen ändern:
**
**          pd->Lifetime
**          pd->NumADEs
**          ADEA_DepthFade
**
**      Außerdem werden alle Partikel
**
**  INPUTS
**      pd  -> Ptr auf LID
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    BOOL dfade = (pd->Flags & PTLF_DepthFade) ? TRUE:FALSE;
    ULONG i;

    if (pd->NumADEs > 0) {

        for (i=0; i<pd->NumADEs; i++) {
            if (pd->ADEArray[i]) {
                _method(pd->ADEArray[i], OM_SET,
                        ADEA_DepthFade,   dfade,
                        ADEA_Point,       0,           // Mittelpunkt...
                        ADEA_Polygon,     0,           // Poly-Nummer
                        TAG_DONE);
            };
        };

        pd->ADETimeDiff = pd->Lifetime / pd->NumADEs;
    };
}

/*-----------------------------------------------------------------*/
void prtl_NewADEs(struct particle_data *pd,
                  Object **ades)
/*
**  FUNCTION
**      Wird aufgerufen, wenn neue ADEs an das Partikel-Objekt
**      gehängt werden sollen. Falls bereits ADEs angebunden
**      sind, werden diese gelyncht.
**      Alle ADE-Parameter werden neu berechnet.
**
**  INPUTS
**      pd   -> Ptr auf LID
**      ades -> Ptr auf Null-terminiertes ADE-Pointer-Array
**
**  RESULTS
**
**  CHANGED
**      04-Oct-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    ULONG i;

    /* erstmal alle existierenden ADEs killen */
    for (i=0; i<pd->NumADEs; i++) {
        if (pd->ADEArray[i]) _dispose(pd->ADEArray[i]);
    };

    /* dann die neuen rübernehmen und zählen */
    i=0;
    while (ades[i]) {

        pd->ADEArray[i] = ades[i];
        i++;
    };
    pd->NumADEs     = i;
    pd->ADEArray[i] = NULL;

    /* neue ADEs initialisieren */
    prtl_RefreshADEs(pd);
}

/*-----------------------------------------------------------------*/
BOOL prtl_initAttrs(struct particle_data *pd, struct TagItem *attrs)
/*
**  CHANGED
**      04-Sep-95   floh    created - das Berechnen der resultierenden
**                          Parameter wird extern in OM_NEW übernommen,
**                          nachdem prtl_initAttrs() ausgeführt wurde!
**      12-Sep-95   floh    debugging...
**      13-Sep-95   floh    PTLA_StartGeneration und PTLA_StopGeneration
**                          + PTLA_Noise
**                          - PTLA_StartMass, PTLA_EndMass
**      04-Oct-95   floh    * PTLA_ADE Handling angepaßt
**                          + PTLA_ADEArray
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Default-Attribut-Values initialisieren */
    pd->StartSpeed  = (FLOAT) PTLA_StartSpeed_DEF;
    pd->NumContexts = PTLA_NumContexts_DEF;
    pd->CtxLifetime = PTLA_ContextLifetime_DEF;
    pd->Birthrate   = PTLA_BirthRate_DEF;
    pd->Lifetime    = PTLA_Lifetime_DEF;
    pd->StartSize   = (FLOAT) PTLA_StartSize_DEF;
    pd->EndSize     = (FLOAT) PTLA_EndSize_DEF;

    pd->CtxStartGen = PTLA_StartGeneration_DEF;
    pd->CtxStopGen  = PTLA_StopGeneration_DEF;

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
                        if (data) pd->Flags |= PTLF_DepthFade;
                        else      pd->Flags &= ~PTLF_DepthFade;
                        break;

                    case ADEA_Point:
                        pd->PointNum = data;
                        break;
                };

                /* particle.class Attribute */
                switch(tag) {
                    case PTLA_StartSpeed:
                        pd->StartSpeed = (FLOAT) ((LONG)data);
                        break;

                    case PTLA_NumContexts:
                        pd->NumContexts = data;
                        break;

                    case PTLA_ContextLifetime:
                        pd->CtxLifetime = data;
                        break;

                    case PTLA_BirthRate:
                        pd->Birthrate = data;
                        break;

                    case PTLA_Lifetime:
                        pd->Lifetime = data;
                        break;

                    case PTLA_ADE:
                        {
                            Object *local_ades[2];
                            local_ades[0] = (Object *) data;
                            local_ades[1] = NULL;
                            prtl_NewADEs(pd, local_ades);
                        };
                        break;

                    case PTLA_StartSize:
                        pd->StartSize = (FLOAT) ((LONG)data);
                        break;

                    case PTLA_EndSize:
                        pd->EndSize = (FLOAT) ((LONG)data);
                        break;

                    case PTLA_StartGeneration:
                        pd->CtxStartGen = data;
                        break;

                    case PTLA_StopGeneration:
                        pd->CtxStopGen = data;
                        break;

                    case PTLA_Noise:
                        pd->Noise = ((FLOAT)((LONG)data))/10.0;
                        break;

                    case PTLA_ADEArray:
                        prtl_NewADEs(pd, (Object **) data);
                        break;
                };
        };
    };

    /*** der Rest wird extern erledigt ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL prtl_setAttrs(struct particle_data *pd, struct TagItem *attrs)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      13-Sep-95   floh    neu: PTLA_StartGeneration und PTLA_StopGeneration
**                          + PTLA_Noise
**                          - PTLA_StartMass, PTLA_EndMass
**      04-Oct-95   floh    * verändertes PTLA_ADE Handling
**                          + PTLA_ADEArray
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Flags, welche Subsystem refreshed werden müssen */
    BOOL refresh_contexts  = FALSE;      // Neuallokation notwendig
    BOOL refresh_particles = FALSE;
    BOOL refresh_vecs      = FALSE;
    BOOL refresh_ades      = FALSE;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* die ade.class Attribute */
                switch(tag) {

                    case ADEA_DepthFade:
                        if (data) pd->Flags |= PTLF_DepthFade;
                        else      pd->Flags &= ~PTLF_DepthFade;
                        refresh_ades = TRUE;
                        break;

                    case ADEA_Point:
                        pd->PointNum = data;
                        break;
                };

                /* particle.class Attribute */
                switch(tag) {
                    case PTLA_StartSpeed:
                        pd->StartSpeed = (FLOAT) ((LONG)data);
                        break;

                    case PTLA_NumContexts:
                        pd->NumContexts  = data;
                        refresh_contexts = TRUE;
                        break;

                    case PTLA_ContextLifetime:
                        pd->CtxLifetime = data;
                        refresh_vecs = TRUE;
                        break;

                    case PTLA_BirthRate:
                        pd->Birthrate = data;
                        refresh_contexts  = TRUE;   // Neu allokieren!!!
                        refresh_particles = TRUE;
                        break;

                    case PTLA_Lifetime:
                        pd->Lifetime = data;
                        refresh_contexts  = TRUE;   // Neu allokieren!!!
                        refresh_particles = TRUE;
                        refresh_ades      = TRUE;
                        break;

                    case PTLA_ADE:
                        {
                            Object *local_ades[2];
                            local_ades[0] = (Object *) data;
                            local_ades[1] = NULL;
                            prtl_NewADEs(pd, (Object **) local_ades);
                        };
                        break;

                    case PTLA_StartSize:
                        pd->StartSize = (FLOAT) ((LONG)data);
                        refresh_particles = TRUE;
                        break;

                    case PTLA_EndSize:
                        pd->EndSize = (FLOAT) ((LONG)data);
                        refresh_particles = TRUE;
                        break;

                    case PTLA_StartGeneration:
                        pd->CtxStartGen  = data;
                        refresh_contexts = TRUE;
                        break;

                    case PTLA_StopGeneration:
                        pd->CtxStopGen   = data;
                        refresh_contexts = TRUE;
                        break;

                    case PTLA_Noise:
                        pd->Noise = ((FLOAT)((LONG)data))/10.0;
                        break;

                    case PTLA_ADEArray:
                        prtl_NewADEs(pd, (Object **) data);
                        break;
                };
        };
    };

    /*** Subsysteme refreshen, falls notwendig ***/
    if (refresh_contexts) {
        if (!prtl_NewContexts(pd)) return(FALSE);
    };
    if (refresh_vecs)      prtl_RefreshVecs(pd);
    if (refresh_particles) prtl_RefreshParticles(pd);
    if (refresh_ades)      prtl_RefreshADEs(pd);

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void prtl_getAttrs(struct particle_data *pd, struct TagItem *attrs)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      13-Sep-95   floh    neu: PTLA_StartGeneration und PTLA_StopGeneration
**                          + PTLA_Noise
**                          - PTLA_StartMass, PTLA_EndMass
**      04-Oct-95   floh    * PTLA_ADE Handling verändert
**                          + PTLA_ADEArray
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch(tag) {

            /* die System-Tags */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

                /* die Attribute */
                switch (tag) {

                    case PTLA_StartSpeed:
                        *value = (LONG) pd->StartSpeed;
                        break;

                    case PTLA_NumContexts:
                        *value = pd->NumContexts;
                        break;

                    case PTLA_ContextLifetime:
                        *value = pd->CtxLifetime;
                        break;

                    case PTLA_BirthRate:
                        *value = pd->Birthrate;
                        break;

                    case PTLA_Lifetime:
                        *value = pd->Lifetime;
                        break;

                    case PTLA_ADE:
                        *value = (ULONG) pd->ADEArray[0];
                        break;

                    case PTLA_StartSize:
                        *value = (LONG) pd->StartSize;
                        break;

                    case PTLA_EndSize:
                        *value = (LONG) pd->EndSize;
                        break;

                    case PTLA_StartGeneration:
                        *value = pd->CtxStartGen;
                        break;

                    case PTLA_StopGeneration:
                        *value = pd->CtxStopGen;
                        break;

                    case PTLA_Noise:
                        *value = (LONG) (pd->Noise*10.0);
                        break;

                    case PTLA_ADEArray:
                        *value = (LONG) &(pd->ADEArray[0]);
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
_dispatcher(Object *, prtl_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    Object *newo;
    struct particle_data *pd;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    pd = INST_DATA(cl,newo);

    /*** internes Skeleton basteln ***/
    if (!prtl_MakeSkeleton(pd)) {
        _methoda(newo, OM_DISPOSE, 0);
        return(NULL);
    };

    /*** (I)-Attribute auswerten ***/
    if (!prtl_initAttrs(pd,attrs)) {
        _methoda(newo, OM_DISPOSE, 0);
        return(NULL);
    };

    /*** resultierende Parameter initialisieren ***/
    if (!prtl_NewContexts(pd)) {
        _methoda(newo, OM_DISPOSE, 0);
        return(NULL);
    };
    prtl_RefreshVecs(pd);
    prtl_RefreshParticles(pd);
    prtl_RefreshADEs(pd);

    /*** that's it! ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, prtl_OM_DISPOSE, void *ignore)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      13-Sep-95   floh    debugging...
**      04-Oct-95   floh    es existieren jetzt 1..pd->NumADEs
**                          ADEs, die disposed werden müssen.
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    ULONG i;

    /*** Speicherbereiche ***/
    if (pd->CtxStart) {
        for (i=0; i<(pd->NumContexts); i++) {
            if (pd->CtxStart[i].Start) {
                prtl_FreeVec(pd->CtxStart[i].Start);
            };
        };
        prtl_FreeVec(pd->CtxStart);
    };

    /*** eingebettete Objects und internes Skeleton ***/
    for (i=0; i<pd->NumADEs; i++) {
        if (pd->ADEArray[i]) _dispose(pd->ADEArray[i]);
    };
    if (pd->Skeleton) _dispose(pd->Skeleton);

    /* OM_DISPOSE an Parent-Klasse */
    return((BOOL) _supermethoda(cl,o,OM_DISPOSE,ignore));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    prtl_setAttrs(pd, attrs);
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    prtl_getAttrs(pd, attrs);
    _supermethoda(cl,o,OM_GET,attrs);
}

/*=================================================================*/
BOOL handle_ATTR(Object *newo, struct particle_data *pd, struct IFFHandle *iff)
{
    if (newo) {

        struct part_IFFAttr iffattr;
        struct TagItem tags[16];
        struct ptl_vector_msg vec;

        /* lese und endian-konvertiere Chunk-Inhalt */
        _ReadChunkBytes(iff, &iffattr, sizeof(iffattr));

        v2nw(&(iffattr.version));

        v2nl(&(iffattr.start_accel.x));
        v2nl(&(iffattr.start_accel.y));
        v2nl(&(iffattr.start_accel.z));

        v2nl(&(iffattr.end_accel.x));
        v2nl(&(iffattr.end_accel.y));
        v2nl(&(iffattr.end_accel.z));

        v2nl(&(iffattr.start_magnify.x));
        v2nl(&(iffattr.start_magnify.y));
        v2nl(&(iffattr.start_magnify.z));

        v2nl(&(iffattr.end_magnify.x));
        v2nl(&(iffattr.end_magnify.y));
        v2nl(&(iffattr.end_magnify.z));

        v2nl(&(iffattr.start_speed));
        v2nl(&(iffattr.num_contexts));
        v2nl(&(iffattr.ctx_lifetime));
        v2nl(&(iffattr.ctx_startgen));
        v2nl(&(iffattr.ctx_stopgen));
        v2nl(&(iffattr.birthrate));
        v2nl(&(iffattr.lifetime));
        v2nl(&(iffattr.start_size));
        v2nl(&(iffattr.end_size));
        v2nl(&(iffattr.noise));

        /* initialisiere (I)-TagList */
        if (iffattr.version >= 1) {

            tags[0].ti_Tag  = PTLA_StartSpeed;
            tags[0].ti_Data = iffattr.start_speed;

            tags[1].ti_Tag  = PTLA_NumContexts;
            tags[1].ti_Data = iffattr.num_contexts;

            tags[2].ti_Tag  = PTLA_ContextLifetime;
            tags[2].ti_Data = iffattr.ctx_lifetime;

            tags[3].ti_Tag  = PTLA_BirthRate;
            tags[3].ti_Data = iffattr.birthrate;

            tags[4].ti_Tag  = PTLA_Lifetime;
            tags[4].ti_Data = iffattr.lifetime;

            tags[5].ti_Tag  = PTLA_StartSize;
            tags[5].ti_Data = iffattr.start_size;

            tags[6].ti_Tag  = PTLA_EndSize;
            tags[6].ti_Data = iffattr.end_size;

            tags[7].ti_Tag  = PTLA_StartGeneration;
            tags[7].ti_Data = iffattr.ctx_startgen;

            tags[8].ti_Tag  = PTLA_StopGeneration;
            tags[8].ti_Data = iffattr.ctx_stopgen;

            tags[9].ti_Tag  = PTLA_Noise;
            tags[9].ti_Data = iffattr.noise;
        };
        tags[10].ti_Tag = TAG_DONE;
        _methoda(newo, OM_SET, tags);

        vec.start_vec = iffattr.start_accel;
        vec.end_vec   = iffattr.end_accel;
        _methoda(newo, PTLM_ACCEL, &vec);

        vec.start_vec = iffattr.start_magnify;
        vec.end_vec   = iffattr.end_magnify;
        _methoda(newo, PTLM_MAGNIFY, &vec);

        /*** that's it, basically :-))) ***/
    };

    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, prtl_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
**      13-Sep-95   floh    neu: PTLA_StartGeneration und 
**                          PTLA_StopGeneration
**                          + PTLA_Noise
**                          - PTLA_StartMass, PTLA_EndMass
**      04-Oct-95   floh    Es können jetzt beliebig viele
**                          ADE-Objekt-Chunks vorhanden sein.
**      17-Jan-96   floh    revised & updated
*/
{
    struct IFFHandle *iff    = iffmsg->iffhandle;
    Object *newo             = NULL;
    struct particle_data *pd = NULL;
    ULONG ifferror;
    struct ContextNode *cn;
    Object *ades[PTL_MAXNUMADES];
    ULONG act_ade = 0;

    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror) {
            if (newo) _methoda(newo, OM_DISPOSE, 0);
            return(NULL);
        };

        cn = _CurrentChunk(iff);

        /* FORM der SuperClass? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == ADEIFF_FORMID)) {

            newo = (Object *) _supermethoda(cl,o,OM_NEWFROMIFF,iffmsg);
            if (!newo) return(NULL);
            pd = INST_DATA(cl,newo);

            continue;
        };

        /* AREAIFF_Attrs Chunk? ***/
        if (cn->cn_ID == PARTIFF_Attrs) {
            if (!handle_ATTR(newo,pd,iff)) {
                _methoda(newo,OM_DISPOSE,0); return(NULL);
            };
            /* EndOfChunk holen */
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /* Object Chunk? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == FORM_Object)) {

            ades[act_ade] = _LoadInnerObject(iff);
            if (ades[act_ade] == NULL) {
                _methoda(newo,OM_DISPOSE,0); return(NULL);
            };
            act_ade++;

            /* KEIN EndOfChunk */
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);

        /* Ende der Parse-Schleife */
    };

    if (newo) {

        /*** internes Skeleton basteln ***/
        if (!prtl_MakeSkeleton(pd)) {
            _methoda(newo,OM_DISPOSE,0);
            return(FALSE);
        };

        /*** "Subsysteme" initialisieren ***/
        if (!prtl_NewContexts(pd)) {
            _methoda(newo,OM_DISPOSE,0);
            return(FALSE);
        };
        ades[act_ade] = NULL;
        prtl_RefreshVecs(pd);
        prtl_RefreshParticles(pd);
        prtl_NewADEs(pd,ades);
    };

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, prtl_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      04-Sep-95   floh    created
**      13-Sep-95   floh    neu: PTLA_StartGeneration und 
**                          PTLA_StopGeneration
**                          + PTLA_Noise
**                          - PTLA_StartMass, PTLA_EndMass
**      04-Oct-95   floh    es werden jetzt 1..n ADEs gesaved.
**      17-Jan-96   floh    revised & updated
*/
{
    ULONG i;
    struct particle_data *pd = INST_DATA(cl,o);
    struct IFFHandle *iff    = iffmsg->iffhandle;
    struct part_IFFAttr iffattr;

    /* eigene FORM schreiben */
    if (_PushChunk(iff, PARTIFF_FORMID, ID_FORM, IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /* Methode an SuperClass */
    if (!_supermethoda(cl, o, OM_SAVETOIFF, iffmsg)) return(FALSE);

    /* PARTIFF_Attrs Chunk erzeugen */
    _PushChunk(iff, 0, PARTIFF_Attrs, IFFSIZE_UNKNOWN);

    iffattr.version = PARTIFF_VERSION;
    iffattr.start_accel   = pd->StartAccel;
    iffattr.end_accel     = pd->EndAccel;
    iffattr.start_magnify = pd->StartMagnify;
    iffattr.end_magnify   = pd->EndMagnify;

    iffattr.start_speed   = (LONG) pd->StartSpeed;
    iffattr.num_contexts  = pd->NumContexts;
    iffattr.ctx_lifetime  = pd->CtxLifetime;
    iffattr.ctx_startgen  = pd->CtxStartGen;
    iffattr.ctx_stopgen   = pd->CtxStopGen;
    iffattr.birthrate     = pd->Birthrate;
    iffattr.lifetime      = pd->Lifetime;
    iffattr.start_size    = (LONG) pd->StartSize;
    iffattr.end_size      = (LONG) pd->EndSize;
    iffattr.noise         = (LONG) (pd->Noise * 10.0);

    /*** Endian-Konvertierung ***/
    n2vw(&(iffattr.version));

    n2vl(&(iffattr.start_accel.x));
    n2vl(&(iffattr.start_accel.y));
    n2vl(&(iffattr.start_accel.z));

    n2vl(&(iffattr.end_accel.x));
    n2vl(&(iffattr.end_accel.y));
    n2vl(&(iffattr.end_accel.z));

    n2vl(&(iffattr.start_magnify.x));
    n2vl(&(iffattr.start_magnify.y));
    n2vl(&(iffattr.start_magnify.z));

    n2vl(&(iffattr.end_magnify.x));
    n2vl(&(iffattr.end_magnify.y));
    n2vl(&(iffattr.end_magnify.z));

    n2vl(&(iffattr.start_speed));
    n2vl(&(iffattr.num_contexts));
    n2vl(&(iffattr.ctx_lifetime));
    n2vl(&(iffattr.ctx_startgen));
    n2vl(&(iffattr.ctx_stopgen));
    n2vl(&(iffattr.birthrate));
    n2vl(&(iffattr.lifetime));
    n2vl(&(iffattr.start_size));
    n2vl(&(iffattr.end_size));
    n2vl(&(iffattr.noise));

    _WriteChunkBytes(iff, &iffattr, sizeof(iffattr));
    _PopChunk(iff);

    /*** eingebettetes ADE-Object schreiben ***/
    for (i=0; i<pd->NumADEs; i++) {
        if (pd->ADEArray[i]) {
            if (!_SaveInnerObject(pd->ADEArray[i],iff)) return(FALSE);
        };
    };

    /*** das war's ***/
    if (_PopChunk(iff) != 0) return(FALSE);

    return(TRUE);
}

/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_PTLM_ACCEL, struct ptl_vector_msg *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**      msg->start_vec   -> Definition des Anfangs-Vektors
**      msg->end_vec     -> Definition des End-Vektors
**
**  RESULTS
**      ---
**
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);

    pd->StartAccel = msg->start_vec;
    pd->EndAccel   = msg->end_vec;
    prtl_RefreshVecs(pd);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_PTLM_GETACCEL, struct ptl_vector_msg *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    msg->start_vec = pd->StartAccel;
    msg->end_vec   = pd->EndAccel;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_PTLM_MAGNIFY, struct ptl_vector_msg *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**      msg->start_vec   -> Definition des Anfangs-Vektors
**      msg->end_vec     -> Definition des End-Vektors
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);

    pd->StartMagnify = msg->start_vec;
    pd->EndMagnify   = msg->end_vec;
    prtl_RefreshVecs(pd);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_PTLM_GETMAGNIFY, struct ptl_vector_msg *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      04-Sep-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    msg->start_vec = pd->StartMagnify;
    msg->end_vec   = pd->EndMagnify;
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, prtl_PTLM_ADDADE, Object **msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Oct-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    Object *ade = *msg;

    if (pd->NumADEs < PTL_MAXNUMADES) {
        pd->ADEArray[pd->NumADEs++] = ade;
        prtl_RefreshADEs(pd);
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, prtl_PTLM_REMADE, ULONG *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Oct-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    ULONG num = *msg;
    Object *ret = NULL;

    if (num < pd->NumADEs) {

        ret = pd->ADEArray[num];
        for (num; num<pd->NumADEs; num++) {
            pd->ADEArray[num] = pd->ADEArray[num+1];
        };
        pd->NumADEs--;
        prtl_RefreshADEs(pd);
    };
    return(ret);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_PTLM_ADEUP, ULONG *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Oct-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    ULONG num = *msg;

    if ((num > 0) && (num < pd->NumADEs)) {
        Object *store       = pd->ADEArray[num];
        pd->ADEArray[num]   = pd->ADEArray[num-1];
        pd->ADEArray[num-1] = store;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, prtl_PTLM_ADEDOWN, ULONG *msg)
/*
**  FUNCTION
**      siehe Spezifikation in "ade/particleclass.h"
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Oct-95   floh    created
**      17-Jan-96   floh    revised & updated
*/
{
    struct particle_data *pd = INST_DATA(cl,o);
    ULONG num = *msg;

    if ((num >= 0) && (num < (pd->NumADEs - 1))) {
        Object *store       = pd->ADEArray[num];
        pd->ADEArray[num]   = pd->ADEArray[num+1];
        pd->ADEArray[num+1] = store;
    };
}
/*=-END-OF-FILE-===================================================*/
