/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_attrs.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:22:59 $
**  $Locker: floh $
**  $Author: floh $
**
**  Attribut-Handling für ypatank.class.
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
#include "ypa/ypatankclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

/*-----------------------------------------------------------------*/
BOOL yt_initAttrs(Object *o, struct ypatank_data *ytd, struct TagItem *attrs)
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

    ytd->flags            = YTF_Tip | YTF_RotWhileWait;

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
                    ytd->world = (Object *) data;
                    ytd->ywd   = INST_DATA( ((struct nucleusdata *)data)->o_Class,
                                 data);
                    break;

                case YTA_Tip:

                    if( data )
                        ytd->flags |= YTF_Tip;
                    else
                        ytd->flags &= ~YTF_Tip;
                    break;

            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yt_setAttrs(Object *o, struct ypatank_data *ytd, struct TagItem *attrs)
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
                
                case YTA_Tip:

                    if( data )
                        ytd->flags |= YTF_Tip;
                    else
                        ytd->flags &= ~YTF_Tip;
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yt_getAttrs(Object *o, struct ypatank_data *ytd, struct TagItem *attrs)
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

                case YTA_Tip:

                    if( ytd->flags & YTF_Tip )
                        *value = (LONG) TRUE;
                    else
                        *value = (LONG) FALSE;
                    break;

            };
        };
    };

    /*** Ende ***/
}

