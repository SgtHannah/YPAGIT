#ifndef YPA_YPAHISTORY_H
#define YPA_YPAHISTORY_H
/*
**  $Source: PRG:VFM/Include/ypa/ypahistory.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:26:37 $
**  $Locker:  $
**  $Author: floh $
**
**  ypahistory.h -- Definitionen für History-Buffer und Debriefing.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

/*-------------------------------------------------------------------
**  Die History-Struktur hält Statusinformationen und eine Liste
**  von History-Puffern, welche mit Hilfe der Methode YWM_NOTIFYEVENT
**  gefüllt wird. Das Debriefing wertet diese History-Informationen
**  dann aus und updated die PlayerStats-Strukturen.
*/
struct HistoryBuffer {
    struct MinNode nd;
    UBYTE *inst_begin;  // zeigt auf Start des Instruction-Puffers
    UBYTE *inst_end;    // zeigt auf 1. Byte nach Ende des Instruction-Puffer
};

struct HistoryHeader {
    struct MinList ls;      // List Of <struct HistoryBuffer>
    ULONG num_frames;       // Anzahl Frames
    ULONG time_stamp;       // TimeStamp des letzten Frames
    ULONG num_bufs;         // Anzahl Event-Puffer
    UBYTE *inst_ptr;        // aktueller "Instruction-Pointer"
    UBYTE *previnst_ptr;    // Ptr auf vorherige Instruktion
    UBYTE *endinst_ptr;     // Ende des aktuellen Instruction-Puffer/
};

#define YPAHIST_BUFSIZE (4096)

/*** Definition der History-Events ***/
#define YPAHIST_INVALID      (0)    // als Puffer-Terminator benutzt
#define YPAHIST_NEWFRAME     (1)    // <struct ypa_HistNewFrame>
#define YPAHIST_CONSEC       (2)    // <struct ypa_HistConSec>
#define YPAHIST_VHCLKILL     (3)    // <struct ypa_HistVhclKill>
#define YPAHIST_VHCLCREATE   (4)    // <struct ypa_HistVhclCreate>
#define YPAHIST_SQUADPOS     (5)    // <struct ypa_HistSquadPos>
#define YPAHIST_POWERSTATION (6)    // <struct ypa_HistConSec>
#define YPAHIST_TECHUPGRADE  (7)    // <struct ypa_HistTechUpgrade>

/*** ein Frame wird nur geschrieben, wenn innerhalb des Frames ***/
/*** Ereignisse stattfinden!!!                                 ***/
struct ypa_HistNewFrame {
    UBYTE cmd;              // YPAHIST_NEWFRAME
    ULONG time_stamp;       // Timestamp dieses Frames
};

/*** ein Sektor wurde erobert ***/
struct ypa_HistConSec {
    UBYTE cmd;              // YPAHIST_CONSEC/YPAHIST_POWERSTATION/YPAHIST_TECHUPGRADE
    UBYTE sec_x;
    UBYTE sec_y;
    UBYTE new_owner;
};

/*** ein Techupgrade wurde erobert... ***/
struct ypa_HistTechUpgrade {
    UBYTE cmd;              // YPAHIST_TECHUPGRADE
    UBYTE sec_x;
    UBYTE sec_y;
    UBYTE old_owner;
    UBYTE new_owner;
    UBYTE type;             // siehe unten
    WORD  vp_num;           // Prototype-Nummer des betroffenen Vehicles
    WORD  wp_num;
    WORD  bp_num;
};
#define YPAHIST_TECHTYPE_NONE               (0)    
#define YPAHIST_TECHTYPE_WEAPON             (1)
#define YPAHIST_TECHTYPE_ARMOR              (2)
#define YPAHIST_TECHTYPE_VEHICLE            (3)
#define YPAHIST_TECHTYPE_BUILDING           (4)
#define YPAHIST_TECHTYPE_RADAR              (5)
#define YPAHIST_TECHTYPE_BUILDANDVEHICLE    (6)
#define YPAHIST_TECHTYPE_GENERIC            (7)

/*
**  Ein Vehicle wurde gekillt!
**  ==========================
**  Aufbau des Owner-Felds:
**      Bit 7    -> 1, wenn Killer der User war
**      Bit 6    -> 1, wenn Opfer der User war
**      Bit 5..3 -> Owner-ID des Killers
**      Bit 2..0 -> Owner-ID des Opfers
**
**  Aufbau des vp-Feldes:
**      Bit 15    -> gesetzt, wenn das Opfer ein Robo war
**      Bit 14..0 -> die normale VehicleProto des Opfers
*/
struct ypa_HistVhclKill {
    UBYTE cmd;              // YPAHIST_VHCLKILL
    UBYTE owners;           // siehe oben
    UWORD vp;               // siehe oben
    UBYTE pos_x,pos_z;      // (pos) / ((sizeof(World)/256);
};

/*
**  Ein Vehicle wurde erzeugt!
**  ==========================
*/
struct ypa_HistVhclCreate {
    UBYTE cmd;              // YPAHIST_VHCLCREATE
    UBYTE owner;
    UWORD vp;
    UBYTE pos_x,pos_z;
};

/*
**  Position eines Squads
**  =====================
**  Aufbau des <own_id> Felds
**      Bit 15..13  - Owner ID des Squads
**      Bit 12..0   - maskierte CommandID des Squads
*/

struct ypa_HistSquadPos {
    UBYTE cmd;              // YPAHIST_SQUADPOS
    UWORD own_id;           // siehe oben
    UBYTE pos_x,pos_z;
};

/*-----------------------------------------------------------------*/
#endif
