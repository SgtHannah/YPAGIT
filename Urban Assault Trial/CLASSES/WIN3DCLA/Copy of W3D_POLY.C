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

/*** importiert aus windd.class ***/
extern LPDIRECTDRAW lpDD;
extern LPDIRECT3D2  lpD3D2;
extern struct wdd_Data wdd_Data;

/*** aus w3d_txtcache.c ***/
void *w3d_ValidateTexture(struct windd_data *,struct win3d_data *,struct w3d_BmpAttach *,unsigned long);

void w3d_CheckFlushBuffer(struct windd_data *wdd,
                          struct win3d_data *w3d,
                          unsigned char *ptr);

/*=================================================================**
**  EXECUTE BUFFER HANDLING                                        **
**=================================================================*/

/*-----------------------------------------------------------------*/
unsigned char *w3d_BeginProcessVertices(struct windd_data *wdd,
                                        struct win3d_data *w3d,
                                        unsigned long cnt)

/*
**  FUNCTION
**      Schreibt eine ProcessVertices Instruktion in den
**      Render-Puffer.
**
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    unsigned char *end_ptr = exec->inst +
                             2*sizeof(D3DINSTRUCTION) +
                             cnt*sizeof(D3DPROCESSVERTICES);
    w3d_CheckFlushBuffer(wdd,w3d,end_ptr);
    OP_PROCESS_VERTICES(cnt,exec->inst);
    return(exec->inst);
}

/*-----------------------------------------------------------------*/
void w3d_EndProcessVertices(struct windd_data *wdd,
                            struct win3d_data *w3d,
                            unsigned char *ptr)
/*
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    exec->inst = ptr;
}

/*-----------------------------------------------------------------*/
unsigned char *w3d_BeginRenderState(struct windd_data *wdd,
                                    struct win3d_data *w3d,
                                    unsigned long cnt)
/*
**  FUNCTION
**      Startet eine Sequenz von RenderState-Modifkationen.
**      Testet auf Buffer-Überlauf, wenn dem so ist, wird
**      der Buffer geflusht.
**
**  CHANGED
**      12-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    unsigned char *end_ptr   = exec->inst +
                               2*sizeof(D3DINSTRUCTION) +
                               cnt*sizeof(D3DSTATE);

    /*** Puffer-Überlauf? OP_EXIT() muß mit eingerechnet werden! ***/
    w3d_CheckFlushBuffer(wdd,w3d,end_ptr);
    if (QWORD_ALIGNED(exec->inst)) {
        OP_NOP(exec->inst);
    };
    OP_STATE_RENDER(cnt,exec->inst);
    return(exec->inst);
}

/*-----------------------------------------------------------------*/
void w3d_EndRenderState(struct windd_data *wdd,
                        struct win3d_data *w3d,
                        unsigned char *ptr)
/*
**  FUNCTION
**      Beendet die Definition eines Renderstate-Blocks.
**      Der übergebene Pointer wird nach <exec->inst>
**      geschrieben.
**
**  CHANGED
**      12-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    exec->inst = ptr;
}

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
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    unsigned long i;
    unsigned long num;
    unsigned char *inst;

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
        inst = w3d_BeginRenderState(wdd,w3d,num);
        if (inst) {
            /*** Pass2: Diff-States schreiben ***/
            for (i=0; i<W3DSTATE_NUM; i++) {
                if ((exec->glb_state[i].status!=exec->cur_state[i].status)||flush) {
                    STATE_DATA(exec->cur_state[i].id,exec->cur_state[i].status,inst);
                    /*** Global State updaten ***/
                    exec->glb_state[i].status = exec->cur_state[i].status;
                };
            };
            w3d_EndRenderState(wdd,w3d,inst);
        };
    } else return(FALSE);

    /*** das war's bereits ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void w3d_BeginScene(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Kapselt BeginScene-Calls zum Render-Device. Muß
**      1x pro Frame in DISPM_Begin aufgerufen werden.
**
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    HRESULT ddrval;
    ddrval = wdd->d3d->lpD3DDevice->lpVtbl->BeginScene(wdd->d3d->lpD3DDevice);
}

/*-----------------------------------------------------------------*/
void w3d_EndScene(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Kapselt EndScene Calls zum Render-Device. Muß
**      1x pro Frame in RASTM_Flush aufgerufen werden.
**
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    HRESULT ddrval;
    ddrval = wdd->d3d->lpD3DDevice->lpVtbl->EndScene(wdd->d3d->lpD3DDevice);
}

/*-----------------------------------------------------------------*/
unsigned long w3d_BeginRender(struct windd_data *wdd,
                              struct win3d_data *w3d)
/*
**  FUNCTION
**      Bereitet ExecuteBuffer zum Füllen vor:
**
**          * Buffer wird gelocked
**          * w3d->p->is_locked     -> TRUE
**          * w3d->p->exec.start
**            w3d->p->exec.end
**            w3d->p->exec.inst
**            w3d->p->exec.vertex
**            Pointer werden validiert.
**
**      Die Funktion muß innerhalb BeginScene/EndScene
**      plaziert sein. Sobald der Buffer voll ist,
**      muß ein w3d_EndRendering()/w3d_BeginRendering()
**      ausgeführt werden, welches den Buffer leert.
**      Das passiert automatisch in den weiteren w3d_Begin#?()
**      Methoden. Unmittelbar vor EndScene muß pauschal
**      ein EndRender() gemacht werden, um den halbvollen
**      Buffer zu flushen.
**
**  CHANGED
**      12-Mar-97   floh    created
**      03-Mar-98   floh    + Executebuffer wird nicht mehr gelöscht
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    D3DEXECUTEBUFFERDESC exbuf_desc;
    HRESULT ddrval;
    unsigned char *inst;

    exec->is_locked = FALSE;
    memset(&exbuf_desc,0,sizeof(exbuf_desc));
    exbuf_desc.dwSize = sizeof(exbuf_desc);

    do {
        ddrval = wdd->d3d->lpD3DExecBuffer->lpVtbl->Lock(wdd->d3d->lpD3DExecBuffer,&exbuf_desc);
        if (ddrval == D3DERR_EXECUTE_LOCKED) {
            /*** Ooops, hier hat jemand RASTM_Flush vergessen ***/
            wdd->d3d->lpD3DExecBuffer->lpVtbl->Unlock(wdd->d3d->lpD3DExecBuffer);
        };
    } while (ddrval != D3D_OK);

    exec->is_locked     = TRUE;
    exec->start         = exbuf_desc.lpData;
    exec->end           = ((char *)exec->start) + exbuf_desc.dwBufferSize;
    exec->inst          = exec->start;
    exec->vertex        = exec->end;
    exec->last_triangle = NULL;

    // FIXME: lösche den Executebuffer
    // memset(exec->start,0,(exec->end-exec->start));

    /*** Schreibe eine einzige PROCESS_VERTICES an den Anfang des Buffers ***/
    inst = w3d_BeginProcessVertices(wdd,w3d,1);
        PROCESSVERTICES_DATA(D3DPROCESSVERTICES_COPY,
                             0, 0,
                             inst);
    w3d_EndProcessVertices(wdd,w3d,inst);

    return(TRUE);
}

/*-----------------------------------------------------------------*/
void w3d_EndRender(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      -> setzt ein OP_EXIT() an das Ende des Instruction-Streams
**      -> Unlockt den Puffer
**      -> Executed den Puffer
**      -> w3d->p->is_locked     -> FALSE
**         w3d->p->exec.start
**         w3d->p->exec.end
**         w3d->p->exec.inst
**         w3d->p->exec.vertex
**         Pointer werden invalidiert.
**
**  CHANGED
**      12-Mar-97   floh    created
*/
{
    HRESULT ddrval;
    struct w3d_Execute *exec = &(w3d->p->exec);
    D3DEXECUTEDATA exec_data;
    LPD3DPROCESSVERTICES vinst_ptr;

    /*** überschreibe Start und Anzahl Vertices ***/
    vinst_ptr = (LPD3DPROCESSVERTICES) (exec->start + sizeof(D3DINSTRUCTION));
    vinst_ptr->wStart  = (exec->vertex - exec->start)/sizeof(D3DTLVERTEX);
    vinst_ptr->wDest   = vinst_ptr->wStart;
    vinst_ptr->dwCount = ((exec->end-exec->vertex)/sizeof(D3DTLVERTEX)),

    /*** Execute Buffer terminieren ***/
    OP_EXIT(exec->inst);

    /*** Execute Buffer unlocken... ***/
    exec->is_locked = FALSE;
    ddrval = wdd->d3d->lpD3DExecBuffer->lpVtbl->Unlock(wdd->d3d->lpD3DExecBuffer);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("win3d.class/w3d_poly/w3d_EndRender","IDirect3DExecuteBuffer::Unlock()",ddrval);
        return;
    };

    /*** ... und ausführen ***/
    memset(&exec_data,0,sizeof(exec_data));
    exec_data.dwSize = sizeof(exec_data);
    exec_data.dwVertexOffset = (unsigned long) ((exec->vertex - exec->start)/(sizeof(D3DTLVERTEX)));
    exec_data.dwVertexCount  = (unsigned long) ((exec->end - exec->vertex)/(sizeof(D3DTLVERTEX)));
    exec_data.dwInstructionOffset = 0;
    exec_data.dwInstructionLength = (unsigned long) (exec->inst - exec->start);
    ddrval = wdd->d3d->lpD3DExecBuffer->lpVtbl->SetExecuteData(
             wdd->d3d->lpD3DExecBuffer,&exec_data);
    ddrval = wdd->d3d->lpD3DDevice->lpVtbl->Execute(
             wdd->d3d->lpD3DDevice, wdd->d3d->lpD3DExecBuffer,
             wdd->d3d->lpD3DViewport,D3DEXECUTE_UNCLIPPED);
    if (ddrval != DD_OK) {
        wdd_FailMsg("win3d.class/w3d_poly/w3d_EndRender","IDirect3DExecuteBuffer:Execute",ddrval);
    };

    /*** Pointer invalidieren ***/
    exec->start  = NULL;
    exec->end    = NULL;
    exec->inst   = NULL;
    exec->vertex = NULL;
    exec->last_triangle = NULL;
}

/*-----------------------------------------------------------------*/
void w3d_CheckFlushBuffer(struct windd_data *wdd,
                          struct win3d_data *w3d,
                          unsigned char *ptr)
/*
**  FUNCTION
**      Testet, ob der neue Pointer enweder eine Kollision
**      mit dem Instruction-Block (wächst von unten nach
**      oben) oder dem Vertex-Block (wächst von oben nach unten)
**      verursacht. Wenn dem so ist, wird der Buffer
**      geflusht und neu initialisiert.
**
**  CHANGED
**      12-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    if ((ptr > exec->vertex) || (ptr < exec->inst)) {
        /*** Puffer muß geflusht werden! ***/
        w3d_EndRender(wdd,w3d);
        w3d_BeginRender(wdd,w3d);
    };
}

/*-----------------------------------------------------------------*/
unsigned char *w3d_BeginTriangleList(struct windd_data *wdd,
                                     struct win3d_data *w3d,
                                     unsigned long cnt)
/*
**  FUNCTION
**      Schreibt eine Triangle-Instruktion in den Renderpuffer.
**
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    unsigned char *end_ptr = exec->inst +
                             3*sizeof(D3DINSTRUCTION) +
                             cnt*sizeof(D3DTRIANGLE);
    w3d_CheckFlushBuffer(wdd,w3d,end_ptr);

    /*** Triangle Data QWORD aligned... ***/
    if (QWORD_ALIGNED(exec->inst)) {
        OP_NOP(exec->inst);
    };
    exec->last_triangle = exec->inst;
    OP_TRIANGLE_LIST(cnt,exec->inst);
    return(exec->inst);
}

/*-----------------------------------------------------------------*/
void w3d_EndTriangleList(struct windd_data *wdd,
                         struct win3d_data *w3d,
                         unsigned char *ptr)
/*
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    exec->inst = ptr;
}

/*-----------------------------------------------------------------*/
D3DTLVERTEX *w3d_GetVertexPointer(struct windd_data *wdd,
                                  struct win3d_data *w3d,
                                  unsigned long num_v)
/*
**  FUNCTION
**      Schafft im ExecuteBuffer Platz für <num_v>
**      D3DTLVERTEX Strukturen. Tritt eine Kollision
**      mit dem Instruktions-Buffer auf, wird der
**      Puffer vorher geflusht (Dabei wird eine
**      256 Byte große Bufferzone mit einberechnet.
**      Returniert wird ein Pointer auf die Vertex-Data.
**
**
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    w3d_CheckFlushBuffer(wdd,w3d,exec->vertex-num_v*sizeof(D3DTLVERTEX)-256);
    exec->vertex -= num_v*sizeof(D3DTLVERTEX);
    return(exec->vertex);
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

    /*** Initial-Zustand herstellen ***/
    ddrval = wdd->d3d->lpD3DDevice->lpVtbl->BeginScene(wdd->d3d->lpD3DDevice);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("win3d.class/w3d_poly.c/w3d_InitPolyEngine()","IDirect3DDevice::BeginScene()",ddrval);
        w3d_KillPolyEngine(wdd,w3d);
        return(FALSE);
    };

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

    if (w3d_BeginRender(wdd,w3d)) {

        unsigned char *inst;
        inst = w3d_BeginRenderState(wdd,w3d,22);
            STATE_DATA(D3DRENDERSTATE_ANTIALIAS,w3d->antialias,inst);
            STATE_DATA(D3DRENDERSTATE_TEXTUREADDRESS,D3DTADDRESS_CLAMP,inst);
            STATE_DATA(D3DRENDERSTATE_ZENABLE,TRUE,inst);
            STATE_DATA(D3DRENDERSTATE_FILLMODE,D3DFILL_SOLID,inst);
            STATE_DATA(D3DRENDERSTATE_SHADEMODE,D3DSHADE_GOURAUD,inst);
            STATE_DATA(D3DRENDERSTATE_MONOENABLE,FALSE,inst);
            STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE,TRUE,inst);
            STATE_DATA(D3DRENDERSTATE_ALPHATESTENABLE,FALSE,inst);
            STATE_DATA(D3DRENDERSTATE_TEXTUREMAG,(w3d->filter?D3DFILTER_LINEAR:D3DFILTER_NEAREST),inst);
            STATE_DATA(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE,inst);
            STATE_DATA(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ZERO,inst);
            STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATE,inst);
            STATE_DATA(D3DRENDERSTATE_CULLMODE,D3DCULL_NONE,inst);
            STATE_DATA(D3DRENDERSTATE_ZFUNC,D3DCMP_LESSEQUAL,inst);
            STATE_DATA(D3DRENDERSTATE_DITHERENABLE,w3d->dither,inst);
            STATE_DATA(D3DRENDERSTATE_BLENDENABLE,FALSE,inst);
            STATE_DATA(D3DRENDERSTATE_FOGENABLE,FALSE,inst);
            STATE_DATA(D3DRENDERSTATE_FOGTABLEDENSITY,(D3DVALUE)0.5,inst);
            STATE_DATA(D3DRENDERSTATE_SPECULARENABLE,FALSE,inst);
            STATE_DATA(D3DRENDERSTATE_SUBPIXEL,TRUE,inst);
            STATE_DATA(D3DRENDERSTATE_STIPPLEDALPHA,wdd_Data.Driver.CanDoStipple,inst);
            STATE_DATA(D3DRENDERSTATE_STIPPLEENABLE,FALSE,inst);

        w3d_EndRenderState(wdd,w3d,inst);
        w3d_EndRender(wdd,w3d);
    };
    ddrval = wdd->d3d->lpD3DDevice->lpVtbl->EndScene(wdd->d3d->lpD3DDevice);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("win3d.class/w3d_poly.c/w3d_InitPolyEngine()","IDirect3DDevice::EndScene()",ddrval);
        w3d_KillPolyEngine(wdd,w3d);
        return(FALSE);
    };

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
**      03-Mar-98   floh    + Transparenz für SrcAlpha/InvSrcAlpha-Only-
**                            Karten gefixt
*/
{
    long xmin,xmax,ymin,ymax;
    unsigned long i,vx_offset;
    unsigned char *inst;
    float xsize,ysize;
    LPD3DTEXTUREHANDLE t_handle;
    LPD3DTLVERTEX v;
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

    /*** Pointer auf Vertex-Data besorgen ***/
    v = w3d_GetVertexPointer(wdd,w3d,p->pnum);
    vx_offset = (exec->vertex - exec->start)/sizeof(D3DTLVERTEX);

    /*** Eckpunkte konvertieren, MiniMax-Ermittlung ***/
    xmin = xmax = 0;
    ymin = ymax = 0;
    for (i=0; i<p->pnum; i++) {
        v[i].sx       = D3DVAL((p->xyz[i*3+0] + 1.0) * w3d->p->x_scale);
        v[i].sy       = D3DVAL((p->xyz[i*3+1] + 1.0) * w3d->p->y_scale);
        v[i].sz       = D3DVAL((p->xyz[i*3+2] / 8192.0));
        v[i].rhw      = D3DVAL(1.0 / p->xyz[i*3+2]);
        v[i].color    = RGBA_MAKE(255,255,255,255);
        v[i].specular = RGB_MAKE(255,0,0);
        v[i].tu       = D3DVAL(0.0);
        v[i].tv       = D3DVAL(0.0);
        if (v[i].sx < v[xmin].sx)      xmin=i;
        else if (v[i].sx > v[xmax].sx) xmax=i;
        if (v[i].sy < v[ymin].sy)      ymin=i;
        else if (v[i].sy > v[ymax].sy) ymax=i;
    };

    /*** Poly immer noch gültig? ***/
    if ((xsize = v[xmax].sx - v[xmin].sx) <= 0) return;
    if ((ysize = v[ymax].sy - v[ymin].sy) <= 0) return;

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
            v[i].tu = D3DVAL(p->uv[i*2+0]);
            v[i].tv = D3DVAL(p->uv[i*2+1]);
        };

    } else if (p->flags & W3DF_RPOLY_PERSPMAP) {

        /*** RenderState-Mods für Perspektiv-Mapping ***/
        exec->cur_state[W3DSTATE_TEXTUREHANDLE].status      = t_handle;
        exec->cur_state[W3DSTATE_TEXTUREPERSPECTIVE].status = TRUE;

        /*** [u,v] Channel füllen ***/
        for (i=0; i<p->pnum; i++) {
            v[i].tu = p->uv[i*2+0];
            v[i].tv = p->uv[i*2+1];
        };
    } else {
        /*** schwarz und flat ***/
        for (i=0; i<p->pnum; i++) {
            v[i].color = RGBA_MAKE(0,0,0,255);
        };
    };

    /*** Shading ***/
    if (p->flags & (W3DF_RPOLY_FLATSHADE|W3DF_RPOLY_GRADSHADE)) {
        /*** Renderstate-Mods für Gourauld-Shading ***/
        exec->cur_state[W3DSTATE_SHADEMODE].status       = D3DSHADE_GOURAUD;
        exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATE;
        /*** [rgba] Channel mit Helligkeit füllen ***/
        for (i=0; i<p->pnum; i++) {
            /*** Kontrast Hack ***/
            long b = (long) ((1.0 - p->b[i])*255.0);
            v[i].color = RGBA_MAKE(b,b,b,255);
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
            v[i].color &= 0x00ffffff;
            v[i].color |= ((w3d->alpha)<<24);
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

    /*** Paletten-Effekte reinrechnen ***/
    if ((w3d->p->pal_r<1.0)||(w3d->p->pal_g<1.0)||(w3d->p->pal_b<1.0)) {
        /*** falls noch TextureBlendCopy aktiviert, auf Modulate schalten ***/
        if (exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status == D3DTBLEND_COPY) {
            exec->cur_state[W3DSTATE_TEXTUREMAPBLEND].status = D3DTBLEND_MODULATE;
        };
        /*** Eckpunkt-Farben modifizieren ***/
        for (i=0; i<p->pnum; i++) {
            unsigned long r = RGBA_GETRED(v[i].color)   * w3d->p->pal_r;
            unsigned long g = RGBA_GETGREEN(v[i].color) * w3d->p->pal_g;
            unsigned long b = RGBA_GETBLUE(v[i].color)  * w3d->p->pal_b;
            unsigned long c = RGB_MAKE(r,g,b);
            v[i].color = (v[i].color & 0xff000000)|c;
        };
    };

    /*** RenderState-Instruktionen schreiben ***/
    if ((!w3d_RenderStateChanged(wdd,w3d,FALSE))&&(exec->last_triangle!=NULL)) {
        /*** Renderstate hat sich nicht verändert, letzte ***/
        /*** Triangle-Liste erweitern.                    ***/
        exec->last_triangle->wCount += p->pnum-2;
        inst = exec->inst;
        for (i=1; i<(p->pnum-1); i++) {
            ((LPD3DTRIANGLE)inst)->v1 = vx_offset;
            ((LPD3DTRIANGLE)inst)->v2 = vx_offset+i;
            ((LPD3DTRIANGLE)inst)->v3 = vx_offset+i+1;
            if (i==1) ((LPD3DTRIANGLE)inst)->wFlags = D3DTRIFLAG_STARTFLAT(p->pnum-3);
            else      ((LPD3DTRIANGLE)inst)->wFlags = D3DTRIFLAG_EVEN;
            inst += sizeof(D3DTRIANGLE);
        };
        exec->inst = inst;
    } else {
        /*** neue Triangle-Liste muß angefangen werden ***/
        inst = w3d_BeginTriangleList(wdd,w3d,p->pnum-2);
            for (i=1; i<(p->pnum-1); i++) {
                ((LPD3DTRIANGLE)inst)->v1 = vx_offset;
                ((LPD3DTRIANGLE)inst)->v2 = vx_offset+i;
                ((LPD3DTRIANGLE)inst)->v3 = vx_offset+i+1;
                if (i==1) ((LPD3DTRIANGLE)inst)->wFlags = D3DTRIFLAG_STARTFLAT(p->pnum-3);
                else      ((LPD3DTRIANGLE)inst)->wFlags = D3DTRIFLAG_EVEN;
                inst += sizeof(D3DTRIANGLE);
            };
        w3d_EndTriangleList(wdd,w3d,inst);
    };

    /*** Ende ***/
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
**                          wird, muß der ExecuteBuffer geflusht werden.
*/
{
    if (w3d->p->num_solid > 0) {

        unsigned long i;
        void *act_txt = NULL;

        /*** sortiere Delayed-Array nach Texturen ***/
        qsort(&(w3d->p->solid[0]),w3d->p->num_solid,
              sizeof(struct w3d_DelayedPoly),
              w3d_SolidCmpFunc);

        /*** Polys mit identischer Textur folgen jetzt aufeinander ***/
        for (i=0; i<w3d->p->num_solid; i++) {
            /*** Polygone wegrendern... ***/
            struct w3d_DelayedPoly *dp = &(w3d->p->solid[i]);
            if (dp->bmp_attach->lpTexture != act_txt) {
                /*** wenn sich der Texture-Cache ändert, muß    ***/
                /*** der Execute-Buffer vorher geflusht werden! ***/
                act_txt = dp->bmp_attach->lpTexture;
                w3d_EndRender(wdd,w3d);
                w3d_BeginRender(wdd,w3d);
            };
            w3d_DrawPoly(wdd,w3d,dp->p,dp->bmp_attach,TRUE,FALSE);
        };
        w3d->p->num_solid = 0;
    };
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
            if (dp->bmp_attach->lpTexture != act_txt) {
                /*** wenn sich der Texture-Cache ändert, muß    ***/
                /*** der Execute-Buffer vorher geflusht werden! ***/
                act_txt = dp->bmp_attach->lpTexture;
                w3d_EndRender(wdd,w3d);
                w3d_BeginRender(wdd,w3d);
            };
            w3d_DrawPoly(wdd,w3d,dp->p,dp->bmp_attach,TRUE,TRUE);
        };
        w3d->p->num_tracy = 0;
    };
}

