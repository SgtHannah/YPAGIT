/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_history.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 16:21:09 $
**  $Locker:  $
**  $Author: floh $
**
**  yw_history.c -- History-Event-Recorder für Debriefing.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

struct __score_qsort_struct {
    LONG owner;
    LONG score;
};

/*-----------------------------------------------------------------*/
_extern_use_nucleus

/*-----------------------------------------------------------------*/
void yw_KillHistoryBuffer(struct HistoryBuffer *hbuf)
/*
**  FUNCTION
**      Entfernt einen HistoryBuffer aus der Puffer-Liste
**      und gibt ihn komplett frei.
**
**  CHANGED
**      25-Aug-97   floh    created
*/
{
    _Remove((struct Node *)hbuf);
    _FreeVec(hbuf->inst_begin);
    _FreeVec(hbuf);
}

/*-----------------------------------------------------------------*/
BOOL yw_NewHistoryBuffer(struct HistoryHeader *hhead, ULONG size)
/*
**  FUNCTION
**      Erzeugt und initialisiert einen neuen HistoryBuffer,
**      hängt diesen dann an die History-Buffer-Liste des
**      HistoryHeader, füllt die Pointer <inst_ptr> und
**      <endinst_ptr> in <hhead> aus und kehrt bei Erfolg
**      mit TRUE zurück.
**
**  CHANGED
**      25-Aug-97   floh    created
*/
{
    BOOL success = FALSE;
    struct HistoryBuffer *hbuf;
    hbuf = _AllocVec(sizeof(struct HistoryBuffer),MEMF_PUBLIC|MEMF_CLEAR);
    if (hbuf) {
        hbuf->inst_begin = _AllocVec(size,MEMF_PUBLIC|MEMF_CLEAR);
        if (hbuf->inst_begin) {
            hbuf->inst_end = hbuf->inst_begin + size;
            _AddTail((struct List *)&(hhead->ls),(struct Node *)hbuf);
            hhead->inst_ptr    = hbuf->inst_begin;
            hhead->endinst_ptr = hbuf->inst_end;
            hhead->num_bufs++;
            success = TRUE;
        } else {
            _FreeVec(hbuf);
        };
    };
    return(success);
}

/*-----------------------------------------------------------------*/
void yw_KillHistory(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt die History und alle angehängten Event-Puffer.
**
**  CHANGED
**      25-Aug-97   floh    created
*/
{
    if (ywd->History) {
        while (ywd->History->ls.mlh_Head->mln_Succ) {
            struct HistoryBuffer *hbuf = (struct HistoryBuffer *)
                                         ywd->History->ls.mlh_Head;
            yw_KillHistoryBuffer(hbuf);
        };
        _FreeVec(ywd->History);
        ywd->History = NULL;
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_InitHistory(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert ywd->History. Die History MUSS vorher
**      deinitialisiert gewesen sein, sonst gibt's
**      Speicherlecks.
**      Danach kann die History per YWM_NOTIFYHISTORYEVENT
**      gefüllt werden.
**
**  CHANGED
**      25-Aug-97   floh    created
*/
{
    if (ywd->History) yw_KillHistory(ywd);
    ywd->History = _AllocVec(sizeof(struct HistoryHeader),MEMF_PUBLIC|MEMF_CLEAR);
    if (ywd->History) {
        memset(ywd->History,0,sizeof(ywd->History));
        _NewList((struct List *)&(ywd->History->ls));
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_AddHistoryEvent(struct ypaworld_data *ywd,
                        struct HistoryHeader *hhead,
                        UBYTE *event)
/*
**  FUNCTION
**      Hängt ein neues Event in die History. Das erste Byte
**      der übergebenen Event-Struktur muß den Typ enthalten,
**      daraus leitet die Struktur die Länge des Events ab (der
**      Event-Typ muß der Routine also bekannt sein, unbekannte
**      werden ignoriert!
**      Allokiert bei Bedarf einen neuen EventPuffer, löscht bei einem
**      NEWFRAME den letzten Frame, falls dieser keine Events
**      enthielt.
**
**  CHANGED
**      25-Aug-97   floh    created
**      26-May-98   floh    + Scoring
*/
{
    UBYTE type = *event;
    ULONG size;

    /*** ist das der allererste Aufruf??? ***/
    if (NULL == hhead->inst_ptr) {
        /*** dann erstmal einen Eventpuffer allokieren ***/
        if (!yw_NewHistoryBuffer(hhead,YPAHIST_BUFSIZE)) return(FALSE);
    };

    /*** Event-Type auswerten ***/
    switch(type) {
        case YPAHIST_NEWFRAME:
            size = sizeof(struct ypa_HistNewFrame);

            /*** Special Case NewFrame ***/
            hhead->num_frames++;
            hhead->time_stamp = ywd->TimeStamp;

            /*** letztes Event auch schon ein NewFrame? ***/
            if (hhead->previnst_ptr &&
                (YPAHIST_NEWFRAME == *hhead->previnst_ptr))
            {
                /*** dann ist im letzten Frame nix passiert, ***/
                /*** also letzte Instruktion überschreiben   ***/
                hhead->inst_ptr = hhead->previnst_ptr;
            };
            break;
        case YPAHIST_CONSEC:       size=sizeof(struct ypa_HistConSec);      break;
        case YPAHIST_VHCLKILL:     size=sizeof(struct ypa_HistVhclKill);    break;
        case YPAHIST_VHCLCREATE:   size=sizeof(struct ypa_HistVhclCreate);  break;
        case YPAHIST_SQUADPOS:     size=sizeof(struct ypa_HistSquadPos);    break;
        case YPAHIST_POWERSTATION: size=sizeof(struct ypa_HistConSec);      break;
        case YPAHIST_TECHUPGRADE:  size=sizeof(struct ypa_HistTechUpgrade); break;
        default:                   size=0;
    };
    if (size > 0) {
        /*** neue Instruktion übernehmen ***/
        hhead->previnst_ptr = hhead->inst_ptr;
        if ((hhead->inst_ptr + size + 1) >= hhead->endinst_ptr) {
            /*** Overflow im aktuellen EventPuffer! ***/
            if (!yw_NewHistoryBuffer(hhead,YPAHIST_BUFSIZE)) return(FALSE);
        };
        memcpy(hhead->inst_ptr,event,size);
        hhead->inst_ptr += size;
    };
    
    /*** Ingame Scoring ***/
    yw_Score(ywd, event, &(ywd->IngameStats));

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_NOTIFYHISTORYEVENT, UBYTE *msg)
/*
**  CHANGED
**      25-Aug-97   floh    created
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    if (ywd->History) {
        yw_AddHistoryEvent(ywd,ywd->History,msg);
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadHistoryBuffer(struct HistoryHeader *hhead, APTR fp)
/*
**  FUNCTION
**      Erzeugt einen HistoryBuffer mit yw_NewHistoryBuffer()
**      und füllt diesen mit den Daten aus dem ASCII-Stream.
**      Der Stream muß gestoppt sein NACH "history_buffer".
**
**  CHANGED
**      10-Sep-97   floh    created
**      19-May-98   floh    + TechUpgrades neu
*/
{
    ULONG num_cols,num_lines;
    UBYTE line[1024];
    UBYTE *ptr;
    BOOL retval = FALSE;

    /*** lese Anzahl Spalten und Zeilen ***/
    _FGetS(line,sizeof(line),fp);
    ptr       = strtok(line," \n");
    num_cols  = strtol(ptr,NULL,0);
    ptr       = strtok(NULL," \n");
    num_lines = strtol(ptr,NULL,0);

    /*** erzeuge und lese HistoryBuffer ***/
    if (yw_NewHistoryBuffer(hhead,num_cols*num_lines)) {
        UBYTE *inst_ptr = hhead->inst_ptr;
        UBYTE cmd;
        ULONG x,y,size;
        for (y=0; y<num_lines; y++) {
            /*** neue Zeile lesen ***/
            UBYTE *first_hit;
            _FGetS(line,sizeof(line),fp);
            first_hit = line;
            for (x=0; x<num_cols; x++) {
                ptr = strtok(first_hit," \n");
                if (first_hit) first_hit=NULL;
                *inst_ptr++ = (UBYTE) strtol(ptr,NULL,16);
            };
        };

        /*** setze den ActInstr-Pointer auf die erste Invalid-Adresse ***/
        inst_ptr = hhead->inst_ptr;
        while (cmd = *inst_ptr) {
            switch(cmd) {
                case YPAHIST_NEWFRAME:
                    size = sizeof(struct ypa_HistNewFrame); break;
                case YPAHIST_CONSEC:
                case YPAHIST_POWERSTATION:
                    size = sizeof(struct ypa_HistConSec); break;
                case YPAHIST_TECHUPGRADE:
                    size = sizeof(struct ypa_HistTechUpgrade); break;
                case YPAHIST_VHCLKILL:
                    size = sizeof(struct ypa_HistVhclKill); break;
                case YPAHIST_VHCLCREATE:
                    size = sizeof(struct ypa_HistVhclCreate); break;
                default:
                    /*** unbekannte Instruktion, Abbruch ***/
                    size=0; *inst_ptr=0;
            };
            inst_ptr += size;
        };
        hhead->inst_ptr     = inst_ptr;
        hhead->previnst_ptr = NULL;
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
ULONG yw_SaveHistoryBuffer(struct HistoryBuffer *hbuf, APTR fp)
/*
**  FUNCTION
**      Saved einen History-Buffer in einen ASCII-Stream als
**      Riesen-Binär-Blob.
**
**  CHANGED
**      10-Sep-97   floh    created
*/
{
    ULONG x,y,num_lines,num_cols;
    UBYTE *ptr;
    num_cols  = 64;
    num_lines = (hbuf->inst_end-hbuf->inst_begin) / num_cols;
    ptr       = hbuf->inst_begin;
    fprintf(fp,"    history_buffer = \n");
    fprintf(fp,"    %d %d\n",num_cols,num_lines);
    for (y=0; y<num_lines; y++) {
        fprintf(fp,"    ");
        for (x=0; x<num_cols; x++) {
            UBYTE p = *ptr++;
            fprintf(fp,"%02x ",p);
        };
        fprintf(fp,"\n");
    };
    fprintf(fp,"\n");
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_SaveHistory(struct ypaworld_data *ywd, APTR fp)
/*
**  FUNCTION
**      Saved den gesamten Historybuffer in einen ASCII-Stream.
**
**  CHANGED
**      10-Sep-97   floh    created
*/
{
    if (ywd->History) {
        struct MinList *ls;
        struct MinNode *nd;
        fprintf(fp,";------------------------------------------------------------\n");
        fprintf(fp,"; History Buffers\n");
        fprintf(fp,";------------------------------------------------------------\n");
        fprintf(fp,"begin_history\n");
        ls = &(ywd->History->ls);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            struct HistoryBuffer *hbuf = (struct HistoryBuffer *) nd;
            yw_SaveHistoryBuffer(hbuf,fp);
        };
        fprintf(fp,"end");
    };
    return(TRUE);
}

/*-----------------------------------------------------------------*/
ULONG yw_HistoryParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst den "begin_history" Block in Savegames.
**      Erwartet <struct ypaworld_data *ywd> in <p->target>.
**
**  CHANGED
**      10-Sep-97   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct ypaworld_data *ywd = (struct ypaworld_data *) p->target;

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Context ***/
        if (stricmp(kw, "begin_history")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0) {
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);
        } else if (stricmp(kw,"history_buffer")==0) {
            if (ywd->History) {
                yw_LoadHistoryBuffer(ywd->History,p->fp);
            };
        } else return(PARSE_UNKNOWN_KEYWORD);

        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
int __score_qsort_hook(struct __score_qsort_struct *elm1,
                       struct __score_qsort_struct *elm2)
{
    return(elm2->score-elm1->score);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RenderIngameScore(struct ypaworld_data *ywd,
                            UBYTE *str,
                            struct ypa_PlayerStats *stats,
                            ULONG w)
/*
**  CHANGED
**      26-May-98   floh    created
*/
{
    if (ywd->playing_network) {
    
        struct __score_qsort_struct score_array[MAXNUM_OWNERS];
        ULONG i,num_owners;    

        /*** initialisiere und sortiere Owner-Array ***/    
        num_owners=0;
        for (i=0; i<MAXNUM_OWNERS; i++) {
            if (ywd->Level->OwnerMask & (1<<i)) {
                score_array[num_owners].owner=i;
                score_array[num_owners].score=stats[i].Score;
                num_owners++;
            };
        };
        qsort(score_array,num_owners,sizeof(struct __score_qsort_struct),__score_qsort_hook);
        
        /*** layoute jede Zeile ***/
        for (i=0; i<num_owners; i++) {
            UBYTE *name;
            ULONG c_index;
            struct ypa_ColumnItem col[2];
            UBYTE buf_0[32];
            switch(score_array[i].owner) {
                case 1:
                    c_index = YPACOLOR_OWNER_1;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_RESISTANCE,"RESISTANCE");
                    break;
                case 2:
                    c_index = YPACOLOR_OWNER_2;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_SULG,"SULGOGARS");
                    break;
                case 3:
                    c_index = YPACOLOR_OWNER_3;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_MYKO,"MYKONIANS");
                    break;
                case 4:
                    c_index = YPACOLOR_OWNER_4;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_TAER,"TAERKASTEN");
                    break;
                case 5:
                    c_index = YPACOLOR_OWNER_5;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_BLACK,"BLACK SECT");
                    break;
                case 6:
                    c_index = YPACOLOR_OWNER_6;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_KYT,"GHORKOV");
                    break;
                default:  
                    c_index = YPACOLOR_OWNER_7;
                    name = ypa_GetStr(ywd->LocHandle,STR_RACE_NEUTRAL,"NEUTRAL");
                    break;
            };
            dbcs_color(str,yw_Red(ywd,c_index),yw_Green(ywd,c_index),yw_Blue(ywd,c_index));    
            
            /*** Column-Layout initialisieren ***/        
            col[0].string       = name;
            col[0].width        = w * 0.5;
            col[0].font_id      = FONTID_TRACY;
            col[0].space_chr    = ' ';
            col[0].prefix_chr   = 0;
            col[0].postfix_chr  = 0;
            col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
             
            sprintf(buf_0,"%d",stats[score_array[i].owner].Score);         
            col[1].string       = buf_0;
            col[1].width        = w * 0.5;
            col[1].font_id      = FONTID_TRACY;
            col[1].space_chr    = ' ';
            col[1].prefix_chr   = 0;
            col[1].postfix_chr  = 0;
            col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
            str = yw_BuildColumnItem(ywd,str,2,col);
            new_line(str);
        };
        new_line(str);
    };
    return(str);
}

