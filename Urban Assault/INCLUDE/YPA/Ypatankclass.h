#ifndef YPA_TANKCLASS_H
#define YPA_TANKCLASS_H
/*
**  $Source: PRG:MovingCubesII/Include/ypa/ypabactclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/08 23:10:23 $
**  $Locker: floh $
**  $Author: floh $
**
**  Panzer. Subclass der ypabact.class
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
**      ypatank.class
**      ~~~~~~~~~~~~~
**          YTA_GunAngle    Kanonenwinkel bezüglich lokaler x-z-Ebene
**                          alle Werte 1000-fach!!! (Get und Set)
**
**          YTA_Tip         Kippen, wenn Kraft wirkt
*/
#define YPATANK_CLASSID "ypatank.class"

/*-----------------------------------------------------------------*/
#define YTM_BASE            (YBM_BASE+METHOD_DISTANCE)
#define YTM_ALIGNVEHICLE_A  (YTM_BASE)
#define YTM_ALIGNVEHICLE_U  (YTM_BASE + 1)


/*-----------------------------------------------------------------*/
#define YTA_BASE            (YBA_BASE+ATTRIB_DISTANCE)
#define YTA_Tip             (YTA_BASE)


/*-------------------------------------------------------------------
**  Defaults für Attribute
*/



/*-----------------------------------------------------------------*/
struct ypatank_data {

    Object      *world;             // für schnellen Zugriff
    struct ypaworld_data *ywd;
    struct Bacterium *bact;         // für schnelleren Zugriff
    UBYTE       flags;              // Flags, siehe unten

    /*** Zeuch ***/
    LONG        wait_count;         // Zeit für's drehen

    /*** Kollisionsbearbeitung ***/
    struct flt_triple collvec;
    FLOAT  collangle;
    FLOAT  collway;
    UBYTE  collflags;
};


#define TCF_WALL_L      (1)
#define TCF_WALL_R      (2)
#define TCF_SLOPE_L     (4)
#define TCF_SLOPE_R     (8)


/*-------------------------------------------------------------------
**  Definitionen für ypatank_data.flags
*/
#define YTF_Tip             (1<<0)  // Kippen bei Beschleunigung
#define YTF_RotWhileWait    (1<<1)  // Rotieren beim Warten (dürfen SK evtl. nicht)


/*-------------------------------------------------------------------
**  Strukturen
*/

struct alignvehicle_msg  {

    FLOAT   time;               // die frame_time in sec
    struct  flt_triple old_dir; // alte Fahrtrichtung
    ULONG   ret_msg;            // wurde was zurückgegeben?
    struct  flt_triple slope;   // Normalenvektor eines möglichen Abhangs
};

#define AVMRET_SLOPE    (1L<<0) // slope ist gültig

/*-------------------------------------------------------------------
** noch Zeuch eben
*/

/*-----------------------------------------------------------------*/
#endif

