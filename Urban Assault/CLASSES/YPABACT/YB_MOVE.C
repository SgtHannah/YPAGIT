/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Ballerroutinen!!!!!!!!!!!!!!
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
#include "input/input.h"
#include "audio/audioengine.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine


void yb_DrehObjektInFlugrichtung( struct ypabact_data *ybd, ULONG t );
void yb_CalculateForce( struct ypabact_data *ybd, struct trigger_logic_msg *msg );
void yb_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle);
void yb_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
void yb_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle);
void yb_RichteObjektAus( struct ypabact_data *ybd, struct flt_triple *vec,
                         FLOAT time);
void yb_DoSpecialEffect( struct ypabact_data *ybd, LONG frame_time );
void yb_RandomSpeed( struct ypabact_data *ybd );



/*-----------------------------------------------------------------*/
_dispatcher(void, yb_YBM_MOVE, struct move_msg *move)
{
/*
**  FUNCTION
**
**      Realisiert die Bewegung. Um die geschwindigkeit zu ändern,
**      ist eine Kraft notwendig. Die Schubkraft steht in der
**      übergebenen power_msg, in der auch noch die frametime
**      steht. Ich möchte festlegen, daß die Bewegung und Drehung des
**      Objektes ausschließlich in Move gemacht wird. Sonst hat der
**      Export in eine ExtraMethode wahrscheinlich wenig Sinn...
**
**      Deshalb drehe ich auch erst hier das Objekt, damit new_dof
**      etwas damit anfangen kann. dadurch kann der Rechenaufwand
**      für die Handsteuerung steigen, aber wenigstens ist die
**      Schnittstelle zwischen Logik und bewegung klar definiert:
**      Kraft.
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

    struct ypabact_data *ybd;


    struct uvec grav;       /* Gravitations-Kraftvektor */
    struct uvec schub;      /* Schub-Kraftvektor */
    struct uvec lw;         /* Luftwiederstand */
    struct uvec res;
    FLOAT delta_betrag, new_v_x, new_v_y, new_v_z;
    FLOAT factor, max;

    ybd = INST_DATA( cl, o);

    /*
    ** Die alte Pos merken, wo sie verändert wird!! also hier!! sonst
    ** bekommt Kollisionsverhütung Probleme!!!!
    */
    ybd->bact.old_pos = ybd->bact.pos;


    /* -------------------------------------------------------------
    ** Gravitation wirkt immer nach unten. Weil die Optik mit der
    ** geometrie in keinster Weise übereinstimmt, müssen wir mit
    ** einem Hack nachhelfen: Sind wir tot, so vervielfachen wir die
    ** GRAVITY...
    ** -----------------------------------------------------------*/
    grav.x = 0.0;
    grav.y = 1.0;
    grav.z = 0.0;
    if( ACTION_DEAD == ybd->bact.MainState )
        grav.v = GRAV_FACTOR * GRAVITY * ybd->bact.mass;
    else
        grav.v = GRAVITY * ybd->bact.mass;

    /*** Schub wirkt in Richtung negative lokale Y-Achse ***/
    if( move->flags & MOVE_NOFORCE ) {

        schub.x = 0.0;
        schub.y = 0.0;
        schub.z = 0.0;
        schub.v = 0.0;
        }
    else {

        schub.x = -(ybd->bact.dir.m21);
        schub.y = -(ybd->bact.dir.m22);
        schub.z = -(ybd->bact.dir.m23);
        schub.v = ybd->bact.act_force;

        /*** User-Korrektur ***/
        if( ybd->flags & YBF_UserInput ) {

            /*** Am Anfang Schub etwas staerker ***/
            schub.x  = nc_sgn( schub.x ) * nc_sqrt( fabs( schub.x ) );
            schub.z  = nc_sgn( schub.z ) * nc_sqrt( fabs( schub.z ) );
            schub.y  = nc_sgn( schub.y ) * schub.y * schub.y;
            }
        }

    /*** Luftwiderstand ***/
    lw.x = -ybd->bact.dof.x;
    lw.y = -ybd->bact.dof.y;
    lw.z = -ybd->bact.dof.z;
    lw.v = ybd->bact.dof.v * ybd->bact.air_const;

    /*** resultierender Kraftvektor ***/
    res.x = (grav.x*grav.v)+(schub.x*schub.v)+(lw.x*lw.v);
    res.y = (grav.y*grav.v)+(schub.y*schub.v)+(lw.y*lw.v);
    res.z = (grav.z*grav.v)+(schub.z*schub.v)+(lw.z*lw.v);
    res.v = nc_sqrt(res.x*res.x + res.y*res.y + res.z*res.z);
    
    /*** bei UserInput unabhaengig vom Vorzeichen y-Komponnete staerker ***/
    if( ybd->flags & YBF_UserInput )
        res.y *= 2.0;

    /*** neuer dof - zum alten aufaddieren ***/
    if( res.v > 0.0 ) {

        delta_betrag = (res.v / ybd->bact.mass) * move->t;
        new_v_x = ybd->bact.dof.x * ybd->bact.dof.v + (res.x / res.v ) * delta_betrag;
        new_v_y = ybd->bact.dof.y * ybd->bact.dof.v + (res.y / res.v ) * delta_betrag;
        new_v_z = ybd->bact.dof.z * ybd->bact.dof.v + (res.z / res.v ) * delta_betrag;

        ybd->bact.dof.v = nc_sqrt( new_v_x * new_v_x + new_v_y * new_v_y +
                                new_v_z * new_v_z );
        if( ybd->bact.dof.v > 0.0 ) {
            ybd->bact.dof.x = new_v_x / ybd->bact.dof.v;
            ybd->bact.dof.y = new_v_y / ybd->bact.dof.v;
            ybd->bact.dof.z = new_v_z / ybd->bact.dof.v;
            }
        }


    /* ------------------------------------------------------------------
    ** Jetzt haben wir eine Neue Geschwindigkeit, da müssen wir aber noch
    ** die Position neu setzen. Die Ausrichtung wurde schon erledigt...
    ** Alle Rundungen bei niedriger Geschwindigkeit machen wir hier!
    ** ----------------------------------------------------------------*/
    if( fabs(ybd->bact.dof.v) > IS_SPEED ) {

        ybd->bact.pos.x += ybd->bact.dof.x * ybd->bact.dof.v * move->t * METER_SIZE;
        ybd->bact.pos.y += ybd->bact.dof.y * ybd->bact.dof.v * move->t * METER_SIZE;
        ybd->bact.pos.z += ybd->bact.dof.z * ybd->bact.dof.v * move->t * METER_SIZE;
        }

    /*** Nun, lasset uns noch testen, ob wir nicht über die Welt geflogen sind ***/
    _methoda( o, YBM_CORRECTPOSITION, NULL );

    /*** Alte Sachen vor Veränderung ***/
    ybd->bact.sc.src[ VP_NOISE_NORMAL ].pitch  = ybd->bact.bu_pitch;
    ybd->bact.sc.src[ VP_NOISE_NORMAL ].volume = ybd->bact.bu_volume;


    /*** Ich mache die Tonhöhe mal nur von der act_force abhängig ***/
    if( ybd->bact.max_pitch > -0.8 )
        max = ybd->bact.max_pitch;
    else
        max = MSF_HELICOPTER;

    factor = max * fabs( ybd->bact.dof.v ) /
                  ( nc_sqrt( ybd->bact.max_force * ybd->bact.max_force -
                    100 * ybd->bact.mass * ybd->bact.mass) / ybd->bact.bu_air_const);
    if( factor > max ) factor = max;

    if( ybd->bact.sc.src[ VP_NOISE_NORMAL ].sample )
        ybd->bact.sc.src[ VP_NOISE_NORMAL ].pitch += (LONG)
            ( ((FLOAT) ybd->bact.sc.src[ VP_NOISE_NORMAL ].pitch +
               (FLOAT) ybd->bact.sc.src[ VP_NOISE_NORMAL ].sample->SamplesPerSec) *
               factor);

}



_dispatcher( BOOL, yb_YBM_CRASHBACTERIUM, struct crash_msg *crash)
{
/*
**  FUNCTION    Läßt ein Objekt nach Wunsch Landen oder Abstürzen.
**              Dabei kann es das Objekt abrutschen lassen. Solange
**              wir nicht ruhig liegen, geben wir FALSE zurück, andern-
**              falls TRUE. Außerdem setzen wir dann das LANDED-Flag.
**              Wenn wir landen, wird das Objekt ausgerichtet.
**              Für den Fall des Aufprallens bei MegaDeth (was bei
**              mehreren Durchläufen passieren kann), richten wir uns
**              am Untergrund aus.
**
**  INPUT       CRASH_CRASH     wirklich abstürzen, andernfall landen
**              CRASH_SPIN      Trudeln bei Absturz
**
**  RESULTS     TRUE, wenn wir ruhig liegen
**
**  CHANGED     16-Oct-95   created 8100 000C
*/


    struct ypabact_data *ybd;
    FLOAT over_eof, radius, betrag, ca, sa, angle;
    FLOAT time, rot_x, rot_y, rot_z;
    FLOAT A = 0.0, B = 0.0, C = 0.0, spin;
    struct intersect_msg inter;
    struct move_msg move;
    struct flt_triple vec, *bpos, *bold;
    struct flt_m3x3 nm, rm;
    struct insphere_msg ins;
    struct Insect insect[10];
    struct setstate_msg state;
    struct bactcollision_msg bcoll;
    struct recoil_msg rec;
    Object *VP;
    FLOAT  LAND_B;
    BOOL   sphere_coll = FALSE, coll, Bodenfahrzeug;
    LONG   maxcollisioncounts;

    ybd = INST_DATA(cl, o );

    /*** SpcialEffects sind extra ***/
    if( ybd->bact.ExtraState & EXTRA_SPECIALEFFECT ) {

        yb_DoSpecialEffect( ybd, crash->frame_time);
        return( FALSE );
        }

    /*** Einige Vorbereitungen ***/
    if(ybd->flags & YBF_Viewer) {
        radius   = ybd->bact.viewer_radius;
        over_eof = ybd->bact.viewer_over_eof;
        }
    else {
        radius   = ybd->bact.radius;
        over_eof = ybd->bact.over_eof;
        } 
        
    /* -----------------------------------------------
    ** ein abgestuerzter Robo braucht ein hartgec.
    ** overeof, weil oeof schon anders verwendet wird.
    ** ---------------------------------------------*/
    if( BCLID_YPAROBO == ybd->bact.BactClassID )
        over_eof = 60;   

    time = (FLOAT) crash->frame_time / 1000.0;
    bpos = &(ybd->bact.pos);
    bold = &(ybd->bact.old_pos);
    VP   = ybd->vis_proto;

    /* ----------------------------------------------------------------
    ** Panzer bleiben auch auf steilerebn Flächen liegen, wenn sie noch 
    ** nicht tot sind. (Sehr un-OOPisch, naja... )
    ** --------------------------------------------------------------*/
    if( (ybd->bact.BactClassID == BCLID_YPACAR) ||
        (ybd->bact.BactClassID == BCLID_YPAHOVERCRAFT) ||
        (ybd->bact.BactClassID == BCLID_YPATANK) )
        Bodenfahrzeug = TRUE;
    else
        Bodenfahrzeug = FALSE;

    //if( (ybd->bact.MainState != ACTION_DEAD) &&
    //    Bodenfahrzeug )                        es ist unsinn, einen Panzer auf einer Flaeche
    //    LAND_B = YPS_PLANE;            abrutschen zu lassen, wo er stehen kann. denn
    //else                                                       dof ist dafuer nicht zurechtgebogen
    //    LAND_B = 0.707;
        LAND_B = YPS_PLANE;


    /* 
    ** Los geht es! Wenn wir landen sollen, dann drehen wir das Objekt,
    ** damit es gerade aufsetzt. rot ist die Drehachse.
    */

    rot_x = -ybd->bact.dir.m23;
    rot_y = 0;
    rot_z = ybd->bact.dir.m21;
    betrag = nc_sqrt( rot_x*rot_x + rot_z*rot_z );
    
    if( ( betrag > 0.001 ) && (!(crash->flags & CRASH_CRASH)) ) {

        /*** Das Objekt ist noch nicht gerade, also ausrichten ***/
        rot_x /= betrag;        rot_z /= betrag;
        
        angle = nc_acos( ybd->bact.dir.m22 );
        if( angle > ( ybd->bact.max_rot * time ) )
                      angle = ybd->bact.max_rot * time;

        if( fabs( angle ) > 0.02 )  {

            /*** nur dann lohnt sich die Rotation ***/
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

            nc_m_mul_m( &(ybd->bact.dir), &rm, &nm );

            ybd->bact.dir = nm;
            }

        }

    /*** Wird während des Abstürzens ein Trudeln des Objektes verlangt? ***/
    if( crash->flags & CRASH_SPIN ) {

        /*** Trudeln lassen. (Rotation um die lokale x-Achse) ***/
        spin  = fabs( ybd->bact.dof.v );
        spin *= time;
        spin *= 0.08;
        yb_rot_round_lokal_z( &(ybd->bact), spin );
        }


    /*
    ** Nun runter mit dem Gerät!
    */

    if( !( ybd->bact.ExtraState & EXTRA_LANDED ) ) {

        /*
        ** Landen: hohe Luftdichte, Crash: geringe Luftdichte,
        ** dann nur noch fallen lassen! 
        */
        if( crash->flags & CRASH_CRASH )
            ybd->bact.air_const = 0;
        else
            ybd->bact.air_const = 500;

        maxcollisioncounts = 3;
        while( maxcollisioncounts-- ) {

            /*** Ganz normal an Move übergeben, frei von Antriebskräften ***/
            move.t     = time;
            move.flags = MOVE_NOFORCE;
            _methoda( o, YBM_MOVE, &move);

            coll = FALSE;

            /*
            ** KOLLISIONEN
            ** -----------
            ** Als erstes kann es passieren, daß wir mit einem Bacterium zamprallen 
            */
            if( ybd->flags & YBF_BactCollision ) {
                bcoll.frame_time = crash->frame_time;
                if( _methoda( o, YBM_BACTCOLLISION, &bcoll)) {

                    if( (BCLID_YPATANK == ybd->bact.BactClassID) ||
                        (BCLID_YPACAR  == ybd->bact.BactClassID) )
                        yb_RandomSpeed( ybd );
                    return( FALSE );
                    }
                }

            /* ------------------------------------------------------------------
            ** Ein Kugeltest für die Kollision. Dieser wird nur gemacht, wenn ich
            ** Viewer bin. Für autonome Objekte reicht die normale Intersection
            ** ----------------------------------------------------------------*/
            
            if( ybd->flags & YBF_Viewer ) {

                ins.pnt.x  = bpos->x +
                             ybd->bact.dof.x * ybd->bact.dof.v * time * METER_SIZE;
                ins.pnt.y  = bpos->y +
                             ybd->bact.dof.y * ybd->bact.dof.v * time * METER_SIZE;
                ins.pnt.z  = bpos->z +
                             ybd->bact.dof.z * ybd->bact.dof.v * time * METER_SIZE;
                ins.dof.x  = ybd->bact.dof.x;
                ins.dof.y  = ybd->bact.dof.y;
                ins.dof.z  = ybd->bact.dof.z;
                ins.radius = radius;
                ins.chain  = insect;
                ins.flags  = 0;
                ins.max_insects = 10;

                _methoda( ybd->world, YWM_INSPHERE, &ins );

                if( ins.num_insects > 0 ) {

                    /*
                    ** Es trat eine Kollision auf! Wir summieren alle Normalen-
                    ** vektoren der Ebenen auf und prallen von dort ab. Dann gehen
                    ** wir mit großer wait_time sofort zurück.
                    */
                    BOOL LandFound = FALSE;

                    sphere_coll = TRUE;

                    A = 0;
                    B = 0;
                    C = 0;

                    while( ins.num_insects-- ) {

                        A += ins.chain[ ins.num_insects ].pln.A;
                        B += ins.chain[ ins.num_insects ].pln.B;
                        C += ins.chain[ ins.num_insects ].pln.C;

                        if( B > YPS_PLANE ) LandFound = TRUE;
                        }

                    betrag = nc_sqrt( A*A + B*B + C*C );
                    
                    if( betrag == 0) {

                        /*
                        ** Was komischerweise scheinbar vorkommt...
                        */
                        vec.x = rec.vec.x = ybd->bact.dof.x;
                        vec.y = rec.vec.y = ybd->bact.dof.y;
                        vec.z = rec.vec.z = ybd->bact.dof.z;
                        }
                    else {

                        vec.x = rec.vec.x = A / betrag;
                        vec.y = rec.vec.y = B / betrag;
                        vec.z = rec.vec.z = C / betrag;
                        }
                    
                    /*** Es gab eine "Berührung". ***/
                    if( crash->flags & CRASH_CRASH ) {

                        /* ------------------------------------------
                        ** Ein würdiger richtiger Absturz. Wir ziehen 
                        ** zuerst Energie ab 
                        ** ----------------------------------------*/
                        ybd->bact.Energy -= fabs(ybd->bact.dof.v) * 10;
                
                        /*** Trümmerhaufen, wenn tot oder Aufprall ***/
                        if( (ybd->bact.Energy <= 0) ||
                            ((VP == ybd->bact.vis_proto_dead) &&
                             (ybd->bact.MainState == ACTION_DEAD)) ) {

                            /*** Wenn er bloß so runtergeknallt ist ***/
                            state.extra_on = EXTRA_MEGADETH;
                            state.main_state = state.extra_off = 0;
                            _methoda( o, YBM_SETSTATE, &state);
                            }
                        
                        /* -----------------------------------------------
                        ** Wenn die Fläche in etwa gerade war, bleiben wir
                        ** liegen, sonst prallen wir ab 
                        ** Dabei vergessen wir nicht den Abprall-Zähler
                        ** Auf jeden Fall knallts
                        ** ---------------------------------------------*/

                        if( ybd->flags & YBF_UserInput ) {

                            struct yw_forcefeedback_msg ffb;

                            /*** Dös Geräusch (nur User) ***/
                            if( fabs( ybd->bact.dof.v ) > NOISE_SPEED )
                                _StartSoundSource( &(ybd->bact.sc), VP_NOISE_CRASHLAND );

                            ffb.type    = YW_FORCE_COLLISSION;
                            ffb.power   = 1.0;

                            /*** Trick: Pos + Hindernisrichtung ***/
                            ffb.dir_x   = ybd->bact.pos.x + 10 * A;
                            ffb.dir_y   = ybd->bact.pos.z + 10 * C;
                            _methoda( ybd->world, YWM_FORCEFEEDBACK, &ffb );
                            }
                        
                        /* --------------------------------------------------
                        ** Liegenbleiben, wenn flache Fläche und Speed gering
                        ** ist.
                        ** ------------------------------------------------*/
                        if( (B >= LAND_B) && LandFound ) {

                            bpos->y = bold->y;
                            ybd->bact.ExtraState |= EXTRA_LANDED;
                            ybd->bact.dof.v *= nc_sqrt( ybd->bact.dof.x*ybd->bact.dof.x +
                                                        ybd->bact.dof.z*ybd->bact.dof.z);
                            yb_RichteObjektAus( ybd, &vec, time);

                            ybd->bact.reccount = 0;
                            }
                        else {

                            /*** Abprallen ***/
                            coll = TRUE;
                            rec.mul_y = 0.7;  rec.mul_v = 0.7;  rec.time = time;
                            _methoda( o, YBM_RECOIL, &rec);

                            /*** Abprall-Ende-Test ***/
                            ybd->bact.reccount++;
                            if( ybd->bact.reccount > 50 ) {

                                /*** Abtöten ***/
                                ybd->bact.ExtraState |= EXTRA_LANDED;
                                ybd->bact.Energy = -10000;
                                }
                            }
                        }
                    else {
                        
                        /*** Der Versuch einer Landung ***/
                        if( B >= LAND_B ) {

                            bpos->y = bold->y;
                            ybd->bact.dof.v = 0.0;
                            ybd->bact.ExtraState |= EXTRA_LANDED;

                            ybd->bact.reccount = 0;
                            }
                        else {

                            coll = TRUE;
                            rec.mul_y = 2.0;  rec.mul_v = 0.7;  rec.time = time;
                            _methoda( o, YBM_RECOIL, &rec);
                            }
                        }
                    }
                }

            /* ------------------------------------------------------------------
            ** Ende Kugelkollision. Die Intersection machen wir, wenn keine
            ** Kugelkollision auftrat (denn nur wenn "Viewer" konnte die Variable
            ** auf TRUE gesetzt werden!)
            ** ----------------------------------------------------------------*/
            if( !sphere_coll ) {

                /*** Keine Kugelkollision oder NichtViewer ***/
                inter.pnt.x = bold->x;
                inter.pnt.y = bold->y;
                inter.pnt.z = bold->z;
                inter.vec.x = bpos->x - bold->x;
                inter.vec.z = bpos->z - bold->z;
                inter.vec.y = bpos->y - bold->y + over_eof;
                inter.flags = 0;
                _methoda( ybd->world, YWM_INTERSECT, &inter);

                if( inter.insect ) {
                        
                    vec.x = rec.vec.x = inter.sklt->PlanePool[ inter.pnum ].A;
                    vec.y = rec.vec.y = inter.sklt->PlanePool[ inter.pnum ].B;
                    vec.z = rec.vec.z = inter.sklt->PlanePool[ inter.pnum ].C;
                        
                    if( crash->flags & CRASH_CRASH ) {

                        /* ------------------------------------------
                        ** Ein Absturz. Wir ziehen zuerst Energie ab,
                        ** wenn der Aufprall hart war 
                        ** FALSCH, kein Test mehr, weil stehende bei
                        ** Abschuß sonst nicht richtig explodieren
                        ** ----------------------------------------*/

                        ybd->bact.Energy -= fabs(ybd->bact.dof.v) * 10;

                        /* -------------------------------------------------------
                        ** Wenn er stark aufgeknallt ist oder schon tot war. Wegen
                        ** der möglichen Visproto-Überschneidung auch ACTION_DEAD
                        ** mit testen. 
                        ** -----------------------------------------------------*/
                        if( (ybd->bact.Energy <= 0) ||
                            ((VP == ybd->bact.vis_proto_dead) &&
                             (ybd->bact.MainState == ACTION_DEAD)) ) {

                            state.extra_on = EXTRA_MEGADETH;
                            state.main_state = state.extra_off = 0;
                            _methoda( ybd->bact.BactObject, YBM_SETSTATE, &state);
                            }
                        
                        /* -----------------------------------------------
                        ** Wenn die Fläche in etwa gerade war, bleiben wir
                        ** liegen, sonst prallen wir ab 
                        ** ---------------------------------------------*/

                        if( ybd->flags & YBF_UserInput ) {

                            struct yw_forcefeedback_msg ffb;
                        
                            /*** Dös Geräusch (evtl. mal in oberes if !!) ***/
                            if( fabs( ybd->bact.dof.v ) > NOISE_SPEED )
                                _StartSoundSource( &(ybd->bact.sc), VP_NOISE_CRASHLAND );

                            ffb.type    = YW_FORCE_COLLISSION;
                            ffb.power   = 1.0;

                            /*** Trick: Pos + Hindernisrichtung ***/
                            ffb.dir_x   = ybd->bact.pos.x + 10 * vec.x;
                            ffb.dir_y   = ybd->bact.pos.z + 10 * vec.z;
                            _methoda( ybd->world, YWM_FORCEFEEDBACK, &ffb );
                            }
                        
                        /* --------------------------------------------------
                        ** Liegenbleiben, wenn flache Fläche und Speed gering
                        ** ist.
                        ** ------------------------------------------------*/
                        if( inter.sklt->PlanePool[ inter.pnum ].B >= LAND_B) {

                                                        bpos->x = inter.ipnt.x;
                                                        bpos->z = inter.ipnt.z;
                            bpos->y = inter.ipnt.y - over_eof;
                            ybd->bact.ExtraState |= EXTRA_LANDED;
                            ybd->bact.dof.v *= nc_sqrt( ybd->bact.dof.x*ybd->bact.dof.x +
                                                        ybd->bact.dof.z*ybd->bact.dof.z);
                            yb_RichteObjektAus( ybd, &vec, time);
                            
                            ybd->bact.reccount = 0;
                            }
                        else {
                            
                            /*
                            **  Abprallen geht nicht, weil Trümmerhaufen nicht fliegen.
                            ** Wir müssen aber v ausrichten. Deshalb rufen wir PrallAb
                            ** auf und projizieren v in die Ebene. Dazu ändere ich nur
                            ** dof.y OHNE zu normieren 
                            */
                            coll = TRUE;
                            rec.mul_y = 0.7;  rec.mul_v = 0.7;  rec.time = time;
                            _methoda( o, YBM_RECOIL, &rec);

                            /*** Abprall-Ende-Test ***/
                            ybd->bact.reccount++;
                            if( ybd->bact.reccount > 50 ) {

                                /*** Abtöten ***/
                                ybd->bact.ExtraState |= EXTRA_LANDED;
                                ybd->bact.Energy = -10000;
                                }
                            }
                        }
                    else {
                        
                        /*** Der Versuch einer Landung ***/
                        if( inter.sklt->PlanePool[ inter.pnum ].B >= LAND_B ) {

                            bpos->y = inter.ipnt.y - over_eof;
                            ybd->bact.dof.v = 0.0;
                            ybd->bact.ExtraState |= EXTRA_LANDED;
                            ybd->bact.reccount = 0;
                            }
                        else {

                            coll = TRUE;
                            rec.mul_y = 2.0;  rec.mul_v = 0.7;  rec.time = time;
                            _methoda( o, YBM_RECOIL, &rec);
                            }
                        }
                    }
                }
            if( !coll ) break;
            }
        }

    /*
    ** Wenn es uns (mehr oder weniger unbeschadet) gelungen ist zu landen,
    ** dann geben wir TRUE zurück.
    */

    if( ybd->bact.ExtraState & EXTRA_LANDED )
        return( TRUE );
    else
        return( FALSE );
}


_dispatcher( void, yb_YBM_RECOIL, struct recoil_msg *rec)
{
/*
**  FUNCTION    Läßt ein Objekt von einer Wand, deren Normalenvektor
**              übergeben wurde, abprallen.
**              Wir prallen aber nur ab, wenn der dof in Richtung des Hindernisses
**              zeigt.
**
**  INPUT       vec   - der Normalenvektor
**              mul_v - damit wird dof.v multipliziert
**              mul_y - damit wird dof.y mult, wenn es negativ ist und dadurch
**                      dividiert, wenn es positiv ist --> Abrutscheffekt
**                      0.0 < mul_y < 1.0
**              time  - FLOAT
**
**  RESULTS     Nitschewo
**
**  CHANGED     18-Oct-95 af created
**              18-Feb-96 af Richtung mit beachten
**               5-Apr-96 af geschwindigkeitsabhängiger Abprallfaktor
**                           und Colltest nach außen verlagert....
**                           Abprall nur noch v
*/

    struct ypabact_data *ybd;
    FLOAT  A,B,C,doppel_cos, new_betrag, rv;

    ybd = INST_DATA(cl, o );

    if( ybd->bact.ExtraState & EXTRA_LANDED ) return;

    A = rec->vec.x;
    B = rec->vec.y;
    C = rec->vec.z;

    if( ((ybd->bact.dof.x * A + ybd->bact.dof.y * B + ybd->bact.dof.z * C) < 0.0 ) ||
        (ybd->bact.dof.v == 0.0) )
        return;

    /*** Och, ne? ***/
    ybd->bact.pos = ybd->bact.old_pos;

    new_betrag = ybd->bact.dof.x * ybd->bact.dof.v * A +
                 ybd->bact.dof.y * ybd->bact.dof.v * B +
                 ybd->bact.dof.z * ybd->bact.dof.v * C;
        
    doppel_cos = 2*( A * ybd->bact.dof.x + B * ybd->bact.dof.y + 
                     C * ybd->bact.dof.z);

    /*** DOF reflektieren... ***/
    ybd->bact.dof.x = ybd->bact.dof.x - A * doppel_cos;
    ybd->bact.dof.y = ybd->bact.dof.y - B * doppel_cos;
    ybd->bact.dof.z = ybd->bact.dof.z - C * doppel_cos;
    
    /*** Versuch eines geschwindigkeitsabhängigen Abprallfaktors ***/
    rv = 25 / ( fabs(ybd->bact.dof.v) + 10);
    ybd->bact.dof.v *= rv;

}


_dispatcher( BOOL, yb_YBM_CHECKLANDPOS, struct checklandpos_msg *clp )
{
/*
** Testet nur, ob die derzeitige Position nicht auf einem Slurp liegt.
** Das wäre zwar kein Problem, aber wir könnten durch die Kollisionsbear-
** beitung in feindliches Gebiet "abrutschen".
*/

    struct ypabact_data *ybd;
    FLOAT  middle_x, middle_z, distance;
    BOOL   noslurp = TRUE, dist = TRUE, nobuilding = TRUE;
    struct intersect_msg inter;

    ybd = INST_DATA(cl, o );

    if( clp->flags & CLP_COMMANDERONLY ) {

        /*** TRUE zurück, weil wir dann ja immer landen ***/
        if( ybd->bact.master != ybd->bact.robo ) return( TRUE );
        }

    if( clp->flags & CLP_NOSLURP ) {

        /*** Soll kein Slurp sein ***/
        middle_x =  ((FLOAT)ybd->bact.SectX + 0.5) * SECTOR_SIZE;
        middle_z = -((FLOAT)ybd->bact.SectY + 0.5) * SECTOR_SIZE;

        if( (fabs( middle_x - ybd->bact.pos.x) < (SECTOR_SIZE * 1 / 3) ) &&
            (fabs( middle_z - ybd->bact.pos.z) < (SECTOR_SIZE * 1 / 3) ) )
            noslurp = TRUE;
        else
            noslurp = FALSE;
        }

    if( clp->flags & CLP_NOBUILDING ) {

        /*** kein Gebäude ***/
        inter.pnt.x = ybd->bact.pos.x;
        inter.pnt.y = ybd->bact.pos.y;
        inter.pnt.z = ybd->bact.pos.z;
        inter.vec.x = inter.vec.z = 0.0;
        inter.vec.y = 30000.0;
        inter.flags = 0;
        _methoda( ybd->world, YWM_INTERSECT, &inter );
        if( inter.insect ) {

            if( fabs(inter.ipnt.y - ybd->bact.Sector->Height) < 10.0 )
                nobuilding = TRUE;
            else
                nobuilding = FALSE;
            }
        else
            nobuilding = FALSE;
        }

    if( clp->flags & CLP_DISTANCE ) {

        /*** NUR X_Z_ENTFERNUNG!!!!! ***/
        struct flt_triple *tv = &(ybd->bact.tar_vec);
        distance = nc_sqrt( tv->x*tv->x + tv->z*tv->z );
        if( distance <= clp->distance )
            dist = TRUE;
        else
            dist = FALSE;
        }

    if( noslurp && dist && nobuilding )
        return( TRUE );
    else
        return( FALSE );
}



_dispatcher( void, yb_YBM_IMPULSE, struct impulse_msg *imp )
{
/*
**  FUNCTION    Billige Variante eines Impulses. Soll nur das Objekt
**              etwas wegschmeißen und 'n bissel zur Seite drehen. Dabei
**              beachte ich, daß es sogar recht häufig vorkommen
**              wird, daß 'ne Rakete genau im Mittelpunkt explodiert
**              Ich ändere nur Geschwindigkeiten, sonst muß ich Intersections
**              machen.
**
**  ERWEITERUNG Von jetzt ab geht das wie folgt:
**              Wird das Objekt getroffen, so addieren sich die Impulse, wird
**              es nicht getroffen, so ist der Impuls von der Energie, also
**              der Druckwelle abhängig, wirkt dann aber nur auf Flugobjekte
**
**  INPUT       'ne ominöse Stärke (also ein Kraftstoß-Äquivalent)
**              Knallpunkt
*+
**  RESULTS     nix
**
**  CHANGED     15/16 Nov created von mir
*/

    struct ypabact_data *ybd;
    struct flt_triple richtung, new_speed;
    FLOAT  distance, ma, po, cos_a, angle, impf;
    WORD   zufallszahl;

    ybd = INST_DATA( cl, o );

    /*** mit dem folgenden Zeug müss' mr ewing ägsbärimendiern ***/
    ma   = 50.0 / ybd->bact.mass;
    po   = imp->impulse / 2500.0;
    impf = 2.5; // imp->impulse / 4000;
    zufallszahl = abs((WORD)(ybd->bact.pos.y)) % 3;

    richtung.x = ybd->bact.pos.x - imp->pos.x;
    richtung.y = ybd->bact.pos.y - imp->pos.y;
    richtung.z = ybd->bact.pos.z - imp->pos.z;

    /*** Wie weit ist die Sache weg? ***/
    distance = nc_sqrt( richtung.x * richtung.x +
                        richtung.y * richtung.y +
                        richtung.z * richtung.z );

    /*** War es eine direkte Kollision ? ***/
    if( distance > ybd->bact.radius ) {

        /*** Weg vom Flugobjekt explodiert ***/
        richtung.x /= distance;
        richtung.y /= distance;
        richtung.z /= distance;


        /*** Wegsetzen (nur Speed, sonst Intersection notwendig) ***/
        new_speed.x = ybd->bact.dof.x * ybd->bact.dof.v;
        new_speed.y = ybd->bact.dof.y * ybd->bact.dof.v;
        new_speed.z = ybd->bact.dof.z * ybd->bact.dof.v;
        new_speed.x += richtung.x * ma * po / distance;
        new_speed.y += richtung.y * ma * po / distance;
        new_speed.z += richtung.z * ma * po / distance;

        ybd->bact.dof.v = nc_sqrt( new_speed.x * new_speed.x + new_speed.y *
                                   new_speed.y + new_speed.z * new_speed.z );

        if( ybd->bact.dof.v > 0.0 ) {
            ybd->bact.dof.x = new_speed.x / ybd->bact.dof.v;
            ybd->bact.dof.y = new_speed.y / ybd->bact.dof.v;
            ybd->bact.dof.z = new_speed.z / ybd->bact.dof.v;
            }
        }
    else {

        /* --------------------------------------------------------------
        ** Im Fluggerät explodiert, Addition der Impulse. Des Effektes
        ** wegen verdreifachen wir den Raketenimpuls (Explosionsleistung)
        ** ----------------------------------------------------------- */
        distance = 1.0;

        new_speed.x = (ybd->bact.mass * ybd->bact.dof.x * ybd->bact.dof.v +
                       impf * imp->miss_mass * imp->miss.x * imp->miss.v) /
                       (ybd->bact.mass + imp->miss_mass);
        new_speed.y = (ybd->bact.mass * ybd->bact.dof.y * ybd->bact.dof.v +
                       impf * imp->miss_mass * imp->miss.y * imp->miss.v) /
                       (ybd->bact.mass + imp->miss_mass);
        new_speed.z = (ybd->bact.mass * ybd->bact.dof.z * ybd->bact.dof.v +
                       impf * imp->miss_mass * imp->miss.z * imp->miss.v) /
                       (ybd->bact.mass + imp->miss_mass);

        ybd->bact.dof.v = nc_sqrt( new_speed.x * new_speed.x + new_speed.y *
                                   new_speed.y + new_speed.z * new_speed.z );
        if( ybd->bact.dof.v > 0.0 ) {
            ybd->bact.dof.x = new_speed.x / ybd->bact.dof.v;
            ybd->bact.dof.y = new_speed.y / ybd->bact.dof.v;
            ybd->bact.dof.z = new_speed.z / ybd->bact.dof.v;
            }

        richtung.x = imp->miss.x;
        richtung.y = imp->miss.y;
        richtung.z = imp->miss.z;
        }

    /*** Trotz alledem Positionsüberprüfung! ***/
    _methoda( o, YBM_CORRECTPOSITION, NULL );


    ybd->bact.ExtraState &= ~EXTRA_LANDED;

    /*** Wegdrehen ***/
    cos_a = ybd->bact.dir.m31 * richtung.x + ybd->bact.dir.m32 * richtung.y +
            ybd->bact.dir.m33 * richtung.z;
    angle = 0.01 * ma * po / distance;

    if( fabs( cos_a ) > 0.7071 ) {

        /*** Drehen um die x-Achse ***/
        if( cos_a > 0.7071 ) {

            /*** nach hinten kippen ***/
            yb_rot_round_lokal_x( &(ybd->bact), (FLOAT) -angle );
            }
        else {

            /*** nach vorn kippen ***/
            yb_rot_round_lokal_x( &(ybd->bact), (FLOAT) angle );
            }
        }
    else {

        /*** Drehen um die z-Achse ***/
        if( (richtung.x*ybd->bact.dir.m33 - richtung.z*ybd->bact.dir.m31) < 0.0) {

            /*** Drehen nach links ***/
            yb_rot_round_lokal_z( &(ybd->bact), (FLOAT) -angle );
            }
        else {

            /*** Drehen nach rechts ***/
            yb_rot_round_lokal_z( &(ybd->bact), (FLOAT) angle );
            }
        }
}


_dispatcher( void, yb_YBM_STOPMACHINE, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION    Richtet ein Objekt aus und stellt alle Kräfte so ein, daß
**              es in der Luft an dieser Stelle bleibt.
**
**  INPUT       trigger_logic_msg wegen der Zeit
**
**  RESULTS     keene
**
**  CHANGED     heute erstmal gebastelt, Änderungen später
*/


    FLOAT angle, rot_x, rot_y, rot_z, new_x, new_y, new_z, old_x, old_y, old_z;
    FLOAT betrag, time, sa, ca, fct;
    struct flt_m3x3 nm, rm;
    struct ypabact_data *ybd;

    ybd = INST_DATA(cl, o);


    ybd->bact.act_force = ybd->bact.mass * (GRAVITY-0.03); // testen -->Rundungsfehler
    time = (FLOAT) msg->frame_time / 1000;


    old_x = ybd->bact.dir.m21;
    old_y = ybd->bact.dir.m22;
    old_z = ybd->bact.dir.m23;

    new_x = new_z = 0.0;
    new_y = 1.0;

    /*
    ** Jetzt drehe ich den Hubschrauber um die Achse, die senkrecht
    ** auf der alten und neuen Schubkraftachse steht um den Winkel
    ** zwischen den beiden Achsen. Die Orientierung des Winkels bekomme
    ** ich irgendwie aus der Orientierung der Rot-Achse.
    */

    rot_y = new_x * old_z - old_x * new_z;
    rot_x = old_y * new_z - old_z * new_y;
    rot_z = old_x * new_y - old_y * new_x;

    /* rot muß normiert sein */
    betrag = nc_sqrt( rot_x * rot_x + rot_y * rot_y + rot_z * rot_z );

    if( betrag > 0.001 ) {

        /*
        ** denn sonst war keine Rotation notwendig, weil beide Vektoren
        ** identisch sind.
        */

        rot_x /= betrag;
        rot_y /= betrag;
        rot_z /= betrag;

        /* old und new dürften normiert sein... */
        angle = nc_acos( old_x * new_x + old_y * new_y + old_z * new_z);
        fct = 1.5 * angle;
        if( angle > ( ybd->bact.max_rot * time ) )
                      angle = fct * ybd->bact.max_rot * time;

        if( fabs( angle ) > (OR_ANGLE / 2) )  {

            /*** nur dann lohnt sich die Rotation ***/
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

            nc_m_mul_m( &(ybd->bact.dir), &rm, &nm );

            ybd->bact.dir = nm;
            }
        else {

            /*
            ** Wir sind fast gerade, also setzen wir uns auch gerade hin.
            ** Einfach setzen eben.
            */

            /*** y ***/
            ybd->bact.dir.m21 = 0.0;
            ybd->bact.dir.m22 = 1.0;
            ybd->bact.dir.m23 = 0.0;

            /*** x ***/
            betrag = nc_sqrt( ybd->bact.dir.m11 * ybd->bact.dir.m11 +
                              ybd->bact.dir.m13 * ybd->bact.dir.m13 );
            ybd->bact.dir.m11 /= betrag;
            ybd->bact.dir.m12  = 0.0;
            ybd->bact.dir.m13 /= betrag;

            /*** z ***/
            betrag = nc_sqrt( ybd->bact.dir.m31 * ybd->bact.dir.m31 +
                              ybd->bact.dir.m33 * ybd->bact.dir.m33 );
            ybd->bact.dir.m31 /= betrag;
            ybd->bact.dir.m32  = 0.0;
            ybd->bact.dir.m33 /= betrag;

            if( fabs(ybd->bact.dof.v) < IS_SPEED ) {

                ybd->bact.dof.v = 0.0;
                ybd->bact.dof.x = 0.0;
                ybd->bact.dof.y = 1.0;
                ybd->bact.dof.z = 0.0;
                }
            }
        }

    ybd->bact.dof.v *= 0.8;             // nur so... doch doch!
}


void yb_rot_round_lokal_z( struct Bacterium *bact, FLOAT angle)
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



void yb_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
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


void yb_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;        rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;          rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;       rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, &(bact->dir), &neu_dir);

    bact->dir = neu_dir;
}


void yb_rot_round_global_y( struct Bacterium *bact, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;        rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;          rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;       rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m( &(bact->dir), &rm, &neu_dir);

    bact->dir = neu_dir;
}

void yb_rot_round_lokal_y2( struct flt_m3x3 *dir, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;        rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;          rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;       rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, dir, &neu_dir);

    *dir = neu_dir;
}



void yb_DrehObjektInFlugrichtung( struct ypabact_data *ybd, ULONG timE )
{

    /* ------------------------------------------------------------
    ** Diese Routine dreht ein Objekt in Flugrichtung und beachtet
    ** dabei, daß es sich nur um einen maximalen Winkel pro Sekunde
    ** drehen darf.
    **
    ** NEU: Wir drehen nur, wenn wir nicht (fast) gerade sind
    ** ----------------------------------------------------------*/
    FLOAT y_angle, afactor;

    y_angle = nc_acos( ybd->bact.dir.m22 );

    if( y_angle > OR_ANGLE ) {

        /*** Wir sind "geneigt" genug ***/

        if( ((ybd->bact.dof.z != 0) || (ybd->bact.dof.x != 0)) &&
            (ybd->bact.dof.v > 0.0) ) {

            FLOAT angle;
            struct flt_m3x3 rm,nm;
            FLOAT fx = ybd->bact.dof.x;
            FLOAT fz = ybd->bact.dof.z;
            FLOAT zx = ybd->bact.dir.m31;
            FLOAT zz = ybd->bact.dir.m33;
            FLOAT time = (FLOAT) timE / 1000;

            angle = nc_acos( ( fx * zx + fz * zz ) /
                               nc_sqrt( fx * fx + fz * fz ) /
                               nc_sqrt( zz * zz + zx * zx ) );

            /* afactor ist auch winkelabhängig */
            afactor = 0.2 + 0.8 * fabs( angle );

            if( angle > ( ybd->bact.max_rot * time * afactor ) )
                angle = ybd->bact.max_rot * time * afactor;

            /* nun noch die Orientierung über die y-Komponente des VP */
            if( ( fx * zz - zx * fz ) < 0 ) angle = -angle;

            /*
            ** nun rotieren
            */

            rm.m11 = cos(angle); rm.m12 = 0.0;       rm.m13 = -sin(angle);
            rm.m21 = 0.0;        rm.m22 = 1.0;       rm.m23 = 0.0;
            rm.m31 = sin(angle); rm.m32 = 0.0;       rm.m33 = cos(angle);
            
            nc_m_mul_m( &(ybd->bact.dir), &rm, &nm );

            ybd->bact.dir = nm;
            }
        }
}


void yb_CalculateForce( struct ypabact_data *ybd, struct trigger_logic_msg *msg )
{
    /* Schubkraftberechnung für Hubschrauber */

    FLOAT new_x, new_y, new_z, old_x, old_y, old_z, rot_x, rot_y, rot_z;
    FLOAT angle, schub, gewicht, y_res, sin_a, ca, sa, betrag, time;
    struct flt_m3x3 nm, rm;



    ybd->bact.act_force = ybd->bact.max_force;
    time = (FLOAT) msg->frame_time / 1000.0;


    /*
    ** die y-Komponente der Schubkraft: y = cos a
    ** sin a = -0.5 y nc_sqrt(1-y*y) s/g +/- nc_sqrt( 0.25 y*y (1 - y*y)s*s/g*g
    **                                          - s*s/g*g + 1)
    ** y = -tar_unit.y  s schub g gewicht
    ** supereinfach!
    */


    y_res = -ybd->bact.tar_unit.y;
    if( y_res ==  1.0 ) y_res =  0.99999;
    if( y_res == -1.0 ) y_res = -0.99999;
    gewicht = ybd->bact.mass * GRAVITY;
    schub   = ybd->bact.act_force;
    if( schub == 0.0) schub = 0.1;


    sin_a = -0.5 * y_res * nc_sqrt( 1 - y_res * y_res ) * gewicht / schub +
          nc_sqrt( 1 - gewicht*gewicht / (schub*schub) *
          ( 1 - 0.25*y_res*y_res + 0.25*y_res*y_res*y_res*y_res ) );

    /* new_y = -acos a , a = asin ( sin_a) .... hm....scheiße */
    new_y = -cos( nc_asin( sin_a ) );

    if( ybd->bact.tar_unit.z == 0.0 )
        new_z = 0.0;
    else {

        new_z = nc_sqrt( ( 1 - new_y * new_y ) / ( 1 +
                         ybd->bact.tar_unit.x * ybd->bact.tar_unit.x /
                       ( ybd->bact.tar_unit.z * ybd->bact.tar_unit.z ) ) );
        if( ybd->bact.tar_unit.z < 0 ) new_z = -new_z;
        }

    if( ybd->bact.tar_unit.x == 0.0 )
        new_x = 0.0;
    else {

        new_x = nc_sqrt( ( 1 - new_y * new_y ) / ( 1 +
                        ybd->bact.tar_unit.z * ybd->bact.tar_unit.z /
                      ( ybd->bact.tar_unit.x * ybd->bact.tar_unit.x ) ) );
        if( ybd->bact.tar_unit.x < 0 ) new_x = -new_x;
        }


    /*
    ** Nun haben wir den Wunschschubvektor. Da ich den Hubschrauber nur um einen
    ** gewissen Betrag pro Sekunde drehen kann, muß ich den Drehwinkel evtl.
    ** reduzieren. Dazu brauche ich erstmal den alten Vektor...
    */

    old_x = -( ybd->bact.dir.m21);
    old_y = -( ybd->bact.dir.m22);
    old_z = -( ybd->bact.dir.m23);

    /*
    ** Jetzt drehe ich den Hubschrauber um die Achse, die senkrecht
    ** auf der alten und neuen Schubkraftachse steht um den Winkel
    ** zwischen den beiden Achsen. Die Orientierung des Winkels bekomme
    ** ich irgendwie aus der Orientierung der Rot-Achse.
    */

    rot_y = new_x * old_z - old_x * new_z;
    rot_x = old_y * new_z - old_z * new_y;
    rot_z = old_x * new_y - old_y * new_x;

    /* rot muß normiert sein */
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
        if( angle > ( ybd->bact.max_rot * time ) )
                      angle = ybd->bact.max_rot * time;

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

            nc_m_mul_m( &(ybd->bact.dir), &rm, &nm );

            ybd->bact.dir = nm;
            }
        }

    /* Ende der Schubkraftberechnung */
}



void yb_RichteObjektAus( struct ypabact_data *ybd, struct flt_triple *vec,
                         FLOAT time)
{
    /*
    ** Richtet Objekt an Hand des übergebenen EbenenNormalenvektors aus
    */

    FLOAT old_x, old_y, old_z, new_x, new_y, new_z, rot_x, rot_y, rot_z;
    FLOAT betrag, sa, ca, angle;
    struct flt_m3x3 nm, rm;

    old_x = ybd->bact.dir.m21;
    old_y = ybd->bact.dir.m22;
    old_z = ybd->bact.dir.m23;

    new_x = vec->x;
    new_y = vec->y;
    new_z = vec->z;

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

        // if( angle > ybd->bact.max_rot * time )
        //    angle = ybd->bact.max_rot * time;

        if( angle > 0.001 ) {

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

            nc_m_mul_m( &(ybd->bact.dir), &rm, &nm );

            ybd->bact.dir = nm;
            }
        }
}


void yb_DoSpecialEffect( struct ypabact_data *ybd, LONG frame_time )
{
    /* ---------------------------------------------------------------------
    ** Die Sache ist die: SpecialFX sind sowieso tot und da CRASHBACTERIUM
    ** immer aufgerufen wird, bietet es sich an, diese hier einzubauen.
    **
    ** Specialef dürfen nicht fallen und sich nicht bewegen. Das realisieren
    ** wir dadurch, daß EXTRA_LANDED erst gesetzt wird, wenn die Sache ab-
    ** gelaufen ist und setzen dann auch gleich YLS runter. Damit kann das
    ** Object sofort feigegeben werden.
    ** -------------------------------------------------------------------*/
    LONG i, vpcnt;

    ybd->bact.ExtraState |= EXTRA_SCALE;

    /*** A B R U C H ***/
    if( ybd->bact.scale_count >= ybd->bact.scale_duration ) {

        /*** gleich freigeben ***/
        ybd->YourLastSeconds = -1;
        _methoda( ybd->bact.BactObject, YBM_RELEASEVEHICLE, ybd->bact.BactObject);
        return;
        }

    /*** S C A L I E R U N G ***/

    /*** aktueller Scale_speed ***/
    ybd->bact.scale_speed += ybd->bact.scale_accel * (FLOAT)frame_time / 1000.0;

    /*** aktueller Scaler ***/
    ybd->bact.scale_start += ybd->bact.scale_speed * (FLOAT)frame_time / 1000.0;

    ybd->bact.scale_x = ybd->bact.scale_start;
    ybd->bact.scale_y = ybd->bact.scale_start;
    ybd->bact.scale_z = ybd->bact.scale_start;

    /*** Rotation, eifach um Max_rot ***/
    yb_rot_round_lokal_y( &(ybd->bact),
                          (ybd->bact.max_rot * (FLOAT)frame_time / 1000.0 ));

    /*** V I S P R O T O S ***/

    /*** Wieviele haben wir? ***/
    vpcnt = 0;
    for( i = 0; i < MAXNUM_VPFX; i++ ) 
        if( ybd->bact.vp_proto_fx[ i ] ) vpcnt++;

    /*** Offset ermitteln ***/
    if( vpcnt ) {

        LONG offset;

        offset = ( vpcnt * ybd->bact.scale_count ) / ybd->bact.scale_duration;
        ybd->vis_proto = ybd->bact.vp_proto_fx[ offset ];
        ybd->vp_tform  = ybd->bact.vp_tform_fx[ offset ];
        }

    ybd->bact.scale_count += frame_time;
}

void yb_RandomSpeed( struct ypabact_data *ybd )
{
    /* ----------------------------------------------------
    ** Wir verstärken die x-z-Geschwindigkeit. Das allein
    ** reicht aber noch nicht, weil sich die vehicle auch
    ** so aufschaukeln können. Deshalb gibt es noch einen
    ** Drall. Als weitere Alternative könnte man teile davon
    ** nur bei gewissen y-Werten der geschwindigkeit machen.
    ** Wenn das auch nicht hilft, hilft gar nix mehr...
    ** ---------------------------------------------------*/
    FLOAT norm;
    //if( fabs( ybd->bact.dof.y ) > 0.97 ) {

    /*** Anfangskick ***/
    if( ybd->bact.dof.x <  0.0 )
        ybd->bact.dof.x -= 7.0;
    if( ybd->bact.dof.z <  0.0 )
        ybd->bact.dof.z -= 7.0;
    if( ybd->bact.dof.x >= 0.0 )
        ybd->bact.dof.x += 7.0;
    if( ybd->bact.dof.z >= 0.0 )
        ybd->bact.dof.z += 7.0;

    /*** E bissel was muß sich schon tun ***/
    if( ybd->bact.dof.v < 15 ) ybd->bact.dof.v = 15;

    /*** Renormierung ***/
    norm = nc_sqrt( ybd->bact.dof.x * ybd->bact.dof.x +
                    ybd->bact.dof.y * ybd->bact.dof.y +
                    ybd->bact.dof.z * ybd->bact.dof.z);

    if( norm > 0.001 ) {

        ybd->bact.dof.x = ybd->bact.dof.x / norm;
        ybd->bact.dof.y = ybd->bact.dof.y / norm;
        ybd->bact.dof.z = ybd->bact.dof.z / norm;
        }
    else {

        ybd->bact.dof.x = 0.0;
        ybd->bact.dof.y = 1.0;
        ybd->bact.dof.z = 0.0;
        }
}

