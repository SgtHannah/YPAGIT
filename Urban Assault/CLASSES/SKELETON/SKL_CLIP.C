/*
**  $Source: PRG:VFM/Classes/_SkeletonClass/skl_clip.c,v $
**  $Revision: 38.4 $
**  $Date: 1997/09/01 20:37:03 $
**  $Locker:  $
**  $Author: floh $
**
**  5 Stage Polygon Clipper
**
**  (C) Copyright 1994 by A.Weissflog
*/
/* Amiga-[Emul]-Includes */
#include <exec/types.h>
#include <math.h>

/* VFM Includes */
#include "nucleus/nucleus2.h"
#include "skeleton/skeletonclass.h"

/*-------------------------------------------------------------------
**  Globale Variablen
*/
BOOL skel_ClipUV = FALSE;

/*-------------------------------------------------------------------
**  Zwischenpuffer für's Clippen...
*/
#ifdef AMIGA
__far fp3d clipbuf_poly[6][32];
__far struct VFMOutline clipbuf_uv[6][32];
#else
fp3d clipbuf_poly[6][32];
struct VFMOutline clipbuf_uv[6][32];
#endif

/*-----------------------------------------------------------------*/
void skel_SetClipCode(fp3d *p, FLOAT min_z, FLOAT max_z)
/*
**  FUNCTION
**      "Lokale" Version zur Bestimmung der 3D-Clipcodes.
**
**  INPUTS
**      point -> Pointer auf fp3d-Punkt, für den die Clipcodes
**               bestimmt werden sollen.
**
**  RESULTS
**      point->flags = ClipCodes:
**          CLIP3D_LEFTOUT:     x < -z
**          CLIP3D_RIGHTOUT:    x >  z
**          CLIP3D_TOPOUT:      y < -z
**          CLIP3D_BOTOUT:      y >  z
**          CLIP3D_BEHIND:      z < min_z
**
**  CHANGED
**      10-Jun-96   floh    übernommen nach skeleton.class
**      01-Jun-97   floh    hintere Clip-Plane
*/
{
    ULONG flags = 0;
    FLOAT pos_z = p->z;
    FLOAT neg_z = -p->z;
    if (p->z < min_z)      flags |= CLIP3D_BEHIND;
    else if (p->z > max_z) flags |= CLIP3D_TOOFAR;
    if (p->x > pos_z)      flags |= CLIP3D_RIGHTOUT;
    if (p->x < neg_z)      flags |= CLIP3D_LEFTOUT;
    if (p->y > pos_z)      flags |= CLIP3D_BOTOUT;
    if (p->y < neg_z)      flags |= CLIP3D_TOPOUT;
    p->flags = flags;
}

/*-----------------------------------------------------------------*/
void skel_ClipPolyLine(struct sklt_clip *clip,
                       fp3d *s_poly_off,
                       fp3d *s_poly_in,
                       struct VFMOutline *s_uv_off,
                       struct VFMOutline *s_uv_in,
                       fp3d *t_poly,
                       struct VFMOutline *t_uv)
/*
**  FUNCTION
**      Clippt die mit <s_poly_off> und <source_poly_in>
**      definierte 3D-Linie gegen die eine beliebige Viewcone-Begrenzung.
**
**  INPUTS
**      clip->code  -> definiert die Seite, gegen die geclippt wird
**      clip->min_z -> MinZ eben...
**      s_poly_off  -> Punkt der 3D-Linie, der im "Off" liegt
**                     (also links außen).
**      s_poly_in   -> Punkt der 3D-Linie, der im "In" liegt
**      s_uv_off    -> Off-Punkt [u,v]
**      s_uv_in     -> In-Punkt [u,v]
**      t_poly      -> Pointer auf 3D-Ziel-Polygon (hierhin wird
**                     geclippt)
**      t_uv        -> Pointer auf Map-Ziel-Outline
**
**  CHANGED
**      10-Jun-96   floh    übernommen nach skeleton.class
**      01-Jun-97   floh    clippt jetzt auch hinten raus
*/
{
    #define x0 (s_poly_off->x)
    #define y0 (s_poly_off->y)
    #define z0 (s_poly_off->z)
    #define x1 (s_poly_in->x)
    #define y1 (s_poly_in->y)
    #define z1 (s_poly_in->z)

    FLOAT dx = x1-x0;
    FLOAT dy = y1-y0;
    FLOAT dz = z1-z0;

    /* ermittle Linien-Parameter t */
    FLOAT t;
    switch (clip->code) {
        case CLIP3D_LEFTOUT:
            t = (x0+z0)/(-dx-dz);
            t_poly->x = x0 + t*dx;
            t_poly->y = y0 + t*dy;
            t_poly->z = -t_poly->x;
            break;

        case CLIP3D_RIGHTOUT:
            t = (x0-z0)/(-dx+dz);
            t_poly->x = x0 + t*dx;
            t_poly->y = y0 + t*dy;
            t_poly->z = t_poly->x;
            break;

        case CLIP3D_TOPOUT:
            t = (y0+z0)/(-dy-dz);
            t_poly->x = x0 + t*dx;
            t_poly->y = y0 + t*dy;
            t_poly->z = -t_poly->y;
            break;

        case CLIP3D_BOTOUT:
            t = (y0-z0)/(-dy+dz);
            t_poly->x = x0 + t*dx;
            t_poly->y = y0 + t*dy;
            t_poly->z = t_poly->y;
            break;

        case CLIP3D_BEHIND:
            t = (clip->min_z-z0)/dz;
            t_poly->x = x0 + t*dx;
            t_poly->y = y0 + t*dy;
            t_poly->z = clip->min_z;
            break;

        case CLIP3D_TOOFAR:
            t = (clip->max_z-z0)/dz;
            t_poly->x = x0 + t*dx;
            t_poly->y = y0 + t*dy;
            t_poly->z = clip->max_z;
            break;
    };

    /* ClipCode im Target-Pixel3D setzen */
    skel_SetClipCode(t_poly,clip->min_z,clip->max_z);

    /* UV-Clipping ? */
    if (skel_ClipUV) {
        FLOAT ex_x0 = s_uv_off->x;
        FLOAT ex_y0 = s_uv_off->y;
        FLOAT ex_x1 = s_uv_in->x;
        FLOAT ex_y1 = s_uv_in->y;
        t_uv->x = ex_x0 + t*(ex_x1 - ex_x0);
        t_uv->y = ex_y0 + t*(ex_y1 - ex_y0);
    };

    /* fertig */
}

/*-----------------------------------------------------------------*/
void skel_InternalClipPolygon(struct sklt_clip *clip)
/*
**  FUNCTION
**      Universeller Polygon-Clipper für alle Viewcone-
**      Begrenzungen.
**      Clippt <s_poly> nach <t_poly>, sowie, wenn vorhanden,
**      <s_uv> nach <t_uv>. <code> definiert, welche
**      Seite geclippt wird (CLIP3D_#?). <min_z> und <max_z>
**      definieren die Front-/Backplanes.
**
**  INPUTS
**      clip    - hält Clip-Parameter.
**
**  RESULTS
**      Target_Poly hält den geclippten 3D-Polygon, mit korrekten
**      Flag-Feldern.
**
**  CHANGED
**      10-Jun-96   floh    übernommen nach skeleton.class
*/
{
    fp3d *act_poly             = clip->s_poly;
    fp3d *next_poly            = clip->s_poly+1;
    fp3d *t_poly               = clip->t_poly;
    struct VFMOutline *act_uv  = clip->s_uv;
    struct VFMOutline *next_uv = clip->s_uv+1;
    struct VFMOutline *t_uv    = clip->t_uv;

    LONG act_flags;
    LONG next_flags;

    /* für jedes Element des Source-Polygons... */
    while ((act_flags = act_poly->flags) >= 0) {

        /* ist nächster Punkt letzter Punkt? */
        if ((next_flags = next_poly->flags) < 0) {
            /* dann Polygon auf Ausgangspunkt zurückführen */
            next_poly  = clip->s_poly;
            next_flags = next_poly->flags;
            next_uv    = clip->s_uv;
        };

        if ((next_flags & clip->code) == 0) {
            /* ist Endpunkt innerhalb */

            if ((act_flags & clip->code) == 0) {
                /* Startpunkt innerhalb, Endpunkt innerhalb */
                *t_poly++ = *next_poly;
                if (skel_ClipUV) {
                    *t_uv++ = *next_uv;
                };
            } else {
                /* Startpunkt draußen, Endpunkt innerhalb */
                skel_ClipPolyLine(clip,
                                  act_poly, next_poly,
                                  act_uv,   next_uv,
                                  t_poly++, t_uv++);
                /* Endpunkt noch nach Target-Poly... */
                *t_poly++ = *next_poly;
                if (skel_ClipUV) {
                    *t_uv++ = *next_uv;
                };
            };
        } else {
            /* Endpunkt außerhalb */

            if ((act_flags & clip->code) == 0) {
                /* Startpunkt innerhalb, Endpunkt außerhalb */
                skel_ClipPolyLine(clip,
                                  next_poly, act_poly,
                                  next_uv,   act_uv,
                                  t_poly++,  t_uv++);
            };
        };

        /* update Pointer */
        act_poly++;
        next_poly++;
        act_uv++;
        next_uv++;

        /* ...und weiter geht's... */
    };

    /* geclippten Polygon begrenzen */
    t_poly->flags = P3DF_DONE;

    /* Ende InternalClipPolygon() */
}

/*-----------------------------------------------------------------*/
void skel_Clip3DPolygon(struct sklt_clip *clip)
/*
**  FUNCTION
**      5-Stage-Polygon-Clipper.
**
**  INPUTS
**      clip->s_poly        - Source-Poly
**      clip->t_poly        - Polygon hierhin clippen
**      clip->s_uv          - optional: Source-Outline
**      clip->t_uv          - optional: Outline hierhin clippen
**      clip->code          - or_clip_code (!=0) !!!
**      clip->min_z         - Z-Koordinate der Front-Plane
**      clip->max_z         - Z-Koordinate der Back-Plane (wird aber
**                            nicht geclippt!)
**
**  FUNCTION
**      10-Jun-96   floh    übernommen in skeleton.class + Änderungen
**      01-Jun-97   floh    clippt jetzt auch gegen Backplane
*/
{
    struct sklt_clip temp;
    ULONG or_code = clip->code;
    ULONG i;

    if (clip->s_uv) skel_ClipUV = TRUE;
    else            skel_ClipUV = FALSE;

    /*** von CLIP3D_BEHIND bis CLIP3D_LEFTOUT ***/
    temp = *clip;
    for (i=5; i>=0; i--) {

        ULONG act_code = (1<<i);

        /*** aktuelle Seite clippen? ***/
        if (or_code & act_code) {

            /*** wird das der letzte Clip? ***/
            or_code &= ~act_code;
            if (or_code) {
                /* nein -> nach Zwischenpuffer clippen */
                temp.t_poly = &(clipbuf_poly[i][0]);
                temp.t_uv   = &(clipbuf_uv[i][0]);
            } else {
                /* ja -> nach finalem Puffer clippen */
                temp.t_poly = clip->t_poly;
                temp.t_uv   = clip->t_uv;
            };

            /*** clippen... ***/
            temp.code = act_code;
            skel_InternalClipPolygon(&temp);

            /*** Source-Pointer für nächsten Clip ***/
            if (or_code) {
                temp.s_poly = temp.t_poly;
                temp.s_uv   = temp.t_uv;
            } else break;
        };
    };

    /* Ende */
}

