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
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypamissileclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypavehicles.h"
#include "input/input.h"
#include "audio/audioengine.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

extern UBYTE **BactLocaleHandle;

#define SHOT_DIST      1000.0           // für MGs etc.
#define MISSILE_DIST   1200.0           // für Raketen

_dispatcher(void, yb_YBM_FIGHTBACT, struct fight_msg *fight)
{
/*
**  FUNCTION
**
**
**  INPUT
**
**
**  RESULTS
**
**
**  CHANGED
**
*/

    struct ypabact_data *ybd;
    BOOL   FIGHT = FALSE, PRIMTARGET = FALSE, SECTARGET = FALSE, UCOM;
    struct flt_triple richtung, tarpos;
    FLOAT  distance;
    struct firemissile_msg fire;
    struct setstate_msg state;
    struct settarget_msg st;
    struct checkautofirepos_msg cafp;
    struct fireyourguns_msg fyg;
    struct assesstarget_msg at;
    ULONG  at_ret;
    Object *userrobo;
    struct bact_message log;

    ybd = INST_DATA(cl, o );

    /*** Position aktualisieren ***/
    fight->pos = fight->enemy.bact->pos;

    /*** Zielvektor ***/
    richtung.x = fight->enemy.bact->pos.x - ybd->bact.pos.x;
    richtung.y = fight->enemy.bact->pos.y - ybd->bact.pos.y;
    richtung.z = fight->enemy.bact->pos.z - ybd->bact.pos.z;

    /*** Entfernung ***/
    distance = nc_sqrt( richtung.x * richtung.x + richtung.y * richtung.y +
                        richtung.z * richtung.z );

    /*** Zielrichtung ***/
    richtung.x /= distance;
    richtung.y /= distance;
    richtung.z /= distance;

    if( ybd->bact.SecondaryTarget.Bact == fight->enemy.bact ) {

        tarpos      = ybd->bact.SecondaryTarget.Bact->pos;
        SECTARGET   = TRUE;
        at.priority = 1;
        }
    else {

        tarpos      = ybd->bact.PrimaryTarget.Bact->pos;
        PRIMTARGET  = TRUE;
        at.priority = 0;
        }

    _get( ybd->world, YWA_UserRobo, &userrobo );
    if( (ybd->bact.master == ybd->bact.robo) && (ybd->bact.robo != NULL) && 
        (ybd->bact.robo   == userrobo) )
        UCOM = TRUE;
    else
        UCOM = FALSE;

    /*** Ziel überprüfen ***/
    if( ((ybd->bact.internal_time - ybd->bact.assess_time) > 500 ) ||
        (ybd->bact.internal_time < 500) ) {

        /*** erneut fragen ***/
        ybd->bact.assess_time = ybd->bact.internal_time;
        at.target_type        = TARTYPE_BACTERIUM;
        at_ret                = _methoda( o, YBM_ASSESSTARGET, &at);
        ybd->bact.at_ret      = at_ret;
        }
    else at_ret = ybd->bact.at_ret;



    /* -------------------------------------------------------------
    **             ANFLUG - ÜBERFLUG -ODER WAS?
    **
    ** Zuerst eine Positionsüberprüfung. Sind wir zu nah am
    ** ziel, so biegen wir tar_vec je nach Wunsch auf einen
    ** Punkt vor oder hinter dem Ziel und setzen ein Merkflag.
    ** Ist das Merkflag an, so setzen wir die ai1- und ai2-Zeiten
    ** auf die globaltime, um die Zielaufnahme auszubremsen.
    ** haben wir den Punkt ungefähr erreicht, so setzen wir das Flag
    ** zurück.
    ** Es scheint sinnvoll zu sein, auch zufallsabhängige Auswahl
    ** anzubieten.
    **
    ** NUR IM KAMPFMODUS
    ** -----------------------------------------------------------*/
    if( AT_FIGHT == at_ret ) {

        BOOL  weltrand;
        FLOAT target_distance;

        /*** Weltrandtest ***/
        if( (ybd->bact.pos.x <  1.1 * SECTOR_SIZE) ||
            (ybd->bact.pos.z > -1.1 * SECTOR_SIZE) ||
            (ybd->bact.pos.x > ybd->bact.WorldX - 1.1 * SECTOR_SIZE) ||
            (ybd->bact.pos.z < ybd->bact.WorldZ + 1.1 * SECTOR_SIZE) )
            weltrand = TRUE;
        else
            weltrand = FALSE;
            
        target_distance = nc_sqrt( (tarpos.x - ybd->bact.pos.x) *
                                   (tarpos.x - ybd->bact.pos.x) +
                                   (tarpos.z - ybd->bact.pos.z) *
                                   (tarpos.z - ybd->bact.pos.z) );

        if( ybd->bact.ExtraState & EXTRA_APPROACH ) {
        
            ybd->bact.ExtraState &= ~EXTRA_ATTACK;

            /*** Wir sind bereits in der Anflug(Wegflug-)phase ***/
            if( weltrand ||
                (ybd->bact.adist_bact < target_distance ) ) {

                /*** Nun wieder weit genug weg ***/
                ybd->bact.ExtraState &= ~EXTRA_APPROACH;
                }
            else {

                /*** Immer noch im Wegflug ***/
                ybd->bact.time_ai1 = ybd->bact.time_ai2 = ybd->bact.internal_time;
                }
            }
        else {

            /*** Fliegen drauf zu. Ist das Ziel in der Nähe? ***/
            if( ybd->bact.sdist_bact  > nc_sqrt( (tarpos.x - ybd->bact.pos.x) *
                                                 (tarpos.x - ybd->bact.pos.x) +
                                                 (tarpos.z - ybd->bact.pos.z) *
                                                 (tarpos.z - ybd->bact.pos.z) ) ) {
                                                
                /*** Wir sind in der Nähe ***/
                ybd->bact.ExtraState &= ~EXTRA_ATTACK;
                
                if( (ybd->bact.FWay == FlyOver) ||
                    ((fight->global_time & 1) && (ybd->bact.FWay == FlyDifferent)) ) {

                    /*** Zielpunkt ist vor mir ***/
                    ybd->bact.tar_vec.x = ybd->bact.dof.x;
                    //ybd->bact.tar_vec.y = ybd->bact.dof.y;
                    ybd->bact.tar_vec.z = ybd->bact.dof.z;
                    }
                else {

                    /*** Zielpunkt ist hinter mir ***/
                    ybd->bact.tar_vec.x = -ybd->bact.dof.x;
                    //ybd->bact.tar_vec.y = -ybd->bact.dof.y;
                    ybd->bact.tar_vec.z = -ybd->bact.dof.z;
                    }

                ybd->bact.ExtraState |= EXTRA_APPROACH;
                ybd->bact.time_ai1 = ybd->bact.time_ai2 = ybd->bact.internal_time;
                }
            else {
            
                /*** Attackephase, wenn unter adist. D.h. wir fahren drauf zu ***/
                if( ybd->bact.adist_sector > target_distance ) 
                    ybd->bact.ExtraState |= EXTRA_ATTACK;
                else
                    ybd->bact.ExtraState &= ~EXTRA_ATTACK;
                }    
            }
        }
    else {
    
        /*** auf keinen fall an oder wegflug ***/
        ybd->bact.ExtraState &= ~EXTRA_APPROACH;
        ybd->bact.ExtraState &= ~EXTRA_ATTACK;
        }


    /* ---------------------------------------------------------------------
    ** Mit dem Ziel kann ich verschiedene Sachen machen, je nach Aggression,
    ** Sicht und Eigentümer
    ** Doch zuerst mal die Flags löschen, ich setze sie vielleicht wieder
    ** -------------------------------------------------------------------*/
    // ybd->bact.ExtraState &= ~(EXTRA_FIGHTS|EXTRA_FIGHTP);

   if( at_ret != AT_REMOVE ) {

        /*** Sichtbar ***/
        if( at_ret == AT_IGNORE ) {

            /*** Nicht bekämpfen. Wir sind in der Nähe und landen ***/
            ybd->bact.ExtraState &= ~EXTRA_APPROACH;

            if( ybd->bact.ExtraState & EXTRA_FIRE ) {

                state.main_state = state.extra_on = 0;
                state.extra_off  = EXTRA_FIRE;
                _methoda( o, YBM_SETSTATE, &state );
                }

            if( ybd->bact.SecondaryTarget.Bact == fight->enemy.bact ) {

                /* --------------------------------------------------------
                ** Die Meldung machen wir bei der Teilzielaufnahme, also in
                ** AI2
                ** ------------------------------------------------------*/
                ybd->bact.ExtraState &= ~EXTRA_FIGHTS;

                st.target_type = TARTYPE_NONE;
                st.priority    = 1;
                _methoda( o, YBM_SETTARGET, &st );

                /*** Mir schaltn hiern Ziel aus, ne ?! ***/
                SECTARGET = FALSE;
                }
            else {

                ybd->bact.ExtraState &= ~EXTRA_FIGHTP;
                if( UCOM && (ybd->bact.MainState != ACTION_WAIT) &&
                    (!(ybd->bact.ExtraState & EXTRA_ESCAPE)) ) {

                    log.ID       = LOGMSG_DONE;
                    log.para1    = log.para2 = log.para3 = 0;
                    log.sender   = &(ybd->bact);
                    log.pri      = 32;
                    _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                    }

                /*** Mir sei do! Wenn es das Hauptziel war! ***/
                state.extra_off = 0;
                state.extra_on  = 0;
                state.main_state= ACTION_NORMAL;
                _methoda( o, YBM_SETSTATE, &state );
                ybd->bact.MainState = ACTION_WAIT;      // noch kein Prototyp
                }
            }


        if( at_ret == AT_GOTO ) {

            /*** Wir würden dorthin gehen, aber das machen wir sowieso ***/

            if( ybd->bact.ExtraState & EXTRA_FIRE ) {

                state.main_state = state.extra_on = 0;
                state.extra_off  = EXTRA_FIRE;
                _methoda( o, YBM_SETSTATE, &state );
                }
            }


        if( at_ret == AT_FIGHT ) {

            /*** BÖSE!!! Bekämpfen das Ding! ***/

            cafp.target_pos  = fight->enemy.bact->pos;
            cafp.target_type = TARTYPE_BACTERIUM;
            cafp.radius      = fight->enemy.bact->radius;

            if( _methoda( o,YBM_CHECKAUTOFIREPOS, &cafp) ) {

                /*** Flags ***/
                if( SECTARGET )
                    ybd->bact.ExtraState |= EXTRA_FIGHTS;
                else
                    ybd->bact.ExtraState |= EXTRA_FIGHTP;

                /* -----------------------------------------------------------
                ** ich mache mal ein Experiment: Wenn CheckAutofirepos was
                ** sinnvolles zurückgebracht hat, dann zeigt das Objekt
                ** ja schon in die Richtung. Somit mache ich mal (auch bei
                ** Sektorbekämpfung) folgendes: wenn die Rakete ungelenkt ist,
                ** dann setze ich fire.dir auf tar_unit.
                ** ---------------------------------------------------------*/

                /*** neuer Vesuch einer Durchmischung...***/
                fire.dir.x         = ybd->bact.dir.m31;
                if( BCLID_YPATANK != ybd->bact.BactClassID )
                    fire.dir.y     = ybd->bact.dir.m32 - ybd->bact.gun_angle; // Hack!
                else
                    fire.dir.y     = richtung.y;

                fire.dir.z         = ybd->bact.dir.m33;

                //fire.dir.x         = richtung.x;
                //fire.dir.y         = richtung.y;
                //fire.dir.z         = richtung.z;

                fire.target_type   = TARTYPE_BACTERIUM;
                fire.target.bact   = fight->enemy.bact;
                fire.target_pos    = fight->pos;
                fire.wtype         = ybd->bact.auto_ID;
                fire.global_time   = ybd->bact.internal_time;
                /*** Hubschrauber. etwas runter + Seite ***/
                if( fight->global_time % 2 == 0 )
                    fire.start.x   = -ybd->bact.firepos.x;
                else
                    fire.start.x   = ybd->bact.firepos.x;
                fire.start.y       = ybd->bact.firepos.y;
                fire.start.z       = ybd->bact.firepos.z;
                fire.flags         = 0;
                _methoda( o, YBM_FIREMISSILE, &fire );
                }
            else {
            
                /*** Ziel kann nicht getroffen werden? Attack aus! ***/
                ybd->bact.ExtraState &= ~EXTRA_ATTACK;
                }

            if( (distance < SHOT_DIST) &&
                (ybd->bact.mg_shot != NO_MACHINEGUN) ) {

                /* ----------------------------------------------
                ** Test Extra, weil CAFP waffensensitiv reagiert!
                ** --------------------------------------------*/
                if( (richtung.x * ybd->bact.dir.m31 +
                     richtung.y * ybd->bact.dir.m32 +
                     richtung.z * ybd->bact.dir.m33) > 0.85 )
                    FIGHT = TRUE;
                }


            /*** Wenn FIGHT == TRUE, dann schießen wir ***/
            if( FIGHT ) {

                /*** Flags ***/
                if( SECTARGET )
                    ybd->bact.ExtraState |= EXTRA_FIGHTS;
                else
                    ybd->bact.ExtraState |= EXTRA_FIGHTP;

                if( !(ybd->bact.ExtraState & EXTRA_FIRE) ) {

                    state.main_state = state.extra_off = 0;
                    state.extra_on   = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }

                /*** das Opfer bekämpfen ***/
                fyg.time        = fight->time;
                fyg.global_time = ybd->bact.internal_time;
                fyg.dir.x       = ybd->bact.dir.m31;
                fyg.dir.y       = ybd->bact.dir.m32;
                fyg.dir.z       = ybd->bact.dir.m33;
                _methoda( o, YBM_FIREYOURGUNS, &fyg );
                }
            else {

                if( ybd->bact.ExtraState & EXTRA_FIRE ) {

                    state.main_state = state.extra_on = 0;
                    state.extra_off  = EXTRA_FIRE;
                    _methoda( o, YBM_SETSTATE, &state );
                    }
                }
            }
        }
    else {

        /* ----------------------------------------------------
        ** Unsichtbar --> eigentlich Abmelden. Weil das aber
        ** bedeuten kann, sich einfach auf feindliche Sektoren
        ** zu setzen, Sektorziel der aktuellen position setzen.
        ** Bei Nebenzielen natürlich abmelden...
        ** --------------------------------------------------*/
        if( PRIMTARGET ) {

            ybd->bact.ExtraState &= ~EXTRA_FIGHTP;

            st.priority    = 0;
            st.target_type = TARTYPE_SECTOR;
            st.pos         = ybd->bact.PrimPos; //habe ich mir ja gemerkt!
            _methoda( o, YBM_SETTARGET, &st );
            }

        if( SECTARGET ) {

            ybd->bact.ExtraState &= ~EXTRA_FIGHTS;
            st.target_type = TARTYPE_NONE;
            st.priority = 1;
            _methoda( o, YBM_SETTARGET, &st );
            }
            
        /*** keine Meldung! ***/
            
        /*** Feuern abmelden ***/
        ybd->bact.ExtraState &= ~EXTRA_APPROACH;
        if( ybd->bact.ExtraState & EXTRA_FIRE ) {

            state.main_state = state.extra_on = 0;
            state.extra_off  = EXTRA_FIRE;
            _methoda( o, YBM_SETSTATE, &state );
            }      
        }
}

_dispatcher(void, yb_YBM_FIGHTSECTOR, struct fight_msg *fight)
{
/*
**  FUNCTION
**
**      schießt mit Raketen, denn die Bordkanone bewirkt nix!
**
**
**  INPUT
**
**
**  RESULTS
**
**
**  CHANGED
**
**          4.9.95      created     8100000C
**
*/

    struct ypabact_data *ybd;
    struct setstate_msg state;
    struct firemissile_msg fire;
    struct settarget_msg target;
    BOOL   PRIMTARGET = FALSE, SECTARGET = FALSE, UCOM, ACOM;
    struct checkautofirepos_msg cafp;
    ULONG  at_ret;
    struct assesstarget_msg at;
    struct bact_message log;
    Object *userrobo;
    struct flt_triple tarpos;
    FLOAT  distance;

    ybd = INST_DATA(cl, o );


    if( ybd->bact.SecondaryTarget.Sector == fight->enemy.sector ) {

        tarpos      = ybd->bact.SecPos; // Zielposition im Sektor
        SECTARGET   = TRUE;
        at.priority = 1;
        }
    else {

        tarpos      = ybd->bact.PrimPos; // Zielposition im Sektor
        PRIMTARGET  = TRUE;
        at.priority = 0;
        }

    _get( ybd->world, YWA_UserRobo, &userrobo );
    if( (ybd->bact.master == ybd->bact.robo) && (ybd->bact.robo != NULL) && 
        (ybd->bact.robo   == userrobo) )
        UCOM = TRUE;
    else
        UCOM = FALSE;

    distance = nc_sqrt( (ybd->bact.pos.x-tarpos.x) * (ybd->bact.pos.x-tarpos.x)+
                        (ybd->bact.pos.z-tarpos.z) * (ybd->bact.pos.z-tarpos.z) );


    /*** Zielinfos holen bzw. überprüfen ***/
    if( ((ybd->bact.internal_time - ybd->bact.assess_time) > 500 ) ||
        (ybd->bact.internal_time < 500) ) {

        /*** erneut fragen ***/
        ybd->bact.assess_time = ybd->bact.internal_time;
        at.target_type        = TARTYPE_SECTOR;
        at_ret                = _methoda( o, YBM_ASSESSTARGET, &at);
        ybd->bact.at_ret      = at_ret;
        }
    else at_ret = ybd->bact.at_ret;



    /* -------------------------------------------------------------
    **             ANFLUG - ÜBERFLUG -ODER WAS?
    **
    ** Zuerst eine Positionsüberprüfung. Sind wir zu nah am
    ** ziel, so biegen wir tar_vec je nach Wunsch auf einen
    ** Punkt vor oder hinter dem Ziel und setzen ein Merkflag.
    ** Ist das Merkflag an, so setzen wir die ai1- und ai2-Zeiten
    ** auf die globaltime, um die Zielaufnahme auszubremsen.
    ** haben wir den Punkt ungefähr erreicht, so setzen wir das Flag
    ** zurück.
    ** Es scheint sinnvoll zu sein, auch zufallsabhängige Auswahl
    ** anzubieten.
    ** Das gibt es jedoch nur im FightModus!
    ** -----------------------------------------------------------*/
    if( AT_FIGHT == at_ret ) {

        BOOL  weltrand;
        FLOAT target_distance;

        /*** Weltrandtest ***/
        if( (ybd->bact.pos.x <  1.1 * SECTOR_SIZE) ||
            (ybd->bact.pos.z > -1.1 * SECTOR_SIZE) ||
            (ybd->bact.pos.x > ybd->bact.WorldX - 1.1 * SECTOR_SIZE) ||
            (ybd->bact.pos.z < ybd->bact.WorldZ + 1.1 * SECTOR_SIZE) )
            weltrand = TRUE;
        else
            weltrand = FALSE;

        target_distance = nc_sqrt(
                             (tarpos.x - ybd->bact.pos.x) *
                             (tarpos.x - ybd->bact.pos.x) +
                             (tarpos.z - ybd->bact.pos.z) *
                             (tarpos.z - ybd->bact.pos.z) );
                             
        if( ybd->bact.ExtraState & EXTRA_APPROACH ) {
        
            ybd->bact.ExtraState &= ~EXTRA_ATTACK;

            /*** Wir sind bereits in der Anflug(Wegflug-)phase ***/
            if( weltrand ||
                (ybd->bact.adist_sector < target_distance) ) {

                /*** Nun wieder weit genug weg ***/
                ybd->bact.ExtraState &= ~EXTRA_APPROACH;
                }
            else {

                /*** Immer noch im Wegflug ***/
                ybd->bact.time_ai1 = ybd->bact.time_ai2 = ybd->bact.internal_time;
                }
            }
        else {

            /*** Fliegen drauf zu. Ist das Ziel in der Nähe? ***/
            if( ybd->bact.sdist_sector > target_distance ) {

                /*** Wir sind in der Nähe ***/
                ybd->bact.ExtraState &= ~EXTRA_ATTACK;
                
                if( (ybd->bact.FWay == FlyOver) ||
                    ((fight->global_time & 1) && (ybd->bact.FWay == FlyDifferent)) ) {

                    /*** Zielpunkt ist vor mir ***/
                    ybd->bact.tar_vec.x = ybd->bact.dof.x;
                    ybd->bact.tar_vec.y = ybd->bact.dof.y;
                    ybd->bact.tar_vec.z = ybd->bact.dof.z;
                    }
                else {

                    /*** Zielpunkt ist hinter mir ***/
                    ybd->bact.tar_vec.x = -ybd->bact.dof.x;
                    ybd->bact.tar_vec.y = -ybd->bact.dof.y;
                    ybd->bact.tar_vec.z = -ybd->bact.dof.z;
                    }

                ybd->bact.ExtraState |= EXTRA_APPROACH;
                ybd->bact.time_ai1 = ybd->bact.time_ai2 = ybd->bact.internal_time;
                }
            else {
            
                /*** Attackephase, wenn unter adist. D.h. wir fahren drauf zu ***/
                if( ybd->bact.adist_sector > target_distance ) 
                    ybd->bact.ExtraState |= EXTRA_ATTACK;
                else
                    ybd->bact.ExtraState &= ~EXTRA_ATTACK;
                }
            }
        }
    else {
    
        /*** auf keinen fall an oder wegflug ***/
        ybd->bact.ExtraState &= ~EXTRA_APPROACH;
        ybd->bact.ExtraState &= ~EXTRA_ATTACK;
        }


    /*** Sektoren werden nie mit MGs bekämpft ***/
    if( ybd->bact.ExtraState & EXTRA_FIRE ) {

        state.extra_off = EXTRA_FIRE;
        state.extra_on  = 0;
        state.main_state= 0;
        _methoda( o, YBM_SETSTATE, &state );
        }

    if( at_ret != AT_REMOVE ) {

        if( at_ret == AT_IGNORE ) {
        
            /*** Nicht bekämpfen, weil wir schon da sind und es freundlich ist ***/
            ybd->bact.ExtraState &= ~EXTRA_APPROACH;

            // Relikt: if( (ybd->bact.Sector == fight->enemy.sector)

                /*** Geschafft. (Neben-) Ziel freigeben. ***/

            if( SECTARGET ) {

                /*** Sagen, daß wir damit fertig sind ***/
                ybd->bact.ExtraState &= ~EXTRA_FIGHTS;
                if( UCOM ) {

                    /*** Auch nur Meldung wenn nicht noch Hauptziel ***/
                    if( ybd->bact.PrimaryTarget.Sector !=
                        ybd->bact.SecondaryTarget.Sector ) {

                        log.para1  = (LONG)( ybd->bact.SecPos.x / SECTOR_SIZE );
                        log.para2  = (LONG)(-ybd->bact.SecPos.z / SECTOR_SIZE );
                        log.ID     = LOGMSG_CONQUERED;
                        log.para3  = 0;
                        log.sender = &(ybd->bact);
                        log.pri    = 22;
                        _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                        }
                    }

                /*** und abmelden ***/
                target.priority    = 1;
                target.target_type = TARTYPE_NONE;
                _methoda( o, YBM_SETTARGET, &target );

                /*** !!! ***/
                SECTARGET = FALSE;
                }
            else {

                /* --------------------------------------------------------
                ** Es war unser Hauptziel. Melden, wenn wir Commander sind.
                ** Die Einmaligkeit realisieren wir durch einen WAIT-Test
                ** ------------------------------------------------------*/
                ybd->bact.ExtraState &= ~EXTRA_FIGHTP;
                if( UCOM && (ybd->bact.MainState != ACTION_WAIT) ) {

                    log.para1  = log.para2 = log.para3 = 0;
                    log.ID     = LOGMSG_DONE;
                    log.sender = &(ybd->bact);
                    log.pri    = 32;
                    _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                    }
            
                ybd->bact.MainState = ACTION_WAIT;
                }
            }


        if( at_ret == AT_GOTO ) {

            /*** dafür brauchen wir hier nix zu machen ***/
            }

        
        if( at_ret == AT_FIGHT ) {

            /*** Draufhalten ***/



            /*** Korrigieren (auch hier Meldung mit!) ***/
            if( PRIMTARGET ) {

                /* ----------------------------------------------------------------
                ** Wenn vorher noch kein Kampf war, melden wir die Bekämpfung. Nun
                ** kann es passieren, daß das auch das Hauptziel ist und wir wollen
                ** ja nicht doppelt melden. Deshalb 'n Test 
                ** --------------------------------------------------------------*/

                /*** in Kampfnähe? ***/
                if( distance < MISSILE_DIST ) {

                    /*** Meldung ***/
                    if( !(ybd->bact.ExtraState & EXTRA_FIGHTP) ) {

                        if( UCOM ) {

                            /*** Als Spieler melde ich Angriffe ***/
                            if( ybd->bact.PrimaryTarget.Sector != ybd->bact.SecondaryTarget.Sector) {

                                log.para1  = (LONG)( ybd->bact.PrimPos.x / SECTOR_SIZE );
                                log.para2  = (LONG)(-ybd->bact.PrimPos.z / SECTOR_SIZE );
                                log.ID     = LOGMSG_FIGHTSECTOR;
                                log.sender = &(ybd->bact);
                                log.para3  = 0;
                                log.pri    = 20;
                                _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                                }
                            }
                        }

                    /*** Flag setzen ***/
                    ybd->bact.ExtraState |= EXTRA_FIGHTP;
                    }

                _methoda( o, YBM_GETBESTSUB, &(ybd->bact.PrimPos));
                fight->pos = ybd->bact.PrimPos;
                }

            if( SECTARGET ) {

                /*** Wenn vorher noch kein Kampf war ***/

                if( distance < MISSILE_DIST ) {

                    /*** Meldung ***/
                    if( UCOM && (!(ybd->bact.ExtraState & EXTRA_FIGHTS)) ) {

                        log.para1  = (LONG)( ybd->bact.SecPos.x / SECTOR_SIZE );
                        log.para2  = (LONG)(-ybd->bact.SecPos.z / SECTOR_SIZE );
                        log.ID     = LOGMSG_FIGHTSECTOR;
                        log.sender = &(ybd->bact);
                        log.para3  = 0;
                        log.pri    = 20;
                        _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                        }

                    /*** Flag setzen ***/
                    ybd->bact.ExtraState |= EXTRA_FIGHTS;
                    }

                _methoda( o, YBM_GETBESTSUB, &(ybd->bact.SecPos));
                fight->pos = ybd->bact.SecPos;
                }

            cafp.target_type = TARTYPE_SECTOR;
            cafp.target_pos  = fight->pos;
            if( _methoda( o, YBM_CHECKAUTOFIREPOS, &cafp ) ) {

                FLOAT tdist = nc_sqrt(
                      (ybd->bact.pos.x + ybd->bact.firepos.x - fight->pos.x)*
                      (ybd->bact.pos.x + ybd->bact.firepos.x - fight->pos.x)+
                      (ybd->bact.pos.y + ybd->bact.firepos.y - fight->pos.y)*
                      (ybd->bact.pos.y + ybd->bact.firepos.y - fight->pos.y)+
                      (ybd->bact.pos.z + ybd->bact.firepos.z - fight->pos.z)*
                      (ybd->bact.pos.z + ybd->bact.firepos.z - fight->pos.z));
                if( tdist < 0.01 ) tdist = 0.01;

                if( SECTARGET )
                    ybd->bact.ExtraState |= EXTRA_FIGHTS;
                else
                    ybd->bact.ExtraState |= EXTRA_FIGHTP;

                /*** Günstig. Drück' auf's Knöpfchen, Max! ***/
                // fire.dir.x         = ybd->bact.dir.m31;
                // fire.dir.y         = ybd->bact.dir.m32 - ybd->bact.gun_angle;
                // fire.dir.z         = ybd->bact.dir.m33;
                fire.dir.x         = -(ybd->bact.pos.x + ybd->bact.firepos.x -
                                       fight->pos.x ) / tdist;
                fire.dir.y         = -(ybd->bact.pos.y + ybd->bact.firepos.y -
                                       fight->pos.y ) / tdist;
                fire.dir.z         = -(ybd->bact.pos.z + ybd->bact.firepos.z -
                                       fight->pos.z ) / tdist;
                fire.target_type   = TARTYPE_SECTOR;
                fire.target.sector = fight->enemy.sector;
                fire.target_pos    = fight->pos;
                fire.wtype         = ybd->bact.auto_ID;
                fire.global_time   = ybd->bact.internal_time;

                /*** Hubschrauber. etwas runter + Seite ***/
                if( fight->global_time % 2 == 0 )
                    fire.start.x   = -ybd->bact.firepos.x;
                else
                    fire.start.x   =  ybd->bact.firepos.x;
                fire.start.y       =  ybd->bact.firepos.y;
                fire.start.z       =  ybd->bact.firepos.z;
                    fire.flags     = 0;
                _methoda( o, YBM_FIREMISSILE, &fire );
                }
            else {
            
                /*** Ziel kann nicht getroffen werden? Attack aus! ***/
                ybd->bact.ExtraState &= ~EXTRA_ATTACK;
                }
            }
        }
    else {

        /*** freigeben ***/
        ybd->bact.ExtraState &= ~EXTRA_APPROACH;
        if( PRIMTARGET ) {

            /*** Ab-Meldung ***/
            if( UCOM ) {

                log.ID     = LOGMSG_REMSECTOR,
                log.sender = &(ybd->bact);
                log.para1  = (LONG)( ybd->bact.PrimPos.x / SECTOR_SIZE );
                log.para2  = (LONG)(-ybd->bact.PrimPos.z / SECTOR_SIZE );
                log.para3  = 0;
                log.pri    = 18;
                _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                }

            target.target_type = TARTYPE_SECTOR;
            target.pos.x       = ybd->bact.pos.x;
            target.pos.z       = ybd->bact.pos.z;
            target.priority    = 0;
            ybd->bact.ExtraState &= ~EXTRA_FIGHTP;
            _methoda( o, YBM_SETTARGET, &target );
            }

        if( SECTARGET ) {

            /*** Ab-Meldung ***/
            if( UCOM ) {

                log.ID     = LOGMSG_REMSECTOR,
                log.sender = &(ybd->bact);
                log.para1  = (LONG)( ybd->bact.SecPos.x / SECTOR_SIZE );
                log.para2  = (LONG)(-ybd->bact.SecPos.z / SECTOR_SIZE );
                log.para3  = 0;
                log.pri    = 18;
                _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                }

            target.target_type = TARTYPE_NONE;
            ybd->bact.ExtraState &= ~EXTRA_FIGHTS;
            target.priority = 1;
            _methoda( o, YBM_SETTARGET, &target );
            }
        }
}




_dispatcher(BOOL, yb_YBM_FIREMISSILE, struct firemissile_msg *fire)
{
    /* 
    ** Fordert eine Rakete an, übergibt ihr ein Ziel (woher?) und klinkt
    ** sie in eine Liste ein, so daß sie getriggert werden kann. 
    **
    ** wenn wir schiessen konnten, geben wir TRUE zurück
    */
    
    Object *rakete;
    struct settarget_msg st;
    struct MinNode *node;
    struct Bacterium *rbact;
    struct ypabact_data *ybd;
    struct createweapon_msg cw;
    ULONG  num_weapons, i, wtime, Handle;
    struct WeaponProto *wproto;


    ybd = INST_DATA( cl, o );

    _get( ybd->world, YWA_WeaponArray, &wproto );

    /*** Darf das Objekt schießen ? ***/
    if( fire->wtype == NO_AUTOWEAPON ) return( FALSE );

    /*** Wie alt ist die vorherige Rakete? ***/
    if( ybd->bact.last_weapon != 0 ) {

        /* ----------------------------------------------------------------
        ** Wir haben schon einmal 'ne rakete abgefeuert. Wann war denn das?
        ** Wenn wir User sind, warten wir anders als als Autonomer.
        ** --------------------------------------------------------------*/

        if( ybd->flags & YBF_UserInput )
            wtime = wproto[ fire->wtype ].ShotTime_User;
        else
            wtime = wproto[ fire->wtype ].ShotTime;

        if( wproto[ fire->wtype ].SalveShots ) {

            /* --------------------------------------------------------
            ** Dann testen, ob wir die Maximalzahl einer Salve erreicht
            ** haben, dann warten wir laenger.
            ** ------------------------------------------------------*/
            if( ybd->bact.salve_count >= wproto[ fire->wtype ].SalveShots )
                wtime = wproto[ fire->wtype ].SalveDelay;
            }

        if( (fire->global_time - ybd->bact.last_weapon) < wtime ) return( FALSE );
        }

    /*** Schusszahl erhoehen ***/
    if( ybd->bact.salve_count >= wproto[ fire->wtype ].SalveShots )
        ybd->bact.salve_count  = 1;
    else
        ybd->bact.salve_count++;

    /*** Force-Feedback-Effekt ***/
    if (ybd->flags & YBF_UserInput) {
        /*** Waffentyp? ***/
        if (wproto[fire->wtype].Flags & WPF_Driven) {
            /*** eine Rakete ***/
            struct yw_forcefeedback_msg yffm;
            yffm.type = YW_FORCE_MISSILE_LAUNCH;
            _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
        } else if (wproto[fire->wtype].Flags & WPF_Impulse) {
            /*** eine Granate ***/
            struct yw_forcefeedback_msg yffm;
            yffm.type = YW_FORCE_GRENADE_LAUNCH;
            _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
        } else {
            /*** eine Bombe ***/
            struct yw_forcefeedback_msg yffm;
            yffm.type = YW_FORCE_BOMB_LAUNCH;
            _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
        };
    };

    /*** num_weapon Raketen abfeuern ***/
    num_weapons = max( ybd->bact.num_weapons, 1);
    for( i = 0; i < num_weapons; i++ ) {

        FLOAT start_x;
        struct ypaworld_data *ywd;
        struct ypamessage_newweapon nwm;
        struct sendmessage_msg sm;

        /* -----------------------------------------------------
        ** Wie ist denn die x-Startpostion. Wir zählen von
        ** -x nach +x. Wenn es nur eine ist, dann lassen wir die
        ** übergebene, schon "gezufallte" Position.
        ** ---------------------------------------------------*/
        if( 1 == num_weapons )
            start_x = fire->start.x;
        else 
            start_x = -fabs( fire->start.x ) +
                       i * 2 * fabs( fire->start.x ) / (num_weapons - 1);

        /* -----------------------------------------
        ** Anfordern vonne WO. Die Startposition ist
        ** MP + start.x * local_x + ....
        ** ---------------------------------------*/
        cw.wp    = fire->wtype;
        cw.x     = ybd->bact.pos.x + ybd->bact.dir.m11 * start_x +
                                     ybd->bact.dir.m21 * fire->start.y +
                                     ybd->bact.dir.m31 * fire->start.z;
        cw.y     = ybd->bact.pos.y + ybd->bact.dir.m12 * start_x +
                                     ybd->bact.dir.m22 * fire->start.y +
                                     ybd->bact.dir.m32 * fire->start.z;
        cw.z     = ybd->bact.pos.z + ybd->bact.dir.m13 * start_x +
                                     ybd->bact.dir.m23 * fire->start.y +
                                     ybd->bact.dir.m33 * fire->start.z;
        rakete = (Object *) _methoda( ybd->world, YWM_CREATEWEAPON, &cw);

        if( !rakete ) return( FALSE ) ;
        _get( rakete, YBA_Bacterium, &rbact);

        /*** wegen Netzwerk VOR SetViewer !!!! später mit in WC ***/
        _set( rakete, YMA_RifleMan, &(ybd->bact));
        _set( rakete, YMA_StartHeight, ((LONG)(cw.y)) );  // fuer Bomben

        rbact->Owner = ybd->bact.Owner;

        /*** Energieabzug (1/300 der Raketenzerstörungskraft) ***/
        if( BCLID_YPAGUN != ybd->bact.BactClassID )
            ybd->bact.Energy -= rbact->Energy / ENERGY_PRO_DEST;

        /*** Wenn die Wunschrichtung 0-0-0 ist, heißt das lokal z ***/
        if( (fire->dir.x == 0.0) && (fire->dir.y == 0.0) && (fire->dir.z == 0.0) ) {

            rbact->dof.x = ybd->bact.dir.m31;
            rbact->dof.y = ybd->bact.dir.m32;
            rbact->dof.z = ybd->bact.dir.m33;
            }
        else {

            rbact->dof.x = fire->dir.x;
            rbact->dof.y = fire->dir.y;
            rbact->dof.z = fire->dir.z;
            }

        /*** Bomben ausbremsen ***/
        rbact->dof.v = ybd->bact.dof.v + wproto[ fire->wtype ].StartSpeed;
        if( !(wproto[ fire->wtype ].Flags & (WPF_Driven|WPF_Impulse)) )
            rbact->dof.v *= 0.2;

        /*** Ausrichten in Wunschrichtung (dof!) Dabei Seitenneigung beachten ***/
        //_methoda( rakete, YMM_ALIGNMISSILE_S, NULL );
        rbact->dir.m31 = rbact->dof.x;
        rbact->dir.m32 = rbact->dof.y;
        rbact->dir.m33 = rbact->dof.z;
        rbact->dir.m11 = ybd->bact.dir.m11;
        rbact->dir.m12 = ybd->bact.dir.m12;
        rbact->dir.m13 = ybd->bact.dir.m13;
        rbact->dir.m21 = rbact->dir.m32 * rbact->dir.m13 - rbact->dir.m33 * rbact->dir.m12;
        rbact->dir.m22 = rbact->dir.m33 * rbact->dir.m11 - rbact->dir.m31 * rbact->dir.m13;
        rbact->dir.m23 = rbact->dir.m31 * rbact->dir.m12 - rbact->dir.m11 * rbact->dir.m32;
        
        /*** Viewer etwas zurücksetzen ***/
        if( (0 == i) && (fire->flags & FIRE_VIEW) ) {

            rbact->pos.x -= rbact->dir.m31 * 30;
            rbact->pos.y -= rbact->dir.m32 * 30;
            rbact->pos.z -= rbact->dir.m33 * 30;
            }

        /* ----------------------------------------------------------------------
        ** für die Raketen brauche ich einige Sonderbehandlungen. Ich nehme sie
        ** einfach  aus dem Totencache raus, klinke sie in meine Liste.
        ** Der MasterPointer bleibt 0, damit das WO nicht versucht, bei der Frei-
        ** gabe ihn aus einer Liste zu klinken 
        ** --------------------------------------------------------------------*/

        if( rbact->master ) {
            
            _Remove( (struct Node *) &(rbact->slave_node));
            rbact->master = NULL;
            }

        _get( rakete, YMA_AutoNode, &node );
        _AddTail( (struct List *) &(ybd->bact.auto_list),
                  (struct Node *) node );

        _get( rakete, YMA_Handle ,&Handle );

        if( Handle == YMF_Search ) {

            st.target.bact = fire->target.bact;
            st.target_type = fire->target_type;
            st.priority    = 0;
            st.pos         = fire->target_pos;

            _methoda( rakete, YBM_SETTARGET, &st );

            /*** y-Komponente (hackmäßig) nachträglich korrigieren? ***/
            if( (fire->flags & FIRE_CORRECT) && 
                (fire->target_type == TARTYPE_SECTOR))
                rbact->PrimPos.y = fire->target_pos.y;  // Sektorhöhe aufheben
            }

        /*** gleich noch fuers Netz... ***/
        nwm.target_pos = rbact->PrimPos;

        if( Handle == YMF_Simple ) {

            /*** dof ist bereits initialisiert worden ***/
            rbact->PrimTargetType = TARTYPE_SIMPLE;
            rbact->tar_unit.x     = rbact->dof.x;
            rbact->tar_unit.y     = rbact->dof.y;
            rbact->tar_unit.z     = rbact->dof.z;
            }

        /*** Robo mitgeben, das brauchen wir für Effekte ***/
        rbact->robo = ybd->bact.robo;

        /*** und zum Schluß noch die Zeit eintragen ***/
        ybd->bact.last_weapon = fire->global_time;

        /*** Geräusch ***/
        _StartSoundSource( &(rbact->sc), WP_NOISE_LAUNCH );

        /*** Übers Netz public machen ***/
        ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
        if( ywd->playing_network ) {

            rbact->ident          |= (((ULONG)ybd->bact.Owner) << 24);
            nwm.generic.message_id = YPAM_NEWWEAPON;
            nwm.generic.owner      = ybd->bact.Owner;
            nwm.ident              = rbact->ident;
            nwm.rifleman           = ybd->bact.ident;
            nwm.type               = fire->wtype;
            nwm.pos.x              = cw.x;
            nwm.pos.y              = cw.y;
            nwm.pos.z              = cw.z;
            nwm.flags              = 0;

            /*** komplette geschwindigkeit, weil kein schatten mehr ***/
            nwm.dir.x              = rbact->dof.x * rbact->dof.v;
            nwm.dir.y              = rbact->dof.y * rbact->dof.v;
            nwm.dir.z              = rbact->dof.z * rbact->dof.v;

            /*** Ziel mit uebbergeben (s.o.) ***/
            nwm.target_type        = rbact->PrimTargetType;
            if( TARTYPE_BACTERIUM == rbact->PrimTargetType ) {
                nwm.target         = rbact->PrimaryTarget.Bact->ident;
                nwm.target_owner   = rbact->PrimaryTarget.Bact->Owner;
                }
            sm.data                = &nwm;
            sm.data_size           = sizeof( nwm );
            sm.receiver_kind       = MSG_ALL;
            sm.receiver_id         = NULL;
            sm.guaranteed          = TRUE;
            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
            }

        /*** Viewer setzen? Erst nach Erschaffensmeldung! ***/
        if( (fire->flags & FIRE_VIEW) && (0 == i) ) {

            if( ybd->flags & YBF_Viewer ) {

                /*** Dann waren wir auch Viewer und dürfen die Sicht "abgeben" ***/
                _set( o,      YBA_Viewer, FALSE);
                _set( rakete, YBA_Viewer, TRUE );
                //_set( o,      YBA_UserInput, FALSE);
                //_set( rakete, YBA_UserInput, TRUE );
                }
            }

        /* -----------------------------------------------------------
        ** Lifetime nachkorrigieren. Wenn kein TARTYPE_BACTERIUM ange-
        ** geben wurde, wird eine andere Lifetime gesetzt, egal ob es
        ** eine Lenkwaffe ist oder nicht.
        ** ---------------------------------------------------------*/
        if( TARTYPE_BACTERIUM != fire->target_type ) {

            /* ---------------------------------------------------
            ** Wenn das Keyword 0 ist, wird nichts geaendert, also
            ** die normale filetime genommen
            ** -------------------------------------------------*/
            if( wproto[ fire->wtype ].LifeTimeNT )
                _set( rakete, YMA_LifeTime, wproto[ fire->wtype ].LifeTimeNT );
            }
        }

    /* ---------------------------------------------------------------
    ** Auf Wunsch den Schuetzen toeten (Kamikazeeffect, nur Waffe lebt
    ** weiter). Wenn der Schuetze User war, dann setzen wir ihn immer
    ** in die Bombe.
    ** -------------------------------------------------------------*/
    if( ybd->bact.kill_after_shot ) {

        struct modvehicleenergy_msg mve;

        if( ybd->flags & YBF_UserInput ) {

            _set( o,      YBA_Viewer, FALSE);
            _set( rakete, YBA_Viewer, TRUE );
            }

        mve.killer = ybd->bact.master_bact;
        mve.energy = (-2) * ybd->bact.Maximum;
        _methoda( o, YBM_MODVEHICLEENERGY, &mve );
        }

    return( TRUE );
}


_dispatcher(BOOL, yb_YBM_FIREYOURGUNS, struct fireyourguns_msg *fyg )
{
/*
**  FUNCTION    feuert mit der Kanone entlang einer vorgegebenen Linie.
**              macht dabei einen CheckRohrTest ähnlich wie die Rakete.
**              Die, die wir treffen, denen ziehen wir Energie ab und 
**              töten sie evtl.
**
**  INPUT       Richtung (Länge ist globales SHOT_DIST)
**
**  RESULT      ShotTime des Einschusses, wenn diese um war
**
**  CHANGED     30-Jan-96 created 8100 000C
**               5-Aug-96 Rückkehrwert jetzt TRUE, wenn ein Schuß abgegeben
**                        werden konnte (nur für User)
**                        Weiterhin wird nur ein Einschuß gesetzt (und damit
**                        TRUE zurückgegeben), wenn das Maximum aus
**                        ShotTime und FrameTime um ist!!!!
**
**  GUNS WIRKEN NICHT AUF SEKTOREN - ZU SCHWACH
*/

    struct ypabact_data *ybd;
    struct ypaworld_data *ywd;
    struct Cell *cell_array[3];
    struct flt_triple firestart, fireend, spos, ziel, vp, elpos, pt;
    FLOAT  entfernung_ok, produkt, entf, kandrad;
    struct setstate_msg state;
    struct getsectorinfo_msg gsi;
    int    i, testcount;
    struct Bacterium *kandidat, *next;
    LONG   UI;
    struct alignmissile_msg am;
    struct intersect_msg inter;
    struct createweapon_msg cw;
    BOOL   GROUND, just_sub, ret_value = FALSE, robo_gun;
    Object *shot;
    struct Node *node;
    struct ExtCollision *ecoll;
    struct WeaponProto *wp;
    struct Bacterium *shotbact;
    BOOL   SHOT = FALSE;
    BOOL   really_network = FALSE;


    ybd = INST_DATA( cl, o);

    #ifdef __NETWORK__
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    if( ywd->playing_network ) really_network = TRUE;
    #endif

    /*** Darf überhaupt geschossen werden? ***/
    if( ybd->bact.mg_shot == NO_MACHINEGUN ) return( FALSE );


    next = NULL;
    entf = 0.0;

    /*** Wie verläuft der Strahl? ***/
    firestart = ybd->bact.pos;
    fireend.x = ybd->bact.pos.x + fyg->dir.x * SHOT_DIST;
    fireend.y = ybd->bact.pos.y + fyg->dir.y * SHOT_DIST;
    fireend.z = ybd->bact.pos.z + fyg->dir.z * SHOT_DIST;

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


    /*** Ob ich RoboBordkanone bin, teste ich hier schon ***/
    robo_gun = FALSE;
    if( BCLID_YPAGUN == ybd->bact.BactClassID ) {

        /*** Bin ich Bordkanone? ***/
        LONG g;
        _get( o, YGA_RoboGun, &g );
        if( g ) robo_gun = TRUE;
        }

    /*** Egal, ob wir was treffen oder nicht, es gibt Energieabzug ***/
    if( BCLID_YPAGUN != ybd->bact.BactClassID )
        ybd->bact.Energy -= (ybd->bact.gun_power * fyg->time) / ENERGY_PRO_DEST;

    /*** Ok, 3 potentielle Sektoren sind da. testen wir deren Bakterien ***/
    for( i=0; i<3; i++) {

        if( i > 0 ) {

            /*** testen, ob wir den Sektor schon mal hatten ***/
            if( cell_array[ i-1 ] == cell_array[ i ] )
                continue;
            }

        kandidat = (struct Bacterium *) cell_array[ i ]->BactList.mlh_Head;
        while( ((struct Node *)kandidat)->ln_Succ != 0 ) {

            /* ---------------------------------------------------------
            ** sind wir das selbst bzw. ist es unser Auftraggeber ? oder
            ** eine andere Rakete?  Tot-test nicht mit TESTDESTROY,
            ** sondern mit ACTION_DEAD, weil das bei Brocken Probleme
            ** bringen kann und zerstörbare Leichen gibt es nicht mehr!
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
            ** Alle 100% RoboGuns ignorieren. Die liegen sonst wie ein
            ** Schild davor
            ** -----------------------------------------------------*/
            if( (BCLID_YPAGUN == kandidat->BactClassID) &&
                (100          >= kandidat->Shield) ) {
                
                ULONG rgun;
                _get( kandidat->BactObject, YGA_RoboGun, &rgun );
                if( rgun ) {
                
                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }
                }

            /*** Automatischer Schütze? ***/
            if( !(ybd->flags & YBF_UserInput) )
                if( kandidat->Owner == ybd->bact.Owner ) {

                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }

            /*** Wenn wir RoboFlak sind, können wir den eigenen Robo nicht treffen ***/
            if( robo_gun ) {

                if( ybd->bact.robo == kandidat->BactObject ) {

                    kandidat = (struct Bacterium *)
                               ((struct Node *)kandidat)->ln_Succ;
                    continue;
                    }
                }

            /* ------------------------------------------------------------
            ** wir testen nun, ob kandidat innerhalb der Röhre liegt.
            ** Dazu bilden wir das Vektorprodukt von der normierten
            ** Flugrichtung und der Entfernung zum kandidaten von
            ** old_pos aus. Der Betrag muß kleiner als die Summe von
            ** explode_radius und kandidat->radius sein. Weiterhin muß
            ** die Entfernung kleiner als die Pythagorassumme von
            ** dem Betrag des VP und der Entfernung old->pos sein, denn mit
            ** dem VP allein ist es noch nicht getan.
            ** ----------------------------------------------------------*/

            _get( kandidat->BactObject, YBA_ExtCollision, &ecoll );

            if( ecoll )
                testcount = ecoll->number;
            else
                testcount = 1;

            /*** Nun alle Testpunkte ***/
            just_sub = FALSE;
            while( testcount-- ) {

                /*** Wo kommt der Punkt her? ***/
                if( ecoll ) {

                    /*** Der Punkt ***/
                    pt     = kandidat->pos;
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
                    kandrad = ecoll->points[ testcount ].radius;
                    }
                else {

                    pt      = kandidat->pos;
                    kandrad = kandidat->radius;
                    }

                ziel.x = pt.x - ybd->bact.old_pos.x;
                ziel.y = pt.y - ybd->bact.old_pos.y;
                ziel.z = pt.z - ybd->bact.old_pos.z;

                /*** Hinter mir? ***/
                if( (ziel.x * ybd->bact.dir.m31 + ziel.y * ybd->bact.dir.m32 +
                     ziel.z * ybd->bact.dir.m33) < 0.3 ) {

                    /*** gleich weiter ***/
                    continue;
                    }

                entfernung_ok = nc_sqrt(ziel.x*ziel.x + ziel.y*ziel.y + ziel.z*ziel.z);

                vp.x = fyg->dir.y * ziel.z - ziel.y * fyg->dir.z;
                vp.y = fyg->dir.z * ziel.x - ziel.z * fyg->dir.x;
                vp.z = fyg->dir.x * ziel.y - ziel.x * fyg->dir.y;

                if( (produkt = nc_sqrt(vp.x * vp.x + vp.y * vp.y + vp.z * vp.z) ) <
                    (kandrad + ybd->bact.gun_radius ) ) {

                    /*
                    ** Das Objekt liegt nah genug an der Strecke. Nun testen wir,
                    ** ob es auch innerhalb der zurückgelegten Strecke liegt.
                    */

                    if( nc_sqrt( SHOT_DIST*SHOT_DIST + produkt*produkt )>entfernung_ok) {

                        /*** Tatsächlich! Der kriegt was ab ***/

                        /*** unschöne Sache wegen _get. Aber ich muß den User ***/
                        /*** schonen. nur einmal abziehen!                    ***/
                        if( !just_sub ) {

                            LONG en;
                            struct modvehicleenergy_msg en_msg;

                            _get( kandidat->BactObject, YBA_UserInput, &UI );
                            if( UI || (kandidat->ExtraState & EXTRA_ISVIEWER))
                                en = (LONG)
                                     (ybd->bact.gun_power * fyg->time *
                                     (100.0 - (FLOAT)(kandidat->Shield)) )/250.0;
                            else
                                en = (LONG)
                                     (ybd->bact.gun_power * fyg->time *
                                     (100.0 - (FLOAT)(kandidat->Shield)) )/100;

                            en_msg.energy = -en;
                            en_msg.killer = &(ybd->bact);
                            
                            if( en != 0 )
                                _methoda( kandidat->BactObject, YBM_MODVEHICLEENERGY, &en_msg );
                            }

                        just_sub = TRUE;

                        /*** Ist es der Nächstliegende? ***/
                        if( (next == NULL ) || (entf > entfernung_ok) ) {

                            next = kandidat;
                            entf = entfernung_ok;
                            spos = kandidat->pos;
                            }
                        }
                    }
                }

            /*** war nix. Nächster ***/
            kandidat = (struct Bacterium *) ((struct Node *)kandidat)->ln_Succ;
            }
        }

    /* ------------------------------------------------------------------------
    ** Wenn ich User bin, dann darf ich Einschusslöcher setzen. Dazu teste ich
    ** zuerst, ob ich jemanden getroffen habe. Wenn ja, kriegt der es ab ( also
    ** der nächstliegende). Wenn nicht, dann mache ich eine intersection mit 
    ** einer festen Länge. Habe ich da was gefunden, gibt es das Einschußloch.
    **
    ** Dieses ist eine tote Rakete, die einfach positioniert wird. Das Runter-
    ** zählen und wegnehmen erfolgt automatisch. Allerdings muß ich einen
    ** eigenen Zähler nehmen, weil ich sonst mit den richtigen Kanonen durch-
    ** einander komme.
    ** ----------------------------------------------------------------------*/

    /*** Im Netzwerkmodus machen wir keine Einschüsse ***/
    _get( o, YBA_UserInput, &UI );

    if( (UI || _methoda( ybd->world, YWM_ISVISIBLE, &(ybd->bact)) ) &&
        (!really_network) ) {

        LONG set_shot_time;

        /*** Ist schon genug Zeit seit dem letzten Einschuß vergangen ***/
        _get( ybd->world, YWA_WeaponArray, &wp );
        if( UI )
            set_shot_time = max( wp[ ybd->bact.mg_shot ].ShotTime_User,
                                 fyg->time * 1000 );
        else
            set_shot_time = max( wp[ ybd->bact.mg_shot ].ShotTime,
                                 fyg->time * 1000 );

        if( (fyg->global_time - ybd->bact.mg_time) > set_shot_time ) {

            /*** merken ***/
            ybd->bact.mg_time = fyg->global_time;

            /*** Schuß abgegeben, wenn auch nicht unbedingt Treffer ***/
            ret_value = TRUE;

            /*** was getroffen ? ***/
            if( next ) {

                elpos.x  = spos.x - 0.7 * kandrad * (spos.x-ybd->bact.pos.x)/entf;
                elpos.y  = spos.y - 0.7 * kandrad * (spos.y-ybd->bact.pos.y)/entf;
                elpos.z  = spos.z - 0.7 * kandrad * (spos.z-ybd->bact.pos.z)/entf;
                GROUND = FALSE;
                SHOT   = TRUE;
                }
            else {

                /*** Intersection machen ***/
                inter.pnt.x = ybd->bact.pos.x;
                inter.pnt.y = ybd->bact.pos.y;
                inter.pnt.z = ybd->bact.pos.z;
                inter.vec.x = fyg->dir.x * SHOT_DIST;
                inter.vec.y = fyg->dir.y * SHOT_DIST;
                inter.vec.z = fyg->dir.z * SHOT_DIST;
                inter.flags = 0;
                _methoda( ybd->world, YWM_INTERSECT2, &inter );
                if( !inter.insect ) {

                    ret_value = TRUE; // weil Schuß abgegeben
                    SHOT      = FALSE;
                    }
                else {

                    elpos.x = inter.ipnt.x;
                    elpos.y = inter.ipnt.y;
                    elpos.z = inter.ipnt.z;
                    GROUND  = TRUE;
                    SHOT    = TRUE;
                    }
                }

            /*** Zeuch anfordern ***/
            if( SHOT ) {

                cw.x  = elpos.x;
                cw.y  = elpos.y;
                cw.z  = elpos.z;
                cw.wp = ybd->bact.mg_shot;
                shot  = (Object *) _methoda( ybd->world, YWM_CREATEWEAPON, &cw );

                if( shot ) {

                    _get( shot, YBA_Bacterium, &shotbact );
                    shotbact->Owner = ybd->bact.Owner;

                    /*** einklinken und auf tot setzen ***/
                    if( shotbact->master ) {
                        
                        _Remove( (struct Node *) &(shotbact->slave_node));
                        shotbact->master = NULL;
                        }

                    _get( shot, YMA_AutoNode, &node );
                    _AddTail( (struct List *) &(ybd->bact.auto_list),
                              (struct Node *) node );

                    /*** Bodeneinschuß oder Objekt ( zuerst auf jeden Fall "töten") ***/
                    state.main_state = ACTION_DEAD;
                    state.extra_off  = state.extra_on = 0;
                    _methoda( shot, YBM_SETSTATE_I, &state );
                    
                    if( GROUND ) {

                        state.extra_on   = EXTRA_MEGADETH;
                        state.extra_off  = state.main_state = 0;
                        _methoda( shot, YBM_SETSTATE_I, &state );
                        
                        /*** am Boden ausrichten ***/
                        am.vec.x = inter.sklt->PlanePool[ inter.pnum ].A;
                        am.vec.y = inter.sklt->PlanePool[ inter.pnum ].B;
                        am.vec.z = inter.sklt->PlanePool[ inter.pnum ].C;
                        _methoda( shot, YMM_ALIGNMISSILE_V, &am );
                        }
                    }
                }
            }
        }

    /*** und Tschüß ***/
    return( ret_value );

}


_dispatcher( void, yb_YBM_GETBESTSUB, struct flt_triple *pos)
{
    /* --------------------------------------------------------------------
    ** Ermittelt den energiereichsten  Subsektor (-mittelpunkt) oder
    ** den Mittelpunkt des Kompaktsektors. So schießen wir auf lohnenswerte
    ** ziele. Ob diese schon unter 0 sind, interessiert uns nicht. 
    ** Dabei brauchen wir gar nicht so sehr zu beachten, ob es sich um
    ** Kompakt oder 9-fachSektoren handelt, da die Energie nichtbenötigter 
    ** Subsektoren 0 ist.
    ** ------------------------------------------------------------------*/

    /*** MODIFIZIERT BY FLOH !!! ***/

    struct getsectorinfo_msg gsi;
    struct ypabact_data *ybd;
    FLOAT MP_x,MP_z;

    ybd = INST_DATA(cl, o);
    
    gsi.abspos_x = pos->x;
    gsi.abspos_z = pos->z;
    _methoda( ybd->world, YWM_GETSECTORINFO, &gsi);

    MP_x = (FLOAT)  (gsi.sec_x * SECTOR_SIZE + SECTOR_SIZE/2);
    MP_z = (FLOAT) -(gsi.sec_y * SECTOR_SIZE + SECTOR_SIZE/2);

    pos->x = MP_x;
    pos->z = MP_z;

    /*** wenn's doch kein Kompakt-Sektor war... ***/
    if (gsi.sector->SType != SECTYPE_COMPACT) {

        LONG merke = 0;
        LONG ix,iy;
        
        for (iy=0; iy<3; iy++) {
            for (ix=0; ix<3; ix++) {

                if (gsi.sector->SubEnergy[ix][iy] > merke) {

                    pos->x = MP_x + ((ix-1) * SLURP_WIDTH);
                    pos->z = MP_z + ((iy-1) * SLURP_WIDTH);
                    merke  = gsi.sector->SubEnergy[ix][iy];
                };
            };
        };
    };
}




_dispatcher( BOOL, yb_YBM_CHECKAUTOFIREPOS, struct checkautofirepos_msg *msg)
{
/*
**  FUNCTION    Testet, ob wir in Abschußposition sind. Dabei müssen wir
**              unterscheiden, ob wir ein Bacterium oder einen Sektor
**              bekämpfen und welche Art von Waffe wir haben.
**
**  INPUT       Position und Art des Zieles
**
**  RESULT      TRUE, wenn günstig
**
**  CHANGED     kurz vor Jahresende     created     8100 000C
*/

    struct ypabact_data *ybd;
    struct flt_triple richtung;
    FLOAT  distance;
    struct WeaponProto *warray, *waffe;
    UBYTE  flags;

    ybd = INST_DATA( cl, o);

    if( msg->target_type == TARTYPE_BACTERIUM ) {

        richtung.x = msg->target_pos.x - ybd->bact.pos.x;
        richtung.y = msg->target_pos.y - ybd->bact.pos.y;
        richtung.z = msg->target_pos.z - ybd->bact.pos.z;
        }
    else {

        richtung.x = msg->target_pos.x - ybd->bact.pos.x;
        richtung.y = ybd->bact.pref_height;
        richtung.z = msg->target_pos.z - ybd->bact.pos.z;
        }

    distance = nc_sqrt( richtung.x * richtung.x + richtung.y * richtung.y +
                        richtung.z * richtung.z );
    if( distance == 0 ) return( FALSE );

    richtung.x /= distance;
    richtung.y /= distance;
    richtung.z /= distance;    

    /*** Waffeninformationen holen ***/
    _get( ybd->world, YWA_WeaponArray, &warray );
    if( ybd->bact.auto_ID == NO_AUTOWEAPON ) {

        waffe = NULL;
        }
    else {

        waffe = &( warray[ ybd->bact.auto_ID ] );

        /*** autonome Waffe ??? ***/
        if( !(waffe->Flags & WPF_Autonom) )
            waffe = NULL;
        else
            flags = waffe->Flags & ~WPF_Autonom;
        }

    /*** Wenn waffe == NULL, dann kann nur noch das MG helfen ***/
    if( waffe == NULL ) {

        if( ybd->bact.mg_shot == NO_MACHINEGUN )
            return( FALSE );      // weil es keinen Sinn hat...
        else
            flags = WPF_Driven; // weil es sich so in etwa verhält.
        }

    /*** Nun testen ***/
    if( msg->target_type == TARTYPE_BACTERIUM ) {

        FLOAT  b_vp, o_radius, d;
        struct flt_triple vp;

        /*** Wenn keine Waffe trotzdem für MG testen ***/
        if( waffe )
            o_radius = max( 40, 0.8 * msg->radius + waffe->Radius);
        else
            o_radius = max( 40, 0.8 * msg->radius);

        switch( flags ) {

            case 0:

                /* ---------------------------------------------
                ** Bombe. Wuerde die treffen? Dann muss die x-z-
                ** entfernung dem radius entsprechen.
                ** keine Ziele ueber mir aufnehmen.
                ** -------------------------------------------*/
                d = nc_sqrt( (msg->target_pos.x - ybd->bact.pos.x) *
                             (msg->target_pos.x - ybd->bact.pos.x) +
                             (msg->target_pos.z - ybd->bact.pos.z) *
                             (msg->target_pos.z - ybd->bact.pos.z) );

                if( (o_radius > d) && 
                    (msg->target_pos.y > ybd->bact.pos.y) )
                    return( TRUE );
                else
                    return( FALSE );
                break;      // aus Gewohnheit...

            case WPF_Impulse:

                /*** Granate, nur x-z-Richtung ***/
                if( (distance < MISSILE_DIST) && 
                    ( (richtung.x*ybd->bact.dir.m31 +
                       richtung.z*ybd->bact.dir.m33) > 0.93 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;

            default:

                /* -----------------------------------------------
                ** neuer versuch einer Röhre..., nur hier wird der
                ** radius des zu treffenden ausgewertet!
                ** ---------------------------------------------*/
                vp.x = richtung.y * ybd->bact.dir.m33 -
                       richtung.z * ybd->bact.dir.m32;
                vp.y = richtung.z * ybd->bact.dir.m31 -
                       richtung.x * ybd->bact.dir.m33;
                vp.z = richtung.x * ybd->bact.dir.m32 -
                       richtung.y * ybd->bact.dir.m31;
                b_vp = nc_sqrt( vp.x * vp.x + vp.z * vp.z + vp.y * vp.y );

                if( (distance < MISSILE_DIST) &&

                    ( (richtung.x * ybd->bact.dir.m31 +
                       richtung.y * ybd->bact.dir.m32 +
                       richtung.z * ybd->bact.dir.m33) > 0.0 ) &&

                    ( b_vp < (o_radius / distance) ) )

                    return( TRUE );
                else
                    return( FALSE );
                break;
            }
        }
    else {

        FLOAT d;

        /*** Mit MGs auf Sektoren? Sinnlos! ***/
        if( waffe == NULL ) return( FALSE );

        switch( flags ) {

            case 0:

                /* ---------------------------------------------
                ** Bombe. Wuerde die treffen? Dann muss die x-z-
                ** entfernung dem radius entsprechen
                ** -------------------------------------------*/
                d = nc_sqrt( (msg->target_pos.x - ybd->bact.pos.x) *
                             (msg->target_pos.x - ybd->bact.pos.x) +
                             (msg->target_pos.z - ybd->bact.pos.z) *
                             (msg->target_pos.z - ybd->bact.pos.z) );

                if( waffe->Radius > d )
                    return( TRUE );
                else
                    return( FALSE );
                break;

            case WPF_Impulse:

                /*** Granate, nur x-z-Richtung ***/
                if( (distance < MISSILE_DIST) && 
                    ( (richtung.x*ybd->bact.dir.m31 +
                       richtung.z*ybd->bact.dir.m33) > 0.91 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;

            default:

                /*** keine Röhre... ***/
                if( (distance < MISSILE_DIST) && 
                    ( (richtung.x*ybd->bact.dir.m31 +
                       richtung.y*ybd->bact.dir.m32 +
                       richtung.z*ybd->bact.dir.m33) > 0.91 ) )
                    return( TRUE );
                else
                    return( FALSE );
                break;
            }
        }
}

