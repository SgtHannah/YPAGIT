#ifndef INPUT_CLICKBOX_H
#define INPUT_CLICKBOX_H
/*
**  $Source: PRG:VFM/Include/input/clickbox.h,v $
**  $Revision: 38.4 $
**  $Date: 1996/03/21 23:34:05 $
**  $Locker: floh $
**  $Author: floh $
**
**  Struktur-Definitionen für anklickbare Bereiche, die die
**  Input-Engine gegen Mouse-Clicks testen kann.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

/*-------------------------------------------------------------------
**  Eine Click-Box-Struktur besteht aus einer 2-stufigen
**  Hierarchie. An der Spitze steht jeweils eine <struct ClickBox>,
**  die ein Rechteck definiert, in das beliebig viele
**  <struct ClickButton>'s *eingebettet* sind. Die ClickBox
**  entspricht also etwa einem Requester/Window, die ClickButtons
**  sind die Gadgets. Auf der input.engine Ebene sind diesen
**  Strukturen allerdings keinerlei visuelle Attribute zugeordnet.
**
**  Das gesamte GUI-Handling ist polling-orientiert, nicht
**  event-orientiert, wie "normale" GUIs, weil sich auch das
**  GUI-Handling der Framerate unterordnen muß, die den
**  Herzschlag der Applikation letztenendes definiert.
**
**  How it works:
**  ~~~~~~~~~~~~~
**  Die input.engine kann eine beliebig lange dynamische Liste
**  von ClickBox-Strukturen verwalten. Jede dieser Boxen hat eine
**  X/Y-Position und eine X/Y-Ausdehnung.
**
**  Die ClickBox-Strukturen sind so sortiert, daß die 'oberste'
**  sichtbare CLickbox-Struktur zuerst in der Liste kommt,
**  die unterste im ClickBox-Stapel zuletzt. Damit können
**  sich die ClickBoxen auch gegenseitig überdecken, ohne daß
**  es zu Zuordnungs-Problemen kommt. Nicht eindeutige Maus-Klicks
**  bekommt jeweils die "oberste" Clickbox, die zuerst in der
**  Liste steht, zugeordnet.
**
**  Fand ein Mausklick innerhalb einer Klickbox statt, werden die
**  eingebetteten ClickButtons auf einen Treffer untersucht.
**
**  Die Abfrage von GUI-Mouseklicks erfolgt ganz normal innerhalb
**  _GetInput() (dazu muß die input.engine allerdings in den
**  GUI-Modus gesetzt worden sein -> siehe <input/input.h>).
**  Die Standard-Struktur VFMInput besitzt eine eingebettete
**  <struct ClickInfo>, in der alle GUI-relevanten Informationen
**  zu finden sind.
**-----------------------------------------------------------------*/

/*** Flags für _AddClickBox() ***/
#define IE_CBX_ADDHEAD  (0)
#define IE_CBX_ADDTAIL  (1)

/*** Struktur-Definitionen ***/
struct ClickRect {
    WORD x,y;          // x,y-Position, Nullpunkt links oben
    WORD w,h;          // Breite, Höhe.
};

struct ClickButton {
    struct ClickRect rect;      // Position *relativ* zu ClickBox-Position!
};

#define MAXNUM_CLICKBUTTONS (48)
struct ClickBox {
    struct MinNode node;            // zum Einklinken in eine Liste
    struct ClickRect rect;          // der Klick-Bereich
    ULONG num_buttons;              // Anzahl Buttons in ClickButtons-Array
    ULONG userdata;                 // beliebig verwendbar
    struct ClickButton *buttons[MAXNUM_CLICKBUTTONS];   // ClickButtons-Pointer
};

struct MouseCoords {
    WORD scrx, scry;        // relativ zu "virtuellem Mousescreen"
    WORD boxx, boxy;        // relativ zu ClickBox
    WORD btnx, btny;        // relativ zu ClickButton
};

/*** eingebettet in <struct VFMInfo> ***/
struct ClickInfo {
    ULONG flags;            // siehe unten...

    struct ClickBox *box;   // Mouse innerhalb dieser Box oder NULL
    LONG btn;               // Mouse innerhalb dieses Buttons (Index ins
                            // Buttons-Array der <box>), oder -1L

    /*** Mouse-Coords bei _GetInput() ***/
    struct MouseCoords act;     // Mouse-Koords bei _GetInput()
    struct MouseCoords down;    // Mouse-Koords bei CIF_MOUSEDOWN, CIF_BUTTONDOWN
    struct MouseCoords up;      // Mouse-Koords bei CIF_MOUSEUP, CIF_BUTTONUP
};

/*** Definitionen für ClickInfo.flags ***/

#define CIF_VALID       (1<<0)

#define CIF_MOUSEDOWN   (1<<1)
#define CIF_MOUSEHOLD   (1<<2)
#define CIF_MOUSEUP     (1<<3)

#define CIF_BUTTONDOWN  (1<<4)
#define CIF_BUTTONHOLD  (1<<5)
#define CIF_BUTTONUP    (1<<6)

#define CIF_RMOUSEDOWN  (1<<7)      // wie CIF_MOUSEDOWN, aber RMB
#define CIF_RMOUSEHOLD  (1<<8)
#define CIF_RMOUSEUP    (1<<9)

#define CIF_MMOUSEDOWN  (1<<10)
#define CIF_MMOUSEHOLD  (1<<11)
#define CIF_MMOUSEUP    (1<<12)

#define CIF_MOUSEDBLCLCK    (1<<13) // ein linker Doubleklick

/*
**  CIF_VALID: Die Informationen in der ClickInfo-Struktur
**  sind _nur_dann_gültig_, wenn dieses Flag gesetzt ist. Andernseits
**  befindet sich die input.engine NICHT im GUI-Mode und
**  ignoriert damit während _GetInput() die in die VFMInput-Struktur
**  eingebettete ClickInfo-Struktur.
*/
/*-----------------------------------------------------------------*/
#endif




