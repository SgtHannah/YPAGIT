/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_superitem.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/02/16 22:45:54 $
**  $Locker: floh $
**  $Author: floh $
**
**  yw_superitem.c -- die Superitems.
**
**  (C) Copyright 1998 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_audio_engine

extern struct YPAEnergyBar EB;

void yw_SuperBombStartTrigger(struct ypaworld_data *ywd, ULONG item_num);
void yw_SuperBombTrigger(struct ypaworld_data *ywd, ULONG item_num);

/*-----------------------------------------------------------------*/
ULONG yw_ParseSuperItemData(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst globale Superitem-Definitionen aus der world.ini.
**
**  CHANGED
**      12-Feb-98   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    if (PARSESTAT_READY == p->status) {

        /*** momentan ausserhalb eines Contexts ***/
        if (stricmp(kw,"begin_superitem")==0) {
            struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
            ywd->SBombWallVProto   = 0;
            ywd->SBombCenterVProto = 0;
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** superbomb_center_vproto ***/
        }else if (stricmp(kw,"superbomb_center_vproto")==0)
            ywd->SBombCenterVProto = strtol(data,NULL,0);

        /*** superbomb_wall_vproto ***/
        else if (stricmp(kw,"superbomb_wall_vproto")==0)
            ywd->SBombWallVProto = strtol(data,NULL,0);

        /*** UNKNOWN KEYWORD ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** alles ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
void yw_InitSuperItems(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert die vom LDF-Parser gelesenen
**      Superitem-Definitionen.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    ULONG i;
    for (i=0; i<ywd->Level->NumItems; i++) {

        ULONG j;
        struct SuperItem *item = &(ywd->Level->Item[i]);
        struct createbuilding_msg cvm;

        item->sec = &(ywd->CellArea[item->sec_y * ywd->MapSizeX + item->sec_x]);

        /*** baue inactive Build Proto ***/
        cvm.job_id    = item->sec->Owner;
        cvm.owner     = item->sec->Owner;
        cvm.bp        = item->inactive_bp;
        cvm.immediate = TRUE;
        cvm.sec_x     = item->sec_x;
        cvm.sec_y     = item->sec_y;
        cvm.flags     = 0;
        _methoda(ywd->world,YWM_CREATEBUILDING,&cvm);

        item->sec->WType  = WTYPE_SuperItem;
        item->sec->WIndex = i;

        for (j=0; j<item->num_keysecs; j++) {
            ULONG x = item->keysec[j].sec_x;
            ULONG y = item->keysec[j].sec_y;
            if ((x > 0) && (x < ywd->MapSizeX-1) &&
                (y > 0) && (y < ywd->MapSizeY-1))
            {
                item->keysec[j].sec = &(ywd->CellArea[y * ywd->MapSizeX + x]);
            };
        };

        item->status = SI_STATUS_INACTIVE;
        item->active_timestamp  = 0;
        item->trigger_timestamp = 0;
        item->activated_by      = 0;
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_AllSectorsOwned(struct ypaworld_data *ywd,
                        ULONG item_num,
                        ULONG owner)
/*
**  FUNCTION
**      Returniert TRUE, falls alle Sektoren dem angegebenen
**      Owner gehören.
**
**  CHANGED
**      10-Feb-98   floh    created
**      18-Feb-98   floh    Bugfix: klappte nur, wenn mindestens
**                          1 Keysektor definiert war
*/
{
    ULONG i;
    BOOL retval = TRUE;
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    if (item->sec->Owner != owner) retval = FALSE;
    else {
        for (i=0; i<item->num_keysecs; i++) {
            if (item->keysec[i].sec->Owner != owner) {
                retval = FALSE;
                break;
            };
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL yw_NoSectorsOwned(struct ypaworld_data *ywd,
                       ULONG item_num,
                       ULONG owner)
/*
**  FUNCTION
**      Returniert TRUE, falls keiner der Item-Sektoren
**      dem angegebenen Owner gehören.
**
**  CHANGED
**      10-Feb-98   floh    created
**      18-Feb-98   floh    + Bugfix: klappte nur, wenn mindestens 1
**                            Keysektor definiert war.
*/
{
    ULONG i;
    BOOL retval = TRUE;
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    if (item->sec->Owner == owner) retval = FALSE;
    else {
        for (i=0; i<item->num_keysecs; i++) {
            if (item->keysec[i].sec->Owner == owner) {
                retval = FALSE;
                break;
            };
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL yw_ExistsOwnerRobo(struct ypaworld_data *ywd, ULONG owner)
/*
**  FUNCTION
**      Returniert TRUE, falls noch mindestens 1 Robo
**      mit dem angegebenen Owner-Code existiert.
**
**  CHANGED
**      16-Feb-98   floh    created
*/
{
    struct MinList *ls;
    struct MinNode *nd;

    ls = &(ywd->CmdList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct OBNode *obnd = (struct OBNode *)nd;
        struct Bacterium *rbact = obnd->bact;
        if ((BCLID_YPAROBO == rbact->BactClassID) &&
            (owner         == rbact->Owner))
        {
            /*** es existiert noch einer! ***/
            return(TRUE);
        };
    };

    /*** wenn bis hier gekommen, existiert keiner mehr ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_InitActiveItem(struct ypaworld_data *ywd,
                       ULONG item_num,
                       BOOL continue_freeze)
/*
**  FUNCTION
**      Schaltet ein Superitem aktiv.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    struct logmsg_msg lm;
    struct createbuilding_msg cvm;

    /*** Item-Struktur updaten ***/
    item->status = SI_STATUS_ACTIVE;
    item->trigger_timestamp = 0;
    item->activated_by = item->sec->Owner;
    if (!continue_freeze) {
        item->active_timestamp  = ywd->TimeStamp;
        item->countdown    = item->time_diff;
        item->last_ten_sec = 0;
        item->last_sec     = 0;
    };

    /*** Aktiv-Zustand visuell ***/
    cvm.job_id    = item->sec->Owner;
    cvm.owner     = item->sec->Owner;
    cvm.bp        = item->active_bp;
    cvm.immediate = TRUE;
    cvm.sec_x     = item->sec_x;
    cvm.sec_y     = item->sec_y;
    cvm.flags     = 0;
    _methoda(ywd->world,YWM_CREATEBUILDING,&cvm);
    item->sec->WType  = WTYPE_SuperItem;
    item->sec->WIndex = item_num;

    lm.bact = NULL;
    lm.pri  = 90;
    switch(item->type) {
        case SI_TYPE_BOMB:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERBOMB_ACTIVATED,"Superbomb activated.");
            lm.code = LOGMSG_SUPERBOMB_ACTIVATED;
            break;
        case SI_TYPE_WAVE:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERWAVE_ACTIVATED,"Superwave activated.");
            lm.code = LOGMSG_SUPERWAVE_ACTIVATED;
            break;
        default:
            lm.msg  = "Cant happen.";
            lm.code = LOGMSG_NOP;
            break;
    };
    _methoda(ywd->world,YWM_LOGMSG,&lm);
}

/*-----------------------------------------------------------------*/
void yw_InitTriggerItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Schaltet Superitem in Trigger-Zustand.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    struct logmsg_msg lm;
    struct createbuilding_msg cvm;

    /*** Item-Struktur updaten ***/
    item->status = SI_STATUS_TRIGGERED;
    item->trigger_timestamp = ywd->TimeStamp;

    /*** Trigger-Zustand visuell ***/
    cvm.job_id    = item->sec->Owner;
    cvm.owner     = item->sec->Owner;
    cvm.bp        = item->trigger_bp;
    cvm.immediate = TRUE;
    cvm.sec_x     = item->sec_x;
    cvm.sec_y     = item->sec_y;
    cvm.flags     = 0;
    _methoda(ywd->world,YWM_CREATEBUILDING,&cvm);
    item->sec->WType  = WTYPE_SuperItem;
    item->sec->WIndex = item_num;

    yw_SuperBombStartTrigger(ywd,item_num);

    lm.bact = NULL;
    lm.pri  = 90;
    switch(item->type) {
        case SI_TYPE_BOMB:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERBOMB_TRIGGERED,"Superbomb triggered.");
            lm.code = LOMGSG_SUPERBOMB_TRIGGERED;
            break;
        case SI_TYPE_WAVE:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERWAVE_TRIGGERED,"Superwave triggered.");
            lm.code = LOGMSG_SUPERWAVE_TRIGGERED;
            break;
        default:
            lm.msg  = "Cant happen.";
            lm.code = LOGMSG_NOP;
            break;
    };
    _methoda(ywd->world,YWM_LOGMSG,&lm);
}

/*-----------------------------------------------------------------*/
void yw_InitFreezeItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Schaltet SuperItem in Freeze-Zustand.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    struct logmsg_msg lm;
    struct createbuilding_msg cvm;

    /*** Item-Struktur updaten ***/
    item->status = SI_STATUS_FROZEN;

    /*** inaktiv Zustand visuell ***/
    cvm.job_id    = item->sec->Owner;
    cvm.owner     = item->sec->Owner;
    cvm.bp        = item->inactive_bp;
    cvm.immediate = TRUE;
    cvm.sec_x     = item->sec_x;
    cvm.sec_y     = item->sec_y;
    cvm.flags     = 0;
    _methoda(ywd->world,YWM_CREATEBUILDING,&cvm);
    item->sec->WType  = WTYPE_SuperItem;
    item->sec->WIndex = item_num;

    lm.bact = NULL;
    lm.pri  = 90;
    switch(item->type) {
        case SI_TYPE_BOMB:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERBOMB_FROZEN,"Superbomb frozen.");
            lm.code = LOGMSG_SUPERBOMB_FROZEN;
            break;
        case SI_TYPE_WAVE:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERWAVE_FROZEN,"Superwave frozen.");
            lm.code = LOGMSG_SUPERWAVE_FROZEN;
            break;
        default:
            lm.msg  = "Cant happen.";
            lm.code = LOGMSG_NOP;
            break;
    };
    _methoda(ywd->world,YWM_LOGMSG,&lm);
}

/*-----------------------------------------------------------------*/
void yw_InitInactiveItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Initialisiert das Superitem in den Inaktiv-Zustand.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    struct logmsg_msg lm;
    struct createbuilding_msg cvm;

    /*** Item-Struktur updaten ***/
    item->status = SI_STATUS_INACTIVE;
    item->active_timestamp  = 0;
    item->trigger_timestamp = 0;
    item->activated_by      = 0;
    item->countdown         = 0;

    /*** inaktiv Zustand visuell ***/
    cvm.job_id    = item->sec->Owner;
    cvm.owner     = item->sec->Owner;
    cvm.bp        = item->inactive_bp;
    cvm.immediate = TRUE;
    cvm.sec_x     = item->sec_x;
    cvm.sec_y     = item->sec_y;
    cvm.flags     = 0;
    _methoda(ywd->world,YWM_CREATEBUILDING,&cvm);
    item->sec->WType  = WTYPE_SuperItem;
    item->sec->WIndex = item_num;

    lm.bact = NULL;
    lm.pri  = 90;
    switch(item->type) {
        case SI_TYPE_BOMB:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERBOMB_DEACTIVATED,"Superbomb deactivated.");
            lm.code = LOGMSG_SUPERBOMB_DEACTIVATED;
            break;
        case SI_TYPE_WAVE:
            lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_SUPERWAVE_DEACTIVATED,"Superwave deactivated.");
            lm.code = LOGMSG_SUPERWAVE_DEACTIVATED;
            break;
        default:
            lm.msg  = "Cant happen.";
            lm.code = LOGMSG_NOP;
            break;
    };
    _methoda(ywd->world,YWM_LOGMSG,&lm);
}

/*-----------------------------------------------------------------*/
void yw_HandleInactiveItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Handler für inaktive Superitems.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    if ((yw_AllSectorsOwned(ywd,item_num,item->sec->Owner)) &&
        (yw_ExistsOwnerRobo(ywd,item->sec->Owner)))
    {
        yw_InitActiveItem(ywd,item_num,FALSE);
    };
}

/*-----------------------------------------------------------------*/
void yw_HandleActiveItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Handelt ein aktiv geschaltetes Item ab.
**
**  CHANGED
**      10-Feb-98   floh    created
**      16-Feb-98   floh    + falls für das Item kein Robo gleichen
**                            Owners mehr existiert, wird das Item
**                            deaktiviert.
**      03-Apr-98   floh    + falls Keysektor-loses Item, wird
**                            Item nicht eingefroren, sondern
**                            für anderen User aktiviert
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);

    /*** noch ein Robo dafür da? ***/
    if (yw_ExistsOwnerRobo(ywd,item->sec->Owner)) {
        if (yw_AllSectorsOwned(ywd,item_num,item->sec->Owner)) {
            /*** alle Sektoren ok ***/
            if (item->countdown <= 0) {
                yw_InitTriggerItem(ywd,item_num);
            } else {
                /*** Countdown zählt weiter ***/
                item->countdown -= ywd->FrameTime;
            };
        } else {
            if (item->num_keysecs == 0){
                /*** falls Keysektor-loses Item, aktivieren ***/
                yw_InitActiveItem(ywd,item_num,FALSE);
            }else{
                /*** mindestens ein Keysektor ist wech, also freezen ***/
                yw_InitFreezeItem(ywd,item_num);
            };
        };
    } else {
        /*** ooops, kein Robo mehr da ***/
        yw_InitInactiveItem(ywd,item_num);
    };
}

/*-----------------------------------------------------------------*/
void yw_HandleFreezeItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Handler für SuperItems im Freeze-Zustand.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);

    if (yw_ExistsOwnerRobo(ywd,item->activated_by)) {
        /*** Aktivator-Robo existiert noch ***/
        if (yw_AllSectorsOwned(ywd,item_num,item->activated_by)) {
            /*** alle Sektoren gehören wieder dem Aktivator ***/
            yw_InitActiveItem(ywd,item_num,TRUE);
        } else if (yw_NoSectorsOwned(ywd,item_num,item->activated_by)) {
            /*** keiner der Sektoren gehört dem Aktivator ***/
            yw_InitInactiveItem(ywd,item_num);
        };
    } else {
        /*** Aktivator tot, Item deaktivieren ***/
        yw_InitInactiveItem(ywd,item_num);
    };
}

/*-----------------------------------------------------------------*/
void yw_HandleTriggeredItem(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Triggered-Zustand bleibt solange aktiv, bis ALLE
**      Sektoren jemanden anderes gehören, als dem
**      Aktivator. Dann wird das Item inaktiv geschaltet.
**      Ansonsten wird der Trigger-Handler aufgerufen.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    if ((yw_NoSectorsOwned(ywd,item_num,item->activated_by)) ||
        (!yw_ExistsOwnerRobo(ywd,item->activated_by)))
    {
        /*** alle Sektoren weggenommen, oder Aktivator tot, abschalten ***/
        yw_InitInactiveItem(ywd,item_num);
    } else {
        /*** die Triggerhandler aufrufen ***/
        switch(item->type) {
            case SI_TYPE_BOMB:
                yw_SuperBombTrigger(ywd,item_num);
                break;
            case SI_TYPE_WAVE:
                // FIXME: Stoudsonwave-Handler aufrufen
                break;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerItemCountdownSound(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Handelt die Countdown-Soundausgabe ab.
**
**  CHANGED
**      12-Feb-98   floh    created
*/
{
    if (ywd->gsr) {
        struct SuperItem *item = &(ywd->Level->Item[item_num]);
        LONG sec = (item->countdown>>10);
        if ((sec<10) && (sec != item->last_sec)) {
            /*** die Einer ***/
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
            item->last_sec = sec;
        };
        if ((sec/10) != item->last_ten_sec) {
            /*** die Zehner ***/
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
            item->last_ten_sec = (sec/10);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerSuperItems(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Handelt jedes einzelne Superitem entsprechend seinem
**      aktuellem Status ab.
**
**  CHANGED
**      10-Feb-98   floh    created
*/
{
    ULONG i;

    /*** für jedes SuperItem... ***/
    for (i=0; i<ywd->Level->NumItems; i++) {

        struct SuperItem *item = &(ywd->Level->Item[i]);
        if (item->type != SI_TYPE_NONE) {

            switch(item->status) {

                case SI_STATUS_INACTIVE:
                    /*** Item ist inaktiv, und wird aktiviert, wenn ***/
                    /*** alle Keysektoren, inklusive Item einem     ***/
                    /*** Owner gehören.                             ***/
                    yw_HandleInactiveItem(ywd,i);
                    break;

                case SI_STATUS_ACTIVE:
                    /*** Status bleibt aktiv, solange alle          ***/
                    /*** Keysektoren inklusive dem Item selbst      ***/
                    /*** dem ursprünglichen Aktivator gehören, und  ***/
                    /*** der Countdown heruntergezählt ist. Fällt   ***/
                    /*** ein Sektor weg, wird der Frozen-Zustand    ***/
                    /*** aktiviert, ist der Countdown herunter-     ***/
                    /*** gezählt, wird der Trigger-Zustand          ***/
                    /*** aktiviert.                                 ***/
                    yw_HandleActiveItem(ywd,i);
                    yw_TriggerItemCountdownSound(ywd,i);
                    break;

                case SI_STATUS_FROZEN:
                    /*** Status bleibt eingefroren, solange noch    ***/
                    /*** mindestens 1 Keysektor dem Aktivator       ***/
                    /*** gehört. Wenn ihm keiner mehr gehört, wird  ***/
                    /*** das Item deaktiviert. Gehören alle         ***/
                    /*** Keysektoren plus Item dem Aktivator, wird  ***/
                    /*** der Active-Status angeschaltet.            ***/
                    yw_HandleFreezeItem(ywd,i);
                    break;

                case SI_STATUS_TRIGGERED:
                    /*** Item wird getriggert, dies ist ein         ***/
                    /*** kontinuierlicher Prozess. Das Triggern     ***/
                    /*** kann unterbrochen werden, indem alle Key-  ***/
                    /*** sektoren inklusive Item dem ursprünglichen ***/
                    /*** Eigentümer weggenommen werden.             ***/
                    yw_HandleTriggeredItem(ywd,i);
                    break;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_RenderSuperItemStatus(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Zeigt den aktuellen Superitem-Status an. Muß innerhalb
**      RASTM_Begin2D/RASTM_End2D aufgerufen werden.
**
**  CHANGED
**      11-Feb-98   floh    created
**                          + Audio-Countdown-Status.
**      16-Feb-98   floh    + Name lokalisiert
**      03-Apr-98   floh    + Countdown sollte jetzt korrekt angezeigt
**                            werden
**      27-May-98   floh    + handelt jetzt auch das Multiplayer-Score
**                            Rendering ab.
*/
{
    ULONG i;
    UBYTE str_buf[1024];
    UBYTE *str = str_buf;
    WORD xpos,ypos;
    struct rast_text rt;

    xpos = (ywd->DspXRes*2)/3;
    new_font(str,FONTID_TRACY);
    pos_brel(str,xpos,EB.bar_height + (ywd->FontH>>1));
    
    /*** Ingame-Score rendern ***/    
    str = yw_RenderIngameScore(ywd,str,&(ywd->IngameStats),(ywd->DspXRes-xpos));

    /*** Superitem-Status ***/
    for (i=0; i<ywd->Level->NumItems; i++) {
        struct SuperItem *item = &(ywd->Level->Item[i]);
        if (item->type != SI_TYPE_NONE) {

            ULONG cix = YPACOLOR_OWNER_0 + item->activated_by;
            UBYTE line_buf[128];
            BOOL do_line = FALSE;
            UBYTE *name;

            /*** was isses denn? ***/
            switch(item->type) {
                case SI_TYPE_BOMB:
                    name = ypa_GetStr(ywd->LocHandle,STR_SUPERBOMB_NAME,"STOUDSON BOMB");
                    break;
                case SI_TYPE_WAVE:
                    name = ypa_GetStr(ywd->LocHandle,STR_SUPERWAVE_NAME,"STOUDSON WAVE");
                    break;
                default:
                    name = "SUPER ITEM";
                    break;
            };

            /*** Zustands-abhängige Behandlung ***/
            if ((item->status == SI_STATUS_ACTIVE) ||
                (item->status == SI_STATUS_FROZEN))
            {
                LONG sec,min;

                /*** Active oder Frozen, Countdown anzeigen ***/
                sec = ((item->countdown+1023)/1024);
                if (sec < 0) sec=0;
                min = sec / 60;
                sprintf(line_buf,"%s: %02d:%02d",name,min%60,sec%60);
                do_line = TRUE;

            }else if (item->status == SI_STATUS_TRIGGERED){
                sprintf(line_buf,"%s: TRIGGERED.",name);
                do_line = TRUE;
            };

            /*** Testausgabe zusammenbasteln ***/
            if (do_line) {
                xpos_brel(str,xpos);
                dbcs_color(str,yw_Red(ywd,cix),yw_Green(ywd,cix),yw_Blue(ywd,cix));
                str = yw_TextBuildClippedItem(ywd->Fonts[FONTID_TRACY],
                      str, line_buf, ywd->DspXRes-xpos, ' ');
                new_line(str);
            };
        };
    };

    /*** EOS und Text rendern ***/
    eos(str);
    rt.string = str_buf;
    rt.clips  = NULL;
    _methoda(ywd->GfxObject,RASTM_Text,&rt);
}

/*=================================================================**
**                                                                 **
**  SUPERITEM TYPSPEZIFISCHE HANDLER                               **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
void yw_SuperBombStartTrigger(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Wird einmalig aufgerufen, wenn Superbombe in den
**      Triggerzustand geschaltet wird.
**
**  CHANGED
**      11-Feb-98   floh    created
*/
{
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    item->last_radius  = 0;
}

/*-----------------------------------------------------------------*/
void yw_SuperBombVehicleOverkill(struct ypaworld_data *ywd,
                                 struct SuperItem *item,
                                 FLOAT mid_x, FLOAT mid_z,
                                 FLOAT radius)
/*
**  FUNCTION
**      Geht gesamten Level durch und killt alles, was
**      sich innerhalb des Radius befindet.
**
**  CHANGED
**      01-Mar-98   floh    created
*/
{
    ULONG num_sec = ywd->MapSizeX * ywd->MapSizeY;
    struct Cell *sec = ywd->CellArea;
    ULONG i;

    /*** für jeden Sektor... ***/
    for (i=0; i<num_sec; i++) {
        /*** für alle Bakterien im Sektor... ***/
        struct MinNode *nd;
        struct MinList *ls = (struct MinList *) &(sec->BactList);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            struct Bacterium *b = (struct Bacterium *)nd;
            BOOL do_vehicle = FALSE;
            if (ywd->playing_network) {
                /*** Multiplayer Test-Kriterium ***/
                if ((b->Owner == ywd->UVBact->Owner) &&
                    (b->Owner != item->activated_by) &&
                    (b->MainState != ACTION_DEAD))
                {
                    do_vehicle = TRUE;
                };
            } else {
                /*** Singleplayer Test-Kriterium ***/
                if ((b->Owner != item->activated_by) &&
                    (b->MainState != ACTION_DEAD))
                {
                    do_vehicle = TRUE;
                };
            };
            if (do_vehicle) {
                /*** Entfernung zum SuperItem ***/
                FLOAT dist;
                FLOAT dx,dz;
                dx = mid_x - b->pos.x;
                dz = mid_z - b->pos.z;
                dist = nc_sqrt(dx*dx + dz*dz);
                if (dist < radius) {
                    struct modvehicleenergy_msg mve;
                    mve.energy = -22000000;
                    mve.killer = NULL;
                    _methoda(b->BactObject,YBM_MODVEHICLEENERGY,&mve);
                };
            };
        };
        sec++;
    };
}

/*-----------------------------------------------------------------*/
void yw_SuperBombTrigger(struct ypaworld_data *ywd, ULONG item_num)
/*
**  FUNCTION
**      Wird für jeden Frame aufgerufen, wenn Superbombe
**      getriggert ist.
**
**  CHANGED
**      11-Feb-98   floh    created
**      16-Feb-98   floh    + modifiziert jetzt auch den Owner-Code
**                          + Behandlung für Netzwerk: Auf jeder Maschine
**                            werden nur die eigenen Vehikel gekillt,
**                            so paradox das auch klingen mag.
**      01-Mar-98   floh    + Fahrzeug-Vernichtund komplett neu, damit
**                            garantiert keine Fahrzeuge mehr durchrutschen
**                            können.
*/
{
    /*
    **  Die Superbombe erzeugt eine Druckwelle, die mit
    **  einer Geschwindigkeit von etwa 1 Sektor pro
    **  Sekunde über den Level rauscht und
    **  massiven Schaden anrichtet.
    */
    struct SuperItem *item = &(ywd->Level->Item[item_num]);
    LONG time_diff = ywd->TimeStamp - item->trigger_timestamp;
    FLOAT mid_x,mid_z;  // Zentrum der Explosion
    FLOAT max_x,max_z,max_r;

    mid_x  = (FLOAT) (item->sec_x * SECTOR_SIZE + SECTOR_SIZE/2);
    mid_z  = (FLOAT) -(item->sec_y * SECTOR_SIZE + SECTOR_SIZE/2);
    item->radius = (SECTOR_SIZE * time_diff)/2400;

    max_r = nc_sqrt(ywd->WorldSizeX*ywd->WorldSizeX + ywd->WorldSizeY*ywd->WorldSizeY);

    /*** ist es wieder an der Zeit? ***/
    if ((item->radius > 300) &&
        ((item->radius - item->last_radius) > 200) &&
        (item->radius < max_r))
    {
        /*** wieviele Punkte brauchen wir auf dem Radius, um ***/
        /*** jeden Subsektor zu berühren?                    ***/
        FLOAT u = 2*item->radius * 3.1415;
        FLOAT num_points = (u / 150.0);
        item->last_radius = item->radius;
        if (num_points > 2.0) {

            FLOAT dr = 2*3.1415 / num_points;
            FLOAT act_r = 0.0;

            /*** für jeden Punkt auf Radius... ***/
            for (act_r=0.0; act_r<(2*3.1415); act_r+=dr) {
                FLOAT act_x = mid_x + item->radius*cos(act_r);
                FLOAT act_z = mid_z + item->radius*sin(act_r);
                if ((act_x > 600.0) && (act_z < -600.0) &&
                    (act_x < (ywd->WorldSizeX-600.0))&&
                    (act_z > (-(ywd->WorldSizeY-600))))
                {
                    struct energymod_msg emm;
                    ULONG dest_fx_bu = ywd->NumDestFX;

                    /*** ModSectorEnergy auf jeden Punkt ***/
                    ywd->NumDestFX = 2;
                    emm.pnt.x  = act_x;
                    emm.pnt.y  = item->sec->Height;
                    emm.pnt.z  = act_z;
                    emm.energy = 200000;
                    emm.owner  = item->activated_by;
                    emm.hitman = NULL;
                    _methoda(ywd->world, YWM_MODSECTORENERGY, &emm);
                    ywd->NumDestFX = dest_fx_bu;
                };
            };
        };
    };

    /*** unabhängig davon in jedem Frame Vehicles innerhalb Radius killen ***/
    yw_SuperBombVehicleOverkill(ywd,item,mid_x,mid_z,(FLOAT)item->radius);
}

/*-----------------------------------------------------------------*/
BOOL yw_IsPntVisible(struct ypaworld_data *ywd, FLOAT x, FLOAT z)
/*
**  FUNCTION
**      Guckt, ob 3D-Punkt potentiell sichtbar ist.
**
**  CHANGED
**      12-Feb-98   floh    created
**      18-Feb-98   floh    + oops, am unten und rechten Rand
**                            trat wurde immer "nicht sichtbar"
**                            zurückgegeben
*/
{
    /*** ist dieser Punkt im sichtbaren Bereich? ***/
    LONG subx = GET_SUB(x);
    LONG suby = GET_SUB(-z);
    LONG secx = subx >> 2;
    LONG secy = suby >> 2;
    if ((secx > 0) && (secx < ywd->MapSizeX) &&
        (secy > 0) && (secy < ywd->MapSizeY))
    {
        if (ywd->Viewer) {
            LONG dx = ywd->Viewer->SectX - secx;
            LONG dy = ywd->Viewer->SectY - secy;
            dx = (dx < 0) ? -dx:dx;
            dy = (dy < 0) ? -dy:dy;
            if ((dx+dy) <= ((ywd->VisSectors-1)>>1)) return(TRUE);
        };
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_RenderSuperBombWall(struct ypaworld_data *ywd,
                            FLOAT x, FLOAT z,
                            FLOAT mid_x, FLOAT mid_z,
                            struct basepublish_msg *bpub_msg)
/*
**  FUNCTION
**      Rendert ein Element der Superbomben-Wand.
**
**  CHANGED
**      12-Feb-98   floh    created
*/
{
    if (ywd->SBombWallVProto) {
        if ((x>0.0) && (z<0.0) && (x<ywd->WorldSizeX) && (z>(-ywd->WorldSizeY)))
        {
            if (yw_IsPntVisible(ywd,x,z)) {

                struct VehicleProto *vp = &(ywd->VP_Array[ywd->SBombWallVProto]);
                struct VisProto *visp = &(ywd->VisProtos[vp->TypeNormal]);
                Object *vp_o = visp->o;
                struct tform *vp_tform = visp->tform;
                FLOAT y = 0.0;

                if (vp_o && vp_tform) {
                    /*** Y-Ermittlung ist haarig... ***/
                    LONG subx = GET_SUB(x);
                    LONG suby = GET_SUB(-z);
                    LONG secx = subx >> 2;
                    LONG secy = suby >> 2;
                    LONG inx  = (subx & 3);
                    LONG iny  = (suby & 3);
                    fp3d vec;
                    FLOAT l;

                    if ((inx==0)||(iny==0)) {
                        /*** ein Slurp -> mit Intersection Höhe ermitteln ***/
                        struct intersect_msg imsg;
                        imsg.vec.x = 0.0;
                        imsg.vec.y = 50000.0;
                        imsg.vec.z = 0.0;
                        imsg.pnt.x = x;
                        imsg.pnt.y = -25000.0;
                        imsg.pnt.z = z;
                        imsg.flags = 0;
                        _methoda(ywd->world,YWM_INTERSECT,&imsg);
                        if (imsg.insect) y = imsg.ipnt.y;
                    } else {
                        /*** ein Sektor, einfach Sektor-Höhe nehmen ***/
                        struct Cell *sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);
                        y = sec->Height;
                    };

                    /*** Positionieren und Rotieren ***/
                    vp_tform->loc.x = x;
                    vp_tform->loc.y = y;
                    vp_tform->loc.z = z;
                    vec.x = (x - mid_x);
                    vec.y = 0.0;
                    vec.z = (z - mid_z);
                    l = nc_sqrt(vec.x*vec.x + vec.z*vec.z);
                    if (l > 0.0) {
                        vec.x /= l;
                        vec.z /= l;
                    };
                    vp_tform->loc_m.m11 = vec.z;
                    vp_tform->loc_m.m12 = 0.0;
                    vp_tform->loc_m.m13 = -vec.x;
                    vp_tform->loc_m.m21 = 0.0;
                    vp_tform->loc_m.m22 = 1.0;
                    vp_tform->loc_m.m23 = 0.0;
                    vp_tform->loc_m.m31 = vec.x;
                    vp_tform->loc_m.m32 = vec.y;
                    vp_tform->loc_m.m33 = vec.z;
                    _methoda(vp_o,BSM_PUBLISH,bpub_msg);
                };
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_RenderSuperItems(struct ypaworld_data *ywd,
                         struct basepublish_msg *bpub_msg)
/*
**  FUNCTION
**      Übernimmt das Rendering der Spezial-Effekte bei den
**      Superitems. Wird aus yw_RenderFrame() aufgerufen.
**
**  CHANGED
**      12-Feb-98
*/
{
    ULONG i;
    for (i=0; i<ywd->Level->NumItems; i++) {
        struct SuperItem *item = &(ywd->Level->Item[i]);
        if (item->status == SI_STATUS_TRIGGERED) {
            FLOAT mid_x  = (FLOAT) (item->sec_x * SECTOR_SIZE + SECTOR_SIZE/2);
            FLOAT mid_z  = (FLOAT) -(item->sec_y * SECTOR_SIZE + SECTOR_SIZE/2);
            FLOAT max_r = 0.5*nc_sqrt(ywd->WorldSizeX*ywd->WorldSizeX + ywd->WorldSizeY*ywd->WorldSizeY);
            if ((item->radius > 300) && (item->radius < max_r))
            {
                /*** wieviele Punkte brauchen wir auf dem Radius? ***/
                FLOAT u = 2*item->radius * 3.1415;
                FLOAT num_points = (u / 300.0); // alle 300 Koords einen
                if (num_points > 2.0) {
                    FLOAT dr = 2*3.1415 / num_points;
                    FLOAT act_r = 0.0;

                    /*** für jeden Punkt auf Radius... ***/
                    for (act_r=0.0; act_r<(2*3.1415); act_r+=dr) {
                        FLOAT act_x = mid_x + item->radius*cos(act_r);
                        FLOAT act_z = mid_z + item->radius*sin(act_r);
                        yw_RenderSuperBombWall(ywd,act_x,act_z,mid_x,mid_z,bpub_msg);
                    };
                };
            };
        };
    };
}

