/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Untersuchungsmethoden des Robo
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
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


void yr_CheckWorld( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
LONG yr_CheckRadar( struct yparobo_data *yrd, struct Cell *sector );
LONG yr_CheckPower( struct yparobo_data *yrd, struct Cell *sector );
LONG yr_CheckReconnoitre( struct yparobo_data *yrd, struct Cell *sector );
LONG yr_CheckTerritorium( struct yparobo_data *yrd, struct Cell *sector );
LONG yr_CheckEnemy( struct yparobo_data *yrd, LONG *CID, WORD *sx, WORD *sy );
LONG yr_CheckRobo( struct yparobo_data *yrd, LONG *CID, WORD *sx, WORD *sy );
LONG yr_CheckSafety( struct yparobo_data *yrd, struct Cell *sector );
FLOAT yr_AnalyzeCommand( struct OBNode *Commander, struct yparobo_data *yrd,
                         WORD my_x, WORD my_y );
void yr_GetBestSectorToTarget( struct yparobo_data *yrd, UWORD pos, 
                               struct settarget_msg *target );
LONG yr_CheckPlace( struct yparobo_data *yrd );
LONG yr_GoodSector( struct yparobo_data *yrd, WORD sec_x, WORD sec_y );
int  yr_NumberOfFlaks( struct yparobo_data *yrd );
int  yr_NumberOfRadars( struct yparobo_data *yrd );
int  yr_OwnSectors( struct yparobo_data *yrd );
int  yr_UnvisibleSectors( struct yparobo_data *yrd );
BOOL yr_RoboNearRobo( struct yparobo_data *yrd );
BOOL yr_EnemyNearRobo( struct yparobo_data *yrd );




/*** Globale Arrays ***/
WORD   p[16] = { -1,-1,  0,-1,  1,-1,  -1,0,  1,0,  -1,1,  0,1,  1,1};
WORD   delta[16] = { -1,-1,  0,-1,  1,-1,  -1,0,  1,0,  -1,1,  0,1,  1,1};

/*-----------------------------------------------------------------*/



void yr_CheckWorld( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
{
    /*
    ** Durchsucht die Welt nach Sektoren, die es lohnt zu erobern oder zu
    ** bebauen.
    */


    LONG   value, seccount, CommandID;
    struct getcommander_msg gc;
    struct Cell *sector;
    WORD   sec_x, sec_y, secs_per_flak;
    FLOAT  ratio;
    struct getrldratio_msg gr;
    BOOL   terr_allowed, radar_allowed, recon_allowed, power_allowed, safety_allowed;
    BOOL   defense_allowed, place_allowed, robo_allowed;

    /*** --------------------------***/
    /*** --- Eroberungswürdig? --- ***/
    /*** --------------------------***/

    /*** je kleiner die Welt, desto weniger Testsektoren ***/
    seccount = (yrd->bact->WSecX + yrd->bact->WSecY) / 20 + 1;

    if( yrd->NewAI ) {
        yrd->chk_Terr_Delay -= msg->frame_time;
        if( yrd->chk_Terr_Delay <= 0 )
            terr_allowed = TRUE;
        else
            terr_allowed = FALSE;
        }
    else terr_allowed = TRUE;

    if( (yrd->ep_Conquer > 0) && (terr_allowed) ) {

        while( seccount-- ) {

            /*** --- Ende? --- ***/
            if( yrd->chk_Terr_Count == (yrd->bact->WSecX * yrd->bact->WSecY) ) {

                yrd->chk_Terr_Count = 0;

                /*** Test-Fertig-Flags setzen ***/
                yrd->RoboState |= ROBO_CONQUERREADY;
                return;
                }


            sector = &( yrd->FirstSector[ yrd->chk_Terr_Count ] );

            /*** --- RandSector? --- ***/
            sec_x = yrd->chk_Terr_Count % yrd->bact->WSecX;
            sec_y = yrd->chk_Terr_Count / yrd->bact->WSecX;
            if( (sec_x == 0) || (sec_x == (yrd->bact->WSecX-1)) ||
                (sec_y == 0) || (sec_y == (yrd->bact->WSecY-1)) ) {

                yrd->chk_Terr_Count++;
                continue;
                }


            /*** --- Testdurchlauf --- ***/
            value = yr_CheckTerritorium( yrd, sector );
            if( value > yrd->chk_Terr_Value ) {

                /*** Gibt es für dieses Ziel schon einenCommander? ***/
                gc.flags = GC_PRIMTARGET;
                gc.targetpos.x =  (sec_x + 0.5) * SECTOR_SIZE;
                gc.targetpos.z = -(sec_y + 0.5) * SECTOR_SIZE;
                gc.target_type = TARTYPE_SECTOR;
                _methoda( yrd->bact->BactObject, YRM_GETCOMMANDER, &gc );

                if( !gc.com ) {

                    /*** Da arbeitet noch keiner dran ***/
                    yrd->chk_Terr_Value  = value;
                    yrd->chk_Terr_Pos    = yrd->chk_Terr_Count;
                    yrd->chk_Terr_Sector = sector;
                    yrd->chk_Terr_Time   = msg->global_time;
                    }
                }
            yrd->chk_Terr_Count++;
            }
        }
    else {

        yrd->RoboState &= ~ROBO_CONQUERREADY;
        yrd->chk_Terr_Count = 0;
        yrd->chk_Terr_Value = 0;
        }


    /*** --------------------------------***/
    /*** --- Verteidigung notwendig? --- ***/
    /*** --------------------------------***/

    if( yrd->NewAI ) {
        yrd->chk_Enemy_Delay -= msg->frame_time;
        if( yrd->chk_Enemy_Delay <= 0 )
            defense_allowed = TRUE;
        else
            defense_allowed = FALSE;
        }
    else defense_allowed = TRUE;

    /*** Feinde an der Station? dann sofort testen ***/
    if( yr_EnemyNearRobo( yrd ) )
        defense_allowed = TRUE;

    if( (yrd->ep_Defense > 0) && (defense_allowed) ) {

        /*** Nach Commandern suchen ***/
        value = yr_CheckEnemy( yrd, &CommandID, &sec_x, &sec_y );
        if( value > yrd->chk_Enemy_Value ) {

            /*** arbeitet da schon jemand dran? ***/
            gc.flags         = GC_PRIMTARGET;
            gc.PrimCommandID = CommandID;
            gc.target_type   = TARTYPE_BACTERIUM;
            _methoda( yrd->bact->BactObject, YRM_GETCOMMANDER, &gc);
            if( gc.com == NULL ) {

                /*** merken, arbeitet noch keiner dran ***/
                yrd->chk_Enemy_Value     = value; // nur gefunden/nicht gefunden
                yrd->chk_Enemy_CommandID = CommandID;
                yrd->chk_Enemy_Time      = msg->global_time;

                /* ------------------------------------------------------
                ** Für Entfernungstest müssen wir die derzeitige Position
                ** des Feind-Geschwaders als Pos merken
                ** ----------------------------------------------------*/
                yrd->chk_Enemy_Pos = sec_x + sec_y * yrd->bact->WSecX;
                }
            }
        yrd->RoboState |= ROBO_DEFENSEREADY;
        }
    else {

        yrd->RoboState &= ~ROBO_DEFENSEREADY;
        yrd->chk_Enemy_Value = 0;
        }

    /*** -----------------------------***/
    /*** --- Test auf Roboangriff --- ***/
    /*** -----------------------------***/

    if( yrd->NewAI ) {
        yrd->chk_Robo_Delay -= msg->frame_time;
        if( yrd->chk_Robo_Delay <= 0 ) 
            robo_allowed = TRUE;
        else
            robo_allowed = FALSE;
        }
    else robo_allowed = TRUE;

    /*** Wenn Robo neben mir... ***/
    if( yr_RoboNearRobo( yrd ) )
        robo_allowed = TRUE;

    if( (yrd->ep_Robo > 0) && (robo_allowed) ) {

        /*** Nach Robos suchen ***/
        value = yr_CheckRobo( yrd, &CommandID, &sec_x, &sec_y );
        if( value > yrd->chk_Robo_Value ) {

            /*** arbeitet da schon jemand dran? ***/
            gc.flags         = GC_PRIMTARGET;
            gc.PrimCommandID = CommandID;
            gc.target_type   = TARTYPE_BACTERIUM;
            _methoda( yrd->bact->BactObject, YRM_GETCOMMANDER, &gc);
            if( gc.com == NULL ) {

                /*** merken, arbeitet noch keiner dran ***/
                yrd->chk_Robo_Value     = value; // nur gefunden/nicht gefunden
                yrd->chk_Robo_CommandID = CommandID;
                yrd->chk_Robo_Time      = msg->global_time;

                /* ------------------------------------------------------
                ** Für Entfernungstest müssen wir die derzeitige Position
                ** des Feind-Geschwaders als Pos merken
                ** ----------------------------------------------------*/
                yrd->chk_Robo_Pos = sec_x + sec_y * yrd->bact->WSecX;
                }
            }
        yrd->RoboState |= ROBO_ROBOREADY;
        }
    else {

        yrd->RoboState &= ~ROBO_ROBOREADY;
        yrd->chk_Robo_Value = 0;
        }


    /*** ------------------------------- ***/
    /*** --- Platzwechsel notwendig? --- ***/
    /*** ------------------------------- ***/

    /*** neuen Platz suchen ***/
    seccount = (yrd->bact->WSecX + yrd->bact->WSecY) / 10 + 1; // häufiger

    if( yrd->NewAI ) {
        yrd->chk_Place_Delay -= msg->frame_time;
        if( yrd->chk_Place_Delay <= 0 ) 
            place_allowed = TRUE;
        else
            place_allowed = FALSE;
        }
    else place_allowed = TRUE;
    
    /*** Dürfen wir sowas überhaupt suchen? ***/
    if( (yrd->ep_ChangePlace > 0) && (place_allowed) ) {

        while( seccount-- ) {

            /*** --- Ende? --- ***/
            if( yrd->chk_Place_Count == (yrd->bact->WSecX * yrd->bact->WSecY) ) {

                yrd->chk_Place_Count = 0;

                /*** Test-Fertig-Flags setzen ***/
                yrd->RoboState |= ROBO_FOUNDPLACE;
                return;
                }
            
            /*** --- RandSector? --- ***/
            sec_x = yrd->chk_Place_Count % yrd->bact->WSecX;
            sec_y = yrd->chk_Place_Count / yrd->bact->WSecX;
            if( (sec_x == 0) || (sec_x == (yrd->bact->WSecX-1)) ||
                (sec_y == 0) || (sec_y == (yrd->bact->WSecY-1)) ) {

                yrd->chk_Place_Count++;
                continue;
                }


            value = yr_CheckPlace( yrd );
            if( value > yrd->chk_Place_Value ) {

                /*** es gibt was besseres ***/
                yrd->chk_Place_Value = value;
                yrd->chk_Place_Pos   = yrd->chk_Place_Count;
                yrd->chk_Place_Time  = msg->global_time;
                }
            yrd->chk_Place_Count++;
            }
        }
    else {

        /*** Vorsichtshalber auf jeden Fall abmelden ***/
        yrd->RoboState &= ~ROBO_FOUNDPLACE;
        yrd->chk_Place_Value = 0;
        yrd->chk_Place_Count = 0;
        }


    /*** ------------------------------- ***/
    /*** --- Kraftwerkstest erlaubt? --- ***/
    /*** ------------------------------- ***/

    /*** je kleiner die Welt, desto weniger Testsektoren ***/
    seccount = (yrd->bact->WSecX + yrd->bact->WSecY) / 20 + 1;
    
    /*** Test notwendig? ***/
    gr.owner = yrd->bact->Owner;
    _methoda( yrd->world, YWM_GETRLDRATIO, &gr );

    if( yrd->NewAI ) {
        yrd->chk_Power_Delay -= msg->frame_time;
        if( yrd->chk_Power_Delay <= 0 )
            power_allowed = TRUE;
        else
            power_allowed = FALSE;
        }
    else power_allowed = TRUE;

    /* ------------------------------------------------
    ** wenn erlaubt, aber kein Kraftwerk (gr.ratio==0),
    ** dann in jedem Falle bauen.
    ** ----------------------------------------------*/
    if( (gr.ratio == 0.0) && (yrd->ep_Power > 0) )
        power_allowed = TRUE;

    /* ------------------------------------------------------------
    ** die rough-Ratio kann groesser als 1 sein, damit kann ich das
    ** bauen ausbremsen. Bei 50% muss er also 2*YR_PowerRatio
    ** erreicht haben!
    ** ----------------------------------------------------------*/
    if( yrd->ep_Power > 0 )
        ratio = YR_PowerRatio * (100.0 / (FLOAT)yrd->ep_Power);
    else
        ratio = YR_PowerRatio;

    if( (yrd->ep_Power > 0) &&
        ((gr.rough_ratio > ratio) || (gr.ratio < 0.0001)) &&
        (power_allowed) ) {

        while( seccount-- ) {

            /*** --- Ende? --- ***/
            if( yrd->chk_Power_Count == (yrd->bact->WSecX * yrd->bact->WSecY) ) {

                yrd->chk_Power_Count = 0;

                /*** Test-Fertig-Flags setzen ***/
                yrd->RoboState |= ROBO_POWERREADY;
                return;
                }
            
            /*** --- RandSector? --- ***/
            sec_x = yrd->chk_Power_Count % yrd->bact->WSecX;
            sec_y = yrd->chk_Power_Count / yrd->bact->WSecX;
            if( (sec_x == 0) || (sec_x == (yrd->bact->WSecX-1)) ||
                (sec_y == 0) || (sec_y == (yrd->bact->WSecY-1)) ) {

                yrd->chk_Power_Count++;
                continue;
                }


            /*** --- Testdurchlauf --- ***/
            sector = &( yrd->FirstSector[ yrd->chk_Power_Count ] );

            value = yr_CheckPower( yrd, sector );
            if( value > yrd->chk_Power_Value ) {
                yrd->chk_Power_Value  = value;
                yrd->chk_Power_Pos    = yrd->chk_Power_Count;
                yrd->chk_Power_Sector = sector;
                yrd->chk_Power_Time   = msg->global_time;
                }

            yrd->chk_Power_Count++;
            }
        }
    else {

        yrd->RoboState &= ~ROBO_POWERREADY;
        yrd->chk_Power_Value = 0;
        yrd->chk_Power_Count = 0;
        }
            

    /*** -------------------------- ***/
    /*** --- Radartest erlaubt? --- ***/
    /*** -------------------------- ***/

    /*** je kleiner die Welt, desto weniger Testsektoren ***/
    seccount = (yrd->bact->WSecX + yrd->bact->WSecY) / 20 + 1;

    if( yrd->NewAI ) {
        yrd->chk_Radar_Delay -= msg->frame_time;
        if( yrd->chk_Radar_Delay <= 0 )
            radar_allowed = TRUE;
        else
            radar_allowed = FALSE;
        }
    else radar_allowed = TRUE;
    
    if( (yrd->ep_Radar > 0) &&
        (yr_UnvisibleSectors( yrd ) / YR_SecsPerRadar > yr_NumberOfRadars(yrd)) &&
        (radar_allowed) ) {

        while( seccount-- ) {

            /*** --- Ende? --- ***/
            if( yrd->chk_Radar_Count == (yrd->bact->WSecX * yrd->bact->WSecY) ) {

                yrd->chk_Radar_Count = 0;

                /*** Test-Fertig-Flags setzen ***/
                yrd->RoboState |= ROBO_RADARREADY;
                return;
                }
            
            /*** --- RandSector? --- ***/
            sec_x = yrd->chk_Radar_Count % yrd->bact->WSecX;
            sec_y = yrd->chk_Radar_Count / yrd->bact->WSecX;
            if( (sec_x == 0) || (sec_x == (yrd->bact->WSecX-1)) ||
                (sec_y == 0) || (sec_y == (yrd->bact->WSecY-1)) ) {

                yrd->chk_Radar_Count++;
                continue;
                }


            /*** --- Testdurchlauf --- ***/
            sector = &( yrd->FirstSector[ yrd->chk_Radar_Count ] );

            value = yr_CheckRadar( yrd, sector );
            if( value > yrd->chk_Radar_Value ) {
                yrd->chk_Radar_Value  = value;
                yrd->chk_Radar_Pos    = yrd->chk_Radar_Count;
                yrd->chk_Radar_Sector = sector;
                yrd->chk_Radar_Time   = msg->global_time;
                }

            yrd->chk_Radar_Count++;
            }
        }
    else {

        yrd->RoboState &= ~ROBO_RADARREADY;
        yrd->chk_Radar_Count = 0;
        yrd->chk_Radar_Value = 0;
        }            


    /*** ------------------------- ***/
    /*** --- Flaktest erlaubt? --- ***/
    /*** ------------------------- ***/

    /*** je kleiner die Welt, desto weniger Testsektoren ***/
    seccount = (yrd->bact->WSecX + yrd->bact->WSecY) / 20 + 1;

    if( yrd->NewAI ) {
        yrd->chk_Safety_Delay -= msg->frame_time;
        if( yrd->chk_Safety_Delay <= 0 ) 
            safety_allowed = TRUE;
        else
            safety_allowed = FALSE;
        }
    else safety_allowed = TRUE;

    /*** Auch die Flaks werden anzahlmaessig ausgebremst. ***/
    if( yrd->ep_Safety )
        secs_per_flak = YR_SecsPerFlak * (100 / yrd->ep_Safety);
    else
        secs_per_flak = YR_SecsPerFlak;

    if( (yrd->ep_Safety > 0) &&
        (yr_OwnSectors( yrd ) / secs_per_flak >= yr_NumberOfFlaks(yrd)) &&
        (safety_allowed) ) {

        while( seccount-- ) {

            /*** --- Ende? --- ***/
            if( yrd->chk_Safety_Count == (yrd->bact->WSecX * yrd->bact->WSecY) ) {

                yrd->chk_Safety_Count = 0;

                /*** Test-Fertig-Flags setzen ***/
                yrd->RoboState |= ROBO_SAFETYREADY;
                return;
                }
            
            /*** --- RandSector? --- ***/
            sec_x = yrd->chk_Safety_Count % yrd->bact->WSecX;
            sec_y = yrd->chk_Safety_Count / yrd->bact->WSecX;
            if( (sec_x == 0) || (sec_x == (yrd->bact->WSecX-1)) ||
                (sec_y == 0) || (sec_y == (yrd->bact->WSecY-1)) ) {

                yrd->chk_Safety_Count++;
                continue;
                }


            /*** --- Testdurchlauf --- ***/
            sector = &( yrd->FirstSector[ yrd->chk_Safety_Count ] );

            value = yr_CheckSafety( yrd, sector );
            if( value > yrd->chk_Safety_Value ) {
                yrd->chk_Safety_Value  = value;
                yrd->chk_Safety_Pos    = yrd->chk_Safety_Count;
                yrd->chk_Safety_Sector = sector;
                yrd->chk_Safety_Time   = msg->global_time;
                }

            yrd->chk_Safety_Count++;
            }
        }
    else {

        yrd->chk_Safety_Count = 0;
        yrd->chk_Safety_Value = 0;
        yrd->RoboState &= ~ROBO_SAFETYREADY;
        }

    /*** ----------------------- ***/
    /*** --- Aufklärungstest --- ***/
    /*** ----------------------- ***/

    /*** je kleiner die Welt, desto weniger Testsektoren ***/
    seccount = (yrd->bact->WSecX + yrd->bact->WSecY) / 20 + 1;

    if( yrd->NewAI ) {
        yrd->chk_Recon_Delay -= msg->frame_time;
        if( yrd->chk_Recon_Delay <= 0 )
            recon_allowed = TRUE;
        else
            recon_allowed = FALSE;
        }
    else recon_allowed = TRUE;

    if( (yrd->ep_Reconnoitre > 0) && (recon_allowed) ) {

        while( seccount-- ) {

            /*** --- Ende? --- ***/
            if( yrd->chk_Recon_Count == (yrd->bact->WSecX * yrd->bact->WSecY) ) {

                yrd->chk_Recon_Count = 0;

                /*** Test-Fertig-Flags setzen ***/
                yrd->RoboState |= ROBO_RECONREADY;
                return;
                }
            
            /*** --- RandSector? bei Rec. ist Rand größer! --- ***/
            sec_x = yrd->chk_Recon_Count % yrd->bact->WSecX;
            sec_y = yrd->chk_Recon_Count / yrd->bact->WSecX;
            if( (sec_x <= 1) || (sec_x >= (yrd->bact->WSecX-2)) ||
                (sec_y <= 1) || (sec_y >= (yrd->bact->WSecY-2)) ) {

                yrd->chk_Recon_Count++;
                continue;
                }


            /*** --- Testdurchlauf --- ***/
            sector = &( yrd->FirstSector[ yrd->chk_Recon_Count ] );

            value = yr_CheckReconnoitre( yrd, sector );
            if( value > yrd->chk_Recon_Value ) {

                /*** Gibt es für dieses Ziel schon einenCommander? ***/
                //gc.flags = GC_PRIMSECTORDIST;
                //gc.distance    =  6 * SECTOR_SIZE;
                gc.target_type = TARTYPE_SECTOR;
                gc.flags       = GC_PRIMTARGET;
                gc.targetpos.x =  (sec_x + 0.5) * SECTOR_SIZE;
                gc.targetpos.z = -(sec_y + 0.5) * SECTOR_SIZE;
                _methoda( yrd->bact->BactObject, YRM_GETCOMMANDER, &gc );

                if( !gc.com ) {
                    yrd->chk_Recon_Value  = value;
                    yrd->chk_Recon_Pos    = yrd->chk_Recon_Count;
                    yrd->chk_Recon_Sector = sector;
                    yrd->chk_Recon_Time   = msg->global_time;
                    }
                }

            yrd->chk_Recon_Count++;
            }
        }
    else {

        yrd->RoboState &= ~ROBO_RECONREADY;
        yrd->chk_Recon_Value = 0;
        yrd->chk_Recon_Count = 0;
        }
            
}



LONG yr_CheckRadar( struct yparobo_data *yrd, struct Cell *sector )
{
/*
**  FUNCTION    Testet den Sector auf Verwendung für eine Radarstation
**              Notwendig:  neutral oder meine
**                          sichtbar
**
**              Günstig:    unbekanntes gebiet im Umfeld
**
**  RESULT      Wert für Eignung
**
**  CHANGED     Bauen nur auf eigenen Sektoren
**
*/

    FLOAT  value, dist;
    struct Cell *tsec;
    int    i, j;
    LONG   energy;
    WORD   sec_x, sec_y, my_x, my_y;
    UBYTE  mask;
    BOOL   found_one;
    
    sec_x = yrd->chk_Radar_Count % yrd->bact->WSecX;
    sec_y = yrd->chk_Radar_Count / yrd->bact->WSecX;
    my_x  = yrd->bact->SectX;
    my_y  = yrd->bact->SectY;

    mask = (UBYTE)( 1<< yrd->bact->Owner );
    if( mask & sector->FootPrint ) {
                
        /*** Energie ***/
        if( sector->SType == SECTYPE_COMPACT )
            energy = sector->SubEnergy[0][0];
        else {

            energy = 0;
            for( i=0; i<3; i++ )
                for( j=0; j<3; j++ )
                    energy += sector->SubEnergy[ i ][ j ];
            }

        if( sector->Owner == yrd->bact->Owner ) {

            /*** Ist er schon bebaut? ***/
            if( sector->WType == 0 ) {           // ist das richtig?

                /*** Jetzt noch den günstigsten ***/
                value = -1.0;

                /*** Entfernung trotzdem ***/
                dist = nc_sqrt( (FLOAT)( (my_x - sec_x) * (my_x - sec_x) +
                                         (my_y - sec_y) * (my_y - sec_y) ) );
                if( dist > 0.01 )
                    value = 1000.0 / dist;
                else
                    value = 500.0;    // mein Sector, nicht ganz so günstig

                /* ----------------------------------------------------------
                ** umliegende Sektoren sollten unbekannt sein! Wenn ich mein
                ** gesamtes Gebiet und den feindlichen Rand überblicke, dann
                ** baue ich eben nicht!
                ** --------------------------------------------------------*/
                found_one = FALSE;
                for( i=0; i<8; i++ ) {

                    tsec = &(sector[ p[2*i+1]*yrd->bact->WSecX + p[2*i] ]);
                    if( !(tsec->FootPrint & mask) ) {
                        found_one = TRUE;
                        value += 10;
                        }
                    }

                /* ------------------------------------------------------
                ** Randsektoren werden nicht bebaut, weil der
                ** Rand unsichtbar ist. Ein reiner Abzug geht aber nicht,
                ** weil dann dort trotzdem massiv gebaut wird, wenn der
                ** Robo sein gesamtes Land überblicken kann
                ** ----------------------------------------------------*/
                if( (sec_x <= 1) || (sec_x >= (yrd->bact->WSecX-2)) ||
                    (sec_y <= 1) || (sec_y >= (yrd->bact->WSecY-2)) )
                    found_one = FALSE;

                if( found_one )
                    return( (LONG) value );
                else
                    return( -1 );
                }
            }
        }
    return( -1 );
}

LONG yr_CheckReconnoitre( struct yparobo_data *yrd, struct Cell *sector )
{
/*
**  FUNCTION    Testet den Sector auf "Unbekanntheit", also ob es sich lohnt,
**              dort einen Satelliten hinzuschicken. Dabei müssen wir natürlich
**              testen, ob nicht schon jemand den als Ziel hat, egal, ob er
**              Satellit ist oder nicht.
**
**              Notwendig:  unsichtbar
**
**              Günstig:    unbekanntes gebiet im Umfeld
**                          große Entfernung
**
**  RESULT      Wert für Eignung
**
**  CHANGED     Bauen nur auf eigenen Sektoren
**
*/

    FLOAT  value, dist;
    int    i;
    WORD   sec_x, sec_y, my_x, my_y;
    UBYTE  mask;
    
    sec_x = yrd->chk_Recon_Count % yrd->bact->WSecX;
    sec_y = yrd->chk_Recon_Count / yrd->bact->WSecX;
    my_x  = yrd->bact->SectX;
    my_y  = yrd->bact->SectY;

    mask = (UBYTE)( 1<< yrd->bact->Owner );
    if( !(mask & sector->FootPrint) ) {

        /*** Jetzt den günstigsten ***/
        value = -1.0;

        /*** Entfernung trotzdem ***/
        dist = nc_sqrt( (FLOAT)( (my_x - sec_x) * (my_x - sec_x) +
                                 (my_y - sec_y) * (my_y - sec_y) ) );
        if( dist > 0.01 )
            value = dist;
        else
            value = 0.0;    // mein Sector, nicht ganz so günstig

        /*** umliegende Sektoren sollten unbekannt sein ***/
        for( i=0; i<8; i++ ) {

            struct Cell *tsec;

            tsec = &(sector[ p[2*i+1]*yrd->bact->WSecX + p[2*i] ]);
            if( !(tsec->FootPrint & mask) ) {
                value += 5;
                }
            }

        return( (LONG) value );
        }

    return( -1 );
}


LONG yr_CheckSafety( struct yparobo_data *yrd, struct Cell *sector )
{
/*
**  FUNCTION    Testet den Sector auf Verwendung für eine Flakstellung
**              Notwendig:  neutral oder meine
**                          sichtbar
**
**              Günstig:    eigenes bebautes gebiet im Umfeld
**
**  RESULT      Wert für Eignung
**
**  CHANGED     Bauen nur auf eigenen Sektoren
**
*/

    FLOAT  value, dist;
    struct Cell *tsec;
    int    i, j;
    LONG   energy;
    WORD   sec_x, sec_y, my_x, my_y;
    UBYTE  mask;
    
    sec_x = yrd->chk_Safety_Count % yrd->bact->WSecX;
    sec_y = yrd->chk_Safety_Count / yrd->bact->WSecX;
    my_x  = yrd->bact->SectX;
    my_y  = yrd->bact->SectY;

    mask = (UBYTE)( 1<< yrd->bact->Owner );
    if( mask & sector->FootPrint ) {
                
        /*** Energie ***/
        if( sector->SType == SECTYPE_COMPACT )
            energy = sector->SubEnergy[0][0];
        else {

            energy = 0;
            for( i=0; i<3; i++ )
                for( j=0; j<3; j++ )
                    energy += sector->SubEnergy[ i ][ j ];
            }

        if( sector->Owner == yrd->bact->Owner ) {

            /*** Ist er schon bebaut? ***/
            if( sector->WType == 0 ) {           // ist das richtig?

                /*** Jetzt noch den günstigsten ***/
                value = -1.0;

                /*** Entfernung trotzdem ***/
                dist = nc_sqrt( (FLOAT)( (my_x - sec_x) * (my_x - sec_x) +
                                         (my_y - sec_y) * (my_y - sec_y) ) );
                if( dist > 0.01 )
                    value = 1000.0 / dist;
                else
                    value = 500.0;    // mein Sector, nicht ganz so günstig

                /*** umliegende Sektoren sollten bebaut und eigen sein ***/
                for( i=0; i<8; i++ ) {

                    tsec = &(sector[ p[2*i+1]*yrd->bact->WSecX + p[2*i] ]);
                    if( (tsec->WType != 0) && (tsec->Owner == yrd->bact->Owner) )
                        value += 5;
                    }
                return( (LONG) value );
                }
            }
        }
    return( -1 );
}


    
LONG yr_CheckPower( struct yparobo_data *yrd, struct Cell *sector )
{
/*
**  FUNCTION    Testet die Eignung für einen Kraftwerksbau.
**              Notwendig:  Eigentümer ich oder 0
**                          ( Owner 0 mit Energie ist das Felsen? )
**                          ungesättigt
**
**              Sinnvoll:   Entfernung
**                          keine Bakterien dort
**                          geringe Energie
**                          eigene Sektoren nebenan
**
**  RESULT      Punktzahl
**
**  INPUT       dr Sägdor
**
**  CHANGED     23-Jan-96  8100 000C created
**              bauen nur auf eigenem Gebiet
*/

    WORD   sec_x, sec_y, my_x, my_y, i, j, e;
    FLOAT  dist;
    struct Cell *tsec;
    LONG   energy;
    UBYTE  mask;
    FLOAT  value;
    struct Bacterium *bact;

    sec_x = yrd->chk_Power_Count % yrd->bact->WSecX;
    sec_y = yrd->chk_Power_Count / yrd->bact->WSecX;
    my_x  = yrd->bact->SectX;
    my_y  = yrd->bact->SectY;

    /*** Sichtbar ? ***/
    mask = (UBYTE) 1 << yrd->bact->Owner;
    if( mask & sector->FootPrint ) {

        /*** Ungesättigt? ***/
        if( sector->EnergyFactor < 255 ) {

            /*** Schon Kraftwerk? ***/
            if( sector->WType == 0 ) {      // geht das? ja
    
                /*** Energetisch runter? ***/
                if( sector->SType == SECTYPE_COMPACT )
                    energy = sector->SubEnergy[0][0];
                else {

                    energy = 0;
                    for( i=0; i<3; i++ )
                        for( j=0; j<3; j++ )
                            energy += sector->SubEnergy[ i ][ j ];
                    }

                if( sector->Owner == yrd->bact->Owner ) {

                    /*** OK, von nun an ist alles wichtig, aber nicht notwendig ***/

                    value = -1.0;
                    
                    /*** Abstand vom Robo (groß --> schlecht) ***/
                    dist = nc_sqrt( (FLOAT)( (my_x - sec_x) * (my_x - sec_x) +
                                             (my_y - sec_y) * (my_y - sec_y) ) );
                    if( dist > 0.01 )
                        value = 1000.0 / dist;
                    else
                        value = 500.0;    // mein Sector, nicht ganz so günstig

                    /*** Wenn die Distanz zu groß ist, dann bauen wir nicht! ***/
                    if( dist > 8.0 ) return( -1.0 );
                    
                    /*** Stromstärke dort (groß --> schlecht) ***/
                    value += (255 - sector->EnergyFactor) / 3.0;

                    /*
                    ** Sektoren rundherum (keine Grenzen, Rand wird 
                    ** rausgefiltert) 
                    */
                    for( i=0; i<8; i++ ) {

                        tsec = &(sector[ p[2*i+1]*yrd->bact->WSecX + p[2*i] ]);
                        if( (tsec->Owner == yrd->bact->Owner) ) {

                            /*** Eigenes Gebiet. Energie untersuchen ***/
                            if( tsec->SType == SECTYPE_COMPACT )
                                e = tsec->SubEnergy[0][0];
                            else {
                                int x,y;

                                e = 0;
                                for( x=0; x<3; x++ )
                                    for( y=0; y<3; y++)
                                        e += tsec->SubEnergy[ x ][ y ];
                                }

                            value += 10 + ((255 - e) / 20.0 );
                            }

                        /*** Weiterhin sollten umliegende Sektoren keine KW sein ***/
                        if( tsec->WType == WTYPE_Kraftwerk )
                            value *= 0.7;
                        }
                    
                    /*** Punktabzug für Bakterien im Sektor ***/
                    bact = (struct Bacterium *) sector->BactList.mlh_Head;
                    while( bact->SectorNode.mln_Succ ) {

                        value -= 5.0;
                        bact = (struct Bacterium *) bact->SectorNode.mln_Succ;
                        }


                    return( (LONG) value );
                    }
                }
            }
        }
    return( -1 );
}

    

LONG yr_CheckTerritorium( struct yparobo_data *yrd, struct Cell *sector )
{
/*
** Wir untersuchen den Sektor, wie sehr er für Eroberungen geeignet ist.
** Dem ordnen wir einen Wert zu.
** Notwendig ist:
**                  feindliches Gebiet (auch 0!)
** Wichtig ist:     Nähe,
**                  Stärke der Feinde
**                  angrenzendes eigenes Gebiet
*/

    UBYTE  mask;
    WORD   sec_x, sec_y, my_x, my_y;
    FLOAT  value, dist;
    int    i;
    struct Bacterium *bact;


    sec_x = yrd->chk_Terr_Count % yrd->bact->WSecX;
    sec_y = yrd->chk_Terr_Count / yrd->bact->WSecX;
    my_x  = yrd->bact->SectX;
    my_y  = yrd->bact->SectY;

    /*** Sichtbar und feindlich? ***/
    mask = (UBYTE) 1 << yrd->bact->Owner;
    if( sector->Owner != yrd->bact->Owner ) {

        /*** Für die Entfernung gibt es 100 / distance Punkte ***/
        dist = nc_sqrt( (FLOAT)( (my_x - sec_x) * (my_x - sec_x) +
                                 (my_y - sec_y) * (my_y - sec_y) ) );
        if( dist > 0.01 )
            value = 1000.0 / dist;
        else
            value = 1500.0; // mein Sector

        /*** Bebaunungspunkte gibt es nur bei Sichtbarkeit, sonst Beschiß ***/
        if( sector->FootPrint & mask ) {

            /*** Für Bebauung gibt es Pluspunkte ***/
            if( sector->WType ) value += 20;

            /*** Hauptaufgabe ist nachwievor, "Gates" zu killen ***/
            if( WTYPE_OpenedGate  == sector->WType ) value += 40;
            if( WTYPE_Kraftwerk   == sector->WType ) value += 50;
            }

        /*** Für jeden angrenzenden eigenen Sektor gibt es Punkte ***/
        for( i = 0; i < 8; i++ ) {

            /*** RandSektor? ***/
            if( ((sec_x + delta[ 2*i ])   < 1) ||
                ((sec_y + delta[ 2*i+1 ]) < 1) ||
                ((sec_x + delta[ 2*i ])   > (yrd->bact->WSecX-2)) ||
                ((sec_y + delta[ 2*i+1 ]) > (yrd->bact->WSecY-2)) ) {

                /*** Ignorieren ***/
                }
            else {

                /*** Testen ***/
                if( sector[ delta[ 2*i ]+delta[ 2*i+1 ]*yrd->bact->WSecX ].Owner
                    == yrd->bact->Owner )
                    value += 3.0;
                }
            }

        /*** Commandertest nur bei Sichtbarkeit ***/
        if( sector->FootPrint & mask ) {

            /*** Für feindliche Commander gibts Pluspunkte ***/
            bact = (struct Bacterium *) sector->BactList.mlh_Head;
            while( bact->SectorNode.mln_Succ ) {

                if( (bact->Owner       != yrd->bact->Owner) &&
                    (bact->Owner       != 0) &&
                    (bact->BactClassID != BCLID_YPAMISSY) ) {

                    if( bact->BactClassID == BCLID_YPAROBO )
                        value += 100.0;
                    else
                        value += 5;
                    }
                bact = (struct Bacterium *) bact->SectorNode.mln_Succ;
                }
            }

        return( (LONG) value );
        }
    else
        return( -1 );
}

    
LONG yr_CheckEnemy( struct yparobo_data *yrd, LONG *CID, WORD *sec_x, WORD *sec_y )
{
/*  ----------------------------------------------------------------------------
**  Wir suchen Feinde. Dabei interessieren uns nur Eindringlinge, das heißt
**  Leute auf eigenem gebiet. Einzige Ausnahme ist der Robo, da entscheidet
**  nur die Sichtbarkeit. Dabei suchen wir nicht sektorweise, was zu kompliziert
**  ist, sondern scannen alle Robos und deren Commander
**  Robos können BakterienHauptziele sein und brauchen deshalb eine CommandID!
**
**  Notwendig:  feindlicher Commander oder Robo auf sichtbarem Gebiet
**  Wichtig:    Nähe
**              Stärke
**              eigenes Gebiet
**
**  Im Gegensatz zu Sektoren sind 0-Vehicle nicht eroberungswürdig, weil neutral
**  Robos werden nicht beachtet, weil die Extra untersucht werden!!! Mit einer
**  Ausnahme: Wenn der Robo nicht Ziel eines geplanten Angriffes, sondern Ziel
**  einer Verteidigungshandlung ist. Das passiert, wenn er direkt neben mir
**  ist.
**
**  zeitliche Ausbremsung: Ich teste zuerst alle, deren (ident % 8) == 0 ist,
**  dann die mit 1 usw. Bei 7 fange ich wieder von vorn an.
**  --------------------------------------------------------------------------*/

    struct MinList *list, *slist;
    struct MinNode *node;
    FLOAT  value, merkvalue = -0.5;
    UBYTE  mask;
    WORD   my_x, my_y;
    struct OBNode *robo, *slave;
    ULONG  CommandID;


    my_x = yrd->bact->SectX;
    my_y = yrd->bact->SectY;
    
    mask = (UBYTE)( 1 << yrd->bact->Owner );

    node = (struct MinNode *) &(yrd->bact->slave_node);
    while( node->mln_Pred ) node = node->mln_Pred;
    list = (struct MinList *) node;
    robo = (struct OBNode *) list->mlh_Head;

    /*** Nun alle mal durchsuchen ***/
    while( robo->nd.mln_Succ ) {

        if( (robo->bact->Owner     == yrd->bact->Owner) ||
            (robo->bact->Owner     == 0) ||
            (robo->bact->MainState == ACTION_DEAD) ) {

            robo = (struct OBNode *) robo->nd.mln_Succ;
            continue;
            }

        /* --------------------------------------------------------------
        ** Robos hier nur untersuchen, wenn sie neben mir sitzen, es sich
        ** also um eine Verteidigung handelt
        ** ------------------------------------------------------------*/
        if( ( abs(robo->bact->SectX - yrd->bact->SectX) <= 2 ) &&
            ( abs(robo->bact->SectY - yrd->bact->SectY) <= 2 ) ) {

            /*** Das ist schlimm. Darum kuemmern wir uns! ***/
            *CID     = robo->bact->CommandID;
            *sec_x   = robo->bact->SectX;
            *sec_y   = robo->bact->SectY;
            return( 500 );
            }

        /*** Commander des Robos scannen ***/
        slist = &(robo->bact->slave_list);
        slave = (struct OBNode *) slist->mlh_Head;

        while( slave->nd.mln_Succ ) {

            ULONG robogun = 0L;

            /*** Darf ich das untersuchen? (Ausbremsung) ***/
            if( yrd->chk_Enemy_Div == (slave->bact->ident % 16) ) {

                /*** Bordkanonen und tote Brocken rausfiltern ***/
                if( BCLID_YPAGUN == slave->bact->BactClassID )
                    _get( slave->o, YGA_RoboGun, &robogun );

                /*** Sichtbar? ***/
                if( (slave->bact->Sector->FootPrint & mask) &&
                    (ACTION_DEAD != slave->bact->MainState) &&
                    (0L == robogun) ) {

                    value = yr_AnalyzeCommand( slave, yrd, my_x, my_y );
                    if( value > merkvalue ) {

                        CommandID     = slave->bact->CommandID;
                        merkvalue     = value;
                        *sec_x        = slave->bact->SectX;
                        *sec_y        = slave->bact->SectY;
                        }
                    }
                }

            /*** next one ***/
            slave = (struct OBNode *) slave->nd.mln_Succ;
            }

        /*** next robo ***/
        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    yrd->chk_Enemy_Div++;
    if( yrd->chk_Enemy_Div > 15 ) yrd->chk_Enemy_Div = 0L;

    /*** fertsch, nu guckn, was mr gefun'n ham ***/
    if( merkvalue < 0.0 ) {

        *CID = 0;
        return( -1 );
        }
    else {

        *CID = CommandID;
        return( (LONG) merkvalue );
        }
}

    
LONG yr_CheckRobo( struct yparobo_data *yrd, LONG *CID, WORD *sec_x, WORD *sec_y )
{
/*  --------------------------------------
**  Sucht nur nach Robos und sonst nix
**
**  Notwendig:  Robo auf sichtbarem Gebiet
**  Wichtig:    Nähe
**              Stärke
**              eigenes Gebiet
**
**  ------------------------------------*/

    struct MinList *list;
    struct MinNode *node;
    FLOAT  value, merkvalue = -0.5;
    UBYTE  mask;
    struct OBNode *robo;
    ULONG  CommandID;
    WORD   my_x, my_y;


    my_x = yrd->bact->SectX;
    my_y = yrd->bact->SectY;

    mask = (UBYTE)( 1 << yrd->bact->Owner );

    node = (struct MinNode *) &(yrd->bact->slave_node);
    while( node->mln_Pred ) node = node->mln_Pred;
    list = (struct MinList *) node;
    robo = (struct OBNode *) list->mlh_Head;

    /*** Nun alle mal durchsuchen ***/
    while( robo->nd.mln_Succ ) {

        if( (robo->bact->Owner     == yrd->bact->Owner) ||
            (robo->bact->Owner     == 0) ||
            (robo->bact->MainState == ACTION_DEAD) ) {

            robo = (struct OBNode *) robo->nd.mln_Succ;
            continue;
            }

        /*** Robo sichtbar? Dann untersuchen ***/
        if( robo->bact->Sector->FootPrint & mask ) {

            value = yr_AnalyzeCommand( robo, yrd, my_x, my_y );
            if( value > merkvalue ) {

                CommandID = robo->bact->CommandID;
                merkvalue = value;
                *sec_x    = robo->bact->SectX;
                *sec_y    = robo->bact->SectY;
                }
            }

        /*** next robo ***/
        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    /*** fertsch, nu guckn, was mr gefun'n ham ***/
    if( merkvalue < 0.0 ) {

        *CID = 0;
        return( -1 );
        }
    else {

        *CID = CommandID;
        return( (LONG) merkvalue );
        }
}


FLOAT yr_AnalyzeCommand( struct OBNode *Commander, struct yparobo_data *yrd,
                         WORD my_x, WORD my_y )
{
    WORD   sec_x, sec_y;
    FLOAT  dist, value;

    /*** Lebt der überhaupt noch? ***/
    if( Commander->bact->ExtraState & EXTRA_LOGICDEATH ) return( -1.0 );

    /*** Entfernung untersuchen ***/
    sec_x = Commander->bact->SectX;
    sec_y = Commander->bact->SectY;
    dist = nc_sqrt((FLOAT)( (my_x - sec_x) * (my_x - sec_x) +
                            (my_y - sec_y) * (my_y - sec_y) ) );
    if( dist != 0 )
        value = 100 / dist;
    else
        value = 200; // auf eigenem Sector

   /*** auf eigenem Gebiet? ***/
    if( Commander->bact->Sector->Owner == yrd->bact->Owner )
        value += 100;

    return( value );
}    


LONG yr_CheckPlace( struct yparobo_data *yrd ) {

    /* -------------------------------------------------------------
    ** Suche den energetisch günstigsten Platz. Dazu vergleichen wir
    ** den aktuellen testsector mit dem eigenen Sector
    ** notwendig: mehr Energie als eigener Sektor und besseres
    ** Umfeld
    ** -----------------------------------------------------------*/

    LONG  value1, value2;
    WORD  sec_x, sec_y;
    FLOAT dist;

    sec_x = yrd->chk_Place_Count % yrd->bact->WSecX;
    sec_y = yrd->chk_Place_Count / yrd->bact->WSecX;

    value1 = yr_GoodSector( yrd, yrd->bact->SectX, yrd->bact->SectY );
    value2 = yr_GoodSector( yrd, sec_x, sec_y );

    /* -----------------------------------------------------------------
    ** Nach allgemeiner Wertigkeit jetzt auch Entfernung mit einrechnen.
    ** Je groesser die Entfernung, desto Schlechter. damit schlechte
    ** Werte durch die Entfernung nicht "verbessert" werden, nehmen wir
    ** nur welche, die besser als der atuelle sind. 
    ** ---------------------------------------------------------------*/
    dist = nc_sqrt( (yrd->bact->SectX - sec_x) * (yrd->bact->SectX - sec_x) +
                    (yrd->bact->SectY - sec_y) * (yrd->bact->SectY - sec_y) );
    if( (dist > 0) && (value1 < value2) ) {

        value2 = ((FLOAT)value2) * (1.0 - dist * 0.8 / 91);
        }

    if( value2 > value1 )
        return( value2 );
    else
        return( -1 );
}


LONG yr_GoodSector( struct yparobo_data *yrd, WORD sec_x, WORD sec_y )
{
    /* --------------------------------------------------------------
    ** Wie gut ist dieser Sector? dazu teste ich diesen und alle
    ** umliegenden Sektoren auf Eigentümer und feinde und den Eigenen
    ** auf Energie. Das geht, weil Randsektoren sowieso rausgefiltert
    ** werden.
    ** ------------------------------------------------------------*/
    FLOAT   value = 0;
    struct Cell *sector;
    WORD   x, y;

    for( x = -1; x < 2; x++ ) {
        for( y = -1; y < 2; y++ ) {

            struct Bacterium *v;
            struct getrldratio_msg gr;
            FLOAT  owner_factor;

            /*** Infos holen ***/
            sector = &(yrd->FirstSector[ sec_x + x +
                                        (sec_y + y ) * yrd->bact->WSecX ]);

            /* -------------------------------------------------------
            ** Energiefaktor und Mikrowirkung sollten wir doch überall
            ** testen, denn, ja, doch, ist schon besser so!
            ** Ausserdem hat der Owner noch was zu sagen. Selbst wenn
            ** energie 0 ist, sollten wir nicht auf einen feindlichen
            ** sektor gehen. Der Zielsektor selbst hat noch eine 
            ** hoehere Bedeutung.
            ** -----------------------------------------------------*/

            gr.owner = sector->Owner;
            _methoda( yrd->world, YWM_GETRLDRATIO, &gr );

            if( (x==0) && (y==0) )
                owner_factor = 3.0;
            else
                owner_factor = 1.0;

            if( yrd->bact->Owner == sector->Owner ) {
                value += ((FLOAT)sector->EnergyFactor) * gr.ratio;
                value += owner_factor;
                }
            else {
                value -= ((FLOAT)sector->EnergyFactor) * gr.ratio;
                value -= owner_factor;
                }

            /* --------------------------------------------------
            ** Vehicletest: eigen: +1, fremd -1. nur bei normalen
            ** Flaks und Robo, sonst wird es zu chaotisch 
            ** ------------------------------------------------*/
            v = (struct Bacterium *) sector->BactList.mlh_Head;

            while( v->SectorNode.mln_Succ ) {

                FLOAT check_this = 0.0;

                if( (BCLID_YPAROBO == v->BactClassID) &&
                    (yrd->bact     != v) ) {
                    check_this = 5.0;
                    }
                else {
                    if( BCLID_YPAGUN == v->BactClassID ) {
                        ULONG rgun;
                        _get( v->BactObject, YGA_RoboGun, &rgun );
                        if( !rgun ) check_this = 1.0;
                        }
                    }

                if( check_this > 0.0 ) {

                    if( v->Owner != yrd->bact->Owner )
                        value -= check_this;
                    else
                        value += check_this;
                    }

                v = (struct Bacterium *) v->SectorNode.mln_Succ;
                }
            }
        }

    return( (LONG) value );
}


//LONG yr_CheckPlace( struct yparobo_data *yrd, WORD *sec_x, WORD *sec_y )
//{
//    /* ------------------------------------------------------------
//    ** Wir suchen den besten Sektor im Sichtbereich. Sind wir schon
//    ** auf selbigen, dann geben wir -1 zurück
//    ** ----------------------------------------------------------*/
//
//    LONG dx, dy, mx, my, value, merk_value = -1;
//
//    for( dx = -yrd->bact->View; dx <= yrd->bact->View; dx++ )
//
//        for( dy =  (LONG)(-nc_sqrt( yrd->bact->View * yrd->bact->View - dx * dx));
//             dy <= (LONG)( nc_sqrt( yrd->bact->View * yrd->bact->View - dx * dx));
//             dy++ ) {
//
//            struct Cell *sector;
//            struct Bacterium *leut;
//
//            /*** testen, ob es den Sektor gibt ***/
//            if( ((yrd->bact->SectX + dx) < 1 ) ||
//                ((yrd->bact->SectX + dx) > (yrd->bact->WSecX - 2)) ||
//                ((yrd->bact->SectY + dy) < 1 ) ||
//                ((yrd->bact->SectY + dy) > (yrd->bact->WSecY - 2)) )
//                continue;
//
//            /*** sector holen ***/
//            sector = &( yrd->FirstSector[ (yrd->bact->SectY + dy) *
//                                          yrd->bact->WSecX +
//                                          yrd->bact->SectX + dx ] );
//
//            value = 0;
//
//            /* -----------------------------------------------
//            ** eigener Sektor bekommt Bonus, damit wir uns nur
//            ** bewegen, wenn es sinnvoll ist
//            ** ---------------------------------------------*/
//            if( (0 == dx) && (0 == dy) ) value += 10;
//
//            /*** Energie untersuchen ***/
//            if( sector->Owner == yrd->bact->Owner )
//                value += sector->EnergyFactor;
//            else
//                value -= sector->EnergyFactor;
//
//            /*** Leute dort untersuchen ***/
//            leut = (struct Bacterium *) sector->BactList.mlh_Head;
//            while( leut->SectorNode.mln_Succ ) {
//
//                if( leut->Owner != yrd->bact->Owner ) {
//
//                    value -= 15;
//                    if( BCLID_YPAROBO == leut->BactClassID ) value -= 1000;
//                    }
//
//                leut = (struct Bacterium *) leut->SectorNode.mln_Succ;
//                }
//
//            /*** wie ist das einzuschätzen? ***/
//            if( value > merk_value ) {
//
//                /*** merken ***/
//                merk_value = value;
//                mx     = dx;
//                my     = dy;
//                *sec_x = yrd->bact->SectX + dx;
//                *sec_y = yrd->bact->SectY + dy;
//                }
//
//            /*** 's war für den Sektor alles ***/
//            }
//
//    /*** war es etwa der eigene? ***/
//    if( (0 == mx) && (0 == my ) )
//        merk_value = -1;
//
//    /*** 0 is nüscht ***/
//    if( merk_value == 0 )
//        merk_value = -1;
//
//    return( merk_value );
//}



void yr_GetBestSectorToTarget( struct yparobo_data *yrd, UWORD pos, 
                               struct settarget_msg *target )
{
    /*
    ** Ermittelt den besten Sektor in Richtung des Zieles. Das Ziel ermittle
    ** ich aus pos. Das Problem ist, daß ich einen Sektor NEBEN dem Sektor
    ** brauche, den ich bebauen will.
    */

    WORD ziel_x, ziel_y;

    ziel_x = pos % yrd->bact->WSecX;
    ziel_y = pos / yrd->bact->WSecX;

    if( (ziel_x == yrd->bact->SectX) && (ziel_y == yrd->bact->SectY) ) {

        if( ziel_x < 2 )
            ziel_x++;
        else
            ziel_x--;
        }
    else {

        if( ziel_x > yrd->bact->SectX ) ziel_x--;
        if( ziel_x < yrd->bact->SectX ) ziel_x++;
        if( ziel_y > yrd->bact->SectY ) ziel_y--;
        if( ziel_y < yrd->bact->SectY ) ziel_y++;
        }

    /*** Daraus nun ein Ziel machen ***/
    target->pos.x        =  (ziel_x + 0.5) * SECTOR_SIZE;
    target->pos.z        = -(ziel_y + 0.5) * SECTOR_SIZE;
    target->target_type  = TARTYPE_SECTOR;
    target->priority     = 0;
}


                
int yr_NumberOfFlaks( struct yparobo_data *yrd )
{
    /* -----------------------------------------------------------
    ** Gibt Zahl der Flaks zurück, indem es alle Building-Sektoren
    ** mit bewaffneten "BCLID_YPAGUN"s zählt.
    ** Achtung, Sektor nur einmal zählen!
    ** ---------------------------------------------------------*/

    int i, count = 0;

    for( i = 0; i < yrd->bact->WSecX * yrd->bact->WSecY; i++ ) {

        if( yrd->FirstSector[ i ].WType == WTYPE_Building ) {

            /*** Eine Flak im Sektor? ***/
            struct Bacterium *vhcl;

            vhcl = (struct Bacterium *) yrd->FirstSector[ i ].BactList.mlh_Head;
            while( vhcl->SectorNode.mln_Succ ) {

                /*** Wenn eine Flak drinnen ist, dann hochzählen und abbrechen ***/
                if( (BCLID_YPAGUN     == vhcl->BactClassID) &&
                    (ACTION_DEAD      != vhcl->MainState)   &&
                    (yrd->bact->Owner == vhcl->Owner) ) {

                    ULONG rg;

                    _get( vhcl->BactObject, YGA_RoboGun, &rg );

                    if( (rg == 0) &&
                        ((vhcl->auto_ID != NO_AUTOWEAPON) ||
                         (vhcl->mg_shot != NO_MACHINEGUN)) ) {

                        /*** treffer ***/
                        count++;
                        break;
                        }
                    }

                vhcl = (struct Bacterium *) vhcl->SectorNode.mln_Succ;
                }
            }
        }

    return( count );
}

                
int yr_NumberOfRadars( struct yparobo_data *yrd )
{
    /* ----------------------------------------------------------------
    ** Gibt Zahl der RadarSchüsseln zurück. Dazu suche ich zuerst mal
    ** alle WType_Buildings, was Flaks oder Schüsseln sein können, denn
    ** die anderen haben nen Extra-WType. So, und dann kann mr de
    ** Bactliste Scannen.
    ** --------------------------------------------------------------*/

    int i, count = 0;

    for( i = 0; i < yrd->bact->WSecX * yrd->bact->WSecY; i++ ) {

        if( yrd->FirstSector[ i ].WType == WTYPE_Building ) {

            /*** Eine Schüssel im Sektor? ***/
            struct Bacterium *vhcl;

            vhcl = (struct Bacterium *) yrd->FirstSector[ i ].BactList.mlh_Head;
            while( vhcl->SectorNode.mln_Succ ) {

                /*** Wenn eine Schüssel drinnen ist, dann hochzählen und abbrechen ***/
                if( (BCLID_YPAGUN     == vhcl->BactClassID) &&
                    (ACTION_DEAD      != vhcl->MainState)   &&
                    (yrd->bact->Owner == vhcl->Owner) ) {

                    ULONG rg;

                    _get( vhcl->BactObject, YGA_RoboGun, &rg );

                    if( (rg == 0) &&
                        (vhcl->auto_ID == NO_AUTOWEAPON) &&
                        (vhcl->mg_shot == NO_MACHINEGUN) ) {

                        /*** treffer ***/
                        count++;
                        break;
                        }
                    }

                vhcl = (struct Bacterium *) vhcl->SectorNode.mln_Succ;
                }
            }
        }


    return( count );
}


int yr_OwnSectors( struct yparobo_data *yrd )
{
    int i, count = 0;

    for( i = 0; i < yrd->bact->WSecX * yrd->bact->WSecY; i++ ) {

        if( yrd->FirstSector[ i ].Owner == yrd->bact->Owner )
            count++;
        }

    return( count );
}


int yr_UnvisibleSectors( struct yparobo_data *yrd )
{
    int i, count = 0;

    for( i = 0; i < yrd->bact->WSecX * yrd->bact->WSecY; i++ ) {

        if( !( yrd->FirstSector[ i ].FootPrint & (1<<yrd->bact->Owner)) )
            count++;
        }

    return( count );
}


BOOL yr_RoboNearRobo( struct yparobo_data *yrd )
{
    BOOL   ret = FALSE;
    struct MinNode *node;
    struct OBNode  *robo;
    struct MinList *list;

    node = (struct MinNode *) &(yrd->bact->slave_node);
    while( node->mln_Pred ) node = node->mln_Pred;
    list = (struct MinList *) node;
    robo = (struct OBNode *) list->mlh_Head;

    while( robo->nd.mln_Succ ) {

        if( (BCLID_YPAROBO    == robo->bact->BactClassID) &&
            (yrd->bact->Owner != robo->bact->Owner) ) {

            /*** Ein Robo ***/
            if( (abs( yrd->bact->SectX - robo->bact->SectX) < 3) &&
                (abs( yrd->bact->SectY - robo->bact->SectY) < 3) ) {
                ret = TRUE;
                break;
                }
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    return( ret );
}


BOOL yr_EnemyNearRobo( struct yparobo_data *yrd )
{
    /* ----------------------------------------------------
    ** Keine Robos! die werden extra getestet, weil da auch
    ** andere Vehicle benoetigt werden.
    ** --------------------------------------------------*/
    BOOL   ret = FALSE;
    struct Cell *sector, *tsec;
    int    i, j;
    struct Bacterium *v;

    sector = yrd->bact->Sector;

    for( i = -2; i <= 2; i++ ) {
        for( j = -2; j <= 2; j++ ) {

            /*** Sektor vorhanden? ***/
            if( ((yrd->bact->SectX + i) > 0) &&
                ((yrd->bact->SectX + i) < (yrd->bact->WSecX - 1)) &&
                ((yrd->bact->SectY + j) > 0) &&
                ((yrd->bact->SectY + j) < (yrd->bact->WSecY - 1)) ) {

                /*** BactList Scannen ***/
                tsec = &(sector[ j * yrd->bact->WSecX + i ]);

                v = (struct Bacterium *) tsec->BactList.mlh_Head;
                while( v->SectorNode.mln_Succ ) {

                    if( (v->Owner       != 0) &&
                        (v->Owner       != yrd->bact->Owner) &&
                        (v->BactClassID != BCLID_YPAMISSY) &&
                        (v->BactClassID != BCLID_YPAROBO) &&
                        (v->MainState   != ACTION_DEAD) ) {

                        ret = TRUE;
                        break;
                        }

                    v = (struct Bacterium *)v->SectorNode.mln_Succ;
                    }
                }
            }
        }

    return( ret );
}

