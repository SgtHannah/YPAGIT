#ifndef YPA_UFOCLASS_H
#define YPA_UFOCLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypaufoclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:29:10 $
**  $Locker:  $
**  $Author: floh $
**
**  ufos. Subclass der ypabact.class
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
**      ypatank.class --
**
**
**  FUNCTION
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**          YBM_AI_LEVEL3     wird hier überladen und an das Fahrverhalten 
**                            angepaßt
**
**          YBM_MOVE          Ebenso.
**
**          YBM_CHECKCOLLISION   Entfällt
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**      ypaufo.class
**      ~~~~~~~~~~~~~
*/
#define YPAUFO_CLASSID "ypaufo.class"

/*-----------------------------------------------------------------*/
#define YUM_BASE            (YBM_BASE+METHOD_DISTANCE)


/*-----------------------------------------------------------------*/
#define YUA_BASE            (YBA_BASE+ATTRIB_DISTANCE)
#define YUA_ToGo            (YUA_BASE)

/*-------------------------------------------------------------------
**  Defaults für Attribute
*/

#define YUA_ToGo_DEF        (200.0)


/*-----------------------------------------------------------------*/
struct ypaufo_data {

    Object      *world;             // für schnellen Zugriff
    struct ypaworld_data *ywd;
    struct Bacterium *bact;         // für schnelleren Zugriff

    FLOAT       togo;               // Weg nach Kurskorrektur o.ä.
    FLOAT       gone;               // noch zu gehen von togo
    FLOAT       buoyancy;           // Auftrieb;
    FLOAT       rotangle;           // Kollisionsbearbeitung
    
    ULONG       flags;
};

/*-------------------------------------------------------------------
**  Definitionen für ypaufo_data.flags
*/
#define YUF_AVOID       (1L<<0)     // wir weichen aus bzw. sollen das tun
#define YUF_GOUP        (1L<<1)     // und zwar nach oben
#define YUF_GODOWN      (1L<<2)     // und zwar nach unten
#define YUF_ROT         (1L<<3)     // und zwar zur Seite


/*-------------------------------------------------------------------
** noch Zeuch eben
*/

/*-----------------------------------------------------------------*/
#endif

