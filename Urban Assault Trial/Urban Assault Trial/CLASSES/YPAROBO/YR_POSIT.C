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
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


_dispatcher(BOOL, yr_YBM_SETPOSITION, struct setposition_msg *setpos)
{
    /*
    ** FUNCTION:    Setzt ein Objekt auf eine Position. Dabei sehen wir nach, ob
    **              es schon in einer BactList ist, nehmen es dort raus, klinken
    **              es in die neue Liste ein und setzen den Sektorpointer.
    **              Evtl. wird das Objekt, wenn es gewünscht ist, noch auf
    **              Bodenhöhe gesetzt.
    **              Zusätzlich Position halten
    **
    ** RESULTS      TRUE, wenn die Position ok war, sonst FALSE
    **
    ** CHANGES      24.9.95 kriäted af
    **
    */

    struct yparobo_data *yrd;
    int    i;

    yrd = INST_DATA( cl, o );

    /*** erstmal nach oben weitergeben ***/
    if( !_supermethoda( cl, o, YBM_SETPOSITION, setpos))  return( FALSE );

    /*** Kraft einstellen ***/
    yrd->buoyancy = yrd->bact->mass * GRAVITY;

    /*** Meine Position den Kanonen mitteilen ***/
    for( i = 0; i < NUMBER_OF_GUNS; i++ ) {

        struct setposition_msg sp;

        if( yrd->gun[ i ].go ) {

            sp.x = yrd->bact->pos.x + yrd->bact->dir.m11 * yrd->gun[ i ].pos.x +
                                      yrd->bact->dir.m21 * yrd->gun[ i ].pos.y +
                                      yrd->bact->dir.m31 * yrd->gun[ i ].pos.z;
            sp.y = yrd->bact->pos.y + yrd->bact->dir.m12 * yrd->gun[ i ].pos.x +
                                      yrd->bact->dir.m22 * yrd->gun[ i ].pos.y +
                                      yrd->bact->dir.m32 * yrd->gun[ i ].pos.z;
            sp.z = yrd->bact->pos.z + yrd->bact->dir.m13 * yrd->gun[ i ].pos.x +
                                      yrd->bact->dir.m23 * yrd->gun[ i ].pos.y +
                                      yrd->bact->dir.m33 * yrd->gun[ i ].pos.z;
            sp.flags = YGFSP_SetFree;
            
            _methoda( yrd->gun[ i ].go, YBM_SETPOSITION, &sp );
            }
        }

    yrd->merke_y = yrd->bact->pos.y;

    return( TRUE );

}


_dispatcher(void, yr_YBM_CHECKPOSITION, void *nix)
{
}



