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
#include "ypa/ypamissileclass.h"
#include "ypa/ypagunclass.h"
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#include "input/input.h"



#ifndef AMIGA
#ifndef _MSC_VER
int matherr( struct exception *err_info)
{
    switch( err_info->type ) {

        case DOMAIN:

            _LogMsg("DOMAIN ERROR\n");
            break;

        case SING:

            _LogMsg("SING ERROR\n");
            break;

        case OVERFLOW:

            _LogMsg("OVERFLOW ERROR\n");
            break;

        case UNDERFLOW:

            _LogMsg("UNDERFLOW ERROR\n");
            break;

        case TLOSS:

            _LogMsg("TLOSS ERROR\n");
            break;

        case PLOSS:

            _LogMsg("PLOSS ERROR\n");
            break;
        }

    _LogMsg("Function %s with argument %f\n", err_info->name, err_info->arg1);

    return( 0 );
}
#endif
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


#define NO_SPEED        4.0
#define MISSILE_TARGET_DISTANCE     1000.0       /* ab da keine Höhenkorr. mehr */

/*-----------------------------------------------------------------*/
void ym_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void ym_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle);
void ym_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle);
void ym_rot_round_global_y( struct Bacterium *bact, FLOAT angle);
BOOL ym_VisibleTest( struct ypazepp_data *ymd);
BOOL ym_CheckRohr( struct ypamissile_data *ymd );
void ym_CalculateForce(struct ypamissile_data *ymd, struct move_msg *move);


/*-----------------------------------------------------------------*/
_dispatcher( void, ym_YBM_AI_LEVEL1, struct trigger_logic_msg *msg)
{
/*
** FUNCTION     Berechnet den Zielvektor. Wir müssen das bei den raketen 
**              überlagern, weil AI1 zu sehr auf Bakterien ausgelegt ist
**              (PrimCommandID etc. es kann passieren,daß jemand stirbt, den
**              wir als Ziel haben und wir können es nicht merken )
**
**              NIE WIEDER SO ETWAS WIE RAKETEN MIT DEN BAKTERIEN AUF EINE
**              STUFE STELLEN!!!
**
*/

    struct ypamissile_data *ymd;
    struct flt_triple *vc;

    ymd = INST_DATA( cl, o );

    if( ACTION_DEAD == ymd->bact->MainState ) {

        LONG YLS;
        _get( o, YBA_YourLS, &YLS );
        YLS -= msg->frame_time;
        _set( o, YBA_YourLS,  YLS );
        }


    vc = &(ymd->bact->tar_vec);

    if( ymd->bact->PrimTargetType != (UBYTE) TARTYPE_NONE ) {

        if( ymd->bact->PrimTargetType == (UBYTE) TARTYPE_BACTERIUM ) {

            vc->x = ymd->bact->PrimaryTarget.Bact->pos.x-ymd->bact->pos.x;
            vc->y = ymd->bact->PrimaryTarget.Bact->pos.y-ymd->bact->pos.y;
            vc->z = ymd->bact->PrimaryTarget.Bact->pos.z-ymd->bact->pos.z;
            }
        else {
            
            /*** Sektor oder Formation ***/
            vc->x = ymd->bact->PrimPos.x - ymd->bact->pos.x;
            vc->z = ymd->bact->PrimPos.z - ymd->bact->pos.z;
            vc->y = ymd->bact->PrimPos.y - ymd->bact->pos.y;
            }
        }

    _methoda( o, YBM_AI_LEVEL2, msg );
}


/*-----------------------------------------------------------------*/
_dispatcher(void, ym_YBM_AI_LEVEL2, struct trigger_logic_msg *msg)
{
    /* nix!! */
    _methoda( o, YBM_AI_LEVEL3, msg );
}


/*-----------------------------------------------------------------*/
_dispatcher(void, ym_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION
**
**
**  INPUTS
**
**      Eine Wunschrichtung, die in tar_vec steht.
**
**  RESULTS
**
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/

    FLOAT betrag, time;
    struct move_msg move;
    struct ypamissile_data *ymd;
    struct setstate_msg state;
    struct settarget_msg target;
    struct intersect_msg inter;
    struct energymod_msg energy;
    ULONG  VIS, RMUI;
    struct alignmissile_msg am;

    ymd = INST_DATA( cl, o );
    VIS = _methoda( ymd->world, YWM_ISVISIBLE, ymd->bact );

    betrag = nc_sqrt( ymd->bact->tar_vec.x * ymd->bact->tar_vec.x +
                      ymd->bact->tar_vec.y * ymd->bact->tar_vec.y +
                      ymd->bact->tar_vec.z * ymd->bact->tar_vec.z);
    
    if( (betrag > 0.1) && (ymd->bact->PrimTargetType != TARTYPE_SIMPLE)) {

        /*** wenn es ein Ziel gibt und wir tar_unit verändern dürfen ***/
        betrag = 1 / betrag;
        ymd->bact->tar_unit.x = ymd->bact->tar_vec.x * betrag;
        ymd->bact->tar_unit.y = ymd->bact->tar_vec.y * betrag;
        ymd->bact->tar_unit.z = ymd->bact->tar_vec.z * betrag;
        }

    time = (FLOAT) msg->frame_time / 1000.0;

    /*** Raketen müssen immer bearbeitet werden! ***/
    ymd->bact->time_ai1  = 0;
    ymd->bact->act_force = ymd->bact->max_force;

    switch( ymd->bact->MainState ) {

        case ACTION_NORMAL:

            /*** Runterzählen? ***/
            if( ymd->flags & YMF_CountDelay ) {

                ymd->delay -= msg->frame_time;
                if( ymd->delay <= 0 ) {

                    _methoda( o, YMM_DOIMPULSE, NULL );
                    
                    ymd->bact->MainState = ACTION_DEAD;
                    state.extra_on   = EXTRA_MEGADETH;
                    state.extra_off  = 0;
                    state.main_state = 0;
                    _methoda(o, YBM_SETSTATE, &state );

                    /*** EnergieAbzug, wenn ich darf ***/
                    if( !( (ymd->flags & YMF_IgnoreBuildings) &&
                           (ymd->bact->Sector->WType) ) ) {

                        /* --------------------------------------
                        ** Nur abziehen, wenn originalrakete oder
                        ** Einzelplayer
                        ** ------------------------------------*/
                        if( (ymd->ywd->URBact->Owner == (ULONG)ymd->bact->Owner) ||
                            (FALSE == ymd->ywd->playing_network) ) {

                            energy.pnt.x = ymd->bact->pos.x + 5 * ymd->bact->dof.x;
                            energy.pnt.z = ymd->bact->pos.z + 5 * ymd->bact->dof.z;
                            energy.energy = (LONG) ymd->bact->Energy;
                            // FIXME_FLOH!
                            energy.hitman = ymd->rifle_man;
                            _methoda( o, YBM_MODSECTORENERGY, &energy);
                            }
                        }

                    return;
                    }

                /*** andernfalls nicht verlassen, da sonst noch Mine ! ***/
                }

            /*** Move at first ***/
            switch( ymd->handle ) {

                case YMF_Bomb:

                    move.t = time;
                    move.flags = MOVE_NOFORCE;
                    _methoda( o, YBM_MOVE, &move);
                    break;

                case YMF_Grenade:

                    /* ------------------------------------------------------
                    ** vorher muß dof in Wunschrichtung ausgerichtet werden,
                    ** im gegensatz zu Simple kein Ausgleich der Gravitation.
                    ** Typisch für Panzer
                    ** ----------------------------------------------------*/
                    move.schub_x = ymd->bact->dof.x;
                    move.schub_y = ymd->bact->dof.y;
                    move.schub_z = ymd->bact->dof.z;
                    move.t = time;
                    move.flags = 0;
                    _methoda( o, YBM_MOVE, &move);
                    break;

                case YMF_Simple:

                    /* ------------------------------------------------------
                    ** mit Ausgleich der Gravitation, aber ohne Ziel. Typisch
                    ** für ungelenkte Luftraketen
                    ** ----------------------------------------------------*/
                    move.t = time;
                    move.flags = 0;
                    ym_CalculateForce( ymd, &move);
                    _methoda( o, YBM_MOVE, &move);
                    break;

                case YMF_Search:

                    move.t = time;
                    move.flags = 0;
                    ym_CalculateForce( ymd, &move);
                    _methoda( o, YBM_MOVE, &move);
                    break;
                }

            /* ----------------------------------------------------------------
            ** Es gibt 2 Arten der Kollisionsverhütung. Einmal nur entlang
            ** der Flugrichtung, ein andermal mit Betrachtung der Umgebung.
            ** Bei letzterem muß beachtet werden, dß nicht das eigene träger-
            ** Objekt zerstört wird. Aber auf jeden Fall muß ein YWM_INTERSECT-
            ** Test gemacht werden. Vielleicht sollte man den bakterientest
            ** gleich als Röhre machen??? 
            ** --------------------------------------------------------------*/

            if( ym_CheckRohr( ymd )) {
                
                /* ich habe jemanden getroffen. Vorher Impulse! */
                _methoda( o, YMM_DOIMPULSE, NULL );

                /* Dann muß ich explodieren und mich dann verabschieden. Ich
                ** nehme den Dead-VP, der nach allen Seiten Explodiert. */
                state.main_state = ACTION_DEAD;
                state.extra_off =  0;
                state.extra_on =   0;
                _methoda(o, YBM_SETSTATE, &state );

                _methoda( o, YMM_RESETVIEWER, NULL);

                /* vorerst nix, weil das WO mich löscht */
                return;
                }

            /*** Falls Mine, dann raus hier, weil wir nur liegen, aber ***/
            /*** den Rohrtest machen müssen.                           ***/
            if( ymd->handle == YMF_Mine ) return;

            /*** jetzt kommt noch der Kollisionstest mit der Landschaft, ***/
            /*** die ich ja auch zerstören kann                          ***/

            inter.pnt.x = ymd->bact->old_pos.x;
            inter.pnt.y = ymd->bact->old_pos.y;
            inter.pnt.z = ymd->bact->old_pos.z;
            inter.vec.x = ymd->bact->pos.x - ymd->bact->old_pos.x;
            inter.vec.y = ymd->bact->pos.y - ymd->bact->old_pos.y;
            inter.vec.z = ymd->bact->pos.z - ymd->bact->old_pos.z;
            inter.flags = 0;
            _methoda( ymd->world, YWM_INTERSECT, &inter );
            if( inter.insect ) {

                /*
                ** Für diese Explosion nehme ich die gerichtete Explosion, also
                ** den MegaDeth-VP. Vorher muß ich das Objekt noch ausrichten.
                */
                am.vec.x = inter.sklt->PlanePool[ inter.pnum ].A;
                am.vec.y = inter.sklt->PlanePool[ inter.pnum ].B;
                am.vec.z = inter.sklt->PlanePool[ inter.pnum ].C;
                _methoda( o, YMM_ALIGNMISSILE_V, &am );
                    
                /*** die Explosion ist dort, wo das Hindernis ist. Klar! ***/
                ymd->bact->pos.x = inter.ipnt.x;
                ymd->bact->pos.y = inter.ipnt.y;
                ymd->bact->pos.z = inter.ipnt.z;

                /*** Viewer wieder an Schütze ***/
                _methoda( o, YMM_RESETVIEWER, NULL);

                /*** Flag zum Runterzählen setzen ***/
                ymd->flags  |= YMF_CountDelay;
                ymd->handle  = YMF_Mine;

                if( ymd->delay == 0 ) {

                    _methoda( o, YMM_DOIMPULSE, NULL );
                    
                    /*** Wenn nix, dann sofort ***/
                    ymd->bact->MainState = ACTION_DEAD;
                    state.extra_on   = EXTRA_MEGADETH;
                    state.extra_off  = 0;
                    state.main_state = 0;
                    _methoda(o, YBM_SETSTATE, &state );

                    /*** EnergieAbzug, wenn ich darf ***/
                    if( !( (ymd->flags & YMF_IgnoreBuildings) &&
                           (ymd->bact->Sector->WType) ) ) {

                        if( (ymd->ywd->URBact->Owner == (ULONG)ymd->bact->Owner) ||
                            (FALSE == ymd->ywd->playing_network) ) {

                            energy.pnt.x = ymd->bact->pos.x + 5 * ymd->bact->dof.x;
                            energy.pnt.z = ymd->bact->pos.z + 5 * ymd->bact->dof.z;
                            energy.energy = (LONG) ymd->bact->Energy;
                            // FIXME_FLOH!
                            energy.hitman = ymd->rifle_man;
                            _methoda( o, YBM_MODSECTORENERGY, &energy);
                            }
                        }
                    }

                /* ------------------------------------------------------
                ** Wenn mein Schütze Commander ist, dann gebe ich ihm die
                ** Einschusskoordinaten als Hauptziel! Dann greifen seine
                ** Slaves das Zeug auch an
                ** ---------------------------------------------------*/
                _get( ymd->rifle_man->BactObject, YBA_UserInput, &RMUI );
                if( RMUI ) {

                    if( (ymd->rifle_man->robo == ymd->rifle_man->master) && 
                        (ymd->rifle_man->robo != NULL) ) {

                        target.target_type = TARTYPE_SECTOR;
                        target.pos         = ymd->bact->pos;
                        target.priority    = 0;
                        _methoda( ymd->rifle_man->BactObject, YBM_SETTARGET, &target );
                        }
                    }

                return;
                }


            /*** Runterbrennen des Triebwerkes ***/
            ymd->drive_time -= msg->frame_time;
            if( ymd->drive_time < 0 ) {

                /*** In Bombe verwandeln ***/
                ymd->handle = YMF_Bomb;
                ymd->bact->bu_air_const = ymd->bact->air_const = 10;
                }


            /*** Interner Zeitzünder ***/
            ymd->life_time -= msg->frame_time;
            if( ymd->life_time < 0 ) {

                /*** Explodieren lassen ***/
                _methoda( o, YMM_DOIMPULSE, NULL );

                state.extra_on   = state.extra_off  = 0;
                state.main_state = ACTION_DEAD;
                _methoda(o, YBM_SETSTATE, &state );

                _methoda( o, YMM_RESETVIEWER, NULL );
                return;
                }
            

            /* -----------------------------------------------
            ** Rakete muß in Flugrichtung ausgerichtet werden.
            ** Wegen Netzwerk sollte das immer passieren. 
            ** ---------------------------------------------*/
          
            am.frame_time = ((FLOAT)msg->frame_time) / 1000.0;
            _methoda( o, YMM_ALIGNMISSILE_S, &am );
                

            break;

        case ACTION_DEAD:

            /*
            ** Explodieren macht der VisProto. Der wurde schon gesetzt.
            ** Jetzt zählen wir noch die Zeit runter und werden dann
            ** irgendwann zurückgegeben.
            */
            
            break;
        }
}



/*-----------------------------------------------------------------*/
_dispatcher(void, ym_YBM_MOVE, struct move_msg *move)
{
/*
**  FUNCTION
**
**
**  INPUTS
**
**      Die Kraft wird hier in Move übergeben
**
**  RESULTS
**
**      ---
**
**  CHANGED
**      29-Jun-95   8100000C    created
*/

    struct ypamissile_data *ymd;
    struct uvec grav;       /* Gravitations-Kraftvektor */
    struct uvec schub;      /* Schub-Kraftvektor */
    struct uvec lw;         /* Luftwiederstand */
    struct uvec res;
    FLOAT  delta_betrag, new_v_x, new_v_y, new_v_z, new_v_v;

    ymd = INST_DATA( cl, o);
    
    
    /*
    ** Zuerst wird immer die alte Position gemerkt. Immer !
    */
    ymd->bact->old_pos = ymd->bact->pos;

    /* ----------------------------------------------------------
    ** Gravitation wirkt immer nach unten. Bomben werden wie Tote
    ** behandeln
    ** --------------------------------------------------------*/
    grav.x = 0.0;
    grav.y = 1.0;
    grav.z = 0.0;
    if( (ACTION_DEAD == ymd->bact->MainState) ||
        (YMF_Bomb    == ymd->handle) )
        grav.v = GRAV_FACTOR * GRAVITY * ymd->bact->mass;
    else
        grav.v = GRAVITY * ymd->bact->mass;


    /*** Schub wirkt in Richtung lokale z-Achse ***/
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
        schub.v = ymd->bact->act_force;
        }

    /*** Luftwiderstand ***/
    lw.x = -ymd->bact->dof.x;
    lw.y = -ymd->bact->dof.y;
    lw.z = -ymd->bact->dof.z;
    lw.v = ymd->bact->dof.v * ymd->bact->air_const;

    /*** resultierender Kraftvektor ***/
    res.x = (grav.x*grav.v)+(schub.x*schub.v)+(lw.x*lw.v);
    res.y = (grav.y*grav.v)+(schub.y*schub.v)+(lw.y*lw.v);
    res.z = (grav.z*grav.v)+(schub.z*schub.v)+(lw.z*lw.v);
    res.v = nc_sqrt(res.x*res.x + res.y*res.y + res.z*res.z);

    /*** neuer dof - zum alten aufaddieren ***/
    
    if( res.v > 0.0 ) {

        delta_betrag = (res.v / ymd->bact->mass) * move->t;
        new_v_x = ymd->bact->dof.x * ymd->bact->dof.v + (res.x / res.v ) * delta_betrag;
        new_v_y = ymd->bact->dof.y * ymd->bact->dof.v + (res.y / res.v ) * delta_betrag;
        new_v_z = ymd->bact->dof.z * ymd->bact->dof.v + (res.z / res.v ) * delta_betrag;
        new_v_v = nc_sqrt( new_v_x * new_v_x + new_v_y * new_v_y + new_v_z * new_v_z );
        
        if( new_v_v > 0.0 ) {
            new_v_x /= new_v_v;
            new_v_y /= new_v_v;
            new_v_z /= new_v_v;
            }

        ymd->bact->dof.x = new_v_x;
        ymd->bact->dof.y = new_v_y;
        ymd->bact->dof.z = new_v_z;
        ymd->bact->dof.v = new_v_v;
        }


    /*
    ** Jetzt haben wir eine Neue Geschwindigkeit, da müssen wir aber noch
    ** die Position neu setzen. Die Ausrichtung wurde schon erledigt...
    */
    ymd->bact->pos.x += ymd->bact->dof.x * ymd->bact->dof.v * move->t * METER_SIZE;
    ymd->bact->pos.y += ymd->bact->dof.y * ymd->bact->dof.v * move->t * METER_SIZE;
    ymd->bact->pos.z += ymd->bact->dof.z * ymd->bact->dof.v * move->t * METER_SIZE;

    _methoda( o, YBM_CORRECTPOSITION, NULL );

}


/*-----------------------------------------------------------------*/
_dispatcher(void, ym_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**
**      Realisiert die Verwaltung der Oberfläche und die Auswertung
**      der Eingabeereignisse. Die Oberfläche wird hier nicht gezeichnet,
**      das macht TR_VISUAL. Die Oberfläche ist ein Array, welches
**      in diesem Quelltextfragment global definiert wird (das macht ja 
**      jede Subklasse neu).
**
**      Es werden hier 2 Ereignisse unterschieden. Das können zum einen
**      GameEvents und zum anderen Surfaceevents sein. Bei Surfaceevents
**      (mit rechter Maustaste) ist der Mauszeiger eingeblendet und ein
**      LMB war der Versuch, einen Teil der Oberfläche zu bedienen.
**
**  INPUTS
**
**  RESULTS
**
**      Ermittelt Kraftvektor für Flugverhalten (Move)
**
**  CHANGED
**      28-Jun-95   8100 000C    created
*/
{
    
    struct ypamissile_data *ymd = INST_DATA(cl,o);

    ymd->bact->old_pos = ymd->bact->pos;


    switch( ymd->bact->MainState ) {

        case ACTION_NORMAL:

            /*** Wenn die rakete nicht selbststeuernd ist, dann können ***/
            /*** wir sie auch nicht per Hand steuern.                  ***/
            _methoda( o, YBM_AI_LEVEL1, msg );

            break;

        case ACTION_DEAD:

            /* Aus Leichen werden wir sofort rausgeschaltet, dann auto-
            ** matisch weiter */
            _methoda( o, YMM_RESETVIEWER, NULL );

            break;
        }
    return;
    
}


void ym_rot_round_global_y( struct Bacterium *bact, FLOAT angle)
/// "rot um y"
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&(bact->dir), &rm, &neu_dir);

    bact->dir = neu_dir;
}
///

void ym_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
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

void ym_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
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

void ym_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle)
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



BOOL ym_CheckRohr( struct ypamissile_data *ymd )
{
    /* 
    ** macht einen Röhrentest mit einem Radius von explode_radius von
    ** old_pos nach pos. Der kann über mehrere Sektoren gehen, da die
    ** Rakete sehr schnell ist.
    ** Hierbei mache ich eine PrimitivVariante. Ich hole mir die Sektoren
    ** von old_pos und new_pos. Sind sie gleich, ist es gut. Andernfalls
    ** teile ich die Strecke durch 5 und bekomme damit 3 neue Sektorpointer.
    ** dann teste ich alle gefundenen Sektoren und lasse die, die schon mal
    ** bearbeitet wurden, aus (können nur vorherige sein).
    **
    ** 5 sind zuviel! 3 reichen! geändert am 30-Jan-96
    **
    ** neu 23-Apr-96 Exploderadius ist jetzt Radius !!!
    **
    */

    struct Cell *cell_array[3];
    struct getsectorinfo_msg gsi;
    struct Bacterium *kandidat;
    struct flt_triple pos, vp, richtung, weg, ziel, pt;
    FLOAT  produkt, entfernung_ok, entfernung_op;
    int    i, count, testcount;
    ULONG  USERINPUT, UI;
    BOOL   exploded = FALSE;
    struct ExtCollision *ecoll;
    FLOAT  radius_des_getroffenen, kandrad;

    pos.x = pos.z = pos.y  = 0.0;
    count                  = 0;
    radius_des_getroffenen = 0.0;


    /* -------------------------------------------------------------------
    ** Ich schieße Leute eigenen Owners nur ab, wenn der Schütze User ist.
    ** Das ist so, wenn er das UserInput-Flag gesetzt hat oder die Rakete
    ** viewer ist.
    ** -----------------------------------------------------------------*/
    _get( ymd->rifle_man->BactObject, YBA_UserInput, &USERINPUT );
    if( !USERINPUT ) _get( ymd->bact->BactObject, YBA_Viewer, &USERINPUT );


    /*** Die Sektorinfos ***/
    gsi.abspos_x = ymd->bact->old_pos.x;
    gsi.abspos_z = ymd->bact->old_pos.z;
    _methoda( ymd->world, YWM_GETSECTORINFO, &gsi);
    cell_array[0] = gsi.sector;

    gsi.abspos_x = ymd->bact->pos.x;
    gsi.abspos_z = ymd->bact->pos.z;
    _methoda( ymd->world, YWM_GETSECTORINFO, &gsi);
    cell_array[2] = gsi.sector;

    if( cell_array[0] == cell_array[2] ) {

        cell_array[1] = cell_array[0];
        }
    else {

        gsi.abspos_x = ymd->bact->old_pos.x + 
                       (ymd->bact->pos.x - ymd->bact->old_pos.x)/2;
        gsi.abspos_z = ymd->bact->old_pos.z +
                       (ymd->bact->pos.z - ymd->bact->old_pos.z)/2;
        _methoda( ymd->world, YWM_GETSECTORINFO, &gsi);
        cell_array[1] = gsi.sector;
        }

    /*** Ok, 3 potentielle Sektoren sind da. testen wir deren Bakterien ***/
    for( i=0; i<3; i++) {

        if( i > 0 ) {

            /*** testen, ob wir den Sektor schon mal hatten ***/
            if( cell_array[ i-1 ] == cell_array[ i ] )
                continue;
            }

        kandidat = (struct Bacterium *) cell_array[ i ]->BactList.mlh_Head;
        while( ((struct Node *)kandidat)->ln_Succ != 0 ) {

            /* ---------------------------------------------------------
            ** sind wir das selbst bzw. ist es unser Auftraggeber ? oder
            ** eine andere Rakete? 
            ** Den Tot-test mache ich mir ACTION_DEAD, nicht mit
            ** TESTDESTROY, weil das bei Brocken Probleme bringen kann!
            ** -------------------------------------------------------*/

            if( ( kandidat       == ymd->bact ) ||
                ( kandidat       == ymd->rifle_man ) ||
                ( BCLID_YPAMISSY == kandidat->BactClassID ) ||
                ( ACTION_DEAD    == kandidat->MainState ) ) {

                kandidat = (struct Bacterium *)
                           ((struct Node *)kandidat)->ln_Succ;
                continue;
                }
                
            /* -------------------------------------------------------
            ** Alle 100% RoboGuns ignorieren. Die liegen sonst wie ein
            ** Schild davor
            ** -----------------------------------------------------*/
            if( (BCLID_YPAGUN == kandidat->BactClassID) &&
                (100          >= kandidat->Shield) ) {
                
                ULONG rgun;
                _get( kandidat->BactObject, YGA_RoboGun, &rgun );
                if( rgun ) {
                
                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }
                }


            /*** Automatischer Schütze? ***/
            if( !USERINPUT )
                if( kandidat->Owner == ymd->rifle_man->Owner ) {

                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }

            /*** Als Bordkanone darf ich den Robo nicht kaputt machen ***/
            if( ymd->rifle_man->BactClassID == BCLID_YPAGUN ) {
                
                if( kandidat->Owner == ymd->bact->Owner ) {

                    LONG rg;

                    _get( ymd->rifle_man->BactObject, YGA_RoboGun, &rg );
                    if( rg ) {

                        /*** Treffer  Robo oder Borkanone? ***/
                        if( kandidat->BactClassID == BCLID_YPAROBO ) {

                            kandidat = (struct Bacterium *)
                                       ((struct Node *)kandidat)->ln_Succ;
                            continue;
                            }
                        
                        if( kandidat->BactClassID == BCLID_YPAGUN ) {
                    
                            _get( kandidat->BactObject, YGA_RoboGun, &rg );
                            if( rg ) {

                                kandidat = (struct Bacterium *)
                                           ((struct Node *)kandidat)->ln_Succ;
                                continue;
                                }
                            }
                        }
                    }
                }

            /* ------------------------------------------------------------
            ** wir testen nun, ob kandidat innerhalb der Röhre liegt.
            ** Dazu bilden wir das Vektorprodukt von der normierten
            ** Flugrichtung und der Entfernung zum kandidaten von
            ** old_pos aus. Der Betrag muß kleiner als die Summe von
            ** explode_radius und kandidat->radius sein. Weiterhin muß
            ** die Entfernung kleiner als die Pythagorassumme von
            ** dem Betrag des VP und der Entfernung old->pos sein, denn mit
            ** dem VP allein ist es noch nicht getan. 
            **
            ** Erweiterung: wir holen die Punkte auch aus einer ExtColl.
            ** ----------------------------------------------------------*/

            _get( kandidat->BactObject, YBA_ExtCollision, &ecoll );
        
            if( ecoll )
                testcount = ecoll->number;
            else
                testcount = 1;

            /*** Nun alle Testpunkte ***/
            while( testcount-- ) {
            
                FLOAT weapon_radius;

                /*** Wo kommt der Punkt her? ***/
                if( ecoll ) {

                    /*** Der Punkt ***/
                    pt     = kandidat->pos;
                    pt.x += ( kandidat->dir.m11 * ecoll->points[ testcount ].point.x +
                              kandidat->dir.m21 * ecoll->points[ testcount ].point.y +
                              kandidat->dir.m31 * ecoll->points[ testcount ].point.z );
                    pt.y += ( kandidat->dir.m12 * ecoll->points[ testcount ].point.x +
                              kandidat->dir.m22 * ecoll->points[ testcount ].point.y +
                              kandidat->dir.m32 * ecoll->points[ testcount ].point.z );
                    pt.z += ( kandidat->dir.m13 * ecoll->points[ testcount ].point.x +
                              kandidat->dir.m23 * ecoll->points[ testcount ].point.y +
                              kandidat->dir.m33 * ecoll->points[ testcount ].point.z );
                    
                    /*** Radius ***/
                    if( ecoll->points[ testcount ].radius < 0.01 ) continue;
                    kandrad = ecoll->points[ testcount ].radius;
                    }
                else {

                    pt      = kandidat->pos;
                    kandrad = kandidat->radius;
                    }

                /*** der zurückgelegt Weg ***/
                weg.x = ymd->bact->pos.x - ymd->bact->old_pos.x;
                weg.y = ymd->bact->pos.y - ymd->bact->old_pos.y;
                weg.z = ymd->bact->pos.z - ymd->bact->old_pos.z;

                /*** Der Vektor zum Testpunkt ***/
                ziel.x = pt.x - ymd->bact->old_pos.x;
                ziel.y = pt.y - ymd->bact->old_pos.y;
                ziel.z = pt.z - ymd->bact->old_pos.z;

                /*** Hinter mir? ***/
                if( (ziel.x * ymd->bact->dir.m31 + ziel.y * ymd->bact->dir.m32 +
                     ziel.z * ymd->bact->dir.m33) < 0.3 ) {

                    /*** gleich weiter ***/
                    continue;
                    }

                /*** Die Entfernungen ***/
                entfernung_op = nc_sqrt( weg.x*weg.x + weg.y*weg.y + weg.z*weg.z);
                entfernung_ok = nc_sqrt(ziel.x*ziel.x + ziel.y*ziel.y + ziel.z*ziel.z);
            
                richtung.x = weg.x / entfernung_op;
                richtung.y = weg.y / entfernung_op;
                richtung.z = weg.z / entfernung_op;
            
                /*** Das Vektorprodunkt --> Entfernung zur Bewegungsachse ***/
                vp.x = richtung.y * ziel.z - ziel.y * richtung.z;
                vp.y = richtung.z * ziel.x - ziel.z * richtung.x;
                vp.z = richtung.x * ziel.y - ziel.x * richtung.y;
                
                switch( kandidat->BactClassID ) {
                        
                    case BCLID_YPABACT:     weapon_radius = (FLOAT)ymd->radius_heli; break;
                    case BCLID_YPATANK:
                    case BCLID_YPACAR:      weapon_radius = (FLOAT)ymd->radius_tank; break;
                    case BCLID_YPAFLYER:
                    case BCLID_YPAUFO:      weapon_radius = (FLOAT)ymd->radius_flyer; break;
                    case BCLID_YPAROBO:     weapon_radius = (FLOAT)ymd->radius_robo; break;
                    default:                weapon_radius = (FLOAT)ymd->bact->radius; break;
                    }
                if( 0.0 == weapon_radius ) weapon_radius =  (FLOAT)ymd->bact->radius;   
 
                if( (produkt = nc_sqrt(vp.x * vp.x + vp.y * vp.y + vp.z * vp.z) ) <
                    (kandrad + weapon_radius ) ) {

                    /* ----------------------------------------------------------
                    ** Das Objekt liegt nah genug an der Strecke. Nun testen wir,
                    ** ob es auch innerhalb der zurückgelegten Strecke liegt 
                    ** --------------------------------------------------------*/

                    if( nc_sqrt( entfernung_op*entfernung_op + produkt*produkt ) >
                        fabs( entfernung_ok - weapon_radius) ) {

                        LONG   en;
                        struct modvehicleenergy_msg en_msg;
                        struct Bacterium *URBact;
                        Object *urobo;

                        _get( ymd->world, YWA_UserRobo, &urobo );
                        _get( urobo, YBA_Bacterium, &URBact );

                        /* ---------------------------------------------------
                        ** Tatsächlich! Der kriegt was ab.
                        ** Wir verlassen dann aber auch die Testpunktschleife,
                        ** weil wir ja nicht 2x abziehen wollen.
                        ** -------------------------------------------------*/
                        exploded = TRUE;
                        kandidat->ExtraState &= ~EXTRA_LANDED;  // !!!

                        pos.x += kandidat->pos.x;
                        pos.y += kandidat->pos.y;
                        pos.z += kandidat->pos.z;
                        count++;

                        /*** Radius merken wir uns auch ***/
                        radius_des_getroffenen += kandrad;

                        /* ------------------------------------------------
                        ** unschöne Sache wegen _get. Aber ich muß den User
                        ** schonen.
                        ** ----------------------------------------------*/

                        _get( kandidat->BactObject, YBA_UserInput, &UI );   
                        switch( kandidat->BactClassID ) {
                        
                            case BCLID_YPABACT:     en = (LONG)( ymd->energy_heli  * ((FLOAT)ymd->bact->Energy) ); break;
                            case BCLID_YPATANK:
                            case BCLID_YPACAR:      en = (LONG)( ymd->energy_tank  * ((FLOAT)ymd->bact->Energy) ); break;
                            case BCLID_YPAFLYER:
                            case BCLID_YPAUFO:      en = (LONG)( ymd->energy_flyer * ((FLOAT)ymd->bact->Energy) ); break;
                            case BCLID_YPAROBO:     en = (LONG)( ymd->energy_robo  * ((FLOAT)ymd->bact->Energy) ); break;
                            default:                en = ymd->bact->Energy; break;
                            }
                                   
                        if( UI || (kandidat->ExtraState & EXTRA_ISVIEWER) )
                            en = (en * (100 - kandidat->Shield) )/250;
                        else
                            en = (en * (100 - kandidat->Shield) )/100;

                        en_msg.energy = -en;
                        en_msg.killer = ymd->rifle_man;

                        /* -------------------------------------------------
                        ** Energieabzug nur, wenn dies hier die originale
                        ** Waffe ist! Die anderen explodieren nur einfach so
                        ** -----------------------------------------------*/
                        if( (URBact->Owner == ymd->bact->Owner) ||
                            (FALSE == ymd->ywd->playing_network) )
                            _methoda( kandidat->BactObject, YBM_MODVEHICLEENERGY, &en_msg );

                       /*** Wir hatten einen Treffer und gehen deshalb raus ***/
                       break; 
                       } 
                   } 
               }

            /* war nix. Nächster */
            kandidat = (struct Bacterium *) ((struct Node *)kandidat)->ln_Succ;
            }
        }

    if( exploded ) {

        /* ----------------------------------------------------------------
        ** Die Explosion soll im Mittelpunkt aller getroffenen stattfinden,
        ** sofern es mehrere Treffer gab. Dann verschieben wir die Position
        ** noch um den durchschnittlichen Radius in die Richtung, aus der
        ** wir kamen.
        ** (count müßte größer 0 sein)
        ** --------------------------------------------------------------*/

        FLOAT f;

        /*** pos setzen ... ***/
        ymd->bact->pos.x = pos.x / count;
        ymd->bact->pos.y = pos.y / count;
        ymd->bact->pos.z = pos.z / count;

        radius_des_getroffenen /= count;

        if( radius_des_getroffenen >= 50 ) {

            /*** ... und verschieben ***/
            f = nc_sqrt( (ymd->bact->pos.x - ymd->bact->old_pos.x) *
                         (ymd->bact->pos.x - ymd->bact->old_pos.x) +
                         (ymd->bact->pos.y - ymd->bact->old_pos.y) *
                         (ymd->bact->pos.y - ymd->bact->old_pos.y) +
                         (ymd->bact->pos.z - ymd->bact->old_pos.z) *
                         (ymd->bact->pos.z - ymd->bact->old_pos.z) );

            if( f < 1.0 ) f = 1.0;  // kann ja 'n Fehler auftreten, ne?

            ymd->bact->pos.x -= (ymd->bact->pos.x - ymd->bact->old_pos.x) *
                                radius_des_getroffenen / f;
            ymd->bact->pos.y -= (ymd->bact->pos.y - ymd->bact->old_pos.y) *
                                radius_des_getroffenen / f;
            ymd->bact->pos.z -= (ymd->bact->pos.z - ymd->bact->old_pos.z) *
                                radius_des_getroffenen / f;
            }
        }
    
    return( exploded );
}


void ym_CalculateForce(struct ypamissile_data *ymd, struct move_msg *move)
{
    /* ---------------------------------------------------------------------
    ** berechnet die Schubkraft nach tar_unit. Kompensiert dabei
    ** die Erdanziehungskraft. Es wird von act_force = max_force ausgegangen
    ** und somit hier auch gleich so gesetzt.
    ** -------------------------------------------------------------------*/

    FLOAT betrag;

    ymd->bact->act_force = ymd->bact->max_force;

    /* ---------------------------------------------------------------------
    ** Ich mache es mir einfach. ich gehe davon aus, da0 die resultierende
    ** Kraft act_force ist. dann berechne ich damit die Schubkraft, die sich
    ** davon nicht all zu sehr unterscheiden wird. Dann normiere ich das
    ** Zeug und schreibe es in move.
    ** -------------------------------------------------------------------*/


    move->schub_x = ymd->bact->act_force * ymd->bact->tar_unit.x +
                    ymd->bact->dof.x * ymd->bact->dof.v * ymd->bact->air_const;
    move->schub_z = ymd->bact->act_force * ymd->bact->tar_unit.z +
                    ymd->bact->dof.z * ymd->bact->dof.v * ymd->bact->air_const;
    move->schub_y = ymd->bact->act_force * ymd->bact->tar_unit.y -
                    ymd->bact->mass * GRAVITY +
                    ymd->bact->dof.y * ymd->bact->dof.v * ymd->bact->air_const;

    betrag = nc_sqrt( move->schub_x*move->schub_x + move->schub_y*move->schub_y +
                      move->schub_z*move->schub_z );

    if( betrag > 0.0 ) {

        betrag         = 1 / betrag;
        move->schub_x *= betrag;
        move->schub_y *= betrag;
        move->schub_z *= betrag;
        }
}


_dispatcher( void, ym_YMM_ALIGNMISSILE_S, struct alignmissile_msg *am )
{
    /* -------------------------------------------------------------------
    ** Richtet Rakete aus. Trotz alledem (bevor es Vorzeichenfehler gibt):
    ** Wir drehen old_lokal_z auf dof 
    ** -----------------------------------------------------------------*/

    struct ypamissile_data *ymd;
    FLOAT  rot_x, rot_y, rot_z, new_x, new_y, new_z, old_x, old_y, old_z;
    struct flt_m3x3 nm, rm;
    FLOAT  ca, sa, angle, betrag;

    ymd = INST_DATA( cl, o );

    new_x = ymd->bact->dof.x;
    new_y = ymd->bact->dof.y;
    new_z = ymd->bact->dof.z;

    if( (new_x==0.0) && (new_y==0.0) && (new_z==0.0) ) return;

    old_x = ymd->bact->dir.m31;
    old_y = ymd->bact->dir.m32;
    old_z = ymd->bact->dir.m33;

    rot_y = new_x * old_z - old_x * new_z;
    rot_x = old_y * new_z - old_z * new_y;
    rot_z = old_x * new_y - old_y * new_x;

    /* rot muß normiert sein */
    betrag = nc_sqrt( rot_x * rot_x + rot_y * rot_y + rot_z * rot_z );

    if( betrag > 0.0 ) {

        /*
        ** denn sonst war keine Rotation notwendig, weil beide Vektoren
        ** identisch sind.
        */
        betrag = 1 / betrag;
        rot_x *= betrag;
        rot_y *= betrag;
        rot_z *= betrag;

        /* old und new dürften normiert sein... */
        angle = nc_acos( old_x * new_x + old_y * new_y + old_z * new_z);

        /*** Bomben richten sich mit MaxRot aus ***/
        if( (ymd->handle == YMF_Bomb) && am ) {

            if( angle < -(ymd->bact->max_rot * am->frame_time) )
                angle = -(ymd->bact->max_rot * am->frame_time);
            if( angle >   ymd->bact->max_rot * am->frame_time )
                angle =   ymd->bact->max_rot * am->frame_time;
            }

        if( (angle > 0.01) || (angle < -0.01) )  {

            /* nur dann lohnt sich die Rotation */
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

            nc_m_mul_m( &(ymd->bact->dir), &rm, &nm );

            ymd->bact->dir = nm;
            }
        }

    /*** Wenn Viewer, dann noch eindrehen ***/
    if( ymd->flags & YMF_Viewer ) {

        /*** jetzige Seitenneigung (W. zw. x-z-Ebene und lok. x-Achse) ***/
        angle = nc_acos( nc_sqrt(ymd->bact->dir.m11 * ymd->bact->dir.m11 +
                                 ymd->bact->dir.m13 * ymd->bact->dir.m13 ));

        /*** Kopfstehend? also y nach oben ***/
        if( ymd->bact->dir.m22 < 0.0 )
            angle = PI - angle;

        /*** Winkel pos oder neg? ***/
        if( ymd->bact->dir.m12 < 0.0 )
            angle = -angle;

        ym_rot_round_lokal_z( ymd->bact, -angle );
        }
}


_dispatcher( void, ym_YMM_ALIGNMISSILE_V, struct alignmissile_msg *am )
{
    /*
    ** Richtet Objekt an Hand des übergebenen EbenenNormalenvektors aus
    */

    FLOAT old_x, old_y, old_z, new_x, new_y, new_z, rot_x, rot_y, rot_z;
    FLOAT betrag, sa, ca, angle;
    struct flt_m3x3 nm, rm;
    struct ypamissile_data *ymd = INST_DATA( cl, o );

    old_x = ymd->bact->dir.m21;
    old_y = ymd->bact->dir.m22;
    old_z = ymd->bact->dir.m23;

    new_x = am->vec.x;
    new_y = am->vec.y;
    new_z = am->vec.z;

    rot_y = new_x * old_z - old_x * new_z;
    rot_x = old_y * new_z - old_z * new_y;
    rot_z = old_x * new_y - old_y * new_x;

    betrag = nc_sqrt( rot_x * rot_x + rot_y * rot_y + rot_z * rot_z );

    if( betrag != 0 ) {

        /*
        ** denn sonst war keine Rotation notwendig, weil beide Vektoren
        ** identisch sind.
        */

        rot_x /= betrag;
        rot_y /= betrag;
        rot_z /= betrag;

        /* old und new dürften normiert sein... */
        angle = nc_acos( old_x * new_x + old_y * new_y + old_z * new_z);

        if( fabs( angle ) > 0.02 )  {

            /* nur dann lohnt sich die Rotation */
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

            nc_m_mul_m( &(ymd->bact->dir), &rm, &nm );

            ymd->bact->dir = nm;
            }
        }
}   


