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
#include "ypa/ypacarclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine


#define TEST_DISTANCE   300.0       /* maximal 300 ! */
#define KOLL_FAKTOR     0.2
#define TARGET_DISTANCE 400.0       /* Quadrat der Entfernung zum Ziel, inner-
                                    ** halb derer gebremst wird */
#define TEST_SEITE      30.0
#define NO_SPEED        4.0
#define ZUR_SEITE       15.0        /* für Panzerstrahlen (Kollision) */


/*-----------------------------------------------------------------*/
void yc_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yc_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle);
BOOL yc_BactCollision(struct ypacar_data *ycd, FLOAT time );
void yc_RotateTank( struct Bacterium *bact, FLOAT angle, UBYTE wie );
void yc_DoKamikaze( struct ypacar_data *ycd );


_dispatcher(void, yc_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION    Jetzt gibt es doch noch ein AI3, welches aber nichts
**              weiter macht, als zu warten. Der grund ist das Kamikaze-
**              verhalten.
**              Sind wir kein Kamikaze, geht es sofort an AI3 des
**              Panzers.
*/

    struct ypacar_data *ycd = INST_DATA( cl, o );
    struct alignvehicle_msg avm;

    if( !( ycd->kamikaze ) ) {

        _supermethoda( cl, o, YBM_AI_LEVEL3, msg );
        return;
        }

    /*** So, nur warten ***/
    switch( ycd->bact->MainState ) {

        case ACTION_NORMAL:

            /*** keine Bakterienkollision notwendig, nur beim fallen ***/

            /*** Fallen lassen ***/
            if( !(ycd->bact->ExtraState & EXTRA_LANDED) ) {

                struct crash_msg crash;

                crash.flags      = CRASH_CRASH;
                crash.frame_time = msg->frame_time;
                _methoda( o, YBM_CRASHBACTERIUM, &crash);
                return;
                }

            /*** kein Break, weil warten auch bei ACTION_WAIT ***/

        /*-------------------------------------------------------------------*/

        case ACTION_WAIT:

            avm.time      = ((FLOAT)(msg->frame_time)) / 1000.0;
            avm.old_dir.x = ycd->bact->dir.m31;
            avm.old_dir.y = ycd->bact->dir.m32;
            avm.old_dir.z = ycd->bact->dir.m33;
            _methoda( o, YTM_ALIGNVEHICLE_A, &avm );
            break;

        /*-------------------------------------------------------------------*/

        case ACTION_DEAD:

            //if( !(ycd->bact->ExtraState & EXTRA_LANDED ) ) {
            //
            //    struct crash_msg crash;
            //
            //    crash.flags      = CRASH_CRASH;
            //    crash.frame_time = msg->frame_time;
            //    _methoda( o, YBM_CRASHBACTERIUM, &crash);
            //    }
            //else {
            //
            //    LONG YLS;
            //
            //    /*** Rückgabe des Leergutes ***/
            //    _get( o, YBA_YourLS, &YLS);
            //    YLS -= msg->frame_time;
            //    _set( o, YBA_YourLS, YLS );
            //
            //    if( YLS <= 0 )
            //        _methoda( o, YBM_RELEASEVEHICLE, o );
            //    }
            
            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;

        /*-------------------------------------------------------------------*/

        case ACTION_CREATE:

            
            /*** Zeuch machen ***/
            _methoda( o, YBM_DOWHILECREATE, msg );

            break;

        /*-------------------------------------------------------------------*/

        case ACTION_BEAM:

            _methoda( o, YBM_DOWHILEBEAM, msg );
            break;
        }

}

/*-----------------------------------------------------------------*/
_dispatcher(void, yc_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
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
**      28-Jun-95   8100 000C   created
**      01-Oct-97   floh        spezieller Joystick-Handler-Code
**                              von Tank-Class übernommen
*/
{
    
    FLOAT  time, angle, spf;
    struct move_msg move;
    struct ypacar_data *ycd = INST_DATA(cl,o);
    struct intersect_msg inter;
    ULONG  BACTCOLL;
    struct firemissile_msg fire;
    struct setstate_msg state;
    struct bactcollision_msg bcoll;
    struct crash_msg crash;
    struct alignvehicle_msg avm;
    struct fireyourguns_msg fyg;
    struct visier_msg visier;
    LONG   YLS;


    /*
    ** Wie fährt ein Auto?
    */

    ycd->bact->air_const = ycd->bact->bu_air_const;

    _get( o, YBA_BactCollision, &BACTCOLL);
    time = (FLOAT) msg->frame_time/1000;
    ycd->bact->old_pos = ycd->bact->pos;
    avm.time    = time;
    avm.old_dir.x = ycd->bact->dir.m31;
    avm.old_dir.y = ycd->bact->dir.m32;
    avm.old_dir.z = ycd->bact->dir.m33;

    switch( ycd->bact->MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:
            
            /* ---------------------------------------------------------------
            ** Wieder zuerst die Bewegung. Wir merken uns die alte Position
            ** und berechnen die neue. Dann erst machen wir den Kollisionstest
            ** und setzen evtl. wieder die alte Position.
            ** -------------------------------------------------------------*/

            /*** Untergebene ausbremsen ***/
            if( ycd->bact->dof.v == 0.0 ) {

                /*** Wenn wir in Zielnähe sind, warten wir nur "scheinbar" ! ***/
                if( (ycd->bact->PrimTargetType == TARTYPE_SECTOR) &&
                    (nc_sqrt( (ycd->bact->PrimPos.x - ycd->bact->pos.x ) * 
                              (ycd->bact->PrimPos.x - ycd->bact->pos.x ) +
                              (ycd->bact->PrimPos.z - ycd->bact->pos.z ) * 
                              (ycd->bact->PrimPos.z - ycd->bact->pos.z ) ) >
                             (2 * FOLLOW_DISTANCE)) ) {
                    
                    /*** weit weg ***/
                    if( ycd->bact->ExtraState & EXTRA_FIRE )
                        ycd->bact->MainState = ACTION_WAIT;
                    else {
                        
                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                else {

                    /*** Am Kampfort ***/
                    if( !(ycd->bact->ExtraState & EXTRA_FIRE)) {

                         state.main_state = ACTION_WAIT;
                         state.extra_on = state.extra_off = 0;
                         _methoda( o, YBM_SETSTATE, &state );
                         }
                    ycd->bact->MainState = ACTION_NORMAL;    // !!
                    }
                }
            else {

                if( !(ycd->bact->ExtraState & EXTRA_FIRE) ) {

                    state.main_state = ACTION_NORMAL;
                    state.extra_on = state.extra_off = 0;
                    _methoda( o, YBM_SETSTATE, &state );
                    }
                }

            if( (msg->input->Slider[ SL_DRV_DIR ] != 0.0) &&
                (ycd->bact->dof.v != 0) ) {

                /*** Ok, drehen wir das Ding um seine lokale (!) y-Achse ***/
                spf   = nc_sqrt( fabs( ycd->bact->dof.v ) ) / 5.0;
                angle = -msg->input->Slider[ SL_DRV_DIR ] * 
                         ycd->bact->max_rot * time * spf;

                /*** Drehen, dann am Untergrund ausrichten und tschüß ***/
                yc_rot_round_lokal_y( ycd->bact, angle );
                }
            
            /* ----------------------------------------------------------------
            ** Es war kein Drehen und kein Abprallen, also fragen wir mal
            ** ab, ob wir beschleunigen oder bremsen sollen. Deshalb lassen wir
            ** auch negative Kräfte zu. Dann fahren wir rückwärts. 
            ** --------------------------------------------------------------*/
            /*** Beschleunigung oder Bremsmanöver? ***/
            //if( ( ( msg->input->Slider[ SL_DRV_SPEED ] < 0 ) &&
            //      ( ycd->bact->act_force > 0 ) ) ||
            //    ( ( msg->input->Slider[ SL_DRV_SPEED ] > 0 ) &&
            //      ( ycd->bact->act_force < 0 ) ) ) {
            if( ( ( ycd->bact->dof.v < 0 ) &&
                  ( ycd->bact->act_force > 0 ) ) ||
                ( ( ycd->bact->dof.v > 0 ) &&
                  ( ycd->bact->act_force < 0 ) ) ) {

                if( fabs( ycd->bact->dof.y ) > IS_SPEED ) {

                    /*** nur dann, weil sonst Probleme mit Stand ***/
                    ycd->bact->dof.v *= max( 0.1, (1 - 4 * time));
                    }
                }

            /*** FIXME_FLOH: spezieller Joystick-"Gaspedal"-Hack ***/
            if (msg->input->Buttons & (1<<31)) {
                /*** Joystick-Control an, Special Hack ***/
                ycd->bact->act_force = ycd->bact->max_force * -msg->input->Slider[13];
                if (fabs(msg->input->Slider[13]) > 0.1) {
                    ycd->bact->ExtraState |= EXTRA_MOVE;
                };
            } else {
                ycd->bact->act_force += 0.75 * time * ycd->bact->max_force *
                                        msg->input->Slider[ SL_DRV_SPEED ];
            };
            
            if( ycd->bact->act_force > ycd->bact->max_force )
                ycd->bact->act_force = ycd->bact->max_force;

            if( ycd->bact->act_force < -ycd->bact->max_force )
                ycd->bact->act_force = -ycd->bact->max_force;

            if( msg->input->Slider[ SL_DRV_SPEED ] != 0.0)
                ycd->bact->ExtraState |= EXTRA_MOVE;


            /*** Kanone nach oben/unten ***/
            ycd->bact->gun_angle_user += (time * msg->input->Slider[ SL_GUN_HEIGHT ]);
            
            if( ycd->bact->gun_angle_user < -0.3)
                ycd->bact->gun_angle_user = -0.3;
            if( ycd->bact->gun_angle_user >  0.8)
                ycd->bact->gun_angle_user =  0.8;


            /*** Zielaufnahme, die machen wir immer (vorher default) ***/
            fire.target_type  = TARTYPE_SIMPLE;
            fire.target_pos.x = ycd->bact->dir.m31;
            fire.target_pos.y = ycd->bact->dir.m32;
            fire.target_pos.z = ycd->bact->dir.m33;

            visier.flags = VISIER_ENEMY | VISIER_NEUTRAL;
            visier.dir.x = ycd->bact->dir.m31;
            visier.dir.y = ycd->bact->dir.m32 - ycd->bact->gun_angle_user;
            visier.dir.z = ycd->bact->dir.m33;
            if( _methoda( o, YBM_VISIER, &visier ) ) {

                fire.target_type = TARTYPE_BACTERIUM;
                fire.target.bact = visier.enemy;
                }

            /*** Gamf ***/
            if( (msg->input->Buttons & BT_FIRE ) ||
                (msg->input->Buttons & BT_FIREVIEW ) ) {

                if( ycd->kamikaze ) {

                    /*** Kamikazeverhalten ***/
                    yc_DoKamikaze( ycd );
                    }
                else {

                    /*** Schießen Rakete ***/
                    fire.wtype       = ycd->bact->auto_ID;
                    fire.dir.x       = ycd->bact->dir.m31 - ycd->bact->dir.m21 *
                                                            ycd->bact->gun_angle_user;
                    fire.dir.y       = ycd->bact->dir.m32 - ycd->bact->dir.m22 *
                                                            ycd->bact->gun_angle_user;
                    fire.dir.z       = ycd->bact->dir.m33 - ycd->bact->dir.m23 *
                                                            ycd->bact->gun_angle_user;
                    fire.global_time = ycd->bact->internal_time;
                    /*** Tank. etwas höher ***/
                    if( ycd->bact->internal_time % 2 == 0 )
                        fire.start.x     = ycd->bact->firepos.x;
                    else
                        fire.start.x     = -ycd->bact->firepos.x;
                    fire.start.y     = ycd->bact->firepos.y;
                    fire.start.z     = ycd->bact->firepos.z;
                    /* Viewer? */
                    if( msg->input->Buttons & BT_FIREVIEW )
                        fire.flags   = FIRE_VIEW;
                    else
                        fire.flags   = 0;
                    fire.flags      |= FIRE_CORRECT;
                    _methoda( o, YBM_FIREMISSILE, &fire );
                    }
                }


            /*** Schießen, Kanone ***/
            if( ycd->bact->mg_shot != NO_MACHINEGUN ) {

                if( (ycd->bact->ExtraState & EXTRA_FIRE) &&
                    (!(msg->input->Buttons & BT_FIREGUN)) ) {

                    /*** Feuern aus ***/
                    // Abmelden beim WO

                    state.main_state = state.extra_on = 0;
                    state.extra_off  = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                if( msg->input->Buttons & BT_FIREGUN ) {

                    /*** Feuern einschalten? ***/
                    if( !(ycd->bact->ExtraState & EXTRA_FIRE) ) {

                        // Anmelden WO

                        state.main_state = state.extra_off = 0;
                        state.extra_on   = EXTRA_FIRE;
                        _methoda( o, YBM_SETSTATE, &state );
                        }

                    /*** Feuern an sich ***/
                    fyg.dir.x = ycd->bact->dir.m31 - ycd->bact->dir.m21 *
                                                     ycd->bact->gun_angle_user;
                    fyg.dir.y = ycd->bact->dir.m32 - ycd->bact->dir.m22 *
                                                     ycd->bact->gun_angle_user;
                    fyg.dir.z = ycd->bact->dir.m33 - ycd->bact->dir.m23 *
                                                     ycd->bact->gun_angle_user;
                    fyg.time  = time;
                    fyg.global_time = ycd->bact->internal_time;
                    _methoda( o, YBM_FIREYOURGUNS, &fyg );
                    }
                }

            /* ----------------------------------
            ** Fallen wir? Dann laßt uns fallen !
            ** --------------------------------*/
            if( !(ycd->bact->ExtraState & EXTRA_LANDED ) ) {

                /* ein Landen gibts beim Panzer nicht! */
                crash.flags      = CRASH_CRASH;
                crash.frame_time = msg->frame_time;
                _methoda( o, YBM_CRASHBACTERIUM, &crash);
                return;
                }


            if( msg->input->Buttons & BT_STOP ) {

                /* --------------------------------------------------------
                ** Bremsen. Wir reduzieren den Betrag der Geschwindigkeit
                ** und setzen ihn 0, wenn wir unter einer bestimmten Grenze
                ** sind. Die Kraft setzen wir 0. 
                ** ------------------------------------------------------*/

                ycd->bact->act_force = 0.0;

                if( fabs( ycd->bact->dof.v ) < NO_SPEED ) {
                    ycd->bact->ExtraState &= ~EXTRA_MOVE;
                    ycd->bact->dof.v = 0.0;
                    }
                else
                    ycd->bact->dof.v *= max( 0.1, (1 - 4 * time));

                move.flags = MOVE_STOP;
                }
            else move.flags = 0;
            
            /* ------------------------------------------------------
            ** So, nun fahren wir. Fahrtrichtung ist immer die lokale
            ** z-Achse.
            ** ----------------------------------------------------*/

            move.t = (FLOAT) msg->frame_time / 1000;
            move.flags = 0;
            if( ycd->bact->ExtraState & EXTRA_MOVE )
                _methoda( o, YBM_MOVE, &move );
            
            
            /*
            **  direkte Kollision mit den Bakterien
            */
            if( BACTCOLL ) {

                bcoll.frame_time = msg->frame_time;
                if( _methoda( o, YBM_BACTCOLLISION, &bcoll) ) {
                    _methoda( o, YTM_ALIGNVEHICLE_U, &avm );
                    return;
                    }
                }
            
            /* 
            ** Andernfalls kann es immer noch passieren, daß wir unter
            ** die Welt geknallt sind 
            */
            inter.pnt.x = ycd->bact->old_pos.x;
            inter.pnt.y = ycd->bact->old_pos.y;
            inter.pnt.z = ycd->bact->old_pos.z;
            inter.vec.x = ycd->bact->pos.x - ycd->bact->old_pos.x;
            inter.vec.y = ycd->bact->pos.y - ycd->bact->old_pos.y;
            inter.vec.z = ycd->bact->pos.z - ycd->bact->old_pos.z;
            inter.flags = 0;
            _methoda( ycd->world, YWM_INTERSECT, &inter );
            if( inter.insect ) {

                ycd->bact->pos = ycd->bact->old_pos;
                ycd->bact->dof.v     = 0.0;
                ycd->bact->act_force = 0.0;
                return;
                }

            /* 
            ** Der Kollisionstest. Den machen wir hier einfacher. Für den Fall,
            ** daß wir irgendwo runterstürzen, gibts dann ja den Zustand CRASH.
            ** Wir schicken einen kurzen Strahl voraus. Ist die y-Komponente des
            ** Normalenvektors der Schnittebene vom Betrag her zu klein, ist die
            ** Ebene zu steil und wir bleiben stehen. Andernfalls können wir fahren 
            ** und neigen danach das Fahrzeug. Means, wir schieben es einfach mal
            ** vorwärts und richten es danach am Untergrund aus (auch Absturz) 
            ** Achtung! Bei jeder Kollision müssen wir beachten, daß die 
            ** Testpunkte auch außerhalb liegen müssen! Deshalb nehmen wir als
            ** Anfangspunkte für die Kollision die Testpunkte!!!
            */

            _methoda( o, YTM_ALIGNVEHICLE_U, &avm);


            break;

        case ACTION_DEAD:
            
            //if( !(ycd->bact->ExtraState & EXTRA_LANDED ) ) {
            //
            //    crash.flags      = CRASH_CRASH;
            //    crash.frame_time = msg->frame_time;
            //    _methoda( o, YBM_CRASHBACTERIUM, &crash);
            //    }
            //else {
            //
            //    /* -------------------------------------------------------
            //    ** Wir sind gelandet und schalten noch den MegaTötler ein,
            //    ** wenn vorher vp_dead war! Dann geht das Objekt ans WO!
            //    ** -----------------------------------------------------*/
            //    state.extra_on  = EXTRA_MEGADETH;
            //    state.extra_off = state.main_state = 0;
            //    _methoda( o, YBM_SETSTATE, &state);
            //
            //    /*** Rückgabe des Leergutes ***/
            //    _get( o, YBA_YourLS, &YLS );
            //    if( YLS <= 0 ) ycd->bact->ExtraState |= EXTRA_DONTRENDER;
            //    }
            
            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;
        }
    return;
    
}



void yc_DoKamikaze( struct ypacar_data *ycd )
{
    /*
    ** Wir wirken in einem bestimmten Bereich auf die Welt und auf die
    ** vehicle.
    ** Welt: Dazu lasse ich auf eine Kreisfläche, die durch Punkte im
    ** Abstand einer Subsektorlänge gegeben ist, Energieabzüge wirken.
    ** Der radius und die Wirkung ist abhängig von der Sprengladung.
    **
    ** Vehicle: Aus dem gleichen Kreis ermittle ich alle Sektoren
    ** und scanne die bactListen.
    **
    ** MSE geht itze och übers Netz
    */

    FLOAT radius, abstand, betrag;
    LONG  anzahl, i, j;
    struct ypaworld_data *ywd;
    struct setstate_msg state;

    /*
    ** Die Wirkungsreichweite lege ich so, daß nach SECTOR_SIZE / 4 die
    ** Wirkung sich halbiert hat. Folglich sieht die Formel dafür wie
    ** folgt aus (is ja ne e-Funktion):
    **                                    -2.8
    ** wirkung = ycd->blast * e hoch  ( ----------- * distance )
    **                                  SECTOR_SIZE
    ** Der Radius geht bis zu der Grenze, wo noch 10000 Energie wirken.
    ** Das Halbieren geht auch mittels LeftSchift, aber für die Bakterien
    ** brauchen wir sowieso die Formel, also evtl. Sektorenergie optimieren
    */
    if( ycd->blast < 10000 )
        radius = 1; // weniger als SS / 4
    else
        radius = - SECTOR_SIZE / 2.8 * log( 10000 / ((FLOAT)ycd->blast) );

    /*** Weltkreis ***/
    abstand = SECTOR_SIZE / 4;
    anzahl  = radius / abstand;     //  in eine Richtung!

    for( i = 0; i < anzahl; i++ ) {

        LONG rand = (LONG) nc_sqrt( (FLOAT)(anzahl * anzahl + i * i ) );

        for( j = 0; j <= rand; j++ ) {

            /*** Wirkung auf +/- i und +/- j ***/
            struct energymod_msg emod;
            FLOAT  d;

            d = nc_sqrt( (FLOAT)( i * i + j * j ) ) * SECTOR_SIZE;

            emod.energy = (LONG)( ((FLOAT)ycd->blast) * exp( -2.8 *
                                d / SECTOR_SIZE ) );

            emod.pnt.y  = 0;

            emod.pnt.x  = ycd->bact->pos.x + i * abstand;
            emod.pnt.z  = ycd->bact->pos.z + j * abstand;
            // FIXME_FLOH
            emod.hitman = ycd->bact;
            if( (emod.pnt.x >  SECTOR_SIZE) &&
                (emod.pnt.x <  ycd->bact->WorldX - SECTOR_SIZE) &&
                (emod.pnt.z < -SECTOR_SIZE) &&
                (emod.pnt.z >  ycd->bact->WorldZ + SECTOR_SIZE) )
                _methoda( ycd->bact->BactObject, YBM_MODSECTORENERGY, &emod );

            if( i != 0 ) {

                emod.pnt.x  = ycd->bact->pos.x - i * abstand;
                emod.pnt.z  = ycd->bact->pos.z + j * abstand;
                if( (emod.pnt.x >  SECTOR_SIZE) &&
                    (emod.pnt.x <  ycd->bact->WorldX - SECTOR_SIZE) &&
                    (emod.pnt.z < -SECTOR_SIZE) &&
                    (emod.pnt.z >  ycd->bact->WorldZ + SECTOR_SIZE) )
                    _methoda( ycd->bact->BactObject, YBM_MODSECTORENERGY, &emod );
                }

            if( j != 0 ) {

                emod.pnt.x  = ycd->bact->pos.x + i * abstand;
                emod.pnt.z  = ycd->bact->pos.z - j * abstand;
                if( (emod.pnt.x >  SECTOR_SIZE) &&
                    (emod.pnt.x <  ycd->bact->WorldX - SECTOR_SIZE) &&
                    (emod.pnt.z < -SECTOR_SIZE) &&
                    (emod.pnt.z >  ycd->bact->WorldZ + SECTOR_SIZE) )
                    _methoda( ycd->bact->BactObject, YBM_MODSECTORENERGY, &emod );
                }

            if( ( i != 0 ) && (j != 0 ) ) {

                emod.pnt.x  = ycd->bact->pos.x - i * abstand;
                emod.pnt.z  = ycd->bact->pos.z - j * abstand;
                if( (emod.pnt.x >  SECTOR_SIZE) &&
                    (emod.pnt.x <  ycd->bact->WorldX - SECTOR_SIZE) &&
                    (emod.pnt.z < -SECTOR_SIZE) &&
                    (emod.pnt.z >  ycd->bact->WorldZ + SECTOR_SIZE) )
                    _methoda( ycd->bact->BactObject, YBM_MODSECTORENERGY, &emod );
                }
            }
        }

    /*** Vehiclekreis ***/
    abstand = SECTOR_SIZE;
    anzahl  = radius / abstand;     //  in eine Richtung!

    for( i = 0; i < anzahl; i++ ) {

        LONG rand = (LONG) nc_sqrt( (FLOAT)(anzahl * anzahl + i * i ) );

        for( j = 0; j <= rand; j++ ) {

            if( (ycd->bact->SectX + i > 0) &&
                (ycd->bact->SectX + i < ycd->bact->WSecX - 1) &&
                (ycd->bact->SectY + j > 0) &&
                (ycd->bact->SectY + j < ycd->bact->WSecY - 1) ) {

                /*** Der Sector sollte existieren ***/
                struct Cell *sector;
                struct Bacterium *kandidat;

                sector = &(ycd->bact->Sector[ i + j * ycd->bact->WSecX ]);

                /* --------------------------------------------------------------
                ** BactList scannen und alles killen. Normalerweise Energieabzug,
                ** bei Missiles und Leuten unter 0 DEATH einschalten
                ** ------------------------------------------------------------*/
                kandidat = (struct Bacterium *) sector->BactList.mlh_Head;
                while( kandidat->SectorNode.mln_Succ ) {

                    BOOL really_network = FALSE;

                    /*** Fürs debuggen ***/
                    if( kandidat == ycd->bact ) {

                        kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                        continue;
                        }

                    if( BCLID_YPAMISSY != kandidat->BactClassID ) {

                        LONG  energy;
                        FLOAT distance;

                        distance =
                           nc_sqrt( (ycd->bact->pos.x - kandidat->pos.x) *
                                    (ycd->bact->pos.x - kandidat->pos.x) +
                                    (ycd->bact->pos.y - kandidat->pos.y) *
                                    (ycd->bact->pos.y - kandidat->pos.y) +
                                    (ycd->bact->pos.z - kandidat->pos.z) *
                                    (ycd->bact->pos.z - kandidat->pos.z) );

                        energy = (LONG) ( ((FLOAT)ycd->blast) *
                                          exp( -2.8 * distance / SECTOR_SIZE) );

                        energy = (LONG) ( ((FLOAT)energy) *
                                          (1.0 - ((FLOAT)kandidat->Shield)/100.0) );
                        kandidat->Energy -= energy;

                        #ifdef __NETWORK__
                        /*** Abzug mit evtl. folgendem ACTION_DEAD übers Netz***/
                        ywd = INST_DATA( ((struct nucleusdata *)ycd->world)->o_Class, ycd->world);
                        if( ywd->playing_network ) {

                            struct ypamessage_vehicleenergy re;
                            struct sendmessage_msg sm;

                            really_network = TRUE;

                            re.generic.message_id = YPAM_VEHICLEENERGY;
                            re.generic.timestamp  = ywd->TimeStamp;
                            re.generic.owner      = kandidat->Owner;
                            re.ident              = kandidat->ident;
                            re.energy             = -energy;

                            sm.receiver_id         = NULL;
                            sm.receiver_kind       = MSG_ALL;
                            sm.sender_id           = ywd->gsr->NPlayerName;
                            sm.sender_kind         = MSG_PLAYER;
                            sm.data                = &re;
                            sm.data_size           = sizeof( re );
                            sm.guaranteed          = TRUE;
                            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                            }
                        #endif
                        }

                    /*** Explodieren? ***/
                    if( (kandidat->Energy <= 0 ) ||
                        (BCLID_YPAMISSY == kandidat->BactClassID) ) {

                        /* -----------------------------------------------
                        ** Mit dem Energieabzug ist bereits ein SETSTATE
                        ** für die Schatten verbunden. Wir machen also
                        ** ein SETSTATE_I für die vehicle und ein SETSTATE
                        ** für die Waffen, bei denen Energyabzug keinen
                        ** Sinn hat.
                        ** ---------------------------------------------*/

                        state.main_state = ACTION_DEAD;
                        state.extra_off  = state.extra_on = 0;

                        if( BCLID_YPAMISSY == kandidat->BactClassID )
                            _methoda( kandidat->BactObject, YBM_SETSTATE, &state );
                        else
                            _methoda( kandidat->BactObject, YBM_SETSTATE_I, &state );

                        /*** Sonderbehandlung Robo ***/
                        if( (BCLID_YPAROBO == kandidat->BactClassID) &&
                            (FALSE         == really_network) ) {

                            struct notifydeadrobo_msg ndr;

                            /*** Meldung ans WO ***/
                            ndr.victim    = kandidat;
                            ndr.killer_id = ycd->bact->Owner;
                            _methoda( ycd->world, YWM_NOTIFYDEADROBO, &ndr);
                            }
                        }

                    kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                    }
                }
            }
        }

    /*** Wegfliegen ***/

    /* -----------------------------------------------------------------
    ** Weil wir keine Winkelgeschwindigkeit haben, müssen wir das Objekt
    ** ausrichten auf das Ziel ausrichten. Die Drehung in CRASHBACTERIUM
    ** ist zufallsabhängig und sollte ausgeschalten werden.
    ** ---------------------------------------------------------------*/

    /*** lokal z nach unten biegen und renormalisieren ***/
    ycd->bact->dir.m32 = 1.9;
    betrag = nc_sqrt( ycd->bact->dir.m31 * ycd->bact->dir.m31 +
                      ycd->bact->dir.m32 * ycd->bact->dir.m32 +
                      ycd->bact->dir.m33 * ycd->bact->dir.m33 );
    ycd->bact->dir.m31 /= betrag;
    ycd->bact->dir.m32 /= betrag;
    ycd->bact->dir.m33 /= betrag;

    /*** lokal x ist mit y = 0 waagerecht und senkrecht auf z ***/
    if( fabs( ycd->bact->dir.m33) > 0.1 ) {

        ycd->bact->dir.m11 = nc_sqrt( 1 /
            ( 1 +  ycd->bact->dir.m31 * ycd->bact->dir.m31 /
                  (ycd->bact->dir.m33 * ycd->bact->dir.m33) ) );
        ycd->bact->dir.m13 = -ycd->bact->dir.m31 * ycd->bact->dir.m11 /
                              ycd->bact->dir.m33;
        }
    else {

        ycd->bact->dir.m13 = nc_sqrt( 1 /
            ( 1 +  ycd->bact->dir.m33 * ycd->bact->dir.m33 /
                  (ycd->bact->dir.m31 * ycd->bact->dir.m31) ) );
        ycd->bact->dir.m11 = -ycd->bact->dir.m33 * ycd->bact->dir.m13 /
                              ycd->bact->dir.m31;
        }
    ycd->bact->dir.m12 = 0.0;

    /* ----------------------------------------------------------------------
    ** sqrt schluckt das vorzeichen. Die y- komponente von (lx x lz) muß nach
    ** oben zeigen, also kleiner 0 sein, sonst stehen wir Kopf
    ** --------------------------------------------------------------------*/
    if( ycd->bact->dir.m13 * ycd->bact->dir.m31 -
        ycd->bact->dir.m11 * ycd->bact->dir.m33 > 0.0 ) {

        ycd->bact->dir.m11 = -ycd->bact->dir.m11;
        ycd->bact->dir.m13 = -ycd->bact->dir.m13;
        }

    /*** y ergibt sich aus -( lx x lz) ***/
    ycd->bact->dir.m21 = ycd->bact->dir.m32 * ycd->bact->dir.m13 -
                         ycd->bact->dir.m33 * ycd->bact->dir.m12;
    ycd->bact->dir.m22 = ycd->bact->dir.m33 * ycd->bact->dir.m11 -
                         ycd->bact->dir.m31 * ycd->bact->dir.m13;
    ycd->bact->dir.m23 = ycd->bact->dir.m31 * ycd->bact->dir.m12 -
                         ycd->bact->dir.m32 * ycd->bact->dir.m11;

    /*** Wegschleudern ***/
    ycd->bact->dof.x = -ycd->bact->dir.m31;
    ycd->bact->dof.y = -ycd->bact->dir.m32;
    ycd->bact->dof.z = -ycd->bact->dir.m33;
    ycd->bact->dof.v = 200;

    ycd->bact->ExtraState &= ~EXTRA_LANDED;

    /* -----------------------------------------------------------------------
    ** Kille kille. Weil mittels eines hacks alle Bodenfahrzeuge,
    ** die auf DEAD gesetzt werden sollen, sofort auf Megadeth gesetzt werden,
    ** hier aber DEAD explizit verlangt wird, helfe ich mir mit einem
    ** absolut dreckigen Hack: Ich verändere die BactClassID!
    ** ---------------------------------------------------------------------*/
    //ycd->bact->MainState = ACTION_DEAD;

    ycd->bact->BactClassID = BCLID_YPAFLYER;
    state.main_state = ACTION_DEAD;
    state.extra_off  = state.extra_on = 0;
    _methoda( ycd->bact->BactObject, YBM_SETSTATE, &state);
    ycd->bact->BactClassID = BCLID_YPACAR;

    ycd->bact->Energy    = -10;
}
