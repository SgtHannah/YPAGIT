#ifndef INPUT_DMOUSECLASS_H
#define INPUT_DMOUSECLASS_H
/*
**  $Source: PRG:VFM/Include/input/dmouseclass.h,v $
**  $Revision: 38.4 $
**  $Date: 1996/11/26 01:52:35 $
**  $Locker:  $
**  $Author: floh $
**
**  WIMP-Treiber-Klasse f�r eine PC-BIOS-Mouse.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_IWIMPCLASS_H
#include "input/iwimpclass.h"
#endif

#ifndef PROTOCOL_MSDOS_H
#include "protocol/msdos.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/input/dmouse.class -- Treiber f�r PC-BIOS-Mouse
**
**  FUNCTION
**      Vollst�ndiger PC-Mouse-Treiber. Arbeitet mit dem
**      BIOS-Mouse-Interface. Die Maus-Darstellung wird
**      selbst �bernommen. Folgende Input-Elemente werden
**      angeboten:
**
**          "lmb"       - linke...
**          "mmb"       - mittlere...
**          "rmb"       - rechte Maustaste
**
**          "x"         - horizontale Bewegungs-Achse
**          "y"         - vertikale Bewegungs-Achse
**
**      Der Treiber kann bei jeder Mausbewegung eine
**      externe Callback-Funktion aufrufen, dazu mu�
**      per IWIMPA_Environment ein <struct msdos_DispEnv *>
**      �bergeben werden.
*/

/*-----------------------------------------------------------------*/
#define DMOUSE_CLASSID "drivers/input/dmouse.class"
/*-----------------------------------------------------------------*/
struct dmouse_data {

    LONG remap_index;           // Position in Remap-Tabelle

    /*** Status des Mouse-Handlers ***/
    LONG mickey_pos;            // Speicher f�r letzte Mickey-Position
    LONG virtual_pos;           // f�r Slider-Handling (siehe MAX_VIRTUAL)

    UWORD lmb_down,lmb_up;      // Z�hler f�r LMB Down/Up Events
    UWORD rmb_down,rmb_up;      // Z�hler f�r RMB Down/Up Events
    UWORD mmb_down,mmb_up;      // Z�hler f�r MMB Down/Up Events

    /*** Koordinaten in "echten" Screen-Koordinaten ***/
    struct idev_xy lmb_down_pos;
    struct idev_xy rmb_down_pos;
    struct idev_xy mmb_down_pos;

    struct idev_xy lmb_up_pos;
    struct idev_xy rmb_up_pos;
    struct idev_xy mmb_up_pos;

    struct idev_xy screen_pos;      // Pointerpos in Pixels
    struct idev_xy raw_screen_pos;  // Pointer-Pos in Mickeys

    struct msdos_DispEnv *import;   // von Display-Treiber-Klasse
};

/*-------------------------------------------------------------------
**  Konstanten
*/
#define DMOUSE_BTN_LMB    (1<<0)  // equiv. RetVal der BIOS-Funcs
#define DMOUSE_BTN_RMB    (1<<1)
#define DMOUSE_BTN_MMB    (1<<2)

#define DMOUSE_SLD_X      (0x80)  // Bit 7 als Unterscheidung zu Buttons
#define DMOUSE_SLD_Y      (0x81)

#define DMOUSE_MAXVIRTUAL   (300)

/*-----------------------------------------------------------------*/
#endif
