#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H
/*
**  $Source: PRG:VFM/Include/input/input.h,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 12:59:52 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Lowlevel-IO in VFM.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INPUT_CLICKBOX_H
#include "input/clickbox.h"
#endif

#ifndef INPUT_KEYMAP_H
#include "input/keymap.h"
#endif

/*-----------------------------------------------------------------*/
struct VFMInput {
    /*** Time ***/
    ULONG FrameTime;        // in 1/1000 sec

    /*** Keys ***/
    UBYTE ContKey;          // zuletzt gedrückte Taste, nicht entprellt
    UBYTE NormKey;          // zuletzt gedrückte Taste, entprellt
    UBYTE HotKey;           // eine der 32 Hotkeys
    UBYTE AsciiKey;         // auf ASCII gemappte Key
    UBYTE pad;

    /*** Kompatiblität: alte Slider ***/
    WORD SliderX;           // Slider für X-Achse
    WORD SliderY;           // Slider für Y-Achse
    WORD SliderZ;           // Slider für Z-Achse
    WORD pad1;

    /*** 32 1-dimensionale Universal-Sliders ***/
    FLOAT Slider[32];

    /*** bis zu 32 Buttons ***/
    ULONG Buttons;

    /*** GUI ***/
    struct ClickInfo ClickInfo;   // siehe "input/clickbox.h"
};

/*-------------------------------------------------------------------
**  ACHTUNG:
**  ~~~~~~~~
**  Die Buttons [23..31] und Sliders [23..31] sind reserviert für
**  VFM. Diese Elemente werden von der base.class für
**  ihr Input-Handling benutzt und werden damit in allen
**  VFM-Tools, die einen Viewer integrieren, benutzt.
**  Es ist deshalb wichtig, das "Custom Applications"
**  diese Slider und Buttons NICHT benutzen.
*/
#define SYS_Slider_RotX     (31)
#define SYS_Slider_RotY     (30)
#define SYS_Slider_RotZ     (29)
#define SYS_Slider_Walk     (28)
#define SYS_Slider_Fly      (27)

#define SYS_Button_Center   (1<<31)

/*-----------------------------------------------------------------*/
#endif



