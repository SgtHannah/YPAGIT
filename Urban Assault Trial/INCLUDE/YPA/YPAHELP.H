#ifndef YPA_HELP_H
#define YPA_HELP_H
/*
**  $Source: PRG:VFM/Include/ypa/ypahelp.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:26:20 $
**  $Locker:  $
**  $Author: floh $
**
**
**  (C) Copyright 1997 by Andreas Flemming
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef ADE_ADE_CLASS_H     /* für ArgStack/PubStack-Entry-Zeug */
#include "ade/ade_class.h"
#endif

#ifndef BASECLASS_BASECLASS_H
#include "baseclass/baseclass.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

#ifndef YPA_BACTCLASS_H
#include "ypa/ypabactclass.h"
#endif


/*** Die Struktur für eine Basisaktion ***/
struct BasisAction {

    // Enthält die Bits für die Basisaktionen, die schon getätigt
    // sein müssen
    ULONG   condition_mask;

    // Wie lange müssen wir nach der Erfüllung der Condition_mask
    // noch warten?
    LONG    time_to_wait;

    // Der Zähler hierfür. Wenn die Message mehrmasl gesendet werden
    // darf, wird der Zähler auf 0 zurückgesetzt, andernfalls wird die
    // Aktion als gemacht  (in AllAActions) deklariert
    LONG    wait_count;

    // message gesendet
    BOOL    sent;
};

#define MAXNUM_BASISACTIONS         20

/* --------------------------------------------------------------------
** Die Basisaktionen.
** OFS ist das Offset, BIT die Bitmaske, also (1L<<OFS) und DEF ist die
** voreingestellte condition_mask, die aber später aus einem File
** gelesen werden sollte, ebenso wie die Zeit TME
** ------------------------------------------------------------------*/

// das Spiel ging los
#define OFS_WELCOME                 0
#define BIT_WELCOME                 (1L<<OFS_WELCOME)
#define DEF_WELCOME                 0
#define TME_WELCOME                 5000

// du kannst Fahrzeuge bauen
#define OFS_BUILDVEHICLE            1
#define BIT_BUILDVEHICLE            (1L<<OFS_BUILDVEHICLE)
#define DEF_BUILDVEHICLE            BIT_WELCOME
#define TME_BUILDVEHICLE            30000

// du kannst dich da reinsetzen
#define OFS_CONTROLVEHICLE          2
#define BIT_CONTROLVEHICLE          (1L<<OFS_CONTROLVEHICLE)
#define DEF_CONTROLVEHICLE          BIT_BUILDVEHICLE
#define TME_CONTROLVEHICLE          10000

// Uservehicle ist gestorben, DEF egal, weil sowieso nur Hilfsaktion
#define OFS_DEADVEHICLE             3
#define BIT_DEADVEHICLE             (1L<<OFS_DEADVEHICLE)
#define DEF_DEADVEHICLE             BIT_CONTROLVEHICLE
#define TME_DEADVEHICLE             15000

// Soll zurück zur Station
#define OFS_AFTERDEATHTOSTATION     4
#define BIT_AFTERDEATHTOSTATION     (1L<<OFS_AFTERDEATHTOSTATION)
#define DEF_AFTERDEATHTOSTATION     BIT_DEADVEHICLE
#define TME_AFTERDEATHTOSTATION     10000

// Userrobo ist tot, ist ne Hilfsmessage
#define OFS_USERROBODEAD            5
#define BIT_USERROBODEAD            (1L<<OFS_USERROBODEAD)
#define DEF_USERROBODEAD            BIT_WELCOME
#define TME_USERROBODEAD            0

// Du solltest das Spiel verlassen!!!
#define OFS_FAILEXIT                6
#define BIT_FAILEXIT                (1L<<OFS_FAILEXIT)
#define DEF_FAILEXIT                BIT_USERROBODEAD
#define TME_FAILEXIT                15000

// Der Feind ist tot. Hilfsmessage
#define OFS_ENEMYROBODEAD           7
#define BIT_ENEMYROBODEAD           (1L<<OFS_ENEMYROBODEAD)
#define DEF_ENEMYROBODEAD           BIT_WELCOME
#define TME_ENEMYROBODEAD           0

// Schicke Leute ins Beamgate
#define OFS_VEHICLEINTHEGATE        8
#define BIT_VEHICLEINTHEGATE        (1L<<OFS_VEHICLEINTHEGATE)
#define DEF_VEHICLEINTHEGATE        BIT_ENEMYROBODEAD
#define TME_VEHICLEINTHEGATE        15000

// Beende, weil du gewonnen hast
#define OFS_WINEXIT                 9
#define BIT_WINEXIT                 (1L<<OFS_WINEXIT)
#define DEF_WINEXIT                 BIT_VEHICLEINTHEGATE|BIT_ENEMYROBODEAD
#define TME_WINEXIT                 15000

// //
// #define OFS_
// #define BIT_
// #define DEF_
// #define TME_
//
// //
// #define OFS_
// #define BIT_
// #define DEF_
//

#endif
