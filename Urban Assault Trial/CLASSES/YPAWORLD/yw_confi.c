/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  yw_confirm.c -- Do-You-Really-Want-To-Requester fuer YPA.
**
**  (C) Copyright 1998 by A.Weissflog
**
*/
#ifdef _MSC_VER
#include <windows.h>
#endif

#include <exec/types.h>

#include <string.h>
#include <stdio.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guiconfirm.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_audio_engine
/*-----------------------------------------------------------------*/
struct YPAConfirmReq CR;

#define CR_BTN_OK       (LV_NUM_STD_BTN)
#define CR_BTN_CANCEL   (LV_NUM_STD_BTN+1)

/*-----------------------------------------------------------------*/
void yw_CRInitButtons(struct ypaworld_data *ywd)
/*
**  CHANGED
**      22-Apr-98   floh    created
*/
{
    WORD ypos = CR.l.UpperVBorder + ywd->FontH + 2*CR.l.EntryHeight;
    struct ClickButton *ok_btn     = CR.l.Req.req_cbox.buttons[CR_BTN_OK];
    struct ClickButton *cancel_btn = CR.l.Req.req_cbox.buttons[CR_BTN_CANCEL];
    CR.edge_width = ywd->EdgeW + 2*ywd->Fonts[FONTID_DEFAULT]->fchars[' '].width;
    CR.btn_width  = (CR.l.ActEntryWidth - 2*CR.edge_width)/3;
    CR.l.Req.req_cbox.num_buttons = LV_NUM_STD_BTN + 2;
    
    ok_btn->rect.x = CR.edge_width;
    ok_btn->rect.y = ypos;
    ok_btn->rect.w = CR.btn_width;
    ok_btn->rect.h = CR.l.EntryHeight;
    
    cancel_btn->rect.x = CR.l.ActEntryWidth - CR.edge_width - CR.btn_width;
    cancel_btn->rect.y = ypos;
    cancel_btn->rect.w = CR.btn_width;
    cancel_btn->rect.h = CR.l.EntryHeight;

    CR.btn_space = cancel_btn->rect.x - (ok_btn->rect.x + CR.btn_width);
}    
       
/*-----------------------------------------------------------------*/
BOOL yw_InitCR(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Confirm-Requester.
**
**  CHANGED
**      22-Apr-98   floh    created
**      23-Apr-98   floh    wird jetzt ohne HelpGadget erzeugt.
*/
{
    LONG width;
    BOOL retval = FALSE;

    /*** Breite des Requesters festlegen ***/
    width  = 3.5 * yw_StrLen("WWWWWWW", ywd->Fonts[FONTID_DEFAULT]) + 16;

    if (yw_InitListView(ywd, &CR,
            LIST_Title,          " ",
            LIST_Resize,         FALSE,
            LIST_NumEntries,     3,
            LIST_ShownEntries,   3,
            LIST_FirstShown,     0,
            LIST_Selected,       0,
            LIST_MaxShown,       3,
            LIST_MinShown,       3,
            LIST_DoIcon,         FALSE,
            LIST_EntryHeight,    ywd->FontH,
            LIST_EntryWidth,     width,
            LIST_Enabled,        TRUE,
            LIST_VBorder,        ywd->EdgeH,
            LIST_StaticItems,    TRUE,
            LIST_CloseChar,      'U',
            LIST_HasHelpGadget,  FALSE,
            TAG_DONE))
    {
        /*** initialisiere ClickButtons ***/
        yw_CRInitButtons(ywd);
        CR.status = YPACR_STATUS_CLOSED;            
    
        /*** alles OK ***/
        retval = TRUE;
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillCR(struct ypaworld_data *ywd)
/*
**  CHANGED
**      18-Jun-96   floh    created
*/
{
    yw_KillListView(ywd,&CR);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_CRPutLine(struct ypaworld_data *ywd, UBYTE *str, UBYTE *text)
/*
**  FUNCTION
**      Rendert eine Text-Zeile im Confirm-Req. 
**
**  CHANGED
**      22-Apr-98   floh    created
*/
{
    UBYTE fnt_id = FONTID_DEFAULT;
    put(str,'{');   // linker Rand
    put(str,' ');
    put(str,' ');
    str = yw_TextBuildCenteredItem(ywd->Fonts[fnt_id],str,text,
                                   CR.l.ActEntryWidth - 2*CR.edge_width,' ');
    put(str,' ');
    put(str,' ');
    put(str,'}');   // rechter Rand
    new_line(str);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_CRPutButtons(struct ypaworld_data *ywd, UBYTE *str,
                       ULONG down_flags) 
/*
**  CHANGED
**      22-Apr-98   floh    created
*/
{
    UBYTE fnt_id;
    UBYTE *text;    
    
    put(str,'{');   // linker Req-Rand
    put(str,' ');
    put(str,' ');
        
    /*** OK Button ***/ 
    text = ypa_GetStr(ywd->LocHandle,STR_OK,"OK");
    if (down_flags & (1<<CR_BTN_OK)) {
        fnt_id=FONTID_GADGET;
        set_dbcs_flags(str,DBCSF_PRESSED);
    } else {
        fnt_id=FONTID_DEFBLUE;
    };
    new_font(str,fnt_id);
    put(str,'b');   // linker Buttonrand
    str = yw_TextBuildCenteredItem(ywd->Fonts[fnt_id],str,text,
                                   CR.btn_width - 2*ywd->EdgeW,' ');
    unset_dbcs_flags(str,DBCSF_PRESSED);
    put(str,'d');   // rechter Buttonrand
    
    /*** Platz zwischen den Buttons ***/
    new_font(str,FONTID_DEFAULT);
    stretch(str,CR.btn_space);
    put(str,' ');
    
    /*** Cancel-Button ***/
    text = ypa_GetStr(ywd->LocHandle,STR_CANCEL,"CANCEL");
    if (down_flags & (1<<CR_BTN_CANCEL)) {
        fnt_id=FONTID_GADGET;
        set_dbcs_flags(str,DBCSF_PRESSED);
    } else {
        fnt_id=FONTID_DEFBLUE;
    };
    new_font(str,fnt_id);
    put(str,'b');   // linker Buttonrand
    str = yw_TextBuildCenteredItem(ywd->Fonts[fnt_id],str,text,
                                   CR.btn_width - 2*ywd->EdgeW,' ');
    unset_dbcs_flags(str,DBCSF_PRESSED);
    put(str,'d');   // rechter Buttonrand
    new_font(str,FONTID_DEFAULT);
    put(str,' ');
    put(str,' ');
    put(str,'}');
    new_line(str);
    return(str);
}     

/*-----------------------------------------------------------------*/
void yw_CRLayoutItems(struct ypaworld_data *ywd, ULONG down_flags)
/*
**  FUNCTION
**      Layoutet den gesamten Itemblock des ConfirmReq. 
**
**  CHANGED
**      22-Apr-98   floh    created
*/
{
    UBYTE *str = CR.l.Itemblock;

    /*** oberer Rand, Textzeilen, Buttonzeile, unterer Rand ***/    
    str = yw_LVItemsPreLayout(ywd, &CR, str, FONTID_DEFAULT, "{ }");
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEFAULT),yw_Green(ywd,YPACOLOR_TEXT_DEFAULT),yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT));
    str = yw_CRPutLine(ywd,str,CR.text);
    str = yw_CRPutLine(ywd,str," ");
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_BUTTON),yw_Green(ywd,YPACOLOR_TEXT_BUTTON),yw_Blue(ywd,YPACOLOR_TEXT_BUTTON));
    str = yw_CRPutButtons(ywd,str,down_flags);
    str = yw_LVItemsPostLayout(ywd, &CR, str, FONTID_DEFAULT, "xyz");
    
    /*** EOS ***/
    eos(str);
}    

/*-----------------------------------------------------------------*/
void yw_OpenCR(struct ypaworld_data *ywd, UBYTE *text, void *owner) 
/*
**  FUNCTION
**      Wird von extern aufgerufen, um den ConfirmReq in
**      einem bestimmten Modus zu oeffnen.                
**
**  CHANGED
**      31-May-98   floh    neues Arg <owner>
*/
{ 
    yw_OpenReq(ywd,&(CR.l.Req));
    yw_ReqToFront(ywd,&(CR.l.Req));
    CR.text   = text;
    CR.status = YPACR_STATUS_OPEN; 
    CR.owner  = owner;
}

/*-----------------------------------------------------------------*/
void yw_CloseCR(struct ypaworld_data *ywd, BOOL ok)
/*
**  CHANGED
**      23-Apr-98   floh    created
*/
{
    yw_CloseReq(ywd,&(CR.l.Req));
    if (ok) CR.status = YPACR_STATUS_OK;
    else    CR.status = YPACR_STATUS_CANCEL;
}

/*-----------------------------------------------------------------*/
void *yw_CRGetOwner(struct ypaworld_data *ywd)
/*
**  CHANGED
**      31-May-98   floh    created
*/
{
    return(CR.owner);
}

/*-----------------------------------------------------------------*/
ULONG yw_CRGetStatus(struct ypaworld_data *ywd)
/*
**  CHANGED
**      23-Apr-98   floh    created
*/
{
    /*** falls CR.status noch auf Open, aber der Requester         ***/
    /*** schon zu, wurde der Requester per CloseGadget weggeclickt ***/
    if ((CR.status == YPACR_STATUS_OPEN) && (CR.l.Req.flags & REQF_Closed)) {
        CR.status = YPACR_STATUS_CANCEL;
    };
    return(CR.status);
}

/*-----------------------------------------------------------------*/
ULONG yw_CRSetStatus(struct ypaworld_data *ywd, ULONG status)
/*
**  CHANGED
**      23-Apr-98   floh    created
*/
{
    CR.status = status;
}

/*-----------------------------------------------------------------*/
void yw_HandleInputCR(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  CHANGED
**      22-Apr-98   floh    created
**      20-May-98   floh    + Tasteninterface
*/
{
    if (!(CR.l.Req.flags & REQF_Closed)) {
        struct ClickInfo *ci = &(ip->ClickInfo);
        ULONG down_flags = 0;
        
        /*** Tastatur? ***/
        if (ywd->NormKeyBackup == KEYCODE_RETURN) {
            yw_CloseCR(ywd,TRUE);
            ip->NormKey = 0;
            ip->ContKey = 0;
            ip->HotKey  = 0;
        } else if (ywd->NormKeyBackup == KEYCODE_ESCAPE) {
            yw_CloseCR(ywd,FALSE);
            ip->NormKey = 0;
            ip->ContKey = 0;
            ip->HotKey  = 0;
        } else if (ci->box == &(CR.l.Req.req_cbox)) {
        
            /*** Sound ***/
            if ((ywd->gsr) && (ci->btn>=LV_NUM_STD_BTN) && (ci->flags & CIF_BUTTONDOWN)) {
                _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
            };
            switch(ci->btn) {
                case CR_BTN_OK:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        down_flags |= (1<<CR_BTN_OK);
                    };                    
                    if (ci->flags & CIF_BUTTONUP) {
                        yw_CloseCR(ywd,TRUE);
                    };
                    break;
                
                case CR_BTN_CANCEL:
                    if (ci->flags & (CIF_BUTTONDOWN|CIF_BUTTONHOLD)) {
                        down_flags |= (1<<CR_BTN_CANCEL);
                    };
                    if (ci->flags & CIF_BUTTONUP) {
                        yw_CloseCR(ywd,FALSE);
                    };
                    break;
            };
        };
        
        /*** Listview-Input-Handling und Layout ***/
        yw_ListHandleInput(ywd, &CR, ip);
        yw_ListLayout(ywd, &CR);
        yw_CRLayoutItems(ywd,down_flags);
    } else {
        /*** alle kontinuierlichen Aktionen loeschen ***/
        yw_ListHandleInput(ywd, &CR, ip);
    };
}                          
