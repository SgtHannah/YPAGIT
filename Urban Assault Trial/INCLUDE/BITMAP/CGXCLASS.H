#ifndef BITMAP_CGXCLASS_H
#define BITMAP_CGXCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/cgxclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/06/09 02:25:29 $
**  $Locker:  $
**  $Author: floh $
**
**  Display-Treiber-Klasse für CybergraphX.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef BITMAP_DISPLAYCLASS_H
#include "bitmap/displayclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/gfx/cgx.class   -- Treiberklasse für CybergraphX
**
**  FUNCTION
**      Interface zur CybergraphX-Library aufm Amiga.
**      Eigentlich ist CybergraphX gar nicht notwendig,
**      nur ein schnelles WriteChunkyPixels()
**
**  METHODS
**
**  ATTRIBUTES
**      DISPA_DisplayID
**          Anmerkung: die cgx.class interpretiert dieses Attribut
**          die ModeID des Amiga-Screens.
**
**      DISPA_DisplayHandle
**          Anmerkung: die cgx.class interpretiert dieses Attribut
**          als AmigaOS-Window-Pointer.
**
**-----------------------------------------------------------------*/
#define CGXM_BASE       (DISPM_BASE+METHOD_DISTANCE)
#define CGXA_BASE       (DISPA_BASE+ATTRIB_DISTANCE)

#define CGX_CLASSID "drivers/gfx/cgx.class"

/*-----------------------------------------------------------------*/
struct cgx_data {
    ULONG id;               // Display-ID
    struct VFMBitmap *r;    // Pointer auf VFMBitmap
    struct Screen *scr;     // Screen-Pointer
    struct Window *win;     // Window-Pointer
};

/*-----------------------------------------------------------------*/
#endif

