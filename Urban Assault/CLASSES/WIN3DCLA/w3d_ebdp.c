/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  w3d_ebdp.c -- transparentes Executebuffer/Drawprimitive Handling.
**
**  (C) Copyright 1998 by A.Weissflog
*/
#include "stdlib.h"
#include "string.h"

#define WIN3D_WINBOX
#include "bitmap/win3dclass.h"

/*** importiert aus windd.class ***/
extern LPDIRECTDRAW lpDD;
extern LPDIRECT3D2  lpD3D2;
extern struct wdd_Data wdd_Data;

/*-----------------------------------------------------------------*/
void w3d_StartExecuteBuffer(struct windd_data *wdd,
                            struct win3d_data *w3d)
/*
**  FUNCTION
**      Bereitet Executebuffer zum Fuellen vor. 
**
**  CHANGED
**      11-May-98   floh    created
*/
{
    D3DEXECUTEBUFFERDESC exbuf_desc;
    struct w3d_Execute *exec = &(w3d->p->exec);
    void *inst;
    HRESULT ddrval;

    /*** Executebuffer locken ***/        
    memset(&exbuf_desc,0,sizeof(exbuf_desc));
    exbuf_desc.dwSize = sizeof(exbuf_desc);
    do {
        ddrval = wdd->d3d->lpD3DExecBuffer->lpVtbl->Lock(wdd->d3d->lpD3DExecBuffer,&exbuf_desc);
        if (ddrval == D3DERR_EXECUTE_LOCKED) {
            wdd->d3d->lpD3DExecBuffer->lpVtbl->Unlock(wdd->d3d->lpD3DExecBuffer);
        };
    } while (ddrval != D3D_OK);

    exec->flags         = W3D_EBF_ISLOCKED;
    exec->start         = exbuf_desc.lpData;
    exec->inst_start    = ((char *)exec->start) + (exbuf_desc.dwBufferSize>>1);
    exec->vertex_start  = exec->start;
    exec->end           = ((char *)exec->start) + exbuf_desc.dwBufferSize;
    exec->inst          = exec->inst_start;
    exec->vertex        = exec->vertex_start;
    exec->vertex_index  = 0;
    exec->vertex_count  = 0;

    /*** Prozess-Vertices ist 1.Instruktion (wird von EndExecuteBuffer() modifiziert) ***/
    OP_PROCESS_VERTICES(1,exec->inst);
    PROCESSVERTICES_DATA(D3DPROCESSVERTICES_COPY,0,0,exec->inst);
}

/*-----------------------------------------------------------------*/
void w3d_EndExecuteBuffer(struct windd_data *wdd,
                          struct win3d_data *w3d)
/*
**  FUNCTION
**      Exekutiert ExecuteBuffer.
**
**  CHANGED
**      11-May-98   floh    created
*/
{
    HRESULT ddrval;
    LPVOID tmp_inst;    
    D3DEXECUTEDATA ed;    
    struct w3d_Execute *exec = &(w3d->p->exec);
    
    /*** Executebuffer terminieren... ***/
    OP_EXIT(exec->inst);
    tmp_inst = exec->inst_start;
    OP_PROCESS_VERTICES(1,tmp_inst);
    PROCESSVERTICES_DATA(D3DPROCESSVERTICES_COPY,0,exec->vertex_count,tmp_inst);
    
    /*** ...unlocken... ***/
    ddrval = wdd->d3d->lpD3DExecBuffer->lpVtbl->Unlock(wdd->d3d->lpD3DExecBuffer);
    if (ddrval != D3D_OK) {
        wdd_FailMsg("win3d.class","IDirect3DExecuteBuffer::Unlock() failed",ddrval);
        return;
    };
    
    /*** ...und ausfuehren... ***/
    memset(&ed,0,sizeof(ed));
    ed.dwSize = sizeof(ed);
    ed.dwVertexOffset      = 0;
    ed.dwVertexCount       = exec->vertex_count;
    ed.dwInstructionOffset = ((DWORD)exec->inst_start) - ((DWORD)exec->start);
    ed.dwInstructionLength = ((DWORD)exec->inst) - ((DWORD)exec->inst_start);
    ddrval = wdd->d3d->lpD3DExecBuffer->lpVtbl->SetExecuteData(
                wdd->d3d->lpD3DExecBuffer,&ed);
    if (ddrval != DD_OK) {
        wdd_FailMsg("win3d.class","IDirect3DExecuteBuffer::SetExecuteData() failed",ddrval);
    } else {
        ddrval = wdd->d3d->lpD3DDevice->lpVtbl->Execute(
                    wdd->d3d->lpD3DDevice,
                    wdd->d3d->lpD3DExecBuffer,
                    wdd->d3d->lpD3DViewport,
                    D3DEXECUTE_UNCLIPPED);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","IDirect3DDevice::Execute() failed",ddrval);
        };
    };
    
    /*** Pointer invalidieren ***/
    exec->flags  = 0;
    exec->start  = NULL;
    exec->inst_start    = NULL;
    exec->vertex_start  = NULL;
    exec->end    = NULL;
    exec->inst   = NULL;
    exec->vertex = NULL;
    exec->vertex_index  = 0;
    exec->vertex_count  = 0;
}

/*-----------------------------------------------------------------*/
void w3d_CheckEBOverflow(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Macht einen Test auf Pufferueberlauf und fuehrt den
**      Puffer bei Bedarf aus...
**
**  CHANGED
**      11-May-98   floh    created
*/
{
    struct w3d_Execute *exec = &(w3d->p->exec);
    if (((((DWORD)exec->inst)+1000) > ((DWORD)exec->end)) ||
        ((((DWORD)exec->vertex)+1000) > ((DWORD)exec->inst_start)))
    {     
        w3d_EndExecuteBuffer(wdd,w3d);
        w3d_StartExecuteBuffer(wdd,w3d);
    };
}

/*-----------------------------------------------------------------*/
void w3d_BeginScene(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Kapselt BeginScene-Calls zum Render-Device. Muß
**      1x pro Frame in DISPM_Begin aufgerufen werden.
**
**  CHANGED
**      11-Mar-98   floh    created
*/
{

    HRESULT ddrval;
    if (wdd->usedrawprimitive) {
        ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->BeginScene(wdd->d3d->lpD3DDevice2);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","D3DDevice2::BeginScene() failed",ddrval);
        }; 
    } else {
        /*** BeginScene auf das D3DDevice ***/
        ddrval = wdd->d3d->lpD3DDevice->lpVtbl->BeginScene(wdd->d3d->lpD3DDevice);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","D3DDevice::BeginScene() failed",ddrval);
        }; 
        w3d_StartExecuteBuffer(wdd,w3d);        
    };
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
    if (wdd->usedrawprimitive) {
        ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->EndScene(wdd->d3d->lpD3DDevice2);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","D3DDevice2::EndScene() failed",ddrval);
        }; 
    } else {
        w3d_EndExecuteBuffer(wdd,w3d);
        ddrval = wdd->d3d->lpD3DDevice->lpVtbl->EndScene(wdd->d3d->lpD3DDevice);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","D3DDevice::EndScene() failed",ddrval);
        };
    }; 
}

/*-----------------------------------------------------------------*/
void w3d_BeginRenderStates(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      w3d_RenderState() Aufrufe muessen in ein w3d_BeginRenderState(),
**      w3d_EndRenderState() gekapselt sein.
**
**  CHANGED
**      11-May-98   floh    created
*/
{ 
    if (!wdd->usedrawprimitive) {
        struct w3d_Execute *exec = &(w3d->p->exec);
        w3d_CheckEBOverflow(wdd,w3d);
        exec->rs_start = exec->inst;
        exec->rs_count = 0;
        OP_STATE_RENDER(exec->rs_count,exec->inst);
    };
}

/*-----------------------------------------------------------------*/
void w3d_EndRenderStates(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      11-May-98   floh    created   
*/
{
    if (!wdd->usedrawprimitive) {
        struct w3d_Execute *exec = &(w3d->p->exec);
        void *tmp_inst = exec->rs_start;
        OP_STATE_RENDER(exec->rs_count,tmp_inst);
        exec->rs_start = NULL;
        exec->rs_count = 0;
    };
}

/*-----------------------------------------------------------------*/
void w3d_RenderState(struct windd_data *wdd, 
                     struct win3d_data *w3d,
                     D3DRENDERSTATETYPE r_type,
                     DWORD r_state) 
/*
**  FUNCTION
**      Setzt einen RenderState entweder in den Executebuffer
**      oder in per Drawprimitive.
**
**  CHANGED
**      10-May-98   floh    created
*/
{
    HRESULT ddrval;
    if (wdd->usedrawprimitive) {
        ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->SetRenderState(wdd->d3d->lpD3DDevice2,r_type,r_state);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","SetRenderState() failed",ddrval);
        };
    } else {
        /*** Execute Buffer Handling ***/
        struct w3d_Execute *exec = &(w3d->p->exec);
        exec->flags &= ~W3D_EBF_NOSTATECHANGE;  // Renderstates wurden geaendert
        exec->rs_count++;
        STATE_DATA(r_type,r_state,exec->inst);
    };
}

/*-----------------------------------------------------------------*/
void w3d_Primitive(struct windd_data *wdd, struct win3d_data *w3d,
                   LPVOID v_array, DWORD v_num)  
/*
**  CHANGED
**      11-May-98   floh    created
*/
{
    HRESULT ddrval;
    if (wdd->usedrawprimitive) {
        /*** DrawPrimitive Handling (ist so schoen einfach) ***/
        ddrval = wdd->d3d->lpD3DDevice2->lpVtbl->DrawPrimitive(
                    wdd->d3d->lpD3DDevice2, 
                    D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX,
                    v_array, v_num, 
                    D3DDP_DONOTCLIP|D3DDP_DONOTUPDATEEXTENTS);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","DrawPrimitive() failed",ddrval);
        }; 
    } else {
        /*** Execute Buffer Handling ***/
        struct w3d_Execute *exec = &(w3d->p->exec);
        LONG num_tris = v_num-2;
        ULONG i;
        LPD3DTLVERTEX v;
        
        /*** potentiellen Execute Buffer Ueberlauf abhandeln ***/        
        w3d_CheckEBOverflow(wdd,w3d);        
        
        if (exec->flags & W3D_EBF_NOSTATECHANGE) {
            /*** Renderstates sind identisch, vorherige Triangleliste erweitern ***/
            void *tmp_inst  = exec->tri_start;
            LPD3DTRIANGLE tri;
            exec->tri_count += num_tris;
            OP_TRIANGLE_LIST(exec->tri_count,tmp_inst);
            tri = exec->inst;
            for (i=1; i<(v_num-1); i++) {
                tri->v1 = exec->vertex_index;
                tri->v2 = exec->vertex_index+i;
                tri->v3 = exec->vertex_index+i+1;
                if (i==1) tri->wFlags = D3DTRIFLAG_STARTFLAT(v_num-3);
                else      tri->wFlags = D3DTRIFLAG_EVEN;
                tri++;
            };
            exec->inst = tri;
        } else {
            /*** Renderstates wurden geaendert, neue Triangleliste anfangen ***/
            LPD3DTRIANGLE tri;
            exec->tri_start = exec->inst;
            exec->tri_count = num_tris;
            OP_TRIANGLE_LIST(exec->tri_count,exec->inst);
            tri = exec->inst;
            for (i=1; i<(v_num-1); i++) {
                tri->v1 = exec->vertex_index;
                tri->v2 = exec->vertex_index+i;
                tri->v3 = exec->vertex_index+i+1;
                if (i==1) tri->wFlags = D3DTRIFLAG_STARTFLAT(v_num-3);
                else      tri->wFlags = D3DTRIFLAG_EVEN;
                tri++;
            };
            exec->inst = tri;
        };
        
        /*** schreibe Vertices ***/
        exec->vertex_index += v_num;            
        exec->vertex_count += v_num;            
        v = exec->vertex;
        memcpy(v,v_array,v_num*sizeof(D3DTLVERTEX));
        v += v_num;
        exec->vertex = v;        

        exec->flags |= W3D_EBF_NOSTATECHANGE;
    };
}

/*-----------------------------------------------------------------*/
void w3d_FlushPrimitives(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Flusht die Rendering-Pipeline (zum Beispiel weil sich
**      der Zustand des Texturchaches geaendert hat.
**
**  CHANGED
**      11-May-98   floh    created
*/
{
    if (wdd->usedrawprimitive) {
        w3d_RenderState(wdd,w3d,D3DRENDERSTATE_FLUSHBATCH,(DWORD)TRUE); 
    } else {
        w3d_EndExecuteBuffer(wdd,w3d);
        w3d_StartExecuteBuffer(wdd,w3d);
    };
}

