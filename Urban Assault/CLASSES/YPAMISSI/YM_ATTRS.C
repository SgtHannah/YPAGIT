/*
**  $Source: PRG:MovingCubesII/Classes/_YPABactClass/yb_attrs.c,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:22:59 $
**  $Locker: floh $
**  $Author: floh $
**
**  Attribut-Handling fnr ypazepp.class.
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

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


/*-----------------------------------------------------------------*/
BOOL ym_initAttrs(Object *o, struct ypamissile_data *ymd, struct TagItem *attrs)
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

    /*** zuerst die Defaultwerte ***/
    ymd->handle         = YMF_Bomb;
    ymd->rifle_man      = NULL;
    ymd->life_time      = YMA_LifeTime_DEF;
    ymd->delay          = 0;


    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die SonderfSlle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* die Attribute */
            switch(tag) {

                case YBA_World:

                    /* -----------------------------------------------------
                    ** zuerst filtern wir den Weltpointer raus, den brauchen
                    ** wir hier auch.
                    ** ---------------------------------------------------*/
                    ymd->world = (Object *) data;
                    ymd->ywd   = INST_DATA( ((struct nucleusdata *)data)->o_Class,
                                 data);
                    break;

                case YBA_Viewer:

                    /* Das wird von der SuperClass abgehandelt. Wir setzen nur
                    ** das Flag */
                    if( data )
                        ymd->flags |= YMF_Viewer;
                    else
                        ymd->flags &= ~YMF_Viewer;
                    break;

                case YMA_Handle:

                    ymd->handle = (UBYTE) data;
                    break;

                case YMA_RifleMan:

                    ymd->rifle_man = (struct Bacterium *) data;
                    break;

                case YMA_LifeTime:

                    ymd->life_time = data;
                    break;

                case YMA_DriveTime:

                    ymd->drive_time = data;
                    break;

                case YMA_Delay:

                    ymd->delay = (LONG) data;
                    break;

                case YMA_IgnoreBuildings:

                    if( data )
                        ymd->flags |= YMF_IgnoreBuildings;
                    else
                        ymd->flags &= ~YMF_IgnoreBuildings;
                    break;

            };
        };
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void ym_setAttrs(Object *o, struct ypamissile_data *ymd, struct TagItem *attrs)
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

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die SonderfSlle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

            /* dann die eigentlichen Attribute, sch÷n nacheinander */
            switch (tag) {

                case YBA_Viewer:

                    /* Das wird von der SuperClass abgehandelt. Wir setzen nur
                    ** das Flag */
                    if( data )
                        ymd->flags |=  YMF_Viewer;
                    else
                        ymd->flags &= ~YMF_Viewer;
                    break;

                case YMA_Handle:

                    ymd->handle = (UBYTE) data;
                    break;

                case YMA_RifleMan:

                    ymd->rifle_man = (struct Bacterium *) data;
                    break;

                case YMA_LifeTime:

                    ymd->life_time = data;
                    break;

                case YMA_DriveTime:

                    ymd->drive_time = data;
                    break;

                case YMA_Delay:

                    ymd->delay = (LONG) data;
                    break;

                case YMA_IgnoreBuildings:

                    if( data )
                        ymd->flags |= YMF_IgnoreBuildings;
                    else
                        ymd->flags &= ~YMF_IgnoreBuildings;
                    break;
                    
                case YMA_EnergyHeli:
                
                    ymd->energy_heli = ((FLOAT)data) / 1000.0;
                    break;
                                        
                case YMA_EnergyTank:
                
                    ymd->energy_tank = ((FLOAT)data) / 1000.0;
                    break;
                                        
                case YMA_EnergyFlyer:
                
                    ymd->energy_flyer = ((FLOAT)data) / 1000.0;
                    break;
                                        
                case YMA_EnergyRobo:
                
                    ymd->energy_robo = ((FLOAT)data) / 1000.0;
                    break;          
                    
                case YMA_RadiusHeli:
                
                    ymd->radius_heli = (FLOAT)data;
                    break;              
                    
                case YMA_RadiusTank:
                
                    ymd->radius_tank = (FLOAT)data;
                    break;              
                    
                case YMA_RadiusFlyer:
                
                    ymd->radius_flyer = (FLOAT)data;
                    break;              
                    
                case YMA_RadiusRobo:
                
                    ymd->radius_robo = (FLOAT)data;
                    break;              
            };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void ym_getAttrs(Object *o, struct ypamissile_data *ymd, struct TagItem *attrs)
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

            /* erstmal die SonderfSlle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs  = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* dann die eigentlichen Attribute, sch÷n nacheinander */
            switch (tag) {

                case YMA_AutoNode:

                    *value = (ULONG) &(ymd->auto_node);
                    break;

                case YMA_LifeTime:

                    *value = (ULONG) ymd->life_time;
                    break;

                case YMA_DriveTime:

                    *value = (ULONG) ymd->drive_time;
                    break;

                case YMA_Handle:

                    *value = (ULONG) ymd->handle;
                    break;

                case YMA_RifleMan:

                    *value = (ULONG) ymd->rifle_man;
                    break;

                case YMA_Delay:

                    *value = (ULONG) ymd->delay;
                    break;

                case YMA_IgnoreBuildings:

                    *value = (ULONG)(ymd->flags & YMF_IgnoreBuildings);
                    break; 
                    
                case YMA_EnergyHeli:
                
                    *value = (ULONG)(1000 * ymd->energy_heli);
                    break;   
                    
                case YMA_EnergyTank:
                
                    *value = (ULONG)(1000 * ymd->energy_tank);
                    break;   
                    
                case YMA_EnergyFlyer:
                
                    *value = (ULONG)(1000 * ymd->energy_flyer);
                    break;   
                    
                case YMA_EnergyRobo:
                
                    *value = (ULONG)(1000 * ymd->energy_robo);
                    break; 
                    
                case YMA_RadiusHeli:
                
                    *value = (ULONG)ymd->radius_heli;
                    break;
                    
                case YMA_RadiusTank:
                
                    *value = (ULONG)ymd->radius_tank;
                    break;
                    
                case YMA_RadiusFlyer:
                
                    *value = (ULONG)ymd->radius_flyer;
                    break;
                    
                case YMA_RadiusRobo:
                
                    *value = (ULONG)ymd->radius_robo;
                    break;
                          
            };
        };
    };

    /*** Ende ***/
}

