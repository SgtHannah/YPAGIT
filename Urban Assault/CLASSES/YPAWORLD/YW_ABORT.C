/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_abort.c,v $
**  $Revision: 38.9 $
**  $Date: 1998/01/06 16:16:47 $
**  $Locker: floh $
**  $Author: floh $
**
**  Mission-Abort-Requester.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <string.h>
#include <stdio.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guiabort.h"
#include "ypa/guiconfirm.h"

#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "yw_netprotos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_audio_engine
/*-----------------------------------------------------------------*/
struct YPAAbortReq AMR;

/*-----------------------------------------------------------------*/
void yw_AMRSetButtons(struct ypaworld_data *ywd)
/*
**  CHANGED
**      23-Apr-98   floh    created
*/
{
    WORD ypos = AMR.l.UpperVBorder + ywd->FontH + AMR.entry_space;
    ULONG i,j;
    AMR.edge_width = ywd->EdgeW + 2*ywd->Fonts[FONTID_DEFAULT]->fchars[' '].width;
    AMR.btn_width  = AMR.l.ActEntryWidth - 2*AMR.edge_width;
    for (j=0,i=AMR_BTN_CANCEL; i<=AMR_BTN_RESUME; i++,j++) {
        struct ClickButton *cb = AMR.l.Req.req_cbox.buttons[i];
        cb->rect.x = AMR.edge_width;
        cb->rect.y = ypos + j*AMR.entry_height;
        cb->rect.w = AMR.btn_width;
        cb->rect.h = ywd->FontH;
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_InitAMR(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Abort-Mission-Req (ist eigentlich ein
**      Listview, um Zeit zu sparen...)
**
**  CHANGED
**      18-Jun-96   floh    created
**      19-Jun-96   floh    + LISTF_Static, sowie Manipulation der
**                            Buttons im Requester (für Yes,No-Buttons)
**      31-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      10-Aug-96   floh    + CloseGadget mit Custom-Icon
**      20-Aug-96   floh    + Bugfix: LIST_VBorder war auf ywd->EdgeW
**                            statt ywd->EdgeH
**      01-Oct-96   floh    + Neues Layout, mehr Möglichkeiten
*/
{
    LONG width;
    BOOL retval = FALSE;

    UBYTE *title = ypa_GetStr(ywd->LocHandle, STR_WIN_ABORT, "GAME PAUSED");

    /*** Breite des Requesters festlegen ***/
    width  = 2 * yw_StrLen("WWWWWWW", ywd->Fonts[FONTID_DEFAULT]) + 16;
    AMR.entry_height = ywd->FontH + (ywd->FontH>>1);
    AMR.entry_space  = ywd->FontH>>2;
    if (yw_InitListView(ywd, &(AMR.l),
            LIST_Title,          title,
            LIST_Resize,         FALSE,
            LIST_NumEntries,     5,
            LIST_ShownEntries,   5,
            LIST_FirstShown,     0,
            LIST_Selected,       0,
            LIST_MaxShown,       5,
            LIST_MinShown,       5,
            LIST_DoIcon,         FALSE,
            LIST_EntryHeight,    AMR.entry_height,
            LIST_EntryWidth,     width,
            LIST_Enabled,        TRUE,
            LIST_VBorder,        ywd->EdgeH,
            LIST_StaticItems,    TRUE,
            LIST_CloseChar,      'U',
            TAG_DONE))
    {
        /*** alles OK ***/
        yw_AMRSetButtons(ywd);
        retval = TRUE;
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillAMR(struct ypaworld_data *ywd)
/*
**  CHANGED
**      18-Jun-96   floh    created
*/
{
    yw_KillListView(ywd,&(AMR.l));
}

/*-----------------------------------------------------------------*/
void yw_OpenAMR(struct ypaworld_data *ywd)
/*
**  CHANGED
**      23-Apr-98   floh    created
*/
{
    /*** falls ConfirmReq gerade auf ist, ignorieren ***/
    if (yw_CRGetStatus(ywd) != YPACR_STATUS_OPEN) {
        yw_OpenReq(ywd,&(AMR.l.Req));
        _Remove((struct Node *) &(AMR.l.Req));
        _AddHead((struct List *) &(ywd->ReqList),
                 (struct Node *) &(AMR.l.Req));
    };
}

/*-----------------------------------------------------------------*/
void yw_CloseAMR(struct ypaworld_data *ywd)
/*
**  CHANGED
**      23-Apr-98   floh    created
*/    
{
    /*** falls ConfirmReq gerade auf ist, ignorieren ***/
    if (yw_CRGetStatus(ywd) != YPACR_STATUS_OPEN) {
        yw_CloseReq(ywd,&(AMR.l.Req));
    };
}

/*-----------------------------------------------------------------*/
UBYTE *yw_AMRLine(struct ypaworld_data *ywd,
                  UBYTE *str,
                  BOOL disabled, BOOL down,
                  UBYTE *text)
/*
**  CHANGED
**      01-Oct-96   floh    created
**      23-Jan-97   floh    neue Fonts.
**      24-Nov-97   floh    + DBCS Enabled
**      23-Apr-98   floh    + neues "Spacy" Layout
*/
{
    UBYTE fnt_id;
    
    /*** upper Space ***/
    len_vert(str,AMR.entry_space);
    put(str,'{');
    put(str,' ');
    put(str,' ');
    lstretch_to(str,(AMR.btn_width + AMR.edge_width));
    put(str,' ');
    put(str,' ');
    put(str,' ');
    put(str,'}');
    new_line(str);
    
    /*** eigentliche Buttonzeile ***/
    put(str,'{');   // linker Rand Requester
    put(str,' ');
    put(str,' ');

    if (disabled)  fnt_id=FONTID_DEFWHITE;
    else if (down) fnt_id=FONTID_GADGET;
    else           fnt_id=FONTID_DEFBLUE;

    new_font(str,fnt_id);
    put(str,'b');   // linker Rand Button
    if (disabled) text = " ";
    if (down) {
        set_dbcs_flags(str,DBCSF_PRESSED);
    };
    str = yw_TextBuildCenteredItem(ywd->Fonts[fnt_id],str,text,
                                   AMR.btn_width - 2*ywd->EdgeW,' ');
    unset_dbcs_flags(str,DBCSF_PRESSED);
    put(str,'d');   // rechter Rand Button
    new_font(str,FONTID_DEFAULT);
    put(str,' ');
    put(str,' ');
    put(str,'}');   // rechter Rand
    new_line(str);
    
    /*** lower Space ***/
    len_vert(str,AMR.entry_space);
    put(str,'{');
    put(str,' ');
    put(str,' ');
    lstretch_to(str,(AMR.btn_width + AMR.edge_width));
    put(str,' ');
    put(str,' ');
    put(str,' ');
    put(str,'}');
    new_line(str);

    return(str);
}

/*-----------------------------------------------------------------*/
void yw_AMRLayoutItems(struct ypaworld_data *ywd, ULONG down, ULONG dis)
/*
**  FUNCTION
**      Layoutet Item-Block des AMR, <down> und <dis>
**      definieren pro Button ein Bit für Down und
**      disabled!
**
**  CHANGED
**      18-Jun-96   floh    created
**      31-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      10-Aug-96   floh    oberen Rand angepaßt an neues Layout
**      19-Aug-96   floh    Layout-Bug im Hires-Modus
**      01-Oct-96   floh    neues Layout, mehr Optionen
*/
{
    UBYTE *str = AMR.l.Itemblock;
    UBYTE *can_str  = ypa_GetStr(ywd->LocHandle,STR_CANCELMISSION,"CANCEL MISSION");
    UBYTE *save_str = ypa_GetStr(ywd->LocHandle,STR_SAVE,"SAVE");
    UBYTE *load_str = ypa_GetStr(ywd->LocHandle,STR_LOAD,"LOAD");
    UBYTE *rest_str = ypa_GetStr(ywd->LocHandle,STR_RESTART,"RESTART");
    UBYTE *resm_str = ypa_GetStr(ywd->LocHandle,STR_RESUME,"RESUME");

    /*** oberen Rand rendern ***/
    str = yw_LVItemsPreLayout(ywd, &(AMR.l), str, FONTID_DEFAULT, "{ }");

    /*** die Items ***/
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_BUTTON),yw_Green(ywd,YPACOLOR_TEXT_BUTTON),yw_Blue(ywd,YPACOLOR_TEXT_BUTTON));
    str = yw_AMRLine(ywd,str,(dis&(1<<AMR_BTN_CANCEL)),(down&(1<<AMR_BTN_CANCEL)),can_str);
    str = yw_AMRLine(ywd,str,(dis&(1<<AMR_BTN_SAVE)),(down&(1<<AMR_BTN_SAVE)),save_str);
    str = yw_AMRLine(ywd,str,(dis&(1<<AMR_BTN_LOAD)),(down&(1<<AMR_BTN_LOAD)),load_str);
    str = yw_AMRLine(ywd,str,(dis&(1<<AMR_BTN_RESTART)),(down&(1<<AMR_BTN_RESTART)),rest_str);
    str = yw_AMRLine(ywd,str,(dis&(1<<AMR_BTN_RESUME)),(down&(1<<AMR_BTN_RESUME)),resm_str);

    /*** unteren Rand rendern ***/
    str = yw_LVItemsPostLayout(ywd, &(AMR.l), str, FONTID_DEFAULT, "xyz");

    /*** EOS ***/
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_HandleInputAMR(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Input-Handler des AMR, etwas eigenartig, weil der AMR
**      ja eigentlich ein ListView ist.
**
**  CHANGED
**      18-Jun-96   floh    created
**      31-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      01-Oct-96   floh    revised & updated (Paused Game Funktionalität)
**      24-Oct-96   floh    + LEVELSTAT_SAVE und LEVELSTAT_LOAD
**      07-Dec-96   floh    + Palette-Fade bei LEVELSTAT_ABORT,
**                            LEVELSTAT_LOAD, LEVELSTAT_RESTART.
**      16-Dec-96   floh    + SAVE setzt CANCEL-Pointer und
**                            schließt den Pausen-Requester
**                          + HOTKEY_QUIT Handling entfernt, in
**                            Statusbar-Input-Handling wird dann der
**                            Pausenrequester bei HOTKEY_QUIT wieder
**                            geschlossen.
**                          + Test, ob für aktuellen Level Savegame
**                            existiert
**      12-Jan-97   floh    + falls UserVehicle tot, kein Savegame
**                            möglich
**      02-Aug-97   floh    + testet, ob Finalsavegame existiert
**      04-Aug-97   floh    + bei Restart-Button-Test darf natürlich
**                            nicht auf Final-, sondern auf Restart-
**                            Savegame getestet werden!
**      15-Aug-97   floh    + ruft yw_ListHandleInput() jetzt auch
**                            im geschlossenen Zustand auf, um
**                            kontinuierliche Prozesse abzubrechen.
**      23-Oct-97   floh    + Sound
**      09-Feb-98   floh    + Bugfix: im Netzwerk-Spiel war Load, Save
**                            und Restart an.
**      05-Mar-98   floh    + Online-Hilfe
**      23-Apr-98   floh    + umgeschrieben fuer neuen Abort-Req
*/
{
    ULONG cr_status;

    if (AMR.l.Req.flags & REQF_Closed) {
        if (ywd->Level->Status != LEVELSTAT_PLAYING) {
            /*** wurde Savegame erzeugt? ***/
            if (LEVELSTAT_SAVE == ywd->Level->Status) {
                struct logmsg_msg lm;
                lm.bact = NULL;
                lm.pri  = 10;
                lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_GAME_SAVED,"GAME SAVED OK.");
                lm.code = LOGMSG_NOP;
                _methoda(ywd->world,YWM_LOGMSG,&lm);
            };
            ywd->Level->Status = LEVELSTAT_PLAYING;
        };
    } else {
        if (ywd->Level->Status != LEVELSTAT_PAUSED) {
            /*** Req wurde gerade geöffnet ***/
            ywd->Level->Status = LEVELSTAT_PAUSED;
            if (ywd->gsr) {
                /*** existiert SaveGame für diesen Slot (0) ? ***/
                if (yw_ExistSaveGame(0,ywd->gsr->UserName))
                    ywd->SaveGameExists = TRUE;
                else
                    ywd->SaveGameExists = FALSE;
                if (yw_ExistRestartSaveGame(ywd->Level->Num,ywd->gsr->UserName))
                    ywd->RestartSaveGameExists = TRUE;
                else
                    ywd->RestartSaveGameExists = FALSE;
            };
        };
    };
    
    /*** Confirm-Requester auswerten und evtl. Aktion starten ***/
    cr_status = yw_CRGetStatus(ywd);
    if (cr_status == YPACR_STATUS_OK) {
        switch (AMR.action) {
            case AMR_BTN_CANCEL:
                ywd->Level->Status = LEVELSTAT_ABORTED;
                if(ywd->playing_network) yw_SendAnnounceQuit(ywd);
                break;
            case AMR_BTN_SAVE:
                ywd->Level->Status = LEVELSTAT_SAVE;
                break;
            case AMR_BTN_LOAD:
                ywd->Level->Status = LEVELSTAT_LOAD;
                break;
            case AMR_BTN_RESTART:
                ywd->Level->Status = LEVELSTAT_RESTART;
                break;
        };
        yw_CRSetStatus(ywd,YPACR_STATUS_CLOSED);
    } else if (cr_status == YPACR_STATUS_CANCEL) {
        yw_CRSetStatus(ywd,YPACR_STATUS_CLOSED);
    };
    
    /*** eigentliches Input-Handling ***/
    if (!(AMR.l.Req.flags & REQF_Closed)) {

        struct ClickInfo *ci = &(ip->ClickInfo);
        ULONG down_flags = 0;
        ULONG dis_flags  = 0;

        /*** Load-Button disablen, weil kein SaveGame? ***/
        if (!ywd->SaveGameExists)        dis_flags |= (1<<AMR_BTN_LOAD);
        if (!ywd->RestartSaveGameExists) dis_flags |= (1<<AMR_BTN_RESTART);

        /*** Save-Button disablen, weil UserVehicle tot? ***/
        if (ywd->UVBact->MainState==ACTION_DEAD) dis_flags |= (1<<AMR_BTN_SAVE);

        /*** Save- und Load-Button disablen, weil Networkplay? ***/
        if (ywd->playing_network) {
            dis_flags |= ((1<<AMR_BTN_SAVE)|(1<<AMR_BTN_LOAD)|(1<<AMR_BTN_RESTART));
        };

        /*** GUI Input auswerten ***/
        if (ci->box == &(AMR.l.Req.req_cbox)) {

            /*** Sound ***/
            if ((ywd->gsr)&&(ci->btn>=LV_NUM_STD_BTN)&&(ci->flags & CIF_BUTTONDOWN)) {
                _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
            };
            switch (ci->btn) {

                case LV_BTN_HELP:
                    if (ci->flags & CIF_BUTTONUP) {
                        ywd->Url = ypa_GetStr(ywd->LocHandle,STR_HELP_INGAMEMENU,"help\\l16.html");
                    };
                    break;

                case AMR_BTN_CANCEL:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        down_flags |= (1<<AMR_BTN_CANCEL);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        AMR.action = AMR_BTN_CANCEL;
                        yw_CloseAMR(ywd);
                        yw_OpenCR(ywd,ypa_GetStr(ywd->LocHandle,STR_CONFIRM_EXIT,"REALLY EXIT MISSION ?"));
                    };
                    break;

                case AMR_BTN_SAVE:
                    /*** Save-Button kann disabled sein ***/
                    if ((dis_flags & (1<<AMR_BTN_SAVE)) == 0) {
                        if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                            down_flags |= (1<<AMR_BTN_SAVE);
                        };
                        if (ci->flags & CIF_BUTTONUP) {
                            AMR.action = AMR_BTN_SAVE;
                            yw_CloseAMR(ywd);
                            yw_OpenCR(ywd,ypa_GetStr(ywd->LocHandle,STR_CONFIRM_SAVE,"REALY SAVE GAME ?"));
                        };
                    };
                    break;

                case AMR_BTN_LOAD:
                    /*** Load-Button kann disabled sein ***/
                    if ((dis_flags & (1<<AMR_BTN_LOAD)) == 0) {
                        if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                            down_flags |= (1<<AMR_BTN_LOAD);
                        };
                        if (ci->flags & CIF_BUTTONUP) {
                            AMR.action = AMR_BTN_LOAD;
                            yw_CloseAMR(ywd);
                            yw_OpenCR(ywd,ypa_GetStr(ywd->LocHandle,STR_CONFIRM_LOAD,"REALY LOAD GAME ?"));
                        };
                    };
                    break;

                case AMR_BTN_RESTART:
                    /*** Restart-Button kann jetzt auch disabled sein ***/
                    if ((dis_flags & (1<<AMR_BTN_RESTART)) == 0) {
                        if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                            down_flags |= (1<<AMR_BTN_RESTART);
                        };
                        if (ci->flags & CIF_BUTTONUP) {
                            AMR.action = AMR_BTN_RESTART;
                            yw_CloseAMR(ywd);
                            yw_OpenCR(ywd,ypa_GetStr(ywd->LocHandle,STR_CONFIRM_RESTART,"REALLY RESTART MISSION ?"));
                        };
                    };
                    break;

                case AMR_BTN_RESUME:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        down_flags |= (1<<AMR_BTN_RESUME);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        /*** Requester schließen und einfach weiter ***/
                        yw_CloseReq(ywd,&(AMR.l.Req));
                    };
                    break;
            };
        };

        /*** Listview-Input-Handling, und Layout ***/
        yw_ListHandleInput(ywd, &(AMR.l), ip);
        yw_ListLayout(ywd, &(AMR.l));

        /*** Itemblock layouten ***/
        yw_AMRLayoutItems(ywd,down_flags,dis_flags);
    } else {
        /*** alle kontinuierlichen Aktionen löschen ***/
        yw_ListHandleInput(ywd, &(AMR.l), ip);
    };
}

