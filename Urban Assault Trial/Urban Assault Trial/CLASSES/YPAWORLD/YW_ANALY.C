/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_analyze.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 16:16:58 $
**  $Locker:  $
**  $Author: floh $
**
**  yw_analyze.c -- Game State Analyzer
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "ypa/ypaworldclass.h"

_extern_use_nucleus

extern struct YPAStatusReq SR;

/*=================================================================**
**                                                                 **
**  ANALYZER CHECK ROUTINEN                                        **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
ULONG yw_EnemyAttacksStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet, ob Feinde gerade die Station angreifen...
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    LONG x,y;
    LONG start_x,start_y;
    LONG end_x,end_y;

    /*** falls im Radius von 1 Sektor Feinde sind (außer Ufos) ***/
    start_x = ywd->URBact->SectX-1;
    start_y = ywd->URBact->SectY-1;
    end_x = ywd->URBact->SectX+1;
    end_y = ywd->URBact->SectY+1;
    for (y=start_y; y<=end_y; y++) {
        for (x=start_x; x<=end_x; x++) {
            /*** Sektor-Pos gültig? ***/
            if ((x>=0) && (x<ywd->MapSizeX) && (y>=0) && (y<ywd->MapSizeY)) {
                struct Cell *sec = &(ywd->CellArea[y * ywd->MapSizeX + x]);
                struct MinNode *nd;
                struct MinList *ls = &(sec->BactList);
                for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                    struct Bacterium *b = (struct Bacterium *) nd;
                    if ((b->MainState   != ACTION_CREATE) &&
                        (b->MainState   != ACTION_DEAD)   &&
                        (b->BactClassID != BCLID_YPAUFO)     &&
                        (b->Owner       != ywd->URBact->Owner))
                    {
                        return(TRUE);
                    };
                };
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserStationScrewed(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet, ob die UserStation in einem lebensbedrohlichen
**      Zustand ist.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    if (ywd->URBact->Energy < (ywd->URBact->Maximum/8)) {
        return(TRUE);
    } else {
        return(FALSE);
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_UserHasEscapePowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet, ob der User mindestens eine Escape-Powerstation
**      hat.
**      Geht alle Powerstations nach User-Powerstations durch,
**      und testet, ob die Entfernung dahin mindestens
**      4 Sektoren ist.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector &&
            (kw_array[i].sector->Owner == ywd->URBact->Owner))
        {
            WORD dx = abs(kw_array[i].x - ywd->URBact->SectX);
            WORD dy = abs(kw_array[i].y - ywd->URBact->SectY);
            if ((dx >= 4) || (dy >= 4)) {
                return(TRUE);
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_GroupsInFight(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet, ob eigene Squads in Kämpfe verwickelt sind,
**      bei denen es nicht nur um ein UFO geht. Ob der
**      Kampf kritisch ist, wird daran ermittelt, ob der
**      Commander selbst ein Nebenziel aufgenommen hat,
**      weil der ja (neuerdings) versucht, sich möglichst
**      nebenzielfrei zu halten.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<ywd->NumCmdrs; i++) {
        struct Bacterium *b = ywd->CmdrRemap[i];
        if (TARTYPE_BACTERIUM == b->SecTargetType) {
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserOwnsPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob der User irgendwelche Kraftwerke
**      besitzt.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector) {
            if (kw_array[i].sector->Owner == ywd->URBact->Owner) {
                return(TRUE);
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserSitsOnStrongestPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob der User auf seinem stärksten Kraftwerk
**      sitzt.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    ULONG max_kw = 0;
    struct KraftWerk *kw_array = ywd->KraftWerks;
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector &&
            (kw_array[i].sector->Owner == ywd->URBact->Owner))
        {
            if (kw_array[i].factor > kw_array[max_kw].factor) max_kw=i;
        };
    };
    if (ywd->URBact->Sector == kw_array[max_kw].sector) {
        return(TRUE);
    } else {
        return(FALSE);
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_PowerEfficiencyIsTooLow(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob die aktuelle Powerstation-Effizienz
**      zu niedrig ist.
**
**  CHANGED
**      05-Aug-97   floh    created
**      18-Aug-97   floh    + auf konservativere 0.9 runtergesetzt
*/
{
    if (ywd->RatioCache[ywd->URBact->Owner] < 0.9) {
        return(TRUE);
    } else {
        return(FALSE);
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_UserSeesEnemyPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob User ein feindliches Kraftwerk
**      sehen kann.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    struct KraftWerk *kw_array = ywd->KraftWerks;
    for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
        if (kw_array[i].sector) {
            struct Cell *sec = kw_array[i].sector;
            if ((sec->Owner != ywd->URBact->Owner) &&
                (sec->FootPrint & (1<<ywd->URBact->Owner)))
            {
                return(TRUE);
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserCanBuildPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob User momentan ein Kraftwerk bauen
**      kann.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    ULONG batt_energy;
    _get(ywd->UserRobo, YRA_BattVehicle, &batt_energy);
    for (i=0; i<SR.NumBuildings; i++) {
        struct BuildProto *bp = &(ywd->BP_Array[SR.BPRemap[i]]);
        if (BUILD_BASE_KRAFTWERK == bp->BaseType) {
            /*** reicht auch die Energie? ***/
            if (batt_energy >= bp->CEnergy) {
                return(TRUE);
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserCanSeeTechUpgrade(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob sich ein nicht eroberter Wunderstein
**      im Sichtbereich des User befindet.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<MAXNUM_WUNDERSTEINS; i++) {
        struct Wunderstein *g = &(ywd->gem[i]);
        struct Cell *sec = &(ywd->CellArea[g->sec_y * ywd->MapSizeX + g->sec_x]);
        if ((WTYPE_Wunderstein  == sec->WType)  &&
            (ywd->URBact->Owner != sec->Owner) &&
            ((1<<ywd->URBact->Owner) & sec->FootPrint))
        {
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserCanSeeBeamGate(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob User ein nicht erobertes Beamgate
**      sehen kann.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<ywd->Level->NumGates; i++) {
        struct Gate *gate = &(ywd->Level->Gate[i]);
        if ((WTYPE_ClosedGate == gate->sec->WType) &&
            (ywd->URBact->Owner != gate->sec->Owner) &&
            ((1<<ywd->URBact->Owner) & gate->sec->FootPrint))
        {
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_UserCanSeeKeysecOfOwnedBeamgate(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob User einen nicht eroberten Keysektor
**      eines eroberten Beamgates sehen kann.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<ywd->Level->NumGates; i++) {
        ULONG j;
        struct Gate *gate = &(ywd->Level->Gate[i]);
        for (j=0; j<gate->num_keysecs; j++) {
            struct KeySector *ksec = &(gate->keysec[j]);
            if ((ywd->URBact->Owner != ksec->sec->Owner) &&
                ((1<<ywd->URBact->Owner) & ksec->sec->FootPrint))
            {
                return(TRUE);
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_EnemyStationVisible(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob eine gegnerische Station sichtbar ist.
**
**  CHANGED
**      05-Aug-97   floh    created
**      22-Jun-98   floh    + macht jetzt ACTION_DEAD Test
*/
{
    struct MinList *ls = &(ywd->CmdList);
    struct MinNode *nd;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        if ((b != ywd->URBact) &&
            (b->BactClassID == BCLID_YPAROBO) &&
            (b->MainState   != ACTION_DEAD)   &&
            (b->Sector->FootPrint & (1<<ywd->URBact->Owner)))
        {
            return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_EnemyStationVisibleAndSitsNearPowerStation(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob eine gegnerische Station sichtbar ist,
**      und nahe eines Kraftwerks steht.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    struct MinList *ls = &(ywd->CmdList);
    struct MinNode *nd;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        if ((b != ywd->URBact) &&
            (b->BactClassID == BCLID_YPAROBO) &&
            (b->Sector->FootPrint & (1<<ywd->URBact->Owner)))
        {
            /*** feindliche Station ist sichtbar, ist sie ***/
            /*** in der Nähe eines ihrer Kraftwerke???    ***/
            ULONG i;
            struct KraftWerk *kw_array = ywd->KraftWerks;
            for (i=0; i<ywd->FirstFreeKraftWerk; i++) {
                struct Cell *sec = kw_array[i].sector;
                if ((sec) && (sec->Owner==b->Owner) &&
                    (sec->FootPrint & (1<<b->Owner)))
                {
                    LONG dx = abs(b->SectX - kw_array[i].x);
                    LONG dy = abs(b->SectY - kw_array[i].y);
                    if ((dx < 3) || (dy < 3)) {
                        return(TRUE);
                    };
                };
            };
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG yw_ToFewLandExplored(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob der erkundete Anteil an Land
**      im Verhältnis zur Gesamtsektor-Anzahl ausreichend ist
**      (< 90%).
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG num_sec    = ywd->MapSizeX * ywd->MapSizeY;
    struct Cell *sec = ywd->CellArea;
    ULONG i;
    ULONG num_expl = 0;
    FLOAT val;

    /*** wieviele Sektoren erkundet? ***/
    for (i=0; i<num_sec; i++) {
        if (sec->FootPrint & (1<<ywd->URBact->Owner)) num_expl++;
        sec++;
    };

    /*** Verhältnis zu Gesamtanzahl ***/
    num_sec = (ywd->MapSizeX-2) * (ywd->MapSizeY-2);
    val = ((FLOAT)num_expl) / ((FLOAT)num_sec);
    if (val < 0.75) {
        return(TRUE);
    } else {
        return(FALSE);
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_ToFewLandConquered(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert, ob im Verhältnis zum erkundeten
**      Land zuwenig Land erobert wurde.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG num_sec    = ywd->MapSizeX * ywd->MapSizeY;
    struct Cell *sec = ywd->CellArea;
    ULONG i;
    ULONG num_expl = 0;
    ULONG num_conq = ywd->SectorCount[ywd->URBact->Owner];
    FLOAT val;

    /*** wieviele Sektoren erkundet? ***/
    for (i=0; i<num_sec; i++) {
        if (sec->FootPrint & (1<<ywd->URBact->Owner)) num_expl++;
        sec++;
    };

    /*** Verhältnis zu erobert ***/
    if (num_conq > 0) val = ((FLOAT)num_expl) / ((FLOAT)num_conq);
    else              val = 0.0;
    if (val < 0.75) {
        return(TRUE);
    } else {
        return(FALSE);
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_ReadyToBeam(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet, ob alles zum Beamen bereit (mindestens ein
**      Beamgate offen).
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<ywd->Level->NumGates; i++) {
        struct Gate *gate = &(ywd->Level->Gate[i]);
        if (WTYPE_OpenedGate == gate->sec->WType) {
            return(TRUE);
        };
    };
    return(FALSE);
}

/*=================================================================**
**                                                                 **
**  ANALYZER ADVICE ROUTINEN                                       **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvEscapeBeam(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE1,"ADVICE #1: ESCAPE TO BEAMGATE"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvYourScrewed(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE2,"ADVICE #2: YOUR SCREWED"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvDefendHostStation(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE3,"ADVICE #3: DEFEND HOSTSTATION"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvSendReinforcementOrHelpByHand(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE4,"ADVICE #4: SEND REINFORMCEMENT OR HELP BY HAND"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvConquerSectorsForBetterEfficiency(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE5,"ADVICE #5: CONQUER SECTORS TO BUMP UP EFFICIENCY"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvBeamToStrongestPowerStation(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE6,"ADVICE #6: BEAM TO YOUR STRONGEST POWER STATION"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvConquerEnemyPowerStation(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE7,"ADVICE #7: CONQUER ENEMY POWER STATION"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvBuildPowerStation(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE8,"ADVICE #8: BUILD A POWER STATION"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvConquerTechUpgrade(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE9,"ADVICE #9: CONQUER TECH UPGRADE"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvConquerBeamGate(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE10,"ADVICE #10: CONQUER BEAM GATE"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvConquerKeysector(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE11,"ADVICE #11: CONQUER KEY SECTORS TO OPEN BEAM GATE"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvCollectForcesForPowerStationAttack(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE12,"ADVICE #12: COLLECT FORCES FOR POWER STATION ATTACK"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvCollectForcesForStationAttack(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE13,"ADVICE #13: COLLECT FORCES FOR ROBO ATTACK"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvExploreLand(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE14,"ADVICE #14: EXPLORE LAND"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvConquerAndStabilizeLand(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE15,"ADVICE #15: CONQUER AND STABILIZE LAND"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvPlaceForcesOnBeamGateAndBeamAway(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE16,"ADVICE #16: PLACE FORCES ONTO BEAM GATE AND LEAVE LEVEL"));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AdvNoProblem(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    return(ypa_GetStr(ywd->LocHandle,STR_GSTATE_ADVICE17,"ADVICE #17: NO PROBLEM IDENTIFIED"));
}

/*=================================================================**
**                                                                 **
**  ANALYZER ITSELF                                                **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
void yw_AnalyzeGameState(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Frontend-Routine für Gamestate-Analyzer, testet
**      schrittweise kritische Situationen durch, und
**      macht die kritischste + Lösungsvorschlag per
**      Logmessage bekannt.
**
**  CHANGED
**      05-Aug-97   floh    created
*/
{
    UBYTE *str = NULL;

    /*** Defensiv ***/
    if (yw_EnemyAttacksStation(ywd)) {
        if (yw_UserStationScrewed(ywd)) {
            if (yw_UserHasEscapePowerStation(ywd)) {
                str = yw_AdvEscapeBeam(ywd,str);
            } else {
                str = yw_AdvYourScrewed(ywd,str);
            };
        } else {
            str = yw_AdvDefendHostStation(ywd,str);
        };
    } else if (yw_GroupsInFight(ywd))
        str = yw_AdvSendReinforcementOrHelpByHand(ywd,str);

    /*** Energie ***/
    else if (yw_UserOwnsPowerStation(ywd) &&
             yw_UserSitsOnStrongestPowerStation(ywd) &&
             yw_PowerEfficiencyIsTooLow(ywd))
    {
        str = yw_AdvConquerSectorsForBetterEfficiency(ywd,str);
    } else if (yw_UserOwnsPowerStation(ywd) &&
              (!yw_UserSitsOnStrongestPowerStation(ywd)))
    {
        str = yw_AdvBeamToStrongestPowerStation(ywd,str);
    } else if (yw_UserSeesEnemyPowerStation(ywd))
        str = yw_AdvConquerEnemyPowerStation(ywd,str);
    else if ((!yw_UserOwnsPowerStation(ywd)) && 
             yw_UserCanBuildPowerStation(ywd)) 
    {
        str = yw_AdvBuildPowerStation(ywd,str);

    /*** Tech Upgrades ***/
    } else if (yw_UserCanSeeTechUpgrade(ywd))
        str = yw_AdvConquerTechUpgrade(ywd,str);
    else if (yw_UserCanSeeBeamGate(ywd))
        str = yw_AdvConquerBeamGate(ywd,str);
    else if (yw_UserCanSeeKeysecOfOwnedBeamgate(ywd))
        str = yw_AdvConquerKeysector(ywd,str);

    /*** Offensive ***/
    else if (yw_EnemyStationVisible(ywd)) {
        /*** gegnerische Station sichtbar, in Nähe eines Kraftwerks? ***/
        if (yw_EnemyStationVisibleAndSitsNearPowerStation(ywd)) {
            str = yw_AdvCollectForcesForPowerStationAttack(ywd,str);
        } else {
            str = yw_AdvCollectForcesForStationAttack(ywd,str);
        };
    } else if (yw_ToFewLandExplored(ywd))
        str = yw_AdvExploreLand(ywd,str);
    else if (yw_ToFewLandConquered(ywd))
        str = yw_AdvConquerAndStabilizeLand(ywd,str);
    else if (yw_ReadyToBeam(ywd))
        str = yw_AdvPlaceForcesOnBeamGateAndBeamAway(ywd,str);
    else
        str = yw_AdvNoProblem(ywd,str);

    /*** falls <str> gültig, druggen ***/
    if (str) {
        struct logmsg_msg lmm;
        lmm.bact = NULL;
        lmm.pri  = 100;
        lmm.msg  = str;
        lmm.code = LOGMSG_NOP;
        _methoda(ywd->world, YWM_LOGMSG, &lmm);
    };
}

