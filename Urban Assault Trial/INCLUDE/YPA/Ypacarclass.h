#ifndef YPA_CARCLASS_H
#define YPA_CARCLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypacarclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:24:41 $
**  $Locker:  $
**  $Author: floh $
**
**  cars. Subclass der ypabact.class
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

#ifndef YPA_TANKCLASS_H
#include "ypa/ypatankclass.h"
#endif

#ifndef YPA_BACTCLASS_H
#include "ypa/ypabactclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      ypacar.class --
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
**      ypacar.class
**      ~~~~~~~~~~~~~
*/
#define YPACAR_CLASSID "ypacar.class"

/*-----------------------------------------------------------------*/
#define YCM_BASE            (YTM_BASE+METHOD_DISTANCE)


/*-----------------------------------------------------------------*/
#define YCA_BASE            (YTA_BASE+ATTRIB_DISTANCE)
#define YCA_Kamikaze        (YCA_BASE)
#define YCA_Blast           (YCA_BASE + 1)


/*-------------------------------------------------------------------
**  Defaults für Attribute
*/



/*-----------------------------------------------------------------*/
struct ypacar_data {

    Object      *world;             // für schnellen Zugriff
    struct ypaworld_data *ywd;
    struct Bacterium *bact;         // für schnelleren Zugriff

    ULONG       kamikaze;           // für besonderes verhalten
    ULONG       blast;              // Sprengkraft im Zentrum
};

/*-------------------------------------------------------------------
**  Definitionen für ypacar_data.flags
*/


/*-------------------------------------------------------------------
** noch Zeuch eben
*/

/*-----------------------------------------------------------------*/
#endif

