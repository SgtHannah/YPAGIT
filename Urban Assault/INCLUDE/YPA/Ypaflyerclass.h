#ifndef YPA_FLYERCLASS_H
#define YPA_FLYERCLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypaflyerclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:25:20 $
**  $Locker:  $
**  $Author: floh $
**
**  Viele viele Fluggeräte
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
**      ypaflyer.class --
**
**
**  FUNCTION
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
*/
#define YPAFLYER_CLASSID "ypaflyer.class"

/*-----------------------------------------------------------------*/
#define YFM_BASE            (YBM_BASE+METHOD_DISTANCE)

#define YFM_AI_LEVEL3       YBM_AI_LEVEL3
#define YFM_MOVE            YBM_MOVE
#define YFM_HANDLEINPUT     YBM_HANDLEINPUT

/*-----------------------------------------------------------------*/
#define YFA_BASE        (YBA_BASE+ATTRIB_DISTANCE)
#define YFA_FlightType  (YFA_BASE)



/*-------------------------------------------------------------------
**  Defaults für Attribute
*/


/*-------------------------------------------------------------------
**  Eine OBNode hält gleichzeitig einen Pointer auf das Object
**  und die Bakterien-Struktur.
*/

/*-----------------------------------------------------------------*/
struct ypaflyer_data {

    /*** Häufig gebrauchtes ***/
    Object *world;
    struct ypaworld_data *ywd;
    struct Bacterium *bact;

    /*** Physikalisches ***/
    FLOAT   buoyancy;           // Auftrieb
    UBYTE   flight_type;        // Art der Ausrichtung bei Manövern
};

/*-------------------------------------------------------------------
**  Definitionen für ypaairplane_data.flags
*/


/*-------------------------------------------------------------------
**  FlightTypes
*/
#define YFF_RotX        (1<<0)      // "grob" um x-Achse drehen
#define YFF_RotZ        (1<<1)      //  um z-Achse drehen

#define YFF_AirPlane    (YFF_RotX | YFF_RotZ)
#define YFF_Zepp        0
#define YFF_Glider      YFF_RotZ


/*-----------------------------------------------------------------*/
#endif

