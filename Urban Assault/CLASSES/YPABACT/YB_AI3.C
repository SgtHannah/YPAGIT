/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**  zweiter Teil mit den Helicopterspezifischen Sachen
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
#include "input/input.h"
#include "audio/audioengine.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine



/*-----------------------------------------------------------------*/
void yb_m_mul_m(struct flt_m3x3 *m1, struct flt_m3x3 *m2, struct flt_m3x3 *m);
void yb_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle);
void yb_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yb_rot_round_lokal_y2( struct flt_m3x3 *dir,  FLOAT angle);
void yb_DrehObjektInFlugrichtung( struct ypabact_data *ybd, ULONG t );
void yb_GiveFormationPos(struct Bacterium *me,struct flt_triple *rel,ULONG count);
void yb_CalculateForce( struct ypabact_data *ybd, struct trigger_logic_msg *msg );
void yb_GiveSecTarget( struct get_sectar_msg *st, Object *world);
BOOL yb_BactCollision(struct ypabact_data *ybd, FLOAT time );
_dispatcher(void, yb_YBM_DIE, void *nix);
struct Bacterium *yb_GetEnemy( struct Cell *sector, struct Bacterium *last, UBYTE owner );



/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION
**
**      Teil 3 der Intelligencekette. Der Zielvektor zeigt in eine
**      Richtung, in die wir wollen. Was das ist, interessiert nicht.
**      Hier geht es darum, nachzusehen, ob der Weg frei ist und evtl.
**      die Flughöhe zu korrigieren. Die Kollisionsvermeidung ist
**      primär vor allem anderen, das heißt auch, daß wir im Falle einer
**      zu erwartenden Kollision das Ziel vollkommen vergessen.
**      Die Flughöhenkorektur erfolgt nur im Status "Fliegen", weil
**      sie sonst störend sein kann. Die Flughöhe bezieht sich
**      auf die Sektorhöhe (logisch!)
**      Wir ermitteln den für die Flugrichtung notwendigen Kraftvektor
**      und richten das Objekt danach aus. Achtung! AI3 ist Flugobjekt-
**      spezifisch und richtet bereits das Objekt aus. Move realisiert
**      dann nur die Bewegung auf Grundlage des von AI3 vorgegeben
**      Kraftmodells.
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
**      28-Jun-95   8100000C    created
**       8-Jul-95   8100000C    Kollisionsverhütung
**      12-Jul-95   8100000C    Kollisionssachen ausgelagert in Methoden
**      19-Jul-95   8100000C    vereinfacht, Zeit
*/

    FLOAT  betrag, radius, over_eof, weg;
    FLOAT  max_speed, time;
    struct move_msg move;
    struct flt_triple *bold, *bpos;
    struct intersect_msg vorn, rechts, links, hoehe;
    BOOL   CHECKVISEXACTLY, VIEW, NEARPRIMTARGET;
    ULONG  VIS, recoil_count;
    struct fight_msg fight;
    struct setstate_msg state;
    struct crash_msg crash;
    struct bactcollision_msg bcoll;
    struct recoil_msg rec;
    struct settarget_msg target;
    struct assesstarget_msg at1, at2;
    ULONG  rat1, rat2;


    /*
    ** Diverse Vorbereitungen
    */
    
    struct ypabact_data *ybd = INST_DATA( cl, o );
    time = (FLOAT) msg->frame_time / 1000.0;

    weg = nc_sqrt( ybd->bact.tar_vec.x * ybd->bact.tar_vec.x +
                   ybd->bact.tar_vec.y * ybd->bact.tar_vec.y +
                   ybd->bact.tar_vec.z * ybd->bact.tar_vec.z);
    
    if( weg > 0.0 ) {
        
        ybd->bact.tar_unit.x = ybd->bact.tar_vec.x / weg;
        ybd->bact.tar_unit.y = ybd->bact.tar_vec.y / weg;
        ybd->bact.tar_unit.z = ybd->bact.tar_vec.z / weg;
        }

    VIEW            = ybd->flags & YBF_Viewer;
    CHECKVISEXACTLY = ybd->flags & YBF_CheckVisExactly;
    VIS             = _methoda( ybd->world, YWM_ISVISIBLE, &(ybd->bact) );
    if( (TARTYPE_NONE == ybd->bact.SecTargetType) &&
        (weg          <  SECTOR_SIZE) )
        NEARPRIMTARGET = TRUE;
    else
        NEARPRIMTARGET = FALSE;

    if( VIEW ) {
        over_eof = ybd->bact.viewer_over_eof;
        radius   = ybd->bact.viewer_radius;
        }
    else {
        over_eof = ybd->bact.viewer_over_eof;
        radius   = ybd->bact.radius;
        }


    /*
    ** Wenn wir den Zustand Crash bis hierunter schleppen, dann ist die
    ** direkte Kollision wichtig. Deshalb werden wir nichts auslagen, sondern
    ** in den switch-cases lassen 
    */

    switch( ybd->bact.MainState ) {

        case ACTION_NORMAL:
            
            /*** Koll. mit Flugobjekt? (nur im sichtbaren Bereich) ***/
            if( (ybd->flags & YBF_BactCollision) &&
                (VIS || NEARPRIMTARGET) &&
                (!(ybd->bact.ExtraState & EXTRA_LANDED)) ) {
                
                bcoll.frame_time = msg->frame_time;
                _methoda( o, YBM_BACTCOLLISION, &bcoll);
                }

            /* Wenn wir kein Ziel haben, brauchen wir uns gar nicht weiter 
            ** zu kümmern. Später evtl. tartype auf oldpos biegen oder so
            ** ähnlich... */
            if( ( ybd->bact.PrimTargetType == TARTYPE_NONE ) &&
                ( ybd->bact.SecTargetType  == TARTYPE_NONE ) ) {

                /*** in ACTION_WAIT übergehen ***/
                ybd->bact.MainState = ACTION_WAIT;
                
                if( ybd->bact.ExtraState & EXTRA_FIRE ) {

                    state.extra_on  = state.main_state = 0;
                    state.extra_off = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }
                break;
                }

            /*** Nun geht es an die Kollisionsverhütung ***/

            bpos = &(ybd->bact.pos);
            bold = &(ybd->bact.old_pos);

            vorn.insect   = FALSE;
            links.insect  = FALSE;
            rechts.insect = FALSE;

            vorn.pnt.x = bold->x;
            vorn.pnt.y = bold->y;
            vorn.pnt.z = bold->z;

            vorn.vec.x = bpos->x - bold->x;
            vorn.vec.y = 0.0; /* bpos->y - bold->y; */
            vorn.vec.z = bpos->z - bold->z;

            betrag = nc_sqrt(vorn.vec.x * vorn.vec.x +
                             vorn.vec.y * vorn.vec.y +
                             vorn.vec.z * vorn.vec.z);


            if( betrag > 0.0 ) {
                vorn.vec.x *= 300 / betrag;
                vorn.vec.y *= 300 / betrag;
                vorn.vec.z *= 300 / betrag;
            } else {
                vorn.vec.x = ybd->bact.dir.m31 * 300;
                vorn.vec.y = ybd->bact.dir.m32 * 300;
                vorn.vec.z = ybd->bact.dir.m33 * 300;
            }

            /*
            ** Weil wir nicht alles testen, setzen wir die Intersections
            ** erstmal auf falsch!
            */
            vorn.insect   = FALSE;
            rechts.insect = FALSE;
            links.insect  = FALSE;
            vorn.flags    = 0;
            rechts.flags  = 0;
            links.flags   = 0;


            /* ------------------------------------------------------------
            ** Nun die Tests. Es können Slurpprobleme auftreten, die hier
            ** aber keine zu großen Auswirkungen haben. Machen wir es nicht
            ** komplizierter als notwendig....
            ** ----------------------------------------------------------*/
            if( VIEW || (ybd->bact.ExtraState & EXTRA_RIGHT ) ||
                ( VIS && CHECKVISEXACTLY ) ) {

                links.pnt.x = bold->x;
                links.pnt.y = bold->y;
                links.pnt.z = bold->z;

                links.vec.x = 0.93969 * vorn.vec.x - 0.34202 * vorn.vec.z;
                links.vec.y = vorn.vec.y;
                links.vec.z = 0.93969 * vorn.vec.z + 0.34202 * vorn.vec.x;

                _methoda( ybd->world, YWM_INTERSECT, &links );
                }

            if( VIEW || (ybd->bact.ExtraState & EXTRA_LEFT ) ||
                ( VIS && CHECKVISEXACTLY ) ) {

                rechts.pnt.x = bold->x;
                rechts.pnt.y = bold->y;
                rechts.pnt.z = bold->z;

                rechts.vec.x = 0.93969 * vorn.vec.x + 0.34202 * vorn.vec.z;
                rechts.vec.y = vorn.vec.y;
                rechts.vec.z = 0.93969 * vorn.vec.z - 0.34202 * vorn.vec.x;

                _methoda( ybd->world, YWM_INTERSECT, &rechts );
                }

            if( VIEW || !(ybd->bact.ExtraState & (EXTRA_RIGHT|EXTRA_LEFT)) ||
                ( VIS && CHECKVISEXACTLY ) ) {

                _methoda( ybd->world, YWM_INTERSECT, &vorn );
                }

            rec.vec.x    = rec.vec.y = rec.vec.z = 0.0;
            recoil_count = 0;

            /*** Kollision nach vorn??? ***/
            if(vorn.insect) {

                /*** direkte Kollision? ***/
                if( (vorn.t * 300 ) < ( betrag + radius ) ) {
                    /*** abprallen ***/
                    rec.vec.x += vorn.sklt->PlanePool[ vorn.pnum ].A;
                    rec.vec.y += vorn.sklt->PlanePool[ vorn.pnum ].B;
                    rec.vec.z += vorn.sklt->PlanePool[ vorn.pnum ].C;
                    recoil_count++;
                    }
                }

            /*** Links? ***/
            if(links.insect) {

                /*** direkte Kollision? ***/
                if( (links.t * 300 ) < ( betrag + radius ) ) {
                    /*** abprallen ***/
                    rec.vec.x += links.sklt->PlanePool[ links.pnum ].A;
                    rec.vec.y += links.sklt->PlanePool[ links.pnum ].B;
                    rec.vec.z += links.sklt->PlanePool[ links.pnum ].C;
                    recoil_count++;
                    }
                }

            /*** Rechts? ***/
            if(rechts.insect) {

                /*** direkte Kollision? ***/
                if( (rechts.t * 300 ) < ( betrag + radius ) ) {
                    /*** abprallen ***/
                    rec.vec.x += rechts.sklt->PlanePool[ rechts.pnum ].A;
                    rec.vec.y += rechts.sklt->PlanePool[ rechts.pnum ].B;
                    rec.vec.z += rechts.sklt->PlanePool[ rechts.pnum ].C;
                    recoil_count++;
                    }
                }


            if( !vorn.insect && !links.insect && !rechts.insect) {

                ybd->bact.ExtraState &= ~(EXTRA_LEFT|EXTRA_RIGHT);
                ybd->bact.ExtraState |= EXTRA_MOVE;
            }

            if ((ybd->bact.ExtraState & (EXTRA_LEFT|EXTRA_RIGHT)) == 0) {

                if ((links.insect == TRUE) && (rechts.insect == TRUE)) {
                    if (links.t < rechts.t) ybd->bact.ExtraState |= EXTRA_RIGHT;
                    else                    ybd->bact.ExtraState |= EXTRA_LEFT;
                };

                if ((links.insect == TRUE) && (rechts.insect == FALSE)) {
                    ybd->bact.ExtraState |= EXTRA_RIGHT;
                };

                if ((links.insect == FALSE) && (rechts.insect == TRUE)) {
                    ybd->bact.ExtraState |= EXTRA_LEFT;
                };

                if ((links.insect == FALSE) && (rechts.insect == FALSE) &&
                    (vorn.insect == TRUE))
                {
                    ybd->bact.ExtraState |= EXTRA_LEFT;
                };
            };

            /* ------------------------------------------------------------
            ** Jetzt kommt noch ein Höhentest. Der zeigt auch etwas in x-z-
            ** Richtung der geschwindigkeit und ist von dieser abhängig
            ** (also vom Betrag). Die Maximalgeschwindigkeit wird etwas
            ** seltsam berechnet, weil herkömmliche berechnungsarten bei
            ** "unüblichen " Kräfteverhältnissen danebenhauen.
            ** ----------------------------------------------------------*/
            max_speed  = max( ybd->bact.mass * GRAVITY, ybd->bact.max_force );
            max_speed /= max( 10.0, ybd->bact.air_const );

            /*** Bei Explosionsimpulsen können irrige v's auftreten ***/
            hoehe.vec.x = ybd->bact.dof.x * 200 * ybd->bact.dof.v / max_speed;
            if( hoehe.vec.x < -200.0 ) hoehe.vec.x = -200.0;
            if( hoehe.vec.x >  200.0 ) hoehe.vec.x =  200.0;
            hoehe.vec.y = ybd->bact.pref_height;
            hoehe.vec.z = ybd->bact.dof.z * 200 * ybd->bact.dof.v / max_speed;
            if( hoehe.vec.z < -200.0 ) hoehe.vec.z = -200.0;
            if( hoehe.vec.z >  200.0 ) hoehe.vec.z =  200.0;

            hoehe.pnt.x = bold->x;
            hoehe.pnt.y = bold->y;
            hoehe.pnt.z = bold->z;
            hoehe.flags = 0;

            _methoda( ybd->world, YWM_INTERSECT, &hoehe );
            if( hoehe.insect )
                ybd->bact.tar_unit.y = -( 1.0 - hoehe.t );
            else {

                /* ----------------------------------------------------
                ** Eigentlich müssen wir runter, wenn wir zu hoch sind,
                ** aber da kann es ja noch sein, daß wir gerade den
                ** User als Ziel haben, der ungehindert nach oben ab-
                ** hauen kann. Dann korrigieren wir nicht.
                ** --------------------------------------------------*/
                Object *user;
                _get( ybd->world, YWA_UserVehicle, &user );

                if( !( (((TARTYPE_BACTERIUM == ybd->bact.SecTargetType) &&
                         ((ybd->bact.SecondaryTarget.Bact->BactObject == user) ||
                          (ybd->bact.SecondaryTarget.Bact->BactClassID == BCLID_YPAROBO))) ||

                        ((TARTYPE_BACTERIUM == ybd->bact.PrimTargetType) &&
                         ((ybd->bact.PrimaryTarget.Bact->BactObject == user) ||
                          (ybd->bact.PrimaryTarget.Bact->BactClassID == BCLID_YPAROBO))) ) &&

                         (ybd->bact.tar_unit.y < -0.01) ) )
                    ybd->bact.tar_unit.y = max( 0.15, ybd->bact.tar_unit.y);

                }

            /* -----------------------------------------------------------
            ** Wenn wir gerade ausweichen, auch nach oben. Trotzdem Höhen-
            ** test wegen abprallen
            ** ---------------------------------------------------------*/
            if( ybd->bact.ExtraState & (EXTRA_LEFT | EXTRA_RIGHT) )
                ybd->bact.tar_unit.y = -0.2;

            if( hoehe.insect && 
                ((hoehe.t * ybd->bact.pref_height) < ybd->bact.radius) &&
                (ybd->bact.dof.y > 0.0) ) {

                /*** abprallen ***/
                rec.vec.x += hoehe.sklt->PlanePool[ hoehe.pnum ].A;
                rec.vec.y += hoehe.sklt->PlanePool[ hoehe.pnum ].B;
                rec.vec.z += hoehe.sklt->PlanePool[ hoehe.pnum ].C;
                recoil_count++;
                }

            /*** Abprallen? ***/
            if( recoil_count ) {

                rec.vec.x /= recoil_count;
                rec.vec.y /= recoil_count;
                rec.vec.z /= recoil_count;
                rec.mul_v = 0.7;    rec.mul_y = 2.0;    rec.time = time;
                _methoda( o, YBM_RECOIL, &rec );
                
                // Zur zeit bei autonomen kein Crashsound
                //if( (!(ybd->bact.sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE)) &&
                //    (fabs( ybd->bact.dof.v ) > NOISE_SPEED) &&
                //    (!(ybd->bact.ExtraState & EXTRA_LANDCRASH)) ) {
                //
                //    ybd->bact.ExtraState |= EXTRA_LANDCRASH;
                //    _StartSoundSource( &(ybd->bact.sc), VP_NOISE_CRASHLAND );
                //    }
                }
            else ybd->bact.ExtraState &= ~EXTRA_LANDCRASH;

            /*** nachnormieren, ist einfacher als drehen ***/
            if( ybd->bact.tar_unit.y != 0.0 ) {

                FLOAT b;
                struct flt_triple *tv = &(ybd->bact.tar_unit);

                b = nc_sqrt( tv->x*tv->x + tv->y*tv->y + tv->z*tv->z);
                tv->x /= b; tv->y /= b;  tv->z /= b;
                }


            /*** Die Bewegung ***/

            if( ybd->bact.ExtraState & EXTRA_LEFT ) {

                /* den tar_unit nach links drehen */
                betrag = nc_sqrt( vorn.vec.x*vorn.vec.x + vorn.vec.z*vorn.vec.z );
                /*ybd->bact.tar_unit.x = -vorn.vec.x / betrag;
                ybd->bact.tar_unit.z =  vorn.vec.z / betrag;   */
                ybd->bact.tar_unit.x = -vorn.vec.z / betrag;
                ybd->bact.tar_unit.z =  vorn.vec.x / betrag;

            } else if( ybd->bact.ExtraState & EXTRA_RIGHT ) {

                /* den tar_unit nach rechts drehen  */
                betrag = nc_sqrt( vorn.vec.x*vorn.vec.x + vorn.vec.z*vorn.vec.z );
                /*ybd->bact.tar_unit.x =  vorn.vec.x / betrag;
                ybd->bact.tar_unit.z = -vorn.vec.z / betrag;   */
                ybd->bact.tar_unit.x =  vorn.vec.z / betrag;
                ybd->bact.tar_unit.z = -vorn.vec.x / betrag;

            };
            

            /*
            ** Nun haben wir in tar_unit evtl. eine Richtung. Ist dem so,
            ** bewegen wir uns dorthin. Hubschrauber bewegen sich immer, 
            ** weil sie sich nicht einfach wie Panzer drehen können! 
            */

            yb_CalculateForce( ybd, msg );

            if( ybd->bact.ExtraState & EXTRA_MANEUVER ) {

                /*** Wir machen zur Zeit ein Ausweichmanöver. ***/
                ybd->bact.dof.v *= 0.95;
                }

            /*** Schubkorrektur ***/
            ybd->bact.act_force = (0.85 - ybd->bact.tar_unit.y) * 
                                          ybd->bact.max_force;


            move.flags = 0;
            move.t     = (FLOAT)msg->frame_time/1000;
            _methoda(ybd->bact.BactObject, YBM_MOVE, &move);

            /*
            ** Wir haben die neue position und die neue Geschwindigkeit.
            ** Nun drehen den Hubschrauber noch in Flugrichtung 
            */

            yb_DrehObjektInFlugrichtung( ybd, msg->frame_time );


            /* 
            ** Nun geht es um die Zielbearbeitung, so wir welche haben.
            ** Wichtig ist zuerst das Nebenziel. Wenn wir eins haben,
            ** bekämpfen wir es. Dazu rufen wir die Methoden zur Sektor-
            ** bekämpfung bzw. zur Bakterienbekämpfung auf, denn nur diese
            ** kennen die Waffen und können uber Schußweite und Streuung
            ** entscheiden 
            */

            fight.time = time;
            fight.global_time = ybd->bact.internal_time;
            if( ybd->bact.SecTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                fight.enemy.bact = ybd->bact.SecondaryTarget.Bact;
                fight.priority   = 1;
                _methoda(o, YBM_FIGHTBACT, &fight );
                }
            else
                if( ybd->bact.SecTargetType == (UBYTE)TARTYPE_SECTOR ) {
                    fight.pos          = ybd->bact.SecPos;
                    fight.enemy.sector = ybd->bact.SecondaryTarget.Sector;
                    fight.priority     = 1;
                    _methoda(o, YBM_FIGHTSECTOR, &fight );
                    }
                else
                    if( ybd->bact.PrimTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                        fight.enemy.bact = ybd->bact.PrimaryTarget.Bact;
                        fight.priority   = 0;
                        _methoda(o, YBM_FIGHTBACT, &fight );
                        }
                    else
                        if( ybd->bact.PrimTargetType == (UBYTE)TARTYPE_SECTOR ){
                            fight.pos.x        = ybd->bact.PrimPos.x;
                            fight.pos.z        = ybd->bact.PrimPos.z;
                            fight.enemy.sector = ybd->bact.PrimaryTarget.Sector;
                            fight.priority     = 0;
                            _methoda(o, YBM_FIGHTSECTOR, &fight );
                            }
                        else {
                            /* -------------------------------------------------------
                            ** was anderes bekämpfen wir nicht. Nun kann es aber sein, 
                            ** daß von user.c noch der FireZustand gesetzt ist. 
                            ** Also aus!
                            ** -----------------------------------------------------*/
                            if( ybd->bact.ExtraState & EXTRA_FIRE ) {

                                state.extra_on  = state.main_state = 0;
                                state.extra_off = EXTRA_FIRE;
                                _methoda( o, YBM_SETSTATE, &state );
                                }
                            ybd->bact.ExtraState &= ~(EXTRA_FIGHTS|EXTRA_FIGHTP);
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

            if( (ybd->bact.internal_time - ybd->bact.newtarget_time) > 500 ) {

                ybd->bact.newtarget_time = ybd->bact.internal_time;
            
                /*** Kann ich mir das leisten? ***/
                at1.target_type = ybd->bact.SecTargetType;
                at1.priority    = 1;
                at2.target_type = ybd->bact.PrimTargetType;
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
                        target.pos.x       = ybd->bact.pos.x;
                        target.pos.z       = ybd->bact.pos.z;
                        target.priority    = 0;
                        _methoda( o, YBM_SETTARGET, &target );
                        }

                    /*** Ist noch was übriggeblieben? ***/
                    if( (ybd->bact.PrimTargetType != TARTYPE_NONE) ||
                        (ybd->bact.SecTargetType  != TARTYPE_NONE) ) {

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

            if( ybd->flags & YBF_LandWhileWait ) {

                /*** Als Hubschrauber landen wir ***/
                ybd->bact.act_force = 0;
                if( !(ybd->bact.ExtraState & EXTRA_LANDED) ) {
                    
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
                    inter.pnt.x = ybd->bact.pos.x;
                    inter.pnt.y = ybd->bact.pos.y;
                    inter.pnt.z = ybd->bact.pos.z;
                    inter.vec.x = 0.0;
                    inter.vec.y = ybd->bact.over_eof + 50;
                    inter.vec.z = 0.0;
                    inter.flags = 0;
                    _methoda( ybd->world, YWM_INTERSECT, &inter );
                    if( inter.insect ) ybd->bact.pos.y = inter.ipnt.y -
                                                         ybd->bact.over_eof;
                    }
                }
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


_dispatcher( void, yb_YBM_ASSEMBLESLAVES, void *nix )
{
/*
**  FUNCTION    Wird nur ausgeführt, wenn ich Commander bin. Dann teste ich
**              alle  meine Untergebenen. Sind sie weit weg, kriegen sie
**              ihr TARTYPE_FORMATION, andernfalls TARTYPE_NONE. Somit kann
**              ich als wartender Commander alle verstreuten Leute um mich
**              sammeln. Da FORMATION ein Hauptziel ist, schneidet es sich 
**              auch nicht mit eventuellen Nebenzielen.
**
**              Sinnvoll ist der Aufruf z.B. von WAIT aus...
**
**  INPUT       siehe result
**
**  RESULT      nüscht
**
**  CHANGED     8-Dec-95    created 8100000C
**
*/

    struct MinNode *node;
    struct flt_triple vec;
    struct settarget_msg st;
    struct getformationpos_msg gfp;
    FLOAT  abstand;
    ULONG  count;
    struct ypabact_data *ybd = INST_DATA( cl, o);

    /*** Bin ich Commander oder Füllelement? ***/
    if( (ybd->bact.master == ybd->bact.robo) || (ybd->bact.robo == NULL) ) {

        node = (struct MinNode *) ybd->bact.slave_list.mlh_Head;
        count = 0;

        while( node->mln_Succ ) {

            /*** Nur die Lebenden beachten ***/
            if( ACTION_DEAD != ((struct OBNode *)node)->bact->MainState ) {

                /*** Abstand bestimmen (vom Formationspunkt!) ***/
                gfp.count = count;
                _methoda( o, YBM_GETFORMATIONPOS, &gfp );
                vec.x = ((struct OBNode *)node)->bact->pos.x - gfp.pos.x;
                vec.y = ((struct OBNode *)node)->bact->pos.y - gfp.pos.y;
                vec.z = ((struct OBNode *)node)->bact->pos.z - gfp.pos.z;

                abstand = nc_sqrt( vec.x*vec.x + vec.y*vec.y + vec.z*vec.z );

                /**** 2x if --> Trigger-Effekt ***/
                if( abstand < 200 ) {
                    
                    /*** Ziel ausschalten ***/
                    st.target_type = TARTYPE_NONE;
                    st.priority    = 0;
                    _methoda( ((struct OBNode *)node)->o, YBM_SETTARGET, &st );
                    }

                if( abstand > TAR_DISTANCE ) {
                    
                    /*** Formationspunkt übergeben ***/
                    st.pos = gfp.pos;
                    st.priority = 0;
                    st.target_type = TARTYPE_FORMATION;  // TARTYPE_FORMATION;
                    _methoda( ((struct OBNode *)node)->o, YBM_SETTARGET, &st);
                    }
                }

            node = node->mln_Succ;
            count++;
            }
        }
}


_dispatcher( void, yb_YBM_DOWHILECREATE, struct trigger_logic_msg *msg )
{
/*
**  FUNCTION    Zeugs machen während ich ge- sorry erzeugt werde
**
**  IN&OUT      nix
**
**  CHANGED     21.12.95    created 8100 000C
*/

    struct ypabact_data *ybd = INST_DATA( cl, o);
    FLOAT  time = ((FLOAT)(msg->frame_time)) / 1000;
    FLOAT  factor;
    struct Bacterium *robo_bact;

    ybd->bact.scale_time -= msg->frame_time;
    if( ybd->bact.scale_time <= 0 ) {

        struct setposition_msg sp;
        struct setstate_msg state;
        struct bact_message log;
        Object *userrobo;

        /*** Wieder normal werden ***/
        state.main_state = ACTION_NORMAL;
        state.extra_off  = state.extra_on = 0;
        _methoda( o, YBM_SETSTATE, &state );
        ybd->bact.ExtraState &= ~EXTRA_SCALE;

        /*** Uffm Sektor festkloppm ***/
        sp.x     = ybd->bact.pos.x;
        sp.y     = ybd->bact.pos.y;
        sp.z     = ybd->bact.pos.z;
        sp.flags = 0; // YBFSP_SetGround;
        _methoda( o, YBM_SETPOSITION, &sp );

        _get( ybd->world, YWA_UserRobo, &userrobo );

        /*** Meldung, daß ich da bin... nur als Commander ***/
        if( (ybd->bact.robo == userrobo) &&
            (ybd->bact.robo == ybd->bact.master) ) {

            log.ID    = LOGMSG_CREATED;
            log.para1 = 0;
            log.para2 = 0;
            log.para3 = 0;
            log.pri   = 26;
            log.sender = &(ybd->bact);
            _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
            }

        /*** Noch e wengel en Impulse ***/
        if( (ybd->bact.robo) && (BCLID_YPAGUN != ybd->bact.BactClassID) ) {

            FLOAT b;

            _get( ybd->bact.robo, YBA_Bacterium, &robo_bact );
            ybd->bact.dof.x = sp.x - robo_bact->pos.x;
            ybd->bact.dof.y = sp.y - robo_bact->pos.y;
            ybd->bact.dof.z = sp.z - robo_bact->pos.z;

            b = nc_sqrt( ybd->bact.dof.x * ybd->bact.dof.x +
                         ybd->bact.dof.y * ybd->bact.dof.y +
                         ybd->bact.dof.z * ybd->bact.dof.z );

            if( b > 0.001 ) {

                b = 1/b;
                ybd->bact.dof.x *= b;
                ybd->bact.dof.y *= b;
                ybd->bact.dof.z *= b;
                }

            ybd->bact.dof.v = 20;
            }
        }
    else {

        /*** Zeuch machen ***/
        ybd->bact.ExtraState |= EXTRA_SCALE;

        /*** Skalierungsfaktor... ***/
        if( ybd->bact.scale_time >= 0 ) {

            ybd->bact.scale_x = 0.1 + 0.9 / ( 0.9 + ybd->bact.scale_time / 1000.0);
            ybd->bact.scale_y = ybd->bact.scale_x;
            ybd->bact.scale_z = ybd->bact.scale_x;
            }
        else {

            ybd->bact.scale_x = 1.0;
            ybd->bact.scale_y = 1.0;
            ybd->bact.scale_z = 1.0;
            }

        /*** Rotieren ***/
        factor = 2.5 / ybd->bact.scale_x;
        yb_rot_round_lokal_y( &(ybd->bact), (FLOAT)(factor * time) );
        }
}


_dispatcher( BOOL, yb_YBM_DOWHILEBEAM, struct trigger_logic_msg *msg )
{
/*
** FUNCTION     Erledigt alles, was mit Beameffekten zu tun hat. Da es
**              nur im autonomen Mode möglich ist und die Leute danach
**              tot sind, können wir an der Matrix dranrumspielen.
**
**              Ich zerre das vehicle (zählerabhängig) nach oben, schalte
**              dann den CREATE-VP ein und lasse es wieder zusammenfallen
**
**              Am Ende muß das Ding korrekt gekillt und zurückgegeben werden!
**
** OUTPUT       TRUE, wenn wir mit der Sache fertig sind
**
** INPUT        Vor allem zeiten über die TLMessage
**
** CHANGED      na jetzt, wann sonst?
*/

    struct ypabact_data *ybd = INST_DATA( cl, o);
    BOOL   ret_value;
    FLOAT  first_part = 0.66;

    /*** Verzögerung ***/
    if( ybd->bact.scale_delay > 0 ) {

        ybd->bact.scale_delay -= msg->frame_time;
        return( FALSE );
        }

    if( ybd->bact.scale_time < BEAM_TIME * first_part ) {

        /*** Umschalten, wenn noch nicht getan ***/
        if( ybd->vis_proto != ybd->bact.vis_proto_create ) {

            struct setstate_msg state;

            state.main_state = ACTION_BEAM;
            state.extra_off  = state.extra_on = 0;
            _methoda(o, YBM_SETSTATE, &state );
            }

        /*** Aufzerren ***/
        ybd->bact.scale_x = 1.0;
        ybd->bact.scale_y = BEAM_MAX * ybd->bact.scale_time /
                            ( BEAM_TIME * first_part );
        ybd->bact.scale_z = 1.0;

        ybd->bact.ExtraState |= EXTRA_SCALE;
        ret_value = FALSE;
        }
    else {

        if( ybd->bact.scale_time < BEAM_TIME ) {

            
            /*** Schrumpfen ***/
            ybd->bact.scale_x = 1.0;
            ybd->bact.scale_y = BEAM_MAX -
                                (ybd->bact.scale_time - first_part * BEAM_TIME) *
                                BEAM_MAX / (BEAM_TIME * ( 1 - first_part ));
            ybd->bact.scale_z = 1.0;

            ybd->bact.ExtraState |= EXTRA_SCALE;
            ret_value = FALSE;
            }
        else {

            /*** killen und freigeben ***/
            struct beamnotify_msg bnm;
            bnm.b = &(ybd->bact);
            _methoda(ybd->world, YWM_BEAMNOTIFY, &bnm);

            /*** Meldungen unterdruecken ***/
            ybd->bact.ExtraState |= EXTRA_CLEANUP;

            /*** logisches Töten ***/
            _methoda( o, YBM_DIE, NULL );

            /*** Abmelden erst über BactClass ***/
            if( ybd->flags & YBF_UserInput )
                ybd->bact.ExtraState |= EXTRA_DONTRENDER;
            else       
                _methoda( o, YBM_RELEASEVEHICLE, o);

            ybd->bact.ExtraState &= ~EXTRA_SCALE;
            ret_value = TRUE;
            }
        }

    /*** Hochzählen ***/
    ybd->bact.scale_time += msg->frame_time;
    return( ret_value );
}


void yb_DoWhilePlasma( struct ypabact_data *ybd, LONG sc_time, 
                       LONG frame_time, FLOAT sc_fct )
{
    /* -----------------------------------------------------
    ** Was soll waehrend des Plasmazustandes gemacht werden.
    ** Achtung, die Roboklasse kann diese Routine nicht ver-
    ** wenden, dort also auch aktualisieren
    ** ---------------------------------------------------*/ 
    FLOAT angle, radikant;
        
    /*** Scalierung runter ***/
    radikant = (FLOAT)ybd->bact.scale_time / (FLOAT)sc_time;
    ybd->bact.extravp[0].scale = sc_fct * nc_sqrt( radikant );
        
    if( ybd->bact.extravp[0].scale < 0.0) 
        ybd->bact.extravp[0].scale = 0.0;
        
    /*** Winkel neu berechnen ***/
    angle = 2.0 * ybd->bact.max_rot * (FLOAT)frame_time / 1000.0;
    yb_rot_round_lokal_y2( &(ybd->bact.extravp[0].dir),angle ); 
}


_dispatcher( void, yb_YBM_DOWHILEDEATH, struct trigger_logic_msg *msg )
{
/*
** FUNCTION     Erledigt alles, was wir so machen müssen, während
**              wir tot sind.
**              Wir fallen, bis wir aufschlagen oder eine Zeit
**              vergangen ist.
**
**              NEU: Wir nehmen diese Funktion jetzt auch fuer den
**              Fall, dass der User drinnen sitzt. Wegen Netzwerk.
**              das einzige, was nicht passieren darf, ist doe Freigabe.
**              Wir machen alles wie bisher (abstuerzen, runterzaehlen)
**              geben aber erst bei !User frei
**
** INPUT        Vor allem zeiten über die TLMessage
**
** CHANGED      na jetzt, wann sonst?
*/

    struct ypabact_data *ybd = INST_DATA( cl, o );


    /* --------------------------------------------------------------
    ** Aufgeschlagen oder lange genug gefallen? Achtung, erst nach
    ** Logicdeath ist dead_entry aktuell und darf ausgewertet werden!
    ** ------------------------------------------------------------*/
    if( (ybd->bact.ExtraState & EXTRA_LANDED) ||

        (((ybd->bact.internal_time - ybd->bact.dead_entry) > DEATH_TIME) &&
        (ybd->bact.ExtraState & EXTRA_LOGICDEATH)) ) {

        /*** Wenn noch nicht Megadeth, dann schalten ***/
        if( !(ybd->bact.ExtraState & EXTRA_MEGADETH ) ) {

            struct setstate_msg state;
            state.extra_off = state.main_state = 0;
            state.extra_on  = EXTRA_MEGADETH;
            _methoda( o, YBM_SETSTATE, &state );
            }

        /*** Aufschlag vortäuschen (z.B. für YLS) ***/
        ybd->bact.ExtraState |= EXTRA_LANDED;

        /* ------------------------------------------
        ** Wenn das Ding mal einen Owner hatte, dann 
        ** war es ein rchtiges vehicle und darf einen
        ** Plasmaklumpen hinterlassen. Achtung, das
        ** muss auch uebers Netz!
        ** AUCH IN YB_NETWORK updaten!!!!!
        ** Ein CreateVP sollte schon da sein!
        ** ----------------------------------------*/
        if( (ybd->bact.Owner) &&
            (BCLID_YPAMISSY != ybd->bact.BactClassID) &&
            (ybd->bact.vis_proto_create != NULL) ) { 
            
            LONG  sc_time = (LONG)( PLASMA_TIME * (FLOAT)ybd->bact.Maximum);
            FLOAT sc_fct  = PLASMA_SCALE;

            if( sc_time < PLASMA_MINTIME )
                    sc_time = PLASMA_MINTIME;
            if( sc_time > PLASMA_MAXTIME )
                    sc_time = PLASMA_MAXTIME;
                        
            if( ybd->bact.extravp[0].flags & EVF_Active ) {
                
                ybd->bact.scale_time -= msg->frame_time;
                if( ybd->bact.scale_time <= 0 ) {
                    
                    /* -------------------------------------------
                    ** Flags nicht veraendern, sonst faengt alles
                    ** wieder von vorn an, weil das Objekt noch da 
                    ** ist, das ACTIVE-Flag aber nicht gesetzt ist
                    ** -----------------------------------------*/
                    ybd->bact.extravp[0].vis_proto = NULL;
                    ybd->bact.extravp[0].vp_tform  = NULL;
                    
                    if( ybd->YourLastSeconds <= 0) {
                        
                        if( ybd->flags & YBF_UserInput )
                            ybd->bact.ExtraState |= EXTRA_DONTRENDER;
                        else    
                            _methoda( o, YBM_RELEASEVEHICLE, o );
                        }    
                    }
                else { 
                
                    yb_DoWhilePlasma( ybd, sc_time, msg->frame_time, 
                                      sc_fct );

                                        /*** Weil plasma i.a. laenger dauert...***/
                    if( ybd->YourLastSeconds <= 0) 
                        ybd->bact.ExtraState |= EXTRA_DONTRENDER;
                    }    
                }
            else {
            
                struct ypaworld_data *ywd;
                
                /*** Wir fangen erst an, also init ***/
                ybd->bact.scale_time           = sc_time;
                ybd->bact.extravp[0].scale     = sc_fct;
                ybd->bact.extravp[0].pos       = ybd->bact.pos;
                ybd->bact.extravp[0].dir       = ybd->bact.dir; 

                ybd->bact.extravp[0].vis_proto = ybd->bact.vis_proto_create;
                ybd->bact.extravp[0].vp_tform  = ybd->bact.vp_tform_create;
                ybd->bact.extravp[0].flags    |= (EVF_Scale | EVF_Active);
                
                #ifdef __NETWORK__
                ywd = INST_DATA( ((struct nucleusdata *)(ybd->world))->o_Class,
                                 ybd->world );
                if( ywd->playing_network ) { 
                
                    struct sendmessage_msg sm;
                    struct ypamessage_startplasma sp;
                
                    /*** Message versenden ***/
                    sp.generic.message_id = YPAM_STARTPLASMA;
                    sp.generic.owner      = ybd->bact.Owner;
                    sp.scale              = sc_fct;
                    sp.time               = sc_time;
                    sp.ident              = ybd->bact.ident;
                    
                    /* ----------------------------------------------
                    ** Pos und dir mit verschicken, weil auf der Ziel
                    ** maschine alles "etwas verschoben" ablauft     
                    ** --------------------------------------------*/
                    sp.pos                = ybd->bact.pos;
                    sp.dir                = ybd->bact.dir;
                    
                    sm.receiver_id        = NULL;
                    sm.receiver_kind      = MSG_ALL;
                    sm.data               = &sp;
                    sm.data_size          = sizeof( sp );
                    sm.guaranteed         = TRUE;
                    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                    }                 
                #endif
                } 
                         
            }
        else {
        
            /*** kein Plasme, Freigeben, wenn YLS auf 0 ist ***/
            if( ybd->YourLastSeconds <= 0 ) {
            
                if( ybd->flags & YBF_UserInput )
                    ybd->bact.ExtraState |= EXTRA_DONTRENDER;
                else       
                    _methoda( o, YBM_RELEASEVEHICLE, o);
                }    
            }    
        }
    else {

        /*** Fallen lassen ***/
        struct crash_msg crash;

        crash.flags      = CRASH_CRASH | CRASH_SPIN;
        crash.frame_time = msg->frame_time;
        _methoda(o, YBM_CRASHBACTERIUM, &crash);
        }
}

