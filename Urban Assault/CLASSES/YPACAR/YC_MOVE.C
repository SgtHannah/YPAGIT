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
#include "ypa/ypacarclass.h"
#include "audio/audioengine.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

void yc_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle);
BOOL yc_BactCollision(struct ypacar_data *ycd, FLOAT time );
void yc_TipCar( struct ypacar_data *ycd, struct alignvehicle_msg *msg, 
                                         struct flt_triple *rot );
void yc_rot_round_lokal_x( struct Bacterium *bact, float angle );



_dispatcher( BOOL, yc_YTM_ALIGNVEHICLE_A, struct alignvehicle_msg *avm)
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
*/

    ULONG VIEW, TIP;
    struct intersect_msg inter;
    FLOAT over_eof, betrag;
    FLOAT A, B, C, ca, sa, angle, rot_x, rot_y, rot_z;
    struct flt_m3x3 rm, nm;
    struct flt_triple ra;

    struct ypacar_data *ycd = INST_DATA( cl, o);
    avm->ret_msg = 0;

    _get( o, YBA_Viewer, &VIEW );
    if( VIEW )
        over_eof = ycd->bact->viewer_over_eof;
    else
        over_eof = ycd->bact->over_eof;

    inter.pnt.x = ycd->bact->pos.x + ycd->bact->dir.m21 * over_eof;
    inter.pnt.y = ycd->bact->pos.y + ycd->bact->dir.m22 * over_eof - 50;
    inter.pnt.z = ycd->bact->pos.z + ycd->bact->dir.m23 * over_eof;

    inter.vec.x = 0.0;
    inter.vec.y = 110 + ycd->bact->over_eof;
    inter.vec.z = 0.0;
    inter.flags = 0;

    _methoda( ycd->world, YWM_INTERSECT, &inter );

    if( inter.insect ) {

        /*** War es ein guter Schnitt? ***/
        if( inter.sklt->PlanePool[ inter.pnum ].B >= YPS_PLANE ) {

            /* -------------------------------------------------------------------
            ** Wir haben Boden unter den Füßen. Wir setzen uns auf ipnt - over_eof
            ** und richten uns, sofern wir sichtbar sind, aus. 
            ** -----------------------------------------------------------------*/

            A = inter.sklt->PlanePool[ inter.pnum ].A;
            B = inter.sklt->PlanePool[ inter.pnum ].B;
            C = inter.sklt->PlanePool[ inter.pnum ].C;

            /*** autonome Autos tippen nich'... ***/
            _get( o, YTA_Tip, &TIP );
            //if( TIP ) {
            //
            //    ra.x = A; ra.y = B; ra.z = C;
            //    yc_TipCar( ycd, avm, &ra );
            //    A = ra.x; B = ra.y; C = ra.z;
            //    }

            rot_x = ycd->bact->dir.m22 * C - ycd->bact->dir.m23 * B;
            rot_y = ycd->bact->dir.m23 * A - ycd->bact->dir.m21 * C;
            rot_z = ycd->bact->dir.m21 * B - ycd->bact->dir.m22 * A;

            betrag = nc_sqrt( rot_x * rot_x + rot_y * rot_y + rot_z * rot_z );

            if( betrag > 0.0 ) {

                /* ... und der Rotationswinkel */
                angle = nc_acos( A * ycd->bact->dir.m21 + B * ycd->bact->dir.m22 +
                                 C * ycd->bact->dir.m23 );

                if( angle > 2 * ycd->bact->max_rot * avm->time )
                    angle = 2 * ycd->bact->max_rot * avm->time;
                if( fabs( angle ) < 1e-02 ) angle = 0.0;

                /* eine Winkelorientierung brauchen wir, so glaube ich, nicht zu machen.
                ** das übernimmt die Orientierung der Rotationsachse. Drehen wir! */

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

                nc_m_mul_m( &(ycd->bact->dir), &rm, &nm );

                ycd->bact->dir = nm;
                }

            /* --------------------------------------------------------
            ** Wegen dem "Nachhintenkippen" des Mittelpunktes muß alles
            ** global-y ausgerichtet sein!!!
            ** Weil der testpunkt nun am Boden liegt, probiere ich es
            ** nochmal
            ** ------------------------------------------------------*/
            ycd->bact->pos.x = inter.ipnt.x - ycd->bact->dir.m21 * over_eof;
            ycd->bact->pos.y = inter.ipnt.y - ycd->bact->dir.m22 * over_eof;
            ycd->bact->pos.z = inter.ipnt.z - ycd->bact->dir.m23 * over_eof;
            return( TRUE );
            }
        else {

            /*** Schnitt, aber Abhang ***/
            avm->ret_msg |= AVMRET_SLOPE;
            avm->slope.x  = inter.sklt->PlanePool[ inter.pnum ].A;
            avm->slope.y  = inter.sklt->PlanePool[ inter.pnum ].B;
            avm->slope.z  = inter.sklt->PlanePool[ inter.pnum ].C;
            return( FALSE );
            }
        }
    else {

        /*** nix gefunden, Griff ins leere ***/
        return( FALSE );
        }
}



_dispatcher( LONG, yc_YTM_ALIGNVEHICLE_U, struct alignvehicle_msg *avm)
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
    struct flt_triple *pos, *old, phinten, plinks, prechts, rot, ra, dvl, dvr;
    struct uvec *dof;
    FLOAT  tan_fkt = 1.73;       /* 1.73 == 30 Grad */
    FLOAT  lfactor = 1.7;
    ULONG  USER, TIP, VIEW;
    FLOAT  round_rot;
    BOOL   Schub_Links = FALSE, Schub_Rechts = FALSE, Schub_Hinten = FALSE;

    struct ypacar_data *ycd = INST_DATA( cl, o);

    _get( o, YBA_Viewer, &VIEW );
    _get( o, YBA_UserInput, &USER );
    if( VIEW )
        over_eof = ycd->bact->viewer_over_eof;
    else
        over_eof = ycd->bact->over_eof;

    /* Achtung, wir müssen natürlich die Fahrtrichtung beachten */
    pos = &(ycd->bact->pos);
    dir = &(ycd->bact->dir);
    old = &(ycd->bact->old_pos);
    dof = &(ycd->bact->dof);

    if( ycd->bact->dof.v >= 0.0 ) {

        plinks.x  = pos->x+(dir->m31-dir->m11)*ycd->bact->viewer_radius*0.7071;
        plinks.y  = pos->y+(dir->m32-dir->m12)*ycd->bact->viewer_radius*0.7071;
        plinks.z  = pos->z+(dir->m33-dir->m13)*ycd->bact->viewer_radius*0.7071;

        prechts.x = pos->x+(dir->m31+dir->m11)*ycd->bact->viewer_radius*0.7071;
        prechts.y = pos->y+(dir->m32+dir->m12)*ycd->bact->viewer_radius*0.7071;
        prechts.z = pos->z+(dir->m33+dir->m13)*ycd->bact->viewer_radius*0.7071;

        phinten.x = pos->x - dir->m31 * ycd->bact->viewer_radius;
        phinten.y = pos->y - dir->m32 * ycd->bact->viewer_radius;
        phinten.z = pos->z - dir->m33 * ycd->bact->viewer_radius;
        VV = 1.0;
        }
    else {

        plinks.x  = pos->x+(-dir->m31-dir->m11)*ycd->bact->viewer_radius*0.7071;
        plinks.y  = pos->y+(-dir->m32-dir->m12)*ycd->bact->viewer_radius*0.7071;
        plinks.z  = pos->z+(-dir->m33-dir->m13)*ycd->bact->viewer_radius*0.7071;

        prechts.x = pos->x+(-dir->m31+dir->m11)*ycd->bact->viewer_radius*0.7071;
        prechts.y = pos->y+(-dir->m32+dir->m12)*ycd->bact->viewer_radius*0.7071;
        prechts.z = pos->z+(-dir->m33+dir->m13)*ycd->bact->viewer_radius*0.7071;

        phinten.x = pos->x + dir->m31 * ycd->bact->viewer_radius;
        phinten.y = pos->y + dir->m32 * ycd->bact->viewer_radius;
        phinten.z = pos->z + dir->m33 * ycd->bact->viewer_radius;
        VV = -1.0;
        }

    /* Punkte noch runtersetzen */
    plinks.y  += ycd->bact->viewer_over_eof;
    prechts.y += ycd->bact->viewer_over_eof;
    phinten.y += ycd->bact->viewer_over_eof;

    interh.pnt.x=interr.pnt.x=interl.pnt.x=pos->x;
    interh.pnt.y=interr.pnt.y=interl.pnt.y=pos->y-tan_fkt*ycd->bact->viewer_radius;
    interh.pnt.z=interr.pnt.z=interl.pnt.z=pos->z;

    interl.vec.x = (plinks.x  - interl.pnt.x) * lfactor;
    interl.vec.y = (plinks.y  - interl.pnt.y) * lfactor;
    interl.vec.z = (plinks.z  - interl.pnt.z) * lfactor;
    interr.vec.x = (prechts.x - interr.pnt.x) * lfactor;
    interr.vec.y = (prechts.y - interr.pnt.y) * lfactor;
    interr.vec.z = (prechts.z - interr.pnt.z) * lfactor;
    interh.vec.x = (phinten.x - interh.pnt.x) * lfactor;
    interh.vec.y = (phinten.y - interh.pnt.y) * lfactor;
    interh.vec.z = (phinten.z - interh.pnt.z) * lfactor;

    interl.flags = 0;
    interr.flags = 0;
    interh.flags = 0;

    /* ogäj, wir können ... */
    _methoda( ycd->world, YWM_INTERSECT, &interl );

    /*** Nun geht es an die Auswertung der Schnittresultate ***/
    A = 0.0;    B = 0.0;    C = 0.0;

    if( interl.insect ) {

        /* Hindernis? */
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
                
                /*** Wand voraus. Stehenbleiben oder abrutschen? ***/
                if( sp < 0.82 ) {

                    /*** wegrutschen ... ***/
                    pos->x = old->x - interl.sklt->PlanePool[ interl.pnum ].A * 10;
                    pos->z = old->z - interl.sklt->PlanePool[ interl.pnum ].C * 10;
                    }
                else {

                    /*** stehenbleiben ***/
                    dof->v = 0.0;
                    ycd->bact->act_force = 0.0;
                    *pos = *old;
                    }

                /*** ...und krachen lassen ***/
                if( ycd->bact->dof.v > NOISE_SPEED / 3 ) {

                    _StartSoundSource( &(ycd->bact->sc), VP_NOISE_CRASHLAND );

                    if( USER ) {

                        struct yw_forcefeedback_msg ffb;

                        ffb.type    = YW_FORCE_COLLISSION;
                        ffb.power   = 1.0;
                        ffb.dir_x   = interl.ipnt.x;
                        ffb.dir_y   = interl.ipnt.z;
                        _methoda( ycd->world, YWM_FORCEFEEDBACK, &ffb );
                        }
                    }
                }
            else {
                //pos->x += dof->v * dof->x * avm->time * METER_SIZE;
                //pos->y += dof->v * dof->y * avm->time * METER_SIZE;
                //pos->z += dof->v * dof->z * avm->time * METER_SIZE;
                ycd->bact->act_force = 0.0;
                }
            return( 2L );
            }
        }
    else {

        /*** Abhang ***/
        if( USER ) {

            interl.ipnt.x = pos->x + interl.vec.x;
            interl.ipnt.y = pos->y + interl.vec.y - tan_fkt * ycd->bact->viewer_radius;
            interl.ipnt.z = pos->z + interl.vec.z;
            Schub_Links = TRUE;
            }
        else {
            return( 2L );
            }
        }

    /*** und noch eine! ***/
    _methoda( ycd->world, YWM_INTERSECT, &interr );

    if( interr.insect ) {

        /*** Hindernis? ***/
        if( fabs(interr.sklt->PlanePool[interr.pnum].B) < YPS_PLANE ) {

            sp = (dir->m31 * interr.sklt->PlanePool[ interr.pnum ].A +
                  dir->m32 * interr.sklt->PlanePool[ interr.pnum ].B +
                  dir->m33 * interr.sklt->PlanePool[ interr.pnum ].C) * VV;

            if( sp > 0.0 ) {

                if( sp < 0.82 ) {

                    /*** wegrutschen ***/
                    pos->x = old->x - interr.sklt->PlanePool[ interr.pnum ].A * 10;
                    pos->z = old->z - interr.sklt->PlanePool[ interr.pnum ].C * 10;
                    }
                else {

                    /*** Stehenbleiben ***/
                    dof->v = 0.0;
                    ycd->bact->act_force = 0.0;
                    *pos = *old;
                    }

                /*** ...und krachen lassen ***/
                if( ycd->bact->dof.v > NOISE_SPEED / 3 ) {

                    _StartSoundSource( &(ycd->bact->sc), VP_NOISE_CRASHLAND );

                    if( USER ) {

                        struct yw_forcefeedback_msg ffb;

                        ffb.type    = YW_FORCE_COLLISSION;
                        ffb.power   = 1.0;
                        ffb.dir_x   = interr.ipnt.x;
                        ffb.dir_y   = interr.ipnt.z;
                        _methoda( ycd->world, YWM_FORCEFEEDBACK, &ffb );
                        }
                    }
                }
            else {
                //pos->x += dof->v * dof->x * avm->time * METER_SIZE;
                //pos->y += dof->v * dof->y * avm->time * METER_SIZE;
                //pos->z += dof->v * dof->z * avm->time * METER_SIZE;
                ycd->bact->act_force = 0.0;
                }
            return( 1L );
            }
        }
    else {

        /*** Abhang ***/
        if( USER ) {

            interr.ipnt.x = pos->x + interr.vec.x;
            interr.ipnt.y = pos->y + interr.vec.y - tan_fkt * ycd->bact->viewer_radius;
            interr.ipnt.z = pos->z + interr.vec.z;
            Schub_Rechts = TRUE;
            }
        else {
            return( 1L );
            }
        }

    /*** und gleich noch eine! ***/
    _methoda( ycd->world, YWM_INTERSECT, &interh );

    if( interh.insect ) {

        if( fabs(interh.sklt->PlanePool[interh.pnum].B) < YPS_PLANE ) {

            /*** nur weg von der Wand ! ***/
            pos->x += dof->v * dof->x * avm->time * METER_SIZE;
            pos->y += dof->v * dof->y * avm->time * METER_SIZE;
            pos->z += dof->v * dof->z * avm->time * METER_SIZE;
            ycd->bact->act_force = 0.0;

            /*** ...und krachen lassen ***/
            if( ycd->bact->dof.v < -NOISE_SPEED / 3 ) {

                _StartSoundSource( &(ycd->bact->sc), VP_NOISE_CRASHLAND );

                if( USER ) {

                    struct yw_forcefeedback_msg ffb;

                    ffb.type    = YW_FORCE_COLLISSION;
                    ffb.power   = 1.0;
                    ffb.dir_x   = interh.ipnt.x;
                    ffb.dir_y   = interh.ipnt.z;
                    _methoda( ycd->world, YWM_FORCEFEEDBACK, &ffb );
                    }
                }

            return( 3L );
            }
        }
    else {

        /* es geht ins Leere. Richtung ist die Steilwand in Testrichtung */
        interh.ipnt.x = pos->x + interh.vec.x;
        interh.ipnt.y = pos->y + interh.vec.y - tan_fkt * ycd->bact->viewer_radius;
        interh.ipnt.z = pos->z + interh.vec.z;
        Schub_Hinten = TRUE;
        }


    /*** Spezieller test (vor allem wegen ecken) ***/
    if( VIEW && (VV > 0.0) ) {

        struct intersect_msg interv;

        interv.pnt.x = ycd->bact->pos.x;
        interv.pnt.y = ycd->bact->pos.y;
        interv.pnt.z = ycd->bact->pos.z;
        interv.vec.x = ycd->bact->dir.m31 * ycd->bact->viewer_radius;
        interv.vec.y = ycd->bact->dir.m32 * ycd->bact->viewer_radius;
        interv.vec.z = ycd->bact->dir.m33 * ycd->bact->viewer_radius;
        interv.flags = 0;

        _methoda( ycd->world, YWM_INTERSECT, &interv );
        if( interv.insect &&
            (interv.sklt->PlanePool[ interv.pnum ].B < YPS_PLANE) ) {

            if( ycd->bact->dof.v > NOISE_SPEED / 3 ) {

                _StartSoundSource( &(ycd->bact->sc), VP_NOISE_CRASHLAND );

                if( USER ) {

                    struct yw_forcefeedback_msg ffb;

                    ffb.type    = YW_FORCE_COLLISSION;
                    ffb.power   = 1.0;
                    ffb.dir_x   = interv.ipnt.x;
                    ffb.dir_y   = interv.ipnt.z;
                    _methoda( ycd->world, YWM_FORCEFEEDBACK, &ffb );
                    }
                }

            *pos = *old;
            ycd->bact->act_force = 0;
            dof->v = 0.0;
            }
        }

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


        betrag = VV * nc_sqrt( A*A + B*B + C*C );
        if( betrag != 0.0 ) {

            A /= betrag;    B /= betrag;    C /= betrag;
            }
        else {

            A = 0;  B = 1;  C = 0;
            }

        /*** Wegen umkippen ***/
        if( B < -0.1 ) {

            A *= -1.0;    B *= -1.0;    C *= -1.0;
            }

        _get( o, YTA_Tip, &TIP );
        if( TIP ) {

            ra.x = A; ra.y = B; ra.z = C;
            yc_TipCar( ycd, avm, &ra );
            A = ra.x; B = ra.y; C = ra.z;
            }

        /*** nun kommt die Rotationsachse... ***/
        rot.x = dir->m22 * C - dir->m23 * B;
        rot.y = dir->m23 * A - dir->m21 * C;
        rot.z = dir->m21 * B - dir->m22 * A;
                
        /*** Rundung ***/
        if( fabs( ycd->bact->dof.v ) < 0.5 )
            round_rot = 0.01;
        else
            round_rot = 0.007;
        if( (betrag = nc_sqrt( rot.x*rot.x + rot.y*rot.y + rot.z*rot.z )) > 0.0) {

            rot.x /= betrag;    rot.y /= betrag;    rot.z /= betrag;

            /* ... und der Rotationswinkel */
            angle = nc_acos( A * dir->m21 + B * dir->m22 + C * dir->m23 );

            if( angle > 2 * ycd->bact->max_rot * avm->time )
                angle = 2 * ycd->bact->max_rot * avm->time;

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

                nc_m_mul_m( &(ycd->bact->dir), &rm, &nm );

                ycd->bact->dir = nm;
                }
            }
        }

    /*
    ** ++++++++++++++++++++++++++++ FALLEN ? +++++++++++++++++++++++++++++
    */

    if( !interl.insect && !interr.insect && !interh.insect ) {

        ycd->bact->ExtraState &= ~EXTRA_LANDED;
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

        l = ycd->bact->viewer_over_eof * lfactor * 0.8;

        zusatz.pnt.x = pos->x;  zusatz.pnt.y = pos->y;  zusatz.pnt.z = pos->z;
        zusatz.vec.x = 0;       zusatz.vec.z = 0;
        zusatz.pnt.y -= l;      zusatz.vec.y = 2 * l;
        zusatz.flags = 0;
        _methoda( ycd->world, YWM_INTERSECT, &zusatz );

        if( (!zusatz.insect) ||
            (zusatz.insect && (zusatz.sklt->PlanePool[zusatz.pnum].B<YPS_PLANE)) ) {

            struct flt_triple mvec;

            /*** Fallen... ***/
            ycd->bact->ExtraState &= ~EXTRA_LANDED;

            /*** ... und noch einen kleinen Impuls ***/
            mvec.x = mvec.y = mvec.z = 0.0;

            if( Schub_Links ) {

                mvec.x += (VV * ycd->bact->dir.m31 - ycd->bact->dir.m11);
                mvec.y += (VV * ycd->bact->dir.m32 - ycd->bact->dir.m12);
                mvec.z += (VV * ycd->bact->dir.m33 - ycd->bact->dir.m13);
                }

            if( Schub_Rechts ) {

                mvec.x += (VV * ycd->bact->dir.m31 + ycd->bact->dir.m11);
                mvec.y += (VV * ycd->bact->dir.m32 + ycd->bact->dir.m12);
                mvec.z += (VV * ycd->bact->dir.m33 + ycd->bact->dir.m13);
                }

            if( Schub_Hinten ) {

                mvec.x -= VV * ycd->bact->dir.m31;
                mvec.y -= VV * ycd->bact->dir.m32;
                mvec.z -= VV * ycd->bact->dir.m33;
                }

            ycd->bact->pos.x += mvec.x * avm->time * 400.0;
            ycd->bact->pos.y += mvec.y * avm->time * 400.0;
            ycd->bact->pos.z += mvec.z * avm->time * 400.0;

            return( 0L );
            }
        else {

            /*** y-Korrektur ***/
            ycd->bact->pos.y = zusatz.ipnt.y - over_eof;
            }
        }
    else {

        /*** Korrektur der y-Position ***/
        // ycd->bact->pos.x = (interl.ipnt.x + interr.ipnt.x + interh.ipnt.x) / 3 - A * over_eof;
        ycd->bact->pos.y = (interl.ipnt.y + interr.ipnt.y + interh.ipnt.y) / 3 - over_eof;
        // ycd->bact->pos.z = (interl.ipnt.z + interr.ipnt.z + interh.ipnt.z) / 3 - C * over_eof;
        }

    return( 0L );
}



void yc_TipCar( struct ypacar_data *ycd, struct alignvehicle_msg *avm, 
                                         struct flt_triple *rot )
{
    FLOAT  angle, force, abs_force, ca, sa, ta;
    struct flt_triple merke, rrr;
    struct flt_m3x3 rm;
    struct flt_triple new_dir;
    ULONG  VIEW;

    /*
    ** Übergeben wird die Wunsch-y-Achse. Wir drehen diese um die derzeitige
    ** lokale x-Achse je nach resultierender Kraft.
    */
    new_dir.x = ycd->bact->dir.m31;
    new_dir.y = ycd->bact->dir.m32;
    new_dir.z = ycd->bact->dir.m33;

    _get( ycd->bact->BactObject, YBA_Viewer, &VIEW );
    if( VIEW )
        ta = -0.5;
    else
        ta = -0.2;

    /*** Rotation um x ***/
    force = ycd->bact->act_force - ycd->bact->air_const * ycd->bact->dof.v;
    if( (abs_force = fabs( force )) > 0.1 ) {

        angle = ta * force / ycd->bact->max_force;
        
        /*** Rotachse ist lokale x-Achse ***/
        rrr.x = ycd->bact->dir.m11;
        rrr.y = ycd->bact->dir.m12;
        rrr.z = ycd->bact->dir.m13;

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

    /*** Rotation um z ***/

    /*** Wie hat sich die Richtung geändert? ***/
    angle = nc_acos( (new_dir.x*avm->old_dir.x + new_dir.z*avm->old_dir.z ) /
                      nc_sqrt( new_dir.x  * new_dir.x  +
                               new_dir.z  * new_dir.z  ) /
                      nc_sqrt( avm->old_dir.x * avm->old_dir.x +
                               avm->old_dir.z * avm->old_dir.z ) );

    /*** Wie drehen wir nun? ***/
    angle *= fabs( ycd->bact->dof.v) * 0.002 / avm->time;
    
    if( angle > 0.001 ) {

        if( (new_dir.x * avm->old_dir.z - new_dir.z * avm->old_dir.x) < 0.0 )
            angle = -angle;
        
        /*** Rotachse ist lokale z-Achse ***/
        rrr.x = ycd->bact->dir.m31;
        rrr.y = ycd->bact->dir.m32;
        rrr.z = ycd->bact->dir.m33;

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


void yc_rot_round_lokal_y( struct Bacterium *bact, FLOAT angle)
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


void yc_rot_round_lokal_x( struct Bacterium *bact, FLOAT angle)
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




