/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Attribut-Handling für yparobo.class.
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
#include "ypa/yparoboclass.h"
#include "ypa/ypagunclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

extern ULONG YPA_CommandCount;         // 0 heißt kein Geschwader!!!!!!!!

void yr_dir_rot_round_lokal_y( struct flt_m3x3 *dir, FLOAT angle);

/*-----------------------------------------------------------------*/
void yr_StandardMatrix( struct flt_m3x3 *dir )
{
    dir->m11 = dir->m22 = dir->m33 = 1.0;
    dir->m12 = dir->m13 = dir->m21 = 0.0;
    dir->m23 = dir->m31 = dir->m32 = 0.0;
}


BOOL yr_initAttrs(Object *o, struct yparobo_data *yrd, struct TagItem *attrs)
/*
**  FUNCTION
**      (I)-Attribut-Handler.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/
{
    register ULONG tag;

    /* 
    ** Zuerst die ganzen Defaultwerte, die bei Bedarf wieder
    ** überschrieben werden 
    **
    ** Die Roboextension wird nur noch in setattrs abgehandelt
    ** (wird ja in init sowieso nicht gebraucht)
    */

    yrd->ep_Conquer         = (UBYTE)YRA_EPConquer_DEF;
    yrd->ep_Defense         = (UBYTE)YRA_EPDefense_DEF;
    yrd->ep_Radar           = (UBYTE)YRA_EPRadar_DEF;
    yrd->ep_Power           = (UBYTE)YRA_EPPower_DEF;
    yrd->ep_Safety          = (UBYTE)YRA_EPSafety_DEF;
    yrd->ep_Reconnoitre     = (UBYTE)YRA_EPReconnoitre_DEF;
    yrd->ep_ChangePlace     = (UBYTE)YRA_EPChangePlace_DEF;
    yrd->waitflags          = 0;
    yrd->FillModus          = YRF_Fill_All;



    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* die Attribute */
            switch(tag) {

                case YBA_World:

                    /* zuerst filtern wir den Weltpointer raus, den brauchen
                    ** wir hier auch. */
                    yrd->world = (Object *) data;
                    yrd->ywd   = INST_DATA( ((struct nucleusdata *)data)->o_Class,
                                 data);
                    break;

                case YBA_UserInput:

                    if (data) {

                        /*** Zusätzlich noch ... ***/
                        _set(yrd->world, YWA_UserRobo, yrd->bact->BactObject);
                        yrd->RoboState |= ROBO_USERROBO;
                        }

                    break;

                /*** Strategie ***/
                case YRA_EPConquer:
                    
                    yrd->ep_Conquer = (UBYTE) data;
                    break;

                case YRA_EPDefense:
                    
                    yrd->ep_Defense = (UBYTE) data;
                    break;

                case YRA_EPRadar:
                    
                    yrd->ep_Radar = (UBYTE) data;
                    break;

                case YRA_EPPower:
                    
                    yrd->ep_Power = (UBYTE) data;
                    break;

                case YRA_EPSafety:
                    
                    yrd->ep_Safety = (UBYTE) data;
                    break;

                case YRA_EPReconnoitre:
                    
                    yrd->ep_Reconnoitre = (UBYTE) data;
                    break;

                case YRA_EPChangePlace:
                    
                    yrd->ep_ChangePlace = (UBYTE) data;
                    break;

                case YRA_EPRobo:
                    
                    yrd->ep_Robo = (UBYTE) data;
                    break;

                case YRA_BattVehicle:

                    yrd->BattVehicle = data;
                    break;

                case YRA_BattBuilding:

                    yrd->BattBuilding = data;
                    break;

                case YRA_BattBeam:

                    yrd->BattBeam = data;
                    break;

                case YRA_FillModus:

                    yrd->FillModus = (UBYTE) data;
                    break;

                case YRA_WaitSway:

                    if( data )
                        yrd->waitflags |=  RWF_SWAY;
                    else
                        yrd->waitflags &= ~RWF_SWAY;
                    break;

                case YRA_WaitRotate:

                    if( data )
                        yrd->waitflags |=  RWF_ROTATE;
                    else
                        yrd->waitflags &= ~RWF_ROTATE;
                    break;

                case YRA_ViewAngle:

                    /* ------------------------------------------------------
                    ** Etwas obskur, weil in Grad. müssen wir umrechnen.
                    ** Der Winkel bezieht sich auf die Nordstellung, folglich
                    ** müssen wir die Viewermatrix erstmal ausrichten.
                    ** ----------------------------------------------------*/
                    yr_StandardMatrix( &(yrd->bact->Viewer.dir));

                    /*** Drehen ***/
                    yrd->bact->Viewer.horiz_angle = (data * PI) / 180.0;
                    yr_dir_rot_round_lokal_y( &(yrd->bact->Viewer.dir),
                                                yrd->bact->Viewer.horiz_angle );
                    break;

                case YRA_SafDelay:

                    yrd->chk_Safety_Delay = (LONG) data;
                    break;

                case YRA_PowDelay:

                    yrd->chk_Power_Delay = (LONG) data;
                    break;

                case YRA_RadDelay:

                    yrd->chk_Radar_Delay = (LONG) data;
                    break;

                case YRA_CplDelay:

                    yrd->chk_Place_Delay = (LONG) data;
                    break;

                case YRA_DefDelay:

                    yrd->chk_Enemy_Delay = (LONG) data;
                    break;

                case YRA_ConDelay:

                    yrd->chk_Terr_Delay = (LONG) data;
                    break;

                case YRA_RobDelay:

                    yrd->chk_Robo_Delay = (LONG) data;
                    break;

                case YRA_RecDelay:

                    yrd->chk_Recon_Delay = (LONG) data;
                    break;
            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yr_setAttrs(Object *o, struct yparobo_data *yrd, struct TagItem *attrs)
/*
**  FUNCTION
**      (S)-Attribut-Handler.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      20-Jul-95   8100000C    created
*/
{
    register ULONG tag;
    int i;
    struct RoboExtension *ext;
    struct gun_data *gd;

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

                case YBA_UserInput:

                    if (data) {

                        /*** Zusätzlich noch ... ***/
                        _set(yrd->world, YWA_UserRobo, yrd->bact->BactObject);
                        yrd->RoboState |= ROBO_USERROBO;
                        }
                    break;

                case YRA_Extension:

                    /*
                    ** Es wird eine Struktur übergeben, aus der wir Sachen kopieren.
                    */
                    ext = (struct RoboExtension *) data;
                    
                    /*** alte Kanonen freigeben ***/
                    for( gd = &(yrd->gun[ 0 ]);
                         gd < &(yrd->gun[ NUMBER_OF_GUNS ]);
                         gd++ )

                        if( gd->go ) {

                            struct Bacterium *gb;
                            Object *kanone;

                            /*** Kanone merken, weil YBM_DIE diese abmeldet ***/
                            kanone = gd->go;

                            /*** auch sterben lassen? ***/
                            _get( gd->go, YBA_Bacterium, &gb );
                            if( !(gb->ExtraState & EXTRA_LOGICDEATH ) )
                                _methoda( gd->go, YBM_DIE, NULL );

                            /*** Rückgabe auch übers netz ??? ***/
                            _methoda( o, YBM_RELEASEVEHICLE, kanone );
                            gd->go = NULL;
                            }

                    /*** neue Kanonen übernehmen ***/
                    for( i=0; i < ext->number_of_guns; i++ ) {

                        struct createvehicle_msg cv;
                        struct installgun_msg ig;
                        struct Bacterium *gbact;

                        /*** Daten kopieren ***/
                        yrd->gun[ i ] = ext->gun[ i ];

                        /*** Kanonen erzeugen ***/
                        cv.x  = yrd->bact->pos.x + yrd->bact->dir.m11 * yrd->gun[ i ].pos.x +
                                                   yrd->bact->dir.m21 * yrd->gun[ i ].pos.y +
                                                   yrd->bact->dir.m31 * yrd->gun[ i ].pos.z;
                        cv.y  = yrd->bact->pos.y + yrd->bact->dir.m12 * yrd->gun[ i ].pos.x +
                                                   yrd->bact->dir.m22 * yrd->gun[ i ].pos.y +
                                                   yrd->bact->dir.m32 * yrd->gun[ i ].pos.z;
                        cv.z  = yrd->bact->pos.z + yrd->bact->dir.m13 * yrd->gun[ i ].pos.x +
                                                   yrd->bact->dir.m23 * yrd->gun[ i ].pos.y +
                                                   yrd->bact->dir.m33 * yrd->gun[ i ].pos.z;
                        cv.vp = yrd->gun[ i ].gun;

                        if( !(yrd->gun[ i ].go = (Object *)
                            _methoda( yrd->world, YWM_CREATEVEHICLE, &cv))) {

                            /*** Fehlschlag ***/
                            _LogMsg("Unable to create Robo-Gun\n");
                            }
                        else {

                            struct ypaworld_data *ywd;

                            /*** Kanone einrichten ***/
                            ig.basis = ext->gun[ i ].dir;
                            ig.flags = 0;
                            _methoda( yrd->gun[ i ].go, YGM_INSTALLGUN, &ig );

                            /*** Markieren ***/
                            _set( yrd->gun[ i ].go, YGA_RoboGun, TRUE );

                            /*** Initialisieren ***/
                            _get( yrd->gun[ i ].go, YBA_Bacterium, &gbact );
                            gbact->Owner      = yrd->bact->Owner;
                            gbact->CommandID  = YPA_CommandCount++;
                            gbact->robo       = yrd->bact->BactObject;

                            #ifdef __NETWORK__
                            ywd = INST_DATA( ((struct nucleusdata *)yrd->world)->o_Class, yrd->world);
                            if( ywd->playing_network ) {

                                gbact->ident     |= (((ULONG)gbact->Owner) << 24);
                                gbact->CommandID |= (((ULONG)gbact->Owner) << 24);
                                }
                            #endif
                            gbact->Aggression = 60;

                            /*** Kanone in Liste einklinken ***/
                            _methoda( yrd->bact->BactObject, YBM_ADDSLAVE, yrd->gun[ i ].go);
                            }
                        }

                    /*** Dock-Position ***/
                    yrd->dock_pos = ext->dock_pos;

                    /*** ViewerPosition ***/
                    yrd->bact->Viewer.relpos   = ext->Viewer.relpos;   // 0, 90, 120
                    yrd->bact->Viewer.max_up   = ext->Viewer.max_up;
                    yrd->bact->Viewer.max_down = ext->Viewer.max_down;
                    yrd->bact->Viewer.max_side = ext->Viewer.max_side;
                    _set( o, YBA_ExtraViewer, TRUE );
                    _set( o, YBA_AlwaysRender, TRUE );

                    /*** Kollisionsstruktur ***/
                    yrd->coll = ext->coll;
                    break;

                /*** Strategie ***/
                case YRA_EPConquer:
                    
                    yrd->ep_Conquer = (UBYTE) data;
                    break;

                case YRA_EPDefense:
                    
                    yrd->ep_Defense = (UBYTE) data;
                    break;

                case YRA_EPRadar:
                    
                    yrd->ep_Radar = (UBYTE) data;
                    break;

                case YRA_EPPower:
                    
                    yrd->ep_Power = (UBYTE) data;
                    break;

                case YRA_EPSafety:
                    
                    yrd->ep_Safety = (UBYTE) data;
                    break;

                case YRA_EPChangePlace:
                    
                    yrd->ep_ChangePlace = (UBYTE) data;
                    break;

                case YRA_EPReconnoitre:
                    
                    yrd->ep_Reconnoitre = (UBYTE) data;
                    break;

                case YRA_EPRobo:
                    
                    yrd->ep_Robo = (UBYTE) data;
                    break;

                case YRA_CommandCount:

                    YPA_CommandCount = data;
                    break;

                case YRA_BattVehicle:

                    yrd->BattVehicle = data;
                    break;

                case YRA_BattBuilding:

                    yrd->BattBuilding = data;
                    break;

                case YRA_BattBeam:

                    yrd->BattBeam = data;
                    break;

                case YRA_FillModus:

                    yrd->FillModus = (UBYTE) data;
                    break;

                case YRA_WaitSway:

                    if( data )
                        yrd->waitflags |=  RWF_SWAY;
                    else
                        yrd->waitflags &= ~RWF_SWAY;
                    break;

                case YRA_WaitRotate:

                    if( data )
                        yrd->waitflags |=  RWF_ROTATE;
                    else
                        yrd->waitflags &= ~RWF_ROTATE;
                    break;

                case YRA_ViewAngle:

                    /* ------------------------------------------------------
                    ** Etwas obskur, weil in Grad. müssen wir umrechnen.
                    ** Der Winkel bezieht sich auf die Nordstellung, folglich
                    ** müssen wir die Viewermatrix erstmal ausrichten.
                    ** ----------------------------------------------------*/
                    yr_StandardMatrix( &(yrd->bact->Viewer.dir));

                    /*** Drehen ***/
                    yrd->bact->Viewer.horiz_angle = (data * PI) / 180.0;
                    yr_dir_rot_round_lokal_y( &(yrd->bact->Viewer.dir),
                                                yrd->bact->Viewer.horiz_angle );
                    break;

                case YRA_SafDelay:

                    yrd->chk_Safety_Delay = (LONG) data;
                    break;

                case YRA_PowDelay:

                    yrd->chk_Power_Delay = (LONG) data;
                    break;

                case YRA_RadDelay:

                    yrd->chk_Radar_Delay = (LONG) data;
                    break;

                case YRA_CplDelay:

                    yrd->chk_Place_Delay = (LONG) data;
                    break;

                case YRA_DefDelay:

                    yrd->chk_Enemy_Delay = (LONG) data;
                    break;

                case YRA_ConDelay:

                    yrd->chk_Terr_Delay = (LONG) data;
                    break;

                case YRA_RobDelay:

                    yrd->chk_Robo_Delay = (LONG) data;
                    break;

                case YRA_RecDelay:

                    yrd->chk_Recon_Delay = (LONG) data;
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yr_getAttrs(Object *o, struct yparobo_data *yrd, struct TagItem *attrs)
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
**      28-Apr-98   floh    + YRA_LoadFlags und YRA_LossFlags
**      20-May-98   floh    + YRA_AbsReload
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
                
                /*** Strategie ***/
                case YRA_EPConquer:
                    
                    *value = (LONG) yrd->ep_Conquer;
                    break;

                case YRA_EPDefense:
                    
                    *value = (LONG) yrd->ep_Defense;
                    break;

                case YRA_EPRadar:
                    
                    *value = (LONG) yrd->ep_Radar;
                    break;

                case YRA_EPPower:
                    
                    *value = (LONG) yrd->ep_Power;
                    break;

                case YRA_EPSafety:
                    
                    *value = (LONG) yrd->ep_Safety;
                    break;

                case YRA_EPChangePlace:
                    
                    *value = (LONG) yrd->ep_ChangePlace;
                    break;

                case YRA_EPReconnoitre:
                    
                    *value = (LONG) yrd->ep_Reconnoitre;
                    break;

                case YRA_EPRobo:
                    
                    *value = (LONG) yrd->ep_Robo;
                    break;

                case YBA_ExtCollision:

                    *value = (ULONG) &(yrd->coll);
                    break;

                case YRA_CommandCount:

                    *value = YPA_CommandCount;
                    break;

                case YRA_BattVehicle:

                    *value = yrd->BattVehicle;
                    break;

                case YRA_BattBuilding:

                    *value = yrd->BattBuilding;
                    break;

                case YRA_BattBeam:

                    *value = yrd->BattBeam;
                    break;

                case YRA_FillModus:

                    *value = (LONG)yrd->FillModus;
                    break;
                
                case YRA_GunArray:

                    *value = (LONG) (yrd->gun);
                    break;
                
                case YRA_RoboState:

                    *value = yrd->RoboState;
                    break;

                case YRA_SafDelay:

                    *value = yrd->chk_Safety_Delay;
                    break;

                case YRA_PowDelay:

                    *value = yrd->chk_Power_Delay;
                    break;

                case YRA_RadDelay:

                    *value = yrd->chk_Radar_Delay;
                    break;

                case YRA_CplDelay:

                    *value = yrd->chk_Place_Delay;
                    break;

                case YRA_ConDelay:

                    *value = yrd->chk_Terr_Delay;
                    break;

                case YRA_DefDelay:

                    *value = yrd->chk_Enemy_Delay;
                    break;

                case YRA_RobDelay:

                    *value = yrd->chk_Robo_Delay;
                    break;

                case YRA_RecDelay:

                    *value = yrd->chk_Recon_Delay;
                    break;
                    
                case YRA_LoadFlags:
                    *value = yrd->LoadFlags;
                    break;
                    
                case YRA_LossFlags:
                    *value = yrd->LossFlags;
                    break;                 
                    
                case YRA_AbsReload:
                    *value = yrd->AbsReload;
                    break;
            };
        };
    };

    /*** Ende ***/
}

