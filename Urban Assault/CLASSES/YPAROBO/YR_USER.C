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
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypatankclass.h"
#include "ypa/ypagunclass.h"
#include "input/input.h"

#ifdef __NETWORK__
#include "network/networkflags.h"
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

extern ULONG YPA_CommandCount;
extern UBYTE **RoboLocaleHandle;


/*-----------------------------------------------------------------*/
void yr_dir_rot_round_lokal_x( struct flt_m3x3 *dir, FLOAT angle);
void yr_dir_rot_round_lokal_y( struct flt_m3x3 *dir, FLOAT angle);
void yr_rot_round_lokal_x(  struct yparobo_data *yrd, FLOAT angle);
void yr_rot_round_lokal_y(  struct yparobo_data *yrd, FLOAT angle);
void yr_rot_round_global_y( struct yparobo_data *yrd, FLOAT angle);
void yr_RealizeMoving( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
BOOL yr_CollisionCheck( struct yparobo_data *yrd, FLOAT time );
void yr_AI_RealizeMoving( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
BOOL yr_TestMoveCommand( struct trigger_logic_msg *msg );
void yr_CheckCommander( struct yparobo_data *yrd, struct trigger_logic_msg *msg,
                        BOOL auto_only);
void yr_MerkeDieAltenDinger( struct yparobo_data *yrd );
void yr_Oueiouaouea( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
void yr_RoboDangerCheck( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
BOOL yr_IsComNearest( struct Bacterium *Commander );
void yr_HistoryCreate( Object *world, struct Bacterium *bact );
void yr_SearchEnemyRobo( struct yparobo_data *yrd );
void yr_DoUserStationEnergyCheck( struct yparobo_data *yrd );
void yr_GiveWayPointToSlaves( struct Bacterium *bact );
void yr_HandleSurfaceStuff( struct yparobo_data *yrd, struct trigger_logic_msg *msg );
yr_DoBeamStuff( struct yparobo_data *yrd, LONG frame_time );
BOOL yr_AreThereGroundVehicles( struct Bacterium *commander );
void yr_SetBactTarget( struct Bacterium *commander, struct Bacterium *targetbact );
void yr_SetSectorTarget( struct Bacterium *commander, FLOAT tarpoint_x, FLOAT tarpoint_z );


/*-----------------------------------------------------------------*/
_dispatcher(void, yr_YBM_HANDLEINPUT, struct trigger_logic_msg *msg)
/*
**  FUNCTION
**
**      Realisiert die Verwaltung der Oberfläche und die Auswertung
**      der Eingabeereignisse. Die Oberfläche wird hier nicht gezeichnet,
**      das macht TR_VISUAL. Die Oberfläche ist ein Array, welches
**      in diesem Quelltextfragment global definiert wird (das macht ja 
**      jede Subklasse neu).
**
**      Es werden hier 2 Ereignisse unterschieden. Das können zum einen
**      GameEvents und zum anderen Surfaceevents sein. Bei Surfaceevents
**      (mit rechter Maustaste) ist der Mauszeiger eingeblendet und ein
**      LMB war der Versuch, einen Teil der Oberfläche zu bedienen.
**
**  INPUTS
**
**  RESULTS
**
**      Ermittelt Kraftvektor für Flugverhalten (Move)
**
**  CHANGED
**      28-Jun-95   8100 000C    created
*/
{
    struct yparobo_data *yrd;
    ULONG  BACTCOLL;
    FLOAT  time, betrag;
    struct bactcollision_msg bcoll;
    struct settarget_msg target;
    struct setposition_msg setpos;
    struct setstate_msg state;

    yrd = INST_DATA( cl, o);
    _get( o, YBA_BactCollision, &BACTCOLL );
    time = (FLOAT) msg->frame_time / 1000.0;


    /*** Tar_unit bilden ***/
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


    /*
    ** Nach den Zuständen unterscheiden
    */

    switch( yrd->bact->MainState ) {

        case ACTION_NORMAL:

            /*** wurde eine Bewegung angemeldet? ***/
            if( yrd->RoboState & ROBO_MOVE ) {

                yr_DoBeamStuff( yrd, msg->frame_time );
                }


            /*** Bakterienkollision ***/
            if( BACTCOLL ) {

                bcoll.frame_time = msg->frame_time;
                if( _methoda(o, YBM_BACTCOLLISION, &bcoll)) return;
                }

            /*----------------------------------------------------------
            **      Überprüfen der eigenen Leute (für Flucht etc.)
            **--------------------------------------------------------*/
            yr_CheckCommander( yrd, msg, FALSE );

            /*---------------------------------------------------------
            **               Umgebung mal abchecken
            **-------------------------------------------------------*/
            yr_RoboDangerCheck( yrd, msg );

            /* --------------------------------------------------------
            **             Nach FeindRobo suchen und Energy
            ** ------------------------------------------------------*/
            yr_SearchEnemyRobo( yrd );
            yr_DoUserStationEnergyCheck( yrd );

            /*---------------------------------------------------------
            **               Tastaturbezogene Ereignisse
            **-------------------------------------------------------*/
            
            /*** Ausrichten ***/
            if( msg->input->Buttons & BT_STOP )
                _methoda( o, YBM_STOPMACHINE, msg );

            /*------------------------------------------------------------
            **             oberflächenbezogene Ereignisse
            **----------------------------------------------------------*/
                        yr_HandleSurfaceStuff( yrd, msg );

            /* ----------------------------------------------------------
            ** Bewegung als solche gibt es nicht mehr. In folgender
            ** ehemliger UserRoutine werten wir nur noch die Kopfbewegung
            ** aus
            ** --------------------------------------------------------*/
            yr_RealizeMoving( yrd, msg );
            yr_Oueiouaouea( yrd, msg );

            /*** Matrix des Viewers aktualisieren ***/
            yrd->bact->Viewer.dir = yrd->bact->dir;

            /*** Eindrehen ***/
            yr_dir_rot_round_lokal_y( &(yrd->bact->Viewer.dir), 
                                        yrd->bact->Viewer.horiz_angle );
            yr_dir_rot_round_lokal_x( &(yrd->bact->Viewer.dir), 
                                        yrd->bact->Viewer.vert_angle );

            break;

        case ACTION_DEAD:

            /*** DWD ist jetzt robospezifisch ***/
            _methoda( o, YBM_DOWHILEDEATH, msg );
            break;

        case ACTION_WAIT:

            /*** bloß auf normal schalten ***/
            state.main_state = ACTION_NORMAL;
            state.extra_on = state.extra_off = 0;
            _methoda( o, YBM_SETSTATE, &state );

            /*** Landed ausschalten ***/
            yrd->bact->ExtraState &= ~EXTRA_LANDED;

            break;
        }
    
}


void yr_RealizeMoving( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
{
    /* -----------------------------------------------------------------------
    ** UserBewegung ist nur noch Kopfdrehung. Dazu kommt das Schwanken und die
    ** Kanonenkorrektur
    ** ---------------------------------------------------------------------*/

    FLOAT time, x;

    time = (FLOAT) msg->frame_time / 1000.0;        

    /*** Kopf-Drehkommandos auszuwerten ***/
    if( msg->input->Slider[ SL_FLY_HEIGHT ] > 0.001 ) {

        x = msg->input->Slider[ SL_FLY_HEIGHT ] *
            yrd->bact->max_rot * time * 2;

        yrd->bact->Viewer.vert_angle += x;
        
        /*** Begrenzung ***/
        if( yrd->bact->Viewer.vert_angle >  yrd->bact->Viewer.max_up )   
            yrd->bact->Viewer.vert_angle =  yrd->bact->Viewer.max_up;
        if( yrd->bact->Viewer.vert_angle < -yrd->bact->Viewer.max_down ) 
            yrd->bact->Viewer.vert_angle = -yrd->bact->Viewer.max_down;
        }
    else {

        if( msg->input->Slider[ SL_FLY_HEIGHT ] < -0.001 ) {
            
            x = msg->input->Slider[ SL_FLY_HEIGHT ] *
                yrd->bact->max_rot * time * 2;

            yrd->bact->Viewer.vert_angle += x;
        
            /*** Begrenzung ***/
            if( yrd->bact->Viewer.vert_angle >  yrd->bact->Viewer.max_up )   
                yrd->bact->Viewer.vert_angle =  yrd->bact->Viewer.max_up;
            if( yrd->bact->Viewer.vert_angle < -yrd->bact->Viewer.max_down ) 
                yrd->bact->Viewer.vert_angle = -yrd->bact->Viewer.max_down;
            }
        }

    if( msg->input->Slider[ SL_FLY_DIR ] > 0.001 ) {

        x = msg->input->Slider[ SL_FLY_DIR ] *
            yrd->bact->max_rot * time * 2;

        yrd->bact->Viewer.horiz_angle -= x;
        
        /*** Begrenzung ***/
        if( yrd->bact->Viewer.max_side < 3.15 ) {

            /*** nur dann begrenzung ***/
            if( yrd->bact->Viewer.horiz_angle >  yrd->bact->Viewer.max_side ) 
                yrd->bact->Viewer.horiz_angle =  yrd->bact->Viewer.max_side;
            if( yrd->bact->Viewer.horiz_angle < -yrd->bact->Viewer.max_side ) 
                yrd->bact->Viewer.horiz_angle = -yrd->bact->Viewer.max_side;
            }
        }
    else {

        if( msg->input->Slider[ SL_FLY_DIR ] < -0.001 ) {
            
            x = msg->input->Slider[ SL_FLY_DIR ] *
                yrd->bact->max_rot * time * 2;

            yrd->bact->Viewer.horiz_angle -= x;
        
            /*** Begrenzung ***/
            if( yrd->bact->Viewer.max_side < 3.15 ) {

                /*** nur dann begrenzung ***/
                if( yrd->bact->Viewer.horiz_angle >  yrd->bact->Viewer.max_side ) 
                    yrd->bact->Viewer.horiz_angle =  yrd->bact->Viewer.max_side;
                if( yrd->bact->Viewer.horiz_angle < -yrd->bact->Viewer.max_side ) 
                    yrd->bact->Viewer.horiz_angle = -yrd->bact->Viewer.max_side;
                }
            }
        }
}



BOOL yr_TestMoveCommand( struct trigger_logic_msg *msg )
{
    /*
    ** Liefert ein TRUE zurück, wenn im input-Teil der tlm ein
    ** Robo-Steuer-Commando gefunden wurde, also rauf, runter,
    ** schneller und bremsen, links und rechts. Nicht aber Kopf
    ** drehen!
    ** Es müßte eigentlich reichen, die ContKeys zu beachten.
    */

    if( (msg->input->Buttons & ( BT_STOP )) ||

        (fabs(msg->input->Slider[ SL_FLY_HEIGHT ]) > 0.0001) ||
        (fabs(msg->input->Slider[ SL_FLY_DIR ]) > 0.0001) ||
        (fabs(msg->input->Slider[ SL_FLY_SPEED ]) > 0.0001) )
        return( TRUE );
    else
        return( FALSE );
}


void yr_dir_rot_round_lokal_y( struct flt_m3x3 *alt_dir, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );

    rm.m11 = cos_y;     rm.m12 = 0.0;       rm.m13 = sin_y;
    rm.m21 = 0.0;       rm.m22 = 1.0;       rm.m23 = 0.0;
    rm.m31 = -sin_y;    rm.m32 = 0.0;       rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, alt_dir, &neu_dir);

    *alt_dir = neu_dir;
}


void yr_dir_rot_round_lokal_x( struct flt_m3x3 *alt_dir, FLOAT angle)
{

    struct flt_m3x3 neu_dir, rm;
    FLOAT sin_y = sin( angle );
    FLOAT cos_y = cos( angle );

    rm.m11 = 1.0;       rm.m12 = 0.0;       rm.m13 = 0.0;
    rm.m21 = 0.0;       rm.m22 = cos_y;     rm.m23 = sin_y;
    rm.m31 = 0.0;       rm.m32 = -sin_y;    rm.m33 = cos_y;
    
    nc_m_mul_m(&rm, alt_dir, &neu_dir);

    *alt_dir = neu_dir;
}


void yr_Oueiouaouea( struct yparobo_data *yrd, struct trigger_logic_msg *msg )
{
    /*** Läßt den Robo leicht schwanken ***/
    int i;

    #define ROBO_AMPLITUDE      25.0
    #define ROBO_PERIODENDAUER  3000

    yrd->bact->pos.y = yrd->merke_y + ROBO_AMPLITUDE *
                       sin( PI * msg->global_time / ROBO_PERIODENDAUER );

    /*** Kanonenkorrektur ***/
    for( i = 0; i < NUMBER_OF_GUNS; i++ ) {

        struct setposition_msg sp;

        if( yrd->gun[ i ].go ) {

            sp.x = yrd->bact->pos.x + yrd->bact->dir.m11 * yrd->gun[ i ].pos.x +
                                      yrd->bact->dir.m21 * yrd->gun[ i ].pos.y +
                                      yrd->bact->dir.m31 * yrd->gun[ i ].pos.z;
            sp.y = yrd->bact->pos.y + yrd->bact->dir.m12 * yrd->gun[ i ].pos.x +
                                      yrd->bact->dir.m22 * yrd->gun[ i ].pos.y +
                                      yrd->bact->dir.m32 * yrd->gun[ i ].pos.z;
            sp.z = yrd->bact->pos.z + yrd->bact->dir.m13 * yrd->gun[ i ].pos.x +
                                      yrd->bact->dir.m23 * yrd->gun[ i ].pos.y +
                                      yrd->bact->dir.m33 * yrd->gun[ i ].pos.z;
            sp.flags = YGFSP_SetFree;

            _methoda( yrd->gun[ i ].go, YBM_SETPOSITION, &sp );
            }
        }

}


BOOL yr_IsComNearest( struct Bacterium *Commander )
{
    /*** Testet, ob Commander der Zielnächste ist ***/
    FLOAT  com_dist;
    struct flt_triple tpos;
    struct OBNode *slave;

    if( TARTYPE_BACTERIUM == Commander->PrimTargetType ) {

        tpos = Commander->PrimaryTarget.Bact->pos;
        }
    else {

        if( TARTYPE_SECTOR == Commander->PrimTargetType ) {

            tpos = Commander->PrimPos;
            }
        else return( TRUE );
        }

    com_dist = nc_sqrt( (Commander->pos.x - tpos.x) * (Commander->pos.x - tpos.x) +
                        (Commander->pos.y - tpos.y) * (Commander->pos.y - tpos.y) +
                        (Commander->pos.z - tpos.z) * (Commander->pos.z - tpos.z));

    slave = (struct OBNode *) Commander->slave_list.mlh_Head;
    while( slave->nd.mln_Succ ) {

        if( com_dist >
        nc_sqrt( (slave->bact->pos.x - tpos.x) * (slave->bact->pos.x - tpos.x) +
                 (slave->bact->pos.y - tpos.y) * (slave->bact->pos.y - tpos.y) +
                 (slave->bact->pos.z - tpos.z) * (slave->bact->pos.z - tpos.z)))
            return( FALSE );

        slave = (struct OBNode *) slave->nd.mln_Succ;
        }

    return( TRUE );
}


void yr_SearchEnemyRobo( struct yparobo_data *yrd )
{
    /* -----------------------------------------------------------------
    ** Nach Robos (für meldungen) müssen wir Extra suchen. Die
    ** normalen Checkroutinen geben auf, nachdem sie den ersten gefunden
    ** haben. Diese aber weitersuchen zu lassen, ist allerdings
    ** in Sachen Rechenzeit unverantwortlich.
    ** Somit machen wir folgendes: Wir testen alle FeindRobos, ob sie
    ** sichtbar sind, und wenn ja, liefern wir den nächstliegenden
    ** Commander (auch Flaks!) , der das ja gemeldet haben könnte.
    ** Weil wir mehrere Robos finden können, schicken wir die meldungen
    ** hier !!!
    ** ---------------------------------------------------------------*/

    struct ypaworld_data *ywd;
    struct OBNode *robonode;

    ywd = INST_DATA( ((struct nucleusdata *)(yrd->world))->o_Class, yrd->world);
    robonode = (struct OBNode *) ywd->CmdList.mlh_Head;
    while( robonode->nd.mln_Succ ) {

        if( (ACTION_DEAD   != robonode->bact->MainState) &&
            (yrd->bact     != robonode->bact) &&
            (BCLID_YPAROBO == robonode->bact->BactClassID) ) {

            /*** Das ist ein FeindRobo. Ist er sichtbar? ***/
            if( robonode->bact->Sector->FootPrint & (1<<yrd->bact->Owner) ) {

                struct Bacterium *melder = NULL;
                FLOAT  melder_entfernung = 0.0, entfernung;
                struct OBNode *commander;

                /*** "Melder" suchen ***/
                commander = (struct OBNode *) yrd->bact->slave_list.mlh_Head;
                while( commander->nd.mln_Succ ) {

                    if( ACTION_DEAD != commander->bact->MainState ) {

                        entfernung =
                            (commander->bact->pos.x - robonode->bact->pos.x) *
                            (commander->bact->pos.x - robonode->bact->pos.x) +
                            (commander->bact->pos.z - robonode->bact->pos.z) *
                            (commander->bact->pos.z - robonode->bact->pos.z);

                        if( (melder == NULL) ||
                            (entfernung < melder_entfernung) ) {

                            melder            = commander->bact;
                            melder_entfernung = entfernung;
                            }
                        }

                    commander = (struct OBNode *) commander->nd.mln_Succ;
                    }

                /* ----------------------------------------------------
                ** Zu diesem Robo wen gefunden? Sollte eigentlich sein!
                ** und wenn es die Bordflak ist!
                ** --------------------------------------------------*/
                if( melder ) {

                    struct bact_message log;

                    log.ID     = LOGMSG_FOUNDROBO;
                    log.para1  = (LONG)robonode->bact->Owner;
                    log.para2  = 0;
                    log.para3  = 0;
                    log.pri    = 48;
                    log.sender = melder;
                    _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );
                    }
                }
            }

        robonode = (struct OBNode *) robonode->nd.mln_Succ;
        }
}


void yr_DoUserStationEnergyCheck( struct yparobo_data *yrd )
{

    FLOAT my_energy, max_energy, factor;

    my_energy  = (FLOAT) yrd->bact->Energy;
    max_energy = (FLOAT) yrd->bact->Maximum;
    factor     = my_energy / max_energy;

    if( factor < YR_MinEnergy ) {

        struct bact_message log;

        if( factor < 0.07 ) factor = 0.01;
        log.para1  = 60000 * factor / YR_MinEnergy;     // 1min bei 20% Energy
        log.ID     = LOGMSG_HOST_ENERGY_CRITICAL;
        log.para2  = 0;
        log.para3  = 0;
        log.pri    = 98;
        log.sender = yrd->bact;
        _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );
        }
}


void yr_GiveWayPointToSlaves( struct Bacterium *bact )
{
    /* ----------------------------------------------------------
    ** kopiert aktuelle Wegpunktsituation, also num_waypoints und
    ** waypoint, nicht aber count_waypoints. Wenn es aber der
    ** erste Wegpunkt war, num_waypoints also 1 ist, dann setze
    ** ich den count schon auf 0.
    **
    ** ACHTUNG, AUCH IN TANKCLASS AKTUALISIEREN
    ** --------------------------------------------------------*/
    struct OBNode *slave;

    for( slave = (struct OBNode *) bact->slave_list.mlh_Head;
         slave->nd.mln_Succ;
         slave = (struct OBNode *)  slave->nd.mln_Succ ) {

        int i;

        slave->bact->num_waypoints = bact->num_waypoints;


        if( slave->bact->num_waypoints == 1 )       // Anfang?
            slave->bact->count_waypoints = 0;

        slave->bact->ExtraState   |= EXTRA_DOINGWAYPOINT;

        if( bact->ExtraState & EXTRA_WAYPOINTCYCLE )
            slave->bact->ExtraState |=  EXTRA_WAYPOINTCYCLE;
        else
            slave->bact->ExtraState &= ~EXTRA_WAYPOINTCYCLE;

        for( i = 0; i < MAXNUM_WAYPOINTS; i++ )
            slave->bact->waypoint[ i ] = bact->waypoint[ i ];
        }
}


void yr_ClearSecondaryTargets( struct Bacterium *com )
{
    struct settarget_msg target;
    struct OBNode *slave;

    target.target_type = TARTYPE_NONE;
    target.priority    = 1;
    _methoda( com->BactObject, YBM_SETTARGET, &target );
    slave = (struct OBNode *) com->slave_list.mlh_Head;
    while( slave->nd.mln_Succ ) {

        _methoda( slave->o, YBM_SETTARGET, &target );
        slave = (struct OBNode *) slave->nd.mln_Succ;
        }
}


void yr_HandleSurfaceStuff( struct yparobo_data *yrd,
                            struct trigger_logic_msg *msg )
{

    struct settarget_msg target;
    struct createvehicle_msg cv;
    struct createbuilding_msg cb;
    struct setstate_msg state;
    LONG   energy;
    struct VehicleProto *array;
    struct BuildProto *b_array;
    struct Bacterium *bact;
    Object *com, *slave; 
    ULONG  BC;
    struct bact_message log;
                                                                                                
    #ifdef __NETWORK__
    struct ypamessage_newvehicle nvm;
    struct sendmessage_msg sm;
    struct ypaworld_data *ywd;
    #endif
  
  
    switch( msg->user_action ) {

        case YW_ACTION_NONE:

            /*** Festhalten!!! Jetzt passiert: NICHTS! ***/
            break;

        case YW_ACTION_GOTO:

            /* ----------------------------------------------------
            ** Jemand wird wohin geschickt. Dieser Jemand steht
            ** in selbact und muß ein Commander sein. Das Ziel wird
            ** als Sektor als 3D-Punkt verlangt, als Bakterium
            ** nehmen wir den Pointer.
            ** --------------------------------------------------*/

            msg->selbact->ExtraState &= ~(EXTRA_DOINGWAYPOINT|
                                          EXTRA_WAYPOINTCYCLE);

            /* -----------------------------------------------------------
            ** Setway arbeitet rekursiv, braucht also die Greschwader-
            ** struktur. Folglich muss NEARTOCOM vor dem Setzen des Zieles
            ** erfolgen. Also setzen wir das Ziel einfach, so dass
            ** NEARTOCOM daten hat und dann machen wir es nochmal richtig.
            ** ---------------------------------------------------------*/
            if( msg->tarsec ) {
                target.pos.x       = msg->tarpoint.x;
                target.pos.z       = msg->tarpoint.z;
                target.target_type = TARTYPE_SECTOR;
                }
            else {
                target.target.bact = msg->tarbact;
                target.target_type = TARTYPE_BACTERIUM;
                }
            target.priority    = 0;
            _methoda( msg->selbact->BactObject, YBM_SETTARGET, &target);

            /*** Geschwader evtl. umschichten ***/
            if( !yr_IsComNearest( msg->selbact ) ) {

                /*** Itze grundsätzlich ***/
                struct organize_msg org;

                org.specialbact = NULL;
                org.mode        = ORG_NEARTOCOM;
                _methoda( msg->selbact->BactObject, YBM_ORGANIZE, &org );
                }

            /*** neuer Commander. selbact umbiegen ***/
            if( msg->selbact->robo != msg->selbact->master )
                msg->selbact = msg->selbact->master_bact;

            /*** Nun richtige Zielpunktvergabe ***/
            if( msg->tarsec ) {

                yr_SetSectorTarget( msg->selbact, msg->tarpoint.x, msg->tarpoint.z );

                }
            else {

                yr_SetBactTarget( msg->selbact, msg->tarbact );
                }

            /*** selbact oder sein Master ist Sender für M. ***/
            if( msg->selbact->master == msg->selbact->robo )
                log.sender = msg->selbact;
            else
                log.sender = msg->selbact->master_bact;
            log.para1 = log.para2 = log.para3 = 0;
            if( TARTYPE_SECTOR == log.sender->PrimTargetType ) {
                if( log.sender->PrimaryTarget.Sector->Owner ==
                    yrd->bact->Owner )
                    log.ID = LOGMSG_ORDERMOVE;
                else
                    log.ID = LOGMSG_ORDERATTACK;
                }
            else {
                if( log.sender->PrimaryTarget.Bact->Owner ==
                    yrd->bact->Owner )
                    log.ID = LOGMSG_ORDERMOVE;
                else
                    log.ID = LOGMSG_ORDERATTACK;
                }
            log.pri = 38;
            _methoda( yrd->bact->BactObject, YRM_LOGMSG, &log );

            /*** Slaves Nebenziele loeschen ***/
            yr_ClearSecondaryTargets( msg->selbact );

            /*** ESCAPE ausschalten ***/
            msg->selbact->ExtraState &= ~EXTRA_ESCAPE;
            break;

        case YW_ACTION_WAYPOINT_START:

            /* ----------------------------------------------------------
            ** Es beginnt eine neu Wegpunktkette. Dieser Punkt wird
            ** auf position 0 gesetzt und anschließend als HauptZiel
            ** übergeben. Wenn das Fahrzeug diesen Punkt vor dem nächsten
            ** erreicht, wird der Wegpunktmodus eben wieder ausgeschaltet
            ** --------------------------------------------------------*/

            /*** Flags ***/
            msg->selbact->ExtraState |= EXTRA_DOINGWAYPOINT;

            /*** Punkte kopieren ***/
            msg->selbact->waypoint[ 0 ].x = msg->tarpoint.x;
            msg->selbact->waypoint[ 0 ].z = msg->tarpoint.z;

            /*** Anzahl ***/
            msg->selbact->num_waypoints   = 1;
            msg->selbact->count_waypoints = 0;

            /*** ersten Punkt als Ziel setzen ***/
            target.pos.x       = msg->selbact->waypoint[ 0 ].x;
            target.pos.z       = msg->selbact->waypoint[ 0 ].z;
            target.target_type = TARTYPE_SECTOR;
            target.priority    = 0;
            _methoda( msg->selbact->BactObject,
                      YBM_SETTARGET, &target);

            /*** Vor dem Umschichten des GEschwaders! ***/
            yr_GiveWayPointToSlaves( msg->selbact );

            /*** Geschwader evtl. umschichten ***/
            if( !yr_IsComNearest( msg->selbact ) ) {

                /*** Itze grundsätzlich ***/
                struct organize_msg org;

                org.specialbact = NULL;
                org.mode        = ORG_NEARTOCOM;
                _methoda( msg->selbact->BactObject, YBM_ORGANIZE, &org );
                }

            /*** Slaves Nebenziele loeschen ***/
            yr_ClearSecondaryTargets( msg->selbact );

            /*** und raus ***/
            break;

        case YW_ACTION_WAYPOINT_ADD:

            /* ----------------------------------------------------------
            ** Ein zusäzlicher Wegpunkt. Wir schalten den Waypoint-Modus
            ** wieder ein, weil er durch das vorzeitige Erreichen des
            ** vorherigen Punktes schon wieder ausgeschalten sein kann.
            ** --------------------------------------------------------*/

            /*** Zuviele? ***/
            if( msg->selbact->num_waypoints >= MAXNUM_WAYPOINTS )
                break;

            /*** Punkte kopieren ***/
            msg->selbact->waypoint[ msg->selbact->num_waypoints ].x = msg->tarpoint.x;
            msg->selbact->waypoint[ msg->selbact->num_waypoints ].z = msg->tarpoint.z;

            /*** Schon vorherigen Punkt erreicht? ***/
            if( !( msg->selbact->ExtraState & EXTRA_DOINGWAYPOINT ) ) {

                /* ----------------------------------------
                ** Dann Modus neu einschalten und Hauptziel
                ** nochmal von Hand setzen
                ** --------------------------------------*/
                msg->selbact->ExtraState |= EXTRA_DOINGWAYPOINT;

                /*** ersten Punkt als Ziel setzen ***/
                target.pos.x       = msg->selbact->waypoint[ msg->selbact->num_waypoints ].x;
                target.pos.z       = msg->selbact->waypoint[ msg->selbact->num_waypoints ].z;
                target.target_type = TARTYPE_SECTOR;
                target.priority    = 0;
                _methoda( msg->selbact->BactObject,
                          YBM_SETTARGET, &target);
                }

            /*** Anzahl ***/
            msg->selbact->num_waypoints++;

            yr_GiveWayPointToSlaves( msg->selbact );

            /*** und raus ***/
            break;

        case YW_ACTION_WAYPOINT_CYCLE:

            /*** Nur den Cycle-Modus schalten ***/
            msg->selbact->ExtraState |= EXTRA_WAYPOINTCYCLE;
            yr_GiveWayPointToSlaves( msg->selbact );
            break;

        case YW_ACTION_PANIC:

            /*** Not longer supported ***/
            break;

        case YW_ACTION_NEW:

            /* --------------------------------------------------------
            ** Wir sollen einen neuen Commander erzeugen. Na okay.
            ** Wir nehmen kein Dock, sondern setzen die Leute in die
            ** Landschaft.
            ** Die kriegen dann das Ziel, wo sie warten
            ** sollen. Darum brauchen wir uns nicht zu kümmern, wenn
            ** nur ihre Create_time richtig gesetzt ist, höhren sie von
            ** allein auf. Wir sollten hier die CT nur wenig unter der 
            ** dock_time ansiedeln.
            ** ------------------------------------------------------*/

            _get( yrd->world, YWA_VehicleArray, &array );
            energy = array[ msg->proto_id ].Energy * CREATE_ENERGY_FACTOR;

            /*** Energieaufteilung ist uninteressant! ***/
            if( energy <= yrd->BattVehicle ) {

                /*** Energie reicht ***/
                cv.x  = msg->tarpoint.x;
                cv.y  = msg->tarpoint.y;
                cv.z  = msg->tarpoint.z;
                cv.vp = msg->proto_id;

                com = (Object *) _methoda( yrd->world, YWM_CREATEVEHICLE, &cv);
                if( com ) {

                    /*
                    ** in ACTION_CREATE setzen, ankoppeln und
                    ** initialisieren.
                    */
                    target.pos.x       = yrd->bact->pos.x +
                                         4 * (cv.x - yrd->bact->pos.x);
                    target.pos.y       = cv.y;
                    target.pos.z       = yrd->bact->pos.z +
                                         4 * (cv.z - yrd->bact->pos.z);
                    target.priority    = 0;
                    target.target_type = TARTYPE_SECTOR;
                    _methoda( com, YBM_SETTARGET, &target );

                    state.main_state = ACTION_CREATE;
                    state.extra_off  = state.extra_on = 0;
                    _methoda( com, YBM_SETSTATE, &state );

                    _get( com, YBA_Bacterium, &bact );
                    bact->scale_time = (LONG)( bact->Maximum * YR_CreationTimeU);
                    bact->CommandID  = YPA_CommandCount++;
                    bact->Owner      = yrd->bact->Owner;

                    _methoda( yrd->bact->BactObject, YBM_ADDSLAVE, com);

                    #ifdef __NETWORK__
                    /*** Id aktualisieren und Message losschicken ***/
                    ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
                    if( ywd->playing_network ) {

                        bact->ident           |= (((ULONG)bact->Owner) << 24);
                        bact->CommandID       |= (((ULONG)bact->Owner) << 24);

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
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                        }
                    #endif
                    
                    /*
                    ** Damit der Commander wartet, setze ich seine
                    ** Aggressivität auf 0.
                    */
                    _set( com, YBA_Aggression, 60);
                    
                    _get( yrd->bact->BactObject, YBA_BactCollision, &BC );
                    _set( com, YBA_BactCollision, BC );
                    bact->robo = yrd->bact->BactObject;

                    /*** Energieabzug ***/
                    yrd->BattVehicle -= CREATE_ENERGY_FACTOR*bact->Maximum;

                    /*** Meldung machen ***/
                    yr_HistoryCreate( yrd->world, bact );
                    }
                }
            break;
            
            
        case YW_ACTION_ADD:

            /*
            ** Wir sollen einen neuen Untergebenen erzeugen.
            **
            ** Die Untergebenen bekommen kein Ziel, da sie an den
            ** Commander angekoppelt werden.
            */

            _get( yrd->world, YWA_VehicleArray, &array );
            energy = array[ msg->proto_id ].Energy * CREATE_ENERGY_FACTOR;

            /*** Energieaufteilung ist uninteressant! ***/
            if( energy <= yrd->BattVehicle ) {

                /*** Energie reicht ***/
                cv.x  = msg->tarpoint.x;
                cv.y  = msg->tarpoint.y;
                cv.z  = msg->tarpoint.z;
                cv.vp = msg->proto_id;

                slave = (Object *) _methoda( yrd->world, YWM_CREATEVEHICLE, &cv);
                if( slave ) {

                    /*
                    ** in ACTION_CREATE setzen, ankoppeln und
                    ** initialisieren.
                    */

                    state.main_state = ACTION_CREATE;
                    state.extra_off  = state.extra_on = 0;
                    _methoda( slave, YBM_SETSTATE, &state );

                    _get( slave, YBA_Bacterium, &bact );
                    bact->scale_time = (LONG)( bact->Maximum * YR_CreationTimeU);
                    bact->Owner      = yrd->bact->Owner;
                    bact->CommandID  = msg->selbact->CommandID; // !!

                    _methoda( msg->selbact->BactObject, YBM_ADDSLAVE, slave);
                    bact->Aggression = msg->selbact->Aggression;

                    #ifdef __NETWORK__
                    /*** Id aktualisieren und Message losschicken ***/
                    ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
                    if( ywd->playing_network ) {

                        bact->ident     |= (((ULONG)bact->Owner) << 24);
                        nvm.generic.message_id = YPAM_NEWVEHICLE;
                        nvm.generic.owner      = bact->Owner;
                        nvm.pos                = bact->pos;
                        nvm.type               = bact->TypeID;
                        nvm.master             = bact->master_bact->ident;
                        nvm.ident              = bact->ident;
                        nvm.vkind              = NV_SLAVE;
                        nvm.command_id         = msg->selbact->CommandID;

                        sm.receiver_id         = NULL;
                        sm.receiver_kind       = MSG_ALL;
                        sm.data                = &nvm;
                        sm.data_size           = sizeof( nvm );
                        sm.guaranteed          = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                        }
                    #endif

                    _get( yrd->bact->BactObject, YBA_BactCollision, &BC );
                    _set( slave, YBA_BactCollision, BC );
                    bact->robo =  yrd->bact->BactObject;

                    /*** Energieabzug ***/
                    yrd->BattVehicle -= CREATE_ENERGY_FACTOR*bact->Maximum;

                    /*** Meldung machen ***/
                    yr_HistoryCreate( yrd->world, bact );

                    /* -------------------------------------------
                    ** Vom Commander alle Zielsachen übernehmen.
                    ** Wir übernehmen einfach den Zustand des Com.
                    ** Waypoints können dann überschreiben.
                    ** -----------------------------------------*/
                    target.target.bact = msg->selbact->PrimaryTarget.Bact;
                    target.target_type = msg->selbact->PrimTargetType;
                    target.pos         = msg->selbact->PrimPos;
                    target.priority    = 0;
                    _methoda( slave, YBM_SETTARGET, &target );

                    bact->num_waypoints      = msg->selbact->num_waypoints;
                    bact->count_waypoints    = 0; // damit er am Anfang anfängt!!
                    memcpy( bact->waypoint, msg->selbact->waypoint,
                            sizeof( bact->waypoint ) );
                    if( msg->selbact->ExtraState & EXTRA_DOINGWAYPOINT ) {

                        /*** Dann ersten Wegpunkt als Ziel setzen ***/
                        bact->ExtraState |= EXTRA_DOINGWAYPOINT;
                        if( msg->selbact->ExtraState & EXTRA_WAYPOINTCYCLE )
                            bact->ExtraState |= EXTRA_WAYPOINTCYCLE;
                        target.pos.x       = bact->waypoint[ 0 ].x;
                        target.pos.z       = bact->waypoint[ 0 ].z;
                        target.target_type = TARTYPE_SECTOR;
                        target.priority    = 0;
                        _methoda( bact->BactObject, YBM_SETTARGET, &target );
                        }
                    }
                }
            break;
            

        case YW_ACTION_FIRE:

            /*** Das is gegessen ***/
            break;

        case YW_ACTION_APILOT:

            /* -------------------------------------------------
            ** Neu: Der Robo wird plaziert, also gebeamt. Dies
            ** war möglich, enn ich die Message bekomme. Ich                    ** niemand die Handsteuerung übernommen hat. Vorerst
            ** muß also die Energie abziehen und ein SETPOSITION
            ** machen.
            ** NEU: Nur, wenn ich nicht schon beame...
            ** -----------------------------------------------*/
            if( !(yrd->RoboState & ROBO_MOVE) ) {

                ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
                
                /*** BeamIn ***/
                _StartSoundSource( &(yrd->bact->sc), VP_NOISE_BEAMIN );

                /*** Merk-Flag setzen ***/
                yrd->RoboState |= ROBO_MOVE;

                /*** Energieabzug ***/
                yrd->BattBeam -= msg->energy;

                /*** Position merken ***/
                yrd->BeamPos.x  = msg->tarpoint.x;
                yrd->BeamPos.y  = msg->tarpoint.y;
                yrd->BeamPos.z  = msg->tarpoint.z;
                yrd->BeamInTime = BEAM_IN_TIME;

                /* ----------------------------------------------
                ** beginn eines Effekts anmelden.
                ** --------------------------------------------*/
                #ifdef __NETWORK__
                if( ywd->playing_network ) {

                    struct ypamessage_startbeam stb;
                    struct sendmessage_msg sm;

                    stb.generic.message_id = YPAM_STARTBEAM;
                    stb.generic.owner      = yrd->bact->Owner;
                    stb.ident              = yrd->bact->ident;
                    stb.BeamPos            = yrd->BeamPos;

                    sm.receiver_id         = NULL;
                    sm.receiver_kind       = MSG_ALL;
                    sm.data                = &stb;
                    sm.data_size           = sizeof( stb );
                    sm.guaranteed          = TRUE;
                    _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                    }
                #endif
 
                }

            break;

        case YW_ACTION_BUILD:

            /* ---------------------------------------------------
            ** Baut auf übergebenen Sektor ein Gebäude. Wir müssen
            ** nur noch testen, ob die Energie reicht.
            ** -------------------------------------------------*/
            
            _get( yrd->world, YWA_BuildArray, &b_array );
            energy = b_array[ msg->proto_id ].CEnergy;

            if( energy <= yrd->BattVehicle ) {

                cb.job_id    = yrd->bact->Owner;
                cb.owner     = yrd->bact->Owner;
                cb.bp        = msg->proto_id;
                cb.immediate = FALSE;
                cb.sec_x     = msg->tarsec_x;
                cb.sec_y     = msg->tarsec_y;
                cb.flags     = 0;
                if( _methoda(yrd->world, YWM_CREATEBUILDING, &cb) ) {

                    /*** Energie abziehen ***/
                    yrd->BattVehicle -= energy;

                    #ifdef __NETWORK__

                    /*** Bauen auch auf anderen Maschinen initial. ***/
                    ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
                    if( ywd->playing_network ) {

                        struct ypamessage_startbuild sb;

                        sb.generic.message_id = YPAM_STARTBUILD;
                        sb.generic.owner      = yrd->bact->Owner;
                        sb.bproto             = msg->proto_id;
                        sb.sec_x              = msg->tarsec_x;
                        sb.sec_y              = msg->tarsec_y;

                        sm.receiver_id        = NULL;
                        sm.receiver_kind      = MSG_ALL;
                        sm.data               = &sb;
                        sm.data_size          = sizeof( sb );
                        sm.guaranteed         = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                        }
                    #endif
                    }
                }
            break;
        }
}


yr_DoBeamStuff( struct yparobo_data *yrd, LONG frame_time )
{
    struct ypaworld_data *ywd;

    ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
    yrd->BeamInTime -= frame_time;

    /*** Ist der BeamIn-Effekt zu Ende? ***/
    if( yrd->BeamInTime <= 0 ) {

        struct flt_triple before_beam;
        struct setposition_msg setpos;

        yrd->BeamInTime = 0;

        /*** alte Position merken ***/
        before_beam   = yrd->bact->pos;
        before_beam.y = yrd->merke_y;

        /*** Bewegen ***/
        setpos.x = yrd->BeamPos.x;
        setpos.y = yrd->BeamPos.y;
        setpos.z = yrd->BeamPos.z;
        setpos.flags = 0;
        _methoda( yrd->bact->BactObject, YBM_SETPOSITION, &setpos );

        /* -------------------------------------------------
        ** bei mir und allen Bordflaks fuer den Fall
        ** einer Netzsession das SETDIRECTLY-Flak setzen, so
        ** dass nicht interpoliert wird (Gummibandeffekt)
        ** -----------------------------------------------*/
        #ifdef __NETWORK__
        if( ywd->playing_network ) {

            struct OBNode *com = (struct OBNode *) yrd->bact->slave_list.mlh_Head;
            yrd->bact->ExtraState |= EXTRA_SETDIRECTLY;
            while( com->nd.mln_Succ ) {
                if( BCLID_YPAGUN == com->bact->BactClassID ) {
                    ULONG rgun;
                    _get( com->o, YGA_RoboGun, &rgun );
                    if( rgun ) com->bact->ExtraState |= EXTRA_SETDIRECTLY;
                    }
                com = (struct OBNode *)com->nd.mln_Succ;
                }
            }
        #endif

        /*** Der vorherige Stand in old_pos ***/
        yrd->bact->old_pos   = before_beam;
        yrd->bact->old_pos.y = before_beam.y;

        /*** BeamOut ***/
        _StartSoundSource( &(yrd->bact->sc), VP_NOISE_BEAMOUT );

        /*** Move-Zustand abmelden ***/
        yrd->RoboState &= ~ROBO_MOVE;

        /*** VP aus! ***/
        yrd->bact->extravp[ 0 ].flags = 0;
        yrd->bact->extravp[ 1 ].flags = 0;
        #ifdef __NETWORK__
        if( ywd->playing_network ) {

            struct ypamessage_endbeam eb;
            struct sendmessage_msg sm;

            eb.generic.message_id = YPAM_ENDBEAM;
            eb.generic.owner      = yrd->bact->Owner;
            eb.ident              = yrd->bact->ident;
            
            sm.receiver_id        = NULL;
            sm.receiver_kind      = MSG_ALL;
            sm.data               = &eb;
            sm.data_size          = sizeof( eb );
            sm.guaranteed         = TRUE;
            _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
            }
        #endif
        }
    else {

        /*** Wir sind noch davor. ***/

        /* --------------------------------------------
        ** Wir lassen um uns einen CreateVP flackern.
        ** Dazu schalten wir ihn abwechselnd an oder 
        ** aus. Je kleiner die Zeit wird, desto laenger
        ** bleibt er an und desto kuerzer werden die
        ** "Ohne"-intervalle.
        ** Ebenso setzen wir was an den Zielpunkt.
        ** ------------------------------------------*/
        if( yrd->beam_fx_time <= 0 ) {

            /*** Es ist an der Zeit, was zu tun ***/
            LONG time_da   = (BEAM_IN_TIME - yrd->BeamInTime) / 10;
            LONG time_wech = yrd->BeamInTime / 10;

            /*** Am alten Ort ***/
            if( yrd->bact->extravp[0].flags & EVF_Active ) {

                /*** wieder ausschalten ***/
                yrd->beam_fx_time = time_wech;
                yrd->bact->extravp[ 0 ].flags &= ~EVF_Active;
                }
            else {

                /*** Anschalten ***/
                yrd->beam_fx_time = time_da;

                yrd->bact->extravp[ 0 ].pos     = yrd->bact->pos;
                yrd->bact->extravp[ 0 ].dir     = yrd->bact->dir;
                yrd->bact->extravp[ 0 ].flags   = (EVF_Active | EVF_Scale);
                yrd->bact->extravp[ 0 ].scale   = 1.25;
                yrd->bact->extravp[0].vis_proto = yrd->bact->vis_proto_create;
                yrd->bact->extravp[0].vp_tform  = yrd->bact->vp_tform_create;
                }

            /*** Am neuen Ort ***/
            if( yrd->bact->extravp[1].flags & EVF_Active ) {

                /*** wieder ausschalten ***/
                yrd->beam_fx_time = time_wech;
                yrd->bact->extravp[ 1 ].flags &= ~EVF_Active;
                }
            else {

                /*** Anschalten ***/
                yrd->beam_fx_time = time_da;

                yrd->bact->extravp[ 1 ].pos     = yrd->BeamPos;
                yrd->bact->extravp[ 1 ].dir     = yrd->bact->dir;
                yrd->bact->extravp[ 1 ].flags   = EVF_Active;
                yrd->bact->extravp[1].vis_proto = yrd->bact->vis_proto_create;
                yrd->bact->extravp[1].vp_tform  = yrd->bact->vp_tform_create;
                }
           }

        yrd->beam_fx_time -= frame_time;
        }
}


BOOL yr_AreThereGroundVehicles( struct Bacterium *commander )
{
    /*** testet, ob in diesem Geschwader Bodenfahrzeuge sind ***/
    struct OBNode *slave;

    if( (BCLID_YPATANK == commander->BactClassID) ||
        (BCLID_YPACAR  == commander->BactClassID) )
        return( TRUE );

    slave = (struct OBNode *) commander->slave_list.mlh_Head;
    while( slave->nd.mln_Succ ) {

        if( (BCLID_YPATANK == slave->bact->BactClassID) ||
            (BCLID_YPACAR  == slave->bact->BactClassID) )
            return( TRUE );

        slave = (struct OBNode *) slave->nd.mln_Succ;
        }

    return( FALSE );
}


void yr_SetBactTarget( struct Bacterium *commander, struct Bacterium *targetbact )
{
    /* -------------------------------------------------------
    ** ein Bakterienziel. Hmm. Wenn wir Wegpunkte benoetigen,
    ** dann speichern wir das Ziel extra ab und machen erstmal
    ** Waypoints draus. Kurz vorm ende schalten wir wieder
    ** das originale Ziel ein.
    ** -----------------------------------------------------*/
    struct settarget_msg target;

    if( yr_AreThereGroundVehicles( commander ) ) {

        struct findpath_msg fp;

        fp.num_waypoints = MAXNUM_WAYPOINTS;
        fp.from_x        = commander->pos.x;
        fp.from_z        = commander->pos.z;
        fp.to_x          = targetbact->pos.x;
        fp.to_z          = targetbact->pos.z;
        fp.flags         = WPF_Normal;

        /*** Mit FINDPATH nur testen ***/
        _methoda( commander->BactObject, YBM_FINDPATH, &fp );

        /*** Wenn wir WAYPOINTS brauchen, dann SektorZiele ***/
        if( fp.num_waypoints > 1 ) {

            /*** Ok, nochmal, aber richtig ***/
            fp.num_waypoints = MAXNUM_WAYPOINTS;
            _methoda( commander->BactObject, YBM_SETWAY, &fp );

            /* ----------------------------------------------------
            ** und buffern (commander reicht, der gibt eh an Slaves
            ** weiter
            ** --------------------------------------------------*/
            commander->mt_commandid = targetbact->CommandID;
            commander->mt_owner     = targetbact->Owner;
            }
        else {

            /*** klassisch ***/
            target.target.bact = targetbact;
            target.priority    = 0;
            target.target_type = TARTYPE_BACTERIUM;
            _methoda( commander->BactObject, YBM_SETTARGET, &target);
            }
        }
    else {

        /*** normales Bact-Ziel ***/
        target.target.bact = targetbact;
        target.priority    = 0;
        target.target_type = TARTYPE_BACTERIUM;
        _methoda( commander->BactObject, YBM_SETTARGET, &target);
        }
}


void yr_SetSectorTarget( struct Bacterium *commander, FLOAT tarpoint_x, FLOAT tarpoint_z )
{
    struct settarget_msg target;

    /*** Bodenfahrzeuge Sonderbehandlung ***/
    if( yr_AreThereGroundVehicles( commander ) ) {

        struct findpath_msg fp;

        fp.num_waypoints = MAXNUM_WAYPOINTS;
        fp.from_x        = commander->pos.x;
        fp.from_z        = commander->pos.z;
        fp.to_x          = tarpoint_x;
        fp.to_z          = tarpoint_z;
        fp.flags         = WPF_Normal;
        _methoda( commander->BactObject, YBM_SETWAY, &fp );
        }
    else {

        /*** ein Sektorziel ***/
        target.pos.x       = tarpoint_x;
        target.pos.z       = tarpoint_z;
        target.target_type = TARTYPE_SECTOR;
        target.priority    = 0;
        _methoda( commander->BactObject, YBM_SETTARGET, &target);
        }
}
