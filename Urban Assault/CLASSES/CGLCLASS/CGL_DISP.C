/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_disp.c,v $
**  $Revision: 38.2 $
**  $Date: 1997/02/26 17:22:12 $
**  $Locker: floh $
**  $Author: floh $
**
**  Abgeleitete display.class Methoden der cgl.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap/cglclass.h"

extern struct MinList cgl_IDList;
extern struct disp_idnode *cgl_GetIDNode(ULONG);

/*** aus cgl_cache.c ***/
void cgl_TxtCacheBeginFrame(struct cgl_data *);
void cgl_TxtCacheEndFrame(struct cgl_data *);

/*** aus cgl_poly.c ***/
void cgl_FlushTracy(struct cgl_data *);
void cgl_FlushDelayed(struct cgl_data *);

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, cgl_DISPM_Query, struct disp_query_msg *msg)
/*
**  CHANGED
**      17-Aug-96   floh    created
*/
{
    ULONG id = msg->id;
    struct disp_idnode *ind;

    if (id != 0) ind = cgl_GetIDNode(id);
    else         ind = (struct disp_idnode *) cgl_IDList.mlh_Head;

    if (ind) {
        struct disp_idnode *next;
        msg->id = ind->id;
        msg->w  = ind->w;
        msg->h  = ind->h;
        strncpy(msg->name,ind->name,32);
        next = (struct disp_idnode *) ind->nd.mln_Succ;
        if (next->nd.mln_Succ) return(next->id);
    };

    return(0);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_Begin, void *nil)
/*
**  CHANGED
**      17-Aug-96   floh    created
**      18-Aug-96   floh    + cgl_TxtCacheBeginFrame()
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    cgl_TxtCacheBeginFrame(cd);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_End, void *nil)
/*
**  CHANGED
**      18-Aug-96   floh    created
**                          + cgl_TxtCacheEndFrame()
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);

    /*** Buffer swappen ***/
    cglSwapBuffer();
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_Show, void *nil)
/*
**  CHANGED
**      18-Aug-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_Hide, void *nil)
/*
**  CHANGED
**      18-Aug-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_MixPalette, struct disp_mixpal_msg *msg)
/*
**  CHANGED
**      18-Aug-96   floh    created
**                          + erzeugt jetzt auch die für das
**                            Textur-Remapping erforderliche
**                            RGB5551-Tabelle
**      30-Nov-96   floh    + uggh, nicht so einfach wie ich dachte...
**                            mal sehen...
*/
{
    struct cgl_data *cd     = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);
    CGL_COLOR_ST pal[256];
    ULONG i;

    /*** Superklasse übernimmt das Mixing... ***/
    _supermethoda(cl,o,DISPM_MixPalette,msg);

    /*** Rendition spezifisch !!! ***/
    memset(pal,0,sizeof(pal));
    for (i=0; i<256; i++) {
        pal[dd->slot[0].rgb[i].r].bRed   = dd->pal.rgb[i].r;
        pal[dd->slot[0].rgb[i].g].bGreen = dd->pal.rgb[i].g;
        pal[dd->slot[0].rgb[i].b].bBlue  = dd->pal.rgb[i].b;
    };

    /*** auf der 3D-Blaster macht das auch bei DirectColor Sinn! ***/
    // cglSetPalette(pal,0,256);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_SetPalette, struct disp_setpal_msg *msg)
/*
**  CHANGED
**      29-Nov-96   floh    created
**      30-Nov-96   floh    Palette wird zuerst auf Realwert gesetzt.
*/
{
    struct cgl_data *cd     = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);

    /*** erstmal an Superklasse ***/
    _supermethoda(cl,o,DISPM_SetPalette,msg);

    /*** falls es Slot #0 war, Paletten-Lookup-Table erzeugen ***/
    if (msg->slot == 0) {

        ULONG i;
        CGL_COLOR_ST pal[256];

        for (i=0; i<256; i++) {
            ULONG r = (dd->slot[0].rgb[i].r >> 3) << 10;
            ULONG g = (dd->slot[0].rgb[i].g >> 3) << 5;
            ULONG b = (dd->slot[0].rgb[i].b >> 3);
            cd->rgb5551_table[i] = (1<<15)|r|g|b;
        };

        /*** lösche Overlay-Bit bei Farbe 0 ***/
        cd->rgb5551_table[0] &= ~(1<<15);

        /*** Rendition-"Palette" restaurieren ***/
        for (i=0; i<256; i++) {
            pal[i].bRed   = (i*6)/7;
            pal[i].bGreen = i;
            pal[i].bBlue  = i;
            pal[i].bAlpha = 0;
        };
        // cglSetPalette(pal,0,256);
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_SetPointer, struct disp_pointer_msg *msg)
/*
**  CHANGED
**      28-Nov-96   floh    created
**      02-Dec-96   floh    + Wird nur noch modifiziert, wenn sich
**                            Image tatsächlich ändert, damit
**                            reduziert sich hoffentlich das Nadeln
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);

    /*** nur reagieren, wenn ein neues Image gesetzt wird ***/
    if (cd->ptr != msg->pointer) {
        if (cd->ptr = msg->pointer) {

            /*** Image-Data konvertieren ***/
            UBYTE *src  = (UBYTE *) cd->ptr->Data;
            UBYTE *dest = &(cd->ptr_data[0]);
            ULONG i,x,y,pos;
            UBYTE mask[4];
            CGL_COLOR_ST col_array[3];

            /*** Masken-Array (4 In-Byte-Positionen, Farbe 1) ***/
            for (pos=0; pos<4; pos++) {
                mask[pos] = 1<<(pos*2);
            };

            /*** Beachte: Destination ist immer 32x32 Pixel! ***/
            memset(cd->ptr_data,0,sizeof(cd->ptr_data));
            for (y=0; y<cd->ptr->Height; y++) {
                for (x=0; x<cd->ptr->Width; x++) {
                    UBYTE p = *src++;
                    if (p != 0) dest[((y<<5) + x)>>2] |= mask[x&3];
                };
            };

            /*** Cursor-Image setzen ***/
            for (i=0; i<3; i++) {
                col_array[i].bBlue  = 255;
                col_array[i].bGreen = 255;
                col_array[i].bRed   = 255;
                col_array[i].bAlpha = 0;
            };
            _disable();
            cglSetCursor(CGL_32x32x3,col_array,&(cd->ptr_data));
            _enable();

        } else {
            /*** Cursor unsichtbar schalten ***/
            _disable();
            cglSetCursor(CGL_DISABLE,NULL,NULL);
            _enable();
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, cgl_DISPM_ObtainTexture, struct disp_texture *msg)
/*
**  FUNCTION
**      Allokiert Platz für die vom 3D-Blaster verwendeten
**      16-Bit-Texturen.
**
**  CHANGED
**      26-Feb-97   floh    created
*/
{
    struct VFMBitmap *bmp = msg->texture;
    ULONG h = bmp->Height;

    bmp->BytesPerRow = 2*bmp->Width;
    bmp->Data        = (UBYTE *) _AllocVec(bmp->BytesPerRow*h,MEMF_PUBLIC|MEMF_CLEAR);
    bmp->Flags      |= VBF_Texture;
    if (bmp->Data) return(TRUE);
    else           return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_ReleaseTexture, struct disp_texture *msg)
/*
**  CHANGED
**      26-Feb-97   floh    created
*/
{
    struct VFMBitmap *bmp = msg->texture;
    if (bmp->Data) {
        _FreeVec(bmp->Data);
        bmp->Data = NULL;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_MangleTexture, struct disp_texture *msg)
/*
**  FUNCTION
**      8bpp/CLUT -> RGB5551.
**      Falls die Bitmap eine lokale Colormap besitzt, wird diese
**      für die Umwandlung verwendet, sonst die globale des
**      Raster-Objects.
**
**  CHANGED
**      26-Feb-97   floh    created
*/
{
    struct cgl_data *cd     = INST_DATA(cl,o);

    struct VFMBitmap *bmp = msg->texture;
    ULONG w = bmp->Width;
    ULONG h = bmp->Height;
    UBYTE *src;
    UWORD *tar;
    UWORD rgb_table[256];
    UWORD *rm_ptr;

    /*** lokale Colormap? ***/
    if (bmp->Flags & VBF_HasColorMap) {

        struct disp_RGB *cm = (struct disp_RGB *) bmp->ColorMap;
        ULONG i;

        for (i=0; i<256; i++) {
            ULONG r = (cm[i].r >> 3) << 10;
            ULONG g = (cm[i].g >> 3) << 5;
            ULONG b = (cm[i].r >> 3);
            rgb_table[i] = (1<<15)|r|g|b;
        };

        /*** Farbe 0 transparent ***/
        rgb_table[0] &= ~(1<<15);

        /*** Remap-Table-Ptr validieren ***/
        rm_ptr = rgb_table;
    } else {
        rm_ptr = cd->rgb5551_table;
    };

    /*** Remapping ***/
    src = ((UBYTE *)bmp->Data) + bmp->Width * bmp->Height;
    tar = ((UBYTE *)bmp->Data) + bmp->BytesPerRow * bmp->Height;
    while (src > bmp->Data) *--tar = rm_ptr[*--src];
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, cgl_DISPM_LockTexture, struct disp_texture *msg)
/*
**  CHANGED
**      26-Feb-97   floh    created
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_DISPM_UnlockTexture, struct disp_texture *msg)
/*
**  CHANGED
**      26-Feb-97   floh    created
*/
{ }


