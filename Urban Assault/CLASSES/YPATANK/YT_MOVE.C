/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Bewegung und Bodenausrichtung
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
#include "ypa/ypatankclass.h"
#include "audio/audioengine.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

void yt_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
BOOL yt_BactCollision(struct ypatank_data *ytd, FLOAT time );
void yt_RotateTank( struct Bacterium *bact, FLOAT angle, UBYTE wie );
void yt_TipTank( struct ypatank_data *ytd, float time, struct flt_triple *rot );
void yt_rot_round_lokal_x( struct Bacterium *bact, float angle );


/*-----------------------------------------------------------------*/
_dispatcher(void, yt_YBM_MOVE, struct move_msg *move)
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

    struct ypatank_data *ytd;
    struct uvec grav;       /* Gravitations-Kraftvektor */
    struct uvec schub;      /* Schub-Kraftvektor */
    struct uvec lw;         /* Luftwiederstand */
    struct uvec res;
    FLOAT  delta_betrag, new_v_x, new_v_y, new_v_z, new_v_v;
    BOOL   merke_null = FALSE;
    FLOAT  verhaeltnis, max;

    ytd = INST_DATA( cl, o);

    ytd->bact->old_pos = ytd->bact->pos;
    
    /*
    ** Wir müssen die Fahrtrichtung auch in y-Richtung beachten, denn
    ** bergauf muß es schon schwerer sein.
    */

    if( ytd->bact->dof.v == 0.0 ) merke_null = TRUE;


    /*** Gravitation wirkt immer nach unten ***/
    if( (move->flags & MOVE_STOP) &&
        (ytd->bact->ExtraState & EXTRA_LANDED) ) {

        /*** Hangabtrieb und Bremsen können sich beißen ***/
        grav.x = 0.0;
        grav.y = 1.0;
        grav.z = 0.0;
        grav.v = 0.0;
        }
    else {

        grav.x = 0.0;
        grav.y = 1.0;
        grav.z = 0.0;
        if( ACTION_DEAD == ytd->bact->MainState )
            grav.v = GRAV_FACTOR * GRAVITY * ytd->bact->mass;
        else
            grav.v = GRAVITY * ytd->bact->mass;
        }

    /*** Schub wirkt in Richtung lokale z-Achse ***/
    if( move->flags & (UBYTE) MOVE_NOFORCE ) {
        schub.x = 0;
        schub.y = 1;
        schub.z = 0;
        schub.v = 2*grav.v;   // so zum Testen....
        }
    else {
        schub.x = ytd->bact->dir.m31;
        schub.y = ytd->bact->dir.m32;
        schub.z = ytd->bact->dir.m33;
        schub.v = ytd->bact->act_force;
        }

    /*** Luftwiderstand ***/
    lw.x = -ytd->bact->dof.x;
    lw.y = -ytd->bact->dof.y;
    lw.z = -ytd->bact->dof.z;
    lw.v = ytd->bact->dof.v * ytd->bact->air_const;

    /*** resultierender Kraftvektor ***/
    res.x = (grav.x*grav.v)+(schub.x*schub.v)+(lw.x*lw.v);
    res.y = (grav.y*grav.v)+(schub.y*schub.v)+(lw.y*lw.v);
    res.z = (grav.z*grav.v)+(schub.z*schub.v)+(lw.z*lw.v);
    res.v = nc_sqrt(res.x*res.x + res.y*res.y + res.z*res.z);

    /*** neuer dof - zum alten aufaddieren ***/
    
    if( res.v > 0.0 ) {

        delta_betrag = (res.v / ytd->bact->mass) * move->t;
        new_v_x = ytd->bact->dof.x * ytd->bact->dof.v + (res.x / res.v ) * delta_betrag;
        new_v_y = ytd->bact->dof.y * ytd->bact->dof.v + (res.y / res.v ) * delta_betrag;
        new_v_z = ytd->bact->dof.z * ytd->bact->dof.v + (res.z / res.v ) * delta_betrag;
        new_v_v = nc_sqrt( new_v_x * new_v_x + new_v_y * new_v_y + new_v_z * new_v_z );
        
        /* wir haben Zwangskräfte. Das Heißt, wir nehmen nur den Teil
        ** der Geschwindigkeit, der in lokale z-Richtung zeigt. Die dof-
        ** Richtung ist also die lokale z_richtung und der dof-Betrag ist
        ** das Skalarprodukt aus Richtung der Geschwindigkeit und Fahrt-
        ** richtung.
        */
        if( new_v_v > 0.0 ) {
            new_v_x /= new_v_v;
            new_v_y /= new_v_v;
            new_v_z /= new_v_v;
            }

        if( !(ytd->bact->ExtraState & EXTRA_LANDED) ) {
            
            /* Es wirken keine Zwangskräfte */
            ytd->bact->dof.x = new_v_x;
            ytd->bact->dof.y = new_v_y;
            ytd->bact->dof.z = new_v_z;
            ytd->bact->dof.v = new_v_v;
            }
        else {

            /* Es wirken Zwangskräfte */
            ytd->bact->dof.x = ytd->bact->dir.m31;
            ytd->bact->dof.y = ytd->bact->dir.m32;
            ytd->bact->dof.z = ytd->bact->dir.m33;
            ytd->bact->dof.v = new_v_v * ( ytd->bact->dir.m31 * new_v_x + 
                                           ytd->bact->dir.m32 * new_v_y + 
                                           ytd->bact->dir.m33 * new_v_z );
            }
        }


    /*
    ** Jetzt haben wir eine Neue Geschwindigkeit, da müssen wir aber noch
    ** die Position neu setzen. Die Ausrichtung wurde schon erledigt...
    */
    ytd->bact->pos.x += ytd->bact->dof.x * ytd->bact->dof.v * move->t * METER_SIZE;
    ytd->bact->pos.y += ytd->bact->dof.y * ytd->bact->dof.v * move->t * METER_SIZE;
    ytd->bact->pos.z += ytd->bact->dof.z * ytd->bact->dof.v * move->t * METER_SIZE;

    /*** Weltrand ***/
    _methoda( o, YBM_CORRECTPOSITION, NULL );
    
    /*** Alte Sachen vor Veränderung ***/
    ytd->bact->sc.src[ VP_NOISE_NORMAL ].pitch  = ytd->bact->bu_pitch;
    ytd->bact->sc.src[ VP_NOISE_NORMAL ].volume = ytd->bact->bu_volume;

    /*** Soundveränderung ***/

    /* -------------------------------------------------------------
    ** Das verhältnis aus jetziger und möglicher Geschwindigkeit
    ** kann zwischen 0 und 1 schwanken. daraus machen wir noch einen
    ** faktor, der zwischen 1 (v=0) und 3 oder 4 (1) schwankt
    ** -----------------------------------------------------------*/
    // if( fabs(ytd->bact->act_force) < 0.1 )
    //     verhaeltnis = 0.0;
    // else
    //     verhaeltnis = fabs( ytd->bact->act_force - ytd->bact->dof.v *
    //                         ytd->bact->air_const ) / ytd->bact->max_force;

    verhaeltnis = fabs( ytd->bact->dof.v ) / 
                  (ytd->bact->max_force / ytd->bact->bu_air_const);

    if( ytd->bact->max_pitch > -0.8 )
        max = ytd->bact->max_pitch;
    else
        max = MSF_TANK;

    verhaeltnis = max * verhaeltnis;
    if( verhaeltnis > max ) verhaeltnis = max;

    if( ytd->bact->sc.src[ VP_NOISE_NORMAL ].sample )
        ytd->bact->sc.src[ VP_NOISE_NORMAL ].pitch += (LONG)
             (( (FLOAT)ytd->bact->sc.src[ VP_NOISE_NORMAL ].pitch +
                (FLOAT)ytd->bact->sc.src[ VP_NOISE_NORMAL ].sample->SamplesPerSec) *
                verhaeltnis);

}


_dispatcher(void, yt_YBM_IMPULSE, struct impulse_msg *imp )
{
/*
**  FUNCTION    Überlagert den Impuls. Wir werden etwas zur Seite
**              gesetzt (mit intersection!)
**              Evtl. 'n bissel springen und um y drehen
**
**  INPUT       wie immer....
**
**  RESULT      was erwartest du denn?
**
**  CHANGED     23-Nov-95   created von mir
*/
    
    struct ypatank_data *ytd;
    struct flt_triple richtung, newpos;
    FLOAT  distance, ma, po, stueck;
    struct intersect_msg inter;


    ytd = INST_DATA( cl, o );

    /*** mit dem folgenden Zeug müss' mr ewing ägsbärimendiern ***/
    ma = 500.0 / ytd->bact->mass;
    po = imp->impulse / 2500.0;

    richtung.x = ytd->bact->pos.x - imp->pos.x;
    richtung.y = ytd->bact->pos.y - imp->pos.y;
    richtung.z = ytd->bact->pos.z - imp->pos.z;
    
    /*** Wie weit ist die Sache weg? ***/
    distance = nc_sqrt( richtung.x * richtung.x +
                        richtung.y * richtung.y +
                        richtung.z * richtung.z );

    if( distance > 1.0 ) {

        /*** Weg vom Flugobjekt explodiert ***/
        richtung.x /= distance;
        richtung.y /= distance;
        richtung.z /= distance;
        }
    else {

        /*** Im Fluggerät explodiert ***/
        distance = 1.0;
        
        richtung.x = imp->miss.x;
        richtung.y = imp->miss.y;
        richtung.z = imp->miss.z;
        }

    /*** Nun wegsetzen und Intersectiontest machen (TRUE->Ignore) ***/
    stueck = 10 * po * ma / distance;

    newpos.x = ytd->bact->pos.x + richtung.x * stueck;
    newpos.y = ytd->bact->pos.y - richtung.y * stueck;  // Sprung
    newpos.z = ytd->bact->pos.z + richtung.z * stueck;

    inter.pnt.x = ytd->bact->pos.x;
    inter.pnt.y = ytd->bact->pos.y;
    inter.pnt.z = ytd->bact->pos.z;
    inter.vec.x = newpos.x - ytd->bact->pos.x;
    inter.vec.y = newpos.y - ytd->bact->pos.y;
    inter.vec.z = newpos.z - ytd->bact->pos.z;
    inter.flags = 0;
    _methoda( ytd->world, YWM_INTERSECT, &inter );
    if( inter.insect == FALSE ) {

        /*** Wegsetzen ***/
        ytd->bact->pos = newpos;

        /*** wegdrehen ***/

        }
}



_dispatcher( BOOL, yt_YTM_ALIGNVEHICLE_A, struct alignvehicle_msg *avm)
{
/*
**  FUNCTION    Richtet einen autonomen Panzer aus. Übergeben wird die
**              Zeit (FrameTime)
**
**  INPUT       alignvehicle_msg
**
**  RESULT      nitschewo
**
**  CHANGED     8-12-95     8100 000C Zeuch als Methode
**              27-Mai-96   neu: avm kann den Abhang-Vektor hier schon zurückgeben
**                          wenn eine Intersection auftrat!
*/

    ULONG VIEW;
    struct intersect_msg inter;
    FLOAT over_eof, betrag;
    FLOAT A, B, C, ca, sa, angle, rot_x, rot_y, rot_z;
    struct flt_m3x3 rm, nm;
    struct flt_triple ra;

    struct ypatank_data *ytd = INST_DATA( cl, o);
    avm->ret_msg = 0;

    _get( o, YBA_Viewer, &VIEW );
    if( VIEW )
        over_eof = ytd->bact->viewer_over_eof;
    else
        over_eof = ytd->bact->over_eof;

    inter.pnt.x = ytd->bact->pos.x + ytd->bact->dir.m21 * over_eof;
    inter.pnt.y = ytd->bact->pos.y + ytd->bact->dir.m22 * over_eof - 50;
    inter.pnt.z = ytd->bact->pos.z + ytd->bact->dir.m23 * over_eof;

    inter.vec.x = 0.0;
    inter.vec.y = 110 + ytd->bact->over_eof;
    inter.vec.z = 0.0;
    inter.flags = 0;

    _methoda( ytd->world, YWM_INTERSECT, &inter );

    if( inter.insect ) {

        if( inter.sklt->PlanePool[ inter.pnum ].B >= YPS_PLANE ) {

            /* -------------------------------------------------------------------
            ** Wir haben Boden unter den Füßen. Wir setzen uns auf ipnt - over_eof
            ** und richten uns, sofern wir sichtbar sind, aus. 
            ** -----------------------------------------------------------------*/

            A = inter.sklt->PlanePool[ inter.pnum ].A;
            B = inter.sklt->PlanePool[ inter.pnum ].B;
            C = inter.sklt->PlanePool[ inter.pnum ].C;


            /* nun kommt die Rotationsachse... */
            if( ytd->flags & YTF_Tip ) {

                ra.x = A; ra.y = B; ra.z = C;
                yt_TipTank( ytd, avm->time, &ra );
                A = ra.x; B = ra.y; C = ra.z;
                }

            rot_x = ytd->bact->dir.m22 * C - ytd->bact->dir.m23 * B;
            rot_y = ytd->bact->dir.m23 * A - ytd->bact->dir.m21 * C;
            rot_z = ytd->bact->dir.m21 * B - ytd->bact->dir.m22 * A;

            betrag = nc_sqrt( rot_x * rot_x + rot_y * rot_y + rot_z * rot_z );

            if( betrag > 0.0 ) {

                /* ... und der Rotationswinkel */
                angle = nc_acos( A * ytd->bact->dir.m21 + B * ytd->bact->dir.m22 +
                                 C * ytd->bact->dir.m23 );

                if( angle > 2 * ytd->bact->max_rot * avm->time )
                    angle = 2 * ytd->bact->max_rot * avm->time;
                if( fabs( angle ) < 1e-02 ) angle = 0.0;

                /* -------------------------------------------------------------
                ** eine Winkelorientierung brauchen wir, so glaube ich, nicht 
                ** zu machen. das übernimmt die Orientierung der Rotationsachse. 
                ** Drehen wir! 
                ** -----------------------------------------------------------*/

                rot_x /= betrag;    rot_y /= betrag;    rot_z /= betrag;

                ca = cos(  angle );
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

                nc_m_mul_m( &(ytd->bact->dir), &rm, &nm );

                ytd->bact->dir = nm;
                }

            /* --------------------------------------------------------
            ** Wegen dem "Nachhintenkippen" des Mittelpunktes muß alles
            ** global-y ausgerichtet sein!!!
            ** Weil der testpunkt nun am Boden liegt, probiere ich es
            ** nochmal
            ** ------------------------------------------------------*/
            ytd->bact->pos.x = inter.ipnt.x - ytd->bact->dir.m21 * over_eof;
            ytd->bact->pos.y = inter.ipnt.y - ytd->bact->dir.m22 * over_eof;
            ytd->bact->pos.z = inter.ipnt.z - ytd->bact->dir.m23 * over_eof;
            return( TRUE );
            }
        else {

            /*** Zwar Schnitt, aber schlecht. Zurückgeben! ***/
            avm->ret_msg |= AVMRET_SLOPE;
            avm->slope.x  = inter.sklt->PlanePool[ inter.pnum ].A;
            avm->slope.y  = inter.sklt->PlanePool[ inter.pnum ].B;
            avm->slope.z  = inter.sklt->PlanePool[ inter.pnum ].C;
            return( FALSE );
            }
        }
    else {

        /*** Gar nüscht gfunden ***/
        return( FALSE );
        }
}



_dispatcher( LONG, yt_YTM_ALIGNVEHICLE_U, struct alignvehicle_msg *avm)
{
/*
**  FUNCTION    Richtet einen Spieler-Panzer aus. Übergeben wird die
**              Zeit (FrameTime)
**
**  INPUT       alignvehicle_msg
**
**  RESULT      nitschewo
**
**  CHANGED     8-12-95     8100 000C Zeuch als Methode
*/


    /*
    ** Die UserVariante...ist sehr anders, da wir hier die direkte Kollision
    ** gleich mitmachen und diese somit auch zurückmelden müssen (TRUE).
    ** Sichtbarkeit interessiert nicht. VIEWER sind wir immer.
    */

    struct intersect_msg interl, interr, interh;
    FLOAT  A, B, C, ca, sa, angle, betrag, VV, sp, over_eof;
    struct flt_m3x3 rm, nm, *dir;
    struct flt_triple *pos, *old, phinten, plinks, prechts, rot, ra, dvr, dvl;
    struct uvec *dof;
    FLOAT  tan_fkt = 1.73;       /* 1.73 == 30 Grad */
    FLOAT  lfactor = 1.7;
    FLOAT  round_rot;
    ULONG  USER, VIEW;
    BOOL   Schub_Links = FALSE, Schub_Rechts = FALSE, Schub_Hinten = FALSE;

    struct ypatank_data *ytd = INST_DATA( cl, o);

    _get( o, YBA_Viewer, &VIEW );
    _get( o, YBA_UserInput, &USER );
    if( VIEW )
        over_eof = ytd->bact->viewer_over_eof;
    else
        over_eof = ytd->bact->over_eof;

    /*
    ** ++++++++++++++++++++++++++++ TESTS +++++++++++++++++++++++++++++++++
    */
    pos = &(ytd->bact->pos);
    dir = &(ytd->bact->dir);
    old = &(ytd->bact->old_pos);
    dof = &(ytd->bact->dof);

    /*** Achtung, wir müssen natürlich die Fahrtrichtung beachten ***/
    if( ytd->bact->dof.v >= 0.0 ) {

        plinks.x  = pos->x+(dir->m31-dir->m11)*ytd->bact->viewer_radius*0.7071;
        plinks.y  = pos->y+(dir->m32-dir->m12)*ytd->bact->viewer_radius*0.7071;
        plinks.z  = pos->z+(dir->m33-dir->m13)*ytd->bact->viewer_radius*0.7071;

        prechts.x = pos->x+(dir->m31+dir->m11)*ytd->bact->viewer_radius*0.7071;
        prechts.y = pos->y+(dir->m32+dir->m12)*ytd->bact->viewer_radius*0.7071;
        prechts.z = pos->z+(dir->m33+dir->m13)*ytd->bact->viewer_radius*0.7071;

        phinten.x = pos->x - dir->m31 * ytd->bact->viewer_radius;
        phinten.y = pos->y - dir->m32 * ytd->bact->viewer_radius;
        phinten.z = pos->z - dir->m33 * ytd->bact->viewer_radius;
        VV = 1.0;
        }
    else {

        plinks.x  = pos->x+(-dir->m31-dir->m11)*ytd->bact->viewer_radius*0.7071;
        plinks.y  = pos->y+(-dir->m32-dir->m12)*ytd->bact->viewer_radius*0.7071;
        plinks.z  = pos->z+(-dir->m33-dir->m13)*ytd->bact->viewer_radius*0.7071;

        prechts.x = pos->x+(-dir->m31+dir->m11)*ytd->bact->viewer_radius*0.7071;
        prechts.y = pos->y+(-dir->m32+dir->m12)*ytd->bact->viewer_radius*0.7071;
        prechts.z = pos->z+(-dir->m33+dir->m13)*ytd->bact->viewer_radius*0.7071;

        phinten.x = pos->x + dir->m31 * ytd->bact->viewer_radius;
        phinten.y = pos->y + dir->m32 * ytd->bact->viewer_radius;
        phinten.z = pos->z + dir->m33 * ytd->bact->viewer_radius;
        VV = -1.0;
        }

    /*** Punkte noch runtersetzen ***/
    plinks.y  += ytd->bact->viewer_over_eof;
    prechts.y += ytd->bact->viewer_over_eof;
    phinten.y += ytd->bact->viewer_over_eof;

    /*** Startpunkt hochsetzen ***/
    interh.pnt.x=interr.pnt.x=interl.pnt.x=pos->x;
    interh.pnt.y=interr.pnt.y=interl.pnt.y=pos->y-tan_fkt*ytd->bact->viewer_radius;
    interh.pnt.z=interr.pnt.z=interl.pnt.z=pos->z;

    /*** Vektoren ermitteln ***/
    interl.vec.x = (plinks.x  - interl.pnt.x) * lfactor;
    interl.vec.y = (plinks.y  - interl.pnt.y) * lfactor;
    interl.vec.z = (plinks.z  - interl.pnt.z) * lfactor;
    interr.vec.x = (prechts.x - interr.pnt.x) * lfactor;
    interr.vec.y = (prechts.y - interr.pnt.y) * lfactor;
    interr.vec.z = (prechts.z - interr.pnt.z) * lfactor;
    interh.vec.x = (phinten.x - interh.pnt.x) * lfactor;
    interh.vec.y = (phinten.y - interh.pnt.y) * lfactor;
    interh.vec.z = (phinten.z - interh.pnt.z) * lfactor;

    if( VIEW ) {

        interh.flags = 0;
        interr.flags = 0;
        interl.flags = 0;
        }
    else {

        interh.flags = INTERSECTF_CHEAT;
        interr.flags = INTERSECTF_CHEAT;
        interl.flags = INTERSECTF_CHEAT;
        }

    /*** Intersections immer schön nacheinander, wegen den Slurps ***/
    _methoda( ytd->world, YWM_INTERSECT, &interl );

    /*
    ** +++++++++++++++++++ AUSWERTUNG +++++++++++++++++++++++++++
    */

    /*** Nun geht es an die Auswertung der Schnittresultate ***/
    if( interl.insect ) {

        /*** Hindernis? dann raus ***/
        if( fabs(interl.sklt->PlanePool[interl.pnum].B) < YPS_PLANE ) {
        
            /*
            ** Er muß von der Wand weg! Das heißt nicht auf old_pos! Siehe Abhang.
            ** Die Geschwindigkeit negieren wir, wenn wir auf die Wand zugefahren
            ** sind (dof*ABC >0) 
            */

            sp = (dir->m31 * interl.sklt->PlanePool[ interl.pnum ].A +
                  dir->m32 * interl.sklt->PlanePool[ interl.pnum ].B +
                  dir->m33 * interl.sklt->PlanePool[ interl.pnum ].C) * VV;
            if( sp > 0.0 ) {

                /*** ...und krachen lassen (test wegen dof.v vorher) ***/
                if( ytd->bact->dof.v > NOISE_SPEED / 3) {

                    if( USER ) {

                        struct yw_forcefeedback_msg ffb;

                        /*** Um Krach beim Auseinanderwuseln zu vermeiden... ***/
                        _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHLAND );

                        ffb.type    = YW_FORCE_COLLISSION;
                        ffb.power   = 1.0;
                        ffb.dir_x   = interl.ipnt.x;
                        ffb.dir_y   = interl.ipnt.z;
                        _methoda( ytd->world, YWM_FORCEFEEDBACK, &ffb );
                        }
                    }

                /*** Wand voraus. Stehenbleiben oder abrutschen? ***/
                if( sp < 0.82 ) {

                    /*** wegrutschen ***/
                    pos->x = old->x - interl.sklt->PlanePool[ interl.pnum ].A * 10;
                    pos->z = old->z - interl.sklt->PlanePool[ interl.pnum ].C * 10;
                    }
                else {

                    /*** stehenbleiben ***/
                    dof->v = 0.0;
                    ytd->bact->act_force = 0.0;
                    *pos = *old;
                    }
                }
            else {
                //pos->x += dof->v * dof->x * avm->time * METER_SIZE;
                //pos->y += dof->v * dof->y * avm->time * METER_SIZE;
                //pos->z += dof->v * dof->z * avm->time * METER_SIZE;
                ytd->bact->act_force = 0.0;
                }
            return( 2L );
            }
        }
    else {

        /*** Abhang, Für den Autonomen ist es ein Hindernis ***/
        if( USER ) {

            /*** Wir täuschen den Endpunkt vor, lassen inter.insect aber FALSE ***/
            interl.ipnt.x = pos->x + interl.vec.x;
            interl.ipnt.y = pos->y + interl.vec.y - tan_fkt * ytd->bact->viewer_radius;
            interl.ipnt.z = pos->z + interl.vec.z;
            Schub_Links = TRUE;
            }
        else {
            return( 2L );
            }
        }

    /*** und noch eine! ***/
    _methoda( ytd->world, YWM_INTERSECT, &interr );

    if( interr.insect ) {

        /* Hindernis? */
        if( fabs(interr.sklt->PlanePool[interr.pnum].B) < YPS_PLANE ) {

            sp = (dir->m31 * interr.sklt->PlanePool[ interr.pnum ].A +
                  dir->m32 * interr.sklt->PlanePool[ interr.pnum ].B +
                  dir->m33 * interr.sklt->PlanePool[ interr.pnum ].C) * VV;

            if( sp > 0.0 ) {

                /*** ...und krachen lassen ***/
                if( ytd->bact->dof.v > NOISE_SPEED / 3 ) {

                    if( USER ) {

                        struct yw_forcefeedback_msg ffb;

                        _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHLAND );

                        ffb.type    = YW_FORCE_COLLISSION;
                        ffb.power   = 1.0;
                        ffb.dir_x   = interr.ipnt.x;
                        ffb.dir_y   = interr.ipnt.z;
                        _methoda( ytd->world, YWM_FORCEFEEDBACK, &ffb );
                        }
                    }

                if( sp < 0.82 ) {

                    /*** wegrutschen ***/
                    pos->x = old->x - interr.sklt->PlanePool[ interr.pnum ].A * 10;
                    pos->z = old->z - interr.sklt->PlanePool[ interr.pnum ].C * 10;
                    }
                else {

                    /*** Stehenbleiben ***/
                    dof->v = 0.0;
                    ytd->bact->act_force = 0.0;
                    *pos = *old;
                    }
                }
            else {
                //pos->x += dof->v * dof->x * avm->time * METER_SIZE;
                //pos->y += dof->v * dof->y * avm->time * METER_SIZE;
                //pos->z += dof->v * dof->z * avm->time * METER_SIZE;
                ytd->bact->act_force = 0.0;
                }
            return( 1L );
            }
        }
    else {

        /*** Abhang ***/
        if (USER ) {

            interr.ipnt.x = pos->x + interr.vec.x;
            interr.ipnt.y = pos->y + interr.vec.y - tan_fkt * ytd->bact->viewer_radius;
            interr.ipnt.z = pos->z + interr.vec.z;
            Schub_Rechts = TRUE;
            }
        else {
            return( 1L );
            }
        }

    /*** und gleich noch eine! ***/
    _methoda( ytd->world, YWM_INTERSECT, &interh );

    if( interh.insect ) {

        if( fabs(interh.sklt->PlanePool[interh.pnum].B) < YPS_PLANE ) {
            ytd->bact->act_force = 0.0;

            /*** krachen lassen, wenn wir auch rückwärts gefahren sind ***/
            if( ytd->bact->dof.v < -NOISE_SPEED / 3 ) {

                if( USER ) {

                    struct yw_forcefeedback_msg ffb;

                    _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHLAND );

                    ffb.type    = YW_FORCE_COLLISSION;
                    ffb.power   = 1.0;
                    ffb.dir_x   = interh.ipnt.x;
                    ffb.dir_y   = interh.ipnt.z;
                    _methoda( ytd->world, YWM_FORCEFEEDBACK, &ffb );
                    }
                }

            /*** nur weg von der Wand ! ***/
            pos->x += dof->v * dof->x * avm->time * METER_SIZE;
            pos->y += dof->v * dof->y * avm->time * METER_SIZE;
            pos->z += dof->v * dof->z * avm->time * METER_SIZE;

            return( 3L );
            }
        }
    else {

        /* es geht ins Leere. Richtung ist die Steilwand in Testrichtung */
        interh.ipnt.x = pos->x + interh.vec.x;
        interh.ipnt.y = pos->y + interh.vec.y - tan_fkt * ytd->bact->viewer_radius;
        interh.ipnt.z = pos->z + interh.vec.z;
        Schub_Hinten = TRUE;
        }

    /*** Spezieller test (vor allem wegen ecken) ***/
    if( VIEW && (VV > 0.0) ) {

        struct intersect_msg interv;

        interv.pnt.x = ytd->bact->pos.x;
        interv.pnt.y = ytd->bact->pos.y;
        interv.pnt.z = ytd->bact->pos.z;
        interv.vec.x = ytd->bact->dir.m31 * ytd->bact->viewer_radius;
        interv.vec.y = ytd->bact->dir.m32 * ytd->bact->viewer_radius;
        interv.vec.z = ytd->bact->dir.m33 * ytd->bact->viewer_radius;
        interv.flags = 0; // da Viewer

        _methoda( ytd->world, YWM_INTERSECT, &interv );
        if( interv.insect &&
            (interv.sklt->PlanePool[ interv.pnum ].B < YPS_PLANE) ) {

            if( ytd->bact->dof.v > NOISE_SPEED / 3 ) {

                if( USER ) {

                    struct yw_forcefeedback_msg ffb;

                    _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHLAND );

                    ffb.type    = YW_FORCE_COLLISSION;
                    ffb.power   = 1.0;
                    ffb.dir_x   = interv.ipnt.x;
                    ffb.dir_y   = interv.ipnt.z;
                    _methoda( ytd->world, YWM_FORCEFEEDBACK, &ffb );
                    }
                }

            *pos = *old;
            ytd->bact->act_force = 0;
            dof->v = 0.0;
            }
        }

    /*
    ** +++++++++++++++++++++++ DREHUNG / NEIGUNG +++++++++++++++++++++++++++++
    */

    /* --------------------------------------------------------------------------
    ** Fläche ermitteln. Die wird durch die drei ipnt's festgelegt. Wir ermitteln
    ** den Normalenvektor aus dem Vektorprodukt der Differenzvektoren.
    ** ------------------------------------------------------------------------*/

    if( interr.insect && interl.insect && interh.insect ) {

        dvl.x = interl.ipnt.x - interh.ipnt.x;
        dvl.y = interl.ipnt.y - interh.ipnt.y;
        dvl.z = interl.ipnt.z - interh.ipnt.z;
        dvr.x = interr.ipnt.x - interh.ipnt.x;
        dvr.y = interr.ipnt.y - interh.ipnt.y;
        dvr.z = interr.ipnt.z - interh.ipnt.z;

        A = dvl.y * dvr.z - dvl.z * dvr.y;
        B = dvl.z * dvr.x - dvl.x * dvr.z;
        C = dvl.x * dvr.y - dvl.y * dvr.x;

        /*** VP ist richtungsabhängig! ***/
        betrag = VV * nc_sqrt( A*A + B*B + C*C );

        if( fabs(betrag) > 0.0001 ) {

            A /= betrag;    B /= betrag;    C /= betrag;
            }
        else {

            A = 0.0;    B = 1.0;    C = 0.0;
            }

        /*** Wegen umkippen ***/
        if( B < -0.1 ) {

            A *= -1.0;    B *= -1.0;    C *= -1.0;
            }

        if( ytd->flags & YTF_Tip ) {

            ra.x = A; ra.y = B; ra.z = C;
            yt_TipTank( ytd, avm->time, &ra );
            A = ra.x; B = ra.y; C = ra.z;
            }

        /*** nun kommt die Rotationsachse... ***/
        rot.x = dir->m22 * C - dir->m23 * B;
        rot.y = dir->m23 * A - dir->m21 * C;
        rot.z = dir->m21 * B - dir->m22 * A;
                
        /*** Rundung ***/
        if( fabs( ytd->bact->dof.v ) < 5.0 )
            round_rot = 0.01;
        else
            round_rot = 0.007;

        if( (betrag = nc_sqrt( rot.x*rot.x + rot.y*rot.y + rot.z*rot.z )) > 0.0) {

            rot.x /= betrag;    rot.y /= betrag;    rot.z /= betrag;

            /* ... und der Rotationswinkel */
            angle = nc_acos( A * dir->m21 + B * dir->m22 + C * dir->m23 );

            if( angle > 2 * ytd->bact->max_rot * avm->time )
                angle = 2 * ytd->bact->max_rot * avm->time;

            if (fabs(angle) > round_rot) {

                ca = cos(  angle );            sa = sin( -angle );

                rm.m11 = ca + (1-ca) * rot.x * rot.x;
                rm.m12 = (1-ca) * rot.x * rot.y - sa * rot.z;
                rm.m13 = (1-ca) * rot.z * rot.x + sa * rot.y;
                rm.m21 = (1-ca) * rot.x * rot.y + sa * rot.z;
                rm.m22 = ca + (1-ca) * rot.y * rot.y;
                rm.m23 = (1-ca) * rot.y * rot.z - sa * rot.x;
                rm.m31 = (1-ca) * rot.z * rot.x - sa * rot.y;
                rm.m32 = (1-ca) * rot.y * rot.z + sa * rot.x;
                rm.m33 = ca + (1-ca) * rot.z * rot.z;

                nc_m_mul_m( &(ytd->bact->dir), &rm, &nm );

                ytd->bact->dir = nm;
                }
            }
        }

    /*
    ** ++++++++++++++++++++++++++++ FALLEN ? +++++++++++++++++++++++++++++
    */

    if( !interl.insect && !interr.insect && !interh.insect ) {

        ytd->bact->ExtraState &= ~EXTRA_LANDED;
        return( 0L );
        }

    /* -----------------------------------------------------------
    ** Zusätzlich eine Intersection, um eine eindeutige Aussage zu
    ** haben. Diesen test machen wir, wenn mindestens eine Inter-
    ** section ins leere geht.
    ** ---------------------------------------------------------*/
    if( Schub_Links || Schub_Rechts || Schub_Hinten ) {

        struct intersect_msg zusatz;
        FLOAT  l;

        l = ytd->bact->viewer_over_eof * lfactor * 0.8;

        zusatz.pnt.x = pos->x;  zusatz.pnt.y = pos->y;  zusatz.pnt.z = pos->z;
        zusatz.vec.x = 0;       zusatz.vec.z = 0;
        zusatz.pnt.y -= l;      zusatz.vec.y = 2 * l;
        if( VIEW )
            zusatz.flags = 0;
        else
            zusatz.flags = INTERSECTF_CHEAT;

        _methoda( ytd->world, YWM_INTERSECT, &zusatz );

        if( (!zusatz.insect) ||
            (zusatz.insect && (zusatz.sklt->PlanePool[zusatz.pnum].B<YPS_PLANE)) ) {

            struct flt_triple mvec;

            /*** Fallen... ***/
            ytd->bact->ExtraState &= ~EXTRA_LANDED;

            /*** ... und noch einen kleinen Impuls ***/
            mvec.x = mvec.y = mvec.z = 0.0;

            if( Schub_Links ) {

                mvec.x += (VV * ytd->bact->dir.m31 - ytd->bact->dir.m11);
                mvec.y += (VV * ytd->bact->dir.m32 - ytd->bact->dir.m12);
                mvec.z += (VV * ytd->bact->dir.m33 - ytd->bact->dir.m13);
                }

            if( Schub_Rechts ) {

                mvec.x += (VV * ytd->bact->dir.m31 + ytd->bact->dir.m11);
                mvec.y += (VV * ytd->bact->dir.m32 + ytd->bact->dir.m12);
                mvec.z += (VV * ytd->bact->dir.m33 + ytd->bact->dir.m13);
                }

            if( Schub_Hinten ) {

                mvec.x -= VV * ytd->bact->dir.m31;
                mvec.y -= VV * ytd->bact->dir.m32;
                mvec.z -= VV * ytd->bact->dir.m33;
                }

            ytd->bact->pos.x += mvec.x * avm->time * 400.0;
            ytd->bact->pos.y += mvec.y * avm->time * 400.0;
            ytd->bact->pos.z += mvec.z * avm->time * 400.0;

            return( 0L );
            }
        else {

            /*** y-Korrektur ***/
            ytd->bact->pos.y = zusatz.ipnt.y - over_eof;
            }
        }
    else {

        /*** Korrektur der y-Position ***/
        ytd->bact->pos.y = (interl.ipnt.y + interr.ipnt.y + interh.ipnt.y) / 3 - over_eof;
        }

    return( 0L );
}


void yt_RotateTank( struct Bacterium *bact, FLOAT angle, UBYTE wie )
{
    /* rotiert den Panzer um seine lokale y-Achse */

    if( wie == 0 )
        yt_rot_round_lokal_y( bact, (FLOAT)  angle );
    else
        yt_rot_round_lokal_y( bact, (FLOAT) -angle);
}


void yt_TipTank( struct ypatank_data *ytd, float time, struct flt_triple *rot )
{
    FLOAT angle, force, abs_force, ca, sa, ta;
    struct flt_triple merke, rrr;
    struct flt_m3x3 rm;
    ULONG VIEW;

    /*
    ** Übergeben wird die Wunsch-y-Achse. Wir drehen diese um die derzeitige
    ** lokale x-Achse je nach resultierender Kraft.
    */
    _get( ytd->bact->BactObject, YBA_Viewer, &VIEW );
    if( VIEW )
        ta = -0.4;
    else
        ta = -0.2;

    /*** resultierende Kraft ***/
    force = ytd->bact->act_force - ytd->bact->air_const * ytd->bact->dof.v;
    if( (abs_force = fabs( force )) > 3.0 ) {

        FLOAT v = force / ytd->bact->max_force;

        angle = ta * v;

        /*** Rotachse ist lokale x-Achse ***/
        rrr.x = ytd->bact->dir.m11;
        rrr.y = ytd->bact->dir.m12;
        rrr.z = ytd->bact->dir.m13;

        ca = cos(  angle );            sa = sin( -angle );

        rm.m11 = ca + (1-ca) * rrr.x * rrr.x;
        rm.m12 = (1-ca) * rrr.x * rrr.y - sa * rrr.z;
        rm.m13 = (1-ca) * rrr.z * rrr.x + sa * rrr.y;
        rm.m21 = (1-ca) * rrr.x * rrr.y + sa * rrr.z;
        rm.m22 = ca + (1-ca) * rrr.y * rrr.y;
        rm.m23 = (1-ca) * rrr.y * rrr.z - sa * rrr.x;
        rm.m31 = (1-ca) * rrr.z * rrr.x - sa * rrr.y;
        rm.m32 = (1-ca) * rrr.y * rrr.z + sa * rrr.x;
        rm.m33 = ca + (1-ca) * rrr.z * rrr.z;

        /*** Nun Vektor mit Matrix multiplizieren ***/
        merke.x = rm.m11 * rot->x + rm.m12 * rot->y + rm.m13 * rot->z;
        merke.y = rm.m21 * rot->x + rm.m22 * rot->y + rm.m23 * rot->z;
        merke.z = rm.m31 * rot->x + rm.m32 * rot->y + rm.m33 * rot->z;

        rot->x = merke.x;
        rot->y = merke.y;
        rot->z = merke.z;
        }
}



_dispatcher( void, yt_YBM_RECOIL, struct recoil_msg *rec)
{
    /*
    **  FUNCTION:   Tanks haben keine Auftriebskraft (aha!!!), Somit sieht
    **              das Abprallen z.T. schlechter aus. Somit verstärken
    **              wir einfach das Abprallen.
    **              Zuerst rufen wir die Supermethode auf. Dann geben wir
    **              einen Zusatzimpuls in Richtung des Abprallvektors.
    **              Nur die geschwindigkeit verstärken klappt nicht, weil
    **              der geschwindigkeitsvektor durch die Schwerkraft
    **              immer flacher wird!
    **
    */

    struct ypatank_data *ytd;
    ytd = INST_DATA(cl, o );

    if( ytd->bact->ExtraState & EXTRA_LANDED ) return;

    _supermethoda( cl, o, YBM_RECOIL, rec );

    /*** Verstärkung ***/
    ytd->bact->dof.x = ytd->bact->dof.x * ytd->bact->dof.v - rec->vec.x * 5;
    ytd->bact->dof.y = ytd->bact->dof.y * ytd->bact->dof.v - rec->vec.y * 5;
    ytd->bact->dof.z = ytd->bact->dof.z * ytd->bact->dof.v - rec->vec.z * 5;

    ytd->bact->dof.v = nc_sqrt( ytd->bact->dof.x * ytd->bact->dof.x +
                                ytd->bact->dof.y * ytd->bact->dof.y +
                                ytd->bact->dof.z * ytd->bact->dof.z );
    if( ytd->bact->dof.v > 0.001 ) {

        ytd->bact->dof.x /= ytd->bact->dof.v;
        ytd->bact->dof.y /= ytd->bact->dof.v;
        ytd->bact->dof.z /= ytd->bact->dof.v;
        }
}
