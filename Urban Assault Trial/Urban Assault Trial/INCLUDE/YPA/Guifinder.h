#ifndef YPA_GUIFINDER_H
#define YPA_GUIFINDER_H
/*
**  $Source: PRG:VFM/Include/ypa/guifinder.h,v $
**  $Revision: 38.6 $
**  $Date: 1998/01/06 14:22:29 $
**  $Locker:  $
**  $Author: floh $
**
**  Defs für Finder-Requester.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef YPA_GUILIST_H
#include "ypa/guilist.h"
#endif

/*-------------------------------------------------------------------
**  Weil ich immer damit rechnen muß, daß eine Bakterie von einem
**  Frame auf den nächsten verschwindet, kann ich nicht mit
**  direkten Pointern arbeiten, um irgendwelche Positionen etc.
**  aufzuheben.
**
**  Statt dessen arbeite ich mit einer Positions-Nummer. Diese
**  Nummer beschreibt eine Bakterie in der Geschwader-Struktur
**  nach folgendem Muster:
**      - Commander werden gezählt, solange sie nicht tot,
**        oder ein "Sonderelement" sind, welches im Finder nichts
**        zu suchen hat.
**      - Wenn das Geschwader "aufgepoppt" ist (und nur dann)
**        werden die Slaves des Geschwaders mitgezählt.
**
**  Auf diese Weise wird auch für jeden Frame die Remap-Tabelle
**  erzeugt. Diese enthält für jedes momentan sichtbare Item
**  den Bakterien-Pointer. Da diese Pointer nur für den aktuellen
**  Frame gültig sind, treten da keine Probleme auf.
*/
struct YPAFinder {
    struct YPAListReq l;    // eingebettete Listen-Requester-Struktur
    struct Bacterium *b_map[LV_MAXNUMITEMS];    // Remap-Tabelle mit Pointern
    ULONG flags;            // siehe unten

    LONG squad_num;         // ausgefüllt von yw_FRGetBactUnderMouse()
    LONG unit_num;          // ausgefüllt von yw_FRGetBactUnderMouse()

    /*** Drag'n'Drop-Info ***/
    LONG drag_squad_num;            // Geschwader-Nummer des Drag-Kandidaten
    LONG drag_unit_num;             // Einheiten-Nummer in Geschw., -1 ist Cmdr
    struct Bacterium *start_drag;   // ursprüngliche Drag-Bakterie...
    WORD orig_x, orig_y;            // Store-Koordinaten für Dragging
    WORD icon_x, icon_y;            // aktuelle Drag-Icon-Position
    WORD start_x,start_y;           // Start-X/Y-Position beim Draggen

    /*** Dynamic-Layout-Info ***/
    LONG type_icon_width;
    LONG leader_pix_off;
    LONG bunch_pix_off;
    LONG lborder_icon;  // Status-Balken: ab hier startet das Aktions-Icon
    LONG lborder_aggr;  // Status-Balken: ab hier startet die Aggr-Icon-Gruppe
};

/*** Defs für YPAFinder.flags ***/
#define FRF_Dragging    (1<<0)  // about to drag...
#define FRF_IsMultiDrag (1<<1)  // zusaetzlich gesetzt, falls ein Multi-Drag

/*** Aktions-Icon-Konstanten ***/
#define FR_ACTION_NONE      ('a')       // FIXME
#define FR_ACTION_WAIT      ('a')
#define FR_ACTION_GOTOPRIM  ('b')
#define FR_ACTION_GOTOSEC   ('c')
#define FR_ACTION_FIGHTPRIM ('d')
#define FR_ACTION_FIGHTSEC  ('e')
#define FR_ACTION_ESCAPE    ('f')

/*-----------------------------------------------------------------*/
#endif

