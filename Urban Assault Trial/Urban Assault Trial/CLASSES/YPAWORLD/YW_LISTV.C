/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 16:22:32 $
**  $Locker: floh $
**  $Author: floh $
**
**  Support-Modul für Listview-Requester.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine

/*-----------------------------------------------------------------*/
BOOL yw_LVInitIconString(struct ypaworld_data *ywd,
                         struct YPAListReq *l,
                         UWORD icon_pos,
                         UBYTE icon_char)
/*
**  FUNCTION
**      Allokiert und initialisiert Icon-String für
**      Listview-Requester.
**
**  CHANGED
**      20-Nov-95   floh    created
**      28-Nov-95   floh    wertet jetzt LISTF_HasIcon aus...
**      27-Jul-96   floh    revised & updated
**      10-Dec-97   floh    nicht mehr supported
*/
{
    UBYTE *str;

    str = (UBYTE *) _AllocVec(32, MEMF_PUBLIC);
    if (str) {
        l->Req.icon_string = str;
        eos(str);
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LVInitReqString(struct ypaworld_data *ywd, struct YPAListReq *l)
/*
**  FUNCTION
**      Initialisiert Master-Req-String für ListView-Requester.
**
**  CHANGED
**      20-Nov-95   floh    created
**      29-Nov-95   floh    wertet jetzt LISTF_HasTitle aus...
**                          (es werden unterschiedliche Req-Strings
**                          erzeugt, je nachdem, ob der Req einen
**                          TitleBar hat oder nicht...)
**      20-Mar-96   floh    no more Popup-Gadget
**      27-Jul-96   floh    revised & updated
**      10-Dec-97   floh    + 64 war definitiv zu wenig fuer das 
**                            DBCS-Truetype-Zeug, also hoch auf 512.
*/
{
    UBYTE *str;

    str = (UBYTE *) _AllocVec(512, MEMF_PUBLIC);
    if (str) {

        WORD x, y, w;

        x = l->Req.req_cbox.rect.x - (ywd->DspXRes>>1);
        y = l->Req.req_cbox.rect.y - (ywd->DspYRes>>1);
        w = l->Req.req_cbox.rect.w;

        l->Req.req_string = str;

        if (l->Flags & LISTF_HasTitleBar) {
            /*** kompletten TitleBar aufbauen ***/
            str = yw_BuildReqTitle(ywd,x,y,w,l->TitleStr,str,0,l->Req.flags);
            new_line(str);
        } else {
            /*** kein TitleBar, nur Font + Pos-Init ***/
            new_font(str,FONTID_DEFAULT);
            pos_abs(str,x,y);
        };
        /*** die Clips includen, und fertig ***/
        clip(str,LV_CLIP_ITEMBLOCK);
        clip(str,LV_CLIP_SCROLLER);
        eos(str);

        return(TRUE);
    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LVInitScrollerString(struct ypaworld_data *ywd,
                             struct YPAListReq *l)
/*
**  FUNCTION
**      Es wird nur ein Buffer für den Scroller-String allokiert
**      und mit EOS initialisiert...
**
**  CHANGED
**      20-Nov-95   floh    created
**      27-Jul-96   floh    revised & updated
*/
{
    UBYTE *str;

    str = (UBYTE *) _AllocVec(256, MEMF_PUBLIC);
    if (str) {

        l->Scroller = str;
        eos(str);

        return(TRUE);
    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_LVInitItemBlockString(struct ypaworld_data *ywd,
                              struct YPAListReq *l)
/*
**  FUNCTION
**      Es wird nur ein 4kB großer Buffer allokiert und
**      mit EOS initialisiert.
**
**  CHANGED
**      20-Nov-95   floh    created
**      27-Jul-96   floh    revised & updated
**      10-Dec-97   floh    + Buffersize verdoppelt
*/
{
    UBYTE *str;

    str = (UBYTE *) _AllocVec(8192, MEMF_PUBLIC);
    if (str) {

        l->Itemblock = str;
        eos(str);

        return(TRUE);
    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_KillListView(struct ypaworld_data *ywd, struct YPAListReq *l)
/*
**  FUNCTION
**      Gibt alle am ListView-Requester hängenden Resourcen
**      sicher frei.
**      DARF NUR ALS GEGENSTUECK ZU yw_InitListView()
**      VERWENDET WERDEN!!!
**
**  CHANGED
**      20-Nov-95   floh    created
*/
{
    /*** das Clips-Array ***/
    if (l->Req.req_clips) {
        _FreeVec(l->Req.req_clips);
        l->Req.req_clips = NULL;
    };

    /*** die String-Buffer ***/
    if (l->Itemblock) {
        _FreeVec(l->Itemblock);
        l->Itemblock = NULL;
    };
    if (l->Scroller) {
        _FreeVec(l->Scroller);
        l->Scroller = NULL;
    };
    if (l->Req.req_string) {
        _FreeVec(l->Req.req_string);
        l->Req.req_string = NULL;
    };
    if (l->Req.icon_string) {
        _FreeVec(l->Req.icon_string);
        l->Req.icon_string = NULL;
    };

    /*** Button-Buffer killen ***/
    if (l->Req.req_cbox.buttons[0]) {
        _FreeVec(l->Req.req_cbox.buttons[0]);
        l->Req.req_cbox.buttons[0] = NULL;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_ListInitBuffers(struct ypaworld_data *ywd,
                        struct YPAListReq *l,
                        struct TagItem *tlist)
/*
**  FUNCTION
**      Initialisiert folgende String-Buffer:
**
**          l->Req.req_string       //
**          l->Req.icon_string      // EOS, falls kein Icon
**          l->Scroller             // EOS
**          l->ItemBlock            // EOS
**
**      Außerdem wird das Clips-Array
**
**          l->Req.req_clips
**
**      allokiert und korrekt initialisiert.
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      l       -> Ptr auf teilweise initialisierten YPAListReq
**      tlist   -> Pointer auf Input-TagList von yw_InitListView
**
**  RESULTS
**      FALSE   -> no mem
**
**  CHANGED
**      28-Nov-95   floh    created
**      27-Jul-96   floh    revised & updated
*/
{
    /*** TagList auswerten ***/
    UBYTE icon_pos  = (UBYTE) _GetTagData(LIST_IconPos, 0, tlist);
    UBYTE icon_char = (UBYTE) _GetTagData(LIST_IconChar, 0, tlist);

    /*** Icon-String ***/
    if (!yw_LVInitIconString(ywd, l, icon_pos, icon_char)) return(FALSE);

    /*** Req-String ***/
    if (!yw_LVInitReqString(ywd, l)) return(FALSE);

    /*** ItemBlock ***/
    if (!yw_LVInitItemBlockString(ywd, l)) return(FALSE);

    /*** Scroller-Buffer ***/
    if (!yw_LVInitScrollerString(ywd, l)) return(FALSE);

    /*** ClipsArray allokieren und initialisieren ***/
    l->Req.req_clips = (UBYTE **) _AllocVec(LV_NUMCLIPS * sizeof(UBYTE *),
                                  MEMF_PUBLIC|MEMF_CLEAR);

    if (l->Req.req_clips) {
        l->Req.req_clips[LV_CLIP_SCROLLER]  = l->Scroller;
        l->Req.req_clips[LV_CLIP_ITEMBLOCK] = l->Itemblock;
    } else return(FALSE);

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_ListSetRect(struct ypaworld_data *ywd, 
                    struct YPAListReq *l,
                    WORD xpos, WORD ypos)
/*
**  FUNCTION
**      Berechnet alle Values in
**
**          l->Req.req_cbox.rect
**
**      neu und korrigiert diese evtl. (z.B. bei Rand-Überschreitung).
**
**      Achtung, die Routine synchronisiert nicht die "visuelle"
**      Requester-Position im ReqString etc..., das müssen
**      die Layout-Routinen machen...
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      l       -> Ptr auf YPAListReq Struktur
**      xpos    -> neue X-Position (0..DisplayWidth)
**      ypos    -> neue Y-Position (0..DisplayHeight)
**
**      Folgende Pos-Sonderfälle existieren:
**
**          -1  -> der Req. wird zentriert
**          -2  -> Pos wird nicht geändert, nur die Breite/Höhe
**                 neu berechnet inkl. Randkorrektur.
**
**  RESULTS
**      ---
**
**  CHANGED
**      29-Nov-95   floh    created
**      01-Dec-95   floh    debugging -> Req-Width wurde falsch berechnet,
**                          wenn Resize-Gadget vorhanden.
**      11-Dec-95   floh    + VBorder
**      13-Dec-95   floh    debugging (Entscheidung, ob Scrollbar
**                          vorhanden ist mit ShownEntries, statt
**                          NumEntries).
**      27-Jul-96   floh    revised & updated
**      22-Jul-97   floh    + oberer und unterer VBorder getrennt
**      29-Jul-97   floh    + testet und korrigiert jetzt ActEntryWidth gegen
**                            MinEntryWidth und MaxEntryWidth
**                          + ... und ShownEntries/MinShown/MaxShown
**      11-Oct-97   floh    + Randcheck jetzt per ywd->UpperTabu und
**                            ywd->LowerTabu
*/
{
    WORD w,h;

    /*** Breite berechnen (evtl. Scrollerbreite mit einbeziehen) ***/
    if (l->ActEntryWidth < l->MinEntryWidth) l->ActEntryWidth=l->MinEntryWidth;
    if (l->ActEntryWidth > l->MaxEntryWidth) l->ActEntryWidth=l->MaxEntryWidth;
    if (l->ShownEntries < l->MinShown) l->ShownEntries=l->MinShown;
    if (l->ShownEntries > l->MaxShown) l->ShownEntries=l->MaxShown;
    w = l->ActEntryWidth;
    if ((l->Flags & LISTF_Resizeable) || (l->NumEntries > l->MaxShown)) {
        w += ywd->PropW;
    };

    /*** Höhe berechnen (evtl. TitleBar mit einbeziehen) ***/
    h = l->ShownEntries * l->EntryHeight + l->LowerVBorder + l->UpperVBorder;
    if (l->Flags & LISTF_HasTitleBar)  h += ywd->FontH;

    /*** X-Position ***/
    if (xpos == -1) {
        xpos = (ywd->DspXRes>>1) - (w>>1);
    } else if (xpos == -2) {
        xpos = l->Req.req_cbox.rect.x;
    };

    /*** Y-Position ***/
    if (ypos == -1) {
        ypos = (ywd->DspYRes>>1) - (h>>1);
    } else if (ypos == -2) {
        ypos = l->Req.req_cbox.rect.y;
    };

    /*** Rand-Korrektur (Statusbars beachten!) ***/
    if (xpos < 0) xpos = 0;
    if (ypos < ywd->UpperTabu) ypos=ywd->UpperTabu;
    if ((xpos + w) >= ywd->DspXRes) xpos = ywd->DspXRes - w;
    if ((ypos + h) >= (ywd->DspYRes-ywd->LowerTabu)) {
        ypos = ywd->DspYRes - ywd->LowerTabu - h;
    };

    l->Req.req_cbox.rect.x = xpos;
    l->Req.req_cbox.rect.y = ypos;
    l->Req.req_cbox.rect.w = w;
    l->Req.req_cbox.rect.h = h;

    /*** over & out ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_ListInitButtons(struct ypaworld_data *ywd,
                        struct YPAListReq *l)
/*
**  FUNCTION
**      Allokiert ein Array mit sovielen Buttons wie geht
**      und initialisiert sie. Standard-Buttons wie
**      Close, Drag etc... werden allokiert, aber nur
**      mit gültigen Werten versehen, wenn der Requester
**      einen TitleBar besitzt. Die Scroller-Buttons
**      werden nicht initialisiert, die ItemButtons
**      werden alle initialisiert (Vorsicht, die Breite
**      ist ja evtl. variabel).
**
**  CHANGED
**      29-Nov-95   floh    created
**      20-Mar-96   floh    no more Popup-Gadget
**      27-Jul-96   floh    revised & updated
**      22-Jul-97   floh    + LV_BTN_LOWVBORDER
*/
{
    /*** FYI: Es gibt 6 Standard-Buttons, Close, Drag,        ***/
    /*** ScrollTop, ScrollKnob, ScrollBottom und Resize. Ob   ***/
    /*** diese tatsächlich genutzt werden, ist hier           ***/
    /*** nebensächlich.                                       ***/

    ULONG size_button_array = (LV_MAXNUMITEMS + LV_NUM_STD_BTN + 1) *
                              sizeof(struct ClickButton);
    struct ClickButton *btn;
    ULONG i;

    /*** Init-Anzahl Buttons in Req ***/
    l->Req.req_cbox.num_buttons = LV_NUM_STD_BTN + l->ShownEntries;

    /*** mit { 0,0,0,0 } initialisiertes ClickButton-Array ***/
    btn = (struct ClickButton *) 
          _AllocVec(size_button_array, MEMF_PUBLIC|MEMF_CLEAR);
    if (!btn) return(FALSE);

    /*** Req-Button-Pointer eintragen ***/
    for (i=0; i<(LV_MAXNUMITEMS + LV_NUM_STD_BTN); i++) {
        l->Req.req_cbox.buttons[i] = &(btn[i]);
    };

    /*** Falls vorhanden, Icon-Button-Ptr eintragen ***/
    if (l->Flags & LISTF_HasIcon) {

        l->Req.icon_cbox.num_buttons = 1;
        l->Req.icon_cbox.buttons[0]  = &(btn[LV_MAXNUMITEMS + LV_NUM_STD_BTN]);

        l->Req.icon_cbox.buttons[0]->rect.x = 0;
        l->Req.icon_cbox.buttons[0]->rect.y = 0;
        l->Req.icon_cbox.buttons[0]->rect.w = 16;
        l->Req.icon_cbox.buttons[0]->rect.h = 16;

    } else {
        l->Req.icon_cbox.num_buttons = 0;
    };

    /*** der Rest wird von den Layout-Routinen übernommen ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_LVLayoutTitle(struct ypaworld_data *ywd,
                      struct YPAListReq *l)
/*
**  FUNCTION
**      Layouter für TitleBar. Modifiziert (so vorhanden):
**
**          - Stretch-Value für DragBar
**          - Button-Strukturen werden korrekt ausgefüllt
**
**      Die Routine benötigt eine korrekte Requester-Ausdehnung
**      in l->Req.req_cbox.rect
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      29-Nov-95   floh    created
**      01-Dec-95   floh    vergessen, Req-String neu zu positionieren!
**      20-Mar-96   floh    no more Popup-Gadget
**      27-Jul-96   floh    revised & updated (baut TitleString,
**                          bzw. ReqString komplett neu auf).
**      30-Jul-96   floh    BugFix: Button-X des Dragbar war noch auf 8
**                          fest.
**      05-Jul-97   floh    Closebutton oben rechts
**      29-Oct-97   floh    + Help-Button
**      22-Apr-98   floh    + Help- und Closebuttons sind jetzt optional
*/
{
    if (l->Flags & LISTF_HasTitleBar) {

        struct ClickButton **btn = &(l->Req.req_cbox.buttons[0]);
        UBYTE *str = l->Req.req_string;
        WORD w     = l->Req.req_cbox.rect.w;
        WORD xpos  = l->Req.req_cbox.rect.x - (ywd->DspXRes>>1);
        WORD ypos  = l->Req.req_cbox.rect.y - (ywd->DspYRes>>1);
        WORD close_w,help_w;
        if (l->Req.flags & REQF_HasCloseGadget) close_w = ywd->CloseW;
        else                                    close_w = 0;
        if (l->Req.flags & REQF_HasHelpGadget)  help_w  = ywd->CloseW;
        else                                    help_w  = 0;

        if (l->Flags & LISTF_HasTitleBar) {
            /*** kompletten TitleBar aufbauen ***/
            str = yw_BuildReqTitle(ywd,xpos,ypos,w,l->TitleStr,str,0,l->Req.flags);
            new_line(str);
        } else {
            /*** kein TitleBar, nur Font + Pos-Init ***/
            new_font(str,FONTID_DEFAULT);
            pos_abs(str,xpos,ypos);
        };
        /*** die Clips includen, und fertig ***/
        clip(str,LV_CLIP_ITEMBLOCK);
        clip(str,LV_CLIP_SCROLLER);
        eos(str);

        btn[LV_BTN_DRAG]->rect.x = 0;
        btn[LV_BTN_DRAG]->rect.y = 0;
        btn[LV_BTN_DRAG]->rect.w = w - close_w - help_w;
        btn[LV_BTN_DRAG]->rect.h = ywd->FontH;


        if (l->Req.flags & REQF_HasHelpGadget) {
            btn[LV_BTN_HELP]->rect.x = w - close_w - help_w;
            btn[LV_BTN_HELP]->rect.y = 0;
            btn[LV_BTN_HELP]->rect.w = close_w;
            btn[LV_BTN_HELP]->rect.h = ywd->FontH;
        } else {
            btn[LV_BTN_HELP]->rect.x = 0;
            btn[LV_BTN_HELP]->rect.y = 0;
            btn[LV_BTN_HELP]->rect.w = 0;
            btn[LV_BTN_HELP]->rect.h = 0;
        };
        if (l->Req.flags & REQF_HasCloseGadget) {                       
            btn[LV_BTN_CLOSE]->rect.x = w - close_w;
            btn[LV_BTN_CLOSE]->rect.y = 0;
            btn[LV_BTN_CLOSE]->rect.w = close_w;
            btn[LV_BTN_CLOSE]->rect.h = ywd->FontH;
        } else {
            btn[LV_BTN_CLOSE]->rect.x = 0;
            btn[LV_BTN_CLOSE]->rect.y = 0;
            btn[LV_BTN_CLOSE]->rect.w = 0;
            btn[LV_BTN_CLOSE]->rect.h = 0;
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_ScrollerParamsFromEntries(struct ypaworld_data *ywd,
                                  struct YPAListReq *l)
/*
**  FUNCTION
**      Berechnet
**
**          l->ScrSize
**          l->KnobSize
**          l->KnobStart
**
**      aus
**
**          l->Req.req_cbox.rect.h
**          l->ShownEntries
**          l->NumEntries
**          l->FirstShown
**
**      Inklusive Korrekturen.
**
**  CHANGED
**      02-Dec-95   floh    created
**      19-Mar-96   floh    (NumEntries == 0) jetzt möglich
**      27-Jul-96   floh    revised & updated
*/
{
    LONG scrsize;   // Größe des Scroll-Containers in Pixel
    LONG knobsize;  // Größe des Knobs in Pixel
    LONG knobstart; // Start des Knobs relativ zu Container-Top

    /*** Größe des Scroll-Containers ermitteln ***/
    scrsize = l->Req.req_cbox.rect.h;
    if (l->Flags & LISTF_HasTitleBar) scrsize -= ywd->FontH; // TitleHeight
    if (l->Flags & LISTF_Resizeable)  scrsize -= ywd->PropH; // ResizeHeight

    /*** Knob-Params ermitteln ***/
    if (l->NumEntries == 0) {
        knobsize  = scrsize;
        knobstart = 0;
    } else {
        knobsize  = (scrsize * l->ShownEntries)/l->NumEntries;
        knobstart = (scrsize * l->FirstShown)/l->NumEntries;
    };

    /*** MiniMax-Korrektur ***/
    if (knobsize < 2) {
        knobstart -= (2-knobsize)>>1;
        knobsize   = 2;
    } else if (knobsize > scrsize) knobsize = scrsize;
    if (knobstart < 0) knobstart = 0;
    if ((knobstart + knobsize) > scrsize) knobstart = scrsize-knobsize;

    /*** nach <l> übernehmen ***/
    l->ScrSize   = scrsize;
    l->KnobSize  = knobsize;
    l->KnobStart = knobstart;
}

/*-----------------------------------------------------------------*/
void yw_EntriesFromScrollerParams(struct ypaworld_data *ywd,
                                  struct YPAListReq *l)
/*
**  FUNCTION
**      Berechnet:
**
**          l->FirstShown
**
**      aus:
**          l->KnobStart
**
**      Sollte nur während des Scrollings benutzt werden.
**      (damit wird ein Springen des Scrollers verhindert)
**
**  CHANGED
**      02-Dec-95   floh    created
**      19-Mar-96   floh    ShownEntries darf jetzt größer NumEntries 
**                          sein
**      27-Jul-96   floh    revised & updated
*/
{
    l->FirstShown = (l->NumEntries * l->KnobStart) / l->ScrSize;

    /*** unwahrscheinlich, aber trotzdem ggfls. Korrektur ***/
    if ((l->FirstShown + l->ShownEntries) > l->NumEntries) {
        l->FirstShown = l->NumEntries - l->ShownEntries;
        if (l->FirstShown < 0) l->FirstShown = 0;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_LVLayoutScroller(struct ypaworld_data *ywd,
                         struct YPAListReq *l)
/*
**  FUNCTION
**      Layoutet Scroller-String inklusive Resize-Gadget.
**      Falls der Requester nicht resizeable ist wird das
**      Scroller-Teil ganz weggelassen, wenn er resizeable
**      ist immer gezeichnet.
**
**  CHANGED
**      29-Nov-95   floh    created
**      01-Dec-95   floh    miminale Size auf 2 Pixel gesetzt
**      02-Dec-95   floh    ruft jetzt, je nach Scrolling-Status,
**                          yw_ScrollerParamsFromEntries() oder
**                          yw_EntriesFromScrollerParams() auf
**      13-Dec-95   floh    debugging -> Entscheidung ob Scroller
**                          gezeichnet werden muß, siehe yw_ListSetRect()
**      27-Jul-96   floh    revised & updated
**      29-Jul-96   floh    es war noch eine Konstante drin, die
**                          nicht "Dynamic Layout" konform war, nämlich
**                          die Höhe des Scroller-Strings (hatte ich
**                          mit 32 angenommen).
**      30-Jul-96   floh    benutzt jetzt ywd->VPropH als Layout-Variable
**      22-Jul-97   floh    + initialisiert jetzt auch den neuen
**                            LV_BTN_LOWVBORDER Button aus
**      07-Apr-98   floh    + Scrollbar wird jetzt immer gezeichnet
*/
{
    UBYTE *str = l->Scroller;
    struct ClickButton **btn = &(l->Req.req_cbox.buttons[0]);
    LONG size;
    WORD xpos,ypos; // X/Y-Position des Scrollers absolut

    /*** OBSOLETE: Muß Scrollbar gezeichnet werden? ***/
    if ((l->Flags & LISTF_Resizeable) || (l->NumEntries > l->MaxShown)) {

        /*** Berechne Scroller-Params oder FirstShown ***/
        if (l->Flags & LISTF_Scrolling) {

            /*** FirstShown neu ermitteln (während Scrolling) ***/
            yw_EntriesFromScrollerParams(ywd, l);

        } else {

            /*** ScrollerParams aus Entry-Params ermitteln ***/
            yw_ScrollerParamsFromEntries(ywd, l);

        };

        /*** X/Y-Position des Scrollers ***/
        xpos  = l->Req.req_cbox.rect.w;
        xpos -= ywd->PropW;  // ScrollerWidth

        ypos  = 0;
        if (l->Flags & LISTF_HasTitleBar) {
            ypos += ywd->FontH; // TitleHeight
        };

        /*** die Button-Strukturen updaten ***/
        btn[LV_BTN_SCROLLTOP]->rect.x = xpos;
        btn[LV_BTN_SCROLLTOP]->rect.y = ypos;
        btn[LV_BTN_SCROLLTOP]->rect.w = ywd->PropW;     // ScrollerWidth
        btn[LV_BTN_SCROLLTOP]->rect.h = l->KnobStart;

        btn[LV_BTN_SCROLLKNOB]->rect.x = xpos;
        btn[LV_BTN_SCROLLKNOB]->rect.y = ypos + l->KnobStart;
        btn[LV_BTN_SCROLLKNOB]->rect.w = ywd->PropW;    // ScrollerWidth
        btn[LV_BTN_SCROLLKNOB]->rect.h = l->KnobSize;

        btn[LV_BTN_SCROLLBOTTOM]->rect.x = xpos;
        btn[LV_BTN_SCROLLBOTTOM]->rect.y = ypos + l->KnobStart + l->KnobSize;
        btn[LV_BTN_SCROLLBOTTOM]->rect.w = ywd->PropW;  // ScrollerWidth
        btn[LV_BTN_SCROLLBOTTOM]->rect.h = l->ScrSize - (l->KnobStart + l->KnobSize);

        btn[LV_BTN_LOWVBORDER]->rect.x = 0;
        btn[LV_BTN_LOWVBORDER]->rect.y = l->Req.req_cbox.rect.h - l->LowerVBorder;
        btn[LV_BTN_LOWVBORDER]->rect.w = l->Req.req_cbox.rect.w - ywd->PropW;
        btn[LV_BTN_LOWVBORDER]->rect.h = l->LowerVBorder;

        if (l->Flags & LISTF_Resizeable) {
            btn[LV_BTN_RESIZE]->rect.x = xpos;
            btn[LV_BTN_RESIZE]->rect.y = ypos + l->ScrSize;
            btn[LV_BTN_RESIZE]->rect.w = ywd->PropW;    // ScrollerWidth
            btn[LV_BTN_RESIZE]->rect.h = ywd->PropH;    // ResizeHeight
        } else {
            memset(btn[LV_BTN_RESIZE], 0, sizeof(struct ClickButton));
        };

        /*** String aufbauen ***/
        xpos += l->Req.req_cbox.rect.x;     // absolutisieren
        xpos -= ywd->DspXRes >> 1;          // Nullpunkt in Screenmitte
        ypos += l->Req.req_cbox.rect.y;     // absolutisieren
        ypos -= ywd->DspYRes >> 1;          // NullPunkt in Screenmitte
        pos_abs(str,xpos,ypos);

        /*** der obere Teil des Scrollers ***/
        size = l->KnobStart;
        if (size > 0) {

            /* Background, Top */
            newfont_flush(str,FONTID_MAPVERT1);
            put(str,'C');
            new_line(str);

            size -= 1;

            /* Background, Top, Inneres */
            newfont_flush(str,FONTID_MAPVERT);
            while (size >= ywd->VPropH) {
                put(str,'B');
                new_line(str);
                size -= ywd->VPropH;
            };
            if (size > 0) {
                len_vert(str,size);
                put(str,'B');
                new_line(str);
            };
        };

        /*** der Scroller-Knob ***/
        size = l->KnobSize;

        if (size > 0) {
            /* Knob, Top */
            newfont_flush(str,FONTID_MAPVERT1);
            put(str,'E');
            new_line(str);

            size -= 1;

            /*** Knob, Inneres ***/
            newfont_flush(str,FONTID_MAPVERT);
            while (size > ywd->VPropH) {
                put(str,'C');
                new_line(str);
                size -= ywd->VPropH;
            };
            if (size > 1) {
                len_vert(str,(size-1));
                put(str,'C');
                new_line(str);
            };
        };

        /* Knob, Bottom */
        newfont_flush(str,FONTID_MAPVERT1);
        put(str,'F');
        new_line(str);

        /*** Scroller-Bottom ***/
        size = l->ScrSize - (l->KnobStart + l->KnobSize);
        if (size > 0) {

            /* Bottom-Inneres */
            newfont_flush(str,FONTID_MAPVERT);
            while (size > ywd->VPropH) {
                put(str,'B');
                new_line(str);
                size -= ywd->VPropH;
            };
            if (size > 1) {
                len_vert(str,(size-1));
                put(str,'B');
                new_line(str);
            };
            /* Bottom-Rand */
            newfont_flush(str,FONTID_MAPVERT1);
            put(str,'D');
            new_line(str);
        };

        /*** Resize-Gadget ***/
        if (l->Flags & LISTF_Resizeable) {
            newfont_flush(str,FONTID_MAPHORZ);
            put(str,'G');
        };

        /*** EOS ***/
        eos(str);

    } else {

        /*** alle Scroller-relevanten ClickButtons disablen ***/
        memset(btn[LV_BTN_SCROLLTOP],0,sizeof(struct ClickButton));
        memset(btn[LV_BTN_SCROLLKNOB],0,sizeof(struct ClickButton));
        memset(btn[LV_BTN_SCROLLBOTTOM],0,sizeof(struct ClickButton));
        memset(btn[LV_BTN_RESIZE],0,sizeof(struct ClickButton));
        memset(btn[LV_BTN_LOWVBORDER],0,sizeof(struct ClickButton));

        /*** ein EOS an den Anfang des Scroller-Strings ***/
        eos(str);
    };

    /*** Uff... ***/
}

/*-----------------------------------------------------------------*/
void yw_LVLayoutItemBlock(struct ypaworld_data *ywd,
                          struct YPAListReq *l)
/*
**  FUNCTION
**      Im Unterschied zu den anderen Layout-Funktionen
**      layoutet diese nicht den ItemBlock-String, sondern
**      nur die ClickButton-Strukturen der Items.
**
**  CHANGED
**      29-Nov-95   floh    created
**      11-Dec-95   floh    + VBorder Beachtung
**      12-Dec-95   floh    + Anpassung von <num_buttons>
**      14-Dec-95   floh    Debugging... aus mir unerfindlichen
**                          Gründen habe ich für die Höhe eines Buttons
**                          die Konstante "LV_RESIZEHEIGHT" statt
**                          l->EntryHeight verwendet :-/
**      27-Jul-96   floh    revised & updated
**      22-Jul-97   floh    + oberer und untere Rand getrennt
*/
{
    struct ClickButton **btn = &(l->Req.req_cbox.buttons[0]);
    ULONG i;
    WORD ypos;

    /*** NumButtons anpassen... ***/
    l->Req.req_cbox.num_buttons = LV_NUM_STD_BTN + l->ShownEntries;

    /*** Start-Y-Position des ItemBlocks ***/
    ypos = l->UpperVBorder;
    if (l->Flags & LISTF_HasTitleBar) ypos+=ywd->FontH; // TitleHeight

    for (i=0; i<l->ShownEntries; i++) {
        btn[LV_NUM_STD_BTN+i]->rect.x = 0;
        btn[LV_NUM_STD_BTN+i]->rect.y = ypos + i*l->EntryHeight;
        btn[LV_NUM_STD_BTN+i]->rect.w = l->ActEntryWidth;
        btn[LV_NUM_STD_BTN+i]->rect.h = l->EntryHeight;
    };

    /*** dat war's (?) ***/
}

/*-----------------------------------------------------------------*/
UBYTE *yw_LVItemsPreLayout(struct ypaworld_data *ywd,
                           struct YPAListReq *m,
                           UBYTE *str, UBYTE fnt, UBYTE *chrs)
/*
**  FUNCTION
**      Support-Funktion, layoutet den oberen Rand eines
**      Listview-Itemblocks.
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      m       - Ptr auf gültiges Listview
**      str     - Ptr auf Output-Stream
**      fnt     - zu verwendende Font-ID
**      chrs    - String mit den zu verwenden Chars im Font:
**
**                  chrs[0] = Ecke, links oben (2 Pixel breit)
**                  chrs[1] = oberer Rand
**                  chrs[2] = Ecke, rechts oben (2 Pixel breit)
**
**  RESULTS
**      str - Pointer auf modifizierten Output-Stream
**
**  CHANGED
**      19-Mar-96   floh    erzeugt aus yw_statusbar.c/yw_MenuPreLayout()
**      27-Jul-96   floh    revised & updated
**      22-Jul-97   floh    + oberer und unterer Rand getrennt
**
**  NOTE
**      Der verwendete Font muß mindestens so hoch sein, wie
**      der VBorder-Wert des Requesters. Außerdem muß die
**      Fonthöhe mit dem EntryHeight-Wert des Requesters
**      übereinstimmen!
**
**      Der String wird mit einem Newline abgeschlossen.
**
**      Die resultierende Länge darf > 256 sein!
*/
{
    WORD xpos,ypos,w;

    xpos = m->Req.req_cbox.rect.x - (ywd->DspXRes>>1);
    ypos = m->Req.req_cbox.rect.y - (ywd->DspYRes>>1);
    w    = m->ActEntryWidth;

    if (m->Flags & LISTF_HasTitleBar) ypos += ywd->FontH;   // TitleHeight

    /*** Font, Positionierung ***/
    newfont_flush(str,fnt);
    pos_abs(str,xpos,ypos);

    /*** oberen Rand zeichnen ***/
    len_vert(str,m->UpperVBorder);
    put(str,*chrs++);                   // Ecke links oben
    lstretch_to(str,(w-ywd->EdgeW));
    put(str,*chrs++);                   // Rand oben
    put(str,*chrs);                     // Ecke rechts oben
    new_line(str);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_LVItemsPostLayout(struct ypaworld_data *ywd,
                            struct YPAListReq *m,
                            UBYTE *str, UBYTE fnt, UBYTE *chrs)
/*
**  FUNCTION
**      Wie yw_LVItemsPreLayout(), nur für unteren
**      Menu-Rand, ansonsten GELTEN DIE SELBEN BEDINGUNGEN
**      WIE FÜR yw_LVItemsPreLayout()!!!!!
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      m       - Ptr auf gültiges Listview
**      str     - Ptr auf Output-Stream
**      fnt     - zu verwendende Font-ID
**      chrs    - String mit den zu verwenden Chars im Font:
**
**                  chrs[0] = Ecke, links unten (2 Pixel breit)
**                  chrs[1] = untere Rand
**                  chrs[2] = Ecke, rechts unten (2 Pixel breit)
**
**  CHANGED
**      19-Mar-96   floh    created
**      27-Jul-96   floh    revised & updated
**      21-Aug-96   floh    + Bugfix: Font wurde nicht initialisiert
**                          + uggh, böser Bug, statt der "echten" Font-Höhe
**                            für die EntryHeight genommen... Bogus^3!
**      22-Jul-97   floh    + oberer und unterer Rand getrennt
**
**  NOTE
**      Die resultierende Länge darf > 256 sein!
*/
{
    WORD w = m->ActEntryWidth;
    newfont_flush(str,fnt);
    off_vert(str,(ywd->Fonts[fnt]->height - m->LowerVBorder));
    put(str,*chrs++);   // Ecke links unten
    lstretch_to(str,(w-ywd->EdgeW));
    put(str,*chrs++);   // Rand unten
    put(str,*chrs);     // Ecke rechts unten
    return(str);
}

/*-----------------------------------------------------------------*/
BOOL yw_InitListView(struct ypaworld_data *ywd,
                     struct YPAListReq *l,
                     ULONG tags, ...)
/*
**  FUNCTION
**      Initialisiert eine YPAListReq Struktur so weit wie's
**      geht.
**
**      Die Routine räumt im Fehlerfalle hinter sich auf,
**      es ist aber sicher, in jedem Zustand yw_KillListView() auf die
**      Struktur anzuwenden.
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**      l       -> Ptr auf statische YPAListReq-Struktur
**      tags    -> Taglist, siehe ypa/guilist.h
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> out of mem
**
**  CHANGED
**      19-Nov-95   floh    created
**      28-Nov-95   floh    praktisch neu...
**      11-Dec-95   floh    + VBorder
**      13-Dec-95   floh    + Immediate Input
**      07-Jan-96   floh    Bugfix: return(TRUE) vergessen.
**      19-Mar-96   floh    + LIST_MinShown
**                          + bei Resizeable-Gadgets definiert MaxShown die
**                            maximale Höhe des Reqs, weil der Scrollbar
**                            ja eh immer gezeichnet wird!,
**      19-Jun-96   floh    + LIST_StaticItems-Attribut (def = FALSE);
**      27-Jul-96   floh    revised & updated
**      10-Aug-96   floh    + LIST_CloseChar
**      18-Sep-96   floh    + <MouseItem> Init
**      22-Jul-97   floh    + oberer und unterer Rand können
**                            jetzt getrennt angegeben werden
**      17-Feb-98   floh    + LISTF_KeyboardInput Flag
**      23-Apr-98   floh    + HelpGadget jetzt optional
*/
{
    struct TagItem *tlist = (struct TagItem *) &(tags);
    ULONG has_help_gadget;

    /*** Tags abfragen ***/
    UBYTE *title    = (UBYTE *) _GetTagData(LIST_Title, NULL, tlist);
    BOOL do_resize  = (BOOL)  _GetTagData(LIST_Resize, FALSE, tlist);
    BOOL do_icon    = (BOOL)  _GetTagData(LIST_DoIcon, FALSE, tlist);
    UBYTE icon_pos  = (UBYTE) _GetTagData(LIST_IconPos, 0, tlist);
    BOOL enabled    = (BOOL)  _GetTagData(LIST_Enabled, FALSE, tlist);
    BOOL immediate  = (BOOL)  _GetTagData(LIST_ImmediateInput, FALSE, tlist);
    BOOL stat_items = (BOOL)  _GetTagData(LIST_StaticItems, FALSE, tlist);
    BOOL keyboard   = (BOOL)  _GetTagData(LIST_KeyboardInput, FALSE,tlist);

    l->CloseChar    = (ULONG) _GetTagData(LIST_CloseChar, 'A', tlist);
    l->NumEntries   = (WORD)  _GetTagData(LIST_NumEntries, 0, tlist);
    l->ShownEntries = (WORD)  _GetTagData(LIST_ShownEntries, 0, tlist);
    l->FirstShown   = (WORD)  _GetTagData(LIST_FirstShown, 0, tlist);
    l->Selected     = (WORD)  _GetTagData(LIST_Selected, 0, tlist);
    l->MaxShown     = (WORD)  _GetTagData(LIST_MaxShown, 0, tlist);
    l->MinShown     = (WORD)  _GetTagData(LIST_MinShown, 0, tlist);
    l->EntryHeight  = (WORD)  _GetTagData(LIST_EntryHeight, ywd->FontH, tlist);
    l->ActEntryWidth = (WORD) _GetTagData(LIST_EntryWidth, 80, tlist);
    l->MinEntryWidth = (WORD) _GetTagData(LIST_MinEntryWidth, 60, tlist);
    l->MaxEntryWidth = (WORD) _GetTagData(LIST_MaxEntryWidth, 1024, tlist);
    l->UpperVBorder = (WORD) _GetTagData(LIST_UpperVBorder,0,tlist);
    l->LowerVBorder = (WORD) _GetTagData(LIST_LowerVBorder,0,tlist);
    has_help_gadget = (LONG) _GetTagData(LIST_HasHelpGadget,TRUE,tlist);
    if ((l->UpperVBorder==0)&&(l->LowerVBorder==0)) {
        l->UpperVBorder = (WORD) _GetTagData(LIST_VBorder,0,tlist);
        l->LowerVBorder = l->UpperVBorder;
    };
    l->MouseItem = -1;
    
    /*** Listview-Flags setzen ***/
    l->Flags = 0;
    if (enabled) l->Flags |= LISTF_Enabled;
    if (title) {
        l->Flags |= LISTF_HasTitleBar;
        strncpy(l->TitleStr,title,sizeof(l->TitleStr));
    };
    if (do_resize)   l->Flags |= LISTF_Resizeable;
    if (immediate)   l->Flags |= LISTF_Immediate;
    if (stat_items)  l->Flags |= LISTF_Static;
    if (keyboard)    l->Flags |= LISTF_KeyboardInput;
    if (do_icon && (l->Flags & LISTF_HasTitleBar)) l->Flags |= LISTF_HasIcon;

    /*** Requester-Flags setzen ***/
    l->Req.flags = 0;
    if (l->Flags & LISTF_HasIcon) {
        l->Req.flags |= (REQF_HasIcon|REQF_Iconified);
    } else {
        l->Req.flags |= REQF_Closed;
    };

    if (l->Flags & LISTF_HasTitleBar) {
        l->Req.flags |= (REQF_HasDragBar|REQF_HasCloseGadget);
        if (has_help_gadget) l->Req.flags |= REQF_HasHelpGadget;
    };

    /*** Requester-Part initialisieren ***/
    if (l->Flags & LISTF_HasIcon) {
        
        l->Req.icon_cbox.rect.x = icon_pos * 16;
        l->Req.icon_cbox.rect.y = ywd->DspYRes - 32;
        l->Req.icon_cbox.rect.w = 16;
        l->Req.icon_cbox.rect.h = 16;
        l->Req.icon_cbox.num_buttons = 1;

    } else {

        /*** es ist eh kein Icon da ... ***/
        l->Req.icon_cbox.rect.x = 0;
        l->Req.icon_cbox.rect.y = 0;
        l->Req.icon_cbox.rect.w = 0;
        l->Req.icon_cbox.rect.h = 0;
        l->Req.icon_cbox.num_buttons = 0;
    };

    /*** Puffer allokieren und (teilweise) ausfüllen ***/
    if (!yw_ListInitBuffers(ywd, l, tlist)) return(FALSE);

    /*** Init-Position/Size absolut setzen (inkl. Korrektur) ***/
    yw_ListSetRect(ywd, l, -1, -1);

    /*** Button-Zeugs initialisieren ***/
    if (!yw_ListInitButtons(ywd, l)) return(FALSE);

    /*** Subclips layouten ***/
    yw_LVLayoutTitle(ywd, l);
    yw_LVLayoutScroller(ywd, l);
    yw_LVLayoutItemBlock(ywd, l);

    /*** den ItemBlock-Clip-Inhalt MUSS der Client initialisieren ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_ListHandleInput(struct ypaworld_data *ywd,
                        struct YPAListReq *l,
                        struct VFMInput *ip)
/*
**  FUNCTION
**      Basis-Inputhandler für Listen-Requester, handelt
**      (falls existent) folgende Sachen ab:
**
**      - Resize
**      - Scrolling
**
**      - yw_LVLayoutTitle()
**        yw_LVLayoutScroller()
**        yw_LVLayoutItemBlock()
**
**      Die Routine darf nur aufgerufen werden, wenn der
**      Listen-Requester auch tatsächlich offen ist!!!
**
**  INPUTS
**      ywd     -> Ptr auf LID des WeltObjects
**      l       -> Ptr auf Listen-Requester
**      ip      -> Ptr auf gültige VFMInput-Struktur
**
**  RESULTS
**      ---
**
**  CHANGED
**      01-Dec-95   floh    created
**      14-Dec-95   floh    debugging (EntryHeight)
**      21-Dec-95   floh    sobald ein MouseClick außerhalb der
**                          ClickBox erfolgt und der Requester
**                          im Immediate Modus ist, wird er
**                          geschlossen.
**      30-Dec-95   floh    debugging...
**      31-Dec-95   floh    STAT_HEIGHT breiter Tabu-Streifen oben und unten
**      19-Mar-96   floh    Minimal/Maximal-Korrektur bei Resizing richtet
**                          sich nicht mehr nach Entries, sondern nach
**                          den übergebenen MinHeight/MaxHeight-Values
**      21-Mar-96   floh    Scrolling jetzt auch per Scroll-Container
**      10-May-96   floh    beim Drag-Scrolling und Resizing wird jetzt das
**                          DragLock-Flag gesetzt, damit der Mauspointer
**                          identisch bleibt.
**      27-Jul-96   floh    revised & updated
**      18-Sep-96   floh    + MouseItem wird ausgefüllt
**      30-May-97   floh    + LISTF_SelectDone Flag, wird ausgefüllt,
**                            wenn Maus über einem gültigen Item
**                            losgelassen!
**      22-Jul-97   floh    + oberer und unterer Rand getrennt
**      15-Aug-97   floh    + wenn das Listview geschlossen ist, werden
**                            alle kontinuierlichen Prozesse (Select,
**                            Resize etc...) abgebrochen, damit die
**                            Maus nicht in diesem Mode hängenbleibt,
**                            wenn das Fenster geschlossen wird. Das heißt
**                            aber, daß yw_ListHandleInput() jetzt auch
**                            bei geschlossenem Requester aufgerufen
**                            werden muß!
**      17-Feb-98   floh    + wenn Keyboard-Focus-Flag gesetzt ist, wird
**                            Keyup, Keydown, Escape und Enter beachtet.
*/
{
    if (!(l->Req.flags & (REQF_Iconified|REQF_Closed))) {

        struct ClickInfo *ci = &(ip->ClickInfo);

        WORD req_x = l->Req.req_cbox.rect.x;
        WORD req_y = l->Req.req_cbox.rect.y;
        WORD req_w = l->Req.req_cbox.rect.w;
        WORD req_h = l->Req.req_cbox.rect.h;

        l->MouseItem = -1;
        l->Flags &= ~LISTF_SelectDone;

        /*** Keyboard-Handling, so aktiviert ***/
        if (l->Flags & LISTF_KeyboardInput) {
            switch (ip->NormKey) {
                case KEYCODE_CURSOR_DOWN:
                    l->Selected++;
                    if (l->Selected >= l->NumEntries) {
                        /*** Überlauf, auf Anfang zurücksetzen ***/
                        l->Selected   = 0;
                        l->FirstShown = 0;
                    };
                    if (l->Selected >= (l->FirstShown + l->ShownEntries - 1)) {
                        /*** ist Selected aus sichtbarem Bereich raus? ***/
                        l->FirstShown = l->Selected - l->ShownEntries + 1;
                    };
                    break;

                case KEYCODE_CURSOR_UP:
                    l->Selected--;
                    if (l->Selected < 0) {
                        l->Selected   = l->NumEntries-1;
                        l->FirstShown = l->NumEntries-l->ShownEntries;
                    };
                    if (l->Selected < l->FirstShown) {
                        l->FirstShown = l->Selected;
                    };
                    break;
            };
        };

        /*** DragLock Handling (Mauspointer auf Normal zwingen) ***/
        if ((l->Flags & (LISTF_Resizing|LISTF_Scrolling|LISTF_Select)) &&
            (ci->flags & CIF_MOUSEHOLD))
        {
            ywd->DragLock = TRUE;
        } else {
            ywd->DragLock = FALSE;
        };

        /*** LISTF_NoScroll löschen, wenn Maus über einem Entry ***/
        if ((ci->box == &(l->Req.req_cbox)) && (ci->btn >= LV_NUM_STD_BTN)) {
            /*** bei Menü-Type-Listviews anzeigen, daß Maus zum  ***/
            /*** 1.Mal in der ItemBox ist, ab jetzt kann also    ***/
            /*** gescrollt werden, wenn die Maus oben oder unten ***/
            /*** rausrutscht.                                    ***/
            l->Flags &= ~LISTF_NoScroll;
        };

        /*** Input Handling ***/
        if (l->Flags & LISTF_Resizing) {

            /*** Resizing underway ***/
            if (ci->flags & CIF_MOUSEHOLD) {

                WORD new_w, new_h;
                WORD max_w, max_h;
                WORD min_w, min_h;

                /*** die neue absolute Höhe/Breite ***/
                new_w = l->RszX + ci->act.scrx;
                new_h = l->RszY + ci->act.scry;

                /*** Maximal-Ausdehnung berechnen ***/
                max_w = l->MaxEntryWidth+ywd->PropW;  // ScrollerWidth
                max_h = l->MaxShown*l->EntryHeight + l->UpperVBorder + l->LowerVBorder;

                /*** minimale Ausdehnung berechnen (ohne VBorder!) **/
                min_w = l->MinEntryWidth + ywd->PropW;  // ScrollerWidth
                min_h = l->MinShown * l->EntryHeight + l->UpperVBorder + l->LowerVBorder;

                if (l->Flags & LISTF_HasTitleBar) {
                    max_h += ywd->FontH;    // TitleHeight
                    min_h += ywd->FontH;    // TitleHeight
                };

                /*** Ausdehnungs-Korrektur ***/
                if (new_w > max_w)      new_w = max_w;
                else if (new_w < min_w) new_w = min_w;
                if (new_h > max_h)      new_h = max_h;
                else if (new_h < min_h) new_h = min_h;

                /*** Rand-Korrektur (Statusbalken beachten!) ***/
                if ((req_x + new_w) >= ywd->DspXRes) new_w=ywd->DspXRes-req_x;
                if ((req_y + new_h) >= (ywd->DspYRes-ywd->LowerTabu)) {
                    new_h = ywd->DspYRes - ywd->LowerTabu - req_y;
                };

                /*** List-Req-Parameter refreshen ***/
                if (l->Flags & LISTF_HasTitleBar) {
                    l->ShownEntries = (new_h - l->LowerVBorder - l->UpperVBorder - ywd->FontH)/l->EntryHeight;
                } else {
                    l->ShownEntries = (new_h - l->LowerVBorder - l->UpperVBorder)/l->EntryHeight;
                };
                if ((l->FirstShown + l->ShownEntries) > l->NumEntries) {
                    l->FirstShown = l->NumEntries - l->ShownEntries;
                    if (l->FirstShown < 0) l->FirstShown = 0;
                };
                l->ActEntryWidth = new_w - ywd->PropW; // ScrollerWidth

                /*** Höhe und Breite indirekt zurechtstutzen ***/
                yw_ListSetRect(ywd, l, -2, -2);

            } else {
                /*** Resizing abbrechen ***/
                l->Flags &= ~LISTF_Resizing;
            };

        } else if (l->Flags & LISTF_Scrolling) {

            /*** Scrolling underway ***/
            if (ci->flags & CIF_MOUSEHOLD) {

                l->KnobStart += ci->act.scry - l->ScrY;
                l->ScrY = ci->act.scry;

                /*** Korrektur ***/
                if (l->KnobStart < 0) {
                    l->KnobStart = 0;
                } else if ((l->KnobStart + l->KnobSize) > l->ScrSize) {
                    l->KnobStart = l->ScrSize - l->KnobSize;
                };

                /*** FirstShown einstellen ist in yw_LVLayoutScroller ***/

            } else {
                /*** Scrolling abbrechen ***/
                l->Flags &= ~LISTF_Scrolling;
            };

        } else if (l->Flags & LISTF_Select) {

            /*** Selection underway ***/
            if ((ci->box == &(l->Req.req_cbox)) && (ci->btn >= LV_NUM_STD_BTN)) {

                if (ci->flags & (CIF_MOUSEUP|CIF_MOUSEHOLD)) {
                    /*** Maus ist über einem gültigem Button ***/
                    l->Selected  = l->FirstShown + ci->btn - LV_NUM_STD_BTN;
                    l->MouseItem = l->Selected;

                    /*** Mouse ueber gueltigem Button losgelassen ... ***/
                    if (ci->flags & CIF_MOUSEUP) {

                        /*** auf alle Fälle Select-Vorgang beenden ***/
                        l->Flags &= ~LISTF_Select;
                        l->Flags |= LISTF_SelectDone;

                        /*** Requester schließen, wenn im Immediate Mode ***/
                        if (l->Flags & LISTF_Immediate) {
                            l->Req.flags |= REQF_Closed;
                            _RemClickBox(&(l->Req.req_cbox));
                        };
                    };
                } else {
                    l->Flags &= ~LISTF_Select;
                };

            } else {

                /*** Selektion underway und Maus außerhalb des Requesters ***/
                if ((ci->flags & CIF_MOUSEHOLD) && (!(l->Flags & LISTF_NoScroll))) {

                    struct ClickButton *first_btn,*last_btn;
                    first_btn = l->Req.req_cbox.buttons[LV_NUM_STD_BTN];
                    last_btn  = l->Req.req_cbox.buttons[l->ShownEntries-1+LV_NUM_STD_BTN];

                    /*** Maus gedrueckt ausserhalb: Scrollen ***/
                    if (l->ScrollTimer <= 0) {

                        /*** Scroller-Frequenz rücksetzen ***/
                        l->ScrollTimer = 70;

                        if (ci->act.scry < (req_y + first_btn->rect.y)) {
                            /*** 1 Item nach oben scrollen ***/
                            l->FirstShown--;
                            if (l->FirstShown < 0) l->FirstShown = 0;
                            l->Selected = l->FirstShown;
                        } else if (ci->act.scry > (req_y+last_btn->rect.y+last_btn->rect.h)) {
                            /*** 1 Item nach unten scrollen ***/
                            l->FirstShown++;
                            if ((l->FirstShown + l->ShownEntries) > l->NumEntries) {
                                l->FirstShown = l->NumEntries - l->ShownEntries;
                                if (l->FirstShown < 0) l->FirstShown = 0;
                            };
                            l->Selected = l->FirstShown+l->ShownEntries-1;
                        };
                    } else {
                        /*** ScrollTimer runterzaehlen ***/
                        l->ScrollTimer -= ip->FrameTime;
                    };

                    /*** auch ohne Scrolling Selected auf First oder Last setzen ***/
                    if (ci->act.scry < (req_y + first_btn->rect.y)) {
                        l->Selected = l->FirstShown;
                    } else if (ci->act.scry > (req_y+last_btn->rect.y+last_btn->rect.h)) {
                        l->Selected = l->FirstShown+l->ShownEntries-1;
                    };

                } else if (!(ci->flags & CIF_MOUSEHOLD)) {
                    /*** Mouse wurde nicht ueber einem Item-Button losgelassen, ***/
                    /*** Select-Prozess abschliessen...                         ***/
                    l->Flags &= ~LISTF_Select;
                };
            };

        } else if (ci->box == &(l->Req.req_cbox)) {

            /*** MouseItem ausfüllen ***/
            if (ci->btn >= LV_NUM_STD_BTN) {
                l->MouseItem = l->FirstShown + ci->btn - LV_NUM_STD_BTN;
            };

            /*** Help-Button ***/
            if (ci->btn == LV_BTN_HELP) {
                if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                    l->Req.flags |= REQF_HelpDown;
                };
                if (ci->flags & CIF_BUTTONUP) {
                    l->Req.flags &= ~REQF_HelpDown;
                };
            };

            /*** anhaltende Buttons ***/
            if (ci->flags & CIF_BUTTONHOLD) {
                switch(ci->btn) {
                    /*** Upper Scroll Container ***/
                    case LV_BTN_SCROLLTOP:
                        if (l->ScrollTimer <= 0) {
                            l->ScrollTimer = 70;
                            l->FirstShown--;
                            if (l->FirstShown < 0) l->FirstShown = 0;
                        }else{
                            l->ScrollTimer -= ip->FrameTime;
                        };
                        break;

                    /*** Lower Scroll Container ***/
                    case LV_BTN_SCROLLBOTTOM:
                        if (l->ScrollTimer <= 0) {
                            l->ScrollTimer = 70;
                            l->FirstShown++;
                            if ((l->FirstShown + l->ShownEntries) > l->NumEntries) {
                                l->FirstShown = l->NumEntries - l->ShownEntries;
                                if (l->FirstShown < 0) l->FirstShown = 0;
                            };
                        }else{
                            l->ScrollTimer -= ip->FrameTime;
                        };
                        break;
                };
            };

            /*** immediate Buttons... ***/
            if (ci->flags & CIF_BUTTONDOWN) {

                switch (ci->btn) {

                    /*** Resize Button ***/
                    case LV_BTN_RESIZE:
                        l->Flags |= LISTF_Resizing;
                        l->RszX = req_w - ci->down.scrx;
                        l->RszY = req_h - ci->down.scry;
                        break;

                    /*** Scroller Knob ***/
                    case LV_BTN_SCROLLKNOB:
                        l->Flags |= LISTF_Scrolling;
                        l->ScrY = ci->down.scry;
                        break;
                };

                /*** Mouse wurde über einem Item niedergedrückt ***/
                if (ci->btn >= LV_NUM_STD_BTN) {
                    l->Flags   |= LISTF_Select;
                    l->Selected = l->MouseItem;
                };
            };

        } else if ((l->Flags & LISTF_Immediate) && (ci->flags & CIF_MOUSEDOWN)) {

            /*** Immediate-Modus: Mouse außerhalb Req niedergedrückt ***/
            /*** also Requester zumachen                             ***/
            l->Req.flags |= REQF_Closed;
            _RemClickBox(&(l->Req.req_cbox));
        };
    } else {
        /*** wenn Listview geschlossen, alle kontinuierlichen Aktionen löschen ***/
        l->Flags &= ~(LISTF_Resizing|LISTF_Scrolling|LISTF_Select);
    };
}

/*-----------------------------------------------------------------*/
void yw_ListLayout(struct ypaworld_data *ywd, struct YPAListReq *l)
/*
**  FUNCTION
**      Layoutet Listview, muß *nach* HandleInput aufgerufen
**      werden.
**
**  CHANGED
**      18-Dec-95   floh    created
**      19-Jun-96   floh    + falls LISTF_Static gesetzt, wird
**                            yw_LVLayoutItemBlock() nicht aufgerufen
**      27-Jul-96   floh    + revised & updated
*/
{
    /*** Layout ***/
    yw_LVLayoutTitle(ywd, l);
    yw_LVLayoutScroller(ywd, l);
    if (!(l->Flags & LISTF_Static)) yw_LVLayoutItemBlock(ywd, l);
}

