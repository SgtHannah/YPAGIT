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
#include "ypa/ypaufoclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"
#include "audio/audioengine.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine



/*-----------------------------------------------------------------*/
void yu_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yu_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle);
void yu_rot_round_global_y( struct Bacterium *bact, FLOAT angle);



/*-----------------------------------------------------------------*/
_dispatcher(void, yu_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
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
**      29-11-95        created von mir
*/


    FLOAT  time, over_eof, radius, rotangle;
    FLOAT  tardist, togo, d;
    ULONG  VIEW, CHECKVISEXACTLY, BACTCOLL, VIS, NEARPRIMTARGET;
    LONG   LWW;
    struct move_msg move;
    struct fight_msg fight;
    struct setstate_msg state;
    struct bactcollision_msg bcoll;
    struct crash_msg crash;
    struct settarget_msg target;
    struct recoil_msg rec;
    struct assesstarget_msg at1, at2;
    ULONG  rat1, rat2;

    struct intersect_msg vorn;
    struct intersect_msg hoehe;

    struct flt_triple *ops, *pos;

    struct ypaufo_data *yud = INST_DATA( cl, o );

    /*** Einige Vorbetrachtungen... ***/

    time = (FLOAT) msg->frame_time/1000;

    _get( o, YBA_CheckVisExactly, &CHECKVISEXACTLY );
    _get( o, YBA_Viewer, &VIEW );
    _get( o, YBA_BactCollision, &BACTCOLL );
    VIS = _methoda( yud->world, YWM_ISVISIBLE, yud->bact );

    if( VIEW ) {
        radius   = yud->bact->viewer_radius;
        over_eof = yud->bact->viewer_over_eof;
    } else {
        radius   = yud->bact->radius;
        over_eof = yud->bact->over_eof;
    }
    

    /* tar_vec normieren */
    tardist = nc_sqrt( yud->bact->tar_vec.x * yud->bact->tar_vec.x +
                       yud->bact->tar_vec.y * yud->bact->tar_vec.y +
                       yud->bact->tar_vec.z * yud->bact->tar_vec.z );
    
    if( tardist != 0.0 ) {
        yud->bact->tar_unit.x = yud->bact->tar_vec.x / tardist;
        yud->bact->tar_unit.y = yud->bact->tar_vec.y / tardist;
        yud->bact->tar_unit.z = yud->bact->tar_vec.z / tardist;
        }

    if( (TARTYPE_NONE == yud->bact->SecTargetType) &&
        (tardist      <  SECTOR_SIZE) )
        NEARPRIMTARGET = TRUE;
    else
        NEARPRIMTARGET = FALSE;

    /*** Natürlich müssen wir an das Ziel auch rankommen! ***/
    if( tardist > yud->togo )
        togo = yud->togo;
    else
        togo = tardist;

    switch( yud->bact->MainState ) {

        case ACTION_NORMAL:

            yud->bact->act_force = yud->bact->max_force;

            /*** Bakterienkollision ***/
            if( BACTCOLL &&
                (VIS || NEARPRIMTARGET) ) {
                bcoll.frame_time  = msg->frame_time;
                if( _methoda( o, YBM_BACTCOLLISION, &bcoll)) return;
                }

            /*** Noch ein Ziel da? ***/
            if( ( yud->bact->PrimTargetType == TARTYPE_NONE ) &&
                ( yud->bact->SecTargetType  == TARTYPE_NONE ) ) {

                /*** in ACTION_WAIT übergehen. VProto sofort setzen, da ***/
                /*** UFO's nicht landen                                 ***/
                
                yud->bact->ExtraState &= ~(EXTRA_FIGHTP | EXTRA_FIGHTS);
                if( yud->bact->ExtraState & EXTRA_FIRE ) {

                    state.extra_on  = state.main_state = 0;
                    state.extra_off = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                state.main_state = ACTION_WAIT;
                state.extra_on   = state.extra_off = 0;
                _methoda( o, YBM_SETSTATE, &state );
                break;
                }

            
            /*** Standards, die später evtl. verändert werden ***/
            yud->buoyancy = yud->bact->mass * GRAVITY;
            yud->bact->act_force = yud->bact->max_force;

            /* --------------------------------------------------------------
            ** Wenn ein Hindernis da ist (vorn oder unten, bremse ich ab und 
            ** bearbeite die Situation. Zu große Höhe und Abweichung vom Ziel
            ** bearbeite ich nach einer Mindeststrecke
            ** ------------------------------------------------------------*/
            pos = &yud->bact->pos;
            ops = &yud->bact->old_pos;

            /*** Haben wir einen Ausweichauftrag? ***/
            if( yud->flags & YUF_AVOID ) {

                /*** act_force bleibt auf 0 ***/
                yud->bact->act_force  = 0;

                /*** Stillstand für Manöver? ***/
                if( (fabs( yud->bact->dof.v * yud->bact->dof.x) < IS_SPEED) &&
                    (fabs( yud->bact->dof.v * yud->bact->dof.z) < IS_SPEED) ) {

                    /*** Ich darf reagieren. War es ein Drehen? ***/
                    if( yud->flags & YUF_ROT ) {

                        /*** ROTIEREN / rotangle ist Vorzeichenabhängig ***/
                        if( fabs( yud->rotangle ) < (yud->bact->max_rot * time) ) {

                            /*** letztes Stück drehen ***/
                            yu_rot_round_lokal_y( yud->bact, yud->rotangle );
                            yud->rotangle = 0.0;

                            /*** Ausweichmanöver abmelden ***/
                            yud->flags &= ~(YUF_AVOID | YUF_ROT );
                            yud->gone   = togo;
                            }
                        else {

                            /*** Rotieren, was möglich ist ***/
                            if( yud->rotangle < 0.0 )
                                rotangle = -yud->bact->max_rot * time;
                            else
                                rotangle =  yud->bact->max_rot * time;
                            yud->rotangle -= rotangle;
                            yu_rot_round_lokal_y( yud->bact, rotangle );
                            }
                        }
                    else {

                        if( yud->flags & YUF_GOUP ) {

                            /* -----------------------------------------------
                            ** Wir müssen nach oben und setzen deshalb nur die
                            ** Auftriebskraft hoch. Ob wr den Vorgang beenden
                            ** müssen, entscheiden wir bei den Höhentests
                            ** ---------------------------------------------*/
                            yud->buoyancy = 2 * yud->bact->mass * GRAVITY;
                            }

                        if( yud->flags & YUF_GODOWN ) {

                            /* ---------------------
                            ** Wir müssen nach unten
                            ** -------------------*/
                            yud->buoyancy = 0.5 * yud->bact->mass * GRAVITY;
                            }
                        }
                    }
                else {

                    /* --------------------------------------------------------
                    ** Wir wollen ein Ausweichmanöver machen, sind aber noch zu 
                    ** schnell. Also bremsen
                    ** ------------------------------------------------------*/

                    yud->bact->act_force  = 0;
                    yud->bact->dof.v     *= 0.8;
                    }
                }

            /*** Nun geht es an die Bewegung ***/
            move.flags = 0;
            move.t     = time;
            _methoda( o, YBM_MOVE, &move );

            /* ---------------------------------------------------------------
            ** Jetzt kommt die Kollisionsuntersuchung. Wir schicken 2 Strahlen
            ** los (nach vorn und nach unten) und werten die Resultate aus.
            ** gab es eine direkte Kollision, prallen wir ab, werten aber
            ** nicht aus, das passiert dann automatisch im nächsten Frame.
            ** -------------------------------------------------------------*/
            vorn.pnt.x = ops->x;
            vorn.pnt.y = ops->y;
            vorn.pnt.z = ops->z;
            
            d = nc_sqrt( (pos->x - ops->x) * (pos->x - ops->x) +
                         (pos->z - ops->z) * (pos->z - ops->z) );
            
            if( d < 0.01 ) {

                vorn.vec.x = yud->bact->dir.m31 * 300;
                vorn.vec.y = yud->bact->dir.m32 * 300;
                vorn.vec.z = yud->bact->dir.m33 * 300;
                }
            else {


                vorn.vec.x = (pos->x - ops->x) * 300 / d;
                vorn.vec.y = 0.0;
                vorn.vec.z = (pos->z - ops->z) * 300 / d;
                }
             
            hoehe.pnt.x = pos->x;  // pos, weil das aktueller ist
            hoehe.pnt.y = pos->y;
            hoehe.pnt.z = pos->z;
            hoehe.vec.x = hoehe.vec.z = 0.0;
            hoehe.vec.y = yud->bact->pref_height;

            vorn.flags  = 0;
            hoehe.flags = 0;

            _methoda( yud->world, YWM_INTERSECT, &hoehe );
            _methoda( yud->world, YWM_INTERSECT, &vorn );

            /*** War das ein Crash ? ***/
            if( (vorn.insect  && (vorn.t * 300  < yud->bact->radius)) ||
                (hoehe.insect && (hoehe.t * 300 < yud->bact->radius)) ) {

                /* ----------------------------------------------------------
                ** Wir prallen ab, dadurch ändert sich die Situation, so daß
                ** wir uns darauf verlassen, daß alles im nächsten Frame noch 
                ** korrekt bearbeitet wird.
                ** --------------------------------------------------------*/
                
                if( vorn.insect && (vorn.t * 300 < yud->bact->radius)) {

                    /*** Aha, davon müssen wir abprallen ***/
                    rec.vec.x = vorn.sklt->PlanePool[ vorn.pnum ].A;
                    rec.vec.y = vorn.sklt->PlanePool[ vorn.pnum ].B;
                    rec.vec.z = vorn.sklt->PlanePool[ vorn.pnum ].C;
                    rec.mul_y = 0.7; rec.mul_y = 2.0; rec.time = time;
                    _methoda( o, YBM_RECOIL, &rec );
                    }
                else {

                    /*** Dann war es logischerweise die Höhenintersection ***/
                    rec.vec.x = hoehe.sklt->PlanePool[ hoehe.pnum ].A;
                    rec.vec.y = hoehe.sklt->PlanePool[ hoehe.pnum ].B;
                    rec.vec.z = hoehe.sklt->PlanePool[ hoehe.pnum ].C;
                    rec.mul_y = 0.7; rec.mul_y = 2.0; rec.time = time;
                    _methoda( o, YBM_RECOIL, &rec );
                    }

                /*** und tschüß ***/
                return;
                }
            else {

                /* ---------------------------------------------------------
                ** Also normal weiter. Wenn wir gerade ein Ausweichmanöver 
                ** bearbeiten, dann machen wir das. ansonsten werten wir die
                ** Intersections aus
                ** -------------------------------------------------------*/
                if( yud->flags & YUF_AVOID ) {

                    /* ------------------------------------------------------
                    ** Ok, wir reagieren nur auf YUF_GOUP, weil das andere
                    ** schon vorher bearbeitet wurde. Wenn keine Intersection 
                    ** nach unten ist oder sie nicht mehr stark ist, schalten
                    ** wir die Sache aus
                    ** ----------------------------------------------------*/
                    if( yud->flags & (YUF_GOUP|YUF_GODOWN) ) {

                        if( yud->flags & YUF_GOUP ) {

                            /*** schon zu hoch? ***/
                            if( (!hoehe.insect) ||
                                ( hoehe.insect && (hoehe.t > 0.8) ) ) {

                                yud->flags &= ~( YUF_GOUP | YUF_AVOID );
                                yud->gone   = togo;
                                }
                            }

                        if( yud->flags & YUF_GODOWN ) {

                            /*** schon zu tief? ***/
                            if( (hoehe.insect && (hoehe.t < 0.8) ) ) {

                                yud->flags &= ~( YUF_GODOWN | YUF_AVOID );
                                yud->gone   = togo;
                                }
                            }
                        }
                    }
                else {

                    /* ---------------------------------------------------
                    ** Gucken wir mal, ob die Situation kritsch ist. dann
                    ** erstellen wir einen Ausweichauftrag. Andernfalls
                    ** sehen wir mal nach, ob der MindestWeg unter 0 ist,
                    ** dann können wir mal sehen, ob wir zu hoch sind oder
                    ** vom rechten Pfad abgekommen sind
                    ** -------------------------------------------------*/
                    if( (vorn.insect  && (vorn.t  < 0.5)) ||
                        (hoehe.insect && (hoehe.t < 0.5)) ) {

                        /*** Kritische Situation. was machen ***/
                        if( (hoehe.insect && (hoehe.t < 0.5)) ) {

                            /*** nach oben ***/
                            yud->flags |= ( YUF_AVOID | YUF_GOUP );
                            }
                        else {

                            /*** Ausweichwinkel ermitteln ***/
                            yud->flags |= (YUF_AVOID | YUF_ROT);
                            
                            yud->rotangle = PI / 2 - nc_acos(
                            
                            (yud->bact->dir.m31 * vorn.sklt->PlanePool[vorn.pnum].A +
                             yud->bact->dir.m33 * vorn.sklt->PlanePool[vorn.pnum].C) /
                            
                            nc_sqrt(yud->bact->dir.m31 * yud->bact->dir.m31 +
                                    yud->bact->dir.m33 * yud->bact->dir.m33) /
                            nc_sqrt(vorn.sklt->PlanePool[ vorn.pnum ].A *
                                    vorn.sklt->PlanePool[ vorn.pnum ].A +
                                    vorn.sklt->PlanePool[ vorn.pnum ].C *
                                    vorn.sklt->PlanePool[ vorn.pnum ].C) );
                            }
                        }
                    else {

                        /* -------------------------------------------------------
                        ** Nichts schlimmes. Normalerweise würden wir uns bewegen.
                        ** Wenn der Mindestweg allerdings zurückgelegt ist, können
                        ** mal gucken, wie es so ist....
                        ** -----------------------------------------------------*/
                        if( yud->gone <= 0.0 ) {

                            /*** zu hoch? ***/
                            if( !hoehe.insect ) {

                                /* ----------------------------------------------------
                                ** Eigentlich müssen wir runter, wenn wir zu hoch sind,
                                ** aber da kann es ja noch sein, daß wir gerade den
                                ** User als Ziel haben, der ungehindert nach oben ab-
                                ** hauen kann. Dann korrigieren wir nicht.
                                ** --------------------------------------------------*/
                                Object *user;
                                _get( yud->world, YWA_UserVehicle, &user );

                                if( !( (((TARTYPE_BACTERIUM == yud->bact->SecTargetType) &&
                                         (yud->bact->SecondaryTarget.Bact->BactObject == user)) ||
                                        ((TARTYPE_BACTERIUM == yud->bact->PrimTargetType) &&
                                         (yud->bact->PrimaryTarget.Bact->BactObject == user))) &&
                                         (yud->bact->tar_unit.y < 0.0) ) )
                                    yud->flags |= (YUF_AVOID | YUF_GODOWN );
                                }
                            else {

                                /*** Vom Ziel abgewichen? ***/
                                rotangle = nc_acos(
                            
                                (yud->bact->dir.m31 * yud->bact->tar_unit.x +
                                 yud->bact->dir.m33 * yud->bact->tar_unit.z) /
                                
                                nc_sqrt(yud->bact->dir.m31 * yud->bact->dir.m31 +
                                        yud->bact->dir.m33 * yud->bact->dir.m33) /
                                nc_sqrt(yud->bact->tar_unit.x * yud->bact->tar_unit.x +
                                        yud->bact->tar_unit.z * yud->bact->tar_unit.z));

                                if( rotangle > 0.2 ) {

                                    /*** Eindrehen ***/
                                    if( (yud->bact->dir.m31 * yud->bact->tar_unit.z -
                                         yud->bact->dir.m33 * yud->bact->tar_unit.x)<0)
                                         rotangle = -rotangle;
                                    yud->rotangle = rotangle;

                                    yud->flags |= (YUF_AVOID | YUF_ROT );
                                    }
                                }
                            }
                        }
                    }
                }


            /*** Fight fire with fire ***/
            fight.time        = time;
            fight.global_time = yud->bact->internal_time;
            if( yud->bact->SecTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                fight.enemy.bact = yud->bact->SecondaryTarget.Bact;
                fight.priority   = 1;
                _methoda(o, YBM_FIGHTBACT, &fight );
                }
            else
                if( yud->bact->SecTargetType == (UBYTE)TARTYPE_SECTOR ) {
                    fight.pos          = yud->bact->SecPos;
                    fight.enemy.sector = yud->bact->SecondaryTarget.Sector;
                    fight.priority     = 1;
                    _methoda(o, YBM_FIGHTSECTOR, &fight );
                    }
                else
                    if( yud->bact->PrimTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                        fight.enemy.bact = yud->bact->PrimaryTarget.Bact;
                        fight.priority   = 0;
                        _methoda(o, YBM_FIGHTBACT, &fight );
                        }
                    else
                        if( yud->bact->PrimTargetType == (UBYTE)TARTYPE_SECTOR ){
                            fight.pos          = yud->bact->PrimPos;
                            fight.enemy.sector = yud->bact->PrimaryTarget.Sector;
                            fight.priority     = 0;
                            _methoda(o, YBM_FIGHTSECTOR, &fight );
                            }
                        else {

                            /* --------------------------------------------------
                            ** was anderes bekämpfen wir nicht. Es kann aber noch
                            ** sein, daß wir noch feuern 
                            ** ------------------------------------------------*/
                            yud->bact->ExtraState &= ~(EXTRA_FIGHTP | EXTRA_FIGHTS);
                            if( yud->bact->ExtraState & EXTRA_FIRE ) {

                                state.extra_on  = state.main_state = 0;
                                state.extra_off = EXTRA_FIRE;
                                _methoda( o, YBM_SETSTATE, &state );
                                }
                            }


            break;

        
        case ACTION_WAIT:

            /*
            ** Solange, bis ein neues Ziel registriert wurde, dann wieder
            ** NORMAL. In dieser Zeit kann man allen möglichen Unsinn machen 
            */

            if( (yud->bact->internal_time - yud->bact->newtarget_time) > 500 ) {
            
                /*** Kann ich mir das leisten? ***/
                at1.target_type = yud->bact->SecTargetType;
                at1.priority    = 1;
                at2.target_type = yud->bact->PrimTargetType;
                at2.priority    = 0;
                rat1 = _methoda( o, YBM_ASSESSTARGET, &at1 );
                rat2 = _methoda( o, YBM_ASSESSTARGET, &at2 );

                if( (rat1 != AT_IGNORE) || (rat2 != AT_IGNORE ) ) {

                    /*** was machen ***/
                    if( rat1 == AT_REMOVE ) {

                        /*** ich bin zu schwach oder keine Lust... ***/
                        target.target_type = TARTYPE_NONE;
                        target.priority    = 1;
                        _methoda( o, YBM_SETTARGET, &target );
                        }

                    if( rat2 == AT_REMOVE ) {

                        /*** ich bin zu schwach oder keine Lust... ***/
                        target.target_type = TARTYPE_SECTOR;
                        target.pos.x       = yud->bact->pos.x;
                        target.pos.z       = yud->bact->pos.z;
                        target.priority    = 0;
                        _methoda( o, YBM_SETTARGET, &target );
                        }

                    /*** Ist noch was übriggeblieben? ***/
                    if( (yud->bact->PrimTargetType != TARTYPE_NONE) ||
                        (yud->bact->SecTargetType  != TARTYPE_NONE) ) {

                        /*** Ja, also raus aus der Warteschleife ***/
                        state.extra_off  = EXTRA_LANDED;    /* doch */
                        state.extra_on   = 0;
                        state.main_state = ACTION_NORMAL;
                        _methoda( o, YBM_SETSTATE, &state);
                        break;
                        }
                    }
                }

            /*** die Untergebenen ranholen ***/
            //_methoda( o, YBM_ASSEMBLESLAVES, NULL );

            /*** Ä bissel drehen, net landen ***/
            _get( o, YBA_LandWhileWait, &LWW );
            if( LWW ) {

                yud->bact->act_force = 0;
                yud->buoyancy        = 0;
                if( !(yud->bact->ExtraState & EXTRA_LANDED) ) {

                    crash.flags      = 0;
                    crash.frame_time = msg->frame_time;
                    _methoda( o, YBM_CRASHBACTERIUM, &crash);
                    }
                else {

                    struct intersect_msg inter;

                    state.extra_on = state.extra_off = 0;
                    state.main_state = ACTION_WAIT;
                    _methoda( o, YBM_SETSTATE, &state);

                    /*** Am Boden ausrichten ***/
                    inter.pnt.x = yud->bact->pos.x;
                    inter.pnt.y = yud->bact->pos.y;
                    inter.pnt.z = yud->bact->pos.z;
                    inter.vec.x = 0.0;
                    inter.vec.y = yud->bact->over_eof + 50;
                    inter.vec.z = 0.0;
                    inter.flags = 0;
                    _methoda( yud->world, YWM_INTERSECT, &inter );
                    if( inter.insect ) yud->bact->pos.y = inter.ipnt.y -
                                                          yud->bact->over_eof;
                    }
                }
            else {

                yud->bact->act_force = 0;
                yu_rot_round_global_y( yud->bact,
                                       (FLOAT)( yud->bact->max_rot * time ));
                }

            break;

        /*-------------------------------------------------------------------*/

        case ACTION_DEAD:
            
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
_dispatcher(void, yu_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**
**
**  INPUTS
**
**  RESULTS
**
**      Ermittelt Kraftvektor für Flugverhalten (Move)
**
**  CHANGED
**      29-Nov-95   8100 000C    created
*/
{
    
    FLOAT  time, angle, A, B, C, betrag, norm_height;
    struct move_msg move;
    struct ypaufo_data *yud = INST_DATA(cl,o);
    struct intersect_msg inter;
    struct Insect insect[10];
    ULONG  BACTCOLL, maxcollisioncounts;
    struct setstate_msg state;
    struct bactcollision_msg bcoll;
    struct recoil_msg rec;
    struct insphere_msg ins;
    struct flt_triple vec;
    struct firemissile_msg fire;
    struct fireyourguns_msg fyg;
    struct visier_msg visier;
    BOOL   coll;


    /*
    ** khdmfdg.,mfcnvkjvckj  b  ,cxkjcxnf,mfd g,mfcjhlkjngmdsf ,dxmflkxj
    */

    yud->bact->air_const = yud->bact->bu_air_const;

    _get( o, YBA_BactCollision, &BACTCOLL);
    time = (FLOAT) msg->frame_time/1000;
    yud->bact->old_pos = yud->bact->pos;


    switch( yud->bact->MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:

            /* ----------------------------------------------------------------------
            ** UFOs dienen der Beobachtung. Es wäre wünschenswert, aus den
            ** UFOs rauszuschauen und dabei beliebig den Kopf zu drehen. Um
            ** die UFOs nicht wie die Robos mit Extra-Viewer-Hacks zu überladen,
            ** wird bei HandleInput die Bewegung grundsätzlich ausgeschaltet.
            ** außerdem sparen wir uns aufwendige Kollisionsverhütungstests.
            ** Das Ziel sollte erhalten bleiben, so daß das UFO nachdem wir 
            ** uns rausgeschalten haben, weiterfliegen kann. In SetAttrs/Viewer/FALSE 
            ** müssen wir die lokale Matrix dann wieder auf die Standardmatrix 
            ** rücksetzen, damit das UFO fliegen kann.
            ** --------------------------------------------------------------------*/


            /*** Untergebene ausbremsen ***/
            if( yud->bact->dof.v == 0.0 ) {

                /*** Optisch wait nur, wenn wir gelandet sind ***/
                inter.pnt.x = yud->bact->pos.x;
                inter.pnt.y = yud->bact->pos.y;
                inter.pnt.z = yud->bact->pos.z;
                inter.vec.x = inter.vec.z = 0.0;
                inter.vec.y = 1.5 * max( yud->bact->viewer_over_eof,
                                         yud->bact->viewer_radius);
                inter.flags = 0;
                _methoda( yud->world, YWM_INTERSECT, &inter );
                if( inter.insect &&
                    (yud->bact->act_force == 0) &&
                    (yud->buoyancy <= yud->bact->mass * GRAVITY) ) {

                    yud->bact->ExtraState |= EXTRA_LANDED;
                    yud->bact->dof.v       = 0.0;
                    yud->bact->act_force   = 0.0;
                    yud->buoyancy          = yud->bact->mass * GRAVITY;
                    }
                else
                    yud->bact->ExtraState &= ~EXTRA_LANDED;

                /*** Wenn wir in Zielnähe sind, warten wir nur "scheinbar" ! ***/
                if( (yud->bact->PrimTargetType == TARTYPE_SECTOR) &&
                    (nc_sqrt( (yud->bact->PrimPos.x - yud->bact->pos.x ) * 
                              (yud->bact->PrimPos.x - yud->bact->pos.x ) +
                              (yud->bact->PrimPos.z - yud->bact->pos.z ) * 
                              (yud->bact->PrimPos.z - yud->bact->pos.z ) ) >
                             (2 * FOLLOW_DISTANCE)) ) {
                    
                    /*** weit weg ***/
                    yud->bact->MainState = ACTION_WAIT;

                    if( (yud->bact->ExtraState & EXTRA_LANDED) &&
                        (!(yud->bact->ExtraState & EXTRA_FIRE)) ) {
                        
                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                else {

                    /*** Am Kampfort ***/
                    if( (yud->bact->ExtraState & EXTRA_LANDED) &&
                        (!(yud->bact->ExtraState & EXTRA_FIRE )) ) {

                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    yud->bact->MainState = ACTION_NORMAL;    // !!
                    }
                }
            else {

                if( !(yud->bact->ExtraState & EXTRA_FIRE )) {

                    state.main_state = ACTION_NORMAL;
                    state.extra_on = state.extra_off = 0;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                /*** Landed ausschalten ***/
                yud->bact->ExtraState &= ~EXTRA_LANDED;
                }


            /* -------------------------------------------
            **              Drehung unf Höhe
            ** -----------------------------------------*/

            /*** Seitendrehung ***/
            angle = -msg->input->Slider[ SL_FLY_DIR ] *
                     yud->bact->max_rot * time;

            if( fabs( angle ) > 0.0 ) {

                /*** Drehen, dann am Untergrund ausrichten und tschüß ***/
                yu_rot_round_global_y( yud->bact, angle );
                }
            
            /*** vertikale Kopfdrehung ***/
            angle = msg->input->Slider[ SL_FLY_HEIGHT ] *
                    yud->bact->max_rot * time;

            if( fabs( angle ) > 0.0 ) {

                /*** Drehen, dann am Untergrund ausrichten und tschüß ***/
                yu_rot_round_lokal_x( yud->bact, angle );
                }

            /*** Höhenänderung ***/
            if( msg->input->Slider[ SL_FLY_SPEED ] != 0.0 ) {

                /*** Veränderung der Höhe gewünscht ***/
                yud->buoyancy = (1 + 4 * msg->input->Slider[ SL_FLY_SPEED ]) *
                                 yud->bact->mass * GRAVITY;

                /*** Maximalhöhenkorrektur ***/
                norm_height = yud->bact->Sector->Height - yud->bact->pos.y;
                if( yud->buoyancy > ( 5 * yud->bact->mass * GRAVITY *
                                      (yud->bact->max_user_height - norm_height) /
                                       yud->bact->max_user_height ) )
                    yud->buoyancy = 5 * yud->bact->mass * GRAVITY *
                                       (yud->bact->max_user_height - norm_height) /
                                       yud->bact->max_user_height;
                }
            else {

                /*** Höhe lassen ***/
                yud->buoyancy = yud->bact->mass * GRAVITY;
                }

            /*  -----------------------------------
            **      Kollision mit den Bakterien
            **  ---------------------------------*/

            if( BACTCOLL && (!( yud->bact->ExtraState & EXTRA_LANDED)) ) {

                bcoll.frame_time = msg->frame_time;
                _methoda( o, YBM_BACTCOLLISION, &bcoll);
                }

            /* ----------------------------------------------------------------
            ** Abbremsen. Weil sofortiger Stillstand blöd erscheint und wir
            ** auch auf Einschüsse reagieren müssen, muß Bewegung real 
            ** stattfinden. Deshalb runter mit act_force und v in x-z-Richtung,
            ** denn die Höhenänderung sollte schon noch funktionieren.
            ** --------------------------------------------------------------*/


            //yud->bact->act_force = 0;

            //if( ( fabs( yud->bact->dof.x * yud->bact->dof.v ) > IS_SPEED ) ||
            //    ( fabs( yud->bact->dof.z * yud->bact->dof.v ) > IS_SPEED ) )
            //    yud->bact->dof.v *= 0.6;


            if( msg->input->Buttons & BT_STOP ) {

                /* ------------------------------------------------------
                ** Bremsen. Das lassen wir auch, weil hier das Ausrichten 
                ** mit drin ist, auch wenn das vielleicht keiner braucht
                ** ----------------------------------------------------*/

                yud->bact->act_force = 0.0;
                yud->buoyancy = yud->bact->mass * GRAVITY;
                yud->bact->dof.v *= 0.7;
                if( yud->bact->dof.v < IS_SPEED ) yud->bact->dof.v = 0;
                }

            /*** Zielaufnahme ***/
            fire.target_type  = TARTYPE_SIMPLE;
            fire.target_pos.x = yud->bact->dir.m31;
            fire.target_pos.y = yud->bact->dir.m32;
            fire.target_pos.z = yud->bact->dir.m33;

            visier.flags = VISIER_ENEMY | VISIER_NEUTRAL;
            visier.dir.x = yud->bact->dir.m31;
            visier.dir.y = yud->bact->dir.m32;
            visier.dir.z = yud->bact->dir.m33;
            if( _methoda( o, YBM_VISIER, &visier ) ) {

                fire.target_type = TARTYPE_BACTERIUM;
                fire.target.bact = visier.enemy;
                }


            /*** Schießen ***/
            if( (msg->input->Buttons & BT_FIRE ) ||
                (msg->input->Buttons & BT_FIREVIEW ) ) {

                fire.wtype = yud->bact->auto_ID;
                fire.dir.x = yud->bact->dir.m31;
                fire.dir.y = yud->bact->dir.m32;
                fire.dir.z = yud->bact->dir.m33;
                fire.target_type = TARTYPE_NONE;
                fire.global_time = yud->bact->internal_time;
                /*** UFO. etwas tiefer ***/
                if( yud->bact->internal_time % 2 == 0 )
                    fire.start.x   = yud->bact->firepos.x;
                else
                    fire.start.x   = -yud->bact->firepos.x;
                fire.start.y       = yud->bact->firepos.y;
                fire.start.z       = yud->bact->firepos.z;
                /*** Viewer? ***/
                if( msg->input->Buttons & BT_FIREVIEW )
                    fire.flags = FIRE_VIEW;
                else
                    fire.flags = 0;
                
                _methoda( o, YBM_FIREMISSILE, &fire );
                }

            /*** Wenn keine Waffe, dann mit Schuss mal kurz Schub geben ***/
            if( yud->bact->auto_ID == NO_AUTOWEAPON ) {

                FLOAT max_force = yud->bact->max_force;

                /* -------------------------------------------
                ** Mit Feuer geben wir einen Impuls, aber
                ** nicht kontinuierlich, Sondern nur, wenn die
                ** Kraft wieder auf 0 ist
                ** -----------------------------------------*/
                if( (msg->input->Buttons & BT_FIRE) ||
                    (msg->input->Buttons & BT_FIREVIEW) ) {

                    if( yud->bact->act_force < max_force ) {

                        yud->bact->act_force += (msg->frame_time *
                                                 yud->bact->max_force/ 100);
                        }
                    }
                else {

                    /*** Ausklingen lassen, wenn keine Bewegung! ***/
                    if( yud->bact->act_force > 0)
                        yud->bact->act_force -= (max_force * msg->frame_time/1000);
                    else {
                        yud->bact->act_force = 0;

                        if( msg->input->Slider[ SL_FLY_SPEED ] == 0.0 )
                            yud->bact->dof.v *= 0.6;
                        }
                    }
                }
            else {

                yud->bact->act_force = 0;
                if( ( fabs( yud->bact->dof.x * yud->bact->dof.v ) > IS_SPEED ) ||
                    ( fabs( yud->bact->dof.z * yud->bact->dof.v ) > IS_SPEED ) )
                    yud->bact->dof.v *= 0.6;
                }

            /*** Schießen, Kanone ***/
            if( yud->bact->mg_shot != NO_MACHINEGUN ) {

                if( (yud->bact->ExtraState & EXTRA_FIRE) &&
                    (msg->input->Buttons & BT_FIREGUN) ) {

                    /*** Feuern aus ***/
                    // Abmelden beim WO

                    state.main_state = state.extra_on = 0;
                    state.extra_off  = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                if( msg->input->Buttons & BT_FIREGUN ) {

                    /*** Feuern einschalten? ***/
                    if( !(yud->bact->ExtraState & EXTRA_FIRE) ) {

                        // Anmelden WO

                        state.main_state = state.extra_off = 0;
                        state.extra_on   = EXTRA_FIRE;
                        _methoda( o, YBM_SETSTATE, &state );
                        }

                    /*** Feuern an sich ***/
                    fyg.dir.x = yud->bact->dir.m31;
                    fyg.dir.y = yud->bact->dir.m32;
                    fyg.dir.z = yud->bact->dir.m33;
                    fyg.time  = time;
                    fyg.global_time = yud->bact->internal_time;
                    _methoda( o, YBM_FIREYOURGUNS, &fyg );
                    }
                }


            if( !(yud->bact->ExtraState & EXTRA_LANDED )) {


                maxcollisioncounts = 4;
                while( maxcollisioncounts-- ) {

                    /* ------------------------------------------------------
                    ** So, nun fahren wir. Fahrtrichtung ist immer die lokale
                    ** z-Achse.
                    ** ----------------------------------------------------*/

                    move.t     = (FLOAT) msg->frame_time / 1000;
                    move.flags = 0;
                    _methoda( o, YBM_MOVE, &move );


                    /*** Kollisionsuntersuchung ***/
                    coll = FALSE;

                    ins.pnt.x       = yud->bact->pos.x;
                    ins.pnt.y       = yud->bact->pos.y;
                    ins.pnt.z       = yud->bact->pos.z;
                    ins.dof.x       = yud->bact->dof.x;
                    ins.dof.y       = yud->bact->dof.y;
                    ins.dof.z       = yud->bact->dof.z;
                    ins.radius      = yud->bact->viewer_radius;
                    ins.chain       = insect;
                    ins.max_insects = 10;
                    ins.flags       = 0;

                    _methoda( yud->world, YWM_INSPHERE, &ins );

                    if( ins.num_insects > 0 ) {

                        /*
                        ** Es trat eine Kollision auf! Wir summieren alle Normalen-
                        ** vektoren der Ebenen auf und prallen von dort ab. Dann gehen
                        ** wir mit großer wait_time sofort zurück.
                        */
                        coll = TRUE;

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
                            vec.x = yud->bact->dof.x;
                            vec.y = yud->bact->dof.y;
                            vec.z = yud->bact->dof.z;
                            }
                        else {

                            vec.x = A / betrag;
                            vec.y = B / betrag;
                            vec.z = C / betrag;
                            }

                        yud->bact->act_force = 0.0;
                        
                        rec.vec.x = vec.x;      rec.vec.y = vec.y;
                        rec.vec.z = vec.z;      rec.time = time;
                        rec.mul_y = 2.0;        rec.mul_v = 0.7;
                        _methoda( o, YBM_RECOIL, &rec);

                        coll = TRUE;
                        }


                    /* 
                    ** Andernfalls kann es immer noch passieren, daß wir unter
                    ** die Welt geknallt sind 
                    */
                    if( !coll ) {

                        inter.pnt.x = yud->bact->old_pos.x;
                        inter.pnt.y = yud->bact->old_pos.y;
                        inter.pnt.z = yud->bact->old_pos.z;
                        inter.vec.x = yud->bact->pos.x - yud->bact->old_pos.x;
                        inter.vec.y = yud->bact->pos.y - yud->bact->old_pos.y;
                        inter.vec.z = yud->bact->pos.z - yud->bact->old_pos.z;
                        inter.flags = 0;
                        _methoda( yud->world, YWM_INTERSECT, &inter );
                        if( inter.insect ) {

                            /*** Abprallen ***/
                            yud->bact->act_force = 0.0;
                            coll = TRUE;
                            
                            rec.vec.x = inter.sklt->PlanePool[ inter.pnum ].A;
                            rec.vec.y = inter.sklt->PlanePool[ inter.pnum ].B;
                            rec.vec.z = inter.sklt->PlanePool[ inter.pnum ].C;
                            
                            rec.time = time;
                            rec.mul_y = 2.0;        rec.mul_v = 0.7;
                            _methoda( o, YBM_RECOIL, &rec);
                            }
                        }

                    if( !coll ) {

                        yud->bact->ExtraState &= ~EXTRA_LANDCRASH;
                        break;
                        }
                    else {

                        /*** Knallgeräusch ***/
                        if( (!(yud->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE )) &&
                            (!(yud->bact->ExtraState & EXTRA_LANDCRASH )) ) {

                            struct yw_forcefeedback_msg ffb;

                            yud->bact->ExtraState |= EXTRA_LANDCRASH;
                            _StartSoundSource( &(yud->bact->sc), VP_NOISE_CRASHLAND );

                            ffb.type    = YW_FORCE_COLLISSION;
                            ffb.power   = 1.0;

                            /*** Trick: Pos + Richtung ***/
                            ffb.dir_x   = yud->bact->pos.x + 10 * A;
                            ffb.dir_y   = yud->bact->pos.z + 10 * C;
                            _methoda( yud->world, YWM_FORCEFEEDBACK, &ffb );
                            }
                        }
                    }
                }
            else {

                /*** Move trotzdem ***/
                move.t     = (FLOAT) msg->frame_time / 1000;
                move.flags = 0;
                _methoda( o, YBM_MOVE, &move );
                }

            break;

        case ACTION_DEAD:

            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;
        }
    return;
    
}


_dispatcher( void, yu_YBM_REINCARNATE, void *nix )
{
    struct ypaufo_data *yud;
    yud = INST_DATA( cl, o );

    /*** nach oben ***/
    _supermethoda(cl, o, YBM_REINCARNATE, NULL );

    yud->buoyancy       = 0.0;

    /*** Flyer sind defaultmäßig keine Lander :-( ***/
    _set( o, YBA_LandWhileWait, FALSE );
}


/*
** Die diverstesten Routinen, die wir so brauchen.
*/


void yu_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
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

void yu_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = 1.0;     rm.m12 = 0.0;          rm.m13 = 0.0;
    rm.m21 = 0.0;     rm.m22 = cos_y;        rm.m23 = sin_y;
    rm.m31 = 0.0;     rm.m32 = -sin_y;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}

void yu_rot_round_global_y( struct Bacterium *bact, FLOAT angle)
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


