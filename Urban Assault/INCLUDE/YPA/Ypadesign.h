#ifndef YPA_YPADESIGN_H
#define YPA_YPADESIGN_H
/*
**  $Source: PRG:VFM/Include/ypa/ypadesign.h,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 14:25:12 $
**  $Locker:  $
**  $Author: floh $
**
**  Die IDs aller Fahrzeuge, Waffen und Buildings im Spiel.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Max. Anzahl für Prototypes.
*/

#define NUM_VEHICLEPROTOS   (256)    // max. ID/Anzahl Vehikle
#define NUM_WEAPONPROTOS    (128)    // max. ID/Anzahl Waffen
#define NUM_BUILDPROTOS     (128)    // max. ID/Anzahl Gebäude

/*** um die Debug-Dinger ein-/auszublenden ***/
#define YPA_DESIGN_DEBUG   (1)

//*******************************************************************
//  VEHICLE PROTOS
//*******************************************************************
//
// Fahrzeuge
// ~~~~~~~~~
#define VP_PANZ1        (0)
#define VP_CAR1         (1)
#define VP_SPAEWAGEN    (2)
#define VP_WABBEL       (3)

//
// Flugdinger
// ~~~~~~~~~~
#define VP_HUBI1        (16)
#define VP_FLYER1       (17)
#define VP_GLIDER1      (18)
#define VP_MINIZEPP1    (19)
#define VP_UFO1         (20)
#define VP_SATELLIT     (21)
#define VP_FLUGISIMPLE  (22)
#define VP_WASP         (23)
#define VP_JET1         (24)
#define VP_GLIDER2      (25)

//
// stationäre Waffensysteme
// ~~~~~~~~~~~~~~~~~~~~~~~~
#define VP_GULASCH      (32)
#define VP_SPAGHETTI    (33)
#define VP_KRAUT        (34)
#define VP_HAMBURGER    (35)

//
// Robos
// ~~~~~
#define VP_ROBO1        (48)

//
// Debug-Fahrzeuge
// ~~~~~~~~~~~~~~~
#if YPA_DESIGN_DEBUG

#define VP_EVILFURZ     (60)

#endif

//*******************************************************************
//  WAFFEN PROTOS
//*******************************************************************

//
// normale Raketen
//
#define WP_YOUREND          (0)     // wenn ich jetzt wüßte...
#define WP_MALEFACTOR       (1)
#define WP_EVILMESSAGE      (2)
#define WP_HELLBONBON       (3)
#define WP_LITTLEMONSTER    (4)

//
// Einschüsse
// ~~~~~~~~~~
#define WP_FIRSTSHOT        (60)

//*******************************************************************
//  BUILDING PROTOS
//*******************************************************************

//
//  Kraftwerke
//  ~~~~~~~~~~
#define BP_DUMMY            (0)     // NICHT ÄNDERN!!!
#define BP_KRAFTWERK_1      (1)
#define BP_KRAFTWERK_2      (2)
#define BP_FLAK_1           (3)

//
//  Radarstationen
//  ~~~~~~~~~~~~~~
#define BP_RADAR_1          (40)


//
//  sonstige
//  ~~~~~~~~

/*-----------------------------------------------------------------*/
#endif
