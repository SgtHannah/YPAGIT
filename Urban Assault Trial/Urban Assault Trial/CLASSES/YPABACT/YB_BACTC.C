/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Diverses...
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

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


/* Brot-O-Typen */
yb_PickupEnergy( struct Bacterium *Me, struct Bacterium *batzen );


/*** Globale Arrays ***/
struct flt_triple richtung = { 0.0, 0.0, 0.0}, vec = {0.0, 0.0, 0.0};
struct flt_triple schwerpunkt = { 0.0, 0.0, 0.0}, weg = {0.0, 0.0, 0.0};



_dispatcher( BOOL, yb_YBM_BACTCOLLISION, struct bactcollision_msg *bcoll )
{
/*
**  FUNCTION    Testet, ob wir mit einem Bakterium zusammengeknallt sind
**              und  reagiert zum Teil. SEHR KLASSENSPEZIFISCH!
**
**              Neu (wieder mal): Ich ermittle aus allen, die in meinem
**              Radius reingeknallt sind eine imaginäre Wand, von der
**              ich wie bei 'ner richtigen Wand abralle.
**
**              Neu. Als Robo (ecoll) pralle ich nur ab, wenn ich auf andere
**              ecolls treffe!
**
**  INPUT
**
**  RESULT      TRUE, wenns geknallt hat
**
**  CHANGED     3-Dez-95   8100 000C created
*/

    struct ypabact_data *ybd;
    FLOAT  kandidat_radius, radius, abstand, time;
    struct Bacterium *kandidat;
    struct recoil_msg rec;
    ULONG  VIEW, zaehler, testcount;
    struct ExtCollision *ecoll;
    struct flt_triple pt;
    ULONG  Iam_ecoll, youare_ecoll;



    /*
    ** Vorbereitungen
    */

    ybd = INST_DATA(cl, o);

    _get( ybd->bact.BactObject, YBA_Viewer, &VIEW );
    if( VIEW ) {
        radius = ybd->bact.viewer_radius;
        }
    else {
        radius = ybd->bact.radius;
        }

    youare_ecoll = Iam_ecoll = (ULONG) FALSE;
    _get( o, YBA_ExtCollision, &Iam_ecoll );    // weil NULL == FALSE...


    /*** Wenn wir uns nicht bewegen brauchen wir auch nicht zu testen ***/
    if( ybd->bact.dof.v == 0 ) return( FALSE );

    /*** Wer is'n so da? ***/
    kandidat      = (struct Bacterium *) ybd->bact.Sector->BactList.mlh_Head;
    zaehler       = 0;
    schwerpunkt.x = 0.0;
    schwerpunkt.y = 0.0;
    schwerpunkt.z = 0.0;
    time          = (FLOAT) bcoll->frame_time / 1000.0;
    while( kandidat->SectorNode.mln_Succ ) { 
    
        BOOL plasma;

        /*** ist das einer dieser energiebatzen??? ***/
        if( (ACTION_DEAD == kandidat->MainState) &&
            (EVF_Active   & kandidat->extravp[0].flags) &&
            (ybd->flags   & YBF_UserInput) &&
            (0            < kandidat->scale_time) ) 
            plasma = TRUE;
        else
            plasma = FALSE;     
            
            
        /*** Bin ich dat? ***/
        if( kandidat->BactObject == o ) {

            kandidat = (struct Bacterium *)kandidat->SectorNode.mln_Succ;
            continue;
            }

        /*** Isses 'ne Rakete? oder ein Megatötler? ***/
        if( (kandidat->BactClassID == BCLID_YPAMISSY) ||
            (_methoda( kandidat->BactObject, YBM_TESTDESTROY, NULL) &&
             (plasma == FALSE)) ) {

            kandidat = (struct Bacterium *)kandidat->SectorNode.mln_Succ;
            continue;
            }

        /*
        ** Nun testen wir mit allen Kollisionspunkten der anderen. Wenn es sich
        ** im Spiel als Sinnvoll herausstellt, auch bei mir das erweiterte 
        ** Kollisionsmodell zu beachten, tue ich das, aber meist fliegen die anderen
        ** in der Nähe des Robo
        */

        _get( kandidat->BactObject, YBA_ExtCollision, &ecoll );

        if( ecoll ) {
            testcount       = ecoll->number;
            youare_ecoll    = TRUE;
            }
        else {
            testcount       = 1;
            }

        /*** Nun alle Testpunkte ***/
        while( testcount-- ) {

            if( ecoll ) {

                /*** Der Punkt ***/
                pt    = kandidat->pos;
                pt.x += ( kandidat->dir.m11 * ecoll->points[ testcount ].point.x +
                          kandidat->dir.m21 * ecoll->points[ testcount ].point.y +
                          kandidat->dir.m31 * ecoll->points[ testcount ].point.z );
                pt.y += ( kandidat->dir.m12 * ecoll->points[ testcount ].point.x +
                          kandidat->dir.m22 * ecoll->points[ testcount ].point.y +
                          kandidat->dir.m32 * ecoll->points[ testcount ].point.z );
                pt.z += ( kandidat->dir.m13 * ecoll->points[ testcount ].point.x +
                          kandidat->dir.m23 * ecoll->points[ testcount ].point.y +
                          kandidat->dir.m33 * ecoll->points[ testcount ].point.z );
                
                /*** Der Radius ***/
                if( ecoll->points[ testcount ].radius < 0.01 ) continue;
                kandidat_radius = ecoll->points[ testcount ].radius;
                }
            else {
                pt = kandidat->pos;
                kandidat_radius = radius;   // Viewer schon eingerechnet
                }

            /*** In Reichweite? ***/
            weg.x = ybd->bact.pos.x - pt.x;
            weg.y = ybd->bact.pos.y - pt.y;
            weg.z = ybd->bact.pos.z - pt.z;
            
            abstand = nc_sqrt( weg.x*weg.x + weg.y*weg.y + weg.z*weg.z );

            if( abstand <= (radius + kandidat_radius) ) {
                
                if( plasma ) {
                                
                    struct ypaworld_data *ywd;
                    
                    ywd = INST_DATA( ((struct nucleusdata *)(ybd->world))->o_Class,
                                     ybd->world );
                        
                    /* ---------------------------------------------
                    ** Als user ueber einen dieser batzen gefahren.
                    ** Mir die Energie goennen, dann den ausschalten
                    ** (Zeit runtersetzen) und evtl. noch nen Sound
                    ** abspielen
                    ** -------------------------------------------*/ 
                    
                    /*** Energy je nach scaltetime ***/
                    yb_PickupEnergy( &(ybd->bact), kandidat );
                    
                    /*** deaktivieren ***/
                    kandidat->scale_time = -1;
                    
                    /*** Sound ??? ***/
                    if( ywd->gsr )
                        _StartSoundSource( &(ywd->gsr->ShellSound2), 
                                           SHELLSOUND_PLASMA);
                                     
                    #ifdef __NETWORK__
                    if( ywd->playing_network ) { 
                        
                        struct sendmessage_msg sm;
                        struct ypamessage_endplasma ep;
                        
                        /*** Message versenden ***/
                        ep.generic.message_id = YPAM_ENDPLASMA;
                        
                        /*** beim kandidat muss es beendet werden ***/
                        ep.generic.owner      = kandidat->Owner;
                        ep.ident              = kandidat->ident;
                            
                        sm.receiver_id        = NULL;
                        sm.receiver_kind      = MSG_ALL;
                        sm.data               = &ep;
                        sm.data_size          = sizeof( ep );
                        sm.guaranteed         = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
            
                        /* ------------------------------------------------
                        ** Das is ein Netzvehicle. Es kann dauern, bis ein
                        ** release zurueckkommt. deshalb optik ausschalten.
                        ** Nur, wenn es kein Eigenes ist!!!!
                        ** ----------------------------------------------*/
                        if( ybd->bact.Owner != kandidat->Owner ) {
                            kandidat->extravp[0].vis_proto = NULL;
                            kandidat->extravp[0].flags     = 0L;
                            kandidat->extravp[0].vp_tform  = NULL;
                            }
                        }                 
                        #endif
                    break;     
                    }
                else {
                    
                    /*** Jo, normales Vehicle, den addieren wir drauf ***/
                    schwerpunkt.x += pt.x;
                    schwerpunkt.y += pt.y;
                    schwerpunkt.z += pt.z;

                    zaehler++;  
                    }
                }
            }

        /*** Nun der nächste testkandidat ***/
        kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
        }

    /*** Überhaupt jemanden getroffen ***/
    if( zaehler == 0 ) {
    
        ybd->bact.ExtraState &= ~EXTRA_BACTCRASH;
        return( FALSE );
        }

    /*** Lohnt es abzuprallen ***/
    if( Iam_ecoll && (!( (BOOL) youare_ecoll)) ) {
        
        ybd->bact.ExtraState &= ~EXTRA_BACTCRASH;
        return( FALSE );
        }

    /*** Da sind ein paar Leute ***/
    schwerpunkt.x /= zaehler;
    schwerpunkt.y /= zaehler;
    schwerpunkt.z /= zaehler;

    weg.x = schwerpunkt.x - ybd->bact.pos.x;
    weg.y = schwerpunkt.y - ybd->bact.pos.y;
    weg.z = schwerpunkt.z - ybd->bact.pos.z;
    abstand = nc_sqrt( weg.x*weg.x + weg.y*weg.y + weg.z*weg.z );
    if( abstand < 0.0001 ) return( FALSE );


    rec.vec.x = weg.x / abstand;
    rec.vec.y = weg.y / abstand;
    rec.vec.z = weg.z / abstand;
    
    /*** Fahren wir vom Schwerpunkt weg? ***/
    if( nc_acos( ybd->bact.dof.x * weg.x + ybd->bact.dof.y * weg.y +
                 ybd->bact.dof.z * weg.z ) > 1.5708 ) return( FALSE );

    rec.time  = time;
    rec.mul_y = 2.0;

    /*** Bei Komplexen Objekten prallen wir mehr zurück! ***/
    if( ecoll )
        rec.mul_v = 1.2;
    else
        rec.mul_v = 0.8;

    /*** Sound und Feedback ***/
    if( !(ybd->bact.ExtraState & EXTRA_BACTCRASH) ) {

        /*** Feedback ***/
        if( VIEW ) {

            struct yw_forcefeedback_msg ffb;

            /*** Sound nur bei User ***/
            _StartSoundSource( &(ybd->bact.sc), VP_NOISE_CRASHVHCL );
            ybd->bact.ExtraState |= EXTRA_BACTCRASH;

            ffb.type    = YW_FORCE_COLLISSION;
            ffb.power   = 1.0;
            ffb.dir_x   = schwerpunkt.x;
            ffb.dir_y   = schwerpunkt.z;
            _methoda( ybd->world, YWM_FORCEFEEDBACK, &ffb );
            }
        }

    /* ----------------------------------------------------------------
    ** Hack: bei dof.v == 0 gibt es kein Abprallen. Wir müssen uns aber
    ** irgendwie bewegen. Da mach mr mal nen Hack...
    ** --------------------------------------------------------------*/
    if( fabs( ybd->bact.dof.v ) < 0.1 ) ybd->bact.dof.v = 1.0;

    _methoda( o, YBM_RECOIL, &rec );

    /*** Trägheit im Wegfliegen (vor allem wegen schnellen Flugzeugen) ***/
    ybd->bact.tar_vec.x = ybd->bact.dof.x;
    ybd->bact.tar_vec.y = ybd->bact.dof.y;
    ybd->bact.tar_vec.z = ybd->bact.dof.z;
    ybd->bact.time_ai1  = ybd->bact.internal_time;
    ybd->bact.time_ai2  = ybd->bact.internal_time;

    return( TRUE );
}


yb_PickupEnergy( struct Bacterium *Me, struct Bacterium *batzen )
{              
    LONG   sc_time, energy;
    FLOAT  factor;

    sc_time = (LONG)( PLASMA_TIME * (FLOAT)batzen->Maximum);
        if( sc_time < PLASMA_MINTIME )
                sc_time = PLASMA_MINTIME;
        if( sc_time > PLASMA_MAXTIME )
                sc_time = PLASMA_MAXTIME;

    factor  = PLASMA_ENERGY * (FLOAT)batzen->scale_time / (FLOAT)sc_time;
    energy  = (LONG)(factor * (FLOAT)batzen->Maximum);

    /*** Energy zuerst auf mich ***/
    if( (Me->Energy + energy) <= Me->Maximum ) {
    
        Me->Energy += energy;
        }
    else {
        
        struct yparobo_data *yrd;
        yrd = INST_DATA( ((struct nucleusdata *)(Me->robo))->o_Class,
                         Me->robo );   
        
        energy -= (Me->Maximum - Me->Energy);
        Me->Energy = Me->Maximum;
        
        /*** Es bleibt noch was fuer die Systembatterie ***/
        if( (yrd->bact->Energy + energy) <= yrd->bact->Maximum ) {
        
            yrd->bact->Energy += energy;
            }
        else { 
        
            energy -= (yrd->bact->Maximum - yrd->bact->Energy);
            yrd->bact->Energy = yrd->bact->Maximum;
            
            /*** Es bleibt noch was fuer die VhclBatt ***/
            if( (yrd->BattVehicle + energy) < yrd->bact->Maximum ) {
            
                yrd->BattVehicle += energy;
                }
            else {
                
                energy -= (yrd->bact->Maximum - yrd->BattVehicle);
                yrd->BattVehicle = yrd->bact->Maximum;
                            
                /*** da bleibt noch was fuer die BeamBatterie ***/
                yrd->BattBeam += energy;
                if( yrd->BattBeam > yrd->bact->Maximum )
                    yrd->BattBeam = yrd->bact->Maximum;
                }
            }
        }
}                        
                                

