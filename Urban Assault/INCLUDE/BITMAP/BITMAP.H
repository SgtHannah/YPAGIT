#ifndef BITMAP_BITMAP_H
#define BITMAP_BITMAP_H
/*
**  $Source: PRG:VFM/Include/bitmap/bitmap.h,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 12:53:31 $
**  $Locker: floh $
**  $Author: floh $
**
**  Definitionen für VFMBitmap-Struktur.
**
**  (C) Copyright 1994,1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef VISUALSTUFF_PRIMITIVE_H
#include "visualstuff/primitive.h"
#endif

/*-------------------------------------------------------------------
**  Background:
**  ~~~~~~~~~~~
**  Das Standard-Format von VFMBitmaps ist immer 8 bpp/clut,
**  die Color-Lookup-Table ist optional und besteht aus
**  RGB8 (== 3 Byte) je Eintrag.
**  Sobald allerdings das VBF_Texture Flag gesetzt ist,
**  kann <Data> auf jedes beliebige Textur-Format zeigen
**  (nämlich das bevorzugte Format des Display-Objects).
**  Die <ColorMap> bleibt auch in diesem Fall RGB8,
**  da die Textur-Map optional eine CLUT im eigenen
**  Format ausfüllen kann.
**
**  Für mehr Info zu Texturemaps siehe die Textur-Managment-
**  Methoden der raster.class und die bitmap.class.
*/
struct VFMBitmap {

    APTR Data;                      // zeigt auf Bitmap-Daten
    UWORD Width;                    // Breite in Pixel
    UWORD Height;                   // Höhe in Pixel
    ULONG BytesPerRow;              // Breite in Bytes
    APTR  ColorMap;                 // optional, UBYTE [3] *, R,G,B
    ULONG Flags;                    // siehe unten

    /*** Texture Stuff (nur gültig mit VBF_Texture gesetzt) ***/
    APTR TxtHandle;                 // optional
    APTR TxtCLUT;                   // optional
};

/*** VFMBitmap.Flags ***/
#define VBF_AlienData       (1<<0)  // Fremd-Body (Non-Standard-PixFmt!)
#define VBF_Texture         (1<<1)  // wird als Textur benutzt (Non-Standard-PixFmt!)
#define VBF_HasColorMap     (1<<2)  // VFMBitmap.ColorMap ist gültig
#define VBF_TxtBlittable    (1<<3)  // Textur soll im Display-Format vorliegen
                                    // (muß parallel zu VBF_Texture gesetzt sein)
#define VBF_AlphaHint       (1<<4)  // wird als Halbtransparent Map verwendet
/*-----------------------------------------------------------------*/
#endif

