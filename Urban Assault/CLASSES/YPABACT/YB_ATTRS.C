/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_attrs.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:22:59 $
**  $Locker: floh $
**  $Author: floh $
**
**  Attribut-Handling für ypabact.class.
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypamissileclass.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine
void yb_TakeCommandersTarget( struct Bacterium *slave, struct Bacterium *chief, Object *world );


/*-----------------------------------------------------------------*/
BOOL yb_initAttrs(Object *o, struct ypabact_data *ybd, struct TagItem *attrs)
/*
**  FUNCTION
**      (I)-Attribut-Handler.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      09-Jun-95   floh    created
**      13-Jun-95   floh    YBA_AirConst
*/
{
    register ULONG tag;
    struct OBNode  *slave;
    struct ypaworld_data *ywd;

    #ifdef __NETWORK__
    struct ypamessage_viewer vm;
    struct sendmessage_msg sm;
    #endif

    /* Default-Attribute eintragen */
    ybd->bact.mass            = (FLOAT) YBA_Mass_DEF;
    ybd->bact.max_force       = (FLOAT) YBA_MaxForce_DEF;
    ybd->bact.air_const       = (FLOAT) YBA_AirConst_DEF;
    ybd->bact.max_rot         = (FLOAT) YBA_MaxRot_DEF;
    ybd->bact.pref_height     = (FLOAT) YBA_PrefHeight_DEF;
    ybd->bact.radius          = (FLOAT) YBA_Radius_DEF;
    ybd->bact.viewer_radius   = (FLOAT) YBA_Viewer_Radius_DEF;
    ybd->bact.over_eof        = (FLOAT) YBA_OverEOF_DEF;
    ybd->bact.viewer_over_eof = (FLOAT) YBA_Viewer_OverEOF_DEF;
    ybd->bact.Energy          = (ULONG) YBA_Energy_DEF;
    ybd->bact.Shield          = (UBYTE) YBA_Shield_DEF;
    ybd->bact.HC_Speed        = (FLOAT) YBA_HCSpeed_DEF;
    ybd->YourLastSeconds      = (LONG)  YBA_YourLastSeconds_DEF;
    ybd->bact.Aggression      = (UBYTE) YBA_Aggression_DEF;
    ybd->bact.Maximum         = (ULONG) YBA_Maximum_DEF;
    ybd->bact.Transfer        = (WORD)  YBA_Transfer_DEF;
    ybd->bact.max_user_height = (FLOAT) YBA_MaxUserHeight_DEF;
    ybd->bact.gun_radius      = (FLOAT) YBA_GunRadius_DEF;
    ybd->bact.gun_power       = (FLOAT) YBA_GunPower_DEF;

    ybd->bact.adist_sector    = (FLOAT) YBA_ADist_Sector_DEF;
    ybd->bact.adist_bact      = (FLOAT) YBA_ADist_Bact_DEF;
    ybd->bact.sdist_sector    = (FLOAT) YBA_SDist_Sector_DEF;
    ybd->bact.sdist_bact      = (FLOAT) YBA_SDist_Bact_DEF;

    /*** normalerweise exakt testen ***/
    ybd->flags = YBF_CheckVisExactly;

    /*** YBA_World muß vorher geholt werden... ***/
    ybd->world = (Object *) _GetTagData(YBA_World, NULL, attrs);
    if (NULL == ybd->world) return(FALSE);
    ybd->ywd   = INST_DATA( ((struct nucleusdata *)(ybd->world))->o_Class,
                             (ybd->world));

    /*** Attribut-Liste scannen... ***/
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /*** erstmal die Sonderfälle... ***/
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /*** die Attribute ***/
            switch(tag) {

                case YBA_Viewer:

                    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);

                    if (data) {

                        /*** Welt-Object benachrichtigen ***/
                        _methoda(ybd->world, YWM_SETVIEWER, &(ybd->bact));

                        /*** und das interne Flag setzen... ***/
                        ybd->flags |= YBF_Viewer;

                        /*** Übers Netz ***/
                        #ifdef __NETWORK__
                        if( ywd->playing_network )
                            vm.viewer = 1;
                        #endif

                        /*** CockPitGeräusch ein ***/
                        _StartSoundSource( &(ybd->bact.sc), VP_NOISE_COCKPIT );

                    } else {

                        /*** Flag rücksetzen ***/
                        ybd->flags &= ~YBF_Viewer;

                        /*** Übers Netz ***/
                        #ifdef __NETWORK__
                        if( ywd->playing_network )
                            vm.viewer = 0;
                        #endif

                        /*** CockPitGeräusch aus ***/
                        _EndSoundSource( &(ybd->bact.sc), VP_NOISE_COCKPIT );
                    };

                    #ifdef __NETWORK__
                    /*** Id aktualisieren und Message losschicken ***/
                    if( ywd->playing_network ) {

                        vm.generic.message_id = YPAM_VIEWER;
                        vm.generic.owner      = ybd->bact.Owner;
                        vm.class              = ybd->bact.BactClassID;
                        vm.ident              = ybd->bact.ident;
                        if( BCLID_YPAMISSY == vm.class ) {

                            struct Bacterium *rman;
                            _get( o, YMA_RifleMan, &rman );
                            vm.rifleman = rman->ident;
                            }

                        sm.receiver_id         = NULL;
                        sm.receiver_kind       = MSG_ALL;
                        sm.data                = &vm;
                        sm.data_size           = sizeof( vm );
                        sm.guaranteed          = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                        }
                    #endif

                    break;

                case YBA_UserInput:
                    if (data) {
                        ybd->flags |= YBF_UserInput;
                        _set(ybd->world, YWA_UserVehicle, ybd->bact.BactObject);
                        }
                    else      ybd->flags &= ~YBF_UserInput;
                    break;
                
                case YBA_BactCollision:
                    if (data) ybd->flags |= YBF_BactCollision;
                    else      ybd->flags &= ~YBF_BactCollision;
                    break;
                
                case YBA_CheckVisExactly:
                    if (data) ybd->flags |= YBF_CheckVisExactly;
                    else      ybd->flags &= ~YBF_CheckVisExactly;
                    break;
                
                case YBA_LandWhileWait:
                    if (data) ybd->flags |= YBF_LandWhileWait;
                    else      ybd->flags &= ~YBF_LandWhileWait;
                    break;
                
                case YBA_AirConst:
                    ybd->bact.air_const    = (FLOAT) data;
                    ybd->bact.bu_air_const = (FLOAT) data;
                    break;

                case YBA_YourLS:
                    ybd->YourLastSeconds = (LONG) data;
                    break;

                case YBA_Aggression:
                    ybd->bact.Aggression = (UBYTE) data;
                    slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                    while( slave->nd.mln_Succ ) {

                        slave->bact->Aggression = (UBYTE) data;
                        slave = (struct OBNode *) slave->nd.mln_Succ;
                        }
                    break;

                case YBA_VPTForm:

                    ybd->vp_tform = (struct tform *) data;
                    break;

                case YBA_VisProto:

                    ybd->vis_proto = (ULONG *) data;
                    break;

                case YBA_ExtraViewer:

                    if( data )
                        ybd->flags |=  YBF_ExtraViewer;
                    else
                        ybd->flags &= ~YBF_ExtraViewer;
                    break;

                case YBA_AlwaysRender:

                    if( data )
                        ybd->flags |=  YBF_AlwaysRender;
                    else
                        ybd->flags &= ~YBF_AlwaysRender;
                    break;
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yb_setAttrs(Object *o, struct ypabact_data *ybd, struct TagItem *attrs)
/*
**  FUNCTION
**      (S)-Attribut-Handler.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      09-Jun-95   floh    created
*/
{
    register ULONG tag;
    struct OBNode  *slave;
    struct ypaworld_data *ywd;

    #ifdef __NETWORK__
    struct ypamessage_viewer vm;
    struct sendmessage_msg sm;
    #endif

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* dann die eigentlichen Attribute, schön nacheinander */
            switch (tag) {

                case YBA_Viewer:

                    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);

                    if (data) {

                        /*** Welt-Object benachrichtigen ***/
                        _methoda(ybd->world, YWM_SETVIEWER, &(ybd->bact));

                        /*** und das interne Flag setzen... ***/
                        ybd->flags |= YBF_Viewer;

                        /*** Übers Netz ***/
                        #ifdef __NETWORK__
                        if( ywd->playing_network )
                            vm.viewer = 1;
                        #endif

                        /*** Zaehler fuer Salve zuruecksetzen ***/
                        ybd->bact.salve_count = 0;

                        /*** Wenn fliegender Hubschrauber, dan Kraft auf max ***/
                        if( (BCLID_YPABACT == ybd->bact.BactClassID) &&
                            (!(EXTRA_LANDED & ybd->bact.ExtraState)) &&
                            (ACTION_NORMAL == ybd->bact.MainState) )
                            ybd->bact.act_force = ybd->bact.max_force;

                        /*** CockPitGeräusch ein ***/
                        _StartSoundSource( &(ybd->bact.sc), VP_NOISE_COCKPIT );

                    } else {

                        /*** Flags rücksetzen ***/
                        ybd->flags &= ~YBF_Viewer;

                        /*** Übers Netz ***/
                        if( ywd->playing_network )
                            vm.viewer = 0;

                        /*** CockPitGeräusch aus ***/
                        _EndSoundSource( &(ybd->bact.sc), VP_NOISE_COCKPIT );

                        /* ----------------------------------------------------
                        ** Wenn ich Commander war, dann meinen Slaves mein
                        ** Hauptziel geben, weil im "FORMATIONslosen Zeitalter"
                        ** die Zielweitergabe nicht mehr automatisch erfolgt.
                        ** Ausserdem kann ich das geschwader ganz schoen auf-
                        ** gewuehlt haben. Als Slave uebernehme ich das Haupt-
                        ** ziel des Commanders. Das alles jedoch nur, wenn
                        ** wir nicht im Cyclemodus sind.
                        ** --------------------------------------------------*/
                        if( (BCLID_YPAMISSY != ybd->bact.BactClassID) &&
                            (BCLID_YPAROBO  != ybd->bact.BactClassID) &&
                            (ACTION_DEAD    != ybd->bact.MainState) ) {
                        
                            if( ybd->bact.master == ybd->bact.robo ) {
                                
                                if( !((ybd->bact.ExtraState & EXTRA_DOINGWAYPOINT) &&
                                      (ybd->bact.ExtraState & EXTRA_WAYPOINTCYCLE)) ) {
                                    /*** mein Ziel an Slaves ***/
                                    struct OBNode *slave;
                                    slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                                    while( slave->nd.mln_Succ ) {
                                        yb_TakeCommandersTarget( slave->bact, &(ybd->bact), ybd->world );
                                        slave = (struct OBNode *) slave->nd.mln_Succ;
                                        } 
                                    }
                                }
                            else {
                                if( !((ybd->bact.master_bact->ExtraState & EXTRA_DOINGWAYPOINT) &&
                                      (ybd->bact.master_bact->ExtraState & EXTRA_WAYPOINTCYCLE)) ) {
                                    /*** Ziel vom Commander uebernehmen ***/
                                    yb_TakeCommandersTarget( &(ybd->bact), ybd->bact.master_bact, ybd->world );
                                    }
                                }
                            }
                    };

                    #ifdef __NETWORK__
                    /*** Id aktualisieren und Message losschicken ***/
                    if( ywd->playing_network ) {

                        vm.generic.message_id = YPAM_VIEWER;
                        vm.generic.owner      = ybd->bact.Owner;
                        vm.class              = ybd->bact.BactClassID;
                        vm.ident              = ybd->bact.ident;
                        if( BCLID_YPAMISSY == vm.class ) {

                            struct Bacterium *rman;
                            _get( o, YMA_RifleMan, &rman );
                            vm.rifleman = rman->ident;
                            }

                        sm.receiver_id         = NULL;
                        sm.receiver_kind       = MSG_ALL;
                        sm.data                = &vm;
                        sm.data_size           = sizeof( vm );
                        sm.guaranteed          = TRUE;
                        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
                        }
                    #endif

                    break;

                case YBA_UserInput:
                    if (data) {
                        ybd->flags |= YBF_UserInput;
                        _set(ybd->world, YWA_UserVehicle, ybd->bact.BactObject);

                        /*** Zurechtrücken, sofern keine GUN!! ***/
                        if( BCLID_YPAGUN != ybd->bact.BactClassID )
                            _methoda( o, YBM_CHECKPOSITION, NULL );
                        }
                    else      ybd->flags &= ~YBF_UserInput;
                    break;
                
                case YBA_BactCollision:
                    if (data) ybd->flags |= YBF_BactCollision;
                    else      ybd->flags &= ~YBF_BactCollision;
                    break;
                
                case YBA_CheckVisExactly:
                    if (data) ybd->flags |= YBF_CheckVisExactly;
                    else      ybd->flags &= ~YBF_CheckVisExactly;
                    break;
                
                case YBA_LandWhileWait:
                    if (data) ybd->flags |= YBF_LandWhileWait;
                    else      ybd->flags &= ~YBF_LandWhileWait;
                    break;
                
                case YBA_AirConst:
                    ybd->bact.air_const    = (FLOAT) data;
                    ybd->bact.bu_air_const = (FLOAT) data;
                    break;

                case YBA_YourLS:
                    ybd->YourLastSeconds = (LONG) data;
                    break;

                case YBA_Aggression:
                    ybd->bact.Aggression = (UBYTE) data;
                    slave = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
                    while( slave->nd.mln_Succ ) {

                        slave->bact->Aggression = (UBYTE) data;
                        slave = (struct OBNode *) slave->nd.mln_Succ;
                        }
                    break;

                case YBA_VPTForm:

                    ybd->vp_tform = (struct tform *) data;
                    break;

                case YBA_VisProto:

                    ybd->vis_proto = (ULONG *) data;
                    break;

                case YBA_ExtraViewer:

                    if( data )
                        ybd->flags |=  YBF_ExtraViewer;
                    else
                        ybd->flags &= ~YBF_ExtraViewer;
                    break;

                case YBA_AlwaysRender:

                    if( data )
                        ybd->flags |=  YBF_AlwaysRender;
                    else
                        ybd->flags &= ~YBF_AlwaysRender;
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yb_getAttrs(Object *o, struct ypabact_data *ybd, struct TagItem *attrs)
/*
**  FUNCTION
**      Handelt alle (G)-Attribute komplett ab.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      24-Apr-95   floh    created
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs  = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* dann die eigentlichen Attribute, schön nacheinander */
            switch (tag) {

                case YBA_TForm:
                    *value = (ULONG) &(ybd->bact.tf);
                    break;

                case YBA_Bacterium:
                    *value = (ULONG) &(ybd->bact);
                    break;

                case YBA_Viewer:
                    *value = (ULONG) (ybd->flags & YBF_Viewer) ? TRUE:FALSE;
                    break;

                case YBA_UserInput:
                    *value = (ULONG) (ybd->flags & YBF_UserInput) ? TRUE:FALSE;
                    break;

                case YBA_CheckVisExactly:
                    *value = (ULONG) (ybd->flags & YBF_CheckVisExactly) ? TRUE:FALSE;
                    break;

                case YBA_BactCollision:
                    *value = (ULONG) (ybd->flags & YBF_BactCollision) ? TRUE:FALSE;
                    break;

                case YBA_LandWhileWait:
                    *value = (ULONG) (ybd->flags & YBF_LandWhileWait) ? TRUE:FALSE;
                    break;

                case YBA_World:
                    *value = (ULONG) ybd->world;
                    break;

                case YBA_AttckList:
                    *value = (ULONG) &(ybd->attck_list);
                    break;

                case YBA_YourLS:
                    *value = (ULONG) ybd->YourLastSeconds;
                    break;

                case YBA_VisProto:
                    *value = (ULONG) ybd->vis_proto;
                    break;

                case YBA_Aggression:    // nur der Vollständigkeit halber...
                    *value = (ULONG) ybd->bact.Aggression;
                    break;

                /*** Wenn eine Klasse erweiterte Kollision hat, muß sie ***/
                /*** das Attribut überlagern!                           ***/
                
                case YBA_ExtCollision:
                    *value = 0L;
                    break;

                case YBA_VPTForm:

                    *value = (ULONG) ybd->vp_tform;
                    break;
                
                case YBA_ExtraViewer:

                    *value = (ULONG) (ybd->flags & YBF_ExtraViewer) ? TRUE:FALSE;
                    break;

                case YBA_PtAttckNode:

                    *value = (ULONG) &(ybd->pt_attck_node);
                    break;

                case YBA_StAttckNode:

                    *value = (ULONG) &(ybd->st_attck_node);
                    break;
                
                case YBA_AlwaysRender:

                    *value = (ULONG) (ybd->flags & YBF_AlwaysRender) ? TRUE:FALSE;
                    break;

            };
        };
    };

    /*** Ende ***/
}


