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
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"
#include "input/input.h"
#include "audio/audioengine.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine



/*-----------------------------------------------------------------*/
void yr_rot_round_lokal_x(  struct yparobo_data *yrd, FLOAT angle);
void yr_rot_round_lokal_y(  struct yparobo_data *yrd, FLOAT angle);
void yr_rot_round_global_y( struct yparobo_data *yrd, FLOAT angle);
void yr_rotate_guns( struct yparobo_data *yrd, struct flt_triple *rot, FLOAT angle );
BOOL yr_CollisionCheck( struct yparobo_data *yrd, FLOAT time );
void yr_MerkeDieAltenDinger( struct yparobo_data *yrd );
BOOL yw_I_am_autonom( struct yparobo_data *yrd );
void yr_Oueiouaouea( struct yparobo_data *yrd, struct trigger_logic_msg *msg );


/*** Global für CollisionCheck ***/
FLOAT  relpos[] = { 1.0, 0.0,   0.0, 0.0,   -1.0, 0.0,   0.0, 1.0};

/*-----------------------------------------------------------------*/
_dispatcher(void, yr_YBM_MOVE, struct move_msg *move)
/// "MoveMethode (YBM_MOVE)"
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
    struct yparobo_data *yrd;
    struct uvec grav;       /* Gravitations-Kraftvektor */
    struct uvec schub;      /* Schub-Kraftvektor */
    struct uvec auft;       /* AuftriebsKraftvektor */
    struct uvec lw;         /* Luftwiederstand */
    struct uvec res;
    FLOAT  delta_betrag, new_v_x, new_v_y, new_v_z, new_v_v;
    FLOAT  verhaeltnis, max;
    int    i;

    yrd = INST_DATA( cl, o);
    
    /*
    ** Fast wie beim Zeppelin, allerdings wird die Kraftrichtung in der Move-
    ** Message übergeben, da hier Flug- und Sichtrichtung unabhängig voneinander
    ** sind!
    */

    /*
    ** Zuerst wird immer die alte Position gemerkt. Immer !
    */
    yrd->bact->old_pos = yrd->bact->pos;



    /*** Gravitation wirkt immer nach unten ***/
    grav.x = 0.0;
    grav.y = 1.0;
    grav.z = 0.0;
    if( ACTION_DEAD == yrd->bact->MainState )
        grav.v = GRAV_FACTOR * GRAVITY * yrd->bact->mass;
    else
        grav.v = GRAVITY * yrd->bact->mass;

    /*** Auftrieb ***/
    if( move->flags & (UBYTE) MOVE_NOFORCE ) {
        auft.x = 0;
        auft.y = 0;
        auft.z = 0;
        auft.v = 0;
        }
    else {
        auft.x =  0.0;
        auft.y = -1.0;
        auft.z =  0.0;
        auft.v =  yrd->buoyancy;
        }

    /*** Schub wirkt in Richtung move.x/y/z ***/
    if( move->flags & (UBYTE) MOVE_NOFORCE ) {
        schub.x = 0;
        schub.y = 0;
        schub.z = 0;
        schub.v = 0;
        }
    else {
        schub.x = move->schub_x;
        schub.y = move->schub_y;
        schub.z = move->schub_z;
        schub.v = yrd->bact->act_force;
        }

    /*** Luftwiderstand ***/
    lw.x = -yrd->bact->dof.x;
    lw.y = -yrd->bact->dof.y;
    lw.z = -yrd->bact->dof.z;
    lw.v = yrd->bact->dof.v * yrd->bact->air_const;

    /*** resultierender Kraftvektor ***/
    res.x = (grav.x*grav.v)+(schub.x*schub.v)+(lw.x*lw.v)+(auft.x*auft.v);
    res.y = (grav.y*grav.v)+(schub.y*schub.v)+(lw.y*lw.v)+(auft.y*auft.v);
    res.z = (grav.z*grav.v)+(schub.z*schub.v)+(lw.z*lw.v)+(auft.z*auft.v);
    res.v = nc_sqrt(res.x*res.x + res.y*res.y + res.z*res.z);

    /*** neuer dof - zum alten aufaddieren ***/
    
    if( res.v > 0.0 ) {

        delta_betrag = (res.v / yrd->bact->mass) * move->t;
        new_v_x = yrd->bact->dof.x * yrd->bact->dof.v + (res.x / res.v ) * delta_betrag;
        new_v_y = yrd->bact->dof.y * yrd->bact->dof.v + (res.y / res.v ) * delta_betrag;
        new_v_z = yrd->bact->dof.z * yrd->bact->dof.v + (res.z / res.v ) * delta_betrag;
        new_v_v = nc_sqrt( new_v_x * new_v_x + new_v_y * new_v_y + new_v_z * new_v_z );
        
        if( new_v_v > 0.0 ) {
            new_v_x /= new_v_v;
            new_v_y /= new_v_v;
            new_v_z /= new_v_v;
            }

        yrd->bact->dof.x = new_v_x;
        yrd->bact->dof.y = new_v_y;
        yrd->bact->dof.z = new_v_z;
        yrd->bact->dof.v = new_v_v;
        }


    /*
    ** Jetzt haben wir eine Neue Geschwindigkeit, da müssen wir aber noch
    ** die Position neu setzen. Die Ausrichtung wurde schon erledigt...
    */
    if( fabs(yrd->bact->dof.v) > IS_SPEED ) {

        yrd->bact->pos.x += yrd->bact->dof.x * yrd->bact->dof.v * move->t * METER_SIZE;
        yrd->bact->pos.y += yrd->bact->dof.y * yrd->bact->dof.v * move->t * METER_SIZE;
        yrd->bact->pos.z += yrd->bact->dof.z * yrd->bact->dof.v * move->t * METER_SIZE;
        }

    _methoda( o, YBM_CORRECTPOSITION, NULL );

    /*** Meine Position hat sich verändert, also ändern sich auch meine Kanonen ***/
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
    
    /*** Alte Sachen vor Veränderung ***/
    yrd->bact->sc.src[ VP_NOISE_NORMAL ].pitch  = yrd->bact->bu_pitch;
    yrd->bact->sc.src[ VP_NOISE_NORMAL ].volume = yrd->bact->bu_volume;

    /*** Wir beachten act_force und buoyancy ***/

    // verhaeltnis = yrd->bact->act_force / yrd->bact->max_force;
    //
    // /*** Maximalgrenzen für den Auftrieb gibt es nicht ***/
    // bv = 1 + yrd->buoyancy / ( 8 * yrd->bact->mass * GRAVITY );
    // if( bv < 0.0 ) bv = 0.0;
    // verhaeltnis += bv;
    
    verhaeltnis = fabs( yrd->bact->dof.v ) / 
                  ( yrd->bact->max_force / yrd->bact->bu_air_const );

    max = 1.4;
    verhaeltnis *= max;

    if( yrd->bact->sc.src[ VP_NOISE_NORMAL ].sample )
        yrd->bact->sc.src[ VP_NOISE_NORMAL ].pitch = (LONG)
            ( ((FLOAT) yrd->bact->sc.src[ VP_NOISE_NORMAL ].pitch +
               (FLOAT) yrd->bact->sc.src[ VP_NOISE_NORMAL ].sample->SamplesPerSec ) *
                verhaeltnis );
}
///


_dispatcher( void, yr_YBM_STOPMACHINE, struct trigger_logic_msg *msg )
{
/*
**  FUNCTION    Zur Ruhe bringen - ROBO NICHT KIPPEN!
**
**  INPUT       trigger_logic_msg
**
**  RESULT      nüscht!
*/

    struct yparobo_data *yrd;

    yrd = INST_DATA(cl, o );

    /*** noch ein paar Flyer-Spezialitäten ***/
    yrd->bact->dof.v     *= 0.8;             // nur so... doch doch!
    yrd->bact->act_force  = 0.0;
    yrd->buoyancy         = yrd->bact->mass * GRAVITY;
}



/*
** Die diverstesten Routinen, die wir so brauchen.
*/




void yr_rot_round_lokal_y( struct yparobo_data *yrd, FLOAT angle)
/// "Rotation um lokale y-Achse"
{

    struct flt_m3x3 neu_dir, rm;
    struct Bacterium *bact;
    struct flt_triple ra;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );

    bact = yrd->bact;

    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;

    /*** Kanonen rotieren ***/
    ra.x = yrd->bact->dir.m21;
    ra.y = yrd->bact->dir.m22;
    ra.z = yrd->bact->dir.m23;
    yr_rotate_guns( yrd, &ra, angle );

    /*** drehung um y-achse noch merken ***/
    yrd->coll.last_angle = angle;
}
///


void yr_rot_round_global_y( struct yparobo_data *yrd, FLOAT angle)
/// "Rotation um globale y-Achse"
{

    struct flt_m3x3 neu_dir, rm;
    struct Bacterium *bact;
    struct flt_triple ra;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );

    bact = yrd->bact;

    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m( &(bact->dir), &rm, &neu_dir);

    bact->dir = neu_dir;

    /*** Kanonen rotieren ***/
    ra.x = 0;
    ra.y = 1;
    ra.z = 0;
    yr_rotate_guns( yrd, &ra, angle );

    /*** drehung um y-achse noch merken ***/
    yrd->coll.last_angle = angle;
}
///


void yr_rot_round_lokal_x( struct yparobo_data *yrd, FLOAT angle)
/// "Rotation um lokale x-Achse"
{

    struct flt_m3x3 neu_dir, rm;
    struct Bacterium *bact;
    struct flt_triple ra;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );

    bact = yrd->bact;

    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_y;     rm.m23 = sin_y;
    rm.m31 = 0.0;       rm.m32 = -sin_y;    rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;

    /*** Kanonen rotieren ***/
    ra.x = yrd->bact->dir.m11;
    ra.y = yrd->bact->dir.m12;
    ra.z = yrd->bact->dir.m13;
    yr_rotate_guns( yrd, &ra, angle );
}
///


void yr_rotate_guns( struct yparobo_data *yrd, struct flt_triple *rot, FLOAT angle )
{
    /*** Rotiert alle Kanonen um die übergebene Achse und Winkel ***/
    int    i;
    struct rotategun_msg rg;

    /*** Der Winkel wird erst hier negiert! ***/
    angle = -angle;

    for( i = 0; i < NUMBER_OF_GUNS; i++ ) {

        if( yrd->gun[ i ].go ) {

            rg.rot.x = rot->x;
            rg.rot.y = rot->y;
            rg.rot.z = rot->z;
            rg.angle = angle;

            /*** Rotieren ***/
            _methoda( yrd->gun[ i ].go, YGM_ROTATEGUN, &rg );

            /*** Rückschreiben ***/
            yrd->gun[ i ].dir = rg.basis;
            }
        }
}

BOOL yr_CollisionCheck( struct yparobo_data *yrd, FLOAT time )
{
/*
**  FUNCTION    Alles neu macht der Februar. Jetzt gibt es das erweiterte
**              Kollsionsmodell, welches der Robo unterstützt.
**              Wir machen Intersections von allen OLD-Testpoints zu den
**              NEW-Testpoints (und darüber hinaus). Sind wir Viewer, machen wir
**              für jeden testpoint noch eine Kugelkollision
**
**  INPUT       Zeit und Zeuch
**
**  RESULTS     TRUE bei direkter Kollision
**
**  CHANGED     an einem schönen Tag zamnandergehackt
**              an einem Tag vor wichtigen Tagen total geändert
*/

    struct insphere_msg ins;
    struct Insect insect[32];
    struct flt_triple *pos, *old, vec;
    struct uvec *dof;
    FLOAT  A, B, C, betrag, distance;
    struct recoil_msg rec, rec2;
    struct intersect_msg inter;
    int    coll_count, i;
    BOOL   ret;
    ULONG  VIEW, VIS, CVE, USER;
    struct Cell *sector;
    struct energymod_msg mse;
    struct getsectorinfo_msg gsi;

    /*** Vorbereitungen ***/
    pos = &(yrd->bact->pos);
    dof = &(yrd->bact->dof);
    old = &(yrd->bact->old_pos);
    
    _get( yrd->bact->BactObject, YBA_Viewer, &VIEW );
    _get( yrd->bact->BactObject, YBA_UserInput, &USER );
    _get( yrd->bact->BactObject, YBA_CheckVisExactly, &CVE );


    /*** Haben wir uns überhaupt bewegt??? ***/
    if( (pos->x == old->x) && (pos->y == old->y) && (pos->z == old->z) )
        return( FALSE );

    /*** tests für jeden Testpunkt ***/
    rec2.vec.x = rec2.vec.y = rec2.vec.z = 0.0;
    coll_count = 0;

    for( i = 0; i < yrd->coll.number; i++ ) {

        struct flt_triple neu, alt;

        /* ---------------------------------------------------------------------
        ** Intersection. Diese dient vor allem der Verhütung (von Kollisionen!).
        ** Folglich machen wir sie nur, wenn wir autonom sind 
        ** -------------------------------------------------------------------*/

        if( !( USER && ( yrd->bact->PrimTargetType == TARTYPE_NONE) ) ) {
        
            alt.x = yrd->coll.points[ i ].old.x;
            alt.y = yrd->coll.points[ i ].old.y;
            alt.z = yrd->coll.points[ i ].old.z;
            
            neu.x = yrd->bact->pos.x +
                    yrd->bact->dir.m11 * yrd->coll.points[ i ].point.x +
                    yrd->bact->dir.m12 * yrd->coll.points[ i ].point.y +
                    yrd->bact->dir.m13 * yrd->coll.points[ i ].point.z;
            neu.y = yrd->bact->pos.y +
                    yrd->bact->dir.m21 * yrd->coll.points[ i ].point.x +
                    yrd->bact->dir.m22 * yrd->coll.points[ i ].point.y +
                    yrd->bact->dir.m23 * yrd->coll.points[ i ].point.z;
            neu.z = yrd->bact->pos.z +
                    yrd->bact->dir.m31 * yrd->coll.points[ i ].point.x +
                    yrd->bact->dir.m32 * yrd->coll.points[ i ].point.y +
                    yrd->bact->dir.m33 * yrd->coll.points[ i ].point.z;

            /*** Wenn wir uns nach oben bewegen, dann testen wir auch nach vorn!! ***/
            inter.pnt.x = alt.x;
            inter.pnt.y = alt.y;
            inter.pnt.z = alt.z;
            inter.flags = 0;

            distance = nc_sqrt( (alt.x - neu.x) * (alt.x - neu.x) +
                                (alt.z - neu.z) * (alt.z - neu.z) );

            if( distance > 1.0 ) {

                inter.vec.x = ( neu.x - alt.x ) * 300 / distance;
                inter.vec.y = 0.0;
                inter.vec.z = ( neu.z - alt.z ) * 300 / distance;
                }
            else {

                inter.vec.x = yrd->bact->dof.x * 300;
                inter.vec.y = yrd->bact->dof.y * 300;
                inter.vec.z = yrd->bact->dof.z * 300;
                }

            // Wozu dieser Scheiß?
            //distance = nc_sqrt( inter.vec.x * inter.vec.x + inter.vec.y *
            //                    inter.vec.y + inter.vec.z * inter.vec.z );

            _methoda( yrd->world, YWM_INTERSECT, &inter );

            ret = FALSE;

            /*** Auswertung je nachdem, ob USER ***/
            if( inter.insect ) {

                /*** direkte Kollision? (Nur bei Radius != 0) ***/
                if( (inter.t * 300) < (distance + yrd->coll.points[ i ].radius) ) {

                    /*** Nur abrallen, wenn es ein Kollisionspunkt war ***/
                    if( yrd->coll.points[ i ].radius > 0.01 ) {

                        /*** krach ***/
                        rec.vec.x = inter.sklt->PlanePool[ inter.pnum ].A;
                        rec.vec.y = inter.sklt->PlanePool[ inter.pnum ].B;
                        rec.vec.z = inter.sklt->PlanePool[ inter.pnum ].C;
                        rec.time  = time;
                        rec.mul_y = 2.0;     // Werte halt setzen, wenn
                        rec.mul_v = 0.2;     // auch sinnlos
                        _methoda( yrd->bact->BactObject, YBM_RECOIL, &rec);

                        /*** Wegdrehen (letzten Winkel ermitteln und rückdrehen) ***/
                        if( fabs( yrd->coll.last_angle ) > 0.0 ) {

                            yr_rot_round_lokal_y( yrd, (FLOAT) -yrd->coll.last_angle );
                            yrd->coll.last_angle = 0.0;
                            }

                        /* ------------------------------------------------------
                        ** Draufhauen. Wir ziehen Energie ab, wenn wir User
                        ** sind oder AI und der Sektor kein wichtiges Gebäude ist 
                        ** ----------------------------------------------------*/
                        
                        gsi.abspos_x = inter.ipnt.x;
                        gsi.abspos_z = inter.ipnt.z;
                        _methoda( yrd->world, YWM_GETSECTORINFO, &gsi );    // muß klappen
                        sector = gsi.sector;
                        
                        if( USER || (!(sector->WType)) ) {

                            /*** Nur pro forma übers Netz ***/
                            mse.pnt.x  = inter.ipnt.x;
                            mse.pnt.y  = inter.ipnt.y;
                            mse.pnt.z  = inter.ipnt.z;
                            mse.energy = (LONG) (YR_DestV * distance / time );
                            mse.owner  = 255;
                            mse.hitman = NULL;
                            _methoda( yrd->bact->BactObject, YBM_MODSECTORENERGY, &mse );
                            }

                        /*** Nachkorrigieren ***/
                        yrd->bact->dof.v *= 0.4;

                        yrd->bact->ExtraState |= EXTRA_OVER;

                        /*** Es hat geknallt. Sound setzen ***/
                        //if( (!(yrd->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE )) &&
                        //    (fabs( yrd->bact->dof.v ) > NOISE_SPEED) &&
                        //    (!(yrd->bact->ExtraState & EXTRA_LANDCRASH )) ) {
                        //
                        //    yrd->bact->ExtraState |= EXTRA_LANDCRASH;
                        //    _StartSoundSource( &(yrd->bact->sc), VP_NOISE_CRASHLAND );
                        //    }

                        ret = TRUE;
                        }
                    }
                else yrd->bact->ExtraState &= ~EXTRA_LANDCRASH;

                /*** Drüber weg, 's reicht, ExtraOver zu setzen ***/
                yrd->bact->ExtraState |= EXTRA_OVER;

                /*** Höhentest unnötig, wir machen sowieso nach oben ***/
                return( ret );
                }
            }

        /* ----------------------------------------------------------------
        ** Das war die Intersection. Wenn diese nicht gebracht hat und
        ** wir Viewer sind, dann können wir noch eine Kugelkollision machen
        ** --------------------------------------------------------------*/
        VIS = _methoda( yrd->world, YWM_ISVISIBLE, yrd->bact );

        if( (VIEW || ( VIS && CVE )) &&
            (yrd->coll.points[ i ].radius > 0.01) ) {

            struct flt_triple alt, neu;

            neu.x  = yrd->bact->pos.x +
                     yrd->bact->dir.m11 * yrd->coll.points[ i ].point.x +
                     yrd->bact->dir.m12 * yrd->coll.points[ i ].point.y +
                     yrd->bact->dir.m13 * yrd->coll.points[ i ].point.z;
            neu.y  = yrd->bact->pos.y +
                     yrd->bact->dir.m21 * yrd->coll.points[ i ].point.x +
                     yrd->bact->dir.m22 * yrd->coll.points[ i ].point.y +
                     yrd->bact->dir.m23 * yrd->coll.points[ i ].point.z;
            neu.z  = yrd->bact->pos.z +
                     yrd->bact->dir.m31 * yrd->coll.points[ i ].point.x +
                     yrd->bact->dir.m32 * yrd->coll.points[ i ].point.y +
                     yrd->bact->dir.m33 * yrd->coll.points[ i ].point.z;

            alt.x =  yrd->coll.points[ i ].old.x;
            alt.y =  yrd->coll.points[ i ].old.y;
            alt.z =  yrd->coll.points[ i ].old.z;

            ins.pnt.x = alt.x;
            ins.pnt.y = alt.y;
            ins.pnt.z = alt.z;

            distance = nc_sqrt( (alt.x - neu.x) * (alt.x - neu.x) +
                                (alt.z - neu.z) * (alt.z - neu.z) );

            if( distance > 1.0 ) {

                ins.dof.x  = (neu.x - alt.x) / distance;
                ins.dof.y  = 0.0;
                ins.dof.z  = (neu.x - alt.x) / distance;
                }
            else {

                ins.dof.x  = yrd->bact->dof.x;
                ins.dof.y  = yrd->bact->dof.y;
                ins.dof.z  = yrd->bact->dof.z;
                }

            ins.radius = yrd->coll.points[ i ].radius;
            ins.chain  = insect;
            ins.max_insects = 32;
            ins.flags  = 0;

            _methoda( yrd->world, YWM_INSPHERE, &ins );

            if( ins.num_insects > 0 ) {

                /*
                ** Es trat eine Kollision auf! Wir summieren alle Normalen-
                ** vektoren der Ebenen auf und prallen von dort ab. Dann gehen
                ** wir mit großer wait_time sofort zurück.
                */

                coll_count++;

                A = 0;
                B = 0;
                C = 0;

                while( ins.num_insects-- ) {

                    /*** "Mittel"-Vektor ermitteln ***/
                    A += ins.chain[ ins.num_insects ].pln.A;
                    B += ins.chain[ ins.num_insects ].pln.B;
                    C += ins.chain[ ins.num_insects ].pln.C;

                    /*** Weil wir gerade zählen, hier auch die Energieabzüge ***/
                    
                    /* -------------------------------------------------------
                    ** Draufhauen. Wir ziehen Energie ab, wenn wir User
                    ** sind oder AI und der Sektor kein wichtiges Gebäude ist.
                    ** Wir müssen die Position nehmen, wo die Kollision auf-
                    ** getreten ist!
                    ** -----------------------------------------------------*/
                    
                    gsi.abspos_x = ins.chain[ ins.num_insects ].ipnt.x;
                    gsi.abspos_z = ins.chain[ ins.num_insects ].ipnt.z;
                    _methoda( yrd->world, YWM_GETSECTORINFO, &gsi );    // muß klappen
                    sector = gsi.sector;
                    
                    if( USER || (!(sector->WType)) ) {

                        /*** Der Form halber auch übers Netz ***/
                        mse.pnt.x  = ins.chain[ ins.num_insects ].ipnt.x;
                        mse.pnt.y  = ins.chain[ ins.num_insects ].ipnt.y;
                        mse.pnt.z  = ins.chain[ ins.num_insects ].ipnt.z;
                        mse.energy = (LONG) ( YR_DestV * distance / time );
                        mse.owner  = 255;
                        mse.hitman = 0;
                        _methoda( yrd->bact->BactObject, YBM_MODSECTORENERGY, &mse );
                        }
                    }

                betrag = nc_sqrt( A*A + B*B + C*C );

                if( betrag == 0) {

                    /*
                    ** Was komischerweise scheinbar vorkommt...
                    */
                    vec.x = dof->x;
                    vec.y = dof->y;
                    vec.z = dof->z;
                    }
                else {

                    vec.x = A / betrag;
                    vec.y = B / betrag;
                    vec.z = C / betrag;
                    }

                /*** Aufaddieren der Richtung ***/
                rec2.vec.x += vec.x;
                rec2.vec.y += vec.y;
                rec2.vec.z += vec.z;
                }

            /*** Traten Kugelkollisionen auf ? ***/
            if( coll_count ) {

                /*** Gesamtvektor ermitteln ***/
                rec2.vec.x /= coll_count;
                rec2.vec.y /= coll_count;
                rec2.vec.z /= coll_count;

                yrd->bact->pos = yrd->bact->old_pos;
                rec2.time  = time;
                rec2.mul_y = 2.0;        
                rec2.mul_v = 0.4;
                _methoda( yrd->bact->BactObject, YBM_RECOIL, &rec2);
                    
                /*** Wegdrehen (letzten Winkel ermitteln und rückdrehen) ***/
                if( fabs( yrd->coll.last_angle ) > 0.0 ) {

                    yr_rot_round_lokal_y( yrd, (FLOAT) -yrd->coll.last_angle );
                    yrd->coll.last_angle = 0.0;
                    }


                /* ---------------------------------------------------------
                ** Weil die Abprallgeschwindigkeit Speedabhängig ist, 
                ** müssen wir nachkorrigieren. Eine andere Variante wäre die 
                ** Überlagerung von RECOIL, aber das ist zuviel Aufwand 
                ** -------------------------------------------------------*/
                yrd->bact->dof.v *= 0.4;

                yrd->bact->ExtraState |= EXTRA_OVER;

                //if( (!(yrd->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE )) &&
                //    (fabs( yrd->bact->dof.v ) > NOISE_SPEED) &&
                //    (!(yrd->bact->ExtraState & EXTRA_LANDCRASH )) ) {
                //
                //    yrd->bact->ExtraState |= EXTRA_LANDCRASH;
                //    _StartSoundSource( &(yrd->bact->sc), VP_NOISE_CRASHLAND );
                //    }

                return( TRUE );
                }
            }
        }

    /*** Wenn wir bis hierher gekommen sind, dann sind wir nicht abgeprallt ***/
    yrd->bact->ExtraState &= ~EXTRA_LANDCRASH;

    /* -------------------------------------------------------------------
    ** Nun haben wir alle Punkte ausgetestet (ähem...), aber es fehlt noch
    ** ein Höhentest.
    ** -----------------------------------------------------------------*/


    inter.pnt.x = yrd->bact->pos.x;
    inter.pnt.y = yrd->bact->pos.y;
    inter.pnt.z = yrd->bact->pos.z;
    inter.vec.x = yrd->bact->dof.x * 100.0;
    inter.vec.y = 1.5 * yrd->bact->pref_height;
    inter.vec.z = yrd->bact->dof.z * 100.0;
    inter.flags = 0;
    _methoda( yrd->world, YWM_INTERSECT, &inter );

    if( inter.insect ) {

        /*** wirklich nach oben? ***/
        if( inter.t < 0.66 ) {

            yrd->bact->tar_unit.y  = -1.0;
            yrd->bact->ExtraState |= EXTRA_OVER;

            /*** Wenn keine Kugel mehr, dann hier das Abprallen ***/
            }
        }
    else {

        yrd->bact->tar_unit.y  = 1.0;

        /*** jetzt ist alles frei --> Over aus ***/
        yrd->bact->ExtraState &= ~EXTRA_OVER;
        }

    /*** Lastangle nicht benötigt ***/
    yrd->coll.last_angle = 0.0;

    /*** Das war's schon wieder. Hoffe, 's hat euch gefallen ***/
    return( FALSE );
}


void yr_AI_RealizeMoving( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
{
    /*
    ** Kraft berechnen, Move aufrufen und bei Bedarf (eigentlich
    ** immer) ausrichten.
    **
    ** Der Robo kennt Primärziele wie alle anderen auch. Nebenziele haben
    ** aber weniger Sinn. trotzdem steht erstmal eine Richtung in tar_unit.
    ** Steht dort nix, ist auch nicht schlimm, dann ändern wir nur die Höhe.
    **
    ** ACHTUNG! Ich bewege mich nur, wenn das Dock inaktiv ist!!!
    **  doch, doch, weil wir uns sonst in kritischen Situationen nie bewegen!
    */

    struct move_msg move;
    FLOAT  angle, distance;
    FLOAT  betrag_dir, betrag_tar;
    ULONG  VIEW;
    struct flt_m3x3   *bdir = &(yrd->bact->dir);
    struct flt_triple *btar = &(yrd->bact->tar_unit);
    FLOAT  time, xfactor;
    struct settarget_msg target;

    /*** Alte Positionen der testpunkte merken ***/
    yr_MerkeDieAltenDinger( yrd );

    time = (FLOAT) msg->frame_time / 1000.0;

    /*** Auftrieb ***/
    yrd->buoyancy = yrd->bact->mass * GRAVITY;  // default

    if( yrd->bact->tar_unit.y < -0.7 )
        yrd->buoyancy = 3.0 * yrd->bact->mass * GRAVITY;

    if( yrd->bact->tar_unit.y > 0.7 )
        yrd->buoyancy = 0.91 * yrd->bact->mass * GRAVITY;


    /* --------------------------------------------------------------------
    ** Es gibt beim Robo keinen Wait-Zustand, da wir immer denken müssen.
    ** Deshalb entscheiden wir hier, wie groß der betrag des tar_vec's ist.
    ** je kleiner er wird, desto größer wird die Luftdichte.
    ** ------------------------------------------------------------------*/

    if( yrd->bact->PrimTargetType != TARTYPE_NONE )
        distance = nc_sqrt( yrd->bact->tar_vec.x * yrd->bact->tar_vec.x +
                            yrd->bact->tar_vec.z * yrd->bact->tar_vec.z );
    else
        distance = 0.0;

    if( distance < 100.0 ) {

        /* -------------------------------------------------------------
        ** Wir sind nah dran, Also Luftdichte hoch, wenn wir uns noch
        ** in xz-Richtung bewegen. Stehen wir still, dann schwanken wir,
        ** was durch die Höhenkorrektur automatisch passieren dürfte
        ** -----------------------------------------------------------*/
        _get( yrd->bact->BactObject, YBA_Viewer, &VIEW );
        yrd->bact->act_force  = 0;

        if( ( fabs( yrd->bact->dof.x * yrd->bact->dof.v ) > IS_SPEED ) ||
            ( fabs( yrd->bact->dof.z * yrd->bact->dof.v ) > IS_SPEED ) ) {

            yrd->bact->air_const *= 3;
            yrd->bact->dof.v     *= 0.8;

            /* ---------------------------------------------------------
            ** Wenn wir jetzt unter die Geschwindigkeit kommen, schalten
            ** wir das Ziel aus.
            ** -------------------------------------------------------*/
            if( ( fabs( yrd->bact->dof.x * yrd->bact->dof.v ) < IS_SPEED ) &&
                ( fabs( yrd->bact->dof.z * yrd->bact->dof.v ) < IS_SPEED ) ) {

                /*** uff NULL setzen ***/
                yrd->bact->dof.x = yrd->bact->dof.z = 0.0;

                /*** und Ziel abmelden ***/
                target.target_type = TARTYPE_NONE;
                target.priority    = 0;
                _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
                }
            }
        else {

            /* --------------------------------------------------------------
            ** Wir sind am Ziel. Wenn wir autonom sind, dann können wir etwas
            ** machen, was von außen gut aussieht. Das kann ne Menge sein.
            ** Sind wir Viewer, so nehmen wir nur die Höhe ein, was wir auch
            ** machen, wenn wir nicht schwanken.
            ** Natürlich dürfen wir uns nicht drehen und schwanken, wenn wir
            ** beim Bauen sind.
            ** ------------------------------------------------------------*/
            if( (!(yrd->RoboState & ROBO_BUILDRADAR))  &&
                (!(yrd->RoboState & ROBO_BUILDPOWER))  &&
                (!(yrd->RoboState & ROBO_BUILDSAFETY)) ) {

                BOOL autonom;

                autonom = yw_I_am_autonom( yrd );

                if( !autonom || (!(yrd->waitflags & RWF_SWAY)) ) {

                    /* --------------------------------------------------------
                    ** Höhe einnehmen. Das heißt, wir führen bei günstiger Höhe
                    ** (worüber tar_unit Auskunft gibt), ein zusätzliches
                    ** Abbremsmanöver durch.
                    ** ------------------------------------------------------*/
                    if( fabs( yrd->bact->tar_unit.y ) < 0.7 )
                        yrd->bact->dof.v *= 0.35;
                    }
                else {

                    /* ------------------------------
                    ** kein Viewer und Schwankwillig.
                    ** stärker runterfallen lassen
                    ** ----------------------------*/

                    if( yrd->bact->ExtraState & EXTRA_OVER )
                        yrd->buoyancy = 1.5  * yrd->bact->mass * GRAVITY;
                    else
                        yrd->buoyancy = 0.82 * yrd->bact->mass * GRAVITY;
                    /*** andernfalls bleibt alles ***/
                    }

                if( autonom && (yrd->waitflags & RWF_ROTATE) ) {

                    yr_rot_round_lokal_y( yrd, (FLOAT)( 0.3 * yrd->bact->max_rot * time ) );
                    }
                }
            }

        /*** Schub brauchen wir nicht mehr ***/
        move.schub_x = 0.0;
        move.schub_y = 0.0;
        move.schub_z = 0.0;

        /*** der Rest ***/
        move.flags = 0;
        move.t     = time;

        /*** wech ***/
        _methoda( yrd->bact->BactObject, YBM_MOVE, &move );
        }
    else {

        /* ---------------------------------------------------------------
        ** Wir sind weit weg und bewegen uns entsprechend. Dazu gehört die
        ** Ausrichtung, Höhenkorrektur und die normale Bewegung
        ** -------------------------------------------------------------*/

        /*** EXTRA_OVER ist stärker als tar_unit ***/
        if( yrd->bact->ExtraState & EXTRA_OVER ) {

            /* -----------------------------------------------------------
            ** ExtraOver ist Folge einer Kollisionsvermeidung. Da wir sehr
            ** träge sind müssen wir abbremsen mit hoher Luftdichte.
            ** ---------------------------------------------------------*/
            FLOAT factor;

            yrd->bact->air_const *= 3.0;
            yrd->buoyancy         = 2.5 * yrd->bact->mass * GRAVITY;
            yrd->bact->act_force  = 0.0;
            factor                = ( 1.0 - 4 * time );
            if( factor < 0.0 ) factor = 0.1;
            yrd->bact->dof.v     *= factor;
            }
        else {

            /*** nun act_force ... (evtl. später zustandsabhängig) ***/
            if( yrd->bact->PrimTargetType == TARTYPE_NONE )
                yrd->bact->act_force = 0;
            else
                yrd->bact->act_force = yrd->bact->max_force;
            }

        /*** eindrehen auf tar_unit, wenn nicht over ***/
        if( !(yrd->bact->ExtraState & EXTRA_OVER) ) {

            betrag_dir = nc_sqrt( bdir->m31*bdir->m31 + bdir->m33*bdir->m33 );
            betrag_tar = nc_sqrt( btar->x  *btar->x   + btar->z  *btar->z   );

            if( (betrag_dir > 0.0001 ) && (betrag_tar > 0.0001) ) {

                angle = nc_acos( ( bdir->m31*btar->x + bdir->m33*btar->z ) /
                                 ( betrag_tar * betrag_dir) );

                if( fabs(angle) >= 0.001 ) {

                    if( fabs(angle) > time * yrd->bact->max_rot )
                              angle = time * yrd->bact->max_rot;
                    if( ( bdir->m31 * btar->z - bdir->m33 * btar->x ) < 0.0 )
                        angle = -angle;

                    /*** Trägheit einbauen. Winkel 0 is' nich' ***/
                    if( angle > 0 ) {

                        if( (yrd->bact->ExtraState & EXTRA_HC_XLEFT) ||
                            (yrd->bact->sliderx_time == 0) )
                            yrd->bact->sliderx_time = msg->global_time;
                        xfactor = 0.1 + 0.9 * (msg->global_time - yrd->bact->sliderx_time)/
                                  1000;
                        yrd->bact->ExtraState &= ~EXTRA_HC_XLEFT;
                        }
                    else {

                        if( (!(yrd->bact->ExtraState & EXTRA_HC_XLEFT)) ||
                            (yrd->bact->sliderx_time == 0) )
                            yrd->bact->sliderx_time = msg->global_time;
                        xfactor = 0.1 + 0.9 * (msg->global_time - yrd->bact->sliderx_time)/
                                  1000;
                        yrd->bact->ExtraState |= EXTRA_HC_XLEFT;
                        }
                    if( xfactor > 1.0 ) xfactor = 1.0;

                    angle *= xfactor;

                    /*** nun rotieren ***/
                    yr_rot_round_global_y( yrd, angle );
                    }
                }
            else {

                /*** Seitendrehung-Trägheit ausschalten ***/
                yrd->bact->sliderx_time = 0;
                yrd->bact->ExtraState &= ~EXTRA_HC_XLEFT;
                }
            }

        /*** MOVE immer, weil dort auch die Guns gesetztt werden! ***/
        move.schub_x = yrd->bact->tar_vec.x / distance;
        move.schub_z = yrd->bact->tar_vec.z / distance;
        move.schub_y = 0.0;

        /*** der Rest ***/
        move.flags = 0;
        move.t     = time;

        /*** wech ***/
        _methoda( yrd->bact->BactObject, YBM_MOVE, &move );
        }

}


void yr_MerkeDieAltenDinger( struct yparobo_data *yrd )
{
/*  ----------------------------------------------------------------------------
**  Merkt sich die alten Positionen der testpunkte (wir haben ja ein komplexeres
**  Kollsionsmodell, welches auch auf Drehung reagieren muß). In old merken
**  wir uns der Einfachheit halber gleich die absoluten Koordinaten, das geht
**  schneller und ist einfacher
**  --------------------------------------------------------------------------*/
    
    int i;

    for( i = 0; i < yrd->coll.number; i++ ) {

        yrd->coll.points[ i ].old.x = yrd->bact->pos.x +
             yrd->bact->dir.m11 * yrd->coll.points[ i ].point.x +
             yrd->bact->dir.m12 * yrd->coll.points[ i ].point.y +
             yrd->bact->dir.m13 * yrd->coll.points[ i ].point.z;
        
        yrd->coll.points[ i ].old.y = yrd->bact->pos.y +
             yrd->bact->dir.m21 * yrd->coll.points[ i ].point.x +
             yrd->bact->dir.m22 * yrd->coll.points[ i ].point.y +
             yrd->bact->dir.m23 * yrd->coll.points[ i ].point.z;
        
        yrd->coll.points[ i ].old.z = yrd->bact->pos.z +
             yrd->bact->dir.m31 * yrd->coll.points[ i ].point.x +
             yrd->bact->dir.m32 * yrd->coll.points[ i ].point.y +
             yrd->bact->dir.m33 * yrd->coll.points[ i ].point.z;
        }
}


BOOL yw_I_am_autonom( struct yparobo_data *yrd )
{
/* ----------------------------------------------------------------
** testet, ob es sich um einen selbstgesteuerten Robo handelt. Also
** es ist nicht der Viewer, sondern ein feind oder wir sitzen nicht
** drin!
** Warum ist das so kompliziert? Weil ja auch eine der Bordkanonen
** der Viewer sein kann!
** --------------------------------------------------------------*/

    Object *UV;
    int     i;


    _get( yrd->world, YWA_UserVehicle, &UV );
    if( UV == yrd->bact->BactObject ) return( FALSE );

    for( i = 0; i < NUMBER_OF_GUNS; i++ ) {

        if( yrd->gun[ i ].go ) {

            if( UV == yrd->gun[ i ].go ) return( FALSE );
            }
        }

    /*** nüscht gefunden, also bin ich was eigenständiges ***/
    return( TRUE );
}
