#ifndef YPA_YPAPLAYERSTATS_H
#define YPA_YPAPLAYERSTATS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypaplayerstats.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:27:45 $
**  $Locker:  $
**  $Author: floh $
**
**  ypaplayerstats.h -- Strukturen für Playerstatistiks.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Dies sind die Player Stats für jeweils einen Player.
**  Die Stats existieren jeweils in einer "lokalen"
**  Version (nur aktueller Level) und in einer
**  globalen Version, welche über die gesamte
**  Playtime des Players hochgezählt wird.
*/
struct ypa_PlayerStats {
    ULONG Kills;        // Kills
    ULONG UserKills;    // Kills durch User
    ULONG Time;         // Playing Time
    ULONG SecCons;      // Sectors Conquered
    ULONG Score;        // Score dieses Players
    ULONG Power;        // PowerStations erobert/gebaut
    ULONG Techs;        // Techupgrades erobert
};

/*-----------------------------------------------------------------*/
#endif

