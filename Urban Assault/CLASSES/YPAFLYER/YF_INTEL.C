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


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

#define MAX_NEIGUNG         (1.0)



/*-----------------------------------------------------------------
** Brot-O-Typen
*/
void yf_DrehObjektInFlugrichtung( struct ypaflyer_data *yfd, FLOAT time,
                                  struct flt_triple *old_dir, BOOL view );
void yf_CalculateForce( struct ypaflyer_data *yfd, FLOAT time );
void yf_rot_round_lokal_y( struct Bacterium  *bact, FLOAT angle );
void yf_rot_round_global_y( struct Bacterium  *bact, FLOAT angle );


/*-----------------------------------------------------------------*/
_dispatcher(void, yf_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
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
**      Eine Objektausrichtung und eine aktuelle Kraft, aus der Move die
**      Bewegung berechnen kann.
**
**  CHANGED
**       1-Nov-95   8100000C    created
*/

    struct ypaflyer_data *yfd;

    FLOAT  betrag, radius, over_eof;
    FLOAT  max_speed, time;
    struct move_msg move;
    struct flt_triple *bold, *bpos;
    struct intersect_msg vorn, rechts, links, hoehe;
    ULONG  NEARPRIMTARGET, BACTCOLLISION, VIS, CHECKVISEXACTLY, VIEW;
    struct fight_msg fight;
    struct setstate_msg state;
    struct crash_msg crash;
    struct bactcollision_msg bcoll;
    struct recoil_msg rec;
    struct settarget_msg target;
    LONG   LWW, YLS;
    struct flt_triple old_dir;
    struct setposition_msg sp;
    struct assesstarget_msg at1, at2;
    ULONG  rat1, rat2, rec_count;


    /*
    ** Diverse Vorbereitungen
    */
    
    yfd = INST_DATA( cl, o );
    time = (FLOAT) msg->frame_time / 1000.0;

    betrag = nc_sqrt( yfd->bact->tar_vec.x * yfd->bact->tar_vec.x +
                      yfd->bact->tar_vec.y * yfd->bact->tar_vec.y +
                      yfd->bact->tar_vec.z * yfd->bact->tar_vec.z);
    
    if( betrag > 0.0 ) {
        
        yfd->bact->tar_unit.x = yfd->bact->tar_vec.x / betrag;
        yfd->bact->tar_unit.y = yfd->bact->tar_vec.y / betrag;
        yfd->bact->tar_unit.z = yfd->bact->tar_vec.z / betrag;
        }

    _get( o, YBA_Viewer,          &VIEW );
    _get( o, YBA_CheckVisExactly, &CHECKVISEXACTLY );
    _get( o, YBA_BactCollision,   &BACTCOLLISION );
    VIS = _methoda( yfd->world, YWM_ISVISIBLE, yfd->bact );
    if( (TARTYPE_NONE == yfd->bact->SecTargetType) &&
        (betrag       <  SECTOR_SIZE) )
        NEARPRIMTARGET = TRUE;
    else
        NEARPRIMTARGET = FALSE;

    if( VIEW ) {
        over_eof = yfd->bact->viewer_over_eof;
        radius   = yfd->bact->viewer_radius;
        }
    else {
        over_eof = yfd->bact->viewer_over_eof;
        radius   = yfd->bact->radius;
        }

    /*
    ** Wenn wir den Zustand Crash bis hierunter schleppen, dann ist die
    ** direkte Kollision wichtig. Deshalb werden wir nichts auslagen, sondern
    ** in den switch-cases lassen */

    switch( yfd->bact->MainState ) {

        case ACTION_NORMAL:

            /*** alte Geschwindigkeit merken ***/
            old_dir.x = yfd->bact->dir.m31;
            old_dir.y = yfd->bact->dir.m32;
            old_dir.z = yfd->bact->dir.m33;

            /*** Koll. mit Flugobjekt? (nur wenn sichtbar) ***/
            if( BACTCOLLISION &&
                (VIS || NEARPRIMTARGET) ) {
                bcoll.frame_time = msg->frame_time;
                _methoda( o, YBM_BACTCOLLISION, &bcoll);
                //    break;
                }


            /*
            ** Wenn wir kein Ziel haben, brauchen wir uns gar nicht weiter
            ** zu kümmern. Später evtl. tartype auf oldpos biegen oder so
            ** ähnlich... 
            */
            if( ( yfd->bact->PrimTargetType == TARTYPE_NONE ) &&
                ( yfd->bact->SecTargetType  == TARTYPE_NONE ) ) {

                /*** in ACTION_WAIT übergehen ***/
                yfd->bact->MainState = ACTION_WAIT;

                if( yfd->bact->ExtraState & EXTRA_FIRE ) {

                    state.extra_on  = state.main_state = 0;
                    state.extra_off = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }
                break;
                }

            /*** Nun geht es an die Kollisionsverhütung ***/

            bpos = &(yfd->bact->pos);
            bold = &(yfd->bact->old_pos);

            vorn.insect   = FALSE;
            links.insect  = FALSE;
            rechts.insect = FALSE;

            vorn.pnt.x = bold->x;
            vorn.pnt.y = bold->y;
            vorn.pnt.z = bold->z;

            vorn.vec.x = bpos->x - bold->x;
            vorn.vec.y = 0.0; // bpos->y - bold->y;
            vorn.vec.z = bpos->z - bold->z;

            betrag = nc_sqrt(vorn.vec.x * vorn.vec.x +
                             vorn.vec.y * vorn.vec.y +
                             vorn.vec.z * vorn.vec.z);

            if( betrag > 0.0 ) {
                vorn.vec.x *= 300 / betrag;
                vorn.vec.y *= 300 / betrag;
                vorn.vec.z *= 300 / betrag;
            } else {
                vorn.vec.x = yfd->bact->dir.m31 * 300;
                vorn.vec.y = yfd->bact->dir.m32 * 300;
                vorn.vec.z = yfd->bact->dir.m33 * 300;
            }

            /*** Intersect-Sachen vorsichtshalber löschen ***/
            vorn.insect   = FALSE;
            links.insect  = FALSE;
            rechts.insect = FALSE;

            vorn.flags    = 0;
            links.flags   = 0;
            rechts.flags  = 0;

            if( VIEW || (yfd->bact->ExtraState & EXTRA_RIGHT ) ||
                ( VIS && CHECKVISEXACTLY ) ) {

                links.pnt.x = bold->x;
                links.pnt.y = bold->y;
                links.pnt.z = bold->z;

                links.vec.x = 0.93969 * vorn.vec.x - 0.34202 * vorn.vec.z;
                links.vec.y = vorn.vec.y;
                links.vec.z = 0.93969 * vorn.vec.z + 0.34202 * vorn.vec.x;

                _methoda( yfd->world, YWM_INTERSECT, &links );
                }

            if( VIEW || (yfd->bact->ExtraState & EXTRA_LEFT ) ||
                ( VIS && CHECKVISEXACTLY ) ) {

                rechts.pnt.x = bold->x;
                rechts.pnt.y = bold->y;
                rechts.pnt.z = bold->z;

                rechts.vec.x = 0.93969 * vorn.vec.x + 0.34202 * vorn.vec.z;
                rechts.vec.y = vorn.vec.y;
                rechts.vec.z = 0.93969 * vorn.vec.z - 0.34202 * vorn.vec.x;

                _methoda( yfd->world, YWM_INTERSECT, &rechts );
                }

            if( VIEW || !(yfd->bact->ExtraState & (EXTRA_RIGHT|EXTRA_LEFT)) ||
                ( VIS && CHECKVISEXACTLY ) ) {

                _methoda( yfd->world, YWM_INTERSECT, &vorn );
                }

            rec_count = 0;
            rec.vec.x = rec.vec.y = rec.vec.z = 0.0;

            /*** Kollision nach vorn??? ***/
            if(vorn.insect) {

                /*** direkte Kollision? ***/
                if( (vorn.t * 300 ) < ( betrag + radius ) ) {
                    /*** ja, abprallen ***/
                    rec.vec.x += vorn.sklt->PlanePool[ vorn.pnum ].A;
                    rec.vec.y += vorn.sklt->PlanePool[ vorn.pnum ].B;
                    rec.vec.z += vorn.sklt->PlanePool[ vorn.pnum ].C;
                    rec_count++;
                    }
                }

            if(links.insect) {

                /*** direkte Kollision? ***/
                if( (links.t * 300 ) < ( betrag + radius ) ) {
                    /*** abprallen ***/
                    rec.vec.x += links.sklt->PlanePool[ links.pnum ].A;
                    rec.vec.y += links.sklt->PlanePool[ links.pnum ].B;
                    rec.vec.z += links.sklt->PlanePool[ links.pnum ].C;
                    rec_count++;
                    }
                }

            if(rechts.insect) {

                /*** direkte Kollision? ***/
                if( (rechts.t * 300 ) < ( betrag + radius ) ) {
                    /*** abprallen ***/
                    rec.vec.x += rechts.sklt->PlanePool[ rechts.pnum ].A;
                    rec.vec.y += rechts.sklt->PlanePool[ rechts.pnum ].B;
                    rec.vec.z += rechts.sklt->PlanePool[ rechts.pnum ].C;
                    rec_count++;
                    }
                }

            if( rec_count > 0 ) {

                /*** in rec stehen korrekte Werte, laßt uns abbprallen ***/
                rec.vec.x /= rec_count;
                rec.vec.y /= rec_count;
                rec.vec.z /= rec_count;
                rec.mul_v = 0.7;    rec.mul_y = 2.0;    rec.time = time;
                _methoda( o, YBM_RECOIL, &rec );

                /*** Das gibt Krach ***/
                //if( (!(yfd->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE )) &&
                //    (fabs( yfd->bact->dof.v ) > NOISE_SPEED) &&
                //    (!(yfd->bact->ExtraState & EXTRA_LANDCRASH)) ) {
                //
                //    yfd->bact->ExtraState |= EXTRA_LANDCRASH;
                //    _StartSoundSource( &(yfd->bact->sc), VP_NOISE_CRASHLAND );
                //    }
                }
            else yfd->bact->ExtraState &= ~EXTRA_LANDCRASH;


            if( !vorn.insect && !links.insect && !rechts.insect) {

                yfd->bact->ExtraState &= ~(EXTRA_LEFT|EXTRA_RIGHT);
                yfd->bact->ExtraState |= EXTRA_MOVE;
            }

            if ((yfd->bact->ExtraState & (EXTRA_LEFT|EXTRA_RIGHT)) == 0) {

                if ((links.insect == TRUE) && (rechts.insect == TRUE)) {
                    if (links.t < rechts.t) yfd->bact->ExtraState |= EXTRA_RIGHT;
                    else                    yfd->bact->ExtraState |= EXTRA_LEFT;
                };

                if ((links.insect == TRUE) && (rechts.insect == FALSE)) {
                    yfd->bact->ExtraState |= EXTRA_RIGHT;
                };

                if ((links.insect == FALSE) && (rechts.insect == TRUE)) {
                    yfd->bact->ExtraState |= EXTRA_LEFT;
                };

                if ((links.insect == FALSE) && (rechts.insect == FALSE) &&
                    (vorn.insect == TRUE))
                {
                    yfd->bact->ExtraState |= EXTRA_LEFT;
                };
            };

            /*** Jetzt kommt noch ein Höhentest ***/
            hoehe.pnt.x = bold->x;
            hoehe.pnt.y = bold->y;
            hoehe.pnt.z = bold->z;

            max_speed  = max( yfd->bact->mass * GRAVITY, yfd->bact->max_force );
            max_speed /= max( 10.0, yfd->bact->air_const );

            hoehe.vec.x = yfd->bact->dof.x * 200 * yfd->bact->dof.v / max_speed;
            if( hoehe.vec.x < -200.0 ) hoehe.vec.x = -200.0;
            if( hoehe.vec.x >  200.0 ) hoehe.vec.x =  200.0;
            hoehe.vec.y = yfd->bact->pref_height;
            hoehe.vec.z = yfd->bact->dof.z * 200 * yfd->bact->dof.v / max_speed;
            if( hoehe.vec.z < -200.0 ) hoehe.vec.z = -200.0;
            if( hoehe.vec.z >  200.0 ) hoehe.vec.z =  200.0;
            hoehe.flags = 0;

            _methoda( yfd->world, YWM_INTERSECT, &hoehe );
            if( hoehe.insect )
                yfd->bact->tar_unit.y = -( 1.0 - hoehe.t );
            else {

                /* ----------------------------------------------------
                ** Eigentlich müssen wir runter, wenn wir zu hoch sind,
                ** aber da kann es ja noch sein, daß wir gerade den
                ** User als Ziel haben, der ungehindert nach oben ab-
                ** hauen kann. Dann korrigieren wir nicht.
                ** --------------------------------------------------*/
                Object *user;
                _get( yfd->world, YWA_UserVehicle, &user );

                if( !( (((TARTYPE_BACTERIUM == yfd->bact->SecTargetType) &&
                        ((yfd->bact->SecondaryTarget.Bact->BactClassID == BCLID_YPAROBO) ||
                         (yfd->bact->SecondaryTarget.Bact->BactObject == user))) ||

                        ((TARTYPE_BACTERIUM == yfd->bact->PrimTargetType) &&
                        ((yfd->bact->PrimaryTarget.Bact->BactClassID == BCLID_YPAROBO) ||
                         (yfd->bact->PrimaryTarget.Bact->BactObject == user))) ) &&

                         (yfd->bact->tar_unit.y < -0.01) ) )
                    yfd->bact->tar_unit.y = max( 0.15, yfd->bact->tar_unit.y);
                }

            /* -----------------------------------------------------------
            ** Wenn wir gerade ausweichen, auch nach oben. Trotzdem Höhen-
            ** test wegen abprallen
            ** ---------------------------------------------------------*/
            if( yfd->bact->ExtraState & (EXTRA_LEFT | EXTRA_RIGHT) )
                yfd->bact->tar_unit.y = -0.2;
            
            if( hoehe.insect &&
                ((hoehe.t * yfd->bact->pref_height) < yfd->bact->radius) &&
                (yfd->bact->dof.y > 0.0) ) {

                /*** abprallen ***/
                rec.vec.x = hoehe.sklt->PlanePool[ hoehe.pnum ].A;
                rec.vec.y = hoehe.sklt->PlanePool[ hoehe.pnum ].B;
                rec.vec.z = hoehe.sklt->PlanePool[ hoehe.pnum ].C;
                rec.mul_v = 0.7;    rec.mul_y = 2.0;    rec.time = time;
                _methoda( o, YBM_RECOIL, &rec );
                
                /*** Das gibt Krach ***/
                //if( (!(yfd->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE )) &&
                //    (fabs( yfd->bact->dof.v ) > NOISE_SPEED) &&
                //    (!(yfd->bact->ExtraState & EXTRA_LANDCRASH)) ) {
                //
                //    yfd->bact->ExtraState |= EXTRA_LANDCRASH;
                //    _StartSoundSource( &(yfd->bact->sc), VP_NOISE_CRASHLAND );
                //    }
                }
            else yfd->bact->ExtraState &= ~EXTRA_LANDCRASH;



            /*** nachnormieren, ist einfacher als drehen ***/
            if( yfd->bact->tar_unit.y != 0.0 ) {

                FLOAT b;
                struct flt_triple *tv = &(yfd->bact->tar_unit);

                b = nc_sqrt( tv->x*tv->x + tv->y*tv->y + tv->z*tv->z);
                tv->x /= b; tv->y /= b;  tv->z /= b;
                }


            /*** Die Bewegung ***/
            if( yfd->bact->ExtraState & EXTRA_LEFT ) {

                /*** den tar_unit nach links drehen ***/
                betrag = nc_sqrt( vorn.vec.x*vorn.vec.x + vorn.vec.z*vorn.vec.z );
                yfd->bact->tar_unit.x = -vorn.vec.z / betrag;
                yfd->bact->tar_unit.z =  vorn.vec.x / betrag;

            } else if( yfd->bact->ExtraState & EXTRA_RIGHT ) {

                /*** den tar_unit nach rechts drehen  ***/
                betrag = nc_sqrt( vorn.vec.x*vorn.vec.x + vorn.vec.z*vorn.vec.z );
                yfd->bact->tar_unit.x =  vorn.vec.z / betrag;
                yfd->bact->tar_unit.z = -vorn.vec.x / betrag;

            };
            

            /*
            ** Nun haben wir in tar_unit evtl. eine Richtung. Ist dem so,
            ** bewegen wir uns dorthin. Hubschrauber bewegen sich immer, 
            ** weil sie sich nicht einfach wie Panzer drehen können! 
            */

            yf_CalculateForce( yfd, time );

            move.flags = 0;
            move.t     = (FLOAT)msg->frame_time/1000;
            _methoda(yfd->bact->BactObject, YBM_MOVE, &move);

            /*
            ** Wir haben die neue position und die neue Geschwindigkeit.
            ** Nun drehen den Hubschrauber noch in Flugrichtung 
            */

            yf_DrehObjektInFlugrichtung( yfd, time, &old_dir, FALSE );


            /*
            ** Nun geht es um die Zielbearbeitung, so wir welche haben.
            ** Wichtig ist zuerst das Nebenziel. Wenn wir eins haben,
            ** bekämpfen wir es. Dazu rufen wir die Methoden zur Sektor-
            ** bekämpfung bzw. zur Bakterienbekämpfung auf, denn nur diese
            ** kennen die Waffen und können uber Schußweite und Streuung
            ** entscheiden 
            */

            fight.time = time;
            fight.global_time = yfd->bact->internal_time;
            if( yfd->bact->SecTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                fight.enemy.bact = yfd->bact->SecondaryTarget.Bact;
                fight.priority   = 1;
                _methoda(o, YBM_FIGHTBACT, &fight );
                }
            else
                if( yfd->bact->SecTargetType == (UBYTE)TARTYPE_SECTOR ) {
                    fight.pos          = yfd->bact->SecPos;
                    fight.enemy.sector = yfd->bact->SecondaryTarget.Sector;
                    fight.priority     = 1;
                    _methoda(o, YBM_FIGHTSECTOR, &fight );
                    }
                else
                    if( yfd->bact->PrimTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                        fight.enemy.bact = yfd->bact->PrimaryTarget.Bact;
                        fight.priority   = 0;
                        _methoda(o, YBM_FIGHTBACT, &fight );
                        }
                    else
                        if( yfd->bact->PrimTargetType == (UBYTE)TARTYPE_SECTOR ){
                            fight.pos          = yfd->bact->PrimPos;
                            fight.enemy.sector = yfd->bact->PrimaryTarget.Sector;
                            fight.priority     = 0;
                            _methoda(o, YBM_FIGHTSECTOR, &fight );
                            }
                        else {

                            /* --------------------------------------------------
                            ** was anderes bekämpfen wir nicht. Es kann aber noch
                            ** sein, daß wir noch feuern 
                            ** ------------------------------------------------*/
                            yfd->bact->ExtraState &= ~(EXTRA_FIGHTP | EXTRA_FIGHTS);
                            if( yfd->bact->ExtraState & EXTRA_FIRE ) {

                                state.extra_on  = state.main_state = 0;
                                state.extra_off = EXTRA_FIRE;
                                _methoda( o, YBM_SETSTATE, &state );
                                }
                            }

            break;

        /*--------------------------------------------------------------------*/

        case ACTION_DEAD:

            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;

        /*--------------------------------------------------------------------*/

        case ACTION_WAIT:

            /* -----------------------------------------------------------------
            ** Ich bin wartend geschalten worden, weil ich kein Ziel mehr habe
            ** oder das HZ erreicht habe. Wenn ich das HZ erreicht habe, behalte
            ** ich es natülich, damit ich nach einer Ablenkung zurückkehren
            ** kann. Um mich aus dem Wartezustand zu bringen, muß ich also ein
            ** NZ bekommen oder ein HZ, welches kein Sektor ist oder ein
            ** Sektor, der außerhalb ist. 
            ** ---------------------------------------------------------------*/

            if( ( yfd->bact->internal_time - yfd->bact->newtarget_time) > 500 ) {
            
                /*** Kann ich mir das leisten? ***/
                at1.target_type = yfd->bact->SecTargetType;
                at1.priority    = 1;
                at2.target_type = yfd->bact->PrimTargetType;
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
                        target.pos.x       = yfd->bact->pos.x;
                        target.pos.z       = yfd->bact->pos.z;
                        target.priority    = 0;
                        _methoda( o, YBM_SETTARGET, &target );
                        }

                    /*** Ist noch was übriggeblieben? ***/
                    if( (yfd->bact->PrimTargetType != TARTYPE_NONE) ||
                        (yfd->bact->SecTargetType  != TARTYPE_NONE) ) {

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
            
            /*** Als Gleiter landen wir (vielleicht auch nicht) ***/
            _get( o, YBA_LandWhileWait, &LWW );
            if( LWW ) {

                yfd->bact->act_force = 0;
                yfd->buoyancy        = 0;
                if( !(yfd->bact->ExtraState & EXTRA_LANDED) ) {
                    
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
                    inter.pnt.x = yfd->bact->pos.x;
                    inter.pnt.y = yfd->bact->pos.y;
                    inter.pnt.z = yfd->bact->pos.z;
                    inter.vec.x = 0.0;
                    inter.vec.y = yfd->bact->over_eof + 50;
                    inter.vec.z = 0.0;
                    inter.flags = 0;
                    _methoda( yfd->world, YWM_INTERSECT, &inter );
                    if( inter.insect ) yfd->bact->pos.y = inter.ipnt.y -
                                                          yfd->bact->over_eof;
                    }
                }
            break;

        /*--------------------------------------------------------------------*/

        case ACTION_CREATE:

            /*** Zeuch machen ***/
            _methoda( o, YBM_DOWHILECREATE, msg );

            break;

        /*--------------------------------------------------------------------*/

        case ACTION_BEAM:

            _methoda( o, YBM_DOWHILEBEAM, msg );
            break;
        }

}

/*-----------------------------------------------------------------*/
_dispatcher(void, yf_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**
**
**  INPUTS
**
**  RESULTS
**
**
**  CHANGED
**      3-Nov-95   8100 000C    created
*/
{
    FLOAT  x_rad, y_rad;
    struct move_msg move;
    struct ypaflyer_data *yfd = INST_DATA(cl,o);
    struct insphere_msg ins;
    struct intersect_msg inter;
    struct flt_triple vec;
    FLOAT  betrag, A,B,C, norm_height;
    struct Insect insect[10];
    FLOAT  bsp, time;
    ULONG  BACTCOLL, maxcollisionchecks;
    struct firemissile_msg fire;
    struct setstate_msg state;
    struct crash_msg crash;
    struct bactcollision_msg bcoll;
    struct recoil_msg rec;
    struct flt_triple old_dir;
    struct fireyourguns_msg fyg;
    struct visier_msg visier;
    BOOL   coll;


    yfd->bact->air_const = yfd->bact->bu_air_const;

    time = (FLOAT) msg->frame_time / 1000;
    _get( o, YBA_BactCollision, &BACTCOLL );

    switch( yfd->bact->MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:

            
            /*** alte Richtung ***/
            old_dir.x = yfd->bact->dir.m31;
            old_dir.y = yfd->bact->dir.m32;
            old_dir.z = yfd->bact->dir.m33;

            /*** Trägheitsfaktoren für Handsteuerungen ***/
            x_rad = -msg->input->Slider[ SL_FLY_DIR ] *
                     yfd->bact->max_rot * time;

            y_rad = msg->input->Slider[ SL_FLY_HEIGHT ] *
                    yfd->bact->max_rot * time;
            
            
            /*** direkte Kollision ***/
            if( BACTCOLL && (!(yfd->bact->ExtraState & EXTRA_LANDED)) ) {

                bcoll.frame_time = msg->frame_time;
                _methoda( o, YBM_BACTCOLLISION, &bcoll );
                }

            if( yfd->bact->ExtraState & EXTRA_LANDED )
                bsp = IS_SPEED;
            else
                bsp = 10 * IS_SPEED;

            /*** Untergebene ausbremsen ***/
            if( yfd->bact->dof.v < bsp ) {
                
                /*** Optisch wait nur, wenn wir gelandet sind ***/
                inter.pnt.x = yfd->bact->pos.x;
                inter.pnt.y = yfd->bact->pos.y;
                inter.pnt.z = yfd->bact->pos.z;
                inter.vec.x = inter.vec.z = 0.0;
                inter.vec.y = 1.5 * max( yfd->bact->viewer_over_eof,
                                         yfd->bact->viewer_radius );
                inter.flags = 0;
                _methoda( yfd->world, YWM_INTERSECT, &inter );
                if( inter.insect &&
                    (yfd->bact->act_force < 0.001 * yfd->bact->max_force) &&
                    (yfd->buoyancy <= yfd->bact->mass * GRAVITY) ) {
                
                    yfd->bact->ExtraState |= EXTRA_LANDED;
                    yfd->bact->dof.v       = 0;
                    yfd->buoyancy          = yfd->bact->mass * GRAVITY;
                    yfd->bact->act_force   = 0;
                    }
                else
                    yfd->bact->ExtraState &= ~EXTRA_LANDED;

                /*** Wenn wir in Zielnähe sind, warten wir nur "scheinbar" ! ***/
                if( (yfd->bact->PrimTargetType == TARTYPE_SECTOR) &&
                    (nc_sqrt( (yfd->bact->PrimPos.x - yfd->bact->pos.x ) * 
                              (yfd->bact->PrimPos.x - yfd->bact->pos.x ) +
                              (yfd->bact->PrimPos.z - yfd->bact->pos.z ) * 
                              (yfd->bact->PrimPos.z - yfd->bact->pos.z ) ) >
                             (2 * FOLLOW_DISTANCE)) ) {
                    
                    /*** weit weg ***/
                    yfd->bact->MainState = ACTION_WAIT;

                    if( (yfd->bact->ExtraState & EXTRA_LANDED) &&
                        (!(yfd->bact->ExtraState & EXTRA_FIRE)) ) {
                        
                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                else {

                    /*** Am Kampfort ***/
                    if( (yfd->bact->ExtraState & EXTRA_LANDED) &&
                        (!(yfd->bact->ExtraState & EXTRA_FIRE)) ) {

                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    yfd->bact->MainState = ACTION_NORMAL;    // !!
                    }
                }
            else {

                if( !(yfd->bact->ExtraState & EXTRA_FIRE) ) {

                    state.main_state = ACTION_NORMAL;
                    state.extra_on = state.extra_off = 0;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                /*** Landed ausschalten ***/
                yfd->bact->ExtraState &= ~EXTRA_LANDED;
                }


            /* ----------------------------------------------------
            ** Nach den Vorbetrachtungen bzgl. Trägheit etc. kommen
            ** nun die ganzen Steuersachen
            ** --------------------------------------------------*/
            

            /*** Seitendrehung ***/
            yf_rot_round_global_y( yfd->bact, x_rad );
                


            /*** Speedkorrektur ***/
            yfd->bact->act_force += 0.3 * time * yfd->bact->max_force *
                                    msg->input->Slider[ SL_FLY_SPEED ];

            if( yfd->bact->act_force > yfd->bact->max_force )
                yfd->bact->act_force = yfd->bact->max_force;
            if( yfd->bact->act_force < 0.0 )
                yfd->bact->act_force = 0.0;


            /* -------------------------------------------------------------
            ** Rauf oder runter (speedabhängig), Erazer ist Vergleichspunkt.
            ** bei vmax = 111 ist b = f/2. Somit muß v/111 = 1 sein.
            ** Drehe ich den Zähler hoch, muß ich auch den Nenner verändern.
            ** Wichtig ist aber die Kraft. Also legen wir es darauf fest!!!
            ** An Stelle von max_force schreiben wir
            ** -----------------------------------------------------------*/
            yfd->buoyancy = yfd->bact->mass * GRAVITY +
                            msg->input->Slider[ SL_FLY_HEIGHT ] *
                            20000 *
                            ( 1 + fabs( yfd->bact->dof.v ) / 111 )/2;


            /*** Maximalhöhenkorrektur ***/
            norm_height = yfd->bact->Sector->Height - yfd->bact->pos.y;
            if( yfd->buoyancy > ( 7 * yfd->bact->mass * GRAVITY *
                                  (1 - norm_height * norm_height /
                                   yfd->bact->max_user_height /
                                   yfd->bact->max_user_height) ) )
                yfd->buoyancy = 7 * yfd->bact->mass * GRAVITY *
                                  (1 - norm_height * norm_height /
                                   yfd->bact->max_user_height /
                                   yfd->bact->max_user_height);

            
            /* --------------------------------------------------------------
            ** Jetzt kommen die ganzen Waffensachen, wie Ein- und Ausschalten
            ** des Visiers und die verschiedenen Arten des Rumplatzens
            ** ------------------------------------------------------------*/

            /*** Zielaufnahme, die machen wir immer (vorher default) ***/
            fire.target_type  = TARTYPE_SIMPLE;
            fire.target_pos.x = yfd->bact->dir.m31;
            fire.target_pos.y = yfd->bact->dir.m32;
            fire.target_pos.z = yfd->bact->dir.m33;

            visier.flags = VISIER_ENEMY | VISIER_NEUTRAL;
            visier.dir.x = yfd->bact->dir.m31;
            visier.dir.y = yfd->bact->dir.m32;
            visier.dir.z = yfd->bact->dir.m33;
            if( _methoda( o, YBM_VISIER, &visier ) ) {

                fire.target_type = TARTYPE_BACTERIUM;
                fire.target.bact = visier.enemy;
                }


            /*** Schießen Rakete ***/
            if( (msg->input->Buttons & BT_FIRE ) ||
                (msg->input->Buttons & BT_FIREVIEW ) ) {

                fire.dir.x       = fire.dir.y = fire.dir.z = 0.0;
                fire.wtype       = yfd->bact->auto_ID;
                fire.global_time = yfd->bact->internal_time;
                /*** Hubschrauber. etwas runter + Seite ***/
                if( yfd->bact->internal_time % 2 == 0 )
                    fire.start.x = -yfd->bact->firepos.x;
                else
                    fire.start.x = yfd->bact->firepos.x;
                fire.start.y     = yfd->bact->firepos.y;
                fire.start.z     = yfd->bact->firepos.z;
                
                /*** Viewer oder nicht? ***/
                if( msg->input->Buttons & BT_FIREVIEW )
                    fire.flags   = FIRE_VIEW;
                else
                    fire.flags   = 0;
                fire.flags      |= FIRE_CORRECT;
                _methoda( o, YBM_FIREMISSILE, &fire );
                }

            /*** Schießen, Kanone ***/
            if( yfd->bact->mg_shot != NO_MACHINEGUN ) {

                if( (yfd->bact->ExtraState & EXTRA_FIRE) &&
                    (!(msg->input->Buttons & BT_FIREGUN)) ) {

                    /*** Feuern aus ***/
                    // Abmelden beim WO

                    state.main_state = state.extra_on = 0;
                    state.extra_off  = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                if( msg->input->Buttons & BT_FIREGUN ) {

                    /*** Feuern einschalten? ***/
                    if( !(yfd->bact->ExtraState & EXTRA_FIRE) ) {

                        // Anmelden WO

                        state.main_state = state.extra_off = 0;
                        state.extra_on   = EXTRA_FIRE;
                        _methoda( o, YBM_SETSTATE, &state );
                        }

                    /*** Feuern an sich ***/
                    fyg.dir.x = yfd->bact->dir.m31;
                    fyg.dir.y = yfd->bact->dir.m32;
                    fyg.dir.z = yfd->bact->dir.m33;
                    fyg.time  = time;
                    fyg.global_time = yfd->bact->internal_time;
                    _methoda( o, YBM_FIREYOURGUNS, &fyg );
                    }
                }


            /*** Ausrichten ***/
            if( msg->input->Buttons & BT_STOP )
                _methoda( o, YBM_STOPMACHINE, msg );

            /*** Kollisionstests u.ä. nur, wenn fliegend ***/
            if( !(yfd->bact->ExtraState & EXTRA_LANDED )) {
                

                maxcollisionchecks = 4;
                while( maxcollisionchecks-- ) {

                    coll = FALSE;

                    /*** nun an Move übergeben ***/
                    move.t     = time;
                    move.flags = 0;
                    _methoda(yfd->bact->BactObject, YBM_MOVE, &move);
                    

                    /* -------------------------------------------------------------
                    ** Nun die Kollisionsbearbeitung. Wir machen eine Kugelkollision
                    ** und dann trotz alledem noch eine Intersection von old_pos
                    ** auf pos.
                    ** -----------------------------------------------------------*/
                    
                    /*** Kugel-Kollisionstest ***/
                    ins.pnt.x       = yfd->bact->pos.x + yfd->bact->dir.m31 * 0.5 * yfd->bact->viewer_radius;
                    ins.pnt.y       = yfd->bact->pos.y + yfd->bact->dir.m32 * 0.5 * yfd->bact->viewer_radius;
                    ins.pnt.z       = yfd->bact->pos.z + yfd->bact->dir.m33 * 0.5 * yfd->bact->viewer_radius;
                    ins.dof.x       = yfd->bact->dof.x;
                    ins.dof.y       = yfd->bact->dof.y;
                    ins.dof.z       = yfd->bact->dof.z;
                    ins.radius      = yfd->bact->viewer_radius;
                    ins.chain       = insect;
                    ins.max_insects = 10;
                    ins.flags       = 0;

                    _methoda( yfd->world, YWM_INSPHERE, &ins );

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
                            vec.x = yfd->bact->dof.x;
                            vec.y = yfd->bact->dof.y;
                            vec.z = yfd->bact->dof.z;
                            }
                        else {

                            vec.x = A / betrag;
                            vec.y = B / betrag;
                            vec.z = C / betrag;
                            }

                        coll      = TRUE;
                        rec.vec.x = vec.x;      rec.vec.y = vec.y;
                        rec.vec.z = vec.z;      rec.mul_y = 2.0;
                        rec.mul_v = 0.7;        rec.time = time;
                        _methoda( o, YBM_RECOIL, &rec);

                        /*** Test ***/
                        yfd->buoyancy = yfd->bact->mass * GRAVITY;
                        }

                    /*** Intersection ***/
                    if( !coll ) {

                        inter.pnt.x = yfd->bact->old_pos.x;
                        inter.pnt.y = yfd->bact->old_pos.y;
                        inter.pnt.z = yfd->bact->old_pos.z;
                        inter.vec.x = yfd->bact->pos.x - yfd->bact->old_pos.x;
                        inter.vec.y = yfd->bact->pos.y - yfd->bact->old_pos.y;
                        inter.vec.z = yfd->bact->pos.z - yfd->bact->old_pos.z;
                        inter.flags = 0;

                        /*** Bei Luftfahrzeugen I2, auch wegen Impulsen! ***/
                        _methoda( yfd->world, YWM_INTERSECT2, &inter );
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

                        yfd->bact->ExtraState &= ~EXTRA_LANDCRASH;
                        break;
                        }
                    else {

                        /*** Knallgeräusch ***/
                        if( (!(yfd->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE )) &&
                            (!(yfd->bact->ExtraState & EXTRA_LANDCRASH )) ) {

                            struct yw_forcefeedback_msg ffb;

                            yfd->bact->ExtraState |= EXTRA_LANDCRASH;
                            _StartSoundSource( &(yfd->bact->sc), VP_NOISE_CRASHLAND );

                            ffb.type    = YW_FORCE_COLLISSION;
                            ffb.power   = 1.0;

                            /*** Trick: Pos + Richtung ***/
                            ffb.dir_x   = yfd->bact->pos.x + 10 * A;
                            ffb.dir_y   = yfd->bact->pos.z + 10 * C;
                            _methoda( yfd->world, YWM_FORCEFEEDBACK, &ffb );
                            }

                        /*** Kick nach oben ***/
                        yfd->bact->dof.y -= 0.2;
                        }
                    }

                /*** Noch ausrichten ***/
                yf_DrehObjektInFlugrichtung( yfd, time, &old_dir, TRUE );
                }
            else {

                /*** Gelanded, trotzdem bewegen um wieder loszukommen ***/
                move.t     = time;
                move.flags = 0;
                _methoda(yfd->bact->BactObject, YBM_MOVE, &move);
                }

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


_dispatcher( void, yf_YBM_REINCARNATE, void *nix )
{
    struct ypaflyer_data *yfd;
    yfd = INST_DATA( cl, o );

    /*** nach oben ***/
    _supermethoda(cl, o, YBM_REINCARNATE, NULL );

    /*** Voreinstellungen und / oder Standardwerte ***/
    yfd->flight_type    = 0;
    yfd->buoyancy       = 0.0;

}

