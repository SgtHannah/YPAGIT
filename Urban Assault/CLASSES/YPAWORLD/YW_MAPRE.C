/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_mapreq.c,v $
**  $Revision: 38.27 $
**  $Date: 1998/01/06 16:23:29 $
**  $Locker: floh $
**  $Author: floh $
**
**  The amazing Map-Requester!
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "bitmap/rasterclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"
#include "ypa/ypatooltips.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_audio_engine

extern struct YPAStatusReq SR;
/*-------------------------------------------------------------------
**  die Subclips, davon gibt's eine ganze Menge, das vereinfacht
**  das Map-Handling nämlich ziemlich enorm.
*/

UBYTE MR_ReqString[512];
UBYTE MR_ClipYScroller[256];
UBYTE MR_ClipXScroller[256];

/*** Viel Platz für das Map-Innere, selbstpositionierend ***/
#ifdef AMIGA
__far
#endif
UBYTE MR_ClipRealMap[32768];

/*-------------------------------------------------------------------
**  Das globale Clips-Array... massiv attack
*/
UBYTE *MR_ClipArray[] = {
    MR_ClipYScroller,
    MR_ClipXScroller,
    MR_ClipRealMap,
};

/*-----------------------------------------------------------------*/
/*** Titlebar ClickButtons ***/
struct ClickButton MR_Iconify;
struct ClickButton MR_DragBar;
struct ClickButton MR_Zip;
struct ClickButton MR_Help;

/*** Buttonbar ClickButtons ***/
struct ClickButton MR_LandLayer;
struct ClickButton MR_OwnerLayer;
struct ClickButton MR_HeightLayer;

struct ClickButton MR_ViewerLock;
struct ClickButton MR_SelectLock;

struct ClickButton MR_ZoomIn;
struct ClickButton MR_ZoomOut;

/*** Y-Scroller ClickButtons ***/
struct ClickButton MR_YScrollTop;
struct ClickButton MR_YScrollKnob;
struct ClickButton MR_YScrollBottom;

/*** X-Scroller ClickButtons ***/
struct ClickButton MR_XScrollLeft;
struct ClickButton MR_XScrollKnob;
struct ClickButton MR_XScrollRight;

/*** Resize ClickButton ***/
struct ClickButton MR_Resize;

/*** Map-Inhalt ClickButton ***/
struct ClickButton MR_MapInterior;

/*** der Map-Requester itself... ***/
struct YPAMapReq MR = { NULL };

/*-----------------------------------------------------------------*/
void yw_MRLayoutReqString(struct ypaworld_data *ywd, BOOL init)
/*
**  FUNCTION
**      Füllt MR_ReqString:
**          - Positionierung
**          - TitleBar mit CloseGadget und MapButtons
**          - LeftEdge
**          - Clip für Y-Scroller
**          - Clip für X-Scroller
**          - Clip für Map-Interior
**
**      Außerdem werden alle Buttonstrukturen des
*+      TitleBars initialisiert/updated.
**
**  INPUTS
**      ywd     - Pointer auf LID des Welt-Objects
**      init    - wenn TRUE wird ALLES initialisiert, sonst
**                nur die Parameter, die sich von Frame zu
**                Frame ändern können.
**
**  CHANGED
**      30-Jul-96   floh    created (kompletter Rewrite)
**      29-May-97   floh    + Changes wegen Map-Buttons aus Titelzeile
**                            raus und in Map-Inneres.
**      28-Oct-97   floh    + Help-Button
*/
{
    struct ClickBox *cb = &(MR.req.req_cbox);
    ULONG btn_pos;
    ULONG btn;
    LONG le_size;

    UBYTE *str = MR.req.req_string;
    WORD x = MR.req.req_cbox.rect.x - (ywd->DspXRes>>1);
    WORD y = MR.req.req_cbox.rect.y - (ywd->DspYRes>>1);
    WORD w = MR.req.req_cbox.rect.w;
    WORD h = MR.req.req_cbox.rect.h;

    /*** Close-Button, DragBar-Button ***/
    MR_Iconify.rect.x = w - ywd->CloseW;
    MR_Iconify.rect.y = 0;
    MR_Iconify.rect.w = ywd->CloseW;
    MR_Iconify.rect.h = ywd->FontH;

    MR_Help.rect.x = w - 2*ywd->CloseW;
    MR_Help.rect.y = 0;
    MR_Help.rect.w = ywd->CloseW;
    MR_Help.rect.h = ywd->FontH;

    MR_DragBar.rect.x = 0;
    MR_DragBar.rect.y = 0;
    MR_DragBar.rect.w = w - 2*ywd->CloseW - MR.ZipW;
    MR_DragBar.rect.h = ywd->FontH;

    MR_Zip.rect.x = MR_DragBar.rect.x + MR_DragBar.rect.w;
    MR_Zip.rect.y = 0;
    MR_Zip.rect.w = MR.ZipW;
    MR_Zip.rect.h = ywd->FontH;

    /*** die Map-Control-Buttons ***/
    btn     = MAPBTN_LANDLAYER;
    btn_pos = MR.BorLeftW + MR.ButtonSpace;
    for (btn,btn_pos; btn<=MAPBTN_ZOOMOUT; btn++, btn_pos+=MR.ButtonW) {
        cb->buttons[btn]->rect.x = btn_pos;
        cb->buttons[btn]->rect.y = h - MR.BorBottomH - MR.ButtonH;
        cb->buttons[btn]->rect.w = MR.ButtonW;
        cb->buttons[btn]->rect.h = MR.ButtonH;
    };

    /*** Title-Zeile (Close + Drag) ***/
    str = yw_BuildReqTitle(ywd, x, y, w,
          ypa_GetStr(ywd->LocHandle,STR_WIN_MAP,"MAP"),
          str,
          (MR.flags & MAPF_ZIP_DOWN) ? 's':'q',
          MR.req.flags);

    /*** die LeftEdge ***/
    le_size = h - MR.BorVert;
    new_line(str);
    newfont_flush(str,FONTID_MAPVERT1);
    put(str,'A');   // LeftEdge, Top
    new_line(str);
    le_size--;

    newfont_flush(str,FONTID_MAPVERT);
    while (le_size > ywd->VPropH) {
        put(str,'A');   // LeftEdge, Inneres
        new_line(str);
        le_size -= ywd->VPropH;
    };
    if (le_size > 1) {
        len_vert(str,(le_size-1));
        put(str,'A');
        new_line(str);
    };

    newfont_flush(str,FONTID_MAPVERT1);
    put(str,'B');
    new_line(str);

    /*** schließlich noch die Clips für die komplexeren Bestandteile ***/
    clip(str,0);        // MR_ClipYScroller
    clip(str,1);        // MR_ClipXScroller
    clip(str,2);        // MR_ClipRealMap
    eos(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MRLayoutMapButtons(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Rendert die Map-Mode-Buttons. Muß im Post-Draw-Hook
**      aufgerufen werden.
**
**  CHANGED
**      29-May-97   floh    created
**      11-Dec-97   floh    extra NoLock Button ist raus.
*/
{
    WORD x = MR.req.req_cbox.rect.x - (ywd->DspXRes>>1);
    WORD y = MR.req.req_cbox.rect.y - (ywd->DspYRes>>1);
    WORD h = MR.req.req_cbox.rect.h;
    WORD xpos = x + MR.BorLeftW + MR.ButtonSpace;
    WORD ypos = y + (h - (MR.BorBottomH + MR.ButtonH));

    new_font(str,FONTID_MAPBTNS);
    pos_abs(str,xpos,ypos);
    if (MR.layers & MAP_LAYER_LANDSCAPE) { put(str,'a'); }
    else                                 { put(str,'A'); }
    if (MR.layers & MAP_LAYER_OWNER)     { put(str,'b'); }
    else                                 { put(str,'B'); }
    if (MR.layers & MAP_LAYER_HEIGHT)    { put(str,'d'); }
    else                                 { put(str,'D'); }

    if (MR.lock_mode == MAP_LOCK_VIEWER)   { put(str,'f'); }
    else                                   { put(str,'F'); }

    if (MR.flags & MAPF_ZOOMIN_DOWN)  { put(str,'j'); }
    else                              { put(str,'J'); }
    if (MR.flags & MAPF_ZOOMOUT_DOWN) { put(str,'k'); }
    else                              { put(str,'K'); }
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_MRLayoutXScroller(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Füllt MR_ClipXScroller, außerdem werden alle
**      XScroller-Button-Strukturen initialisiert.
**
**  CHANGED
**      30-Jul-96   floh    created
*/
{
    UBYTE *str  = MR_ClipXScroller;

    WORD x = MR.req.req_cbox.rect.x;
    WORD y = MR.req.req_cbox.rect.y;
    WORD w = MR.req.req_cbox.rect.w;
    WORD h = MR.req.req_cbox.rect.h;

    WORD xpos = x - (ywd->DspXRes >> 1);
    WORD ypos = (y + h - MR.BorBottomH) - (ywd->DspYRes >> 1);

    WORD scr  = MR.xscroll.size;
    WORD kofs = MR.xscroll.knoboffset;
    WORD ksze = MR.xscroll.knobsize;

    MR_XScrollLeft.rect.x = 0;
    MR_XScrollLeft.rect.y = h - MR.BorBottomH;
    MR_XScrollLeft.rect.w = kofs;
    MR_XScrollLeft.rect.h = MR.BorBottomH;

    MR_XScrollKnob.rect.x = MR_XScrollLeft.rect.w;
    MR_XScrollKnob.rect.y = MR_XScrollLeft.rect.y;
    MR_XScrollKnob.rect.w = ksze;
    MR_XScrollKnob.rect.h = MR_XScrollLeft.rect.h;

    MR_XScrollRight.rect.x = MR_XScrollKnob.rect.x + MR_XScrollKnob.rect.w;
    MR_XScrollRight.rect.y = MR_XScrollLeft.rect.y;
    MR_XScrollRight.rect.w = scr - kofs - ksze;
    MR_XScrollRight.rect.h = MR_XScrollLeft.rect.h;

    MR_Resize.rect.x = w - MR.BorRightW;
    MR_Resize.rect.y = h - MR.BorBottomH;
    MR_Resize.rect.w = MR.BorRightW;
    MR_Resize.rect.h = MR.BorBottomH;

    /*** String aufbauen ***/
    new_font(str,FONTID_MAPHORZ);
    pos_abs(str,xpos,ypos);

    /*** linker Scrollcontainer ***/
    if (kofs > 0) {
        put(str,'A');
        if (kofs > 1) {
            lstretch_to(str,kofs);
            put(str,'B');
        };
    };

    /*** Knob ***/
    put(str,'D');
    lstretch_to(str,(kofs+ksze-1));
    put(str,'E');
    put(str,'F');

    /*** rechter Rand ***/
    if ((kofs + ksze) < scr) {
        if ((kofs + ksze) < (scr-1)) {
            lstretch_to(str,(scr-1));
            put(str,'B');
        };
        put(str,'C');
    };

    /*** Resize Knob und Schluss ***/
    put(str,'G');
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_MRLayoutYScroller(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Entspricht yw_MRLayoutXScroller(), nur eben
**      für den Vertikal-Scroller.
**
**  CHANGED
**      30-Jul-96   floh    created
*/
{
    LONG dy;
    UBYTE *str = MR_ClipYScroller;

    WORD x = MR.req.req_cbox.rect.x;
    WORD y = MR.req.req_cbox.rect.y;
    WORD w = MR.req.req_cbox.rect.w;

    WORD xpos = (x + w - MR.BorRightW) - (ywd->DspXRes >> 1);
    WORD ypos = (y + MR.BorTopH) - (ywd->DspYRes >> 1);

    WORD scr  = MR.yscroll.size;
    WORD kofs = MR.yscroll.knoboffset;
    WORD ksze = MR.yscroll.knobsize;

    MR_YScrollTop.rect.x = w - MR.BorRightW;
    MR_YScrollTop.rect.y = MR.BorTopH;
    MR_YScrollTop.rect.w = MR.BorRightW;
    MR_YScrollTop.rect.h = kofs;

    MR_YScrollKnob.rect.x = MR_YScrollTop.rect.x;
    MR_YScrollKnob.rect.y = MR_YScrollTop.rect.y + MR_YScrollTop.rect.h;
    MR_YScrollKnob.rect.w = MR_YScrollTop.rect.w;
    MR_YScrollKnob.rect.h = ksze;

    MR_YScrollBottom.rect.x = MR_YScrollTop.rect.x;
    MR_YScrollBottom.rect.y = MR_YScrollKnob.rect.y + MR_YScrollKnob.rect.h;
    MR_YScrollBottom.rect.w = MR_YScrollTop.rect.w;
    MR_YScrollBottom.rect.h = scr - kofs - ksze;

    /*** String aufbauen ***/
    pos_abs(str,xpos,ypos);

    /*** oberer Teil des Scrollers ***/
    dy = kofs;
    if (dy > 0) {

        /* Background, Top */
        newfont_flush(str,FONTID_MAPVERT1);
        put(str,'C');
        new_line(str);

        dy -= 1;

        /* Background, Top, Inneres */
        newfont_flush(str,FONTID_MAPVERT);
        while (dy >= ywd->VPropH) {
            put(str,'B');
            new_line(str);
            dy -= ywd->VPropH;
        };
        if (dy > 0) {
            len_vert(str,dy);
            put(str,'B');
            new_line(str);
        };
    };

    /*** Scroller-Knob ***/
    dy = ksze;
    if (dy > 0) {
        /* Knob, Top */
        newfont_flush(str,FONTID_MAPVERT1);
        put(str,'E');
        new_line(str);

        dy -= 1;

        /*** Knob, Inneres ***/
        newfont_flush(str,FONTID_MAPVERT);
        while (dy > ywd->VPropH) {
            put(str,'C');
            new_line(str);
            dy -= ywd->VPropH;
        };
        if (dy > 1) {
            len_vert(str,(dy-1));
            put(str,'C');
            new_line(str);
        };

        /* Knob, Bottom */
        newfont_flush(str,FONTID_MAPVERT1);
        put(str,'F');
        new_line(str);
    };

    /*** unterer Teil des Scrollers ***/
    dy = scr - kofs - ksze;
    if (dy > 0) {

        /* Bottom-Inneres */
        newfont_flush(str,FONTID_MAPVERT);
        while (dy > ywd->VPropH) {
            put(str,'B');
            new_line(str);
            dy -= ywd->VPropH;
        };
        if (dy > 1) {
            len_vert(str,(dy-1));
            put(str,'B');
            new_line(str);
        };
        /* Bottom-Rand */
        newfont_flush(str,FONTID_MAPVERT1);
        put(str,'D');
        new_line(str);
    };

    /*** EOS ***/
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_MRLayoutMapInterior(struct ypaworld_data *ywd)
/*
**  CHANGED
**      30-Jul-96   floh    created
*/
{
    UBYTE *str = MR_ClipRealMap;

    WORD x = MR.req.req_cbox.rect.x;
    WORD y = MR.req.req_cbox.rect.y;
    WORD w = MR.req.req_cbox.rect.w;
    WORD h = MR.req.req_cbox.rect.h;

    WORD xpos = (x + MR.BorLeftW) - (ywd->DspXRes>>1);
    WORD ypos = (y + MR.BorTopH)  - (ywd->DspYRes>>1);

    MR_MapInterior.rect.x = MR.BorLeftW;
    MR_MapInterior.rect.y = MR.BorTopH;
    MR_MapInterior.rect.w = w - MR.BorHori;
    MR_MapInterior.rect.h = h - MR.BorVert;

    yw_RenderMapInterior(ywd,str,xpos,ypos);
}

/*-----------------------------------------------------------------*/
void yw_RefreshScrollerParams(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Errechnet aus den midx, midz, x_aspect, y_aspect
**      Werten die aktuellen Pixel-Parameter für die
**      beiden Scroller.
**
**  CHANGED
**      23-Oct-95   floh    created
**      13-Mar-96   floh    revised & updated (neues Map-Layout)
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    recised & updated (x_aspect, y_aspect getrennt)
*/
{
    FLOAT wsize_x = ywd->MapSizeX * SECTOR_SIZE;
    FLOAT wsize_z = ywd->MapSizeY * SECTOR_SIZE;

    FLOAT shown_x = MR.x_aspect * (MR.req.req_cbox.rect.w - MR.BorHori);
    FLOAT shown_z = MR.y_aspect * (MR.req.req_cbox.rect.h - MR.BorVert);

    FLOAT start_x = MR.midx - (shown_x * 0.5);
    FLOAT start_z = -(MR.midz + (shown_z * 0.5));

    WORD knobsize, knobstart, scrsize;

    /*** zuerst den X-Scroller... ***/
    scrsize   = MR.req.req_cbox.rect.w - MR.BorRightW;
    knobsize  = (scrsize * shown_x)/wsize_x;
    knobstart = (scrsize * start_x)/wsize_x;

    if (knobsize < MR.MinKnobSize) {
        knobstart -= (MR.MinKnobSize-knobsize)>>1;
        knobsize   = MR.MinKnobSize;
    } else if (knobsize > scrsize) knobsize = scrsize;
    if (knobstart < 0) knobstart = 0;
    if ((knobstart + knobsize) > scrsize) knobstart = scrsize-knobsize;

    MR.xscroll.size       = scrsize;
    MR.xscroll.knoboffset = knobstart;
    MR.xscroll.knobsize   = knobsize;

    /*** ...dann den Y-Scroller ***/
    scrsize   = MR.req.req_cbox.rect.h - MR.BorVert;
    knobsize  = (scrsize * shown_z)/wsize_z;
    knobstart = (scrsize * start_z)/wsize_z;

    if (knobsize < MR.MinKnobSize) {
        knobstart -= (MR.MinKnobSize-knobsize)>>1;
        knobsize   = MR.MinKnobSize;
    } else if (knobsize > scrsize) knobsize = scrsize;
    if (knobstart < 0) knobstart = 0;
    if ((knobstart + knobsize) > scrsize)  knobstart = scrsize-knobsize;

    MR.yscroll.size       = scrsize;
    MR.yscroll.knoboffset = knobstart;
    MR.yscroll.knobsize   = knobsize;
}

/*-----------------------------------------------------------------*/
void yw_CheckMidPoint(struct ypaworld_data *ywd, LONG w, LONG h)
/*
**  FUNCTION
**      Testet, ob beim aktuellen MidPoint Bereiche der
**      Map im "nicht definierten Bereich" liegen und
**      korrigiert den Mittelpunkt, wenn das der Fall ist.
**
**      Muß angewendet werden, bei: Scrolling, Zooming, Resizing.
**
**  INPUTS
**      ywd     -> Pointer auf LID des Welt-Objects
**      w       -> sichtbare Breite in Pixel
**      h       -> sichtbare Höhe in Pixel
**
**  CHANGED
**      08-Nov-95   floh    created
**      26-Jun-96   floh    erweitert für neue Map-Revision (Window
**                          darf größer als Map sein): falls das
**                          der Fall ist, wird <midx,midy> auf die
**                          Mitte der Welt gesetzt.
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      14-Sep-96   floh    Höhe und Breite müssen von außen gefüttert
**                          werden.
**      15-Sep-96   floh    x_aspect und y_aspect getrennt
*/
{
    FLOAT wsizex, wsizey, shown, start, end;

    wsizex = ywd->MapSizeX * SECTOR_SIZE;
    wsizey = ywd->MapSizeY * SECTOR_SIZE;

    /*** MidX-Korrektur ***/
    shown = MR.x_aspect * w;
    start = MR.midx - shown*0.5;
    end   = MR.midx + shown*0.5;

    if      (shown > wsizex)  MR.midx = wsizex*0.5;
    else if (start < 0.0)     MR.midx = shown*0.5;
    else if (end   > wsizex)  MR.midx = wsizex-shown*0.5;

    /*** MidZ-Korrektur ***/
    shown = MR.y_aspect * h;
    start = MR.midz + shown*0.5;
    end   = MR.midz - shown*0.5;

    if      (shown > wsizey)  MR.midz = -wsizey*0.5;
    else if (start > 0.0)     MR.midz = -shown*0.5;
    else if (end   < -wsizey) MR.midz = -wsizey+shown*0.5;
}

/*-----------------------------------------------------------------*/
void yw_MapZoom(struct ypaworld_data *ywd, ULONG control)
/*
**  FUNCTION
**      Kontrolle über Map-Zoomer, die Zoom-Level sind teilweise
**      abhängig vom eingestellten Map-Mode, deshalb ist die
**      Sache etwas komplexer...
**
**  INPUTS
**      ywd     -> LID des Welt-Objects
**      control -> one of:
**                  MAP_ZOOM_CORRECT    // nur korrigieren
**                  MAP_ZOOM_IN         // reinzoomen, wenn möglich
**                  MAP_ZOOM_OUT        // rauzoomen, wenn möglich
**
**  RESULTS
**      MR.zoom, MR.x_aspect, MR.y_aspect wird evtl. modifiziert
**
**  CHANGED
**      03-Nov-95   floh    created
**      04-Nov-95   floh    neu: jetzt wird jedesmal getestet,
**                          ob durch das Zooming leere Bereiche
**                          entstanden sein könnten. Wenn dem so
**                          ist, wird der Mittelpunkt entsprechend 
**                          verschoben.
**      08-Nov-95   floh    jetzt insgesamt 5 Zoom-Stufen
**      15-Nov-95   floh    vor yw_CheckMidPoint() jetzt ein
**                          yw_RefreshScrollerParams(), weil
**                          yw_CheckMidPoint() auf Scroller-Parameter
**                          zurückgreift
**      16-Nov-95   floh    Die Mittelpunkt-Korrektur wurde ausgelagert
**                          nach yw_RenderMap(), damit keine
**                          Redundanzen auftreten.
**      17-Nov-95   floh    Owner.Modus nur noch 2 Zoom-Stufen...
**      05-Feb-96   floh    ein paar Zoom-Stufen sind weggefallen,
**                          außerdem besitzen jetzt alle Layer dieselben
**                          Zoomstufen (notwendigerweise, weil sie ja
**                          übereinander geblendet werden).
**      26-Jun-96   floh    jetzt ohne Ausdehnung der Max-Window-Size
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    x_aspect und y_aspect getrennt
**      03-Jun-97   floh    neue Zoom-Stufe
*/
{
    /*** erstmal eine Hardcore-Modifikation ***/
    switch (control) {
        case MAP_ZOOM_IN:   MR.zoom++; break;
        case MAP_ZOOM_OUT:  MR.zoom--; break;
    };

    /*** Korrektur ***/
    if      (MR.zoom < 0) MR.zoom = 0;
    else if (MR.zoom > 4) MR.zoom = 4;

    /*** Pixel-Aspekt setzen ***/
    switch (MR.zoom) {
        case 0:
            /* (1 Sektor == 4 Pixel) -> 1200/4 */
            MR.x_aspect = 300.0;
            MR.y_aspect = 300.0;
            break;

        case 1:
            /* (1 Sektor == 8 Pixel) -> 1200/8 */
            MR.x_aspect = 150.0;
            MR.y_aspect = 150.0;
            break;

        case 2:
            /* (1 Sektor == 16 Pixel) -> 1200/16 */
            MR.x_aspect = 75.0;
            MR.y_aspect = 75.0;
            break;

        case 3:
            /* (1 Sektor == 32 Pixel) -> 1200/32 */
            MR.x_aspect = 37.5;
            MR.y_aspect = 37.5;
            break;

        case 4:
            /* (1 Sektor == 64 Pixel) -> 1200/64) */
            MR.x_aspect = 18.75;
            MR.y_aspect = 18.75;
            break;
    };
}

/*-----------------------------------------------------------------*/
void yw_InitMapReq(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Map-Requester.
**
**  CHANGED
**      23-Oct-95   floh    created
**      30-Oct-95   floh    statt Resize-Bar jetzt ein einfaches
**                          Resize-Button
**      17-Nov-95   floh    + 2 neue Buttons (Minimize, Maximize)
**                          + alle MapButton-IDs per #define
**      21-Nov-95   floh    + <post_draw> Hook wird initialisiert
**      22-Nov-95   floh    + Render-Vehicles-Hook eingebunden
**      28-Nov-95   floh    + neue Standard-Requester-Flags
**      26-Dec-95   floh    Jetzt ohne Icon, weil Open/Close von
**                          einem Status-Bar-Button geregelt wird.
**      05-Feb-96   floh    Teilweise neues Map-Layout, arbeitet
**                          jetzt mit "Layern", die beliebig
**                          übereinander geblendet werden können.
**      11-Mar-96   floh    der MR_ClipRealMap[] Buffer wird jetzt
**                          als "non initialized data" behandelt,
**                          damit reduziert sich hoffentlich die
**                          Executable-Größe etwas...
**      13-Mar-96   floh    revised & updated (neues Map-Layout)
**      21-Mar-96   floh    + MR.footprint Initialisierung
**      05-May-96   floh    + Map wird auf Userrobo zentriert, so
**                            einer existiert
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      13-Sep-96   floh    + Prefs-Handling
**      14-Sep-96   floh    + Radar-Handling
**      15-Sep-96   floh    + x_aspect, y_aspect getrennt
**      21-Sep-96   floh    + Bugfix: Prefs-Einstellungen für Layers
**                            und Zoom wurden durch die Default-Einstellungen
**                            überschrieben
**      23-Apr-97   floh    + "Radar" umpositioniert
**      29-May-97   floh    + Map-Buttons umpositioniert (aus Titelzeile raus,
**                            ins Map-Innere
**      11-Oct-97   floh    + Prefs-Korrektur per ywd->UpperTabu und
**                            ywd->LowerTabu
**      11-Dec-97   floh    + NoLock Button raus
**      30-May-98   floh    + Prefs-Handling ist rausgeflogen...
**                          + per Default groesste Zoom-Stufe und
**                            alle Layer an
*/
{
    /*** Global Data init... ***/
    memset(MR_ReqString,0,sizeof(MR_ReqString));
    memset(MR_ClipYScroller,0,sizeof(MR_ClipYScroller));
    memset(MR_ClipXScroller,0,sizeof(MR_ClipXScroller));
    memset(MR_ClipRealMap,0,sizeof(MR_ClipRealMap));

    /*** Requester-Struktur initialisieren ***/
    MR.req.flags = REQF_Closed|REQF_HasDragBar|REQF_HasCloseGadget|REQF_HasHelpGadget;

    /*** Position und Ausdehnung ***/
    MR.MinKnobSize = 8;
    MR.ButtonW     = ywd->Fonts[FONTID_MAPBTNS]->fchars['A'].width;
    MR.ButtonH     = ywd->Fonts[FONTID_MAPBTNS]->height;
    MR.ButtonSpace = 4;
    MR.ZipW        = ywd->Fonts[FONTID_DEFAULT]->fchars['q'].width;
    MR.BorTopH     = ywd->FontH;
    MR.BorBottomH  = ywd->PropH;
    MR.BorLeftW    = ywd->Fonts[FONTID_MAPVERT]->fchars['A'].width;
    MR.BorRightW   = ywd->PropW;
    MR.BorHori     = MR.BorLeftW + MR.BorRightW;
    MR.BorVert     = MR.BorTopH + MR.BorBottomH;
    MR.min_width  = MR.BorLeftW + 2*MR.ButtonSpace + 7*MR.ButtonW + MR.BorRightW;
    MR.min_height = MR.BorTopH + MR.BorBottomH + 96;
    MR.max_width  = 32000;
    MR.max_height = 32000;

    /*** Default-Einstellungen ***/
    MR.req.req_cbox.rect.x = 0;
    MR.req.req_cbox.rect.y = ywd->UpperTabu;
    MR.req.req_cbox.rect.w = (ywd->DspXRes*2)/3;
    MR.req.req_cbox.rect.h = (ywd->DspYRes*2)/3;
    MR.layers = MAP_LAYER_LANDSCAPE|MAP_LAYER_OWNER|MAP_LAYER_HEIGHT;
    MR.zoom   = 4;

    MR.req.req_cbox.num_buttons = MAP_NUMBUTTONS;

    MR.req.req_cbox.buttons[MAPBTN_ICONIFY]       = &MR_Iconify;
    MR.req.req_cbox.buttons[MAPBTN_DRAGBAR]       = &MR_DragBar;
    MR.req.req_cbox.buttons[MAPBTN_HELP]          = &MR_Help;
    MR.req.req_cbox.buttons[MAPBTN_ZIP]           = &MR_Zip;

    MR.req.req_cbox.buttons[MAPBTN_LANDLAYER]     = &MR_LandLayer;
    MR.req.req_cbox.buttons[MAPBTN_OWNERLAYER]    = &MR_OwnerLayer;
    MR.req.req_cbox.buttons[MAPBTN_HEIGHTLAYER]   = &MR_HeightLayer;

    MR.req.req_cbox.buttons[MAPBTN_VIEWERLOCK]    = &MR_ViewerLock;

    MR.req.req_cbox.buttons[MAPBTN_ZOOMIN]        = &MR_ZoomIn;
    MR.req.req_cbox.buttons[MAPBTN_ZOOMOUT]       = &MR_ZoomOut;

    MR.req.req_cbox.buttons[MAPBTN_YSCROLLTOP]    = &MR_YScrollTop;
    MR.req.req_cbox.buttons[MAPBTN_YSCROLLKNOB]   = &MR_YScrollKnob;
    MR.req.req_cbox.buttons[MAPBTN_YSCROLLBOTTOM] = &MR_YScrollBottom;

    MR.req.req_cbox.buttons[MAPBTN_XSCROLLLEFT]   = &MR_XScrollLeft;
    MR.req.req_cbox.buttons[MAPBTN_XSCROLLKNOB]   = &MR_XScrollKnob;
    MR.req.req_cbox.buttons[MAPBTN_XSCROLLRIGHT]  = &MR_XScrollRight;

    MR.req.req_cbox.buttons[MAPBTN_RESIZE]        = &MR_Resize;
    MR.req.req_cbox.buttons[MAPBTN_MAPINTERIOR]   = &MR_MapInterior;

    MR.req.req_string  = MR_ReqString;
    MR.req.req_clips   = MR_ClipArray;
    MR.req.post_draw   = yw_RenderMapVehicles;

    /*** Requester-Erweiterung initialisieren ***/
    if (ywd->URBact) {
        MR.midx = ywd->URBact->pos.x;
        MR.midz = ywd->URBact->pos.z;
    } else {
        MR.midx = 32*SECTOR_SIZE + (SECTOR_SIZE/2);
        MR.midz = -(32*SECTOR_SIZE + (SECTOR_SIZE/2));
    };

    /*** Pixel-Aspekt setzen ***/
    switch (MR.zoom) {
        case 0:
            /* (1 Sektor == 4 Pixel) -> 1200/4 */
            MR.x_aspect = 300.0;
            MR.y_aspect = 300.0;
            break;

        case 1:
            /* (1 Sektor == 8 Pixel) -> 1200/8 */
            MR.x_aspect = 150.0;
            MR.y_aspect = 150.0;
            break;

        case 2:
            /* (1 Sektor == 16 Pixel) -> 1200/16 */
            MR.x_aspect = 75.0;
            MR.y_aspect = 75.0;
            break;

        case 3:
            /* (1 Sektor == 32 Pixel) -> 1200/32 */
            MR.x_aspect = 37.5;
            MR.y_aspect = 37.5;
            break;

        case 4:
            /* (1 Sektor == 64 Pixel) -> 1200/64 */
            MR.x_aspect = 18.75;
            MR.y_aspect = 18.75;
            break;
    };

    /*** weitere Map-Parameter setzen ***/
    MR.lock_mode  = MAP_LOCK_NONE;
    MR.flags      = 0;
    MR.footprint  = (1<<1);

    MR.zip_zap.x = ywd->DspXRes - MR.min_width;
    MR.zip_zap.y = ywd->DspYRes - ywd->LowerTabu - MR.min_height;
    MR.zip_zap.w = MR.min_width;
    MR.zip_zap.h = MR.min_height;

    /*** Radar-Stuff ***/
    MR.r_x0 = 0.45;
    MR.r_y0 = 0.0;
    MR.r_x1 = 0.95;
    MR.r_y1 = 0.6;

    /*** den ganzen Rest initialisieren ***/
    yw_RefreshScrollerParams(ywd);
    yw_MRLayoutReqString(ywd,TRUE);
    yw_MRLayoutXScroller(ywd);
    yw_MRLayoutYScroller(ywd);
    yw_MRLayoutMapInterior(ywd);
    yw_CheckMidPoint(ywd,MR.xscroll.size-MR.BorLeftW,MR.yscroll.size);

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_LayoutMR(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet Map-Requester (wird 1x pro Frame aufgerufen).
**
**  CHANGED
**      30-Oct-95   floh    created
**      03-Nov-95   floh    erweitert
**      17-Nov-95   floh    + MiniMax Buttons
**      28-Nov-95   floh    + neue Standard-Requester-Flags
**      05-Feb-96   floh    neues Layout-Handling
**      13-Mar-96   floh    revised & updated (neues Map-Layout)
**      18-Mar-96   floh    + MAPF_SCROLL
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    if (!(MR.req.flags & REQF_Closed)) {

        /*** LockModi... Scrolling ist aber erlaubt ***/
        if (!(MR.flags & (MAPF_SCROLLX|MAPF_SCROLLY|MAPF_SCROLL))) {

            switch (MR.lock_mode) {
                case MAP_LOCK_VIEWER:
                    if (ywd->Viewer) {
                        MR.midx = ywd->Viewer->pos.x;
                        MR.midz = ywd->Viewer->pos.z;
                        yw_CheckMidPoint(ywd,MR.xscroll.size-MR.BorLeftW,MR.yscroll.size);
                    };
                    break;
            };
        };

        /*** ButtonDown-Flags löschen ***/
        MR.flags &= ~(MAPF_ZIP_DOWN|MAPF_ZOOMIN_DOWN|MAPF_ZOOMOUT_DOWN);
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MRScrollX(struct ypaworld_data *ywd, LONG delta, FLOAT aspect)
/*
**  FUNCTION
**      Realisiert X-Scrolling in Map, übergeben wird ein
**      Mouse-Delta-Value. Returniert wird
**
**  INPUT
**      ywd     - Ptr auf LID des Welt-Objects
**      delta   - Pixel-Delta-Value
**      aspect  - Umrechnungs-Multiplikator für Pixel-Delta auf
**                Welt-Koordinaten
**
**  CHANGED
**      18-Mar-96   floh    created
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      11-Dec-97   floh    schaltet Scroll-Lock ab
*/
{
    MR.midx = MR.scr_oldmidx + (delta * aspect);
    MR.lock_mode  = MAP_LOCK_NONE;
}

/*-----------------------------------------------------------------*/
void yw_MRScrollY(struct ypaworld_data *ywd, LONG delta, FLOAT aspect)
/*
**  FUNCTION
**      siehe yw_MRScrollY()
**
**  CHANGED
**      18-Mar-96   floh    created
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      11-Dec-97   floh    schaltet Scroll-Lock ab
*/
{
    MR.midz = MR.scr_oldmidz - (delta * aspect);
    MR.lock_mode  = MAP_LOCK_NONE;
}

/*-----------------------------------------------------------------*/
void yw_MRGetDragCoords(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Ermittelt aus den Screen-Dragbox-Koordinaten
**      die korrepondierenden Welt-Koordinaten (werden nach
**      <drag_world> geschrieben.
**      Die Koordinaten werden NICHT gegen die echte
**      Welt-Grenze geclippte (das sollte für die Dragbox
**      auch unerheblich sein).
**
**  CHANGED
**      12-Dec-96   floh    created
*/
{
    /*** Pixel-Adder für Screen-Koords -> relativ zu Map-Mittelpunkt ***/
    WORD rel_x,rel_y;
    rel_x = -(MR.req.req_cbox.rect.x+MR_MapInterior.rect.x+(MR_MapInterior.rect.w>>1));
    rel_y = -(MR.req.req_cbox.rect.y+MR_MapInterior.rect.y+(MR_MapInterior.rect.h>>1));

    MR.drag_world.xmin = MR.midx+((MR.drag_scr.xmin+rel_x)*MR.x_aspect);
    MR.drag_world.xmax = MR.midx+((MR.drag_scr.xmax+rel_x)*MR.x_aspect);
    MR.drag_world.ymin = MR.midz-((MR.drag_scr.ymin+rel_y)*MR.y_aspect);
    MR.drag_world.ymax = MR.midz-((MR.drag_scr.ymax+rel_y)*MR.y_aspect);

    /*** sicherstellen, daß (xmin<xmax) und (ymin<ymax) ***/
    if (MR.drag_world.xmin > MR.drag_world.xmax) {
        FLOAT t = MR.drag_world.xmin;
        MR.drag_world.xmin = MR.drag_world.xmax;
        MR.drag_world.xmax = t;
    };
    if (MR.drag_world.ymin > MR.drag_world.ymax) {
        FLOAT t = MR.drag_world.ymin;
        MR.drag_world.ymin = MR.drag_world.ymax;
        MR.drag_world.ymax = t;
    };
}

/*-----------------------------------------------------------------*/
void yw_MRDragDone(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erledigt alle Aktionen, wenn ein DragSelect in der
**      Map erfolgreich abgeschlossen wurde.
**
**  CHANGED
**      13-Dec-96   floh    created
**      16-Dec-96   floh    + das erzeugte Geschwader wird zum
**                            selektierten gemacht
**      19-Jan-97   floh    + das Selektieren des neuen Geschwaders
**                            ist jetzt cleverer (per CommandID-
**                            Scanning).
**      08-Dec-97   floh    + falls Robo der einzige Dragkandidat war,
**                            wird Beamen aktiviert.
*/
{
    struct Bacterium *first_bact;
    LONG sx,sy;
    LONG start_sx,start_sy;
    LONG end_sx,end_sy;
    BOOL is_robo_alone = FALSE;

    /*** Start- und Endsektoren des Drag-Rect ermitteln ***/
    start_sx = ((LONG)(MR.drag_world.xmin))/((WORD)SECTOR_SIZE);
    start_sy = ((LONG)(-MR.drag_world.ymax))/((WORD)SECTOR_SIZE);   // kein Bug!
    end_sx   = ((LONG)(MR.drag_world.xmax))/((WORD)SECTOR_SIZE);
    end_sy   = ((LONG)(-MR.drag_world.ymin))/((WORD)SECTOR_SIZE);   // kein Bug!

    /*** Clipping gegen inneren Welt-Rand ***/
    if      (start_sx < 1)              start_sx = 1;
    else if (start_sx >= ywd->MapSizeX) start_sx = ywd->MapSizeX-1;
    if      (end_sx < 1)                end_sx   = 1;
    else if (end_sx >= ywd->MapSizeX)   end_sx   = ywd->MapSizeX-1;
    if      (start_sy < 1)              start_sy = 1;
    else if (start_sy >= ywd->MapSizeY) start_sy = ywd->MapSizeY-1;
    if      (end_sy < 1)                end_sy   = 1;
    else if (end_sy >= ywd->MapSizeY)   end_sy   = ywd->MapSizeY-1;

    /*** Sektoren abklappern (abgeglichen mit yw_RenderMapVehicles()) ***/
    first_bact = NULL;
    for (sy=start_sy; sy<=end_sy; sy++) {
        for (sx=start_sx; sx<=end_sx; sx++) {

            struct Cell *sec = &(ywd->CellArea[sy*ywd->MapSizeX + sx]);
            if (sec->FootPrint & MR.footprint) {

                struct MinList *ls;
                struct MinNode *nd;

                ls = (struct MinList *) &(sec->BactList);
                for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                    struct Bacterium *b = (struct Bacterium *)nd;

                    /*** die Auswahl-Kriterien sind ziemlich komplex... ***/
                    if ((b->Owner == ywd->URBact->Owner)   &&
                        (b->pos.x > MR.drag_world.xmin)    &&
                        (b->pos.z > MR.drag_world.ymin)    &&
                        (b->pos.x < MR.drag_world.xmax)    &&
                        (b->pos.z < MR.drag_world.ymax)    &&
                        (b->BactClassID != BCLID_YPAMISSY) &&
                        (b->BactClassID != BCLID_YPAGUN)   &&
                        (b->MainState   != ACTION_CREATE)  &&
                        (b->MainState   != ACTION_BEAM)    &&
                        (b->MainState   != ACTION_DEAD))
                    {
                        if (!first_bact) {
                            if (b->BactClassID != BCLID_YPAROBO) {
                                /*** der 1.Treffer bildet das Geschwader ***/
                                struct organize_msg om;
                                om.mode = ORG_NEWCOMMAND;
                                om.specialbact = NULL;
                                _methoda(b->BactObject,YBM_ORGANIZE,&om);
                                first_bact = b;
                                is_robo_alone = FALSE;
                            } else if (b == ywd->URBact) {
                                is_robo_alone = TRUE; 
                            };
                        } else {
                            if (b->BactClassID != BCLID_YPAROBO) {
                                /*** alle weiteren werden dort rangehängt ***/
                                struct organize_msg om;
                                om.mode = ORG_ADDSLAVE;
                                om.specialbact = b;
                                _methoda(first_bact->BactObject,YBM_ORGANIZE,&om);
                                is_robo_alone = FALSE;
                            };
                        };
                    };
                };
            };
        };
    };

    /*** wenn alleinig der Robo ausgewaehlt wurde, Beamen anschalten ***/
    if (is_robo_alone) SR.ActiveMode = (STAT_MODEF_AUTOPILOT & SR.EnabledModes); 

    /*** alles erledigt... Commander-Array updaten ***/
    if (first_bact) ywd->ActCmdID=first_bact->CommandID;
    yw_RemapCommanders(ywd);
}

/*-----------------------------------------------------------------*/
void yw_HandleInputMR(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Handelt MapRequester-Input ab. Außerdem werden alle
**      GUI-Elemente upgedatet, die durch Dragging/Resizing/Scrolling
**      verändert werden.
**
**  CHANGED
**      30-Oct-95   floh    created
**      03-Nov-95   floh    Button-Bar implementiert
**      08-Nov-95   floh    neu: Resize und Scroll verwenden
**                          jetzt yw_CheckMidPoint()
**      15-Nov-95   floh    yw_CheckMidPoint() verwendete
**                          Scroller-Parameter, die aber erst
**                          von yw_RefreshScrollerParams()
**                          updated werden... Deshalb gab
**                          es zumindest beim Resizing
**                          Probleme...
**      16-Nov-95   floh    + Test, ob iconifiziert, in dem Fall
**                            wird nichts gemacht
**      17-Nov-95   floh    + alle MapButton-IDs als Define
**                          + MiniMax-Button Auswertung
**      28-Nov-95   floh    + neue Standard-Requester-Flags
**      31-Dec-95   floh    + STAT_HEIGHT breiter Tabu-Streifen oben und unten
**      05-Feb-96   floh    revised & updated für neues Map-Handling
**                          (Layers)
**      13-Mar-96   floh    revised & updated (neues Map-Layout)
**      18-Mar-96   floh    + Double-Map-Scrolling per RMB
**                          + Scrolling über Map-Scroll-Container-Buttons
**      21-Mar-96   floh    + Footprint-Maske wird gesetzt, auf Tastendruck
**                            auch im Debug-Modus (Sichtbereiche der einzelnen
**                            Robos sichtbar machen).
**      25-Mar-96   floh    + HotKey-Support
**      08-Apr-96   floh    + falls Map im Radar-Modus, wird sie
**                            automatisch hinter alle anderen Requester
**                            gesetzt (in diesem Fall der Finder, falls offen)
**      10-May-96   floh    + DragLock Handling
**      23-May-96   floh    + Footprint-Handling im Design-Modus
**      13-Jun-96   floh    + Bugfix: Map im Radarmodus nicht mehr
**                            per Hotkey manipulierbar
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Sep-96   floh    + x_aspect und y_aspect getrennt
**      18-Sep-96   floh    + Map-Button-Tooltips
**      21-Sep-96   floh    + Cheat: auf F8 kann man die gesamte Map
**                            begutachten
**      06-Nov-96   floh    + F8-Cheat ersetzt durch das allgemeinere
**                            Debug-Info-Flag
**      11-Dec-96   floh    + Vorbereitungen für DragSelect in Map
**      12-Dec-96   floh    + die Überschneidung der MouseDown-Events
**                            in der Map mit dem DragBox-Handling wird
**                            jetzt etwas cleverer gelöst (einfach auf
**                            MouseUp schalten war nicht gut...)
**      03-Jul-97   floh    + ein erfolgreich abgeschlossener Drag-Vorgang
**                            schaltet immer den Order-Modus ein
**      23-Oct-97   floh    + Sound
**      11-Dec-97   floh    + Scrolling schaltet jetzt Lockmodus aus
**                          + NoLock Button raus
**      05-Mar-98   floh    + Online-Hilfe
**      22-May-98   floh    + Dragselect hat jetzt groesseren Toleranz-Bereich
**                            (5 Pixel)
*/
{
    if (!(MR.req.flags & REQF_Closed)) {

        struct ClickInfo *ci = &(ip->ClickInfo);

        WORD req_x = MR.req.req_cbox.rect.x;
        WORD req_y = MR.req.req_cbox.rect.y;
        WORD req_w = MR.req.req_cbox.rect.w;
        WORD req_h = MR.req.req_cbox.rect.h;

        /*** Footprint-Zeugs ***/
        MR.footprint = (1<<(ywd->URBact->Owner));

        // FIXME!!!
        if (ywd->DebugInfo) MR.footprint = 0xff;

        #ifdef YPA_CUTTERMODE
            MR.footprint = 0xff;
        #endif
        #ifdef YPA_DESIGNMODE
            MR.footprint = 0xff;
        #endif

        /*** HotKey-Support (per ButtonDown-Simulation) ***/
        if (ip->HotKey != 0) {

            switch(ip->HotKey) {
                case HOTKEY_MAP_LAND:
                    ci->flags |= CIF_BUTTONDOWN;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_LANDLAYER;
                    break;

                case HOTKEY_MAP_OWNER:
                    ci->flags |= CIF_BUTTONDOWN;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_OWNERLAYER;
                    break;

                case HOTKEY_MAP_HEIGHT:
                    ci->flags |= CIF_BUTTONDOWN;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_HEIGHTLAYER;
                    break;

                case HOTKEY_MAP_LOCKVWR:
                    ci->flags |= CIF_BUTTONDOWN;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_VIEWERLOCK;
                    break;

                case HOTKEY_MAP_ZOOMIN:
                    ci->flags |= CIF_BUTTONUP;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_ZOOMIN;
                    break;

                case HOTKEY_MAP_ZOOMOUT:
                    ci->flags |= CIF_BUTTONUP;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_ZOOMOUT;
                    break;

                case HOTKEY_MAP_MINIMIZE:
                    ci->flags |= CIF_BUTTONUP;
                    ci->box    = &(MR.req.req_cbox);
                    ci->btn    = MAPBTN_ZIP;
                    break;
            };
        };

        /*** kontinuierliche Prozesse ***/
        if (MR.flags & MAPF_RESIZE) {

            /*** Resizing underway ***/
            if (ci->flags & CIF_MOUSEHOLD) {

                WORD new_w = MR.rsz_x + ci->act.scrx;
                WORD new_h = MR.rsz_y + ci->act.scry;

                /*** Minimal/Maximal-Ausdehnungs-Korrektur ***/
                if (new_w > MR.max_width)       new_w=MR.max_width;
                else if (new_w < MR.min_width)  new_w=MR.min_width;
                if (new_h > MR.max_height)      new_h=MR.max_height;
                else if (new_h < MR.min_height) new_h=MR.min_height;

                /*** Rand-Korrektur ***/
                if ((req_x + new_w) >= ywd->DspXRes) new_w=ywd->DspXRes-req_x;
                if ((req_y + new_h) >= (ywd->DspYRes-ywd->LowerTabu)) {
                    new_h = ywd->DspYRes - ywd->LowerTabu - req_y;
                };

                /*** neue Größe eintragen ***/
                MR.req.req_cbox.rect.w = new_w;
                MR.req.req_cbox.rect.h = new_h;
                req_w = new_w;
                req_h = new_h;

            } else {

                /*** Resizing abbrechen ***/
                MR.flags &= ~MAPF_RESIZE;
                ywd->DragLock = FALSE;
            };

        } else if (MR.flags & MAPF_SCROLLX) {

            /*** X-Scrolling underway ***/
            if (ci->flags & CIF_MOUSEHOLD) {
                FLOAT asp = (ywd->MapSizeX * SECTOR_SIZE) / MR.xscroll.size;
                yw_MRScrollX(ywd, ci->act.scrx - MR.scr_x, asp);
            } else {
                /*** X-Scrolling abbrechen ***/
                MR.flags &= ~MAPF_SCROLLX;
                ywd->DragLock = FALSE;
            };

        } else if (MR.flags & MAPF_SCROLLY) {

            /*** Y-Scrolling underway ***/
            if (ci->flags & CIF_MOUSEHOLD) {
                FLOAT asp = (ywd->MapSizeY * SECTOR_SIZE) / MR.yscroll.size;
                yw_MRScrollY(ywd, ci->act.scry - MR.scr_y, asp);
            } else {
                /*** Y-Scrolling abbrechen ***/
                MR.flags &= ~MAPF_SCROLLY;
                ywd->DragLock = FALSE;
            };

        } else if (MR.flags & MAPF_SCROLL) {

            /*** Double-Scrolling underway ***/
            if (ci->flags & CIF_RMOUSEHOLD) {
                yw_MRScrollX(ywd, MR.scr_x - ci->act.scrx, MR.x_aspect);
                yw_MRScrollY(ywd, MR.scr_y - ci->act.scry, MR.y_aspect);
            } else {
                /*** wurde Maus bewegt? ***/
                LONG dx = abs(MR.scr_x - ci->act.scrx);
                LONG dy = abs(MR.scr_y - ci->act.scry);
                if ((dx <= 1) && (dy <= 1)) {
                    /*** deselektiere aktuelles Geschwader ***/
                    ywd->ActCmdID = 0;
                    ywd->ActCmdr  = -1;
                };
                /*** Double-Scroll abbrechen ***/
                MR.flags &= ~MAPF_SCROLL;
                ywd->DragLock = FALSE;
            };

        } else if (MR.flags & MAPF_DRAGGING) {

            /*** Drag-Select underway ***/

            /*** erstmal die Mauskoords mitschreiben, egal was is ***/
            MR.drag_scr.xmax = ci->act.scrx;
            MR.drag_scr.ymax = ci->act.scry;
            yw_MRGetDragCoords(ywd);

            /*** Maus losgelassen? ***/
            if (!(ci->flags & CIF_MOUSEHOLD)) {
                /*** weit genug gedraggt? ***/
                LONG dx = MR.drag_scr.xmax - MR.drag_scr.xmin;
                LONG dy = MR.drag_scr.ymax - MR.drag_scr.ymin;
                if ((abs(dx)>5) && (abs(dy)>5)) {
                    /*** Drag-Vorgang erfolgreich abschließen ***/
                    SR.ActiveMode = STAT_MODEF_ORDER;
                    MR.flags &= ~MAPF_DRAGGING;
                    yw_MRDragDone(ywd);
                }else{
                    /*** nicht gemoved, Pseudo-MouseDown simulieren ***/
                    MR.flags &= ~MAPF_DRAGGING;
                    ci->flags |= CIF_MOUSEDOWN;
                };
            };

        } else if (ci->box == &(MR.req.req_cbox)) {

            /*** Right Mouse Button-Stuff... ***/
            if ((ci->flags&CIF_RMOUSEDOWN) && (ci->btn==MAPBTN_MAPINTERIOR)) {
                MR.flags |= MAPF_SCROLL;
                MR.scr_x       = ci->act.scrx;
                MR.scr_y       = ci->act.scry;
                MR.scr_oldmidx = MR.midx;
                MR.scr_oldmidz = MR.midz;
                ywd->DragLock = TRUE;
            };

            /*** Dragselect? ***/
            if ((ci->flags&CIF_MOUSEDOWN) && (ci->btn==MAPBTN_MAPINTERIOR)) {

                /*** NOTE: DragLock darf NICHT aktiviert werden! ***/
                MR.flags |= MAPF_DRAGGING;
                MR.drag_scr.xmin = ci->act.scrx;
                MR.drag_scr.ymin = ci->act.scry;
                MR.drag_scr.xmax = ci->act.scrx;
                MR.drag_scr.ymax = ci->act.scry;
                yw_MRGetDragCoords(ywd);

                /*** MouseDown MUSS geschluckt werden ***/
                ci->flags &= ~CIF_MOUSEDOWN;
            };

            /*** Buttonclick Sound ***/
            if ((ci->btn > MAPBTN_DRAGBAR) &&
                (ci->btn <= MAPBTN_ZIP) &&
                (ci->flags & CIF_BUTTONDOWN) &&
                (ywd->gsr))
            {
                _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
            };

            /*** immediate Left Buttons... ***/
            if (ci->flags & CIF_BUTTONDOWN) {

                switch (ci->btn) {

                    /*** Resize Button ***/
                    case MAPBTN_RESIZE:
                        MR.flags |= MAPF_RESIZE;
                        MR.rsz_x = req_w - ci->down.scrx;
                        MR.rsz_y = req_h - ci->down.scry;
                        ywd->DragLock = TRUE;
                        break;

                    /*** Scroll X Knob ***/
                    case MAPBTN_XSCROLLKNOB:
                        MR.flags      |= MAPF_SCROLLX;
                        MR.scr_x       = ci->down.scrx;
                        MR.scr_oldmidx = MR.midx;
                        ywd->DragLock = TRUE;
                        break;

                    /*** Scroll Y Knob ***/
                    case MAPBTN_YSCROLLKNOB:
                        MR.flags      |= MAPF_SCROLLY;
                        MR.scr_y       = ci->down.scry;
                        MR.scr_oldmidz = MR.midz;
                        ywd->DragLock = TRUE;
                        break;

                    /*** Landscape Layer toggeln ***/
                    case MAPBTN_LANDLAYER:
                        if (MR.layers & MAP_LAYER_LANDSCAPE)
                            MR.layers &= ~MAP_LAYER_LANDSCAPE;
                        else
                            MR.layers |= MAP_LAYER_LANDSCAPE;
                        yw_MapZoom(ywd, MAP_ZOOM_CORRECT);
                        break;

                    /*** Owner Layer toggeln ***/
                    case MAPBTN_OWNERLAYER:
                        if (MR.layers & MAP_LAYER_OWNER)
                            MR.layers &= ~MAP_LAYER_OWNER;
                        else
                            MR.layers |= MAP_LAYER_OWNER;
                        yw_MapZoom(ywd, MAP_ZOOM_CORRECT);
                        break;

                    /*** Height Layer toggeln ***/
                    case MAPBTN_HEIGHTLAYER:
                        if (MR.layers & MAP_LAYER_HEIGHT)
                            MR.layers &= ~MAP_LAYER_HEIGHT;
                        else
                            MR.layers |= MAP_LAYER_HEIGHT;
                        yw_MapZoom(ywd, MAP_ZOOM_CORRECT);
                        break;

                    /*** Lock On Viewer ***/
                    case MAPBTN_VIEWERLOCK:
                        if (MR.lock_mode == MAP_LOCK_VIEWER) MR.lock_mode = MAP_LOCK_NONE;
                        else                                 MR.lock_mode = MAP_LOCK_VIEWER;
                        break;
                };
            };

            /*** "echte" Buttons, die gecancelt werden können ***/
            switch(ci->btn) {

                /*** linker/rechter X-Scroll-Container ***/
                case MAPBTN_XSCROLLLEFT:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        FLOAT asp = (ywd->MapSizeX * SECTOR_SIZE) / MR.xscroll.size;
                        MR.scr_oldmidx = MR.midx;
                        yw_MRScrollX(ywd, -1, asp);
                    };
                    break;

                case MAPBTN_XSCROLLRIGHT:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        FLOAT asp = (ywd->MapSizeX * SECTOR_SIZE) / MR.xscroll.size;
                        MR.scr_oldmidx = MR.midx;
                        yw_MRScrollX(ywd, +1, asp);
                    };
                    break;

                /*** upper/lower-Y-Scroll-Container ***/
                case MAPBTN_YSCROLLTOP:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        FLOAT asp = (ywd->MapSizeY * SECTOR_SIZE) / MR.yscroll.size;
                        MR.scr_oldmidz = MR.midz;
                        yw_MRScrollY(ywd, -1, asp);
                    };
                    break;

                case MAPBTN_YSCROLLBOTTOM:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        FLOAT asp = (ywd->MapSizeY * SECTOR_SIZE) / MR.yscroll.size;
                        MR.scr_oldmidz = MR.midz;
                        yw_MRScrollY(ywd, +1, asp);
                    };
                    break;

                /*** Zip ***/
                case MAPBTN_ZIP:
                    if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                        MR.flags |= MAPF_ZIP_DOWN;
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        /*** ZipZap pressed ***/
                        struct ClickRect bup;
                        bup = MR.zip_zap;
                        MR.zip_zap = MR.req.req_cbox.rect;
                        MR.req.req_cbox.rect = bup;
                    };
                    break;

                /*** Help ***/
                case MAPBTN_HELP:
                    if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                        MR.req.flags |= REQF_HelpDown;
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        MR.req.flags &= ~REQF_HelpDown;
                        ywd->Url = ypa_GetStr(ywd->LocHandle,STR_HELP_INGAMEMAP,"help\\l14.html");
                    };
                    break;

                /*** Zoom In ***/
                case MAPBTN_ZOOMIN:
                    if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                        MR.flags |= MAPF_ZOOMIN_DOWN;
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        yw_MapZoom(ywd, MAP_ZOOM_IN);
                    };
                    break;

                /*** Zoom Out ***/
                case MAPBTN_ZOOMOUT:
                    if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                        MR.flags |= MAPF_ZOOMOUT_DOWN;
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        yw_MapZoom(ywd, MAP_ZOOM_OUT);
                    };
                    break;
            };

            /*** Tooltip-Messages ***/
            switch(ci->btn) {
                case MAPBTN_LANDLAYER:      yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_LAND,HOTKEY_MAP_LAND); break;
                case MAPBTN_OWNERLAYER:     yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_OWNER,HOTKEY_MAP_OWNER); break;
                case MAPBTN_HEIGHTLAYER:    yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_EXTINFO,HOTKEY_MAP_HEIGHT); break;
                case MAPBTN_VIEWERLOCK:     yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_LOCKVWR,HOTKEY_MAP_LOCKVWR); break;
                case MAPBTN_ZOOMIN:         yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_ZOOMIN,HOTKEY_MAP_ZOOMIN); break;
                case MAPBTN_ZOOMOUT:        yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_ZOOMOUT,HOTKEY_MAP_ZOOMOUT); break;
                case MAPBTN_ZIP:            yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP_SIZE,HOTKEY_MAP_MINIMIZE); break;
            };
        };
    } else {
        /*** wenn Map zu, alle kontinuierlichen Prozesse löschen ***/
        MR.flags &= ~(MAPF_RESIZE|MAPF_SCROLLX|MAPF_SCROLLY|MAPF_SCROLL|MAPF_DRAGGING);
    };
}

/*-----------------------------------------------------------------*/
void yw_RenderMap(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert die Map selbst und updatet die Scrollbars,
**      egal, ob GUI-Handling an oder aus ist. Das ganze
**      läßt sich nicht innerhalb yw_Layout#?() erledigen,
**      weil das direkte Input-Handling zu sehr verschliffen
**      wird.
**
**  INPUTS
**      ywd -> Ptr auf LID des Welt-Objects
**
**  RESULTS
**
**  CHANGED
**      06-Nov-95   floh    created
**      28-Nov-95   floh    + neue Standard-Requester-Flags
**      05-Feb-96   floh    revised & updated für neues Map-Handling
**                          (Layers)
*/
{
    if (!(MR.req.flags & REQF_Closed)) {

        /*** Mittelpunkt-Korrektur ***/
        yw_RefreshScrollerParams(ywd);
        yw_CheckMidPoint(ywd,MR.xscroll.size-MR.BorLeftW,MR.yscroll.size);

        /*** Visual GUI updaten ***/
        yw_MRLayoutReqString(ywd,FALSE);
        yw_MRLayoutXScroller(ywd);
        yw_MRLayoutYScroller(ywd);
        yw_MRLayoutMapInterior(ywd);
    };

    /*** Ende ***/
}

