/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**
**  (C) Copyright 1995 by Andreas Flemming
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
#include "ypa/ypaflyerclass.h"
#include "input/input.h"
#include "audio/audioengine.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine




/*-----------------------------------------------------------------
** Brot-O-Typen
*/
void yf_DrehObjektInFlugrichtung( struct ypaflyer_data *yfd, FLOAT time,
                                  struct flt_triple *old_dir, BOOL view );
void yf_CalculateForce( struct ypaflyer_data *yfd, FLOAT time );
void yf_rot_round_lokal_x( struct Bacterium *Be, FLOAT angle );
void yf_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yf_rot_round_lokal_z( struct Bacterium *Be, FLOAT angle );
void yf_rot_round_global_y( struct Bacterium *Be, FLOAT angle );
void yf_rot_round_horiz_x( struct Bacterium *Be, FLOAT angle );




/*-----------------------------------------------------------------*/
_dispatcher(void, yf_YBM_MOVE, struct move_msg *move)
{
/*
**  FUNCTION
**
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

    struct ypaflyer_data *yfd;


    struct uvec grav;       /* Gravitations-Kraftvektor */
    struct uvec schub;      /* Schub-Kraftvektor */
    struct uvec lw;         /* Luftwiederstand */
    struct uvec res;
    struct uvec auf;
    FLOAT  delta_betrag, new_v_x, new_v_y, new_v_z;
    FLOAT  verhaeltnis, max;

    yfd = INST_DATA( cl, o);


    /*
    ** Die alte Pos merken, wo sie verändert wird!! also hier!! sonst
    ** bekommt Kollisionsverhütung Probleme!!!!
    */
    yfd->bact->old_pos = yfd->bact->pos;


    /*** Gravitation wirkt immer nach unten ***/
    grav.x = 0.0;
    grav.y = 1.0;
    grav.z = 0.0;
    if( ACTION_DEAD == yfd->bact->MainState )
        grav.v = GRAV_FACTOR * GRAVITY * yfd->bact->mass;
    else
        grav.v = GRAVITY * yfd->bact->mass;

    /*** Schub wirkt in Richtung lokal_z ***/
    if( move->flags & MOVE_NOFORCE ) {

        schub.x = 0.0;
        schub.y = 0.0;
        schub.z = 0.0;
        schub.v = 0.0;
        }
    else {

        schub.x = yfd->bact->dir.m31;       // ohne Korrektur, da haben wir
        schub.y = 0.0;                      // so eine Art Projektion. Mal sehen
        schub.z = yfd->bact->dir.m33;       // was das bringt
        schub.v = yfd->bact->act_force;
        }

    /*** Auftrieb extra ***/
    if( move->flags & MOVE_NOFORCE ) {

        auf.x = 0.0;
        auf.y = 0.0;
        auf.z = 0.0;
        auf.v = 0.0;
        }
    else {

        auf.x =  0.0;
        auf.y = -1.0;
        auf.z =  0.0;
        auf.v = yfd->buoyancy;
        }

    /*** Luftwiderstand ***/
    lw.x = -yfd->bact->dof.x;
    lw.y = -yfd->bact->dof.y;
    lw.z = -yfd->bact->dof.z;
    lw.v = yfd->bact->dof.v * yfd->bact->air_const;

    /*** resultierender Kraftvektor ***/
    res.x = (grav.x*grav.v)+(schub.x*schub.v)+(lw.x*lw.v)+(auf.x*auf.v);
    res.y = (grav.y*grav.v)+(schub.y*schub.v)+(lw.y*lw.v)+(auf.y*auf.v);
    res.z = (grav.z*grav.v)+(schub.z*schub.v)+(lw.z*lw.v)+(auf.z*auf.v);
    res.v = nc_sqrt(res.x*res.x + res.y*res.y + res.z*res.z);

    /*** neuer dof - zum alten aufaddieren ***/
    
    if( res.v > 0.0 ) {

        delta_betrag = (res.v / yfd->bact->mass) * move->t;
        new_v_x = yfd->bact->dof.x * yfd->bact->dof.v + (res.x / res.v ) * delta_betrag;
        new_v_y = yfd->bact->dof.y * yfd->bact->dof.v + (res.y / res.v ) * delta_betrag;
        new_v_z = yfd->bact->dof.z * yfd->bact->dof.v + (res.z / res.v ) * delta_betrag;

        yfd->bact->dof.v = nc_sqrt( new_v_x * new_v_x + new_v_y * new_v_y +
                                    new_v_z * new_v_z );
        if( yfd->bact->dof.v > 0.0 ) {
            yfd->bact->dof.x = new_v_x / yfd->bact->dof.v;
            yfd->bact->dof.y = new_v_y / yfd->bact->dof.v;
            yfd->bact->dof.z = new_v_z / yfd->bact->dof.v;
            }
        }


    /*
    ** Jetzt haben wir eine Neue Geschwindigkeit, da müssen wir aber noch
    ** die Position neu setzen. Die Ausrichtung wurde schon erledigt...
    */
    if( yfd->bact->dof.v > 1.5 ) {

        yfd->bact->pos.x += yfd->bact->dof.x * yfd->bact->dof.v * move->t * METER_SIZE;
        yfd->bact->pos.y += yfd->bact->dof.y * yfd->bact->dof.v * move->t * METER_SIZE;
        yfd->bact->pos.z += yfd->bact->dof.z * yfd->bact->dof.v * move->t * METER_SIZE;
        }

    /*** Nun lasset uns noch testen, ob wir nicht über die Welt geflogen sind ***/
    _methoda( o, YBM_CORRECTPOSITION, NULL );

    /*** Alte Sachen vor Veränderung ***/
    yfd->bact->sc.src[ VP_NOISE_NORMAL ].pitch  = yfd->bact->bu_pitch;
    yfd->bact->sc.src[ VP_NOISE_NORMAL ].volume = yfd->bact->bu_volume;

    /*** Wir beachten act_force und buoyancy ***/

    // verhaeltnis = yfd->bact->act_force / yfd->bact->max_force;
    //
    // /*** Maximalgrenzen für den Auftrieb gibt es nicht ***/
    // bv = 1 + yfd->buoyancy / ( 8 * yfd->bact->mass * GRAVITY );
    // if( bv < 0.0 ) bv = 0.0;
    // verhaeltnis += bv;
    
    verhaeltnis = fabs( yfd->bact->dof.v ) / 
                  (yfd->bact->max_force / yfd->bact->bu_air_const );

    if( yfd->bact->max_pitch > -0.8 )
        max = yfd->bact->max_pitch;
    else
        max = MSF_FLYER;

    verhaeltnis *= max;
    if( verhaeltnis > max ) verhaeltnis = max;

    if( yfd->bact->sc.src[ VP_NOISE_NORMAL ].sample )
        yfd->bact->sc.src[ VP_NOISE_NORMAL ].pitch += (LONG)
            ( ((FLOAT) yfd->bact->sc.src[ VP_NOISE_NORMAL ].pitch +
               (FLOAT) yfd->bact->sc.src[ VP_NOISE_NORMAL ].sample->SamplesPerSec) *
               verhaeltnis );
}


_dispatcher( void, yf_YBM_STOPMACHINE, struct trigger_logic_msg *msg )
{
/*
**  FUNCTION    Zur Ruhe bringen
**
**  INPUT       trigger_logic_msg
**
**  RESULT      nüscht!
*/

    struct ypaflyer_data *yfd;

    yfd = INST_DATA( cl, o);

    _supermethoda( cl, o, YBM_STOPMACHINE, msg );

    /*** noch ein paar Flyer-Spezialitäten ***/
    yfd->bact->act_force = 0.0;
    yfd->buoyancy        = yfd->bact->mass * GRAVITY;
}



void yf_CalculateForce( struct ypaflyer_data *yfd, FLOAT time )
{
    /*
    ** Berechnet Auftrieb und Kraftrichtung nach den Wünschen des
    ** tar_unit. Allerdings kann ein Flugzeug sich nicht so schnell drehen
    ** wie ein Hubschrauber. Wir nehmen der Einfachheit halber aber
    ** die HS-Routinen. Deshalb modifizieren nachher...
    */

    FLOAT angle;

    /*** Wollen wir irgendwohin außer nach oben ode unten? ***/
    if( (yfd->bact->tar_unit.x != 0.0) || (yfd->bact->tar_unit.z != 0.0) ) {
    
        angle = nc_acos((yfd->bact->tar_unit.x * yfd->bact->dir.m31 +
                         yfd->bact->tar_unit.z * yfd->bact->dir.m33)/
                         nc_sqrt(yfd->bact->tar_unit.x*yfd->bact->tar_unit.x+
                                 yfd->bact->tar_unit.z*yfd->bact->tar_unit.z)/
                         nc_sqrt(yfd->bact->dir.m31*yfd->bact->dir.m31+
                                 yfd->bact->dir.m33*yfd->bact->dir.m33));

        if( fabs(angle) > yfd->bact->max_rot * time )
            angle = yfd->bact->max_rot * time;

        if( (yfd->bact->dir.m33*yfd->bact->tar_unit.x -
             yfd->bact->dir.m31*yfd->bact->tar_unit.z) > 0.0 )
            angle = -angle;

        yf_rot_round_lokal_y( yfd->bact, angle );
        }

    /*** Die Kraft ***/
    yfd->bact->act_force = yfd->bact->max_force;
    
    /*** nach oben oder unten? ***/
    if( fabs(yfd->bact->tar_unit.y) < 0.1 )
        yfd->buoyancy = yfd->bact->mass * GRAVITY;
    else
        if( yfd->bact->tar_unit.y <= -0.1 )
            yfd->buoyancy = 2.5 * yfd->bact->mass * GRAVITY;
        else
            yfd->buoyancy = 0.7 * yfd->bact->mass * GRAVITY;

    /*** Wesentlich stärker als tar_unit ist das Ausweichen, welches ***/
    /*** hier nur nach oben bedeuten kann                            ***/

    if( yfd->bact->ExtraState & EXTRA_OVER )
        yfd->buoyancy = 2.5 * yfd->bact->mass * GRAVITY;


}


void yf_DrehObjektInFlugrichtung( struct ypaflyer_data *yfd, FLOAT time,
                                  struct flt_triple *old_dir, BOOL VIEW )
{

    /*
    ** Das Flugobjekt wird um die lokale z-Achse und  um die xz-
    ** Projektion der lokalen x-Achse gedreht, wenn dies gewünscht ist.
    */
    struct flt_triple new_dir;
    FLOAT  xz_angle, side_angle_now, side_angle_wish, side_angle_rot, factor;
    FLOAT  y_angle_now, y_angle_wish, y_angle_rot;
    FLOAT  max_speed;
            
    max_speed = yfd->bact->max_force / yfd->bact->air_const;

    /*** Winkelbegrenzungen ***/
    #define WINKEL_NACH_OBEN        (0.8)
    #define WINKEL_NACH_UNTEN       (1.2)
    #define MAX_AP_NEIGUNG          (1.45)
    
    if( (yfd->bact->dof.x==0.0)&&(yfd->bact->dof.y==0.0)&&(yfd->bact->dof.z==0.0))
        return;

    if( (old_dir->x==0.0) && (old_dir->y==0.0) && (old_dir->z==0.0) ) {

        old_dir->x = yfd->bact->dir.m31;
        old_dir->y = yfd->bact->dir.m32;
        old_dir->z = yfd->bact->dir.m33;
        }

    new_dir.x = yfd->bact->dir.m31;
    new_dir.y = yfd->bact->dir.m32;
    new_dir.z = yfd->bact->dir.m33;
    
    /*** aktuelle Neigungen ***/
    side_angle_now = nc_asin( yfd->bact->dir.m12 );
    y_angle_now = nc_asin( yfd->bact->dir.m32 );


    /*** Nach oben/unten? ***/
    if( (yfd->flight_type & YFF_RotX) && (yfd->bact->dof.v >= IS_SPEED) )
        y_angle_wish = nc_asin( yfd->bact->dof.y ) * yfd->bact->dof.v / max_speed;
    else
        y_angle_wish = 0;

    if( y_angle_wish < 0 ) {

        if( y_angle_wish < -WINKEL_NACH_OBEN )
            y_angle_wish = -WINKEL_NACH_OBEN;
        }
    else {

        if( y_angle_wish >  WINKEL_NACH_UNTEN )
            y_angle_wish =  WINKEL_NACH_UNTEN;
        }

    if( y_angle_now < 0 )
        factor = -0.8 * ( y_angle_now - 0.6 );
    else
        factor =  0.8 * ( y_angle_now + 0.6 );

    y_angle_rot = -( y_angle_wish - y_angle_now );
    if( y_angle_rot < 0.0 ) {
        if( y_angle_rot < -yfd->bact->max_rot * time * factor )
            y_angle_rot = -yfd->bact->max_rot * time * factor;
        else
            if( y_angle_rot > -0.0005 )    y_angle_rot = 0.0;
        }
    else {
        if( y_angle_rot >  yfd->bact->max_rot * time * factor )
            y_angle_rot =  yfd->bact->max_rot * time * factor;
        else
            if( y_angle_rot <  0.0005 )    y_angle_rot = 0.0;
        }

    /*** Rotieren nur, wenn wir fliegen und dann abhängig von dof.v * dof.y ***/
    // y_angle_rot *= yfd->bact->dof.v /
    //                (yfd->bact->max_force / yfd->bact->air_const);
    yf_rot_round_horiz_x( yfd->bact, y_angle_rot);



    /*** Zur Seite ***/
    if( yfd->flight_type & YFF_RotZ ) {

        if( ( (new_dir.x  != 0.0) || (new_dir.z  != 0.0) ) &&
            ( (old_dir->x != 0.0) || (old_dir->z != 0.0) ) ) {

            /*** Wie hat sich die Richtung geändert? ***/
            xz_angle = nc_acos( (new_dir.x*old_dir->x + new_dir.z*old_dir->z ) /
                                nc_sqrt( new_dir.x  * new_dir.x  +
                                         new_dir.z  * new_dir.z  ) /
                                nc_sqrt( old_dir->x * old_dir->x +
                                         old_dir->z * old_dir->z ) );
        
            if( xz_angle < 0.001 ) xz_angle = 0.0;
            if( (new_dir.x * old_dir->z - new_dir.z * old_dir->x) < 0.0 )
                xz_angle = -xz_angle;

            /*** Wie drehen wir nun? ***/
            if( yfd->bact->dof.v >= IS_SPEED ) {

                /*** der Winkel an sich ***/
                side_angle_wish = MAX_AP_NEIGUNG * xz_angle /
                                  ( time * yfd->bact->max_rot);

                /*** evtl. geschwindigkeitsabhängige Nachkorrektur ***/
                side_angle_wish *= fabs(yfd->bact->dof.v)*yfd->bact->air_const/
                                   yfd->bact->max_force;
                }
            else
                side_angle_wish = 0.0;

            //if( side_angle_wish < 0 ) {
            //    if( side_angle_wish < -MAX_AP_NEIGUNG )
            //        side_angle_wish = -MAX_AP_NEIGUNG;
            //    }
            //else {
            //    if( side_angle_wish >  MAX_AP_NEIGUNG )
            //        side_angle_wish =  MAX_AP_NEIGUNG;
            //    }

            /* ---------------------------------------------------------
            ** Geschwindigkeit der Seitenneigung für Viewer und Autonome 
            ** getrennt. Viewer ist schneller
            ** -------------------------------------------------------*/

            if( VIEW ) {

                if( side_angle_now < 0 )
                    factor =  1.0; // -( 3 * side_angle_now - 0.3 );
                else
                    factor =  1.0; //  ( 3 * side_angle_now + 0.3 );
                }
            else {

                if( side_angle_now < 0 )
                    factor = -( side_angle_now - 0.4 );
                else
                    factor =  ( side_angle_now + 0.4 );
                }

            side_angle_rot = side_angle_wish - side_angle_now;
            if( side_angle_rot < 0.0 ) {
                if( side_angle_rot < -yfd->bact->max_rot * time * factor )
                    side_angle_rot = -yfd->bact->max_rot * time * factor;
                else
                    if( side_angle_rot > -0.001 )    side_angle_rot = 0.0;
                }
            else {
                if( side_angle_rot >  yfd->bact->max_rot * time * factor )
                    side_angle_rot =  yfd->bact->max_rot * time * factor;
                else
                    if( side_angle_rot <  0.001 )    side_angle_rot = 0.0;
                }

            /*** Rotieren nur, wenn wir fliegen ***/
            // side_angle_rot *= yfd->bact->dof.v /
            //                  (yfd->bact->max_force / yfd->bact->air_const);
            // if( (yfd->bact->dof.v > 1.0) && (side_angle_rot != 0.0 ) )
            yf_rot_round_lokal_z( yfd->bact, side_angle_rot);
            }
        }
    else {

        /* -----------------------------------------------------------
        ** Wenn dem nicht so ist, muß das Objekt trotzdem aufgerichtet
        ** werden, weil es sich durch Impulse neugen kann.
        ** Einfach um das bereits berechnete side_angle_now drehen.
        ** ---------------------------------------------------------*/
        if( side_angle_now < 0.0 ) {
            if( side_angle_now < -yfd->bact->max_rot * time * factor )
                side_angle_now = -yfd->bact->max_rot * time * factor;
            else
                if( side_angle_now > -0.001 )    side_angle_now = 0.0;
            }
        else {
            if( side_angle_now >  yfd->bact->max_rot * time * factor )
                side_angle_now =  yfd->bact->max_rot * time * factor;
            else
                if( side_angle_now <  0.001 )    side_angle_now = 0.0;
            }

        yf_rot_round_lokal_z( yfd->bact, (FLOAT)( -side_angle_now ));
        }
}


void yf_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle)
/// "rot um z"
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;     rm.m12 = sin_y;     rm.m13 = 0.0;
    rm.m21 = -sin_y;    rm.m22 = cos_y;     rm.m23 = 0.0;
    rm.m31 = 0.0;       rm.m32 = 0.0;       rm.m33 = 1.0;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}
///

void yf_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
/// "rot um x"
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_y;     rm.m23 = sin_y;
    rm.m31 = 0.0;       rm.m32 = -sin_y;    rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}
///

void yf_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
/// "rot um y"
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}
///


void yf_rot_round_global_y( struct Bacterium *bact, FLOAT angle)
/// "rot um y"
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;     rm.m12 = 0.0;      rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;      rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;      rm.m33 = cos_y;
    
    nc_m_mul_m(&(bact->dir), &rm, &neu_dir);

    bact->dir = neu_dir;
}
///

void yf_rot_round_horiz_x( struct Bacterium *bact, FLOAT angle )
{
    /*
    ** Rotiert nicht um die lokale x-Achse, sondern um eine
    ** Achse, die wie die lokale x-Achse verläuft, aber keine y-
    ** Komponente hat.
    */

    FLOAT rot_x, rot_y, rot_z, betrag, ca, sa;
    struct flt_m3x3 nm, rm;

    betrag = nc_sqrt( bact->dir.m11*bact->dir.m11 + bact->dir.m13*bact->dir.m13 );

    if( betrag < 0.001 ) return;

    /*** Nun rot-Achse ermitteln ***/
    rot_x = bact->dir.m11 / betrag;
    rot_y = 0.0;
    rot_z = bact->dir.m13 / betrag;

    /*** Matrix basteln ***/
    ca = cos( angle );
    sa = sin( -angle );

    rm.m11 = ca + (1-ca) * rot_x * rot_x;
    rm.m12 = (1-ca) * rot_x * rot_y - sa * rot_z;
    rm.m13 = (1-ca) * rot_z * rot_x + sa * rot_y;
    rm.m21 = (1-ca) * rot_x * rot_y + sa * rot_z;
    rm.m22 = ca + (1-ca) * rot_y * rot_y;
    rm.m23 = (1-ca) * rot_y * rot_z - sa * rot_x;
    rm.m31 = (1-ca) * rot_z * rot_x - sa * rot_y;
    rm.m32 = (1-ca) * rot_y * rot_z + sa * rot_x;
    rm.m33 = ca + (1-ca) * rot_z * rot_z;

    /*** Und rotieren ***/
    nc_m_mul_m( &(bact->dir), &rm, &nm );

    bact->dir = nm;
}

