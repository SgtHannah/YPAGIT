/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_poly.c,v $
**  $Revision: 38.2 $
**  $Date: 1997/02/26 17:21:52 $
**  $Locker: floh $
**  $Author: floh $
**
**  Polygon-Render-Modul für cgl.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap/cglclass.h"

#define NOMATH_FUNCTIONS
#include "nucleus/math.h"

extern ULONG cgl_ValidateTexture(struct cgl_data *,struct VFMBitmap *,BOOL);

/*-----------------------------------------------------------------*/
ULONG cgl_InitPolyEngine(struct cgl_data *cd)
/*
**  FUNCTION
**      Initialisiert die Listen für das Delayed-Polygon-
**      Handling.
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    ULONG i;

    /*** initialisiere Polygon-Listen ***/
    _NewList((struct List *)&(cd->free_list));
    _NewList((struct List *)&(cd->delayed_list));
    _NewList((struct List *)&(cd->tracy_list));
    memset(cd->dp_buf,0,sizeof(cd->dp_buf));
    for (i=0; i<CGL_MAXNUM_DELAYED; i++) {
        _AddTail((struct List *)&(cd->free_list),
                 (struct Node *)&(cd->dp_buf[i]));
    };

    /*** Initialisiere Stream-Puffer ***/
    cd->poly_buf = (struct cgl_Polygon *)
                   _AllocVec(CGL_POLYBUF_MAX*sizeof(struct cgl_Polygon),
                   MEMF_CLEAR|MEMF_PUBLIC);
    cd->vertex_buf = (CGL_VERTEX3D_ST *)
                   _AllocVec(CGL_VERTEXBUF_MAX*sizeof(CGL_VERTEX3D_ST),
                   MEMF_CLEAR|MEMF_PUBLIC);
    cd->uv_buf = (CGL_TEXTURE2D_ST *)
                   _AllocVec(CGL_VERTEXBUF_MAX*sizeof(CGL_TEXTURE2D_ST),
                   MEMF_CLEAR|MEMF_PUBLIC);
    cd->uvz_buf = (CGL_TEXTURE3D_ST *)
                   _AllocVec(CGL_VERTEXBUF_MAX*sizeof(CGL_TEXTURE3D_ST),
                   MEMF_CLEAR|MEMF_PUBLIC);
    cd->color_buf = (CGL_COLOR_ST *)
                   _AllocVec(CGL_VERTEXBUF_MAX*sizeof(CGL_COLOR_ST),
                   MEMF_CLEAR|MEMF_PUBLIC);
    if (!(cd->poly_buf&&cd->vertex_buf&&cd->uv_buf&&cd->uvz_buf&&cd->color_buf))
        return(FALSE);

    /*** Initialisierung der Puffer-Elemente ***/
    cd->poly_index = 0;
    cd->vx_index   = 0;
    for (i=0; i<CGL_POLYBUF_MAX; i++) {

        struct cgl_Polygon *p = &(cd->poly_buf[i]);

        cd->stream[i] = p;

        p->rend.wOperation   = CGL_RENDER;
        p->rend.pDepth       = &(p->depth);
        p->rend.pBlend       = &(p->blend);
        p->rend.pTextureEnv  = &(p->text);

        p->depth.writeEnable = CGL_ENABLE;

        p->text.wBufferWidth = 256;
        p->text.wWidth       = 256;
        p->text.wHeight      = 256;
        p->text.wColorOrder  = CGL_RGB_MODE;
        p->text.wColorFormat = CGL_RGB5551;
        p->text.wS_Wrap      = CGL_TEXTURE_CLAMP;
        p->text.wT_Wrap      = CGL_TEXTURE_CLAMP;
        p->text.wChroma      = CGL_DISABLE;
    };

    /*** alles in Lot aufm Boot ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void cgl_KillPolyEngine(struct cgl_data *cd)
/*
**  FUNCTION
**      Gegenstück zu cgl_InitPolyEngine().
**
**  CHANGED
**      06-Dec-96   floh    created
*/
{
    if (cd->color_buf)  { _FreeVec(cd->color_buf); cd->color_buf=NULL; };
    if (cd->uvz_buf)    { _FreeVec(cd->uvz_buf); cd->uvz_buf=NULL; };
    if (cd->uv_buf)     { _FreeVec(cd->uv_buf); cd->uv_buf=NULL; };
    if (cd->vertex_buf) { _FreeVec(cd->vertex_buf); cd->vertex_buf=NULL; };
    if (cd->poly_buf)   { _FreeVec(cd->poly_buf); cd->poly_buf=NULL; };
}

/*-----------------------------------------------------------------*/
void cgl_FlushPolyBuf(struct cgl_data *cd)
/*
**  FUNCTION
**      Alle Polys im PolyBuf werden gerendert, und
**      der Puffer somit geleert.
**
**  CHANGED
**      06-Dec-96   floh    created
*/
{
    if (cd->poly_index > 0) {
        cglSendStream(cd->stream,cd->poly_index);
        cd->poly_index = 0;
        cd->vx_index   = 0;
    };
}

/*-----------------------------------------------------------------*/
struct cgl_Polygon *cgl_GetPoly(struct cgl_data *cd, ULONG nump)
/*
**  FUNCTION
**      Returniert ein vorinitialisiertes Element im
**      Polygon-Puffer, falls der Polygon-Puffer, oder
**      die Vertex-Puffer überlaufen würden, werden
**      sie zuerst geflusht (gerendert!).
**      Siehe cgl_PolyDone()!!!
**
**  INPUTS
**      cd      - Ptr auf LID des cgl.class Objects
**      nump    - Anzahl Punkte des Polys
**
**  RESULTS
**      p -- Pointer auf zu benutzende <struct cgl_Polygon>
**      cd->vx = zu benutzender Index in die Vertex-Arrays
**
**  CHANGED
**      06-Dec-96   floh    created
*/
{
    struct cgl_Polygon *p;

    /*** Überlauf? ***/
    if ((cd->poly_index      >= CGL_POLYBUF_MAX) ||
        ((cd->vx_index+nump) >  CGL_VERTEXBUF_MAX))
    {
        cgl_FlushPolyBuf(cd);
    };

    /*** Rückgabe-Parameter ***/
    p = &(cd->poly_buf[cd->poly_index]);
    cd->vx = cd->vx_index;
    return(p);
}

/*-----------------------------------------------------------------*/
void cgl_PolyDone(struct cgl_data *cd, ULONG nump)
/*
**  FUNCTION
**      Schaltet die Puffer-Indizes weiter, vorher muß
**      cgl_GetPoly() und die Polygon-Initialisierung
**      stattgefunden haben!
**
**  CHANGED
**      06-Dec-96   floh    created
*/
{
    cd->poly_index++;
    cd->vx_index += nump;
}

/*-----------------------------------------------------------------*/
BOOL cgl_DelayPolygon(struct cgl_data *cd, struct rast_poly *p)
/*
**  FUNCTION
**      Pusht Polygon auf den Delay-Stack.
**
**  INPUTS
**      cd      - LID des cgl.class Objects
**      p       - Polygon
**
**  RESULTS
**      TRUE    - all ok
**      FALSE   - kein Platz mehr auf dem Delay-Stack.
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    struct DelayedPoly *dp;

    dp = (struct DelayedPoly *) _RemTail((struct List *) &(cd->free_list));
    if (dp) {
        dp->p = p;
        _AddTail((struct List *) &(cd->delayed_list),
                 (struct Node *) dp);
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL cgl_TracyPolygon(struct cgl_data *cd, struct rast_poly *p)
/*
**  FUNCTION
**      Hängt den Poly an die Tracy-Liste (die muß als
**      letztes gezeichnet werden).
**
**  CHANGED
**      19-Aug-96   floh    created
*/
{
    struct DelayedPoly *dp;

    dp = (struct DelayedPoly *) _RemTail((struct List *) &(cd->free_list));
    if (dp) {
        dp->p = p;
        _AddHead((struct List *) &(cd->tracy_list),
                 (struct Node *) dp);
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void cgl_DrawPoly(struct cgl_data *cd,
                  struct rast_poly *p,
                  BOOL force, BOOL tracy)
/*
**  FUNCTION
**      Lowlevel-CGL-Polygon-Renderer.
**
**  INPUTS
**      cd    - Ptr auf LID des cgl.class Objects
**      p     - beschreibt den Polygon
**      force - TRUE: erzwinge Textur bei einem Cache-Miss, sonst
**              vefrachte den Polygon auf den Delayed-Cache.
**      tracy - TRUE: erzwinge das Rendern von transparenten
**              Polys (diese werden beim 1.Durchlauf delayed).
**
**  CHANGED
**      18-Aug-96   floh    created
**      02-Dec-96   floh    Textur-Filtering Config-abhängig
**      06-Dec-96   floh    arbeitet jetzt mit Primitiv-Stream-Puffer
**                          (dürfte etwas effizienter sein).
**      26-Feb-97   floh    ignoriert Texturemaps, die nicht das
**                          VBF_Texture Flag gesetzt haben.
*/
{
    ULONG i;
    ULONG t_handle = NULL;
    LONG xmin,xmax,ymin,ymax;
    FLOAT xsize,ysize;
    struct cgl_Polygon *cgl_p;
    CGL_VERTEX3D_ST *vertex;
    CGL_COLOR_ST *color;
    CGL_TEXTURE2D_ST *uv;
    CGL_TEXTURE3D_ST *uvz;

    /*** Eckpunkte ok? Textur ok? ***/
    if ((p->pnum < 3) || (p->pnum > CGL_MAXNUM_VERTEX)) return;
    if (!(p->map[0]->Flags & VBF_Texture)) return;

    /*** Transparent und nicht der Tracy-Pass -> Delay ***/
    if ((p->flags & (RPF_ZeroTracy|RPF_LUMTracy)) && (!tracy)) {
        cgl_TracyPolygon(cd,p);
        return;
    };

    /*** Textur bereitstellen, oder ein Delay ***/
    if (p->flags & (RPF_LinMap|RPF_PerspMap)) {
        t_handle = cgl_ValidateTexture(cd,p->map[0],force);
        if (!t_handle) { cgl_DelayPolygon(cd,p); return; };
    };

    /*** nächster Polygon im Stream-Puffer ***/
    cgl_p = cgl_GetPoly(cd,p->pnum);
    vertex = &(cd->vertex_buf[cd->vx]);
    color  = &(cd->color_buf[cd->vx]);
    uv     = &(cd->uv_buf[cd->vx]);
    uvz    = &(cd->uvz_buf[cd->vx]);
    cgl_p->rend.pVertex = vertex;
    cgl_p->rend.pColor  = color;
    cgl_p->text.dwTextureAddress = t_handle;

    /*** Eckpunkte konvertieren, MiniMax Ermittlung ***/
    xmin = xmax = 0;
    ymin = ymax = 0;
    for (i=0; i<p->pnum; i++) {
        vertex[i].x = (p->xyz[i].x + 1.0) * cd->x_scale;
        vertex[i].y = (p->xyz[i].y + 1.0) * cd->y_scale;
        vertex[i].z = p->xyz[i].z;
        if (vertex[i].x < vertex[xmin].x)      xmin=i;
        else if (vertex[i].x > vertex[xmax].x) xmax=i;
        if (vertex[i].y < vertex[ymin].y)      ymin=i;
        else if (vertex[i].y > vertex[ymax].y) ymax=i;
    };

    /*** Poly immer noch gültig? ***/
    if ((xsize = vertex[xmax].x - vertex[xmin].x) <= 0) return;
    if ((ysize = vertex[ymax].y - vertex[ymin].y) <= 0) return;

    /*** Linear-Map-Optimierung? ***/
    if (p->flags & RPF_PerspMap) {
        if ((xsize < 32) && (ysize < 32)) {
            p->flags &= ~RPF_PerspMap;
            p->flags |= RPF_LinMap;
        };
    };

    /*** konstante CGL-Parameter initialisieren ***/
    cgl_p->rend.dwNoOfVertices = p->pnum;

    if (p->pnum == 3)      cgl_p->rend.wPrimitiveType = CGL_TRIANGLE;
    else if (p->pnum == 4) cgl_p->rend.wPrimitiveType = CGL_QUAD;
    else                   cgl_p->rend.wPrimitiveType = CGL_TRIANGLE_FAN;

    cgl_p->rend.uPropertyEnableMask.i = 0;
    cgl_p->rend.uPropertyEnableMask.u.depth = 1;
    cgl_p->depth.writeEnable = CGL_ENABLE;

    /*** variable CGL-Parameter initialisieren ***/
    if (p->flags & RPF_LinMap) {

        /*** Linear Mapping ***/
        cgl_p->rend.uPropertyEnableMask.u.texture = 1;
        cgl_p->rend.pTexture = uv;

        cgl_p->text.wTextureMode   = CGL_COPY_TEXTURE; // changes, wenn Shading!
        cgl_p->text.wOverlay       = CGL_DISABLE;      // changes, wenn Tracy!
        cgl_p->text.wTextureFilter = CGL_TEXTURE_DISABLE_FILTER;
        if (((xsize>15)||(ysize>15)) && (cd->flags & CGLF_Filter)) {
            cgl_p->text.wTextureFilter = CGL_TEXTURE_FILTER_TYPE_1;
        };

        /*** UV-Channel füllen ***/
        for (i=0; i<p->pnum; i++) {
            uv[i].s = p->uv[i].u * 256.0;
            uv[i].t = p->uv[i].v * 256.0;
        };

    } else if (p->flags & RPF_PerspMap) {

        FLOAT min_z;

        /*** Perspektiv-Mapping ***/
        cgl_p->rend.uPropertyEnableMask.u.texture     = 1;
        cgl_p->rend.uPropertyEnableMask.u.perspective = 1;
        cgl_p->rend.pTexture = uvz;

        cgl_p->text.wTextureMode   = CGL_COPY_TEXTURE; // changes, wenn Shading!
        cgl_p->text.wOverlay       = CGL_DISABLE;      // changes, wenn Tracy!
        cgl_p->text.wTextureFilter = CGL_TEXTURE_DISABLE_FILTER;
        if (((xsize>15)||(ysize>15)) && (cd->flags & CGLF_Filter)) {
            cgl_p->text.wTextureFilter = CGL_TEXTURE_FILTER_TYPE_1;
        };

        /*** suche min_z für Perspektiv-Korrektur ***/
        min_z = 128000.0;
        for (i=0; i<p->pnum; i++) {
            if (p->xyz[i].z < min_z) min_z = p->xyz[i].z;
        };

        /*** fülle uvz[] Channel ***/
        for (i=0; i<p->pnum; i++) {
            FLOAT q = min_z / p->xyz[i].z;
            uvz[i].s = p->uv[i].u * 256.0 * q;
            uvz[i].t = p->uv[i].v * 256.0 * q;
            uvz[i].q = q;
        };
    };

    if (p->flags & (RPF_FlatShade|RPF_GradShade)) {

        /*** Polygon ist Gourauld-geshadet ***/
        cgl_p->rend.uPropertyEnableMask.u.shade = 1;
        cgl_p->text.wTextureMode = CGL_MODULATE_TEXTURE;

        /*** Color-Channel füllen (kein farbiges Shading) ***/
        for (i=0; i<p->pnum; i++) {
            LONG b = 255 - (FLOAT_TO_INT(p->b[i]*255.0));
            color[i].bRed   = b;
            color[i].bGreen = b;
            color[i].bBlue  = b;
            color[i].bAlpha = 255;
        };
    } else {
        /*** kein Shading ***/
        color[0].bRed   = 0;
        color[0].bGreen = 0;
        color[0].bBlue  = 0;
        color[0].bAlpha = 255;
    };

    /*** Transparenz-Handling ***/
    if (p->flags & (RPF_ZeroTracy|RPF_LUMTracy)) {
        cgl_p->depth.writeEnable = CGL_DISABLE;
        cgl_p->text.wOverlay     = CGL_ENABLE;
        if (p->flags & RPF_LUMTracy) {
            cgl_p->text.wTextureMode = CGL_DECAL_TEXTURE;
            cgl_p->rend.uPropertyEnableMask.u.blend = 1;
            cgl_p->blend.bSrcAlphaValue = 64;
        };
    };

    /*** Rendering erst in cgl_Flush ***/
    cgl_PolyDone(cd,p->pnum);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_Poly, struct rast_poly *msg)
/*
**  CHANGED
**      19-Aug-96   floh    created
*/
{
    /*** in diesem Durchlauf können Polygine beliebig delayed werden! ***/
    struct cgl_data *cd = INST_DATA(cl,o);
    cgl_DrawPoly(cd,msg,FALSE,FALSE);
}

/*-----------------------------------------------------------------*/
void cgl_FlushDelayed(struct cgl_data *cd)
/*
**  FUNCTION
**      Erzwingt das Rendern aller delayed Polygone.
**
**  CHANGED
**      19-Aug-96   floh    created
*/
{
    struct MinList *ls = &(cd->delayed_list);
    struct MinNode *nd;
    struct MinNode *next;
    struct VFMBitmap *map;


    while ((nd = ls->mlh_Head)->mln_Succ) {

        map = NULL;
        while (nd->mln_Succ) {

            struct DelayedPoly *dp = (struct DelayedPoly *) nd;

            /*** Lookahead Pointer ***/
            next = nd->mln_Succ;

            /*** jeweils alle Polygone mit identischer Textur wegrendern ***/
            if (!map) map=dp->p->map[0];
            if (map == dp->p->map[0]) {
                cgl_DrawPoly(cd,dp->p,TRUE,FALSE);
                _Remove((struct Node *)nd);
                _AddTail((struct List *) &(cd->free_list),
                         (struct Node *) nd);
            };
            nd = next;
        };
    };
}

/*-----------------------------------------------------------------*/
void cgl_FlushTracy(struct cgl_data *cd)
/*
**  FUNCTION
**      Erzwingt das Rendern aller transparenten Polygone.
**
**  CHANGED
**      19-Aug-96   floh    created
*/
{
    struct MinList *ls = &(cd->tracy_list);
    struct MinNode *nd;
    struct MinNode *next;
    struct VFMBitmap *map;


    while ((nd = ls->mlh_Head)->mln_Succ) {

        map = NULL;
        while (nd->mln_Succ) {

            struct DelayedPoly *dp = (struct DelayedPoly *) nd;

            /*** Lookahead Pointer ***/
            next = nd->mln_Succ;

            /*** jeweils alle Polygone mit identischer Textur wegrendern ***/
            if (!map) map=dp->p->map[0];
            if (map == dp->p->map[0]) {
                cgl_DrawPoly(cd,dp->p,TRUE,TRUE);
                _Remove((struct Node *)nd);
                _AddTail((struct List *) &(cd->free_list),
                         (struct Node *) nd);
            };
            nd = next;
        };
    };
}


