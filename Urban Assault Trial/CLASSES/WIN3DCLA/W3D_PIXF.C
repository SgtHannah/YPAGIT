/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_pixfmt.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 15:02:44 $
**  $Locker:  $
**  $Author: floh $
**
**  w3d_pixfmt.c -- Pixelformat-Sachen.
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

/*-----------------------------------------------------------------*/
unsigned long w3d_ColorConvert(unsigned long r,
                               unsigned long g,
                               unsigned long b,
                               unsigned long a,
                               long r_shift,
                               long g_shift,
                               long b_shift,
                               long a_shift,
                               long r_mask,
                               long g_mask,
                               long b_mask,
                               long a_mask)
/*
**  FUNCTION
**      Konvertiert einen RGBA8 Wert in ein beliebiges
**      anderes RGBA-Format.
**
**  CHANGED
**      22-Mar-97   floh    created
**      20-Aug-97   floh    created
*/
{
    if (r_shift >= 0) r <<= r_shift;
    else              r >>= -r_shift;
    if (g_shift >= 0) g <<= g_shift;
    else              g >>= -g_shift;
    if (b_shift >= 0) b <<= b_shift;
    else              b >>= -b_shift;
    if (a_shift >= 0) a <<= a_shift;
    else              a >>= -a_shift;
    r &= r_mask;
    g &= g_mask;
    b &= b_mask;
    a &= a_mask;
    return((unsigned long)(a|r|g|b));
}

/*-----------------------------------------------------------------*/
void w3d_KillPixFormats(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  CHANGED
**      10-Mar-97   floh    created
*/
{
    if (w3d->p) {
        if (w3d->p->disp_pfmt.rmap_table) {
            free(w3d->p->disp_pfmt.rmap_table);
            w3d->p->disp_pfmt.rmap_table = NULL;
        };
        if (w3d->p->txt_pfmt.rmap_table) {
            free(w3d->p->txt_pfmt.rmap_table);
            w3d->p->txt_pfmt.rmap_table = NULL;
        };
    };
}

/*-----------------------------------------------------------------*/
unsigned long w3d_InitPixFormats(struct windd_data *wdd, struct win3d_data *w3d)
/*
**  FUNCTION
**      Initialisiert die Pixelformat-Strukturen, welche für
**      diverse Remapping-Jobs benötigt werden.
**
**  CHANGED
**      10-Mar-97   floh    created
**      20-Aug-97   floh    + Alphachannel-Beachtung
*/
{
    unsigned long i;
    struct w3d_PixelFormat *pfmt;
    LPDDSURFACEDESC prim = &(wdd_Data.Primary);
    LPDDSURFACEDESC txt  = &(wdd->d3d->TxtFormats[wdd->d3d->ActTxtFormat]);

    /*** Display-Pixelformat-Struktur aufbauen ***/
    pfmt = &(w3d->p->disp_pfmt);
    memset(pfmt,0,sizeof(struct w3d_PixelFormat));
    pfmt->byte_size  = prim->ddpfPixelFormat.dwRGBBitCount >> 3;
    switch(pfmt->byte_size) {
        case 1:  pfmt->shift_size=0; break;
        case 2:  pfmt->shift_size=1; break;
        case 4:  pfmt->shift_size=2; break;
        default: pfmt->shift_size=0; break;
    };
    if ((prim->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) == 0) {

        /*** ein RGB-Format, Konvertierungsparameter ausfüllen ***/
        unsigned long m;
        int r,g,b,a;

        /*** Komponenten-Maske ***/
        pfmt->r_mask = prim->ddpfPixelFormat.dwRBitMask;
        pfmt->g_mask = prim->ddpfPixelFormat.dwGBitMask;
        pfmt->b_mask = prim->ddpfPixelFormat.dwBBitMask;
        pfmt->a_mask = prim->ddpfPixelFormat.dwRGBAlphaBitMask;

        /*** Shift-Wert für Bits-Per-Komponente ***/
        if (prim->ddpfPixelFormat.dwRBitMask) {
            for (r=0, m=prim->ddpfPixelFormat.dwRBitMask; !(m&1); r++,m>>=1);
            pfmt->r_shift = r;
            for (r=0; m&1; r++,m>>=1);
            pfmt->r_shift -= (8-r);
        } else pfmt->r_shift = 0;
        if (prim->ddpfPixelFormat.dwGBitMask) {
            for (g=0, m=prim->ddpfPixelFormat.dwGBitMask; !(m&1); g++,m>>=1);
            pfmt->g_shift = g;
            for (g=0; m&1; g++,m>>=1);
            pfmt->g_shift -= (8-g);
        } else pfmt->g_shift = 0;
        if (prim->ddpfPixelFormat.dwBBitMask) {
            for (b=0, m=prim->ddpfPixelFormat.dwBBitMask; !(m&1); b++,m>>=1);
            pfmt->b_shift = b;
            for (b=0; m&1; b++,m>>=1);
            pfmt->b_shift -= (8-b);
        } else pfmt->b_shift = 0;
        if (prim->ddpfPixelFormat.dwRGBAlphaBitMask) {
            for (a=0, m=prim->ddpfPixelFormat.dwRGBAlphaBitMask; !(m&1); a++,m>>=1);
            pfmt->a_shift = a;
            for (a=0; m&1; a++,m>>=1);
            pfmt->a_shift -= (8-b);
        } else pfmt->a_shift = 0;

        /*** ColorKey ermitteln und eintragen ***/
        pfmt->color_key = w3d_ColorConvert(0xff,0xff,0x0,0x0,
                          pfmt->r_shift,pfmt->g_shift,pfmt->b_shift,pfmt->a_shift,
                          pfmt->r_mask,pfmt->g_mask,pfmt->b_mask,pfmt->a_mask);

        /*** Remap-Tabelle allokieren (für VFM/8Bit -> PixelFormat) ***/
        pfmt->rmap_table = malloc(pfmt->byte_size * 256);
        if (!pfmt->rmap_table) {
            w3d_KillPixFormats(wdd,w3d);
            return(FALSE);
        };
    };

    /*** Remap-Tabelle für Linien ***/
    w3d->p->line_actindex = 0;
    for (i=0; i<W3D_COPPER_LINES; i++) {
        unsigned char r = i & 255;
        unsigned char g = i & 255;
        unsigned char b = i & 255;
        w3d->p->line_remap_table[i] = w3d_ColorConvert(r,g,b,255,
                          pfmt->r_shift,pfmt->g_shift,pfmt->b_shift,pfmt->a_shift,
                          pfmt->r_mask,pfmt->g_mask,pfmt->b_mask,pfmt->a_mask);
    };

    /*** Textur-Pixelformat-Struktur aufbauen ***/
    pfmt = &(w3d->p->txt_pfmt);
    memset(pfmt,0,sizeof(struct w3d_PixelFormat));
    pfmt->byte_size  = txt->ddpfPixelFormat.dwRGBBitCount >> 3;
    switch(pfmt->byte_size) {
        case 1:  pfmt->shift_size=0; break;
        case 2:  pfmt->shift_size=1; break;
        case 4:  pfmt->shift_size=2; break;
        default: pfmt->shift_size=0; break;
    };
    if ((txt->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) == 0) {

        /*** ein RGB-Format, Konvertierungsparameter ausfüllen ***/
        unsigned long m;
        int r,g,b,a;

        /*** Komponenten-Maske ***/
        pfmt->r_mask = txt->ddpfPixelFormat.dwRBitMask;
        pfmt->g_mask = txt->ddpfPixelFormat.dwGBitMask;
        pfmt->b_mask = txt->ddpfPixelFormat.dwBBitMask;
        pfmt->a_mask = txt->ddpfPixelFormat.dwRGBAlphaBitMask;

        /*** Shift-Wert für Bits-Per-Komponente ***/
        if (txt->ddpfPixelFormat.dwRBitMask) {
            for (r=0, m=txt->ddpfPixelFormat.dwRBitMask; !(m&1); r++,m>>=1);
            pfmt->r_shift = r;
            for (r=0; m&1; r++,m>>=1);
            pfmt->r_shift -= (8-r);
        } else pfmt->r_shift = 0;
        if (txt->ddpfPixelFormat.dwGBitMask) {
            for (g=0, m=txt->ddpfPixelFormat.dwGBitMask; !(m&1); g++,m>>=1);
            pfmt->g_shift = g;
            for (g=0; m&1; g++,m>>=1);
            pfmt->g_shift -= (8-g);
        } else pfmt->g_shift = 0;
        if (txt->ddpfPixelFormat.dwBBitMask) {
            for (b=0, m=txt->ddpfPixelFormat.dwBBitMask; !(m&1); b++,m>>=1);
            pfmt->b_shift = b;
            for (b=0; m&1; b++,m>>=1);
            pfmt->b_shift -= (8-b);
        } else pfmt->b_shift = 0;
        if (txt->ddpfPixelFormat.dwRGBAlphaBitMask) {
            for (a=0, m=txt->ddpfPixelFormat.dwRGBAlphaBitMask; !(m&1); a++,m>>=1);
            pfmt->a_shift = a;
            for (a=0; m&1; a++,m>>=1);
            pfmt->a_shift -= (8-a);
        } else pfmt->a_shift = 0;

        /*** ColorKey ermitteln und eintragen ***/
        pfmt->color_key = w3d_ColorConvert(0xff,0xff,0x0,0x0,
                          pfmt->r_shift,pfmt->g_shift,pfmt->b_shift,pfmt->a_shift,
                          pfmt->r_mask,pfmt->g_mask,pfmt->b_mask,pfmt->a_mask);

        /*** Remap-Tabelle allokieren (für VFM/8Bit -> PixelFormat) ***/
        pfmt->rmap_table = malloc(pfmt->byte_size * 256);
        if (!pfmt->rmap_table) {
            w3d_KillPixFormats(wdd,w3d);
            return(FALSE);
        };
    };

    return(TRUE);
}

