#ifndef YPA_YPAKEYS_H
#define YPA_YPAKEYS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypakeys.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:26:50 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition der Tasten
**
**
**  (C) Copyright 1995 by Andreas Flemming
**  (Achtung! Drauﬂen schneit's nicht mehr !! Klar schneit das, und wie !!)
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif



/*
** Hier werden die Defaultwerte der Tastaturereignisse definiert. Will der
** User andere haben, so ersetzt das WO diese durch die originalen Ereignisse
*/

// /*** Feuern ***/
// #define YPAKEY_FIRE             KEYCODE_SPACEBAR
// #define YPAKEY_FIREVIEW         KEYCODE_RSHIFT
// #define YPAKEY_FIREGUN          KEYCODE_RETURN
// #define YPAKEY_VISIER           KEYCODE_V
//
// /*** Kanone bewegen ***/
// #define YPAKEY_GUNUP            KEYCODE_A
// #define YPAKEY_GUNDOWN          KEYCODE_YZ
// #define YPAKEY_GUNLEFT
// #define YPAKEY_GUNRIGHT
//
// /*** Objekt rotieren ***/
// #define YPAKEY_ROTUP            KEYCODE_O
// #define YPAKEY_ROTDOWN          KEYCODE_U
// #define YPAKEY_ROTLEFT          KEYCODE_L
// #define YPAKEY_ROTRIGHT         KEYCODE_R
//
// /*** Ausrichten/Stoppen (Panzer nur bremsen) ***/
// #define YPAKEY_STOP             KEYCODE_NUM_0
// #define YPAKEY_HOLDHEIGHT       KEYCODE_H
//
// /*** Auftrieb ***/
// #define YPAKEY_UP               KEYCODE_CTRL
// #define YPAKEY_DOWN             KEYCODE_LALT
//
// /*** Wegschalten ***/
// #define YPAKEY_TOROBO           KEYCODE_F1
// #define YPAKEY_TOCOMMANDER      KEYCODE_F2


/*  ---------------------------------------------------------------------
**  New System: Im folgenden werden die Slider- und Button-IDs f¸r die
**  Input-Klassen deklariert. Ich gehe dabei wie folgt vor. Von den
**  jeweils 32 Elementen stehen mir 24 zur Verf¸gung.
**  Ich nehme je einen Slider und 2 Buttons zur Festlegung einer Sache
**  Somit kann garantiert werden, daﬂ das Soiel voll mit tastatur gespielt
**  werden kann.
**  Eine Unterteilung erfolgt lediglich in Land und Luft
**  -------------------------------------------------------------------*/

/*** die Slider ***/
#define SL_FLY_DIR          0       // FlugModell, die Richtung
#define SL_FLY_HEIGHT       1       // FM, die Hˆhe
#define SL_FLY_SPEED        2       // FM, die geschwindigkeit

#define SL_DRV_DIR          3       // Fahrmodell, die Richtung
#define SL_DRV_SPEED        4       // FM, die Geschwindigkeit

#define SL_GUN_HEIGHT       5       // Kanone hoch und runter
#define SL_ROT_VERT         6       // vertikale   Kopfrotation --> Robo
#define SL_ROT_HORIZ        7       // horizontale Kopfrotation --> Robo

/*** die Buttons ***/

/*** Feuer-Buttons ***/
#define BT_FIRE             (1L<<0)     //  normales feuern
#define BT_FIREVIEW         (1L<<1)     //  mit Viewer, also Mitfliegen
#define BT_FIREGUN          (1L<<2)     //  feuern mit Kanone

/*** Weitere Buttons ***/
#define BT_STOP             (1L<<3)
#define BT_WAYPOINT         (1L<<4)

/*-----------------------------------------------------------------*/
#endif


