#ifndef INPUT_DBGMCLASS_H
#define INPUT_DBGMCLASS_H
/*
**  $Source: PRG:VFM/Include/input/dbgmclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/03/12 23:21:29 $
**  $Locker:  $
**  $Author: floh $
**
**  Debug-Mousetreiber-Klasse für DOS-PCs, benutzen den
**  Standard-Mouse-Treiber, deshalb Watcom-Dbg-sicher,
**  aber ohne Custom-Images, und ohne VESA-Support.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_IWIMPCLASS_H
#include "input/iwimpclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/input/dbgm.class -- Debug-Treiber für PC-BIOS-Mouse
**
**  FUNCTION
**          "lmb"       - linke...
**          "mmb"       - mittlere...
**          "rmb"       - rechte Maustaste
**
**          "x"         - horizontale Bewegungs-Achse
**          "y"         - vertikale Bewegungs-Achse
**
**  METHODS
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define DBGME_CLASSID "drivers/input/dbgm.class"
/*-----------------------------------------------------------------*/
struct dbgm_data {
    LONG remap_index;   // Position in Remap-Tabelle
    LONG store;         // letzte Mouse-Position
};

/*-------------------------------------------------------------------
**  Buttons und Sliders
*/
#define DBGM_BTN_LMB    (1<<0)  // equiv. RetVal der BIOS-Funcs
#define DBGM_BTN_RMB    (1<<1)
#define DBGM_BTN_MMB    (1<<2)

#define DBGM_SLD_X      (0x80)  // Bit 7 als Unterscheidung zu Buttons
#define DBGM_SLD_Y      (0x81)

/*-----------------------------------------------------------------*/
#endif
