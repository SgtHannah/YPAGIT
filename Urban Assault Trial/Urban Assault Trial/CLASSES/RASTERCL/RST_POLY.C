/*
**  $Source: PRG:VFM/Classes/_RasterClass/rst_poly.c,v $
**  $Revision: 38.8 $
**  $Date: 1997/02/12 14:35:56 $
**  $Locker:  $
**  $Author: floh $
**
**  Polygon-Renderer-Routinen für raster.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <stdlib.h>
#include <string.h>

#define NOMATH_FUNCTIONS 1

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/rasterclass.h"

_extern_use_nucleus

/*** für Dynamic Cluster Size ***/
LONG rst_ClusterArray[8] = {
    6,
    5,
    5,
    4,
    4,
    4,
    4,
    4,
};

/*** Fixed-Point Rundungs-Macro ***/
#define round(x,p) (((x)+(1<<(p-1)))>>p)

/*-----------------------------------------------------------------*/
void rst_FillEdgeTable(struct raster_data *rd,
                       ULONG flags,
                       struct rast_ppoint *p0,
                       struct rast_ppoint *p1,
                       struct rast_ppoint *et)
/*
**  FUNCTION
**      Füllt Edge-Table für eine Polygon-Kante. Die Abtast-
**      Richtung ist immer von p0 nach p1, und von oben
**      nach unten. p0->y muß also kleiner sein als p1->y.
**
**      Für jedes Zeile wird initialisiert:
**
**          et[y].x
**          et[y].b
**          et[y].z
**          et[y].u
**          et[y].v
**
**  INPUTS
**      p0      -> Eckpunkt 1
**      p1      -> Eckpunkt 2
**      et      -> Pointer auf Edge-Table, die Elemente
**                 et[p0->y] bis ausschließlich et[p1->y] werden
**                 gefüllt.
**
**  CHANGED
**      06-Jun-96   floh    created
**      10-Jun-96   floh    + Persp-Mapper-Korrekturen
**      27-Oct-96   floh    + Persp-Mapper-Korrekturen (Genauigkeit!)
**      06-Jan-96   floh    + Erweiterungen für Subpixel-Präzision
*/
{
    ULONG x = p0->x;
    ULONG b = p0->b;
    ULONG u = p0->u;
    ULONG v = p0->v;
    ULONG z = p0->z;

    ULONG y0 = round(p0->y,16);
    ULONG y1 = round(p1->y,16);
    LONG  dy = y1-y0;

    if (dy > 0) {

        LONG dx = ((LONG)(p1->x - x))/dy;
        LONG db = ((LONG)(p1->b - b))/dy;
        LONG du = ((LONG)(p1->u - u))/dy;
        LONG dv = ((LONG)(p1->v - v))/dy;
        LONG dz = ((LONG)(p1->z - z))/dy;

        if (flags & RPF_PerspMap) {
            u<<=1; du<<=1;
            v<<=1; dv<<=1;
        };

        for (y0; y0<y1; y0++) {
            et[y0].x=x; x+=dx;
            et[y0].z=z; z+=dz;
            et[y0].u=u; u+=du;
            et[y0].v=v; v+=dv;
            et[y0].b=b; b+=db;
        };
    };
}

/*-----------------------------------------------------------------*/
void rst_TraverseEdgeTables(struct raster_data *rd,
                            struct rast_poly *rp,
                            LONG ymin, LONG ymax)
/*
**  FUNCTION
**      Scannt die linke und rechte Edge-Table im Bereich
**      [ymin -> ymax], baut die Spans und rendert sie.
**
**  INPUTS
**      rd   - LID des raster.class Objects
**      rp   - der Ursprungs-Polygon
**      ymin - Edge-Tables ab hier scannen
**      ymax - Edge-Tables bis hierhin scannen
**
**  CHANGED
**      06-Jun-96   floh    created + debugging
**      12-Feb-97   floh    Pitch korrekt
*/
{
    struct rast_scanline rs;
    struct rast_ppoint *l = rd->left_edge;
    struct rast_ppoint *r = rd->right_edge;
    UBYTE *addr = ((UBYTE *)rd->r->Data) + (rd->r->BytesPerRow * ymin);
    LONG y;

    /*** statische Parameter ausfüllen ***/
    rs.draw_span = rd->drawspan_lut[rp->flags];
    rs.flags     = rp->flags;
    if (rp->flags & (RPF_LinMap|RPF_PerspMap)) rs.map=rp->map[0]->Data;

    /*** für jede Scanline... ***/
    for (y=ymin; y<ymax; y++,addr+=rd->r->BytesPerRow) {

        /*** rechte X-Koordinate inklusiv (deshalb +1) ***/
        rs.dx = (round(r[y].x,16) - round(l[y].x,16)) + 1;
        rs.x0 = round(l[y].x,16);
        if (rs.dx > 0) {
            rs.y  = y;
            rs.z0 = l[y].z;
            rs.dz = ((LONG)(r[y].z - l[y].z))/rs.dx;
            if (rs.flags & (RPF_FlatShade|RPF_GradShade)) {
                rs.b0 = l[y].b;
                rs.db = ((LONG)(r[y].b - l[y].b))/rs.dx;
            };
            if (rs.flags & (RPF_LinMap|RPF_PerspMap)) {
                rs.u0 = l[y].u;
                rs.du = ((LONG)(r[y].u - l[y].u))/rs.dx;
                rs.v0 = l[y].v;
                rs.dv = ((LONG)(r[y].v - l[y].v))/rs.dx;
            };
            rst_seAddSpan(addr,&rs,rd);
        };
    };
}

/*-----------------------------------------------------------------*/
void rst_ConvertPoints(struct raster_data *rd,
                       struct rast_poly *rp,
                       struct rast_ppoint *t)
/*
**  FUNCTION
**      Konvertiert auflösungs-unabhängige FLOAT-Definition
**      des Polygons nach <t>.
**
**  INPUTS
**      rd      - LID des raster.class Objects
**      rp      - komplette rast_poly-Struktur
**      rt      - Zeiger auf Array mit <struct rast_ppoint>'s
**
**  CHANGED
**      06-Jun-96   floh    created
**      09-Jun-96   floh    Z-Koordinate wurde nicht per FLOAT_TO_INT
**                          konvertiert...
**      13-Jun-96   floh    [u,v] werden jetzt anders "entschärft".
**      17-Jun-96   floh    [u,v] Entschärfung wieder mal anders,
**                          nämlich per Range-Check.
**      06-Jan-96   floh    + Änderungen für Subpixel-Präzision
*/
{
    ULONG pnum;
    FLOAT mulx  = rd->foff_x * 65536.0;
    FLOAT muly  = rd->foff_y * 65536.0;
    ULONG checkx = (rd->r->Width<<16)  - 0x8001;
    ULONG checky = (rd->r->Height<<16) - 0x8001;

    for (pnum=0; pnum<(rp->pnum); pnum++) {

        /*** [x,y,z] [16.16] ***/
        t->x = (LONG)(FLOAT_TO_INT((rp->xyz[pnum].x+1.0)*mulx));
        t->y = (LONG)(FLOAT_TO_INT((rp->xyz[pnum].y+1.0)*muly));
        t->z = (LONG)(FLOAT_TO_INT(rp->xyz[pnum].z));

        /*** Rounding-Fehler ausschalten ***/
        if (t->x < 0x8000)      t->x=0x8000;
        else if (t->x > checkx) t->x=checkx;
        if (t->y < 0x8000)      t->y=0x8000;
        else if (t->y > checky) t->y=checky;

        if (rp->flags & (RPF_LinMap|RPF_PerspMap)) {

            ULONG u,v;
            u = FLOAT_TO_INT(rp->uv[pnum].u * 65536.0);
            v = FLOAT_TO_INT(rp->uv[pnum].v * 65536.0);

            /*** Rounding-Fehler ausschalten ***/
            if      (u<0x0080) u=0x0080;
            else if (u>0xff7f) u=0xff7f;
            if      (v<0x0080) v=0x0080;
            else if (v>0xff7f) v=0xff7f;

            t->u = u;
            t->v = v;

        } else { t->u=0; t->v=0; };
        if (rp->flags & (RPF_FlatShade|RPF_GradShade)) {
            // FIXME???
            t->b = (LONG)(FLOAT_TO_INT(rp->b[pnum] * 64768.0)) + 0x180;
        } else { t->b=0; };
        t++;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Poly, struct rast_poly *msg)
/*
**  CHANGED
**      06-Jun-96   floh    created
**      10-Jun-96   floh    + Persp-Mapper exakt auf vorherigen
**                            Zustand (hoffe ich)
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_ppoint p[12];
    LONG xsize,ysize;
    LONG i;
    LONG y0,y1;

    LONG ymin,ymax,xmin,xmax;

    /*** Eckpunkte ok? ***/
    if ((msg->pnum < 3) || (msg->pnum > 12)) return;

    /*** Eckpunkte konvertieren ***/
    rst_ConvertPoints(rd,msg,p);

    /*** Eckpunkte untersuchen ***/
    ymin = ymax = 0;
    xmin = xmax = 0;
    for (i=0; i<(msg->pnum); i++) {
        if (p[i].x < p[xmin].x)      xmin=i;
        else if (p[i].x > p[xmax].x) xmax=i;
        if (p[i].y < p[ymin].y)      ymin=i;
        else if (p[i].y > p[ymax].y) ymax=i;
    };

    /*** Polygon immer noch gültig (>= als 1 Pixel) ? ***/
    if ((xsize = p[xmax].x - p[xmin].x) <= (1<<15)) return;
    if ((ysize = p[ymax].y - p[ymin].y) <= (1<<15)) return;

    /*** Perspektiv-Mapping: [u,v,z] Parameter mangeln ***/
    if (msg->flags & RPF_PerspMap) {
        /*** Linear-Optimierung ***/
        if ((xsize < (48*(1<<16))) && (ysize < (48*(1<<16)))) {
            msg->flags &= ~RPF_PerspMap;
            msg->flags |= RPF_LinMap;
        } else {
            for (i=0; i < (msg->pnum); i++) {
                /*** u=(u/z),v=(v/z) [5.27]; z=(1/z) [2.30]    ***/
                /*** (1 Bit Vorzeichen MUSS erhalten bleiben!) ***/
                p[i].u = (((ULONG)((p[i].u<<16)/p[i].z))<<3);
                p[i].v = (((ULONG)((p[i].v<<16)/p[i].z))<<3);
                p[i].z = (ULONG)((1<<30)/p[i].z);
            };
        };
    };

    /*** Left Edge ***/
    i = ymin;
    do {
        LONG ni = i-1;
        if (ni < 0) ni=msg->pnum-1;
        rst_FillEdgeTable(rd,msg->flags,&(p[i]),&(p[ni]),rd->left_edge);
        i = ni;
    } while (i != ymax);

    /*** Right Edge ***/
    i = ymin;
    do {
        LONG ni = i+1;
        if (ni >= msg->pnum) ni=0;
        rst_FillEdgeTable(rd,msg->flags,&(p[i]),&(p[ni]),rd->right_edge);
        i = ni;
    } while (i != ymax);

    /*** Scanline-Ausgabe ***/
    y0 = round(p[ymin].y,16);
    y1 = round(p[ymax].y,16);
    rst_TraverseEdgeTables(rd,msg,y0,y1);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__asm void rst_GenClusterStack(__a0 struct rast_cluster *cs, __a1 struct rast_scanline *rs)
#else
void rst_GenClusterStack(struct rast_cluster *cs, struct rast_scanline *rs)
#endif
/*
**  FUNCTION
**      Initialisiert "ClusterStack" für einen
**      perspektiv-korrigierten Span.
**
**  INPUTS
**      cs  - Pointer auf Clusterstack
**      rs  - beschreibt Span
**
**      erwartete Formate:
**          rs->z0 = 2.30
**          rs->u0 = 4.28
**          rs->v0 = 4.28
**
**  CHANGED
**      06-Jun-96   floh    übernommen aus diversen Vorgänger-Sourcen
*/
{
    WORD cnt = rs->dx;

    ULONG cluster_size, cluster_shift;
    ULONG ix;

    ULONG u,v;        // resultierendes u und v
    ULONG u0,v0,z0;
    LONG du0,dv0,dz0;

    ix = abs(rs->dz) >> 13;
    if (ix > 7) ix=7;

    cluster_shift = rst_ClusterArray[ix];
    cluster_size  = 1<<cluster_shift;

    u0  = rs->u0;
    v0  = rs->v0;
    z0  = rs->z0;

    du0 = rs->du << cluster_shift;
    dv0 = rs->dv << cluster_shift;
    dz0 = rs->dz << cluster_shift;

    z0  >>= 10;
    dz0 >>= 10;

    u = u0/z0; // u [8.8]
    v = v0/z0; // v [8.8]

    cs->m0 = (u<<16)|v;

    while ((cnt -= cluster_size) >= 0) {

        ULONG nu,nv;    // next [u,v] in [8.8]
        LONG du,dv;

        z0 += dz0;
        u0 += du0;
        v0 += dv0;

        nu = u0/z0;
        du = ((LONG)(nu-u))>>cluster_shift;

        nv = v0/z0;
        dv = ((LONG)(nv-v))>>cluster_shift;

        cs[1].m0  = (nu<<16)|nv;
        cs->dm    = (du<<16)|((UWORD)dv);
        cs->count = cluster_size;
        cs++;

        u = nu;
        v = nv;
    };

    /*** Rest der Scanline abhandeln ***/
    if (cnt < 0) {
        WORD num_tail = cnt + cluster_size;
        if (num_tail > 0) {

            ULONG eu,ev,ez;
            LONG du,dv;

            ez = (rs->z0 + rs->dz * rs->dx)>>10;
            eu = ((ULONG)(rs->u0 + rs->du * rs->dx))/ez;
            ev = ((ULONG)(rs->v0 + rs->dv * rs->dx))/ez;

            du = ((LONG)(eu-u))/num_tail;
            dv = ((LONG)(ev-v))/num_tail;

            cs->dm    = (du<<16)|((UWORD)dv);
            cs->count = num_tail;
            cs++;
        };
    };

    /* Cluster-Stack begrenzen */
    cs->count = 0;
}
