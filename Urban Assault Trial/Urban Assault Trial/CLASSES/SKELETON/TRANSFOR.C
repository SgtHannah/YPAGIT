/*
**  $Source: PRG:VFM/Classes/_SkeletonClass/transform.c,v $
**  $Revision: 38.5 $
**  $Date: 1997/06/01 12:21:53 $
**  $Locker:  $
**  $Author: floh $
**
**  3D-Transformations-Routinen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "types.h"
#include "modules.h"
#include "skeleton/skeletonclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

extern void skel_Clip3DPolygon(struct sklt_clip *);

/*-----------------------------------------------------------------*/
BOOL skel_Local2Viewer(struct local2vwr_msg *msg,
                       fp3d *from, fp3d *to,
                       ULONG num_pnts)
/*
**  FUNCTION
**      Transformiert Objekt-Koordinaten (in <from>)
**      in den Viewer-Space, die Viewer-Koordinaten
**      werden in <to> abgelegt. Auf die Viewer-Koordinaten
**      wird ein ClipCode-Check gemacht.
**
**  INPUTS
**      msg      - Hieraus wird die lokale TForm des Objects,
**                 die TForm des Viewers, sowie die Entfernung
**                 der Frontclip-Plane und der Backclip-Plane
**                 gewonnen.
**      from     - Source-Puffer mit Objekt-Koordinaten
**      to       - Target-Puffer mit Viewer-Koordinaten und ClipCodes
**      num_pnts - Anzahl Koordinaten
**
**  RESULTS
**      TRUE    Pool ist (zumindest teilweise) sichtbar
**      FALSE   Pool voll unsichtbar
**
**  CHANGED
**      16-Jan-96   floh    gemerged aus _LocalToGlobal(), 
**                          _GlobalToViewer() und _Set3DClipCodes()
**                          oder so.
*/
{
    ULONG and_code = 0xffffffff;
    ULONG i;
    tform *l = msg->local;
    tform *v = msg->viewer;

    for (i=0; i<num_pnts; i++) {

        FLOAT gx,gy,gz;
        FLOAT vx,vy,vz;
        ULONG cc;

        /*** aktueller Objekt-Point ***/
        FLOAT lx = from[i].x;
        FLOAT ly = from[i].y;
        FLOAT lz = from[i].z;

        /*** ObjectSpace -> GlobalSpace ***/
        if (l->flags & TFF_GlobalizeNoRot) {

            /*** nur T ***/
            gx = lx + l->glb.x;
            gy = ly + l->glb.y;
            gz = lz + l->glb.z;

        } else {

            /*** R und T ***/
            gx = (l->glb_m.m11*lx + l->glb_m.m12*ly + l->glb_m.m13*lz) + l->glb.x;
            gy = (l->glb_m.m21*lx + l->glb_m.m22*ly + l->glb_m.m23*lz) + l->glb.y;
            gz = (l->glb_m.m31*lx + l->glb_m.m32*ly + l->glb_m.m33*lz) + l->glb.z;

        };

        /*** Translation in Viewer-Space ***/
        gx -= v->glb.x;
        gy -= v->glb.y;
        gz -= v->glb.z;

        /*** Rotation in Viewer-Space ***/
        vx = v->glb_m.m11*gx + v->glb_m.m12*gy + v->glb_m.m13*gz;
        vy = v->glb_m.m21*gx + v->glb_m.m22*gy + v->glb_m.m23*gz;
        vz = v->glb_m.m31*gx + v->glb_m.m32*gy + v->glb_m.m33*gz;

        /*** ClipCodes für aktuellen Punkt ermitteln ***/
        cc = 0;
        if      (vz < msg->min_z) cc |= CLIP3D_BEHIND;
        else if (vz > msg->max_z) cc |= CLIP3D_TOOFAR;
        if (vx > vz)  cc |= CLIP3D_RIGHTOUT;
        if (vx < -vz) cc |= CLIP3D_LEFTOUT;
        if (vy > vz)  cc |= CLIP3D_BOTOUT;
        if (vy < -vz) cc |= CLIP3D_TOPOUT;

        and_code &= cc;

        /*** Letztendes noch Target ausfüllen ***/
        to[i].flags = cc;
        to[i].x     = vx;
        to[i].y     = vy;
        to[i].z     = vz;
    };

    /*** Ende ***/
    if (and_code == 0) return(TRUE);
    else               return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG skel_ExtractPoly(fp3d *pool, fp3d *poly, WORD *pvec)
/*
**  FUNCTION
**
**  INPUTS
**
**  CHANGED
**      10-Jun-96   floh    created
*/
{
    LONG or_clip  = 0;
    LONG and_clip = 0xffff;
    ULONG nump;
    ULONG i;

    /* isoliere Polygon (und setze Ende-Flag) */
    nump = *pvec++;
    for (i=0; i<nump; i++) {
        poly[i]   = pool[*pvec++];
        or_clip  |= poly[i].flags;
        and_clip &= poly[i].flags;
    };
    poly[i].flags = P3DF_DONE;

    /* Flächen-Rückenuntersuchung */
    if ((nump > 2) && (and_clip == 0)) {

        /* Flächen-Normale im Viewer-System... */
        FLOAT ax = poly[1].x - poly[0].x;
        FLOAT ay = poly[1].y - poly[0].y;
        FLOAT az = poly[1].z - poly[0].z;
        FLOAT bx = poly[2].x - poly[1].x;
        FLOAT by = poly[2].y - poly[1].y;
        FLOAT bz = poly[2].z - poly[1].z;

        FLOAT nx = ay*bz - az*by;
        FLOAT ny = az*bx - ax*bz;
        FLOAT nz = ax*by - ay*bx;

        /* Flächen-Normale relativ zu Beobachter */
        FLOAT vis_code = nx*poly[0].x + ny*poly[0].y + nz*poly[0].z;
        if (vis_code < 0.0) and_clip |= (1<<15);
    };

    return((ULONG)((and_clip<<16)|(or_clip)));
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, skel_SKLM_LOCAL2VWR, struct local2vwr_msg *msg)
/*
**  FUNCTION
**      Transformiert die Objekt-Koordinaten (aus 'Pool') in 
**      den Viewer-Space und vollzieht gleichzeitig eine
**      Sichtbarkeits-Untersuchung.
**
**      Die Viewer-Koordinaten werden im TransformPool
**      abgelegt, jede versehen mit ihrem 3D-ClipCode.
**
**      Returniert wird das "AND-Resultat" aller ClipCodes.
**      Ist dieses != 0, ist der gesamte Punkt-Pool außerhalb
**      des View-Volumens.
**
**      Falls ein Skeleton-Pool existiert, und dieser
**      mindestens 4x weniger Koordinaten besitzt als der
**      "echte" Pool, wird zuerst eine Sichtbarkeits-
**      Untersuchung auf des Skeleton-Pool gemacht.
**      Verläuft dieser positiv, wird dessen Ergebnis
**      verworfen und die "echte" Transformation vollzogen.
**
**  INPUTS
**      msg->local  - zeigt auf TForm des Objekts
**      msg->viewer - zeigt auf TForm des Viewers
**      msg->min_z  - definiert die Front-Clipping-Plane
**      msg->max_z  - definiert die Back-Clipping-Plane
**
**      Aus den TForms werden folgende Informationen
**      benötigt:
**
**      tform.glb   - globale Position
**      tform.glb_m - globale 3x3 Rotations-Matrix
**
**  RESULTS
**      TRUE    - wenn voll oder teilweise sichtbar
**      FALSE   - wenn voll unsichtbar
**
**  CHANGED
**      16-Jan-96   floh    created
**      18-Jan-96   floh    debugging: Entscheidung, ob Sensor-Pool
**                          getestet werden soll, war Rubbish!
*/
{
    struct skeleton_data *skld = INST_DATA(cl,o);
    struct Skeleton *sklt = skld->skeleton;
    BOOL vis = TRUE;

    /*** Sensor-Pool voruntersuchen? ***/
    if ((sklt->NumSensorPoints > 0) &&
        (sklt->NumSensorPoints < (sklt->NumPoolPoints>>2))) 
    {
        vis = skel_Local2Viewer(msg, 
                                sklt->SensorPool,
                                sklt->TransformPool,
                                sklt->NumSensorPoints);
    };

    /*** die "echte" Transformation ***/
    if (vis) {
        vis = skel_Local2Viewer(msg, 
                                sklt->Pool,
                                sklt->TransformPool,
                                sklt->NumPoolPoints);
    };

    /*** und zurück ***/
    return(vis);
}

/*-----------------------------------------------------------------*/
_dispatcher(void *, skel_SKLM_EXTRACTPOLY, struct sklt_extract_msg *msg)
/*
**  CHANGED
**      10-Jun-96   floh    created
**      11-Jun-96   floh    debugging...
*/
{
    struct skeleton_data *sd = INST_DATA(cl,o);
    struct Skeleton *sklt = sd->skeleton;
    fp3d raw_poly[12];
    fp3d clp_poly[12];
    struct VFMOutline clp_uv[12];
    ULONG cc;

    cc = skel_ExtractPoly(sklt->TransformPool, raw_poly, sklt->Areas[msg->pnum]);

    /*** Polygon sichtbar? ***/
    if (((UWORD)(cc>>16))==0) {

        fp3d *p;
        struct VFMOutline *uv;
        ULONG pnum;
        void *outp;

        /*** nicht interessierende Codes ausmaskieren ***/
        cc &= (CLIP3D_LEFTOUT  |
               CLIP3D_RIGHTOUT |
               CLIP3D_TOPOUT   |
               CLIP3D_BOTOUT   |
               CLIP3D_BEHIND   |
               CLIP3D_TOOFAR);

        /*** muß geclippt werden? ***/
        if (cc != 0) {

            struct sklt_clip clip;

            clip.s_poly = raw_poly;
            clip.t_poly = clp_poly;
            clip.s_uv   = msg->uv;
            clip.t_uv   = clp_uv;
            clip.code   = cc;
            clip.min_z  = msg->min_z;
            clip.max_z  = msg->max_z;
            skel_Clip3DPolygon(&clip);
            p  = clp_poly;
            uv = clp_uv;

        } else {
            p  = raw_poly;
            uv = msg->uv;
        };

        /*** 3D->2D Transformation ***/
        pnum = 0;
        while (p[pnum].flags >= 0) {
            msg->poly->xyz[pnum].x = p[pnum].x/p[pnum].z;
            msg->poly->xyz[pnum].y = p[pnum].y/p[pnum].z;
            msg->poly->xyz[pnum].z = p[pnum].z;
            pnum++;
        };
        msg->poly->pnum = pnum;
        outp = &(msg->poly->xyz[pnum]);

        /*** [u,v] Channel füllen ***/
        if (msg->flags & SKLF_UV) {
            msg->poly->uv = outp;
            for (pnum=0; pnum<(msg->poly->pnum); pnum++) {
                msg->poly->uv[pnum].u = uv[pnum].x;
                msg->poly->uv[pnum].v = uv[pnum].y;
            };
            outp = &(msg->poly->uv[pnum]);
        };

        /*** [b] Channel füllen ***/
        if (msg->flags & SKLF_BRIGHT) {
            msg->poly->b = outp;
            if (msg->flags & SKLF_DFADE) {
                for (pnum=0; pnum<(msg->poly->pnum); pnum++) {
                    FLOAT d;
                    FLOAT b;
                    d = nc_sqrt(p[pnum].x*p[pnum].x +
                                p[pnum].y*p[pnum].y +
                                p[pnum].z*p[pnum].z);
                    b = (d-msg->dfade_start)/msg->dfade_len;
                    if (b < 0.0) b=0.0;
                    b += msg->shade;
                    if (b > 1.0) b=1.0;
                    msg->poly->b[pnum] = b;
                };
            } else {
                for (pnum=0; pnum<(msg->poly->pnum); pnum++) {
                    msg->poly->b[pnum] = msg->shade;
                };
            };
            outp = &(msg->poly->b[pnum]);
        };

        /*** Polygon sichtbar ***/
        return(outp);
    };

    /*** Polygon unsichtbar ***/
    return(NULL);
}

