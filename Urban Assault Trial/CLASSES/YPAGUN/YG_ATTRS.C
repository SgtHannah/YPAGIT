/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_attrs.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:22:59 $
**  $Locker: floh $
**  $Author: floh $
**
**  Attribut-Handling für ypacar.class.
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypagunclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

/*-----------------------------------------------------------------*/
BOOL yg_initAttrs(Object *o, struct ypagun_data *ygd, struct TagItem *attrs)
/*
**  FUNCTION
**      (I)-Attribut-Handler.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/
{
    register ULONG tag;

    /*** Default ***/
    ygd->firetime = YGA_FireTime_DEF;
    ygd->firetype = YGA_FireType_DEF;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* die Attribute */
            switch(tag) {

                case YBA_World:

                    /* zuerst filtern wir den Weltpointer raus, den brauchen
                    ** wir hier auch. */
                    ygd->world = (Object *) data;
                    ygd->ywd   = INST_DATA( ((struct nucleusdata *)data)->o_Class,
                                 data);
                    break;

                /*** Nun folgen Winkel, wegen LONG 1000-faches ***/
                case YGA_SideAngle:

                    ygd->max_xz = (FLOAT)data / 1000.0;
                    break;

                case YGA_UpAngle:

                    ygd->max_up = (FLOAT)data / 1000.0;
                    break;

                case YGA_DownAngle:

                    ygd->max_down = (FLOAT)data / 1000.0;
                    break;

                case YGA_FireType:

                    ygd->firetype = (UBYTE) data;
                    break;

                case YGA_FireTime:

                    ygd->firetime = (LONG) data;
                    break;

                case YGA_SetGround:

                    if( data )  ygd->flags |=  GUN_SetGround;
                    else        ygd->flags &= ~GUN_SetGround;
                    break;

                case YGA_RoboGun:

                    if( data )  ygd->flags |=  GUN_RoboGun;
                    else        ygd->flags &= ~GUN_RoboGun;
                    break;
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yg_setAttrs(Object *o, struct ypagun_data *ygd, struct TagItem *attrs)
/*
**  FUNCTION
**      (S)-Attribut-Handler.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* dann die eigentlichen Attribute, schön nacheinander */
            switch (tag) {

                /*** Nun folgen Winkel, wegen LONG 1000-faches ***/
                case YGA_SideAngle:

                    ygd->max_xz = (FLOAT)data / 1000.0;
                    break;

                case YGA_UpAngle:

                    ygd->max_up = (FLOAT)data / 1000.0;
                    break;

                case YGA_DownAngle:

                    ygd->max_down = (FLOAT)data / 1000.0;
                    break;

                case YGA_FireType:

                    ygd->firetype = (UBYTE) data;
                    break;

                case YGA_FireTime:

                    ygd->firetime = (LONG) data;
                    break;

                case YGA_SetGround:

                    if( data )  ygd->flags |=  GUN_SetGround;
                    else        ygd->flags &= ~GUN_SetGround;
                    break;

                case YGA_RoboGun:

                    if( data )  ygd->flags |=  GUN_RoboGun;
                    else        ygd->flags &= ~GUN_RoboGun;
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yg_getAttrs(Object *o, struct ypagun_data *ygd, struct TagItem *attrs)
/*
**  FUNCTION
**      Handelt alle (G)-Attribute komplett ab.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      24-Apr-95   floh    created
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs  = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* dann die eigentlichen Attribute, schön nacheinander */
            switch (tag) {

                /*** Nun folgen Winkel, wegen LONG 1000-faches ***/
                case YGA_SideAngle:

                    *value = (LONG) (ygd->max_xz * 1000);
                    break;

                case YGA_UpAngle:

                    *value = (LONG) (ygd->max_up * 1000);
                    break;

                case YGA_DownAngle:

                    *value = (LONG) (ygd->max_down * 1000);
                    break;

                case YGA_FireType:

                    *value = (LONG) ygd->firetype;
                    break;

                case YGA_FireTime:

                    *value = (LONG) ygd->firetime;
                    break;

                case YGA_SetGround:

                    if( ygd->flags & GUN_SetGround )
                        *value = 1L;
                    else
                        *value = 0L;
                    break;

                case YGA_RoboGun:

                    if( ygd->flags & GUN_RoboGun )
                        *value = 1L;
                    else
                        *value = 0L;
                    break;
            };
        };
    };

    /*** Ende ***/
}

