/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_attrib.c,v $
**  $Revision: 38.26 $
**  $Date: 1998/01/06 16:17:09 $
**  $Locker: floh $
**  $Author: floh $
**
**  Attribute init/set/get.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include <stdlib.h>
#include <ctype.h>

#include "nucleus/nucleus2.h"
#include "ypa/ypavehicles.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

/*-----------------------------------------------------------------*/
BOOL yw_initAttrs(Object *o, struct ypaworld_data *ywd, struct TagItem *attrs)
/*
**  FUNCTION
**      Handelt (I)-Attribute vollständig ab.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      24-Apr-95   floh    created
**      28-Jun-95   floh    neu: YWA_WorldDescription
**      11-Jul-95   floh    (1) neu: Depthfading-Attribute
**                          (2) verwendet die gute alte _GetTagData()
**                              Methode, weils ja wohl kaum auf Speed
**                              ankommt...
**      12-Jul-95   floh    neu: YWA_HeavenHeight, YWA_RenderHeaven
**      29-Jul-95   floh    neu: YWA_DoEnergyCycle
**      25-Aug-95   floh    neu: YWA_VisSectors
**      05-Sep-95   floh    Ooops, return() vergessen...
**      28-Oct-95   floh    MapSizeX, MapSizeY werden jetzt mit den
**                          MapMaxX, MapMaxY initialisiert
**      24-Jan-96   floh    - YWA_WorldDescription
**                          (das hat sich alles in die Set-Initialisierung
**                          verflüchtigt)
**      31-Oct-96   floh    Default-Value für NumDestFX
**      11-Jun-97   floh    + YWA_Version
*/
{
    ywd->NumDestFX = YWA_NumDestFX_DEF;

    /*** alles easy... ***/
    ywd->VisSectors   = _GetTagData(YWA_VisSectors, YWA_VisSectors_DEF, attrs);
    ywd->NormVisLimit = _GetTagData(YWA_NormVisLimit, YWA_NormVisLimit_DEF, attrs);
    ywd->NormDFadeLen = _GetTagData(YWA_NormDFadeLen, YWA_NormDFadeLen_DEF, attrs);
    ywd->HeavenVisLimit = _GetTagData(YWA_HeavenVisLimit, YWA_HeavenVisLimit_DEF, attrs);
    ywd->HeavenDFadeLen = _GetTagData(YWA_HeavenDFadeLen, YWA_HeavenDFadeLen_DEF, attrs);
    ywd->MapMaxX        = _GetTagData(YWA_MapMaxX, YWA_MapMaxX_DEF, attrs);
    ywd->MapMaxY        = _GetTagData(YWA_MapMaxY, YWA_MapMaxY_DEF, attrs);
    ywd->MapSizeX       = ywd->MapMaxX;
    ywd->MapSizeY       = ywd->MapMaxY;
    ywd->HeavenHeight   = _GetTagData(YWA_HeavenHeight, YWA_HeavenHeight_DEF, attrs);
    ywd->RenderHeaven   = _GetTagData(YWA_RenderHeaven, YWA_RenderHeaven_DEF, attrs);
    ywd->DoEnergyCycle  = _GetTagData(YWA_DoEnergyCycle, YWA_DoEnergyCycle_DEF, attrs);
    ywd->Version        = _GetTagData(YWA_Version, NULL, attrs);
    if (ywd->Version) {
        /*** Versions-String ausgabefähig machen ***/
        UBYTE *str = ywd->Version;
        UBYTE chr;
        while (chr=*str) {
            chr = toupper(chr);
            if ((chr<32)||(chr>'Z')) chr='*';
            *str++ = chr;
        };
    };

    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_setAttrs(Object *o, struct ypaworld_data *ywd, struct TagItem *attrs)
/*
**  FUNCTION
**      Handelt alle (S)-Attribute komplett ab.
**      Handicap: OM_SET unterstützt keinen Return-Value!
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      24-Apr-95   floh    created
**      16-May-95   floh    YWA_LevelDescription: die Definition des
**                          Sektor-Map-Polygons wurde korrigiert.
**      11-Jun-95   floh    YWA_Robo existiert nicht mehr
**      28-Jun-95   floh    YWA_LevelDescription existiert nicht mehr
**      12-Jul-95   floh    neu: YWA_HeavenHeight, YWA_RenderHeaven
**      29-Jul-95   floh    neu: YWA_DoEnergyCycle
**      25-Aug-95   floh    neu: YWA_VisSectors
**      06-Dec-95   floh    + YWA_UserRobo
**                          + YWA_UserVehicle
**      20-Dec-95   floh    YWA_UserRobo ermittelt jetzt auch
**                          Pointer auf die Slave-Liste des Robos
**      11-Feb-96   floh    + YWA_LevelFinished
**      27-May-96   floh    + AF's letztes Update
**      25-Jul-96   floh    + YWA_DspXRes, YWA_DspYRes
**      07-Aug-96   floh    + YWA_LevelFinished nicht mehr settable
**      10-Aug-96   floh    + YWA_UserVehicle setzt ShowUserVehicle-Counter
**      19-Aug-96   floh    + Depthfading-Attribute jetzt auch
**                            settable
**      29-Oct-97   floh    + beim Wechseln in ein anderes Vehicle
**                            wird der Joystick-Blanker angeschaltet
**      14-Jan-98   floh    + YWA_DontRender
**      24-Apr-98   floh    + LastOccupiedID wird beim Vehicle-Umschalten
**                            ausgefuellt
*/
{
    register ULONG tag;

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

                    case YWA_NormVisLimit:
                        ywd->NormVisLimit = data;
                        break;

                    case YWA_NormDFadeLen:
                        ywd->NormDFadeLen = data;
                        break;

                    case YWA_HeavenVisLimit:
                        ywd->HeavenVisLimit = data;
                        break;

                    case YWA_HeavenDFadeLen:
                        ywd->HeavenDFadeLen = data;
                        break;

                    case YWA_HeavenHeight:
                        ywd->HeavenHeight = data;
                        break;

                    case YWA_RenderHeaven:
                        ywd->RenderHeaven = data;
                        break;

                    case YWA_DoEnergyCycle:
                        ywd->DoEnergyCycle = data;
                        break;

                    case YWA_VisSectors:
                        ywd->VisSectors = data;
                        break;

                    case YWA_UserRobo:
                        ywd->UserRobo = (Object *) data;
                        _get(ywd->UserRobo, YBA_Bacterium, &(ywd->URBact));
                        ywd->URSlaves = &(ywd->URBact->slave_list);
                        break;

                    case YWA_UserVehicle:
                        if (ywd->UserVehicle != ((Object *)data)) {
                            /*** Window-Handling ***/
                            struct Bacterium *old_vhcl = ywd->UVBact;                            
                            if (ywd->UVBact) ywd->LastOccupiedID = ywd->UVBact->ident;
                            ywd->UserVehicle          = (Object *) data;
                            _get(ywd->UserVehicle, YBA_Bacterium, &(ywd->UVBact));
                            ywd->UserVehicleTimeStamp = ywd->TimeStamp;
                            ywd->UserVehicleCmdId     = ywd->UVBact->CommandID;
                            ywd->DragLock             = FALSE;
                            ywd->JoyIgnoreX           = TRUE;
                            ywd->JoyIgnoreY           = TRUE;
                            // ywd->JoyIgnoreZ           = FALSE;
                            yw_FFVehicleChanged(ywd);
                            if (old_vhcl) yw_SRHandleVehicleSwitch(ywd,old_vhcl,ywd->UVBact);
                        };
                        break;

                    case YWA_DspXRes:
                        ywd->DspXRes = data;
                        break;

                    case YWA_DspYRes:
                        ywd->DspYRes = data;
                        break;

                    case YWA_DontRender:
                        ywd->DontRender = data;
                        break;
                };
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_getAttrs(Object *o, struct ypaworld_data *ywd, struct TagItem *attrs)
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
**      12-Jul-95   floh    neu: YWA_HeavenHeight, YWA_RenderHeaven
**      25-Aug-95   floh    neu: YWA_VisSectors
**      27-Aug-95   floh    YWA_NormVisLimit, YWA_NormDFadeLen,
**      06-Dec-95   floh    + YWA_UserRobo
**                          + YWA_UserVehicle
**                          + YWA_WeaponArray
**                          + YWA_BuildArray
**                          + YWA_VehicleArray
**      20-Dec-95   floh    YWA_WeaponArray, YWA_BuildArray, YWA_VehicleArray
**                          ausgefüllt
**      11-Feb-96   floh    + YWA_LevelFinished
**      25-Jul-96   floh    + YWA_LocaleHandle
**      25-Jul-96   floh    + YWA_DspXRes, YWA_DspYRes
**      06-Aug-96   floh    + YWA_GateStatus
**      07-Aug-96   floh    - YWA_GateStaus :-)
**      02-Oct-96   floh    + YWA_LevelInfo
**      31-Oct-96   floh    + YWA_NumDestFX von AF
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + WP_Array nicht mehr global
**                          + BP_Array nicht mehr global
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

                    case YWA_MapMaxX:
                        *value = (ULONG) ywd->MapMaxX;
                        break;

                    case YWA_MapMaxY:
                        *value = (ULONG) ywd->MapMaxY;
                        break;

                    case YWA_MapSizeX:
                        *value = (ULONG) ywd->MapSizeX;
                        break;

                    case YWA_MapSizeY:
                        *value = (ULONG) ywd->MapSizeY;
                        break;

                    case YWA_SectorSizeX:
                        *(FLOAT *)value = ywd->SectorSizeX;
                        break;

                    case YWA_SectorSizeY:
                        *(FLOAT *)value = ywd->SectorSizeY;
                        break;

                    case YWA_HeavenHeight:
                        *value = ywd->HeavenHeight;
                        break;

                    case YWA_RenderHeaven:
                        *value = ywd->RenderHeaven;
                        break;

                    case YWA_DoEnergyCycle:
                        *value = ywd->DoEnergyCycle;
                        break;

                    case YWA_VisSectors:
                        *value = ywd->VisSectors;
                        break;

                    case YWA_NormVisLimit:
                        *value = ywd->NormVisLimit;
                        break;

                    case YWA_NormDFadeLen:
                        *value = ywd->NormDFadeLen;
                        break;

                    case YWA_VTypes:
                        *value = (ULONG) NULL;
                        break;

                    case YWA_UserRobo:
                        *value = (ULONG) ywd->UserRobo;
                        break;

                    case YWA_UserVehicle:
                        *value = (ULONG) ywd->UserVehicle;
                        break;

                    case YWA_WeaponArray:
                        *value = (ULONG) ywd->WP_Array;
                        break;

                    case YWA_BuildArray:
                        *value = (ULONG) ywd->BP_Array;
                        break;

                    case YWA_VehicleArray:
                        *value = (ULONG) ywd->VP_Array;
                        break;

                    case YWA_LevelFinished:
                        if ((ywd->Level->Status == LEVELSTAT_FINISHED) ||
                            (ywd->Level->Status == LEVELSTAT_ABORTED))
                        {
                            *value = TRUE;
                        } else {
                            *value = FALSE;
                        };
                        break;

                    case YWA_LocaleHandle:
                        *value = (ULONG) ywd->LocaleArray;
                        break;

                    case YWA_DspXRes:
                        *value = (ULONG) ywd->DspXRes;
                        break;

                    case YWA_DspYRes:
                        *value = (ULONG) ywd->DspYRes;
                        break;

                    case YWA_LevelInfo:
                        *value = (ULONG) ywd->Level;
                        break;

                    case YWA_NumDestFX: // neu 8100 000C
                        *value = (ULONG) ywd->NumDestFX;
                        break;

                    #ifdef __NETWORK__
                    case YWA_NetObject:
                        *value = (ULONG) ywd->nwo;
                        break;
                    #endif
                };
        };
    };

    /*** Ende ***/
}

