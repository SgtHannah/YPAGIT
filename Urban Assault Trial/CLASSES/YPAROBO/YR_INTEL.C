/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

extern UBYTE **RoboLocaleHandle;

/*-----------------------------------------------------------------*/
void yr_rot_round_lokal_x(  struct yparobo_data *yrd, FLOAT angle);
void yr_rot_round_lokal_y(  struct yparobo_data *yrd, FLOAT angle);
void yr_rot_round_global_y( struct yparobo_data *yrd, FLOAT angle);
void yr_CheckWorld( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_RealizeWish( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_Service( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_CheckCommander( struct yparobo_data *yrd, struct trigger_logic_msg *msg,
                        BOOL auto_only );
void yr_AI_RealizeMoving( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_RoboFightIntruder( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
BOOL yr_CollisionCheck( struct yparobo_data *yrd, FLOAT time );
void yr_RemoveTrouble( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_RoboDangerCheck( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_Oueiouaouea( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
BOOL yr_IsComNearest( struct Bacterium *Commander );
void yr_SearchEnemyRobo( struct yparobo_data *yrd );
void yr_DoUserStationEnergyCheck( struct yparobo_data *yrd );
void yr_GiveWayPointToSlaves( struct Bacterium *bact );
void yr_HandleSurfaceStuff( struct yparobo_data *yrd,
                                                        struct trigger_logic_msg *msg );
void yr_DoBeamStuff( struct yparobo_data *yrd, LONG ftime );
void yr_ClearRoboAttackerSlots( struct yparobo_data *yrd );


/*-----------------------------------------------------------------*/
_dispatcher(void, yr_YBM_AI_LEVEL1, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION    Macht nichts weiter, als je nach Wunsch auf AI3 (AI) oder
**              HandleInput (UI) zu verzweigen.
**
**  CHANGED     5.1.96  created     8100 000C
*/
    ULONG  HANDLEINPUT;
    struct yparobo_data *yrd = INST_DATA( cl, o);
    int    i;

    /*** Landed ist egal ***/
    if( ACTION_DEAD == yrd->bact->MainState ) {

        LONG YLS;
        _get( o, YBA_YourLS, &YLS );
        YLS -= msg->frame_time;
        _set( o, YBA_YourLS,  YLS );
        }

    /*** Luftdicht aktualisieren, Rest och ***/
    yrd->bact->air_const = yrd->bact->bu_air_const;
    yrd->bact->sc.src[ VP_NOISE_NORMAL ].pitch  = yrd->bact->bu_pitch;
    yrd->bact->sc.src[ VP_NOISE_NORMAL ].volume = yrd->bact->bu_volume;

    /*** Allen Kanonen Owner mitteilen ***/
    for( i = 0; i < NUMBER_OF_GUNS; i++ ) {

        struct Bacterium *gbact;

        if( yrd->gun[ i ].go ) {

            _get( yrd->gun[ i ].go, YBA_Bacterium, &gbact );
            gbact->Owner = yrd->bact->Owner;
            }
        }

    /*** Energie-Handling ***/
    _methoda(o, YBM_GENERALENERGY, msg);
            
    /*
    ** Tar_Unit für die RealizeMoving's ermitteln. Feinde in AI2, also wie gehabt.
    ** Aber trotzdem extra, weil ich kein Geschwader habe und keine Bakterien-
    ** hauptziele.
    */
    if( yrd->bact->PrimTargetType == TARTYPE_SECTOR ) {

        yrd->bact->tar_vec.x = yrd->bact->PrimPos.x - yrd->bact->pos.x;
        yrd->bact->tar_vec.y = yrd->bact->PrimPos.y - yrd->bact->pos.y;
        yrd->bact->tar_vec.z = yrd->bact->PrimPos.z - yrd->bact->pos.z;
        }
    else
        if( yrd->bact->PrimTargetType == TARTYPE_BACTERIUM ) {
            }
        else {

            yrd->bact->tar_vec.x = 0.0;
            yrd->bact->tar_vec.y = 0.0;
            yrd->bact->tar_vec.z = 0.0;
            }

    _get( o, YBA_UserInput, &HANDLEINPUT );
    if( HANDLEINPUT )
        _methoda( o, YBM_HANDLEINPUT, msg );
    else
        _methoda( o, YBM_AI_LEVEL3, msg );
}


/*-----------------------------------------------------------------*/
_dispatcher(void, yr_YBM_AI_LEVEL3, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION    Völlig neuer Versuch einer Robo-Intelligenz. Die gemeldeten
**              Ereignisse werden nicht mehr bearbeitet, sondern nur noch re-
**              gistriert. Dazu wird ein projekt eröffnet. Dieses wird dann
**              bearbeitet. Wenn weitere Meldungen zu einem Projekt kommen,
**              können diese beachtet oder ignoriert werden. Die Projekte
**              haben die gleichen Identifier wie die Events.
**
**
**  INPUTS
**
**
**  RESULTS
**
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/

    struct yparobo_data *yrd;
    ULONG  BACTCOLL;
    LONG   YLS;
    FLOAT  time, betrag;
    struct bactcollision_msg bcoll;
    struct crash_msg crash;

    /* Vorbereitung */
    yrd = INST_DATA( cl, o );
    _get(o, YBA_BactCollision, &BACTCOLL );

    /*** tar_unit bilden ***/
    betrag = nc_sqrt( yrd->bact->tar_vec.x * yrd->bact->tar_vec.x +
                      yrd->bact->tar_vec.y * yrd->bact->tar_vec.y +
                      yrd->bact->tar_vec.z * yrd->bact->tar_vec.z );

    if( betrag > 0.0 ) {

        yrd->bact->tar_unit.x = yrd->bact->tar_vec.x / betrag;
        yrd->bact->tar_unit.y = yrd->bact->tar_vec.y / betrag;
        yrd->bact->tar_unit.z = yrd->bact->tar_vec.z / betrag;
        }
    else {

        yrd->bact->tar_unit.x = yrd->bact->tar_unit.y = 0.0;
        yrd->bact->tar_unit.z = 0.0;
        }


    /* der Robo sollte seine Bewegung nur über Richtungen, nicht aber über Ziele
    ** erledigen, denn die Zielbearbeitung machen die Untergebenen oder sie
    ** folgt aus den Gehirninhalten. Die Richtungen stehen in tar_unit */
    time = (FLOAT) msg->frame_time / 1000.0;

    switch( yrd->bact->MainState ) {

        case ACTION_NORMAL:

            /*** wurde eine Bewegung angemeldet? ***/
            if( yrd->RoboState & ROBO_MOVE ) {

                yr_DoBeamStuff( yrd, msg->frame_time );
                }


            if( BACTCOLL ) {
                bcoll.frame_time = msg->frame_time;
                if( _methoda( o, YBM_BACTCOLLISION, &bcoll) )
                    return;  /* Das Return kommt evtl. wieder weg... */
                }

            
            /* -----------------------------------------------------------
            ** Wegen dem Setzen der Luftdichte checken wir zuerst die Welt
            ** und dann bewegen wir uns.
            ** ---------------------------------------------------------*/
            yr_CollisionCheck( yrd, time );

            /*** Bewegen ***/
            if( yrd->RoboState & ROBO_USERROBO )
                yr_Oueiouaouea( yrd, msg );
            else
                yr_AI_RealizeMoving( yrd, msg );

            /* ----------------------------------------------------------
            ** Die Oberflächenereignisse darf ich nur auswerten, wenn ich
            ** der UserRobo bin
            ** --------------------------------------------------------*/
            if( yrd->RoboState & ROBO_USERROBO ) {

                /* --------------------------------------------------------
                **             Nach FeindRobo suchen (für Meldungen)
                ** ------------------------------------------------------*/
                yr_SearchEnemyRobo( yrd );
                yr_DoUserStationEnergyCheck( yrd );

                /* ------------------------------------------------------
                ** Auswerten der Ereignisse von Oberfläche.
                ** Das mach' mr auch, wenn wir autonom, aber Spieler sind
                ** ----------------------------------------------------*/
                                yr_HandleSurfaceStuff( yrd, msg );
                                }

                
            /* --------------------------------------------------------
            ** Folgendes mache ich nur, wenn ich nicht der UserRobo bin 
            ** ------------------------------------------------------*/
            
            if( !(yrd->RoboState & ROBO_USERROBO) ) {

                /*** Infos sammeln und Projekte erstellen ***/
                yr_CheckWorld( yrd, msg );

                /*** Mache Projekte aus Infos ***/
                yr_RealizeWish( yrd, msg );

                /*** Diverses... ***/
                yr_Service( yrd, msg );

                /*** Feind bekämpfen ***/
                // yr_RoboFightIntruder( yrd, msg );

                /*** Roboattackerslots evtl. loeschen ***/
                yr_ClearRoboAttackerSlots( yrd );
                }

            /* -------------------------------------------
            ** Folgendes mache ich immer, egal wer ich bin
            ** -----------------------------------------*/

            /*** Commander überwachen ***/
            yr_CheckCommander( yrd, msg, TRUE );

            /* ---------------------------------------------------------
            ** Reaktion auf Eindringlinge. Wir machen einen test. Dieser
            ** versendet, wenn notwendig und wenn UserRobo, die meldung.
            ** die reaktion darauf (sprich removeTrouble) macht nur der
            ** Autonome
            ** -------------------------------------------------------*/
            yr_RoboDangerCheck( yrd, msg );
            if( (yrd->RoboState & ROBO_INDANGER) &&
                (!(yrd->RoboState & ROBO_USERROBO)) )
                yr_RemoveTrouble( yrd, msg );

            break;

        case ACTION_DEAD:
            
            /* -----------------------------------------------------
            ** Ein Robo fällt nicht. Sofort In Megadeth übergehen.
            ** Deshalb auch hier nicht die Standardmethode verwenden
            ** (Überlagerung ist Overkill)
            ** Achtung, DWD wird jetzt vom Robo ueberlagert!
            ** ---------------------------------------------------*/
            
            _methoda( o, YBM_DOWHILEDEATH, msg );          
            break;
        }

}


void yr_RemoveTrouble( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
{
    /* --------------------------------------------------------------------------
    ** Das "Ärger-Flag" ist gesetzt. Wir müssen darauf reagieren. Wir testen
    ** also in gewissen Zeitabständen, ob Leute gerade keine Nebenziele
    ** haben. Wenn dem so ist, dann geben wir ihnen den Robo als Hauptziel, nicht
    ** die feinde selbst, die finden sie schon, zumal Nachschub zu erwarten ist.
    **
    ** Wenn es mir besonders schlecht geht, dann beachte ich auch die
    ** Unusable-Leute und die "Nebenzieler".
    **
    ** Erweiterungen: Unterschiedliche Grenzen/Schwellwerte für die Auswahl
    ** ------------------------------------------------------------------------*/

    /*** Wenn ich mehr als 20% Energie habe, mache ich nix ***/
    if( ((FLOAT)yrd->bact->Energy / (FLOAT)yrd->bact->Maximum) >= YR_MinEnergy )
            return;

    /*** Ist seit dem letzten Test genug Zeit vergangen ? ***/
    if( (msg->global_time - yrd->enemy_time) > YR_Enemy_Time ) {

        BOOL   schlimm;
        struct OBNode *commander;

        /*** Zeit merken ***/
        yrd->enemy_time = msg->global_time;

        /* ------------------------------------------------------------
        ** Zustand besonders kritisch? Achtung, 20% Energy sind normal.
        ** Bis dahin darf er sich legal verausgaben
        ** ----------------------------------------------------------*/
        if( ((FLOAT)yrd->bact->Energy / (FLOAT)yrd->bact->Maximum) < (YR_MinEnergy / 2) )
            schlimm = TRUE;
        else
            schlimm = FALSE;

        /*** Nun alle Commander checken ***/
        commander = (struct OBNode *) yrd->bact->slave_list.mlh_Head;
        while( commander->nd.mln_Succ ) {

            /*** Dürfen wir überhaupt? ***/
            if( (ACTION_NORMAL == commander->bact->MainState) ||
                (ACTION_WAIT   == commander->bact->MainState) ) {

                if( BCLID_YPAGUN != commander->bact->BactClassID ) {

                    BOOL   nehmen = FALSE;
                    struct settarget_msg stg;

                    /*** Auswahl, je nach schlimm oder nicht schlimm ***/
                    if( schlimm ) {

                        nehmen = TRUE;

                        /*** Unusable ausschalten! jetzt herrscht Panik! ***/
                        commander->bact->ExtraState &= ~EXTRA_UNUSABLE;
                        }
                    else {

                        if( (commander->bact->ExtraState & EXTRA_UNUSABLE) ||
                            (commander->bact->SecTargetType != TARTYPE_NONE) )
                            nehmen = FALSE;
                        else
                            nehmen = TRUE;
                        }

                    /* -------------------------------------------------
                    ** Wenn er in meiner Nähe ist und mich als Hauptziel
                    ** hat, dann setze ich ihn, wenn es mir schlecht
                    ** geht, auf eine hohe Aggression und lösche sein
                    ** ESCAPE-Flag, so daß er sich für mich verausgabt.
                    ** -----------------------------------------------*/

                    if( ((commander->bact->pos.x - yrd->bact->pos.x) *
                         (commander->bact->pos.x - yrd->bact->pos.x) +
                         (commander->bact->pos.z - yrd->bact->pos.z) *
                         (commander->bact->pos.z - yrd->bact->pos.z) <
                         SECTOR_SIZE * SECTOR_SIZE * 3 ) &&
                        (TARTYPE_BACTERIUM == commander->bact->PrimTargetType) &&
                        (yrd->bact == commander->bact->PrimaryTarget.Bact) ) {

                        if( schlimm ) {

                            _set( commander->o, YBA_Aggression, AGGR_ALL - 1);
                            commander->bact->ExtraState &= ~EXTRA_ESCAPE;
                            }
                        else
                            _set( commander->o, YBA_Aggression, AGGR_AI_STANDARD);
                        }

                    /*** Zielzuweisung ***/
                    if( nehmen ) {

                        stg.priority    = 0;
                        stg.target.bact = yrd->bact;
                        stg.target_type = TARTYPE_BACTERIUM;
                        _methoda( commander->o, YBM_SETTARGET, &stg );
                        }
                    }
                }

            /*** next one ***/
            commander = (struct OBNode *) commander->nd.mln_Succ;
            }
        }
}


void yr_RoboDangerCheck( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
{
    /* --------------------------------------------------------------------
    ** testet, ob Feinde angekommen sind (umliegende 8 Sektoren + Mitte,
    ** da dies Nebenzielbereich ist). Dann Meldung machen. Reaktion obliegt
    ** dem UserRobo nicht.
    **
    ** Es hat sich als Blödsinn herausgestellt, die Autonomemsache in
    ** CheckWorld zu machen, also wird es für User und Autonomer nicht
    ** getrennt. (Nur die Meldung...)
    ** ------------------------------------------------------------------*/

    BOOL found_one = FALSE;
    LONG i, j, found_ID;

    for( i = -1; i <= 1; i++ ) {

        for( j = -1; j <= 1; j++ ) {

            /*** korrektes Gebiet? ***/
            if( ((yrd->bact->SectX + i) > 0)                    &&
                ((yrd->bact->SectX + i) < (yrd->bact->WSecX-2)) &&
                ((yrd->bact->SectY + j) > 0)                    &&
                ((yrd->bact->SectY + j) < (yrd->bact->WSecY-2)) ) {

                struct Cell *sector;
                struct Bacterium *sbact;

                /*** Sektor holen ***/
                sector = &(yrd->bact->Sector[ j * yrd->bact->WSecX + i ]);

                /* ----------------------------------------------
                ** Leute dort checken. Müssen anderen Owner haben
                ** und bewaffnet sein und leben
                ** --------------------------------------------*/

                sbact = (struct Bacterium *) sector->BactList.mlh_Head;
                while( sbact->SectorNode.mln_Succ ) {

                    /*** Nun testen ***/
                    if( sbact->Owner != yrd->bact->Owner) {

                        /*** lebendig? ***/
                        if( ACTION_DEAD != sbact->MainState ) {

                            /*** bewaffnet? ***/
                            if( (sbact->auto_ID != NO_AUTOWEAPON) ||
                                (sbact->mg_shot != NO_MACHINEGUN) ) {

                                found_one = TRUE;
                                found_ID  = sbact->CommandID;
                                break;
                                }
                            }
                        }

                    /*** nächstes bacterium ***/
                    sbact = (struct Bacterium *) sbact->SectorNode.mln_Succ;
                    }
                }
            if( found_one ) break;
            }

        if( found_one ) break;
        }


    /*** Meldung machen ??? ***/
    if( found_one ) {

        /*** Flag schon gesetzt? wegen meldung und so... ***/
        if( (!(yrd->RoboState & ROBO_INDANGER)) &&
            (yrd->RoboState & ROBO_USERROBO) ) {

            struct bact_message log;

            log.ID     = LOGMSG_USERROBODANGER;
            log.para1  = found_ID;
            log.para2  = log.para3 = 0;
            log.sender = yrd->bact;
            log.pri    = 99;
            _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log);
            }

        yrd->RoboState |= ROBO_INDANGER;
        }
    else yrd->RoboState &= ~ROBO_INDANGER;
}



void yr_rot_round_lokal_y2( struct flt_m3x3 *dir, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );


    rm.m11 = cos_y;        rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;          rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;       rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, dir, &neu_dir);

    *dir = neu_dir;
}


_dispatcher( void, yr_YBM_DOWHILEDEATH, struct trigger_logic_msg *msg )
{
/*
** FUNCTION             Wie das Vorbild der BactClass, nur ohne fallen. Somit
**                              entsteht der plasma-Batzen in der Luft.
**                              Da wir nun hierfuer ne eigene methoda haben, kann man
**                              die Skalierungen auch etwas abenteuerlicher machen.     
**
** INPUT        Vor allem zeiten über die TLMessage
**
** CHANGED      na jetzt, wann sonst?
*/

    struct yparobo_data *yrd = INST_DATA( cl, o );
    LONG   YourLastSeconds;

    _get( o, YBA_YourLS, &YourLastSeconds );
            
    /*** Wenn noch nicht Megadeth, dann schalten ***/
    if( !(yrd->bact->ExtraState & EXTRA_MEGADETH ) ) {

        struct setstate_msg state;
        state.extra_off = state.main_state = 0;
        state.extra_on  = EXTRA_MEGADETH;
        _methoda( o, YBM_SETSTATE, &state );
        }

    /*** Aufschlag vortäuschen (z.B. für YLS) ***/
    yrd->bact->ExtraState |= EXTRA_LANDED;

    /* ------------------------------------------
    ** Wenn das Ding mal einen Owner hatte, dann 
    ** war es ein rchtiges vehicle und darf einen
    ** Plasmaklumpen hinterlassen. Achtung, das
    ** muss auch uebers Netz!
    ** ----------------------------------------*/
    if( (yrd->bact->Owner) &&
        (yrd->bact->vis_proto_create != NULL) ) { 
        
        LONG  sc_time = (LONG)(PLASMA_TIME*(FLOAT)yrd->bact->Maximum);
        FLOAT sc_fct  = PLASMA_SCALE;

        if( sc_time < PLASMA_MINTIME )
                sc_time = PLASMA_MINTIME;
        if( sc_time > PLASMA_MAXTIME )
                sc_time = PLASMA_MAXTIME;
                
        if( yrd->bact->extravp[0].flags & EVF_Active ) {
            
            yrd->bact->scale_time -= msg->frame_time;
            if( yrd->bact->scale_time <= 0 ) {
                
                /* -------------------------------------------
                ** Flags nicht veraendern, sonst faengt alles
                ** wieder von vorn an, weil das Objekt noch da 
                ** ist, das ACTIVE-Flag aber nicht gesetzt ist
                ** -----------------------------------------*/
                yrd->bact->extravp[0].vis_proto = NULL;
                yrd->bact->extravp[0].vp_tform  = NULL;
                
                if( YourLastSeconds <= 0) {
                                        
                    LONG UI;
                    _get( o, YBA_UserInput, &UI );
                    if( UI )
                        yrd->bact->ExtraState |= EXTRA_DONTRENDER;
                    else    
                        _methoda( o, YBM_RELEASEVEHICLE, o );
                    }    
                }
            else { 
            
                FLOAT angle, radikant;
                    
                /*** Scalierung runter ***/
                radikant = (FLOAT)yrd->bact->scale_time / (FLOAT)sc_time;
                                yrd->bact->extravp[0].scale = sc_fct * nc_sqrt( radikant );
                
                if( yrd->bact->extravp[0].scale < 0.0) 
                    yrd->bact->extravp[0].scale = 0.0;
                    
                /*** Winkel neu berechnen ***/
                angle = 2.0 * yrd->bact->max_rot * (FLOAT)msg->frame_time / 1000.0;
                yr_rot_round_lokal_y2( &(yrd->bact->extravp[0].dir),angle ); 
 
                                /*** Weil plasma i.a. laenger dauert...***/
                if( YourLastSeconds <= 0)
                    yrd->bact->ExtraState |= EXTRA_DONTRENDER;
                }    
            }
        else {
        
            struct ypaworld_data *ywd;
            
            /*** Wir fangen erst an, also init ***/
            yrd->bact->scale_time           = sc_time;
            yrd->bact->extravp[0].scale     = sc_fct;
            yrd->bact->extravp[0].pos       = yrd->bact->pos;
            yrd->bact->extravp[0].dir       = yrd->bact->dir; 
            
            yrd->bact->extravp[0].vis_proto = yrd->bact->vis_proto_create;
            yrd->bact->extravp[0].vp_tform  = yrd->bact->vp_tform_create;
            yrd->bact->extravp[0].flags    |= (EVF_Scale | EVF_Active);
            
            #ifdef __NETWORK__
            ywd = INST_DATA( ((struct nucleusdata *)(yrd->world))->o_Class,
                             yrd->world );
            if( ywd->playing_network ) { 
            
                struct sendmessage_msg sm;
                struct ypamessage_startplasma sp;
            
                /*** Message versenden ***/
                sp.generic.message_id = YPAM_STARTPLASMA;
                sp.generic.owner      = yrd->bact->Owner;
                sp.scale              = sc_fct;
                sp.time               = sc_time;
                sp.ident              = yrd->bact->ident; 
                sp.pos                = yrd->bact->pos;
                sp.dir                = yrd->bact->dir;       
                
                sm.receiver_id        = NULL;
                sm.receiver_kind      = MSG_ALL;
                sm.data               = &sp;
                sm.data_size          = sizeof( sp );
                sm.guaranteed         = TRUE;
                _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                }                 
            #endif
            } 
                     
        }
    else {
        
        /*** kein Plasme, Freigeben, wenn YLS auf 0 ist ***/
        if( YourLastSeconds <= 0 ) {
                        
            LONG UI;
            _get( o, YBA_UserInput, &UI );

            if( UI )
                yrd->bact->ExtraState |= EXTRA_DONTRENDER;
            else       
                _methoda( o, YBM_RELEASEVEHICLE, o);
            }    
        }    
}














