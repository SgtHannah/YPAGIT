/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**
**  (C) Copyright 1995 by 8100 000C
*/
#include <exec/types.h>

#include <math.h>
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"
#include "ypa/ypaufoclass.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


/*-----------------------------------------------------------------*/
_dispatcher(void, yu_YBM_MOVE, struct move_msg *move)
{
/*
**  FUNCTION
**
**
**  INPUTS
**
**      Eine Schubkraft und die Zeit seit dem letzten Frame
**
**  RESULTS
**
**      ---
**
**  CHANGED
**      29-Jun-95   8100000C    created
*/

    struct ypaufo_data *yud;
    struct uvec grav;       /* Gravitations-Kraftvektor */
    struct uvec schub;      /* Schub-Kraftvektor */
    struct uvec lw;         /* Luftwiederstand */
    struct uvec auf;        /* Auftrieb */
    struct uvec res;
    FLOAT  delta_betrag, new_v_x, new_v_y, new_v_z, new_v_v;
    struct flt_triple weg;
    FLOAT  verhaeltnis, bv, max;

    yud = INST_DATA( cl, o);

    yud->bact->old_pos = yud->bact->pos;

    
    /*
    ** Wir müssen die Fahrtrichtung auch in y-Richtung beachten, denn
    ** bergauf muß es schon schwerer sein.
    */



    /*** Gravitation wirkt immer nach unten ***/
    grav.x = 0.0;
    grav.y = 1.0;
    grav.z = 0.0;
    if( ACTION_DEAD == yud->bact->MainState )
        grav.v = GRAV_FACTOR * GRAVITY * yud->bact->mass;
    else
        grav.v = GRAVITY * yud->bact->mass;

    /*** Schub wirkt in Richtung lokale x-z-Ebene ***/
    if( move->flags & (UBYTE) MOVE_NOFORCE ) {
        schub.x = 0;
        schub.y = 0;
        schub.z = 0;
        schub.v = 0;
        }
    else {

        FLOAT b = nc_sqrt( yud->bact->dir.m31 * yud->bact->dir.m31 +
                           yud->bact->dir.m33 * yud->bact->dir.m33 );

        if( b > 0.001 ) {
            schub.x = yud->bact->dir.m31 / b;
            schub.y = 0;
            schub.z = yud->bact->dir.m33 / b;
            schub.v = yud->bact->act_force;
            }
        else {

            schub.x = -yud->bact->dir.m21;
            schub.y = 0;
            schub.z = -yud->bact->dir.m23;
            schub.v = yud->buoyancy;
            }
        }

    /*** Auftrieb wirkt in Richtung globale y-Achse ***/
    if( move->flags & (UBYTE) MOVE_NOFORCE ) {
        auf.x = 0;
        auf.y = 0;
        auf.z = 0;
        auf.v = 0;
        }
    else {
        auf.x =  0.0;
        auf.y = -1.0;
        auf.z =  0.0;
        auf.v =  yud->buoyancy;
        }

    /*** Luftwiderstand ***/
    lw.x = -yud->bact->dof.x;
    lw.y = -yud->bact->dof.y;
    lw.z = -yud->bact->dof.z;
    lw.v = fabs(yud->bact->dof.v) * yud->bact->air_const;

    /*** resultierender Kraftvektor ***/
    res.x = (grav.x*grav.v)+(schub.x*schub.v)+(lw.x*lw.v)+(auf.x*auf.v);
    res.y = (grav.y*grav.v)+(schub.y*schub.v)+(lw.y*lw.v)+(auf.y*auf.v);
    res.z = (grav.z*grav.v)+(schub.z*schub.v)+(lw.z*lw.v)+(auf.z*auf.v);
    res.v = nc_sqrt(res.x*res.x + res.y*res.y + res.z*res.z);

    /*** neuer dof - zum alten aufaddieren ***/
    
    if( res.v > 0.0 ) {

        delta_betrag = (res.v / yud->bact->mass) * move->t;
        new_v_x = yud->bact->dof.x * yud->bact->dof.v + (res.x / res.v ) * delta_betrag;
        new_v_y = yud->bact->dof.y * yud->bact->dof.v + (res.y / res.v ) * delta_betrag;
        new_v_z = yud->bact->dof.z * yud->bact->dof.v + (res.z / res.v ) * delta_betrag;
        new_v_v = nc_sqrt( new_v_x * new_v_x + new_v_y * new_v_y + new_v_z * new_v_z );
        
        /* wir haben Zwangskräfte. Das Heißt, wir nehmen nur den Teil
        ** der Geschwindigkeit, der in lokale z-Richtung zeigt. Die dof-
        ** Richtung ist also die lokale z_richtung und der dof-Betrag ist
        ** das Skalarprodukt aus Richtung der Geschwindigkeit und Fahrt-
        ** richtung.
        */
        if( new_v_v > 0.0 ) {
            new_v_x /= new_v_v;
            new_v_y /= new_v_v;
            new_v_z /= new_v_v;
            }

        yud->bact->dof.x = new_v_x;
        yud->bact->dof.y = new_v_y;
        yud->bact->dof.z = new_v_z;
        yud->bact->dof.v = new_v_v;
        }


    /*
    ** Jetzt haben wir eine Neue Geschwindigkeit, da müssen wir aber noch
    ** die Position neu setzen. Die Ausrichtung wurde schon erledigt...
    */

    weg.x = yud->bact->dof.x * yud->bact->dof.v * move->t * METER_SIZE;
    weg.y = yud->bact->dof.y * yud->bact->dof.v * move->t * METER_SIZE;
    weg.z = yud->bact->dof.z * yud->bact->dof.v * move->t * METER_SIZE;
    
    if( fabs(yud->bact->dof.v) > IS_SPEED ) {

        yud->bact->pos.x += weg.x;
        yud->bact->pos.y += weg.y;
        yud->bact->pos.z += weg.z;
        }

    /*** ...der Weg ***/
    yud->gone -= nc_sqrt( weg.x*weg.x + weg.y*weg.y + weg.z*weg.z );
    if( yud->gone < 0.0 ) 
        yud->gone = 0.0;

    _methoda( o, YBM_CORRECTPOSITION, NULL );
    
    /*** Alte Sachen vor Veränderung ***/
    yud->bact->sc.src[ VP_NOISE_NORMAL ].pitch  = yud->bact->bu_pitch;
    yud->bact->sc.src[ VP_NOISE_NORMAL ].volume = yud->bact->bu_volume;

    /*** Wir beachten act_force und buoyancy ***/

    // verhaeltnis = yud->bact->act_force / yud->bact->max_force;
    //
    // /*** Maximalgrenzen für den Auftrieb gibt es nicht ***/
    // bv = 1 + yud->buoyancy / ( 8 * yud->bact->mass * GRAVITY );
    // if( bv < 0.0 ) bv = 0.0;
    // verhaeltnis += bv;
    
    verhaeltnis = fabs( yud->bact->dof.v) / 
                  ( yud->bact->max_force / yud->bact->bu_air_const );

    if( yud->bact->max_pitch > -0.8 )
        max = yud->bact->max_pitch;
    else
        max = MSF_UFO;

    verhaeltnis *= max;
    if( verhaeltnis > max ) verhaeltnis = max;

    if( yud->bact->sc.src[ VP_NOISE_NORMAL ].sample )
        yud->bact->sc.src[ VP_NOISE_NORMAL ].pitch = (LONG)
            ( ((FLOAT) yud->bact->sc.src[ VP_NOISE_NORMAL ].pitch +
               (FLOAT) yud->bact->sc.src[ VP_NOISE_NORMAL ].sample->SamplesPerSec ) *
                verhaeltnis );
}


