/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  c Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypavehicles.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"

#ifdef __NETWORK__
#include "network/networkflags.h"
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

#define SECTOR_ENERGY_FACTOR    0.2     // Anteil der beachtenswerten 
                                        // Sektorenergie


/*** Globale Variable CommandCount ***/
ULONG YPA_CommandCount;         // 0 heißt kein Geschwader!!!!!!!!
extern UBYTE **RoboLocaleHandle;


/*-----------------------------------------------------------------*/
struct OBNode *yr_AllocForce( struct yparobo_data *yrd, struct allocforce_msg *af);
void  yr_CheckCommander( struct yparobo_data *yrd, struct trigger_logic_msg *msg,
                         BOOL auto_only);
void  yr_RealizeWish( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_Service( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildDefense( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildConquer( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildReconnoitre( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildRadar( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildPower( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_ChangePlace( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildSafety( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_BuildRobo( struct yparobo_data *yrd, struct trigger_logic_msg *msg);
void  yr_InitForce(struct yparobo_data *yrd, struct OBNode *Commander);
void  yr_rot_round_global_y( struct yparobo_data *yrd, FLOAT angle );
LONG  yr_GetBestKraftWerk( struct yparobo_data *yrd, struct BuildProto *b_array );
LONG  yr_GetBestRadar( struct yparobo_data *yrd, struct BuildProto *b_array );
LONG  yr_GetBestSafety( struct yparobo_data *yrd, struct BuildProto *b_array );
void  yr_GetBestSectorToTarget( struct yparobo_data *yrd, UWORD pos,
                                struct settarget_msg *target );
BOOL  yr_GetBetterNeighbour( struct yparobo_data *yrd, struct settarget_msg *st);
void  yr_InitializeCommand( struct yparobo_data *yrd, struct OBNode *Commander );
FLOAT yr_BestClass( WORD enemy, WORD myclass );
FLOAT yr_TestVehicle( struct yparobo_data *yrd, APTR data, UBYTE kind,
                      ULONG job);
void yr_ClearBuildSlots( struct yparobo_data *yrd );
void yr_HistoryCreate( Object *world, struct Bacterium *bact );
BOOL yr_AreThereGroundVehicles( struct Bacterium *commander );
struct roboattacker *yr_GetFreeAttackerSlot( struct yparobo_data *yrd );
void yr_ControlRoboAttacker( struct OBNode *Commander, struct roboattacker *ra );
void yr_SetBactTarget( struct Bacterium *commander, struct Bacterium *targetbact );
void yr_SetSectorTarget( struct Bacterium *commander, FLOAT x, FLOAT z );
void yr_SwitchEscape( struct Bacterium *com, UBYTE wat_n_nu );


void yr_Service( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
/*
** Diese Routine macht so diverse Sachen...
*/

    /* ---------------------------------------------------------------
    ** Ist Dockuser noch da? Evtl. auch tot, wenn Umschichtung spaeter
    ** kommt
    ** -------------------------------------------------------------*/
    if( yrd->dock_user ) {

        struct OBNode *com;
        BOOL   f = FALSE;

        com = (struct OBNode *) yrd->bact->slave_list.mlh_Head;
        while( com->nd.mln_Succ ) {

            if( yrd->dock_user == com->bact->CommandID ) {
                f = TRUE;
                break;
                }
            com = (struct OBNode *) com->nd.mln_Succ;
            }

        if( !f ) {

            yrd->dock_user  = 0L;
            yrd->dock_time  = 0L;
            yrd->RoboState &= ~ROBO_DOCKINUSE;
            }
        }

    /*** Dockzeug ***/
    if( yrd->dock_time > 0 ) {

        yrd->dock_time -= msg->frame_time;

        /*** Unter 0? ***/
        if( yrd->dock_time <= 0 ) {

            yrd->dock_time  = 0;

            /*** War es das letzte Zählen? Ist also kein Nutzer mehr da? ***/
            if( yrd->dock_user == 0 ) {

                /*** alle Flags rücksetzen ***/
                yrd->RoboState &= ~ROBO_DOCKINUSE;
                }
            }
        }
}


void yr_CheckCommander( struct yparobo_data *yrd, struct trigger_logic_msg *msg, 
                        BOOL auto_only )
{
    /*
    ** Hier überwche ich alle Commander.
    ** Wenn es ihnen schlecht geht, kann ich sie zu anderen oder zum Robo 
    ** fliehen lassen oder sie verstärken.
    **
    ** Achtung, auto_only ist aus CompGruenden noch drin, bitte nicht mehr
    ** nutzen, weil ein UserRobo auch aus AI3 heraus aufrufen kann.
    */
    
    struct MinList *CommList, *SlaveList;
    struct OBNode  *Commander, *Slave;
    struct settarget_msg st;
    Object *user_robo;
    BOOL   USER_ROBO;
    struct bact_message log;
    struct howdoyoudo_msg how;

    /*** Ist das der Robo des Spielers ? ***/
    _get( yrd->world, YWA_UserRobo, &user_robo );
    if( user_robo == yrd->bact->BactObject )
        USER_ROBO = TRUE;
    else
        USER_ROBO = FALSE;

    CommList = &(yrd->bact->slave_list);
    Commander = (struct OBNode *) CommList->mlh_Head;

    while( Commander->nd.mln_Succ ) {

        /*
        ** Wenn er tot ist, dann nehme ich gleich den nächsten. Denn tote liegen
        ** ja noch etwas rum... 
        */
        if( Commander->bact->MainState == ACTION_DEAD ) {

            Commander = (struct OBNode *) Commander->nd.mln_Succ;
            continue;
            }

        /*** Ist er im Dock? ***/
        if( (Commander->bact->CommandID == yrd->dock_user) &&
            (yrd->dock_user != 0) ) {

            yr_InitForce( yrd, Commander );

            Commander = (struct OBNode *) Commander->nd.mln_Succ;
            continue;
            }

        /*** Kanonen mißachten ***/
        if( Commander->bact->BactClassID == BCLID_YPAGUN ) {

            Commander = (struct OBNode *) Commander->nd.mln_Succ;
            continue;
            }


        /*** Sind wir auf der Flucht? ***/
        if( Commander->bact->ExtraState & EXTRA_ESCAPE ) {

            /* ------------------------------------------------------------
            ** Entscheiden, ob es sinnvoll ist, die Flucht auszuschalten.
            ** Solange wir auf fremden Gebiet sind, machen wir nichts. Sind 
            ** wir aber auf eigenem Gebiet, so besteht die Möglichkeit, daß
            ** wir uns aufladen und wir können ein HDYD machen. 
            ** ----------------------------------------------------------*/


            /*** Ok, mal guckn ***/
            if( _methoda( Commander->o, YBM_HOWDOYOUDO, &how )) {

                /* ---------------------------------------------------------
                ** So, nun mal ernsthaft: Wenn wir auf eigenem Gebiet sind,
                ** welches auch energetisch versorgt wird, dann schalten wir
                ** die Flucht aus, ansonsten weiter zurück zum Robo!
                ** -------------------------------------------------------*/
                if( (Commander->bact->Owner == Commander->bact->Sector->Owner) &&
                    (Commander->bact->Sector->EnergyFactor > 0) ) {

                    /* -----------------------------------------------
                    ** Uns geht es besser. Ziel ausschalten. Weil wir
                    ** nicht ueber feindlichem Gebiet landen sollten,
                    ** setzen wir das Sektorziel. Dann ergibt sich der
                    ** Rest automatisch
                    ** ---------------------------------------------*/
                    st.target_type  = TARTYPE_SECTOR;
                    st.priority     = 0;
                    st.pos          = Commander->bact->pos;
                    _methoda( Commander->o, YBM_SETTARGET, &st );
                    }

                /*** Flucht weg ***/
                yr_SwitchEscape( Commander->bact , 0 );

                /*** Erholung bekanntgeben ***/
                if( USER_ROBO ) {

                    log.para1  = Commander->bact->CommandID;
                    log.para2  = log.para3 = 0;
                    log.ID     = LOGMSG_RELAXED;
                    log.sender = Commander->bact;
                    log.pri    = 28;
                    _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );
                    }
                }
            
            Commander = (struct OBNode *) Commander->nd.mln_Succ;
            continue;
            }


        /* -------------------------------------------------------------------
        ** Nun mal die Verhältnisse checken. Achtung! Wir flüchten, wenn es
        ** uns schlecht geht, das hat nichts mit der Stärke des Gegners zu
        ** tun, denn wir müssen ja auch Robos angreifen, sondern etwas mit der
        ** Moral in der Truppe. Deshalb machen wir auch kein CheckForceRe-
        ** lation mehr. 
        ** -----------------------------------------------------------------*/
        if( !_methoda( Commander->o, YBM_HOWDOYOUDO, &how) ) {

            /*** Rückzug wegen Schwäche ***/
            yr_SwitchEscape( Commander->bact, 1 );

            /*** Wegpunkte aus ***/
            Commander->bact->ExtraState &= ~(EXTRA_DOINGWAYPOINT|
                                             EXTRA_WAYPOINTCYCLE);
            Commander->bact->num_waypoints = 0;

            /* -----------------------------------------------------------------
            ** "Unnutz" ausschalten, weil es nach Erholung nicht mehr auf seinem
            ** Platz ist und wiederverwendet werden kann
            ** ---------------------------------------------------------------*/
            Commander->bact->ExtraState &= ~EXTRA_UNUSABLE;
            SlaveList = &(Commander->bact->slave_list);
            Slave = (struct OBNode *) SlaveList->mlh_Head;
            while( Slave->nd.mln_Succ ) {

                Slave->bact->ExtraState &= ~EXTRA_UNUSABLE;
                Slave = (struct OBNode *) Slave->nd.mln_Succ;
                }

            /*** Ziel auf eigenen Robo ***/
            if( yr_AreThereGroundVehicles( Commander->bact ) ) {

                struct findpath_msg fp;

                fp.num_waypoints = MAXNUM_WAYPOINTS;
                fp.from_x        = Commander->bact->pos.x;
                fp.from_z        = Commander->bact->pos.z;
                fp.to_x          = yrd->bact->pos.x;
                fp.to_z          = yrd->bact->pos.z;
                fp.flags         = WPF_Normal;

                /*** Mit FINDPATH nur testen ***/
                if( !_methoda( Commander->o, YBM_FINDPATH, &fp ))
                    break;

                /*** Wenn wir WAYPOINTS brauchen, dann SektorZiele ***/
                if( fp.num_waypoints > 1 ) {

                    /*** Ok, nochmal, aber richtig ***/
                    fp.num_waypoints = MAXNUM_WAYPOINTS;
                    _methoda( Commander->o, YBM_SETWAY, &fp );

                    /* ----------------------------------------------------
                    ** und buffern (commander reicht, der gibt eh an Slaves
                    ** weiter
                    ** --------------------------------------------------*/
                    Commander->bact->mt_commandid = yrd->bact->CommandID;
                    Commander->bact->mt_owner     = yrd->bact->Owner;
                    }
                else {

                    /*** klassisch ***/
                    st.target.bact = yrd->bact;
                    st.priority    = 0;
                    st.target_type = TARTYPE_BACTERIUM;
                    _methoda( Commander->o, YBM_SETTARGET, &st);
                    }
                }
            else {

                /*** normales Bact-Ziel ***/
                st.target.bact = yrd->bact;
                st.priority    = 0;
                st.target_type = TARTYPE_BACTERIUM;
                _methoda( Commander->o, YBM_SETTARGET, &st);
                }

            /*** Aggression nicht verändern! ***/

            /*** Flucht bekanntgeben ***/
            if( USER_ROBO ) {

                #ifdef __NETWORK__
                struct sendmessage_msg sm;
                struct ypamessage_logmsg lm;
                struct ypaworld_data *ywd;
                #endif

                /*** meldung, daß es ein SpielerGeschwader flüchtet ***/
                log.para1  = Commander->bact->CommandID;
                log.para2  = log.para3 = 0;
                log.ID     = LOGMSG_ESCAPE;
                log.sender = Commander->bact;
                log.pri    = 40;
                _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );

                /* -------------------------------------------------------
                ** Im Netzwerk gibt es keine autonomen. Folglich melde ich
                ** meine Flucht einem anderen Robo, so daß dieser denkt
                ** er hätte mich vertrieben. Nun sind Schatten aber nicht
                ** in meiner Attackerliste...  Und meine Attckerliste kann
                ** aggressionsbedingt auch leer sein. Also Sektoren scan-
                ** nen mittels YBM_GETSECTARGET
                ** -----------------------------------------------------*/
                #ifdef __NETWORK__
                ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
                if( ywd->playing_network ) {

                    struct getsectar_msg st;
                    st.Me         = Commander->bact;
                    st.flags      = GST_MYPOS;
                    _methoda( yrd->bact->BactObject, YBM_GETSECTARGET, &st );

                    if( st.SecTarget ) {

                        lm.generic.message_id = YPAM_LOGMSG;
                        lm.generic.owner      = yrd->bact->Owner;
                        lm.sender             = st.SecTarget->ident;
                        lm.senderowner        = st.SecTarget->Owner;
                        lm.para1              = 0;
                        lm.para2              = 0;
                        lm.para3              = 0;
                        lm.ID                 = LOGMSG_ENEMYESCAPES;
                        lm.pri                = 34;

                        sm.receiver_id        = NULL;
                        sm.receiver_kind      = MSG_ALL;
                        sm.data               = &lm;
                        sm.data_size          = sizeof( lm );
                        sm.guaranteed         = TRUE;
                        _methoda( yrd->world, YWM_SENDMESSAGE, &sm );
                        }
                    }
                #endif
                }
            else {

                /* -----------------------------------------------------------
                ** Meldung, daß wir einen in die Flucht geschlagen haben.
                ** Sender ist das angreifende Geschwader. Dieses können wir
                ** aus der Attackerliste ermitteln. Wir suchen den ersten
                ** mit Spielerowner UND NUR, WENN wir einen solchen finden,
                ** suchen wir den zugehörigen Commander und geben eine Meldung
                ** von ihm ab.
                ** ---------------------------------------------------------*/
                struct OBNode *attacker;
                struct Bacterium *found = NULL;
                struct MinList *alist;
                struct ypaworld_data *ywd;

                ywd = INST_DATA( ((struct nucleusdata *)(yrd->world))->o_Class,
                                 yrd->world );
                _get( Commander->o, YBA_AttckList, &alist );
                attacker = (struct OBNode *) alist->mlh_Head;
                while( attacker->nd.mln_Succ ) {

                    if( ywd->URBact->Owner == attacker->bact->Owner ) {

                        found = attacker->bact;
                        break;
                        }
                    attacker = (struct OBNode *) attacker->nd.mln_Succ;
                    }

                if( found ) {


                    /*** Chef ermitteln ***/
                    if( BCLID_YPAMISSY == found->BactClassID )
                        _get( found->BactObject, YMA_RifleMan, &found );

                    if( found->master != found->robo )
                        found = found->master_bact;

                    /*** Message losschicken ***/
                    log.para1  = 0;
                    log.para2  = log.para3 = 0;
                    log.ID     = LOGMSG_ENEMYESCAPES;
                    log.sender = found;
                    log.pri    = 34;
                    _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );
                    }
                }
            
            Commander = (struct OBNode *) Commander->nd.mln_Succ;
            continue;
            }
        else {

            /* -----------------------------------------------------------------
            ** Wir sind ok und flüchten noch nicht. Dann kann es uns aber
            ** trotzdem schlecht gehen. Sofern es uns übel geht UND WIR
            ** NOCH NICHT FLÜCHTEN (denn dann jammern wir ja auch beim Robo rum,
            ** außerdem hat es dann keinen Sinn mehr), schreien wir um Hilfe!
            ** Uns geht es schlecht heißt: Unter 50% und mehr Angreifer als
            ** eigene leute da sind.
            ** ---------------------------------------------------------------*/
            if( (how.value < 0.5) && USER_ROBO ) {

                struct sumparameter_msg sum;
                LONG   leute;

                sum.value = 0;
                sum.para  = PARA_NUMBER;
                _methoda( Commander->o, YBM_SUMPARAMETER, &sum );
                leute     = sum.value;

                sum.value = 0;
                sum.para  = PARA_ATTACKER;
                _methoda( Commander->o, YBM_SUMPARAMETER, &sum );

                if( sum.value > leute ) {

                    log.para1  = 20000;             // zeit ist Parameter
                    log.para2  = log.para3 = 0;
                    log.ID     = LOGMSG_REQUESTSUPPORT;
                    log.sender = Commander->bact;
                    log.pri    = 42;
                    _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );
                    }
                }
            }

        /* ------------------------------------------------------------------
        ** Vielleicht macht es sich auch sinnvoll, Verstärkung zu schicken.
        ** (Das sit gar nicht so logisch, weil für den Spieler kaum nachvoll-
        ** ziehbar ist, ob es sich um Verstärkung oder ein neues geschwader
        ** handelt.)  Das passiert dann aber im AUTO-ONLY-Zweig!
        ** ----------------------------------------------------------------*/

        if( FALSE == USER_ROBO ) {

            /* ---------------------------------------------------------------
            ** Bis hierher wurde alles von allen gemacht. Was jetzt kommt, ist
            ** nicht mehr für den UserRobo.
            ** -------------------------------------------------------------*/
            int i;

            /* -----------------------------------------------------
            ** RoboAttacker? Fuer die gibt es eine Sonderbehandlung,
            ** weil wir die Hauptziele oefters mal umschalten.
            ** ---------------------------------------------------*/
            if( (yrd->bact->internal_time - yrd->controlattack_time) > 1000 ) {

                yrd->controlattack_time = yrd->bact->internal_time;

                for( i = 0; i < MAXNUM_ROBOATTACKER; i++ ) {
                    if( yrd->rattack[ i ].attacker_id == Commander->bact->CommandID ) {
                        yr_ControlRoboAttacker( Commander, &(yrd->rattack[i]) );
                        break;
                        }
                    }
                }
            }


        /*** NeXTer ***/
        Commander = (struct OBNode *) Commander->nd.mln_Succ;
        }
}


void yr_RealizeWish( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ------------------------------------------------------------------------------
    ** Neue Form der Jobauswahl. Die Jobs muessen irgendwie ausgebremst werden.
    ** Diese nach ihrer Anmeldung reifen zu lassen, ist insofern nicht gut, weil
    ** bis zu ihrer Abarbeitung 10min vergehen koennen, wo sich die Welt total 
    ** aendert.
    **
    ** Besser ist es, die Suche nach einem Job zu verbieten. Die einzige Streuung
    ** darin ist die Untersuchung der Welt, die mal noch eine Minute dauern kann.
    ** hier waehlen wir einfach den aeltesten Job aus. Der aelteste ist der mit der
    ** negativsten Delay, denn ...Time... wird bei jedem erfolgreichen fund neu
    ** gesetzt. So schmeisst sich ein schneller test immer wieder selbst aus dem
    ** rennen, weil er als zu neu erscheint, dabei testet er schon Urzeiten, naemlich 
    ** seit delay negativ ist.
    ** ----------------------------------------------------------------------------*/

    /*** Ist der Bauslot frei? ***/
    if( 0L == yrd->BuildSlot_Kind ) {

        LONG time_since_anc;
        LONG found, found_time = 0;

        if( (yrd->RoboState & ROBO_POWERREADY) && (yrd->chk_Power_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Power_Time;
            time_since_anc = -yrd->chk_Power_Delay;

            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                found      = ROBO_BUILDPOWER;
                found_time = time_since_anc;
                }
            }

        if( (yrd->RoboState & ROBO_RADARREADY) && (yrd->chk_Radar_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Radar_Time;
            time_since_anc = -yrd->chk_Radar_Delay;

            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_BUILDRADAR;
                found_time = time_since_anc;
                }
            }

        if( (yrd->RoboState & ROBO_SAFETYREADY) && (yrd->chk_Safety_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Safety_Time;
            time_since_anc = -yrd->chk_Safety_Delay;
            
            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_BUILDSAFETY;
                found_time = time_since_anc;
                }
            }

        if( (yrd->RoboState & ROBO_FOUNDPLACE) && (yrd->chk_Place_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Place_Time;
            time_since_anc = -yrd->chk_Place_Delay;
            
            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_CHANGEPLACE;
                found_time = time_since_anc;
                }
            }

        /*** Vielleicht haben wir einen neuen Job ermittelt ... ***/
        if( found_time > 0 ) {

            /*** Dann Daten übertragen und altes Zeug löschen ***/
            switch( found ) {

                case ROBO_BUILDPOWER:

                    yrd->BuildSlot_Kind   = ROBO_BUILDPOWER;
                    yrd->BuildSlot_Pos    = yrd->chk_Power_Pos;
                    yrd->BuildSlot_Sector = yrd->chk_Power_Sector;

                    yrd->chk_Power_Value  = 0;
                    yrd->chk_Power_Count  = 0;
                    yrd->RoboState       &= ~ROBO_POWERREADY;
                    break;

                case ROBO_BUILDRADAR:

                    yrd->BuildSlot_Kind   = ROBO_BUILDRADAR;
                    yrd->BuildSlot_Pos    = yrd->chk_Radar_Pos;
                    yrd->BuildSlot_Sector = yrd->chk_Radar_Sector;

                    yrd->chk_Radar_Value  = 0;
                    yrd->chk_Radar_Count  = 0;
                    yrd->RoboState       &= ~ROBO_RADARREADY;
                    break;

                case ROBO_BUILDSAFETY:

                    yrd->BuildSlot_Kind   = ROBO_BUILDSAFETY;
                    yrd->BuildSlot_Pos    = yrd->chk_Safety_Pos;
                    yrd->BuildSlot_Sector = yrd->chk_Safety_Sector;

                    yrd->chk_Safety_Value = 0;
                    yrd->chk_Safety_Count  = 0;
                    yrd->RoboState       &= ~ROBO_SAFETYREADY;
                    break;

                case ROBO_CHANGEPLACE:

                    yrd->BuildSlot_Kind   = ROBO_CHANGEPLACE;
                    yrd->BuildSlot_Pos    = yrd->chk_Place_Pos;
                    yrd->BuildSlot_Sector = yrd->chk_Place_Sector;

                    yrd->chk_Place_Value  = 0;
                    yrd->chk_Place_Count  = 0;
                    yrd->RoboState       &= ~ROBO_FOUNDPLACE;
                    break;
                }
            }
        }

    /* --------------------------------------------------------------------
    ** ist ein VehicleSlot frei? Weil der Bauslot schon freigegeben wird,
    ** wenn das Dock noch arbeitet, warten wir auch, bis das Dock frei ist,
    ** um übermäßiges Abmelden zu vermeiden. Das ist zwar egal, beim
    ** Debuggen aber bestimmt angenehmer. Mit den neuen Slots kann man auch
    ** mal darüber nachdenken, Dock und VehicleSlot zu vereinen.
    ** ------------------------------------------------------------------*/
    if( (0L == yrd->VehicleSlot_Kind) && (!(yrd->RoboState & ROBO_DOCKINUSE)) ) {

        LONG time_since_anc;
        LONG found, found_time = 0;

        if( (yrd->RoboState & ROBO_DEFENSEREADY) && (yrd->chk_Enemy_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Enemy_Time;
            time_since_anc = -yrd->chk_Enemy_Delay;
            
            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_BUILDDEFENSE;
                found_time = time_since_anc;
                }
            }

        if( (yrd->RoboState & ROBO_CONQUERREADY) && (yrd->chk_Terr_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Terr_Time;
            time_since_anc = -yrd->chk_Terr_Delay;
            
            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_BUILDCONQUER;
                found_time = time_since_anc;
                }
            }

        if( (yrd->RoboState & ROBO_RECONREADY) && (yrd->chk_Recon_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Recon_Time;
            time_since_anc = -yrd->chk_Recon_Delay;
            
            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_BUILDRECON;
                found_time = time_since_anc;
                }
            }

        if( (yrd->RoboState & ROBO_ROBOREADY) && (yrd->chk_Robo_Value != 0) ) {

            //time_since_anc = msg->global_time - yrd->chk_Robo_Time;
            time_since_anc = -yrd->chk_Robo_Delay;
            
            /*** Ausreichend und besser als anderer? ***/
            if( time_since_anc > found_time ) { 
                
                found      = ROBO_BUILDROBO;
                found_time = time_since_anc;
                }
            }


        /*** Haben wir etwas gefunden? ***/
        if( found_time > 0 ) {

            switch( found ) {

                case ROBO_BUILDDEFENSE:

                    yrd->VehicleSlot_CommandID = yrd->chk_Enemy_CommandID;
                    yrd->VehicleSlot_Pos       = yrd->chk_Enemy_Pos;
                    yrd->VehicleSlot_Sector    = yrd->chk_Enemy_Sector;
                    yrd->VehicleSlot_Kind      = ROBO_BUILDDEFENSE;

                    yrd->chk_Enemy_Value       = 0;
                    yrd->RoboState            &= ~ROBO_DEFENSEREADY;
                    break;

                case ROBO_BUILDROBO:

                    yrd->VehicleSlot_CommandID = yrd->chk_Robo_CommandID;
                    yrd->VehicleSlot_Pos       = yrd->chk_Robo_Pos;
                    yrd->VehicleSlot_Sector    = yrd->chk_Robo_Sector;
                    yrd->VehicleSlot_Kind      = ROBO_BUILDROBO;

                    yrd->chk_Robo_Value        = 0;
                    yrd->RoboState            &= ~ROBO_ROBOREADY;
                    break;

                case ROBO_BUILDCONQUER:

                    yrd->VehicleSlot_Pos       = yrd->chk_Terr_Pos;
                    yrd->VehicleSlot_Sector    = yrd->chk_Terr_Sector;
                    yrd->VehicleSlot_Kind      = ROBO_BUILDCONQUER;

                    yrd->chk_Terr_Value        = 0;
                    yrd->chk_Terr_Count        = 0;
                    yrd->RoboState            &= ~ROBO_CONQUERREADY;
                    break;

                case ROBO_BUILDRECON:

                    yrd->VehicleSlot_Pos       = yrd->chk_Recon_Pos;
                    yrd->VehicleSlot_Sector    = yrd->chk_Recon_Sector;
                    yrd->VehicleSlot_Kind      = ROBO_BUILDRECON;

                    yrd->chk_Recon_Value       = 0;
                    yrd->chk_Recon_Count       = 0;
                    yrd->RoboState            &= ~ROBO_RECONREADY;
                    break;
                }
            }
        }

    /* ---------------------------------------------------------------
    ** Das waren die tests, ob was frei und damit neu zu belegen war.
    ** nun gucken wir mal, ob einer der Slots was zu tun hat. Wenn ja,
    ** so rufen wir die zugehörige Routine auf. ja, los gehts!
    ** das Ausbremsen machen wir erst, wenn der Job erfolgreich
    ** erledigt wurde.
    ** -------------------------------------------------------------*/

    switch( yrd->BuildSlot_Kind ) {

        case ROBO_BUILDPOWER:

            yr_BuildPower( yrd, msg );
            break;

        case ROBO_BUILDRADAR:

            yr_BuildRadar( yrd, msg );
            break;

        case ROBO_BUILDSAFETY:

            yr_BuildSafety( yrd, msg );
            break;

        case ROBO_CHANGEPLACE:

            yr_ChangePlace( yrd, msg );
            break;
        }

    switch( yrd->VehicleSlot_Kind ) {

        case ROBO_BUILDDEFENSE:

            yr_BuildDefense( yrd, msg );
            break;

        case ROBO_BUILDCONQUER:

            yr_BuildConquer( yrd, msg );
            break;

        case ROBO_BUILDRECON:

            yr_BuildReconnoitre( yrd, msg );
            break;

        case ROBO_BUILDROBO:

            yr_BuildRobo( yrd, msg );
            break;
        }
}


void yr_BuildRadar( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ---------------------------------------------------------------------
    ** Radarbau, Wie Kraftwerk. Wir überprüfen lediglich nicht den Sektor
    ** während der Anfahrt.
    ** ACHTUNG! Es kann trotzdem passieren, daß wir vom test bis hierher
    ** auf den Sektor schon was anderes Gebaut haben, weil das bauen ja auch
    ** dauert!!!!!!!!!!!!!!!!!!!
    ** NEU: Alle Werte beziehen sich aif den BuildSlot.
    ** -------------------------------------------------------------------*/

    struct settarget_msg target;
    WORD   sec_x, sec_y;
    LONG   bpid;
    FLOAT  angle, time, dist;
    struct flt_triple richtung;
    struct createbuilding_msg cb;
    struct BuildProto *b_array;
    struct Cell *build_sector;

    time  = (FLOAT) msg->frame_time / 1000.0;
    sec_x = yrd->BuildSlot_Pos % yrd->bact->WSecX;
    sec_y = yrd->BuildSlot_Pos / yrd->bact->WSecX;

    /*** Hat da schon jemand anders was hingebaut? ***/
    build_sector = &(yrd->FirstSector[ yrd->BuildSlot_Pos ]);
    if( build_sector->WType != 0 ) {

        /*** schon anderweitig gesättigt? ---> Abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }


    /*** Array holen ***/
    _get( yrd->world, YWA_BuildArray, &b_array );
    
    if( (bpid = yr_GetBestRadar( yrd, b_array )) == -1 ) {

        /* ------------------------------------------------------------
        ** Wir koennen gar nix bauen. Wahrscheinlich reicht die Energie
        ** nicht und dass wir waehrend des bewegens welche bekommen,
        ** ist unwahrscheinlich. 
        ** ----------------------------------------------------------*/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }

    /*** Sind wir am Bauort? ***/
    if( ((abs(yrd->bact->SectX - sec_x)<=1) && (abs(yrd->bact->SectY - sec_y)<=1)) &&
        (!((yrd->bact->SectX - sec_x==0) && (yrd->bact->SectY - sec_y==0)) ) )  {

        /*** Ja, also noch Ziel abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );

        /*** Sind wir ausgerichtet? ***/
        richtung.x =  (sec_x + 0.5) * SECTOR_SIZE - yrd->bact->pos.x;
        richtung.z = -(sec_y + 0.5) * SECTOR_SIZE - yrd->bact->pos.z;
        dist = nc_sqrt( richtung.x*richtung.x + richtung.z*richtung.z);
        richtung.x /= dist;  // dist ist immer größer 0!
        richtung.z /= dist;
        if( (richtung.x*yrd->bact->dir.m31+richtung.z*yrd->bact->dir.m33) > 0.9) {

            /*** es gibt was zum Bauen ***/
            cb.owner     = yrd->bact->Owner;
            cb.job_id    = yrd->bact->Owner;
            cb.bp        = bpid;
            cb.immediate = FALSE;
            cb.sec_x     = sec_x;
            cb.sec_y     = sec_y;
            cb.flags     = 0;
            if( _methoda( yrd->world, YWM_CREATEBUILDING, &cb ) ) {

                /*** Bauen hat angefangen ***/
                if( yrd->BuildSpare < b_array[ bpid ].CEnergy) {
                    yrd->bact->Energy -= (b_array[ bpid ].CEnergy-yrd->BuildSpare);
                    yrd->BuildSpare = 0;
                    }
                else
                    yrd->BuildSpare -= b_array[ bpid ].CEnergy;

                /*** Bauslot freimachen ***/
                yrd->BuildSlot_Kind = 0L;

                /*** Ausbremsen ***/
                if( yrd->NewAI )
                    yrd->chk_Radar_Delay = (100 - yrd->ep_Radar) * yrd->TimeLine / 100;
                }
            else {

                /* ---------------------------------------------------
                ** Das bauen war aus irgendeinem Grunde nicht möglich.
                ** Total abmelden, wenn es wichtig ist, kommt es im 
                ** nächsten Testdurchlauf  wieder
                ** -------------------------------------------------*/
                target.target_type = TARTYPE_NONE;
                target.priority    = 0;
                _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
            
                /*** Slot freimachen ***/
                yrd->BuildSlot_Kind = 0L;
                }
            }
        else {

            /*** Noch ausrichten ***/
            angle = nc_acos( richtung.x * yrd->bact->dir.m31 +
                             richtung.z * yrd->bact->dir.m33 );
            if( (richtung.x*yrd->bact->dir.m33-richtung.z*yrd->bact->dir.m31) > 0 )
                angle = -angle;

            if( angle < -yrd->bact->max_rot * time )
                angle = -yrd->bact->max_rot * time;
            if( angle >  yrd->bact->max_rot * time )
                angle =  yrd->bact->max_rot * time;

            yr_rot_round_global_y( yrd, angle);
            }
        }
    else {

        /*** Na, setzen wir das Ziel (NONREKURSIV) ***/
        yr_GetBestSectorToTarget( yrd, yrd->BuildSlot_Pos, &target );
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}


void yr_BuildSafety( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ----------------------------------------------
    ** Die Flakstellungen. Wie Radarstationen auch...
    ** NEU: es bezieht sich alles auf BuildSlot
    ** --------------------------------------------*/

    struct settarget_msg target;
    WORD   sec_x, sec_y;
    LONG   bpid;
    FLOAT  angle, time, dist;
    struct flt_triple richtung;
    struct createbuilding_msg cb;
    struct BuildProto *b_array;
    struct Cell *build_sector;

    time  = (FLOAT) msg->frame_time / 1000.0;
    sec_x = yrd->BuildSlot_Pos % yrd->bact->WSecX;
    sec_y = yrd->BuildSlot_Pos / yrd->bact->WSecX;

    /*** Hat da schon jemand anders was hingebaut? ***/
    build_sector = &(yrd->FirstSector[ yrd->BuildSlot_Pos ]);
    if( build_sector->WType != 0 ) {

        /*** schon anderweitig gesättigt? ---> Abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        /*** Bauslot freimachen ***/
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }

    /*** Array    holen ***/
    _get( yrd->world, YWA_BuildArray, &b_array );

    /*** Bestes KW für unsere Möglichkeiten holen ***/
    if( (bpid = yr_GetBestSafety( yrd, b_array )) == -1 ) {

        /*** gibt nix ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        /*** Bauslot freimachen ***/
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }

    /*** Sind wir am Bauort? ***/
    if( ((abs(yrd->bact->SectX - sec_x)<=1) && (abs(yrd->bact->SectY - sec_y)<=1)) &&
        (!((yrd->bact->SectX - sec_x==0) && (yrd->bact->SectY - sec_y==0)) ) )  {

        /*** Ja, also noch Ziel abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );

        /*** Sind wir ausgerichtet? ***/
        richtung.x =  (sec_x + 0.5) * SECTOR_SIZE - yrd->bact->pos.x;
        richtung.z = -(sec_y + 0.5) * SECTOR_SIZE - yrd->bact->pos.z;
        dist = nc_sqrt( richtung.x*richtung.x + richtung.z*richtung.z);
        richtung.x /= dist;  // dist ist immer größer 0!
        richtung.z /= dist;
        if( (richtung.x*yrd->bact->dir.m31+richtung.z*yrd->bact->dir.m33) > 0.9) {

            /*** es gibt was zum Bauen ***/
            cb.owner     = yrd->bact->Owner;
            cb.job_id    = yrd->bact->Owner;
            cb.bp        = bpid;
            cb.immediate = FALSE;
            cb.sec_x     = sec_x;
            cb.sec_y     = sec_y;
            cb.flags     = 0;
            if( _methoda( yrd->world, YWM_CREATEBUILDING, &cb ) ) {

                /*** Bauen hat angefangen ***/

                /*** Energie abziehen ***/
                if( yrd->BuildSpare < b_array[ bpid ].CEnergy) {
                    yrd->bact->Energy -= (b_array[ bpid ].CEnergy-yrd->BuildSpare);
                    yrd->BuildSpare = 0;
                    }
                else
                    yrd->BuildSpare -= b_array[ bpid ].CEnergy;

                /*** Slot freimachen ***/
                yrd->BuildSlot_Kind = 0L;

                /*** Ausbremsen ***/
                if( yrd->NewAI )
                    yrd->chk_Safety_Delay = (100 - yrd->ep_Safety) * yrd->TimeLine / 100;
                }
            else {

                /* ---------------------------------------------------
                ** Das bauen war aus irgendeinem Grunde nicht möglich.
                ** Total abmelden, wenn es wichtig ist, kommt es im 
                ** nächsten Testdurchlauf  wieder
                ** -------------------------------------------------*/
                target.target_type = TARTYPE_NONE;
                target.priority    = 0;
                _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
            
                /*** Slot freimachen ***/
                yrd->BuildSlot_Kind = 0L;
                }
            }
        else {

            /*** Noch ausrichten ***/
            angle = nc_acos( richtung.x * yrd->bact->dir.m31 +
                             richtung.z * yrd->bact->dir.m33 );
            if( (richtung.x*yrd->bact->dir.m33-richtung.z*yrd->bact->dir.m31) > 0 )
                angle = -angle;

            if( angle < -yrd->bact->max_rot * time )
                angle = -yrd->bact->max_rot * time;
            if( angle >  yrd->bact->max_rot * time )
                angle =  yrd->bact->max_rot * time;

            yr_rot_round_global_y( yrd, angle);
            }
        }
    else {

        /*** Na, setzen wir das Ziel ***/
        yr_GetBestSectorToTarget( yrd, yrd->BuildSlot_Pos, &target );
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}


void yr_BuildPower( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ----------------------------------------------------------------------
    ** KraftwerksBau. Ich gehe davon aus, daß ich auf einem Nachbarsector
    ** bin. Das müßte die Zielbearbeitung gemacht haben. Ich frage das WO
    ** nach einem Kraftwerk. Bekomme ich das, erzeuge ich es. dann gebe ich
    ** den Bauslot frei.
    ** --------------------------------------------------------------------*/

    struct settarget_msg target;
    WORD   sec_x, sec_y;
    LONG   bpid;
    FLOAT  angle, time, dist;
    struct flt_triple richtung;
    struct createbuilding_msg cb;
    struct BuildProto *b_array;
    struct Cell *build_sector;
    struct getrldratio_msg gr;

    time  = (FLOAT) msg->frame_time / 1000.0;
    sec_x = yrd->BuildSlot_Pos % yrd->bact->WSecX;
    sec_y = yrd->BuildSlot_Pos / yrd->bact->WSecX;

    /*** Hat da schon jemand anders was hingebaut? ***/
    build_sector = &(yrd->FirstSector[ yrd->BuildSlot_Pos ]);
    if( build_sector->WType != 0 ) {

        /*** schon anderweitig gesättigt? ---> Abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        /*** Slot freigeben ***/
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }

    /*** Ist das Bauen noch notwendig? ***/
    gr.owner = yrd->bact->Owner;
    _methoda( yrd->world, YWM_GETRLDRATIO, &gr );

    if( (build_sector->EnergyFactor == 255) ||
        ((gr.ratio < YR_PowerRatio) && (gr.ratio > 0.001)) ) {

        /*** schon anderweitig gesättigt? ---> Abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        /*** Slot freigeben ***/
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }

    /*** Array holen ***/
    _get( yrd->world, YWA_BuildArray, &b_array );

    /*** Ausgerichtet. Bestes KW für unsere Möglichkeiten holen ***/
    if( (bpid = yr_GetBestKraftWerk( yrd, b_array )) == -1 ) {

        /*** gibt nix heute ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
    
        /*** Slot freigeben ***/
        yrd->BuildSlot_Kind = 0L;

        /*** wech hier ***/
        return;
        }

    /*** Sind wir am Bauort? ***/
    if( ((abs(yrd->bact->SectX - sec_x)<=1) && (abs(yrd->bact->SectY - sec_y)<=1)) &&
        (!((yrd->bact->SectX - sec_x==0) && (yrd->bact->SectY - sec_y==0)) ) )  {

        /*** Ja, also noch Ziel abmelden ***/
        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );

        /*** Sind wir ausgerichtet? ***/
        richtung.x =  (sec_x + 0.5) * SECTOR_SIZE - yrd->bact->pos.x;
        richtung.z = -(sec_y + 0.5) * SECTOR_SIZE - yrd->bact->pos.z;
        dist = nc_sqrt( richtung.x*richtung.x + richtung.z*richtung.z);
        richtung.x /= dist;  // dist ist immer größer 0!
        richtung.z /= dist;
        if( (richtung.x*yrd->bact->dir.m31+richtung.z*yrd->bact->dir.m33) > 0.9) {

            /*** es gibt was zum Bauen ***/
            cb.owner     = yrd->bact->Owner;
            cb.job_id    = yrd->bact->Owner;
            cb.bp        = bpid;
            cb.immediate = FALSE;
            cb.sec_x     = sec_x;
            cb.sec_y     = sec_y;
            cb.flags     = 0;
            if( _methoda( yrd->world, YWM_CREATEBUILDING, &cb ) ) {

                /*** Energie abziehen ***/
                if( yrd->BuildSpare < b_array[ bpid ].CEnergy) {
                    yrd->bact->Energy -= (b_array[ bpid ].CEnergy-yrd->BuildSpare);
                    yrd->BuildSpare = 0;
                    }
                else
                    yrd->BuildSpare -= b_array[ bpid ].CEnergy;

                /*** Slot freigeben ***/
                yrd->BuildSlot_Kind = 0L;

                /*** Ausbremsen ***/
                if( yrd->NewAI )
                    yrd->chk_Power_Delay = (100 - yrd->ep_Power) * yrd->TimeLine / 100;
                }
            else {

                /* ---------------------------------------------------
                ** Das bauen war aus irgendeinem Grunde nicht möglich.
                ** Total abmelden, wenn es wichtig ist, kommt es im 
                ** nächsten Testdurchlauf  wieder
                ** -------------------------------------------------*/
                target.target_type = TARTYPE_NONE;
                target.priority    = 0;
                _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
            
                /*** Slot freigeben ***/
                yrd->BuildSlot_Kind = 0L;
                }
            }
        else {

            /*** Noch ausrichten ***/
            angle = nc_acos( richtung.x * yrd->bact->dir.m31 +
                             richtung.z * yrd->bact->dir.m33 );
            if( (richtung.x*yrd->bact->dir.m33-richtung.z*yrd->bact->dir.m31) > 0 )
                angle = -angle;

            if( angle < -yrd->bact->max_rot * time )
                angle = -yrd->bact->max_rot * time;
            if( angle >  yrd->bact->max_rot * time )
                angle =  yrd->bact->max_rot * time;

            yr_rot_round_global_y( yrd, angle);
            }
        }
    else {

        /*** Na, setzen wir das Ziel ***/
        yr_GetBestSectorToTarget( yrd, yrd->BuildSlot_Pos, &target );
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }

}


void yr_ChangePlace( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* --------------------------------------------------------------------
    ** Wir sollen eine neue Position einnehmen, die in chk_Place_Pos steht.
    ** Irgendjemand hat mal gesagt, das sei besser...
    ** ------------------------------------------------------------------*/
    FLOAT  distance;
    struct settarget_msg target;
    WORD   sec_x, sec_y;

    sec_x = yrd->BuildSlot_Pos % yrd->bact->WSecX;
    sec_y = yrd->BuildSlot_Pos / yrd->bact->WSecX;

    /*** Zuerst testen, ob Position sinnvoll ***/
    if( (sec_x <  1) ||
        (sec_x >  (yrd->bact->WSecX - 2) ) ||
        (sec_y <  1) ||
        (sec_y >  (yrd->bact->WSecY - 2) ) ) {

        /*** Auftrag abmelden ***/
        yrd->BuildSlot_Kind = 0L;
        return;
        }
    
    /*** Sind wir schon da? ***/
    distance = nc_sqrt( (FLOAT)
                        ((yrd->bact->SectX - sec_x)*(yrd->bact->SectX - sec_x) +
                         (yrd->bact->SectY - sec_y)*(yrd->bact->SectY - sec_y)));

    if( distance < 0.1 ) {

        /*** In Folge der Bewegung andere Situation ***/
        yr_ClearBuildSlots( yrd );

        /*** geschafft. Bremsen und Auftrag abmelden ***/
        yrd->BuildSlot_Kind = 0L;

        target.target_type = TARTYPE_NONE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );

        /*** Ausbremsen ***/
        if( yrd->NewAI )
            yrd->chk_Place_Delay = (100 - yrd->ep_ChangePlace) * yrd->TimeLine / 100;
        }
    else {

        /*** weit weg. Ziel setzen ***/
        target.target_type = TARTYPE_SECTOR_NR;
        target.pos.x       = ((FLOAT)  sec_x + 0.5) * SECTOR_SIZE;
        target.pos.z       = ((FLOAT) -sec_y - 0.5) * SECTOR_SIZE;
        target.priority    = 0;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}



void yr_BuildConquer( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ---------------------------------------------------------------------
    ** Wir sollen etwas erobern. Na gut. Dazu sehen wir uns die Verhältnisse
    ** dort einmal an und entscheiden dann, ob wir die Energie haben.
    ** Zu CFR addieren wir eine Entfernungsabhängige Energie drauf.
    ** NEU: Alles bezieht sich auf den VehicleSlot
    ** -------------------------------------------------------------------*/
    struct checkforcerelation_msg cfr;
    struct OBNode *Commander;
    struct allocforce_msg af;
    LONG   energy;
    struct settarget_msg target;
    struct intersect_msg inter;
    LONG   flak_sector;
    struct Bacterium *bact;
    struct getsectorinfo_msg gsi;

    cfr.flags = CFR_NOSECTOR;
    cfr.pos.x =  (yrd->VehicleSlot_Pos % yrd->bact->WSecX + 0.5) * SECTOR_SIZE;
    cfr.pos.z = -(yrd->VehicleSlot_Pos / yrd->bact->WSecX + 0.5) * SECTOR_SIZE;
    _methoda( yrd->bact->BactObject, YBM_CHECKFORCERELATION, &cfr );

    //energy = 0.6 * nc_sqrt( cfr.his_energy - cfr.my_energy );
    energy = 0.5 * ( cfr.his_energy - cfr.my_energy );
    if( energy < MinimalProjectEnergy ) energy = MinimalProjectEnergy;

    /*** sind da feindliche bewaffnete NichtRobo-Guns? ***/
    flak_sector  = 0;
    gsi.abspos_x = cfr.pos.x;
    gsi.abspos_z = cfr.pos.z;
    _methoda( yrd->world, YWM_GETSECTORINFO, &gsi );

    bact = (struct Bacterium *) gsi.sector->BactList.mlh_Head;
    while( bact->SectorNode.mln_Succ ) {
        if( (yrd->bact->Owner != bact->Owner) &&
            (BCLID_YPAGUN     == bact->BactClassID)) {
            /*** fremde gun ***/
            if((-1 != bact->auto_ID) ||
               (-1 != bact->mg_shot)) {
                /*** bewaffnet ***/
                ULONG rgun;
                _get( bact->BactObject, YGA_RoboGun, &rgun );
                if( !rgun ) {
                    /*** normale Flak. Staerke wie Roboangriff ***/
                    struct ypaworld_data *ywd;
                    struct OBNode *robo;
                    ywd  = INST_DATA( ((struct nucleusdata *)(yrd->world))->o_Class, yrd->world );
                    robo = (struct OBNode *) ywd->CmdList.mlh_Head;
                    while( robo->nd.mln_Succ ) {

                        if( (BCLID_YPAROBO==robo->bact->BactClassID) &&
                            (ACTION_DEAD  !=robo->bact->MainState) &&
                            (bact->Owner  ==robo->bact->Owner) ) {
                            flak_sector = max( BASIC_ROBO_ENERGY,robo->bact->Maximum/2);
                            break;
                            }
                        robo = (struct OBNode *) robo->nd.mln_Succ;
                        }
                    }
                }
            }

        if( flak_sector )
            break;

        bact = (struct Bacterium *) bact->SectorNode.mln_Succ;
        }

    /*** flak_sector sowohl als Energiezaehler und Flag benutzen ***/
    if( energy < flak_sector ) energy = flak_sector;

    /*** Sind wir in günstiger Abwurfposition? ***/
    inter.pnt.x = yrd->bact->pos.x + yrd->dock_pos.x;
    inter.pnt.y = yrd->bact->pos.y + yrd->dock_pos.y - 100;
    inter.pnt.z = yrd->bact->pos.z + yrd->dock_pos.z;
    inter.vec.x = inter.vec.z = 0.0;
    inter.vec.y = 20000;
    inter.flags = 0;
    _methoda( yrd->world, YWM_INTERSECT, &inter );
    if( (yrd->bact->Sector->Height - inter.ipnt.y) < 50 ) {

        FLOAT  delta_x, delta_z;
        
        /* ----------------------------------------------------------------------
        ** Energietests erfolgen im Gegensatz zum Bauen bei den Vehiclesachen
        ** dort, wo es gebraucht wird. Haben wir keine Energie für den Commander, 
        ** kriegen wir nichts zurück und aufrüsten können wir, bis die Energie 
        ** alle ist (Wenn der Wunsch die Möglichkeiten übersteigt)
        ** --------------------------------------------------------------------*/
        delta_x         = cfr.pos.x - yrd->bact->pos.x;
        delta_z         = cfr.pos.z - yrd->bact->pos.z;
        af.distance     = nc_sqrt( delta_x * delta_x + delta_z * delta_z );
        af.target_pos   = cfr.pos;
        af.target_pos.y = 0.0;
        af.energy       = max( energy, BASIC_CONQUER_ENERGY);        // energy;
        af.target_type  = TARTYPE_SECTOR;

        /*** Bei Kraftwerken mal niedrige Aggression ***/
        if( yrd->VehicleSlot_Sector->WType == WTYPE_Kraftwerk )
            af.aggression   = AGGR_SECBACT - 1;
        else
            af.aggression   = AGGR_AI_STANDARD;

        /*** spezielle Wünsche für Eroberung ***/
        af.forbidden    = ACF_UFO;
        af.necessary    = ACF_WEAPON;
        af.good         = ACF_BOMB | ACF_HELICOPTER;
        af.bad          = 0L;
        af.job          = JOB_CONQUER;

        Commander = yr_AllocForce( yrd, &af );
        if( Commander ) {

            /* ---------------------------------------------------------------
            ** Wir haben jemanden bekommen. Hier ist Platz für spezielle Nach-
            ** initialisierungen. Ziele wurden bereits eingetragen
            ** -------------------------------------------------------------*/
            }

        /*** Ausbremsen, auch, wenn Job nicht geklappt hat ***/
        if( yrd->NewAI )
            yrd->chk_Terr_Delay = (100 - yrd->ep_Conquer) * yrd->TimeLine / 100;
        
        /*** Freigeben, weil alles in den DockInfos steht oder hinfällig ist ***/
        yrd->VehicleSlot_Kind = 0L;
        }
    else {

        /*** Wir müssen uns noch etwas zur Seite bewegen ***/
        target.pos.x       = yrd->bact->pos.x + 200;
        target.pos.z       = yrd->bact->pos.z + 300;
        target.priority    = 0;
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}


void yr_BuildDefense( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ------------------------------------------------------------------------
    ** Wir sollen Jemanden bekämpfen. Um seine Kräfte zu kennen, lassen wir ihn
    ** uns geben und wenden ein SUMPARAMETER auf ihn an.
    ** ----------------------------------------------------------------------*/

    struct sumparameter_msg sum;
    struct OBNode *Commander;
    FLOAT  delta_x, delta_z;
    struct allocforce_msg af;
    LONG   energy;
    struct settarget_msg target;
    struct intersect_msg inter;


    target.priority = yrd->VehicleSlot_CommandID;
    if( !_methoda( yrd->bact->BactObject, YRM_GETENEMY, &target ) ) {

        /*** Hoppla, Feind schon tot! +**/
        yrd->VehicleSlot_Kind = 0L;
        return;
        }

    sum.value = 0;
    sum.para  = PARA_ENERGY;
    _methoda( target.target.bact->BactObject, YBM_SUMPARAMETER, &sum );

    /*** Wegen Aufschaukeln, ausserdem bessere Vehikel ***/
    //energy = nc_sqrt( sum.value );
    energy = sum.value / 2;
    if( energy < MinimalProjectEnergy ) energy = MinimalProjectEnergy;

    /*** Fuer Flaks etwas mehr ***/
    if( BCLID_YPAGUN == target.target.bact->BactClassID ) {
        if( (-1!=target.target.bact->auto_ID) ||
            (-1!=target.target.bact->mg_shot)) {

            ULONG rgun;
            _get( target.target.bact->BactObject, YGA_RoboGun, &rgun );
            if( !rgun ) {

                /* -----------------------------------------
                ** Die energie richtet sich nach der Staerke
                ** des Eigentuemer-Robos
                ** ---------------------------------------*/
                struct ypaworld_data *ywd;
                struct OBNode *robo;
                ywd  = INST_DATA( ((struct nucleusdata *)(yrd->world))->o_Class, yrd->world );
                robo = (struct OBNode *) ywd->CmdList.mlh_Head;
                while( robo->nd.mln_Succ ) {

                    if( (BCLID_YPAROBO            ==robo->bact->BactClassID) &&
                        (ACTION_DEAD              !=robo->bact->MainState) &&
                        (target.target.bact->Owner==robo->bact->Owner) ) {
                        energy = max( BASIC_ROBO_ENERGY,robo->bact->Maximum/2);
                        break;
                        }
                    robo = (struct OBNode *) robo->nd.mln_Succ;
                    }
                }
            }
        }


    /*** Sind wir in günstiger Abwurfposition? ***/
    inter.pnt.x = yrd->bact->pos.x + yrd->dock_pos.x;
    inter.pnt.y = yrd->bact->pos.y + yrd->dock_pos.y - 100;
    inter.pnt.z = yrd->bact->pos.z + yrd->dock_pos.z;
    inter.vec.x = inter.vec.z = 0.0;
    inter.vec.y = 20000;
    inter.flags = 0;
    _methoda( yrd->world, YWM_INTERSECT, &inter );
    if( (yrd->bact->Sector->Height - inter.ipnt.y) < 50 ) {
        
        /*
        ** Energietests erfolgen im Gegensatz zum Bauen bei den Vehiclesachen
        ** dort, wo es gebraucht wird. Haben wir keine Energie für den Commander, 
        ** kriegen wir nichts zurück und aufrüsten können wir, bis die Energie 
        ** alle ist (Wenn der Wunsch die Möglichkeiten übersteigt)
        */

        delta_x        = target.target.bact->pos.x - yrd->bact->pos.x;
        delta_z        = target.target.bact->pos.z - yrd->bact->pos.z;
        af.distance    = nc_sqrt( delta_x * delta_x + delta_z * delta_z );
        af.target_pos  = target.target.bact->pos;
        af.energy      = min( energy, BASIC_DEFENSE_ENERGY);      // energy;
        af.target_type = TARTYPE_BACTERIUM;
        af.target_bact = target.target.bact;
        af.commandID   = yrd->VehicleSlot_CommandID;
        af.aggression   = AGGR_AI_STANDARD;

        /*** Nun spezielle Wünsche (alter Aufruf) ***/
        if( (target.target.bact->BactClassID == BCLID_YPATANK) ||
            (target.target.bact->BactClassID == BCLID_YPACAR) ||
            (target.target.bact->BactClassID == BCLID_YPAHOVERCRAFT) ) {

            /*** Bodenziel ***/
            af.forbidden = ACF_UFO;
            af.necessary = ACF_WEAPON;
            af.good      = ACF_HELICOPTER | ACF_SEARCHMISSILE | ACF_AGILE;
            af.bad       = ACF_FLYINGHIGH;
            }
        else {

            /*** Luftziel ***/
            af.forbidden = ACF_UFO | ACF_BOMB;
            af.necessary = ACF_WEAPON;
            af.good      = ACF_FLYER | ACF_SEARCHMISSILE | ACF_AGILE |
                           ACF_TANK | ACF_FLYINGHIGH;
            af.bad       = ACF_HELICOPTER;
            }

        /*** Für neuen Aufruf ***/
        switch( target.target.bact->BactClassID ) {

            case BCLID_YPACAR:
            case BCLID_YPATANK:
            case BCLID_YPAHOVERCRAFT:

                af.job = JOB_FIGHTTANK;
                break;

            case BCLID_YPAFLYER:
            case BCLID_YPAUFO:

                af.job = JOB_FIGHTFLYER;
                break;

            case BCLID_YPABACT:

                af.job = JOB_FIGHTHELICOPTER;
                break;

            default:

                af.job = JOB_FIGHTHELICOPTER;
                break;
            }

        Commander = yr_AllocForce( yrd, &af );
        if( Commander ) {

            /*** Eventuelle besonderheiten ***/
            }

        /*** Ausbremsen ***/
        if( yrd->NewAI )
            yrd->chk_Enemy_Delay = (100 - yrd->ep_Defense) * yrd->TimeLine / 100;
        
        /*** Löschen, weil alles in den DockInfos steht oder hinfällig ist ***/
        yrd->VehicleSlot_Kind = 0L;
        }
    else {

        /*** Wir müssen uns noch etwas zur Seite bewegen ***/
        target.pos.x       = yrd->bact->pos.x + 200;
        target.pos.z       = yrd->bact->pos.z + 300;
        target.priority    = 0;
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}


void yr_BuildRobo( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ------------------------------------------------------------------
    ** Bekämpfung eines feindlichen Robos. Vorerst zum testen nur anderes
    ** Energieverhältnis als bei BuildDefense
    ** ----------------------------------------------------------------*/

    struct OBNode *Commander;
    struct allocforce_msg af;
    LONG   energy;
    struct settarget_msg target;
    struct intersect_msg inter;


    target.priority = yrd->VehicleSlot_CommandID;
    if( !_methoda( yrd->bact->BactObject, YRM_GETENEMY, &target ) ) {

        /*** Hoppla, Feind schon tot! +**/
        yrd->VehicleSlot_Kind = 0L;
        return;
        }

    /*** Sind wir in günstiger Abwurfposition? ***/
    inter.pnt.x = yrd->bact->pos.x + yrd->dock_pos.x;
    inter.pnt.y = yrd->bact->pos.y + yrd->dock_pos.y - 100;
    inter.pnt.z = yrd->bact->pos.z + yrd->dock_pos.z;
    inter.vec.x = inter.vec.z = 0.0;
    inter.vec.y = 20000;
    inter.flags = 0;
    _methoda( yrd->world, YWM_INTERSECT, &inter );
    if( (yrd->bact->Sector->Height - inter.ipnt.y) < 50 ) {

        FLOAT  delta_x, delta_z;
        
        /* ----------------------------------------------------------------------
        ** Energietests erfolgen im Gegensatz zum Bauen bei den Vehiclesachen
        ** dort, wo es gebraucht wird. Haben wir keine Energie für den Commander, 
        ** kriegen wir nichts zurück und aufrüsten können wir, bis die Energie 
        ** alle ist (Wenn der Wunsch die Möglichkeiten übersteigt)
        ** --------------------------------------------------------------------*/

        delta_x        = target.target.bact->pos.x - yrd->bact->pos.x;
        delta_z        = target.target.bact->pos.z - yrd->bact->pos.z;
        af.distance    = nc_sqrt( delta_x * delta_x + delta_z * delta_z );
        af.target_pos  = target.target.bact->pos;
        /*** Wir machen testhalber die Roboenergie dem Feind ***/
        //af.energy      = BASIC_ROBO_ENERGY;
        af.energy      = max(BASIC_ROBO_ENERGY, target.target.bact->Maximum / 2);
        af.target_type = TARTYPE_BACTERIUM;
        af.target_bact = target.target.bact;
        af.commandID   = yrd->VehicleSlot_CommandID;
        af.aggression   = AGGR_AI_STANDARD;

        /*** Besondere Wünsche ***/
        af.forbidden = ACF_UFO;
        af.necessary = ACF_WEAPON;
        af.good      = ACF_FLYER | ACF_SEARCHMISSILE | ACF_AGILE |
                       ACF_TANK | ACF_FLYINGHIGH;
        af.bad       = ACF_HELICOPTER;
        af.job       = JOB_FIGHTROBO;


        /*** Nutze Zusatzbatterie ***/
        yrd->RoboState |= ROBO_USEVHCLSPARE;

        Commander = yr_AllocForce( yrd, &af );
        if( Commander ) {

            struct roboattacker *ra;

            /*** Als RoboAttacker registrieren ***/
            if( ra = yr_GetFreeAttackerSlot( yrd ) ) {

                ra->robo_id     = target.target.bact->CommandID;
                ra->attacker_id = Commander->bact->CommandID;
                }
                
            /*** eventuelle Nachinitialisierungen ***/
            }

        /*** Ausbremsen, auch es nicht geklappt hat ***/
        if( yrd->NewAI )
            yrd->chk_Robo_Delay = (100 - yrd->ep_Robo) * yrd->TimeLine / 100;
        
        /*** Löschen, weil alles in den DockInfos steht oder hinfällig ist ***/
        yrd->VehicleSlot_Kind = 0L;
        }
    else {

        /*** Wir müssen uns noch etwas zur Seite bewegen ***/
        target.pos.x       = yrd->bact->pos.x + 200;
        target.pos.z       = yrd->bact->pos.z + 300;
        target.priority    = 0;
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}



void yr_BuildReconnoitre( struct yparobo_data *yrd, struct trigger_logic_msg *msg)
{
    /* ------------------------------------------------------------
    ** Schickt einen Aufklärer los, dabei interessieren uns Kräfte-
    ** verhältnisse überhaupt nicht
    ** -----------------------------------------------------------*/

    struct OBNode *Commander;
    struct allocforce_msg af;
    struct settarget_msg target;
    struct intersect_msg inter;

    /*** Sind wir in günstiger Abwurfposition? ***/
    inter.pnt.x = yrd->bact->pos.x + yrd->dock_pos.x;
    inter.pnt.y = yrd->bact->pos.y + yrd->dock_pos.y - 100;
    inter.pnt.z = yrd->bact->pos.z + yrd->dock_pos.z;
    inter.vec.x = inter.vec.z = 0.0;
    inter.vec.y = 20000;
    inter.flags = 0;
    _methoda( yrd->world, YWM_INTERSECT, &inter );
    if( (yrd->bact->Sector->Height - inter.ipnt.y) < 50 ) {

        FLOAT  delta_x, delta_z;
        
        /* ----------------------------------------------------------------------
        ** Energietests erfolgen im Gegensatz zum Bauen bei den Vehiclesachen
        ** dort, wo es gebraucht wird. Haben wir keine Energie für den Commander, 
        ** kriegen wir nichts zurück und aufrüsten können wir, bis die Energie 
        ** alle ist (Wenn der Wunsch die Möglichkeiten übersteigt)
        ** --------------------------------------------------------------------*/

        af.target_pos.x =  (yrd->VehicleSlot_Pos%yrd->bact->WSecX+0.5)*SECTOR_SIZE;
        af.target_pos.z = -(yrd->VehicleSlot_Pos/yrd->bact->WSecX+0.5)*SECTOR_SIZE;
        af.target_pos.y =  0.0;
        delta_x        = af.target_pos.x - yrd->bact->pos.x;
        delta_z        = af.target_pos.z - yrd->bact->pos.z;
        af.distance    = nc_sqrt( delta_x * delta_x + delta_z * delta_z );
        af.energy      = BASIC_RECON_ENERGY;
        af.target_type = TARTYPE_SECTOR;
        af.aggression  = AGGR_FIGHTPRIM;

        /*** Besondere Wünsche ***/
        af.forbidden   = 0L;
        af.necessary   = ACF_FOOTPRINT;
        af.good        = ACF_UFO | ACF_CAR;
        af.bad         = ACF_WEAPON;
        af.job         = JOB_RECONNOITRE;

        Commander = yr_AllocForce( yrd, &af );
        if( Commander ) {

            /*** Aggression runtersetzen? ***/
            }

        /*** Ausbremsen, auch wenn es nicht geklappt hat ***/
        if( yrd->NewAI )
            yrd->chk_Recon_Delay = (100 - yrd->ep_Reconnoitre) * yrd->TimeLine / 100;
        
        /*** Löschen, weil alles in den DockInfos steht oder hinfällig ist ***/
        yrd->VehicleSlot_Kind = 0L;
        }
    else {

        /*** Wir müssen uns noch etwas zur Seite bewegen ***/
        target.pos.x       = yrd->bact->pos.x + 200;
        target.pos.z       = yrd->bact->pos.z + 300;
        target.priority    = 0;
        target.target_type = TARTYPE_SECTOR_NR;
        _methoda( yrd->bact->BactObject, YBM_SETTARGET, &target );
        }
}


void yr_InitForce( struct yparobo_data *yrd, struct OBNode *Commander )
{
    /* ----------------------------------------------------------------------
    ** Wir haben ein geschwader, welches energetisch evtl. noch nicht auf dem
    ** höchsten Stand ist. Der Energiewert steht in dock_energy.
    ** Wenn wir dock_energy unter 0 haben, dann setzen wir alle im Dock auf 
    ** NORMAL, die vorher waren sie auf CREATE. Den Rüstungschutz verändern 
    ** wir nicht, weil man auch Leute im Dock bekämpfen kann.
    ** NEU: Jeder Slave bekommt die CommandID (für Messagesystem)
    ** --------------------------------------------------------------------*/

    struct createvehicle_msg   cv;
    Object                     *slave;
    struct Bacterium           *sbact;
    ULONG                      BC;
    struct setstate_msg        state;
    LONG                       max_energy;

    #ifdef __NETWORK__
    struct sendmessage_msg sm;
    struct ypamessage_newvehicle nvm;
    struct ypaworld_data *ywd;
    #endif



    /*** Dock frei? ***/
    if( yrd->dock_time > 0 ) return;

    if( yrd->dock_energy <= 0 ) {

        /*** Commander fertigmachen ***/
        yr_InitializeCommand( yrd, Commander );
        }
    else {

        struct sumparameter_msg sum;
        FLOAT  ce;
        LONG   v_energy;

        /*** Wir brauchen schon noch etwas ***/
        slave = NULL;

        /* --------------------------------------------------------------------
        ** Wenn die Energie reicht, dann bauen, sonst Geschwader fertigmachen
        ** und unvollständig losschicken. Welcher Pool verantwortlich ist, sagt
        ** das derzeit gesetzte Bauen-Flag.
        **
        ** CreateEnergy ist abhängig von der Anzahl der Vehicle, die ich habe.
        ** ------------------------------------------------------------------*/
        max_energy = yrd->bact->Energy - (LONG)((FLOAT)yrd->bact->Maximum * YR_MinEnergy);

        /*** duerfen wir mehr verwenden? ***/
        if( yrd->RoboState & ROBO_USEVHCLSPARE )
            max_energy += yrd->VehicleSpare;

        sum.para  = PARA_NUMBER;
        sum.value = 0;
        _methoda( yrd->bact->BactObject, YBM_SUMPARAMETER, &sum );
        ce       = 1 + CREATE_ENERGY_AUTONOM * sum.value;
        v_energy = (LONG)((FLOAT) Commander->bact->Maximum * ce );

        if( v_energy < max_energy ) {

            /*** Es kann reichen. pobrobuiwatch ***/
            cv.vp = Commander->bact->TypeID;

            cv.x    = yrd->bact->pos.x + yrd->dock_pos.x;
            cv.y    = yrd->bact->pos.y + yrd->dock_pos.y;
            cv.z    = yrd->bact->pos.z + yrd->dock_pos.z;

            slave = (Object *) _methoda( yrd->world, YWM_CREATEVEHICLE, &cv );
            }

        if( !slave ) {

            /*** Commander fertigmachen ***/
            yr_InitializeCommand( yrd, Commander );
            return;
            }

        /*** Initialisierungen ***/
        _get( slave, YBA_Bacterium, &sbact );
        sbact->MainState = ACTION_CREATE;
        sbact->Owner     = yrd->bact->Owner;
        sbact->CommandID = Commander->bact->CommandID;
        sbact->robo      = yrd->bact->BactObject;
        
        _get( yrd->bact->BactObject, YBA_BactCollision, &BC );
        _set( slave, YBA_BactCollision, BC );

        /*** Anketten ***/
        _methoda( Commander->o, YBM_ADDSLAVE, slave );

        #ifdef __NETWORK__
        /*** Id aktualisieren und Message losschicken ***/
        ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
        if( ywd->playing_network ) {

            sbact->ident    |= (((ULONG)sbact->Owner) << 24);
            nvm.generic.message_id = YPAM_NEWVEHICLE;
            nvm.generic.owner      = sbact->Owner;
            nvm.pos                = sbact->pos;
            nvm.type               = sbact->TypeID;
            nvm.master             = sbact->master_bact->ident;
            nvm.ident              = sbact->ident;
            nvm.vkind              = NV_SLAVE;
            nvm.command_id         = Commander->bact->CommandID;

            sm.receiver_id         = NULL;
            sm.receiver_kind       = MSG_ALL;
            sm.data                = &nvm;
            sm.data_size           = sizeof( nvm );
            sm.guaranteed          = TRUE;
            _methoda( yrd->world, YWM_SENDMESSAGE, &sm );
            }
        #endif

        /*** Energieabzug ***/
        if( yrd->RoboState & ROBO_USEVHCLSPARE ) {

            if( v_energy <= yrd->VehicleSpare )
                yrd->VehicleSpare -= v_energy;
            else {
                yrd->bact->Energy -= (v_energy - yrd->VehicleSpare);
                yrd->VehicleSpare  = 0;
                }
            }
        else
            yrd->bact->Energy -= v_energy;

        /*** DockEnergy reduzieren ***/
        yrd->dock_energy -= sbact->Energy;

        /*** Dockzeug ***/
        state.main_state   = ACTION_CREATE;
        state.extra_off    = state.extra_on = 0;
        _methoda( slave, YBM_SETSTATE, &state );
        sbact->scale_time  = (LONG)( sbact->Maximum * YR_CreationTimeA );
        yrd->dock_time     = sbact->scale_time + YR_DeltaDockTime;
        yrd->dock_count++;

        /*** Meldung machen ***/
        yr_HistoryCreate( yrd->world, sbact );
        }
}


void yr_InitializeCommand( struct yparobo_data *yrd, struct OBNode *Commander )
{
/*
** Macht ein Geschwader fertig, gibt ihm das Ziel und räumt das Dock auf.
*/

    struct settarget_msg target;

    if( yrd->RoboState & ROBO_SETDOCKTARGET ) {

        target.priority    = 0;
        target.pos.x       = yrd->dtpos.x;
        target.pos.z       = yrd->dtpos.z;
        target.target.bact = yrd->dtbact;
        target.target_type = yrd->dttype;

        /*** Pointer aktualisieren ***/
        if( yrd->dttype == TARTYPE_BACTERIUM ) {

            target.priority = yrd->dtCommandID;
            if( !_methoda( yrd->bact->BactObject, YRM_GETENEMY, &target )) {
    
                /* --------------------------------------------
                ** nicht mehr existent. Ziel mittels Sektorziel
                ** abmelden
                ** ------------------------------------------*/
                target.target_type = TARTYPE_SECTOR;
                target.pos.x       = Commander->bact->pos.x;
                target.pos.z       = Commander->bact->pos.z;
                }
            else {

                /* -------------------------------------------------
                ** Wenn das bacterienziel nicht mehr sichtbar ist,
                ** dann machen wir den Sektor zum Ziel mit Aggr3,
                ** den sonst stehen meistens grosse Robogeschwader
                ** in der Landschaft rum.
                ** Wir nehmen die Pos, wo der Robo zuletzt gesichtet
                ** wurde. Die steht im Dock
                ** -----------------------------------------------*/
                if( !(target.target.bact->Sector->FootPrint & (1<<yrd->bact->Owner) )) {

                    target.pos.x       = yrd->dtpos.x;
                    target.pos.z       = yrd->dtpos.z;
                    target.target_type = TARTYPE_SECTOR;
                    yrd->dock_aggr     = AGGR_SECBACT;
                    }
                }
            target.priority = 0;
            }

        /*** Der kann schon tot sein... ***/
        if( ACTION_DEAD != Commander->bact->MainState ) {

            /* -----------------------------------------------------------
            ** Ok, Ziel setzen. Dazu testen wir, ob Bodenfahrzeuge drinnen
            ** sind und ob wir Wegpunkte braeuchten. Ist dem so setzen wir
            ** das Wegpunktziel und nachtraeglich evtl. die Bakterieniden-
            ** tifikation. Andernfalls setzen wir das Ziel direkt.
            ** ---------------------------------------------------------*/
            struct findpath_msg fp;

            fp.from_x   = Commander->bact->pos.x;
            fp.from_z   = Commander->bact->pos.z;
            if( TARTYPE_BACTERIUM == target.target_type ) {
                fp.to_x = target.target.bact->pos.x;
                fp.to_z = target.target.bact->pos.z;
                }
            else {
                fp.to_x = target.pos.x;
                fp.to_z = target.pos.z;
                }
            fp.flags    = WPF_Normal;
            fp.num_waypoints = MAXNUM_WAYPOINTS;

            if( yr_AreThereGroundVehicles( Commander->bact ) &&
                _methoda( Commander->o, YBM_FINDPATH, &fp ) ) {

                fp.num_waypoints = MAXNUM_WAYPOINTS;
                _methoda( Commander->o, YBM_SETWAY, &fp );

                /*** Bacterienziel merken ***/
                if( TARTYPE_BACTERIUM == target.target_type ) {

                    Commander->bact->mt_commandid = target.target.bact->CommandID;
                    Commander->bact->mt_owner     = target.target.bact->Owner;
                    }
                }
            else
                _methoda( Commander->o, YBM_SETTARGET, &target );
            }

        yrd->RoboState &= ~ROBO_SETDOCKTARGET;
        }

    /*** Aggression setzen ***/
    _set( Commander->o, YBA_Aggression, yrd->dock_aggr );

    /*** Dock noch einige Zeit blockieren ***/
    yrd->dock_count = 0;
    yrd->dock_user  = 0;
    yrd->dock_time  = YR_DeltaDockTime; // Service schaltet dann auch die Flags aus

    yrd->RoboState &= ~ROBO_USEVHCLSPARE;
}


LONG yr_GetBestKraftWerk( struct yparobo_data *yrd, struct BuildProto *b_array )
{
/*
**  Sucht das beste Kraftwerk, welches wir bauen dürfen und energetisch
**  auch bauen können. Gibt den Index im Array zurück. Sonst -1, wenn wir
**  nix finden.
*/

    WORD   counter, i;
    LONG   found = -1, foundvalue = 0, value;
    UBYTE  mask;

    mask = (UBYTE) ( 1 << yrd->bact->Owner );

    for( counter = 0; counter < NUM_BUILDPROTOS; counter++ ) {

        /*** darf ich bauen? ***/
        if( mask & b_array[counter].FootPrint ) {

            /*** Ist das überhaupt ein Kraftwerk? ***/
            if( b_array[counter].BaseType == BUILD_BASE_KRAFTWERK ) {

                /* --------------------------------------------
                ** habe ich noch genug Energie dafür? Inklusive
                ** "Bausparvertrag"
                ** ------------------------------------------*/
                if( b_array[counter].CEnergy < (yrd->bact->Energy -
                    (LONG)((FLOAT)yrd->bact->Maximum * YR_MinEnergy) +
                    yrd->BuildSpare) ) {

                    /*** Energie ist da. Nun Qualität begutachten ***/
                    value = b_array[counter].Power;

                    /*** für jedes Bakterium gibts 'n Zuschlag ***/
                    for( i=0; i<8; i++ )
                        if( b_array[counter].SBact[ i ].vp ) value += 10;

                    /*** Vergleichen (Gefunden ist immmer größer 0) ***/
                    if( value > foundvalue ) {

                        found      = counter;
                        foundvalue = value;
                        }
                    }
                }
            }
        }
    
    return( found );
}


LONG yr_GetBestRadar( struct yparobo_data *yrd, struct BuildProto *b_array )
{
/*
**  Sucht die beste Radarstation. Dabei wird nur der Footprint eine Rolle spielen.
**  Gibt den Index im Array zurück. Sonst -1, wenn wir nix finden.
*/

    WORD   counter, i;
    LONG   found = -1, foundvalue = 0, value;
    UBYTE  mask;

    mask = (UBYTE) ( 1 << yrd->bact->Owner );

    for( counter = 0; counter < NUM_BUILDPROTOS; counter++ ) {

        /*** darf ich bauen? ***/
        if( mask & b_array[counter].FootPrint ) {

            /*** Ist das überhaupt ein Kraftwerk? ***/
            if( b_array[counter].BaseType == BUILD_BASE_RADARSTATION ) {

                /*** habe ich noch genug Energie dafür? ***/
                if( b_array[counter].CEnergy < (yrd->bact->Energy -
                    (LONG)((FLOAT) yrd->bact->Maximum * YR_MinEnergy) +
                    yrd->BuildSpare) ) {

                    /*** Energie ist da. Power ist hier Footprint-Weite ***/
                    value = b_array[counter].Power;

                    /*** für jedes Bakterium gibts 'n Zuschlag ***/
                    for( i=0; i<8; i++ )
                        if( b_array[counter].SBact[ i ].vp ) value += 10;

                    /*** Vergleichen (Gefunden ist immmer größer 0) ***/
                    if( value > foundvalue ) {

                        found      = counter;
                        foundvalue = value;
                        }
                    }
                }
            }
        }
    
    return( found );
}



LONG yr_GetBestSafety( struct yparobo_data *yrd, struct BuildProto *b_array )
{
/*
**  Sucht die beste FlakStellung. Dabei richten wir uns nach Power, der Designer
**  muß das eintragen
*/

    WORD   counter, i;
    LONG   found = -1, foundvalue = 0, value;
    UBYTE  mask;

    mask = (UBYTE) ( 1 << yrd->bact->Owner );

    for( counter = 0; counter < NUM_BUILDPROTOS; counter++ ) {

        /*** darf ich bauen? ***/
        if( mask & b_array[counter].FootPrint ) {

            /*** Ist das überhaupt ein Kraftwerk? ***/
            if( b_array[counter].BaseType == BUILD_BASE_DEFCENTER ) {

                /* --------------------------------------------
                ** habe ich noch genug Energie dafür? ich nutze
                ** von nun an das Defensebudget....
                ** ------------------------------------------*/
                if( b_array[counter].CEnergy < (yrd->bact->Energy -
                    (LONG)((FLOAT) yrd->bact->Maximum * YR_MinEnergy) +
                    yrd->BuildSpare) ) {

                    /*** Energie ist da. Power ist hier Footprint-Weite ***/
                    value = b_array[counter].Power;

                    /*** für jedes Bakterium gibts 'n Zuschlag ***/
                    for( i=0; i<8; i++ )
                        if( b_array[counter].SBact[ i ].vp ) value += 10;

                    /*** Vergleichen (Gefunden ist immmer größer 0) ***/
                    if( value > foundvalue ) {

                        found      = counter;
                        foundvalue = value;
                        }
                    }
                }
            }
        }
    
    return( found );
}


FLOAT yr_TestVehicle( struct yparobo_data *yrd, APTR data, UBYTE kind,
                      ULONG job )
{
/* ---------------------------------------------------------------------
** Auswahlhilfe für ein Vehicle. Zum test kommen nur die Vehiclewerte
** aus dem VP_Array zum Einsatz. Dort steht, ob es für den Job geeignet
** ist oder nicht. Weiterhin schließen wir gewisse Sachen aus, wenn sie
** total sinnlos für diesen Job sind, ebenso gibt es mindestforderungen.
** -------------------------------------------------------------------*/

    struct VehicleProto *vehicle;
    struct VehicleProto *VP_Array;
    struct WeaponProto  *weapon;
    struct WeaponProto  *WP_Array;
    BOOL   mg;

    /*** Die Arrays holen ***/
    _get( yrd->world, YWA_VehicleArray, &VP_Array );
    _get( yrd->world, YWA_WeaponArray, &WP_Array );

    /*** Was haben wir bekommen? ***/
    if( kind ) 
        vehicle = &( VP_Array[ ((struct Bacterium *) data)->TypeID ] );
    else
        vehicle = (struct VehicleProto *) data;

    if( vehicle->Weapons[ 0 ] != (BYTE) NO_AUTOWEAPON )
        weapon = &( WP_Array[ vehicle->Weapons[ 0 ] ] );
    else
        weapon = NULL;

    if( vehicle->MG_Shot != (BYTE) NO_MACHINEGUN )
        mg = TRUE;
    else
        mg = FALSE;

    /*** Nun auswerten ***/
    switch( job ) {

        case JOB_FIGHTROBO:

            /*** Ohne Waffen sinnlos ***/
            if( !(mg || weapon) ) return( -1 );

            /*** UFO sinnlos ***/
            if( BCLID_YPAUFO == vehicle->BactClassID ) return( -1 );

            /*** ok, nehmen ***/
            return( (FLOAT) vehicle->JobFightRobo );
            break;

        case JOB_FIGHTTANK:

            /*** Ohne Waffen sinnlos ***/
            if( !(mg || weapon) ) return( -1 );

            /*** UFO sinnlos ***/
            if( BCLID_YPAUFO == vehicle->BactClassID ) return( -1 );

            /*** ok, nehmen ***/
            return( (FLOAT) vehicle->JobFightTank );
            break;

        case JOB_FIGHTHELICOPTER:

            /*** Ohne Waffen sinnlos ***/
            if( !(mg || weapon) ) return( -1 );

            /*** UFO sinnlos ***/
            if( BCLID_YPAUFO == vehicle->BactClassID ) return( -1 );

            /*** ok, nehmen ***/
            return( (FLOAT) vehicle->JobFightHelicopter );
            break;

        case JOB_FIGHTFLYER:

            /*** Ohne Waffen sinnlos ***/
            if( !(mg || weapon) ) return( -1 );

            /*** UFO sinnlos ***/
            if( BCLID_YPAUFO == vehicle->BactClassID ) return( -1 );

            /*** ok, nehmen ***/
            return( (FLOAT) vehicle->JobFightFlyer );
            break;

        case JOB_RECONNOITRE:

            /*** ok, nehmen ***/
            return( (FLOAT) vehicle->JobReconnoitre );
            break;

        case JOB_CONQUER:

            /*** Ohne Raketen sinnlos ***/
            if( !weapon ) return( -1 );

            /*** UFO sinnlos ***/
            if( BCLID_YPAUFO == vehicle->BactClassID ) return( -1 );

            /*** ok, nehmen ***/
            return( (FLOAT) vehicle->JobConquer );
            break;
        }

    /*** Nichts gefunden ***/
    return( -1 );
}


struct OBNode *yr_AllocForce( struct yparobo_data *yrd, struct allocforce_msg *af )
{
/*
** neue Variante für die Bereitstellung von geschwadern für besondere
** Aufgaben.
**
** Übergeben wird ein Wunsch, der sich in 4 Masken äußert (siehe TestVehicle)
** Wir durchsuchen zuerst alle wartenden Geschwader, ob deren Commander
** sich dafür eignet. Finden wir nichts, dann Testen wir das VehicleArray
** durch und erzeugen den gefundenen.
**
** Haben wir einen Commander, egal woher, so machen wir einen Energietest
** (CHECKFORCERELATION) und tragen ihn evtl als Dockuser ein.
**
** Die übergebene message enthält das Ziel für das geschwader, die Masken
** und die Entfernung. Das heißt, diese Routine macht jetzt alles! Sie
** kümmert sich auch um die Zielsetzung. Die externen Routinen müssen sich
** nur noch darum kümmern, daß die Slots korrekt abgemeldet werden. Diese
** Routine erledigt also einen kompletten Auftrag.
*/

    struct OBNode *commander, *merkcommander = NULL;
    FLOAT  value, merkvalue = -2.0;
    LONG   merkdiff = 10000000, merksenergy; // das sollte reichen.....
    FLOAT  dist, merkdist = 128 * SECTOR_SIZE;
    BOOL   using_dock = FALSE;
    struct VehicleProto *VArray;
    WORD   i, merk_id[ 3 ];
    FLOAT  ce;
    struct sumparameter_msg sum;
    char   *jt;
    WORD   num_langweiler = 0;
    BOOL   commander_am_robo = FALSE;

    #ifdef __NETWORK__
    struct ypamessage_newvehicle nvm;
    struct sendmessage_msg sm;
    struct ypaworld_data *ywd;
    #endif

    /* ---------------------------------------------------------------------
    ** Energie entsprechend unseres Zustandes korrigieren
    ** und zwar im selben Verhältnis wie derzeitige Energie / Maximalenergie
    ** Mach mr mal net mehr
    ** -------------------------------------------------------------------*/
    //af->energy = (af->energy * yrd->bact->Energy) / yrd->bact->Maximum;

    /*** Durchtesten aller vorhandenen Geschwader ***/
    commander = (struct OBNode *) yrd->bact->slave_list.mlh_Head;
    while( commander->nd.mln_Succ ) {

        /*** Überhaupt prinzipiell geeignet ***/
        if( (commander->bact->MainState   == ACTION_WAIT ) &&
           (!(commander->bact->ExtraState  & EXTRA_EMPTYAKKU)) &&
           (!(commander->bact->ExtraState  & EXTRA_UNUSABLE)) &&
           (!(commander->bact->ExtraState  & EXTRA_ESCAPE)) &&
            (commander->bact->BactClassID != BCLID_YPAGUN) ) {

            /*** Genauer ansehen ***/
            value = yr_TestVehicle( yrd, commander->bact, 1, af->job);

            /* ---------------------------------------------------------
            ** Das ist ein "Gelangweilter". Davon sollten wir nicht
            ** zuviele haben. Wenn die Eignung auch wichtig sein sollte,
            ** eine Zeile nach unten verschieben
            ** -------------------------------------------------------*/
            num_langweiler++;

            /*** Am Robo? Gewusel aufloesen! ***/
            if( ( abs(commander->bact->SectX - yrd->bact->SectX) < 2) &&
                ( abs(commander->bact->SectY - yrd->bact->SectY) < 2) )
                commander_am_robo = TRUE;

            /*** Ein value von 0 heisst auch "geht nicht!" ???***/
            if( (value > -1.0) && (value >= merkvalue) ) {

                /* -------------------------------------------------
                ** Wenn ein gleichgutes geschwader reinkommt, 
                ** entscheidet die Energie. Wichtig ist die Differenz, 
                ** auch wenn wir etwas unter der geforderten Menge
                ** liegen, damit nix sinnlos verpulvrert wird
                ** -----------------------------------------------*/
                LONG   ediff, v;
                struct sumparameter_msg sum;
                BOOL   guenstiger;

                sum.para  = PARA_ENERGY;
                sum.value = 0;
                _methoda( commander->o, YBM_SUMPARAMETER, &sum );
                ediff = abs( sum.value - af->energy );

                dist = (commander->bact->pos.x-af->target_pos.x) *
                       (commander->bact->pos.x-af->target_pos.x) +
                       (commander->bact->pos.z-af->target_pos.z) *
                       (commander->bact->pos.z-af->target_pos.z);

                /* -------------------------------------------------
                ** besser oder bei gleicher Qual. energetisch
                ** guenstiger? 
                ** bei nicht mehr als 20% Abweichung vom geforderten
                ** Budget entscheidet Entfernung, sonst die
                ** Energiedifferenz.
                ** -----------------------------------------------*/
                guenstiger = FALSE;
                if( ediff == 0 ) v = 6;
                else             v = af->energy / ediff;

                if( v > 5 ) {

                    /*** Entfernungsuntersuchung ***/
                    if( dist < merkdist )
                        guenstiger = TRUE;
                    }
                else {

                    /*** Energieuntersuchung ***/
                    if( ediff < merkdiff )
                        guenstiger = TRUE;
                    }

                if( ( value >  merkvalue) ||
                    ((value == merkvalue) && guenstiger) ) {

                    /*** gleiche ediff ist unwahrscheinlich...***/
                    merkdiff      = ediff;
                    merkdist      = dist;
                    merkvalue     = value;
                    merkcommander = commander;
                    merksenergy   = sum.value; // nur fuer debug-msg!
                    }
                }
            }

        /*** nächster ***/
        commander = (struct OBNode *) commander->nd.mln_Succ;
        }

    /* ------------------------------------------------------------
    ** Dann VehicleArray durchtesten. Dabei interessieren nur die
    ** Werte, die besser sind als das bisher gemerkte in merkvalue. 
    ** ----------------------------------------------------------*/
        
    /*** Können wir bauen? ***/
    if( !(yrd->RoboState & ROBO_DOCKINUSE )) {

        sum.value = 0;
        sum.para  = PARA_NUMBER;
        _methoda( yrd->bact->BactObject, YBM_SUMPARAMETER, &sum );
        ce = 1 + CREATE_ENERGY_AUTONOM * sum.value;

        merk_id[ 0 ] = -1;
        merk_id[ 1 ] = -1;
        merk_id[ 2 ] = -1;
        _get( yrd->world, YWA_VehicleArray, &VArray );

        for( i = 0; i < NUM_VEHICLEPROTOS; i++ ) {

            /*** Ist dieser Slot ausgefüllt? ***/
            if( VArray[ i ].BactClassID ) {

                LONG  me;
                UBYTE mask = 1 << yrd->bact->Owner;

                /*** Energie, die ich zur Verfuegung habe ***/
                me = yrd->bact->Energy -
                     (LONG)((FLOAT)yrd->bact->Maximum*YR_MinEnergy);
                if( yrd->RoboState & ROBO_USEVHCLSPARE )
                    me += yrd->VehicleSpare;

                /*** dürfen wir sowas überhaupt bauen? ***/
                if( (VArray[ i ].FootPrint & mask) &&
                    (VArray[ i ].BactClassID != BCLID_YPAROBO) &&
                    (VArray[ i ].BactClassID != BCLID_YPAGUN) &&
                    (VArray[ i ].BactClassID != BCLID_YPAMISSY) &&
                    (VArray[ i ].Energy * ce <  me) ) {

                    BOOL baue_trotzdem;

                    /*** Nun genauer untersuchen ***/
                    value = yr_TestVehicle( yrd, &(VArray[ i ]), 0, af->job );

                    /* -----------------------------------------------
                    ** Merkvalue ist noch aktuell. Wir bauen, wenn
                    ** es besser oder wenn eine Bedingung sagt, dass
                    ** wir auch bei Gleichheit bauen sollen. Diese
                    ** Bedingungen sind:
                    **      ausreichend Energie
                    **      zuwenige "Gelangweilte"
                    ** Dazu muss aber das Dock frei sein!
                    ** ---------------------------------------------*/
                    baue_trotzdem = FALSE;
                    if( num_langweiler < ((yrd->bact->WSecX+yrd->bact->WSecY)/5))
                        baue_trotzdem = TRUE;

                    /*** Wenn die Energie nicht reicht, vergessen ***/
                    if( yrd->bact->Energy < (LONG)(0.9*yrd->bact->Maximum))
                        baue_trotzdem = FALSE;

                    /*** Wenn das Dock besetzt ist, vergessen wir das ganze ***/
                    if( yrd->RoboState & ROBO_DOCKINUSE )
                        baue_trotzdem = FALSE;

                    /*** Gewusel am Robo aufloesen ***/
                    if( commander_am_robo )
                        baue_trotzdem = FALSE;

                    if( value > -1.0 ) {

                        if( (value >  merkvalue) ||
                           ((value == merkvalue) && baue_trotzdem ) ) {

                            /* --------------------------------------
                            ** Wir sammeln maximal 3 Werte, aus denen
                            ** dann zufällig einer ausgewählt wird.
                            ** ------------------------------------*/
                            merkvalue = value;

                            merk_id[ 2 ] = merk_id[ 1 ];
                            merk_id[ 1 ] = merk_id[ 0 ];
                            merk_id[ 0 ] = i;
                            }
                        }
                    }
                }
            }
        }


switch( af->job ) {
    case JOB_FIGHTHELICOPTER: jt = "fight heli";  break;
    case JOB_FIGHTTANK:       jt = "fight tank";  break;
    case JOB_FIGHTFLYER:      jt = "fight flyer"; break;
    case JOB_FIGHTROBO:       jt = "fight robo";  break;
    case JOB_CONQUER:         jt = "conquering";  break;
    case JOB_RECONNOITRE:     jt = "reconnoitre"; break;
    default:                  jt = "unknown job"; break;
    }


    /*** Was gefunden? ***/
    if( (merk_id[ 0 ]  == -1) && (merkcommander == NULL) ) {

        return( NULL );
        }

    if( merk_id[ 0 ] != -1 ) {

        /*** Was neues ist besser geeeignet ***/
        struct createvehicle_msg cv;
        Object *com;
        struct Bacterium *bact;
        LONG   v_energy, BC;
        struct setstate_msg state;
        struct settarget_msg target;
        int    r;

        /* -------------------------------------------------------------
        ** Zufällig aufmischen. das beste mit 60%, dann mit 30% und dann
        ** mit 10% Wahrscheinlichkeit auswählen
        ** -----------------------------------------------------------*/
        r = yrd->bact->internal_time % 10;
        if( (r > 8) && (merk_id[ 2 ] != -1) )
            cv.vp = merk_id[ 2 ];
        else
            if( (r > 5) && (merk_id[ 1 ] != -1) )
                cv.vp = merk_id[ 1 ];
            else
                cv.vp = merk_id[ 0 ];

        /*** Nun erzeugen ***/
        cv.x  = yrd->bact->pos.x + yrd->dock_pos.x;
        cv.y  = yrd->bact->pos.y + yrd->dock_pos.y;
        cv.z  = yrd->bact->pos.z + yrd->dock_pos.z;

        com = (Object *) _methoda( yrd->world, YWM_CREATEVEHICLE, &cv );
        if( !com ) return( NULL );

        /*** abschließende Initialisierungen ***/
        _get( com, YBA_Bacterium, &bact );
        bact->CommandID = YPA_CommandCount++;
        bact->Owner     = yrd->bact->Owner;
        bact->robo      = yrd->bact->BactObject;

        _get( yrd->bact->BactObject, YBA_BactCollision, &BC );
        _set( com, YBA_BactCollision, BC );

        _methoda( yrd->bact->BactObject, YBM_ADDSLAVE, com );

        /*** Id aktualisieren und Message losschicken ***/
        ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
        if( ywd->playing_network ) {

            bact->ident     |= (((ULONG)bact->Owner) << 24);
            bact->CommandID |= (((ULONG)bact->Owner) << 24);

            nvm.generic.message_id = YPAM_NEWVEHICLE;
            nvm.generic.owner      = bact->Owner;
            nvm.pos                = bact->pos;
            nvm.type               = bact->TypeID;
            nvm.master             = bact->master_bact->ident;
            nvm.ident              = bact->ident;
            nvm.vkind              = NV_COMMANDER;
            nvm.command_id         = bact->CommandID;

            sm.receiver_id         = NULL;
            sm.receiver_kind       = MSG_ALL;
            sm.data                = &nvm;
            sm.data_size           = sizeof( nvm );
            sm.guaranteed          = TRUE;
            _methoda( yrd->world, YWM_SENDMESSAGE, &sm );
            }

        /*** Docknutzung bekanntgeben ***/
        yrd->RoboState |= ROBO_DOCKINUSE;

        /*** Diverse Initialisierungen (kann später überschrieben werden)***/
        _set( com, YBA_Aggression, AGGR_AI_STANDARD );

        /*** Als DockUser eintragen ***/
        yrd->dock_user     = bact->CommandID;
        yrd->dock_energy   = af->energy - bact->Energy;
        bact->scale_time   = (LONG)( bact->Maximum * YR_CreationTimeA );
        yrd->dock_time     = bact->scale_time + YR_DeltaDockTime;

        /*** keine Flucht mehr und vorerst warten ***/
        yr_SwitchEscape( bact, 0);
        state.main_state   = ACTION_CREATE;
        state.extra_off    = state.extra_on = 0;
        _methoda( com, YBM_SETSTATE, &state );

        /*** trotzdem Target, nämlich vor Robo, weil Commander ***/
        target.target_type = TARTYPE_SECTOR;
        target.pos.x       = yrd->bact->pos.x + yrd->bact->dir.m31 * 400;
        target.pos.z       = yrd->bact->pos.z + yrd->bact->dir.m33 * 400;
        target.priority    = 0;
        _methoda( com, YBM_SETTARGET, &target );

        /*** CreateEnergy abziehen ***/
        v_energy = (LONG) (ce * (FLOAT)bact->Maximum);
        if( yrd->RoboState & ROBO_USEVHCLSPARE ) {

            if( v_energy <= yrd->VehicleSpare )
                yrd->VehicleSpare -= v_energy;
            else {
                yrd->bact->Energy -= (v_energy - yrd->VehicleSpare);
                yrd->VehicleSpare  = 0;
                }
            }
        else
            yrd->bact->Energy -= v_energy;


        /*** Hack für eine OBNode ... ***/
        merkcommander = &(bact->slave_node);

        using_dock = TRUE;

        /*** Meldung machen ***/
        yr_HistoryCreate( yrd->world, bact );
        }
    else {

        /* ---------------------------------------------------------------
        ** Ein existierendes ist die bessere Wahl.
        **
        ** Wenn das geschwader zu schwach für die Aufgabe ist, dann tragen
        ** wir es als Dockuser ein. Sofern das Dock frei ist...
        ** -------------------------------------------------------------*/
        if( !(yrd->RoboState & ROBO_DOCKINUSE) ) {

            struct sumparameter_msg sum;

            sum.value = 0;
            sum.para  = PARA_ENERGY;
            _methoda( merkcommander->o, YBM_SUMPARAMETER, &sum );

            if( af->energy > sum.value ) {

                /*** Auffüllen ***/
                using_dock = TRUE;
                yrd->dock_user     = merkcommander->bact->CommandID;
                yrd->dock_energy   = af->energy - sum.value;
                }
            }
        }


    /* ---------------------------------------------------------------
    ** Wir haben jetzt einen Commander. Evtl. ist er noch an das Dock
    ** gebunden (weil neu oder zu schwach). Je nachdem, kommt das Ziel
    ** in das Dock oder direkt zum Commander
    ** -------------------------------------------------------------*/
    if( using_dock ) {

        yrd->RoboState |= ROBO_DOCKINUSE;
        yrd->RoboState |= ROBO_SETDOCKTARGET;

        /*** Ziel und Aggr. in das Dock eintragen ***/
        yrd->dtbact      = af->target_bact;
        yrd->dttype      = af->target_type;
        yrd->dtCommandID = af->commandID;
        yrd->dtpos       = af->target_pos;
        yrd->dock_aggr   = af->aggression;
        
        /* ------------------------------------------------------
        ** Falls wir ein Bacterienziel haben, so merken wir uns
        ** in pos seine derzeitige Position, um beim Verschwinden
        ** aus dem Sichtfeld ein Sektorziel einzustellen
        ** ----------------------------------------------------*/
        if( TARTYPE_BACTERIUM == af->target_type ) 
            yrd->dtpos = af->target_bact->pos; 
        }
    else {

        /*** Ziel und Aggr. direkt an Merkcommander übergeben ***/
        struct settarget_msg target;
        target.pos         = af->target_pos;
        target.priority    = 0;
        target.target.bact = af->target_bact;
        target.target_type = af->target_type;
        _methoda( merkcommander->o, YBM_SETTARGET, &target );

        _set( merkcommander->o, YBA_Aggression, (ULONG) af->aggression );
        }

    return( merkcommander );
}


void yr_ClearBuildSlots( struct yparobo_data *yrd )
{
    /* ----------------------------------------------------------------
    ** Manchmal ändert sich die Situation, zum Beispiel durch Bewegung.
    ** Dann werden die bisher angemeldeten Jobs hinfällig (nicht daß
    ** er neue, bessere Positionen dadurch wieder verläßt)
    ** --------------------------------------------------------------*/

    yrd->chk_Power_Pos = yrd->chk_Radar_Pos  =
    yrd->chk_Place_Pos = yrd->chk_Safety_Pos = 0;

    yrd->chk_Power_Value = yrd->chk_Radar_Value  =
    yrd->chk_Place_Value = yrd->chk_Safety_Value = 0;

    yrd->chk_Power_Count = yrd->chk_Radar_Count  =
    yrd->chk_Place_Count = yrd->chk_Safety_Count = 0;

    yrd->RoboState &= ~(ROBO_POWERREADY | ROBO_SAFETYREADY |
                        ROBO_RADARREADY | ROBO_FOUNDPLACE );
}


struct roboattacker *yr_GetFreeAttackerSlot( struct yparobo_data *yrd )
{
    /*** Liefert einen freien Slot fuer einen Roboattacker zurueck ***/
    int    i;

    for( i = 0; i < MAXNUM_ROBOATTACKER; i++ ) {
        if( 0L == yrd->rattack[ i ].attacker_id ) {

            /*** der is frei ***/
            return( &( yrd->rattack[ i ] ) );
            }
        }

    return( NULL );
}


void yr_GetNewRoboSubTarget( struct OBNode *robo, struct OBNode *Commander )
{
    /* --------------------------------------------------------------------
    ** Sucht fuer Commander ein neues Ziel um robo. Finden wir nichts, dann
    ** setzen wir den robo als Ziel.
    ** ------------------------------------------------------------------*/
    int x, y;
    UBYTE mask;
    BOOL  war_robo;

    mask = (UBYTE)(1<<Commander->bact->Owner);

if( (TARTYPE_BACTERIUM == Commander->bact->PrimTargetType) &&
    (BCLID_YPAROBO     == Commander->bact->PrimaryTarget.Bact->BactClassID) )
    war_robo = TRUE;
else
    war_robo = FALSE;

    /*** Die neun umliegenden Sektoren testen ***/
    for( x = -1; x < 2; x++ ) {
        for( y = -1; y < 2; y++ ) {
            if( ((robo->bact->SectX+x)>0) &&
                ((robo->bact->SectX+x)<(robo->bact->WSecX-1)) &&
                ((robo->bact->SectY+y)>0) &&
                ((robo->bact->SectY+y)<(robo->bact->WSecY-1)) ) {

                struct Cell *sector;

                sector = &(robo->bact->Sector[ y * robo->bact->WSecX + x ]);

                /*** Ein Kraftwerk? ***/
                if( (sector->Owner == robo->bact->Owner) &&
                    (WTYPE_Kraftwerk == sector->WType) ) {

                    /*** Ziel auf Kraftwerk ***/
                    yr_SetSectorTarget( Commander->bact,
                       ((FLOAT)(robo->bact->SectX + x) + 0.5) * SECTOR_SIZE,
                       ((FLOAT)(robo->bact->SectY + y) + 0.5) * (-SECTOR_SIZE));

                    /*** Aggression setzen ***/
                    _set( Commander->o, YBA_Aggression, AGGR_FIGHTPRIM );
                    return;
                    }

                /* ------------------------------------------
                ** Eine Flak? Das ist ein geschwader und muss
                ** natuerlich sichtbar sein.
                ** ----------------------------------------*/
                if( sector->FootPrint & (1<<Commander->bact->Owner) ) {

                    struct Bacterium *b;

                    b = (struct Bacterium *) sector->BactList.mlh_Head;
                    while( b->SectorNode.mln_Succ ) {

                        if( (BCLID_YPAGUN      == b->BactClassID) &&
                            (robo->bact->Owner == b->Owner) &&
                            (ACTION_DEAD       != b->MainState) ) {

                            ULONG rgun;

                            /*** keine Bordkanone ***/
                            _get( b->BactObject, YGA_RoboGun, &rgun );
                            if( !rgun ) {

                                /*** Das ist doch was. Chef der Flak als Ziel ***/
                                if( b->robo == b->master )
                                    yr_SetBactTarget( Commander->bact, b);
                                else
                                    yr_SetBactTarget( Commander->bact, b->master_bact);

                                /*** Aggression setzen ***/
                                _set( Commander->o, YBA_Aggression, AGGR_FIGHTPRIM );
                                return;
                                }
                            }

                        b = (struct Bacterium *) b->SectorNode.mln_Succ;
                        }
                    }
                }
            }
        }

    /*** Kein Ziel gefunden, robo als solches setzen ***/
    if( robo->bact->Sector->FootPrint & mask ) {

        yr_SetBactTarget( Commander->bact, robo->bact );

        /*** Aggression setzen ***/
        _set( Commander->o, YBA_Aggression, AGGR_FIGHTPRIM );
        }
    else {

        yr_SetSectorTarget( Commander->bact,
                          ((FLOAT)(robo->bact->SectX) + 0.5) *   SECTOR_SIZE,
                          ((FLOAT)(robo->bact->SectY) + 0.5) * (-SECTOR_SIZE));

        /*** Aggression setzen ***/
        _set( Commander->o, YBA_Aggression, AGGR_FIGHTSECBACT );
        }
}


void yr_ControlRoboAttacker( struct OBNode *Commander, struct roboattacker *ra )
{
    /* ---------------------------------------------------------------
    ** Weil ein Roboangriff etwas anders ablaeuft, gibt es hier eine
    ** Sonderbehandlung. Ziele muessen evtl. ausgetauscht werden, weil
    ** ein Roboangriff auch Attacke der Kraftwerke und Flaks bedeutet.
    **
    ** Wenn der Robo nicht sichtbar ist, wir ihn aber als Ziel setzen
    ** wollen, gibt es ein Sektorziel wo der Robo ist und Aggr3, sonst
    ** Aggr2 fuer den Commander.
    ** -------------------------------------------------------------*/
    struct MinNode *node;
    struct MinList *list;
    struct OBNode  *robo;

    /* -------------------------------------------------------------
    ** Existiert der Robo ueberhaupt noch? Wenn nicht, dann loeschen
    ** wir den Slot und gehen raus.
    ** -----------------------------------------------------------*/
    node = (struct MinNode *) &(Commander->bact->master_bact->slave_node);
    while( node->mln_Pred ) node = node->mln_Pred;
    list = (struct MinList *) node;
    robo = NULL;

    node = list->mlh_Head;
    while( node->mln_Succ ) {

        if( ((struct OBNode *)node)->bact->CommandID == ra->robo_id ) {
            robo = (struct OBNode *) node;
            break;
            }

        node = node->mln_Succ;
        }

    if( !robo ) {

        ra->attacker_id = 0L;
        ra->robo_id     = 0L;
        return;
        }

    /*** Ist dein Hauptziel der Robo? ***/
    if( (TARTYPE_BACTERIUM == Commander->bact->PrimTargetType) &&
        (BCLID_YPAROBO     == Commander->bact->PrimaryTarget.Bact->BactClassID) ) {

        /* ----------------------------------------------------------------
        ** Wir steuern derzeit auf den Robo zu. Da stellen wir uns doch mal
        ** die Frage, ob nicht um den Robo rum noch ein Sektor ist, den wir
        ** zuerst erobern. Zum beispiel ein Kraftwerk, oder ne Flak oder
        ** so...
        ** --------------------------------------------------------------*/
        yr_GetNewRoboSubTarget( robo, Commander );
        }
    else {

        /*** Wir haben ein anderes Ziel ***/

        switch( Commander->bact->PrimTargetType ) {

            case TARTYPE_SECTOR:

                /*** Schon erobert oder Robosektor? ***/
                if( (Commander->bact->PrimaryTarget.Sector->Owner ==
                     Commander->bact->Owner) ||
                    (Commander->bact->PrimaryTarget.Sector ==
                     robo->bact->Sector) ) {

                    /*** Dann mal neues Ziel suchen ***/
                    yr_GetNewRoboSubTarget( robo, Commander );
                    }

                break;

            case TARTYPE_BACTERIUM:

                /* --------------------------------------------
                ** Wird automatisch abgeschalten wenner tot ist
                ** und wird dann ja Sektorziel...
                ** ------------------------------------------*/
                break;
            }
        }
}


void yr_ClearRoboAttackerSlots( struct yparobo_data *yrd )
{
    /* -------------------------------------------------------
    ** Testet, ob die Geschwader zu den Slots noch existieren.
    ** Wenn nicht, wird der Slot geloescht.
    ** -----------------------------------------------------*/
    if( (yrd->bact->internal_time - yrd->clearattack_time) > 4000 ) {

        int i;

        yrd->clearattack_time = yrd->bact->internal_time;

        /*** Nun zu jedem Slot den Commander suchen ***/
        for( i = 0; i < MAXNUM_ROBOATTACKER; i++ ) {

            if( yrd->rattack[ i ].attacker_id != 0L ) {

                struct OBNode *com;
                BOOL   found = FALSE;

                com = (struct OBNode *) yrd->bact->slave_list.mlh_Head;
                while( com->nd.mln_Succ ) {

                    if( com->bact->CommandID == yrd->rattack[ i ].attacker_id ) {

                        found = TRUE;
                        break;
                        }

                    com = (struct OBNode *) com->nd.mln_Succ;
                    }

                /*** Wenn nichts gefunden, dann loeschen ***/
                if( !found ) {

                    yrd->rattack[ i ].attacker_id = NULL;
                    yrd->rattack[ i ].robo_id     = NULL;
                    }
                }
            }
        }
}


void yr_SwitchEscape( struct Bacterium *com, UBYTE wat_n_nu )
{
    /*** Setzt beim Commander und allen Untergebenen den EscapeZustand ***/
    struct OBNode *slave;
    
    if( wat_n_nu )
        com->ExtraState |=  EXTRA_ESCAPE;
    else
        com->ExtraState &= ~EXTRA_ESCAPE;
        
    slave = (struct OBNode *) com->slave_list.mlh_Head;
    while( slave->nd.mln_Succ ) {
    
        if( wat_n_nu )
            slave->bact->ExtraState |=  EXTRA_ESCAPE;
        else
            slave->bact->ExtraState &= ~EXTRA_ESCAPE;
        slave = (struct OBNode *)slave->nd.mln_Succ;
        }
}
        
