#ifndef INPUT_IE_H
#define INPUT_IE_H
/*
**  $Source: PRG:VFM/Include/input/ie.h,v $
**  $Revision: 38.5 $
**  $Date: 1996/05/27 17:07:07 $
**  $Locker:  $
**  $Author: floh $
**
**  Allgemeine InputEngine-Definitionen.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef INPUT_INPUT_H
#include "input/input.h"
#endif

/*-------------------------------------------------------------------
**  Base-Tags
**
**  C -> Configurable
**  S -> Settable
**  G -> Gettable
*/
#define IET_BASE    (TAG_USER+0x1000)

#define IET_GET_SPEC    (IET_BASE+1)    // (G)
#define IET_Debug       (IET_BASE+2)    // (SG) (IBM only)
#define IET_DeadEnd     (IET_BASE+3)    // (SG) (Amiga only)
#define IET_VideoX      (IET_BASE+4)    // (S) (IBM only, aktuelle X-Res
#define IET_VideoY      (IET_BASE+5)    // (S) (IBM only, aktuelle Y-Res
#define IET_VideoMode   (IET_BASE+6)    // (S) (IBM only, aktueller V-Mode
#define IET_ModeInfo    (IET_BASE+7)    // (S) Ptr auf interne ModeInfo-Struktur
#define IET_Pointer     (IET_BASE+8)    // (S) <struct VFMBitmap *>
#define IET_Object      (IET_BASE+9)    // (G) Ptr auf eingebettetes input.class Object

/*-----------------------------------------------------------------*/
#define IECONF_Debug        "input.debug"       // BOOL

#define IECONF_Timer        "input.timer"       // STRING
#define IECONF_Wimp         "input.wimp"        // STRING
#define IECONF_Keyboard     "input.keyboard"    // STRING

#define IECONF_Button0       "input.button[0]"  // STRING
#define IECONF_Button1       "input.button[1]"
#define IECONF_Button2       "input.button[2]"
#define IECONF_Button3       "input.button[3]"
#define IECONF_Button4       "input.button[4]"
#define IECONF_Button5       "input.button[5]"
#define IECONF_Button6       "input.button[6]"
#define IECONF_Button7       "input.button[7]"
#define IECONF_Button8       "input.button[8]"
#define IECONF_Button9       "input.button[9]"
#define IECONF_Button10      "input.button[10]"
#define IECONF_Button11      "input.button[11]"
#define IECONF_Button12      "input.button[12]"
#define IECONF_Button13      "input.button[13]"
#define IECONF_Button14      "input.button[14]"
#define IECONF_Button15      "input.button[15]"
#define IECONF_Button16      "input.button[16]"
#define IECONF_Button17      "input.button[17]"
#define IECONF_Button18      "input.button[18]"
#define IECONF_Button19      "input.button[19]"
#define IECONF_Button20      "input.button[20]"
#define IECONF_Button21      "input.button[21]"
#define IECONF_Button22      "input.button[22]"
#define IECONF_Button23      "input.button[23]"
#define IECONF_Button24      "input.button[24]"
#define IECONF_Button25      "input.button[25]"
#define IECONF_Button26      "input.button[26]"
#define IECONF_Button27      "input.button[27]"
#define IECONF_Button28      "input.button[28]"
#define IECONF_Button29      "input.button[29]"
#define IECONF_Button30      "input.button[30]"
#define IECONF_Button31      "input.button[31]"

#define IECONF_Slider0       "input.slider[0]"      // STRING
#define IECONF_Slider1       "input.slider[1]"
#define IECONF_Slider2       "input.slider[2]"
#define IECONF_Slider3       "input.slider[3]"
#define IECONF_Slider4       "input.slider[4]"
#define IECONF_Slider5       "input.slider[5]"
#define IECONF_Slider6       "input.slider[6]"
#define IECONF_Slider7       "input.slider[7]"
#define IECONF_Slider8       "input.slider[8]"
#define IECONF_Slider9       "input.slider[9]"
#define IECONF_Slider10      "input.slider[10]"
#define IECONF_Slider11      "input.slider[11]"
#define IECONF_Slider12      "input.slider[12]"
#define IECONF_Slider13      "input.slider[13]"
#define IECONF_Slider14      "input.slider[14]"
#define IECONF_Slider15      "input.slider[15]"
#define IECONF_Slider16      "input.slider[16]"
#define IECONF_Slider17      "input.slider[17]"
#define IECONF_Slider18      "input.slider[18]"
#define IECONF_Slider19      "input.slider[19]"
#define IECONF_Slider20      "input.slider[20]"
#define IECONF_Slider21      "input.slider[21]"
#define IECONF_Slider22      "input.slider[22]"
#define IECONF_Slider23      "input.slider[23]"
#define IECONF_Slider24      "input.slider[24]"
#define IECONF_Slider25      "input.slider[25]"
#define IECONF_Slider26      "input.slider[26]"
#define IECONF_Slider27      "input.slider[27]"
#define IECONF_Slider28      "input.slider[28]"
#define IECONF_Slider29      "input.slider[29]"
#define IECONF_Slider30      "input.slider[30]"
#define IECONF_Slider31      "input.slider[31]"

/*-----------------------------------------------------------------*/
#endif



