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
#include "ypa/ypatankclass.h"
#include "audio/audioengine.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine


/* Brot-O-Typen */
void yb_PrallAb( struct ypabact_data *ybd, struct flt_triple *vec, FLOAT time);
void yt_rot_vec_round_lokal_y( struct flt_triple *vec, FLOAT angle);
yt_PickupEnergy( struct Bacterium *Me, struct Bacterium *batzen );


_dispatcher( BOOL, yt_YBM_BACTCOLLISION, struct bactcollision_msg *bcoll )
{
/*
**  FUNCTION    Testet, ob wir mit einem Bakterium zusammengeknallt sind
**              und  reagiert zum Teil. SEHR KLASSENSPEZIFISCH!
**
**              Wiedermal was neues. Dürfte jetzt der 5. oder 6. Versuch sein.
**
**              Ich mache Vorhersagen nur, wenn ich nicht in Formation
**              bin. Ich suche also alle Objekte in meinem Sektor, die in
**              Schussrichtung liegen. Das mache ich der Eindeutigkeit wegen
**              mit dem Skalarprodukt.
**              Ist die Entfernung vom nächstliegenden größer als ein 
**              Subsektor (o.s.ä), dann mache ich nichts, ist sie kleiner als 
**              die Summe der radien, dann hat es geknallt und die Bewegung 
**              muß ausgeschalten werden.
**
**              Achtung, ich beachte nur den aktuellen Sektor!
**
**
**  INPUT
**
**  RESULT      TRUE
**
**  CHANGED     16-Oct-95   8100 000C created
*/

    #define UK_MAX 10

    struct ypatank_data *ytd;
    struct Bacterium *NeXT, *Kandidat, *uk[ UK_MAX ];
    FLOAT  distance, mdist, radien, sp, time, kandrad, testentfernung;
    struct getsectorinfo_msg gsi;
    struct flt_triple pt, richtung, uk_dir[ UK_MAX ];
    BOOL   direct_collision;
    ULONG  testcount, uk_count, USERINPUT;
    struct ExtCollision *ecoll;
    FLOAT  d_angle, d_sp, mwinkel;
    BOOL   tank_coll = FALSE;
    BOOL   critical_collision = FALSE, turn_left, one_moveable = FALSE;


    /* ---------
    ** Vorarbeit
    ** -------*/

    ytd = INST_DATA(cl, o);  

    /* -------------------------------------------------------------
    ** Wenn ich noch nicht gelandet bin, dann gebe ich gleich an die
    ** Superklasse ab!
    ** -----------------------------------------------------------*/
    if( !(ytd->bact->ExtraState & EXTRA_LANDED) ) {

        return( (BOOL) _supermethoda( cl, o, YBM_BACTCOLLISION, bcoll ) );
        }

    _get( o, YBA_UserInput, &USERINPUT);
    
    /*** Winkel, in denen Kollisionen festgestellt werden ***/
    d_angle = 0.6;
    if( USERINPUT )
        d_sp = 0.6;    
    else
        d_sp = 0.82;

    uk_count         = 0;
    time             = (FLOAT) bcoll->frame_time / 1000.0;
    direct_collision = FALSE;
    Kandidat         = NULL;
    testentfernung   = SECTOR_SIZE / 4;
    
    gsi.abspos_x     = ytd->bact->pos.x;
    gsi.abspos_z     = ytd->bact->pos.z;
    
    if( !_methoda( ytd->world, YWM_GETSECTORINFO, &gsi)) return( FALSE );
    mdist = SECTOR_SIZE;    /* nur so */


    /*** Alle Leute mal auf Kollision testen ***/
    NeXT = (struct Bacterium *) gsi.sector->BactList.mlh_Head;
    while( NeXT->SectorNode.mln_Succ ) { 
    
        BOOL plasma;
    
        /*** ist das einer dieser energiebatzen??? ***/
        if( (ACTION_DEAD == NeXT->MainState) &&
            (EVF_Active   & NeXT->extravp[0].flags) &&
            (USERINPUT)  &&
            (0            < NeXT->scale_time) ) 
            plasma = TRUE;
       else
            plasma = FALSE;      
            

        /*** Ist das uninteressant? ***/
        if( (NeXT->BactClassID == BCLID_YPAMISSY) ||
            (_methoda( NeXT->BactObject, YBM_TESTDESTROY, NULL) &&
             (plasma == FALSE) ) ||
            (NeXT == ytd->bact) ) {

            NeXT = (struct Bacterium *) NeXT->SectorNode.mln_Succ;
            continue;
            }

        /*** Woher kommen die Testpunkte ? ***/
        _get( NeXT->BactObject, YBA_ExtCollision, &ecoll );

        if( ecoll )
            testcount = ecoll->number;
        else
            testcount = 1;

        /*** Nun alle Testpunkte ***/
        while( testcount-- ) {

            if( ecoll ) {

                /*** Der Punkt ***/
                pt     = NeXT->pos;
                pt.x += ( NeXT->dir.m11 * ecoll->points[ testcount ].point.x +
                          NeXT->dir.m21 * ecoll->points[ testcount ].point.y +
                          NeXT->dir.m31 * ecoll->points[ testcount ].point.z );
                pt.y += ( NeXT->dir.m12 * ecoll->points[ testcount ].point.x +
                          NeXT->dir.m22 * ecoll->points[ testcount ].point.y +
                          NeXT->dir.m32 * ecoll->points[ testcount ].point.z );
                pt.z += ( NeXT->dir.m13 * ecoll->points[ testcount ].point.x +
                          NeXT->dir.m23 * ecoll->points[ testcount ].point.y +
                          NeXT->dir.m33 * ecoll->points[ testcount ].point.z );
                
                /*** Der Radius ***/
                if( ecoll->points[ testcount ].radius < 0.01 ) continue;
                kandrad = ecoll->points[ testcount ].radius;
                }
            else {

                pt      = NeXT->pos;
                kandrad = NeXT->radius;
                }

            /*** Wo ist der Punkt des Kandidaten ***/
            richtung.x = pt.x - ytd->bact->pos.x;
            richtung.y = pt.y - ytd->bact->pos.y;
            richtung.z = pt.z - ytd->bact->pos.z;

            /*** Ist er nah genug dran? ***/
            if( ( distance = sqrt( richtung.x*richtung.x + 
                                   richtung.y*richtung.y +
                                   richtung.z*richtung.z) ) < testentfernung ) {
                                   
                radien = ytd->bact->radius + kandrad;

                /*** erstmal das plasma gelumpe ***/
                if( plasma && (distance < radien)) {                   

                    struct ypaworld_data *ywd;
                    
                    ywd = INST_DATA( ((struct nucleusdata *)(ytd->world))->o_Class,
                                     ytd->world );
                                
                    /* ---------------------------------------------
                    ** Als user ueber einen dieser batzen gefahren.
                    ** Mir die Energie goennen, dann den ausschalten
                    ** (Zeit runtersetzen) und evtl. noch nen Sound
                    ** abspielen
                    ** -------------------------------------------*/ 
                    
                    /*** Energy je nach scaltetime ***/
                    yt_PickupEnergy( ytd->bact, NeXT );
                    
                    /*** deaktivieren ***/
                    NeXT->scale_time = -1;
                    
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
                        
                        /*** Owner des kandidaten! ***/
                        ep.generic.owner      = NeXT->Owner;
                        ep.ident              = NeXT->ident;  
                            
                        sm.receiver_id        = NULL;
                        sm.receiver_kind      = MSG_ALL;
                        sm.data               = &ep;
                        sm.data_size          = sizeof( ep );
                        sm.guaranteed         = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                        
                        /* ----------------------------------------------
                        ** das ist ja ein netzvehicle!!!!
                        ** Bis ein Release zurueckkommt, das kann dauern.
                        ** Das mache ich aber NUR, WENN ES KEIN EIGENES
                        ** IST!!!! 
                        ** --------------------------------------------*/
                        if( ytd->bact->Owner != NeXT->Owner ) {
                            NeXT->extravp[0].vis_proto = NULL;
                            NeXT->extravp[0].flags     = 0L;
                            NeXT->extravp[0].vp_tform  = NULL;
                            }
                        }                 
                        #endif
                    
                    /*** der naechste ***/
                    break;
                    }
                    
                /*** liegt er in Flug- /Fahrtrichtung? ***/
                sp = richtung.x * ytd->bact->dir.m31 +
                     richtung.y * ytd->bact->dir.m32 +
                     richtung.z * ytd->bact->dir.m33;
                     
                if( distance > 0.01 ) sp /= distance;

                if( fabs( ytd->bact->dof.v ) < IS_SPEED ) {
                    if( ytd->bact->act_force < 0 ) sp = -sp;
                    }
                else {
                    if( ytd->bact->dof.v < 0 ) sp = -sp;
                    }

                /*** an dem Winkel bissel rumspielen ***/
                if( (sp >= d_sp) && ( distance < radien ) ) {                   

                    /* -----------------------------------------------
                    ** merken wenn noch keiner oder dieser naeher ist.
                    ** Eine Unterscheidung "normal"-Formationfahrt"
                    ** machen wir nicht mehr, weil es letztere seit
                    ** dem Wegpunktsystem so nicht mehr gibt. Ausser-
                    ** dem fahren bei Formationen alle in die gleiche
                    ** Richtung.
                    ** ---------------------------------------------*/
                    if( (Kandidat == NULL) || ( distance < mdist) ) {

                        Kandidat = NeXT;
                        mdist    = distance;
                        mwinkel  = nc_acos(sp); 
                        
                        /*** in welcher Richtung liegt das Hindernis? ***/
                        if( ((richtung.x * ytd->bact->dir.m33) -
                             (richtung.z * ytd->bact->dir.m31)) > 0)
                            turn_left = TRUE;
                        else
                            turn_left = FALSE;

                        if( distance < radien ) 
                            direct_collision = TRUE;
                            
                        /* -------------------------------------------------
                        ** da ist einer im fahrtwinkel. faehrt selbiger auch
                        ** in unsere Richtung? dir_z ist fahrtrichtung, weil 
                        ** nur bei Autonomen von Bedeutung.
                        ** Es ist ebenso ein Hindernis es wartet.
                        ** -----------------------------------------------*/
                        if( ((ytd->bact->dir.m31 * Kandidat->dir.m31) +
                             (ytd->bact->dir.m32 * Kandidat->dir.m32) +
                             (ytd->bact->dir.m33 * Kandidat->dir.m33)) < 0)
                             critical_collision = TRUE;
                             
                        if( ACTION_WAIT == Kandidat->MainState )
                            critical_collision = TRUE;
                            
                        /* -------------------------------------------
                        ** wir brauchen mindestens einen, der den Pool
                        ** aufloesen kann!
                        ** -----------------------------------------*/
                        if( Kandidat->wait_for_coll <= 0 )
                            one_moveable = TRUE;
                        }
                        
                    if( (BCLID_YPATANK == NeXT->BactClassID) ||
                        (BCLID_YPACAR  == NeXT->BactClassID) )
                        tank_coll = TRUE;

                    /*** Wenn wir User Sind, dann die Kandidaten merken ***/
                    if( USERINPUT && (!ecoll) && (uk_count < UK_MAX) ) {

                        uk[ uk_count ] = NeXT;
                        if( distance > 0.0001 ) {

                            uk_dir[ uk_count ].x = richtung.x / distance;
                            uk_dir[ uk_count ].y = richtung.y / distance;
                            uk_dir[ uk_count ].z = richtung.z / distance;
                            }
                        else {

                            uk_dir[ uk_count ].x = richtung.x;
                            uk_dir[ uk_count ].y = richtung.y;
                            uk_dir[ uk_count ].z = richtung.z;
                            }
                        uk_count++;
                        }
                    }
                }
            }

        NeXT = (struct Bacterium *) NeXT->SectorNode.mln_Succ;
        }

    /*** Wenn keine Gefahr bestand, hauen wir hier gleich ab. ***/
    if( Kandidat == NULL ) {
        
        ytd->bact->ExtraState &= ~EXTRA_BACTCRASH;
        return( FALSE );
        }

    /*** Auswertung ***/
    if( USERINPUT ) {

        if( direct_collision && uk_count ) {

            ULONG  i = 0;
            FLOAT  dist, e_sp, dist2;
            struct flt_triple d = {0.0, 0.0, 0.0}, *pos, *old, dir;
            struct yw_forcefeedback_msg ffb;


            pos = &(ytd->bact->pos);
            old = &(ytd->bact->old_pos);

            /*** Wer war alles dabei? ***/
            while( i < uk_count ) {

                FLOAT move_way;
                struct ypaworld_data *ywd;

                /* ----------------------------------------------------------
                ** Hack. Im Netzwerk darf ich fremde nicht wegschieben, weil
                ** Positionsänderungen nur von den Originalfahrzeugen gemacht
                ** werden dürfen.
                ** --------------------------------------------------------*/
                #ifdef __NETWORK__
                ywd = INST_DATA( ((struct nucleusdata *)(ytd->world))->o_Class,
                                 ytd->world );
                if( ywd->playing_network && (ytd->bact->Owner != uk[i]->Owner)) {

                    i++;
                    continue;
                    }
                #endif

                /* -------------------------------------------------------------
                ** Wo könnte ich denn hinschieben? Wir machen das
                ** Weil sowohl Kraft als auch EWnergie wichtig sind, nehmen
                ** wir die Summe der beiden möglichen Impulse m*v und F*t
                ** mit ein paar Korrekturen...
                ** -----------------------------------------------------------*/
                move_way = (FLOAT)ytd->bact->act_force * time * 100 +
                           8.0 * ytd->bact->mass * fabs(ytd->bact->dof.v);
                move_way /= uk[ i ]->mass; // jetzt ist das dv für den anderen
                move_way *= time;          // jetzt ist es ein Weg

                if( move_way > 0.05 ) {

                    /*** wir schaffen auch wirklich was ***/
                    struct flt_triple npos;
                    struct intersect_msg i2;

                    /*** evtl. move_way noch richtungskorrigieren ***/
                    npos.x = uk[ i ]->pos.x + uk_dir[ i ].x * move_way;
                    npos.y = uk[ i ]->pos.y + uk_dir[ i ].y * move_way;
                    npos.z = uk[ i ]->pos.z + uk_dir[ i ].z * move_way;

                    /*** Darf er dorthin kommen? ***/
                    i2.pnt.x = uk[ i ]->pos.x;
                    i2.pnt.y = uk[ i ]->pos.y;
                    i2.pnt.z = uk[ i ]->pos.z;
                    i2.vec.x = npos.x - uk[ i ]->pos.x;
                    i2.vec.y = npos.y - uk[ i ]->pos.y;
                    i2.vec.z = npos.z - uk[ i ]->pos.z;
                    i2.flags = 0;   // weil es fuer solche Sachen keine Unsichtbaren
                                    // Waende geben sollte
                    _methoda( ytd->world, YWM_INTERSECT, &i2 );
                    if( !i2.insect ) {

                        /*** er darf ***/
                        uk[ i ]->old_pos = uk[ i ]->pos;
                        uk[ i ]->pos     = npos;

                        /*** er kann ja wartend sein ***/
                        uk[ i ]->ExtraState &= ~EXTRA_LANDED;

                        _methoda( uk[ i ]->BactObject, YBM_CORRECTPOSITION, NULL);
                        }
                    }

                /*** Was bedeutet das für mich? ***/
                d.x += uk_dir[ i ].x;
                d.y += uk_dir[ i ].y;
                d.z += uk_dir[ i ].z;
                i++;
                }

            /*** Wirkung auf mich... ***/
            d.x /= (FLOAT) uk_count;
            d.y /= (FLOAT) uk_count;
            d.z /= (FLOAT) uk_count;

            /*** maximale Bewegung ***/
            dir.x = ytd->bact->dir.m31;
            dir.y = ytd->bact->dir.m32;
            dir.z = ytd->bact->dir.m33;
            if( ytd->bact->dof.v < 0.0 ) {
                dir.x = -dir.x;  dir.y = -dir.y;  dir.z = -dir.z; }

            e_sp  = (d.x*dir.x + d.z*dir.z);
            dist  = nc_sqrt(dir.x * dir.x + dir.z * dir.z);
            if( dist > 0.001 ) e_sp /= dist;
            dist2 = nc_sqrt(d.x * d.x + d.z * d.z);
            if( dist2 > 0.001 ) e_sp /= dist2;

            if( e_sp < 0.95 ) {

                /*** Zur Seite rutschen ***/
                FLOAT  way, merke;

                /* ------------------------------------------------------
                ** d zeigt auf das zentrum. Wir müssen es jetzt um
                ** einige (weniger als 90 Grad) drehen. In diese Richtung
                ** rutschen wir dann.
                ** ----------------------------------------------------*/
                if( (d.x * dir.z - d.z * dir.x) < 0.0 ) {

                    merke = d.x;
                    d.x   = d.z;
                    d.z   = -merke;
                    }
                else {

                    merke = d.x;
                    d.x   = -d.z;
                    d.z   = merke;
                    }

                // brauchmer bei 90° nicht yt_rot_vec_round_lokal_y( &d, angle);

                way = fabs( ytd->bact->dof.v ) * time * METER_SIZE;
                pos->x = old->x + d.x * way;
                pos->y = old->y + d.y * way;
                pos->z = old->z + d.z * way;

                if( !(ytd->bact->ExtraState & EXTRA_BACTCRASH) ) {

                    _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHVHCL);
                    ytd->bact->ExtraState |= EXTRA_BACTCRASH;
                    }

                /*** Test mit AVM erfolgt außerhalb ***/
                }
            else {

                /*** Stehenbleiben ***/
                ytd->bact->pos       = ytd->bact->old_pos;
                ytd->bact->dof.v     = 0.0;
                //ytd->bact->act_force = 0;

                if( !(ytd->bact->ExtraState & EXTRA_BACTCRASH) ) {

                    _StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHVHCL);
                    ytd->bact->ExtraState |= EXTRA_BACTCRASH;
                    }
                }

            ffb.type    = YW_FORCE_COLLISSION;
            ffb.power   = 1.0;
            ffb.dir_x   = d.x;
            ffb.dir_y   = d.z;
            _methoda( ytd->world, YWM_FORCEFEEDBACK, &ffb );
            }
        else {

            ytd->bact->ExtraState &= ~EXTRA_BACTCRASH;
            }
        }
    else {

        /* ---------------------------------------------------------------------------
        ** Wenn es eine direkte Kollision war, müssen wir stoppen. Wir setzen dann
        ** pos auf old_pos und biegen in jedem Falle, also egal, ob direkt oder nicht,
        ** den tar_unit um. Wenn wir gerade kein Ausweichmanöver machen, wirkt der.
        ** -------------------------------------------------------------------------*/

        if( direct_collision ) {
        
            FLOAT td;

            /*** wegdrehen und stoppen. kein MOVE-Flag nutzen!!!! ***/
            ytd->bact->dof.v = 0;
            ytd->bact->act_force = 0.0;
            ytd->bact->pos = ytd->bact->old_pos;
            ytd->bact->ExtraState &= ~EXTRA_MOVE;

            if( !(ytd->bact->ExtraState & EXTRA_BACTCRASH) ) {

                /*** kein Geräusch für Autonome ***/
                //_StartSoundSource( &(ytd->bact->sc), VP_NOISE_CRASHVHCL);
                ytd->bact->ExtraState |= EXTRA_BACTCRASH;
                }
                        
            /* -----------------------------------------------------------
            ** Wenn es eine NICHTKRITISCHE Kollision war, das heisst die
            ** anderen fahren in meine Richtung, so warte ich nur, ob sich
            ** das problem von selbst loest.
            ** ---------------------------------------------------------*/
            td = (ytd->bact->dir.m31*ytd->bact->tar_unit.x)+
                 (ytd->bact->dir.m32*ytd->bact->tar_unit.y)+
                 (ytd->bact->dir.m33*ytd->bact->tar_unit.z);
                 
            if( (!critical_collision) && one_moveable ) {
                
                /* ------------------------------------------------
                ** es war nicht kritisch und ich gucke schon in die
                ** Richtung, in die ich will
                ** ----------------------------------------------*/
                ytd->bact->wait_for_coll = 1000;
                }
            else {
    
                /*----------------------------------------------------------------------
                ** die bisherige NORMALE Kollisionsbearbeitung.
                ** zum Umbiegen der Richtung machen wir folgendes. Wir tun so, als ob da
                ** eine Wand ist und wir von der wegfahren
                ** -------------------------------------------------------------------*/
                if( !( ytd->collflags & (TCF_SLOPE_L | TCF_SLOPE_R ) ) ) {
        
                    if( ytd->collflags & (TCF_WALL_L | TCF_WALL_R ) ) {
        
                        /* -------------------------------------------------------------
                        ** Wir haben bereits ein Hindernis und drehen uns auch weiterhin 
                        ** so, damit wir nicht in die Wand fahren. 
                        ** -----------------------------------------------------------*/
                        ytd->collangle = 1.5;
                        ytd->collway   = 100;
                        }
                    else {
        
                        /* ------------------------------------------
                        ** Wir sind frei, dann fahren wir nach zu der
                        ** bevorzugten Richtung mit dem notwendigen
                        ** Winkel (90-derzeitig).
                        ** ----------------------------------------*/
                                               
                        if( turn_left )
                            ytd->collflags |= TCF_WALL_R;
                        else
                            ytd->collflags |= TCF_WALL_L;
                        ytd->collangle = 1.5 - mwinkel;
                        ytd->collway   = 80;
                        }
                    } 
                }
            }
        else {

            /*** Nur Speed etwas wegnehmen ***/
            ytd->bact->dof.v *= ( mdist / testentfernung );
            ytd->bact->ExtraState &= ~EXTRA_BACTCRASH;
            }
        }    
        
    if( USERINPUT && direct_collision )
        return( TRUE );
    else
        return( FALSE );
        
}   


yt_PickupEnergy( struct Bacterium *Me, struct Bacterium *batzen )
{              
    LONG   sc_time, energy;
    FLOAT  factor;

    sc_time = (LONG)(PLASMA_TIME * (FLOAT)batzen->Maximum);
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
                                

