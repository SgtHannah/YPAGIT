/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_user.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: floh $
**  $Author: floh $
**
**  Steuerung des Objects durch User.
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
#include "input/input.h"
#include "audio/audioengine.h"


#define MAX_NEIGUNG     (1.0)


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

void yb_DrehObjektInFlugrichtung( struct ypabact_data *ybd, ULONG ftime );
BOOL yb_BactCollision(struct ypabact_data *ybd, FLOAT time );
void yb_MachRaketeFertig( struct ypabact_data *ybd, UBYTE Type );
void yb_rot_round_lokal_y(struct Bacterium *bact, FLOAT angle );
void yb_rot_round_lokal_z(struct Bacterium *bact, FLOAT angle );
void yb_rot_round_lokal_x(struct Bacterium *bact, FLOAT angle );
void yb_rot_round_global_y(struct Bacterium *bact, FLOAT angle );
void yb_DrawVisier( struct ypabact_data *ybd, struct Bacterium *enemy, BOOL bomb );


/*-----------------------------------------------------------------*/
void q_mul_q(struct quat *q1, struct quat *q2, struct quat *q)
/*
**  FUNCTION
**      Quaternation-Multiplikation
**
**      q = q1 * q2
**
**      q1 und q2 sind Unit-Quaternations, das resultierende
**      q ist ebenfalls eine Unit-Quaternation.
**
**      ((q != q1) && (q != q2)) !!!
**
**  CHANGED
**      10-Jun-95   floh    created (nach einer Menge Stress)
*/
{
    q->x = (q1->y*q2->z - q1->z*q2->y) + q1->w*q2->x + q1->x*q2->w;
    q->y = (q1->z*q2->x - q1->x*q2->z) + q1->w*q2->y + q1->y*q2->w;
    q->z = (q1->x*q2->y - q1->y*q2->x) + q1->w*q2->z + q1->z*q2->w;
    q->w = q1->w*q2->w - (q1->x*q2->x + q1->y*q2->y + q1->z*q2->z);
}

/*-----------------------------------------------------------------*/
void q_to_m(struct quat *q, struct flt_m3x3 *m)
/*
**  FUNCTION
**      Wandelt Quaternation in Rotations-Matrix um.
**
**  CHANGED
**      10-Jun-95   floh    created
*/
{
    m->m11 = 1-2*(q->y * q->y  +  q->z * q->z);
    m->m12 =   2*(q->x * q->y  -  q->w * q->z);
    m->m13 =   2*(q->x * q->z  +  q->w * q->y);

    m->m21 =   2*(q->x * q->y  +  q->w * q->z);
    m->m22 = 1-2*(q->x * q->x  +  q->z * q->z);
    m->m23 =   2*(q->y * q->z  -  q->w * q->x);

    m->m31 =   2*(q->x * q->z  -  q->w * q->y);
    m->m32 =   2*(q->y * q->z  +  q->w * q->x);
    m->m33 = 1-2*(q->x * q->x  +  q->y * q->y);
}

/*-----------------------------------------------------------------*/
void yb_rotxy(FLOAT x_rad, FLOAT y_rad, struct flt_m3x3 *m)
/*
**  FUNCTION
**      Rotiert Matrix m um die angegebenen Winkel (in rad) um die
**      X- und Y-Achse (Y-Achse des Global-Systems, X-Achse des
**      Lokal-Systems).
**
**  CHANGED
**      12-Jun-95   floh    created
*/
{
    struct flt_m3x3 rm;
    struct flt_m3x3 tm;

    /*** die Rotationen sind nicht gerade optimiert... ***/
    FLOAT sin_x = sin(x_rad);
    FLOAT cos_x = cos(x_rad);
    FLOAT sin_y = sin(y_rad);
    FLOAT cos_y = cos(y_rad);

    /*** Rotation um globales Y ***/
    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    nc_m_mul_m(m, &rm, &tm);

    /*** Rotation um lokales X ***/
    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_x;     rm.m23 = -sin_x;
    rm.m31 = 0.0;       rm.m32 = sin_x;     rm.m33 = cos_x;
    nc_m_mul_m(&rm, &tm, m);
}

/*-----------------------------------------------------------------*/
void yb_rotxz(FLOAT x_rad, FLOAT z_rad, struct flt_m3x3 *m)
/*
**  FUNCTION
**      Rotiert Matrix m um die angegebenen Winkel (in rad) um die
**      lokalen X- und Z-Achsen
**
**  CHANGED
**      12-Jun-95   floh    created
*/
{
    struct flt_m3x3 rm;
    struct flt_m3x3 tm;

    /*** die Rotationen sind nicht gerade optimiert... ***/
    FLOAT sin_x = sin(x_rad);
    FLOAT cos_x = cos(x_rad);
    FLOAT sin_z = sin(z_rad);
    FLOAT cos_z = cos(z_rad);

    /*** Rotation um lokales z ***/
    rm.m11 = cos_z;     rm.m12 = sin_z;     rm.m13 = 0.0;
    rm.m21 = -sin_z;    rm.m22 = cos_z;     rm.m23 = 0.0;
    rm.m31 = 0.0;       rm.m32 = 0.0;       rm.m33 = 1.0;
    nc_m_mul_m(&rm, m, &tm);

    /*** Rotation um lokales X ***/
    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_x;     rm.m23 = -sin_x;
    rm.m31 = 0.0;       rm.m32 = sin_x;     rm.m33 = cos_x;
    nc_m_mul_m(&rm, &tm, m);
}





/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
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
    FLOAT  x_rad, z_rad, y_rad, fangle, seite;
    struct move_msg move;
    struct ypabact_data *ybd = INST_DATA(cl,o);
    struct insphere_msg ins;
    struct flt_triple vec;
    FLOAT  betrag, A,B,C, afactor, norm_height;
    struct Insect insect[10];
    FLOAT  time, angle, nachvorn, rueck, bsp, merke_force;
    ULONG  BACTCOLL, maxcollisionchecks;
    struct firemissile_msg fire;
    struct setstate_msg state;
    struct crash_msg crash;
    struct bactcollision_msg bcoll;
    struct recoil_msg rec;
    struct fireyourguns_msg fyg;
    struct visier_msg visier;
    struct intersect_msg inter;
    BOOL   coll;

    /*** Vorbetrachtungen ***/
    ybd->bact.air_const = ybd->bact.bu_air_const;
    time = (FLOAT) msg->frame_time / 1000;
    BACTCOLL = (ybd->flags & YBF_BactCollision);

    switch( ybd->bact.MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:

            /*
            ** SliderX: Richtungswechsel
            ** SliderY: Neigungswinkel, also Speed
            ** LMB: mehr Schub
            ** RMB  weniger Schub
            */
            
            /*** alte Pos. merken ***/
            ybd->bact.old_pos = ybd->bact.pos;

            /*** direkte Kollision ***/
            if( BACTCOLL && (!(ybd->bact.ExtraState & EXTRA_LANDED)) ) {

                bcoll.frame_time = msg->frame_time;
                _methoda( o, YBM_BACTCOLLISION, &bcoll );
                }

            /*** Zustandsumschaltung ***/
            if( ybd->bact.ExtraState & EXTRA_LANDED )
                bsp = IS_SPEED;
            else
                bsp = 10 * IS_SPEED;

            if( fabs(ybd->bact.dof.v) < bsp ) {    // keine Vielfachen! IS_SPEED eher hochsetzen!

                /*** Optisch wait nur, wenn wir gelandet sind ***/
                inter.pnt.x = ybd->bact.pos.x;
                inter.pnt.y = ybd->bact.pos.y;
                inter.pnt.z = ybd->bact.pos.z;
                inter.vec.x = inter.vec.z = 0.0;
                inter.vec.y = 1.5 * max( ybd->bact.viewer_over_eof,
                                         ybd->bact.viewer_radius);
                inter.flags = 0;
                _methoda( ybd->world, YWM_INTERSECT, &inter );
                if( inter.insect &&
                    (ybd->bact.act_force <= ybd->bact.mass * GRAVITY) ) {

                    /*** Aufsetzen ***/
                    ybd->bact.ExtraState |=  EXTRA_LANDED;
                    ybd->bact.dof.v   = 0.0;
                    ybd->bact.pos.y       = inter.ipnt.y - ybd->bact.viewer_over_eof;
                    ybd->bact.act_force   = ybd->bact.mass * GRAVITY;
                    }
                else 
                    ybd->bact.ExtraState &= ~EXTRA_LANDED;

                /*** Wenn wir in Zielnähe sind, warten wir nur "scheinbar" ! ***/
                if( (ybd->bact.PrimTargetType == TARTYPE_SECTOR) &&
                    (nc_sqrt( (ybd->bact.PrimPos.x - ybd->bact.pos.x ) * 
                              (ybd->bact.PrimPos.x - ybd->bact.pos.x ) +
                              (ybd->bact.PrimPos.z - ybd->bact.pos.z ) * 
                              (ybd->bact.PrimPos.z - ybd->bact.pos.z ) ) >
                             (2 * FOLLOW_DISTANCE)) ) {
                    
                    /*** weit weg ***/
                    ybd->bact.MainState = ACTION_WAIT;

                    if( (ybd->bact.ExtraState & EXTRA_LANDED) &&
                        (!(ybd->bact.ExtraState & EXTRA_FIRE)) ) {
                        
                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                else {

                    /*** Am Kampfort ***/
                    if( (ybd->bact.ExtraState & EXTRA_LANDED) &&
                        (!(ybd->bact.ExtraState & EXTRA_FIRE )) ) {

                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    ybd->bact.MainState = ACTION_NORMAL;    // !!
                    }
                }
            else {

                if( !(ybd->bact.ExtraState & EXTRA_FIRE) ) {

                    state.main_state = ACTION_NORMAL;
                    state.extra_on = state.extra_off = 0;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                /*** Landed ausschalten ***/
                ybd->bact.ExtraState &= ~EXTRA_LANDED;
                }


            /* -----------------------------------------------------------------
            ** Drehung. Wir lenken nur, wenn die Tasten gedrückt sind. Deshalb
            ** ermitteln wir zuerst den derzeitigen Neigungswinkel. Aus z_rad
            ** ermitteln wir einen gewünschten Neigungswinkel. Aus der Differenz
            ** ermitteln wir den Winkel, um den wir drehen müssen. Dieser muß,
            ** sofern er zu groß ist, noch reduziert werden. 
            ** ---------------------------------------------------------------*/

            /*** typenspezifische Begrenzung Begrenzungen ***/
            x_rad =  msg->input->Slider[ SL_FLY_HEIGHT ] *
                     ybd->bact.max_rot * time;
            y_rad = -msg->input->Slider[ SL_FLY_DIR ] *
                     ybd->bact.max_rot * time;
            z_rad = -y_rad;

            /* ---------------------------------------------------------------
            ** Ist es vielleicht nur eine drehung um y? Diese darf nur sein,
            ** wenn ich gerade stehe oder senkrecht fliege. Wenn ich fast
            ** senkrecht stehe, aber vorwärts fliege, muß ich mich ausrichten.
            ** Entscheidend ist also nur dof.y (0.996 == 5°)
            ** -------------------------------------------------------------*/
            if( ((fabs(ybd->bact.dof.y) > 0.98) ||
                 (ybd->bact.dof.v == 0.0)) &&
                 (ybd->bact.dir.m22 > 0.996) ) {

                FLOAT len;

                /* -------------------------------------------------
                ** Und senkrecht stellen, dass es beim Wegfliegen
                ** keine komischen Ruckler gibt, weil die Kraft doch
                ** in eine etwas andere Richtung zeigt
                ** -----------------------------------------------*/
                ybd->bact.dir.m21 = 0.0;
                ybd->bact.dir.m22 = 1.0;
                ybd->bact.dir.m23 = 0.0;
                ybd->bact.dir.m12 = 0.0;
                ybd->bact.dir.m32 = 0.0;
                len = nc_sqrt( ybd->bact.dir.m11 * ybd->bact.dir.m11 +
                               ybd->bact.dir.m13 * ybd->bact.dir.m13 );
                ybd->bact.dir.m13 /= len;
                ybd->bact.dir.m11 /= len;
                len = nc_sqrt( ybd->bact.dir.m31 * ybd->bact.dir.m31 +
                               ybd->bact.dir.m33 * ybd->bact.dir.m33 );
                ybd->bact.dir.m33 /= len;
                ybd->bact.dir.m31 /= len;
                }
            
            /*** jetzige Seitenneigung (W. zw. x-z-Ebene und lok. z-Achse) ***/
            nachvorn = nc_acos( nc_sqrt(ybd->bact.dir.m31 * ybd->bact.dir.m31 +
                                        ybd->bact.dir.m33 * ybd->bact.dir.m33 ) /
                                nc_sqrt(ybd->bact.dir.m31 * ybd->bact.dir.m31 +
                                        ybd->bact.dir.m32 * ybd->bact.dir.m32 +
                                        ybd->bact.dir.m33 * ybd->bact.dir.m33 ) );

            /*** jetzige Seitenneigung (W. zw. x-z-Ebene und lok. x-Achse) ***/
            angle = nc_acos( nc_sqrt(ybd->bact.dir.m11 * ybd->bact.dir.m11 +
                                     ybd->bact.dir.m13 * ybd->bact.dir.m13 ) /
                             nc_sqrt(ybd->bact.dir.m11 * ybd->bact.dir.m11 +
                                     ybd->bact.dir.m12 * ybd->bact.dir.m12 +
                                     ybd->bact.dir.m13 * ybd->bact.dir.m13 ) );

            /*** Merken zum Ausbremsen der Ausrichtung ***/
            seite = fabs(angle);

            if( ybd->bact.dir.m12 < 0.0 ) angle = -angle;  /* y-Komp. der x-Achse entsch.
                                                              über Vorzeichen */

            /*** der acos ist sehr ungenau und kann Objekte "aus der Ruhe" ***/
            /*** bringen.                                                  ***/
            if( fabs( angle ) < 1e-02 ) angle = 0.0;

            /*** afactor resultiert aus der jetzigen Seiteneigung ***/
            afactor = ybd->bact.HC_Speed/4 +
                      ybd->bact.HC_Speed * fabs( angle );

            /* ------------------------------------------------------------------
            ** gewünschte Seitenneigung. Das ist die jetzige Seitenneigung
            ** + die aus z_rad resultierende Differenz. Evtl. wird die Obergrenze
            ** der Seitenneigung beschränkt.
            ** Ist z_rad == 0, so neigen wir langsam wieder zurück (max_rot/2, da
            ** sonst Rückneigung genauso schnell ist wie mit kraft )
            ** ----------------------------------------------------------------*/
            rueck = ybd->bact.max_rot * time * afactor;
            if( fabs( angle ) < rueck ) rueck = fabs( angle );
            if( angle > 0 ) rueck = -rueck;

            z_rad += rueck;

            if( fabs( z_rad + angle ) > MAX_NEIGUNG ) {

                if( z_rad < 0.0 ) z_rad = fabs( angle ) - MAX_NEIGUNG;
                    else          z_rad = MAX_NEIGUNG - fabs( angle );
                }

            /*** ... und kann nur...zum Test afactor raus  ***/
            if( fabs( z_rad ) > 2.0 * ybd->bact.max_rot * time * afactor ) {

                if( z_rad <  0.0 ) z_rad = -2.0 * ybd->bact.max_rot*time*afactor;
                if( z_rad >= 0.0 ) z_rad =  2.0 * ybd->bact.max_rot*time*afactor;
                }

            /*** Für kleine z_rad nichts machen, sonst permanentes Drehen ***/
            if( fabs( z_rad ) < 0.001 ) z_rad = 0;


            /* ----------------------------------------------------------------
            ** x_rad rotiert das FO um die lokale x-Achse, z_rad um die lokale
            ** z-Achse. Nach dem Aufruf der Bewegung wird es dann ausgerichtet.
            ** (DrehObjektInFlugrichtung())
            ** evtl. später Winkelbegrenzung.
            ** --------------------------------------------------------------*/
            yb_rot_round_lokal_x(  &(ybd->bact), 0.5 * x_rad );
            yb_rot_round_lokal_z(  &(ybd->bact), 0.5 * z_rad );
            yb_rot_round_global_y( &(ybd->bact), 0.5 * y_rad );

            ybd->bact.act_force += ybd->bact.max_force * time * 0.5 *
                                   msg->input->Slider[ SL_FLY_SPEED ];
            
            /*** Begrenzung der Werte ***/
            if( ybd->bact.act_force < 0 ) ybd->bact.act_force = 0;

            if( ybd->bact.act_force > ybd->bact.max_force )
                ybd->bact.act_force = ybd->bact.max_force;

            /* --------------------------------------------------
            ** Da die Koreekturen doch zum Teil massiv reinhauen,
            ** werde ich die Kraft backup-en
            ** ------------------------------------------------*/
            merke_force = ybd->bact.act_force;

            /*** Maximale Flughöhe berücksichtigen! ***/
            norm_height = ybd->bact.Sector->Height - ybd->bact.pos.y;
            if( norm_height > 0.8 * ybd->bact.max_user_height ) {

                /* --------------------------------------------------------
                ** Ab dieser Höhe Kraft nachkorrigieren. Um sich die
                ** Formel wieder zusammenzubasteln, zeichne man ein
                ** Kraft-Höhen-Diagramm und erstelle eine Geradengleichung.
                ** ------------------------------------------------------*/
                if( ybd->bact.act_force >
                    (norm_height - 0.8 * ybd->bact.max_user_height ) *
                    (ybd->bact.mass * GRAVITY - ybd->bact.max_force) /
                    (0.2 * ybd->bact.max_user_height) )

                    ybd->bact.act_force =
                    (norm_height - 0.8 * ybd->bact.max_user_height ) *
                    (ybd->bact.mass * GRAVITY - ybd->bact.max_force) /
                    (0.2 * ybd->bact.max_user_height);

                /* ---------------------------------------------
                ** Der kann trotzdem zu hoch fliegen, dann wirkt
                ** die Kurve zu stark kraftreduzierend
                ** -------------------------------------------*/
                if( ybd->bact.act_force < 0 ) ybd->bact.act_force = 0;
                }


            /*** Zielaufnahme, die machen wir immer (vorher default) ***/
            fire.target_type  = TARTYPE_SIMPLE;
            fire.target_pos.x = ybd->bact.dir.m31;
            fire.target_pos.y = ybd->bact.dir.m32;
            fire.target_pos.z = ybd->bact.dir.m33;

            visier.flags = VISIER_ENEMY | VISIER_NEUTRAL;
            visier.dir.x = ybd->bact.dir.m31;
            visier.dir.y = ybd->bact.dir.m32;
            visier.dir.z = ybd->bact.dir.m33;
            if( _methoda( o, YBM_VISIER, &visier ) ) {

                fire.target_type = TARTYPE_BACTERIUM;
                fire.target.bact = visier.enemy;
                }


            /*** Schießen - Rakete ***/
            if( (msg->input->Buttons & BT_FIRE ) ||
                (msg->input->Buttons & BT_FIREVIEW ) ) {

                /*** Ausfüllen der Fire-Message ***/
                fire.dir.x       = fire.dir.y = fire.dir.z = 0.0;
                fire.wtype       = ybd->bact.auto_ID;
                fire.global_time = ybd->bact.internal_time;
                
                /* Hubschrauber. etwas runter + Seite */
                if( ybd->bact.internal_time % 2 == 0 )
                    fire.start.x   = -ybd->bact.firepos.x;
                else
                    fire.start.x   = ybd->bact.firepos.x;
                fire.start.y       = ybd->bact.firepos.y;
                fire.start.z       = ybd->bact.firepos.z;
                
                /* Viewer oder nicht? */
                if( msg->input->Buttons & BT_FIREVIEW )
                    fire.flags = FIRE_VIEW;
                else
                    fire.flags = 0;
                fire.flags |= FIRE_CORRECT;
                _methoda( o, YBM_FIREMISSILE, &fire );
                }                                     

            /*** MG vorhanden? dann ist der MG_Type 0xFF ***/
            if( (UBYTE)ybd->bact.mg_shot != NO_MACHINEGUN ) {

                /*** Schießen, Kanone ***/
                if( (ybd->bact.ExtraState & EXTRA_FIRE) &&
                    (!(msg->input->Buttons & BT_FIREGUN)) ) {

                    /*** Feuern aus ***/
                    // Abmelden beim WO

                    state.main_state = state.extra_on = 0;
                    state.extra_off  = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                if( msg->input->Buttons & BT_FIREGUN ) {

                    /*** Feuern einschalten? ***/
                    if( !(ybd->bact.ExtraState & EXTRA_FIRE) ) {

                        // Anmelden WO

                        state.main_state = state.extra_off = 0;
                        state.extra_on   = EXTRA_FIRE;
                        _methoda( o, YBM_SETSTATE, &state );
                        }

                    /*** Feuern an sich ***/
                    fyg.dir.x = ybd->bact.dir.m31;
                    fyg.dir.y = ybd->bact.dir.m32;
                    fyg.dir.z = ybd->bact.dir.m33;
                    fyg.time  = time;
                    fyg.global_time = ybd->bact.internal_time;
                    _methoda( o, YBM_FIREYOURGUNS, &fyg );
                    }
                }

            /* ------------------------------------------------------
            ** Alle folgenden Bewegungs- und Kollisionssachen werden
            ** nur gemacht, wenn wir nicht gerade gelandet sind. So
            ** prallen wir nicht permanent ab und sparen uns ne Menge
            ** Arbeit
            ** ----------------------------------------------------*/

            /*** Ausrichten ***/
            if( msg->input->Buttons & BT_STOP ) {
                _methoda( o, YBM_STOPMACHINE, msg );
                merke_force = ybd->bact.act_force;
                }

            if( !(ybd->bact.ExtraState & EXTRA_LANDED) ) {

                
                maxcollisionchecks = 4;
                while( maxcollisionchecks-- ) {

                    /*** Zuerst der Versuch einer bewegung ***/
                    move.t = (FLOAT) msg->frame_time/1000;
                    move.flags = 0;
                    _methoda(ybd->bact.BactObject, YBM_MOVE, &move);

                    coll = FALSE;

                    /*** jetzt Kollisionstest ***/
                    ins.pnt.x       = ybd->bact.pos.x;
                    ins.pnt.y       = ybd->bact.pos.y;
                    ins.pnt.z       = ybd->bact.pos.z;
                    ins.dof.x       = ybd->bact.dof.x;
                    ins.dof.y       = ybd->bact.dof.y;
                    ins.dof.z       = ybd->bact.dof.z;
                    ins.radius      = 32;
                    ins.chain       = insect;
                    ins.max_insects = 10;
                    ins.flags       = 0;

                    _methoda( ybd->world, YWM_INSPHERE, &ins );

                    if( ins.num_insects > 0 ) {

                        /*
                        ** Es trat eine Kollision auf! Wir summieren alle Normalen-
                        ** vektoren der Ebenen auf und prallen von dort ab. Dann 
                        ** gehen wir mit TRUE zurück.
                        */

                        A = 0;
                        B = 0;
                        C = 0;

                        while( ins.num_insects-- ) {

                            A += ins.chain[ ins.num_insects ].pln.A;
                            B += ins.chain[ ins.num_insects ].pln.B;
                            C += ins.chain[ ins.num_insects ].pln.C;
                            }

                        betrag = nc_sqrt( A*A + B*B + C*C );
                        
                        if( betrag == 0) {

                            /*
                            ** Was komischerweise scheinbar vorkommt...
                            */
                            vec.x = ybd->bact.dof.x;
                            vec.y = ybd->bact.dof.y;
                            vec.z = ybd->bact.dof.z;
                            }
                        else {

                            vec.x = A / betrag;
                            vec.y = B / betrag;
                            vec.z = C / betrag;
                            }

                        rec.vec.x = vec.x;      rec.vec.y = vec.y;
                        rec.vec.z = vec.z;      rec.mul_y = 2.0;
                        rec.mul_v = 0.7;        rec.time = time;
                        _methoda( o, YBM_RECOIL, &rec);
                        
                        coll = TRUE;
                        }

                    /*** Intersection ***/
                    if( !coll ) {

                        inter.pnt.x = ybd->bact.old_pos.x;
                        inter.pnt.y = ybd->bact.old_pos.y;
                        inter.pnt.z = ybd->bact.old_pos.z;
                        inter.vec.x = ybd->bact.pos.x - ybd->bact.old_pos.x;
                        inter.vec.y = ybd->bact.pos.y - ybd->bact.old_pos.y;
                        inter.vec.z = ybd->bact.pos.z - ybd->bact.old_pos.z;
                        inter.flags = 0;
                        _methoda( ybd->world, YWM_INTERSECT, &inter );
                        if( inter.insect ) {

                            /*** Wir brauchen nur abzuprallen, weil v groß ist ***/
                            rec.vec.x = inter.sklt->PlanePool[ inter.pnum ].A;
                            rec.vec.y = inter.sklt->PlanePool[ inter.pnum ].B;
                            rec.vec.z = inter.sklt->PlanePool[ inter.pnum ].C;
                            rec.mul_v = 0.7;   rec.mul_y = 2.0;   rec.time = time;
                            _methoda( o, YBM_RECOIL, &rec );

                            coll = TRUE;
                            }
                        }
                    
                    if( !coll ) {

                        ybd->bact.ExtraState &= ~EXTRA_LANDCRASH;
                        break;
                        }
                    else {

                        /*** Knallgeräusch ***/
                        if( (!(ybd->bact.sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE)) &&
                            (!(ybd->bact.ExtraState & EXTRA_LANDCRASH)) ) {

                            struct yw_forcefeedback_msg ffb;

                            ybd->bact.ExtraState |= EXTRA_LANDCRASH;
                            _StartSoundSource( &(ybd->bact.sc), VP_NOISE_CRASHLAND );

                            ffb.type    = YW_FORCE_COLLISSION;
                            ffb.power   = 1.0;

                            /*** Trick: Pos + Richtung ***/
                            ffb.dir_x   = ybd->bact.pos.x + 10 * A;
                            ffb.dir_y   = ybd->bact.pos.z + 10 * C;
                            _methoda( ybd->world, YWM_FORCEFEEDBACK, &ffb );
                            }
                        }
                    }
                }
            else {

                /* -----------------------------------------------------
                ** Wir sind gelanded, trotzdem müssen wir Move aufrufen,
                ** um wieder wegzukommen
                ** ---------------------------------------------------*/
                move.t = (FLOAT) msg->frame_time/1000;
                move.flags = 0;
                _methoda(ybd->bact.BactObject, YBM_MOVE, &move);
                }

            /*** Kraft rücksetzen ***/
            ybd->bact.act_force = merke_force;

            break;

        case ACTION_DEAD:
            
            /* ---------------------------------------------------------------
            ** hier fallen wir, bis wir aufschlagen. Evtl. ist dabei der
            ** Burning-Zustand eingestellt worden. Vorerst fallen wir, bis wir
            ** unten sind. Dann später kann nach der BurningTime der Visproto
            ** weggeschalten werden.
            ** -------------------------------------------------------------*/
            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;
            }

}




_dispatcher( BOOL, yb_YBM_VISIER, struct visier_msg *visier )
{
/*
**  FUNCTION    Sucht das nächstliegende Objekt in einem Radius
**              Flags geben an, was gesucht werden soll.
**
**  INPUT       Flags
**
**  RESULT      TRUE, wenn enemy korrekten Bakterienpointer enthält.
**
**  CHANGED     10-Feb-96   8100000C
**
**  WIRD AUF NUTZUNG DURCH DEN USER KONZIPIERT; WEIL ES SONST KEINEN SINN HAT !!!
*/

    #define VISIER_LEN      2000
    #define VISIER_WIDTH    1000

    struct ypabact_data *ybd;
    struct getsectorinfo_msg gsi;
    struct flt_triple fireend, firestart, vp, ziel;
    struct Cell *cell_array[3];
    int    i;
    struct Bacterium *kandidat, *found_kandidat;
    FLOAT  found_entfernung, entfernung_ok, produkt, found_produkt, dmax, weapon_radius;
    struct WeaponProto *WPArray, *weapon;
    BOOL   bomb;

    ybd = INST_DATA( cl, o);
    found_kandidat = NULL;

    _get( ybd->world, YWA_WeaponArray, &WPArray );
    if( NO_AUTOWEAPON != ybd->bact.auto_ID ) {

        weapon = &( WPArray[ ybd->bact.auto_ID ]);
        weapon_radius = weapon->Radius;
        }
    else {

        weapon_radius = 0.0;
        }

    /* -------------------------------------------------------------------
    ** Durchsuchen, wenn autonome waffe, aber keine Bombe
    ** -----------------------------------------------------------------*/
    if( (ybd->bact.auto_info & WPF_Driven) ||
        (ybd->bact.auto_info & WPF_Impulse) )
        bomb = FALSE;
    else
        bomb = TRUE;

    if( (NO_AUTOWEAPON != ybd->bact.auto_ID) &&
        (bomb          == FALSE) ) {

        firestart = ybd->bact.pos;
        fireend.x = ybd->bact.pos.x + ybd->bact.dir.m31 * SECTOR_SIZE;
        fireend.y = ybd->bact.pos.y + ybd->bact.dir.m32 * SECTOR_SIZE;
        fireend.z = ybd->bact.pos.z + ybd->bact.dir.m33 * SECTOR_SIZE;

        gsi.abspos_x = firestart.x;
        gsi.abspos_z = firestart.z;
        _methoda( ybd->world, YWM_GETSECTORINFO, &gsi);
        cell_array[0] = gsi.sector;

        gsi.abspos_x = fireend.x;
        gsi.abspos_z = fireend.z;
        _methoda( ybd->world, YWM_GETSECTORINFO, &gsi);
        cell_array[2] = gsi.sector;

        if( cell_array[0] == cell_array[2] ) {

            cell_array[1] = cell_array[0];
            }
        else {

            gsi.abspos_x = firestart.x + (fireend.x - firestart.x) / 2;
            gsi.abspos_z = firestart.z + (fireend.z - firestart.z) / 2;
            _methoda( ybd->world, YWM_GETSECTORINFO, &gsi);
            cell_array[1] = gsi.sector;
            }
        

        /*** Ok, 3 potentielle Sektoren sind da. testen wir deren Bakterien ***/
        for( i=0; i<3; i++) {

            if( i > 0 ) {

                /*** testen, ob wir den Sektor schon mal hatten ***/
                if( cell_array[ i-1 ] == cell_array[ i ] )
                    continue;
                }

            /*** Falls der Sektor außerhalb liegt... ***/
            if( cell_array[ i ] == NULL ) continue;

            kandidat = (struct Bacterium *) cell_array[ i ]->BactList.mlh_Head;
            while( ((struct Node *)kandidat)->ln_Succ != 0 ) {

                /* ---------------------------------------------------------
                ** sind wir das selbst bzw. ist es unser Auftraggeber ? oder
                ** eine andere Rakete? Oder ein richtiger Toter, kein TEST-
                ** DESTROY mehr....
                ** -------------------------------------------------------*/
                
                if( ( kandidat       == &(ybd->bact) ) ||
                    ( BCLID_YPAMISSY == kandidat->BactClassID ) ||
                    ( ACTION_DEAD    == kandidat->MainState ) ) {

                    /*** gleich weiter ***/
                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }

                /* -------------------------------------------------------
                ** Roboflaks nehmen wir auch raus, denn wir wollen
                ** ja den Robo und nicht die Flak, ausserdem braucht
                ** das HUD die Robodaten und nicht die der UNZERSTOERBAREN
                ** Flaks
                ** -----------------------------------------------------*/
                if( BCLID_YPAGUN == kandidat->BactClassID ) {

                    ULONG rgun;
                    _get( kandidat->BactObject, YGA_RoboGun, &rgun );
                    if( rgun ) {

                        kandidat = (struct Bacterium *)
                                   ((struct Node *)kandidat)->ln_Succ;
                        continue;
                        }
                    }

                /*** Nun testen, ob das Visier die Sache überhaupt aufnehmen soll ***/
                if( ((!( visier->flags & VISIER_FRIEND )) &&
                      (kandidat->Owner == ybd->bact.Owner) )  ||

                    ((!( visier->flags & VISIER_ENEMY )) &&
                      (kandidat->Owner != ybd->bact.Owner) &&
                      (kandidat->Owner != 0) )  ||

                    ((!( visier->flags & VISIER_NEUTRAL )) &&
                      (kandidat->Owner == 0) ) ) {

                    /*** gleich weiter ***/
                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }

                /*** So, nun das nächstliegende in einem Bereich suchen ***/

                ziel.x = kandidat->pos.x - ybd->bact.old_pos.x;
                ziel.y = kandidat->pos.y - ybd->bact.old_pos.y;
                ziel.z = kandidat->pos.z - ybd->bact.old_pos.z;

                /*** Hinter mir? ***/
                if( (ziel.x * ybd->bact.dir.m31 + ziel.y * ybd->bact.dir.m32 +
                     ziel.z * ybd->bact.dir.m33) < 0.0 ) {

                    /*** gleich weiter ***/
                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }


                entfernung_ok = nc_sqrt(ziel.x*ziel.x + ziel.y*ziel.y + ziel.z*ziel.z);
                
                vp.x = visier->dir.y * ziel.z - ziel.y * visier->dir.z;
                vp.y = visier->dir.z * ziel.x - ziel.z * visier->dir.x;
                vp.z = visier->dir.x * ziel.y - ziel.x * visier->dir.y;

                /* ----------------------------------------------------------
                ** Wir tasten einen Kegel ab, der VISLEN Lang ist (außen) und
                ** vorn VISWIDTH breit ist.
                ** --------------------------------------------------------*/
                dmax = entfernung_ok * VISIER_WIDTH / VISIER_LEN;
                dmax += 20;  // Offset --> aus Kegel wird Kegelstumpf
                produkt = nc_sqrt( vp.x * vp.x + vp.y * vp.y + vp.z * vp.z );

                /* -------------------------------------------------------
                ** Jetzt gibt es zwei Arten der Zielsuche: Bei gelenkten
                ** Raketen untersuchen wir einen Kegel, sonst suchen
                ** wir das nächstliegende, welches auf der lokalen z-Achse
                ** liegt
                ** -----------------------------------------------------*/

                if( ((produkt < dmax) && (ybd->bact.auto_info & WPF_Searching)) ||
                    ((produkt < (kandidat->radius + weapon_radius)) &&
                     (!(ybd->bact.auto_info & WPF_Searching)) ) ) {

                    /*
                    ** Das Objekt liegt nah genug an der Strecke. Nun testen wir,
                    ** ob es auch innerhalb der zurückgelegten Strecke liegt.
                    */

                    if( entfernung_ok < VISIER_LEN ) {

                        /*** Wir haben einen, suchen aber den nächstliegenden ***/
                        //if( (found_kandidat == NULL) ||
                        //    (entfernung_ok < found_entfernung) ) {

                        /*** neu, mal den mit dem besten Winkel ***/
                        if( (found_produkt > produkt) ||
                            (found_kandidat == NULL) ) {

                            /*** merken ***/
                            found_entfernung = entfernung_ok;
                            found_kandidat   = kandidat;
                            found_produkt    = produkt;
                            }
                        }
                    }

                /*** war nix. Nächster ***/
                kandidat = (struct Bacterium *) ((struct Node *)kandidat)->ln_Succ;
                }
            }
        }


    /*** So, nun haben wir alles getestet. Gucken wir mal, was wir zurückgeben ***/
    if( found_kandidat ) {

        struct settarget_msg target;

        /*** Visier malen ***/
        yb_DrawVisier( ybd, found_kandidat, bomb );

        /*** Als Sectarget schalten ***/
        target.priority    = 1;
        target.target_type = TARTYPE_BACTERIUM;
        target.target.bact = found_kandidat;
        _methoda( o, YBM_SETTARGET, &target );

        visier->enemy = found_kandidat;
        return( TRUE );
        }
    else {

        /*** Visier malen ***/
        yb_DrawVisier( ybd, NULL, bomb );
        
        visier->enemy = NULL;
        return( FALSE );
        }
}


void yb_DrawVisier( struct ypabact_data *ybd, struct Bacterium *enemy, BOOL bomb )
{
    /*
    ** Mir müssn a Fodenkreuz zeischn'n. Die Koordinaten liegen zwischen -1 und 1
    ** für jede Achse, den Rest erledigt die Welt. Dabei nutze ich die Tatsache, daß
    ** der Öffnungswinkel 90 Grad ist.
    */
    
    FLOAT iks, yps;
    struct visor_msg visier;

    if( enemy ) {

        struct flt_triple vr, richtung;

        /*** Richtungsvektor zum Feind ***/
        richtung.x = enemy->pos.x - ybd->bact.pos.x;
        richtung.y = enemy->pos.y - ybd->bact.pos.y;
        richtung.z = enemy->pos.z - ybd->bact.pos.z;

        /* -------------------------------------------------------------
        ** Projektion des Richtungsvektors in ViewerKoordinaten. Das ist
        ** so schön klar und viel einfacher als irgendwelche komischen
        ** Verhältnisse. Warum komm' ich nicht auf solche Ideen.
        ** -----------------------------------------------------------*/
        vr.x = ybd->bact.dir.m11 * richtung.x + ybd->bact.dir.m12 * richtung.y +
               ybd->bact.dir.m13 * richtung.z;
        vr.y = ybd->bact.dir.m21 * richtung.x + ybd->bact.dir.m22 * richtung.y +
               ybd->bact.dir.m23 * richtung.z;
        vr.z = ybd->bact.dir.m31 * richtung.x + ybd->bact.dir.m32 * richtung.y +
               ybd->bact.dir.m33 * richtung.z;

        if( vr.z != 0 ) {

            iks = vr.x / vr.z;
            yps = vr.y / vr.z;
            }
        else {

            iks = 0;
            yps = 0;
            }

        /*** Das Ziel höchstselbst ***/
        visier.target = enemy;
        }
    else {

        /* ---------------------------------------------------------------------
        ** Weil der Winkel kein richtiger Winkel ist, sondern eine zahl zwischen 
        ** 0 und 1, die ich auf die y-Komponente addiere, das mache ich hier
        ** auch, weil der Öffnungswinkel wohl 90 Grad ist. Ansonsten nochmal
        ** korrigieren.
        ** -------------------------------------------------------------------*/
        iks = -ybd->bact.gun_leftright;
        yps = -ybd->bact.gun_angle_user;
        visier.target = NULL;
        }

    /*** Anmelden. zuerst MG ***/
    if( ybd->bact.mg_shot != NO_MACHINEGUN ) {

        visier.mg_type = VISORTYPE_MG;
        visier.x       = -ybd->bact.gun_leftright;
        visier.y       = -ybd->bact.gun_angle_user;
        }
    else
        visier.mg_type = VISORTYPE_NONE;

    /*** Die autonomen Waffen ***/
    if( (ybd->bact.auto_ID == NO_AUTOWEAPON) ||
        (bomb)  )
        visier.gun_type = VISORTYPE_NONE;
    else {
        
        /*** Visier zeichnen ***/
        if( ybd->bact.auto_info & WPF_Searching) {

            /*** Visier verfolgt Objekt ***/
            visier.gun_type = VISORTYPE_MISSILE;
            visier.gun_x    = iks;
            visier.gun_y    = yps;
            }
        else if ((!(ybd->bact.auto_info & WPF_Searching)) &&
                   (ybd->bact.auto_info & WPF_Driven))
            {
            /*** ungelenkte Rakete ***/
            visier.gun_type = VISORTYPE_ROCKET;
            visier.gun_x    = -ybd->bact.gun_leftright;
            visier.gun_y    = -ybd->bact.gun_angle_user;
            }
        else {
            /*** Granaten ***/
            visier.gun_type = VISORTYPE_GRENADE;
            visier.gun_x    = -ybd->bact.gun_leftright;
            visier.gun_y    = -ybd->bact.gun_angle_user;
            }
        }   

    /*** und los... ***/
    _methoda( ybd->world, YWM_VISOR, &visier );
}

