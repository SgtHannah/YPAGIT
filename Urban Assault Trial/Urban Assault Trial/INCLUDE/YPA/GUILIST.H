#ifndef YPA_GUILIST_H
#define YPA_GUILIST_H
/*
**  $Source: PRG:VFM/Include/ypa/guilist.h,v $
**  $Revision: 38.8 $
**  $Date: 1998/01/06 14:22:53 $
**  $Locker: floh $
**  $Author: floh $
**
**  Basis-Definitionen für die Listview-Requester in YPA.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef UTILITY_TAGITEM_H
#include "utility/tagitem.h"
#endif

#ifndef YPA_YPAGUI_H
#include "ypa/ypagui.h"
#endif

/*-------------------------------------------------------------------
**  Die YPAListReq Struktur erweitert die YPAReq Struktur und
**  dient selbst als Basis-Struktur für konkrete Listview-
**  Requester. In yw_listview.c sind dann die Stamm-Funktionen
**  definiert, die den größten Teil der Drecksarbeit erledigen.
**
**  Ein YPAListReq kann ein "echter" Standalone-Requester mit
**  eigenem Titlebar sein, oder ein primitives Lowlevel-Popup-Menü.
**
**  Ein Standalone-Requester besitzt einen Titlebar, kann
**  selbständig geschlossen werden, kann gedragged werden
**  und besitzt ein PopUp-Gadget rechts oben, und ein Icon :-)
**
**  Hinweis: Das gesamte Layout des Item-Blocks muß extern
**  erledigt werden, dabei muß beachtet werden, daß bei
**  resizeable Requestern die Item-Strings geclippt sein
**  können (und zwar gegen ActEntryWidth).
*/
struct YPAListReq {
    struct YPAReq Req;

    LONG ScrollTimer;       // für "Maus außerhalb Listview-Scroller"

    ULONG Flags;            // siehe unten...
    ULONG CloseChar;        // Teil von FONTID_ICON_NS/FONTID_ICON_PS!

    WORD NumEntries;        // Gesamt-Anzahl Entries in Liste
    WORD ShownEntries;      // Anzahl momentan angezeigter Entries
    WORD FirstShown;        // momentan erster angezeigter Entry
    WORD Selected;          // momentan selektierter Entry (-1 -> nix selected)
    WORD MaxShown;          // bestimmt MaxSize bei Resizeable
    WORD MinShown;          // bestimmt MinSize bei Resizeable
    WORD MouseItem;         // Item, über dem die Maus ist (-1, über keinem!)

    WORD EntryHeight;       // in Pixel
    WORD UpperVBorder;      // zusätzlicher Rand oben
    WORD LowerVBorder;      // zusätzlicher Rand unten

    WORD ActEntryWidth;     // redundant zu Req.req_cbox.rect.w
    WORD MinEntryWidth;     // nur, wenn Resizeable
    WORD MaxEntryWidth;     // nur, wenn Resizeable

    UBYTE TitleStr[64];     // Platz für Title-String
    UBYTE *Scroller;        // allokiert von yw_InitListReq()
    UBYTE *Itemblock;       // allokiert von yw_InitListReq()

    WORD RszX, RszY;        // Resize-Merker
    WORD ScrX, ScrY;        // Scrolling-Merker

    WORD ScrSize;           // diverse Scroller-Parameter
    WORD KnobSize;
    WORD KnobStart;
};

/*-------------------------------------------------------------------
**  Flag-Defs für YPAListReq.Flags
*/
#define LISTF_Enabled       (1<<0)  // momentan anzeigen/nicht anzeigen
#define LISTF_HasTitleBar   (1<<1)
#define LISTF_Resizeable    (1<<2)
#define LISTF_HasIcon       (1<<3)  // Requester hat "echtes" Icon
#define LISTF_Immediate     (1<<4)  // Immediate Input on

#define LISTF_Resizing      (1<<5)  // Resizing underway
#define LISTF_Scrolling     (1<<6)  // Scrolling underway
#define LISTF_Select        (1<<7)  // Selection underway...

#define LISTF_Static        (1<<8)  // keine Anpassung der Itemblock-Buttons
#define LISTF_NoScroll      (1<<9)  // erst anfangen zu scrollen wenn Maus
                                    // Menü-Inhalt berührte

#define LISTF_SelectDone    (1<<10) // Select Prozeß mit Mouse Up abgeschlossen
#define LISTF_KeyboardInput (1<<11) // Keyboard-Input erlaubt

/*-------------------------------------------------------------------
**  TagItem-Definitionen für yw_InitListView()
*/
#define LIST_Title          (TAG_USER+1)  // UBYTE *, NULL -> kein Titlebar
#define LIST_Resize         (TAG_USER+2)  // BOOL, Resizeable oder nicht
#define LIST_NumEntries     (TAG_USER+3)  // Anzahl Entries bei Init
#define LIST_ShownEntries   (TAG_USER+4)  // angezeigte Entries bei Init
#define LIST_FirstShown     (TAG_USER+5)  // erster angezeigter Entry
#define LIST_Selected       (TAG_USER+6)  // selektierter Entry bei Init
#define LIST_MaxShown       (TAG_USER+7)  // so viele Entries anzeigen, bevor
                                          // Scrollbar aktiviert wird
#define LIST_DoIcon         (TAG_USER+8)  // bekommt Requester ein Icon?
#define LIST_IconPos        (TAG_USER+9)  // nur, wenn GUILIST_DoIcon==TRUE
#define LIST_IconChar       (TAG_USER+10) // nur, wenn GUILIST_DoIcon==TRUE
#define LIST_EntryHeight    (TAG_USER+11) // in Pixel
#define LIST_EntryWidth     (TAG_USER+12) // Entry-Breite in Pixel
#define LIST_MinEntryWidth  (TAG_USER+13) // nur wenn resizeable
#define LIST_MaxEntryWidth  (TAG_USER+14) // nur wenn resizeable
#define LIST_Enabled        (TAG_USER+15) // initially enabled/disabled
#define LIST_VBorder        (TAG_USER+16) // optionaler Vertikal-Rand
#define LIST_ImmediateInput (TAG_USER+17) // Immediate Input Mode halt... :-)
#define LIST_MinShown       (TAG_USER+18)

#define LIST_StaticItems    (TAG_USER+19) // keine Anpassung der Itemblock-Buttons
#define LIST_CloseChar      (TAG_USER+20) // falls Titlebar, Char für CloseGadget
                                          // (FONTID_ICON_NS/FONTID_ICON_PS!)

#define LIST_UpperVBorder   (TAG_USER+21)
#define LIST_LowerVBorder   (TAG_USER+22)

#define LIST_KeyboardInput  (TAG_USER+23)   // akzeptiert Keyboard-Input
#define LIST_HasHelpGadget  (TAG_USER+24)   // default: TRUE

/*-------------------------------------------------------------------
**  Das Clip-Pointer-Array hat folgenden Aufbau:
*/
#define LV_NUMCLIPS         (2)

#define LV_CLIP_SCROLLER    (0) // vertikaler Scroller mit Resize Gadget
#define LV_CLIP_ITEMBLOCK   (1) // wird extern aufgebaut

/*-----------------------------------------------------------------*/
#define LV_NUM_STD_BTN      (8)     // Anzahl Standard-Buttons

/*-------------------------------------------------------------------
**  Button-Definitionen
**  ~~~~~~~~~~~~~~~~~~~
**  Dies ist das maximale Set an Standard-Buttons, die an einem
**  List-Requester gehängt werden können. Manche ListView-Requester
**  haben immer oder temporär einige dieser Buttons *nicht*. In
**  diesem Fall werden die Button-Rects auf 0,0,0,0 gesetzt,
**  sind also nicht anklickbar -> das vereinfacht eine Menge bei
**  der Auswertung.
*/
#define LV_MAXNUMITEMS      (32-LV_NUM_STD_BTN)     // 32 - 8 Standard-Buttons

#define LV_BTN_CLOSE        (0)
#define LV_BTN_DRAG         (1)
#define LV_BTN_SCROLLTOP    (2)
#define LV_BTN_SCROLLKNOB   (3)
#define LV_BTN_SCROLLBOTTOM (4)
#define LV_BTN_RESIZE       (5)
#define LV_BTN_LOWVBORDER   (6)
#define LV_BTN_HELP         (7)

/*-----------------------------------------------------------------*/
#endif
