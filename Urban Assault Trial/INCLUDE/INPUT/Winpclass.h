#ifndef INPUT_WINP_CLASS_H
#define INPUT_WINP_CLASS_H
/*
**  $Source: PRG:VFM/Include/input/winpclass.h,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 13:00:32 $
**  $Locker:  $
**  $Author: floh $
**
**  Input-Treiber-Klasse für Windows.
**  Fuer WinBox: WINP_WINBOX definieren.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_WIMPCLASS_H
#include "input/iwimpclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      drivers/input/winp.class -- Input-Treiber für Windows
**
**  FUNCTION
**      Über IWIMPA_Environment erhält die winp.class
**      einen Pointer auf das Window-Handle der
**      Display-Treiber-Klasse. Die winp.class
**      klemmt dann einen eigenen Message-Handler
**      zwischen Fenster und windd.class WinProc,
**      und zeichnet alle Input-Events an das Fenster
**      auf. Elegant :-)
**
**      Fuer die DirectInput-Initialisierung existiert ein
**      neues Attribut namens WINPA_Instance sowie eine
**      globale Variable namens winp_Instance, eine der
**      beiden muss mit der HINSTANCE der Windows-App
**      initialisiert werden. 
*/

/*-----------------------------------------------------------------*/
#define WINP_CLASSID "drivers/input/winp.class"
/*-----------------------------------------------------------------*/
#define WINPM_BASE        (IWIMPM_BASE+METHOD_DISTANCE)

#define WINPA_BASE        (IWIMPA_BASE+METHOD_DISTANCE)
#define WINPA_Instance    (WINPA_BASE)
#define WINPA_HWnd        (WINPA_BASE+1)

/*-----------------------------------------------------------------*/
struct winp_xy {
    WORD x,y;
};

struct winp_data {
    void *hwnd;             // Window-Handle
    LONG remap_index;       // Pos des Watcher-Elements in Remap-Tabelle
    LONG mouse_store;       // Slider only: letzte Mouse-Pos
    LONG mouse_slider;      // Mouse-Slider-Position
    UBYTE hotkeys[IDEV_NUMHOTKEYS]; // Hotkey-Tabelle
};

/*-------------------------------------------------------------------
**  Spezial-Codes für Non-Keycode-Elemente
*/
#define WINP_CODE_LMB       (0x81)  // linke Maustaste
#define WINP_CODE_RMB       (0x82)  // rechte Maustaste
#define WINP_CODE_MMB       (0x83)  // mittlere Maustaste

#define WINP_CODE_MX        (0x84)  // Maus horizontal
#define WINP_CODE_MY        (0x85)  // Maus vertikal

#define WINP_CODE_JB0       (0x86)  // max. 8 Joystick-Buttons
#define WINP_CODE_JB1       (0x87)
#define WINP_CODE_JB2       (0x88)
#define WINP_CODE_JB3       (0x89)
#define WINP_CODE_JB4       (0x8a)
#define WINP_CODE_JB5       (0x8b)
#define WINP_CODE_JB6       (0x8c)
#define WINP_CODE_JB7       (0x8d)

#define WINP_CODE_JX        (0x8e)  // Joystick-X-Achse
#define WINP_CODE_JY        (0x8f)  // Joystick-Y-Achse
#define WINP_CODE_THROTTLE  (0x90)  // Joystick-Throttle
#define WINP_CODE_HATX      (0x91)  // HatView-X-Komponente
#define WINP_CODE_HATY      (0x92)  // HatView-Y-Komponente
#define WINP_CODE_RUDDER    (0x93)  // Joystick-Ruder

#define WINP_MAX_SLIDER (300)

/*-----------------------------------------------------------------*/
#endif

