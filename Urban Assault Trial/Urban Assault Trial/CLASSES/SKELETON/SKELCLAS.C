/*
**  $Source: PRG:VFM/Classes/_SkeletonClass/skeletonclass.c,v $
**  $Revision: 38.12 $
**  $Date: 1996/11/10 21:04:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Die skeleton.class ist die Rootclass aller Klassen, die
**  Skeleton-Strukturen für 3D-Objekt-Gerüst-Definitionen
**  bereitstellen. Sie ist Subklasse der resource.class und erbt
**  damit alle Resource-Verwaltungs-Eigenschaften.
**
**  (C) Copyright 1994,1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "skeleton/skeletonclass.h"

/*-------------------------------------------------------------------
**  Prototypen
*/
_dispatcher(Object *, skel_OM_NEW, struct TagItem *attrs);
_dispatcher(void, skel_OM_GET, struct TagItem *attrs);

_dispatcher(struct RsrcNode *, skel_RSM_CREATE, struct TagItem *tlist);
_dispatcher(void, skel_RSM_FREE, struct rsrc_free_msg *msg);

_dispatcher(struct Skeleton *, skel_SKLM_GETSKELETON, void *ignore);
_dispatcher(BOOL, skel_SKLM_CREATE_SENSORPOOL, struct create_sensorpool_msg *msg);
_dispatcher(BOOL, skel_SKLM_CREATE_POLYGONS, struct create_polygons_msg *msg);

_dispatcher(void, skel_SKLM_REFRESHPLANE, struct refreshplane_msg *msg);
_dispatcher(BOOL, skel_SKLM_LOCAL2VWR, struct local2vwr_msg *msg);
_dispatcher(void *, skel_SKLM_EXTRACTPOLY, struct sklt_extract_msg *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG skel_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG skel_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo skel_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeSkeletonClass(ULONG id,...);
__geta4 BOOL FreeSkeletonClass(void);
#else
struct ClassInfo *MakeSkeletonClass(ULONG id,...);
BOOL FreeSkeletonClass(void);
#endif

struct GET_Class skeleton_GET = {
    &MakeSkeletonClass,
    &FreeSkeletonClass,
};       

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&skeleton_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *skeleton_Entry(void)
{
    return(&skeleton_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo skeleton_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:skeleton.class"   /* der Segment-Name */
    },
    skeleton_Entry,                 /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeSkeletonClass(ULONG id,...)
#else
struct ClassInfo *MakeSkeletonClass(ULONG id,...)
#endif
/*
**  CHANGED
**      23-Mar-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Methoden-Array initialisieren */
    memset(skel_Methods,0,sizeof(skel_Methods));

    skel_Methods[OM_NEW]     = (ULONG) skel_OM_NEW;
    skel_Methods[OM_GET]     = (ULONG) skel_OM_GET;

    skel_Methods[RSM_CREATE] = (ULONG) skel_RSM_CREATE;
    skel_Methods[RSM_FREE]   = (ULONG) skel_RSM_FREE;

    skel_Methods[SKLM_GETSKELETON]       = (ULONG) skel_SKLM_GETSKELETON;
    skel_Methods[SKLM_CREATE_SENSORPOOL] = (ULONG) skel_SKLM_CREATE_SENSORPOOL;
    skel_Methods[SKLM_CREATE_POLYGONS]   = (ULONG) skel_SKLM_CREATE_POLYGONS;

    skel_Methods[SKLM_REFRESHPLANE] = (ULONG) skel_SKLM_REFRESHPLANE;
    skel_Methods[SKLM_LOCAL2VWR]    = (ULONG) skel_SKLM_LOCAL2VWR;
    skel_Methods[SKLM_EXTRACTPOLY]  = (ULONG) skel_SKLM_EXTRACTPOLY;

    /* ClassInfo-Struktur ausfüllen */
    skel_clinfo.superclassid = RSRC_CLASSID;
    skel_clinfo.methods      = skel_Methods;
    skel_clinfo.instsize     = sizeof(struct skeleton_data);
    skel_clinfo.flags        = 0;

    /* und das war's... */
    return(&skel_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeSkeletonClass(void)
#else
BOOL FreeSkeletonClass(void)
#endif
/*
**  CHANGED
**      23-Mar-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    return(TRUE);
}

/*******************************************************************/
/***    AB HIER METHODEN DISPATCHER    *****************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, skel_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      23-Mar-94   floh    created
**      28-Mar-94   floh    INST_DATA wurde mit >o< und nicht mit
**                          >newo< berechnet.
**      03-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    Object *newo;
    struct skeleton_data *skld;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    skld = INST_DATA(cl,newo);

    _get(newo, RSA_Handle, &(skld->skeleton));

    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, skel_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      23-Mar-94   floh    created
**      01-Apr-94   floh    geringfügige Korrektur bei NumPoints:
**                          NumPoints ist immer mindestens 9, weil in
**                          der BoundingBox 9 Elemente stehen (inkl.
**                          Ende-Element). Selbst wenn im eigentlichen
**                          3D-Pool nur z.B. 4 Elemente def. sind, wird
**                          9 zurückgegeben.
**      13-Nov-94   floh    (1) Es gibt jetzt keine Bounding-Box mehr,
**                              deshalb findet keine BBox-Korrektur bei
**                              SKLA_NumPoints statt. Stattdessen wird
**                              wird max(NumPoolPoints,NumSensorPoints)
**                              zurückgegeben.
**      03-Jan-95   floh    Nucleus2-Revision
**      04-Jan-95   floh    Oh no, böser Bug, "attrs" wurde modifiziert
**                          an die Superclass weitergegeben.
**      15-Jan-96   floh    revised & updated
*/
{
    struct skeleton_data *skld = INST_DATA(cl,o);
    ULONG *value;
    ULONG tag;
    struct TagItem *store_attrs = attrs;

    /* TagList scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        value = (ULONG *) attrs++->ti_Data;
        switch(tag) {
            /* die System-Tags */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* dann die "echten" Attribute */
            switch(tag) {
                case SKLA_Skeleton: 
                    *value = (ULONG) skld->skeleton; 
                    break;

                case SKLA_NumPoints:
                    if (skld->skeleton) {
                        *value = skld->skeleton->NumPoolPoints;
                    } else *value = 0;
                    break;

                case SKLA_NumSensorPoints:
                    if (skld->skeleton) {
                        *value = skld->skeleton->NumSensorPoints;
                    } else *value = 0;
                    break;

                case SKLA_NumPolygons:
                    if (skld->skeleton) {
                        *value = skld->skeleton->NumAreas;
                    } else *value = 0;
             };
        };
    };

    /* Methode nach oben reichen */
    _supermethoda(cl,o,OM_GET,store_attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(struct Skeleton *, skel_SKLM_GETSKELETON, void *ignore)
/*
**  CHANGED
**      23-Mar-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
**      15-Jan-96   floh    revised & updated
*/
{
    return(((struct skeleton_data *)INST_DATA(cl,o))->skeleton);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, skel_SKLM_REFRESHPLANE, struct refreshplane_msg *msg)
/*
**  FUNCTION
**      Berechnet die Ebenen-Parameter [A,B,C,D] des Polygons
**      <msg->pnum> neu. A,B,C entspricht dem Normalen-Vektor der Fläche.
**
**  CHANGED
**      15-Jan-96   floh    created
**      20-Jan-96   floh    _LogMsg()-Warnings wieder removed,
**                          insbesondere, weil 2-Punkt-Flächen
**                          jetzt wieder notwendig sind.
*/
{
    struct skeleton_data *skld = INST_DATA(cl,o);
    struct Skeleton *sklt = skld->skeleton;
    ULONG pnum = msg->pnum;

    if (sklt) {

        /*** hat Fläche mindestens 3 Punkte? ***/
        if (sklt->Areas[pnum][0] < 3) {
            sklt->PlanePool[pnum].A = 0.0;
            sklt->PlanePool[pnum].B = 0.0;
            sklt->PlanePool[pnum].C = 0.0;
            sklt->PlanePool[pnum].D = 0.0;
            return;
        } else {

            WORD p0,p1,p2;
            FLOAT ax,ay,az;
            FLOAT bx,by,bz;
            FLOAT A,B,C,D;
            FLOAT len;
            fp3d *pool = sklt->Pool;

            p0 = sklt->Areas[pnum][1];
            p1 = sklt->Areas[pnum][2];
            p2 = sklt->Areas[pnum][3];

            ax = pool[p1].x - pool[p0].x;
            ay = pool[p1].y - pool[p0].y;
            az = pool[p1].z - pool[p0].z;

            bx = pool[p2].x - pool[p1].x;
            by = pool[p2].y - pool[p1].y;
            bz = pool[p2].z - pool[p1].z;

            A = ay*bz - az*by;
            B = az*bx - ax*bz;
            C = ax*by - ay*bx;

            len = sqrt(A*A + B*B + C*C);

            if (len == 0.0) {
                /*** Fläche ist nicht plan ***/
                A = 0.0;
                B = 0.0;
                C = 0.0;
            } else {
                /*** A,B,C normalisieren ***/
                A /= len;
                B /= len;
                C /= len;
            };

            /*** D = 0 - (Ax + By + Cz) ***/
            D = -(A*pool[p0].x + B*pool[p0].y + C*pool[p0].z);
            sklt->PlanePool[pnum].A = A;
            sklt->PlanePool[pnum].B = B;
            sklt->PlanePool[pnum].C = C;
            sklt->PlanePool[pnum].D = D;
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL skel_Alloc3DPools(struct Skeleton *sklt, ULONG num_pnts)
/*
**  FUNCTION
**      Initialisiert:
**          sklt->Pool
**          sklt->NumPoolPoints
**          sklt->TransformPool
**
**  INPUTS
**      num_pnts    - Anzahl 3D-Punkte
**
**  RESULTS
**      TRUE    alles OK
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    sklt->Pool = (fp3d *) _AllocVec(num_pnts*sizeof(fp3d),
                          MEMF_PUBLIC|MEMF_CLEAR);
    if (sklt->Pool) {

        sklt->TransformPool = (fp3d *)_AllocVec(num_pnts*sizeof(fp3d),
                                      MEMF_PUBLIC|MEMF_CLEAR);
        if (sklt->TransformPool) {

            sklt->NumPoolPoints = num_pnts;
            return(TRUE);
        };
    };

    /*** Fehler ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, skel_SKLM_CREATE_SENSORPOOL, struct create_sensorpool_msg *msg)
/*
**  FUNCTION
**      *** INTERNAL *** INTERNAL *** INTERNAL ***
**
**      Initialisiert:
**          sklt->SensorPool
**          sklt->NumSensorPoints
**
**  INPUTS
**      num_sens    - Anzahl Sensor-Punkte
**
**  RESULTS
**      TRUE    alles OK
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct Skeleton *sklt = msg->sklt;
    LONG num_sens         = msg->num_sensorpnts;

    sklt->SensorPool = (fp3d *) _AllocVec(num_sens*sizeof(fp3d),
                                          MEMF_PUBLIC|MEMF_CLEAR);
    if (sklt->SensorPool) {
        sklt->NumSensorPoints = num_sens;
        return(TRUE);
    };

    /*** Fehler ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, skel_SKLM_CREATE_POLYGONS, struct create_polygons_msg *msg)
/*
**  FUNCTION
**      *** INTERNAL *** INTERNAL *** INTERNAL ***
**
**      Initialisiert
**          sklt->Areas
**          sklt->NumAreas
**          sklt->PlanePool
**
**      Der 1.Pointer im sklt->Areas Block zeigt auf den
**      Anfang des PolygonPools, die weiteren Pointer
**      müssen selbst ausgefüllt werden!
**
**  INPUTS
**      msg->sklt         - Ptr auf Skeleton
**      msg->num_polys    - Anzahl Polygone
**      msg->num_polypnts - Anzahl aller Punkte aller Polygone
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct Skeleton *sklt = msg->sklt;
    LONG num_polys        = msg->num_polys;
    LONG num_polypnts     = msg->num_polypnts;

    /*** Polypool-Size einschließlich NumPoints je Poly ***/
    LONG size = (num_polypnts + num_polys) * sizeof(WORD);

    /*** PolyPool-Size + Pointer auf Polygone im selben MemBlock ***/
    size += num_polys * sizeof(WORD *);

    sklt->Areas = (WORD **) _AllocVec(size, MEMF_PUBLIC|MEMF_CLEAR);
    if (sklt->Areas) {

        /*** 1.Poly-Pointer ausfüllen ***/
        sklt->Areas[0] = (WORD *) (sklt->Areas + num_polys);

        /*** Anzahl Polys eintragen ***/
        sklt->NumAreas = num_polys;

        /*** Plane-Pool ***/
        sklt->PlanePool = (struct Plane *)
            _AllocVec(num_polys * sizeof(struct Plane), MEMF_PUBLIC|MEMF_CLEAR);
        if (!(sklt->PlanePool)) return(FALSE);

        return(TRUE);
    };

    /*** Fehler ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, skel_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**      Falls zumindest SKLA_NumPoints angegeben ist, wird eine
**      Skeleton-Struktur allokiert, sowie sklt->Pool und
**      sklt->TransformPool reserviert (aber verständlicherweise
**      nicht ausgefüllt).
**
**  INPUTS
**      SKLA_NumPoints
**      SKLA_NumSensorPoints    [optional]
**      SKLA_NumPolygons      \ [optional, dann aber beide
**      SKLA_NumPolyPoints    /  zusammen]
**
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct RsrcNode *rnode;

    /*** Resource-Node erzeugen lassen ***/
    rnode = (struct RsrcNode *) _supermethoda(cl,o,RSM_CREATE,tlist);
    if (rnode) {

        /*** genug Information da, um etwas zu allokieren? ***/
        ULONG num_points;

        num_points = _GetTagData(SKLA_NumPoints, 0, tlist);
        if (num_points > 0) {

            /*** Skeleton allokieren ***/
            struct Skeleton *sklt;

            sklt = (struct Skeleton *)
                   _AllocVec(sizeof(struct Skeleton), MEMF_PUBLIC|MEMF_CLEAR);
            if (sklt) {

                LONG num_sens;
                LONG num_polys;

                /*** damit RSM_FREE auch was sieht! ***/
                rnode->Handle = sklt;

                /*** Point- und Transform-Pool ***/
                if (!skel_Alloc3DPools(sklt,num_points)) goto error;

                /*** optional: Sensor-Pool ***/
                num_sens = _GetTagData(SKLA_NumSensorPoints,0,tlist);
                if (num_sens > 0) {
                    if (!_method(o,SKLM_CREATE_SENSORPOOL,
                                 (ULONG)sklt,
                                 num_sens))
                    {
                        goto error;
                    };
                };

                /*** optional: Polygon-Stuff ***/
                num_polys = _GetTagData(SKLA_NumPolygons,0,tlist);
                if (num_polys > 0) {

                    LONG num_polypnts;

                    num_polypnts = _GetTagData(SKLA_NumPolyPoints,0,tlist);
                    if (num_polypnts <= 0) goto error;

                    if (!_method(o,SKLM_CREATE_POLYGONS,
                                 (ULONG)sklt, 
                                 num_polys, 
                                 num_polypnts))
                    {
                        goto error;
                    };
                };

                /*** wenn hier angekommen, alles OK ***/
                return(rnode);
            };
        };
    };

/*** wenn hier angekommen, ging was schief ***/
error:
    if (rnode) _method(o, RSM_FREE, (ULONG) rnode);
    return(NULL);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, skel_RSM_FREE, struct rsrc_free_msg *msg)
/*
**  CHANGED
**      15-Jan-96   floh    created
*/
{
    struct RsrcNode *rnode = msg->rnode;
    struct Skeleton *sklt;

    /*** existiert ein gültiges Handle? ***/
    if (sklt = rnode->Handle) {

        if (sklt->Pool)          _FreeVec(sklt->Pool);
        if (sklt->Areas)         _FreeVec(sklt->Areas);
        if (sklt->SensorPool)    _FreeVec(sklt->SensorPool);
        if (sklt->PlanePool)     _FreeVec(sklt->PlanePool);
        if (sklt->TransformPool) _FreeVec(sklt->TransformPool);
        _FreeVec(sklt);

        rnode->Handle = NULL;
    };

    /*** die rsrc.class erledigt den Rest ***/
    _supermethoda(cl,o,RSM_FREE,msg);
}

