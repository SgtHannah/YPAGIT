/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Attribut-Handling für button.class.
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
#include "requester/buttonclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine

/*-----------------------------------------------------------------*/
BOOL bt_initAttrs(Object *o, struct button_data *btd, struct TagItem *attrs)
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

    /*** Default-Werte ***/
    btd->ch_bopen       = BTC_CH_BOPEN;
    btd->ch_bclose      = BTC_CH_BCLOSE;
    btd->ch_bfill       = BTC_CH_BFILL;
    btd->ch_bslideropen = BTC_CH_BSLIDEROPEN;
    btd->ch_bsliderback = BTC_CH_BSLIDERBACK;
    btd->ch_bsliderclose= BTC_CH_BSLIDERCLOSE;


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

                case BTA_BoxX:

                    btd->click.rect.x = (WORD) data;
                    break;

                case BTA_BoxY:

                    btd->click.rect.y = (WORD) data;
                    break;

                case BTA_BoxW:

                    btd->click.rect.w = (WORD) data;
                    break;

                case BTA_BoxH:

                    btd->click.rect.h = (WORD) data;
                    break;

                case BTA_Chars:

                    btd->ch_bopen  = ((char *)data)[ 0 ];
                    btd->ch_bclose = ((char *)data)[ 1 ];
                    btd->ch_bfill  = ((char *)data)[ 2 ];
                    break;
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void bt_setAttrs(Object *o, struct button_data *btd, struct TagItem *attrs)
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

                case BTA_BoxX:

                    btd->click.rect.x = (WORD) data;
                    break;

                case BTA_BoxY:

                    btd->click.rect.y = (WORD) data;
                    break;

                case BTA_BoxW:

                    btd->click.rect.w = (WORD) data;
                    break;

                case BTA_BoxH:

                    btd->click.rect.h = (WORD) data;
                    break;

                case BTA_Chars:

                    btd->ch_bopen  = ((char *)data)[ 0 ];
                    btd->ch_bclose = ((char *)data)[ 1 ];
                    btd->ch_bfill  = ((char *)data)[ 2 ];
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void bt_getAttrs(Object *o, struct button_data *btd, struct TagItem *attrs)
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

                case BTA_BoxX:

                    *value = (LONG)btd->click.rect.x;
                    break;

                case BTA_BoxY:

                    *value = (LONG)btd->click.rect.y;
                    break;

                case BTA_BoxW:

                    *value = (LONG)btd->click.rect.w;
                    break;

                case BTA_BoxH:

                    *value = (LONG)btd->click.rect.h;
                    break;

                case BTA_InstData:

                    *value = (LONG) btd;
                    break;
            };
        };
    };

    /*** Ende ***/
}

