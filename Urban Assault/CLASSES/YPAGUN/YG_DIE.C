/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus


/*-----------------------------------------------------------------*/

_dispatcher( void, yg_YBM_DIE, void *nix)
{
/*
**  FUNCTION    Überlagert die, denn zusätzlich muß die Position
**              der Kinder wieder "relativiert" werden und das Follow
**              mother-Flag gesetzt werden. Das gilt für alle VP
**
**  CHANGED     8100 000C created 27-Feb
*/

    struct handlevischildren_msg hvc;
    struct ypagun_data *ygd;

    ygd = INST_DATA( cl, o);
    
    if( ygd->bact->ExtraState & EXTRA_LOGICDEATH )
        return;

    /*** Hochreichen ***/
    _supermethoda( cl, o, YBM_DIE, NULL );

    /*** Kinder auf relative Position ***/
    hvc.job = HVC_CONNECTCHILDREN;
    _methoda( o, YBM_HANDLEVISCHILDREN, &hvc );

    /*** Wenn ich Bordkanone bin, dann bei meinem Robo abmelden ***/
    if( ygd->flags & GUN_RoboGun ) {

        struct gun_data *g_array;
        int    i;

        _get( ygd->bact->robo, YRA_GunArray, &g_array );

        for( i = 0; i < NUMBER_OF_GUNS; i++ )
            if( g_array[ i ].go == o )  g_array[ i ].go = NULL;
        }
}

_dispatcher( void, yg_YBM_REINCARNATE, void *nix )
{
/*
**  FUNCTION    Hier origpos
**
**  REST        wie immer
*/

    struct ypagun_data *ygd = INST_DATA( cl, o );

    _supermethoda( cl, o, YBM_REINCARNATE, NULL );

    ygd->flags       = 0;
    ygd->ground_time = 0;

    /* -----------------------------------------------------------------
    ** Flaks bekommen einen ExtraViewer (normal auf relativ (0;0;0), mit
    ** dem wir das Rütteln machen
    ** ---------------------------------------------------------------*/
    _set(o, YBA_ExtraViewer, TRUE );
    ygd->bact->Viewer.relpos.x = 0.0;
    ygd->bact->Viewer.relpos.y = 0.0;
    ygd->bact->Viewer.relpos.z = 0.0;
    ygd->bact->Viewer.dir      = ygd->bact->dir;
}
