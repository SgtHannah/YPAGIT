/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_statusbar.c,v $
**  $Revision: 38.26 $
**  $Date: 1998/01/06 16:27:24 $
**  $Locker: floh $
**  $Author: floh $
**
**  Der Status-Bar ist ein "normaler" Requester....
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "bitmap/ilbmclass.h"
#include "ypa/guilist.h"
#include "ypa/guimap.h"
#include "ypa/guifinder.h"
#include "ypa/guilogwin.h"
#include "ypa/ypabactclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypatooltips.h"
#include "ypa/guiabort.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypakeys.h"

#include "yw_protos.h"

#ifdef YPA_DESIGNMODE
#define YPA_OLDMENUES    (1)
#endif

#ifdef YPA_CUTTERMODE
#define YPA_OLDMENUES    (1)
#endif

#ifdef __WINDOWS__
extern unsigned long wdd_DoDirect3D;
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

/*** HACK, damit die QSort-Hooks wissen, wo YWD ist ***/
struct ypaworld_data *__qsort_ywd;

extern struct YPAMapReq MR;
extern struct YPAFinder FR;
extern struct YPALogWin LW;
extern struct YPAAbortReq AMR;

/*** der ReqString wird voll dynamisch layoutet ***/
BYTE SR_ReqString[1024] = {NULL};
BYTE USR_ReqString[64]  = {NULL};

/*** Statusbar ClickButtons... ***/
struct ClickButton SR_Mode;         // Old Style Statusbar Only
struct ClickButton SR_SubMode;

struct ClickButton SR_New;
struct ClickButton SR_Control;
struct ClickButton SR_Map;
struct ClickButton SR_Finder;
struct ClickButton SR_ToRobo;
struct ClickButton SR_ToCom;
struct ClickButton SR_NextUnit;
struct ClickButton SR_NextCom;
struct ClickButton SR_Help;
struct ClickButton SR_OnlineHelp;
struct ClickButton SR_Quit;

#ifdef YPA_DESIGNMODE
struct ClickButton SR_SetSector;
struct ClickButton SR_SetOwner;
struct ClickButton SR_SetHeight;
#endif
#ifdef YPA_CUTTERMODE
struct ClickButton SR_Player;
struct ClickButton SR_FreeStativ;
struct ClickButton SR_LinkStativ;
#endif

/*** der StatusReq itself ***/
struct YPAStatusReq SR = { NULL };

/*** das PopUp Menü des StatusBars ***/
struct YPAListReq SubMenu    = { NULL };

/*-----------------------------------------------------------------*/
void yw_OpenStatusBar(void)
/*
**  FUNCTION
**      Enabled den StatusBar und hängt ihn in die ClickBox-
**      Liste der Input-Engine...
**
**      Die Routine übernimmt Teilaufgaben, die normalerweise
**      von YWM_ADDREQUESTER übernommen werden. Der StatusBar
**      hat hier aber eine Sonderstellung...
**
**      Falls der StatusBar schon offen ist, passiert nichts.
**
**  CHANGED
**      12-Dec-95   floh    created
**      17-Dec-95   floh    req->req_cbox.userdata bleibt jetzt auf NULL
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    if (SR.EnabledReqs & STAT_REQF_STATUS) {
        if ((SR.OpenReqs & STAT_REQF_STATUS) == 0) {
            SR.OpenReqs |= STAT_REQF_STATUS;
            _AddClickBox(&(SR.req.req_cbox), IE_CBX_ADDHEAD);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_CloseStatusBar(void)
/*
**  FUNCTION
**      Schließt StatusBar. Falls er schon geschlossen
**      ist, passiert nix. Die Routine übernimmt teilweise
**      Aufgaben von YWM_REMREQUESTER.
**
**  CHANGED
**      12-Dec-95   floh    created
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    if (SR.OpenReqs & STAT_REQF_STATUS) {
        SR.OpenReqs &= ~STAT_REQF_STATUS;
        _RemClickBox(&(SR.req.req_cbox));
    };
}

/*-----------------------------------------------------------------*/
void yw_CloseReq(struct ypaworld_data *ywd,struct YPAReq *req)
/*
**  FUNCTION
**      Schließt das entsprechende PopUp Menü. Falls es
**      schon geschlossen ist, passiert nichts.
**
**  CHANGED
**      12-Dec-95   floh    created
**      13-Dec-95   floh    debugging...
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    /*** nur schließen, falls offen ***/
    if ((req->flags & REQF_Closed) == 0) {
        req->flags |= REQF_Closed;
        _RemClickBox(&(req->req_cbox));
        ywd->DragLock = FALSE;
    };
}

/*-----------------------------------------------------------------*/
void yw_OpenReq(struct ypaworld_data *ywd, struct YPAReq *req)
/*
**  FUNCTION
**      Öffnet eines der folgenden PopUp-Menüs:
**
**          ModeMenu
**          SubMenu
**
**      Falls das Menü schon offen ist, passiert nichts.
**
**  INPUT
**      req     -> Ptr auf YPAReq-Struktur des Menüs
**
**  CHANGED
**      12-Dec-95   floh    created
**      13-Dec-95   floh    debugging...
**      17-Dec-95   floh    req->req_cbox.userdata bleibt jetzt auf NULL
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    /*** nur etwas machen, falls Req gerade "zu" ***/
    if (req->flags & REQF_Closed) {
        req->flags &= ~REQF_Closed;
        _AddClickBox(&(req->req_cbox), IE_CBX_ADDHEAD);
        ywd->DragLock = FALSE;
    };
}

/*-----------------------------------------------------------------*/
void yw_ReqToFront(struct ypaworld_data *ywd, struct YPAReq *req)
/*
**  FUNCTION
**      Bringt einen Requester visuell und in der ClickBox-Liste
**      nach vorn. Der Requester muß per YWM_ADDREQUESTER
**      eingeklinkt worden sein.
**
**  CHANGED
**      08-Feb-98   floh    created
*/
{
    if ((req->flags&REQF_Closed)==0) {
        _Remove((struct Node *)req);
        _AddHead((struct List *)&(ywd->ReqList),(struct Node *)req);
        _RemClickBox(&(req->req_cbox));
        _AddClickBox(&(req->req_cbox), IE_CBX_ADDHEAD);
    };
}

/*-----------------------------------------------------------------*/
void yw_CheckWinStatus(struct ypaworld_data *ywd,
                       struct YPAWinStatus *win_stat)
/*
**  FUNCTION
**      Testet, ob die <win_stat> gueltig ist, und die
**      Rechteck-Koords innerhalb des Displays
**      liegen. Ist dem nicht so, wird der Status auf
**      invalid gesetzt. 
**
**  CHANGED
**      30-May-98   floh    created
*/
{
    if (win_stat->IsValid) {
        if ((win_stat->Rect.y < ywd->UpperTabu) ||
            ((win_stat->Rect.x + win_stat->Rect.w) > ywd->DspXRes) ||
            ((win_stat->Rect.y + win_stat->Rect.h) > (ywd->DspYRes-ywd->LowerTabu)))
        {
            win_stat->IsValid = FALSE;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_OpenSubMenu(struct ypaworld_data *ywd,
                    BOOL immediate,
                    ULONG for_mode)
/*
**  FUNCTION
**      Öffnet das Submenu an einer gegebenen Screenpos,
**      im Immediate-, oder Sticky-Mode.
**
**  CHANGED
**      24-Jul-97   floh    created
*/
{
    if ((SR.ActiveMode & for_mode) && (!(SubMenu.Req.flags & REQF_Closed))) {
        /*** gleicher Mode und bereits offen -> schließen ***/
        yw_CloseReq(ywd,&(SubMenu.Req));
    }else{
        /*** war zu, oder anderer Mode, also öffnen ***/
        yw_OpenReq(ywd,&(SubMenu.Req));
        if (immediate) SubMenu.Flags |= (LISTF_Select|LISTF_NoScroll);
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_InitStatusReq(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**
**  CHANGED
**      07-Dec-95   floh    created
**      11-Dec-95   floh    + SubSubMode
**      12-Dec-95   floh    + debugging...
**                          + Init der PopUpMenüs
**      13-Dec-95   floh    + ruft jetzt am Ende yw_LayoutSR()
**                            auf, damit yw_HandleInputSR() eine
**                            Arbeitsgrundlage hat
**      22-Dec-95   floh    + Änderungen für "Selected Zeugs"
**      29-Dec-95   floh    + Mauspointer-Handling jetzt ausgelagert
**      10-Jan-96   floh    + AutoPilot-Modus
**      21-Mar-96   floh    - SubSubMenu
**                          + dynamische Breite des SubMode-Menus
**                          + besseres Layout für höhere Auflösungen
**      08-Apr-96   floh    + SubMenu-Listview auf 12 Entries, ehe
**                            Scrollbar gezeichnet wird
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
**      06-Aug-96   floh    + umgestellt auf globale Icon-Size
**                            Layout-Variablen
**      06-Aug-96   floh    + ModeMenu experimentell als Icon-Only
**                            Balken
**      07-Sep-96   floh    + Designer und Cutter: Old Style Menues,
**                            sonst vollständig auf Icon-Menues
**                            umgestellt.
**      28-Sep-96   floh    + Control-2-Last-Message Button
**      08-Apr-97   floh    + SubModeW ist jetzt 12*sizeof('W')
**      24-Jul-97   floh    + neues Statusbar-Modell (kein aufklappbares
**                            Modemenu)
**      29-Sep-97   floh    + LogWin Icon raus
**      13-Oct-97   floh    + EnergyWindow Icon raus
**      14-Oct-97   floh    + SubMode-Menu verbreitert wegen Kosten-Spalte
**      16-Oct-97   floh    + verschiedene Submenu-Breiten für Low/HiRes
**      04-Dec-97   floh    + Add + Build Button rausgeflogen
**      17-Feb-98   floh    + Submenu-Initialisierung mit
**                            LIST_KeyboardInput ausgestattet.
**      07-Apr-98   floh    + Robo/Vehicle Window Status Handling
**      30-May-98   floh    + Window-Status-Handling geaendert
**
**  NOTE
**      Das <userdata> Field bleibt mit Absicht auf NULL, damit
**      ist dieser Requester als "privat" deklariert, yw_HandleInput()
**      wird also NICHT solche Sachen wie ToFront etc... abhandeln.
*/
{
    ULONG btn;
    struct YPAWinStatus *win_stat;

    /*** erstmal alles löschen ***/
    memset(&SR,0,sizeof(SR));

    /*** lokale Layout-Konstanten initialisieren ***/
    SR.GroupSpace = (ywd->DspXRes-(STAT_LBAR_NUMBUTTONS*ywd->IconBW))/4; // 4 Gruppen!

    if (ywd->DspXRes < 512) {
        SR.SubModeW  = ywd->Fonts[FONTID_DEFAULT]->fchars['W'].width * 18;
    } else {
        SR.SubModeW  = ywd->Fonts[FONTID_DEFAULT]->fchars['W'].width * 14;
    };
    SR.ModeStart = SR.GroupSpace/2;
    SR.WinStart  = SR.ModeStart + SR.GroupSpace + ywd->IconBW;
    SR.NavStart  = SR.WinStart  + SR.GroupSpace + 2*ywd->IconBW;
    SR.QuitStart = SR.NavStart  + SR.GroupSpace + 5*ywd->IconBW;
    SR.NumLines  = 1;
    SR.LBarStart = 0;
    #ifdef YPA_DESIGNMODE
        SR.NumLines  = 2;               
        SR.LBarStart = ywd->IconBH;
    #endif
    #ifdef YPA_CUTTERMODE
        SR.NumLines  = 2;
        SR.LBarStart = ywd->IconBH;
    #endif

    /*** Requester-Struktur initialisieren ***/
    SR.req.flags = 0;   // kein Icon, kein DragBar, kein CloseGadget!

    SR.req.req_cbox.rect.x = 0;
    SR.req.req_cbox.rect.y = ywd->DspYRes - (ywd->IconBH*SR.NumLines);
    SR.req.req_cbox.rect.w = ywd->DspXRes;
    SR.req.req_cbox.rect.h = ywd->IconBH*SR.NumLines;

    SR.req.req_cbox.num_buttons = STAT_NUMBUTTONS;

    SR.req.req_cbox.buttons[STATBTN_NEW]        = &SR_New;
    SR.req.req_cbox.buttons[STATBTN_CONTROL]    = &SR_Control;
    SR.req.req_cbox.buttons[STATBTN_MAP]        = &SR_Map;
    SR.req.req_cbox.buttons[STATBTN_FINDER]     = &SR_Finder;
    SR.req.req_cbox.buttons[STATBTN_TOROBO]     = &SR_ToRobo;
    SR.req.req_cbox.buttons[STATBTN_TOCOM]      = &SR_ToCom;
    SR.req.req_cbox.buttons[STATBTN_NEXTUNIT]   = &SR_NextUnit;
    SR.req.req_cbox.buttons[STATBTN_NEXTCOM]    = &SR_NextCom;
    SR.req.req_cbox.buttons[STATBTN_HELP]       = &SR_Help;
    SR.req.req_cbox.buttons[STATBTN_ONLINEHELP] = &SR_OnlineHelp;
    SR.req.req_cbox.buttons[STATBTN_QUIT]       = &SR_Quit;
    #ifdef YPA_DESIGNMODE
        SR.req.req_cbox.buttons[STATBTN_SETSECTOR] = &SR_SetSector;
        SR.req.req_cbox.buttons[STATBTN_SETHEIGHT] = &SR_SetHeight;
        SR.req.req_cbox.buttons[STATBTN_SETOWNER]  = &SR_SetOwner;
    #endif
    #ifdef YPA_CUTTERMODE
        SR.req.req_cbox.buttons[STATBTN_PLAYER]     = &SR_Player;
        SR.req.req_cbox.buttons[STATBTN_FREESTATIV] = &SR_FreeStativ;
        SR.req.req_cbox.buttons[STATBTN_LINKSTATIV] = &SR_LinkStativ;
    #endif

    /*** alle Buttons löschen ***/
    for (btn=0; btn<STAT_NUMBUTTONS; btn++) {
        struct ClickButton *cb = SR.req.req_cbox.buttons[btn];
        memset(cb,0,sizeof(struct ClickButton));
    };
    SR.req.req_string  = SR_ReqString;

    /*** Requester-Erweiterung initialisieren ***/
    SR.OpenReqs = 0;
    yw_StatusEnable(ywd);

    SR.ActVehicle  = 0;
    SR.ActWeapon   = 0;
    SR.ActBuilding = 0;
    SR.ActThing    = 0;

    /*** Status-Bar pro forma öffnen ***/
    yw_OpenStatusBar();

    /*** SubMenu initialisieren ***/
    if (!yw_InitListView(ywd, &SubMenu,
                        LIST_Resize,        FALSE,
                        LIST_NumEntries,    8,
                        LIST_ShownEntries,  8,
                        LIST_FirstShown,    0,
                        LIST_Selected,      0,
                        LIST_MaxShown,      16,
                        LIST_DoIcon,        FALSE,
                        LIST_EntryHeight,   ywd->FontH,
                        LIST_EntryWidth,    SR.SubModeW,
                        LIST_Enabled,       TRUE,
                        LIST_VBorder,       ywd->EdgeH,
                        LIST_ImmediateInput, TRUE,
                        LIST_KeyboardInput, TRUE,
                        TAG_DONE))
    {
        /*** Initialisierung Sub-Mode-Menu ging schief ***/
        return(FALSE);
    };
    
    /*** wuerden die Fenster ins aktuelle Display passen? ***/
    yw_CheckWinStatus(ywd,&(ywd->Prefs.RoboMapStatus));
    yw_CheckWinStatus(ywd,&(ywd->Prefs.RoboFinderStatus));
    yw_CheckWinStatus(ywd,&(ywd->Prefs.VhclMapStatus));
    yw_CheckWinStatus(ywd,&(ywd->Prefs.VhclFinderStatus));
    
    /*** spezieller ZipZap-Status-Check fuer die Map ***/
    if ((ywd->Prefs.RoboMapStatus.Data[5]==0)||(ywd->Prefs.RoboMapStatus.Data[6]==0))
    {
        ywd->Prefs.RoboMapStatus.IsValid = FALSE;
    };
    if ((ywd->Prefs.VhclMapStatus.Data[5]==0)||(ywd->Prefs.VhclMapStatus.Data[6]==0))
    {
        ywd->Prefs.VhclMapStatus.IsValid = FALSE;
    };

    /*** Fensterstatus initialisieren ***/     
    if (ywd->Prefs.RoboMapStatus.IsValid) {
        if (ywd->Prefs.RoboMapStatus.IsOpen) {
            yw_OpenReq(ywd,&(MR.req));
            yw_ReqToFront(ywd,&(MR.req));
        } else yw_CloseReq(ywd,&(MR.req));
        MR.req.req_cbox.rect = ywd->Prefs.RoboMapStatus.Rect;
        MR.layers    = ywd->Prefs.RoboMapStatus.Data[0];
        MR.lock_mode = ywd->Prefs.RoboMapStatus.Data[1];
        MR.zoom      = ywd->Prefs.RoboMapStatus.Data[2];
        MR.zip_zap.x = ywd->Prefs.RoboMapStatus.Data[3];
        MR.zip_zap.y = ywd->Prefs.RoboMapStatus.Data[4];
        MR.zip_zap.w = ywd->Prefs.RoboMapStatus.Data[5];
        MR.zip_zap.h = ywd->Prefs.RoboMapStatus.Data[6];
        yw_MapZoom(ywd,MAP_ZOOM_CORRECT);
    };
    if (ywd->Prefs.RoboFinderStatus.IsValid) {
        if (ywd->Prefs.RoboFinderStatus.IsOpen) {
            yw_OpenReq(ywd,&(FR.l.Req));
            yw_ReqToFront(ywd,&(FR.l.Req));
        } else yw_CloseReq(ywd,&(FR.l.Req));
        FR.l.Req.req_cbox.rect = ywd->Prefs.RoboFinderStatus.Rect;
        yw_ListSetRect(ywd,&(FR.l),-2,-2);
    };

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillStatusReq(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Gibt alle Resourcen frei, die am Status-Requester hängen.
**
**  CHANGED
**      07-Dec-95   floh    created
**      12-Dec-95   floh    Killing der Popup-Menüs
**      29-Dec-95   floh    Mauspointer-Handling jetzt ausgelagert
**      21-Mar-96   floh    - SubSubMenu
**      10-Jul-96   floh    kein Test mehr, ob Requester auch offen.
**                          Dieser Test war ungültig, wenn bei der
**                          Initialisierung kein Robo existierte (-> Player!)
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
**      24-Jul-97   floh    + neuer Statusbar
*/
{
    /*** falls noch offen, PopUp schließen ***/
    yw_CloseReq(ywd,&(SubMenu.Req));

    /*** StatusBar schließen ***/
    yw_CloseStatusBar();

    /*** Popup-Menüs killen ***/
    yw_KillListView(ywd,&SubMenu);

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_RenderStatusReq(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert den Status-Req + offene Popup-Menüs. Der
**      Status-Req hat einen Sonderstatus innerhalb des
**      GUIs und wird deshalb nicht einfach so in die
**      Requester-Liste eingebunden...
**
**  CHANGED
**      12-Dec-95   floh    created
**      21-Mar-96   floh    - SubSubMenu
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
**      24-Jul-97   floh    + neuer Statusbar
*/
{
    struct drawtext_args dt;

    /*** den Status-Bar... ***/
    if (!(SR.req.flags & REQF_Closed)) {
        dt.string = SR.req.req_string;
        dt.clips  = SR.req.req_clips;
        _DrawText(&dt);

        /*** SubMenu ***/
        if (!(SubMenu.Req.flags & REQF_Closed)) {
            dt.string = SubMenu.Req.req_string;
            dt.clips  = SubMenu.Req.req_clips;
            _DrawText(&dt);
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_StatusEnable(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Aktualisiert
**
**          SR.EnabledModes
**          SR.ActiveMode
**          SR.EnabledReqs
**          SR.OpenReqs
**
**  CHANGED
**      07-Dec-95   floh    created
**      12-Dec-95   floh    die PopUp-Reqs sind nicht mehr eigenständig...
**      26-Dec-95   floh    die meisten der Req-Flags existieren nicht
**                          mehr
**      10-Jan-96   floh    + Autopilot
**                          - falls in einem Vehicle: kein Fight und
**                            Merge mehr
**      21-Mar-96   floh    MERGE generall gekillt, das wird alles im
**                          Finder per Drag'n'Drop erledigt
**      23-May-96   floh    + Designer-Modes...
**      24-May-96   floh    - PANIC Modus removed, weil das ja
**                            jetzt per Aggr-Scroller funktioniert
**      13-Jul-96   floh    + Cutter-Modus
**      22-Jul-96   floh    + falls User in einem Robo-Geschütz sitzt,
**                            wird als Sonderfall das komplette Robo-
**                            Menü beibehalten (aber alles andere
**                            als der Fight-Modus katapultiert den
**                            User zurück in das Robo-Cockpit)
**                          + falls SR.NumWeapons|SR.NumVehicle|SR.NumBuildings
**                            Null, wird der entsprechende Modus disabled
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
**      24-Jul-97   floh    + Add-Modus nur noch enabled, wenn auch ein
**                            ausgewähltes Geschwader existiert.
**                          + Order-Modus nur noch enabled, wenn auch
**                            ein ausgewähltes Geschwader existiert
**      14-Aug-97   floh    + Control-Modus wird generell wieder
**                            eingeschaltet (wenn der User keine Vehicles,
**                            sondern nur Flaks hatte, konnte er sich nicht
**                            in diese reinschalten)
**      07-Dec-97   floh    + in RoboFlak sind jetzt alle Modes zugelassen,
**                            die auch im Robo zugelassen sind
**      08-Feb-98   floh    + falls Robo nicht im Normalzustand ist, werden
**                            diverse Sachen nicht zugelassen
*/
{
    if (ywd->UserVehicle) {
        if ((ywd->UserRobo == ywd->UserVehicle) || (ywd->UserSitsInRoboFlak)) {

            /*** User sitzt im Robo ***/
            SR.EnabledModes = STAT_MODEF_ORDER|STAT_MODEF_AUTOPILOT|STAT_MODEF_CONTROL;
            if (SR.NumVehicles > 0) {
                SR.EnabledModes  |= STAT_MODEF_NEW;
                if (ywd->ActCmdr != -1) SR.EnabledModes |= STAT_MODEF_ADD;
            };
            if (SR.NumBuildings > 0) SR.EnabledModes |= STAT_MODEF_BUILD;
            SR.EnabledReqs = STAT_REQF_STATUS;

        } else {

            /*** User sitzt in einem ordinären Vehikel ***/
            SR.EnabledModes = STAT_MODEF_AUTOPILOT|STAT_MODEF_ORDER|STAT_MODEF_CONTROL;
            SR.EnabledReqs = STAT_REQF_STATUS;
        };

        /*** wenn Robo tot, geht nix mehr ***/
        if (ACTION_DEAD == ywd->URBact->MainState) {
            SR.EnabledModes = 0;
        };

        #ifdef YPA_DESIGNMODE
            SR.EnabledModes |= STAT_MODEF_SETSECTOR|
                               STAT_MODEF_SETHEIGHT|
                               STAT_MODEF_SETOWNER;
        #endif
        #ifdef YPA_CUTTERMODE
            SR.EnabledModes = STAT_MODEF_FREESTATIV|
                              STAT_MODEF_LINKSTATIV|
                              STAT_MODEF_ONBOARD1|
                              STAT_MODEF_ONBOARD2|
                              STAT_MODEF_PLAYER;
        #endif

    } else {
        /*** Ooops, kein User-Vehicle da... ***/
        SR.EnabledModes = 0;
        SR.EnabledReqs  = 0;
    };

    /*** nicht existente Sachen ausblenden ***/
    SR.ActiveMode &= SR.EnabledModes;
    SR.OpenReqs   &= SR.EnabledReqs;
    if (SR.ActiveMode == 0) {
        SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
        #ifdef YPA_CUTTERMODE
            SR.ActiveMode = STAT_MODEF_PLAYER;
        #endif
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_SRSyncActThingVhclBlg(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Ermittelt aus dem gerade ausgewaehlten "Thing-Element"
**      ActVehicle oder ActBuilding, und schaltet je nachdem den 
**      NEW oder BUILD Modus an.
**
**  CHANGED
**      04-Dec-97   floh    created
*/
{
    if (SR.ActThing < SR.NumVehicles) {
        /*** ein Vehicle ist ausgewaehlt ***/
        SR.ActVehicle  = SR.ActThing;
        SR.ActBuilding = -1;
        if (!(SR.ActiveMode & STAT_MODEF_ADD)) SR.ActiveMode = STAT_MODEF_NEW;
    } else {
        /*** ein Building ist ausgewaehlt (unmoeglich im ADD Modus) ***/
        SR.ActVehicle  = -1;
        SR.ActBuilding = SR.ActThing - SR.NumVehicles;
        SR.ActiveMode  = STAT_MODEF_BUILD;
    };
}

/*-----------------------------------------------------------------*/
void yw_RemapWeapons(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      SR.WPRemap[] und SR.NumWeapons werden ausgefüllt.
**
**      Es werden also sozusagen die Waffen aus dem globalen
**      WeaponPrototype-Array rausgefiltert, die der aktuelle
**      Robo bauen kann...
**
**  CHANGED
**      18-Dec-95   floh    created
**      22-Dec-95   floh    Detail-Änderungen...
**      03-Jan-96   floh    falls genau 1 Waffe vorhanden, wird diese aktiv
**      22-Jul-96   floh    + anstatt WeaponProtos werden jetzt Indizes
**                            in das GunData-Array des UserRobos geremapped
**                          + ActWeapon wird manipuliert, falls User
**                            in einem Robo-Geschütz sitzt (damit werden
**                            komische Nebeneffekte verhindert, wenn der
**                            User sich NICHT über STAT_MODEF_FIGHT
**                            (z.B. per Control) in eine Robo-Flak setzt!
**      27-Jul-96   floh    + Bugfix betreffs SR.ActWeapon
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
*/
{
    ULONG i;
    ULONG num_wpns  = 0;
    struct gun_data *gd;

    _get(ywd->UserRobo, YRA_GunArray, &gd);

    for (i=0; i<NUMBER_OF_GUNS; i++) {
        if (gd[i].go) {
            SR.WPRemap[num_wpns] = i;
            if (gd[i].go == ywd->UserVehicle) SR.ActWeapon = num_wpns;
            num_wpns++;
        };
    };
    SR.NumWeapons = num_wpns;

    /*** Sanity Check ***/
    if (num_wpns == 0)      SR.ActWeapon = -1;
    else if (num_wpns == 1) SR.ActWeapon = 0;
    else if (SR.ActWeapon >= num_wpns) SR.ActWeapon = num_wpns-1;
}

/*-----------------------------------------------------------------*/
int yw_SRVhclCompareHook(const void *elm1, const void *elm2)
/*
**  FUNCTION
**      Sortiert die Vehicle im Remap-Array nach Typ und Kosten.
**
**  CHANGED
**      15-Oct-97   floh    created
*/
{
    struct ypaworld_data *ywd = __qsort_ywd;
    ULONG vp_num1 = *((UBYTE *)elm1);
    ULONG vp_num2 = *((UBYTE *)elm2);
    struct VehicleProto *vp1 = &(ywd->VP_Array[vp_num1]);
    struct VehicleProto *vp2 = &(ywd->VP_Array[vp_num2]);
    ULONG vp1_weight, vp2_weight;

    /*** Wichtungen für Kategorien ***/
    switch (vp1->BactClassID) {
        case BCLID_YPABACT:  vp1_weight=1;  break;
        case BCLID_YPATANK:  vp1_weight=0;  break;
        case BCLID_YPAROBO:  vp1_weight=5;  break;
        case BCLID_YPAFLYER: vp1_weight=2;  break;
        case BCLID_YPAUFO:   vp1_weight=3;  break;
        case BCLID_YPACAR:   vp1_weight=0;  break;
        case BCLID_YPAGUN:   vp1_weight=4;  break;
        default:        vp1_weight=10; break;
    };
    switch (vp2->BactClassID) {
        case BCLID_YPABACT:  vp2_weight=1;  break;
        case BCLID_YPATANK:  vp2_weight=0;  break;
        case BCLID_YPAROBO:  vp2_weight=5;  break;
        case BCLID_YPAFLYER: vp2_weight=2;  break;
        case BCLID_YPAUFO:   vp2_weight=3;  break;
        case BCLID_YPACAR:   vp2_weight=0;  break;
        case BCLID_YPAGUN:   vp2_weight=4;  break;
        default:        vp2_weight=10; break;
    };

    /*** Sortierung nach Kategorie und Kosten ***/
    if      (vp1_weight  < vp2_weight)  return(-1);
    else if (vp1_weight  > vp2_weight)  return(+1);
    else if (vp1->Energy < vp2->Energy) return(-1);
    else if (vp1->Energy > vp2->Energy) return(+1);
    else return(0);
}

/*-----------------------------------------------------------------*/
int yw_SRBlgCompareHook(const void *elm1, const void *elm2)
/*
**  FUNCTION
**      Sortiert die Buildings im Remap-Array nach Kosten.
**
**  CHANGED
**      15-Oct-97   floh    created
*/
{
    struct ypaworld_data *ywd = __qsort_ywd;
    ULONG bp_num1 = *((UBYTE *)elm1);
    ULONG bp_num2 = *((UBYTE *)elm2);
    struct BuildProto *bp1 = &(ywd->BP_Array[bp_num1]);
    struct BuildProto *bp2 = &(ywd->BP_Array[bp_num2]);
    ULONG bp1_weight, bp2_weight;

    /*** Wichtungen für Kategorien ***/
    switch (bp1->BaseType) {
        case BUILD_BASE_KRAFTWERK:    bp1_weight=0;  break;
        case BUILD_BASE_RADARSTATION: bp1_weight=2;  break;
        case BUILD_BASE_DEFCENTER:    bp1_weight=1;  break;
        default:                      bp1_weight=10; break;
    };
    switch (bp2->BaseType) {
        case BUILD_BASE_KRAFTWERK:    bp2_weight=0;  break;
        case BUILD_BASE_RADARSTATION: bp2_weight=2;  break;
        case BUILD_BASE_DEFCENTER:    bp2_weight=1;  break;
        default:                      bp2_weight=10; break;
    };

    /*** Sortierung nach Kategorie und Kosten ***/
    if      (bp1_weight  < bp2_weight)    return(-1);
    else if (bp1_weight  > bp2_weight)    return(+1);
    else if (bp1->CEnergy < bp2->CEnergy) return(-1);
    else if (bp1->CEnergy > bp2->CEnergy) return(+1);
    else return(0);
}

/*-----------------------------------------------------------------*/
int yw_SRCmdrCompareHook(struct Bacterium **p_b1, struct Bacterium **p_b2)
/*
**  FUNCTION
**      Sortiert Commander im CmdrRemap-Array
**
**  CHANGED
**      12-Jun-98   floh    created
*/
{
    struct Bacterium *b1 = *p_b1;
    struct Bacterium *b2 = *p_b2;
    LONG val1 = (LONG) b1->CommandID;
    LONG val2 = (LONG) b2->CommandID;
    return(val1-val2);
}

/*-----------------------------------------------------------------*/
void yw_RemapVehicles(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      SR.VPRemap[] und SR.NumVehicles werden ausgefüllt.
**
**  CHANGED
**      18-Dec-95   floh    created
**      22-Dec-95   floh    siehe yw_RemapWeapons()
**      03-Jan-96   floh    falls genau 1 Vehicle vorhanden, wird dieses
**                          aktiv
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      12-Apr-97   floh    + VP_Array nicht mehr global
**      15-Oct-97   floh    + Sortierung
*/
{
    ULONG i;
    ULONG num_vhcls  = 0;
    UBYTE footprint = 0;

    if (ywd->URBact) footprint = (1 << (ywd->URBact->Owner));

    for (i=0; i<NUM_VEHICLEPROTOS; i++) {
        if (ywd->VP_Array[i].FootPrint & footprint) {
            SR.VPRemap[num_vhcls] = i;
            num_vhcls++;
        };
    };
    SR.NumVehicles = num_vhcls;

    /*** sortiere nach Fahrzeug-Typ und Kosten ***/
    __qsort_ywd = ywd;
    qsort(SR.VPRemap, num_vhcls, sizeof(SR.VPRemap[0]), yw_SRVhclCompareHook);

    /*** Sanity Check ***/
    if (num_vhcls == 0)      SR.ActVehicle = -1;
    else if (num_vhcls == 1) SR.ActVehicle = 0;
    else if (SR.ActVehicle >= num_vhcls) SR.ActVehicle = num_vhcls-1;
}

/*-----------------------------------------------------------------*/
void yw_RemapBuildings(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      SR.BPRemap[] und SR.NumBuildings werden ausgefüllt.
**
**  CHANGED
**      20-Dec-95   floh    created
**      22-Dec-95   floh    siehe yw_RemapWeapons()
**      03-jan-96   floh    falls genau 1 Building vorhanden, wird dieses
**                          aktiv
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      12-Apr-97   floh    + BP_Array nicht mehr global
**      15-Oct-97   floh    + Sortierung
*/
{
    ULONG i;
    ULONG num_blds  = 0;
    UBYTE footprint = 0;

    if (ywd->URBact) footprint = (1 << (ywd->URBact->Owner));

    for (i=0; i<NUM_BUILDPROTOS; i++) {
        if (ywd->BP_Array[i].FootPrint & footprint) {
            SR.BPRemap[num_blds] = i;
            num_blds++;
        };
    };
    SR.NumBuildings = num_blds;

    /*** sortiere nach Gebäude-Typ und Kosten ***/
    __qsort_ywd = ywd;
    qsort(SR.BPRemap, num_blds, sizeof(SR.BPRemap[0]), yw_SRBlgCompareHook);

    /*** Sanity Check ***/
    if (num_blds == 0)      SR.ActBuilding = -1;
    else if (num_blds == 1) SR.ActBuilding = 0;
    else if (SR.ActBuilding >= num_blds) SR.ActBuilding = num_blds-1;
}

/*-----------------------------------------------------------------*/
void yw_RemapThings(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Kombination aus yw_RemapVehicles() und yw_RemapBuildings().
**
**      *** ACHTUNG ***
**      yw_RemapVehicles() und yw_Remapbuildings() muss vorher
**      ausgefuehrt werden!
**
**  CHANGED
**      04-Dec-97   floh    created
*/
{
    ULONG i;
    SR.NumThings = 0;

    /*** geremappte Vehikel uebernehmen ***/
    for (i=0; i<SR.NumVehicles; i++) {
        SR.ThingRemap[i].type  = YPATHING_VEHICLE;
        SR.ThingRemap[i].index = SR.VPRemap[i];
    };
    SR.NumThings += SR.NumVehicles;

    /*** falls Add Modus aktiv, KEINE GEBAEUDE! ***/
    if (!(SR.ActiveMode & STAT_MODEF_ADD)) {
        for (i=0; i<SR.NumBuildings; i++) {
            SR.ThingRemap[i+SR.NumVehicles].type  = YPATHING_BUILDING;
            SR.ThingRemap[i+SR.NumVehicles].index = SR.BPRemap[i];
        };
        SR.NumThings += SR.NumBuildings;
    };

    /*** Boundary Check ***/
    if (SR.NumThings == 0)                SR.ActThing = -1;
    else if (SR.NumThings == 1)           SR.ActThing = 0;
    else if (SR.ActThing >= SR.NumThings) SR.ActThing = SR.NumThings-1;    
}

#ifdef YPA_DESIGNMODE
/*-----------------------------------------------------------------*/
void yw_RemapSectors(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Für das Set-Sektor-Menü im Designer-Modus.
**
**  CHANGED
**      23-May-96   floh    created
*/
{
    ULONG i;
    ULONG num_secs = 0;

    for (i=0; i<256; i++) {
        /*** das Font-Char eignet sich zur Gültigkeits-Prüfung ***/
        if (ywd->Sectors[i].Chr != 0) {
            SR.SecRemap[num_secs++] = i;
        };
    };
    SR.NumSectors = num_secs;

    if (num_secs == 0)                 SR.ActSector = -1;
    else if (num_secs == 1)            SR.ActSector = 0;
    else if (SR.ActSector >= num_secs) SR.ActSector = num_secs-1;
}
#endif

/*-----------------------------------------------------------------*/
void yw_RemapCommanders(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      ywd->CmdrRemap[], ywd->SlaveRemap[], ywd->NumCmdrs[],
**      ywd->NumSlaves wird neu ermittelt, außerdem Sanity-Check
**      für ywd->ActCmdr, ywd->ActSlave
**
**  CHANGED
**      21-Dec-95   floh    created
**      22-Dec-95   floh    + jets jants jeu
**      26-Dec-95   floh    Die Commanders werden jetzt etwas
**                          sorgfältiger ausgewählt (anhand der aktuellen
**                          MainState und ExtraState).
**      03-Jan-96   floh    falls genau 1 Commander vorhanden, wird
**                          dieser aktiv (genauso bei seinen Slaves)
**      27-May-96   floh    Update von AF
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      19-Jan-97   floh    + ActCmdr richtet sich jetzt nach ActCmdID
**                            und wird bei jedem Aufruf neu validiert,
**                            das heißt, das selektierte Geschwader
**                            springt nicht mehr hin und her bei
**                            jedem Commander-Wechsel!
**      10-Apr-97   floh    + die Arrays werden jetzt umgekehrt gefüllt
**                          + falls ActCommander invalid, wird der letzte,
**                            und nicht mehr der 1. genommen!
**      19-Feb-98   floh    + falls ActCmdID und ActCmdr ungültig sind,
**                            wird nur noch im NEW oder ADD Mode automatisch
**                            ein neues (nämlich das erste) ausgewählt
**                          + falls aktuelles Squad nicht mehr existiert
**                            wird nicht automatisch ein neues ausgewählt
**      22-Mar-98   floh    + SlaveRemap Array wird nicht mehr ausgefüllt,
**                            nur noch NumSlaves. Stürzte bei mehr als
**                            256 Fahrzeugen im Geschwader ab.
**      20-May-98   floh    + fuellt jetzt auch den LastMessageSender aus
**      12-Jun-98   floh    + wird jetzt sortiert
**                          + ACTION_BEAM wird rausgefiltert
*/
{
    struct MinList *l;
    struct MinNode *nd;
    ULONG num_cmds = 0;

    /*** Commander-Remap-Tabelle ausfüllen ***/
    if (l = ywd->URSlaves) {
        for (nd=l->mlh_TailPred; nd->mln_Pred; nd=nd->mln_Pred) {
            struct Bacterium *b = ((struct OBNode *)nd)->bact;
            if ((b->MainState   != ACTION_DEAD)   &&
                (b->MainState   != ACTION_BEAM)   &&
                (b->BactClassID != BCLID_YPAGUN))
            {
                ywd->CmdrRemap[num_cmds++] = b;
            };
        };
    };
    ywd->NumCmdrs = num_cmds;
    
    /*** sortieren ***/    
    qsort(ywd->CmdrRemap, ywd->NumCmdrs, sizeof(ywd->CmdrRemap[0]), yw_SRCmdrCompareHook);

    /*** ywd->ActCmdID validieren ***/
    if (num_cmds==0) {
        /*** kein Geschwader vorhanden ***/
        ywd->ActCmdID = 0;
        ywd->ActCmdr  = -1;
    } else {
        if (ywd->ActCmdID==0) {
            /*** ActCmdID ungueltig, im New/Add-Mode neuestes Squad auswaehlen ***/
            if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD)) {
                ywd->ActCmdID = ywd->CmdrRemap[num_cmds-1]->CommandID;
                ywd->ActCmdr  = num_cmds-1;
            };
        } else {
            /*** sonst: ActCmdID ist selektiertes Geschwader ***/
            ULONG i;
            for (i=0; i<num_cmds; i++) {
                if (ywd->CmdrRemap[i]->CommandID == ywd->ActCmdID) {
                    ywd->ActCmdr = i;
                    break;
                };
            };
            /*** exististiert aktuelles Squad nicht mehr -> invalidieren? ***/
            if (i==num_cmds) {
                ywd->ActCmdID = 0;
                ywd->ActCmdr  = -1;
            };
        };
    };

    /*** das ganze nochmal für die Bacts ***/
    if (ywd->ActCmdr != -1) {

        ULONG num_slaves = 0;

        l = &(ywd->CmdrRemap[ywd->ActCmdr]->slave_list);
        for (nd=l->mlh_TailPred; nd->mln_Pred; nd=nd->mln_Pred) {
            num_slaves++;
        };
        ywd->NumSlaves = num_slaves;

    } else {
        /*** kein Commander -> keine Slaves! ***/
        ywd->NumSlaves = 0;
    };
    
    /*** LastMessageSender ausfuellen ***/
    ywd->LastMessageSender = yw_GetLastMessageSender(ywd);    

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MenuLayoutItem(struct ypaworld_data *ywd,
                         struct YPAListReq *m,
                         UBYTE *str,
                         UBYTE *item_str,
                         UBYTE type_icon)
/*
**  FUNCTION
**      Layoutet ein *nicht*selektiertes* Menu-Item.
**
**  INPUTS
**      ywd         -> LID des Welt-Objects
**      m           -> Ptr auf YPAListReq des Menüs
**      str         -> Pointer auf Output-Stream
**      item        -> Pointer auf Menu-Item-String
**      type_icon   -> optionales Type-Icon (aus finder.font)
**
**  RESULTS
**      str -> aktueller Pointer auf Output-Stream
**
**  CHANGED
**      19-Dec-95   floh    created
**      08-Apr-96   floh    + Support für optionales TypeIcon
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      24-Nov-97   floh    + unterstuetzt keine TypeIcons mehr, und benutzt
**                            yw_TextBuildClippedItem() (also die DBCS Version)
**      10-Dec-97   floh    + DBCS-Color
*/
{
    WORD w = m->ActEntryWidth;
    put(str,'{');   // linker Rand

    /*** das eigentliche Item ***/
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST),yw_Green(ywd,YPACOLOR_TEXT_LIST),yw_Blue(ywd,YPACOLOR_TEXT_LIST));
    str = yw_TextBuildClippedItem(ywd->Fonts[STAT_MENUUP_FONT],
                                  str, item_str, w-2*ywd->EdgeW, ' ');
    put(str,'}');   // rechter Rand
    new_line(str);

    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_MenuLayoutSelItem(struct ypaworld_data *ywd,
                            struct YPAListReq *m,
                            UBYTE *str,
                            UBYTE *item_str,
                            UBYTE type_icon)
/*
**  FUNCTION
**      Layoutet ein *selektiertes* Menu-Item.
**
**  INPUTS
**      ywd     -> LID des Welt-Objects
**      m       -> Ptr auf YPAListReq des Menüs
**      str     -> Pointer auf Output-Stream
**      item    -> Pointer auf Menu-Item-String
**      type_icon   -> optionales Type-Icon (aus finder.font)
**
**  RESULTS
**      str -> aktueller Pointer auf Output-Stream
**
**  CHANGED
**      19-Dec-95   floh    created
**      08-Apr-96   floh    + Support für optionales TypeIcon
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      24-Nov-97   floh    + unterstuetzt keine TypeIcons mehr, und benutzt
**                            yw_TextBuildClippedItem() (also die DBCS Version)
**      10-Dec-97   floh    + DBCS-Color
*/
{
    WORD w = m->ActEntryWidth;

    put(str,'{');   // rechter Rand, Menü

    new_font(str,STAT_MENUDOWN_FONT);
    put(str,'b');   // linker Rand, Downfont
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST_SEL),yw_Green(ywd,YPACOLOR_TEXT_LIST_SEL),yw_Blue(ywd,YPACOLOR_TEXT_LIST_SEL));
    str = yw_TextBuildClippedItem(ywd->Fonts[STAT_MENUDOWN_FONT],
                                  str, item_str, w-4*ywd->EdgeW, 'c');
    put(str,'d');   // rechter Rand, Downfont
    new_font(str,STAT_MENUUP_FONT);
    put(str,'}');   // rechter Rand, Menü
    new_line(str);

    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_SRLayoutVhclBlgItem(struct ypaworld_data *ywd,
                              ULONG sel,
                              struct YPAListReq *m,
                              UBYTE *str,
                              UBYTE icon,
                              UBYTE *name,
                              LONG cost)
/*
**  FUNCTION
**      Layoutet eine VehicleMenu-/BuildMenu-Zeile, inklusive TypeIcon
**      und Kosten-Punkte.
**
**  CHANGED
**      14-Oct-97   floh    created
**      20-Oct-97   floh    + neues Füllzeichen "f" für nichtselektierte
**                            Items.
**      29-Jan-98   floh    + arbeitet ab jetzt mit dem transparenten
**                            TypeIcons
*/
{
    WORD w = m->ActEntryWidth - 2*ywd->EdgeW;
    UBYTE icon_str[4];
    UBYTE cost_str[32];
    struct ypa_ColumnItem col[3];
    UBYTE fnt_id;
    UBYTE space_chr;
    UBYTE prefix_chr;
    UBYTE postfix_chr;

    memset(col,0,sizeof(col));
    if (sel) {
        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST_SEL),yw_Green(ywd,YPACOLOR_TEXT_LIST_SEL),yw_Blue(ywd,YPACOLOR_TEXT_LIST_SEL));
        fnt_id      = STAT_MENUDOWN_FONT;
        space_chr   = 'c';
        prefix_chr  = 'b';
        postfix_chr = 'd';
    } else {
        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST),yw_Green(ywd,YPACOLOR_TEXT_LIST),yw_Blue(ywd,YPACOLOR_TEXT_LIST));
        fnt_id = STAT_MENUUP_FONT;
        space_chr   = 'f';
        prefix_chr  = 'f';
        postfix_chr = 'f';
    };

    /*** Spalten initialisieren ***/
    icon_str[0] = icon;
    icon_str[1] = 0;
    sprintf(cost_str,"%d",cost);

    /*** TypeIcon mit transparentem Hintergrund ***/
    col[0].string      = icon_str;
    col[0].width       = FR.type_icon_width;
    col[0].font_id     = FONTID_TYPE_NS;
    col[0].space_chr   = '@';
    col[0].prefix_chr  = 0;
    col[0].postfix_chr = 0;
    col[0].flags       = YPACOLF_ALIGNLEFT;

    /*** der Kosten-String (ganz rechts!) ***/
    col[2].string      = cost_str;
    col[2].width       = 5*ywd->Fonts[fnt_id]->fchars['0'].width;
    col[2].font_id     = fnt_id;
    col[2].space_chr   = space_chr;
    col[2].prefix_chr  = 0;
    col[2].postfix_chr = postfix_chr;
    col[2].flags       = YPACOLF_TEXT | YPACOLF_ALIGNRIGHT | YPACOLF_DOPOSTFIX;

    /*** der Name ***/
    col[1].string      = name;
    col[1].width       = w - col[0].width - col[2].width;
    col[1].font_id     = fnt_id;
    col[1].space_chr   = space_chr;
    col[1].prefix_chr  = prefix_chr;
    col[1].postfix_chr = 0;
    col[1].flags       = YPACOLF_TEXT | YPACOLF_ALIGNLEFT | YPACOLF_DOPREFIX;

    new_font(str,STAT_MENUUP_FONT);
    put(str,'{');   // linker Menü-Rand
    new_font(str,fnt_id);
    stretch(str,FR.type_icon_width);        // Hintergrund für TypeIcon
    put(str,space_chr);                     // ditto
    xpos_rel(str,-(FR.type_icon_width));    // Cursor zurückschalten
    str = yw_BuildColumnItem(ywd,str,3,col);
    new_font(str,STAT_MENUUP_FONT);
    put(str,'}');   // rechter Menü-Rand
    new_line(str);
    return(str);
}

#ifdef YPA_DESIGNMODE
/*-----------------------------------------------------------------*/
void yw_SRLayoutSectorMenu(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet das SetSector-Menü, wenn im Design-Modus.
**
**  CHANGED
**      23-May-96   floh    created
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    ULONG i;
    BYTE *str;
    struct YPAListReq *m = &SubMenu;

    str = m->Itemblock;

    /*** Menüs immer vorn ***/
    _RemClickBox(&(m->Req.req_cbox));
    _AddClickBox(&(m->Req.req_cbox), IE_CBX_ADDHEAD);

    /*** Menu-Parameter modifizieren ***/
    m->NumEntries = SR.NumSectors;
    m->Selected   = SR.ActSector;
    if (SR.NumSectors > m->MaxShown) m->ShownEntries = m->MaxShown;
    else                             m->ShownEntries = SR.NumSectors;
    if ((m->FirstShown + m->ShownEntries) > m->NumEntries) {
        m->FirstShown = m->NumEntries - m->ShownEntries;
    };

    /*** Ausdehnung neu berechnen und positionieren ***/
    yw_ListSetRect(ywd, m, -2, -2);
    m->Req.req_cbox.rect.x = SR.req.req_cbox.rect.x + SR.ModeStart;
    m->Req.req_cbox.rect.y = SR.req.req_cbox.rect.y - m->Req.req_cbox.rect.h;

    /*** DAS LAYOUT ***/
    str = yw_LVItemsPreLayout(ywd, m, str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen ***/
    for (i=0; i<(m->ShownEntries); i++) {

        UBYTE item_str[64];
        struct SectorDesc *secd;
        UBYTE *stype;

        ULONG sec_num = SR.SecRemap[i+m->FirstShown];

        /*** ermittle richtigen Proto Pointer und Namen ***/
        secd = &(ywd->Sectors[sec_num]);

        switch(secd->SecType) {
            case SECTYPE_COMPACT:   stype="1X1"; break;
            default:                stype="3X3"; break;
        };
        sprintf(item_str, "S %d, T %s, G %d",
                sec_num, stype, secd->GroundType);

        /*** Sonderfall selektiertes Item ***/
        if ((i+m->FirstShown) == m->Selected) {
            str = yw_MenuLayoutSelItem(ywd, m, str, item_str, 0);
        } else {
            str = yw_MenuLayoutItem(ywd, m, str, item_str, 0);
        };
    };

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, m, str, STAT_MENUUP_FONT, "xyz");

    /*** EOS ***/
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_SRLayoutOwnerMenu(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet das SetOwner-Menü, wenn im Design-Modus.
**
**  CHANGED
**      23-May-96   floh    created
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    ULONG i;
    BYTE *str;
    struct YPAListReq *m = &SubMenu;

    str = m->Itemblock;

    /*** Menüs immer vorn ***/
    _RemClickBox(&(m->Req.req_cbox));
    _AddClickBox(&(m->Req.req_cbox), IE_CBX_ADDHEAD);

    /*** Menu-Parameter modifizieren ***/
    m->NumEntries = 8;
    m->Selected   = SR.ActOwner;
    if (8 > m->MaxShown) m->ShownEntries = m->MaxShown;
    else                 m->ShownEntries = 8;
    if ((m->FirstShown + m->ShownEntries) > m->NumEntries) {
        m->FirstShown = m->NumEntries - m->ShownEntries;
    };

    /*** Ausdehnung neu berechnen und positionieren ***/
    yw_ListSetRect(ywd, m, -2, -2);
    m->Req.req_cbox.rect.x = SR.req.req_cbox.rect.x + SR.ModeStart;
    m->Req.req_cbox.rect.y = SR.req.req_cbox.rect.y - m->Req.req_cbox.rect.h;

    /*** DAS LAYOUT ***/
    str = yw_LVItemsPreLayout(ywd, m, str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen ***/
    for (i=0; i<(m->ShownEntries); i++) {

        UBYTE item_str[64];
        sprintf(item_str, "OWNER %d", i);
        switch(i) {
            case 0: strcat(item_str," (NTRL)"); break;
            case 1: strcat(item_str," (USER)"); break;
        };

        /*** Sonderfall selektiertes Item ***/
        if ((i+m->FirstShown) == m->Selected) {
            str = yw_MenuLayoutSelItem(ywd, m, str, item_str, 0);
        } else {
            str = yw_MenuLayoutItem(ywd, m, str, item_str, 0);
        };
    };

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, m, str, STAT_MENUUP_FONT, "xyz");

    /*** EOS ***/
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_SRLayoutHeightMenu(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet Submenu als Height-Menu im Designer-Modus.
**
**  CHANGED
**      23-May-96   floh    created
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    revised & updated (Icon-Interface)
*/
{
    ULONG i;
    BYTE *str;
    struct YPAListReq *m = &SubMenu;

    str = m->Itemblock;

    /*** Menüs immer vorn ***/
    _RemClickBox(&(m->Req.req_cbox));
    _AddClickBox(&(m->Req.req_cbox), IE_CBX_ADDHEAD);

    /*** Menu-Parameter modifizieren ***/
    m->NumEntries = 2;
    m->Selected   = SR.ActHeight;
    if (2 > m->MaxShown) m->ShownEntries = m->MaxShown;
    else                 m->ShownEntries = 2;
    if ((m->FirstShown + m->ShownEntries) > m->NumEntries) {
        m->FirstShown = m->NumEntries - m->ShownEntries;
    };

    /*** Ausdehnung neu berechnen und positionieren ***/
    yw_ListSetRect(ywd, m, -2, -2);
    m->Req.req_cbox.rect.x = SR.req.req_cbox.rect.x + SR.ModeStart;
    m->Req.req_cbox.rect.y = SR.req.req_cbox.rect.y - m->Req.req_cbox.rect.h;

    /*** DAS LAYOUT ***/
    str = yw_LVItemsPreLayout(ywd, m, str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen ***/
    for (i=0; i<(m->ShownEntries); i++) {

        UBYTE *item;

        switch(i) {
            case 0:  item = "LOWER";  break;
            default: item = "HIGHER"; break;
        };

        /*** Sonderfall selektiertes Item ***/
        if ((i+m->FirstShown) == m->Selected) {
            str = yw_MenuLayoutSelItem(ywd, m, str, item, 0);
        } else {
            str = yw_MenuLayoutItem(ywd, m, str, item, 0);
        };
    };

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, m, str, STAT_MENUUP_FONT, "xyz");

    /*** EOS ***/
    eos(str);
}
#endif

/*-----------------------------------------------------------------*/
void yw_SRLayoutThingMenu(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet das kombinierte Vehicle-/Building-Menu.
**
**  CHANGED
**      04-Dec-97   floh    created
*/
{
    ULONG i;
    BYTE *str;
    struct YPAListReq *m = &SubMenu;

    str = m->Itemblock;

    /*** Menues immer vorn ***/
    _RemClickBox(&(m->Req.req_cbox));
    _AddClickBox(&(m->Req.req_cbox), IE_CBX_ADDHEAD);

    /*** Menu-Parameter modifizieren ***/
    m->NumEntries = SR.NumThings;
    m->Selected   = SR.ActThing;
    if (SR.NumThings > m->MaxShown) m->ShownEntries = m->MaxShown;
    else                            m->ShownEntries = SR.NumThings;
    if ((m->FirstShown + m->ShownEntries) > m->NumEntries) {
        m->FirstShown = m->NumEntries - m->ShownEntries;
    };

    /*** Ausdehnung neu berechnen und positionieren ***/
    yw_ListSetRect(ywd, m, -2, -2);
    m->Req.req_cbox.rect.x = SR.req.req_cbox.rect.x + SR.ModeStart;
    m->Req.req_cbox.rect.y = SR.req.req_cbox.rect.y + SR.LBarStart - m->Req.req_cbox.rect.h;

    /*** DAS LAYOUT ***/
    str = yw_LVItemsPreLayout(ywd, m, str, STAT_MENUUP_FONT, "uvw");

    /*** alle Items zeichnen ***/
    for (i=0; i<(m->ShownEntries); i++) {
        struct VehicleProto *vhcl;
        struct BuildProto *blg;
        ULONG sel = FALSE;
        UBYTE *name;
        ULONG type  = SR.ThingRemap[i+m->FirstShown].type;

        if (YPATHING_VEHICLE == type) {
        
            ULONG vnum  = SR.ThingRemap[i+m->FirstShown].index;
            struct VehicleProto *vhcl = &(ywd->VP_Array[vnum]);
            ULONG cost  = FLOAT_TO_INT((vhcl->Energy * CREATE_ENERGY_FACTOR * yw_GetCostFactor(ywd)) / 100);
            UBYTE *name = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+vnum,vhcl->Name);
            if ((i+m->FirstShown) == m->Selected) sel=TRUE;
            str = yw_SRLayoutVhclBlgItem(ywd,sel,m,str,vhcl->TypeIcon,name,cost);
        
        } else if (YPATHING_BUILDING == type) {
        
            ULONG bnum = SR.ThingRemap[i+m->FirstShown].index;
            struct BuildProto *blg = &(ywd->BP_Array[bnum]);
            ULONG cost  = FLOAT_TO_INT((blg->CEnergy * yw_GetCostFactor(ywd)) / 100);
            UBYTE *name;
            if (ywd->playing_network) name = ypa_GetStr(ywd->LocHandle,STR_NAME_NETWORK_BUILDINGS+bnum,blg->Name);
            else                      name = ypa_GetStr(ywd->LocHandle,STR_NAME_BUILDINGS+bnum,blg->Name);
            if ((i+m->FirstShown) == m->Selected) sel=TRUE;
            str = yw_SRLayoutVhclBlgItem(ywd,sel,m,str,blg->TypeIcon,name,cost);
        };
    };

    /*** zeichne unteren Rand ***/
    str = yw_LVItemsPostLayout(ywd, m, str, STAT_MENUUP_FONT, "xyz");

    /*** EOS ***/
    eos(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutModeIconGroup(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      24-Jul-97   floh    created
**      04-Dec-97   floh    + Add + Build Icons sind raus
**      05-Dec-97   floh    + Order Button raus
**      08-Dec-97   floh    + Beam Button raus
*/
{
    LONG act_x;
    ULONG btn;

    if (ywd->UVBact->MainState == ACTION_DEAD) {
        /*** Buttons disablen, Popupmenu schliessen ***/
        memset(&SR_New,0,sizeof(SR_New));
        new_font(str,FONTID_ICON_DB);
        put(str,'C');
        yw_CloseReq(ywd,&(SubMenu.Req));
    } else {

        /*** erstmal alle Buttons enablen ***/
        act_x = SR.ModeStart;
        for (btn = STATBTN_NEW; btn <= STATBTN_NEW; btn++) {
            struct ClickButton *cb = SR.req.req_cbox.buttons[btn];
            cb->rect.x = act_x;
            cb->rect.y = SR.LBarStart;
            cb->rect.w = ywd->IconBW;
            cb->rect.h = ywd->IconBH;
            act_x += ywd->IconBW;
        };

        /*** Create Button ***/
        if (SR.EnabledModes & (STAT_MODEF_NEW|STAT_MODEF_ADD|STAT_MODEF_BUILD)) {
            /*** Createbutton darstellen ***/
            if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD|STAT_MODEF_BUILD)) {
                if (!(SubMenu.Req.flags & REQF_Closed)) yw_SRLayoutThingMenu(ywd);
                new_font(str,FONTID_ICON_PB);
            } else {
                new_font(str,FONTID_ICON_NB);
            };
            switch(SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD|STAT_MODEF_BUILD)) {
                case STAT_MODEF_NEW:    put(str,'C'); break;    
                case STAT_MODEF_ADD:    put(str,'D'); break;
                case STAT_MODEF_BUILD:  put(str,'F'); break;
                default:                put(str,'C'); break;
            };
        } else {
            /*** kein Create Button ***/
            memset(&SR_New,0,sizeof(SR_New));
            new_font(str,FONTID_ICON_DB);
            put(str,'C');
        };
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutWindowIconGroup(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      03-Aug-96   floh    created
**      05-Aug-96   floh    + transparente Icons
**      01-Sep-96   floh    + Energie-Window-Button wird
**                            zum HUD-Button, wenn User in
**                            einem Vehikel sitzt.
**      29-Sep-96   floh    + explizites HUD-Icon ('Y')
**      03-Nov-96   floh    Sonderfall falls Viewer-Fahrzeug
**                          tot.
**      13-Jan-97   floh    + CutterMode: nur noch Map-Button
**                            gültig
**      29-Sep-97   floh    + LogWindow Icon raus
**      13-Oct-97   floh    + Energy Window Icon raus
**      29-Jan-98   floh    + wenn Tot, wird Disabled Leiste
**                            gezeichnet
*/
{
    LONG act_x;
    ULONG btn;

    if (ywd->UVBact->MainState == ACTION_DEAD) {

        /*** Buttons disablen, Requester schließen ***/
        new_font(str,FONTID_ICON_DB);
        put(str,'H');   // Map Disable Button
        put(str,'I');   // Finder Disable Button
        memset(&SR_Map,0,sizeof(SR_Map));
        memset(&SR_Finder,0,sizeof(SR_Finder));
        yw_CloseReq(ywd,&(MR.req));
        yw_CloseReq(ywd,&(FR.l.Req));
        yw_CloseReq(ywd,&(LW.l.Req));

    } else {

        /*** Buttons enablen ***/
        act_x = SR.WinStart;
        for (btn = STATBTN_MAP; btn <= STATBTN_FINDER; btn++) {
            struct ClickButton *cb = SR.req.req_cbox.buttons[btn];
            cb->rect.x = act_x;
            cb->rect.y = SR.LBarStart;
            cb->rect.w = ywd->IconBW;
            cb->rect.h = ywd->IconBH;
            act_x += ywd->IconBW;
        };

        /*** Layout Map Window Button ***/
        if (MR.req.flags & REQF_Closed) { new_font(str,FONTID_ICON_NB); }
        else                            { new_font(str,FONTID_ICON_PB); }
        put(str,'H');

        #ifndef YPA_CUTTERMODE
            /*** Layout Finder Window Button ***/
            if (FR.l.Req.flags & REQF_Closed) { new_font(str,FONTID_ICON_NB); }
            else                              { new_font(str,FONTID_ICON_PB); }
            put(str,'I');
        #else
            /*** im Cuttermode ist nur das MapWindow erlaubt ***/
            xpos_rel(str,3*ywd->IconBW);
            memset(&SR_Finder,0,sizeof(SR_Finder));
            yw_CloseReq(ywd,&(FR.l.Req));
            yw_CloseReq(ywd,&(LW.l.Req));
        #endif
    };

    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutNavIconGroup(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      03-Aug-96   floh    created
**      05-Aug-96   floh    erstmal richtig mit Funktionalität
**                          ausgefüllt.
**      28-Sep-96   floh    + Control-2-Last-Message-Button
**                          + Control-2-Last-Message wird nicht
**                            mehr angezeigt, wenn der User bereits
**                            in diesem Geschwader sitzt.
**      13-Jan-97   floh    + CutterMode: NavIcon-Group vollständig
**                            disabled
**      25-Jul-97   floh    + Control- und Fight-Buttons
**      29-Jan-98   floh    + andere Behandlung für Flaks und
**                            tote Vehikel
**                          + NextSquad kommt nicht mehr im Robo
**                            vorbei, deshalb wird der Button auch nicht
**                            aktiviert, wenn nur 1 Squad existiert (und
**                            der User im Squad sitzt)
**      08-Feb-98   floh    + Bugfix: toter Robo zeigte NextCom Button
**                            an.
*/
{
    ULONG shown = 0;

    if (ywd->URBact == ywd->UVBact){

        /*** User sitzt in Robo ***/
        if (ywd->NumCmdrs > 0) shown  = (1<<STATBTN_NEXTCOM);
        if (SR.NumWeapons > 0) shown |= (1<<STATBTN_NEXTUNIT);

    }else if (ywd->UVBact->MainState == ACTION_DEAD) {

        /*** tot -> Back to Robo und To Next Unit möglich, ***/
        /*** außer, wenn Robo tot ist, dann geht nix.      ***/
        shown = (1<<STATBTN_TOROBO)|(1<<STATBTN_NEXTCOM);

    }else if (ywd->UserSitsInRoboFlak){

        /*** User sitzt in einer Robo-Flak ***/
        shown = (1<<STATBTN_TOROBO) | (1<<STATBTN_NEXTCOM);
        if (SR.NumWeapons > 1) shown |= (1<<STATBTN_NEXTUNIT);

    }else if (ywd->UVBact->master == ywd->UVBact->robo){

        /*** User sitzt in Geschwader-Führer ***/
        shown = (1<<STATBTN_TOROBO);

        /*** gibt es noch andere Squads? ***/
        if (ywd->UVBact->BactClassID==BCLID_YPAGUN) {
            if (ywd->NumCmdrs > 0) shown |= (1<<STATBTN_NEXTCOM);
        } else {
            if (ywd->NumCmdrs > 1) shown |= (1<<STATBTN_NEXTCOM);
        };

        /*** hat der Squad Commander Untergebene? ***/
        if (!IsListEmpty((struct List *)&(ywd->UVBact->slave_list))){
            shown |= (1<<STATBTN_NEXTUNIT);
        };

    }else{

        /*** User sitzt in einem Untergebenen ***/
        shown = (1<<STATBTN_TOROBO)|(1<<STATBTN_NEXTUNIT)|(1<<STATBTN_TOCOM);

        /*** gibt es noch andere Squads? ***/
        if (ywd->UVBact->BactClassID==BCLID_YPAGUN) {
            if (ywd->NumCmdrs > 0) shown |= (1<<STATBTN_NEXTCOM);
        } else {
            if (ywd->NumCmdrs > 1) shown |= (1<<STATBTN_NEXTCOM);
        };
    };

    /*** wenn Robo tot, geht nix mehr! ***/
    if (ACTION_DEAD == ywd->URBact->MainState) shown = 0;

    #ifdef YPA_CUTTERMODE
        shown = 0;
    #endif

    /*** und das Layout ***/
    if (SR.EnabledModes & STAT_MODEF_CONTROL){
        SR_Control.rect.x = SR.NavStart;
        SR_Control.rect.y = SR.LBarStart;
        SR_Control.rect.w = ywd->IconBW;
        SR_Control.rect.h = ywd->IconBH;
        if (SR.ActiveMode & STAT_MODEF_CONTROL){
            new_font(str,FONTID_ICON_PB);
        }else{
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'E');
    }else{
        memset(&SR_Control,0,sizeof(SR_Control));
        new_font(str,FONTID_ICON_DB);
        put(str,'E');
    };
    if (shown & (1<<STATBTN_TOROBO)){
        SR_ToRobo.rect.x = SR.NavStart + ywd->IconBW;
        SR_ToRobo.rect.y = SR.LBarStart;
        SR_ToRobo.rect.w = ywd->IconBW;
        SR_ToRobo.rect.h = ywd->IconBH;
        if (SR.DownFlags & (1<<STATBTN_TOROBO)) { new_font(str,FONTID_ICON_PB); }
        else                                    { new_font(str,FONTID_ICON_NB); }
        put(str,'Q');
    }else{
        memset(&SR_ToRobo,0,sizeof(SR_ToRobo));
        new_font(str,FONTID_ICON_DB);
        put(str,'Q');
    };

    if (shown & (1<<STATBTN_TOCOM)){
        SR_ToCom.rect.x = SR.NavStart + 2*ywd->IconBW;
        SR_ToCom.rect.y = SR.LBarStart;
        SR_ToCom.rect.w = ywd->IconBW;
        SR_ToCom.rect.h = ywd->IconBH;
        if (SR.DownFlags & (1<<STATBTN_TOCOM)) { new_font(str,FONTID_ICON_PB); }
        else                                   { new_font(str,FONTID_ICON_NB); }
        put(str,'R');
    }else{
        memset(&SR_ToCom,0,sizeof(SR_ToCom));
        new_font(str,FONTID_ICON_DB);
        put(str,'R');
    };

    /*** NextUnit Button ***/
    if (shown & (1<<STATBTN_NEXTUNIT)){

        if ((SR.NumWeapons > 0) &&
            ((ywd->UVBact==ywd->URBact) || (ywd->UserSitsInRoboFlak)))
        {
            /*** Sonderfall, User sitzt in Robo, oder Roboflak ***/
            SR_NextUnit.rect.x = SR.NavStart + 3*ywd->IconBW;
            SR_NextUnit.rect.y = SR.LBarStart;
            SR_NextUnit.rect.w = ywd->IconBW;
            SR_NextUnit.rect.h = ywd->IconBH;
            if (SR.DownFlags & (1<<STATBTN_NEXTUNIT)) { new_font(str,FONTID_ICON_PB); }
            else                                      { new_font(str,FONTID_ICON_NB); }
            put(str,'B');
        }else{
            /*** NextUnit normal an ***/
            SR_NextUnit.rect.x = SR.NavStart + 3*ywd->IconBW;
            SR_NextUnit.rect.y = SR.LBarStart;
            SR_NextUnit.rect.w = ywd->IconBW;
            SR_NextUnit.rect.h = ywd->IconBH;
            if (SR.DownFlags & (1<<STATBTN_NEXTUNIT)) { new_font(str,FONTID_ICON_PB); }
            else                                      { new_font(str,FONTID_ICON_NB); }
            put(str,'S');
        };
    }else{
        /*** NextUnit disabled ***/
        memset(&SR_NextUnit,0,sizeof(SR_NextUnit));
        new_font(str,FONTID_ICON_DB);
        put(str,'S');
    };

    /*** Next Commander ***/
    if (shown & (1<<STATBTN_NEXTCOM)){
        SR_NextCom.rect.x = SR.NavStart + 4*ywd->IconBW;
        SR_NextCom.rect.y = SR.LBarStart;
        SR_NextCom.rect.w = ywd->IconBW;
        SR_NextCom.rect.h = ywd->IconBH;
        if (SR.DownFlags & (1<<STATBTN_NEXTCOM)) { new_font(str,FONTID_ICON_PB); }
        else                                     { new_font(str,FONTID_ICON_NB); }
        put(str,'T');
    }else{
        memset(&SR_NextCom,0,sizeof(SR_NextCom));
        new_font(str,FONTID_ICON_DB);
        put(str,'T');
    };

    /*** das war's ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutQuitIconGroup(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      25-Jul-97   floh    created
**      04-Aug-97   floh    + Help-Button auf Char '?' umgebogen
**      06-Apr-98   floh    + Online-Hilfe
*/
{
    if (ywd->UVBact->MainState != ACTION_DEAD) {
        SR_Help.rect.x = SR.QuitStart;
        SR_Help.rect.y = SR.LBarStart;
        SR_Help.rect.w = ywd->IconBW;
        SR_Help.rect.h = ywd->IconBH;
        if (SR.DownFlags & (1<<STATBTN_HELP)) { new_font(str,FONTID_ICON_PB); }
        else                                  { new_font(str,FONTID_ICON_NB); }
        put(str,'?');
    } else {
        memset(&SR_Help,0,sizeof(SR_Help));
        new_font(str,FONTID_ICON_DB);
        put(str,'?');
    };

    SR_OnlineHelp.rect.x = SR.QuitStart + ywd->IconBW;
    SR_OnlineHelp.rect.y = SR.LBarStart;
    SR_OnlineHelp.rect.w = ywd->IconBW;
    SR_OnlineHelp.rect.h = ywd->IconBH;
    if (SR.DownFlags & (1<<STATBTN_ONLINEHELP)) { new_font(str,FONTID_ICON_PB); }
    else                                        { new_font(str,FONTID_ICON_NB); }
    put(str,'L');   // FIXME!

    SR_Quit.rect.x = SR.QuitStart + 2*ywd->IconBW;
    SR_Quit.rect.y = SR.LBarStart;
    SR_Quit.rect.w = ywd->IconBW;
    SR_Quit.rect.h = ywd->IconBH;
    if (SR.DownFlags & (1<<STATBTN_QUIT)) { new_font(str,FONTID_ICON_PB); }
    else                                  { new_font(str,FONTID_ICON_NB); }
    put(str,'U');

    return(str);
}

#ifdef YPA_DESIGNMODE
/*-----------------------------------------------------------------*/
UBYTE *yw_PutDesignIconGroup(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      29-Jul-97   floh    created
*/
{
    ULONG act_x;
    ULONG btn;

    /*** erstmal alle Buttons enablen ***/
    act_x = SR.ModeStart;
    for (btn = STATBTN_SETSECTOR; btn <= STATBTN_SETHEIGHT; btn++) {
        struct ClickButton *cb = SR.req.req_cbox.buttons[btn];
        cb->rect.x = act_x;
        cb->rect.y = 0;
        cb->rect.w = ywd->IconBW;
        cb->rect.h = ywd->IconBH;
        act_x += ywd->IconBW;
    };

    /*** SetSector Button ***/
    if (SR.EnabledModes & STAT_MODEF_SETSECTOR) {
        if (SR.ActiveMode & STAT_MODEF_SETSECTOR) {
            new_font(str,FONTID_ICON_PB);
            if (!(SubMenu.Req.flags & REQF_Closed)) yw_SRLayoutSectorMenu(ywd);
        } else {
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'W');
    } else {
        memset(&SR_SetSector,0,sizeof(SR_SetSector));
        new_font(str,FONTID_ICON_DB);
        put(str,'W');
    };

    /*** SetOwner Button ***/
    if (SR.EnabledModes & STAT_MODEF_SETOWNER) {
        if (SR.ActiveMode & STAT_MODEF_SETOWNER) {
            new_font(str,FONTID_ICON_PB);
            if (!(SubMenu.Req.flags & REQF_Closed)) yw_SRLayoutOwnerMenu(ywd);
        } else {
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'W');
    } else {
        memset(&SR_SetOwner,0,sizeof(SR_SetOwner));
        new_font(str,FONTID_ICON_DB);
        put(str,'W');
    };

    /*** SetHeight Button ***/
    if (SR.EnabledModes & STAT_MODEF_SETHEIGHT) {
        if (SR.ActiveMode & STAT_MODEF_SETHEIGHT) {
            new_font(str,FONTID_ICON_PB);
            if (!(SubMenu.Req.flags & REQF_Closed)) yw_SRLayoutHeightMenu(ywd);
        } else {
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'W');
    } else {
        memset(&SR_SetHeight,0,sizeof(SR_SetHeight));
        new_font(str,FONTID_ICON_DB);
        put(str,'W');
    };
    return(str);
}
#endif

#ifdef YPA_CUTTERMODE
/*-----------------------------------------------------------------*/
UBYTE *yw_PutCutterIconGroup(struct ypaworld_data *ywd, UBYTE *str)
/*
**  CHANGED
**      20-Aug-97   floh    created
*/
{
    ULONG act_x;
    ULONG btn;

    /*** erstmal alle Buttons enablen ***/
    act_x = SR.ModeStart;
    for (btn = STATBTN_PLAYER; btn <= STATBTN_LINKSTATIV; btn++) {
        struct ClickButton *cb = SR.req.req_cbox.buttons[btn];
        cb->rect.x = act_x;
        cb->rect.y = 0;
        cb->rect.w = ywd->IconBW;
        cb->rect.h = ywd->IconBH;
        act_x += ywd->IconBW;
    };

    /*** Player Button ***/
    if (SR.EnabledModes & STAT_MODEF_PLAYER) {
        if (SR.ActiveMode & STAT_MODEF_PLAYER) {
            new_font(str,FONTID_ICON_PB);
        } else {
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'W');
    } else {
        memset(&SR_Player,0,sizeof(SR_Player));
        new_font(str,FONTID_ICON_DB);
        put(str,'W');
    };

    /*** FreeStativ Button ***/
    if (SR.EnabledModes & STAT_MODEF_FREESTATIV) {
        if (SR.ActiveMode & STAT_MODEF_FREESTATIV) {
            new_font(str,FONTID_ICON_PB);
        } else {
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'W');
    } else {
        memset(&SR_FreeStativ,0,sizeof(SR_FreeStativ));
        new_font(str,FONTID_ICON_DB);
        put(str,'W');
    };

    /*** LinkStativ Button ***/
    if (SR.EnabledModes & STAT_MODEF_LINKSTATIV) {
        if (SR.ActiveMode & STAT_MODEF_LINKSTATIV) {
            new_font(str,FONTID_ICON_PB);
        } else {
            new_font(str,FONTID_ICON_NB);
        };
        put(str,'W');
    } else {
        memset(&SR_LinkStativ,0,sizeof(SR_LinkStativ));
        new_font(str,FONTID_ICON_DB);
        put(str,'W');
    };
    return(str);
}
#endif

/*-----------------------------------------------------------------*/
void yw_LayoutSR(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet den Statusbar, inklusive den Button-Definitionen.
**
**  CHANGED
**      11-Dec-95   floh    created
**      22-Dec-95   floh    Alle "Remap-Routinen" werden jetzt generell
**                          mit jedem Durchlauf ausgeführt.
**      23-Dec-95   floh    + yw_LayoutAggr()
**      26-Dec-95   floh    + yw_LayoutStatusButtons()
**      30-Dec-95   floh    - yw_Remap#?(), wird jetzt direkt
**                            am Anfang von yw_sim.c/yw_BSM_TRIGGER() 
**                            ausgeführt
**      21-Mar-96   floh    - SubSubMenu
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    rewrite: neues Icon-Interface
**      05-Aug-96   floh    + zeichnet jetzt zuerst den
**                            Fade-Balken, dann die einzelnen
**                            Icon drüber!
**      24-Jul-97   floh    + kein Mode-Menu mehr
**      25-Jul-97   floh    + neue Quit-Icon-Gruppe
**      29-Jul-97   floh    + DesignMode reaktiviert
**      30-Jul-97   floh    + gefadeter Balken ist rausgeflogen
**      20-Aug-97   floh    + Cutter reaktiviert
**      11-Jun-98   floh    + yw_LayoutVsValues()
*/
{
    BYTE *str = SR_ReqString;

    /*** damit keine GUI-Inkonsistenzen auftreten können ... ***/
    yw_StatusEnable(ywd);

    /*** StatusBar enabled? ***/
    if (SR.EnabledReqs & STAT_REQF_STATUS) {

        WORD xpos = SR.req.req_cbox.rect.x - (ywd->DspXRes>>1);
        WORD ypos = SR.req.req_cbox.rect.y - (ywd->DspYRes>>1);

        /*** sicherstellen, daß StatusBar offen ist ***/
        yw_OpenStatusBar();

        /*** Statusbar ist immer vorn ***/
        _RemClickBox(&(SR.req.req_cbox));
        _AddClickBox(&(SR.req.req_cbox), IE_CBX_ADDHEAD);

        new_font(str,FONTID_ICON_NB);

        /*** Mode-Icon, Submode-Icon ***/
        pos_abs(str,xpos,ypos+SR.LBarStart);
        if (SR.ModeStart > 0) { xpos_rel(str,SR.ModeStart); };
        str = yw_PutModeIconGroup(ywd,str);

        if (SR.GroupSpace > 0) { xpos_rel(str,SR.GroupSpace); };
        str = yw_PutWindowIconGroup(ywd,str);

        if (SR.GroupSpace > 0) { xpos_rel(str,SR.GroupSpace); };
        str = yw_PutNavIconGroup(ywd,str);

        if (SR.GroupSpace > 0) { xpos_rel(str,SR.GroupSpace); };
        str = yw_PutQuitIconGroup(ywd,str);

        #ifdef YPA_DESIGNMODE
            /*** DesignMode-Buttons ***/
            pos_abs(str,xpos,ypos);
            xpos_rel(str,SR.ModeStart);
            str = yw_PutDesignIconGroup(ywd,str);
        #endif

        #ifdef YPA_CUTTERMODE
            /*** CutterMode-Buttons ***/
            pos_abs(str,xpos,ypos);
            xpos_rel(str,SR.ModeStart);
            str = yw_PutCutterIconGroup(ywd,str);
        #endif
        
        /*** falls NEW oder ADD Modus, Vs-Values visualisieren ***/        
        if (!(SubMenu.Req.flags & REQF_Closed)) str = yw_LayoutVsValues(ywd,str);        
    };

    /*** End Of String ***/
    eos(str);

    /*** ModeMenu-, SubMenu-Listview-Parts layouten ***/
    if (!(SubMenu.Req.flags & REQF_Closed))  yw_ListLayout(ywd, &SubMenu);
}

/*-----------------------------------------------------------------*/
void yw_SRMapV2RControl(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Schaltet Map in Radar- oder Map-Modus, kontrolliert Finder, 
**      je nachdem, ob sich der User gerade in ein Vehikel, oder
**      in einen Robo geschaltet hat.
**
**  CHANGED
**      14-Mar-96   floh    created
**      23-Mar-96   floh    erweitert um Finder
**      06-May-96   floh    erweitert um LogWindow
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      24-Aug-96   floh    WasOpen-Zeugs aufgeräumt und Energy-Window
**                          unterstützt.
**      14-Sep-96   floh    neues Radar-Handling
**      07-Apr-98   floh    + Robo/Vehicle-Switch-Fenster-Handling
*/
{
    /*** hat sich User gerade in den Robo geschaltet? ***/
    if (ywd->URBact == ywd->UVBact) {
        if (MR.flags & MAPF_RADAR) {
            /*** Radar ausschalten ***/
            MR.flags &= ~MAPF_RADAR;
        };
    } else {
        if (!(MR.flags & MAPF_RADAR)) {
            /*** Radar anschalten  ***/
            MR.flags |= MAPF_RADAR;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_SRGetWindowStatus(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Schreibt Window-Status des User-Vehicles mit,
**      so dieses nicht tot ist... 
**
**  CHANGED
**      21-May-98   floh    created
**      30-May-98   floh    Window-Status liegt jetzt in ywd->Prefs    
*/
{
    if (ACTION_DEAD != ywd->UVBact->MainState) {
        if (ywd->UVBact == ywd->URBact) {
            /*** User sitzt gerade im Robo ***/
            ywd->Prefs.RoboMapStatus.IsValid    = TRUE;
            ywd->Prefs.RoboMapStatus.IsOpen     = (MR.req.flags & REQF_Closed) ? FALSE:TRUE;
            ywd->Prefs.RoboMapStatus.Rect       = MR.req.req_cbox.rect;
            ywd->Prefs.RoboMapStatus.Data[0]    = MR.layers;
            ywd->Prefs.RoboMapStatus.Data[1]    = MR.lock_mode;
            ywd->Prefs.RoboMapStatus.Data[2]    = MR.zoom;
            ywd->Prefs.RoboMapStatus.Data[3]    = MR.zip_zap.x;
            ywd->Prefs.RoboMapStatus.Data[4]    = MR.zip_zap.y;
            ywd->Prefs.RoboMapStatus.Data[5]    = MR.zip_zap.w;
            ywd->Prefs.RoboMapStatus.Data[6]    = MR.zip_zap.h;
            ywd->Prefs.RoboFinderStatus.IsValid = TRUE;
            ywd->Prefs.RoboFinderStatus.IsOpen  = (FR.l.Req.flags & REQF_Closed) ? FALSE:TRUE;
            ywd->Prefs.RoboFinderStatus.Rect    = FR.l.Req.req_cbox.rect;
        } else {
            /*** User sitzt gerade in einem normalen Vehikel ***/
            ywd->Prefs.VhclMapStatus.IsValid    = TRUE;
            ywd->Prefs.VhclMapStatus.IsOpen     = (MR.req.flags & REQF_Closed) ? FALSE:TRUE;
            ywd->Prefs.VhclMapStatus.Rect       = MR.req.req_cbox.rect;
            ywd->Prefs.VhclMapStatus.Data[0]    = MR.layers;
            ywd->Prefs.VhclMapStatus.Data[1]    = MR.lock_mode;
            ywd->Prefs.VhclMapStatus.Data[2]    = MR.zoom;
            ywd->Prefs.VhclMapStatus.Data[3]    = MR.zip_zap.x;
            ywd->Prefs.VhclMapStatus.Data[4]    = MR.zip_zap.y;
            ywd->Prefs.VhclMapStatus.Data[5]    = MR.zip_zap.w;
            ywd->Prefs.VhclMapStatus.Data[6]    = MR.zip_zap.h;
            ywd->Prefs.VhclFinderStatus.IsValid = TRUE;
            ywd->Prefs.VhclFinderStatus.IsOpen  = (FR.l.Req.flags & REQF_Closed) ? FALSE:TRUE;
            ywd->Prefs.VhclFinderStatus.Rect    = FR.l.Req.req_cbox.rect;
        };
    };
}                

/*-----------------------------------------------------------------*/
void yw_SRHandleVehicleSwitch(struct ypaworld_data *ywd,
                              struct Bacterium *act_vhcl,
                              struct Bacterium *new_vhcl)
/*
**  FUNCTION
**      Handelt die Fenster beim Schalten zwischen 
**      Vehikeln ab. Darf nur aufgerufen werden, wenn
**      der User auch zwischen 2 Vehikeln gewechselt
**      hat.
**  
**  CHANGED
**      21-May-98   floh    created
**      30-May-98   floh    Window-Status jetzt in ywd->Prefs
*/
{
    /*** Switch von einem Vehikel in den Robo? ***/
    if (BCLID_YPAROBO == new_vhcl->BactClassID) {
        /*** Map: Wenn RoboStatus gueltig, diesen aktivieren ***/
        if (ywd->Prefs.RoboMapStatus.IsValid) {
            if (ywd->Prefs.RoboMapStatus.IsOpen) {
                yw_OpenReq(ywd,&(MR.req));
                yw_ReqToFront(ywd,&(MR.req));
            } else yw_CloseReq(ywd,&(MR.req));
            MR.req.req_cbox.rect = ywd->Prefs.RoboMapStatus.Rect;
            MR.layers    = ywd->Prefs.RoboMapStatus.Data[0];
            MR.lock_mode = ywd->Prefs.RoboMapStatus.Data[1];
            MR.zoom      = ywd->Prefs.RoboMapStatus.Data[2];
            MR.zip_zap.x = ywd->Prefs.RoboMapStatus.Data[3];
            MR.zip_zap.y = ywd->Prefs.RoboMapStatus.Data[4];
            MR.zip_zap.w = ywd->Prefs.RoboMapStatus.Data[5];
            MR.zip_zap.h = ywd->Prefs.RoboMapStatus.Data[6];
            yw_MapZoom(ywd,MAP_ZOOM_CORRECT);
        };

        /*** dasselbe fuer den Finder ***/
        if (ywd->Prefs.RoboFinderStatus.IsValid) {
            if (ywd->Prefs.RoboFinderStatus.IsOpen) {
                yw_OpenReq(ywd,&(FR.l.Req));
                yw_ReqToFront(ywd,&(FR.l.Req));
            } else yw_CloseReq(ywd,&(FR.l.Req));
            FR.l.Req.req_cbox.rect = ywd->Prefs.RoboFinderStatus.Rect;
            yw_ListSetRect(ywd,&(FR.l),-2,-2);
        };
    } else if (BCLID_YPAROBO == act_vhcl->BactClassID) {
        /*** Map: Wenn Vhcltatus gültig, diesen aktivieren ***/
        if (ywd->Prefs.VhclMapStatus.IsValid) {
            if (ywd->Prefs.VhclMapStatus.IsOpen) {
                yw_OpenReq(ywd,&(MR.req));
                yw_ReqToFront(ywd,&(MR.req));
            } else yw_CloseReq(ywd,&(MR.req));
            MR.req.req_cbox.rect = ywd->Prefs.VhclMapStatus.Rect;
            MR.layers    = ywd->Prefs.VhclMapStatus.Data[0];
            MR.lock_mode = ywd->Prefs.VhclMapStatus.Data[1];
            MR.zoom      = ywd->Prefs.VhclMapStatus.Data[2];
            MR.zip_zap.x = ywd->Prefs.VhclMapStatus.Data[3];
            MR.zip_zap.y = ywd->Prefs.VhclMapStatus.Data[4];
            MR.zip_zap.w = ywd->Prefs.VhclMapStatus.Data[5];
            MR.zip_zap.h = ywd->Prefs.VhclMapStatus.Data[6];
            yw_MapZoom(ywd,MAP_ZOOM_CORRECT);
        };

        /*** dasselbe für den Finder ***/
        if (ywd->Prefs.VhclFinderStatus.IsValid) {
            if (ywd->Prefs.VhclFinderStatus.IsOpen) {
                yw_OpenReq(ywd,&(FR.l.Req));
                yw_ReqToFront(ywd,&(FR.l.Req));
            } else yw_CloseReq(ywd,&(FR.l.Req));
            FR.l.Req.req_cbox.rect = ywd->Prefs.VhclFinderStatus.Rect;
            yw_ListSetRect(ywd,&(FR.l),-2,-2);
        };
    } else if (ACTION_DEAD == act_vhcl->BactClassID) {
        /*** von einem toten Vehikel in ein lebendes Vehikel? ***/
        if (ywd->Prefs.VhclMapStatus.IsValid && ywd->Prefs.VhclMapStatus.IsOpen) {
            yw_OpenReq(ywd,&(MR.req));
            yw_ReqToFront(ywd,&(MR.req));
        };
        if (ywd->Prefs.VhclFinderStatus.IsValid && ywd->Prefs.VhclFinderStatus.IsOpen) {
            yw_OpenReq(ywd,&(FR.l.Req));
            yw_ReqToFront(ywd,&(FR.l.Req));
        };
    };        
}         

/*-----------------------------------------------------------------*/
BOOL yw_SRHotKey(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Statusbar-Hotkey-Handler.
**
**  RESULT
**      TRUE    -> SubMenu-Inputhandling abhandeln
**      FALSE   -> SubMenu-Inputhandling ignorieren
**
**      (wegen HOTKEY_PREVITEM, HOTKEY_NEXTITEM)
**
**  CHANGED
**      03-Aug-96   floh    created
**      05-Aug-96   floh    + Quit-Hotkey-Handling
**      17-Aug-96   floh    + Bugfix: <break> vergessen
**                            in HOTKEY_PREVITEM Block
**      21-Aug-96   floh    + HOTKEY_ENERGY
**      28-Sep-96   floh    + HOTKEY_CTRL2LM
**      29-Sep-96   floh    + HOTKEY_HUD
**      03-Nov-96   floh    + diverse Hotkeys werden nicht
**                            genommen, wenn User-Fahrzeug tot
**      13-Jan-97   floh    + im Cutter werden (der Einfachheit
**                            halber) die Statusbar-Hotkeys nicht
**                            akzeptiert.
**      19-Aug-97   floh    + Aggressions-Hotkeys
**      13-Oct-97   floh    + LogWin- und Energy-Hotkeys hartgecodet
**      04-Dec-97   floh    + Add/New/Build unified
**      05-Dec-97   floh    + der Quit-Hotkey schaltet jetzt zuerst in den
**                            Order-Modus
**      08-Dec-97   floh    + Beam Hotkey neu implementiert
**      16-Mar-98   floh    + HOTKEY_HELP Support
**      26-May-98   floh    + HOTKEY_ANALYZER Support
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    BOOL do_sub = TRUE;

    #ifndef YPA_CUTTERMODE
        switch (ip->HotKey) {
            case HOTKEY_ORDER:
                if (SR.EnabledModes & STAT_MODEF_ORDER) {
                    SR.ActiveMode = STAT_MODEF_ORDER;
                };
                break;

            case HOTKEY_AUTOPILOT:
                if (SR.EnabledModes & STAT_MODEF_AUTOPILOT) {
                    if (SR.ActiveMode & STAT_MODEF_AUTOPILOT) {
                        /*** deaktivieren ***/
                        SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
                    } else {
                        /*** aktivieren ***/
                        SR.ActiveMode = STAT_MODEF_AUTOPILOT;
                    };
                };
                break;
                    
            case HOTKEY_MAP:
                if (ywd->UVBact->MainState != ACTION_DEAD) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->btn    = STATBTN_MAP;
                    ci->flags |= CIF_BUTTONDOWN;
                };
                break;

            case HOTKEY_FINDER:
                if (ywd->UVBact->MainState != ACTION_DEAD) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->btn    = STATBTN_FINDER;
                    ci->flags |= CIF_BUTTONDOWN;
                };
                break;

            case HOTKEY_LOGWIN:
                {
                    struct YPAReq *r = &(LW.l.Req);
                    if (r->flags & REQF_Closed) {
                        yw_OpenReq(ywd,r);
                        _Remove((struct Node *)r);
                        _AddHead((struct List *) &(ywd->ReqList),
                                 (struct Node *)r);
                    } else {
                        yw_CloseReq(ywd,r);
                    };
                };
                break;

            case HOTKEY_HUD:
                if (ywd->UserRobo != ywd->UserVehicle) {
                    /*** User in Vehikel -> HUD an/aus ***/
                    if (ywd->Hud.RenderHUD) ywd->Hud.RenderHUD=FALSE;
                    else                    ywd->Hud.RenderHUD=TRUE;
                };
                break;

            case HOTKEY_NEXTITEM:
                do_sub = FALSE;
                switch(SR.ActiveMode) {
                    case STAT_MODEF_NEW:
                    case STAT_MODEF_ADD:
                    case STAT_MODEF_BUILD:
                        if (++SR.ActThing>=SR.NumThings) SR.ActThing=0;
                        yw_SRSyncActThingVhclBlg(ywd);
                        break;

                    #ifdef YPA_DESIGNMODE
                    case STAT_MODEF_SETSECTOR:
                        if (++SR.ActSector>=SR.NumSectors) SR.ActSector=0;
                        break;

                    case STAT_MODEF_SETHEIGHT:
                        if (++SR.ActHeight>=2) SR.ActHeight=0;
                        break;

                    case STAT_MODEF_SETOWNER:
                        if (++SR.ActOwner>=7) SR.ActOwner=0;
                        break;
                    #endif
                };
                break;

            case HOTKEY_PREVITEM:
                do_sub = FALSE;
                switch(SR.ActiveMode) {
                    case STAT_MODEF_NEW:
                    case STAT_MODEF_ADD:
                    case STAT_MODEF_BUILD:
                        if (--SR.ActThing<0) SR.ActThing=SR.NumThings-1;
                        yw_SRSyncActThingVhclBlg(ywd);
                        break;

                    #ifdef YPA_DESIGNMODE
                    case STAT_MODEF_SETSECTOR:
                        if (--SR.ActSector<0) SR.ActSector=SR.NumSectors-1;
                        break;

                    case STAT_MODEF_SETHEIGHT:
                        if (--SR.ActHeight<0) SR.ActHeight=1;
                        break;

                    case STAT_MODEF_SETOWNER:
                        if (--SR.ActOwner<0) SR.ActOwner=7;
                        break;
                    #endif
                };
                break;

            case HOTKEY_QUIT:
                if ((ywd->URBact->MainState != ACTION_DEAD) &&
                    (!(SR.ActiveMode & STAT_MODEF_ORDER)))
                {
                    /*** falls User-Robo nicht(!) tot und kein Order-Modus ***/
                    SR.ActiveMode = STAT_MODEF_ORDER;
                }else if ((ywd->UserVehicle != ywd->UserRobo) &&
                        (ACTION_DEAD != ywd->URBact->MainState))
                {
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONUP;
                    ci->btn    = STATBTN_TOROBO;
                    ip->HotKey = 0; // entprellen(?)
                }else if (SR_Quit.rect.w != 0){
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONUP;
                    ci->btn    = STATBTN_QUIT;
                };
                break;

            case HOTKEY_NEXTCMDR:
                if (SR_NextCom.rect.w != 0) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONUP;
                    ci->btn    = STATBTN_NEXTCOM;
                };
                break;

            case HOTKEY_TOROBO:
                if (SR_ToRobo.rect.w != 0) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONUP;
                    ci->btn    = STATBTN_TOROBO;
                };
                break;

            case HOTKEY_NEXTUNIT:
                if (SR_NextUnit.rect.w != 0) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONUP;
                    ci->btn    = STATBTN_NEXTUNIT;
                };
                break;

            case HOTKEY_FIGHT:
                /*** wenn in Robo, selbe Funktion wie HOTKEY_NEXTUNIT ***/
                if (ywd->URBact == ywd->UVBact) {
                    if (SR_NextUnit.rect.w != 0) {
                        ci->box    = &(SR.req.req_cbox);
                        ci->flags |= CIF_BUTTONUP;
                        ci->btn    = STATBTN_NEXTUNIT;
                    };
                };
                break;

            case HOTKEY_TOCMDR:
                if (SR_ToCom.rect.w != 0) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONUP;
                    ci->btn    = STATBTN_TOCOM;
                };
                break;

            case HOTKEY_NEW:
                if (SR.EnabledModes & STAT_MODEF_NEW) {
                     if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_BUILD|STAT_MODEF_ADD)) {
                         /*** schon an, wieder ausschalten ***/
                         SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
                     } else {
                         /*** NEW oder BUILD Modus aktivieren ***/
                         yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_NEW);
                         if (SR.ActThing < SR.NumVehicles)  SR.ActiveMode = STAT_MODEF_NEW;
                         else                               SR.ActiveMode = STAT_MODEF_BUILD;
                         do_sub=FALSE;
                     };
                };
                break;

            case HOTKEY_ADD:
                if (SR.EnabledModes & STAT_MODEF_ADD) {
                    if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD|STAT_MODEF_BUILD)) {
                        /*** schon an, wieder ausschalten ***/
                        SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
                    } else {
                        /*** ADD Modus aktivieren ***/
                        yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_ADD);
                        SR.ActiveMode = STAT_MODEF_ADD;
                        do_sub = FALSE;
                    };
                };
                break;

            case HOTKEY_CONTROL:
                if (SR.EnabledModes & STAT_MODEF_CONTROL) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->flags |= CIF_BUTTONDOWN;
                    ci->btn    = STATBTN_CONTROL;
                };
                break;

            case HOTKEY_AGGR1:
            case HOTKEY_AGGR2:
            case HOTKEY_AGGR3:
            case HOTKEY_AGGR4:
            case HOTKEY_AGGR5:
                if (ywd->ActCmdr != -1) {
                    struct Bacterium *cmdr = ywd->CmdrRemap[ywd->ActCmdr];
                    ULONG aggr = 0;
                    switch (ip->HotKey) {
                        case HOTKEY_AGGR1: aggr=0; break;
                        case HOTKEY_AGGR2: aggr=25; break;
                        case HOTKEY_AGGR3: aggr=50; break;
                        case HOTKEY_AGGR4: aggr=75; break;
                        case HOTKEY_AGGR5: aggr=100; break;
                    };
                    _set(cmdr->BactObject,YBA_Aggression,aggr);
                    if (ywd->gsr) {
                        _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
                    };
                };
                break;

            case HOTKEY_HELP:
                ywd->Url = ypa_GetStr(ywd->LocHandle,STR_HELP_INGAMEGENERAL,"help\\l17.html");
                break;

            case HOTKEY_ANALYZER:
                if (ywd->UVBact->MainState != ACTION_DEAD) {
                    ci->box    = &(SR.req.req_cbox);
                    ci->btn    = STATBTN_HELP;
                    ci->flags |= CIF_BUTTONUP;
                };
                break;
        };
    #endif // CutterMode

    /*** Ende ***/
    return(do_sub);
}

/*-----------------------------------------------------------------*/
Object *yw_SRNextUnit(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Next-Unit-Button-Funktion.
**
**  CHANGED
**      05-Aug-96   floh    created
**      02-Sep-96   floh    ignoriert jetzt Fahrzeuge im
**                          CREATE-Zustand
**      14-Oct-96   floh    ignoriert alle Fahrzeuge im ACTION_BEAM
**                          Zustand
**      29-Jan-98   floh    Flaks werden nicht mehr ignoriert
*/
{
    struct MinNode *nd;
    Object *new_viewer = NULL;

    if (ywd->UVBact->master == ywd->UVBact->robo) {
        /*** User sitzt in einem Commander ***/
        nd = (struct MinNode *) &(ywd->UVBact->slave_list);
    } else {
        /*** User sitzt in einem Slave ***/
        nd = (struct MinNode *) &(ywd->UVBact->slave_node);
    };

    /*** das nächste gültige Element in dieser Liste ***/
    while ((nd = nd->mln_Succ)->mln_Succ) {
        struct Bacterium *b = ((struct OBNode *)nd)->bact;
        if ((ACTION_CREATE != b->MainState) &&
            (ACTION_DEAD   != b->MainState) &&
            (ACTION_BEAM   != b->MainState))
        {
            new_viewer = b->BactObject;
            break;
        };
    };

    /*** in Liste war nix mehr drin ***/
    if (!new_viewer) {
        /*** wenn nicht bereits drin, in Commander ***/
        /*** reinschalten                          ***/
        if (ywd->UVBact->master != ywd->UVBact->robo) {
            new_viewer = ywd->UVBact->master;
            /*** Sonder-Values ausklammern ***/
            if (new_viewer < ((Object *)0x10)) new_viewer = NULL;
        };
    };

    /*** Ende ***/
    return(new_viewer);
}

/*-----------------------------------------------------------------*/
Object *yw_GetDefaultCmdr(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Falls ActCmdr existiert, wird dieser zurueckgegeben,
**      sonst falls Geschwader existieren, der 1.Commander,
**      sonst der User-Robo.
**
**  CHANGED
**      12-Jun-98   floh    created
*/
{
    if (ywd->ActCmdr != -1)     return(ywd->CmdrRemap[ywd->ActCmdr]->BactObject);
    else if (ywd->NumCmdrs > 0) return(ywd->CmdrRemap[0]->BactObject);
    else                        return(ywd->UserRobo);
}

/*-----------------------------------------------------------------*/
Object *yw_GetCmdrByCmdID(struct ypaworld_data *ywd, ULONG cmd_id)
/*
**  FUNCTION
**      Returniert den Commander zur CommandID, oder den
**      Default-Cmdr, wenn das Squad nicht mehr ex.
**
**  CHANGED
**      12-Jun-98   floh    created
*/
{
    ULONG i;
    Object *cmdr = NULL;
    for (i=0; i<ywd->NumCmdrs; i++) {
        if (ywd->CmdrRemap[i]->CommandID == cmd_id) {
            /*** Treffer... ***/
            return(ywd->CmdrRemap[i]->BactObject);
        };
    };
    /*** ooops, Geschwader gibts nicht (mehr), ausgewaehltes nehmen ***/
    return(yw_GetDefaultCmdr(ywd));
}

/*-----------------------------------------------------------------*/
Object *yw_GetNextCmdrByCmdID(struct ypaworld_data *ywd, ULONG cmd_id)
/*
**  FUNCTION
**      Returniert den folgenden Commander zur CommandID, oder den
**      Default-Cmdr, wenn das Squad nicht mehr ex.
**
**  CHANGED
**      12-Jun-98   floh    created
*/
{
    ULONG i;
    Object *cmdr = NULL;
    for (i=0; i<ywd->NumCmdrs; i++) {
        if (ywd->CmdrRemap[i]->CommandID == cmd_id) {
            /*** Treffer... den naechsten nehmen ***/
            if ((i+1) < ywd->NumCmdrs) return(ywd->CmdrRemap[i+1]->BactObject);
            else                       return(ywd->CmdrRemap[0]->BactObject);
        };
    };
    /*** ooops, Geschwader gibts nicht (mehr), ausgewaehltes nehmen ***/
    return(yw_GetDefaultCmdr(ywd));
}

/*-----------------------------------------------------------------*/
Object *yw_SRNextCom(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      NextCommander-Button-Funktion.
**
**  CHANGED
**      05-Aug-96   floh    created
**      02-Sep-96   floh    ignoriert jetzt Fahrzeuge im
**                          CREATE-Zustand
**      14-Oct-96   floh    + ignoriert alle Fahrzeuge im
**                            ACTION_BEAM-Zustand
**      29-Jan-98   floh    + beachtet jetzt auch den Sonderfall
**                            Unit ist tot...
**      12-Jun-98   floh    + arbeitet jetzt ausschliesslich mit dem
**                            CmdrRemap-Array
*/
{
    struct MinNode *nd;
    Object *new_viewer = NULL;

    if ((ywd->UserRobo == ywd->UserVehicle) || (ywd->UserSitsInRoboFlak)) {
        /*** Sonderfall User sitzt im Robo, oder in Roboflak ***/
        new_viewer = yw_GetDefaultCmdr(ywd);
    } else {
        /*** User sitzt in einem Vehikel ***/
        if (ACTION_DEAD == ywd->UVBact->MainState) {
            /*** Sonderfall totes Vehikel  ***/
            new_viewer = yw_GetCmdrByCmdID(ywd,ywd->UserVehicleCmdId);
        } else {
            /*** Vehikel lebt noch ***/
            if (ywd->UVBact->BactClassID == BCLID_YPAGUN){
                new_viewer = yw_GetDefaultCmdr(ywd);
            } else { 
                /*** ein "normaler" Commander oder Vehikel ***/
                new_viewer = yw_GetNextCmdrByCmdID(ywd,ywd->UserVehicleCmdId);
            };
        };
    };

    /*** Ende ***/
    return(new_viewer);
}

/*-----------------------------------------------------------------*/
Object *yw_SRNextGun(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Schaltet reihum durch die Robo-Guns und zurück
**      durch den Robo.
**
**  CHANGED
**      25-Jul-97   floh    created
*/
{
    Object *new_viewer = NULL;
    if (SR.ActWeapon != -1) {
        if (ywd->UserSitsInRoboFlak) {
            /*** sitzt schon in einer Robo-Flak, also zur nächsten schalten ***/
            SR.ActWeapon++;
            if (SR.ActWeapon >= SR.NumWeapons) {
                /*** Ende erreicht, zurück in Robo ***/
                new_viewer   = ywd->UserRobo;
                SR.ActWeapon = 0;
                ywd->UserSitsInRoboFlak = FALSE;
            } else {
                /*** Ende noch nicht erreicht, nächstes Flak-Object ***/
                struct gun_data *gd;
                _get(ywd->UserRobo, YRA_GunArray, &gd);
                new_viewer = gd[SR.WPRemap[SR.ActWeapon]].go;
                ywd->UserSitsInRoboFlak = TRUE;
            };
        } else {
            /*** in die 1.Roboflak reinschalten ***/
            struct gun_data *gd;
            SR.ActWeapon = 0;
            _get(ywd->UserRobo, YRA_GunArray, &gd);
            new_viewer = gd[SR.WPRemap[SR.ActWeapon]].go;
            ywd->UserSitsInRoboFlak = TRUE;
        };
    };
    return(new_viewer);
}

/*-----------------------------------------------------------------*/
Object *yw_SRCtrl2LM(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Control-2-LastMessage-Funktionalität.
**
**  CHANGED
**      28-Sep-96   floh    created
**      20-May-98   floh    umgeschrieben
*/
{
    struct Bacterium *lm_bact;

    /*** existiert das Meldungs-Geschwader noch??? ***/
    if (lm_bact = yw_GetLastMessageSender(ywd)) return(lm_bact->BactObject);
    else                                        return(NULL);
}

/*-----------------------------------------------------------------*/
Object *yw_SRLastOccupied(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert Object-Pointer des zuletzt besetzten
**      Vehikels.
**
**  CHANGED
**      24-Apr-98   floh    created
*/
{
    if (ywd->LastOccupiedID != 0) {
        ULONG i;
        struct MinList *r_ls;
        struct MinNode *r_nd;
        
        /*** ist es der User-Robo? ***/
        if (ywd->URBact->ident == ywd->LastOccupiedID) {
            /*** Treffer ***/
            return(ywd->URBact->BactObject);
        };
        
        /*** Untergebene des Robos durchgehen ***/
        r_ls = &(ywd->URBact->slave_list);
        for (r_nd=r_ls->mlh_Head; r_nd->mln_Succ; r_nd=r_nd->mln_Succ) {
            struct Bacterium *cmdr = ((struct OBNode *)r_nd)->bact;
            if ((cmdr->MainState != ACTION_DEAD)   &&
                (cmdr->MainState != ACTION_CREATE) &&
                (cmdr->MainState != ACTION_BEAM))
            {
                if (cmdr->ident == ywd->LastOccupiedID) {
                    /*** Treffer ***/
                    return(cmdr->BactObject);
                } else {
                    /*** die Slave-Liste des Commanders durchgehen ***/
                    struct MinList *s_ls;
                    struct MinNode *s_nd;
                    s_ls = &(cmdr->slave_list);
                    for (s_nd=s_ls->mlh_Head; s_nd->mln_Succ; s_nd=s_nd->mln_Succ) {
                        struct Bacterium *slave = ((struct OBNode *)s_nd)->bact;
                        if ((slave->MainState != ACTION_DEAD)   &&
                            (slave->MainState != ACTION_CREATE) &&
                            (slave->MainState != ACTION_BEAM)   &&
                            (slave->ident     == ywd->LastOccupiedID))
                        {
                            /*** Treffer ***/
                            return(slave->BactObject);
                        };
                    };
                };
            };
        };
    };
                    
    /*** oops, kein Treffer, Vehikel existiert wohl nicht mehr ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
void yw_SRMakeCommander(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Macht das aktuelle Viewer-Vehicle zum Commander
**      seines Geschwaders.
**
**  CHANGED
**      25-Apr-98   floh    created
*/
{
    /*** zurueck, wenn schon ein Commander ***/
    if ((ywd->UVBact->master_bact) &&
        (ywd->UVBact->master_bact == ywd->URBact))
    {
        return;
    };

    /*** ein gueltiger Vehicle-Typ? ***/
    if ((ywd->UVBact->BactClassID != BCLID_YPAROBO)  &&
        (ywd->UVBact->BactClassID != BCLID_YPAMISSY) &&
        (ywd->UVBact->BactClassID != BCLID_YPAGUN))
    {
        struct organize_msg om;
        om.mode        = ORG_BECOMECHIEF;
        om.specialbact = ywd->UVBact->master_bact;
        _methoda(ywd->UserVehicle,YBM_ORGANIZE,&om);
    };
}

/*-----------------------------------------------------------------*/
void yw_ChangeViewer(struct ypaworld_data *ywd,
                     Object *new_viewer,
                     Object *act_viewer)
/*
**  FUNCTION
**      Schaltet den User in ein neues Vehikel, mit allem,
**      was so dazugehoert...
**
**  CHANGED
**      22-May-98   floh    created
*/
{
    if (new_viewer != act_viewer) {        

        /*** ist neuer Viewer im Create- oder Beam-Zustand? ***/
        struct Bacterium *new_vb;
        struct Bacterium *act_vb;
        _get(new_viewer,YBA_Bacterium, &new_vb);
        _get(act_viewer,YBA_Bacterium, &act_vb);
        if ((new_vb->MainState != ACTION_CREATE) &&
            (new_vb->MainState != ACTION_DEAD)   &&
            (new_vb->MainState != ACTION_BEAM))
        {
            /*** Viewer switchen ***/
            ywd->Hud.change_vhcl_timer = ywd->TimeStamp;
            _set(act_viewer, YBA_Viewer, FALSE);
            _set(act_viewer, YBA_UserInput, FALSE);
            _set(new_viewer, YBA_Viewer, TRUE);
            _set(new_viewer, YBA_UserInput, TRUE);

            /*** falls Submenu auf -> schließen ***/
            if (!(SubMenu.Req.flags & REQF_Closed)) yw_CloseReq(ywd,&(SubMenu.Req));

            /*** falls gerade im Control-Modus -> ***/
            /*** Order-Modus einschalten          ***/
            if ((SR.EnabledModes & STAT_MODEF_ORDER) && (SR.ActiveMode == STAT_MODEF_CONTROL))
            {
                SR.ActiveMode = STAT_MODEF_ORDER;
            };

            if (new_viewer == ywd->UserRobo) {
                /*** in User-Robo: Mauscontrol deaktivieren... ***/
                ywd->ControlLock = FALSE;
                /*** ... und LogMsg "Welcome Back User" ***/
                yw_BackToRoboNotify(ywd);
            } else {
                /*** normales Vehikel: In Vehikel-Meldung ***/
                struct logmsg_msg lm;
                lm.bact = ywd->UVBact;
                lm.pri  = 33;
                lm.msg  = NULL;
                lm.code = LOGMSG_CONTROL;
                _methoda(ywd->world,YWM_LOGMSG,&lm);
            };

            /*** DragLock deaktivieren ***/
            ywd->DragLock = FALSE;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_HandleInputSR(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  CHANGED
**      12-Dec-95   floh    created
**      19-Dec-95   floh    debugging...
**      21-Dec-95   floh    alle Popup-Menüs komplett
**      26-Dec-95   floh    + Status-Button Input-Handling
**      25-Mar-96   floh    HotKey-Support
**      06-May-96   floh    + LogWin (aka Info-Requester)
**      10-May-96   floh    + DragLock-Handling beim Aggr-Scrolling
**      24-May-96   floh    + DesignMode-Stuff
**                          - Panic-Modus
**      15-Jul-96   floh    + Cutter: nicht gelinkte Modi werden
**                            aktiviert, sobald diese eingeschaltet
**                            werden
**      22-Jul-96   floh    + Diverse Sonderfall-Behandlung für
**                            neuen Fight-Modus.
**      27-Jul-96   floh    + Massives Bugfixing für Robo-Geschütz-Handling
**      28-Jul-96   floh    revised & updated (Dynamic Layout)
**      03-Aug-96   floh    rewrite: neues Icon-Interface
**      05-Aug-96   floh    + Quit-, Nav-Button-Handling
**      21-Aug-96   floh    + Energy-Window-Handling
**      31-Aug-96   floh    + der Maus-Control-Lock wird nur wieder
**                            deaktiviert, wenn man sich in den
**                            Robo zurücksetzt
**      17-Sep-96   floh    + Tooltips für Statusbar-Icons
**      28-Sep-96   floh    + Control-2-LastMessage-Funktionalität
**      03-Oct-96   floh    + Abort-Mission-Fenster verhält sich jetzt
**                            exakt wie jedes andere Fenster (beim Druck
**                            auf das Exit-Button)
**      28-Feb-97   floh    + beim Aufmachen von Mode- und Submenü
**                            wird LISTF_Select *und* LISTF_NoScroll
**                            gesetzt, damit diese erst anfangen zu
**                            scrollen, wenn Maus schon mal über einem
**                            gültigem Item war.
**      30-May-97   floh    + ModeMenu jetzt mit getrenntem "Rollover-"
**                            und "Selected-" State
**      24-Jul-97   floh    + ModeMenu no longer exists!
**      29-Jul-97   floh    + <do_sub> wurde nicht korrekt auf FALSE
**                            gesetzt!
**      15-Aug-97   floh    + ruft jetzt für das Submenu yw_ListHandleInput()
**                            auch im geschlossenen Zustand auf, damit
**                            irgendwelche kontinuierlichen Prozesse
**                            abgeschlossen werden können.
**      26-Sep-97   floh    + VoiceFeedback bei VehicleChange
**      13-Oct-97   floh    + LogWin und Energy Window Icons raus
**                          + Ctrl2LM wieder drin.
**      23-Oct-97   floh    + Soundausgabe
**      04-Dec-97   floh    + New/Add/Build unified
**      05-Dec-97   floh    + Add-Modus per rechten Click auf CREATE
**                          + Order Button raus
**      08-Dec-97   floh    + Beam Button raus
**      17-Feb-98   floh    + falls SubMenu offen und Escape oder
**                            Enter gedrückt, wird SubMenu geschlossen
**      01-Mar-98   floh    + ooops, Enter ist ja Return...
**      24-Apr-98   floh    + Goto Last Occupied Vehicle Handling
**      25-Apr-98   floh    + Make Current Vehicle Commander Handling
**      21-May-98   floh    + advanced Vehicle-Switch-Window-Status-
**                            Handling
**                          + wenn Vehikel tot und Feuer gedrueckt, 
**                            yw_SRNextCom();
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    Object *act_viewer = ywd->Viewer->BactObject;
    Object *new_viewer = NULL;
    BOOL do_sub  = TRUE;
    SR.DownFlags = 0;

    /*** Status-Bar enabled? ***/
    if (SR.EnabledReqs & STAT_REQF_STATUS) {

        /*** HotKeys abhandeln ***/
        if (ip->HotKey == HOTKEY_CTRL2LM)               new_viewer = yw_SRCtrl2LM(ywd);
        else if (ip->HotKey == HOTKEY_LASTOCCUPIED)     new_viewer = yw_SRLastOccupied(ywd);
        else if (ip->HotKey == HOTKEY_MAKECOMMANDER)    yw_SRMakeCommander(ywd);
        else if (ip->HotKey != 0)                       do_sub=yw_SRHotKey(ywd,ip);

        /*** Vehikel tot und Feuertaste gedrueckt? ***/
        if ((ACTION_DEAD == ywd->UVBact->MainState) && (ywd->FireDown)) {
            new_viewer = yw_SRNextCom(ywd);
            ip->Buttons &= ~BT_FIRE;
        };

        /*** Submenues zumachen? ***/
        if (!(SubMenu.Req.flags & REQF_Closed)) {
            if ((ywd->NormKeyBackup == KEYCODE_ESCAPE) ||
                (ywd->NormKeyBackup == KEYCODE_RETURN))
            {
                yw_CloseReq(ywd,&(SubMenu.Req));
                ip->NormKey = 0;
                ip->ContKey = 0;
                ip->HotKey  = 0;
            };
        };

        #ifndef YPA_DESIGNMODE
        /*** Map <-> Radar abhandeln ***/
        yw_SRMapV2RControl(ywd);
        #endif
        
        /*** aktuellen Fensterstatus mitschreiben ***/
        yw_SRGetWindowStatus(ywd);        

        /*** Maus innerhalb Status-Bars? ***/
        if (ci->box == &(SR.req.req_cbox)) {

            /*** Soundausgabe ***/
            if ((ci->btn != -1) && (ywd->gsr) && (ci->flags & CIF_BUTTONDOWN)) {
                _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
            };

            /*** Buttons im Status-Bar ***/
            switch (ci->btn) {

                /*** Mode Gadgets ***/
                case STATBTN_NEW:
                    if (ci->flags & CIF_BUTTONDOWN) {
                        if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_BUILD|STAT_MODEF_ADD)) {
                            /*** schon an, wieder ausschalten ***/
                            SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
                        } else {
                            /*** NEW oder BUILD Modus aktivieren ***/
                            yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_NEW);
                            if (SR.ActThing < SR.NumVehicles)  SR.ActiveMode = STAT_MODEF_NEW;
                            else                               SR.ActiveMode = STAT_MODEF_BUILD;
                            do_sub=FALSE;
                        };
                    } else if (ci->flags & CIF_RMOUSEDOWN) {
                        if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD|STAT_MODEF_BUILD)) {
                            /*** schon an, wieder ausschalten ***/
                            SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
                        } else {
                            /*** ADD Modus aktivieren ***/
                            yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_ADD);
                            SR.ActiveMode = STAT_MODEF_ADD;
                            do_sub = FALSE;
                        };
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_NEW,HOTKEY_NEW);
                    break;

                /*** eines der "normalen" Fenster ***/
                case STATBTN_MAP:
                case STATBTN_FINDER:
                    if (ci->flags & CIF_BUTTONDOWN) {
                        struct YPAReq *r;
                        switch(ci->btn) {
                            case STATBTN_MAP:    r=&(MR.req); break;
                            case STATBTN_FINDER: r=&(FR.l.Req); break;
                        };
                        if (r->flags & REQF_Closed) {
                            yw_OpenReq(ywd,r);
                            _Remove((struct Node *)r);
                            _AddHead((struct List *) &(ywd->ReqList),
                                     (struct Node *)r);
                        } else {
                            yw_CloseReq(ywd,r);
                        };
                    };
                    switch(ci->btn) {
                        case STATBTN_MAP:    yw_TooltipHotkey(ywd,TOOLTIP_GUI_MAP,HOTKEY_MAP); break;
                        case STATBTN_FINDER: yw_TooltipHotkey(ywd,TOOLTIP_GUI_FINDER,HOTKEY_FINDER); break;
                    };
                    break;

                /*** Control (Pseudo-)Modus ***/
                case STATBTN_CONTROL:
                    if (ci->flags & CIF_BUTTONDOWN) { 
                        if (SR.ActiveMode & STAT_MODEF_CONTROL) {
                            /*** deaktivieren ***/
                            SR.ActiveMode = (STAT_MODEF_ORDER & SR.EnabledModes);
                        } else {
                            /*** aktivieren ***/
                            SR.ActiveMode = STAT_MODEF_CONTROL;
                        };
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_CONTROL,HOTKEY_CONTROL);
                    break;

                /*** ToRobo Button ***/
                case STATBTN_TOROBO:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_TOROBO);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        new_viewer = ywd->UserRobo;
                        if (ywd->UserSitsInRoboFlak) {
                            /*** Robo-Flak -> Robo, Spezialfall ***/
                            SR.ActiveMode = STAT_MODEF_ORDER;
                        };
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_TOROBO,HOTKEY_TOROBO);
                    break;

                /*** ToCommander Button ***/
                case STATBTN_TOCOM:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_TOCOM);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        new_viewer = ywd->UVBact->master;
                        /*** Sonder-Values ausklammern ***/
                        if (new_viewer < ((Object *)0x10)) new_viewer = NULL;
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_TOCMD,HOTKEY_TOCMDR);
                    break;

                /*** NextUnit Button ***/
                case STATBTN_NEXTUNIT:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_NEXTUNIT);
                    };
                    if ((ywd->UVBact==ywd->URBact)||(ywd->UserSitsInRoboFlak)) {
                        /*** To Next Robo Flak Funktionalität ***/
                        if (ci->flags&CIF_BUTTONUP) new_viewer=yw_SRNextGun(ywd);
                        yw_TooltipHotkey(ywd,TOOLTIP_GUI_FIGHT,HOTKEY_NEXTUNIT);
                    } else {
                        /*** User sitzt in einem normalen Fahrzeug ***/
                        if (ci->flags&CIF_BUTTONUP) new_viewer=yw_SRNextUnit(ywd);
                        yw_TooltipHotkey(ywd,TOOLTIP_GUI_NEXTUNIT,HOTKEY_NEXTUNIT);
                    };
                    break;

                /*** NextCommander Button ***/
                case STATBTN_NEXTCOM:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_NEXTCOM);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        new_viewer = yw_SRNextCom(ywd);
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_NEXTCMDR,HOTKEY_NEXTCMDR);
                    break;

                /*** Quit-Button ***/
                case STATBTN_QUIT:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_QUIT);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        if (AMR.l.Req.flags & REQF_Closed) yw_OpenAMR(ywd);
                        else                               yw_CloseAMR(ywd);
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_EXIT,HOTKEY_QUIT);
                    break;

                /*** Help Button ***/
                case STATBTN_HELP:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_HELP);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        yw_AnalyzeGameState(ywd);
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_HELP,HOTKEY_ANALYZER);
                    break;

                /*** Online-Hilfe Button ***/
                case STATBTN_ONLINEHELP:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        SR.DownFlags |= (1<<STATBTN_ONLINEHELP);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        ywd->Url = ypa_GetStr(ywd->LocHandle,STR_HELP_INGAMEGENERAL,"help\\l17.html");
                    };
                    yw_TooltipHotkey(ywd,TOOLTIP_GUI_ONLINEHELP,HOTKEY_HELP);
                    break;

                #ifdef YPA_DESIGNMODE
                case STATBTN_SETSECTOR:
                    if (ci->flags & CIF_BUTTONDOWN) {
                        yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_SETSECTOR);
                        SR.ActiveMode = STAT_MODEF_SETSECTOR;
                        do_sub=FALSE;
                    };
                    break;
                case STATBTN_SETOWNER:
                    if (ci->flags & CIF_BUTTONDOWN) {
                        yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_SETOWNER);
                        SR.ActiveMode = STAT_MODEF_SETOWNER;
                        do_sub = FALSE;
                    };
                    break;
                case STATBTN_SETHEIGHT:
                    if (ci->flags & CIF_BUTTONDOWN) {
                        yw_OpenSubMenu(ywd,TRUE,STAT_MODEF_SETHEIGHT);
                        SR.ActiveMode = STAT_MODEF_SETHEIGHT;
                        do_sub = FALSE;
                    };
                    break;
                #endif

                #ifdef YPA_CUTTERMODE
                case STATBTN_PLAYER:
                    if (ci->flags & CIF_BUTTONDOWN) SR.ActiveMode = STAT_MODEF_PLAYER;
                    break;
                case STATBTN_FREESTATIV:
                    if (ci->flags & CIF_BUTTONDOWN) SR.ActiveMode = STAT_MODEF_FREESTATIV;
                    break;
                case STATBTN_LINKSTATIV:
                    if (ci->flags & CIF_BUTTONDOWN) SR.ActiveMode = STAT_MODEF_LINKSTATIV;
                    break;
                #endif

            };
        };

        #ifdef YPA_CUTTERMODE
            if (SR.ActiveMode & STAT_MODEF_FREESTATIV) {
                struct playercontrol_msg pcm;
                pcm.mode = CAMERA_FREESTATIV;
                pcm.arg0 = 0;
                _methoda(ywd->world, YWM_PLAYERCONTROL, &pcm);
            };
            if (SR.ActiveMode & STAT_MODEF_PLAYER) {
                struct playercontrol_msg pcm;
                pcm.mode = CAMERA_PLAYER;
                pcm.arg0 = 0;
                _methoda(ywd->world, YWM_PLAYERCONTROL, &pcm);
            };
        #endif

        /*** Submenu bei Bedarf schliessen ***/
        if (SR.ActiveMode & (STAT_MODEF_ORDER|STAT_MODEF_AUTOPILOT|STAT_MODEF_CONTROL)) {
            yw_CloseReq(ywd,&(SubMenu.Req));
        };    

        /*** SubMenu Input-Handling? ***/
        if (do_sub) {
            /*** yw_ListHandleInput auch im geschlossenen Zustand aufrufen! ***/
            yw_ListHandleInput(ywd,&SubMenu,ip);
            if (!(SubMenu.Req.flags & REQF_Closed)) {
                /*** Selected Item auswerten ***/
                switch (SR.ActiveMode) {
                    case STAT_MODEF_NEW:
                    case STAT_MODEF_ADD:
                    case STAT_MODEF_BUILD:
                        SR.ActThing = SubMenu.Selected;
                        yw_SRSyncActThingVhclBlg(ywd);
                        break;

                    #ifdef YPA_DESIGNMODE
                    case STAT_MODEF_SETSECTOR:
                        SR.ActSector = SubMenu.Selected;
                        break;
                    case STAT_MODEF_SETOWNER:
                        SR.ActOwner  = SubMenu.Selected;
                        break;
                    case STAT_MODEF_SETHEIGHT:
                        SR.ActHeight = SubMenu.Selected;
                        break;
                    #endif
                };
            };
        };
    };

    /*** neuer Viewer? ***/
    if (new_viewer && (new_viewer != act_viewer)) yw_ChangeViewer(ywd,new_viewer,act_viewer);

    /*** alles Layouten ***/
    yw_RemapThings(ywd);
    yw_LayoutSR(ywd);

    /*** Ende ***/
}


