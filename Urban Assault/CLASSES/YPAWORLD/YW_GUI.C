/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_gui.c,v $
**  $Revision: 38.28 $
**  $Date: 1998/01/06 16:19:53 $
**  $Locker: floh $
**  $Author: floh $
**
**  GUI-Elemente der ypaworld.class -- erst mal eher experimentell.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>

#include <stdio.h>          // für sprintf()
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "bitmap/rasterclass.h"

#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"
#include "ypa/guifinder.h"
#include "ypa/guilogwin.h"
#include "ypa/guilogwin.h"
#include "ypa/guinetwork.h"
#include "ypa/guiabort.h"
#include "ypa/guiconfirm.h"
#include "audio/cdplay.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_input_engine
_extern_use_audio_engine

/*-------------------------------------------------------------------
**  externe Requester-Strukturen
*/
extern struct YPAMapReq MR;
extern struct YPAFinder FR;
extern struct YPALogWin LW;
extern struct YPAAbortReq AMR;
extern struct YPAEnergyBar EB;
extern struct YPAConfirmReq CR;

#ifdef __NETWORK__
extern struct YPAMessageWin MW;
#endif

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_ADDREQUESTER, struct YPAReq *req)
/*
**  FUNCTION
**      Hängt eine neue Requester-Struktur in die interne Requester-
**      Liste des Welt-Objects. Der Requester wird an das Ende
**      der Liste gehängt. Das Welt-Object managed Iconify,
**      Move Requester und Requester To Front automatisch ab,
**      außerdem stellt die Welt-Klasse die Requester automatisch
**      dar. Um den Rest muß sich der Besitzer des Requesters
**      selber kümmern.
**
**      Folgende Flags müssen entsprechend den Fähigkeiten 
**      des Requesters gesetzt sein:
**
**          REQF_HasIcon
**          REQF_HasDragBar
**          REQF_HasCloseGadget
**
**      Die globale HandleInput-Routine wird kein "Subsystem"
**      anrühren, für das nicht auch das entsprechende
**      Flag gesetzt ist.
**
**
**  INPUTS
**      req     -> Ptr auf initialisierte YPAReq-Struktur
**                 (siehe ypa/ypagui.h).
**
**  RESULTS
**      ---
**
**  CHANGED
**      12-Aug-95   floh    created
**      13-Aug-95   floh    Falls bereits in einer Liste eingebunden,
**                          wird der Req vorher aus dieser _Remove()'d!
**                          Außerdem wird jetzt das Feld <req->in_list>
**                          auf TRUE gesetzt.
**                          + bindet jetzt automatisch die richtige
**                            ClickBox (Requester oder Icon) in die
**                            Input.Engine ein.
**                          + initialisiert jetzt req->icon_cbox->userdata
**                            und req->req_cbox->userdata mit Pointer
**                            auf req, so wie <yw_HandleGUIInput()>
**                            das sowieso benötigt.
**      28-Nov-95   floh    + neue Requester-Flags
**                          - req->in_list, req->icon
**      26-Dec-95   floh    Bug: Requester ohne Icon, die das REQF_Closed
**                          Flag gesetzt haben, wurden trotzdem per
**                          _AddClickBox() in die Input-Engine geklinkt,
**                          das ergab ein kaputte ClickBox-Liste, wenn
**                          sie geöffnet wurden.
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    /*** etwas Security... ***/
    if (0 == (req->flags & REQF_HasIcon)) req->flags &= ~REQF_Iconified;

    /*** falls schon in ReqListe -> removen ***/
    if (req->flags & REQF_InList) {
        _methoda(o, YWM_REMREQUESTER, req);
    };

    /*** an Anfang der ReqListe hängen ***/
    _AddHead((struct List *) &(ywd->ReqList),
             (struct Node *) req);
    req->flags |= REQF_InList;

    /*** <userdata> initialisieren ***/
    if (req->flags & REQF_HasIcon) {
        req->icon_cbox.userdata = (ULONG) req;
    };
    req->req_cbox.userdata  = (ULONG) req;

    /*** ClickBox einbinden ***/
    if (req->flags & REQF_Iconified) {
        _AddClickBox(&(req->icon_cbox), IE_CBX_ADDHEAD);
    } else if (!(req->flags & REQF_Closed)) {
        _AddClickBox(&(req->req_cbox), IE_CBX_ADDHEAD);
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_REMREQUESTER, struct YPAReq *req)
/*
**  FUNCTION
**      Entfernt diesen Requester aus interner Requester-Liste
**      des Welt-Objects.
**
**  INPUTS
**      req     -> Ptr auf YPAReq-Struktur, die entfernt werden soll.
**
**  RESULTS
**      ---
**  CHANGED
**      12-Aug-95   floh    created
**      13-Aug-95   floh    Das Feld <req->in_list> wird jetzt auf
**                          FALSE gesetzt, als Indikator, das der
**                          Requester momentan in keiner Liste
**                          eingebunden ist.
**                          + ClickBox aus Input.Engine ausklinken...
**      28-Nov-95   floh    + neue Requester-Flags
**                          - req->in_list, req->icon
**      26-Dec-95   floh    Bug: Requester ohne Icons, die geschlossen
**                          sind, wären fatalerweise aus der ClickBox-
**                          Liste removed worden (obwohl sie gar nicht
**                          mehr drin sind).
**      27-May-96   floh    Bugfix: das REQF_InList-Flags wurde bei
**                          Erfolg gesetzt, nicht gelöscht...
*/
{
    /*** aus Req-Liste entfernen ***/
    if (req->flags & REQF_InList) {
        _Remove((struct Node *) req);
        req->flags &= ~REQF_InList;

        /*** ClickBox removen ***/
        if (req->flags & REQF_Iconified) {
            _RemClickBox(&(req->icon_cbox));
        } else if (!(req->flags & REQF_Closed)) {
            _RemClickBox(&(req->req_cbox));
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_HandleGUIInput(struct ypaworld_data *ywd,
                       struct VFMInput *ip)
/*
**  FUNCTION
**      Muß innerhalb yw_BSM_TRIGGER aufgerufen werden!
**
**      Handelt folgende Standard-Aufgaben für *alle* per
**      YWM_ADDREQUESTER eingebundenen Requester ab:
**
**          * Iconify/Explode
**          * Dragging
**          * Window-To-Front
**
**      Außerdem werden die Nonstandard-ClickButtons der
**      ypaworld.class eigenen Requester abgehandelt.
**
**  INPUTS
**      ywd -> Ptr auf LID des Welt-Objects
**      ip  -> aktuelle VFMInput-Struktur für diesen Frame
**
**  RESULTS
**      ---
**
**  CHANGED
**      13-Aug-95   floh    created
**      14-Aug-95   floh    Close- und Icons jetzt depressed.
**      05-Sep-95   floh    ruft jetzt yw_HandleInputMM() auf
**      06-Sep-95   floh    ruft jetzt yw_HandleInputUM() auf
**      30-Oct-95   floh    ruft jetzt yw_HandleInputMR() auf
**      17-Nov-95   floh    Dragging jetzt Pixel-genau am Rand
**      28-Nov-95   floh    beachtet jetzt die neuen Req-Property-Flags
**      01-Dec-95   floh    ruft jetzt yw_HandleInputFR() auf
**      31-Dec-95   floh    jetzt ein oberer und unterer Tabustreifen
**                          von je STAT_HEIGHT Höhe
**                          - ModeMenu Input-Handler
**                          - UnitMenu Input-Handler
**      11-Feb-96   floh    yw_HandleInputUSR() [Upper Status Bar]
**      06-May-96   floh    + Window-Cycle Hotkey, holt jeweils das
**                            hinterste offene Window nach vorn.
**                          + yw_HandleInputLW() -> LogWindow
**      10-May-96   floh    + DragLock-Flag wird korrekt behandelt
**                            (für Window-Dragging)
**      27-Jun-96   floh    + wenn User in einer Waffe sitzt, wird
**                            keine Input-Behandlung durchgeführt
**      14-Jul-96   floh    + CutterMode-Support
**      29-Jul-96   floh    + Dynamic Layout Support
**      05-Aug-96   floh    - Upper Status Req gekillt
**      06-Aug-96   floh    + Dragging an neuen Status-Bar angepaßt
**      10-Aug-96   floh    + Close-Gadget-Handling changed (es
**                            wird der Font ausgetauscht)
**      21-Aug-96   floh    + yw_HandleInputER()
**                          + anstatt das Close-Gadget direkt zu
**                            manipulieren, wird nur das
**                            REQF_CloseDown-Requester-Flag
**                            gesetzt/gelöscht.
**      09-Oct-97   floh    + Window-Cycle-Hotkey ist raus
**      10-Oct-97   floh    + yw_LayoutEB()
**      11-Oct-97   floh    + Ausdehnungs- und Positions-Korrektur beim
**                            Draggen per ywd->UpperTabu und ywd->LowerTabu
**      10-Dec-97   floh    + yw_HandleInputER() ist raus (Energiewindow)
**      23-Apr-98   floh    + yw_HandleInputCR() 
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);

    if (BCLID_YPAMISSY == ywd->UVBact->BactClassID) return;

    /*** ist GUI-Modus überhaupt eingeschaltet? ***/
    if (ci->flags & CIF_VALID) {

        /*** Ptr auf YPAReq-Struktur, gleichzeitig Anzeiger,      ***/
        /*** ob's überhaupt was zu tun gibt (Maus innerhalb einer ***/
        /*** ClickBox? ***/

        /*** alle Standard-Buttons aller Requester in             ***/
        /*** "nicht gepressed" Modus, geht leider nicht anders... ***/
        struct MinList *ls;
        struct MinNode *nd;
        ls = &(ywd->ReqList);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

            struct YPAReq *req = (struct YPAReq *) nd;

            /*** Icon entpressen, so notwendig ***/
            if (req->flags & REQF_HasIcon) {
                if (req->flags & REQF_Iconified) {
                    req->icon_string[2] = 1;
                };
            };

            /*** Close-Gadget entpressen, so notwendig ***/
            if (req->flags & REQF_HasCloseGadget) {
                if (0 == (req->flags & (REQF_Iconified|REQF_Closed))) {
                    req->flags &= ~REQF_CloseDown;
                };
            };
        };

        /*** Ist Maus innerhalb einer ClickBox? ***/
        if (ci->box) {

            struct YPAReq *req = (struct YPAReq *) ci->box->userdata;

            if (req) {

                /*** Icon Stuff abhandeln ***/
                if (req->flags & REQF_Iconified) {

                    /*** Depressed? ***/
                    if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                        req->icon_string[2] = 2;
                    };

                    /*** Explodify ? ***/
                    if (ci->flags & CIF_BUTTONUP) {

                        req->flags &= ~REQF_Iconified;

                        _RemClickBox(&(req->icon_cbox));
                        _AddClickBox(&(req->req_cbox), IE_CBX_ADDHEAD);

                        /*** visual 'To Front' ***/
                        _Remove((struct Node *) req);
                        _AddHead((struct List *) &(ywd->ReqList),
                                 (struct Node *) req);
                    };

                } else {

                    /*** Requester To Front? ***/
                    if (ci->flags & CIF_MOUSEDOWN) {
                        _RemClickBox(&(req->req_cbox));
                        _AddClickBox(&(req->req_cbox), IE_CBX_ADDHEAD);
                        _Remove((struct Node *) req);
                        _AddHead((struct List *) &(ywd->ReqList),
                                 (struct Node *) req);
                    };

                    /*** Close ? ***/
                    if (req->flags & REQF_HasCloseGadget) {

                        if (ci->btn == 0) {

                            /*** press down? ***/
                            if (ci->flags & (CIF_BUTTONHOLD|CIF_BUTTONDOWN)) {
                                req->flags |= REQF_CloseDown;
                            };
                            if ((ci->flags & (CIF_BUTTONDOWN)) && (ywd->gsr)) {
                                _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
                            };
            
                            /*** released? ***/
                            if (ci->flags & CIF_BUTTONUP) {

                                _RemClickBox(&(req->req_cbox));

                                /*** wenn vorhanden, Icon aktivieren ***/
                                if (req->flags & REQF_HasIcon) {
                                    _AddClickBox(&(req->icon_cbox), IE_CBX_ADDHEAD);
                                    req->flags |= REQF_Iconified;
                                } else {
                                    req->flags |= REQF_Closed;
                                };

                                /*** visual 'To Front' ***/
                                _Remove((struct Node *) req);
                                _AddHead((struct List *) &(ywd->ReqList),
                                         (struct Node *) req);
                            };
                        };
                    };
                
                    /*** Start Dragging ? ***/
                    if (req->flags & REQF_HasDragBar) {
                        if ((ci->flags & CIF_BUTTONDOWN) && (ci->btn == 1))
                        {
                            ywd->Dragging = TRUE;
                            ywd->DragLock = TRUE;
                            ywd->DragReq  = req;
                            ywd->DragReqX = ci->down.boxx;
                            ywd->DragReqY = ci->down.boxy;
                        };
                    };
                };
            }; // if (req)
        }; // if (ci->box)

        /*** Dragging? (Mousezeiger darf außerhalb Requester sein!) ***/
        if ((ywd->Dragging) && (ci->flags & CIF_MOUSEHOLD)) {

            struct YPAReq *req = ywd->DragReq;

            /*** neue Req-Coords im virtuellen Maus-Screen ***/
            WORD req_x = ci->act.scrx - ywd->DragReqX;
            WORD req_y = ci->act.scry - ywd->DragReqY;
            WORD req_w = req->req_cbox.rect.w;
            WORD req_h = req->req_cbox.rect.h;

            /*** Rand-Korrektur ***/
            if (req_x < 0) req_x = 0;
            else if ((req_x + req_w) > ywd->DspXRes) req_x = ywd->DspXRes - req_w;
            if (req_y < ywd->UpperTabu) req_y = ywd->UpperTabu;
            else if ((req_y + req_h) > (ywd->DspYRes-ywd->LowerTabu)) {
                req_y = ywd->DspYRes - ywd->LowerTabu - req_h;
            };

            /*** Requester neu positionieren ***/
            req->req_cbox.rect.x = req_x;
            req->req_cbox.rect.y = req_y;

            /*** Requester-String neu positionieren ***/
            req_x -= (ywd->DspXRes >> 1);
            req_y -= (ywd->DspYRes >> 1);
            req->req_string[5]  = (UBYTE) (req_x>>8);
            req->req_string[6]  = (UBYTE) (req_x);
            req->req_string[9]  = (UBYTE) (req_y>>8);
            req->req_string[10] = (UBYTE) (req_y);

            /*** dat war's ***/
        };

        /*** Stop Dragging? ***/
        if ((ywd->Dragging) && (!(ci->flags & CIF_MOUSEHOLD))) {
            ywd->Dragging = FALSE;
            ywd->DragLock = FALSE;
        };

        /*** externe Handler aufrufen ***/

        #ifdef __NETWORK__
        /*** Zuerst der MessageRequester, der offen Tasten schluckt ***/
        if( ywd->playing_network )
            yw_HandleInputMW(ywd,ip);
        #endif

        yw_HandleInputAMR(ywd,ip);
        yw_HandleInputMR(ywd,ip);
        yw_HandleInputFR(ywd,ip);
        yw_HandleInputLW(ywd,ip);
        yw_HandleInputEB(ywd,ip);
        yw_HandleInputCR(ywd,ip);
    };

    /*** der Status-Req wird immer aufgerufen, weil er das ***/
    /*** Layout innerhalb HandleInput abhandelt!           ***/
    yw_HandleInputSR(ywd,ip);

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_RenderRequesters(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert alle Requester in der ReqList. Die Requester
**      müssen alle in einem gültigen Zustand und "layoutet"
**      sein.
**      Konkret wird gemacht: Die ReqList von hinten nach vorn
**      gescannt, und ein _DrawText() entweder auf den IconString
**      oder auf den ReqString gemacht, je nach aktuellem Zustand des
**      Requesters.
**
**  INPUTS
**      ywd     -> Ptr auf LID des Welt-Objects
**
**  RESULTS
**      ---
**
**  CHANGED
**      13-Aug-95   floh    created
**      03-Oct-95   floh    teilt jetzt die vom Requester belegte
**                          Fläche als ClipRegion der Gfx-Engine mit.
**      21-Nov-95   floh    Falls != NULL und der Requester ist
**                          offen, wird der PostDraw-Hook des Requesters
**                          aufgerufen.
**      28-Nov-95   floh    beachtet jetzt die neuen Req-Flags
**      12-Dec-95   floh    Der StatusBar wird jetzt gesondert am
**                          Schluß gezeichnet...
**      11-Feb-96   floh    + yw_DrawCompassVecs()
**      05-Apr-96   floh    _OccupyRegion() gekillt, das war der Grund
**                          für die mysteriösen "Schwarz-Zonen" bei den
**                          Transparenz-Flächen :-/
**      24-Jun-96   floh    + yw_RenderQuickLog(), zeichnet im oberen
**                            Teil des Displays die letzte eingegangene 
**                            LogMsg, wenn sie jünger als 5 Sekunden ist
**      27-Jun-96   floh    + wenn User in einer Waffe sitzt, wird
**                            überhaupt kein Display gezeichnet
**      14-Jul-96   floh    + CutterMode-Support
**      05-Aug-96   floh    - yw_DrawCompassVecs() gekillt
**      01-Sep-96   floh    + yw_RenderHUD()
**      13-Sep-96   floh    + extra Radar-Modus
**      17-Sep-96   floh    - Radar-Rendering (Radar ist jetzt normaler
**                            Bestandteil des HUD.
**                          + Tooltip-Rendering
**      03-Nov-96   floh    + QuickLog wird nicht mehr gerendert, wenn
**                            User-Fahrzeug tot.
**      10-Oct-97   floh    + yw_RenderEB()
**      11-Feb-98   floh    + yw_RenderSuperItemStatus()
**      04-May-98   floh    + ueberfluessigen Code entfernt
*/
{
    struct MinList *ls = &(ywd->ReqList);
    struct MinNode *nd;
    struct drawtext_args dt;

    if (BCLID_YPAMISSY != ywd->UVBact->BactClassID) {

        /*** QuickLog, HUD und Tooltips ***/
        if (ywd->UVBact->MainState != ACTION_DEAD) {
            yw_RenderQuickLog(ywd);
            yw_RenderSuperItemStatus(ywd);
        };
        yw_RenderHUD(ywd);
        yw_RenderTooltip(ywd);

        /*** die "normalen" Requester ***/
        for (nd=ls->mlh_TailPred; nd->mln_Pred; nd=nd->mln_Pred) {

            struct YPAReq *req = (struct YPAReq *) nd;
            BOOL call_hook = FALSE;

            /*** überhaupt was zu zeichnen??? ***/
            if (!(req->flags & REQF_Closed)) {

                /*** das Icon zeichnen? ***/
                if (req->flags & REQF_Iconified) {

                    dt.string = req->icon_string;
                    dt.clips  = NULL;

                } else {

                    /*** sonst den Vanilla-Req-String zeichnen ***/
                    dt.string = req->req_string;
                    dt.clips  = req->req_clips;

                    /*** existiert ein PostDraw-Hook? ***/
                    if (req->post_draw) {
                        call_hook = TRUE;
                    };
                };
                _DrawText(&dt);
                if (call_hook) (*req->post_draw)(ywd);
            };
        };

        /*** StatusBar und EnergyBar zeichnen ***/
        yw_RenderEB(ywd);
        yw_RenderStatusReq(ywd);
    };

    /*** das war's bereits! ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_InitGUIModule(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert die Requester der ypaworld.class.
**
**  INPUTS
**      o   -> Ptr auf Welt-Object
**      ywd -> Ptr auf LID des Welt-Objects
**
**  RESULTS
**      TRUE/FALSE
**
**  CHANGED
**      11-Aug-95   floh    created
**      14-Aug-95   floh    + Sector-Info-Requester
**      05-Sep-95   floh    + ModeMenu
**      06-Sep-95   floh    + UnitMenu
**      28-Oct-95   floh    + MapRequester
**      30-Nov-95   floh    + Finder
**      12-Dec-95   floh    + Status-Requester
**                          - Status-Requester wird NICHT mehr per
**                            YWM_ADDREQUESTER behandelt
**      31-Dec-95           - ModeMenu
**                          - UnitMenu
**      11-Feb-96   floh    + Upper Status Req
**      24-Apr-96   floh    + wenn alles glatt geht, wird ywd->GUI_Ok
**                            gesetzt, als Zeichen für yw_KillGUIModule(),
**                            daß es aufräumen muß
**      06-May-96   floh    + yw_InitLogWin()
**      10-May-96   floh    + ein paar mehr GUI-Variablen werden
**                            ordentlich initialisiert!
**      18-Jun-96   floh    + yw_InitAMR()
**      14-Jul-96   floh    + CutterMode-Support
**      27-Jul-96   floh    + ywd->FontH, ywd->CloseW, ywd->PropW,
**                            ywd->PropH, ywd->EdgeW, ywd->EdgeH
**      30-Jul-96   floh    + ywd->VPropH
**      31-Jul-96   floh    + Initialisierung der globalen Layout-Variablen
**                            verlegt nach yw_LoadFontSet().
**      05-Aug-96   floh    - Upper Status Req gekillt
**      21-Aug-96   floh    + Energy-Window
**      01-Sep-96   floh    + yw_InitHUD()
**      09-Sep-96   floh    - die zwei alten Info-Reqs
**      13-Sep-96   floh    + initialisiert raster.class Object für
**                            "Sekundäre Hidden Bitmap" -> Radarmodus,
**                            etc...
**      23-Sep-96   floh    + Background-Pen für Radar-Raster-Objekt
**                            auf 13 gesetzt.
**      10-Oct-96   floh    + initialisiert ywd->MouseBlanked
**      03-Jan-97   floh    + yw_InitKeycode2StrTable()
**      22-May-97   floh    + yw_InitForceFeedback()
**      03-Jul-97   floh    + ClickControlTimeStamp und Position wird
**                            initialisiert.
**      10-Oct-97   floh    + yw_InitEB()
**      15-Oct-97   floh    + SecObject für DirectDraw Radar rausgehauen
**      26-Oct-97   floh    + yw_InitKeycode2StrTable() wieder raus
**      10-Dec-97   floh    + yw_InitER() (Energiewindow) endgueltig raus.
**      23-Apr-98   floh    + ConfirmRequester
*/
{ 
    if (!(ywd->GUI_Ok)) {

        ywd->Dragging = FALSE;
        ywd->DragReq  = NULL;
        ywd->DragLock = FALSE;
        ywd->MouseBlanked = FALSE;
        ywd->ClickControlTimeStamp = 0;
        ywd->ClickControlX         = 0;
        ywd->ClickControlY         = 0;
        ywd->ClickControlBact      = NULL;

        #ifdef __WINDOWS__
            yw_InitForceFeedback(ywd);
        #endif
        yw_InitHUD(ywd);
        yw_InitEB(ywd);
        yw_InitMapReq(o,ywd);
        yw_InitFinder(o,ywd);
        yw_InitLogWin(o,ywd);
        yw_InitAMR(ywd);
        yw_InitCR(ywd);
        if (yw_InitStatusReq(o,ywd)) {

            struct VFMBitmap *shade_lum,*tracy_lum;
            struct snd_cdcontrol_msg cd;
            ULONG i;

            _get(ywd->TracyRemap, BMA_Bitmap, &tracy_lum);
            _get(ywd->ShadeRemap, BMA_Bitmap, &shade_lum);

            /*** alle Requester anmelden ***/
            _methoda(o, YWM_ADDREQUESTER, &MR);
            _methoda(o, YWM_ADDREQUESTER, &FR);
            _methoda(o, YWM_ADDREQUESTER, &LW);
            _methoda(o, YWM_ADDREQUESTER, &AMR);
            _methoda(o, YWM_ADDREQUESTER, &CR);

            #ifdef __NETWORK__
            yw_InitMW(ywd);
            _methoda(o, YWM_ADDREQUESTER, &MW);
            #endif

            ywd->GUI_Ok = TRUE;

            /*** Starten Lied ***/
            if( ywd->Prefs.Flags & YPA_PREFS_CDSOUNDENABLE) {
                cd.command   = SND_CD_SETTITLE;
                cd.para      = ywd->Level->AmbienceTrack;
                cd.min_delay = ywd->Level->ambience_min_delay;
                cd.max_delay = ywd->Level->ambience_max_delay;
                _ControlCDPlayer( &cd );
                cd.command = SND_CD_PLAY;
                _ControlCDPlayer( &cd );
            };
            return(TRUE);
        };
        yw_KillAMR(ywd);
        yw_KillLogWin(o,ywd);
        yw_KillFinder(o,ywd);
        return(FALSE);
    };

    return((BOOL)ywd->GUI_Ok);
}

/*-----------------------------------------------------------------*/
void yw_KillGUIModule(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Cleanup der ypaworld.class Requester.
**
**  INPUTS
**      o   -> Ptr auf Welt-Object
**      ywd -> Ptr auf LID des Welt-Objects
**
**  CHANGED
**      11-Aug-95   floh    created
**      14-Aug-95   floh    + Sector-Info-Requester
**      05-Sep-95   floh    + ModeMenu
**      28-Oct-95   floh    + MapRequester
**      30-Nov-95   floh    + Finder
**      12-Dec-95   floh    + Status-Req
**                          - Status-Requester wird NICHT mehr per
**                            YWM_REMREQUESTER behandelt
**      31-Dec-95   floh    - ModeMenu
**                          - UnitMenu
**      24-Apr-96   floh    wertet zuerst ywd->GUI_Ok aus, bevor
**                          etwas unternommen wird
**      06-May-96   floh    + yw_KillLogWin()
**      27-May-96   floh    + Bugfix: LogWin wurde nicht YWM_REMREQUESTER()'d
**                            deshalb (wahrscheinlich) auch der Hängenbleiber
**                            in der GameShell
**      18-Jun-96   floh    + yw_KillAMR()
**      14-Jul-96   floh    + CutterMode-Support
**      05-Aug-96   floh    - Upper Status Req gekillt
**      21-Aug-96   floh    + Energy-Window
**      09-Sep-96   floh    - die zwei alten Info-Reqs
**      13-Sep-96   floh    + füllt die GUI-Items der
**                            YPAGamePrefs-Struktur aus (so
**                            daß sie beim nächsten Mal wieder
**                            eingelesen werden können).
**      13-Sep-96   floh    + Sekundär-Hidden-Bitmap wird korrekt
**                            gekillt
**      10-Oct-96   floh    + ywd->MouseBlanked = FALSE
**      22-May-97   floh    + yw_KillForceFeedback()
**      10-Oct-97   floh    + yw_KillEB()
**      15-Oct-97   floh    + SecObject ist rausgeflogen
**      10-Dec-97   floh    + yw_KillER() raus
**      23-Apr-98   floh    + ConfirmReq
**      07-May-98   floh    + Ooops, KillCR vergessen...
*/
{
    if (ywd->GUI_Ok) {

        struct YPAGamePrefs *p = &(ywd->Prefs);

        /*** Prefs-Struktur ausfüllen ***/
        p->valid = TRUE;
        p->WinMap.rect = MR.req.req_cbox.rect;
        p->MapLayers   = MR.layers;
        p->MapZoom     = MR.zoom;
        p->WinFinder.rect = FR.l.Req.req_cbox.rect;
        p->WinLog.rect    = LW.l.Req.req_cbox.rect;

        #ifdef __NETWORK__
        p->WinMessage.rect = MW.Req.req_cbox.rect;
        #endif

        /*** Requester dehydrieren, ähh... ***/
        _methoda(o, YWM_REMREQUESTER, &CR);
        _methoda(o, YWM_REMREQUESTER, &AMR);
        _methoda(o, YWM_REMREQUESTER, &LW);
        _methoda(o, YWM_REMREQUESTER, &FR);
        _methoda(o, YWM_REMREQUESTER, &MR);

        #ifdef __NETWORK__
        _methoda(o, YWM_REMREQUESTER, &MW);
        #endif
        yw_KillAMR(ywd);
        yw_KillLogWin(o,ywd);
        yw_KillFinder(o,ywd);
        yw_KillStatusReq(ywd);
        yw_KillEB(ywd);
        yw_KillHUD(ywd);
        yw_KillCR(ywd);
        #ifdef __WINDOWS__
            yw_KillForceFeedback(ywd);
        #endif

        #ifdef __NETWORK__
        yw_KillMW(ywd);
        #endif

        ywd->MouseBlanked = FALSE;
        ywd->GUI_Ok = FALSE;
    };
}

/*-----------------------------------------------------------------*/
void yw_LayoutGUI(Object *o,struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Updatet alle offenen Requester.
**
**  INPUTS
**      o   -> Ptr auf Welt-Object
**      ywd -> Ptr auf LID des Welt-Objects
**
**  RESULTS
**      ---
**
**  CHANGED
**      14-Aug-95   floh    created
**      03-Sep-95   floh    Aaargh, ich hatte nicht beachtet, daß sprintf()
**                          die Anzahl der generierten Chars *ohne* 0-Byte
**                          zurückliefert... es wurde also keine Doppel-0
**                          Terminierung erzeugt, was der Grund für
**                          einige rätselhafte Abstürze gewesen sein sollte...
**      05-Sep-95   floh    ruft jetzt yw_LayoutMM() auf
**      06-Sep-95   floh    ruft jetzt yw_LayoutUM() auf
**      15-Oct-95   floh    verändertes Sektor-Info-Req Layout
**      30-Oct-95   floh    ruft jetzt yw_LayoutMR() auf (Map-Requester)
**      28-Nov-95   floh    arbeitet jetzt mit neuen Req-Flags
**      01-Dec-95   floh    ruft jetzt yw_LayoutFR() auf
**      12-Dec-95   floh    + yw_LayoutSR()
**      13-Dec-95   floh    - yw_LayoutSR() ;-)
**      31-Dec-95   floh    - ModeMenu
**                          - UnitMenu
**      29-Jan-96   floh    No More sec->ConstFactor
**      09-Sep-96   floh    - die zwei alten Info-Reqs
*/
{
    /* externe Requester */
    yw_LayoutMR(o,ywd);
    yw_LayoutFR(o,ywd);
}

/*=================================================================**
**  GUI-Support-Funktionen                                         **
**=================================================================*/

/*-----------------------------------------------------------------*/
ULONG yw_StrLen(UBYTE *str, struct VFMFont *fnt)
/*                                                                                
**  FUNCTION
**      Ermittelt Pixel-Länge eines C-Strings ohne Kontrollsequenzen.
**
**  INPUTS
**      str     - String, NULL-terminiert, nur gültige Zeichen
**      fnt     - verwendeter Font
**
**  RESULTS
**      ULONG   - Länge des String in Pixel
**
**  CHANGED
**      18-Jun-96   floh    created
*/
{
    UBYTE c;
    ULONG len = 0;
    while (c = *str++) len += fnt->fchars[c].width;
    return(len);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_StpCpy(UBYTE *from, UBYTE *to)
/*
**  FUNCTION
**      Kopiert String <from> nach <to>, returniert Pointer
**      auf 0-Terminator in <to>.
**
**  CHANGED
**      18-Jun-96   floh    created
*/
{
    while (*to++ = *from++);
    return(to-1);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_CenteredSkippedItem(struct VFMFont *fnt,
                              UBYTE *target,
                              UBYTE *source,
                              LONG raw_width)
/*
**  FUNCTION
**      Arbeitet wie yw_BuildClippedItem(),
**      nur das die Prefix- und Postfix-Teile übersprungen
**      bzw. ignoriert werden.
**
**      <raw_width> darf größer als 256 sein.
**
**      Font und "Pseudo-Position" muß bereits initialisiert sein.
**
**      EINSCHRÄNKUNGEN:
**          Die resultierende Länge des Strings muß
**          kleiner sein als raw_width, sonst wird
**          einfach nichts gerendert (damit's keinen
**          Absturz gibt).
**
**  CHANGED
**      24-Jun-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      13-Aug-97   floh    + num_chars
*/
{
    LONG width = 0; // String-Breite
    UBYTE chr;
    UBYTE *dummy = source;
    WORD prefix;

    /*** Länge des Strings ermitteln ***/
    while (chr = *dummy++) width += fnt->fchars[chr].width;
    if (width < raw_width) {

        /*** Prefix Skip ***/
        prefix = ((raw_width - width)>>1);
        if (prefix > 0) {
            xpos_rel(target,prefix);
            raw_width -= prefix;
        };

        /*** String itself ***/
        while (*target++ = *source++);
        target--;

        /*** das war's bereits ***/
    };

    /*** Ende ***/
    return(target);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_TextCenteredSkippedItem(struct VFMFont *fnt,
                                  UBYTE *target,
                                  UBYTE *source,
                                  LONG raw_width)
/*
**  FUNCTION
**      DBCS-Enablete Version von yw_CenteredSkippedItem().
**      
**  CHANGED
**      24-Nov-97   floh    created
*/
{
    #ifdef __DBCS__
        /*** DBCS Handling ***/
        if (raw_width > 0) {
            freeze_dbcs_pos(target);   // DBCS-String auf dieser Position rendern
            put_dbcs(target,raw_width,DBCSF_CENTER,source);
        };
        return(target);
    #else
        return(yw_CenteredSkippedItem(fnt,target,source,raw_width));
    #endif
}

/*-----------------------------------------------------------------*/
UBYTE *yw_BuildClippedItem(struct VFMFont *fnt,
                           UBYTE *target, 
                           UBYTE *source, 
                           LONG raw_width,
                           UBYTE space_chr)
/*
**  FUNCTION
**      Baut ein Item-Fragment auf, das nur den String itself enthaelt.
**      Der String wird geclippt, wenn er <raw_width>
**      überschreitet, sonst wird entsprechend mit den
**      Leerzeichen ('c') des eingestellten Fonts aufgefüllt.
**      Der Bereich außerhalb des String muß extern
**      erledigt werden!
**      Die Routine akzeptiert auch Breiten über 255.
**
**  INPUTS
**      fnt       -> zeigt auf zu verwendende Font-Struktur
**      target    -> zeigt auf Ausgabe-Bytestream
**      source    -> zeigt auf Eingangs-String
**      raw_width -> Max-Breite des Strings in Pixel
**      space_chr -> Zeichen, das als Leerzeichen zum Auffülen benutzt
**                   werden soll
**
**  RESULTS
**      Pointer auf nächstes Byte im ByteStream.
**
**  CHANGED
**      11-Dec-95   floh    created
**      13-Dec-95   floh    + space_chr
**      27-Jul-96   floh    + übernommen nach yw_gui.c
*/
{
    /*** Non DBCS Handling ***/  
    LONG act_width = 0;
    LONG full_chars = 0;            // Anzahl voll dargestellter Buchstaben
    LONG rest_width = 0;            // RestBreite für letzten Buchstaben
    UBYTE act_char;
    UBYTE *dummy = source;
    ULONG i;

    /*** Clip-Parameter ermitteln ***/
    while ((act_char = *dummy++) && (rest_width == 0)) {
        act_width += fnt->fchars[act_char].width;
        if (act_width > raw_width) {
            rest_width = fnt->fchars[act_char].width - (act_width-raw_width);
        } else {
            full_chars++;
        };
    };

    /*** String aufbauen ***/
    for (i=0; i<full_chars; i++) *target++ = *source++;

    if (rest_width > 0) {
        /*** den letzten Buchstaben clippen ***/
        len_hori(target,rest_width);
        put(target,*source);

    } else if (act_width < raw_width) {
        /*** den Rest-Space 'leer' auffüllen ***/
        LONG diff = raw_width - act_width;
        do {
            if (diff > 255) { 
                stretch(target,255);
            } else {
                stretch(target,diff);
            };
            put(target,space_chr);
            diff -= 255;
        } while (diff > 0);
    };
    return(target);

    /*** das war's ***/
}

/*-----------------------------------------------------------------*/
UBYTE *yw_TextBuildClippedItem(struct VFMFont *fnt,
                               UBYTE *target, 
                               UBYTE *source, 
                               LONG raw_width,
                               UBYTE space_chr)
/*
**  FUNCTION
**      Wie yw_BuildClippedItem(), es muss sich aber unbedingt um
**      einen "richtigen" String handeln, weil in der DBCS Version
**      der String als Truetype gerendert wird. In der Nicht-DBCS-
**      Version wird die Routine automatisch auf yw_BuildClippedItem()
**      umgelenkt.
**
**  INPUTS
**      fnt       -> zeigt auf zu verwendende Font-Struktur
**      target    -> zeigt auf Ausgabe-Bytestream
**      source    -> zeigt auf Eingangs-String
**      raw_width -> Max-Breite des Strings in Pixel
**      space_chr -> Zeichen, das als Leerzeichen zum Auffülen benutzt
**                   werden soll
**
**  RESULTS
**      Pointer auf nächstes Byte im ByteStream.
**
**  CHANGED
**      22-Nov-97   floh    created
*/
{
    #ifdef __DBCS__
        /*** DBCS Handling ***/
        if (raw_width > 0) {
            LONG diff = raw_width;
            freeze_dbcs_pos(target);   // DBCS-String auf dieser Position rendern
        
            /*** Hintergrund fuellen ***/
            do {
                if (diff > 255) {
                    stretch(target,255);
                } else {
                    stretch(target,diff);
                };
                put(target,space_chr);
                diff -= 255;
            } while (diff > 0);

            /*** dann die DBCS Anweisung (left-aligned) ***/
            put_dbcs(target,raw_width,DBCSF_LEFTALIGN,source);
        };
        return(target);
    #else
        return(yw_BuildClippedItem(fnt,target,source,raw_width,space_chr));
    #endif
}

/*-----------------------------------------------------------------*/
UBYTE *yw_BuildCenteredItem(struct VFMFont *fnt,
                            UBYTE *target,
                            UBYTE *source,
                            LONG raw_width,
                            UBYTE space_chr)
/*
**  FUNCTION
**      Baut einen zentrierten String auf (NICHT GECLIPPT!).
**
**  INPUTS
**      siehe yw_BuildClippedItem()
**
**  RESULTS
**      siehe yw_BuildClippedItem()
**
**  CHANGED
**      23-Dec-95   floh    created
**      27-Jul-96   floh    + übernommen nach yw_gui.c
*/
{
    LONG width = 0;     // String-Breite
    UBYTE chr;
    UBYTE *dummy = source;
    BYTE prefix;

    /*** Länge des Strings ermitteln ***/
    while (chr = *dummy++) width += fnt->fchars[chr].width;

    /*** vorangestellte Pixel ***/
    prefix = ((raw_width - width)>>1);
    if (prefix > 0) {
        *target++ = 0x0;    // STRETCH
        *target++ = 0xa;
        *target++ = prefix;
        *target++ = space_chr;
        raw_width -= prefix;
    };

    /*** String itself ***/
    while (*target++ = *source++);
    target--;

    /*** nachfolgende Pixel ***/
    raw_width -= width;
    if (raw_width > 0) {
        *target++ = 0x0;    // STRETCH
        *target++ = 0xa;
        *target++ = raw_width;
        *target++ = space_chr;
    };

    /*** Ende ***/
    return(target);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_TextBuildCenteredItem(struct VFMFont *fnt,
                                UBYTE *target,
                                UBYTE *source,
                                LONG raw_width,
                                UBYTE space_chr)
/*
**  FUNCTION
**      Wie yw_BuildCenteredItem(), es muss sich aber um einen
**      "richtigen" Textstring handeln. Der String wird in der
**      DBCS Version als TrueType gerendert, ansonsten wird die
**      Routine automatisch auf yw_BuildClippedItem() umgeleitet.
**
**  CHANGED
**      24-Nov-97   floh    created
*/
{
    #ifdef __DBCS__
        /*** DBCS Handling ***/
        if (raw_width > 0) {
            LONG diff = raw_width;
            freeze_dbcs_pos(target);   // DBCS-String auf dieser Position rendern
        
            /*** Hintergrund fuellen ***/
            do {
                if (diff > 255) {
                    stretch(target,255);
                } else {
                    stretch(target,diff);
                };
                put(target,space_chr);
                diff -= 255;
            } while (diff > 0);

            /*** dann die DBCS Anweisung (centered) ***/
            put_dbcs(target,raw_width,DBCSF_CENTER,source);
        };
        return(target);
    #else
        return(yw_BuildCenteredItem(fnt,target,source,raw_width,space_chr));
    #endif
}

/*-----------------------------------------------------------------*/
UBYTE *yw_BuildReqTitle(struct ypaworld_data *ywd,
                        LONG x, LONG y, LONG w,
                        UBYTE *title,
                        UBYTE *str,
                        UBYTE postfix_char,
                        ULONG req_flags)
/*
**  FUNCTION
**      Universal-Funktion zum dynamischen Aufbau
**      eines Requester-Titels. Es wird *KEIN*
**      NewLine geschrieben. Der Title wird, wenn
**      notwendig geclippt.
**
**  INPUTS
**      ywd           - LID des Welt-Objects
**      x             - X-Position des Requesters
**      y             - Y-Position des Requesters
**      w             - Overall-Width
**      title         - Title-String
**      str           - Output-Stream
**      postfix_char  - optionales Zeichen, welches vor das CloseGadget
**                      gerendert wird. Muß im DEFAULT Font definiert sein!
**      close_char    - Char für Close Gadget
**      req_flags     - die aktuellen Requester-Flags
**
**  RESULTS
**      str - modifizierter Output-Stream.
**
**  CHANGED
**      27-Jul-96   floh    created
**      29-Jul-96   floh    <ignore_close> Argument, damit keine
**                          Probleme mit dem CloseGadget-Handling
**                          gibt.
**      10-Aug-96   floh    + Custom-Close-Gadget Support, das
**                            Close-Gadget muß Teil des FONTID_ICON_NS
**                            oder FONTID_ICON_PS sein!
**                            Wenn Close gedrückt wird, wird nur
**                            die FontID ausgetauscht!
**      21-Aug-96   floh    + <req_flags>, die Routine stellt jetzt
**                            anhand REQF_CloseDown selbst fest,
**                            wie das CloseGadget zu zeichnen ist.
**      05-Jul-97   floh    + Closegadget jetzt rechts...
**      08-Jul-97   floh    + 1 Leerzeichen vor dem Maptitel
**      28-Oct-97   floh    + als CloseGadget wird jetzt generell
**                            FONTID_ICON_#?/'U' genommen.
**                          + HelpGadget wird gerendert FONTID_ICON_#?/'A'
**      22-Nov-97   floh    + umgearbeitet fuer DBCS-Support
**      09-Dec-97   floh    + dbcs_color()
*/
{
    LONG close_w, help_w, postfix_w;
    UBYTE fnt_id;
    UBYTE title_buf[128];
    
    if (req_flags & REQF_HasCloseGadget) close_w = ywd->Fonts[FONTID_ICON_NS]->fchars['A'].width;
    else                                 close_w = 0;
    if (req_flags & REQF_HasHelpGadget)  help_w  = ywd->Fonts[FONTID_ICON_NS]->fchars['B'].width;
    else                                 help_w  = 0;   
    if (postfix_char) postfix_w = ywd->Fonts[FONTID_DEFAULT]->fchars[postfix_char].width;
    else              postfix_w = 0;

    /*** 1 Leerzeichen vor den Titel ***/
    strcpy(title_buf," ");
    strcat(title_buf,title);

    /*** Font, Position, Title ***/
    new_font(str,FONTID_MENUGRAY);
    pos_abs(str,x,y);
    put(str,'b');       // linker Rand
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEFAULT),yw_Green(ywd,YPACOLOR_TEXT_DEFAULT),yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT));
    str = yw_TextBuildClippedItem(ywd->Fonts[FONTID_MENUGRAY],
                                  str, title_buf,
                                  w - close_w - help_w - postfix_w - ywd->EdgeW,
                                  'c');

    /*** optionales Postfix-Char ***/
    if (postfix_char) {
        new_font(str,FONTID_DEFAULT);
        put(str,postfix_char);
    };

    /*** Help-Gadget ***/
    if (req_flags & REQF_HasHelpGadget) {
        if (req_flags & REQF_HelpDown) fnt_id = FONTID_ICON_PS;
        else                           fnt_id = FONTID_ICON_NS;
        new_font(str,fnt_id);
        put(str,'A');
    };

    /*** Close Gadget ***/
    if (req_flags & REQF_HasCloseGadget) {
        if (req_flags & REQF_CloseDown) fnt_id = FONTID_ICON_PS;
        else                            fnt_id = FONTID_ICON_NS;
        new_font(str,fnt_id);
        put(str,'U');
    };

    /*** das war's bereits ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutAlignedClippedString(struct ypaworld_data *ywd,
                                  UBYTE *out_str,
                                  struct ypa_ColumnItem *col)
/*
**  FUNCTION
**      Layoutet einen aligned String, mit Clipping. Geclippt
**      wird je nach Alignment rechts, links, oder beidseitig.
**      Der String wird NICHT durch eos() abgeschlossen!
**      Die Länge darf 255 nicht überschreiten!
**
**  RESULTS
**      <UBYTE *> zeigt auf Ende des Output-Streams
**
**  CHANGED
**      14-Oct-97   floh    created
*/
{
    struct VFMFont *fnt = ywd->Fonts[col->font_id];
    UBYTE *string       = col->string;
    LONG left_clip      = 0;   // soviel links wegclippen
    LONG right_clip     = 0;   // soviel rechts wegclippen
    LONG left_space     = 0;   // soviel links überspringen
    LONG right_space    = 0;   // soviel rechts überspringen
    LONG string_len     = yw_StrLen(string,fnt);
    LONG diff           = col->width - string_len;
    LONG use_string_len;
    UBYTE chr;

    if (col->flags & YPACOLF_DOPREFIX) {
        diff -= fnt->fchars[col->prefix_chr].width;
    };
    if (col->flags & YPACOLF_DOPOSTFIX) {
        diff -= fnt->fchars[col->postfix_chr].width;
    };
    new_font(out_str,col->font_id);
    if (diff > 0) {
        /*** String zu kurz ***/
        if (col->flags & YPACOLF_ALIGNLEFT) {
            right_space = diff;
        } else if (col->flags & YPACOLF_ALIGNRIGHT) {
            left_space = diff;
        } else if (col->flags & YPACOLF_ALIGNCENTER) {
            left_space  = diff>>1;
            right_space = diff-left_space;
        };
    } else if (diff < 0) {
        /*** String zu lang ***/
        if (col->flags & YPACOLF_ALIGNLEFT) {
            right_clip = -diff;
        } else if (col->flags & YPACOLF_ALIGNRIGHT) {
            left_clip  = -diff;
        } else if (col->flags & YPACOLF_ALIGNCENTER) {
            left_clip  = (-diff)>>1;
            right_clip = (-diff)-left_clip;
        };
    };
    use_string_len = string_len - right_clip - left_clip;

    /*** Prefix-Char ***/
    if (col->flags & YPACOLF_DOPREFIX) {
        put(out_str,col->prefix_chr);
    };

    /*** left clip ***/
    chr = 0;
    while (left_clip > 0) {
        chr = *string++;
        left_clip -= fnt->fchars[chr].width;
    };
    if (left_clip < 0) {
        /*** dieses Char links anschneiden ***/
        off_hori(out_str,(fnt->fchars[chr].width + left_clip));
        put(out_str,chr);
    };
    /*** Left Space ***/
    if (left_space > 0) {
        while (left_space > 255) {
            stretch(out_str,255);
            put(out_str,col->space_chr);
            left_space -= 255;
        };
        if (left_space > 0) {
            stretch(out_str,left_space);
            put(out_str,col->space_chr);
        };
    };

    /*** String itself ***/
    while ((use_string_len>0) && (chr=*string++)) {
        use_string_len -= fnt->fchars[chr].width;
        if (use_string_len >= 0) {
            put(out_str,chr);
        }else{
            /*** letzter Buchstabe rechts angeschnitten ***/
            len_hori(out_str,(fnt->fchars[chr].width + use_string_len));
            put(out_str,chr);
        };
    };

    /*** Right Space? ***/
    if (right_space > 0) {
        while (right_space > 255) {
            stretch(out_str,255);
            put(out_str,col->space_chr);
            right_space -= 255;
        };
        if (right_space > 0) {
            stretch(out_str,right_space);
            put(out_str,col->space_chr);
        };
    };

    /*** Postfix-Char ***/
    if (col->flags & YPACOLF_DOPOSTFIX) {
        put(out_str,col->postfix_chr);
    };

    /*** Ende ***/
    return(out_str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_TextPutAlignedClippedString(struct ypaworld_data *ywd,
                                      UBYTE *out_str,
                                      struct ypa_ColumnItem *col)
/*
**  FUNCTION
**      Wie yw_PutAlignedClippedString(), rendert aber im DBCS-
**      Modus TrueType. Im Nicht-DBCS-Modus wird einfach 
**      auf yw_PutAlignedClippedString() umgeleitet.  
**
**  CHANGED
**      24-Nov-97   floh    created
*/
{
    #ifdef __DBCS__
        struct VFMFont *fnt = ywd->Fonts[col->font_id];
        LONG raw_width = col->width;

        if (col->flags & YPACOLF_DOPREFIX) {
            raw_width -= fnt->fchars[col->prefix_chr].width;
        };
        if (col->flags & YPACOLF_DOPOSTFIX) {
            raw_width -= fnt->fchars[col->postfix_chr].width;
        };

        /*** Font einstellen ***/
        new_font(out_str,col->font_id);

        /*** Prefix-Char ***/
        if (col->flags & YPACOLF_DOPREFIX) {
            put(out_str,col->prefix_chr);
        };
   
        if (raw_width > 0) {
            LONG diff = raw_width;
            ULONG flags = 0;
            freeze_dbcs_pos(out_str);   // DBCS-String auf dieser Position rendern
        
            /*** Hintergrund fuellen ***/
            do {
                if (diff > 255) {
                    stretch(out_str,255);
                } else {
                    stretch(out_str,diff);
                };
                put(out_str,col->space_chr);
                diff -= 255;
            } while (diff > 0);

            if (col->flags & YPACOLF_ALIGNLEFT)       flags |= DBCSF_LEFTALIGN;
            else if (col->flags & YPACOLF_ALIGNRIGHT) flags |= DBCSF_RIGHTALIGN;
            else                                      flags |= DBCSF_CENTER;

            /*** dann die DBCS Anweisung ***/
            put_dbcs(out_str,raw_width,flags,col->string);
        };          

        /*** Postfix-Char ***/
        if (col->flags & YPACOLF_DOPOSTFIX) {
            put(out_str,col->postfix_chr);
        };

        /*** Ende ***/
        return(out_str);
    
    #else
        yw_PutAlignedClippedString(ywd,out_str,col);
    #endif
}

/*-----------------------------------------------------------------*/
UBYTE *yw_BuildColumnItem(struct ypaworld_data *ywd,
                          UBYTE  *out_str,
                          ULONG  num_columns,
                          struct ypa_ColumnItem *columns)
/*
**  FUNCTION
**      Multifunktions-Spalten-Layouter. Layoutet eine
**      Zeile aus mehreren Spalten, für jede Spalte muß
**      angegeben werden:
**          - ein Textstring
**          - eine Font-ID
**          - ein Alignment-Code (links-,rechts-aligned, oder zentriert)
**          - ein Füllzeichen
**          - eine Spalten-Breite
**      <struct ypa_Column> ist deklariert in ypagui.h.
**      Für jede Spalte wird der Font neu eingestellt, geclippt
**      wird nach Alignemt-Regel (bei left-aligned rechts, bei
**      right-aligned links, bei zentriert beidseitig).
**      Falls die Routine für Listviews verwendet wird, müssen
**      die Listview-Item-Ränder extern gerendert werden.
**
**      Der String wird NICHT durch eos() abgeschlossen.
**
**  CHANGED
**      14-Oct-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<num_columns; i++) {
        struct ypa_ColumnItem *c = &(columns[i]);
        if (c->flags & YPACOLF_TEXT) {
            out_str = yw_TextPutAlignedClippedString(ywd,out_str,c);
        } else {
            out_str = yw_PutAlignedClippedString(ywd,out_str,c);
        };
    };
    return(out_str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_TextRelWidthItem(struct VFMFont *fnt, 
                           UBYTE *target, UBYTE *source, 
                           LONG relwidth, ULONG align)
/*
**  FUNCTION
**      Zeichnet ein Textitem prozentual geclippt zu seiner Gesamt-Breite...
**      <relwidth> gibt die zu zeichnende Breite in % an. 
**
**  CHANGED
**      25-Nov-97   floh    created
**      28-Nov-97   floh    + Alignment-Code
*/
{
    #ifdef __DBCS__
        /*** DBCS Handling ***/
        ULONG flags = DBCSF_RELWIDTH;
        freeze_dbcs_pos(target);   // DBCS-String auf dieser Position rendern
        switch(align) {
            case YPACOLF_ALIGNLEFT:  flags|=DBCSF_LEFTALIGN; break;
            case YPACOLF_ALIGNRIGHT: flags|=DBCSF_RIGHTALIGN; break;
            default:                 flags|=DBCSF_CENTER; break;
        };
        put_dbcs(target,relwidth,flags,source);
        return(target);
    #else
        /*** konventionell: Laenge des Strings in Pixel ***/
        LONG width = yw_StrLen(source,fnt);
        LONG real_width = (relwidth * width) / 100;
        switch(align) {
            case YPACOLF_ALIGNRIGHT:
                xpos_rel(target,-real_width);
                break;
            case YPACOLF_CENTER:
                xpos_rel(target,(-(real_width>>1)));
                break;
        };
        target = yw_BuildClippedItem(fnt,target,source,real_width,' ');
        return(target);
    #endif
}


    
