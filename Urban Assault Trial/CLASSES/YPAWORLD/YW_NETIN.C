/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für networking
**
**  (C) Copyright 1995 by A.Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "visualstuff/ov_engine.h"
#include "yw_protos.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"

#ifdef __NETWORK__

#include "network/networkclass.h"
#include "ypa/ypamessages.h"

#include "yw_netprotos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

extern ULONG DeathCount;
extern struct ConfigItem yw_ConfigItems[];


BOOL yw_InitNetwork( struct ypaworld_data *ywd )
{
    #ifdef __WINDOWS__
    ywd->nwo = _new( "windp.class", TAG_DONE );
    #endif

    if( ywd->nwo ) {
    
        struct localmachine_msg lm;
        struct sessionident_msg si;

        /*** Derzeit interpolieren ***/
        ywd->interpolate = TRUE;

        /*** kein ASKLOCALMACHINE mehr, wegen dem Dialup-Requester ***/
        memset( ywd->local_address,0, 4 );
        ywd->local_name[ 0 ] = 0;
        ywd->local_addressstring[ 0 ] = 0;
        
        /*** Fuer identifzierungen von Sessions alter Programme ***/   
        si.check_item = ywd->Version;   // damit soll getestet werden    
        _methoda( ywd->nwo, NWM_SETSESSIONIDENT, &si ); 
                
        return( TRUE );
        }
    else {

        _LogMsg("Unable to create network-Object\n");
        return( FALSE );
        }
}


void yw_KillNetwork( struct ypaworld_data *ywd )
{
    if( ywd->nwo ) {

        _dispose( ywd->nwo );
        ywd->nwo = NULL;
        }
}


void yw_CleanupNetworkSession( struct ypaworld_data *ywd )
{

    /*** Nur noch Schale für CleanupNetworkData ***/
    yw_CleanupNetworkData( ywd );
}


_dispatcher(ULONG, yw_YWM_NETWORKLEVEL, struct createlevel_msg *msg)
/*
**  FUNCTION    Initialisiert einen Netzwerklevel. Ist eine Abänderung
**              der CREATELEVEL-methode von Floh.
**
**
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct LevelDesc ld;
    BOOL level_ok = FALSE, found = FALSE;
    ULONG i;
    struct OBNode *robo;

    /* ------------------------------------------------------------
    ** Robos werden ohne Abgleich der idents erzeugt. Die einzelnen
    ** Applikationen haben aber eine zum teil unterschiedliche
    ** "Geschichte" und damit unterschiedliche DeathCounts. Somit
    ** reseten wir den DeathCount. Das ist der einfachste Weg.
    ** ----------------------------------------------------------*/
    DeathCount = (1L<<16);

    // FIXME_FLOH: yw_CommonLevelInit() + Local Vars aufgeräumt!!!
    if (yw_CommonLevelInit(ywd,&ld,msg->level_num,msg->level_mode)) {
        if (yw_LoadTypMap(ywd,ld.typ_map)) {
            if (yw_LoadOwnMap(ywd,ld.own_map)) {
                if (yw_LoadHgtMap(ywd,ld.hgt_map)) {
                    if (yw_NetPlaceRobos(ywd, &(ld.robos[0]),ld.num_robos) ) {
                        if (yw_LoadBlgMap(o,ywd,ld.blg_map)) {

                            ULONG secx,secy;

                            /*** globaler Sector-Owner-Check ***/
                            for (secy=0; secy<ywd->MapSizeY; secy++) {
                                for (secx=0; secx<ywd->MapSizeX; secx++) {
                                    struct Cell *sec;
                                    sec = &(ywd->CellArea[secy*ywd->MapSizeX + secx]);
                                    // FIXME_FLOH
                                    yw_CheckSector(ywd,sec,secx,secy,255,NULL);
                                };
                            };

                            /*** Wundersteine initialisieren ***/
                            yw_InitWundersteins(o,ywd);

                            /*** BeamGates initialisieren ***/
                            yw_InitGates(ywd);
                            yw_InitSuperItems(ywd);

                            /*** More Energy-Init ***/
                            yw_NewEnergyCycle(ywd);

                            /*** GUI-Modul initialisieren ***/
                            if (yw_InitGUIModule(o,ywd)) {
                                level_ok = TRUE;
                            };
                        };
                    };
                };
            };
        };
    };

    if (!level_ok) {

        yw_NetLog("Unable to init network level (1)\n");
        _methoda(o, YWM_KILLLEVEL, NULL);
        return( FALSE );
        }

    /*** Backup von Owner und TypeMap ***/
    if (ywd->TypeMapBU) {
        _dispose(ywd->TypeMapBU);
        ywd->TypeMapBU = NULL;
    };
    if (ywd->OwnerMapBU) {
        _dispose(ywd->OwnerMapBU);
        ywd->OwnerMapBU = NULL;
    };
    if (ywd->TypeMap) {
        ywd->TypeMapBU = yw_CopyBmpObject(ywd->TypeMap,"copyof_typemap");
    };
    if (ywd->OwnerMap) {
        ywd->OwnerMapBU = yw_CopyBmpObject(ywd->OwnerMap,"copyof_ownermap");
    };

    ywd->gsr->msgcount = 0;

    /*** Special Init Things ***/
    for( i = 0; i < MAXNUM_PLAYERS; i++ ) {

        ywd->gsr->player2[ i ].msg[0]             = 0;
        ywd->gsr->player2[ i ].trouble            = 0;
        ywd->gsr->player2[ i ].waiting_for_update = 0;
        }

    for( i = 0; i < 8; i++ ) {

        ywd->gsr->player[ i ].was_killed    = 0;
        ywd->gsr->player[ i ].ready_to_play = 0;
        ywd->gsr->player[ i ].lastmsgtime   = ywd->TimeStamp + 1000; // kein vorzeitiger Abbruch
        ywd->gsr->player[ i ].timestamp     = 0;
        ywd->gsr->player[ i ].status        = NWS_NOTHERE;
        ywd->gsr->player[ i ].trouble_count = 0;
        }

    /*** Wer ist im Spiel? ***/
    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        /*** Als Ingame nortieren ***/
        ywd->gsr->player[ robo->bact->Owner ].status = NWS_INGAME;

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    /*** Eigenen Robo ermitteln ***/
    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robo->nd.mln_Succ ) {

        if( robo->bact->Owner == ywd->gsr->NPlayerOwner ) {
            found = TRUE;
            break;
            }

        robo = (struct OBNode *) robo->nd.mln_Succ;
        }

    ywd->gsr->player[ ywd->gsr->NPlayerOwner ].ready_to_play = TRUE;
    ywd->netgamestartet    = FALSE;
    ywd->netstarttime      = (LONG) yw_ConfigItems[1].data;
    ywd->gsr->kickoff_time = (LONG) yw_ConfigItems[2].data;

    yw_NetLog("netstarttime was initialized with %d sec\n", ywd->netstarttime/1000);
    yw_NetLog("kickoff was initialized with      %d sec\n", ywd->gsr->kickoff_time/1000);

    /*** SyncMessage schicken ***/
    if( found ) {

        struct ypamessage_syncgame sg;
        struct sendmessage_msg sm;
        struct flushbuffer_msg fb;
        struct OBNode *gun;
        ULONG  k;

        sg.generic.owner      = ywd->gsr->NPlayerOwner;
        sg.generic.message_id = YPAM_SYNCGAME;
        sg.robo_ident         = robo->bact->ident;
 
        /* ------------------------------------------------------------
        ** Kanonen ausfüllen, wahrscheinlich konnte die Synchronisation
        ** des Idents bei mehreren Spielen nicht garantiert werden!
        ** ----------------------------------------------------------*/
        gun = (struct OBNode *) robo->bact->slave_list.mlh_Head;
        k = 0;

        /*** Es sind zu dieser zeit nicht nur RoboFlaks im Spiel ***/
        while( (gun->nd.mln_Succ) && (k<NUMBER_OF_GUNS) ) {

            ULONG rgun = 0L;
            if( BCLID_YPAGUN == gun->bact->BactClassID )
                _get( gun->o, YGA_RoboGun, &rgun );

            if( rgun ) {

                sg.gun[ k ] = gun->bact->ident;
                k++;
                }

            gun = (struct OBNode *) gun->nd.mln_Succ;
            }

        sm.data               = &sg;
        sm.data_size          = sizeof( sg );
        sm.receiver_kind      = MSG_ALL;
        sm.receiver_id        = NULL;
        sm.guaranteed         = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

        fb.sender_kind         = MSG_PLAYER;
        fb.sender_id           = ywd->gsr->NPlayerName;
        fb.receiver_kind       = MSG_ALL;
        fb.receiver_id         = NULL;
        fb.guaranteed          = TRUE;
        _methoda( ywd->nwo, NWM_FLUSHBUFFER, &fb);

        yw_NetLog("Sent a READY TO PLAY to all for my Owner %d\n",
                   ywd->gsr->NPlayerOwner );

        /*** Viewer nochmal setzen, damit Message ankommt ***/
        _set( robo->o, YBA_Viewer, TRUE );
        }

    /*** Statistik init ***/
    ywd->gsr->transfer_sendcount = 0;
    ywd->gsr->transfer_rcvcount  = 0;
    ywd->gsr->transfer_rcvtime   = 0;
    ywd->gsr->transfer_sbps      = 0;
    ywd->gsr->transfer_rbps      = 0;
    ywd->gsr->transfer_rbps_min  = 0;
    ywd->gsr->transfer_rbps_max  = 0;
    ywd->gsr->transfer_rbps_avr  = 0;
    ywd->gsr->transfer_sbps_min  = 0;
    ywd->gsr->transfer_sbps_max  = 0;
    ywd->gsr->transfer_sbps_avr  = 0;
    ywd->gsr->transfer_gcount    = 0;
    ywd->gsr->transfer_pckt      = 0;
    ywd->gsr->transfer_pckt_min  = 0;
    ywd->gsr->transfer_pckt_max  = 0;
    ywd->gsr->transfer_pckt_count= 0;
    ywd->gsr->transfer_pckt_avr  = 0;
    
    /*** erstmal 5 min Zeit geben, um messages reinzulassen ***/
    ywd->gsr->corpse_check       = ywd->TimeStamp + 300000;
    ywd->gsr->sendscore          = 3000;    // nicht gleich initphase zumuellen   

    /*** Weil die netzwerkauswertung ja von 0 anbeginnt ... ***/
    memset( ywd->GlobalStats, 0, sizeof( ywd->GlobalStats ));
    return(level_ok);
}


BOOL yw_NetPlaceRobos( struct ypaworld_data *ywd, struct NLRoboDesc *robos,
                       ULONG num_robos )
{
    /* --------------------------------------------------------------------
    ** Plaziert die Robos. Wir bekommen die Positionen aus den Scripts.
    ** Solange wir die nicht haben, helfen wir uns mit Berechnung. Eine
    ** weitere Annahme ist die tatsache, daß alle Spieler in den jeweiligen
    ** Netzobjekten in der gleichen Reihenfolge vorliegen. Ich erzeuge
    ** zu jedem Player einen Robo. Die Eigentümer, die zu jedem Offset
    ** gehören, werden fest programmiert. Ist der Spieler  ein eigener,
    ** so setze ich bei diesem den Viewer.
    ** ------------------------------------------------------------------*/

    struct getplayerdata_msg gpd;
    ULONG  i;
    struct NLRoboDesc *rdesc;

    ywd->Level->OwnerMask = 0;
    ywd->Level->UserMask  = 0;

    for( i = 0; i < 8; i++ )
        ywd->gsr->player[ i ].ready_to_play = 0;

    gpd.number  = 0;
    gpd.askmode = GPD_ASKNUMBER;

    while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

        struct createvehicle_msg cv;
        struct intersect_msg ins;
        ULONG  energy;
        FLOAT  pos_x, pos_y, pos_z;
        Object *robo;
        UBYTE  owner, race;

        if( gpd.number == num_robos ) {

            yw_NetLog("No Robo declared for player %d\n", num_robos );
            return( FALSE );
            }

        /*** Welche Rasse hat der gpd.number-te Spieler? ***/
        race = 0;
        if( gpd.flags & NWFP_OWNPLAYER )
            race = ywd->gsr->SelRace;
        else
            race = ywd->gsr->player2[ gpd.number ].race;

        if( !race ) {

            yw_NetLog("Error no race for robo %s found\n", gpd.name );
            return( FALSE );
            }

        /* -------------------------------------------------------------
        ** Der Eigentümer wird entsprechend der Rasse gewählt. Die
        ** Rasse ermittle ich aus GSR->race für autonome und aus SelRace
        ** für mich
        ** -----------------------------------------------------------*/
        switch( race ) {

            case FREERACE_USER:         owner = 1; break;
            case FREERACE_KYTERNESER:   owner = 6; break;
            case FREERACE_MYKONIER:     owner = 3; break;
            case FREERACE_TAERKASTEN:   owner = 4; break;
            }

        /* ------------------------------------------------------------
        ** Die Eigentümer sollten über die interne Nummer erreichbar
        ** sein. Hier stehen die rassen fest, hier können wir die Owner
        ** zuweisen
        ** ----------------------------------------------------------*/
        ywd->gsr->player2[ gpd.number ].owner = owner;

        /*** Welche Robo-Bechreibung ist für meinen Owner aktuell? ***/
        rdesc = NULL;
        for( i = 0; i < num_robos; i++ ) {

            if( owner == robos[ i ].owner ) {

                rdesc = &(robos[ i ]);
                break;
                }
            }

        if( !rdesc ) {

            yw_NetLog("Oh! No owner %d specified in LDF, but somebody wanted to play it!\n",
                    owner );
            return( FALSE);
            }


        /*** Position ermitteln ***/
        pos_x = rdesc->pos.x;
        pos_z = rdesc->pos.z;
        pos_y = rdesc->pos.y;

        /*** Playername merken ***/
        strcpy( ywd->gsr->player[ owner ].name, gpd.name );

        /*** weitere Initialisierungen ***/
        energy = rdesc->energy / 4;      // weiterhin vierteln wegen levels

        ins.pnt.x = pos_x;
        ins.pnt.y = -30000.0;
        ins.pnt.z = pos_z;
        ins.vec.x = 0.0;
        ins.vec.y = 50000.0;
        ins.vec.z = 0.0;
        ins.flags = 0;
        _methoda( ywd->world, YWM_INTERSECT, &ins);

        /*** falls Intersection, Y-Start-Pos anpassen ***/
        if (ins.insect)
            pos_y = ins.ipnt.y + pos_y;
        else
            yw_NetLog("Warning: Robo placed without y-correction\n");

        /* --------------------------------------------------------------
        ** Roboerzeugen. Dabei beachten, daß die Visprotos rassenabhängig
        ** sind. Die rasse ermitteln wir für autonome aus GSR->race, an-
        ** sonsten GSR->SelRace
        ** ------------------------------------------------------------*/
        switch( race ) {

            case FREERACE_USER:         cv.vp = 56; break;
            case FREERACE_KYTERNESER:   cv.vp = 57; break;
            case FREERACE_MYKONIER:     cv.vp = 58; break;
            case FREERACE_TAERKASTEN:   cv.vp = 60; break;
            }

        cv.x    = pos_x;
        cv.y    = pos_y;
        cv.z    = pos_z;

        if( robo = (Object *) _methoda( ywd->world, YWM_CREATEVEHICLE, &cv ) ) {

            struct Bacterium *rbact;
            struct OBNode *gun;

            _get( robo, YBA_Bacterium, &rbact );

            rbact->Owner = owner;
            ywd->Level->OwnerMask |= (1L<<owner);

            /*** Id eindeutig machen ***/
            rbact->ident |= ((ULONG)owner) << 24;

            gun = (struct OBNode *) rbact->slave_list.mlh_Head;
            while( gun->nd.mln_Succ ) {

                gun->bact->ident |= ((ULONG)owner) << 24;

                /*** Gun sollte auch den Owner haben ***/
                gun->bact->Owner = owner;

                gun = (struct OBNode *) gun->nd.mln_Succ;
                }

            /*** Viewer? ***/
            if( gpd.flags & NWFP_OWNPLAYER ) {

                _set( robo, YBA_Viewer,    TRUE );
                _set( robo, YBA_UserInput, TRUE );

                ywd->gsr->NPlayerOwner = owner;
                ywd->Level->UserMask |= (1L<<owner);
                }

            /*** Anketten ***/
            _methoda(ywd->world, YWM_ADDCOMMANDER, robo);

            /*** Energie ***/
            _set(robo, YBA_BactCollision, TRUE);
            _set(robo, YRA_FillModus, YRF_Fill_All);
            _set(robo, YRA_BattVehicle, energy);
            _set(robo, YRA_BattBeam, energy);

            /*** neu: Blickrichtung ***/
            _set(robo, YRA_ViewAngle, rdesc->viewangle);

            rbact->Energy  = energy;
            rbact->Maximum = energy;
            
            // FLOH: 22-Apr-98
            if (rdesc->robo_reload_const) rbact->RoboReloadConst = rdesc->robo_reload_const;
            else                          rbact->RoboReloadConst = rbact->Maximum;
            
            
            }
        else {

            yw_NetLog("NetPlaceRobos: Unable to create robo (owner %d, type %d)\n",
                     gpd.number + 1, cv.vp );
            return( FALSE );
            }

        gpd.number++;
        }

    return( TRUE );
}

#endif
