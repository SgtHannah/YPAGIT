/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_txtcache.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 15:03:21 $
**  $Locker: floh $
**  $Author: floh $
**
**  w3d_txtcache.c -- Textur-Cache-Handling (Windows-Gummizelle).
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
extern void wdd_FailMsg(char *title, char *msg, unsigned long code);

/*** aus w3d_pixfmt.c ***/
unsigned long w3d_ColorConvert(unsigned long r, unsigned long g, unsigned long b, unsigned long a,
                               long r_shift, long g_shift, long b_shift, long a_shift,
                               long r_mask, long g_mask, long b_mask, long a_mask);

/*-----------------------------------------------------------------*/
void w3d_KillTxtCache(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
*/
{
    if (w3d->p) {
        long i;
        for (i=0; i<W3D_MAXNUM_TXTSLOTS; i++) {
            struct w3d_TxtSlot *act_slot = &(w3d->p->slot[i]);

            if (act_slot->lpTxtPalette) {
                act_slot->lpTxtPalette->lpVtbl->Release(act_slot->lpTxtPalette);
                act_slot->lpTxtPalette = NULL;
            };
            if (act_slot->lpTexture) {
                act_slot->lpTexture->lpVtbl->Release(act_slot->lpTexture);
                act_slot->lpTexture = NULL;
            };                
            if (act_slot->lpTexture2) {
                act_slot->lpTexture2->lpVtbl->Release(act_slot->lpTexture2);
                act_slot->lpTexture2   = NULL;
            };
            if (act_slot->lpSurface) {
                act_slot->lpSurface->lpVtbl->Release(act_slot->lpSurface);
                act_slot->lpSurface = NULL;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
unsigned long w3d_InitTxtCache(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
**      21-Aug-97   floh    + falls Sysmemtexturen akzeptiert werden,
**                            wird der Texturcache-Manager überhaupt nicht
**                            initialisiert
**      21-Apr-98   floh    + Texturhandle wird jetzt nicht mehr bereits
**                            bei der Initialisierung geholt, erst nachdem
**                            die Textur tatsaechlich geladen wurde
**                            (in ValidateTexture())
*/
{
    HRESULT ddrval;
    unsigned long done = FALSE;

    w3d->p->num_slots = 0;
    memset(&(w3d->p->slot),0,sizeof(w3d->p->slot));
    while ((!done) && (w3d->p->num_slots < W3D_MAXNUM_TXTSLOTS)) {

        struct w3d_TxtSlot *act_slot = &(w3d->p->slot[w3d->p->num_slots]);
        DDSURFACEDESC ddsd;
        DWORD pcaps;
        struct wdd_TxtFormat *tf = &(wdd->d3d->TxtFormats[wdd->d3d->ActTxtFormat]);

        /*** allgemeine Slot-Init ***/
        act_slot->flags = W3DF_TSLOT_FLUSHME;

        /*** allokiere eine leere Textur-Surface nach der anderen, ***/
        /*** bis kein Platz mehr ist, oder Maximal-Anzahl erreicht ***/
        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize  = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.dwWidth  = 256;
        ddsd.dwHeight = 256;
        ddsd.ddpfPixelFormat = tf->ddsd.ddpfPixelFormat;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD;
        ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(act_slot->lpSurface),NULL);
        if (ddrval == DDERR_OUTOFVIDEOMEMORY) {
            done=TRUE;
            continue;
        }else if (ddrval != DD_OK){
            wdd_FailMsg("win3d.class/w3d_txtcache.c/InitTxtCache()","IDirectDraw::CreateSurface()",ddrval);
            w3d_KillTxtCache(wdd,w3d);
            return(FALSE);
        };

        /*** Palette attachen? ***/
        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            ULONG i;
            PALETTEENTRY ppe[256];
            pcaps = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
            // memset(ppe,0,sizeof(ppe));
            for (i=0; i<255; i++) {
                ppe[i].peRed = i;
                ppe[i].peGreen = i;
                ppe[i].peBlue = i;
            };
            ddrval = lpDD->lpVtbl->CreatePalette(lpDD,pcaps,ppe,&(act_slot->lpTxtPalette),NULL);
            if (ddrval != DD_OK) {
                wdd_FailMsg("win3d.class/w3d_txtcache.c/InitTxtCache()","IDirectDraw::CreatePalette()",ddrval);
                w3d_KillTxtCache(wdd,w3d);
                return(FALSE);
            };
            ddrval = act_slot->lpSurface->lpVtbl->SetPalette(
                     act_slot->lpSurface,act_slot->lpTxtPalette);
            if (ddrval != DD_OK) {
                wdd_FailMsg("win3d.class/w3d_txtcache.c","IDirectDrawSurface::SetPalette()",ddrval);
                w3d_KillTxtCache(wdd,w3d);
                return(FALSE);
            };
        };

        /*** hole altes und neues Textur-Interface der Surface ***/
        ddrval = act_slot->lpSurface->lpVtbl->QueryInterface(
                 act_slot->lpSurface,&IID_IDirect3DTexture2,
                 &(act_slot->lpTexture2));
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","QueryInterface(IID_IDirect3DTextur2) failed.",ddrval);
            w3d_KillTxtCache(wdd,w3d);
            return(FALSE);
        };
        ddrval = act_slot->lpSurface->lpVtbl->QueryInterface(
                 act_slot->lpSurface,&IID_IDirect3DTexture,
                 &(act_slot->lpTexture));
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class","QueryInterface(IID_IDIrect3DTexture) failed.",ddrval);
            w3d_KillTxtCache(wdd,w3d);
            return(FALSE);
        };             

        /*** nächster Slot ***/
        w3d->p->num_slots++;
    };

    /*** alles in Lot aufm Boot ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long w3d_ObtainTxtBlt(struct windd_data *wdd,
                               struct win3d_data *w3d,
                               unsigned long w,
                               unsigned long h,
                               void **data)
/*
**  FUNCTION
**      Alloziert Platz für eine CPU Blittable Textur. Dieser
**      Typ besitzt keine eigene DD-Surface, sondern ist einfach
**      ein Bitmap-Block im aktuellen Display-Pixel-Format.
**      Diese Bitmaps werden für die raster.class Text- und CPU-Blt-
**      Operationen in der win3d.class benötigt.
**
**  INPUTS
**      wdd
**      w3d
**      w,h     - Breite und Höhe in PIXEL
**      data    - hierhin wird der Pointer auf den alloizierten
**                Bitmap-Block geschrieben
**
**  RESULTS
**      BytesPerRow, oder NULL, wenn Fehler.
**
**  CHANGED
**      25-Mar-97   floh    created
*/
{
    void *bmp_data;

    /*** alloziere Bitmap-Body ***/
    *data = NULL;
    bmp_data = malloc(w*h*w3d->p->disp_pfmt.byte_size);
    if (bmp_data) {
        *data = bmp_data;
        return(w*w3d->p->disp_pfmt.byte_size);
    } else return(0);
}

/*-----------------------------------------------------------------*/
unsigned long w3d_ObtainTexture(struct windd_data *wdd,
                                struct win3d_data *w3d,
                                unsigned long w,
                                unsigned long h,
                                struct w3d_BmpAttach **attach)
/*
**  FUNCTION
**      Backend von RASTM_ObtainTexture. Es wird eine Surface
**      und evtl. eine Palette erzeugt
**
**  CHANGED
**      11-Mar-97   floh    created
**      21-Aug-97   floh    + TextureHandle wird ermittelt und zurückgegeben
*/
{
    HRESULT ddrval;
    struct w3d_BmpAttach *p;

    *attach = NULL;

    /*** allokiere Attachment ***/
    p = (struct w3d_BmpAttach *) malloc(sizeof(struct w3d_BmpAttach));
    if (p) {

        /*** Textur kommt in eine Surface rein ***/
        struct wdd_TxtFormat *tfmt = &(wdd->d3d->TxtFormats[wdd->d3d->ActTxtFormat]);
        DDSURFACEDESC ddsd;

        memset(p,0,sizeof(struct w3d_BmpAttach));
        *attach = p;

        /*** Surface erzeugen ***/
        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize   = sizeof(ddsd);
        ddsd.dwFlags  = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
        ddsd.dwWidth  = w;
        ddsd.dwHeight = h;
        ddsd.ddpfPixelFormat = tfmt->ddsd.ddpfPixelFormat;
        ddsd.ddsCaps.dwCaps  = DDSCAPS_SYSTEMMEMORY|DDSCAPS_TEXTURE;
        ddrval = lpDD->lpVtbl->CreateSurface(lpDD,&ddsd,&(p->lpSurface),NULL);
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class/w3d_txtcache.c/ObtainTexture()","IDirect3DTexture::CreateSurface(SrcTxt)",ddrval);
            return(FALSE);
        };

        /*** Paletten-Object dazu? ***/
        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            DWORD pcaps;
            PALETTEENTRY ppe[256];
            pcaps = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
            memset(ppe,0,sizeof(ppe));
            ddrval = lpDD->lpVtbl->CreatePalette(lpDD,pcaps,ppe,&(p->lpPalette),NULL);
            if (ddrval != DD_OK) {
                wdd_FailMsg("win3d.class/w3d_txtcache.c/ObtainTexture()","IDirectDraw::CreatePalette(SrcTxt)",ddrval);
                return(FALSE);
            };
            ddrval = p->lpSurface->lpVtbl->SetPalette(p->lpSurface,p->lpPalette);
            if (ddrval != DD_OK) {
                wdd_FailMsg("win3d.class/w3d_txtcache.c/ObtainTexture()","IDirectDrawSurface::SetPalette(SrcTxt)",ddrval);
                return(FALSE);
            };
        };

        /*** hole Textur-Interface der Surface ***/
        ddrval = p->lpSurface->lpVtbl->QueryInterface(
                 p->lpSurface,&IID_IDirect3DTexture2,
                 &(p->lpTexture));
        if (ddrval != DD_OK) {
            wdd_FailMsg("win3d.class/w3d_txtcache.c/ObtainTexture()","IDirectDrawSurface::QueryInterface(SrcTxt)",ddrval);
            return(FALSE);
        };

    } else {
        wdd_FailMsg("win3d.class/w3d_txtcache.c/ObtainTexture()","Out Of Mem",0);
        return(FALSE);
    };

    return(TRUE);
}

/*-----------------------------------------------------------------*/
void w3d_ReleaseTxtBlt(struct windd_data *wdd,
                       struct win3d_data *w3d,
                       void *data)
/*
**  CHANGED
**      25-Mar-97   floh    created
*/
{
    if (data) free(data);
}

/*-----------------------------------------------------------------*/
void w3d_ReleaseTexture(struct windd_data *wdd,
                        struct win3d_data *w3d,
                        struct w3d_BmpAttach *p)
/*
**  CHANGED
**      11-Mar-96   floh    created
*/
{
    if (p->lpPalette) {
        p->lpPalette->lpVtbl->Release(p->lpPalette);
        p->lpPalette = NULL;
    };
    if (p->lpTexture) {
        p->lpTexture->lpVtbl->Release(p->lpTexture);
        p->lpTexture = NULL;
    };
    if (p->lpSurface) {
        p->lpSurface->lpVtbl->Release(p->lpSurface);
        p->lpSurface = NULL;
    };
    free(p);
}

/*-----------------------------------------------------------------*/
void w3d_MangleTxtBlt(struct windd_data *wdd,
                      struct win3d_data *w3d,
                      char *cm,
                      char *glb_cm,
                      unsigned long w,
                      unsigned long h,
                      void *data)
/*
**  FUNCTION
**      Konvertiert 8-Bit-CLUT-BltTxt ins Display-Pixelformat.
**
**  CHANGED
**      25-Mar-97   floh    created
*/
{
    unsigned long i;
    long r_mask  = w3d->p->disp_pfmt.r_mask;
    long r_shift = w3d->p->disp_pfmt.r_shift;
    long g_mask  = w3d->p->disp_pfmt.g_mask;
    long g_shift = w3d->p->disp_pfmt.g_shift;
    long b_mask  = w3d->p->disp_pfmt.b_mask;
    long b_shift = w3d->p->disp_pfmt.b_shift;

    if (w3d->p->disp_pfmt.byte_size == 2) {

        /*** 16-Bit-RGB-Textur-Format: Pixelkonvertierung ***/
        unsigned short r_table[256];
        unsigned char  *src;
        unsigned short *tar;

        /*** Remap-Tabelle aufbauen ***/
        if (cm) {
            /*** lokale Colormap wird bevorzugt ***/
            for (i=0; i<256; i++) {
                unsigned short r = *cm++;
                unsigned short g = *cm++;
                unsigned short b = *cm++;
                r_table[i] = w3d_ColorConvert(r,g,b,0,
                                 r_shift,g_shift,b_shift,0,
                                 r_mask,g_mask,b_mask,0);
            };
        } else {
            /*** ...sonst die globale nehmen ***/
            for (i=0; i<256; i++) {
                unsigned short r = *glb_cm++;
                unsigned short g = *glb_cm++;
                unsigned short b = *glb_cm++;
                r_table[i] = w3d_ColorConvert(r,g,b,0,
                                 r_shift,g_shift,b_shift,0,
                                 r_mask,g_mask,b_mask,0);
            };
        };

        /*** die Pixels selbst konvertieren ***/
        src   = ((unsigned char *)data)  + w*h;
        tar   = ((unsigned short *)data) + w*h;
        while (src > data) *--tar = r_table[*--src];

    }else if (w3d->p->disp_pfmt.byte_size == 4){

        /*** 32 Bit RGB(A) Textur-Format: Pixelkonvertierung ***/
        unsigned long r_table[256];
        unsigned char *src;
        unsigned long *tar;

        /*** Remap-Tabelle aufbauen ***/
        if (cm) {
            /*** lokale Colormap wird bevorzugt ***/
            for (i=0; i<256; i++) {
                unsigned long r = *cm++;
                unsigned long g = *cm++;
                unsigned long b = *cm++;
                r_table[i] = w3d_ColorConvert(r,g,b,0,
                                 r_shift,g_shift,b_shift,0,
                                 r_mask,g_mask,b_mask,0);
            };
        }else{
            /*** ...sonst die globale nehmen ***/
            for (i=0; i<256; i++) {
                unsigned long r = *glb_cm++;
                unsigned long g = *glb_cm++;
                unsigned long b = *glb_cm++;
                r_table[i] = w3d_ColorConvert(r,g,b,0,
                                 r_shift,g_shift,b_shift,0,
                                 r_mask,g_mask,b_mask,0);
            };
        };

        /*** die Pixels selbst konvertieren ***/
        src   = ((unsigned char *)data)  + w*h;
        tar   = ((unsigned short *)data) + w*h;
        while (src > data) *--tar = r_table[*--src];

    }else{
        /*** Textur-Format nicht unterstützt ***/
        wdd_FailMsg("win3d.class/w3d_txtcacje.c/MangleTxtBlt()","Unsupported txt pixformat.",0);
    };
}

/*-----------------------------------------------------------------*/
void w3d_MangleTexture(struct windd_data *wdd,
                       struct win3d_data *w3d,
                       char *cm,
                       char *glb_cm,
                       struct w3d_BmpAttach *p,
                       ULONG alpha_hint)
/*
**  INPUT
**      wdd
**      w3d
**      cm      - optionaler Pointer auf lokale VFM-Colormap
**      glb_cm  - Pointer auf globale VFMColormap (UBYTE[3])
**      p       - Pointer auf attachte w3d_BmpAttach Struktur
**      alpha_hint - TRUE: für jedes Pixel wird ein Alphawert
**                   nach Intensität berechnet
**                   FALSE: Nicht-Colorkey-Pixel werden solide,
**                   Colorkey-Pixel werden voll durchsichtig
**
**  CHANGED
**      11-Mar-97   floh    created
**      22-Mar-97   floh    initialisiert ColorKey für jede Textur-Surface
**      21-Aug-97   floh    + initialisiert Alpha Channel bei RGBA-Texturen
**      04-Mar-98   floh    + alpha_hint Arg
*/
{
    DDSURFACEDESC ddsd;
    HRESULT ddrval;
    unsigned long w,h,pitch,i;
    unsigned char *cmap = cm ? cm:glb_cm;
    long r_mask  = w3d->p->txt_pfmt.r_mask;
    long r_shift = w3d->p->txt_pfmt.r_shift;
    long g_mask  = w3d->p->txt_pfmt.g_mask;
    long g_shift = w3d->p->txt_pfmt.g_shift;
    long b_mask  = w3d->p->txt_pfmt.b_mask;
    long b_shift = w3d->p->txt_pfmt.b_shift;
    long a_mask  = w3d->p->txt_pfmt.a_mask;
    long a_shift = w3d->p->txt_pfmt.a_shift;

    if (w3d->p->txt_pfmt.byte_size == 1) {

        /*** bei 8-Bit-CLUT muß nur die Palette korrekt gesetzt werden ***/
        PALETTEENTRY pal[256];
        for (i=0; i<256; i++) {
            pal[i].peRed   = *cmap++;
            pal[i].peGreen = *cmap++;
            pal[i].peBlue  = *cmap++;
            pal[i].peFlags = 0;
        };

        /*** Palette der Surface setzen ***/
        ddrval = p->lpPalette->lpVtbl->SetEntries(p->lpPalette,0,0,256,pal);

    } else if (w3d->p->txt_pfmt.byte_size == 2) {

        /*** 16-Bit-RGBA-Textur-Format: Remaptabelle basteln ***/
        unsigned short r_table[256];
        unsigned char  *src;
        unsigned short *tar;
        for (i=0; i<256; i++) {
            unsigned short a;
            unsigned short r = *cmap++;
            unsigned short g = *cmap++;
            unsigned short b = *cmap++;
            /*** ist es eine Colorkey-Farbe? ***/
            if ((r==0xff) && (g==0xff) && (b==0x0)) {
                /*** Colorkey-Farbe, voll durchsichtig ***/
                r=0; g=0; b=0; a=0;
            }else if ((!wdd_Data.Driver.CanDoAdditiveBlend) &&
                      (wdd_Data.Driver.CanDoAlpha) &&
                      (alpha_hint))
            {
                /*** wenn Alpha-Page, Intensitäts-Alpha berechnen ***/
                float rf,gf,bf,maxf;
                rf   = (float)r;
                gf   = (float)g;
                bf   = (float)b;
                maxf = (r>g) ? r:g;
                maxf = (maxf>b) ? maxf:b;
                if (maxf > 8.0f) {
                    rf/=maxf; gf/=maxf; bf/=maxf;
                }else{
                    maxf=rf=gf=bf=0.0f;
                };
                r = (unsigned short) (rf*255.0f);
                g = (unsigned short) (gf*255.0f);
                b = (unsigned short) (bf*255.0f);
                a = (unsigned short) (maxf);
            }else{
                a = 255;
            };
            r_table[i] = w3d_ColorConvert(r,g,b,a,
                             r_shift,g_shift,b_shift,a_shift,
                             r_mask,g_mask,b_mask,a_mask);
        };

        /*** Pixel konvertieren ***/
        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddrval = p->lpSurface->lpVtbl->Lock(p->lpSurface,NULL,&ddsd,0,NULL);
        if (ddrval == DD_OK) {
            w     = ddsd.dwWidth;
            h     = ddsd.dwHeight;
            pitch = ddsd.lPitch;        // FIXME: nicht benutzt
            src   = ((unsigned char *)ddsd.lpSurface)  + w*h;
            tar   = ((unsigned short *)ddsd.lpSurface) + w*h;
            while (src > ddsd.lpSurface) *--tar = r_table[*--src];
            ddrval = p->lpSurface->lpVtbl->Unlock(p->lpSurface,ddsd.lpSurface);
        };

    }else if (w3d->p->txt_pfmt.byte_size == 4){

        /*** 32 Bit RGB(A) Textur-Format: Remap Tabelle bauen ***/
        unsigned long r_table[256];
        unsigned char *src;
        unsigned long *tar;
        for (i=0; i<256; i++) {
            unsigned long a;
            unsigned long r = *cmap++;
            unsigned long g = *cmap++;
            unsigned long b = *cmap++;
            /*** ist es eine Colorkey-Farbe? ***/
            if ((r==0xff) && (g==0xff) && (b==0x0)) {
                /*** Colorkey-Farbe, voll durchsichtig ***/
                r=0; g=0; b=0; a=0;
            }else if ((!wdd_Data.Driver.CanDoAdditiveBlend) &&
                      (wdd_Data.Driver.CanDoAlpha) &&
                      (alpha_hint))
            {
                /*** wenn Alpha-Page, Intensitäts-Alpha berechnen ***/
                float rf,gf,bf,maxf;
                rf   = (float)r;
                gf   = (float)g;
                bf   = (float)b;
                maxf = (r>g) ? r:g;
                maxf = (maxf>b) ? maxf:b;
                if (maxf > 8.0f) {
                    rf/=maxf; gf/=maxf; bf/=maxf;
                }else{
                    maxf=rf=gf=bf=0.0f;
                };
                r = (unsigned long) rf*255.0f;
                g = (unsigned long) gf*255.0f;
                b = (unsigned long) bf*255.0f;
                a = (unsigned long) maxf;
            }else{
                a = 255;
            };
            r_table[i] = w3d_ColorConvert(r,g,b,a,
                             r_shift,g_shift,b_shift,a_shift,
                             r_mask,g_mask,b_mask,a_mask);
        };

        /*** die Pixels selbst konvertieren ***/
        memset(&ddsd,0,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddrval = p->lpSurface->lpVtbl->Lock(p->lpSurface,NULL,&ddsd,0,NULL);
        if (ddrval == DD_OK) {
            w     = ddsd.dwWidth;
            h     = ddsd.dwHeight;
            pitch = ddsd.lPitch;    // FIXME: nicht benutzt
            src   = ((unsigned char *)ddsd.lpSurface)  + w*h;
            tar   = ((unsigned short *)ddsd.lpSurface) + w*h;
            while (src > ddsd.lpSurface) *--tar = r_table[*--src];
            ddrval = p->lpSurface->lpVtbl->Unlock(p->lpSurface,ddsd.lpSurface);
        };

    };

    /*** das war alles ***/
}

/*-----------------------------------------------------------------*/
unsigned long w3d_LockTexture(struct windd_data *wdd,
                              struct win3d_data *w3d,
                              void **data,
                              struct w3d_BmpAttach *p)
/*
**  FUNCTION
**      Lockt die attachte Surface und schreibt den lpSurface
**      Pointer nach *data (sollte auf VFMBitmap.Data zeigen).
**
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    DDSURFACEDESC ddsd;
    HRESULT ddrval;

    *data = NULL;
    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddrval = p->lpSurface->lpVtbl->Lock(p->lpSurface,NULL,&ddsd,0,NULL);
    if (ddrval != DD_OK) {
        wdd_FailMsg("win3d.class/w3d_txtcache.c/LockTexture()","IDirectDrawSurface::Lock()",ddrval);
        return(FALSE);
    };
    *data = ddsd.lpSurface;
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void w3d_UnlockTexture(struct windd_data *wdd,
                       struct win3d_data *w3d,
                       void *data,
                       struct w3d_BmpAttach *p)
/*
**  FUNCTION
**      Unlockt die Textur nach einem Lock wieder.
**
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    HRESULT ddrval;
    ddrval = p->lpSurface->lpVtbl->Unlock(p->lpSurface,data);
    if (ddrval != DD_OK) {
        wdd_FailMsg("win3d.class/w3d_txtcache.c/UnlockTexture()","IDirectDrawSurface::Unlock()",ddrval);
    };
}

/*-----------------------------------------------------------------*/
unsigned long w3d_LoadTexture(struct windd_data *wdd,
                              struct win3d_data *w3d,
                              struct w3d_BmpAttach *p,
                              long slot)
/*
**  CHANGED
**      12-Mar-97   floh    created
**      23-Mar-97   floh    + setzt jetzt Colorkey korrekt
**      24-Mar-97   floh    + Colorkeying-Parameter
**      22-Oct-97   floh    + Colorkey wird jetzt VOR dem
**                            Textur-Upload gesetzt
*/
{
    HRESULT ddrval;
    struct w3d_TxtSlot *s = &(w3d->p->slot[slot]);

    /*** alte Textur Unloaden (nur ExecuteBuffer Modell) ***/
    if (!wdd->usedrawprimitive) {
        if (s->lpSource) {
            ddrval = s->lpTexture->lpVtbl->Unload(s->lpTexture);
            if (ddrval != DD_OK) {
                wdd_FailMsg("win3d.class","IDirect3DTexture::Unload()",ddrval);
                return(FALSE);
            };
        };
    };

    /*** Textur-Slot markieren ***/
    s->flags     &= ~W3DF_TSLOT_FLUSHME;
    s->flags     |= W3DF_TSLOT_USED;
    s->cache_hits = 1;
    s->lpSource   = p->lpTexture;
    s->attach     = p;

    /*** ColorKey Handling nur bei 8-Bit-Texturen ***/
    if (w3d->p->txt_pfmt.byte_size == 1) {
        /*** 8-Bit-Texturen: Farbe 0 ***/
        DDCOLORKEY ddck;
        ddck.dwColorSpaceLowValue  = 0;
        ddck.dwColorSpaceHighValue = 0;
        ddrval = s->lpSurface->lpVtbl->SetColorKey(s->lpSurface,DDCKEY_SRCBLT,&ddck);
    };

    /*** Textur downloaden ***/
    ddrval = s->lpTexture2->lpVtbl->Load(s->lpTexture2,s->lpSource);
    if (ddrval != DD_OK) return(FALSE);

    /*** prinzipiell war's das ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void w3d_TxtCacheBeginFrame(struct windd_data *wdd,struct win3d_data *w3d)
/*
**  FUNCTION
**      Löscht das Used-Flag bei allen Texturen im
**      Textur-Cache. Das Used-Flag wird gesetzt,
**      sobald im aktuellen Frame der Textur-Slot benötigt
**      wird (touch).
**
**  CHANGED
**      11-Mar-97   floh    created
**      08-Apr-97   floh    + Lost-Restore
*/
{
    HRESULT ddrval;
    unsigned long i;
    for (i=0; i<w3d->p->num_slots; i++) {
        /*** Lost-Restore-Check ***/
        struct w3d_TxtSlot *slot = &(w3d->p->slot[i]);
        ddrval = slot->lpSurface->lpVtbl->IsLost(slot->lpSurface);
        if (ddrval != DD_OK) {
            ddrval = slot->lpSurface->lpVtbl->Restore(slot->lpSurface);
            if (slot->lpSource) {
                w3d_LoadTexture(wdd,w3d,slot->attach,i);
            };
        };
        slot->flags &= ~W3DF_TSLOT_USED;
    };
}

/*-----------------------------------------------------------------*/
void w3d_TxtCacheEndFrame(struct windd_data *wdd,struct win3d_data *w3d)
/*
**  FUNCTION
**      Alle Texturen im Cache, die in diesem Frame nicht benötigt
**      wurden, werden als FlushMe markiert (zum Überschreiben
**      im nächsten Frame freigegeben).
**
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    unsigned long i;
    for (i=0; i<w3d->p->num_slots; i++) {
        if (!(w3d->p->slot[i].flags & W3DF_TSLOT_USED)) {
            w3d->p->slot[i].flags |= W3DF_TSLOT_FLUSHME;
        };
    };
}

/*-----------------------------------------------------------------*/
D3DTEXTUREHANDLE *w3d_GetTxtHandle(struct windd_data *wdd,
                                   struct win3d_data *w3d,
                                   struct w3d_TxtSlot *s)
/*
**  FUNCTION
**      Returniert das Texturhandle fuer die durch den Texturslot
**      definierte Resource.
**
**  CHANGED
**      21-Apr-98   floh    created
*/
{
    HRESULT ddrval;
    D3DTEXTUREHANDLE txt_handle;
    if (wdd->usedrawprimitive) {
        ddrval = s->lpTexture2->lpVtbl->GetHandle(s->lpTexture2,
                    wdd->d3d->lpD3DDevice2,&txt_handle);
    } else {
        ddrval = s->lpTexture->lpVtbl->GetHandle(s->lpTexture,
                    wdd->d3d->lpD3DDevice,&txt_handle);
    };
    if (ddrval == DD_OK) return(txt_handle);
    else                 return(NULL);
}

/*-----------------------------------------------------------------*/
void *w3d_ValidateTexture(struct windd_data *wdd,
                          struct win3d_data *w3d,
                          struct w3d_BmpAttach *p,
                          unsigned long force)
/*
**  FUNCTION
**      Siehe cgl.class/cgl_cache.c/cgl_ValidateTexture()
**
**  RETURN
**      LPD3DTEXTUREHANDLE oder NULL
**
**  CHANGED
**      12-Mar-97   floh    created
**      21-Aug-97   floh    falls das 3D-Device SysMemTexturen akzeptiert,
**                          tritt der Texturcache außer Funktion.
**      21-Apr-98   floh    + ruft w3d_GetTxtHandle() auf
*/
{
    if (w3d->p->exec.begin_scene_ok) {
        unsigned long i;
        long free = -1;

        for (i=0; i<w3d->p->num_slots; i++) {
            struct w3d_TxtSlot *s = &(w3d->p->slot[i]);
            if (s->lpSource == p->lpTexture) {
                /*** CacheHit: Textur ist bereits im Cache ***/
                s->flags &= ~W3DF_TSLOT_FLUSHME;    // FlushMe löschen
                s->flags |= W3DF_TSLOT_USED;        // Used setzen
                s->cache_hits++;
                return(w3d_GetTxtHandle(wdd,w3d,s));
            } else if (s->flags & W3DF_TSLOT_FLUSHME) {
                /*** vornewech einen leeren Slot merken ***/
                free = i;
            };
        };

        /*** ab hier CacheMiss, war ein Slot geflushet? ***/
        if (free != -1) {
            /*** ok, diesen überschreiben ***/
            if (w3d_LoadTexture(wdd,w3d,p,free)) {
                return(w3d_GetTxtHandle(wdd,w3d,&(w3d->p->slot[free])));
            };
        } else {

            /*** CacheMiss bei vollem Cache ***/
            if (force) {

                /*** überschreibe Slot mit niedrigstem UseCount ***/
                unsigned long min_slot = 0;
                for (i=0; i<w3d->p->num_slots; i++) {
                    if (w3d->p->slot[i].cache_hits<w3d->p->slot[min_slot].cache_hits)
                    {
                        min_slot = i;
                    };
                };

                /*** dieses Slot zwangsflushen ***/
                w3d_FlushPrimitives(wdd,w3d);
                if (w3d_LoadTexture(wdd,w3d,p,min_slot)) {
                    return(w3d_GetTxtHandle(wdd,w3d,&(w3d->p->slot[min_slot])));
                };
            };
        };
    };
    /*** ab hier "Delay Me", oder begin_scene_ok nicht gesetzt... ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
void w3d_TxtCacheBeginSession(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Flusht den gesamten Texturcache, damit keine Kollisionen
**      mit neu geladenen Texturen auftreten.
**      Ausserdem wird dadurch verhindert, dass freigegebene Texturen
**      referenziert werden koennen (konnte zum Absturz fuehren)     
**
**  CHANGED
**      03-Apr-97   floh    created
*/
{
    HRESULT ddrval;
    unsigned long i;
    for (i=0; i<w3d->p->num_slots; i++) {
        struct w3d_TxtSlot *s = &(w3d->p->slot[i]);
        s->flags      = W3DF_TSLOT_FLUSHME;
        s->cache_hits = 0;
        s->lpSource   = NULL;
        s->attach     = NULL;
    };
}

