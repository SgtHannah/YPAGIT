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
#include "requester/requesterclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine

/*-----------------------------------------------------------------*/
BOOL rq_initAttrs(Object *o, struct requester_data *rqd, struct TagItem *attrs)
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
    rqd->ch_rup         = RQC_CH_RUP;
    rqd->ch_rdown       = RQC_CH_RDOWN;
    rqd->ch_rleft       = RQC_CH_RLEFT;
    rqd->ch_rright      = RQC_CH_RRIGHT;
    rqd->ch_rupleft     = RQC_CH_RUPLEFT;
    rqd->ch_rupright    = RQC_CH_RUPRIGHT;
    rqd->ch_rdownleft   = RQC_CH_RDOWNLEFT;
    rqd->ch_rdownright  = RQC_CH_RDOWNRIGHT;
    rqd->ch_rbackround  = RQC_CH_RBACKROUND;
    rqd->ac_rup         = RQC_AC_RUP;
    rqd->ac_rdown       = RQC_AC_RDOWN;
    rqd->ac_rleft       = RQC_AC_RLEFT;
    rqd->ac_rright      = RQC_AC_RRIGHT;
    rqd->ac_rupleft     = RQC_AC_RUPLEFT;
    rqd->ac_rupright    = RQC_AC_RUPRIGHT;
    rqd->ac_rdownleft   = RQC_AC_RDOWNLEFT;
    rqd->ac_rdownright  = RQC_AC_RDOWNRIGHT;
    rqd->ac_rbackround  = RQC_AC_RBACKROUND;

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

                case RQA_Requester:

                    rqd->requester = (Object *) data;
                    break;

                case RQA_Icon:

                    rqd->icon = (Object *) data;
                    break;

                case RQA_Active:

                    if( data )
                        rqd->flags |=  RF_ACTIVE;
                    else
                        rqd->flags &= ~RF_ACTIVE;
                    break;

                case RQA_Chars:
    
                    rqd->ch_rup         = ((char *)data)[ 0 ];
                    rqd->ch_rdown       = ((char *)data)[ 1 ];
                    rqd->ch_rleft       = ((char *)data)[ 2 ];
                    rqd->ch_rright      = ((char *)data)[ 3 ];
                    rqd->ch_rupleft     = ((char *)data)[ 4 ];
                    rqd->ch_rupright    = ((char *)data)[ 5 ];
                    rqd->ch_rdownleft   = ((char *)data)[ 6 ];
                    rqd->ch_rdownright  = ((char *)data)[ 7 ];
                    rqd->ch_rbackround  = ((char *)data)[ 8 ];
                    rqd->ac_rup         = ((char *)data)[ 9 ];
                    rqd->ac_rdown       = ((char *)data)[ 10 ];
                    rqd->ac_rleft       = ((char *)data)[ 12 ];
                    rqd->ac_rright      = ((char *)data)[ 13 ];
                    rqd->ac_rupleft     = ((char *)data)[ 14 ];
                    rqd->ac_rupright    = ((char *)data)[ 15 ];
                    rqd->ac_rdownleft   = ((char *)data)[ 16 ];
                    rqd->ac_rdownright  = ((char *)data)[ 17 ];
                    rqd->ac_rbackround  = ((char *)data)[ 18 ];
                    break;

                case RQA_Font:

                    rqd->rq_font = (UBYTE) data;
                    break;

                case RQA_MoveBorderU:

                    rqd->moveborder_u = (WORD) data;
                    break;

                case RQA_MoveBorderD:

                    rqd->moveborder_d = (WORD) data;
                    break;

                case RQA_MoveBorderL:

                    rqd->moveborder_l = (WORD) data;
                    break;

                case RQA_MoveBorderR:

                    rqd->moveborder_r = (WORD) data;
                    break;
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void rq_setAttrs(Object *o, struct requester_data *rqd, struct TagItem *attrs)
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

                case RQA_Active:

                    if( data )
                        rqd->flags |=  RF_ACTIVE;
                    else
                        rqd->flags &= ~RF_ACTIVE;
                    break;

                case RQA_Chars:
    
                    rqd->ch_rup         = ((char *)data)[ 0 ];
                    rqd->ch_rdown       = ((char *)data)[ 1 ];
                    rqd->ch_rleft       = ((char *)data)[ 2 ];
                    rqd->ch_rright      = ((char *)data)[ 3 ];
                    rqd->ch_rupleft     = ((char *)data)[ 4 ];
                    rqd->ch_rupright    = ((char *)data)[ 5 ];
                    rqd->ch_rdownleft   = ((char *)data)[ 6 ];
                    rqd->ch_rdownright  = ((char *)data)[ 7 ];
                    rqd->ch_rbackround  = ((char *)data)[ 8 ];
                    rqd->ac_rup         = ((char *)data)[ 9 ];
                    rqd->ac_rdown       = ((char *)data)[ 10 ];
                    rqd->ac_rleft       = ((char *)data)[ 12 ];
                    rqd->ac_rright      = ((char *)data)[ 13 ];
                    rqd->ac_rupleft     = ((char *)data)[ 14 ];
                    rqd->ac_rupright    = ((char *)data)[ 15 ];
                    rqd->ac_rdownleft   = ((char *)data)[ 16 ];
                    rqd->ac_rdownright  = ((char *)data)[ 17 ];
                    rqd->ac_rbackround  = ((char *)data)[ 18 ];
                    break;

                case RQA_Font:

                    rqd->rq_font = (UBYTE) data;
                    break;

                case RQA_MoveBorderU:

                    rqd->moveborder_u = (WORD) data;
                    break;

                case RQA_MoveBorderD:

                    rqd->moveborder_d = (WORD) data;
                    break;

                case RQA_MoveBorderL:

                    rqd->moveborder_l = (WORD) data;
                    break;

                case RQA_MoveBorderR:

                    rqd->moveborder_r = (WORD) data;
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void rq_getAttrs(Object *o, struct requester_data *rqd, struct TagItem *attrs)
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
    LONG pos;

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

                case RQA_Iconified:

                    /*** Mal fragen, ob der Requester gerade iconifiziert ist ***/
                    if( rqd->flags & RF_ICONIFIED )
                        *value = 1L;
                    else
                        *value = 0L;
                    break;

                case RQA_Open:

                    /*** Mal fragen, ob der Requester gerade offen ist ***/
                    if( rqd->flags & RF_PUBLISH )
                        *value = 1L;
                    else
                        *value = 0L;
                    break;

                case RQA_Active:

                    /*** Mal fragen, ob der Requester gerade aktiv ist ***/
                    if( rqd->flags & RF_ACTIVE )
                        *value = 1L;
                    else
                        *value = 0L;
                    break;

                case RQA_Font:

                    *value = (ULONG)rqd->rq_font;
                    break;

                case RQA_PosX:

                    _get( rqd->requester, BTA_BoxX, &pos );
                    *value = pos;
                    break;

                case RQA_PosY:

                    _get( rqd->requester, BTA_BoxY, &pos );
                    *value = pos;
                    break;

                case RQA_MoveBorderU:

                    *value = (LONG)rqd->moveborder_u;
                    break;

                case RQA_MoveBorderD:

                    *value = (LONG)rqd->moveborder_d;
                    break;

                case RQA_MoveBorderL:

                    *value = (LONG)rqd->moveborder_l;
                    break;

                case RQA_MoveBorderR:

                    *value = (LONG)rqd->moveborder_r;
                    break;
            };
        };
    };

    /*** Ende ***/
}

