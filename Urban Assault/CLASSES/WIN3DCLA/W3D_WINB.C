/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_winbox.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 15:03:33 $
**  $Locker:  $
**  $Author: floh $
**
**  w3d_winbox.c -- allgemeine Direct3D-Gummizelle
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include "stdlib.h"
#include "string.h"

#define WIN3D_WINBOX
#include "bitmap/win3dclass.h"

/*** importiert aus windd.class ***/
extern LPDIRECTDRAW lpDD;
extern LPDIRECT3D2  lpD3D2;
extern struct wdd_Data wdd_Data;

/*** aus wdd_winb.c ***/
void wdd_Log(char *string,...);
void wdd_FailMsg(char *, char *, unsigned long);
void wdd_CheckLostSurfaces(struct windd_data *wdd);

/*** aus w3d_poly.c ***/
void w3d_BeginScene(struct windd_data *, struct win3d_data *);
void w3d_EndRender(struct windd_data *wdd, struct win3d_data *);

/*** aus w3d_txtcache.c ***/
void w3d_TxtCacheBeginFrame(struct windd_data *,struct win3d_data *);

/*-----------------------------------------------------------------*/
void w3d_KillWin3D(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
*/
{
    if (w3d->p) {
        free(w3d->p);
        w3d->p = NULL;
    };
}

/*-----------------------------------------------------------------*/
unsigned long w3d_InitWin3D(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
**      04-Apr-97   floh    initialisiert die Paletten-Slot-Farben
*/
{
    /*** alloziere Win3D-Datenstruktur ***/
    w3d->p = (struct w3d_Data *) malloc(sizeof(struct w3d_Data));
    if (w3d->p) {

        unsigned long i;
        memset(w3d->p,0,sizeof(struct w3d_Data));
        w3d->p->x_scale = (float) (wdd->back_w>>1);
        w3d->p->y_scale = (float) (wdd->back_h>>1);

        for (i=0; i<W3D_PAL_NUMSLOTS; i++) {
            unsigned long rgb = 0;
            float r,g,b;
            switch (i) {
                case 0: rgb=W3D_PAL_SLOT0; break;
                case 1: rgb=W3D_PAL_SLOT1; break;
                case 2: rgb=W3D_PAL_SLOT2; break;
                case 3: rgb=W3D_PAL_SLOT3; break;
                case 4: rgb=W3D_PAL_SLOT4; break;
                case 5: rgb=W3D_PAL_SLOT5; break;
                case 6: rgb=W3D_PAL_SLOT6; break;
                case 7: rgb=W3D_PAL_SLOT7; break;
            };
            r = ((float)((rgb>>16) & 0xff)) / 255.0;
            g = ((float)((rgb>>8)  & 0xff)) / 255.0;
            b = ((float)(rgb       & 0xff)) / 255.0;
            w3d->p->pal_slot[i][0] = r;
            w3d->p->pal_slot[i][1] = g;
            w3d->p->pal_slot[i][2] = b;
        };
        w3d->p->pal_r = 1.0;
        w3d->p->pal_g = 1.0;
        w3d->p->pal_b = 1.0;

    }else{
        w3d_KillWin3D(wdd,w3d);
        return(FALSE);
    };
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void w3d_Begin(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Allgemeine Frame-Begin-Routine...
**
**  CHANGED
**      20-Mar-97   floh    created
*/
{
    /*** Display kann hier bereits ungültig sein... ***/
    ENTERED("w3d_Begin()");
    if (wdd->hWnd) {
        w3d_TxtCacheBeginFrame(wdd,w3d);
        w3d_BeginScene(wdd,w3d);
    };
    LEFT("w3d_Begin()");
}

/*-----------------------------------------------------------------*/
unsigned long w3d_LockBackBuffer(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Lockt BackBuffer für direkten Zugriff, wird aufgerufen
**      aus win3d.class/RASTM_Begin2D.
**      Zurückgegeben wird TRUE/FALSE.
**      Initialisiert wird <wdd->back_ptr> und <wdd->back_pitch>.
**      WICHTIG: alle Routinen, die direkt auf den Surface-Mem
**      zugreifen, müssen den wdd->back_ptr auf Gültigkeit
**      testen.
**
**  CHANGED
**      26-Mar-97   floh    created
**		19-Nov-97	floh	benutzt jetzt testweise das DDLOCK_NOSYSLOCK
**							Flag beim Locken der Backsurface
**      08-Jun-97   floh    + Fehlermeldung, wenn Locken schiefgeht...
**                          + Surfaces werden restored, wenn lost.
*/
{
    ENTERED("w3d_LockBackBuffer()");
    if (wdd->hWnd) {

        DDSURFACEDESC ddsd;
        HRESULT ddrval;

        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddrval = wdd->lpDDSBack->lpVtbl->Lock(wdd->lpDDSBack,NULL,&ddsd,
                                              DDLOCK_NOSYSLOCK|DDLOCK_WAIT,NULL);
        if (ddrval == DD_OK) {
            wdd->back_ptr   = ddsd.lpSurface;
            wdd->back_pitch = ddsd.lPitch;
            LEFT("w3d_LockBackBuffer(TRUE)");
            return(TRUE);
        };
        wdd_FailMsg("w3d_LockBackBuffer()","Lock() failed.",ddrval);
        /*** ab hier Fehler ***/
    };
    wdd->back_ptr = NULL;
    wdd->back_pitch = 0;
    LEFT("w3d_LockBackBuffer(FALSE)");
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void w3d_UnlockBackBuffer(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Unlockt den Backbuffer nach einem w3d_LockBackBuffer.
**      wdd->back_ptr und wdd->back_pitch werden auf NULL
**      gesetzt.
**
**  CHANGED
**      26-Mar-97   floh    created
*/
{
    ENTERED("w3d_UnlockBackBuffer");    
    if (wdd->hWnd) {
        HRESULT ddrval;
        ddrval = wdd->lpDDSBack->lpVtbl->Unlock(wdd->lpDDSBack,NULL);
    };
    LEFT("w3d_UnlockBackBuffer");
}

/*-----------------------------------------------------------------*/
void w3d_MixPalette(struct win3d_data *w3d,
                    unsigned long num,
                    unsigned long *slot,
                    unsigned long *weight)
/*
**  FUNCTION
**      Ermittelt eine resultierende Modifikations-Farbe
**      aus gewichteten Einzel-Farben.
**
**  INPUTS
**      wdd
**      w3d
**      num     - Anzahl Einträge in den <slot> und <weight>
**      slot    - zeigt auf Array mit den zu vermischenden
**                Colorslot-Nummern
**      weight  - zeigt auf Array mit den Wichtungen der
**                einzelnen Colorslots
**
**  CHANGED
**      04-Apr-97   floh    created
*/
{
    unsigned long i;
    float r = 0.0;
    float g = 0.0;
    float b = 0.0;
    for (i=0; i<num; i++) {
        float mix_r = w3d->p->pal_slot[slot[i]][0];
        float mix_g = w3d->p->pal_slot[slot[i]][1];
        float mix_b = w3d->p->pal_slot[slot[i]][2];
        float mix_w = ((float)weight[i])/255.0;
        r+=mix_r*mix_w;
        g+=mix_g*mix_w;
        b+=mix_b*mix_w;
    };
    if (r>1.0) r=1.0;
    if (g>1.0) g=1.0;
    if (b>1.0) b=1.0;
    w3d->p->pal_r = r;
    w3d->p->pal_g = g;
    w3d->p->pal_b = b;
}

/*-----------------------------------------------------------------*/
void w3d_FrameSnap(struct windd_data *wdd, struct win3d_data *w3d, void *fp)
/*
**  FUNCTION
**      Macht einen Screenshot des aktuellen Frames und
**      schreibt das Ergebnis als PPM-File in den offenen
**      ANSI-kompatiblen "wb" File-Stream.
**
**  CHANGED
**      18-Jun-97   floh    created
*/
{
    if (wdd->back_ptr) {

        unsigned char head[128];
        unsigned long x,y;
        unsigned long w,h;
        struct w3d_PixelFormat *pfmt = &(w3d->p->disp_pfmt);
        unsigned long pix_size = pfmt->byte_size;
        unsigned long pitch    = wdd->back_pitch / pix_size;

        w = wdd->back_w;
        h = wdd->back_h;
        sprintf(head,"P6\n#YPA screenshot\n%d %d\n255\n",w,h);

        /*** Schreibe Header ***/
        fwrite(head,strlen(head),1,fp);

        if (pix_size == 2) {
            for (y=0; y<h; y++) {
                for (x=0; x<w; x++) {

                    unsigned short *pix_ptr;
                    unsigned short pix;
                    unsigned short r,g,b;

                    /*** hole aktuelles Pixel ***/
                    pix_ptr = ((unsigned short *)wdd->back_ptr) + y*pitch + x;
                    pix = *pix_ptr;

                    /*** in RGB aufsplitten ***/
                    r = (pix & pfmt->r_mask);
                    if (pfmt->r_shift > 0) r >>= pfmt->r_shift;
                    else                   r <<= -pfmt->r_shift;
                    g = (pix & pfmt->g_mask);
                    if (pfmt->g_shift > 0) g >>= pfmt->g_shift;
                    else                   g <<= -pfmt->g_shift;
                    b = (pix & pfmt->b_mask);
                    if (pfmt->b_shift > 0) b >>= pfmt->b_shift;
                    else                   b <<= -pfmt->b_shift;

                    /*** schreibe r,g,b nach Filestream ***/
                    fputc((unsigned char)r,fp);
                    fputc((unsigned char)g,fp);
                    fputc((unsigned char)b,fp);
                };
            };
        };
        /*** FIXME: 32-Bit-Framebuffers nicht supported ***/
    };
}

