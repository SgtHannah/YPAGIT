/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Sterberoutine
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypamissileclass.h"
#include "ypa/ypahistory.h"
#include "input/input.h"

#ifdef __NETWORK__
#include "ypa/ypamessages.h"
#include "network/networkclass.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

extern UBYTE **BactLocaleHandle;
void yb_CopyWayPoints( struct Bacterium *from, struct Bacterium *to );

_dispatcher(void, yb_YBM_DIE, void *nix)
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

    struct OBNode *kandidat, *waffe, *tester, *attacker;
    struct settarget_msg st;
    struct ypabact_data  *ybd;
    struct setstate_msg state;
    struct bact_message log;
    Object *user_robo;
    ULONG  mapmaxx, mapmaxy;
    struct ypa_HistVhclKill vkill;

    #ifdef __NETWORK__
    struct sendmessage_msg sm;
    struct ypamessage_die dm;
    struct ypaworld_data *ywd;
    #endif

    ybd = INST_DATA(cl, o );

    /*** Schon alles erledigt? ***/
    if( ybd->bact.ExtraState & EXTRA_LOGICDEATH )
        return;        

    /*** Brauchen wir für die uswahl eines neuen Commanders ***/
    _get( ybd->world, YWA_MapMaxX, &mapmaxx );
    _get( ybd->world, YWA_MapMaxY, &mapmaxy );
    
    /* --------------------------------------------------------------
    ** später müssen wir alle Leichen dem WO übergeben. jetzt bleiben
    ** wir einfach in der Liste und ketten den Auserwählten hinten an 
    ** ------------------------------------------------------------*/

    #ifdef __NETWORK__
    dm.generic.message_id = YPAM_DIE;
    dm.generic.owner      = ybd->bact.Owner;
    dm.ident              = ybd->bact.ident;
    dm.new_master         = NULL;
    dm.landed             = 0;
    dm.class              = ybd->bact.BactClassID;
    if( ybd->bact.killer )
        dm.killerowner    = ybd->bact.killer->Owner;
    else
        dm.killerowner    = 0;
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    #endif

    if( ybd->bact.slave_list.mlh_Head->mln_Succ ) {

        /* ------------------------------------------------
        ** dann haben wir Untergebene. Suchen wir also den
        ** mir am nächsten, die energetische Betrachtung
        ** hat sich als ungünstig herausgestellt.
        **
        ** NEU: Alle toten Slaves kommen auf Commanderebene
        ** so daß wir niemanden mehr unter uns haben!
        ** ----------------------------------------------*/

        kandidat = NULL;
        tester   = (struct OBNode *) ybd->bact.slave_list.mlh_Head;

        while( tester->nd.mln_Succ ) {

            struct OBNode *next = (struct OBNode *) tester->nd.mln_Succ;

            if( ACTION_DEAD != tester->bact->MainState ) {

                FLOAT tester_distance, kandidat_distance;

                tester_distance = (tester->bact->pos.x - ybd->bact.pos.x) *
                                  (tester->bact->pos.x - ybd->bact.pos.x) +
                                  (tester->bact->pos.z - ybd->bact.pos.z) *
                                  (tester->bact->pos.z - ybd->bact.pos.z);

                if( kandidat )
                    kandidat_distance = (kandidat->bact->pos.x - ybd->bact.pos.x) *
                                        (kandidat->bact->pos.x - ybd->bact.pos.x) +
                                        (kandidat->bact->pos.z - ybd->bact.pos.z) *
                                        (kandidat->bact->pos.z - ybd->bact.pos.z);
                else
                    kandidat_distance = (mapmaxx * mapmaxx +
                                         mapmaxy * mapmaxy) *
                                        SECTOR_SIZE * SECTOR_SIZE; // >= als Welt

                /* ----------------------------------------------------
                ** UFOs sind ungünstig, sollten aber auf Grund reiner
                ** UFO-Geschwader beachtet werden. 128 ist die maximale
                ** Levellänge in Sektoren. ACHTUNG: Ich arbeite aus
                ** Geschwindigkeitsgründen mit den Quadraten!
                ** --------------------------------------------------*/
                if( BCLID_YPAUFO == tester->bact->BactClassID )
                    tester_distance = (mapmaxx * mapmaxx +
                                       mapmaxy * mapmaxy) *
                                      SECTOR_SIZE * SECTOR_SIZE - 1000; // weil sonst nicht kleiner als größte Distanz

                /*** ein neuer ***/
                if( tester_distance <= kandidat_distance )
                    kandidat = tester;
                }
            else {

                /* --------------------------------------------------------------
                ** Dreckig!!! Hier entsteht scheinbar ein neues Geschwader,
                ** ein Anwenden von DIE geht ohne weiteres nicht (Haufm Abstürze)
                ** somit machen wir einfach ein Flag, nichts mehr zu melden.
                ** ------------------------------------------------------------*/

                if( (ULONG)(ybd->bact.master) > 2L )
                    _methoda( ybd->bact.master, YBM_ADDSLAVE, tester->o );
                else
                    _methoda( ybd->world, YWM_ADDCOMMANDER, tester->o );
                tester->bact->ExtraState |= EXTRA_NOMESSAGES;
                }

            tester = next;
            }

        /*** Gibt es einen Nachfolger ? ***/
        if( kandidat ) {

            /* -------------------------------------------------------------------
            ** kandidat ist der Kandidat. Soso! Wir removen ihn, hängen ihn
            ** an meinen Chef (Welt oder O.) und übergeben ihm dann die restlichen
            ** Objekte 
            ** -----------------------------------------------------------------*/

            if( ybd->bact.master == (Object *) 1L ) {

                /*** an das WO ***/
                _methoda( ybd->world, YWM_ADDCOMMANDER, kandidat->o );
                }
            else {

                /*** mein Chef ist auch bloß ein Bakterium ***/
                _methoda( ybd->bact.master, YBM_ADDSLAVE, kandidat->o );
                }

            /*** nun bekommt er all meine Untergebenen ***/
            while( ybd->bact.slave_list.mlh_Head->mln_Succ ) {

                tester = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                _methoda( kandidat->o, YBM_ADDSLAVE, tester->o );
                }

            /*** Zielübergabe an kandidat! ***/
            st.pos.x       = ybd->bact.PrimPos.x;
            st.pos.y       = ybd->bact.PrimPos.y;
            st.pos.z       = ybd->bact.PrimPos.z;
            st.target.bact = ybd->bact.PrimaryTarget.Bact;
            st.target_type = ybd->bact.PrimTargetType;
            st.priority    = 0;
            _methoda( kandidat->o, YBM_SETTARGET, &st );
            yb_CopyWayPoints( &(ybd->bact), kandidat->bact );

            /* --------------------------------------------------------------
            ** Nun bekommt der kandidat die meine CommandID, damit die Gegner
            ** uns nicht verlieren 
            ** ------------------------------------------------------------*/
            kandidat->bact->CommandID     = ybd->bact.CommandID;
            // kandidat->bact->ProjectNumber = ybd->bact.ProjectNumber;
            kandidat->bact->Aggression    = ybd->bact.Aggression;

            #ifdef __NETWORK__
            /*** Das Umschichten auch auf anderen Maschinen erzwingen ***/
            if( ywd->playing_network && (ybd->bact.Owner != 0) ) {

                dm.new_master = kandidat->bact->ident;
                }
            #endif

            /* ---------------------------------------------------------------
            ** das war für diesen Fall erstmal alles. Ich selbst bleibe in der
            ** Liste, denn sonst bekommt RELEASEVEHICLE Probleme 
            ** -------------------------------------------------------------*/
            }
        else {

            struct OBNode *com, *slave, *next_com, *next_slave;

            kandidat = NULL;

            /* -----------------------------------------------------------------
            ** Ich bin tot und meine Untergebenen sind es auch, diese müssen
            ** für YLS aber noch getriggert werden, evtl. länger als ich "lebe".
            ** Für diesen seltsamen Fall (z.B. Tod des Robos) werden alle
            ** Untergebenen direkt an die Welt übergeben. Ich gehe dabei
            ** 2 Ebenen tief.
            ** der kandidatenPointer bleibt NULL, weil ja niemand gefunden wurde.
            ** PS.: Das kann auch im normalen Spiel passieren, hält aber nur
            ** kurz an, so daß wir es nicht übers Netz zu schicken brauchen!
            ** ----------------------------------------------------------------*/

            com = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
            while( com->nd.mln_Succ ) {

                /*** Zuerst die Slaves an die Welt ***/
                slave = (struct OBNode *) com->bact->slave_list.mlh_Head;

                while( slave->nd.mln_Succ ) {

                    next_slave = (struct OBNode *) slave->nd.mln_Succ;
                    _methoda( ybd->world, YWM_ADDCOMMANDER, slave->o );

                    if( ACTION_DEAD != slave->bact->MainState )
                        _LogMsg("Scheisse, da hängt noch ein Lebendiger unter der Leiche! owner %d, state %d, class %d\n",
                                 slave->bact->Owner, slave->bact->MainState, ybd->bact.BactClassID );

                    slave = next_slave;
                    }

                /*** jetzt der Commander ***/
                next_com = (struct OBNode *) com->nd.mln_Succ;
                _methoda( ybd->world, YWM_ADDCOMMANDER, com->o );
                com = next_com;
                }
            }
        }
    else
        kandidat = NULL;

    /* -------------------------------------------------------------------------
    ** Wenn der Kandidatenpointer 0 ist, dann war ich der letzte und muß nun der
    ** Welt begreiflich machen, daß es das geschwader nicht mehr gibt
    ** Das machen wir nur, wenn wir dem User gehören! Ich verlasse mich darauf,
    ** daß der UserRobo-Pointer immer sinnvoll ist. Außerdem hat das natürlich
    ** nur Sinn, wenn ich Commander war!
    ** -----------------------------------------------------------------------*/
    _get( ybd->world, YWA_UserRobo, &user_robo );
    if( (kandidat == NULL) && (ybd->bact.master == ybd->bact.robo) &&
        (!(ybd->bact.ExtraState & EXTRA_NOMESSAGES)) ) {

        /*** Der letzte ist tot ***/
        if( ybd->bact.robo == user_robo ) {

            /* -------------------------------------------------------
            ** Einer von meinen Leuten. Wenn es eine Gun war, also ein
            ** Gun-geschwader vernichtet wurde, dann sehe ich mir den
            ** Sektor an und entscheide daran, ob eine Flak oder eine
            ** Radarstation zerstört wurde.
            ** -----------------------------------------------------*/
            if( BCLID_YPAGUN == ybd->bact.BactClassID ) {

                if( (NO_AUTOWEAPON == ybd->bact.auto_ID) &&
                    (NO_MACHINEGUN == ybd->bact.mg_shot) ) {

                    /*** Unbewaffnet? Eine Schüssel! ***/
                    log.ID     = LOGMSG_RADAR_DESTROYED;
                    log.pri    = 80;
                    }
                else {

                    /*** bewaffnet. Eine Flak ***/
                    log.ID     = LOGMSG_FLAK_DESTROYED;
                    log.pri    = 80;
                    }

                log.para1  = log.para2 = log.para3 = 0;  // brauch mr ja nix
                log.sender = &(ybd->bact);
                _methoda( ybd->bact.robo, YRM_LOGMSG, &log);
                }
            else {

                /*** Es war ein normales Geschwader ***/
                if( !(ybd->bact.ExtraState & EXTRA_CLEANUP) ) {

                    log.para1  = ybd->bact.CommandID;
                    log.para2  = log.para3 = 0;
                    log.sender = &(ybd->bact);  // ich darf noch einige Sekunden verwendet werden
                    log.ID     = LOGMSG_LASTDIED;
                    log.pri    = 44;
                    _methoda( ybd->bact.robo, YRM_LOGMSG, &log);
                    }
                }
            }
        else {

            /* -------------------------------------------------------
            ** Ein fremder. Nur was schicken, wenn er von einem meiner
            ** Leute gekillt wurde
            ** -----------------------------------------------------*/
            if( ybd->bact.killer &&
                (ybd->bact.killer->robo == user_robo) ) {

                log.ID     = LOGMSG_ENEMYDESTROYED;
                log.sender = &(ybd->bact);
                log.para1  = ybd->bact.PrimCommandID;
                log.para2  = log.para3  = 0;
                log.pri    = 36;
                _methoda( ybd->bact.robo, YRM_LOGMSG, &log );
                }
            }
        }


    /* -------------------------------------------------------------------
    ** Nun müssen wir uns von unseren Angreifern verabschieden. Da gibt es
    ** 2 Fälle: Wir haben keine Untergebenen, dann löschen wir bei allen
    ** Angreifern das Ziel, oder aber wir haben einen Nachfolger, dann be-
    ** kommen meine Angreifer den als Ziel, weil sie sonst das Geschwader
    ** verlieren würden 
    ** -----------------------------------------------------------------*/

    while( attacker = (struct OBNode *) _RemHead( (struct List *) &(ybd->attck_list) ) ) {

        /* -----------------------------------------------------------------
        ** Um SETTARGET verwenden zu können, muß vorher TARTYPE_NONE gesetzt
        ** werden, weil wir aus der Liste schon raus sind! 
        **
        ** Es scheint so, als wäre der dritte und größte und bösartigste
        ** Remove-Bug gefunden. Siehe dazu die if-Statements. 5-6 Stunden
        ** habe ich im letzten Anlauf benötigt, und selbst als ich ihn
        ** gefunden hatte, hat er sich noch über 1/2 Stunde gewehrt. Aber
        ** jetzt ist er exorziert und Amok geht einer lichten Zukunft ent-
        ** gegen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        **      af 11.11. 96   0.05 Uhr
        ** ---------------------------------------------------------------*/
        struct OBNode *snode, *pnode;
        _get( attacker->o, YBA_PtAttckNode, &pnode );
        _get( attacker->o, YBA_StAttckNode, &snode );

        if( pnode == attacker ) {

            /*** Wir waren sein Hauptziel ***/
            attacker->bact->PrimaryTarget.Bact = NULL;
            attacker->bact->PrimTargetType = TARTYPE_NONE;
            attacker->bact->assess_time    = 0;
            }
        else {

            /*** else: kann ja nur eine der beiden Nodes Node gewesen sein ***/
            if( snode == attacker ) {

                /* Wir waren (auch) sein Nebenziel. Da ist alles  egal */
                attacker->bact->SecondaryTarget.Bact = NULL;
                attacker->bact->SecTargetType = TARTYPE_NONE;
                attacker->bact->assess_time   = 0;
                }
            else {
                _LogMsg("Hein Blöd\n");
                }               
            }
        }



    /* ------------------------------------------------------------------------
    ** Falls ich noch'n paar Waffen habe, übergebe ich die meinem
    ** Vorgesetzten. Er braucht sie ja nur zu triggern..., aber sonst
    ** bleiben sie in der Luft oder explodieren völlig sinnlos. Der Vorgesetzte 
    ** ist nicht der kandidat, denn den muß es ja nicht geben. Ich übergebe
    ** sie einfach meinem Vorgesetzten. Das geht nur schief, wenn ich der Robo
    ** bin und da lasse ich alles 
    ** ----------------------------------------------------------------------*/
    
    if( ((ULONG)(ybd->bact.master)) > 2L ) {

        while(waffe=(struct OBNode *)_RemHead((struct List *)&ybd->bact.auto_list)) {

            /*** umschichten ***/
            _AddTail( (struct List *) &(ybd->bact.master_bact->auto_list),
                      (struct Node *) waffe);

            /*** neuer Schütze ***/
            _set( waffe->o, YMA_RifleMan, ybd->bact.master_bact );
            }
        }
    else {

        /* --------------------------------------------------------------
        ** Wir haben keinen Chef mehr und lassen die Raketen explodieren.
        ** Das wird nur intern gemacht, nicht verschickt, sondern aufm
        ** anderen Rechner auch gemacht!
        ** ------------------------------------------------------------*/
        while(waffe=(struct OBNode *)_RemHead((struct List *)&ybd->bact.auto_list)) {

            /*** Viewer rücksetzen ***/
            _methoda( waffe->o, YMM_RESETVIEWER, NULL);
    
            state.main_state = ACTION_DEAD;
            state.extra_on = state.extra_off = 0;
            _methoda( waffe->o, YBM_SETSTATE_I, &state );

            /*** Aber wir melden sie auch ab, du KNALLTÜTE! ***/
            st.target_type = TARTYPE_NONE;
            st.priority    = 0;
            _methoda( waffe->o, YBM_SETTARGET, &st );

            /*** auch gleich freigeben ***/
            waffe->bact->master = NULL;
            _methoda( ybd->world, YWM_RELEASEVEHICLE, waffe->o); // nix messageing
            }
        }

    /* -----------------------------------------------------------------------
    ** Natürlich können wir auch Angreifer gewesen sein. Wenn dem so ist, dann
    ** nehmen wir unsere Node von unserem Ziel weg. 
    ** ---------------------------------------------------------------------*/
    if( ybd->bact.SecTargetType == TARTYPE_BACTERIUM )
        _Remove( (struct Node *) &(ybd->st_attck_node) );
    if( ybd->bact.PrimTargetType == TARTYPE_BACTERIUM )
        _Remove( (struct Node *) &(ybd->pt_attck_node) );
    ybd->bact.PrimTargetType = ybd->bact.SecTargetType = TARTYPE_NONE;


    /* -----------------------------------------------------------------------
    ** Ich setze meine GeschwaderID auf 0, damit die Leute wissen, daß ich nix
    ** bearbeite. Das ist wichtig. 
    ** ---------------------------------------------------------------------*/
    ybd->bact.CommandID     = 0L;
    // ybd->bact.ProjectNumber = No_Project;


    /*** Licht aus! ***/
    ybd->bact.MainState   = ACTION_DEAD;
    ybd->bact.ExtraState |= EXTRA_LOGICDEATH;
    ybd->bact.dead_entry  = ybd->bact.internal_time;

    /* -----------------------------------------------------------------
    ** Nachtrag: Wenn ich tot, aber nicht zerstört bin UND gelandet bin,
    ** dann zerstöre ich mich selbst, es sein denn, ich bin im finalen
    ** CREATE-Zustand
    ** ---------------------------------------------------------------*/
    if( ybd->bact.ExtraState & EXTRA_LANDED ) {

        if( (ybd->bact.vpactive == VP_JUSTNORMAL) ||
            (ybd->bact.vpactive == VP_JUSTWAIT) ) {

            state.main_state = state.extra_off = 0;
            state.extra_on   = EXTRA_MEGADETH;
            _methoda( o, YBM_SETSTATE_I, &state );

            #ifdef __NETWORK__
            if( ywd->playing_network && (ybd->bact.Owner != 0) ) {

                dm.landed = 1;
                }
            #endif
            }
        }

    /* -------------------------------------------------------------------
    ** NEU: Wenn ich nicht Viewer bin, aber User war, dann saß mein
    ** Schütze in einer Rakete und hat einen neuen Master, an welchen er
    ** übergeben wurde. Folglich muß ich bei mir UI ausschalten, damit ich
    ** korrekt freigegeben werden kann.
    ** -----------------------------------------------------------------*/
    if( (ybd->flags & YBF_UserInput) &&
        (!(ybd->flags & YBF_Viewer)) &&
        (ybd->bact.master_bact) )
        _set( o, YBA_UserInput, FALSE );


    /* ---------------------------------------------------------------------
    ** Die restlichen Sachen der Sterbemessage und das Abschicken. Um die
    ** Zahl der Messages so gering wie möglich zu halten, wird soviel wie
    ** möglich in eine Message gepackt:
    **  neuer Master
    **  Setstate (LANDED)
    **  Waffenfreigebae wird ebenfalls auf der anderen Seite separat gemacht
    **  Für Robos gibt es eine Extra-Message
    ** -------------------------------------------------------------------*/
    #ifdef __NETWORK__
    if( ywd->playing_network && (ybd->bact.Owner != 0) &&
        (ybd->bact.BactClassID != BCLID_YPAROBO) ) {

        sm.data                = &dm;
        sm.data_size           = sizeof( dm );
        sm.receiver_kind       = MSG_ALL;
        sm.receiver_id         = NULL;
        sm.guaranteed          = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
    #endif

    /*** Todesmeldung für die History, wenn es kein Brocken war... ***/
    if( ybd->bact.Owner ) {
        UBYTE owner;
        if( ybd->bact.killer ) {
            owner = ybd->bact.killer->Owner;
            vkill.cmd    = YPAHIST_VHCLKILL;
            vkill.owners = ( (owner<<3) | (ybd->bact.Owner) );
            vkill.vp     = ybd->bact.TypeID;
            vkill.pos_x  = (UBYTE)((ybd->bact.pos.x*256)/ybd->bact.WorldX);
            vkill.pos_z  = (UBYTE)((ybd->bact.pos.z*256)/ybd->bact.WorldZ);
            _methoda( ybd->world, YWM_NOTIFYHISTORYEVENT, &vkill );
        }
    }
}



/*** Gibt es ein Leben vor dem Tod? ***/

_dispatcher( void, yb_YBM_REINCARNATE, void *nix )
{
/*
**  FUNCTION    Setzt alle, nicht über die bactstruktur vom WO erreichbaren
**              Sachen wieder in den Ausgangszustand. Ist besonders für
**              YWM_OBTAINVEHICLE interessant.
**
**  INPUT/RESULT    ---
**
**  created am Tag 1 nach dem Telefon   8100 000C
**  updated am Tag 0 nach dem Dagmars Geburtstag
*/

    struct ypabact_data *ybd;
    LONG   MX, MY;
    int    i;

    ybd = INST_DATA( cl, o );

    ybd->flags           = YBF_CheckVisExactly;
    ybd->bact.ExtraState = 0L;
    ybd->bact.robo       = NULL;       // um Füllelemente rauszukriegen
    
    /*** Subklassen überschreiben ***/
    ybd->YourLastSeconds = YBA_YourLastSeconds_DEF;

    /*** Ziele rücksetzen ***/
    ybd->bact.PrimTargetType = TARTYPE_NONE;
    ybd->bact.SecTargetType  = TARTYPE_NONE;
    ybd->bact.PrimCommandID  = 0;

    /*** Die Levelgröße ***/
    _get( ybd->world, YWA_MapSizeX, &MX );
    _get( ybd->world, YWA_MapSizeY, &MY );
    ybd->bact.WorldX =  ((FLOAT) MX ) * SECTOR_SIZE;
    ybd->bact.WorldZ = -((FLOAT) MY ) * SECTOR_SIZE;
    ybd->bact.WSecX  = MX;
    ybd->bact.WSecY  = MY;

    /*** Diverses ***/
    ybd->bact.CommandID  = 0;
    ybd->bact.FWay       = FlyBack;
    ybd->bact.killer     = 0;

    /*** Schutz vor verfrühtem Selbstmord ***/
    ybd->bact.bf_time   = 0;
    ybd->bact.bf2_time  = 0;
    ybd->bact.mpos.x    = 0.0;
    ybd->bact.mpos.y    = 0.0;
    ybd->bact.mpos.z    = 0.0;

    /*** merken der originalen TonHöhen ***/
    ybd->bact.bu_volume = ybd->bact.sc.src[ VP_NOISE_NORMAL ].volume;
    ybd->bact.bu_pitch  = ybd->bact.sc.src[ VP_NOISE_NORMAL ].pitch;

    /*** GunAngle-Merkzelle bekommt den init-Wert ***/
    ybd->bact.gun_angle_user = ybd->bact.gun_angle;
    ybd->bact.gun_leftright  = 0.0;

    /* ----------------------------------------------------------
    ** Alle zeiten rücksetzen, weil mit internal_time der globale
    ** Count auch neu beginnt!!!
    ** --------------------------------------------------------*/
    ybd->bact.scale_time     = 0;
    ybd->bact.internal_time  = 0;
    ybd->bact.time_ai1       = 0;
    ybd->bact.time_ai2       = 0;
    ybd->bact.time_test1     = 0;
    ybd->bact.time_test2     = 0;
    ybd->bact.time_search2   = 0;
    ybd->bact.time_search3   = 0;
    ybd->bact.sliderx_time   = 0;
    ybd->bact.slidery_time   = 0;
    ybd->bact.mg_time        = 0;
    ybd->bact.last_weapon    = 0;
    ybd->bact.newtarget_time = 0;
    ybd->bact.assess_time    = 0;
    ybd->bact.scale_count    = 0;
    ybd->bact.scale_delay    = 0;
    ybd->bact.startbeam      = 0;
    ybd->bact.time_energy    = 0;
    ybd->bact.fe_time        = -FE_TIME; // denn feindkontakt sofort sein
    ybd->bact.salve_count    = 0;
    ybd->bact.kill_after_shot= FALSE;

    /*** Defaultmäßig sind's erstmal alles Lander ***/
    ybd->flags |= YBF_LandWhileWait;

    /*** Effekte löschen ***/
    for( i = 0; i < NUM_DESTFX; i ++ ) ybd->bact.DestFX[ i ].proto = 0;

    /*** Extra-Visprotos loeschen samt flags und Schnickschnack ***/
    memset( ybd->bact.extravp, 0, sizeof( ybd->bact.extravp ));
}

_dispatcher( void, yb_YBM_RELEASEVEHICLE, Object *dead_man )
{
/*
**  FUNCTION    Für alle vehicle, deren Sterben über das netz
**              public gemacht werden soll, verwendet ich besser eine
**              eigene Methode. Erstens kapsele ich hier den Netzkram,
**              der viele Objekte (Brocken) gar nix angeht und zweitens
**              brauche ich nicht zu untersuchen, ob ich Host bin,
**              denn das bin ich immer, wenn ich aufrufe. Denn ein Release-
**              vehicle darf im gegensatz zum Setstate->dead nur der
**              Host machen
**
**  REST        Wie zugehörige Weltklassenmethode
*/

    #ifdef __NETWORK__
    struct sendmessage_msg sm;
    struct ypamessage_destroyvehicle dvm;
    struct ypaworld_data *ywd;
    struct Bacterium *b;
    #endif

    struct ypabact_data *ybd = INST_DATA( cl, o);

    #ifdef __NETWORK__
    /*** Abmeldung wenn es etwas sinnvolles war ***/
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    _get( dead_man, YBA_Bacterium, &b );

    /*** Raketen werden nur erzeugt und machen dann alles allein ***/
    if( (b->Owner != 0) && (ywd->playing_network) &&
        (BCLID_YPAMISSY != b->BactClassID) ) {

        dvm.generic.message_id = YPAM_DESTROYVEHICLE;
        dvm.generic.owner      = b->Owner;
        dvm.ident              = b->ident;
        dvm.class              = b->BactClassID;

        sm.data                = &dvm;
        sm.data_size           = sizeof( dvm );
        sm.receiver_kind       = MSG_ALL;
        sm.receiver_id         = NULL;
        sm.guaranteed          = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
    #endif

    /*** das originale Abmelden ***/
    _methoda( ybd->world, YWM_RELEASEVEHICLE, dead_man );

}


