/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_attrs.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:22:59 $
**  $Locker: floh $
**  $Author: floh $
**
**  Attribut-Handling f�r ypatank.class.
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
#include "ypa/ypaufoclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

/*-----------------------------------------------------------------*/
BOOL yu_initAttrs(Object *o, struct ypaufo_data *yud, struct TagItem *attrs)
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

    yud->togo   = YUA_ToGo_DEF;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderf�lle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* die Attribute */
            switch(tag) {

                case YBA_World:

                    /* zuerst filtern wir den Weltpointer raus, den brauchen
                    ** wir hier auch. */
                    yud->world = (Object *) data;
                    yud->ywd   = INST_DATA( ((struct nucleusdata *)data)->o_Class,
                                 data);
                    break;

                case YUA_ToGo:

                    yud->togo = (FLOAT) data;
                    break;

                case YBA_UserInput:

                    if( !data ) {

                        /*** Nach UI wieder autonom --> Matrix "zurechtr�cken" ***/
                        yud->bact->dir.m11=yud->bact->dir.m22=yud->bact->dir.m33 = 1.0;
                        yud->bact->dir.m12=yud->bact->dir.m21=yud->bact->dir.m13 = 0.0;
                        yud->bact->dir.m31=yud->bact->dir.m32=yud->bact->dir.m23 = 0.0;
                        }
                    break;
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yu_setAttrs(Object *o, struct ypaufo_data *yud, struct TagItem *attrs)
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

            /* erstmal die Sonderf�lle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* dann die eigentlichen Attribute, sch�n nacheinander */
            switch (tag) {

                case YUA_ToGo:

                    yud->togo = (FLOAT) data;
                    break;

                case YBA_UserInput:

                    if( !data ) {

                        /*** Nach UI wieder autonom --> Matrix "zurechtr�cken" ***/
                        yud->bact->dir.m11=yud->bact->dir.m22=yud->bact->dir.m33 = 1.0;
                        yud->bact->dir.m12=yud->bact->dir.m21=yud->bact->dir.m13 = 0.0;
                        yud->bact->dir.m31=yud->bact->dir.m32=yud->bact->dir.m23 = 0.0;
                        }
                    break;

            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yu_getAttrs(Object *o, struct ypaufo_data *yud, struct TagItem *attrs)
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

            /* erstmal die Sonderf�lle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs  = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* dann die eigentlichen Attribute, sch�n nacheinander */
            switch (tag) {

                case YUA_ToGo:

                    *value = (LONG) yud->togo;
                    break;


            };
        };
    };

    /*** Ende ***/
}

