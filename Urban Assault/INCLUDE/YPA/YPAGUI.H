#ifndef YPA_YPAGUI_H
#define YPA_YPAGUI_H
/*
**  $Source: PRG:VFM/Include/ypa/ypagui.h,v $
**  $Revision: 38.16 $
**  $Date: 1998/01/06 14:25:52 $
**  $Locker: floh $
**  $Author: floh $
**
**  Hier sind die Grundlagen der YPA-GUI-Elemente definiert,
**  als "visuelle" Erweiterung der input.engine ClickBoxen.
**
**  (C) Copyright 1995,1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INPUT_INPUT_H
#include "input/input.h"
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"
#endif

#ifndef YPA_YPAVEHICLES_H
#include "ypa/ypavehicles.h"
#endif

/*-------------------------------------------------------------------
**  Die Requester-Struktur ist eine Erweiterung der Clickbox-
**  Struktur und wiederum nur Stamm-Struktur für alle konkreten
**  Requester.
*/
struct YPAReq {
    struct MinNode req_node;    // zum Einklinken in Welt-Object

    ULONG flags;        // siehe unten...

    struct ClickBox req_cbox;   // eingebettete Requester-ClickBox-Struktur
    struct ClickBox icon_cbox;  // eingebettete Icon-ClickBox-Struktur

    UBYTE *icon_string;     // direkt für _DrawText()
    UBYTE *req_string;      // direkt für _DrawText()

    UBYTE **req_clips;      // für _DrawText(), Icon hat keine Clips

    void (*post_draw)(struct ypaworld_data *);  // Post-Draw-Hook
};

/*-------------------------------------------------------------------
**  Flagsbits für YPAReq.flags
*/
#define REQF_Iconified  (1<<0)      // ist gerade Iconifiziert
#define REQF_InList     (1<<1)      // ist in Req-Liste eingebunden
#define REQF_HasIcon    (1<<2)      // initialisieren!!!
#define REQF_HasDragBar (1<<3)      // initialisieren!!!
#define REQF_HasCloseGadget (1<<4)  // initialisieren!!!
#define REQF_Closed     (1<<5)      // Alternative!! zu REQF_Iconified,
                                    // die verwendet wird, falls 
                                    // REQF_HasIcon NICHT gesetzt ist!
#define REQF_CloseDown  (1<<6)      // Close-Gadget momentan down
#define REQF_HelpDown   (1<<7)      // Help-Gadget momentan down
#define REQF_HasHelpGadget  (1<<8)  // initialisieren!!!

/*-------------------------------------------------------------------
**  HotKeys
*/
#define HOTKEY_ORDER        (0x80)  // Mode: Order              // OBSOLETE ???  
#define HOTKEY_FIGHT        (0x81)  // Mode: Fight
#define HOTKEY_NEW          (0x82)  // Mode: New
#define HOTKEY_ADD          (0x83)  // Mode: Add
#define HOTKEY_CONTROL      (0x84)  // Mode: Control
#define HOTKEY_BUILD        (0x85)  // Mode: Build              // OBSOLETE
#define HOTKEY_PANIC        (0x86)  // Mode: Panic
#define HOTKEY_AUTOPILOT    (0x87)  // Mode: Autopilot
#define HOTKEY_MAP          (0x88)  // Window: Map open/close
#define HOTKEY_FINDER       (0x89)  // Window: Finder open/close
#define HOTKEY_MAP_LAND     (0x8a)  // Map: Landscape Layer
#define HOTKEY_MAP_OWNER    (0x8b)  // Map: Owner Layer
#define HOTKEY_MAP_HEIGHT   (0x8c)  // Map: Height Layer
#define HOTKEY_MAP_NOLOCK   (0x8d)  // Map: No Lock
#define HOTKEY_MAP_LOCKVWR  (0x8e)  // Map: Lock On Viewer
#define HOTKEY_MAP_LOCKSLC  (0x8f)  // Map: Lock On Selected
#define HOTKEY_MAP_ZOOMIN   (0x90)  // Map: Zoom In
#define HOTKEY_MAP_ZOOMOUT  (0x91)  // Map: Zoom Out
#define HOTKEY_MAP_MINIMIZE (0x92)  // Map: Minimize
#define HOTKEY_MAP_MAXIMIZE (0x93)  // Map: Maximize
#define HOTKEY_NEXTCMDR     (0x94)  // Action: Next Commander
#define HOTKEY_TOROBO       (0x95)  // Action: To Robo
#define HOTKEY_NEXTUNIT     (0x96)  // Action: Next Unit
#define HOTKEY_TOCMDR       (0x97)  // Action: To Commander
#define HOTKEY_QUIT         (0x98)  // Action: Quit
#define HOTKEY_HUD          (0x99)  // Action: Hud an/aus
#define HOTKEY_CYCLE        (0x9a)  // Window: Cycle
#define HOTKEY_LOGWIN       (0x9b)  // Window: LogWindow open/close
#define HOTKEY_NEXTITEM     (0x9c)  // SubMenu: Next Item
#define HOTKEY_PREVITEM     (0x9d)  // SubMenu: Prev Item
#define HOTKEY_ENERGY       (0x9e)  // Window: Energy open/close
#define HOTKEY_CTRL2LM      (0x9f)  // Action: Control-2-Last-Message
#define HOTKEY_PAUSE        (0xa0)  // expliziter Pausen-Hotkey
#define HOTKEY_TOUSER       (0xa1)  // Message zum Owner 1
#define HOTKEY_TOKYTERNESER (0xa2)  // Message zum Owner 6
#define HOTKEY_TOMYKONIER   (0xa3)  // Message zum Owner 3
#define HOTKEY_TOTAERKASTEN (0xa4)  // Message zum Owner 4
#define HOTKEY_TOALL        (0xa5)  // Message für alle
#define HOTKEY_AGGR1        (0xa6)  // Schaltet auf erste Aggressionsstufe
#define HOTKEY_AGGR2        (0xa7)  // Schaltet auf zweite Aggressionsstufe
#define HOTKEY_AGGR3        (0xa8)  // Schaltet auf dritte  Aggressionsstufe
#define HOTKEY_AGGR4        (0xa9)  // Schaltet auf vierte Aggressionsstufe
#define HOTKEY_AGGR5        (0xaa)  // Schaltet auf fünfte Aggressionsstufe
#define HOTKEY_HELP         (0xab)  // ruft Online-Hilfe auf
#define HOTKEY_LASTOCCUPIED  (0xac)  // gehe zurueck in zuletzt besetztes Vehicle
#define HOTKEY_MAKECOMMANDER (0xad)  // mache aktuelles Vehicle zum Commander
#define HOTKEY_ANALYZER     (0xae)  // Situation Analyzer Hotkey

/*-------------------------------------------------------------------
**      Sachen, die im Create-Menue stehen
*/
struct YPAThing {
    UBYTE type;             // see below
    UBYTE index;            // Index in das respektive Array
};

#define YPATHING_NONE           (0)
#define YPATHING_VEHICLE        (1)
#define YPATHING_BUILDING       (2)

/*-------------------------------------------------------------------
**  Der Status-Requester ist der StatusBar am unteren
**  Screen-Rand. Er ist die Zentrale des gesamten GUIs...
*/
struct YPAStatusReq {

    struct YPAReq req;

    ULONG EnabledModes;     // momentan erlaubte Modi -> gesetzte Bits
    ULONG ActiveMode;       // momentan aktiver Modus -> gesetztes Bit

    ULONG EnabledReqs;      // momentan erlaubte Reqs und Menüs
    ULONG OpenReqs;         // momentan offene Reqs und Menüs

    UBYTE WPRemap[256];     // Weapon2Menu-Remap-Tabelle
    UBYTE VPRemap[256];     // Vehicle2Menu-Remap-Tabelle
    UBYTE BPRemap[256];     // Building2Menu-Remap-Tabelle
    struct YPAThing ThingRemap[512];        // kombinierte Vehicle/Building-Remap-Tabelle

    #ifdef YPA_DESIGNMODE
    UBYTE SecRemap[256];    // Sector2Menu-Remap-Tabelle
    ULONG NumSectors;
    LONG  ActSector;
    LONG  ActOwner;         // 0..7
    LONG  ActHeight;        // 0=Higher, 1=Lower
    #endif

    ULONG NumWeapons;       // Anzahl Flaks am Robo...
    ULONG NumVehicles;      // Anzahl Vehicle-Typen, die der Robo bauen kann
    ULONG NumBuildings;     // Anzahl Gebäude-Typen, die der Robo bauen kann
    ULONG NumThings;        // kombinierte Anzahl Vehicles/Buildings

    LONG ActVehicle;        // Index in Weapon-Remap-Tabelle!
    LONG ActWeapon;         // Index in Vehicle-Remap-Tabelle!
    LONG ActBuilding;       // Index in Building-Remap-Tabelle!
    LONG ActThing;

    /*** Dynamic Layout Stuff ***/
    LONG GroupSpace;        // Platz zwischen Button-Gruppen
    LONG SubModeW;          // Breite des SubModeListviews
    LONG ModeStart;         // Start der Mode-Icon-Gruppe
    LONG WinStart;          // Start der Window-Icon-Gruppe
    LONG NavStart;          // Start der Nav-Icon-Gruppe
    LONG QuitStart;         // Start der Quit-Icon-Gruppe
    LONG NumLines;          // Anzahl Zeilen im StatusBar (für Designer)
    LONG LBarStart;         // Pixel Start des Lower Status Bar (für Designer)

    ULONG DownFlags;        // (1<<Button) für niedergedrückte Btns
    ULONG DisableFlags;     // (1<<Button) für disablete Btns
};

/*** Modus-Flags ***/
#define STAT_MODEF_ORDER        (1<<0)      // Order Modus
#define STAT_MODEF_NEW          (1<<1)      // New Geschwader Modus
#define STAT_MODEF_ADD          (1<<2)      // Add Vehicle Modus
#define STAT_MODEF_CONTROL      (1<<3)      // Control Modus
#define STAT_MODEF_BUILD        (1<<4)      // Bauen Modus
#define STAT_MODEF_AUTOPILOT    (1<<5)      // Autopilot für Robo

#define STAT_MODEF_SETSECTOR    (1<<6)      // Designer: Sektor setzen
#define STAT_MODEF_SETHEIGHT    (1<<7)      // Designer: Höhe setzen
#define STAT_MODEF_SETOWNER     (1<<8)      // Designer: Owner setzen

#ifdef YPA_CUTTERMODE
#define STAT_MODEF_FREESTATIV   (1<<10)
#define STAT_MODEF_LINKSTATIV   (1<<11)
#define STAT_MODEF_ONBOARD1     (1<<12)
#define STAT_MODEF_ONBOARD2     (1<<13)
#define STAT_MODEF_PLAYER       (1<<14)
#endif

/*** Req-Flags ***/
#define STAT_REQF_STATUS    (1<<0)      // der Status-Req itself

/*** Button-IDs ***/
#define STAT_NUMBUTTONS         (11)
#define STAT_LBAR_NUMBUTTONS    (11)
#define STAT_HBAR_NUMBUTTONS    (0)

#ifdef YPA_DESIGNMODE
#undef STAT_NUMBUTTONS
#undef STAT_LBAR_NUMBUTTUNS
#undef STAT_HBAR_NUMBUTTONS
#define STAT_NUMBUTTONS         (14)
#define STAT_LBAR_NUMBUTTONS    (11)
#define STAT_HBAR_NUMBUTTONS    (3)
#endif
#ifdef YPA_CUTTERMODE
#undef STAT_NUMBUTTONS
#undef STAT_LBAR_NUMBUTTUNS
#undef STAT_HBAR_NUMBUTTONS
#define STAT_NUMBUTTONS         (14)
#define STAT_LBAR_NUMBUTTONS    (11)
#define STAT_HBAR_NUMBUTTONS    (3)
#endif

#define STATBTN_NEW         (0)
#define STATBTN_CONTROL     (1)
#define STATBTN_MAP         (2)     // Window-Gruppe
#define STATBTN_FINDER      (3)
#define STATBTN_TOROBO      (4)    // Geschwader-Nav-Gruppe
#define STATBTN_TOCOM       (5)
#define STATBTN_NEXTUNIT    (6)    // + Next Flak im Robo
#define STATBTN_NEXTCOM     (7)
#define STATBTN_HELP        (8)
#define STATBTN_ONLINEHELP  (9)
#define STATBTN_QUIT        (10)

#ifdef YPA_DESIGNMODE
#define STATBTN_SETSECTOR   (11)
#define STATBTN_SETOWNER    (12)
#define STATBTN_SETHEIGHT   (13)
#endif
#ifdef YPA_CUTTERMODE
#define STATBTN_PLAYER      (11)
#define STATBTN_FREESTATIV  (12)
#define STATBTN_LINKSTATIV  (13)
#endif

/*** Misc Consts ***/
#define STAT_HEIGHT         (8)     // OBSOLETE: use ywd->FontH

#define STAT_UP_FONT        (FONTID_MENUGRAY)
#define STAT_DOWN_FONT      (FONTID_MENUDOWN)

#define STAT_MENUUP_FONT    (FONTID_DEFAULT)
#define STAT_MENUDOWN_FONT  (FONTID_MENUDOWN)

/*-------------------------------------------------------------------
**  Der Energie-Balken ist sozusagen der obere Statusbalken...
*/
struct YPAEnergyBar {
    struct YPAReq req;

    /*** Layout-Variablen ***/
    ULONG bar_height;       // Höhe des Balkens in Pixel
    ULONG bar_width;        // Gesamt-Breite eines Subbalkens
    ULONG bar_start;
    ULONG bar_space;        // Platz zwischen den Subbalken
    ULONG icon_width;       // Breite des Balken-Icons
    ULONG fuel_bar_start;   // relativ zu rechter Rand Icon
    ULONG fuel_bar_width;
};

#define EB_NUMBUTTONS   (4)

#define EBBTN_RELOAD    (0)
#define EBBTN_SYSTEM    (1)
#define EBBTN_VEHICLE   (2)
#define EBBTN_BEAM      (3)

/*-------------------------------------------------------------------
**  Beschreibung einer Spalte für yw_gui.c/yw_BuildColumnItem()
*/
struct ypa_ColumnItem {
    UBYTE *string;          // Text im Item
    ULONG width;            // Breite des Items
    UBYTE font_id;          // Font ID des Items
    UBYTE space_chr;        // Zeichen zum Auffüllen
    UBYTE prefix_chr;       // optionales Start-Zeichen
    UBYTE postfix_chr;      // optionales End-Zeichen
    UWORD flags;            // siehe unten
};

#define YPACOLF_DOPREFIX    (1<<0)
#define YPACOLF_DOPOSTFIX   (1<<1)
#define YPACOLF_ALIGNLEFT   (1<<2)
#define YPACOLF_ALIGNRIGHT  (1<<3)
#define YPACOLF_ALIGNCENTER (1<<4)
#define YPACOLF_TEXT        (1<<5)  // setzen, wenn Text Only Spalte! (mixed nicht erlaubt)

/*-----------------------------------------------------------------*/
#endif

