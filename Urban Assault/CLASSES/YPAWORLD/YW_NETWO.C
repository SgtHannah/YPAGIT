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
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "visualstuff/ov_engine.h"
#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypamissileclass.h"
#include "ypa/ypagunclass.h"
#include "bitmap/bitmapclass.h"

#ifdef __NETWORK__

#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#include "thirdparty/eulerangles.h"
#include "yw_netprotos.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

extern UBYTE  **GlobalLocaleHandle;
BOOL   REDUCE_DATA_RATE = FALSE;
ULONG  MSG[ 102 ];

/*** der enormen Groesse wegen nicht aufm stack (was lokale Var. ja sind ***/ 
struct ypamessage_vehicledata_i vdm_i;
struct ypamessage_vehicledata_e vdm_e;

void yw_Organize( struct ypamessage_organize *om, struct Bacterium *robo,
                  struct ypaworld_data *ywd)
{
    struct Bacterium *master, *bact, *slave;

    switch( om->modus ) {

        case ORG_NEWCHIEF:

            bact   = yw_GetBactByID( robo, om->ident );
            master = yw_GetBactByID( robo, om->master );

            if( master && bact ) {

                /*** Mich und meine Slaves an Master übergeben ***/
                bact->CommandID = master->CommandID;

                /*** An neuen Chef hängen ***/
                _methoda( master->BactObject, YBM_ADDSLAVE, bact->BactObject );

                /*** Slave noch umschichten ***/
                while( bact->slave_list.mlh_Head->mln_Succ) {

                    struct OBNode *slave;

                    slave = (struct OBNode *) bact->slave_list.mlh_Head;

                    slave->bact->CommandID  = master->CommandID;

                    /*** Umschichten ***/
                    _methoda( master->BactObject, YBM_ADDSLAVE, slave->o );
                    }
                }
            else {

                if( bact == NULL )
                    yw_NetLog("\n+++ ORG/NCH/1 Havent found vehicle %d (%ds)\n",
                            om->ident ,ywd->TimeStamp/1000 );

                if( master == NULL )
                    yw_NetLog("\n+++ ORG/NCH/2 Havent found vehicle %d (%ds)\n",
                            om->master ,ywd->TimeStamp/1000 );
                }

            break;

        case ORG_ADDSLAVE:

            /* ------------------------------------------------------
            ** slave wird mir übergeben und wenn es master gibt, so
            ** wird dies der neue master aus den slaves des slave. In
            ** CommandID steht die neue CommandID. Auf deutsch, wir
            ** bekommen einen neuen Slave, aus dessen slaves evtl.
            ** ein neues Commando gemacht werden muß.
            ** ----------------------------------------------------*/
            bact  = yw_GetBactByID( robo, om->ident );
            slave = yw_GetBactByID( robo, om->slave );

            if( slave && bact ) {

                if( om->master ) {

                    if( master = yw_GetBactByID( robo, om->master ) ) {

                        /* ---------------------------------------------
                        ** Umschichten. master wird von slave abgelöst
                        ** und auf Commanderebene gebracht. Dann bekommt
                        ** master den Rest von slave und om->command_id.
                        ** -------------------------------------------*/
                        _methoda( robo->BactObject, YBM_ADDSLAVE, master->BactObject);

                        master->CommandID = om->command_id;

                        while( slave->slave_list.mlh_Head->mln_Succ ) {

                            struct OBNode *n = (struct OBNode *)slave->slave_list.mlh_Head;
                            _methoda( master->BactObject, YBM_ADDSLAVE, n->o);
                            n->bact->CommandID = om->command_id;
                            }
                        }
                    else
                        yw_NetLog("\n+++ ORG/AS/3 Havent found master %d (%ds)\n",
                                 om->master ,ywd->TimeStamp/1000 );
                    }

                /*** Slave noch mir unterordnen ***/
                _methoda( bact->BactObject, YBM_ADDSLAVE, slave->BactObject);
                slave->CommandID = bact->CommandID;
                }
            else {

                if( bact == NULL )
                    yw_NetLog("\n+++ ORG/AS/1 Havent found vehicle %d (%ds)\n",
                             om->ident ,ywd->TimeStamp/1000 );

                if( slave == NULL )
                    yw_NetLog("\n+++ ORG/AS/2 Havent found slave-vehicle %d (%ds)\n",
                             om->slave ,ywd->TimeStamp/1000 );
                }

            break;

        case ORG_NEWCOMMAND:

            bact   = yw_GetBactByID( robo, om->ident );

            if( bact ) {

                if( om->master ) {
                    /* --------------------------------------------------
                    ** Ich löse mich aus meinem Geschwader und wenn
                    ** master ausgefüllt ist, wird das der neue commander
                    ** ------------------------------------------------*/

                    master = yw_GetBactByID( robo, om->master );
                    if( master ) {

                        while( bact->slave_list.mlh_Head->mln_Succ) {

                            struct OBNode *slave;

                            slave = (struct OBNode *) bact->slave_list.mlh_Head;

                            slave->bact->CommandID  = master->CommandID;

                            /*** Umschichten ***/
                            _methoda( master->BactObject, YBM_ADDSLAVE, slave->o );
                            }

                        /*** master ankoppeln ***/
                        _methoda( robo->BactObject, YBM_ADDSLAVE, master->BactObject );
                        }
                    else
                        yw_NetLog("\n+++ ORG/NCO/1 Havent found vehicle %d (%ds)\n",
                                 om->master ,ywd->TimeStamp/1000 );
                    }

                /*** Ich bleibe auf der Ebene, bekpomme aber neue ID ***/
                bact->CommandID = om->command_id;

                /* ------------------------------------------------
                ** Egal, ob ich Slave bin oder ein Commander war, der seine
                ** Slaves abgegeben hat, ich ordne mich erneut dem Robo
                ** unter und werde damit Commander.
                ** ----------------------------------------------*/
                _methoda( robo->BactObject, YBM_ADDSLAVE, bact->BactObject);
                bact->CommandID = om->command_id;
                }
            else
                yw_NetLog("\n+++ ORG/NCO/2: Havent found vehicle %d (%ds)\n",
                         om->master ,ywd->TimeStamp/1000 );
            break;
        }
}


void yw_RemoveAllShadows( struct ypaworld_data *ywd, struct OBNode *robo )
{

    while( robo->bact->slave_list.mlh_Head->mln_Succ ) {

        struct OBNode *com, *slave;

        com = (struct OBNode *) robo->bact->slave_list.mlh_Head;

        while( com->bact->slave_list.mlh_Head->mln_Succ ) {

            slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

            /*** Waffen freigeben ***/
            yw_ReleaseWeapon( ywd, slave->bact );

            /*** Tschüss zu de Angreifers ***/
            yw_RemoveAttacker( slave->bact );
                    
            /*** als Bordflak abmelden ***/
            yw_DisConnectFromRobo( ywd, slave );
            
            /*** Als tot markieren, weil dispose auch DIE verwendet ***/
            slave->bact->ExtraState |= EXTRA_LOGICDEATH;

            /*** Freigeben ***/
            slave->bact->MainState = ACTION_DEAD;
            _methoda( ywd->world, YWM_RELEASEVEHICLE, slave->o );
            }


        /*** Waffen freigeben ***/
        yw_ReleaseWeapon( ywd, com->bact );

        /*** Von Angreifern abmelden ***/
        yw_RemoveAttacker( com->bact );

        /*** als Bordflak abmelden ***/
        yw_DisConnectFromRobo( ywd, com );
            
        /*** Als tot markieren, weil dispose auch DIE verwendet ***/
        com->bact->ExtraState |= EXTRA_LOGICDEATH;

        /*** Wech dat Zeugs ***/
        com->bact->MainState = ACTION_DEAD;
        _methoda( ywd->world, YWM_RELEASEVEHICLE, com->o );
        } 
        
    /* ---------------------------------------------------
    ** Wie auch immer, deine Guns sind weg! Zur Sicherheit
    ** GunArray aufraeumen.
    ** -------------------------------------------------*/
    if( BCLID_YPAROBO == robo->bact->BactClassID ) { 
    
        struct gun_data *g_array;
        int    i;

        _get( robo->o, YRA_GunArray, &g_array );

        for( i = 0; i < NUMBER_OF_GUNS; i++ )
            g_array[ i ].go = NULL;
        }

    /***  ***/
    yw_RemoveAttacker( robo->bact );
    yw_ReleaseWeapon( ywd, robo->bact );
    robo->bact->MainState = ACTION_DEAD;
    _methoda( ywd->world, YWM_RELEASEVEHICLE, robo->o );
    
    /*** Als tot markieren, weil dispose auch DIE verwendet ***/
    robo->bact->ExtraState |= EXTRA_LOGICDEATH;
}


void yw_RemovePlayerStuff( struct ypaworld_data *ywd, char *name,
                           UBYTE OWNER, UBYTE mode )
{
    BOOL   found;
    UBYTE  owner;
    int    i;
    struct OBNode *robo = NULL;

    /* ----------------------------------------------------------
    ** Mode == 1 heißt, der übergebene Owner ist gültig, 0 heißt,
    ** den Namen interpretieren und danach den Owner suchen
    ** --------------------------------------------------------*/

    if( mode ) {

        owner = OWNER;
        }
    else {

        /*** Aus seinem Namen Owner ermitteln ***/
        found = FALSE;
        for( i = 0; i < 8; i++ ) {
        
            /*** Er sollte natuerlich noch nicht tot sein... ***/
            if( (stricmp( ywd->gsr->player[ i ].name, name ) == 0) &&
                (ywd->gsr->player[ i ].was_killed == 0) ){
                owner = (UBYTE) i;
                found = TRUE;
                break;
                }
            }

        if( !found ) {

            yw_NetLog("destroy player: robo of player %s not found!\n", name);
            yw_NetLog("In cases of trouble with other players this is no problem\n");
            return;
            }
        }

    found = FALSE;
    robo = yw_GetRoboByOwner( ywd, owner );

    if( !robo ) return;

    /*** Alle Schatten-Objeke freigeben ***/
    yw_RemoveAllShadows( ywd, robo );

    /*** Diesen Player für das Spiel (nicht bloß inner Engine) abmelden ***/
    //ywd->gsr->player[ owner ].name[0]    = 0;

    ywd->gsr->player[ owner ].was_killed = WASKILLED_NORMAL | WASKILLED_SHOWIT;
}



BOOL yw_PlayersInGame( struct ypaworld_data *ywd )
{
    /*** Testet, ob die Spieler schon alle da sind ***/

    ULONG numplayers, np, count, cnt;
    char *waiting_array[ 8 ];
    struct rast_text rt;
    char *str, str_buf[ 1024 ], buffer[ 100 ];
    struct VFMInput Ip;

    numplayers = _methoda( ywd->nwo, NWM_GETNUMPLAYERS, NULL );

    count = 0;
    np    = 7;

    /*** Gucken, was los ist ***/
    if( ywd->netgamestartet ) return( TRUE );   // denn dann darf der test net sein

    while( np ) {

        /*** player[ 0 ] gibts ja nich ***/

        /*** Gibt es den Player? ***/
        if( ywd->gsr->player[ np ].name[ 0 ] ) {

            /*** Ist er schon da? ***/
            if( ywd->gsr->player[ np ].ready_to_play ) {
                count++;
                waiting_array[ np ] = NULL;
                }
            else
                waiting_array[ np ] = ywd->gsr->player[ np ].name;
            }
        else waiting_array[ np ] = NULL;

        np--;
        }

    /*** Alles ok ***/
    if( count == numplayers ) {

        ywd->netgamestartet = TRUE;
        return( TRUE );
        }

    #ifndef AMIGA
    delay( 50 );
    #endif
    memset( &Ip, 0, sizeof( Ip ) );
    _GetInput( &Ip );

    /*** Message mit den Problemen ***/
    str = str_buf;
    new_font( str, FONTID_DEFAULT );
    pos_brel(  str, 0, 0 );
    sprintf( buffer, ypa_GetStr( GlobalLocaleHandle, STR_NET_WAITING,
                                 "WAITING FOR PLAYERS: ") );
    #ifdef __DBCS__ 
    freeze_dbcs_pos( str );     
    put_dbcs( str, 2*ywd->DspXRes/3-1, DBCSF_LEFTALIGN, buffer ); 
    #else                                 
    str = yw_StpCpy( buffer, str );
    #endif
    
    new_line( str );

    /*** Zeit runterzählen ***/
    ywd->netstarttime -= Ip.FrameTime;
    if( ywd->netstarttime > 0 ) {

        /*** Zähler ausgeben ***/
        sprintf( buffer, "(%d)", ywd->netstarttime/1000 );
        
        #ifdef __DBCS__
        freeze_dbcs_pos( str );
        put_dbcs( str, ywd->DspXRes/3-1, DBCSF_LEFTALIGN, buffer ); 
        #else
        str = yw_StpCpy( buffer, str );
        #endif
        new_line( str );
        }

    /*** Nun Schleife der einzelnen Spieler ***/
    cnt = 1;
    while( cnt < 8 ) {

        if( waiting_array[ cnt ] ) {

            if( (ywd->netstarttime <= 0) && ywd->gsr->is_host ) {

                struct ypamessage_removeplayer rp;
                struct sendmessage_msg sm;

                /*** !!! Name vor dem Löschen merken !!! ***/
                strcpy( rp.name, waiting_array[ cnt ] );

                /*** erst Dreck wegräumen... ***/
                yw_RemovePlayerStuff( ywd, waiting_array[ cnt ], 0, 0 );

                /*** ... dann rausschmeißen, den Arsch. ***/
                rp.generic.owner      = (UBYTE) cnt;
                rp.generic.message_id = YPAM_REMOVEPLAYER;

                /* -----------------------------------------------------------
                ** Die Message bekommt jeder! Weil der, der rausgeschmissen
                ** wird, meist abgepfiffen ist und somit nicht antworten kann.
                ** Ich muß den aber auch bei mir noch killen!
                ** ---------------------------------------------------------*/
                sm.data          = &rp;
                sm.data_size     = sizeof( rp );
                sm.receiver_kind = MSG_ALL;
                sm.receiver_id   = NULL;
                sm.guaranteed    = TRUE;
                _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

                yw_DestroyPlayer( ywd, rp.name );
                
                /*** Als fehler kennzeichnen, wenn ich nicht schon raus bin! ***/
                if( NETWORKTROUBLE_KICKOFF_YOU != ywd->gsr->network_trouble ) {
                  
                    strcpy( ywd->gsr->network_trouble_name, rp.name );
                    ywd->gsr->network_trouble       = NETWORKTROUBLE_KICKOFF_PLAYER;
                    ywd->gsr->network_trouble_count = KICKOFF_PLAYER_COUNT;
                    }

                yw_NetLog(">>> I have kicked off %s because I didn't heard anything after loading (time %d)\n",
                           rp.name, ywd->TimeStamp/1000 );
                yw_NetLog("    netstarttime was %d\n", ywd->netstarttime );
                }
            else {

                /*** Sagen, auf wen wir warten ***/
                strcpy( buffer, "     ");
                yw_StrUpper2( &( buffer[3] ), waiting_array[ cnt ] );
                
                #ifdef __DBCS__
                freeze_dbcs_pos( str );
                put_dbcs( str, ywd->DspXRes-1, DBCSF_LEFTALIGN, buffer ); 
                #else
                str = yw_StpCpy( buffer, str );
                #endif
                new_line( str );
                }
            }

        cnt++;
        }

    /*** Raus, wenn eh der rest gefeuert wurde ***/
    if( ywd->netstarttime <= 0 ) {
        if( ywd->gsr->is_host )
            return( TRUE );
        else {

            sprintf( buffer, "WAITING FOR HOST" );
            
            #ifdef __DBCS__
            freeze_dbcs_pos( str );
            put_dbcs( str, ywd->DspXRes-1, DBCSF_LEFTALIGN, buffer ); 
            #else
            str = yw_StpCpy( buffer, str );
            #endif
            
            new_line( str );
            }
        }

    eos( str );

    _methoda( ywd->GfxObject, DISPM_Begin, NULL );
    _methoda( ywd->GfxObject, RASTM_Begin2D, NULL );

    rt.string = str_buf;
    rt.clips = NULL;
    _methoda( ywd->GfxObject, RASTM_Text, &rt );

    _methoda( ywd->GfxObject, RASTM_End2D, NULL );
    _methoda( ywd->GfxObject, DISPM_End, NULL );

    return( FALSE );
}


BOOL yw_PlayersOK( struct ypaworld_data *ywd)
{
    struct getplayerdata_msg gpd;
    int i;
    BOOL problems = FALSE;

    /* -------------------------------------------------------------
    ** Ich teste in einer Schleife alle Spieler, deren Namen ich von
    ** der Engine bekomme, wann die letzte Message kam. Die Zeiten
    ** stehen in der Playerdata. 
    ** Weil ich hier keine Tastaturabfragen machen kann, muß ich die
    ** Daten nach außen geben, wo ein Requester bearbeitet werden
    ** kann. Dann kann ich aber das Spiel nicht anhalten...
    ** Ich gehe immer mit TRUE raus, damit die GlobalTime aktuell
    ** ist !!!
    ** -----------------------------------------------------------*/
    gpd.number  = 0;
    gpd.askmode = GPD_ASKNUMBER;
    while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

        struct playerdata *pd;
        UBYTE  owner;


        /*** Eigener Player natürlich nicht ***/
        if( gpd.flags & NWFP_OWNPLAYER ) { gpd.number++; continue; }

        pd = NULL;
        for( i = 0; i < 8; i++ ) {

            if( stricmp( ywd->gsr->player[ i ].name, gpd.name ) == 0 ) {
                pd    = &(ywd->gsr->player[ i ]);
                owner = (UBYTE) i;
                break;
                }
            }

        if( pd ) {

            /*** Vorsichtshalber erstmal "OK" ***/
            if( pd->trouble_count > 0 ) {
                pd->trouble_count -= ywd->TLMsg.frame_time;
                if( pd->trouble_count <= 0 )
                    pd->status = NWS_INGAME;
                }

            /*** Nun mal gucken ***/
            if( ywd->netgamestartet &&
                (ywd->TLMsg.global_time - pd->lastmsgtime > MAX_LATENCY) ) {

                /*** Datenrate reduzieren, um anderen nicht zuzumüllen ***/
                REDUCE_DATA_RATE = TRUE;

                /*** Nur, wenn neue Sekunde angefangen hat ***/
                if( ((ywd->TLMsg.global_time - pd->lastmsgtime)/1000) !=
                    ((ywd->TLMsg.global_time - pd->lastmsgtime - ywd->TLMsg.frame_time)/1000) ) {

                    yw_NetLog("Waiting for player %s. (time %ds at %ds)\n",
                             gpd.name,
                             (ywd->TLMsg.global_time - pd->lastmsgtime)/1000, ywd->TimeStamp/1000);
                    yw_NetLog("    Reduce data transfer rate\n");
                    }

                /*** Fuer Oberfläche ein Zeichen setzen ***/
                ywd->gsr->trouble_count = 4000;

                pd->status        = NWS_TROUBLE;
                pd->trouble_count = 5000;

                /*** Ist es vielleicht schlimmer, als ich dachte? ***/
                if( (ywd->TLMsg.global_time - pd->lastmsgtime > ywd->gsr->kickoff_time) &&
                    (ywd->gsr->is_host) ) {

                    /* ---------------------------------------------
                    ** Ich sollte als Host von meinem Recht Gebrauch
                    ** machen, den Spieler rauszuschmeißen
                    ** -------------------------------------------*/
                    struct ypamessage_removeplayer rp;
                    struct sendmessage_msg sm;
                    char   text[ 200 ];
                    struct logmsg_msg log;

                    /*** !!! Name vor dem Löschen merken !!! ***/
                    strcpy( rp.name, pd->name );

                    /*** erst Dreck wegräumen... ***/
                    yw_RemovePlayerStuff( ywd, pd->name, 0, 0 );

                    /*** ... dann rausschmeißen, den Arsch. ***/
                    rp.generic.owner      = owner;
                    rp.generic.message_id = YPAM_REMOVEPLAYER;
                    
                    /*** nur wenn ich nicht schon rausgeschmissen wurde ***/
                    if( NETWORKTROUBLE_KICKOFF_YOU != ywd->gsr->network_trouble ) {
                    
                        strcpy( ywd->gsr->network_trouble_name, rp.name );
                        ywd->gsr->network_trouble       = NETWORKTROUBLE_KICKOFF_PLAYER;
                        ywd->gsr->network_trouble_count = KICKOFF_PLAYER_COUNT;
                        }

                    /* -------------------------------------------------
                    ** Ich schicke an den Player ein REMOVE und dessen
                    ** DESTROY verschickt dann automatisch Messages. Das
                    ** ist die Theorie. Der andere ist aber meist abge-
                    ** pfiffen und antwortet nicht. Deshalb schicke ich
                    ** an alle!!!
                    ** -----------------------------------------------*/
                    sm.data          = &rp;
                    sm.data_size     = sizeof( rp );
                    sm.receiver_kind = MSG_ALL;
                    sm.receiver_id   = NULL;
                    sm.guaranteed    = TRUE;
                    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

                    /* ----------------------------------------------
                    ** Auch bei mir killen, denn ob der Getötete ant-
                    ** wortet, ist unklar...
                    ** --------------------------------------------*/
                    yw_DestroyPlayer( ywd, rp.name );

                    yw_NetLog(">>> I have kicked off %s because I haven't heard anything for %d sec (at time %d)\n",
                               rp.name, ywd->gsr->kickoff_time / 1000, ywd->TimeStamp/1000 );
                    problems = TRUE;
                    }

                /*** Nicht Host... ***/
                if( (ywd->TLMsg.global_time - pd->lastmsgtime > WAITINGFORPLAYER_TIME) &&
                    (!ywd->gsr->is_host) ) {

                    /* -------------------------------------------------------
                    ** Wenn ich nicht Host bin, sollte ich zumindest eine
                    ** Meldung bringen, daß es probleme bringt.
                    ** Namen und Zeiten koennen aus dem Array genommen werden.
                    ** Wenn natuerlich gerade ein Rausschmissverfahren laeuft,
                    ** dann nicht! 
                    ** -----------------------------------------------------*/
                    if( (NETWORKTROUBLE_KICKOFF_PLAYER != ywd->gsr->network_trouble) &&
                        (NETWORKTROUBLE_KICKOFF_YOU    != ywd->gsr->network_trouble) ) {
                         
                        ywd->gsr->network_trouble       = NETWORKTROUBLE_WAITINGFORPLAYER;
                        ywd->gsr->network_trouble_count = 10; // nicht runterzaehlen
                        problems = TRUE;
                        }
                    }

                pd->no_answer = 1;
                }
            else {
            
                pd->no_answer = 0;
                }
            }
        else yw_NetLog("Warning: No Playerdata for player %s in PlayersOK() (%ds)\n",
                      gpd.name, ywd->TimeStamp / 1000 );

        gpd.number++;
        }
        
    if( (ywd->gsr->network_trouble == NETWORKTROUBLE_WAITINGFORPLAYER) &&
        (FALSE == problems) )
        ywd->gsr->network_trouble = NETWORKTROUBLE_NONE;

    /*** vorerst... ***/
    return( TRUE );
}


void yw_CheckLatency( struct ypaworld_data *ywd )
{
    /* --------------------------------------------------------------
    ** testet Latencies. Dazu versenden wir zu gewissen Zeiten
    ** Messages mit einem TimesStamp, die beantwortet werden muessen.
    ** Dann kann ich die Zeitdifferenz ermitteln.
    ** Ist diese Zeit zu gross, bedeutet das, dass wir evtl. neben
    ** den ueblichen Verzoegerunegn datenstaus haben, die darauf
    ** zurueckzufuehren sind, dass mehr Messages an den provider
    ** gehen als durch die Leitung passen. Dann sollte das Spiel
    ** gestoppt werden, um all die daten abzuarbeiten.
    ** ------------------------------------------------------------*/
    BOOL ok_for_test;
    
    if( (NETWORKTROUBLE_NONE    == ywd->gsr->network_trouble) ||
        (NETWORKTROUBLE_LATENCY == ywd->gsr->network_trouble))
        ok_for_test = TRUE;
    else
        ok_for_test = FALSE;
    
    if( (ywd->gsr->latency_check <= 0) &&
        (ywd->gsr->is_host) && (ok_for_test)) {
    
        struct ypamessage_requestlatency rl;
        struct sendmessage_msg sm;
    
        /* -------------------------------------------------
        ** Wenn schon probleme, dann ist die Frametime auf 1
        ** hartgecodet. Somit muss die Zeit natuerlich
        ** kliener sein
        ** -----------------------------------------------*/
        if( NETWORKTROUBLE_LATENCY == ywd->gsr->network_trouble )
            ywd->gsr->latency_check = 5;
        else
            ywd->gsr->latency_check = TIME_CHECK_LATENCY;
        
        /* ---------------------------------------------------
        ** nun Message losschicken. alles weitere beim Empfang
        ** der Rueckantwort in der Message Abarbeitung.
        ** Die Message geht an alle, waehrend die antworten
        ** nur an mich gehen.
        ** -------------------------------------------------*/
        rl.generic.owner      = ywd->gsr->NPlayerOwner; // egal...
        rl.generic.message_id = YPAM_REQUESTLATENCY;
        rl.time_stamp         = ywd->TimeStamp;

        sm.data          = &rl;
        sm.data_size     = sizeof( rl );
        sm.receiver_kind = MSG_ALL;
        sm.receiver_id   = NULL;
        sm.guaranteed    = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
       
        if( 3 > ywd->TLMsg.frame_time ) {
            
            struct flushbuffer_msg fb;

            /*** Wegen ausgebremster Frametime flushen ***/
            fb.sender_kind   = MSG_PLAYER;
            fb.sender_id     = ywd->gsr->NPlayerName;
            fb.receiver_kind = MSG_ALL;
            fb.receiver_id   = NULL;
            fb.guaranteed    = FALSE;   // weil ingame-zeug, keine Initdaten
            _methoda( ywd->nwo, NWM_FLUSHBUFFER, &fb );
            }
        }
    else 
        ywd->gsr->latency_check -= ywd->TLMsg.frame_time; 
}


void yw_HandleNetworkTrouble( struct ypaworld_data *ywd )
{
    /* --------------------------------------------------------------
    ** Diese Routine bearbeitet den Aerger. Dazu macht sie folgendes:
    **
    ** Sie untersucht, ob das Problem noch besteht,
    **                 zaehlt eine Zeit runter, um zu sehen, ob das
    **                 ueberhaupt loesbar ist und
    **                 arbeitet alle Messages an.
    **
    ** um sie auch fuer andere Sachen zu verwenden, ist sie
    ** fehlerart-sensitiv.
    ** ------------------------------------------------------------*/
    int i;
        
    switch( ywd->gsr->network_trouble ) {
    
        case NETWORKTROUBLE_LATENCY:
        
            /* -------------------------------------------------------------------
            ** Frametime auf 1 setzen (ist ja vor Trigger). Dabei setzen wir
            ** ein Flag fuer ypa.c, weil sich die Frametime verteilt. Ausser-
            ** dem habe ich bei ft = 1 keine aussage mehr ueber messagelaufzeiten.
            ** deshalb warten wir immer eine Mindestzeit und beenden mit
            ** ENDTROUBLE_UNKNOWN, welches keine Message ausgibt.
            ** -----------------------------------------------------------------*/
            
            ywd->gsr->force_frametime_1 = TRUE;
            
            /*** Weil FrameTime 1 ist trouble_count ein Zaehler ***/
            ywd->gsr->network_trouble_count--;
            
            if( ywd->gsr->network_trouble_count <=0) {
            
                int i;
            
                ywd->gsr->network_trouble     = NETWORKTROUBLE_NONE;
                ywd->gsr->network_allok       = ENDTROUBLE_UNKNOWN;
                ywd->gsr->network_allok_count = 0;
                
                for( i = 0; i < MAXNUM_PLAYERS; i++ )
                    ywd->gsr->player[ i ].latency = 0; 
                
                if( ywd->gsr->is_host ) {
           
                    struct ypamessage_endtrouble et;
                    struct sendmessage_msg sm;
                                    
                    et.generic.owner      = ywd->gsr->NPlayerOwner;
                    et.generic.message_id = YPAM_ENDTROUBLE;
                    et.reason             = ENDTROUBLE_UNKNOWN;
                    sm.data               = &et;
                    sm.data_size          = sizeof( et );
                    sm.receiver_kind      = MSG_ALL;
                    sm.receiver_id        = NULL;
                    sm.guaranteed         = TRUE;
                    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                    }
                }
                                       
            break;
            
        case NETWORKTROUBLE_KICKOFF_YOU:
        
            /*** Die Anzeige erfolgt in der NetzStatusanzeige ***/
            ywd->gsr->network_trouble_count -= ywd->TLMsg.frame_time;            
            
            /*** Schon fertig? ***/
            if( ywd->gsr->network_trouble_count < 0 ) {
            
                /*** ganz normaler Abbruch ***/
                //yw_RemovePlayerStuff( ywd, NULL, ywd->gsr->network_trouble_owner, 1 );
                //yw_DestroyPlayer( ywd, ywd->gsr->network_trouble_name );
                ywd->Level->Status = LEVELSTAT_ABORTED;
                ywd->gsr->network_trouble = NETWORKTROUBLE_NONE;
                }
            break;
            
            
        case NETWORKTROUBLE_KICKOFF_PLAYER:
        
            /*** nur runterzaehlen... ***/
            ywd->gsr->network_trouble_count -= ywd->TLMsg.frame_time;            
            if( ywd->gsr->network_trouble_count < 0 ) {
            
                ywd->gsr->network_trouble = NETWORKTROUBLE_NONE;
            
                /* ------------------------------------------------------------
                ** Es wurden die angezeigt, deren Flag WASKILLED_SHOWIT gesetzt
                ** ist. Die sollen beim naechsten problem natuerlich nicht
                ** gezeigt werden. Deshalb Flag ruecksetzen.
                ** ----------------------------------------------------------*/
                for( i = 0; i < MAXNUM_OWNERS; i++ )
                    ywd->gsr->player[ i ].was_killed &= (UBYTE)(~WASKILLED_SHOWIT);
                }    
            break;
            
        case NETWORKTROUBLE_WAITINGFORPLAYER:
        
            /*** nicht runterzaehlen, denn das problem bleibt ja ***/
            break;
        }
}

void yw_NetMessageLoop( struct ypaworld_data *ywd )
{
    /* -------------------------------------------------------------------
    ** Hier rufen wir die Schleife zur Messagebehandlung auf und gehen
    ** wieder raus, sofern alles ok war. War nix ok, bleiben wir in dieser
    ** Schleife, weil alles wieder gut werden kann, wozu wir Messages
    ** brauchen...
    ** -----------------------------------------------------------------*/

    while( 1 ) {
    
        /*** Troublebearbeitung. Kann laenger als ein frame dauern ***/
        if( NETWORKTROUBLE_NONE != ywd->gsr->network_trouble ) 
            yw_HandleNetworkTrouble( ywd ); 
        ywd->gsr->network_allok_count -= ywd->TLMsg.frame_time;     
          
        /*** Messageaufruf ***/
        yw_HandleNetMessages( ywd );

        if( ywd->Level->Status == LEVELSTAT_ABORTED )
            return;

        /* ------------------------------------------------------------
        ** PlayersOK darf diese Variable überschreiben... Kann dann in
        ** sim.c und HandleNetMessages ausgewertet werden, weil es erst
        ** hier wieder rückgesetzt wird.
        ** ----------------------------------------------------------*/
        REDUCE_DATA_RATE = FALSE;

        /*** Tests ***/
        if( !yw_PlayersOK( ywd ) ) {

            }
        else {

            if( !yw_PlayersInGame( ywd ) ) {

                }
            else break;
            }
        }
        
    /*** Latency und damit Datenstaus austesten ***/
    yw_CheckLatency( ywd );
        
    /*** Troublecount runterzaehlen ***/
    if( ywd->gsr->trouble_count > 0 )
        ywd->gsr->trouble_count -= ywd->TLMsg.frame_time;

    /*** Getinput? ***/
}



void yw_HandleNetMessages( struct ypaworld_data *ywd )
{
    /* -----------------------------------------------------------
    ** Bearbeitet alle Messages bzw. empfängt sie erst noch. Weil
    ** wir das Interface zur Welt gering halten müssen, machen wir
    ** hier auch das updaten der Objekte.
    ** ---------------------------------------------------------*/

    struct GameShellReq *GSR;
    struct receivemessage_msg rm;
    ULONG  msg_count, update_time;

    GSR = ywd->gsr;
    //_SetLogMode( LOGMODE_IMMEDIATE );

    /* ---------------------------------------------------------------
    ** update time "problemabhaengig"
    ** -------------------------------------------------------------*/
    if( REDUCE_DATA_RATE )
        update_time = UPDATE_TIME_TROUBLE;
    else
        update_time = GSR->update_time_normal;

    if( ((ywd->TimeStamp - ywd->update_time) > update_time ) &&
        (NETWORKTROUBLE_LATENCY != GSR->network_trouble) ) {

        struct sendmessage_msg sm;
        struct ypamessage_vehicledata_i *vdm;

        if( ywd->interpolate ) {
            
            vdm_i.generic.message_id = YPAM_VEHICLEDATA_I;
            vdm = &vdm_i;
            }
        else {
            
            vdm_e.generic.message_id = YPAM_VEHICLEDATA_E;
            vdm = (struct ypamessage_vehicledata_i *) (&vdm_e);
            }

        vdm->generic.timestamp  = ywd->TimeStamp;

        /*** Daten ermitteln ***/
        if( yw_CollectVehicleData( ywd, vdm ) ) {

            /*** Abschicken ***/
            sm.data              = vdm;

            /*** Wir verschicken nur den Header und vdm.number * vehicledata ***/
            if( ywd->interpolate )
                vdm->head.size   = sizeof( struct vehicledata_i ) * vdm->head.number +
                                   sizeof( struct ypamessage_generic) +
                                   sizeof( struct vehicledata_header );
            else
                vdm->head.size   = sizeof( struct vehicledata_e ) * vdm->head.number +
                                   sizeof( struct ypamessage_generic) +
                                   sizeof( struct vehicledata_header );

            /* -----------------------------------------------------------
            ** bes. für Interpolation Zeit zwischen 2 Updates
            ** (ist von meiner Seite exakter, weil Messagelaufzeiten nicht
            ** beachtet werden)
            ** ---------------------------------------------------------*/
            vdm->head.diff_sendtime = ywd->TimeStamp - ywd->update_time;
            sm.data_size            = vdm->head.size;
            sm.receiver_kind        = MSG_ALL;
            sm.receiver_id          = NULL;
            sm.guaranteed           = FALSE;
            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
            }

        /*** Zeit aktualisieren ***/
        ywd->update_time = ywd->TimeStamp;
        }

    msg_count = 0;

    /*** Message holen. Is eben Polling... ***/
    while( _methoda( ywd->nwo, NWM_RECEIVEMESSAGE, &rm ) ) {

        ULONG  offset, kind, pl_num, inner_msg, i;
        struct sendmessage_msg sm;
        struct ypamessage_welcome wm;
        char   text[ 200 ], text2[ 100 ], error_sender[ 200 ];
        struct logmsg_msg log;
        BOOL   player_found;
        char  *e = NULL;

        ywd->gsr->transfer_rcvcount += rm.size;
        msg_count++;

        /* -----------------------------------------------------------
        ** Wenn wir eigentlich schon raus sein müßten, nehmen wir noch
        ** die messages vom port, weil an alle adressierte messages
        ** anscheinend nicht bei DESTROYPLAYER weggenommen werden.
        ** Aber nur im Spiel!
        ** ---------------------------------------------------------*/
        if( (ywd->Level->Status == LEVELSTAT_ABORTED) &&
            (ywd->netgamestartet) ) continue;

        /*** Kehrt hier langsam Chaos ein? ***/
        pl_num = _methoda( ywd->nwo, NWM_GETNUMPLAYERS, NULL );
        if( msg_count > pl_num * 5 ) {

            ywd->infooverkill = TRUE;

            /*** Troubleshooting durch weniger Messages ***/
            yw_NetLog("Info overkill !!! (msg-count %d at %ds)\n",
                     msg_count, ywd->TimeStamp/1000);
            ywd->gsr->trouble_count = 4000;
            }
        else
            ywd->infooverkill = FALSE;

        /*** Name reinkpoieren, weil Message freigegeben wird! ***/
        error_sender[ 0 ] = 0;
        switch( rm.message_kind ) {

            case MK_CREATEPLAYER: 
                            
                /*** An diesen (!) eine Welcome-Message schicken ***/
                wm.generic.owner      = 0; // egal...
                wm.generic.message_id = YPAM_WELCOME;
                wm.myrace             = ywd->gsr->SelRace;
                wm.cd                 = ywd->gsr->cd;      
                    
                /* ---------------------------------------------
                ** den Ready-To-Start-Status brauchen alle wegen
                ** den Zeichen im Menue
                ** -------------------------------------------*/
                wm.ready_to_start     = GSR->ReadyToStart;
    
                sm.data          = &wm;
                sm.data_size     = sizeof( wm );
                sm.receiver_kind = MSG_PLAYER;
                sm.receiver_id   = (char *)(rm.data);
                sm.guaranteed    = TRUE;
                _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                 
                if(ywd->gsr->NLevelOffset)    
                    yw_SendCheckSum( ywd, ywd->gsr->NLevelOffset );

                /* -----------------------------------------------
                ** Bin ich Host in einem Lobbyspiel und steht die
                ** Levelnummer schon fest, dann hat dieser Spieler
                ** meine vorherige Message nicht erhalten. Also
                ** verschicken wir die Message, aber erst, wenn
                ** der Host im Messagemode ist, die Levelauswahl
                ** also auch abgesegnet wurde!
                ** ---------------------------------------------*/
                if( (GSR->is_host) && (GSR->remotestart) &&
                    (GSR->n_selmode == NM_MESSAGE) &&    
                    (GSR->NLevelOffset > 0) && 
                    (GSR->NLevelOffset < MAXNUM_LEVELS) ) { 
                    
                    struct ypamessage_lobbyinit li;
                                      
                    strncpy( li.hostname, GSR->NPlayerName, STANDARD_NAMELEN );
                    li.levelnum           = GSR->NLevelOffset;
                    li.generic.message_id = YPAM_LOBBYINIT;
                    li.generic.owner      = 0; // weil nicht feststehend

                    sm.data                = (char *) &li;
                    sm.data_size           = sizeof( li );
                    sm.receiver_kind       = MSG_ALL;
                    sm.receiver_id         = NULL;
                    sm.guaranteed          = TRUE;
                    _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);
                    }


                /*** Name in player2 merken ***/
                _get( ywd->nwo, NWA_NumPlayers, &i );
                yw_StrUpper( GSR->player2[ i-1 ].name, (char *)(rm.data));

                /* ----------------------------------------------------
                ** von neuen bekomme ich keine Welcome-Msg, also selbst
                ** setzen
                ** --------------------------------------------------*/
                GSR->player2[ i-1 ].welcomed = 1;
                break;

            case MK_DESTROYPLAYER:

                /* -------------------------------------------------------
                ** Ist sozusagen "harte" Verabschiedung. Ich räume die
                ** Reste auf. Wenn der zu killende Spieler mein eigener
                ** war (kann ja sein, der Host hat mich rausgeschmis-
                ** sen), dann verlasse ich das Spiel. Das geht nicht
                ** mit GETPLAYERDATA, weil ich schon tot bin!
                ** -----------------------------------------------------*/
                player_found = FALSE;

                if( stricmp( ywd->gsr->NPlayerName, (char *)(rm.data) ) == 0 )
                    ywd->Level->Status = LEVELSTAT_ABORTED;

                /*** player2-Eintrag killen, weil Offsets sich ändern ***/
                for( i=0; i < MAXNUM_PLAYERS; i++ ) {

                    if( stricmp( GSR->player2[ i ].name, (char *)(rm.data) )==0){

                        /*** Rest löschen und raus ***/
                        ULONG j;

                        for( j = i; j < (MAXNUM_PLAYERS-1);j++ )
                            GSR->player2[ j ] = GSR->player2[ j + 1 ];

                        player_found = TRUE;
                        break;
                        }
                    }

                yw_NetLog(">>> Received a destroy player for %s at %d\n",
                           (char *)(rm.data), ywd->TimeStamp/1000 );

                /*** wenn  das Spiel schon gestartet wurde ... ***/
                if( ywd->netgamestartet && player_found ) {

                    /*** Gegangen oder rausgeschmissen? ***/
                    for( i = 0; i < 8; i++ ) {

                        if( stricmp( ywd->gsr->player[ i ].name, (char *)(rm.data) ) == 0 ) {
                            if( NWS_LEFTGAME != ywd->gsr->player[i].status)
                                ywd->gsr->player[i].status = NWS_REMOVED;
                            break;
                            }
                        }


                    /*** Name "groß" machen ***/
                    yw_StrUpper2( text2, rm.data );
                    sprintf( text, "%s %s", text2,  ypa_GetStr( GlobalLocaleHandle,
                             STR_NET_REMOVEPLAYER, " LEFT THE GAME") );
                    log.pri  = 10;
                    log.msg  = text;
                    log.bact = NULL;
                    log.code = 0;
                    _methoda( ywd->world, YWM_LOGMSG, &log );

                    yw_RemovePlayerStuff( ywd, rm.data, 0, 0 );
                    }
                else {
                    
                    if( !player_found )    
                        yw_NetLog("    but the player doesn't exist anymore!\n");
                    else    
                        yw_NetLog("    but netgame was not startet!\n");
                    }

                break;

            case MK_HOST:

                ywd->gsr->is_host = TRUE;
                break;

            case MK_NORMAL:

                /* -------------------------------------------------------
                ** In dieser Message können mehrere Messages gepackt sein.
                ** Und zwar können wir solange Messages auslesen, bis
                ** die Gesamtmessage-größe erreicht ist. Diese Größe er-
                ** mitteln wir aus der Art der Message mittels sizeof.
                ** Lediglich bei YPAM_VEHICLEDATA, wo die größe nicht
                ** eindeutig ist, müssen wir diese aus dem Feld auslesen
                ** -----------------------------------------------------*/

                offset = 0;
                inner_msg = 0;
                while( 1 ) {

                    ULONG size;

                    /*** Messagezahl zu abgefahren? ***/
                    inner_msg++;
                    if(inner_msg > 100) {

                        yw_NetLog("Strange Number of Messages (>100)\n");
                        break;
                        }

                    kind = ((struct ypamessage_generic *)(rm.data))->message_id;

                    /*** fuer Debugzwecke ***/
                    MSG[ inner_msg ] = kind;

                    /*** Message bearbeiten? ***/
                    size = yw_HandleThisMessage( ywd, &rm, error_sender );

                    if( 0 == size ) {

                        /* ----------------------------------------------
                        ** Unbekannte Message. damit lassen sich auch die
                        ** nachfolgenden nicht finden..., also raus!
                        ** --------------------------------------------*/
                        yw_NetLog("    unknown message was number %d and message before was type %d\n",
                                   inner_msg, MSG[ inner_msg-1 ] );
                        yw_NetLog("    current offset is %d, size is %d\n", offset, rm.size );
                        break;
                        }

                    /*** Korrekte größe? ***/
                    if( size < sizeof( struct ypamessage_generic ) ) {

                        yw_NetLog("Error: Message %d has strange size %d!\n",
                                kind, size );
                        break;
                        }

                    /*** Noch was da? ***/
                    offset += size;
                    if( offset >= rm.size )
                        break;
                    else
                        rm.data = &( ((char *)rm.data)[ size ]);
                    }
                    
                /*** Auf leichen untersuchen ***/
                if( e = yw_CorpsesInCellar( GSR ) )
                    strcpy( error_sender, e );

                break;
            }

        /*** Gab es einen dramatischen Fehler? ***/
        if( error_sender[ 0 ] ) {

            /* ----------------------------------------------------
            ** Update anfordern, Name steht in error_sender. Vorher
            ** suchen wir den Owner und testen, ob wir bei diesem
            ** schon auf ein Update warten.
            ** --------------------------------------------------*/
            struct ypamessage_requestupdate ru;

            for( i=0; i < MAXNUM_PLAYERS; i++ ) {

                if( stricmp( GSR->player2[ i ].name, error_sender )==0) {

                    /* -----------------------------------------------
                    ** Wenn wir noch nicht auf ein Update warten, dann
                    ** eines anfordern
                    ** ---------------------------------------------*/
                    if( 0 == GSR->player2[i].waiting_for_update ) {

                        yw_NetLog("Drastic Error: Request Update from %s\n", error_sender );

                        ru.generic.owner      = ywd->gsr->NPlayerOwner;
                        ru.generic.message_id = YPAM_REQUESTUPDATE;

                        sm.data          = &ru;
                        sm.data_size     = sizeof( ru );
                        sm.receiver_kind = MSG_PLAYER;
                        sm.receiver_id   = error_sender;
                        sm.guaranteed    = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

                        GSR->player2[i].waiting_for_update = 2000;
                        }

                    break;
                    }
                }
            }
        }

    /*** Empfangsstatistik ***/
    ywd->gsr->transfer_rcvtime += ywd->TLMsg.frame_time;
    if( ywd->gsr->transfer_rcvtime > 3000 ) {

        /*** Durchschnittliche raten ***/
        if( ywd->gsr->transfer_rcvtime ) {
            ywd->gsr->transfer_rbps = 1000 * ywd->gsr->transfer_rcvcount /
                                             ywd->gsr->transfer_rcvtime;
            ywd->gsr->transfer_sbps = 1000 * ywd->gsr->transfer_sendcount /
                                             ywd->gsr->transfer_rcvtime;
            }

        /*** Minimalwerte ***/
        if( ywd->gsr->transfer_rbps_min == 0 )
            ywd->gsr->transfer_rbps_min = ywd->gsr->transfer_rbps;
        else {

            if( ywd->gsr->transfer_rbps_min > ywd->gsr->transfer_rbps )
                ywd->gsr->transfer_rbps_min = ywd->gsr->transfer_rbps;
            }

        if( ywd->gsr->transfer_sbps_min == 0 )
            ywd->gsr->transfer_sbps_min = ywd->gsr->transfer_sbps;
        else {

            if( ywd->gsr->transfer_sbps_min > ywd->gsr->transfer_sbps )
                ywd->gsr->transfer_sbps_min = ywd->gsr->transfer_sbps;
            }

        /*** Die Maximalwerte ***/
        if( ywd->gsr->transfer_rbps_max < ywd->gsr->transfer_rbps )
            ywd->gsr->transfer_rbps_max = ywd->gsr->transfer_rbps;

        if( ywd->gsr->transfer_sbps_max < ywd->gsr->transfer_sbps )
            ywd->gsr->transfer_sbps_max = ywd->gsr->transfer_sbps;

        /*** Langzeitdurchschnitt ***/
        ywd->gsr->transfer_sbps_avr =
            (ywd->gsr->transfer_gcount * ywd->gsr->transfer_sbps_avr + ywd->gsr->transfer_sbps) /
            (ywd->gsr->transfer_gcount + 1);
        ywd->gsr->transfer_rbps_avr =
            (ywd->gsr->transfer_gcount * ywd->gsr->transfer_rbps_avr + ywd->gsr->transfer_rbps) /
            (ywd->gsr->transfer_gcount + 1);

        /*** Pakete, Durchschnittszahl und Durchschnittsgroesse ***/
        if( ywd->gsr->transfer_pckt_count )
            ywd->gsr->transfer_pckt = ywd->gsr->transfer_sendcount /
                                      ywd->gsr->transfer_pckt_count;

        ywd->gsr->transfer_pckt_avr =
            (ywd->gsr->transfer_gcount * ywd->gsr->transfer_pckt_avr + ywd->gsr->transfer_pckt) /
            (ywd->gsr->transfer_gcount + 1);
        ywd->gsr->transfer_gcount++;

        ywd->gsr->transfer_rcvcount   = 0;
        ywd->gsr->transfer_sendcount  = 0;
        ywd->gsr->transfer_pckt_count = 0;
        ywd->gsr->transfer_rcvtime    = 0;
        }
}



ULONG yw_HandleThisMessage( struct ypaworld_data *ywd,
                            struct receivemessage_msg *rm,
                            UBYTE *trouble_maker )
{
    /* --------------------------------------------------------------
    ** Behandelt die Message. Rückkehrwert ist die größe, so daß sie
    ** als Offset für die nächste Message verwendet werden kann. Wenn
    ** ein Fehler auftrat, der die geschwaderstruktur zerstört hat,
    ** dann gebe ich in fatal_error den owner zurück, von dem ich ein
    ** komplettes neues Bild fordern werde. In troublemaker schreibe
    ** ich gleich noch die sender_id.
    ** ------------------------------------------------------------*/

    ULONG  kind, size, vnumber, k, timestamp, msgcount;
    struct ypamessage_newvehicle *nvm;
    struct ypamessage_destroyvehicle *dvm;
    struct ypamessage_newchief *ncm;
    struct ypamessage_newweapon *nwm;
    struct ypamessage_setstate *ssm;
    struct ypamessage_vehicledata_i *vdm_i;
    struct ypamessage_vehicledata_e *vdm_e;
    struct ypamessage_organize *om;
    struct ypamessage_die *dm;
    struct ypamessage_vehicleenergy *ve;
    struct ypamessage_sectorenergy *se;
    struct ypamessage_startbuild *sb;
    struct ypamessage_buildingvehicle *bv;
    struct ypamessage_viewer *vm;
    struct ypamessage_syncgame *sg;
    struct ypamessage_robodie *rd;
    struct ypamessage_debug *db;
    struct ypamessage_text *tm;
    struct ypamessage_removeplayer *rp;
    struct ypamessage_gem *gm;
    struct ypamessage_race *rcm;
    struct ypamessage_welcome *wm;
    struct ypamessage_readytostart *rts;
    struct ypamessage_requestupdate *ru;
    struct ypamessage_update *upd;
    struct ypamessage_impulse *imp;
    struct ypamessage_logmsg *lm;
    struct ypamessage_neworg *norg;
    struct ypamessage_lobbyinit *li;
    struct ypamessage_startplasma *sp;
    struct ypamessage_endplasma *ep;
    struct ypamessage_extravp *evp;
    struct ypamessage_startbeam *stb;
    struct ypamessage_endbeam *eb;
    struct ypamessage_announcequit *aq;
    struct ypamessage_changelevel *cl;
    struct ypamessage_checksum *cs;
    struct ypamessage_requestlatency *rl;
    struct ypamessage_answerlatency *al, sal;
    struct ypamessage_starttrouble *st;
    struct ypamessage_endtrouble *et;
    struct ypamessage_cd *cdm;
    struct ypamessage_race rcm2;
    struct createbuilding_msg cb;
    struct createvehicle_msg cv;
    struct createweapon_msg cw;
    struct energymod_msg mse;
    struct getplayerdata_msg gpd;
    struct sendmessage_msg sm;
    struct Wunderstein *gem;
    struct bact_message blog;
    struct notifydeadrobo_msg ndr;
    struct Cell *sec;
    Object *no;
    struct Bacterium *dv;
    struct OBNode *robo, *commander, *slave, *gun;
    BOOL   MSG_IS_TOO_OLD = FALSE;
    UBYTE  owner, ko;
    char   text[ 200 ];
    struct logmsg_msg log;
    WORD   vproto, bproto;
    struct ypa_HistVhclKill vkill;
    struct getsectorinfo_msg gsi;
    struct yparobo_data *yrd;
    struct playerdata2 *player_data2 = NULL;

    /*** Handelt eine spezielle Message ab und gibt deren Größe zurück ***/
    kind      = ((struct ypamessage_generic *)(rm->data))->message_id;
    owner     = ((struct ypamessage_generic *)(rm->data))->owner;
    timestamp = ((struct ypamessage_generic *)(rm->data))->timestamp;
    msgcount  = ((struct ypamessage_generic *)(rm->data))->msgcount;

    if( (owner < 8) && (owner > 0) ) {

        /*** Zum Überprüfen, ob Messages reinkamen ***/
        ywd->gsr->player[ owner ].lastmsgtime = ywd->TLMsg.global_time;

        /* --------------------------------------------------------------
        ** TimeStamp des Owners, um zu prüfen, ob sie zu alt ist.
        ** Achtung, einige Messages beziehen sich auf Schatten und gelten
        ** (zumindest auch) dem Original. Somit ist die zeit nicht
        ** exakt!
        ** Weiterhin machen wir die tests nur, wenn die Message an alle
        ** adressiert war, weil sonst die owner-spezifischen msgcounts
        ** auf den Schattenmaschinen aus dem Takt kommen.
        ** ------------------------------------------------------------*/
        if( (kind    != YPAM_VEHICLEENERGY) &&
            (kind    != YPAM_ENDPLASMA) &&  
            (MSG_ALL == rm->receiver_kind ) ) {

            if( ywd->gsr->player[ owner ].timestamp > timestamp) {

                /*** nana, das ist aber seltsam... ***/
                MSG_IS_TOO_OLD = TRUE;
                yw_NetLog("Warning, Msg %d from owner %d is too old!\n",
                         kind, owner );
                yw_NetLog("         old: at %d sec  new: at %d sec\n", ywd->gsr->player[ owner ].timestamp, timestamp );
                }
            else {

                /*** Scheint ok zu sein ***/
                if( (timestamp - ywd->gsr->player[ owner ].timestamp) > 6000 )
                    yw_NetLog(" Message %d from owner %d at %d sec is too fast...\n",
                              kind, owner, timestamp/1000);

                MSG_IS_TOO_OLD = FALSE;
                ywd->gsr->player[ owner ].timestamp = timestamp;
                }


            /*** Test anhand von Messagecounts, ob etwas verloren gegangen ist ***/
            if( ywd->gsr->player[ owner ].msgcount == msgcount ) {

                ywd->gsr->player[ owner ].msgcount++;
                }
            else {

                if( ywd->gsr->player[ owner ].msgcount < msgcount ) {

                    /*** Da kam eine Message nicht an... ***/
                    yw_NetLog("PANIC!!! Msg lost! Last msg has count %d\n", ywd->gsr->player[ owner ].msgcount-1);
                    yw_NetLog("         Msg now (%d) has count %d\n", kind, msgcount);
                    yw_NetLog("         From Owner %d at time %d\n\n", owner, timestamp/1000);
                    }
                else {

                    yw_NetLog("HMM !!!  Late message received with count %d\n",msgcount);
                    yw_NetLog("         from owner %d, id %d at time %d\n", owner, kind, timestamp/1000 );
                    }

                /*** Darufhin korrigieren ***/
                ywd->gsr->player[ owner ].msgcount = msgcount + 1;
                }
            }
        }
    else {

        /*** kein Eigentümer ??? Manche Messages lassen das zu... ***/

        if( (kind != YPAM_RACE) &&
            (kind != YPAM_TEXT) &&
            (kind != YPAM_WELCOME) &&
            (kind != YPAM_READYTOSTART) &&
            (kind != YPAM_LOADGAME) )
            yw_NetLog("Warning, no or false owner (%d) specified for msg %d\n",
                     owner, kind );

        MSG_IS_TOO_OLD = FALSE;
        }

    /* ----------------------------------------------------------------
    ** warten wir von diesem sender auf ein Update? Dann viele Messages
    ** ignorieren, weil die ja wieder Fehler enthalten und neue updates
    ** erfordern. Das muessen wir der Messagegroesse wegen (es koennen
    ** ja auch wichtige Messages enthalten sein) bei jeder einzelnen
    ** message machen.
    ** --------------------------------------------------------------*/
    for( k=0; k < MAXNUM_PLAYERS; k++ ) {

        if( stricmp( ywd->gsr->player2[ k ].name, rm->sender_id )==0) {
            player_data2 = &(ywd->gsr->player2[ k ]);

            /*** runterzaehlen gleich mit erledigen ***/
            if( player_data2->waiting_for_update ) {

                player_data2->waiting_for_update -= ywd->TLMsg.frame_time;
                if( player_data2->waiting_for_update < 0 )
                    player_data2->waiting_for_update = 0;
                }

            break;
            }
        }


    switch( kind ) {

        case YPAM_LOADGAME:

            size = sizeof( struct ypamessage_loadgame);

            /*** Weil wir die Größe brauchen, müssen wir überall testen ***/
            if( ywd->gsr->player[ owner ].was_killed ) break;

            /*** Das kann nur kommen, wenn wir in der Shell sind ***/
            ywd->gsr->GSA.LastAction = A_NETSTART;
            ywd->gsr->GSA.ActionParameter[ 0 ] = ((struct ypamessage_loadgame *)(rm->data))->level;
            ywd->gsr->GSA.ActionParameter[ 1 ] = ((struct ypamessage_loadgame *)(rm->data))->level;
            ywd->playing_network = TRUE;

            yw_PrintNetworkInfoStart( ywd->gsr );
            break;

        case YPAM_NEWVEHICLE:

            /* ------------------------------------------------
            ** Es wurde irgendwo ein neues Vehicle erzeugt. Wir
            ** erzeugen es auch und klinken es entsprechend der
            ** übergebenen Parameter ein.
            ** ----------------------------------------------*/
            nvm = (struct ypamessage_newvehicle *)(rm->data);
            size = sizeof( struct ypamessage_newvehicle);
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            cv.x  = nvm->pos.x;
            cv.y  = nvm->pos.y;
            cv.z  = nvm->pos.z;
            cv.vp = nvm->type;
            if( no = (Object *)_methoda( ywd->world,
                                    YWM_CREATEVEHICLE, &cv) ) {

            struct Bacterium *bact;
            struct setstate_msg state;

            _get( no, YBA_Bacterium, &bact);

            /*** CREATE setzen ***/
            state.main_state = ACTION_CREATE;
            state.extra_off  = state.extra_on = 0L;
            _methoda( no, YBM_SETSTATE_I, &state );

            /*** Wirt suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, nvm->generic.owner );

            /*** ...und anketten ***/
            if( robo ) {

                struct OBNode *com;
                BOOL   found = FALSE;
                struct ypa_HistVhclCreate vcreate;

                switch( nvm->vkind ) {

                    case NV_ROBO:

                        /*** Direkt an die Welt ***/
                        _methoda( ywd->world, YWM_ADDCOMMANDER, no );
                        break;

                    case NV_COMMANDER:

                        /*** Direkt unter den Robo ***/
                        _methoda( robo->o, YBM_ADDSLAVE, no);
                        break;

                    case NV_SLAVE:

                        /*** Erstmal Commander suchen ***/
                        found = FALSE;
                        com = (struct OBNode *) robo->bact->slave_list.mlh_Head;
                        while( com->nd.mln_Succ ) {

                            if( com->bact->ident == nvm->master ) {

                                found = TRUE;
                                break;
                                }

                            com = (struct OBNode *) com->nd.mln_Succ;
                            }

                        if( found )
                            _methoda( com->o, YBM_ADDSLAVE, no);
                        else {

                            /*** Fehler und raus hier ***/
                            yw_NetLog("NV: No master for created shadow object!\n");
                            _methoda( ywd->world, YWM_RELEASEVEHICLE, no);
                            strcpy( trouble_maker, rm->sender_id );
                            return( size );
                            }
                        break;
                    }

                /*** Initialisieren ***/
                bact->Owner      = nvm->generic.owner;
                bact->ident      = nvm->ident;
                bact->CommandID  = nvm->command_id;
                bact->last_frame = ywd->TimeStamp;
                bact->robo       = robo->o;

                /* ---------------------------------------------------------
                ** NEWVEHICLE wird doch nur für "normale" Vehicle verwendet,
                ** oder ??? (sonst ExtraFlag ob Meldung für Statistik)
                ** -------------------------------------------------------*/
                vcreate.cmd    = YPAHIST_VHCLCREATE;
                vcreate.owner  = bact->Owner;
                vcreate.vp     = bact->TypeID;
                vcreate.pos_x  = (UBYTE)((bact->pos.x*256)/bact->WorldX);
                vcreate.pos_z  = (UBYTE)((bact->pos.z*256)/bact->WorldZ);
                _methoda( ywd->world, YWM_NOTIFYHISTORYEVENT, &vcreate );

                /*** Debug ***/
                yw_AddCreated( robo->bact->Owner, nvm->ident );
                }
            else {

                yw_NetLog("\n+++ NV: Robo Owner %d not found (%ds)\n",
                         nvm->generic.owner ,ywd->TimeStamp/1000 );
                }
            }

            break;

        case YPAM_NEWCHIEF:

            /*** Vehicle suchen ***/
            ncm = (struct ypamessage_newchief *)(rm->data);
            size = sizeof( struct ypamessage_newchief );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, ncm->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ NC:Havent found robo with owner %d (%ds)\n",
                         ncm->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            vnumber = yw_GetVehicleNumber( robo->bact );

            /* -------------------------------------------
            ** Ein Commander wird removed. In der message
            ** steht außerdem noch, wer der neue Commander
            ** wird und meine Slaves bekommt. Bei id==0
            ** ist eben keiner da.
            ** -----------------------------------------*/
            if( dv = yw_GetBactByID( robo->bact, ncm->ident) ) {

                struct Bacterium *nm;

                if( ncm->new_master &&
                    (nm = yw_GetBactByID( robo->bact,
                                         ncm->new_master))) {

                    struct OBNode *sn;

                    /*** Neuer auf Com.-Niveau ***/
                    _methoda( robo->o, YBM_ADDSLAVE,
                              nm->BactObject );

                    /*** Alle meine Slaves übergeben ***/
                    while( dv->slave_list.mlh_Head->mln_Succ ) {

                        sn = (struct OBNode *) dv->slave_list.mlh_Head;
                        _methoda( nm->BactObject, YBM_ADDSLAVE, sn->o );
                        }

                    /*** Ich muß mich doch auch selbst übergeben (kotzzzz)! ***/
                    _methoda( nm->BactObject, YBM_ADDSLAVE, dv->BactObject );
                    }

                if( vnumber != yw_GetVehicleNumber( robo->bact ) ) {
                    yw_NetLog("\n+++ NC: Vehiclecount changed! (%ds)\n" ,
                             ywd->TimeStamp/1000);
                    strcpy( trouble_maker, rm->sender_id );
                    }
                }
            else {
                yw_NetLog("\n+++ NC: Havent found vehicle %d (%ds)\n",
                         ncm->ident ,ywd->TimeStamp/1000 );
                strcpy( trouble_maker, rm->sender_id );
                }

            break;

        case YPAM_DESTROYVEHICLE:

            /* -------------------------------------------
            ** Nur rausnehmen. Das Logische Sterben machen
            ** wir mit YPAM_DIE. Dies ist das Gegenstück
            ** zu RELEASEVEHICLE
            ** -----------------------------------------*/
            dvm = (struct ypamessage_destroyvehicle *)(rm->data);
            size = sizeof( struct ypamessage_destroyvehicle);
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, dvm->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ DV:Havent found robo with owner %d (%ds)\n",
                         dvm->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            /*** normales vehicle ***/
            dv = yw_GetBactByID( robo->bact, dvm->ident);
            if( dv == NULL ) {
                yw_NetLog("\n+++ DV: Havent found vehicle %d (%ds)\n",
                         dvm->ident ,ywd->TimeStamp/1000 );
                strcpy( trouble_maker, rm->sender_id );
                }

            if( dv ) {


                if( !(dv->ExtraState & EXTRA_LOGICDEATH)) {
                
                    yw_NetLog("+++ DV: Release a non-logic-dead vehicle %d! (%ds)\n",
                             dvm->ident ,ywd->TimeStamp/1000 );
                    strcpy( trouble_maker, rm->sender_id );
                    break;
                    }

                vnumber = yw_GetVehicleNumber( robo->bact );

                /* -----------------------------------------------------------------
                ** Sicherheitscheck: Normalerweise darf es auf grund der festen YLS
                ** nicht passieren, daß ich freigegeben werde und noch tote Slaves
                ** habe. Außerdem ist in YBM_DIE ein diesbezüglicher test. Ich fühle
                ** mich aber trotzdem sicherer, wenn ich nochmal teste.
                ** ---------------------------------------------------------------*/
                while( dv->slave_list.mlh_Head->mln_Succ ) {

                    struct OBNode *slave = (struct OBNode *) dv->slave_list.mlh_Head;
                    _methoda( ywd->world, YWM_RELEASEVEHICLE, slave->o );
                    yw_NetLog("+++ DV: Released vehicle with slave! (%ds)\n" ,
                             ywd->TimeStamp/1000);
                    strcpy( trouble_maker, rm->sender_id );
                    }

                while( dv->auto_list.mlh_Head->mln_Succ ) {

                    struct OBNode *waffe;
                    waffe = (struct OBNode *)_RemHead( (struct List *) &( dv->auto_list) );
                    waffe->bact->master = NULL;
                    
                    /*** Gibt alle Waffen frei ***/
                    yw_ReleaseWeapon( ywd, dv );

                    yw_NetLog("+++ DV: Released vehicle with weapons! (%ds)\n" ,
                             ywd->TimeStamp/1000);
                    strcpy( trouble_maker, rm->sender_id );
                    }

                _methoda( ywd->world, YWM_RELEASEVEHICLE, dv->BactObject);

                /*** Debug ***/
                yw_AddDestroyed( robo->bact->Owner, dvm->ident );

                if( dvm->class != BCLID_YPAMISSY ) {
                    if( (vnumber-1) != yw_GetVehicleNumber( robo->bact ) ) {
                        yw_NetLog("\n+++ DV: Vehiclecount changed more than 1! (%ds)\n" ,
                                 ywd->TimeStamp/1000);
                        strcpy( trouble_maker, rm->sender_id );
                        }
                    }
                }

            break;

        case YPAM_NEWWEAPON:

            /* ---------------------------------------------
            ** Eine Neue Waffe erzeugen und entsprechend der
            ** Raketensonderbehandlung einklinken.
            ** -------------------------------------------*/
            nwm = (struct ypamessage_newweapon *)(rm->data);
            size = sizeof( struct ypamessage_newweapon );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, nwm->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ NW: Havent found robo with owner %d (%ds)\n",
                         nwm->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            cw.x  = nwm->pos.x;
            cw.y  = nwm->pos.y;
            cw.z  = nwm->pos.z;
            cw.wp = nwm->type;
            if( no = (Object *) _methoda( ywd->world, YWM_CREATEWEAPON, &cw ) ) {

                struct WeaponProto *wproto;
                struct Bacterium *bact, *master;
                ULONG  handle;

                _get( ywd->world, YWA_WeaponArray, &wproto );
                _get( no, YBA_Bacterium, &bact );

                /*** Meister suchen ***/
                if( master = yw_GetBactByID( robo->bact, nwm->rifleman)) {

                    struct Node *node;

                    /*** Ankett-Sonderbehandlung ***/
                    if( bact->master ) {

                        _Remove( (struct Node *) &(bact->slave_node));
                        bact->master = NULL;
                        }

                    _get( no, YMA_AutoNode, &node );
                    _AddTail( (struct List *) &(master->auto_list),
                              (struct Node *) node );

                    /*** Schütze eintragen !!!! ***/
                    _set( no, YMA_RifleMan, (ULONG)master );
                    }
                else {

                    /*** kein Wirt. Freigeben ***/
                    yw_NetLog("\n+++ NW: Havent found vehicle %d (%ds)\n",
                             nwm->ident ,ywd->TimeStamp/1000 );
                    _methoda( ywd->world, YWM_RELEASEVEHICLE, no );
                    strcpy( trouble_maker, rm->sender_id );
                    return( size );
                    }

                /*** init ***/
                bact->Owner      = nwm->generic.owner;
                bact->ident      = nwm->ident;
                bact->last_frame = ywd->TimeStamp;

                /*** Debug ***/
                yw_AddCreated( robo->bact->Owner, nwm->ident );

                /*** Rakete nach uebergebener Geschw. ausrichten ***/
                bact->dof.v = nc_sqrt( nwm->dir.x * nwm->dir.x + nwm->dir.y *
                                       nwm->dir.y + nwm->dir.z * nwm->dir.z);
                bact->dof.x = nwm->dir.x;
                bact->dof.y = nwm->dir.y;
                bact->dof.z = nwm->dir.z;
                if( bact->dof.v > 0.001 ) {
                    bact->dof.x /= bact->dof.v;
                    bact->dof.y /= bact->dof.v;
                    bact->dof.z /= bact->dof.v;
                    }
                _methoda( no, YMM_ALIGNMISSILE_S, NULL );

                _get( no, YMA_Handle ,&handle );

                if( YMF_Search == handle ) {

                    struct settarget_msg target;

                    /*** Ziel setzen ***/
                    target.pos         = nwm->target_pos;
                    target.priority    = 0;
                    target.target_type = nwm->target_type;

                    if( TARTYPE_BACTERIUM == nwm->target_type ) {

                        /*** Bacterium dazu holen ***/
                        struct OBNode *r2 = yw_GetRoboByOwner( ywd, nwm->target_owner);
                        if( !r2 ) {

                            yw_NetLog("\n+++ NW: false targetowner %d for weapon\n",
                                     nwm->target_owner );
                            strcpy( trouble_maker, rm->sender_id );
                            return( size );
                            }
                        target.target.bact = yw_GetBactByID( r2->bact, nwm->target);
                        if( !target.target.bact ) {

                            /* --------------------------------------------
                            ** Ist moeglich, weil das vehicle schon gestor-
                            ** ben sein kann. Einfach kein Ziel setzen.
                            ** ------------------------------------------*/
                            target.target_type = TARTYPE_NONE;
                            }
                        }

                    _methoda( no, YBM_SETTARGET, &target );

                    /*** y-pos wurde korrekt geliefert, also immer setzen ***/
                    if( TARTYPE_SECTOR == nwm->target_type )
                        bact->PrimPos.y = nwm->target_pos.y;
                    }

                if( handle == YMF_Simple ) {

                    /*** dof ist bereits initialisiert worden ***/
                    bact->PrimTargetType = TARTYPE_SIMPLE;
                    bact->tar_unit.x     = bact->dof.x;
                    bact->tar_unit.y     = bact->dof.y;
                    bact->tar_unit.z     = bact->dof.z;
                    }

                bact->robo = robo->o;

                if( TARTYPE_BACTERIUM != nwm->target_type ) {

                    if( wproto[ nwm->type ].LifeTimeNT )
                        _set( no, YMA_LifeTime, wproto[ nwm->type ].LifeTimeNT );
                    }
                }
            else
                yw_NetLog("\n+++ NW: Was not able to create weapon %d for owner %d\n",
                        nwm->type, nwm->generic.owner );

            break;

        case YPAM_SETSTATE:

            /* ------------------------------------------
            ** Nur rausnehmen. Das Umorganisieren der
            ** geschwaderstruktur machen wir mit newchief
            ** ----------------------------------------*/
            ssm = (struct ypamessage_setstate *)(rm->data);
            size = sizeof( struct ypamessage_setstate );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, ssm->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ SS: Havent found robo with owner %d (%ds)\n",
                         ssm->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            dv = yw_GetBactByID( robo->bact, ssm->ident );
            if( dv == NULL ) {
                yw_NetLog("\n+++ SS: Havent found vehicle %d (%ds)\n",
                         ssm->ident, ywd->TimeStamp/1000 );
                strcpy( trouble_maker, rm->sender_id );
                }

            if( dv ) {

                struct setstate_msg state;

                state.main_state    = ssm->main_state;
                state.extra_on      = ssm->extra_on;
                state.extra_off     = ssm->extra_off;
                _methoda( dv->BactObject, YBM_SETSTATE_I, &state );
                }

            break;

        case YPAM_VEHICLEDATA_I:

            /* ------------------------------------------
            ** 2 messages! denn die sind unterschiedlich
            ** gross und ausserdem kann ich so testen!
            ** ----------------------------------------*/
            vdm_i = (struct ypamessage_vehicledata_i *)(rm->data);
            size = vdm_i->head.size;
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Zu alte Messages werden nicht beachtet! ***/
            if( MSG_IS_TOO_OLD ) break;

            if( !(ywd->interpolate) ) {

                yw_NetLog("\n+++ VD: An extrapolate-Program gets interpolate data!!!\n");
                break;
                }

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, vdm_i->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ VD: Havent found robo with owner %d (%ds)\n",
                         vdm_i->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            yw_ExtractVehicleData( ywd, vdm_i, robo );

            break;

        case YPAM_VEHICLEDATA_E:

            vdm_e = (struct ypamessage_vehicledata_e *)(rm->data);
            size = vdm_e->head.size;
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Zu alte Messages werden nicht beachtet! ***/
            if( MSG_IS_TOO_OLD ) break;

            if( ywd->interpolate ) {

                yw_NetLog("\n+++ VD: An interpolate-Program gets extrapolate data!!!\n");
                break;
                }

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, vdm_e->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ VD: Havent found robo with owner %d (%ds)\n",
                         vdm_e->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            yw_ExtractVehicleData( ywd, (struct ypamessages_vehicledata *)vdm_e, robo );

            break;

        case YPAM_ORGANIZE:

            /* ----------------------------------------------
            ** Abbild der Organize-Methode. Scheiß anfälliger
            ** Verwaltungskram.
            ** --------------------------------------------*/
            om = (struct ypamessage_organize *)(rm->data);
            size = sizeof( struct ypamessage_organize );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, om->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ O: Havent found robo with owner %d (%ds)\n",
                         om->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            vnumber = yw_GetVehicleNumber( robo->bact );

            yw_Organize( om, robo->bact, ywd );

            if( vnumber != yw_GetVehicleNumber( robo->bact ) ) {
                yw_NetLog("\n+++ O: Vehiclecount changed with mode %d! (%ds)\n",
                         om->modus ,ywd->TimeStamp/1000);
                strcpy( trouble_maker, rm->sender_id );
                }

            break;

        case YPAM_DIE:

            /* ----------------------------------------------
            ** Erledigt alle Formalitäten, die mit dem logischen
            ** Sterben zu tun haben. Dazu gehört: Neuer
            ** master, wenn denn einer da ist, Freigabe der
            ** Raketen und Status setzen
            ** --------------------------------------------*/
            dm = (struct ypamessage_die *)(rm->data);
            size = sizeof( struct ypamessage_die );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, dm->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ D: Havent found robo with owner %d (%ds)\n",
                         dm->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            if( BCLID_YPAMISSY == dm->class ) {

                /*** Dürfte eigentlich nicht passieren ***/
                dv = yw_GetMissileBactByID( robo->bact, dm->ident);
                if( !dv ) {
                    yw_NetLog("\n+++ D: Havent found weapon %d (%ds)\n",
                             dm->ident ,ywd->TimeStamp/1000 );
                    strcpy( trouble_maker, rm->sender_id );
                    }
                }
            else {

                dv = yw_GetBactByID( robo->bact, dm->ident);
                if( !dv ) {
                    yw_NetLog("\n+++ D: Havent found vehicle %d (%ds)\n",
                             dm->ident ,ywd->TimeStamp/1000 );
                    strcpy( trouble_maker, rm->sender_id );
                    }
                }

            if( dv ) {

                struct Bacterium *nm;
                struct OBNode *deadman, *waffe;

                /* ---------------------------------------------------------
                ** Zuerst bekommt mein Master alle leichen, denn die werden,
                ** egal ob ein neuer Master gefunden wurde, nicht übergeben!
                ** -------------------------------------------------------*/
                deadman = (struct OBNode *) dv->slave_list.mlh_Head;
                while( deadman->nd.mln_Succ ) {

                    struct OBNode *n = (struct OBNode *)deadman->nd.mln_Succ;
                    if( ACTION_DEAD == deadman->bact->MainState ) {
                        if( (ULONG)(dv->master) > 2L ) {

                            /*** Darf nicht mehr sein! ***/
                            _methoda( dv->master, YBM_ADDSLAVE, deadman->o );
                            deadman->bact->ExtraState |= EXTRA_NOMESSAGES;
                            }
                        else {

                            _methoda( ywd->world, YWM_ADDCOMMANDER, deadman->o );
                            deadman->bact->ExtraState |= EXTRA_NOMESSAGES;
                            }
                        }

                    deadman = n;
                    }

                /*** Slaves abgeben? ***/
                if( dm->new_master &&
                    (nm = yw_GetBactByID( robo->bact,
                                          dm->new_master))) {

                    struct OBNode *sn;

                    /*** Neuer auf Com.-Niveau ***/
                    _methoda( robo->o, YBM_ADDSLAVE,
                              nm->BactObject );

                    /*** Alle meine Slaves übergeben ***/
                    while( dv->slave_list.mlh_Head->mln_Succ ) {

                        sn = (struct OBNode *) dv->slave_list.mlh_Head;
                        _methoda( nm->BactObject, YBM_ADDSLAVE, sn->o );
                        }
                    }
                else {

                    /*** Noch leute, die keine leichen sind ??? ***/
                    if( dv->slave_list.mlh_Head->mln_Succ ) {

                        if( dm->new_master )
                            yw_NetLog("\n+++ D: No master (%d) found for my slaves (%ds)\n",
                                     dm->new_master ,ywd->TimeStamp/1000);
                        else
                            yw_NetLog("\n+++ D: No master given for my slaves (%ds)\n" ,
                                     ywd->TimeStamp/1000);
                        strcpy( trouble_maker, rm->sender_id );
                        }
                    else {

                        /* ----------------------------------------------
                        ** ich war wirklich der letzte --> SiegesMeldung,
                        ** wenn mein Killer auf diesem Rechner original
                        ** war
                        ** Rguns sterben nicht einzeln und dürfen auch
                        ** keine meldung bringen, wenn Robo stirbt.
                        ** --------------------------------------------*/
                        struct Bacterium *urbact;
                        Object *userrobo;
                        ULONG  rgun = 0L;

                        _get( ywd->world, YWA_UserRobo,  &userrobo );
                        _get( userrobo,   YBA_Bacterium, &urbact );
                        if( BCLID_YPAGUN == dv->BactClassID )
                            _get( dv->robo, YGA_RoboGun, &rgun );

                        if( (dm->killerowner == urbact->Owner) &&
                            (dv->master      == dv->robo) &&
                            (rgun            == 0L) &&
                            (!(dv->ExtraState & EXTRA_NOMESSAGES)) ) {

                            blog.ID     = LOGMSG_ENEMYDESTROYED;
                            blog.sender = dv->killer;
                            blog.para1  = 0;
                            blog.para2  = blog.para3  = 0;
                            blog.pri    = 36;
                            _methoda( robo->o, YRM_LOGMSG, &blog );
                            }
                        }
                    }

                /*** Waffen freigeben ***/
                if( ((ULONG)dv->master) > 2L ) {

                    while(waffe=(struct OBNode *)_RemHead((struct List *)&dv->auto_list)) {

                        /*** umschichten ***/
                        _AddTail( (struct List *) &(dv->master_bact->auto_list),
                                  (struct Node *) waffe);

                        /*** neuer Schütze ***/
                        _set( waffe->o, YMA_RifleMan, dv->master_bact );
                        }
                    }
                else {

                    /*** Wir haben keinen Chef mehr und lassen die Raketen explodieren ***/
                    yw_ReleaseWeapon( ywd, dv );
                    }

                /*** Status setzen ***/
                dv->MainState = ACTION_DEAD;
                if( dm->landed ) {

                    struct setstate_msg state;

                    state.main_state = state.extra_off = 0;
                    state.extra_on   = EXTRA_MEGADETH;
                    _methoda( dv->BactObject, YBM_SETSTATE_I, &state );
                    }

                /*** Abmelden der Attacker ***/
                yw_RemoveAttacker( dv );

                /*** Noch 'n test ***/
                slave = (struct OBNode *)dv->slave_list.mlh_Head;
                while( slave->nd.mln_Succ ) {

                    yw_NetLog("+++ D: I am dead, but I have slave ident %d class %d with state %d (%ds)\n",
                            slave->bact->ident, slave->bact->BactClassID, slave->bact->MainState ,ywd->TimeStamp/1000 );
                    slave = (struct OBNode *) slave->nd.mln_Succ;
                    }

                /*** Kennzeichnen ***/
                dv->ExtraState |= EXTRA_LOGICDEATH;
                
                /*** Killer ermitteln. Der kann schon weg sein ***/
                if( dm->killerowner ) {
                
                    struct OBNode *killer_robo;
                    killer_robo = yw_GetRoboByOwner( ywd, dm->killerowner );
                    dv->killer  = yw_GetBactByID( killer_robo->bact, dm->killer );
                    } 

                /*** (FIXME FLOH) Meldung ans statistische Amt ***/
                if (dv->killer) {
                    ULONG is_user;
                    ko = dv->killer->Owner;
                    vkill.cmd = YPAHIST_VHCLKILL;
                    vkill.owners = ((ko<<3) | (dv->Owner));
                    _get(dv->killer->BactObject,YBA_Viewer,&is_user);
                    if (is_user || (dv->killer->ExtraState & EXTRA_ISVIEWER)) {
                        vkill.owners |= (1<<7); // Killer war ein User
                    };
                    _get(dv->BactObject,YBA_Viewer,&is_user);
                    if (is_user || (dv->ExtraState & EXTRA_ISVIEWER)) {
                        vkill.owners |= (1<<6); // Opfer war ein User
                    };
                    vkill.vp     = dv->TypeID;
                    if (BCLID_YPAROBO == dv->BactClassID) {
                        vkill.vp |= (1<<15); // Opfer war ein Robo
                    };
                    vkill.pos_x  = (UBYTE)((dv->pos.x*256)/dv->WorldX);
                    vkill.pos_z  = (UBYTE)((dv->pos.z*256)/dv->WorldZ);
                    _methoda( ywd->world, YWM_NOTIFYHISTORYEVENT, &vkill );
                };
                
                /*** Als tot markieren, weil dispose auch DIE verwendet ***/
                dv->ExtraState |= EXTRA_LOGICDEATH;
                }

            break;

        case YPAM_VEHICLEENERGY:

            /* ----------------------------------------------
            ** Erledigt Energiehandling und setzt State, wenn
            ** Energie unter 0 ist! STATE dann wieder für
            ** alle! Weil vom Schatten nur diese Message kam.
            ** Wird nur ausgewertet, wenn ich UserRobo bin,
            ** weil nur das original die Energie verwaltet
            ** und eh ein SetState zurückkommt.
            ** --------------------------------------------*/
            ve = (struct ypamessage_vehicleenergy *)(rm->data);
            size = sizeof( struct ypamessage_vehicleenergy );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, ve->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ VE: Havent found robo with owner %d (%ds)\n",
                         ve->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            /*** UserRobo? ***/
            if( ywd->UserRobo != robo->o ) break;

            dv = yw_GetBactByID( robo->bact, ve->ident);

            /* ------------------------------------------------------
            ** Das das Vehicle nicht da ist, ist voellig normal, weil
            ** es im original schon tot sein kann, wenn wir auf den
            ** Schatten schiessen.
            ** ----------------------------------------------------*/
            if( dv ) {

                /*** Energie ***/
                dv->Energy += ve->energy;

                /*** Folgen? ***/
                if( dv->Energy < 0 ) {

                    struct setstate_msg state;

                    /* ------------------------------------------------------
                    ** Wer war der Killer? Achtung, das ist jetzt ein anderer
                    ** Robo!
                    ** ----------------------------------------------------*/
                    if( ve->killer ) {
                        struct OBNode *robo2;
                        robo2 = yw_GetRoboByOwner( ywd, ve->killerowner );
                        if( !robo2) {
                            yw_NetLog("\n+++ VE: No robo with owner %d found\n",
                                     ve->killerowner );
                            break;
                            }

                        if( !(dv->killer = yw_GetBactByID( robo2->bact, ve->killer)) )
                            yw_NetLog("\n+++ VE: False killer %d (owner %d) given\n",
                                     ve->killer, ve->killerowner);
                        }
                    else {
                        dv->killer = NULL;
                        }

                    state.main_state = ACTION_DEAD;
                    state.extra_off  = state.extra_on = 0;
                    _methoda( dv->BactObject, YBM_SETSTATE, &state);
                    }
                }

            break;

        case YPAM_SECTORENERGY:

            /* ---------------------------------------
            ** Energieabzug. Soso. Hm. Ähem. ja. Klar!
            ** Mit Zwangseigentümersetzung, äh... ja
            ** -------------------------------------*/
            se = (struct ypamessage_sectorenergy *)(rm->data);
            size = sizeof( struct ypamessage_sectorenergy );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            robo = yw_GetRoboByOwner( ywd, se->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ VE: Havent found robo with owner %d (%ds)\n",
                         ve->generic.owner ,ywd->TimeStamp/1000 );
                mse.hitman = NULL;
                }
            else
                mse.hitman = yw_GetBactByID( robo->bact, se->hitman );

            mse.energy = se->energy;
            mse.pnt.x  = se->pos.x;
            mse.pnt.y  = se->pos.y;
            mse.pnt.z  = se->pos.z;
            mse.owner  = (UBYTE) se->sectorowner;
            _methoda( ywd->world, YWM_MODSECTORENERGY, &mse );

            break;

        case YPAM_STARTBUILD:

            /* -------------------------------------------------
            ** Startet einen "non-immediate"-Bauauftrag. Set-
            ** sector ist so gestaltet, daß beim "Schattenbauen"
            ** keine Vehicle erzeugt werden. Dazu gibt es eine
            ** Extra-message.
            ** -----------------------------------------------*/
            sb = (struct ypamessage_startbuild *)(rm->data);
            size = sizeof( struct ypamessage_startbuild );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            /*** Existiert der verantw. Robo bei uns? ***/
            robo = yw_GetRoboByOwner( ywd, sb->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ SB: Havent found robo with owner %d (%ds)\n",
                         sb->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            cb.job_id    = sb->generic.owner;
            cb.owner     = sb->generic.owner;
            cb.bp        = sb->bproto;
            cb.immediate = FALSE;
            cb.sec_x     = sb->sec_x;
            cb.sec_y     = sb->sec_y;
            cb.flags     = 0;           // vielleicht mal auf NOBACTS umstellen
            _methoda(ywd->world, YWM_CREATEBUILDING, &cb);

            /* ------------------------------------------------
            ** Ob es geklappt hat, ist erstmal egal. Vielleicht
            ** später für vehicle verwenden...
            ** ----------------------------------------------*/
            break;

        case YPAM_BUILDINGVEHICLE:

            /* --------------------------------------------------
            ** Erzeugt ein geschwader aus einer struktur, welches
            ** wahrscheinlich an ein gebäude gehört. Eine andere
            ** verwendung ist natürlich möglich.
            ** Dr erschte is dr gommandr
            ** ------------------------------------------------*/
            bv = (struct ypamessage_buildingvehicle *)(rm->data);
            size = sizeof( struct ypamessage_buildingvehicle );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Gibts da überhaupt was ??? ***/
            if( !(bv->vehicle[ 0 ].vproto) ) break;

            /*** Existiert der verantw. Robo bei uns? ***/
            robo = yw_GetRoboByOwner( ywd, bv->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ BV: Havent found robo with owner %d (%ds)\n",
                         bv->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            /*** Commander erzeugen ***/
            cv.vp = bv->vehicle[ 0 ].vproto;
            cv.x  = bv->vehicle[ 0 ].pos.x;
            cv.y  = bv->vehicle[ 0 ].pos.y;
            cv.z  = bv->vehicle[ 0 ].pos.z;
            if( no = (Object *) _methoda( ywd->world, YWM_CREATEVEHICLE, &cv) ) {

                struct installgun_msg ig;
                struct setstate_msg state;
                struct Bacterium *b;
                LONG   i;

                ig.flags = 0;
                ig.basis = bv->vehicle[ 0 ].basis;
                _methoda( no, YGM_INSTALLGUN, &ig);

                /*** GENESIS-Zustand aktivieren ***/
                state.main_state = ACTION_CREATE;
                state.extra_off  = state.extra_on = 0;
                _methoda( no, YBM_SETSTATE_I, &state);

                _get( no, YBA_Bacterium, &b );
                b->ident = bv->vehicle[ 0 ].ident;
                b->Owner = bv->generic.owner;
                b->robo  = robo->o;

                _methoda( robo->o, YBM_ADDSLAVE, no );

                /*** Debug ***/
                yw_AddCreated( robo->bact->Owner, b->ident );

                /*** Nun alle Slaves ***/
                i = 1;
                while( bv->vehicle[ i ].vproto ) {

                    Object *co;

                    cv.vp = bv->vehicle[ i ].vproto;
                    cv.x  = bv->vehicle[ i ].pos.x;
                    cv.y  = bv->vehicle[ i ].pos.y;
                    cv.z  = bv->vehicle[ i ].pos.z;
                    if( co = (Object *) _methoda( ywd->world, YWM_CREATEVEHICLE, &cv) ) {

                        ig.flags = 0;
                        ig.basis = bv->vehicle[ i ].basis;
                        _methoda( co, YGM_INSTALLGUN, &ig);

                        /*** GENESIS-Zustand aktivieren ***/
                        state.main_state = ACTION_CREATE;
                        state.extra_off  = state.extra_on = 0;
                        _methoda( co, YBM_SETSTATE_I, &state);

                        _get( co, YBA_Bacterium, &b );
                        b->ident = bv->vehicle[ i ].ident;
                        b->Owner = bv->generic.owner;
                        b->robo  = robo->o;

                                /*** Debug ***/
                                yw_AddCreated( robo->bact->Owner, b->ident );

                        _methoda( no, YBM_ADDSLAVE, co );
                        }
                    i++;
                    }
                }

            break;

        case YPAM_VIEWER:

            /* -----------------------------------------
            ** Kennzeichnet ein vehicle als Viewer einer
            ** anderen Maschine
            ** ---------------------------------------*/
            vm = (struct ypamessage_viewer *)(rm->data);
            size = sizeof( struct ypamessage_viewer );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Existiert der verantw. Robo bei uns? ***/
            robo = yw_GetRoboByOwner( ywd, vm->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ V: Havent found robo with owner %d (%ds)\n",
                         vm->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            if( vm->class == BCLID_YPAMISSY ) {

                dv = yw_GetMissileBactByRifleman( robo->bact, vm->ident, vm->rifleman );
                if( !dv ) {
                    yw_NetLog("\n+++ V: Havent found weapon %d of rifleman %d (%ds)\n",
                            vm->ident, vm->rifleman ,ywd->TimeStamp/1000 );
                    strcpy( trouble_maker, rm->sender_id );
                    }
                }
            else {

                dv = yw_GetBactByID( robo->bact, vm->ident );
                if( !dv ) {
                    yw_NetLog("\n+++ V: Havent found vehicle %d (%ds)\n",
                             vm->ident ,ywd->TimeStamp/1000 );
                    strcpy( trouble_maker, rm->sender_id );
                    }
                }

            if( dv ) {

                if( vm->viewer )
                    dv->ExtraState |=  EXTRA_ISVIEWER;
                else
                    dv->ExtraState &= ~EXTRA_ISVIEWER;
                }

            break;

        case YPAM_SYNCGAME:

            /* -----------------------------------------------------------
            ** Diese Message synchronisiert alle Daten zum Spielstart.
            ** Dazu gehört die "ident"ifikation des zugehörigen Robos.
            ** Weil diese Message geschickt wird, wenn das Spiel auf
            ** dem sendenden rechner anfängt, kann es auch zur
            ** Synchronisation des Startes verwendet werden. Der Host
            ** wartet eben bis sync-Messages von allen Spielern reinkamen.
            ** ---------------------------------------------------------*/
            sg = (struct ypamessage_syncgame *)( rm->data );
            size = sizeof( struct ypamessage_syncgame );
            if( ywd->gsr->player[ owner ].was_killed ) {
                yw_NetLog("\n+++SG: received sync of a dead player %s\n", rm->sender_id );
                break; }

            /*** Existiert der verantw. Robo bei uns? ***/
            robo = yw_GetRoboByOwner( ywd, sg->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ SG: Havent found robo with owner %d (%ds)\n",
                         sg->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            /*** Robo exakt markieren ***/
            robo->bact->ident = sg->robo_ident;

            gun = (struct OBNode *) robo->bact->slave_list.mlh_Head;
            k   = 0;
            while( gun->nd.mln_Succ ) {

               ULONG rgun = 0L;
               if( BCLID_YPAGUN == gun->bact->BactClassID )
                   _get( gun->o, YGA_RoboGun, &rgun );

                if( rgun ) {

                    gun->bact->ident = sg->gun[ k ];
                    k++;
                    }

                gun = (struct OBNode *) gun->nd.mln_Succ;
                }

            /*** Startable merken ***/
            ywd->gsr->player[ sg->generic.owner ].ready_to_play = 1;

            yw_NetLog("received READY TO PLAY from owner %d\n", sg->generic.owner );

            break;

        case YPAM_ROBODIE:

            /* ------------------------------------------------------
            ** Eine Extra-Diemessage für den Robo. Dieser setzt
            ** seine Slaves in einen visuellen Create und logischen
            ** Death-Zustand. Evtl. wird noch eine Message abgefeuert
            ** Weil wir kein DIE auf die Schatten machen, werden
            ** wir die wichtigsten Sachen daraus extra machen: Ab-
            ** melden der Anfreifer und vernichten der Raketen!
            ** ----------------------------------------------------*/
            rd = (struct ypamessage_robodie *)( rm->data );
            size = sizeof( struct ypamessage_robodie );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Existiert der verantw. Robo bei uns? ***/
            robo = yw_GetRoboByOwner( ywd, rd->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ RD: Havent found robo with owner %d (%ds)\n",
                         rd->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            commander = (struct OBNode *) robo->bact->slave_list.mlh_Head;
            while( commander->nd.mln_Succ ) {

                struct setstate_msg state;
                LONG   YLS;
                _get( robo->o, YBA_YourLS, &YLS );

                /*** Die Untergebenen ranholen ***/
                slave = (struct OBNode *) commander->bact->slave_list.mlh_Head;
                while( slave->nd.mln_Succ ) {

                    /*** Angreifer abmelden ***/
                    yw_RemoveAttacker( slave->bact );

                    /*** Raketen freigeben ***/
                    yw_ReleaseWeapon( ywd, slave->bact );

                    /*** Create-VP setzen ***/
                    state.main_state = ACTION_CREATE;
                    state.extra_on  = state.extra_off = 0;
                    _methoda( slave->o, YBM_SETSTATE_I, &state );

                    /*** MainState korrigieren ***/
                    slave->bact->MainState = ACTION_DEAD;

                    /*** Sound korrigieren ***/
                    _EndSoundSource( &(slave->bact->sc), VP_NOISE_GENESIS );
                    _StartSoundSource( &(slave->bact->sc), VP_NOISE_GOINGDOWN );
                    slave->bact->sound &= ~(1L << VP_NOISE_GENESIS );
                    slave->bact->sound |=  (1L << VP_NOISE_GOINGDOWN );

                    /*** YLS mit meinem Wert setzen ***/
                    _set( slave->o, YBA_YourLS, YLS );
                    
                    /*** Als tot markieren, weil dispose auch DIE verwendet ***/
                    slave->bact->ExtraState |= EXTRA_LOGICDEATH;

                    /*** Nachfolger ***/
                    slave = (struct OBNode *) slave->nd.mln_Succ;
                    }

                /*** Die gleichen Sachen noch für den Commander ***/

                /*** Angreifer abmelden ***/
                yw_RemoveAttacker( commander->bact );

                /*** Raketen freigeben ***/
                yw_ReleaseWeapon( ywd, commander->bact );

                /*** Create-VP (DANACH) setzen ***/
                state.main_state = ACTION_CREATE;
                state.extra_on  = state.extra_off = 0;
                _methoda( commander->o, YBM_SETSTATE_I, &state );

                /*** MainState korrigieren ***/
                commander->bact->MainState = ACTION_DEAD;

                /*** Sound korrigieren ***/
                _EndSoundSource( &(commander->bact->sc), VP_NOISE_GENESIS );
                _StartSoundSource( &(commander->bact->sc), VP_NOISE_GOINGDOWN );
                commander->bact->sound &= ~(1L << VP_NOISE_GENESIS );
                commander->bact->sound |=  (1L << VP_NOISE_GOINGDOWN );

                /*** YLS mit meinem Wert setzen ***/
                _set( commander->o, YBA_YourLS, YLS );
                    
                /*** Als tot markieren, weil dispose auch DIE verwendet ***/
                commander->bact->ExtraState |= EXTRA_LOGICDEATH;

                /*** Nachfolger ***/
                commander = (struct OBNode *) commander->nd.mln_Succ;
                }

            /*** Auch als Robo muß ich mich verabschieden ***/
            yw_RemoveAttacker( robo->bact );

            /*** Ich kann ja durch das Umschichten Waffen bekommen haben...***/
            yw_ReleaseWeapon( ywd, robo->bact );
               
            /*** Als tot markieren, weil dispose auch DIE verwendet ***/
            robo->bact->ExtraState |= EXTRA_LOGICDEATH;

            /*** Test, ob ich der letze bin... ***/
            if( robo->bact->Owner != ywd->gsr->NPlayerOwner ) {

                BOOL found_player_robo = FALSE, found_enemy_robo = FALSE;
                struct OBNode *r = (struct OBNode *)ywd->CmdList.mlh_Head;

                while( r->nd.mln_Succ ) {

                    if( (BCLID_YPAROBO          == r->bact->BactClassID) &&
                        (ywd->gsr->NPlayerOwner == r->bact->Owner) &&
                        (ACTION_DEAD            != r->bact->MainState) )
                        found_player_robo = TRUE;

                    if( (BCLID_YPAROBO          == r->bact->BactClassID) &&
                        (ywd->gsr->NPlayerOwner != r->bact->Owner) &&
                        (ACTION_DEAD            != r->bact->MainState) )
                        found_enemy_robo = TRUE;

                    r = (struct OBNode *) r->nd.mln_Succ;
                    }

                /*** Was losschicken? ***/
                if( found_player_robo && (!found_enemy_robo) ) {

                    struct logmsg_msg log;

                    /*** Der Spieler ist der letzte Lebendige ***/
                    log.pri  = 10;
                    log.msg  = ypa_GetStr( GlobalLocaleHandle,
                               STR_LMSG_YOUWIN, "YOU WIN THE GAME");
                    log.bact = NULL;
                    log.code = 0;
                    _methoda( ywd->world, YWM_LOGMSG, &log );
                    }
                else {
                
                    /* --------------------------------
                    ** Dann Meldung:
                    ** 1. Ich habe den besiegt, 
                    ** 2. irgendjemand hat den besiegt.
                    ** 3. irgendjemand ist gestorben
                    ** ------------------------------*/
                    char t[ 300 ];
                    struct logmsg_msg log;
                    
                    if( rd->killerowner ) {
                        
                        if( rd->killerowner == ywd->gsr->NPlayerOwner)
                            sprintf( t, "%s  %s",
                                 ypa_GetStr( GlobalLocaleHandle, 
                                             STR_LMSG_YOUKILLED,
                                             "YOU KILLED"),
                                 rm->sender_id );               
                        else    
                            sprintf( t, "%s  %s  %s", rm->sender_id,
                                 ypa_GetStr( GlobalLocaleHandle, 
                                             STR_LMSG_WASKILLEDBY,
                                             "WAS KILLED BY"),
                                 ywd->gsr->player[ rd->killerowner ].name );
                        }         
                    else
                        sprintf( t, "%s  %s", rm->sender_id,                   
                                 ypa_GetStr( GlobalLocaleHandle, 
                                             STR_LMSG_HASDIED,
                                             "HAS DIED"));
                    log.pri  = 50;  // wie die normale ROBODEAD-message
                    log.msg  = t;
                    log.bact = NULL;
                    log.code = 0;
                    _methoda( ywd->world, YWM_LOGMSG, &log );
                    }    
                }

            /* ----------------------------------------------------------
            ** Meldung ans statistische Amt. Eigentlich müßte ich alle
            ** meine Untergebenen auch melden, aber auf die wird ja
            ** auf der Originalmaschine ein DIE mit nachfolgender Message
            ** angewendet. Und dann geht ja hier bei der Auswertung schon
            ** was los...
            ** --------------------------------------------------------*/
            /*** Killer ermitteln. Der kann schon weg sein ***/
            if( rd->killerowner ) {
            
                struct OBNode *killer_robo;
                killer_robo         = yw_GetRoboByOwner( ywd, rd->killerowner );
                robo->bact->killer  = yw_GetBactByID( killer_robo->bact, rd->killer );
                } 

            if (robo->bact->killer) {
                ULONG is_user;
                ko = robo->bact->killer->Owner;
                vkill.cmd = YPAHIST_VHCLKILL;
                vkill.owners = ((ko<<3) | (robo->bact->Owner));
                _get(robo->bact->killer->BactObject,YBA_Viewer,&is_user);
                if (is_user || (robo->bact->ExtraState & EXTRA_ISVIEWER)) {
                    vkill.owners |= (1<<7); // Killer war ein User
                };
                _get(robo->bact->BactObject,YBA_Viewer,&is_user);
                if (is_user || (dv->ExtraState & EXTRA_ISVIEWER)) {
                    vkill.owners |= (1<<6); // Opfer war ein User
                };
                vkill.vp = dv->TypeID;
                if (BCLID_YPAROBO == robo->bact->BactClassID) {
                    vkill.vp |= (1<<15); // Opfer war ein Robo
                };
                vkill.pos_x  = (UBYTE)((robo->bact->pos.x*256)/robo->bact->WorldX);
                vkill.pos_z  = (UBYTE)((robo->bact->pos.z*256)/robo->bact->WorldZ);
                _methoda( ywd->world, YWM_NOTIFYHISTORYEVENT, &vkill );
            };

            /* ----------------------------------------------------------
            ** Meldung machen. Wie im Einzelspieler kommt die Meldung
            ** immer, allerdings sendet sie im Falle einer Zerstörung der
            ** killer, sonst eben der Robo.
            ** --------------------------------------------------------*/
            if( (rd->killer) && (rd->killerowner == ywd->URBact->Owner) ) {

                /*** killer ermitteln ***/
                struct OBNode *robo2;

                robo2 = yw_GetRoboByOwner( ywd, rd->killerowner );
                if( !robo2 ) {

                    yw_NetLog("\n+++ RD: false owner %d for killer %d\n",
                             rd->killerowner, rd->killer );
                    blog.sender = NULL;
                    }
                else {

                    blog.sender = yw_GetBactByID( robo2->bact, rd->killer );
                    if( !blog.sender )
                        yw_NetLog("\n+++ RD: unknown ID %d for owner %d\n",
                                 rd->killer, rd->killerowner );
                    }
                }
            else {

                /* ----------------------------------------------------
                ** Entweder durch Energieverlust oder nicht durch meine
                ** Hand gestorben. Also ist Sender "neutral"
                ** --------------------------------------------------*/
                blog.sender = NULL;
                }

            /*** immer Meldung ***/
            ndr.killer_id = rd->killerowner;
            ndr.victim    = robo->bact;
            _methoda( ywd->world, YWM_NOTIFYDEADROBO, &ndr );


            //blog.ID     = LOGMSG_ROBODEAD;
            //blog.para1  = robo->bact->Owner;
            //blog.para2  = blog.para3 = 0;
            //blog.pri    = 50;
            //_methoda( ywd->UserRobo, YRMyw_NetLog, &blog);

            break;

        case YPAM_DEBUG:

            db = (struct ypamessage_debug *)( rm->data );
            size = sizeof( struct ypamessage_debug );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /*** Existiert der verantw. Robo bei uns? ***/
            robo = yw_GetRoboByOwner( ywd, db->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ DEBUG: Havent found robo with owner %d (%ds)\n",
                         db->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            /*** Nun Geschwaderstruktur vergleichen ***/
            if( robo->bact->ident == db->data[ 0 ] ) {

                int i;

                /*** Das war schon mal ok ***/
                for( i = 1; i < MAXNUM_DEBUGMSGITEMS; ) {

                    BOOL raus = FALSE;
                    ULONG commander, slave;

                    commander = db->data[ i++ ];
                    if( !yw_GetBactByIDAndInfo( robo->bact, commander,
                                                SI_COMMANDER, 0L )) {
                        yw_NetLog("\n+++DEBUG: commander %d does not exist (%ds)\n",
                                 commander ,ywd->TimeStamp/1000);
                        break;
                        }

                    while( (slave = db->data[ i++ ]) != DBGMSG_NEWCOMMANDER ) {

                        /*** Den Slave untersuchen ***/
                        if( !yw_GetBactByIDAndInfo( robo->bact, slave,
                                                    SI_SLAVE, commander) ) {
                            yw_NetLog("\n+++DEBUG: no slave %d for commander %d (%ds)\n",
                                     slave, commander ,ywd->TimeStamp/1000 );
                            raus = TRUE;
                            break;
                            }
                        } 

                    if( raus ) break;
                    if( db->data[ i ] == DBGMSG_END ) break;
                    }
                }

            break;

        case YPAM_TEXT:

            /* ----------------------------------------------------
            ** Es kam eine Message rein. Wir können nicht mit der
            ** Playerdata-Struktur arbeiten, weil diese z.T. noch
            ** nicht ausgefüllt ist. Deshalb gibt es hier einen
            ** Buffer, wo wir nach dem Namen suchen oder ihn in
            ** den ersten freien Platz eintragen
            ** --------------------------------------------------*/

            tm = (struct ypamessage_text *)( rm->data );
            size = sizeof( struct ypamessage_text );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( stricmp( rm->sender_id, gpd.name ) == 0 ) {

                    WORD number;
                    number = (WORD) strtol( tm->text, NULL, 0 );

                    /*** Sonderbehandlung für Nummmern ***/
                    if( number > 0 ) {

                        yw_LaunchChatSample( ywd, number );
                        break;
                        }

                    if( ywd->netgamestartet ) {

                        char text[ 200 ];
                        struct logmsg_msg log;

                        sprintf( text, "%s: %s", rm->sender_id,tm->text );
                        log.pri  = 10;
                        log.msg  = text;
                        log.bact = NULL;
                        log.code = 0;
                        _methoda( ywd->world, YWM_LOGMSG, &log );
                        }
                    else {

                        yw_AddMessageToBuffer( ywd, rm->sender_id, tm->text );
                        }

                    break;
                    }
                gpd.number++;
                }

            break;

        case YPAM_REMOVEPLAYER:

            /* ------------------------------------------------------------
            ** Ein Player wird abgemeldet. Das kann auch der eigene sein, 
            ** wenn der Host mich rausschmeisst. Ist es ein Fremder, raeume
            ** ich sofort auf, ansonsten setze ich ihn nur auf tot, damit
            ** er keine Messages mehr sendet und empfaengt und gehe dann
            ** in eine Warteschleife, die von HandleNetworkTrouble bearbei-
            ** tet wird, um dem Spieler noch mal kurz zu zeigen, dass er im
            ** arsch ist.
            ** ----------------------------------------------------------*/
            rp = (struct ypamessage_removeplayer *)( rm->data );
            size = sizeof( struct ypamessage_removeplayer );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /*** daten merken ***/
            ywd->gsr->network_trouble_owner = rp->generic.owner;
            strcpy( ywd->gsr->network_trouble_name, rp->name );

            if( stricmp( rp->name, ywd->gsr->NPlayerName ) == 0 ) {
            
                /*** nur anschieben ***/
                ywd->gsr->player[ owner ].was_killed = TRUE;
                ywd->gsr->dont_send                  = TRUE;
                
                ywd->gsr->network_trouble       = NETWORKTROUBLE_KICKOFF_YOU;
                ywd->gsr->network_trouble_count = KICKOFF_YOU_COUNT;
                }
            else {
                        
                /*** Dreck wegräumen ***/
                yw_RemovePlayerStuff( ywd, NULL, rp->generic.owner, 1 );
    
                /* ----------------------------------------------------
                ** Abknallen.
                ** Auch wenn der Wirtsrechner eine Message an alle noch
                ** einmal losschickt, das ist egal, weil keiner weiß,
                ** ob er überhaupt noch ordnungsgemäß funktioniert.
                ** --------------------------------------------------*/
                yw_DestroyPlayer( ywd, rp->name );
                
                if( NETWORKTROUBLE_KICKOFF_YOU != ywd->gsr->network_trouble ) {
                
                    ywd->gsr->network_trouble       = NETWORKTROUBLE_KICKOFF_PLAYER;
                    ywd->gsr->network_trouble_count = KICKOFF_PLAYER_COUNT;
                    }
                }

            /*** Wenn eigener Player-> Spiel beenden ***/
            if( stricmp( rp->name, ywd->gsr->NPlayerName ) == 0 ) 
                yw_NetLog(">>> I was kicked off by the host! (time %d)\n",
                           ywd->TimeStamp/1000);
            else 
                yw_NetLog(">>> Host told me he has kicked off %s (time %d)\n",
                           rp->name, ywd->TimeStamp/1000 );
 
            break;

        case YPAM_GEM:

            /* ---------------------------------------------------
            ** Es hat jemand ein gem erobert. Folglich muß ich bei
            ** mir das zugehörige vehicle wieder ausschalten und
            ** den Wunderstein wieder aktivieren, damit ich das
            ** KnowHow zurückerobern kann. Die Vehiclenummer er-
            ** mittle ich über meinen Owner und der Basisnummer
            ** im Wunderstein, die die Vehiclenummer des User ist.
            ** -------------------------------------------------*/
            gm = (struct ypamessage_gem *)( rm->data );
            size = sizeof( struct ypamessage_gem );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            gem = &(ywd->gem[ gm->gemoffset ]);
            sec = &(ywd->CellArea[ gem->sec_y * ywd->MapSizeX + gem->sec_x ]);

            /*** Prototypen ermitteln ***/
            yw_GetNetGemProtos( ywd, gem, &vproto, &bproto );

            /* -----------------------------------------------------
            ** Vehicle wegschalten, wenn dies wirklich gefordert war
            ** (wir können die Nummern erstmal pauschal ermitteln.
            ** Wenn irgendwas 0 ist, gibt es keine Änderung!)
            ** ---------------------------------------------------*/
            if( vproto )
                ywd->VP_Array[ vproto ].FootPrint  = 0;

            if( bproto )
                ywd->BP_Array[ bproto ].FootPrint  = 0;

            /*** Wunderstein wieder einschalten ***/
            if( sec->WType != WTYPE_Wunderstein ) {

                /*** Evtl. noch Message ausgeben ***/
                strcpy(text,ypa_GetStr(ywd->LocHandle,STR_LMSG_TECH_LOST,"TECH-UPGRADE LOST! "));
                strcat(text,gem->msg);
                log.bact = NULL;
                log.pri  = 80;
                log.msg  = text;
                log.code = LOGMSG_TECH_LOST;
                _methoda(ywd->world,YWM_LOGMSG,&log);
                }

            /* --------------------------------------------------------------
            ** Wenn enable eingeschalten ist, dann schalten wir den WS wieder
            ** ein, was normal ist, andernfalls wird er ausgeschalten, weil
            ** z.B. das gem zamgebombt ist.
            ** ------------------------------------------------------------*/
            if( gm->enable ) {

                sec->WType  = WTYPE_Wunderstein;
                sec->WIndex = gm->gemoffset;
                }
            else {

                sec->WType  = WTYPE_None;
                sec->WIndex = 0;
                }

            break;

        case YPAM_RACE:

            /* --------------------------------------------------
            ** Der Absender hat eine Wahl bezüglich der rasse
            ** getroffen. Ich muß seine alte wieder als anwählbar
            ** schalten und seine neue disablen
            ** ------------------------------------------------*/
            rcm = (struct ypamessage_race *)( rm->data );
            size = sizeof( struct ypamessage_race );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            ywd->gsr->FreeRaces &= ~(rcm->newrace);
            ywd->gsr->FreeRaces |=  (rcm->freerace);

            /* ------------------------------------------------------------
            ** Dieser hat noch keinen Eigentümer. Ich merke mir aber seine
            ** Rasse in einem Array, in welchem die interne Nummer offset
            ** ist, um später in PlaceRobos den richtigen Eigentümer ermit-
            ** teln zu können.
            ** ----------------------------------------------------------*/
            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( stricmp( rm->sender_id, gpd.name ) == 0 ) {

                    ywd->gsr->player2[ gpd.number ].race = rcm->newrace;
                    break;
                    }
                gpd.number++;
                }

            break;

        case YPAM_WELCOME:

            /* ------------------------------------------------------
            ** Wenn ein Spieler sich anmeldet, so können bereits
            ** Daten vergeben sein. Deshalb schickt jeder spieler,
            ** sobald er ein CREATEPLAYER empfängt, eine Message
            ** namens WELCOME NUR(!) an den neuen Spieler, in welcher
            ** er daten mitteilt, wie z.B. seine rasse. Der neue kann
            ** nun darauf reagieren und sich anpassen.
            ** ----------------------------------------------------*/
            wm = (struct ypamessage_welcome *)( rm->data );
            size = sizeof( struct ypamessage_welcome );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            /*** Den Absender bei mir suchen ***/
            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( stricmp( rm->sender_id, gpd.name ) == 0 ) {

                    ywd->gsr->player2[ gpd.number ].race           = wm->myrace;
                    ywd->gsr->player2[ gpd.number ].ready_to_start = wm->ready_to_start;
                    ywd->gsr->player2[ gpd.number ].welcomed       = TRUE;
                    ywd->gsr->player2[ gpd.number ].cd             = wm->cd;
                    break;
                    }
                gpd.number++;
                }   
                
            /*** Im Lobbymodus werte ich die rasseninfos nicht aus ***/
            //if( ywd->gsr->remotestart )
            //    break;    

            /*** Seine rasse bei mir disablen ***/
            ywd->gsr->FreeRaces &= ~(wm->myrace);

            /* ------------------------------------------------------
            ** Wenn die rasse des anderen meiner Rasse entspricht,
            ** muß ich meine ändern, weil ich neu bin. Aber nur dann!
            ** ----------------------------------------------------*/
            if( wm->myrace == ywd->gsr->SelRace ) {

                if( ywd->gsr->FreeRaces & FREERACE_USER )
                    ywd->gsr->SelRace = FREERACE_USER;
                else
                    if( ywd->gsr->FreeRaces & FREERACE_KYTERNESER )
                        ywd->gsr->SelRace = FREERACE_KYTERNESER;
                    else
                        if( ywd->gsr->FreeRaces & FREERACE_MYKONIER )
                            ywd->gsr->SelRace = FREERACE_MYKONIER;
                        else
                            ywd->gsr->SelRace = FREERACE_TAERKASTEN;
                }

            /* ------------------------------------------------------
            ** Diese rasse allen mitteilen, freerace == 0 heißt, daß
            ** woanders nichts eingeschalten wird, weil ja keine frei
            ** wird.
            ** ----------------------------------------------------*/
            ywd->gsr->FreeRaces    &= ~(ywd->gsr->SelRace);
            rcm2.freerace           = 0;
            rcm2.newrace            = ywd->gsr->SelRace;
            rcm2.generic.owner      = ywd->gsr->NPlayerOwner;
            rcm2.generic.message_id = YPAM_RACE;

            sm.data          = &rcm2;
            sm.data_size     = sizeof( rcm2 );
            sm.receiver_kind = MSG_ALL;
            sm.receiver_id   = NULL;
            sm.guaranteed    = TRUE;
            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

            /*** Sind noch weitere Initdaten zu verarbeiten? ***/

            break;

        case YPAM_READYTOSTART:

            /* -----------------------------------------------
            ** Der Absender hat eine Wahl getroffen, ob es von
            ** ihm aus losgehen kann (ready_to_start = 1) oder
            ** ob er noch warten will. Ausgewertet wird es nur
            ** vom Host, aber der kann sich ja ändern
            ** ---------------------------------------------*/
            rts = (struct ypamessage_readytostart *)( rm->data );
            size = sizeof( struct ypamessage_readytostart );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( stricmp( rm->sender_id, gpd.name ) == 0 ) {

                    ywd->gsr->player2[ gpd.number ].ready_to_start =
                         rts->ready_to_start;
                    break;
                    }
                gpd.number++;
                }

            break;

        case YPAM_REQUESTUPDATE:

            /* ----------------------------------------------------------
            ** Ich bastle eine Kopie meiner gesamten Geschwaderstruktur
            ** zusammen mit allen Infos, aus denen sich auf einer anderen
            ** Maschine ein Abbild erstellen läßt.
            ** --------------------------------------------------------*/
            ru = (struct ypamessage_requestupdate *)( rm->data );
            size = sizeof( struct ypamessage_requestupdate );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            yw_SendGameCopy( ywd, ywd->gsr->NPlayerOwner, rm->sender_id );
            break;

        case YPAM_UPDATE:

            /* --------------------------------------------------------------
            ** Ein Update. Das heißt, ich muß von diesem alle Schatten killen
            ** und neu erzeugen
            ** ------------------------------------------------------------*/
            upd  = (struct ypamessage_update *)( rm->data );
            size = upd->basic.size;
            if( ywd->gsr->player[ owner ].was_killed ) {
                yw_NetLog("\n+++UPD: got update from DEAD %s (%d)", rm->sender_id, ywd->TimeStamp/1000 );
                player_data2->waiting_for_update = 0;
                break; }

            /*** Evtl. Abbruch, wenn FALSE zurückkommt ***/
            if( !yw_RestoreShadows( ywd, upd, upd->generic.owner ) ) {

                yw_NetLog("DRAMATIC RESTORE ERROR. Couldn't restore data from owner %d\n",
                         upd->generic.owner );
                }
            else
                yw_NetLog("Received Update, tried successfully to restore vehicle structure\n");

            /*** Egal, wie es gelaufen ist, Bearbeitung melden ***/
            break;

        case YPAM_IMPULSE:

            /* ------------------------------------------------------------------
            ** Das Problem ist folgendes: Impulse wurden bisher auch auf Schatten
            ** angewendet und das Original wußte nichts davon. Das führte zu
            ** unschönen Effekten. Deshalb bekommt im falle eines Impulses
            ** jeder eine Message und wendet dann Impulse auf Objekte SEINES
            ** OWNERS an!.
            ** ----------------------------------------------------------------*/
            imp  = (struct ypamessage_impulse *)( rm->data );
            size = sizeof( struct ypamessage_impulse );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /* ----------------------------------------------------------
            ** nun auf alle Objekte des Sektors der übergebenen Position,
            ** die meinen owner haben, einen Impulse anwenden
            ** --------------------------------------------------------*/
            gsi.abspos_x = imp->pos.x;
            gsi.abspos_z = imp->pos.z;
            if( _methoda( ywd->world, YWM_GETSECTORINFO, &gsi ) ) {

                struct Bacterium *kandidat;
                struct impulse_msg impulse;

                impulse.pos       = imp->pos;
                impulse.impulse   = imp->impulse;
                impulse.miss      = imp->miss;
                impulse.miss_mass = imp->miss_mass;

                kandidat = (struct Bacterium *) gsi.sector->BactList.mlh_Head;
                while( kandidat->SectorNode.mln_Succ ) {

                    if( (kandidat->BactClassID != BCLID_YPAMISSY) &&
                        (kandidat->BactClassID != BCLID_YPAROBO) &&
                        (kandidat->BactClassID != BCLID_YPATANK) &&
                        (kandidat->BactClassID != BCLID_YPACAR)  &&
                        (kandidat->BactClassID != BCLID_YPAGUN)  &&
                        (kandidat->BactClassID != BCLID_YPAHOVERCRAFT) &&
                        (!(kandidat->ExtraState & EXTRA_MEGADETH)) &&
                        (kandidat->Owner       == ywd->URBact->Owner) )

                        _methoda( kandidat->BactObject, YBM_IMPULSE, &impulse );

                    kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
                    }
                }

            break;

        case YPAM_LOGMSG:

            /*** Schickt einfach nur eine LogMsg los ***/
            lm   = (struct ypamessage_logmsg *)( rm->data );
            size = sizeof( struct ypamessage_logmsg );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            if( !(robo = yw_GetRoboByOwner( ywd, lm->senderowner ))) {

                yw_NetLog("\n+++ RD: Havent found robo with owner %d (%ds)\n",
                         lm->senderowner ,ywd->TimeStamp/1000 );
                break;
                }

            /* ----------------------------------------------------------
            ** Ich werte die message nur aus, wenn senderowner auf meiner
            ** Maschine der UserRobo ist. Gibt es da Ausnahmen??????
            ** --------------------------------------------------------*/
            if( ywd->UserRobo == robo->o ) {

                /* ---------------------------------------------------
                ** Es muß nicht unbedingt ein Sender übergeben werden.
                ** Entscheidend ist der Owwner
                ** -------------------------------------------------*/
                if( lm->sender ) {
                    blog.sender = yw_GetBactByID( robo->bact, lm->sender);
                    if( !blog.sender )
                        yw_NetLog("\n+++ LM: sender %d of owner %d not found (message %d)\n",
                                 lm->sender, lm->senderowner, lm->ID );
                    }
                else
                    blog.sender = NULL;

                blog.pri     = lm->pri;
                blog.ID      = lm->ID;
                blog.para1   = lm->para1;
                blog.para2   = lm->para2;
                blog.para3   = lm->para3;
                _methoda( robo->o, YRM_LOGMSG, &blog );
                }

            break;

        case YPAM_NEWORG:

            norg = (struct ypamessage_neworg *)( rm->data );
            size = norg->basic.size;
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;

            /* ---------------------------------------------------------------
            ** Neue organize-message. Diese funktioniert ganz einfach: Bei
            ** diversen Org-Sachen passiert doch folgendes: Ein neuer
            ** Commander wird ausgewählt und bekommt neue Slaves. Ich gehe mal
            ** davon aus, daß jede Umstrukturierung sich so darstellen läßt:
            ** "Du wirst Commander und bekommst die Slaves", auch wenn
            ** wir hier Arbeit doppelt machen (wenn nur ein Slave angekettet
            ** wird) oder 2 Messages losgeschickt werden müssen (wenn vorher
            ** aus Resten noch ein geschwader zusammengesetzt wird).
            ** -------------------------------------------------------------*/

            robo = yw_GetRoboByOwner( ywd, norg->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ NORG: Havent found robo with owner %d (%ds)\n",
                         norg->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }
            else {

                struct Bacterium *com, *slave;
                WORD   vnumber;

                vnumber = yw_GetVehicleNumber( robo->bact );

                /* --------------------------------------------------------
                ** Wegen Umschichtproblemen verwende ich hier eine Routine,
                ** die 3 Ebenen untersucht
                ** ------------------------------------------------------*/
                com = yw_GetBactByID_3( robo->bact, norg->basic.commander );
                if( com ) {

                    int i;

                    com->CommandID = norg->basic.command_id;

                    /*** Zum Commander machen ***/
                    _methoda( robo->o, YBM_ADDSLAVE, com->BactObject );

                    /* -------------------------------------------
                    ** Slaves anhängen, die auch mal unter "neuen"
                    ** Slaves hängen können
                    ** -----------------------------------------*/
                    for( i = 0; i < norg->basic.num_slaves; i++ ) {

                        slave = yw_GetBactByID_3( robo->bact, norg->slaves[ i ]);
                        if( slave )
                            _methoda( com->BactObject, YBM_ADDSLAVE, slave->BactObject);
                        else
                            yw_NetLog("\n+++ NORG: Slave %d of Owner %d not found (%ds)\n",
                                     norg->slaves[ i ], owner, ywd->TimeStamp/1000);
                        }

                    // ADDSLAVE ändert sehr wohl
                    // if( vnumber != yw_GetVehicleNumber( robo->bact ) ) {
                    //     yw_NetLog("\n+++ NORG: Vehiclecount changed with mode %d! (%ds)\n",
                    //              norg->modus ,ywd->TimeStamp/1000);
                    //     yw_NetLog("          Commander has class %d\n", com->BactClassID);
                    //     strcpy( trouble_maker, rm->sender_id );
                    //     }
                    }
                else 
                    yw_NetLog("\n+++ NORG: Commander %d of owner %d not found (%ds)\n",
                            norg->basic.commander, owner, ywd->TimeStamp/1000 );
                }

            break; 
            
        case YPAM_LOBBYINIT:

            li = (struct ypamessage_lobbyinit *)( rm->data );
            size = sizeof( struct ypamessage_lobbyinit );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /*** Die levelnummer mitteilen ***/
            ywd->gsr->NLevelOffset = li->levelnum;
            ywd->gsr->NLevelName = ypa_GetStr( GlobalLocaleHandle,
                                   STR_NAME_LEVELS + ywd->gsr->NLevelOffset,
                                   ywd->LevelNet->Levels[ ywd->gsr->NLevelOffset ].title);

            /*** Host auf ready schalten ***/
            gpd.number = 0;            
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {
                 
                /*** Nicht ganz wasserdicht...***/
                if( stricmp( gpd.name, li->hostname)==0) {
                     
                    ywd->gsr->player2[ gpd.number ].ready_to_start = 1;
                    break;
                    }
                
                gpd.number++;
                } 
                
            /* -------------------------------------------------------
            ** Rasseneinstellung ist im LobbyMode starr, weil alle
            ** zur gleichen Zeit initialisiert werden, klappt das 
            ** WELCOME-System nicht (z.T. ist gehen Msgs verloren, 
            ** weil noch kein Empfaenger existiert). 
            ** Deshalb wird alles Starr eingestellt. Dabei schalten
            ** wir alle nicht unterstuetzten rassen und die, die von
            ** Spielern belegt werden aus. Als x-ter Spieler nehme
            ** ich mir die x-te freie Rasse.  
            ** -----------------------------------------------------*/
            {
                ULONG  plys;
                struct LevelNode *l;
                l = &(ywd->LevelNet->Levels[ li->levelnum ]);
                plys  = _methoda( ywd->nwo, NWM_GETNUMPLAYERS, NULL);
            
                gpd.number  = 0;            
                gpd.askmode = GPD_ASKNUMBER;

                if( l->races & 2 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_USER;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_USER;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace =  FREERACE_USER;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_USER;         
                                 
                if( l->races & 64 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_KYTERNESER;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_KYTERNESER;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace    =  FREERACE_KYTERNESER;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_KYTERNESER;         
                                 
                if( l->races & 8 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_MYKONIER;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_MYKONIER;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace    =  FREERACE_MYKONIER;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_MYKONIER;         
                                 
                if( l->races & 16 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_TAERKASTEN;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_TAERKASTEN;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace    =  FREERACE_TAERKASTEN;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_TAERKASTEN;         
                }                 
                                         
            break;

        case YPAM_CHANGELEVEL:

            /* -------------------------------------------------
            ** Verhaelt sich fast wie Lobbyinit, weil ebenfalls
            ** ein Level eingestellt wird und rassen ausgewaehlt
            ** werden muessen
            ** -----------------------------------------------*/
            cl = (struct ypamessage_changelevel *)( rm->data );
            size = sizeof( struct ypamessage_changelevel );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /*** Die levelnummer mitteilen ***/
            ywd->gsr->NLevelOffset = cl->levelnum;
            ywd->gsr->NLevelName = ypa_GetStr( GlobalLocaleHandle,
                                   STR_NAME_LEVELS + ywd->gsr->NLevelOffset,
                                   ywd->LevelNet->Levels[ ywd->gsr->NLevelOffset ].title);
           
            /* -------------------------------------------------------
            ** Rasseneinstellung wie im LobbyMode starr, weil alle
            ** zur gleichen Zeit initialisiert werden, klappt das 
            ** WELCOME-System nicht (z.T. ist gehen Msgs verloren, 
            ** weil noch kein Empfaenger existiert). 
            ** -----------------------------------------------------*/
            {
                ULONG  plys;
                struct LevelNode *l;
                l = &(ywd->LevelNet->Levels[ cl->levelnum ]);
                plys  = _methoda( ywd->nwo, NWM_GETNUMPLAYERS, NULL);
            
                gpd.number  = 0;            
                gpd.askmode = GPD_ASKNUMBER;

                if( l->races & 2 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_USER;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_USER;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace =  FREERACE_USER;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_USER;         
                                 
                if( l->races & 64 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_KYTERNESER;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_KYTERNESER;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace    =  FREERACE_KYTERNESER;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_KYTERNESER;         
                                 
                if( l->races & 8 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_MYKONIER;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_MYKONIER;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace    =  FREERACE_MYKONIER;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_MYKONIER;         
                                 
                if( l->races & 16 ) { 
                    if( plys > 0 ) {
                        ywd->gsr->FreeRaces &= ~FREERACE_TAERKASTEN;
                        ywd->gsr->player2[ gpd.number ].race = FREERACE_TAERKASTEN;
                        _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd );
                        if( stricmp( gpd.name, ywd->gsr->NPlayerName) == 0) {
                            
                            /*** das bin ich ***/
                            ywd->gsr->SelRace    =  FREERACE_TAERKASTEN;
                            }
                        gpd.number++;
                        plys--;
                        }
                    }    
                else  ywd->gsr->FreeRaces &= ~FREERACE_TAERKASTEN;         
                }  
                
            /*** ein neuer Level? na, da berechnen wir doch gleich mal die CS neu ***/               
            yw_SendCheckSum( ywd, ywd->gsr->NLevelOffset );

            break;

            
        case YPAM_STARTPLASMA:

            sp = (struct ypamessage_startplasma *)( rm->data );
            size = sizeof( struct ypamessage_startplasma );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;
            
            /*** setzt bei diesem den plasme-Visproto ***/
            
            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, sp->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ SP: Havent found robo with owner %d (%ds)\n",
                         sp->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            dv = yw_GetBactByID( robo->bact, sp->ident);
            if( dv == NULL ) {
                yw_NetLog("\n+++ SP: vehicle %d doesn't exist\n", sp->ident);
                break;
                }
                
            dv->scale_time           = sp->time;
            dv->extravp[0].scale     = sp->scale;
            dv->extravp[0].pos       = sp->pos; 
            dv->extravp[0].dir       = sp->dir; 
                
            dv->extravp[0].vis_proto = dv->vis_proto_create;
            dv->extravp[0].vp_tform  = dv->vp_tform_create;
            dv->extravp[0].flags    |= (EVF_Scale | EVF_Active);

            dv->extravp_logic        = EVLF_PLASMA;
                
            break; 
            
        case YPAM_ENDPLASMA:

            ep = (struct ypamessage_endplasma *)( rm->data );
            size = sizeof( struct ypamessage_endplasma );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;
            
            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, ep->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ EP: Havent found robo with owner %d (%ds)\n",
                         ep->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }
                
            /*** nur beim original, wie VehicleEnergy ***/
            if( ywd->UserRobo != robo->o ) break;    

            dv = yw_GetBactByID( robo->bact, ep->ident);
            if( dv == NULL ) {
                yw_NetLog("\n+++ EP: vehicle %d doesn't exist\n", ep->ident);
                break;
                } 
                
            dv->scale_time    = -1;
            dv->extravp_logic =  0;
            break;
          
        
        case YPAM_STARTBEAM:

            stb = (struct ypamessage_startbeam *)( rm->data );
            size = sizeof( struct ypamessage_startbeam );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, stb->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ STB: Havent found robo with owner %d (%ds)\n",
                         stb->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }

            /*** Robospezifische daten ***/
            yrd = INST_DATA( ((struct nucleusdata *)robo->o)->o_Class, robo->o);
            yrd->BeamPos    = stb->BeamPos;
            yrd->BeamInTime = BEAM_IN_TIME;
            
            _StartSoundSource( &(yrd->bact->sc), VP_NOISE_BEAMIN );

            robo->bact->extravp_logic = EVLF_BEAM;
                
            break; 
            
        case YPAM_ENDBEAM:

            eb = (struct ypamessage_endbeam *)( rm->data );
            size = sizeof( struct ypamessage_endbeam );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, eb->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ EP: Havent found robo with owner %d (%ds)\n",
                         eb->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }
                
            robo->bact->extravp[0].flags = 0;
            robo->bact->extravp[1].flags = 0;
            robo->bact->extravp_logic    = 0;
            break;
            
        case YPAM_EXTRAVP:
        
            /*** Schaltet die visproto der extravp-Struktur ***/
            evp = (struct ypamessage_extravp *)( rm->data );
            size = sizeof( struct ypamessage_extravp );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            if( player_data2 && player_data2->waiting_for_update ) break;
            
            /*** Robo suchen .... ***/
            robo = yw_GetRoboByOwner( ywd, evp->generic.owner );

            if( !robo ) {

                yw_NetLog("\n+++ EVP: Havent found robo with owner %d (%ds)\n",
                         evp->generic.owner ,ywd->TimeStamp/1000 );
                break;
                }
                
            dv = yw_GetBactByID( robo->bact, evp->ident);
            if( dv == NULL ) {
                yw_NetLog("\n+++ EP: vehicle %d doesn't exist\n", evp->ident);
                break;
                }
                
            for( k = 0; k < evp->numvp; k++ ) {
            
                if( evp->vp[ k ].flags & EVF_Active ) {
                
                    /*** Einschalten. kopieren... ***/
                    dv->extravp[ k ] = evp->vp[ k ];
                    
                    /*** ...und VP nachkorrigieren ***/
                    switch( (LONG)dv->extravp[ k ].vis_proto ) {

                        case VP_JUSTNORMAL:
                            dv->extravp[ k ].vis_proto = dv->vis_proto_normal;
                            dv->extravp[ k ].vp_tform  = dv->vp_tform_normal;
                            break;

                        case VP_JUSTWAIT:
                            dv->extravp[ k ].vis_proto = dv->vis_proto_wait;
                            dv->extravp[ k ].vp_tform  = dv->vp_tform_wait;
                            break;

                        case VP_JUSTDEATH:
                            dv->extravp[ k ].vis_proto = dv->vis_proto_dead;
                            dv->extravp[ k ].vp_tform  = dv->vp_tform_dead;
                            break;

                        case VP_JUSTMEGADETH:
                            dv->extravp[ k ].vis_proto = dv->vis_proto_megadeth;
                            dv->extravp[ k ].vp_tform  = dv->vp_tform_megadeth;
                            break;

                        case VP_JUSTFIRE:
                            dv->extravp[ k ].vis_proto = dv->vis_proto_fire;
                            dv->extravp[ k ].vp_tform  = dv->vp_tform_fire;
                            break;

                        case VP_JUSTCREATE:
                            dv->extravp[ k ].vis_proto = dv->vis_proto_create;
                            dv->extravp[ k ].vp_tform  = dv->vp_tform_create;
                            break;

                        default: yw_NetLog("\n+++ EVP: Unknown VP %d for %d of class %d at pos %d\n",
                            (LONG)dv->extravp[ k ].vis_proto, dv->ident, dv->BactClassID, k);
                            break;
                        }
                    }
                else {
                
                    /*** nur ausschalten ***/
                    dv->extravp[ k ].flags = 0;
                    }
                } 

            break;        

        case YPAM_ANNOUNCEQUIT:

            /* -------------------------------------------------------
            ** Diese message gibt an, dass die Folgende DESTROYPLAYER
            ** Message Folge eines normalen verlassens und nicht eines
            ** Rausschmisses ist.
            ** -----------------------------------------------------*/
            aq = (struct ypamessage_announcequit *)( rm->data );
            size = sizeof( struct ypamessage_announcequit );
            if( ywd->gsr->player[ owner ].was_killed ) break;

            yw_NetLog(">>> received ANNOUNCEQUIT from %s at %d\n",
                       rm->sender_id, ywd->TimeStamp/1000 );

            ywd->gsr->player[ owner ].status = NWS_LEFTGAME;
            break;
            
        case YPAM_CHECKSUM:

            /* -------------------------------------------------------
            ** Die Checksumme allewr daten zu diesem spieler und level
            ** -----------------------------------------------------*/
            cs = (struct ypamessage_checksum *)( rm->data );
            size = sizeof( struct ypamessage_checksum );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( stricmp( rm->sender_id, gpd.name ) == 0 ) 
                    ywd->gsr->player2[ gpd.number ].checksum = cs->checksum;
                gpd.number++;
                }
                
            /*** damit Message sofort ausgegeben wird ***/
            ywd->gsr->tacs_time = 0;
            break;
            
        case YPAM_REQUESTLATENCY:
        
            /*** nur beantworten ***/
            rl = (struct ypamessage_requestlatency *)( rm->data );
            size = sizeof( struct ypamessage_requestlatency );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            sal.generic.owner      = ywd->gsr->NPlayerOwner;
            sal.generic.message_id = YPAM_ANSWERLATENCY;
            sal.time_stamp         = rl->time_stamp;
 
            sm.data          = &sal;
            sm.data_size     = sizeof( sal );
            sm.receiver_kind = MSG_PLAYER;
            sm.receiver_id   = (char *)(rm->sender_id);
            sm.guaranteed    = TRUE;
            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

            if( 3 > ywd->TLMsg.frame_time ) {
                
                struct flushbuffer_msg fb;
   
                /*** Wegen ausgebremster Frametime flushen ***/
                fb.sender_kind   = MSG_PLAYER;
                fb.sender_id     = ywd->gsr->NPlayerName;
                fb.receiver_kind = MSG_ALL;
                fb.receiver_id   = NULL;
                fb.guaranteed    = FALSE;   // weil ingame-zeug, keine Initdaten
                _methoda( ywd->nwo, NWM_FLUSHBUFFER, &fb );
                }
            
            break;
            
        case YPAM_ANSWERLATENCY:
        
            al = (struct ypamessage_answerlatency *)( rm->data );
            size = sizeof( struct ypamessage_answerlatency );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /* -----------------------------------------------------------
            ** Um einen Pseudodurchschnitt zu bekommen, veraendern wir den
            ** bisherigen Wert so, als ob er zu 80% reinhaut. Dann sieht es
            ** so aus, als haetten wir die letzten 4 gemerkt.
            ** ----------------------------------------------------------*/
            if( ywd->gsr->player[ owner ].latency )
                ywd->gsr->player[ owner ].latency = 
                    ((4 * ywd->gsr->player[ owner ].latency) + (ywd->TimeStamp - al->time_stamp))/5;              
            else
                ywd->gsr->player[ owner ].latency = ywd->TimeStamp - al->time_stamp;
            
            /* ------------------------------------------------------------------------
            ** Auswertung. Wenn ich Host bin, dann darf ich das Spiel anhalten
            ** und warten, bis es besser geht. Dazu setze ich das Flag network_trouble,
            ** welches in NetMessageLoop ausgewertet wird, sende eine Message an alle,
            ** die Besagt, dass es sich um ein Problem Handelt und sie auch in diesen
            ** Troublestatus zu gehen haben (damit sie nicht mehr senden) und ausser-
            ** dem muessen wir noch ne Meldung auf den Bildschirm bringen.
            ** ----------------------------------------------------------------------*/
            if( (ywd->gsr->is_host) &&
                (ywd->gsr->network_trouble != NETWORKTROUBLE_LATENCY) && 
                (ywd->gsr->player[ owner ].latency >= MAX_LATENCY) ) {
                
                struct ypamessage_starttrouble nwt;
                
                ywd->gsr->network_trouble       = NETWORKTROUBLE_LATENCY;
                ywd->gsr->network_trouble_count = LATENCY_COUNT_HOST;
                ywd->gsr->latency_check         = 5;
                               
                nwt.generic.owner      = ywd->gsr->NPlayerOwner;
                nwt.generic.message_id = YPAM_STARTTROUBLE;
                nwt.trouble            = NETWORKTROUBLE_LATENCY;
                
                sm.data                = &nwt;
                sm.data_size           = sizeof( nwt );
                sm.receiver_kind       = MSG_ALL;
                sm.receiver_id         = NULL;
                sm.guaranteed          = TRUE;
                _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                }
            break;
           
        case YPAM_STARTTROUBLE:
        
            st = (struct ypamessage_starttrouble *)( rm->data );
            size = sizeof( struct ypamessage_starttrouble );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            /*** probleme haben angefangen ***/
            ywd->gsr->network_trouble = st->trouble;
            
            switch( st->trouble ) {
            
                case NETWORKTROUBLE_LATENCY:
                                    
                    ywd->gsr->network_trouble_count = LATENCY_COUNT_CLIENT;
                    break;
                }
            break;
           
        case YPAM_ENDTROUBLE:
        
            et = (struct ypamessage_endtrouble *)( rm->data );
            size = sizeof( struct ypamessage_endtrouble );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            ywd->gsr->network_allok       = et->reason;
            ywd->gsr->network_allok_count = ENDTROUBLE_COUNT;
            
            /*** Warum eigentlich? ***/
            switch( et->reason ) {
            
                case ENDTROUBLE_ALLOK:
                
                    break;
                    
                case ENDTROUBLE_TIMEDOUT:
                
                    break;
                    
                case ENDTROUBLE_UNKNOWN:
                
                    ywd->gsr->network_allok_count = 0;
                    break;    
                }
            
            /*** probleme haben aufgehoert ***/
            ywd->gsr->network_trouble = NETWORKTROUBLE_NONE;
            break;
            
        case YPAM_CD:
            
            cdm = (struct ypamessage_cd *)( rm->data );
            size = sizeof( struct ypamessage_cd );
            if( ywd->gsr->player[ owner ].was_killed ) break;
            
            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( stricmp( rm->sender_id, gpd.name ) == 0 ) {
                    ywd->gsr->player2[ gpd.number ].cd = cdm->cd;
                    break;
                    }
                gpd.number++;
                }   

            break;
            
        default:

            /*** Eine unbekannte Message ***/
            yw_NetLog("Warning!! Unknown message ID %d from owner %d/sender %s at %ds\n",
                     kind, owner, rm->sender_id, ywd->TimeStamp/1000 );
            size = 0;
            break;
        }
        
    return( size );
}


void yw_AddMessageToBuffer( struct ypaworld_data *ywd, char *sender, char *text )
{
    /*** Name aendern? ***/
    if( stricmp( sender, ywd->gsr->lastsender ) != 0 ) {

        if( ywd->gsr->act_messageline >= (MAXNUM_MESSAGELINES-1)) {

            int i;

            /*** verschieben ***/
            ywd->gsr->act_messageline = MAXNUM_MESSAGELINES-1;

            for( i = 0; i < (MAXNUM_MESSAGELINES-1); i++ )
                strcpy( ywd->gsr->messagebuffer[i], ywd->gsr->messagebuffer[i+1]);
            }

        memset(  ywd->gsr->messagebuffer[ ywd->gsr->act_messageline ],0,STANDARD_NAMELEN);
        sprintf( ywd->gsr->messagebuffer[ ywd->gsr->act_messageline ],
                 "> %s:", sender);
        memset(  ywd->gsr->lastsender,0,STANDARD_NAMELEN);
        strncpy( ywd->gsr->lastsender,
                 sender, STANDARD_NAMELEN-1);

        ywd->gsr->act_messageline++;
        }

    /*** In Messagebuffer aufnehmen ***/
    if( ywd->gsr->act_messageline >= (MAXNUM_MESSAGELINES-1)) {

        int i;

        /*** verschieben ***/
        ywd->gsr->act_messageline = MAXNUM_MESSAGELINES-1;

        for( i = 0; i < (MAXNUM_MESSAGELINES-1); i++ )
            strcpy( ywd->gsr->messagebuffer[i], ywd->gsr->messagebuffer[i+1]);
        }

    memset(  ywd->gsr->messagebuffer[ ywd->gsr->act_messageline ],0,STANDARD_NAMELEN);
    strncpy( ywd->gsr->messagebuffer[ ywd->gsr->act_messageline ],
             text, STANDARD_NAMELEN-1 );

    ywd->gsr->act_messageline++;

    if( ywd->gsr->n_selmode == NM_MESSAGE ) {
        ywd->gsr->nmenu.FirstShown = max( 0,
            ywd->gsr->act_messageline-(NUM_NET_SHOWN-MAXNUM_PLAYERS-2));
        ywd->gsr->nmenu.NumEntries = ywd->gsr->act_messageline;
        ywd->gsr->nmenu.ShownEntries = min( ywd->gsr->nmenu.NumEntries,
            (NUM_NET_SHOWN - MAXNUM_PLAYERS-2));
        }
}

#endif
