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
#include "ypa/ypatankclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"
#include "audio/audioengine.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine


#define NO_SPEED            4.0

#define TTDIST             300      // solange ist Teststrahl für Hindernisse
                                    // plus radius !!!
#define TTSTOP              50      // ab da plus radius wird gestopped                      
#define SIDEDIST            150     // so lang sind die Seitentestsrahlen
#define PREFERLEFT_ANGLE    0.4     // unterhalb dieses Winkel bevorzugt nach
                                    // Links fahren



#define OVERPT      50.0
#define RUNTER      (OVERPT + 10.0 + ytd->bact->viewer_over_eof)

/*-----------------------------------------------------------------*/
void yt_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yt_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle);
void yt_RotateTank( struct Bacterium *bact, FLOAT angle, UBYTE wie );



/*-----------------------------------------------------------------*/
_dispatcher(void, yt_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
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
**      20-Aug-95               neu!
*/

    FLOAT  betrag, time, over_eof, radius;
    ULONG  VIEW, CHECKVISEXACTLY, BACTCOLL, VIS, NEARPRIMTARGET;
    struct move_msg move;
    struct fight_msg fight;
    struct setstate_msg state;
    struct bactcollision_msg bcoll;
    struct crash_msg crash;
    struct settarget_msg target;
    struct alignvehicle_msg avm;
    struct intersect_msg inter, irechts, ilinks;
    BOOL   wall_left, slope_left, direct_coll, hr, hl;
    FLOAT  max_angle, tu_angle, distance;
    struct assesstarget_msg at1, at2;
    ULONG  rat1, rat2;
    FLOAT  ttdist, ttstop, td, tu_orig_angle;
    BOOL   near_target;

    struct ypatank_data *ytd = INST_DATA( cl, o );

    /*** Einige Vorbetrachtungen... ***/

    time = (FLOAT) msg->frame_time/1000;

    _get( o, YBA_CheckVisExactly, &CHECKVISEXACTLY );
    _get( o, YBA_Viewer, &VIEW );
    _get( o, YBA_BactCollision, &BACTCOLL );
    if( VIEW ) {
        radius   = ytd->bact->viewer_radius;
        over_eof = ytd->bact->viewer_over_eof;
    } else {
        radius   = ytd->bact->radius;
        over_eof = ytd->bact->over_eof;
    }
    
    avm.time    = time;
    avm.old_dir.x = ytd->bact->dir.m31;
    avm.old_dir.y = ytd->bact->dir.m32;
    avm.old_dir.z = ytd->bact->dir.m33;

    /*** Teststrahllänge ***/
    ttdist = ytd->bact->radius + TTDIST;
    ttstop = ytd->bact->radius + TTSTOP;

    /*** Sichtbarkeit ***/
    VIS = _methoda( ytd->world, YWM_ISVISIBLE, ytd->bact );

    /* tar_vec normieren */
    betrag = nc_sqrt( ytd->bact->tar_vec.x * ytd->bact->tar_vec.x +
                      ytd->bact->tar_vec.y * ytd->bact->tar_vec.y +
                      ytd->bact->tar_vec.z * ytd->bact->tar_vec.z );
    
    if( betrag > 0.0 ) {
        ytd->bact->tar_unit.x = ytd->bact->tar_vec.x / betrag;
        ytd->bact->tar_unit.y = ytd->bact->tar_vec.y / betrag;
        ytd->bact->tar_unit.z = ytd->bact->tar_vec.z / betrag;
        }

    /* ------------------------------------------------------------------------
    ** Die Variable NEARPRIMTARGET gibt an, ob wir in Hauptzielnähe sind.
    ** Der Einfachheit halber nehmen wir die Länge von tar_vec, wenn es auf das
    ** Hauptziel zeigt. Sollte ein Nebenziel aktiv sein, ist eh nicht zu er-
    ** warten. daß WAIT demnächst eintritt
    ** ----------------------------------------------------------------------*/
    if( (TARTYPE_NONE == ytd->bact->SecTargetType) &&
        (betrag       <  SECTOR_SIZE) )
        NEARPRIMTARGET = TRUE;
    else
        NEARPRIMTARGET = FALSE;
        
    /*** fuer weitere Tests. nicht kleiner als 0.7 wegen Kompaktsektoren ***/    
    if( betrag < (0.7 * SECTOR_SIZE) )
        near_target = TRUE;
    else
        near_target = FALSE;

    direct_coll = FALSE;    // bis jetzt nirgendwo rangeknallt

    switch( ytd->bact->MainState ) {

        case ACTION_NORMAL:

            /* --------------------------------------------------------
            ** Zuerst rufe ich die Bakterienkollision auf. Wenn es dort
            ** geknallt hat, gehe ich zurück, denn die Position wurde
            ** schon neu gesetzt (nur noch Ausrichtung 
            ** ------------------------------------------------------*/

            
            /*** Fallen lassen ***/
            if( !(ytd->bact->ExtraState & EXTRA_LANDED) ) {

                crash.flags      = CRASH_CRASH;
                crash.frame_time = msg->frame_time;
                _methoda( o, YBM_CRASHBACTERIUM, &crash);
                return;
                }

            /*** Gar kein Ziel? ***/
            if( ( ytd->bact->PrimTargetType == TARTYPE_NONE ) &&
                ( ytd->bact->SecTargetType  == TARTYPE_NONE ) ) {

                /*** in ACTION_WAIT übergehen ***/
                ytd->bact->MainState = ACTION_WAIT;
                
                ytd->bact->ExtraState &= ~(EXTRA_FIGHTP | EXTRA_FIGHTS);
                if( ytd->bact->ExtraState & EXTRA_FIRE ) {

                    state.extra_on  = state.main_state = 0;
                    state.extra_off = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                break;
                }
            if( betrag == 0.0)  break;
            
            /* ----------------------------------------------------------
            ** Warten wir auf grund einer Kollision? Wenn ja, dann ueber-
            ** springen wir allen Bewegungkram. Wir machen sozusagen fuer
            ** eine gewisse Zeit Pause, um die problemloesung anderen
            ** zu ueberlassen.
            ** --------------------------------------------------------*/
            if( ytd->bact->wait_for_coll > 0 ) {
            
                ytd->bact->wait_for_coll -= msg->frame_time;
                goto NACH_BEWEGUNG;
                }
                
            /* ----------------------------------------------------------
            ** Winkel zwischen Ziel und derzeitiger Ausrichtung. brauchen
            ** wir haeufig, somit hier schon mal berechnen
            ** --------------------------------------------------------*/    
            tu_angle = nc_acos( (ytd->bact->dir.m31 * ytd->bact->tar_unit.x +
                                 ytd->bact->dir.m33 * ytd->bact->tar_unit.z) /
                         nc_sqrt(ytd->bact->dir.m31 * ytd->bact->dir.m31+
                                 ytd->bact->dir.m33 * ytd->bact->dir.m33)/
                         nc_sqrt(ytd->bact->tar_unit.x * ytd->bact->tar_unit.x+
                                 ytd->bact->tar_unit.z * ytd->bact->tar_unit.z));
            
            /*** tu_angle wird nachbearbeitet, deshalb richtigen merken ***/
            tu_orig_angle = tu_angle;


            /*** Sind wir in einer Kollisionsbearbeitungsphase? ***/
            if( ytd->collflags & ( TCF_WALL_L  | TCF_WALL_R |
                                   TCF_SLOPE_L | TCF_SLOPE_R ) ) {

                /* ---------------------------------------------------
                ** Nun machen wir Tests, ob die Situation noch aktuell 
                ** ist, also das Hindernis noch da ist oder sich die 
                ** Wunschrichtung geändert hat.
                ** -------------------------------------------------*/

                /*** Ist das Ziel auf der anderen Seite als das Hindernis? ***/
                if( ((ytd->collvec.x * ytd->bact->tar_unit.x +
                      ytd->collvec.z * ytd->bact->tar_unit.z) < 0.0 ) &&
                     (ytd->collangle == 0.0 ) &&
                     (ytd->collway   == 0.0) ) {

                    /* --------------------------------------------------
                    ** Ja, die Richtung hat sich geändert, ich setze also
                    ** die Flags zurück und lösche die Hindernisdaten

                        WIRKLICH ????

                    ** ------------------------------------------------*/

                    ytd->collflags &= ~(TCF_WALL_L  | TCF_WALL_R |
                                        TCF_SLOPE_L | TCF_SLOPE_R);
                    ytd->collway    = 0.0;
                    ytd->collangle  = 0.0;
                    break;
                    }
                else {

                    /* -------------------------------------------------------
                    ** Das Ziel hat sich nicht geändert. Ich mache also Tests,
                    ** ob vielleicht das Hindernis weg ist. Diese Tests darf
                    ** ich aber nur machen, wenn way und angle auf 0 sind!
                    ** -----------------------------------------------------*/

                    /*** Abhangtest ***/
                    if( ( ytd->collflags & ( TCF_SLOPE_L | TCF_SLOPE_R ) ) &&
                        ( ytd->collangle == 0.0 ) &&
                        ( ytd->collway   == 0.0 ) ) {

                        /* -----------------------------------------------------
                        ** Wir testen jetzt (links oder rechts) mit einem leicht
                        ** zu uns geneigten Strahl, ob die Flächennormale, die
                        ** Sache noch als Hindernis erkennt. Ist dem nicht so,
                        ** löschen wir die Flags. Das heißt dann, daß wir (zu-
                        ** mindest in Sachen Abgrund) wieder frei sind.
                        ** Da Abhangflächen an Slurps zur bisherigen Sache auch 
                        ** relativ flach sein können, muß der Strahl von oben
                        ** nach unten gehen.
                        ** ---------------------------------------------------*/

                        if( ytd->collflags & TCF_SLOPE_L ) {

                            /*** Der Abhang ist links, also entgegen lokal x ***/
                            inter.pnt.x = ytd->bact->pos.x - 
                                          2 * ytd->bact->dir.m11 * ytd->bact->radius;
                            inter.pnt.y = ytd->bact->pos.y -
                                          2 * ytd->bact->dir.m12 * ytd->bact->radius;
                            inter.pnt.z = ytd->bact->pos.z -
                                          2 * ytd->bact->dir.m13 * ytd->bact->radius;
                            inter.vec.x = (4*ytd->bact->dir.m11 + 3*ytd->bact->dir.m21)*
                                          ytd->bact->radius;
                            inter.vec.y = (4*ytd->bact->dir.m12 + 3*ytd->bact->dir.m22)*
                                          ytd->bact->radius;
                            inter.vec.z = (4*ytd->bact->dir.m13 + 3*ytd->bact->dir.m23)*
                                          ytd->bact->radius;
                            }
                        else {

                            /*** Nee, rechts ***/
                            inter.pnt.x = ytd->bact->pos.x +
                                          2 * ytd->bact->dir.m11 * ytd->bact->radius;
                            inter.pnt.y = ytd->bact->pos.y +
                                          2 * ytd->bact->dir.m12 * ytd->bact->radius;
                            inter.pnt.z = ytd->bact->pos.z +
                                          2 * ytd->bact->dir.m13 * ytd->bact->radius;
                            inter.vec.x = (-4*ytd->bact->dir.m11 + 3*ytd->bact->dir.m21)*
                                          ytd->bact->radius;
                            inter.vec.y = (-4*ytd->bact->dir.m12 + 3*ytd->bact->dir.m22)*
                                          ytd->bact->radius;
                            inter.vec.z = (-4*ytd->bact->dir.m13 + 3*ytd->bact->dir.m23)*
                                          ytd->bact->radius;
                            }
                        inter.flags = INTERSECTF_CHEAT;

                        /*** Nun die Intersection ***/
                        _methoda( ytd->world, YWM_INTERSECT, &inter );

                        /*** Ist der Abhang weg? ***/
                        if( inter.insect ) {
                            if( inter.sklt->PlanePool[ inter.pnum ].B > YPS_PLANE ) {

                                /*** Weg! Flags ausschalten ***/
                                ytd->collflags &= ~(TCF_SLOPE_L | TCF_SLOPE_R);
                                }
                            }
                        else {

                            /*** keine Intersection --> Abhang weg ***/
                            ytd->collflags &= ~(TCF_SLOPE_L | TCF_SLOPE_R);
                            }

                        /*** noch restweg ***/
                        if( !(ytd->collflags & (TCF_SLOPE_L | TCF_SLOPE_R)))
                            ytd->collway = 80.0;
                        }

                    /*** Wandtest ***/
                    if( ( ytd->collflags & ( TCF_WALL_L | TCF_WALL_R ) ) &&
                        ( ytd->collangle == 0.0 ) &&
                        ( ytd->collway   == 0.0 ) ) {

                        /* -----------------------------------------------------
                        ** Wir fahren an einer Wand lang. Wir müssen also ab und 
                        ** zu mal den Arm raushalten, ob die Wand noch da ist.
                        ** ---------------------------------------------------*/

                        if( ytd->collflags & TCF_WALL_L ) {

                            /*** Hindernis ist links, also entgegen x ***/
                            inter.vec.x = -ytd->bact->dir.m11 * 150;
                            inter.vec.y = -ytd->bact->dir.m12 * 150;
                            inter.vec.z = -ytd->bact->dir.m13 * 150;
                            }
                        else {

                            /*** Hindernis ist rechts, also mit local x ***/
                            inter.vec.x = ytd->bact->dir.m11 * 150;
                            inter.vec.y = ytd->bact->dir.m12 * 150;
                            inter.vec.z = ytd->bact->dir.m13 * 150;
                            }
                        inter.pnt.x = ytd->bact->pos.x;
                        inter.pnt.y = ytd->bact->pos.y;
                        inter.pnt.z = ytd->bact->pos.z;
                        inter.flags = INTERSECTF_CHEAT;

                        /*** Die Intersection ***/
                        _methoda( ytd->world, YWM_INTERSECT, &inter );

                        /*** Hindernis weg? ***/
                        if( inter.insect) {
                            if( inter.sklt->PlanePool[ inter.pnum ].B > YPS_PLANE ) {

                                /*** Weg. Flags ausschalten **/
                                ytd->collflags &= ~(TCF_WALL_L | TCF_WALL_R);
                                }
                            }
                        else {

                            /*** Weg. Flags ausschalten **/
                            ytd->collflags &= ~(TCF_WALL_L | TCF_WALL_R);
                            }

                        /*** noch restweg ***/
                        if( !(ytd->collflags & (TCF_WALL_L | TCF_WALL_R)))
                            ytd->collway = 80.0;
                        }

                    /* ---------------------------------------------------
                    ** So, nun, bevor wir wieder mit dem "Nicht-Flag-Teil"
                    ** zusammenkommen, drehen wir uns. Wenn der Winkel
                    ** 0.0 ist, setzen wir auf jeden Fall das Move-Flag
                    ** -------------------------------------------------*/

                    if( ytd->collangle > 0.001 ) {

                        /*** Winkelbestimmung ***/
                        max_angle = ytd->bact->max_rot * time;

                        if( max_angle > ytd->collangle ) {

                            /*** noch letztes Stück drehen ***/
                            max_angle      = ytd->collangle;
                            ytd->collangle = 0.0;
                            }
                        else
                            ytd->collangle -= max_angle;

                        /*** Drehen ***/
                        if( ytd->collflags & (TCF_WALL_L | TCF_SLOPE_L) ) {

                            /*** Nach links ***/
                            yt_rot_round_lokal_y( ytd->bact,  (FLOAT) -max_angle );
                            }
                        else {

                            /*** Nach rechts ***/
                            yt_rot_round_lokal_y( ytd->bact, (FLOAT) max_angle );
                            }
                        }
                    else ytd->collangle = 0.0;

                    if( ytd->collangle == 0.0 )
                        ytd->bact->ExtraState |= EXTRA_MOVE;

                    }
                }
            else {

                /* -----------------------------------------------------
                ** Es sind keine Hindernisse da, folglich können wir uns
                ** nach tar_unit ausrichten. Move funktioniert ja eh so, 
                ** daß die Kraft in Richtung lokal z wirkt.
                ** Wenn der Zielwinkel zu groß ist, bremsen wir auch
                ** (so verhindern wir das Umfahren eines Zieles)
                ** ---------------------------------------------------*/
                ytd->bact->ExtraState |= EXTRA_MOVE;

                if( (tu_angle > 1.1) && (ytd->collway<=0.001) ) {
                
                    /*** v auch 0, sonst fährt er dann rückwärts ***/
                    //ytd->bact->dof.v       = 0.0;
                    //ytd->bact->ExtraState &= ~EXTRA_MOVE;
                    ytd->bact->act_force *= (0.1/(time + 0.1));
                    }

                if( (ytd->bact->dir.m31 * ytd->bact->tar_unit.z -
                     ytd->bact->dir.m33 * ytd->bact->tar_unit.x) < 0.0 )
                    tu_angle = -tu_angle;

                /*** Begrenzung (Gegenseitiger Ausschluß bei hohen FrameRaten!) ***/
                if( tu_angle < 0.0 ) {
                     
                    if( tu_angle < -ytd->bact->max_rot * time )
                        tu_angle = -ytd->bact->max_rot * time;
                    else
                        if( tu_angle > -0.05 ) tu_angle = 0.0;
                    }
                else {

                    if( tu_angle >  ytd->bact->max_rot * time )
                        tu_angle =  ytd->bact->max_rot * time;
                    else
                        if( tu_angle <  0.05 ) tu_angle = 0.0;
                    }

                /*** Drehen ***/
                if( ytd->collway <= 0.0001 )
                    yt_rot_round_lokal_y( ytd->bact, tu_angle );
                }

            /* ----------------------------------------------------------------
            ** Da sind wir wieder! jetzt sind wir eingedreht und wenn das MOVE-
            ** Flag gesetzt ist, können wir uns bewegen. Nach der Bewegung
            ** testen wir noch, ob wir den zurückgelegten Weg (old -> pos)
            ** abziehen müssen
            ** --------------------------------------------------------------*/

            if( ytd->bact->ExtraState & EXTRA_MOVE ) {

                /* ------------------------------------------------------
                ** Kraft nicht sofort auf maximum, wegen Tip, welches
                ** sich immer wieder auf den Boden bezieht und sich somit
                ** nicht systematisch aufbauen kann. Aber die Kraft kann
                ** es!
                ** ----------------------------------------------------*/
                ytd->bact->act_force += (LONG)( (FLOAT)ytd->bact->max_force *
                                                time * 0.8 );
                if( ytd->bact->act_force > ytd->bact->max_force )
                    ytd->bact->act_force = ytd->bact->max_force;

                /* -------------------------------------------------------
                ** Aber, wenn wir collway haben und frei sind, dann
                ** werden wir uns bald drehen. Kraft etwas runter. collway
                ** ist ueblicherweise max. 100
                ** -----------------------------------------------------*/
                if( (!(ytd->collflags & (TCF_WALL_L|TCF_WALL_R|TCF_SLOPE_L|TCF_SLOPE_R))) &&
                    (ytd->collway > 0.0) )
                    ytd->bact->act_force -= (LONG)( (FLOAT)ytd->bact->max_force *
                                            time * 0.6 );
                if( ytd->bact->act_force < 0.0 )
                    ytd->bact->act_force = 0.0;

                /* ---------------------------------------------------------
                ** Bewegen eigentlich immer, es sei denn, wir fahren auf ein
                ** zu bekaempfendes Ziel zu, welches wir treffen koennen.
                ** -------------------------------------------------------*/
                if( ! ((ytd->bact->ExtraState & EXTRA_ATTACK) &&
                       (ytd->free_lof) &&
                       (0 == ytd->collflags)) ) {
                
                    move.flags = 0;
                    move.t     = time;
                    _methoda( o, YBM_MOVE, &move );
                    }
                
                /*** Strecke abziehen ***/
                if( ytd->collway > 0.0 ) {

                    ytd->collway -= nc_sqrt( 
                                    (ytd->bact->pos.x - ytd->bact->old_pos.x) *
                                    (ytd->bact->pos.x - ytd->bact->old_pos.x) +
                                    (ytd->bact->pos.z - ytd->bact->old_pos.z) *
                                    (ytd->bact->pos.z - ytd->bact->old_pos.z) );
                    
                    if( ytd->collway <= 0.0 ) ytd->collway = 0.0;
                    }
                }
            else {

                /*** wenn nicht MOVE, dann trotzdem Position merken!!! ***/
                ytd->bact->old_pos = ytd->bact->pos;
                ytd->bact->act_force = 0;
                }

            /* --------------------------------------------------------------------
            ** Nun kommen die ganzen Tests. Weil der Abhang wichtiger als die Wand
            ** ist, machen wir ihn danach ( können wir überschreiben ). Also
            ** zuerst der Kollisionstest. Dazu schicken wir einen Strahl nach
            ** vorn. Wenn dieser eine Kollision meldet, dann berechnen wir die
            ** Entfernung, wobei die Fläche schräg geschnitten werden kann! ist die
            ** Entfernung kleiner als ein bestimmter Wert, dann schalten wir das
            ** MOVE-Flag aus. Den Winkel berechnen wir je nach Flags. Sind keine
            ** Wand-Flags gesetzt, dann drehen wir uns so, daß der Winkel kleiner
            ** 90 Grad ist. Sind aber schon Flags gesetzt, dann drehen wir uns vom
            ** Hindernis weg, also nach rechts, wenn TCF_WALL_L.
            ** ------------------------------------------------------------------*/

            inter.pnt.x = ytd->bact->old_pos.x;
            inter.pnt.y = ytd->bact->old_pos.y;
            inter.pnt.z = ytd->bact->old_pos.z;
            inter.vec.x = ytd->bact->pos.x - ytd->bact->old_pos.x;
            inter.vec.y = ytd->bact->pos.y - ytd->bact->old_pos.y;
            inter.vec.z = ytd->bact->pos.z - ytd->bact->old_pos.z;
            
            distance = nc_sqrt( inter.vec.x * inter.vec.x + inter.vec.y *
                                inter.vec.y + inter.vec.z * inter.vec.z );

            if( (distance > 5.0) && (ytd->bact->ExtraState & EXTRA_MOVE) ) {
                
                /*** Distance ist ganz schön groß... ***/
                inter.vec.x *= ( ttdist / distance );
                inter.vec.y *= ( ttdist / distance );
                inter.vec.z *= ( ttdist / distance );
                }
            else {

                inter.vec.x = ytd->bact->dir.m31 * ttdist;
                inter.vec.y = ytd->bact->dir.m32 * ttdist;
                inter.vec.z = ytd->bact->dir.m33 * ttdist;
                }
            inter.flags = INTERSECTF_CHEAT;

            /*** Der test ***/
            _methoda( ytd->world, YWM_INTERSECT, &inter );

            /*** Hindernis voraus? ***/
            if( inter.insect &&
                (inter.sklt->PlanePool[ inter.pnum ].B < YPS_PLANE) &&
                (ytd->collangle == 0.0) ) {
                
                FLOAT  iA, iB, iC;
                FLOAT  rem_angle;
                
                /*** Weil PlaneParas sich bei Int. ändern können, merken ***/
                iA = inter.sklt->PlanePool[ inter.pnum ].A;
                iB = inter.sklt->PlanePool[ inter.pnum ].B;
                iC = inter.sklt->PlanePool[ inter.pnum ].C;
    
                /* --------------------------------------------------------
                ** Es gibt 2 Arten von Hindernissen. Einmal die, wo
                ** wir stoppen, weil sie nah dran sind und dann die,
                ** wo wir versuchen auszuweichen. Die ersteren beschreiben
                ** sozusagen das klassische Verhalten.
                ** Achtung, dass Ausweichen machen wir nur, wenn wir Zeit
                ** dafuer haben (SecTarget), wir nicht woanders hin muessen
                ** (tu_angle), sich das Hindernis nicht drastisch geaendert
                ** hat (rem_angle) und wir nicht in Hauptzielnaehe sind.
                ** ------------------------------------------------------*/
                rem_angle = 0;
                
                if( (inter.t * ttdist > ttstop) &&
                    (fabs(tu_orig_angle) < 1.0) &&
                    (fabs(rem_angle) < 0.7) &&
                    (FALSE == near_target) &&
                    (TARTYPE_NONE == ytd->bact->SecTargetType) ) {
         
                    /* ---------------------------------------------------------
                    ** Wir drehen nur etwas zur Seite. Das machen wir wie folgt:
                    ** wir setzen collway auf etwas, weil damit das eindrehen
                    ** auf tarvec verhindert wird und ausserdem danach noch eine
                    ** Mindeststrecke gefahren wird.
                    ** Weiterhin drehen wir uns in die Richtung, in die wir uns
                    ** auch so drehen wuerden.
                    ** Drehen muss frame_time und speed-abhaengig sein.
                    ** -------------------------------------------------------*/
                    FLOAT ao_angle;
                    
                    ao_angle  = (FLOAT)(msg->frame_time) / 1000.0;
                    ao_angle *= ytd->bact->dof.v/ 10.0;
                           
                    if( (ytd->bact->dir.m31 * iC - ytd->bact->dir.m33 * iA) < 0 )
                        yt_rot_round_lokal_y( ytd->bact, ao_angle );
                    else
                        yt_rot_round_lokal_y( ytd->bact, -ao_angle );
                        
                    ytd->collway = inter.t * ttdist + 10;    
                    }
                    
                /*** Direkte Kollision? klassisch ***/    
                if(inter.t * ttdist <= ttstop) {

                    struct intersect_msg int2;
                    struct flt_triple svec;
    
                    /*** Stoppen? ***/
                    ytd->bact->ExtraState &= ~EXTRA_MOVE;
                    ytd->bact->pos         = ytd->bact->old_pos;
    
                    /* NEU -------------------------------------------------------------
                    ** Die Hindernisrichtung läßt sich nicht allein aus der Fläche, auf
                    ** die ich treffe, ermitteln, weil gar nicht klar ist, aus wel-
                    ** cher Richtung ich komme. Deshalb muß ich die Schnittlinie
                    ** aus meiner und der Hindernisfläche berechnen, die durch das
                    ** VP der Normalenvektoren repräsentiert wird.
                    **
                    ** Wichtig für die weitere Bearbeitung ist letztendlich, daß collvec
                    ** von mir wegzeigt.
                    ** ---------------------------------------------------------------*/
    
                    int2.pnt.x = ytd->bact->pos.x;
                    int2.pnt.y = ytd->bact->pos.y;
                    int2.pnt.z = ytd->bact->pos.z;
                    int2.vec.x = ytd->bact->dir.m21 * 300;
                    int2.vec.y = ytd->bact->dir.m22 * 300;
                    int2.vec.z = ytd->bact->dir.m23 * 300;
                    int2.flags  = INTERSECTF_CHEAT;  // egal, weil sich der Subs. nicht aendert
    
                    _methoda( ytd->world, YWM_INTERSECT, &int2 );
    
                    /*** VP, also Schnittlinie berechnen ***/
                    if( int2.insect ) {
    
                        /*** Was immer sein sollte ***/
                        svec.x = iB * int2.sklt->PlanePool[ int2.pnum ].C -
                                 iC * int2.sklt->PlanePool[ int2.pnum ].B;
                        svec.y = iC * int2.sklt->PlanePool[ int2.pnum ].A -
                                 iA * int2.sklt->PlanePool[ int2.pnum ].C;
                        svec.z = iA * int2.sklt->PlanePool[ int2.pnum ].B -
                                 iB * int2.sklt->PlanePool[ int2.pnum ].A;
                        }
                    else {
    
                        svec.x = -iC;
                        svec.y =  0;
                        svec.z =  iA;
                        }
    
                    /*** Sind Flächen identisch? ***/
                    if( (svec.x == 0.0) && (svec.y == 0.0) && (svec.z == 0.0)) {
    
                        /*** Einfachsterweise lokal x ***/
                        svec.x = ytd->bact->dir.m11;
                        svec.y = ytd->bact->dir.m12;
                        svec.z = ytd->bact->dir.m13;
                        }
    
                    /*** Nun CollVec so ermitteln, daß er von mir weg zeigt ***/
                    ytd->collvec.x = ytd->bact->dir.m22 * svec.z -
                                     ytd->bact->dir.m23 * svec.y;
                    ytd->collvec.y = ytd->bact->dir.m23 * svec.x -
                                     ytd->bact->dir.m21 * svec.z;
                    ytd->collvec.z = ytd->bact->dir.m21 * svec.y -
                                     ytd->bact->dir.m22 * svec.x;
    
                    /*** zeigt der Abhang komischerweise zu mir? ***/
                    if( (ytd->bact->dir.m31 * ytd->collvec.x +
                         ytd->bact->dir.m32 * ytd->collvec.y +
                         ytd->bact->dir.m33 * ytd->collvec.z) < 0.0 ) {
    
                         ytd->collvec.x = -ytd->collvec.x;
                         ytd->collvec.y = -ytd->collvec.y;
                         ytd->collvec.z = -ytd->collvec.z;
                         }
                    
                    /* -------------------------------------------------------------
                    ** Winkel je nach bisherigen Flags bestimmen. Zuerst ermitteln
                    ** wir den Winkel und die Drehrichtung. anschließend entscheiden 
                    ** die Flags, ob wir das überhaupt dürfen.
                    ** Dabei ermitteln wir den Winkel wie folgt: Wir kennen die 
                    ** Richtung des Hindernisses und machen nun zwei Intersections
                    ** parallel zum Hindernis nach beiden Seiten. Liefert eine
                    ** der beiden ein richtiges Hindernis, so fahren wir in die
                    ** andere Richtung, andernfalls entscheidet wie bisher das
                    ** SP zum Ziel.
                    ** -----------------------------------------------------------*/
                    
                    td = nc_sqrt( ytd->collvec.x*ytd->collvec.x+ytd->collvec.z*ytd->collvec.z);
                    irechts.vec.x =  ytd->collvec.z * SIDEDIST / td;
                    irechts.vec.z = -ytd->collvec.x * SIDEDIST / td;
                    ilinks.vec.x  = -ytd->collvec.z * SIDEDIST / td;
                    ilinks.vec.z  =  ytd->collvec.x * SIDEDIST / td;
                    irechts.vec.y =  ilinks.vec.y = 0.0;
                    irechts.pnt.x =  ilinks.pnt.x = ytd->bact->old_pos.x;
                    irechts.pnt.y =  ilinks.pnt.y = ytd->bact->old_pos.y;
                    irechts.pnt.z =  ilinks.pnt.z = ytd->bact->old_pos.z;
                    irechts.flags =  INTERSECTF_CHEAT;
                    ilinks.flags  =  INTERSECTF_CHEAT;
    
                    /* ----------------------------------------------------
                    ** Wenn Ebenenparameter ausgewertet werden sollen, dann
                    ** Intersections nacheinander, weil diese (bei Slurps)
                    ** überschrieben werden können
                    ** --------------------------------------------------*/
    
                    _methoda( ytd->world, YWM_INTERSECT, &irechts );
                    if( irechts.insect &&
                        (irechts.sklt->PlanePool[ irechts.pnum ].B < YPS_PLANE) )
                        hr = TRUE;  else hr = FALSE;
    
                    _methoda( ytd->world, YWM_INTERSECT, &ilinks  );
                    if( ilinks.insect &&
                        (ilinks.sklt->PlanePool[ ilinks.pnum ].B < YPS_PLANE) )
                        hl = TRUE;  else hl = FALSE;
    
                    if( (hl && !hr) || (hr && !hl) ) {
    
                        /*** Nur auf einer Seite ein Hindernis. Entscheidung möglich ***/
                        if( hl ) {
    
                            /*** Hindernis links. Nach rechts drehen ***/
                            ytd->collangle = nc_acos(
                                (ytd->bact->dir.m31 * irechts.vec.x +
                                 ytd->bact->dir.m33 * irechts.vec.z) /
                                nc_sqrt(ytd->bact->dir.m31 * ytd->bact->dir.m31 +
                                        ytd->bact->dir.m33 * ytd->bact->dir.m33) /
                                 SIDEDIST );
    
                            wall_left = TRUE;
    
                            }
                        else {
    
                            /*** Hindernis rechts. Nach links drehen ***/
                            ytd->collangle = nc_acos(
                                (ytd->bact->dir.m31 * ilinks.vec.x +
                                 ytd->bact->dir.m33 * ilinks.vec.z) /
                                nc_sqrt(ytd->bact->dir.m31 * ytd->bact->dir.m31 +
                                        ytd->bact->dir.m33 * ytd->bact->dir.m33) /
                                 SIDEDIST );
    
                            wall_left = FALSE;
                            }
                        }
                    else {
    
                        /* ---------------------------------------
                        ** Keine Hindernisse oder 
                        ** unbedeutende Hindernisse oder
                        ** beidseitige richtige Hindernisse. 
                        ** Wie üblich entscheidet die Zielrichtung 
                        ** -------------------------------------*/
                    
                        ytd->collangle = PI / 2 - nc_acos(
                                         (ytd->bact->dir.m31 * ytd->collvec.x +
                                          ytd->bact->dir.m33 * ytd->collvec.z) /
                                  nc_sqrt(ytd->bact->dir.m31 * ytd->bact->dir.m31 +
                                          ytd->bact->dir.m33 * ytd->bact->dir.m33) /
                                  nc_sqrt(ytd->collvec.x * ytd->collvec.x +
                                          ytd->collvec.z * ytd->collvec.z) );
                        ytd->collangle += 0.01;
    
                        if( (ytd->bact->dir.m31 * ytd->collvec.z - 
                             ytd->bact->dir.m33 * ytd->collvec.x ) > 0.0 )
                            wall_left = TRUE;
                        else
                            wall_left = FALSE;
                        }
    
                    if( (ytd->collflags & TCF_WALL_L) && !wall_left ) {
    
                        /* -------------------------------------------------------
                        ** Links ist eine Wand, also ein Hindernis, und wir wollen
                        ** uns nach links drehen. Das geht schief. Folglich lassen
                        ** wir das Flag und drehen uns 180 - angle.
                        ** -----------------------------------------------------*/
                        ytd->collangle = PI - ytd->collangle;
                        }
                    else {
                        if( (ytd->collflags & TCF_WALL_R) && wall_left ) {
    
                            ytd->collangle = PI - ytd->collangle;
                            }
                        else {
    
                            /* -----------------------------------------------
                            ** Keine probleme, wir brauchen nur noch die Flags
                            ** zu setzen.
                            ** ---------------------------------------------*/
                            if( wall_left )
                                ytd->collflags |= TCF_WALL_L;
                            else
                                ytd->collflags |= TCF_WALL_R;
                            }
                        }
    
                    /*** Der Weg ***/
                    ytd->collway = 100;    // vorher 50
    
                    /*** Nun kann es sein, daß es eine direkte Kollision war ***/
                    if( (inter.t * ttdist) < ytd->bact->radius ) direct_coll = TRUE;
                    }
                }

            /* -------------------------------------------------------------
            ** Nun kommt der Abhangtest. Diesen liefert uns Allignvehicle.
            ** Kommt dies mit FALSE zurück, so ging die Ausrichtung ins 
            ** Leere und wir setzen zurück. Gleichzeitig (naja gut, kurz 
            ** danach) testen wir mit einem Intersectionstrahl, wie denn das
            ** Hindernis aussieht. Wir nehmen nur ALIGNVEHICLE_A !!!
            ** Achtung, ein Abhang kann auch ein Flaches Hindernis sein
            ** (an Slurps), dann wird der Winkel falsch!
            ** -----------------------------------------------------------*/

            if( !_methoda( o, YTM_ALIGNVEHICLE_A, &avm ) ) {

                ytd->bact->dof.v       = 0.0;
                ytd->bact->act_force   = 0.0;

                /*** Wenn uns mit avm keine abhang-Infos übergeben wurden...***/
                inter.insect = FALSE;
                if( !(avm.ret_msg & AVMRET_SLOPE) ) {

                    FLOAT len = 8.0 * ytd->bact->radius;

                    /*** Abhang! Austesten. von pos 45 Grad nach unten zurück ***/
                    inter.pnt.x = ytd->bact->pos.x +
                                  ytd->bact->dir.m31 * ytd->bact->radius;
                    inter.pnt.y = ytd->bact->pos.y +
                                  ytd->bact->dir.m32 * ytd->bact->radius;
                    inter.pnt.z = ytd->bact->pos.z +
                                  ytd->bact->dir.m33 * ytd->bact->radius;
                    inter.vec.x = len * (ytd->bact->dir.m21 - ytd->bact->dir.m31);
                    inter.vec.y = len * (ytd->bact->dir.m22 - ytd->bact->dir.m32);
                    inter.vec.z = len * (ytd->bact->dir.m23 - ytd->bact->dir.m33);
                    inter.flags = INTERSECTF_CHEAT;
                    _methoda( ytd->world, YWM_INTERSECT, &inter );

                    if( (!inter.insect) ||
                        ( inter.insect &&
                        ((inter.t * len * 0.7) > (ytd->bact->over_eof + 30)) ) ) {

                        ytd->bact->ExtraState &= ~EXTRA_LANDED;
                        return;
                        }
                    }
                
                /*** Jetzt erst rücksetzen! ***/
                ytd->bact->pos = ytd->bact->old_pos;

                /*** Wenn zu schräg .... ***/
                if( (inter.insect &&
                    (inter.sklt->PlanePool[ inter.pnum ].B < YPS_PLANE )) ||
                    (avm.ret_msg & AVMRET_SLOPE) ) {

                    struct flt_triple hvec, svec;
                    struct intersect_msg int2;
                    FLOAT  iA, iB, iC;

                    /*** ... dann richtiger Abhang ***/
                    ytd->bact->ExtraState &= ~EXTRA_MOVE;

                    /*** WandFlags ausschalten ***/
                    ytd->collflags &= ~(TCF_WALL_L | TCF_WALL_R );

                    /*** Merken der Ebenenparameter ***/
                    if( inter.insect ) {

                        iA = inter.sklt->PlanePool[ inter.pnum ].A;
                        iB = inter.sklt->PlanePool[ inter.pnum ].B;
                        iC = inter.sklt->PlanePool[ inter.pnum ].C;
                        }

                    /* NEU ----------------------------------------------------
                    ** Die Abhangrichtung wird nicht durch die Flächenausrich-
                    ** tung (weil gar nicht klar ist, aus welcher Richtung ich
                    ** mich dieser nähere), sondern durch den Schnitt aus alter
                    ** und neuer Standfläche bestimmt. Dies entspricht dem
                    ** Vektorprodukt der Normalenvektoren!
                    ** Das programm basiert darauf, daß CollVec von mir weg
                    ** zeigt, ich muß das Resultat also noch so zurechtbiegen
                    ** ------------------------------------------------------*/

                    int2.pnt.x = ytd->bact->pos.x;
                    int2.pnt.y = ytd->bact->pos.y;
                    int2.pnt.z = ytd->bact->pos.z;
                    int2.vec.x = ytd->bact->dir.m21 * 300;
                    int2.vec.y = ytd->bact->dir.m22 * 300;
                    int2.vec.z = ytd->bact->dir.m23 * 300;
                    int2.flags = INTERSECTF_CHEAT;

                    _methoda( ytd->world, YWM_INTERSECT, &int2 );

                    /*** Wie ist denn nun der Abhang ***/
                    if( avm.ret_msg & AVMRET_SLOPE ) {

                        hvec.x = -avm.slope.x;
                        hvec.y = -avm.slope.y;
                        hvec.z = -avm.slope.z;
                        }
                    else {

                        hvec.x = -iA;
                        hvec.y = -iB;
                        hvec.z = -iC;
                        }

                    /*** VP, also Schnittlinie berechnen ***/
                    if( int2.insect ) {     // vorher: inter.insect

                        /*** Was immer sein sollte ***/
                        svec.x = hvec.y * int2.sklt->PlanePool[ int2.pnum ].C -
                                 hvec.z * int2.sklt->PlanePool[ int2.pnum ].B;
                        svec.y = hvec.z * int2.sklt->PlanePool[ int2.pnum ].A -
                                 hvec.x * int2.sklt->PlanePool[ int2.pnum ].C;
                        svec.z = hvec.x * int2.sklt->PlanePool[ int2.pnum ].B -
                                 hvec.y * int2.sklt->PlanePool[ int2.pnum ].A;
                        }
                    else {

                        svec.x = -hvec.z;
                        svec.y =  0;
                        svec.z =  hvec.x;
                        }

                    /*** Nun CollVec so ermitteln, daß er von mir weg zeigt ***/
                    ytd->collvec.x = ytd->bact->dir.m22 * svec.z -
                                     ytd->bact->dir.m23 * svec.y;
                    ytd->collvec.y = ytd->bact->dir.m23 * svec.x -
                                     ytd->bact->dir.m21 * svec.z;
                    ytd->collvec.z = ytd->bact->dir.m21 * svec.y -
                                     ytd->bact->dir.m22 * svec.x;

                    /*** zeigt der Abhang komischerweise zu mir? ***/
                    if( (ytd->bact->dir.m31 * ytd->collvec.x +
                         ytd->bact->dir.m32 * ytd->collvec.y +
                         ytd->bact->dir.m33 * ytd->collvec.z) < 0.0 ) {

                         ytd->collvec.x = -ytd->collvec.x;
                         ytd->collvec.y = -ytd->collvec.y;
                         ytd->collvec.z = -ytd->collvec.z;
                         }

                    /*** Winkelbestimmung wie bei Wand ***/
                    ytd->collangle = PI / 2 - nc_acos(
                                     (ytd->bact->dir.m31 * ytd->collvec.x +
                                      ytd->bact->dir.m33 * ytd->collvec.z ) /
                             nc_sqrt( ytd->bact->dir.m31*ytd->bact->dir.m31+
                                      ytd->bact->dir.m33*ytd->bact->dir.m33) /
                             nc_sqrt( ytd->collvec.x * ytd->collvec.x +
                                      ytd->collvec.z * ytd->collvec.z ) );
                    /*** Er kann ungünstig an die Fläche fahren...***/
                    if( ytd->collangle < 0.1 ) ytd->collangle = 0.1;

                    if( (ytd->bact->dir.m31 * ytd->collvec.z - 
                         ytd->bact->dir.m33 * ytd->collvec.x ) > 0.0 )
                        slope_left = TRUE;
                    else
                        slope_left = FALSE;

                    if( (ytd->collflags & TCF_SLOPE_L) && !slope_left ) {

                        /* -------------------------------------------------------
                        ** Links ist eine Abhang, also ein Hindernis, und wir wollen
                        ** uns nach links drehen. Das geht schief. Folglich lassen
                        ** wir das Flag und drehen uns 180 - angle.
                        ** -----------------------------------------------------*/
                        ytd->collangle = PI - ytd->collangle;
                        }
                    else {
                        if( (ytd->collflags & TCF_SLOPE_R) && slope_left ) {

                            ytd->collangle = PI - ytd->collangle;
                            }
                        else {

                            /* -----------------------------------------------
                            ** Keine probleme, wir brauchen nur noch die Flags
                            ** zu setzen.
                            ** ---------------------------------------------*/
                            if( slope_left )
                                ytd->collflags |= TCF_SLOPE_L;
                            else
                                ytd->collflags |= TCF_SLOPE_R;
                            }
                        }

                    /*** Der Weg ***/
                    ytd->collway = 100;   // vorher 80
                    }
                }


            /* -----------------------------------------------------------
            ** Itze probier ich die BactCollision mal nach der Bewegung.
            ** Aus Effizienzgründen mache ich die nur, wenn ich sichtbar
            ** oder in Hauptzielnähe bin (damit ich nicht im Knäuel warte)
            ** ---------------------------------------------------------*/
            if( BACTCOLL && (VIS || NEARPRIMTARGET) ) {

                /*** Nur Boden, denn wir sind ja gelandet ***/
                bcoll.frame_time  = msg->frame_time;
                if( _methoda( o, YBM_BACTCOLLISION, &bcoll))
                    return;
                }

NACH_BEWEGUNG:
            
            /*** Das Bearbeiten der Ziele ***/
            fight.time        = time;
            fight.global_time = ytd->bact->internal_time;
            if( ytd->bact->SecTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                fight.enemy.bact = ytd->bact->SecondaryTarget.Bact;
                fight.priority   = 1;
                _methoda(o, YBM_FIGHTBACT, &fight );
                }
            else
                if( ytd->bact->SecTargetType == (UBYTE)TARTYPE_SECTOR ) {
                    fight.pos          = ytd->bact->SecPos;
                    fight.enemy.sector = ytd->bact->SecondaryTarget.Sector;
                    fight.priority     = 1;
                    _methoda(o, YBM_FIGHTSECTOR, &fight );
                    }
                else
                    if( ytd->bact->PrimTargetType == (UBYTE)TARTYPE_BACTERIUM ) {
                        fight.enemy.bact = ytd->bact->PrimaryTarget.Bact;
                        fight.priority   = 0;
                        _methoda(o, YBM_FIGHTBACT, &fight );
                        }
                    else
                        if( ytd->bact->PrimTargetType == (UBYTE)TARTYPE_SECTOR ){
                            fight.pos          = ytd->bact->PrimPos;
                            fight.enemy.sector = ytd->bact->PrimaryTarget.Sector;
                            fight.priority     = 0;
                            _methoda(o, YBM_FIGHTSECTOR, &fight );
                            }
                        else {

                            /* --------------------------------------------------
                            ** was anderes bekämpfen wir nicht. Es kann aber noch
                            ** sein, daß wir noch feuern 
                            ** ------------------------------------------------*/
                            ytd->bact->ExtraState &= ~(EXTRA_FIGHTP | EXTRA_FIGHTS);
                            if( ytd->bact->ExtraState & EXTRA_FIRE ) {

                                state.extra_on  = state.main_state = 0;
                                state.extra_off = EXTRA_FIRE;
                                _methoda( o, YBM_SETSTATE, &state );
                                }
                            }

            /*** Soundbearbeitung, vorerst nicht fuer Autonome ***/
            //if( direct_coll ) {
            //
            //    if( (!(ytd->bact->sc.src[ VP_NOISE_CRASHLAND ].flags & AUDIOF_ACTIVE)) &&
            //        (!(ytd->bact->ExtraState & EXTRA_LANDCRASH )) ) {
            //
            //        ytd->bact->ExtraState |= EXTRA_LANDCRASH;
            //        _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHLAND );
            //        }
            //    }
            //else ytd->bact->ExtraState &= ~EXTRA_LANDCRASH;

            /*** Alles getan.... ***/
            break;

        /*-------------------------------------------------------------------*/

        case ACTION_WAIT:

            /* -------------------------------------------------------------
            ** Solange, bis ein neues Ziel registriert wurde, dann wieder
            ** NORMAL. In dieser Zeit kann man allen möglichen Unsinn machen 
            ** -----------------------------------------------------------*/
            
            if( (ytd->bact->internal_time - ytd->bact->newtarget_time) > 500 ) {

                /*** Kann ich mir das leisten? ***/
                at1.target_type = ytd->bact->SecTargetType;
                at1.priority    = 1;
                at2.target_type = ytd->bact->PrimTargetType;
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
                        target.priority    = 0;
                        target.pos.x       = ytd->bact->pos.x;
                        target.pos.z       = ytd->bact->pos.z;
                        _methoda( o, YBM_SETTARGET, &target );
                        }

                    /*** Ist noch was übriggeblieben? ***/
                    if( (ytd->bact->PrimTargetType != TARTYPE_NONE) ||
                        (ytd->bact->SecTargetType  != TARTYPE_NONE) ) {

                        /*** Ja, also raus aus der Warteschleife ***/
                        state.extra_off  = EXTRA_LANDED;    /* doch */
                        state.extra_on   = 0;
                        state.main_state = ACTION_NORMAL;
                        _methoda( o, YBM_SETSTATE, &state);
                        break;
                        }
                    }
                }

            /* -----------------------------------------------------
            ** die Untergebenen NICHT MEHR ranholen, denn das beisst
            ** sich mit den Waypoints. 
            ** ---------------------------------------------------*/

            /*** Triebwerke aus! ***/
            state.extra_on = state.extra_off = 0;
            state.main_state = ACTION_WAIT;
            _methoda( o, YBM_SETSTATE, &state);
            ytd->bact->act_force = 0;
            
            /*** Auch im Wartezustand können wir fallen! --> entfernte Rakete ***/
            if( !(ytd->bact->ExtraState & EXTRA_LANDED ) ) {

                crash.flags      = CRASH_CRASH;
                crash.frame_time = msg->frame_time;
                _methoda( o, YBM_CRASHBACTERIUM, &crash);
                break;
                }
            else {

                /*** Das dürfen wir doch bloß hier, du Knalltüte! ***/
                ytd->bact->dof.v     = 0;
                }

            /* --------------------------------------------------------
            ** Nun etwas rumgammeln, das heißt drehen. Wir nehmen einen
            ** ExtraWert, denn act_force wird für TipTank benötigt! 
            ** ------------------------------------------------------*/
            if( ytd->wait_count == 0 ) {
                
                ytd->bact->ExtraState &= ~(EXTRA_RIGHT|EXTRA_LEFT);
                ytd->wait_count = msg->frame_time / 5;
                
                /*** Rotieren erwünscht? ***/
                if( ytd->flags & YTF_RotWhileWait ) {

                    switch( (ytd->bact->internal_time + (ULONG)(o) ) % 7 ) {
                        case 0:  ytd->bact->ExtraState |= EXTRA_LEFT;
                                 break;
                        case 1:  ytd->bact->ExtraState |= EXTRA_RIGHT;
                                 break;
                        default: /* nitschewo */;
                                 break;
                        }
                    }
                }
            else {

                if( ytd->bact->ExtraState & EXTRA_LEFT )
                    yt_RotateTank( ytd->bact, (FLOAT) (time / 3) ,0);
                else
                    if( ytd->bact->ExtraState & EXTRA_RIGHT )
                        yt_RotateTank( ytd->bact, (FLOAT) (time / 3), 1 );
                ytd->wait_count--;
                }

            /* Trotzdem: Richte Panzer Aus !!! */
            if( !_methoda( o, YTM_ALIGNVEHICLE_A, &avm ))
                ytd->bact->ExtraState &= ~EXTRA_LANDED;

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
_dispatcher(void, yt_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
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
**      01-Oct-97   floh        Spezial-Joystick-Handling
*/
{
    
    FLOAT  time, angle, b;
    struct move_msg move;
    struct ypatank_data *ytd = INST_DATA(cl,o);
    struct intersect_msg inter;
    ULONG  BACTCOLL;
    struct firemissile_msg fire;
    struct setstate_msg state;
    struct bactcollision_msg bcoll;
    struct crash_msg crash;
    struct alignvehicle_msg avm;
    struct fireyourguns_msg fyg;
    struct visier_msg visier;
    struct flt_triple fdir;
    FLOAT force_mul, abs_force_mul, max_force;

    /*
    ** Wie fährt ein Panzer?
    */

    ytd->bact->air_const = ytd->bact->bu_air_const;

    _get( o, YBA_BactCollision, &BACTCOLL);
    time = (FLOAT) msg->frame_time/1000;
    avm.time    = time;
    avm.old_dir.x = ytd->bact->dir.m31;
    avm.old_dir.y = ytd->bact->dir.m32;
    avm.old_dir.z = ytd->bact->dir.m33;

    switch( ytd->bact->MainState ) {

        case ACTION_NORMAL:
        case ACTION_WAIT:
            
            /*** Untergebene ausbremsen ***/
            if( ytd->bact->dof.v == 0.0 ) {

                /*** Wenn wir in Zielnähe sind, warten wir nur "scheinbar" ! ***/
                if( (ytd->bact->PrimTargetType == TARTYPE_SECTOR) &&
                    (nc_sqrt( (ytd->bact->PrimPos.x - ytd->bact->pos.x ) * 
                              (ytd->bact->PrimPos.x - ytd->bact->pos.x ) +
                              (ytd->bact->PrimPos.z - ytd->bact->pos.z ) * 
                              (ytd->bact->PrimPos.z - ytd->bact->pos.z ) ) >
                             (2 * FOLLOW_DISTANCE)) ) {
                    
                    /*** weit weg ***/
                    if( ytd->bact->ExtraState & EXTRA_FIRE )
                        ytd->bact->MainState = ACTION_WAIT;
                    else {

                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    }
                else {

                    /*** Am Kampfort ***/
                    if( !(ytd->bact->ExtraState & EXTRA_FIRE)) {

                        state.main_state = ACTION_WAIT;
                        state.extra_on = state.extra_off = 0;
                        _methoda( o, YBM_SETSTATE, &state );
                        }
                    ytd->bact->MainState = ACTION_NORMAL;    // !!
                    }
                }
            else {

                if( !(ytd->bact->ExtraState & EXTRA_FIRE )) {

                    state.main_state = ACTION_NORMAL;
                    state.extra_on = state.extra_off = 0;
                    _methoda( o, YBM_SETSTATE, &state );
                    }
                }

            /* ---------------------------------------------------------------
            ** Wieder zuerst die Bewegung. Wir merken uns die alte Position
            ** und berechnen die neue. Dann erst machen wir den Kollisionstest
            ** und setzen evtl. wieder die alte Position.
            ** -------------------------------------------------------------*/

            angle = -msg->input->Slider[ SL_DRV_DIR ] * 
                     ytd->bact->max_rot * time * 2.0;
            
            if( fabs( angle ) > 0.0 ) {

                /*** Ok, drehen wir das Ding um seine lokale (!) y-Achse ***/
                yt_rot_round_lokal_y( ytd->bact, angle );
                }
            
            /* ----------------------------------------------------------------
            ** Es war kein Drehen und kein Abprallen, also fragen wir mal
            ** ab, ob wir beschleunigen oder bremsen sollen. Deshalb lassen wir
            ** auch negative Kräfte zu. Dann fahren wir rückwärts. 
            ** --------------------------------------------------------------*/

            /*** Beschleunigung oder Bremsmanöver? ***/
            //if( ( ( msg->input->Slider[ SL_DRV_SPEED ] < 0 ) &&
            //      ( ytd->bact->act_force > 0 ) ) ||
            //    ( ( msg->input->Slider[ SL_DRV_SPEED ] > 0 ) &&
            //      ( ytd->bact->act_force < 0 ) ) ) {
            if( ( ( ytd->bact->dof.v < 0 ) &&
                  ( ytd->bact->act_force > 0 ) ) ||
                ( ( ytd->bact->dof.v > 0 ) &&
                  ( ytd->bact->act_force < 0 ) ) ) {

                if( fabs( ytd->bact->dof.y ) > IS_SPEED ) {

                    /*** nur dann, weil sonst Probleme mit Stand ***/
                    ytd->bact->dof.v *= max( 0.1, (1 - 4 * time));
                    }
                }

            /*** FIXME_FLOH: spezielles Joystick-Handling ***/
            force_mul     = msg->input->Slider[SL_DRV_SPEED];
            abs_force_mul = fabs(force_mul);
            if (force_mul > 1.0)       force_mul =  1.0;
            else if (force_mul < -1.0) force_mul = -1.0;
            ytd->bact->act_force += 0.75 * time * ytd->bact->max_force * force_mul;
            if (msg->input->Buttons & (1<<31)) {
                /*** Joystick ist aktiviert, funktioniert bissel anders ***/
                max_force = ytd->bact->max_force * abs_force_mul;
            } else {
                max_force = ytd->bact->max_force;
            };              
            if (ytd->bact->act_force > max_force) {
                ytd->bact->act_force = max_force;
            };
            if (ytd->bact->act_force < -max_force) {
                ytd->bact->act_force = -max_force;
            };
            if (fabs(force_mul) > 0.001) ytd->bact->ExtraState |= EXTRA_MOVE;
                       
            /*** Kanone nach oben/unten ***/
            ytd->bact->gun_angle_user += (time * msg->input->Slider[ SL_GUN_HEIGHT ]);
            if( ytd->bact->gun_angle_user < -0.3)
                ytd->bact->gun_angle_user = -0.3;
            if( ytd->bact->gun_angle_user >  0.8)
                ytd->bact->gun_angle_user =  0.8;

            /*** Joystick only! Kanone links und rechts! ***/
            if (msg->input->Buttons & (1<<31)) {
                ytd->bact->gun_leftright  -= (time * msg->input->Slider[ 15 ]);
            };
            if( ytd->bact->gun_leftright < -0.8)
                ytd->bact->gun_leftright = -0.8;
            if( ytd->bact->gun_leftright >  0.8)
                ytd->bact->gun_leftright =  0.8;

            /* --------------------------------------------------------
            ** Deshalb lohnt sich die Abschußrichtung vorher
            ** abzuschätzen, weil berechnen kann man sowas nicht nennen
            ** ------------------------------------------------------*/
            fdir.x = ytd->bact->dir.m31;
            fdir.y = ytd->bact->dir.m32;
            fdir.z = ytd->bact->dir.m33;

            fdir.x -= ytd->bact->dir.m21 * ytd->bact->gun_angle_user;
            fdir.y -= ytd->bact->dir.m22 * ytd->bact->gun_angle_user;
            fdir.z -= ytd->bact->dir.m23 * ytd->bact->gun_angle_user;

            fdir.x -= ytd->bact->dir.m11 * ytd->bact->gun_leftright;
            fdir.y -= ytd->bact->dir.m12 * ytd->bact->gun_leftright;
            fdir.z -= ytd->bact->dir.m13 * ytd->bact->gun_leftright;

            b = nc_sqrt( fdir.x * fdir.x + fdir.y * fdir.y + fdir.z * fdir.z );
            if( b > 0.0001 ) {
                fdir.x /= b; fdir.y /= b; fdir.z /= b; }

            /*** Zielaufnahme, die machen wir immer (vorher default) ***/
            fire.target_type  = TARTYPE_SIMPLE;
            fire.target_pos.x = fdir.x; // ytd->bact->dir.m31;
            fire.target_pos.y = fdir.y; // ytd->bact->dir.m32;
            fire.target_pos.z = fdir.z; // ytd->bact->dir.m33;

            visier.flags = VISIER_ENEMY | VISIER_NEUTRAL;
            visier.dir.x = fdir.x;
            visier.dir.y = fdir.y;
            visier.dir.z = fdir.z;
            if( _methoda( o, YBM_VISIER, &visier ) ) {

                fire.target_type = TARTYPE_BACTERIUM;
                fire.target.bact = visier.enemy;
                }


            /*** Schießen Rakete ***/
            if( (msg->input->Buttons & BT_FIRE ) ||
                (msg->input->Buttons & BT_FIREVIEW ) ) {

                fire.wtype       = ytd->bact->auto_ID;
                fire.dir.x       = fdir.x;
                fire.dir.y       = fdir.y;
                fire.dir.z       = fdir.z;

                fire.global_time = ytd->bact->internal_time;
                /*** Tank. etwas höher ***/
                if( ytd->bact->internal_time % 2 == 0 )
                    fire.start.x =  ytd->bact->firepos.x;
                else
                    fire.start.x = -ytd->bact->firepos.x;
                fire.start.y     = ytd->bact->firepos.y;
                fire.start.z     = ytd->bact->firepos.z;
                /*** Viewer? ***/
                if( msg->input->Buttons & BT_FIREVIEW )
                    fire.flags   = FIRE_VIEW;
                else
                    fire.flags   = 0;
                fire.flags      |= FIRE_CORRECT;
                _methoda( o, YBM_FIREMISSILE, &fire );
                }

            if( ytd->bact->mg_shot != NO_MACHINEGUN ) {

                /*** Schießen, Kanone ***/
                if( (ytd->bact->ExtraState & EXTRA_FIRE) &&
                    (!(msg->input->Buttons & BT_FIREGUN)) ) {

                    /*** Feuern aus ***/
                    // Abmelden beim WO

                    state.main_state = state.extra_on = 0;
                    state.extra_off  = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                if( msg->input->Buttons & BT_FIREGUN ) {

                    /*** Feuern einschalten? ***/
                    if( !(ytd->bact->ExtraState & EXTRA_FIRE) ) {

                        // Anmelden WO

                        state.main_state = state.extra_off = 0;
                        state.extra_on   = EXTRA_FIRE;
                        _methoda( o, YBM_SETSTATE, &state );
                        }

                    /*** Feuern an sich ***/
                    fyg.dir.x = fdir.x;
                    fyg.dir.y = fdir.y;
                    fyg.dir.z = fdir.z;
                    fyg.time  = time;
                    fyg.global_time = ytd->bact->internal_time;
                    _methoda( o, YBM_FIREYOURGUNS, &fyg );
                    }
                }


            /* ----------------------------------
            ** Fallen wir? Dann laßt uns fallen !
            ** --------------------------------*/
            if( !(ytd->bact->ExtraState & EXTRA_LANDED ) ) {

                /*** ein Landen gibts beim Panzer nicht! ***/
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

                ytd->bact->act_force = 0.0;

                if( fabs( ytd->bact->dof.v ) < NO_SPEED ) {
                    ytd->bact->ExtraState &= ~EXTRA_MOVE;
                    ytd->bact->dof.v = 0.0;
                    }
                else
                    ytd->bact->dof.v *= max( 0.1, (1 - 4 * time) );

                move.flags = MOVE_STOP;
                }
            else  move.flags = 0;

            
            /* ------------------------------------------------------
            ** So, nun fahren wir. Fahrtrichtung ist immer die lokale
            ** z-Achse.
            ** ----------------------------------------------------*/

            move.t = (FLOAT) msg->frame_time / 1000;
            if( ytd->bact->ExtraState & EXTRA_MOVE )
                _methoda( o, YBM_MOVE, &move );

            
            
            /*  -----------------------------------
            **  direkte Kollision mit den Bakterien
            **  ---------------------------------*/
            if( BACTCOLL ) {

                bcoll.frame_time = msg->frame_time;
                if( _methoda( o, YBM_BACTCOLLISION, &bcoll) ) {
                    _methoda( o, YTM_ALIGNVEHICLE_U, &avm );
                    return;
                    }
                }

            /*
            ** Andernfalls kann es immer noch passieren, daß wir unter
            ** die Welt geknallt sind.
            */
            inter.pnt.x = ytd->bact->old_pos.x;
            inter.pnt.y = ytd->bact->old_pos.y;
            inter.pnt.z = ytd->bact->old_pos.z;
            inter.vec.x = ytd->bact->pos.x - ytd->bact->old_pos.x;
            inter.vec.y = ytd->bact->pos.y - ytd->bact->old_pos.y;
            inter.vec.z = ytd->bact->pos.z - ytd->bact->old_pos.z;
            inter.flags = 0;
            _methoda( ytd->world, YWM_INTERSECT, &inter );
            if( inter.insect ) {

                ytd->bact->pos = ytd->bact->old_pos;
                ytd->bact->dof.v     = 0.0;
                ytd->bact->act_force = 0.0;
                return;
                }

            /* --------------------------------------------------------------------
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
            ** ------------------------------------------------------------------*/

            _methoda( o, YTM_ALIGNVEHICLE_U, &avm);

            break;

        case ACTION_DEAD:
            
            //if( !(ytd->bact->ExtraState & EXTRA_LANDED ) ) {
            //
            //    crash.flags      = CRASH_CRASH;
            //    crash.frame_time = msg->frame_time;
            //    _methoda( o, YBM_CRASHBACTERIUM, &crash);
            //    }
            //else {
            //
            //    /* 
            //    ** Wir sind gelandet und schalten noch den MegaTötler ein,
            //    ** wenn vorher vp_dead war! Dann geht das Objekt ans WO! 
            //    */
            //
            //    LONG YLS;
            //
            //    state.extra_on  = EXTRA_MEGADETH;
            //    state.extra_off = state.main_state = 0;
            //    _methoda( o, YBM_SETSTATE, &state);
            //
            //    /*** Rückgabe des Leergutes erst nach "Automatisierung" ***/
            //    _get( o, YBA_YourLS, &YLS );
            //    if( YLS <= 0 ) ytd->bact->ExtraState |= EXTRA_DONTRENDER;
            //    }
            
            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;
        }
    return;
    
}


_dispatcher( void, yt_YBM_REINCARNATE, void *nix )
{
/*
**  FUNCTION    Uffräume
**
*/

    struct ypatank_data *ytd;

    ytd = INST_DATA(cl, o);

    /*** nach oben ***/
    _supermethoda( cl, o, YBM_REINCARNATE, NULL);

    /*** auch sinnvolle Voreinstellungen ***/
    ytd->wait_count = 0;
}


_dispatcher( BOOL, yt_YBM_TESTSECTARGET, struct Bacterium *b )
{
/*
**  FUNCTION    Testet, ob das übergebene Bacterium für
**              mich als Nebenziel in Frage kommt.
**
**  INPUT       das eventuelle Ziel
**
**  RESULT      TRUE, wenn ok
*/

    struct ypatank_data *ytd;

    ytd = INST_DATA( cl, o );

    /* --------------------------------------------------------
    ** Ok, der erste Test: Ich bin Tank oder Car, kann also
    ** Ziele hinter Abgründen, die nicht zu mir kommen koennen,
    ** nicht bearbeiten.
    ** DiagonalSektoren
    ** NEU: Tanks ignorieren nun alle Nebenziele, die sie
    ** nicht erreichen koennen!
    ** ------------------------------------------------------*/
//    if( (BCLID_YPAGUN  == b->BactClassID) ||
//        (BCLID_YPACAR  == b->BactClassID) ||
//        (BCLID_YPATANK == b->BactClassID) ||
//        (BCLID_YPAROBO == b->BactClassID) ) {

        /*** DiagonalSektoren unterscheiden ***/
        if( (ytd->bact->SectX != b->SectX) &&
            (ytd->bact->SectY != b->SectY) ) {

            /*
            ** Folgende Faelle unterscheiden:
            **      *     | *      *       | *
            **  -----     |      --|       ---
            **  o       o |      o |     o
            */

            struct Cell *ixiy, *ixdy, *dxiy, *dxdy;
            struct getsectorinfo_msg gsi;

            ixiy = ytd->bact->Sector;
            dxdy = b->Sector;

            gsi.abspos_x = ytd->bact->pos.x;
            gsi.abspos_z = b->pos.z;
            _methoda( ytd->world, YWM_GETSECTORINFO, &gsi );
            ixdy = gsi.sector;

            gsi.abspos_x = b->pos.x;
            gsi.abspos_z = ytd->bact->pos.z;
            _methoda( ytd->world, YWM_GETSECTORINFO, &gsi );
            dxiy = gsi.sector;

            if( (fabs(ixiy->Height - ixdy->Height) >= SPV_WallHeight) &&
                (fabs(dxiy->Height - dxdy->Height) >= SPV_WallHeight) )
                return( FALSE );

            if( (fabs(ixiy->Height - dxiy->Height) >= SPV_WallHeight) &&
                (fabs(ixdy->Height - dxdy->Height) >= SPV_WallHeight) )
                return( FALSE );

            if( (fabs(ixiy->Height - ixdy->Height) >= SPV_WallHeight) &&
                (fabs(ixiy->Height - dxiy->Height) >= SPV_WallHeight) )
                return( FALSE );

            if( (fabs(ixdy->Height - dxdy->Height) >= SPV_WallHeight) &&
                (fabs(dxiy->Height - dxdy->Height) >= SPV_WallHeight) )
                return( FALSE );
            }
        else {

            FLOAT hdist;

            hdist = fabs( ytd->bact->Sector->Height - b->Sector->Height );
            if( hdist >= SPV_WallHeight )
                return( FALSE );
            }
//        }

    return( TRUE );
}


/*
** Die diverstesten Routinen, die wir so brauchen.
*/

void yt_m_mul_v(struct flt_m3x3 *m, struct flt_triple *v1, struct flt_triple *v)
{
/*
**  FUNCTION
**      Multipliziert Vector mit matrix.
**
**      v = m1*v1
*/

    v->x = m->m11*v1->x + m->m12*v1->y + m->m13*v1->z;
    v->y = m->m21*v1->x + m->m22*v1->y + m->m23*v1->z;
    v->z = m->m31*v1->x + m->m32*v1->y + m->m33*v1->z;
}

void yt_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
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


void yt_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
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


void yt_rot_vec_round_lokal_y( struct flt_triple *vec, FLOAT angle)
{

    struct flt_m3x3 rm;
    struct flt_triple neu_vec;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    
    yt_m_mul_v( &rm, vec, &neu_vec);

    *vec = neu_vec;
}


