/*
**  $Source: PRG:VFM/Classes/_DisplayClass/disp_main.c,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 14:49:07 $
**  $Locker:  $
**  $Author: floh $
**
**  display.class -- Stammklasse aller Display-Treiber-Klassen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "bitmap/displayclass.h"

struct NucleusBase *disp_NBase;

/*-------------------------------------------------------------------
**  Methoden Handler Protos
*/
_dispatcher(Object *, disp_OM_NEW, struct TagItem *);
_dispatcher(ULONG, disp_OM_DISPOSE, void *);
_dispatcher(void, disp_OM_SET, struct TagItem *);
_dispatcher(void, disp_OM_GET, struct TagItem *);
_dispatcher(void, disp_DISPM_SetPalette, struct disp_setpal_msg *);
_dispatcher(void, disp_DISPM_MixPalette, struct disp_mixpal_msg *);
_dispatcher(void, disp_DISPM_SetPointer, struct disp_pointer_msg *);
_dispatcher(void, disp_DISPM_ShowPointer, void *);
_dispatcher(void, disp_DISPM_HidePointer, void *);
_dispatcher(ULONG, disp_DISPM_ObtainTexture, struct disp_texture *);
_dispatcher(void, disp_DISPM_ReleaseTexture, struct disp_texture *);
_dispatcher(void, disp_DISPM_MangleTexture, struct disp_texture *);
_dispatcher(ULONG, disp_DISPM_LockTexture, struct disp_texture *);
_dispatcher(void, disp_DISPM_UnlockTexture, struct disp_texture *);
_dispatcher(void, disp_DISPM_GetPalette, struct disp_setpal_msg *);
_dispatcher(ULONG, disp_DISPM_ScreenShot, struct disp_screenshot_msg *);

/*-------------------------------------------------------------------
**  Class Header
*/
_use_nucleus

#ifdef AMIGA
__far ULONG disp_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG disp_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo disp_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *disp_MakeClass(ULONG id,...);
__geta4 BOOL disp_FreeClass(void);
#else
struct ClassInfo *disp_MakeClass(ULONG id,...);
BOOL disp_FreeClass(void);
#endif

struct GET_Class disp_GET = {
    &disp_MakeClass,
    &disp_FreeClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&disp_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *disp_Entry(void)
{
    return(&disp_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo disp_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:display.class"
    },
    disp_Entry,
};
#endif

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)
{
    register ULONG act_tag;

    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {
        switch (act_tag) {
            case TAG_DONE:  return(NULL); break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;
            case TAG_SKIP:  tagList += tagList->ti_Data; break;
            default:        tagList++;
        };
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *disp_MakeClass(ULONG id,...)
#else
struct ClassInfo *disp_MakeClass(ULONG id,...)
#endif
/*
**  CHANGES
**      07-Jun-96   floh    created
**      13-Aug-96   floh    display.class Paletten-Handling
**      17-Aug-96   floh    + disp_OM_SET
**      23-Nov-96   floh    + Mousepointer-Methoden
**      24-Feb-97   floh    + initialisiert disp_NBase
**      18-Jun-97   floh    + DISPM_ScreenShot
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    _get_nbase((struct TagItem *)&id,&disp_NBase);

    memset(disp_Methods,0,sizeof(disp_Methods));

    disp_Methods[OM_NEW]               = (ULONG) disp_OM_NEW;
    disp_Methods[OM_DISPOSE]           = (ULONG) disp_OM_DISPOSE;
    disp_Methods[OM_SET]               = (ULONG) disp_OM_SET;
    disp_Methods[OM_GET]               = (ULONG) disp_OM_GET;
    disp_Methods[DISPM_SetPalette]     = (ULONG) disp_DISPM_SetPalette;
    disp_Methods[DISPM_MixPalette]     = (ULONG) disp_DISPM_MixPalette;
    disp_Methods[DISPM_SetPointer]     = (ULONG) disp_DISPM_SetPointer;
    disp_Methods[DISPM_ShowPointer]    = (ULONG) disp_DISPM_ShowPointer;
    disp_Methods[DISPM_HidePointer]    = (ULONG) disp_DISPM_HidePointer;
    disp_Methods[DISPM_ObtainTexture]  = (ULONG) disp_DISPM_ObtainTexture;
    disp_Methods[DISPM_ReleaseTexture] = (ULONG) disp_DISPM_ReleaseTexture;
    disp_Methods[DISPM_MangleTexture]  = (ULONG) disp_DISPM_MangleTexture;
    disp_Methods[DISPM_LockTexture]    = (ULONG) disp_DISPM_LockTexture;
    disp_Methods[DISPM_UnlockTexture]  = (ULONG) disp_DISPM_UnlockTexture;
    disp_Methods[DISPM_GetPalette]     = (ULONG) disp_DISPM_GetPalette;
    disp_Methods[DISPM_ScreenShot]     = (ULONG) disp_DISPM_ScreenShot;

    disp_clinfo.superclassid = RASTER_CLASSID;
    disp_clinfo.methods      = disp_Methods;
    disp_clinfo.instsize     = sizeof(struct display_data);
    disp_clinfo.flags        = 0;

    /* und das war's */
    return(&disp_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL disp_FreeClass(void)
#else
BOOL disp_FreeClass(void)
#endif
/*
**  CHANGED
**      07-Jun-96   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**                                                                 **
**  METHODEN HANDLER                                               **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, disp_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      24-Feb-97   floh    created
*/
{
    /*** initialisiert disp_NBase->GfxObject ***/
    Object *newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    disp_NBase->GfxObject = newo;
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, disp_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      24-Feb-97   floh    created
*/
{
    /*** invalidiere globalen GfxObject Pointer ***/
    disp_NBase->GfxObject = NULL;
    return((ULONG)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      17-Aug-96   floh    created
*/
{
    struct TagItem *ti;

    /*** BMA_ColorMap ***/
    if (ti = _FindTagItem(BMA_ColorMap, attrs)) {
        UBYTE *cm;
        if (cm = (UBYTE *) ti->ti_Data) {
            struct disp_setpal_msg dsm;
            struct disp_mixpal_msg dmm;
            ULONG slot[1],weight[1];

            dsm.slot  = 0;
            dsm.first = 0;
            dsm.num   = 256;
            dsm.pal   = cm;
            _methoda(o, DISPM_SetPalette, &dsm);

            slot[0]    = 0;
            weight[0]  = 256;
            dmm.num    = 1;
            dmm.slot   = (ULONG *) &slot;
            dmm.weight = (ULONG *) &weight;
            _methoda(o, DISPM_MixPalette, &dmm);
        };
    };

    /*** das war's schon ***/
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      29-Sep-96   floh    created
*/
{
    struct TagItem *ti;
    struct display_data *dd = INST_DATA(cl,o);

    /*** BMA_ColorMap ***/
    if (ti = _FindTagItem(BMA_ColorMap, attrs)) {
        ULONG *data = (ULONG *) ti->ti_Data;
        *data = (ULONG) &(dd->pal);
        ti->ti_Tag = TAG_IGNORE; // damit Superklasse nicht überschreibt!
    };

    /*** Ende ***/
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_SetPalette, struct disp_setpal_msg *msg)
/*
**  CHANGED
**      13-Aug-96   floh    created
*/
{
    struct display_data *dd = INST_DATA(cl,o);
    UBYTE *from = msg->pal;
    UBYTE *to   = (UBYTE *) &(dd->slot[msg->slot].rgb[msg->first]);
    ULONG size  = msg->num * sizeof(struct disp_RGB);
    memcpy(to,from,size);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_GetPalette, struct disp_setpal_msg *msg)
/*
**  CHANGED
**      22-Aug-97   floh    created
*/
{
    struct display_data *dd = INST_DATA(cl,o);
    msg->pal = (UBYTE *) &(dd->slot[msg->slot]);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_MixPalette, struct disp_mixpal_msg *msg)
/*
**  CHANGED
**      13-Aug-96   floh    created
*/
{
    struct display_data *dd = INST_DATA(cl,o);
    ULONG entry;

    /*** für jeden Paletten-Eintrag... ***/
    for (entry=0; entry<DISP_PAL_NUMENTRIES; entry++) {

        ULONG i,r,g,b;

        /*** Source-Farben im richtigen Verhältnis mischen ***/
        r = g = b = 0;
        for (i=0; i<msg->num; i++) {
            struct disp_RGB *rgb0 = &(dd->slot[msg->slot[i]].rgb[entry]);
            r += rgb0->r * msg->weight[i];
            g += rgb0->g * msg->weight[i];
            b += rgb0->b * msg->weight[i];
        };

        /*** Festkomma-Korrektur und Überbelichtung clippen ***/
        r >>= 8;
        if (r > 255) r=255;
        g >>= 8;
        if (g > 255) g=255;
        b >>= 8;
        if (b > 255) b=255;

        /*** und in den Paletten-Slot eintragen ***/
        dd->pal.rgb[entry].r = r;
        dd->pal.rgb[entry].g = g;
        dd->pal.rgb[entry].b = b;
    };

    /*** das war's bereits ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_SetPointer, struct disp_pointer_msg *msg)
/*
**  CHANGED
**      23-Nov-96   floh    created
*/
{
    struct display_data *dd = INST_DATA(cl,o);
    _methoda(o,DISPM_HidePointer,NULL);
    dd->pointer = msg->pointer;
    _methoda(o,DISPM_ShowPointer,NULL);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_ShowPointer, void *nil)
/*
**  CHANGED
**      23-Nov-96   floh    created
*/
{
    struct display_data *dd = INST_DATA(cl,o);
    dd->flags &= ~DISPF_PointerHidden;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_HidePointer, void *nil)
/*
**  CHANGED
**      23-Nov-96   floh    created
*/
{
    struct display_data *dd = INST_DATA(cl,o);
    dd->flags |= DISPF_PointerHidden;
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, disp_DISPM_ObtainTexture, struct disp_texture *msg)
/*
**  CHANGED
**      24-Feb-97   floh    created
*/
{
    /*** raster.class benutzt 8bpp/CLUT als Textur-Format ***/
    struct VFMBitmap *bmp = msg->texture;
    ULONG h = bmp->Height;

    bmp->BytesPerRow = bmp->Width;
    bmp->Data        = (UBYTE *) _AllocVec(bmp->BytesPerRow*h,MEMF_PUBLIC|MEMF_CLEAR);
    bmp->Flags      |= VBF_Texture;
    if (bmp->Data) return(TRUE);
    else           return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_ReleaseTexture, struct disp_texture *msg)
/*
**  CHANGED
**      24-Feb-97   floh    created
*/
{
    struct VFMBitmap *bmp = msg->texture;
    if (bmp->Data) {
        _FreeVec(bmp->Data);
        bmp->Data = NULL;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_MangleTexture, struct disp_texture *msg)
/*
**  CHANGED
**      24-Feb-97   floh    created
*/
{
    /*** keine Konvertierung notwendig ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, disp_DISPM_LockTexture, struct disp_texture *msg)
/*
**  CHANGED
**      25-Feb-97   floh    created
*/
{
    /*** raster.class Texturen brauchen keine Lock-Behandlung ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, disp_DISPM_UnlockTexture, struct disp_texture *msg)
/*
**  CHANGED
**      25-Feb-97   floh    created
*/
{
    /*** raster.class Texturen brauchen keine Lock-Behandlung ***/
}

/*=================================================================**
**                                                                 **
**  SUPPORT ROUTINEN FÜR DISPM_ScreenShot                          **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
/*
**  FUNCTION
**      disp_Putbyte() -> schreibt ein Byte nach File
**      disp_Putword() -> schreibt ein Word nach File
**
**  CHANGED
**      03-Apr-96   floh    created in yw_supp.c
**      18-Jun-97   floh    übernommen nach disp_main.c
*/
void disp_Putbyte(ULONG val, APTR file)
{
    UBYTE b = (UBYTE) val;
    _FWrite(&b,sizeof(b),1,file);
}

void disp_Putword(ULONG val, APTR file)
{
    UBYTE lo = val % 256;
    UBYTE hi = val / 256;
    _FWrite(&lo,sizeof(lo),1,file);
    _FWrite(&hi,sizeof(hi),1,file);
}

/*-----------------------------------------------------------------*/
ULONG disp_FrameSnap(UBYTE *name, ULONG w, ULONG h, UBYTE *body, UBYTE *palette)
/*
**  FUNCTION
**      Erzeugt einen Snap-Shot des aktuellen Frames und
**      sichert diesen als PCX nach <name>.
**
**  INPUTS
**      name    - gültiger Filename
**      w,h     - Breite und Höhe des Displays
**      body    - Pointer auf 8-Bit-Bitmap-Data
**      palette - Pointer auf Farbpalette
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> do hoot was net geklappt...
**
**  CHANGED
**      03-Apr-96   floh    created in yw_supp.c
**      18-Jun-97   floh    + übernommen nach disp_main.c
*/
{
    BOOL retval = FALSE;

    if (body && palette) {

        APTR file;

        file = _FOpen(name, "wb");
        if (file) {

            ULONG i;
            LONG idx,pix2go,currentpix,XCo,pixn;

            /*** PCX-Header aufbauen ***/
            disp_Putbyte(0xa,file);   // .PCX magic number
            disp_Putbyte(0x5,file);   // PC Paintbrush version
            disp_Putbyte(0x1,file);   // PCX run length encoding
            disp_Putbyte(0x8,file);   // 8 Bits pro Pixel

            disp_Putword(0,file);     // x1 - image left
            disp_Putword(0,file);     // y1 - image top
            disp_Putword(w-1,file);   // x2 - image right
            disp_Putword(h-1,file);   // y2 - image bottom

            disp_Putword(w,file);     // horizontal resolution
            disp_Putword(h,file);     // vertical resolution

            // 16 Farben Palette
            for (i=0; i<48; i++) disp_Putbyte(palette[i],file);

            disp_Putbyte(0,file); // reserved byte
            disp_Putbyte(1,file); // number of color planes

            disp_Putword(w,file); // Bytes Per Scanline
            disp_Putword(1,file); // palette info

            // fill to end of header
            for (i=0; i<58; i++) disp_Putbyte(0,file);

            /*** Body schreiben ***/
            pixn = w*h;
            idx  = 0;
            while (idx<pixn) {

                pix2go     = 0;
                currentpix = body[idx];
                XCo        = idx % w;

                while ((body[idx+pix2go] == currentpix) &&
                       (pix2go < 63) &&
                       ((pix2go+idx) < pixn) &&
                       ((XCo+pix2go) < w))
                {
                    pix2go++;
                };

                if ((pix2go > 1) || (currentpix > 191)) {
                    disp_Putbyte(192+pix2go,file);
                    disp_Putbyte(currentpix,file);
                } else {
                    disp_Putbyte(currentpix,file);
                };
                idx += pix2go;
            };

            /*** Palette am Ende des Files ***/
            disp_Putbyte(0x0c,file);  // magic for 256 colors
            _FWrite(palette, 3*256, 1, file);
            _FClose(file);
            retval = TRUE;
        };
    };

    /*** Ende ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, disp_DISPM_ScreenShot, struct disp_screenshot_msg *msg)
/*
**  FUNCTION
**      Die 8-Bit-Variante schreibt einen PCX-File
**
**  CHANGED
**      18-Jun-97   floh    created
*/
{
    UBYTE fname[128];
    ULONG w,h;
    UBYTE *body;
    UBYTE *palette;

    struct display_data *dd = INST_DATA(cl,o);
    struct raster_data *rd  = INST_DATA(cl->superclass,o);

    if (rd->r) {
        w = rd->r->Width;
        h = rd->r->Height;
        body = rd->r->Data;
        palette = (UBYTE *) &(dd->pal);
        strcpy(fname,msg->filename);
        strcat(fname,".pcx");
        return(disp_FrameSnap(fname,w,h,body,palette));
    } else return(FALSE);
}

