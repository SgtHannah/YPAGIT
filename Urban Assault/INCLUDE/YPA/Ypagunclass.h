#ifndef YPA_GUNCLASS_H
#define YPA_GUNCLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypagunclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:26:03 $
**  $Locker:  $
**  $Author: floh $
**
**  flaks etc. Subclass der ypabact.class
**
**  (C) Copyright 1995 by Andreas Flemming
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

/*-------------------------------------------------------------------
**  NAME
**      ypagun.class --
**
**
**  FUNCTION
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**
**      ypagun.class
**      ~~~~~~~~~~~~
**
**          YGM_INSTALLGUN
**              Nagelt die Kanone in eine bestimmte Richtung fest und berechnet
**              daraus die lokale matrix
**
**          YGM_ROTATEGUN
**              Dreht die Kanone um eine übergebene Achse um einen Winkel, der
**              korrigierte Basisvektor wird zurückgegeben
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**      ypagun.class
**      ~~~~~~~~~~~~~
*/
#define YPAGUN_CLASSID "ypagun.class"

/*-----------------------------------------------------------------*/
#define YGM_BASE            (YBM_BASE+METHOD_DISTANCE)
#define YGM_INSTALLGUN      (YGM_BASE)
#define YGM_ROTATEGUN       (YGM_BASE + 1)


/*-----------------------------------------------------------------*/
#define YGA_BASE            (YBA_BASE+ATTRIB_DISTANCE)
#define YGA_SideAngle       (YGA_BASE)           // 1000-faches Übergeben !!!
#define YGA_UpAngle         (YGA_BASE + 1)       // wegen LONG
#define YGA_DownAngle       (YGA_BASE + 2)
#define YGA_FireType        (YGA_BASE + 3)
#define YGA_FireTime        (YGA_BASE + 4)
#define YGA_SetGround       (YGA_BASE + 5)
#define YGA_RoboGun         (YGA_BASE + 6)


/*** Feuertypen ***/
#define GF_None             (0)      // nicht. z.B. für Radarstationen
#define GF_Real             (1)      // mit Raketen
#define GF_Proto            (2)      // nur mit Prototyp


/*-------------------------------------------------------------------
**  Defaults für Attribute
*/
#define YGA_FireTime_DEF        (100)
#define YGA_FireType_DEF        GF_Real;

/*-----------------------------------------------------------------*/
struct ypagun_data {

    Object      *world;             // für schnellen Zugriff
    struct ypaworld_data *ywd;
    struct Bacterium *bact;         // für schnelleren Zugriff

    /*** Kanonen-Beschränkung ***/
    FLOAT       max_up;             // nach oben
    FLOAT       max_down;           // nach unten
    FLOAT       max_xz;             // zur Seite
    struct flt_triple basis;        // Basisvektor, normiert
    struct flt_triple rot;          // Rotachse, normiert

    /*** Schußart ***/
    UBYTE       firetype;           // Wie wird geschossen
    LONG        firetime;           // wie lang soll fire-Proto "nachglühen" ?
                                    // hat nur mit GF_Real einen Sinn
    LONG        firecount;          // zum eigentlichen Zählen

    /*** Zeuch ***/
    UBYTE       flags;
    LONG        ground_time;        // für Boden-tests
};



/*** weiteres Setposition-Flag ***/
#define YGFSP_SetFree   (4)         // nicht an Sektorhöhe anpassen




/*-------------------------------------------------------------------
**  Definitionen für ypagun_data.flags
*/
#define GUN_SetGround   (1<<0)      // Setpos wirkt immer mit Setground, egal, was
                                    // YBFSP_SetGround sagt
#define GUN_RoboGun     (1<<1)      // ist Robo-Bordkanone
#define GUN_HangDown    (1<<2)      // Kopie von YGFIG_HangDown
#define GUN_Shot        (1<<3)      // Ruckeln für MG-Typen


/*-------------------------------------------------------------------
** noch Zeuch eben
*/

struct installgun_msg {

    ULONG       flags;              // siehe unten
    struct flt_triple basis;        // Basisvektor, wobei nur x und z interessieren
                                    // muß nicht normiert sein, mache ich bei INSTALL
};

#define YGFIG_HangDown  (1L<<0)     // Die Kanone hängt nach unten


struct rotategun_msg {

    struct flt_triple rot;          // rotachse, sollte normiert sein!
    struct flt_triple basis;        // für Rückgabe der korrigierten Basis
    FLOAT  angle;                   // drehwinkel
};

/*-----------------------------------------------------------------*/
#endif

