/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_poly.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 15:02:52 $
**  $Locker: floh $
**  $Author: floh $
**
**  w3d_poly.c -- Polygon-Engine (Windows-Gummizelle)
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include "stdlib.h"
#include "string.h"

#define WIN3D_WINBOX
#include "bitmap/win3dclass.h"
#include "w3d_protos.h"

/*** importiert aus windd.class ***/
extern LPDIRECTDRAW lpDD;
extern LPDIRECT3D2  lpD3D2;
extern struct wdd_Data wdd_Data;

/*** aus w3d_txtcache.c ***/
void *w3d_ValidateTexture(struct windd_data *,struct win3d_data *,struct w3d_BmpAttach *,unsigned long);
void wdd_FailMsg(char *title, char *msg, unsigned long code);
void wdd_Log(char *string,...);

#define ROUND D3DVAL

/*-----------------------------------------------------------------*/
unsigned long w3d_RenderStateChanged(struct windd_data *wdd,
                                     struct win3d_data *w3d,
                                     unsigned long flush)
/*
**  FUNCTION
**      Vergleicht <glb_state> und <cur_state> miteinander.
**      Falls <cur_state> abweicht, werden die Differenzen
**      als Renderstate-Instruktionen in den Execute
**      Buffer geschrieben und <glb_state> updated.
**
**  RETURNS
**      TRUE    - mindestens 1 der Renderstates hat sich
**                geändert
**      FALSE   - Renderstate hat sich nicht geändert
**
**  CHANGED
**      12-Mar-97   floh    created
**      11-May-98   floh    alternativ Drawprimitive
*/
{
    unsigned long i;
    unsigned long num;
    unsigned char *inst;
    struct w3d_Execute *exec = &(w3d->p->exec);

    /*** Pass1: Unterschiede zählen ***/
    if (flush) num=W3DSTATE_NUM;
    else {
        num=0;
        for (i=0; i<W3DSTATE_NUM; i++) {
            if (exec->glb_state[i].status != exec->cur_state[i].status) num++;
        };
    };

    /*** RenderState Instruktions-Header ***/
    if (num > 0) {
        w3d_BeginRenderStates(wdd,w3d);
        for (i=0; i<W3DSTATE_NUM; i++) {
            if ((exec->glb_state[i].status!=exec->cur_state[i].status)||flush) {
                w3d_RenderState(wdd,w3d,exec->cur_state[i].id,exec->cur_state[i].status);
            };
            exec->glb_state[i].status = exec->cur_state[i].status;
        };
        w3d_EndRenderStates(wdd,w3d);
    } else return(FALSE);

    /*** das war's bereits ***/
    return(TRUE);
}


/*=================================================================**
**  INITIALISIERUNG                                                **
**=================================================================*/

/*-----------------------------------------------------------------*/
void w3d_KillPolyEngine(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
unsigned long w3d_InitPolyEngine(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
**      12-Mar-97   floh    initialisiert jetzt ExecuteBuffer und
**                          Background-Material
*/
{
    HRESULT ddrval;
    LPVOID lpBuffer,lpInstStart;
    struct w3d_Execute *exec = &(w3d->p->exec);

     /*** Renderstate-Strukturen initialisieren ***/
    exec->glb_state[W3DSTATE_TEXTUREHANDLE].id      = D3DRENDERSTATE_TEXTUREHANDLE;
    exec->glb_state[W3DSTATE_TEXTUREPERSPECTIVE].id = D3DRENDERSTATE_TEXTUREPERSPECTIVE;
    exec->glb_state[W3DSTATE_SHADEMODE].id          = D3DRENDERSTATE_SHADEMODE;
    exec->glb_state[W3DSTATE_STIPPLEENABLE].id      = D3DRENDERSTATE_STIPPLEENABLE;
    exec->glb_state[W3DSTATE_SRCBLEND].id           = D3DRENDERSTATE_SRCBLEND;
    exec->glb_state[W3DSTATE_DESTBLEND].id          = D3DRENDERSTATE_DESTBLEND;
    exec->glb_state[W3DSTATE_TEXTUREMAPBLEND].id    = D3DRENDERSTATE_TEXTUREMAPBLEND;
    exec->glb_state[W3DSTATE_BLENDENABLE].id        = D3DRENDERSTATE_BLENDENABLE;
    exec->glb_state[W3DSTATE_ZWRITEENABLE].id       = D3DRENDERSTATE_ZWRITEENABLE;
    exec->glb_state[W3DSTATE_MAGTEXTUREFILTER].id   = D3DRENDERSTATE_TEXTUREMAG;
    exec->glb_state[W3DSTATE_MINTEXTUREFILTER].id   = D3DRENDERSTATE_TEXTUREMIN;

    exec->glb_state[W3DSTATE_TEXTUREHANDLE].status      = NULL;
    exec->glb_state[W3DSTATE_TEXTUREPERSPECTIVE].status = FALSE;
    exec->glb_state[W3DSTATE_SHADEMODE].status          = D3DSHADE_GOURAUD;
    exec->glb_state[W3DSTATE_STIPPLEENABLE].status      = FALSE;
    exec->glb_state[W3DSTATE_SRCBLEND].status           = D3DBLEND_ONE;
    exec->glb_state[W3DSTATE_DESTBLEND].status          = D3DBLEND_ZERO;
    exec->glb_state[W3DSTATE_TEXTUREMAPBLEND].status    = D3DTBLEND_MODULATE;
    exec->glb_state[W3DSTATE_BLENDENABLE].status        = FALSE;
    exec->glb_state[W3DSTATE_ZWRITEENABLE].status       = TRUE;
    exec->glb_state[W3DSTATE_MAGTEXTUREFILTER].status   = (w3d->filter?D3DFILTER_LINEAR:D3DFILTER_NEAREST);
    exec->glb_state[W3DSTATE_MINTEXTUREFILTER].status   = exec->glb_state[W3DSTATE_MAGTEXTUREFILTER].status;

    memcpy(&(exec->cur_state),&(exec->glb_state),sizeof(exec->cur_state));

    w3d_BeginScene(wdd,w3d);
    w3d_BeginRenderStates(wdd,w3d);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_TEXTUREADDRESS,D3DTADDRESS_CLAMP);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_ZENABLE,TRUE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_TEXTUREMAG,(w3d->filter?D3DFILTER_LINEAR:D3DFILTER_NEAREST));
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_CULLMODE,D3DCULL_NONE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_ZFUNC,D3DCMP_LESSEQUAL);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_DITHERENABLE,w3d->dither);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_BLENDENABLE,FALSE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_FOGENABLE,FALSE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_SUBPIXEL,TRUE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_STIPPLEDALPHA,wdd_Data.Driver.CanDoStipple);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_STIPPLEENABLE,FALSE);
    w3d_RenderState(wdd,w3d,D3DRENDERSTATE_COLORKEYENABLE,TRUE);
    w3d_EndRenderStates(wdd,w3d);
    w3d_EndScene(wdd,w3d);

    /*** Delayed-Polygon-Arrays initialisieren ***/
    w3d->p->num_solid = 0;
    memset(&(w3d->p->solid),0,sizeof(w3d->p->solid));
    w3d->p->num_tracy = 0;
    memset(&(w3d->p->tracy),0,sizeof(w3d->p->num_tracy));

    /*** färtsch... ***/
    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
void w3d_DelayPolygon(struct windd_data *wdd,
                      struct win3d_data *w3d,
                      struct w3d_RastPoly *p,
                      struct w3d_BmpAttach *bmp_attach)
/*
**  FUNCTION
**      Legt einen Polygon auf dem Delay-Stack ab.
**
**  CHANGED
**      18-Mar-97   floh    created
*/
{
    if (w3d->p->num_solid < W3D_MAXNUM_DELAYED) {
        w3d->p->solid[w3d->p->num_solid].p = p;
        w3d->p->solid[w3d->p->num_solid].bmp_attach = bmp_attach;
        w3d->p->num_solid++;
    };
}

/*-----------------------------------------------------------------*/
unsigned long w3d_TracyPolygon(struct windd_data *wdd,
                               struct win3d_data *w3d,
                               struct w3d_RastPoly *p,
                               struct w3d_BmpAttach *bmp_attach)
/*
**  FUNCTION
**      Legt einen Polygon auf dem Tracy-Stack ab.
**
**  CHANGED
**      18-Mar-97   floh    created
*/
{
    if (w3d->p->num_tracy < W3D_MAXNUM_DELAYED) {
        w3d->p->tracy[w3d->p->num_tracy].p = p;
        w3d->p->tracy[w3d->p->num_tracy].bmp_attach = bmp_attach;
        w3d->p->num_tracy++;
    };
}

/*-----------------------------------------------------------------*/
void w3d_DrawPoly(struct windd_data *wdd,
                  struct win3d_data *w3d,
                  struct w3d_RastPoly *p,
                  struct w3d_BmpAttach *bmp_attach,
                  unsigned long force,
                  unsigned long tracy)
/*
**  FUNCTION
**      Rendert einen Polygon.
**
**  CHANGED
**      12-Mar-97   floh    created
**      04-Apr-97   floh    + Paletten-Effekte
**      03-Mar-98   floh    + Transparenz fuer SrcAlpha/InvSrcAlpha-Only-
**                            Karten gefixt
**      08-Jun-98   floh    + es wird nix mehr gemacht, wenn begin_scene_ok
**                            nicht ok ist
*/
{
    if (w3d->p->exec.begin_scene_ok) {

        long xmin,xmax,ymin,ymax;
        unsigned long i,vx_offset;
        unsigned char *inst;
        float xsize,ysize;
        D3DTLVERTEX v_array[24];
        D3DTEXTUREHANDLE t_handle;
        HRESULT ddrval;    
        struct w3d_Execute *exec = &(w3d->p->exec);

        /*** Eckpunkte ok? ***/
        if ((p->pnum<3) || (p->pnum>12)) return;

        /*** Transparent und nicht der Tracy-Pass -> Delay ***/
        if ((p->flags & (W3DF_RPOLY_ZEROTRACY|W3DF_RPOLY_LUMTRACY)) && (!tracy)) {
            w3d_TracyPolygon(wdd,w3d,p,bmp_attach);
            return;
        };

        /*** Textur bereitstellen, oder ein Delay ***/
        if (p->flags & (W3DF_RPOLY_LINMAP|W3DF_RPOLY_PERSPMAP)) {
            t_handle = w3d_ValidateTexture(wdd,w3d,bmp_attach,force);
            /*** CacheMiss... ***/
            if (!t_handle) {
                w3d_DelayPolygon(wdd,w3d,p,bmp_attach);
                return;
            };
        };

        /*** Eckpunkte konvertieren, MiniMax-Ermittlung ***/
        xmin = xmax = 0;
        ymin = ymax = 0;
        for (i=0; i<p->pnum; i++) {
            v_array[i].sx       = ROUND((p->xyz[i*3+0] + 1.0) * w3d->p->x_scale);
            v_array[i].sy       = ROUND((p->xyz[i*3+1] + 1.0) * w3d->p->y_scale);
            v_array[i].sz       = ROUND(p->xyz[i*3+2] / 8192.0);
            v_array[i].rhw      = ROUND(1.0 / p->xyz[i*3+2]);
            v_array[i].color    = RGBA_MAKE(255,255,255,255);
            v_array[i].specular = RGB_MAKE(0,0,0);
            v_array[i].tu       = D3DVAL(0.0);
            v_array[i].tv       = D3DVAL(0.0);
            if (v_array[i].sx < v_array[xmin].sx)      xmin=i;
            else if (v_array[i].sx > v_array[xmax].sx) xmax=i;
            if (v_array[i].sy < v_array[ymin].sy)      ymin=i;
            else if (v_array[i].sy > v_array[ymax].sy) ymax=i;
        };

        /*** Poly immer noch gültig? ***/
        if ((xsize = v_array[xmax].sx - v_array[xmin].sx) <= 0) return;
        if ((ysize = v_array[ymax].sy - v_array[ymin].sy) <= 0) return;

        /*** Linear-Map-Optimierung ***/
        if (p->flags & W3DF_RPOLY_PERSPMAP) {
            if ((xsize < 32) && (ysize < 32)) {
                p->flags &= ~W3DF_RPOLY_PERSPMAP;
                p->flags |= W3DF_RPOLY_LINMAP;
            };
        };

        /*** einen definierten Renderstate-Zustand herstellen ***/
        exec->cur_state[W3DSTATE_TEXTUREHANDLE].status      = NULL;
        exec->cur_state[W3DSTATE_TEXTUREPERSPECTIVE].status = FALSE;
        exec->cur_state[W3DSTATE_SHADEMODE].status          = D3DSHADE_FLAT;
        exec->cur_state[W3DSTATE_STIPPLEENABLE].status      = FALSE;
        exec->cur_state[W3DSTATE_SRCBLEND].status           = D3DBLEND_ONE;
        exec->cur_state[W3DSTATE_DESTBLEND].status          = D3DBLEND_ZERO;
        exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status    = D3DTBLEND_COPY;
        exec->cur_state[W3DSTATE_BLENDENABLE].status        = FALSE;
        exec->cur_state[W3DSTATE_ZWRITEENABLE].status       = TRUE;
        exec->cur_state[W3DSTATE_MAGTEXTUREFILTER].status   = (w3d->filter?D3DFILTER_LINEAR:D3DFILTER_NEAREST);
        exec->cur_state[W3DSTATE_MINTEXTUREFILTER].status   = exec->cur_state[W3DSTATE_MAGTEXTUREFILTER].status;

        if (p->flags & W3DF_RPOLY_LINMAP) {

            /*** Renderstate-Mods für Linearmapping ***/
            exec->cur_state[W3DSTATE_TEXTUREHANDLE].status   = t_handle;

            /*** [u,v] Channel füllen ***/
            for (i=0; i<p->pnum; i++) {
                v_array[i].tu = ROUND(p->uv[i*2+0]);
                v_array[i].tv = ROUND(p->uv[i*2+1]);
            };

        } else if (p->flags & W3DF_RPOLY_PERSPMAP) {

            /*** RenderState-Mods für Perspektiv-Mapping ***/
            exec->cur_state[W3DSTATE_TEXTUREHANDLE].status      = t_handle;
            exec->cur_state[W3DSTATE_TEXTUREPERSPECTIVE].status = TRUE;

            /*** [u,v] Channel füllen ***/
            for (i=0; i<p->pnum; i++) {
                v_array[i].tu = p->uv[i*2+0];
                v_array[i].tv = p->uv[i*2+1];
            };
        } else {
            /*** schwarz und flat ***/
            for (i=0; i<p->pnum; i++) {
                v_array[i].color = RGBA_MAKE(0,0,0,255);
            };
        };

        /*** Shading ***/
        if (p->flags & (W3DF_RPOLY_FLATSHADE|W3DF_RPOLY_GRADSHADE)) {
            /*** Renderstate-Mods für Gourauld-Shading ***/
            exec->cur_state[W3DSTATE_SHADEMODE].status       = D3DSHADE_GOURAUD;
            exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATE;
            /*** [rgba] Channel mit Helligkeit füllen ***/
            for (i=0; i<p->pnum; i++) {
                long b = (long) ((1.0 - p->b[i])*255.0);
                v_array[i].color = RGBA_MAKE(b,b,b,255);
            };
        };

        /*** Transparenz ***/
        if (p->flags & W3DF_RPOLY_LUMTRACY){
            if (!w3d->zbufwhentracy) {
                exec->cur_state[W3DSTATE_ZWRITEENABLE].status = FALSE;
            };
            if (wdd_Data.Driver.CanDoAdditiveBlend) {
                /*** Idealfall...additives Blending supported ***/
                exec->cur_state[W3DSTATE_SHADEMODE].status       = D3DSHADE_FLAT;
                exec->cur_state[W3DSTATE_BLENDENABLE].status     = TRUE;
                exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATEALPHA;
                exec->cur_state[W3DSTATE_SRCBLEND].status        = D3DBLEND_SRCALPHA;
                exec->cur_state[W3DSTATE_DESTBLEND].status       = D3DBLEND_ONE;
            } else if (wdd_Data.Driver.CanDoAlpha) {
                /*** sonst additiv-Emulation per Alphachannel ***/
                exec->cur_state[W3DSTATE_SHADEMODE].status       = D3DSHADE_FLAT;
                exec->cur_state[W3DSTATE_BLENDENABLE].status     = TRUE;
                exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATEALPHA;
                exec->cur_state[W3DSTATE_SRCBLEND].status        = D3DBLEND_SRCALPHA;
                exec->cur_state[W3DSTATE_DESTBLEND].status       = D3DBLEND_INVSRCALPHA;
            } else if (wdd_Data.Driver.CanDoStipple) {
                /*** sonst Stipple Transparenz Emulation ***/
                exec->cur_state[W3DSTATE_SHADEMODE].status       = D3DSHADE_FLAT;
                exec->cur_state[W3DSTATE_BLENDENABLE].status     = TRUE;
                exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATEALPHA;
                exec->cur_state[W3DSTATE_SRCBLEND].status        = D3DBLEND_SRCALPHA;
                exec->cur_state[W3DSTATE_DESTBLEND].status       = D3DBLEND_INVSRCALPHA;
                exec->cur_state[W3DSTATE_STIPPLEENABLE].status   = TRUE;
            };
            for (i=0; i<p->pnum; i++) {
                v_array[i].color &= 0x00ffffff;
                v_array[i].color |= ((w3d->alpha)<<24);
            };
        }else if (p->flags & W3DF_RPOLY_ZEROTRACY){
            if (!w3d->zbufwhentracy) {
                exec->cur_state[W3DSTATE_ZWRITEENABLE].status = FALSE;
            };
            /*** bei 8-Bit-Texturen, normales Colorkeying, sonst Alphablending ***/
            if (w3d->p->txt_pfmt.byte_size != 1) {
                /*** per Alphablending ***/
                exec->cur_state[W3DSTATE_SRCBLEND].status  = D3DBLEND_SRCALPHA;
                exec->cur_state[W3DSTATE_DESTBLEND].status = D3DBLEND_INVSRCALPHA;
            };
            exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATE;
            exec->cur_state[W3DSTATE_BLENDENABLE].status     = TRUE;
            /*** Texture-Filtering ausschalten ***/
            exec->cur_state[W3DSTATE_MAGTEXTUREFILTER].status = D3DFILTER_NEAREST;
            exec->cur_state[W3DSTATE_MINTEXTUREFILTER].status = D3DFILTER_NEAREST;
        };

        /*** RenderState-Instruktionen schreiben ***/
        w3d_RenderStateChanged(wdd,w3d,FALSE);
        
        /*** Dreiecke schreiben ***/
        w3d_Primitive(wdd,w3d,(LPVOID)v_array,p->pnum);
    };
}

/*-----------------------------------------------------------------*/
int w3d_SolidCmpFunc(struct w3d_DelayedPoly *p1,
                     struct w3d_DelayedPoly *p2)
/*
**  FUNCTION
**      Sortier-Hook für Delayed-Polys. Sortiert wird nach
**      Textur, so daß Polys mit identischer Textur
**      aufeinanderfolgen.
**
**  CHANGED
**      18-Mar-97   floh    created
*/
{
    return(p2->bmp_attach->lpTexture - p1->bmp_attach->lpTexture);
}

/*-----------------------------------------------------------------*/
int w3d_TracyCmpFunc(struct w3d_DelayedPoly *p1,
                     struct w3d_DelayedPoly *p2)
/*
**  FUNCTION
**      Sortier-Hook für Delayed-Polys. Sortiert wird nach
**      Textur, so daß Polys mit identischer Textur
**      aufeinanderfolgen.
**
**  CHANGED
**      18-Mar-97   floh    created
*/
{
    if (p1->bmp_attach->lpTexture < p2->bmp_attach->lpTexture) return(-1);
    else if (p2->bmp_attach->lpTexture < p1->bmp_attach->lpTexture) return(1);
    else if (p2->max_z < p1->max_z) return(-1);
    else if (p1->max_z < p2->max_z) return(1);
    else return(0);
}

/*-----------------------------------------------------------------*/
void w3d_FlushDelayed(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Rendert alle Polygone, die sich auf dem Delay-
**      Stack befinden.
**
**  CHANGED
**      18-Mar-97   floh    created
**      19-Mar-97   floh    immer wenn eine neue Textur "encountered"
**                          wird, muss der ExecuteBuffer geflusht werden.
*/
{
    ENTERED("w3d_FlushDelayed");
    if (w3d->p->exec.begin_scene_ok) {
        if (w3d->p->num_solid > 0) {

            unsigned long i;

            /*** sortiere Delayed-Array nach Texturen ***/
            qsort(&(w3d->p->solid[0]),w3d->p->num_solid,
                  sizeof(struct w3d_DelayedPoly),
                  w3d_SolidCmpFunc);

            /*** Polys mit identischer Textur folgen jetzt aufeinander ***/
            for (i=0; i<w3d->p->num_solid; i++) {
                /*** Polygone wegrendern... ***/
                struct w3d_DelayedPoly *dp = &(w3d->p->solid[i]);
                w3d_DrawPoly(wdd,w3d,dp->p,dp->bmp_attach,TRUE,FALSE);
            };
            w3d->p->num_solid = 0;
        };
    };
    LEFT("w3d_FlushDelayed");
}

/*-----------------------------------------------------------------*/
void w3d_FlushTracy(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Rendert alle Polygone, die sich auf dem Tracy-
**      Stack befinden.
**
**  CHANGED
**      18-Mar-97   floh    created
*/
{
    ENTERED("w3d_FlushTracy");
    if (w3d->p->exec.begin_scene_ok) {
        if (w3d->p->num_tracy > 0) {

            unsigned long i;
            void *act_txt = NULL;
            
            /*** <max_z> initialisieren ***/
            for (i=0; i<w3d->p->num_tracy; i++) {
                unsigned long j;
                struct w3d_DelayedPoly *dp = &(w3d->p->tracy[i]);
                float max_z = 0.0;
                for (j=0; j<dp->p->pnum; j++) {
                    if (max_z < dp->p->xyz[j*3+2]) max_z=dp->p->xyz[j*3+2];
                };
                dp->max_z = max_z;
            };

            /*** sortiere Delayed-Array nach Texturen ***/
            qsort(&(w3d->p->tracy[0]),w3d->p->num_tracy,
                  sizeof(struct w3d_DelayedPoly),
                  w3d_TracyCmpFunc);
            
            /*** Polys mit identischer Textur folgen jetzt aufeinander ***/
            for (i=0; i<w3d->p->num_tracy; i++) {
                /*** Polygone wegrendern... ***/
                struct w3d_DelayedPoly *dp = &(w3d->p->tracy[i]);
                w3d_DrawPoly(wdd,w3d,dp->p,dp->bmp_attach,TRUE,TRUE);
            };
            w3d->p->num_tracy = 0;
        };
    };
    LEFT("w3d_FlushTracy");
}

