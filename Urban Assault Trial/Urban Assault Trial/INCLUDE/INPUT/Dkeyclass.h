#ifndef INPUT_DKEYCLASS_H
#define INPUT_DKEYCLASS_H
/*
**  $Source: PRG:VFM/Include/input/dkeyclass.h,v $
**  $Revision: 38.4 $
**  $Date: 1996/11/21 01:41:41 $
**  $Locker:  $
**  $Author: floh $
**
**  Keyboard-Treiber für PCs.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_IDEVCLASS_H
#include "input/idevclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/input/dkey.class -- Treiber für PC-Keyboards
**
**  FUNCTION
**      Die Klasse schnappt sich den Keyboard-Interrupt,
**      ist deshalb unter Umständen etwas rüde.
**
**      Button-Descriptoren:
**
**          [a..z]
**          [0..9]
**          [f1..f10]
**          esc
**          tab
**          lshift
**          rshift
**          ctrl
**          alt
**          enter
**          left
**          right
**          up
**          down
**          space
**
**  METHODS
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define DKEY_CLASSID "drivers/input/dkey.class"
/*-----------------------------------------------------------------*/
#define DKEYM_BASE   (IDEVM_BASE+METHOD_DISTANCE)
#define DKEYA_BASE   (IDEVA_BASE+ATTRIB_DISTANCE)
/*-----------------------------------------------------------------*/
struct dkey_data {
    LONG remap_index;       // auf dieses Element gemappt
    FLOAT vpos;             // virtuelle Slider-Position
    UBYTE hotkeys[IDEV_NUMHOTKEYS]; // Hotkey-Tabelle
};
/*-----------------------------------------------------------------*/
#endif



