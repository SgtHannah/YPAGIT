#ifndef BITMAP_VGACLASS_H
#define BITMAP_VGACLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/vgaclass.h,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 12:54:42 $
**  $Locker:  $
**  $Author: floh $
**
**  Display-Treiber-Klasse für VGA Mode 13H und alle VESA-2.x Modi
**  mit linearem Framebuffer-Support.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef VISUALSTUFF_VESAKIT_H
#include "visualstuff/vesakit.h"
#endif

#ifndef BITMAP_DISPLAYCLASS_H
#include "bitmap/displayclass.h"
#endif

#ifndef PROTOCOL_MSDOS_H
#include "protocol/msdos.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/gfx/vga.class -- Treiberklasse für VGA/VESA2.x
**
**  FUNCTION
**      Mode 13H (320x200) ist immer verfügbar. Falls VESA2.x
**      vorhanden, außerdem alle Modi in 8 Bit Tiefe und
**      linearem Framebuffer.
**
**  METHODS
**
**  ATTRIBUTES
**      DISPA_DisplayID
**          Anmerkung: entspricht dem VGA-Modus.
**
**      DISPA_DisplayHandle
**          Anmerkung: Pointer auf VFMBitmap-Struktur,
**          welche das DISPLAY (nicht die Raster-Bitmap)
**          beschreibt.
**
**-----------------------------------------------------------------*/
#define VGAM_BASE       (DISPM_BASE+METHOD_DISTANCE)
#define VGAA_BASE       (DISPA_BASE+ATTRIB_DISTANCE)

#define VGA_CLASSID "drivers/gfx/vga.class"

/*-----------------------------------------------------------------*/
struct vga_mouse_restore {
    ULONG valid;            // TRUE -> Struktur ist gültig
    UBYTE *start;           // direkter Pointer ins VMem
    UWORD width,height;     // Höhe/Breite geclippt!
    UWORD off_add;          // Offset-Adder für 1 Zeile
    UBYTE data[4096];       // Platz für Pixel-Data
};

struct vga_data {

    /*** Misc ***/
    ULONG id;               // aktueller VideoMode
    ULONG flags;            // siehe unten
    struct VFMBitmap *r;    // Pointer auf Raster-Bitmap
    struct VFMBitmap d;     // Display-Beschreibung
    struct vesa_ModeInfoBlock mib;  // Daten des aktuellen VideoMode

    /*** Mouse-Pointer-Zeuch ***/
    LONG m_x,m_y;           // aktuelle Mauspointer-Position
    struct VFMBitmap *ptr;  // Mauspointer-Image
    struct vga_mouse_restore m_restore;

    /*** für den Export... ***/
    struct msdos_DispEnv export;
};

#define VGAF_MouseHidden    (1<<0)  // Mouse im Hidden-Status
#define VGAF_Vesa2          (1<<1)  // Vesa2 vorhanden
#define VGAF_8BitDAC        (1<<2)  // 8-Bit-DAC aktiviert

/*-- private ------------------------------------------------------*/
extern void vga_MRestore(struct vga_data *);
extern void vga_MDraw(struct vga_data *);
extern void vga_MShow(struct vga_data *);
extern void vga_MHide(struct vga_data *);
extern void vga_MFlush(struct vga_data *);
extern void vga_MCallBack(struct vga_data *, LONG, LONG);
/*-----------------------------------------------------------------*/
#endif

