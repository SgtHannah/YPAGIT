/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Sterberoutine des Robo
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>
#include <stdio.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypamissileclass.h"
#include "ypa/yparoboclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

extern ULONG  YPA_CommandCount; // brauchen wir hier für die Robos
extern UBYTE  **RoboLocaleHandle;

struct Cell *yr_FirstSector( struct yparobo_data *yrd );
void yr_RemoveAttacker( struct Bacterium *bact );

#define YR_NUM_CONFIG_ITEMS (2)
struct ConfigItem yr_ConfigItems[YR_NUM_CONFIG_ITEMS] = {
    {"game.newai", CONFIG_BOOL, TRUE},
    {"game.timeline", CONFIG_INTEGER, 600000}   
};

_dispatcher(void, yr_YBM_DIE, void *nix)
{
    /* ------------------------------------------------------------------
    ** |                 Karl Ranseier ist tot!                         |
    ** ------------------------------------------------------------------
    **
    ** Wir gehen in den Sterbezustand über und melden uns ab. Haben wir
    ** Untergebene, machen wir den Energiereichsten zum neuen Kommander.
    ** Wir müssen beachten, daß wir auch Robo sein können (zumindest
    ** in testphasen - es kann also sein, daß wir uns vom WO abmelden
    ** müssen).
    */

    struct yparobo_data  *yrd;
    struct MinList *commlist, *slavelist;
    struct OBNode  *commander, *slave, *next_commander, *next_slave;
    struct bact_message log;
    Object *user_robo;
    struct setstate_msg state;
    LONG   YLS;
    struct ypaworld_data *ywd;

    #ifdef __NETWORK__
    struct ypamessage_robodie rd;
    struct sendmessage_msg sm;
    #endif

    yrd = INST_DATA(cl, o );

    _get( o, YBA_YourLS, &YLS );

    /* ------------------------------------------------------------------------
    ** 
    ** ----------------------------------------------------------------------*/
    
    /*** Schon alles erledigt? ***/
    if( yrd->bact->ExtraState & EXTRA_LOGICDEATH )
        return;
        
    #ifdef __NETWORK__
    ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
    rd.generic.message_id = YPAM_ROBODIE;
    rd.generic.owner      = yrd->bact->Owner;
    if( yrd->bact->killer ) 
        rd.killer         = yrd->bact->killer->ident;
    else 
        rd.killer         = 0L;
    rd.killerowner        = yrd->bact->killer_owner;

    #endif

    /*** Wenn es einen Killer gab, dann NOTIFY. auch in yw_network.c !!! ***/
    if( (yrd->bact->killer_owner) &&
        (!(yrd->bact->ExtraState & EXTRA_CLEANUP)) ) {

        struct notifydeadrobo_msg ndr;

        /*** Meldung an die Welt ***/
        ndr.killer_id = yrd->bact->killer_owner;
        ndr.victim    = yrd->bact;
        _methoda( yrd->world, YWM_NOTIFYDEADROBO, &ndr );
        }

    /* ------------------------------------------------------------------
    ** Weil aus meinen Untergebenen kein neuer Robo gemacht werden kann,
    ** lasse ich sie auf spektakuläre Art und Weise explodieren. Dazu
    ** mache ich sie tot mit em VisProto für Create. Ne.
    ** -----------------------------------------------------------------*/
    
       
    /*** Denn sonst ist das Spiel zu Ende ***/
    commlist = &(yrd->bact->slave_list);
    commander = (struct OBNode *) commlist->mlh_Head;
    while( commander->nd.mln_Succ ) {

        next_commander = (struct OBNode *) commander->nd.mln_Succ;

        /*** Die Untergebenen ranholen ***/
        slavelist = &(commander->bact->slave_list);
        slave = (struct OBNode *) slavelist->mlh_Head;
        while( slave->nd.mln_Succ ) {

            next_slave = (struct OBNode *) slave->nd.mln_Succ;

            /*** Killer mit begreiflich machen ***/
            slave->bact->killer = yrd->bact->killer;

            /*** logisch töten ***/
            if( yrd->bact->ExtraState & EXTRA_CLEANUP )
                slave->bact->ExtraState |= EXTRA_CLEANUP;
                
            _methoda( slave->o, YBM_DIE, NULL );
            slave->bact->ExtraState &= ~EXTRA_LANDED;

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

            /*** Nachfolger ***/
            slave = next_slave;
            }

        /*** Die gleichen Sachen noch für den Commander ***/

        /*** Killer mit begreiflich machen ***/
        commander->bact->killer = yrd->bact->killer;

        /*** logisch töten ***/
        if( yrd->bact->ExtraState & EXTRA_CLEANUP )
            commander->bact->ExtraState |= EXTRA_CLEANUP;
            
        _methoda( commander->o, YBM_DIE, NULL );
        commander->bact->ExtraState &= ~EXTRA_LANDED;

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

        /*** Nachfolger ***/
        commander = next_commander;
        }

    _get( yrd->world, YWA_UserRobo, &user_robo );

    if( o != user_robo ) {   // vielleicht auch für UserRobo? evtl Extra im else-Zweig

        /*** Allen sagen, daß es mich erwischt hat ***/
        log.ID     = LOGMSG_ROBODEAD;
        log.para1  = yrd->bact->Owner;
        log.para2  = log.para3 = 0;
        log.sender = yrd->bact->killer;
        log.pri    = 50;
        _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log);
        }
    else {

        /*** Allen sagen, daß es mich erwischt hat ***/
        if( !(yrd->bact->ExtraState & EXTRA_CLEANUP) ) {

            log.ID     = LOGMSG_USERROBODEAD;
            log.para1  = 0;
            log.para2  = log.para3 = 0;
            log.sender = yrd->bact;
            log.pri    = 100;
            _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log);
            }
        }

    
    /*** Nun rufe ich die Supermethode auf. ***/
    _supermethoda( cl, o, YBM_DIE, NULL );

    if( ywd->playing_network && (yrd->bact->Owner != 0) ) {

        Object *vo;

        sm.data                = &rd;
        sm.data_size           = sizeof( rd );
        sm.receiver_kind       = MSG_ALL;
        sm.receiver_id         = NULL;
        sm.guaranteed          = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );

        /* -------------------------------------------------------------------
        ** Jaja, das Netzwerk, was alles so passiert, wenn sich die Vehicle
        ** nicht selbst updaten und ich permanent ein auge auf die geschwader-
        ** struktur haben muß...
        ** Weil ich immer, auch wenn alles tot ist, Messages verschicken muß,
        ** brauche ich als zentralen Einsprungpunkt den Robo. Folglich darf er
        ** nicht freigegeben werden. Das passiert aber, wenn ich nicht drinnen
        ** sitze. Also hatte Floh die geniale Idee, sich doch dann immer
        ** in den Robo zu setzen, wenn die Station (im Netzmodus) kaputt geht!
        ** -----------------------------------------------------------------*/
        if( o == ywd->UserRobo ) {

            vo = (Object *) ywd->UserVehicle;
            _set( vo, YBA_UserInput, FALSE );
            _set( vo, YBA_Viewer,    FALSE );
            _set(  o, YBA_UserInput, TRUE );
            _set(  o, YBA_Viewer,    TRUE );
            }

        /* ----------------------------------------------------------------
        ** Apropos Netzwerk. Wenn ich der letze Robo der Userrobo ist, dann
        ** mache ich Siegesmeldung.
        ** QUARK!!! Das macht YPAM_ROBODIE!!!
        ** --------------------------------------------------------------*/
//        if( yrd->bact->Owner != ywd->gsr->NPlayerOwner ) {
//
//            BOOL found_player_robo = FALSE, found_enemy_robo = FALSE;
//            struct OBNode *robo = (struct OBNode *)ywd->CmdList.mlh_Head;
//
//            while( robo->nd.mln_Succ ) {
//
//                if( (BCLID_YPAROBO          == robo->bact->BactClassID) &&
//                    (ywd->gsr->NPlayerOwner == robo->bact->Owner) &&
//                    (ACTION_DEAD            != robo->bact->MainState) )
//                    found_player_robo = TRUE;
//
//                if( (BCLID_YPAROBO          == robo->bact->BactClassID) &&
//                    (ywd->gsr->NPlayerOwner != robo->bact->Owner) &&
//                    (ACTION_DEAD            != robo->bact->MainState) )
//                    found_enemy_robo = TRUE;
//
//                robo = (struct OBNode *) robo->nd.mln_Succ;
//                }
//
//            /*** Was losschicken? ***/
//            if( found_player_robo && (!found_enemy_robo) ) {
//
//                //struct logmsg_msg log;
//
//                /*** Der Spieler ist der letzte Lebendige ***/
//                //log.pri  = 10;
//                //log.msg  = ypa_GetStr( RoboLocaleHandle,
//                //           STR_NET_YOUWIN, "YOU WIN THE GAME");
//                //log.bact = NULL;
//                //log.code = 0;
//                //_methoda( ywd->world, YWM_LOGMSG, &log );
//                }
//            }
        }

}        


_dispatcher(void, yr_YBM_REINCARNATE, void *nix)
{
    struct yparobo_data *yrd;
    yrd = INST_DATA( cl, o);
    
    /*** nur so, passiert ja nich', aber wenn doch, dann ausfüllen ***/
    _supermethoda( cl, o, YBM_REINCARNATE, NULL );

    /*** Checkworld-Sachen ***/
    yrd->chk_Radar_Value    = 0;
    yrd->chk_Enemy_Value    = 0;
    yrd->chk_Power_Value    = 0;
    yrd->chk_Safety_Value   = 0;
    yrd->chk_Terr_Value     = 0;
    yrd->chk_Place_Value    = 0;
    yrd->chk_Robo_Value     = 0;
    yrd->chk_Recon_Value    = 0;

    yrd->chk_Power_Count    = 0;
    yrd->chk_Radar_Count    = 0;
    yrd->chk_Safety_Count   = 0;
    yrd->chk_Terr_Count     = 0;

    yrd->chk_Radar_Delay    = 0;
    yrd->chk_Enemy_Delay    = 0;
    yrd->chk_Power_Delay    = 0;
    yrd->chk_Safety_Delay   = 0;
    yrd->chk_Terr_Delay     = 0;
    yrd->chk_Robo_Delay     = 0;
    yrd->chk_Recon_Delay    = 0;
    yrd->chk_Place_Delay    = 0;

    yrd->BuildSpare         = 0;
                
    yrd->FirstSector        = yr_FirstSector( yrd );

    /*** Dock reinigen ***/
    yrd->dock_energy        = 0;
    yrd->dock_count         = 0;
    yrd->dock_time          = 0;
    yrd->dttype             = 0;
    yrd->dock_aggr          = 0;
    yrd->dtCommandID        = 0;
    yrd->dtbact             = NULL;
    yrd->dtpos.x            = 0.0;
    yrd->dtpos.z            = 0.0;

    /*** Zeiten ***/
    yrd->dock_time          = 0;
    yrd->enemy_time         = 0;
    yrd->BeamInTime         = 0;
    yrd->clearattack_time   = 0;
    yrd->controlattack_time = 0;

    yrd->RoboState = 0L;

    /*** Die Guns sollten freigegeben sein, folglich Pointer löschen ***/
    memset( yrd->gun, 0, sizeof( struct gun_data ) * NUMBER_OF_GUNS );
    memset( yrd->rattack, 0, sizeof( struct roboattacker ) * MAXNUM_ROBOATTACKER );

    /*** Wird in Amok immer aufgerufen, somit hier verwendbar ***/
    yrd->bact->CommandID = YPA_CommandCount++;  // Problem: Ich habe hier keinen
                                                // Eigentümer fürs Networking
                                                // Geht es auch ohne

    /*** YourLastSeconds ist anders ***/
    _set( o, YBA_YourLS, YBA_YourLastSeconds_Robo_DEF );

    
    /*** AI-Initialisierungen ***/
    _GetConfigItems(NULL,yr_ConfigItems,YR_NUM_CONFIG_ITEMS);
    yrd->NewAI    = (BOOL)yr_ConfigItems[0].data;
    yrd->TimeLine = (int)yr_ConfigItems[1].data;
}


struct Cell *yr_FirstSector( struct yparobo_data *yrd )
{
    /*
    ** Ermittelt den ersten Sektor. Der Sektorpointer muß noch 
    ** nicht ausgefüllt sein! Auch die Position kann Scheiße sein! Ich
    ** lasse mir einfach den Sektor 0,0 geben.
    */
    
    struct getsectorinfo_msg gsi;

    gsi.abspos_x =  0.5 * SECTOR_SIZE;
    gsi.abspos_z = -0.5 * SECTOR_SIZE;
    _methoda( yrd->world, YWM_GETSECTORINFO, &gsi );

    return( gsi.sector );
}

