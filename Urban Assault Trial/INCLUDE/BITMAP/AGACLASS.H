#ifndef BITMAP_AGACLASS_H
#define BITMAP_AGACLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/agaclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/06/09 20:51:05 $
**  $Locker:  $
**  $Author: floh $
**
**  Display-Treiber für Native-AGA-Screens (8-Bit-planar).
**  Einfach von der cgx.class abgeleitet, der einzige
**  Unterschied ist eine optimierte c2p-Konvertierung.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef BITMAP_CGXCLASS_H
#include "bitmap/cgxclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/gfx/aga.class   -- Treiberklasse für AGA-Amigas
**
**  FUNCTION
**      Erweitert die cgx.class um eine optimierte Chunk2Planar-
**      Konvertierung beim Kopieren in den Amiga-Screen.
*/
#define AGAM_BASE       (CGXM_BASE+METHOD_DISTANCE)
#define AGAA_BASE       (CGXA_BASE+ATTRIB_DISTANCE)

#define AGA_CLASSID "drivers/gfx/aga.class"

/*-----------------------------------------------------------------*/
struct aga_data {
    struct VFMBitmap *r;
    struct Screen *scr;
};
/*-----------------------------------------------------------------*/
#endif

