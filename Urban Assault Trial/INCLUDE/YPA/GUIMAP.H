#ifndef YPA_GUIMAP_H
#define YPA_GUIMAP_H
/*
**  $Source: PRG:VFM/Include/ypa/guimap.h,v $
**  $Revision: 38.10 $
**  $Date: 1998/01/06 14:23:18 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für 2D-Map-Requester.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef BITMAP_RASTERCLASS_H
#include "bitmap/rasterclass.h"
#endif

#ifndef YPA_YPAGUI_H
#include "ypa/ypagui.h"
#endif

struct ScrollerInfo {
    UWORD size;         // aktuelle Größe des Scrollers in Pixel
    UWORD knoboffset;   // Pixel-Start-Offset des Knobs
    UWORD knobsize;     // Pixel-Größe des Knobs
};

/*-------------------------------------------------------------------
**  Die eigentliche Map-Requester-Struktur, die alles
**  zusammenhält.
*/
struct YPAMapReq {
    struct YPAReq req;      // eingebettete Requester-Struktur

    struct ScrollerInfo xscroll;
    struct ScrollerInfo yscroll;

    FLOAT midx,midz;        // aktueller Mittelpunkt der Karte
    FLOAT x_aspect;         // Größe eines Pixels in X-Welt-Koordinaten
    FLOAT y_aspect;         // Größe eines Pixel in Y-Welt-Koordinaten

    ULONG flags;        // siehe unten
    UBYTE layers;       // alle aktiven Layer, zusammenge-oder-t...
    UBYTE lock_mode;    // aktueller Lock-Mode (siehe unten)
    BYTE  zoom;         // aktuelle Zoom-Stufe, je nach Map-Mode
    UBYTE footprint;    // Footprint-Maske, für Sektor sichtbar/unsichtbar

    LONG ps_x,ps_y;             // Pixel-Offset sicht. Bereich in "Map-Bitmap"
    LONG pw_x,pw_y;             // Breite sichtbarer Map-Bereich in Pixel
    LONG topleft_x,topleft_y;   // Position Map in Display

    struct ClickRect zip_zap;       // Austausch-Size für Zip
    WORD min_width,min_height;      // Minimale Ausdehnung
    WORD max_width,max_height;      // Maximale Ausdehnung

    WORD rsz_x;     // Resize-X-Merker
    WORD rsz_y;     // Resize-Y-Merker
    WORD scr_x;     // Scroll-X-Merker
    WORD scr_y;     // Scroll-Y-Merker

    FLOAT scr_oldmidx;   // Mid-Value bei Scroll-Start
    FLOAT scr_oldmidz;

    /*** Dynamic Layout Zeug ***/
    LONG MinKnobSize;   // Minimale Scroller-Knob-Größe
    LONG ButtonW;       // Breite eines Map-Buttons
    LONG ButtonH;       // Höhe eines Map-Buttons
    LONG ButtonSpace;   // Platz vom linken Rand zur Button-Leiste
    LONG ZipW;          // Breite des Size-Zip-Gadgets rechter Rand
    LONG BorTopH;       // Höhe oberer Rand
    LONG BorBottomH;    // Höhe unterer Rand
    LONG BorLeftW;      // Breite linker Rand
    LONG BorRightW;     // Breite rechter Rand
    LONG BorHori;       // BorLeftW + BorRightW
    LONG BorVert;       // BorTopH + BorBottomH

    /*** Radar-Data ***/
    FLOAT r_x0,r_y0,r_x1,r_y1;  // Display-FLOAT-Koords für Radar
    struct flt_m3x3 r_dir;      // nur m11,m13,m31,m33 als norm. 2D-Vecs!

    /*** DragSelect (gültig, wenn MAPF_DRAGSELECT gesetzt) ***/
    struct rast_intrect drag_scr;   // Dragbox in Screen-Koords
    struct rast_rect drag_world;    // Dragbox in Welt-Koords
};

/*** YPAMapReq.layers ***/
#define MAP_LAYER_LANDSCAPE     (1<<0)  // Landscape-Layer
#define MAP_LAYER_OWNER         (1<<1)  // Owner-Layer
#define MAP_LAYER_HEIGHT        (1<<2)  // aka Advanced Info Layer!

/*** YPAMapReq.lock_mode ***/
#define MAP_LOCK_NONE     (0) // kein Locking
#define MAP_LOCK_VIEWER   (1) // Viewer-Lock

/*** YPAMapReq.flags ***/
#define MAPF_RESIZE     (1<<0)  // Resizing underway
#define MAPF_SCROLLX    (1<<1)  // X-Scrolling underway
#define MAPF_SCROLLY    (1<<2)  // Y-Scrolling underway
#define MAPF_RADAR      (1<<3)  // Radar prinzipiell sichtbar
#define MAPF_SCROLL     (1<<4)  // Double-Scroll underway
#define MAPF_ZIP_DOWN   (1<<5)  // Zip-Button down
#define MAPF_ZOOMIN_DOWN    (1<<6)
#define MAPF_ZOOMOUT_DOWN   (1<<7)
#define MAPF_RADAR_MODE     (1<<8)  // Flag für Radar-Render-Routinen
#define MAPF_DRAGGING       (1<<9)  // Drag-Selektion underway

/*-------------------------------------------------------------------
**  Default, Minimal- und Maximal-Größe des Requesters
*/
/*-------------------------------------------------------------------
**  Zoom-Controller-Konstanten
*/
#define MAP_ZOOM_CORRECT (0)    // nur Korrektur
#define MAP_ZOOM_IN      (1)    // reinzoomen
#define MAP_ZOOM_OUT     (2)    // rauszoomen

/*-------------------------------------------------------------------
** Button-Konstanten
*/
#define MAP_NUMBUTTONS          (18)

#define MAPBTN_ICONIFY          (0)
#define MAPBTN_DRAGBAR          (1)
#define MAPBTN_HELP             (2)
#define MAPBTN_LANDLAYER        (3)
#define MAPBTN_OWNERLAYER       (4)
#define MAPBTN_HEIGHTLAYER      (5)
#define MAPBTN_VIEWERLOCK       (6)
#define MAPBTN_ZOOMIN           (7)
#define MAPBTN_ZOOMOUT          (8)
#define MAPBTN_ZIP              (9)

#define MAPBTN_YSCROLLTOP       (10)
#define MAPBTN_YSCROLLKNOB      (11)
#define MAPBTN_YSCROLLBOTTOM    (12)
#define MAPBTN_XSCROLLLEFT      (13)
#define MAPBTN_XSCROLLKNOB      (14)
#define MAPBTN_XSCROLLRIGHT     (15)
#define MAPBTN_RESIZE           (16)
#define MAPBTN_MAPINTERIOR      (17)

/*-- Richtungs-Vektor-Farben --------------------------------------*/
#define MAP_VECPEN_DIRECTION    (9)     // normale Fahrtrichtung
#define MAP_VECPEN_PRIMTARGET   (253)   // Vektor zum Primary Target
#define MAP_VECPEN_SECTARGET    (254)   // Vektor zum Secondary Target
#define MAP_VECPEN_COMMANDER    (255)   // Richtung zum Vorgesetzten
#define MAP_VECPEN_DRAGBOX      (253)   // Farbe der Select-DragBox

#define MAP_HI_VECPEN_DIRECTION     (255)
#define MAP_HI_VECPEN_PRIMTARGET    ((0<<16)|(168<<8)|(255))
#define MAP_HI_VECPEN_SECTARGET     ((245<<16)|(145<<8)|(0))
#define MAP_HI_VECPEN_COMMANDER     ((0<<16)|(84<<8)|(177))
#define MAP_HI_VECPEN_DRAGBOX       ((0<<16)|(168<<8)|(255))

#define MAP_HI_PEN_OWNER1   ((0<<16)|(110<<8)|(194<<0))
#define MAP_HI_PEN_OWNER2   ((0<<16)|(154<<8)|(0<<0))
#define MAP_HI_PEN_OWNER3   ((232<<16)|(232<<8)|(232<<0))
#define MAP_HI_PEN_OWNER4   ((255<<16)|(171<<8)|(28<<0))
#define MAP_HI_PEN_OWNER5   ((116<<16)|(116<<8)|(116<<0))
#define MAP_HI_PEN_OWNER6   ((212<<16)|(0<<8)|(0<<0))
#define MAP_HI_PEN_OWNER7   ((0<<16)|(0<<8)|(0<<0))

/*-- Char-Nummern für Vehikel-Typen -------------------------------*/
#define MAP_FONT_ROBO       (FONTID_MAPROBO)
#define MAP_CHAR_ROBO_HUGE		(24)
#define MAP_CHAR_ROBO_BIG       (32)
#define MAP_CHAR_ROBO_MEDIUM    (40)
#define MAP_CHAR_ROBO_SMALL     (48)

#define MAP_CHAR_RSEL_HUGE		(16)
#define MAP_CHAR_RSEL_BIG		(17)
#define MAP_CHAR_RSEL_MEDIUM	(18)
#define MAP_CHAR_RSEL_SMALL		(19)

#define MAP_CHAR_RPOSSEL_HUGE	(20)
#define MAP_CHAR_RPOSSEL_BIG	(21)
#define MAP_CHAR_RPOSSEL_MEDIUM	(22)
#define MAP_CHAR_RPOSSEL_SMALL	(23)

#define MAP_CHAR_WEAPON     (1)     // 1..8
#define MAP_CHAR_HELI       (MAP_CHAR_WEAPON + 8)
#define MAP_CHAR_PLANE      (MAP_CHAR_HELI   + 8)
#define MAP_CHAR_TANK       (MAP_CHAR_PLANE  + 8)
#define MAP_CHAR_UFO        (MAP_CHAR_TANK   + 8)
#define MAP_CHAR_FLAK       (MAP_CHAR_UFO    + 8)

#define MAP_CURSOR_SELECTED     (132)
#define MAP_CURSOR_POSSEL       (133)   // Possibly Selected User Squad
#define MAP_CURSOR_POSTAR       (134)   // Possibly Targeted Enemy Vehicle
#define MAP_CURSOR_LOCATION     (135)   // allgemeiner Mitschleif-Cursor unter Maus
#define MAP_CURSOR_GOTO         (136)   // Goto-Cursor, wenn Ziel auf eigenem Gebiet
#define MAP_CURSOR_VIEWER       (137)   // blinkender Viewer-Cursor
#define MAP_CURSOR_LASTMESSAGE  (138)   // blinkender Last-Message-Indikator

/*-----------------------------------------------------------------*/
#endif



