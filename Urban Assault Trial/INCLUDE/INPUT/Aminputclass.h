#ifndef INPUT_AMINPUTCLASS_H
#define INPUT_AMINPUTCLASS_H
/*
**  $Source: PRG:VFM/Include/input/aminputclass.h,v $
**  $Revision: 38.6 $
**  $Date: 1996/08/31 00:33:33 $
**  $Locker:  $
**  $Author: floh $
**
**  Auf dem Amiga ist es schlecht machbar, und lohnt sich auch
**  nicht, die Input-Provider in unterschiedliche Klassen
**  aufzuteilen. Die aminput.class bietet alle möglichen
**  Arten von Input-Events, die das Amiga-Keyboard und
**  die Amiga-Mouse bereitstellen, das ganze sogar
**  richtig OS und Multitasking-freundlich.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INTUITION_INTUITION_H
#include "intuition/intuition.h"
#endif

#ifndef INPUT_IWIMPCLASS_H
#include "input/iwimpclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/input/amiga.class  --- AmigaOS-konformer Input-Handler
**
**  FUNCTION
**      Die amigainp.class interpretiert das IWIMPA_ModeInfo
**      als <struct Window> Pointer, sie ist deshalb auf
**      eine "kooperations-bereite" Gfx-Engine angewiesen
**      (alle Amiga-Engines müssen für das OVET_ModeInfo-Tag
**      einen Window-Pointer returnieren).
**      Das Window muß mit den IDCMP-Flags
**          IDCMP_MOUSEBUTTONS
**      ausgestattet sein.
**
**      Weil die Mouse-Verfolgung für die Slider über
**      IDCMP zuviel Overhead verursachen würde,
**      installiert die amiga.class zusätzlich einen
**      kleinen Input-Handler, der die Delta-Maus-Bewegung
**      sowie alle RAWKEY-Events aufzeichnet.
**
**      Die amiga.class stellt folgende Elemente zur Verfügung:
**
**          [a..z]
**          [0..9]
**          [f1..f10]
**          [num9..num9]
**          lshift
**          rshift
**          lalt
**          ralt
**          lamiga
**          ramiga
**          enter
**          left
**          right
**          up
**          down
**          ctrl
**          space
**          tab
**          del
**          help
**
**          lmb
**          mmb
**          rmb
**
**          mouse_x
**          mouse_y
**
**  METHODS
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define AMINPUT_CLASSID "drivers/input/amiga.class"
/*-----------------------------------------------------------------*/
#define AMINPUTM_BASE   (IWIMPM_BASE+METHOD_DISTANCE)
#define AMINPUTA_BASE   (IWIMPA_BASE+ATTRIB_DISTANCE)
/*-----------------------------------------------------------------*/
struct aminp_xy {
    WORD x,y;
};

struct aminput_data {

    struct Window *win;     // von hier bekommen wir unsere IDCMP_Messages

    LONG remap_index;       // Position des Watcher-Elements in Remap-Tabelle

    LONG mickey_store;      // Slider: Speicher für letzte Mouse-Pos
    LONG virtual_pos;       // Slider: "virtuelle" Mouse-Position

    UWORD lmb_down,lmb_up;  // Zähler LMB Down/Up Events
    UWORD rmb_down,rmb_up;  // Zähler RMB Down/Up Events
    UWORD mmb_down,mmb_up;  // Zähler MMB Down/Up Events

    struct aminp_xy lmb_down_pos;
    struct aminp_xy rmb_down_pos;
    struct aminp_xy mmb_down_pos;

    struct aminp_xy lmb_up_pos;
    struct aminp_xy rmb_up_pos;
    struct aminp_xy mmb_up_pos;
};

/*-------------------------------------------------------------------
**  spezielle interne Codes für Non-Keycode-Elemente:
*/
#define AMINPUT_CODE_LMB    (0x81)  // linke Maustaste
#define AMINPUT_CODE_RMB    (0x82)  // rechte Maustaste
#define AMINPUT_CODE_MMB    (0x83)  // mittlere Maustaste

#define AMINPUT_CODE_MX     (0x84)  // horizontal Maus
#define AMINPUT_CODE_MY     (0x85)  // vertikal Maus

#define AMINPUT_MAXVIRTUAL  (300)   // maximale virtuelle Slider-Pos

/*-----------------------------------------------------------------*/
#endif

