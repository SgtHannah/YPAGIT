/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "input/input.h"
#include "ypa/ypatankclass.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


_dispatcher(BOOL, yt_YBM_SETPOSITION, struct setposition_msg *setpos)
{
    /*
    ** FUNCTION:    Setzt ein Objekt auf eine Position. Dabei sehen wir nach, ob
    **              es schon in einer BactList ist, nehmen es dort raus, klinken
    **              es in die neue Liste ein und setzen den Sektorpointer.
    **              Evtl. wird das Objekt, wenn es gewünscht ist, noch auf
    **              Bodenhöhe gesetzt.
    **
    ** RESULTS      TRUE, wenn die Position ok war, sonst FALSE
    **
    ** CHANGES      24.9.95 kriäted af
    **              5.2.96  Panzer nicht auf Gebäude setzen
    **
    */

    struct ypatank_data *ytd;
    struct intersect_msg inter;
    ULONG  VIEW;

    ytd = INST_DATA( cl, o );

    /*** Das sieht gut aus. Zum Einklinken hochreichen ***/
    if( !_supermethoda( cl, o, YBM_SETPOSITION, setpos))   return( FALSE );

    /*** Verlangen die Flags noch'n bissel Schnickschnack ***/
    if( setpos->flags & YBFSP_SetGround ) {

        /*** Zuerst 'ne Intersection ***/
        inter.pnt.x = ytd->bact->pos.x;
        inter.pnt.z = ytd->bact->pos.z;
        inter.pnt.y = -30000.0;
        inter.vec.x = 0.0;
        inter.vec.z = 0.0;
        inter.vec.y = 50000.0;
        inter.flags = 0;
        _methoda( ytd->world, YWM_INTERSECT, &inter);

        if( inter.insect ) {

            /*** y-Korrektur ***/
            _get( o, YBA_Viewer, &VIEW);
            if( VIEW )
                ytd->bact->pos.y = inter.ipnt.y - ytd->bact->viewer_over_eof;
            else
                ytd->bact->pos.y = inter.ipnt.y - ytd->bact->over_eof;
            ytd->bact->ExtraState |= EXTRA_LANDED;
            return( TRUE );
            }
        else {

            /*** da war'n Loch inner Welt ***/
            return( FALSE );
            }
        }
    else
        return( TRUE );
}


_dispatcher( void, yt_YBM_CHECKPOSITION, void *nix )
{
    struct ypatank_data *ytd;

    ytd = INST_DATA( cl, o );

    _supermethoda( cl, o, YBM_CHECKPOSITION, NULL );

    /*** evtl. noch 'ne Bodenausrichtung ***/
    if( ytd->bact->ExtraState & EXTRA_LANDED ) {

        ULONG  VIEW;
        FLOAT  over_eof;
        struct intersect_msg inter;

        _get( o, YBA_Viewer, &VIEW );

        if( VIEW )
            over_eof = ytd->bact->viewer_over_eof;
        else
            over_eof = ytd->bact->over_eof;

        inter.pnt.x = ytd->bact->pos.x;
        inter.pnt.y = ytd->bact->pos.y;
        inter.pnt.z = ytd->bact->pos.z;
        inter.vec.x = 0.0;
        inter.vec.y = 2 * over_eof;
        inter.vec.z = 0.0;
        inter.flags = 0;
        _methoda( ytd->world, YWM_INTERSECT, &inter );
        if( inter.insect ) 
            ytd->bact->pos.y = inter.ipnt.y - over_eof;
        else
            ytd->bact->ExtraState &= ~EXTRA_LANDED;
        }
}



