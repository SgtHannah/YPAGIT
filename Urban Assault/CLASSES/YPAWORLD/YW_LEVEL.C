/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_level.c,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 16:22:16 $
**  $Locker: floh $
**  $Author: floh $
**
**  yw_level.c -- Vehicle-Beamen, Level-Finish-Struktur
**  verwalten, Fahrzeug-Mitnahme über mehrere Level etc.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "nucleus/math.h"
#include "visualstuff/ov_engine.h"
#include "bitmap/rasterclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagameshell.h"
#include "ypa/ypacatch.h"

#include "yw_protos.h"
#include "yw_gsprotos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_audio_engine

//
// interne Protos
// ~~~~~~~~~~~~~~
BOOL yw_ParseWorldIni(struct ypaworld_data *,UBYTE *);

/*-----------------------------------------------------------------*/
void yw_KillLevelNet(struct ypaworld_data *ywd)
/*
**  CHANGED
**      07-Aug-96   floh    created
*/
{
    if (ywd->LevelNet) _FreeVec(ywd->LevelNet);
    if (ywd->Level)    _FreeVec(ywd->Level);
    ywd->LevelNet = NULL;
    ywd->Level    = NULL;
}

/*-----------------------------------------------------------------*/
BOOL yw_InitLevelNet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Allokiert ywd->LevelInfo.
**
**  CHANGED
**      07-Aug-96   floh    created
**      29-Aug-96   floh    + ywd->LevelNet-Initialisierung
**                          + yw_ParseWorldIni()
**      16-Oct-96   floh    + initialisiert für alle Level die
**                            Default-Leveldescription-Filenamen.
**      17-Jan-97   floh    + Cleanup
**      09-Dec-97   floh    + yw_InitColors()
**      26-Jan-98   floh    + yw_ScanLevels(), alle Informationen über
**                            die Levels werden jetzt einmalig am
**                            Anfang geparst...
**      28-Feb-98   floh    + initialisiert ywd->Level->MaxNumBuddies
**      01-Mar-98   floh    + ywd->Level->RaceEncountered[] Array wird
**                            gelöscht
**      24-Mar-98   floh    + Datenset umgebaut
*/
{
    BOOL retval = FALSE;
    ywd->Level = _AllocVec(sizeof(struct LevelInfo),MEMF_PUBLIC|MEMF_CLEAR);
    if (ywd->Level) {
        ywd->LevelNet = _AllocVec(sizeof(struct WorldInfo),MEMF_PUBLIC|MEMF_CLEAR);
        if (ywd->LevelNet) {
            ULONG i;
            yw_InitColors(ywd);
            if (!yw_ParseWorldIni(ywd,"data:world.ini")) {
                _LogMsg("yw_ParseWorldIni() failed.\n");
                yw_KillLevelNet(ywd);
                return(FALSE);
            };
            if (!yw_ScanLevels(ywd)) {
                _LogMsg("yw_ScanLevels() failed.\n");
                yw_KillLevelNet(ywd);
                return(FALSE);
            };
            ywd->Level->MaxNumBuddies = MAXNUM_STARTBUDDIES;
            memset(&(ywd->Level->RaceTouched),0,sizeof(ywd->Level->RaceTouched));
            retval = TRUE;
        };
    };
    if (!retval) yw_KillLevelNet(ywd);
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_DoLevelStatus(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erledigt Level-Status-spezifische Cleanup-
**      Sachen, wenn ein Level beendet wird. Darf
**      nur aus YWM_KILLLEVEL heraus aufgerufen
**      werden.
**
**  CHANGED
**      02-Oct-96   floh    created
**      02-Aug-97   floh    + schaltet Target-Level nur noch auf enabled,
**                            wenn sie momentan disabled sind (damit ihr
**                            Finished-Zustand nicht überschrieben wird)
**      23-Sep-97   floh    + yw_RestoreVehicleData() bei Abbruch
**                            macht die Wundersteine wieder ungültig
**      01-Jun-98   floh    + falls der Level eine Eventloop hatte,
**                            wird er nachtraeglich als ABORTED
**                            gekennzeichnet, auch wenn er gewonnen
**                            wurde. Die Target-Level werden aber
**                            trotzdem aufgeschaltet...
**      17-Jun-98   floh    + ... und wieder raus.
*/
{
    switch(ywd->Level->Status) {

        case LEVELSTAT_ABORTED:
            if (!yw_RestoreVehicleData(ywd)) {
                _LogMsg("yw_RestoreVehicleData() failed.\n");
            };
            break;

        case LEVELSTAT_FINISHED:
            /*** regulär via Beamgate beendet ***/
            {
                ULONG i;
                struct LevelNode *l = &(ywd->LevelNet->Levels[0]);
                struct Gate *g = &(ywd->Level->Gate[ywd->Level->BeamGate]);
                
                /*** aktuellen Level als Finished markieren ***/
                l[ywd->Level->Num].status = LNSTAT_FINISHED;

                /*** Target-Levels aufschließen ***/
                for (i=0; i<g->num_targets; i++) {
                    if (LNSTAT_DISABLED == l[g->targets[i]].status) {
                        l[g->targets[i]].status = LNSTAT_ENABLED;
                    };
                };
            };
            break;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, yw_YWM_BEAMNOTIFY, struct beamnotify_msg *msg)
/*
**  CHANGED
**      07-Aug-96   floh    created
**      12-Dec-97   floh    Build-Batterie raus    
**      28-Feb-98   floh    + returniert jetzt TRUE, wenn noch
**                            Fahrzeuge ins Beamgate passen, sonst
**                            FALSE
**                          + oops... ich habe getestet auf
**                            "falls b eine Gun, oder b eine Gun...",
**                            ich schmeiß jetzt erstmal auch Raketen
**                            raus...
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct Bacterium *b = msg->b;
    ULONG gate_num;
    ULONG retval = TRUE;

    /*** Abbruch-Bedingungen ***/
    if ((b->BactClassID==BCLID_YPAGUN)||(b->BactClassID==BCLID_YPAMISSY)) return(TRUE);
    if (b->Owner != ywd->URBact->Owner) return(TRUE);
    if (b->Sector->WType == WTYPE_OpenedGate) gate_num=b->Sector->WIndex;
    else return(TRUE);

    if (ywd->URBact == msg->b) {

        /*** der User-Robo begehrt Einlass! ***/
        ywd->Level->Status   = LEVELSTAT_FINISHED;
        ywd->Level->BeamGate = gate_num;
        ywd->Level->RSysBatt = b->Energy;
        _get(b->BactObject, YRA_BattVehicle, &(ywd->Level->RVhclBatt));
        _get(b->BactObject, YRA_BattBeam, &(ywd->Level->RVhclBatt));

    } else {

        /*** ein ordinäres Vehikel... ***/
        if ((ywd->Level->NumBuddies < MAXNUM_BUDDIES) &&
            (ywd->Level->NumBuddies < ywd->Level->MaxNumBuddies))
        {
            struct BuddyInfo *bi = &(ywd->Level->Buddies[ywd->Level->NumBuddies]);
            bi->CommandID = b->CommandID;
            bi->TypeID    = b->TypeID;
            bi->Energy    = b->Energy;
            ywd->Level->NumBuddies++;
        }else{
            retval = FALSE;
        };
    };

    /*** Ende ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_CreateBuddies(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Erzeugt die mitgenommenen Buddies in einem neuen
**      Level. ywd->Level muß die Informationen aus dem
**      letzten Level enthalten, und ywd->UserRobo muß
**      bereits gültig sein. Es werden nur die
**      Buddies erzeugt, die den Level aus demselben
**      Beamgate verlassen haben, wie der Robo.
**
**      Nachdem alles fachgerecht erledigt wurde,
**      wird die LevelInfo-Struktur geleert (jedenfalls
**      die Buddy-Sachen).
**
**  CHANGED
**      07-Aug-96   floh    created
**      28-Feb-98   floh    + Entfernung der erzeugten Buddies von
**                            300 auf 500 gesetzt, außerdem Höhe
**                            von 200 auf 100 runtergesetzt
*/
{
    ULONG act_squad;
    LONG act_cmdid;
    ULONG i;

    /*** überhaupt Buddies mitgenommen? ***/
    if (ywd->Level->NumBuddies == 0) return;

    /*** für jedes potentielle Geschwader... ***/
    act_squad = 0;
    do {

        ULONG buddy_array[MAXNUM_BUDDIES];
        struct externmakecommand_msg emc;
        memset(&emc,0,sizeof(emc));
        emc.flags  = EMC_EVER;
        emc.varray = (ULONG *) &buddy_array;

        /*** Mitglieder für ein neues Geschwader sammeln ***/
        act_cmdid = -1;
        for (i=0; i<ywd->Level->NumBuddies; i++) {

            struct BuddyInfo *buddy = &(ywd->Level->Buddies[i]);

            /*** bereits erledigte Buddies ignorieren ***/
            if (buddy->Created == 0) {

                /*** 1.Buddy -> neuer Anführer ***/
                if (act_cmdid == -1) {
                    /*** neues Geschwader anfangen ***/
                    act_cmdid  = buddy->CommandID;
                    emc.vproto = buddy->TypeID;
                    emc.varray[emc.number++] = buddy->TypeID;
                    buddy->Created = 1;
                } else {
                    /*** neuer Slave für aktuellen Geschaders? ***/
                    if (act_cmdid == buddy->CommandID) {
                        /*** yo ***/
                        emc.varray[emc.number++] = buddy->TypeID;
                        buddy->Created = 1;
                    };
                };
            };
        };

        /*** ein neues Geschwader erzeugen? ***/
        if (act_cmdid != -1) {

            /*** Position berechnen (kreisförmig um Robo rum) ***/
            struct intersect_msg im;
            FLOAT rad = act_squad++ * 1.745;  // ca. 100 Grad versetzt

            emc.pos.x = sin(rad)*500.0 + ywd->URBact->pos.x;
            emc.pos.y = ywd->URBact->pos.y;
            emc.pos.z = cos(rad)*500.0 + ywd->URBact->pos.z;

            /*** Intersection wegen Höhe ***/
            im.pnt.x = emc.pos.x + 0.5;
            im.pnt.y = -50000.0;
            im.pnt.z = emc.pos.z + 0.75;
            im.vec.x = 0.0;
            im.vec.y = 100000.0;
            im.vec.z = 0.0;
            im.flags = 0;
            _methoda(ywd->world, YWM_INTERSECT, &im);
            if (im.insect) emc.pos.y = im.ipnt.y - 100.0;

            /*** und Squad erzeugen... ***/
            _methoda(ywd->UserRobo, YRM_EXTERNMAKECOMMAND, &emc);
        };

    } while (act_cmdid != -1);

    /*** Alle "Created-Flags" rücksetzen ***/
    for( i = 0; i < ywd->Level->NumBuddies; i++ )
        ywd->Level->Buddies[ i ].Created = 0;

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_InitSquads(struct ypaworld_data *ywd,
                   ULONG num_squads,
                   struct NLSquadDesc *squads)
/*
**  FUNCTION
**      Erzeugt die Squads, die bereits zu Level-Beginn
**      aktiv sein sollen.
**
**  INPUTS
**      world
**      ywd
**      num_squads  - Anzahl Squads halt
**      squads      - Pointer auf initialisiertes Squad-Array
**
**  CHANGED
**      25-May-96   floh    created
**      08-Jul-96   floh    if (ywd->LevelMode == LEVELMODE_SEQPLAYER)
**                          werden keine Squads initialisiert!
**      07-Aug-96   floh    übernommen nach yw_level.c
**      09-Aug-96   floh    + YRM_EXTERNMAKECOMMAND-Msg angepaßt
**      28-Feb-98   floh    + Höhe über Boden auf 50.0 runtergesetzt
*/
{
    if (LEVELMODE_SEQPLAYER != ywd->Level->Mode) {

        ULONG act;
        for (act=0; act<num_squads; act++) {

            struct NLSquadDesc *s = &(squads[act]);

            if (s->active) {

                struct MinList *ls;
                struct MinNode *nd;
                Object *host_robo = NULL;

                /*** suche einen Hostrobo, der das Squad verwaltet ***/
                ls = &(ywd->CmdList);
                for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                    struct OBNode *obnd = (struct OBNode *)nd;

                    Object *r = obnd->o;
                    struct Bacterium *rbact = obnd->bact;

                    if ((BCLID_YPAROBO == rbact->BactClassID) &&
                        (s->owner      == rbact->Owner))
                    {
                        /*** passenden Robo gefunden ***/
                        host_robo = r;
                        break;
                    };
                };

                if (host_robo) {

                    struct intersect_msg im;
                    struct externmakecommand_msg xmm;

                    /*** Msg zum Erzeugen des Squads ausfüllen ***/
                    xmm.vproto = s->vproto;
                    xmm.number = s->num;
                    xmm.varray = NULL;
                    if (s->useable) xmm.flags = EMC_EVER;
                    else            xmm.flags = EMC_EVER|EMC_UNUSABLE;

                    /*** erstmal eine günstige Position finden... ***/
                    im.pnt.x = s->pos.x;
                    im.pnt.y = -50000.0;
                    im.pnt.z = s->pos.z;
                    im.vec.x = 0.0;
                    im.vec.y = 100000.0;
                    im.vec.z = 0.0;
                    im.flags = 0;
                    _methoda(ywd->world, YWM_INTERSECT, &im);

                    /*** günstigen Punkt bestimmen... ***/
                    if (im.insect) {
                        xmm.pos.x = im.ipnt.x;
                        xmm.pos.y = im.ipnt.y - 50.0;
                        xmm.pos.z = im.ipnt.z;
                    } else {

                        struct getsectorinfo_msg gsm;
                        gsm.abspos_x = s->pos.x;
                        gsm.abspos_z = s->pos.z;
                        if (_methoda(ywd->world, YWM_GETSECTORINFO, &gsm)) {
                            xmm.pos.x = s->pos.x;
                            xmm.pos.y = gsm.sector->Height;
                            xmm.pos.z = s->pos.z;
                        } else {
                            /*** POSITIONS-FEHLER ***/
                            _LogMsg("yw_InitSquads(): no valid position for squad[%d]!\n",act);
                            return;
                        };
                    };

                    /*** und los... ***/
                    _methoda(host_robo, YRM_EXTERNMAKECOMMAND, &xmm);

                } else {
                    _LogMsg("WARNING: yw_InitSquads(): no host robo for squad[%d], owner %d!\n",act,s->owner);
                };
            };
        };
    };
    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_InitGates(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert die (vom Level-Description-File-Parser
**      vorinitialisierten) Beamgates vollständig.
**
**  CHANGED
**      20-Jun-96   floh    created
**      07-Aug-96   floh    revised & updated
*/
{
    ULONG i;

    for (i=0; i<ywd->Level->NumGates; i++) {

        ULONG j;
        struct Gate *g = &(ywd->Level->Gate[i]);
        struct createbuilding_msg cvm;

        /*** Sector-Pointer ***/
        g->sec = &(ywd->CellArea[g->sec_y * ywd->MapSizeX + g->sec_x]);

        /*** baue Closed Build Proto ***/
        cvm.job_id    = g->sec->Owner;
        cvm.owner     = g->sec->Owner;
        cvm.bp        = g->closed_bp;
        cvm.immediate = TRUE;
        cvm.sec_x     = g->sec_x;
        cvm.sec_y     = g->sec_y;
        cvm.flags     = 0;
        _methoda(ywd->world, YWM_CREATEBUILDING, &cvm);

        /*** Gate in Sektor patchen ***/
        g->sec->WType  = WTYPE_ClosedGate;
        g->sec->WIndex = i;

        /*** Pointer der Keysektoren ***/
        for (j=0; j<g->num_keysecs; j++) {
            ULONG x = g->keysec[j].sec_x;
            ULONG y = g->keysec[j].sec_y;
            if ((x > 0) && (x < ywd->MapSizeX-1) &&
                (y > 0) && (y < ywd->MapSizeY-1))
            {
                g->keysec[j].sec = &(ywd->CellArea[y * ywd->MapSizeX + x]);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_CountVehiclesInSector(struct ypaworld_data *ywd, 
                               struct Cell *sec)
/*
**  FUNCTION
**      Zaehlt alle gueltigen Vehikel im Sektor, fuer den
**      Beamgate-Full-Test.
**
**  CHANGED
**      13-Jun-98   floh    created
*/
{
    struct MinList *ls;
    struct MinNode *nd;
    ULONG b_count = 0;
    ls = &(sec->BactList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct Bacterium *b = (struct Bacterium *)nd;
        if ((ACTION_DEAD    != b->MainState)   &&
            (ACTION_BEAM    != b->MainState)   &&
            (BCLID_YPAROBO  != b->BactClassID) &&
            (BCLID_YPAMISSY != b->BactClassID) &&
            (BCLID_YPAGUN   != b->BactClassID))
        {
            b_count++;
        };
    };
    return(b_count);
}    

/*-----------------------------------------------------------------*/
void yw_BeamGateCheck(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Testet alle BeamGates auf Open/Close. Wird einmal
**      pro Frame ausgeführt.
**
**  CHANGED
**      07-Aug-96   floh    created
**      09-Aug-96   floh    vergessen, nach dem Bauen den
**                          WType und WIndex zu checken, dadurch
**                          hat der BeamGate-Check immer getriggert...
**      26-Sep-97   floh    + Pri's für Beamgate LogMsgs eingestellt.
**      01-Mar-98   floh    + testet für jedes offene Beamgate, ob
**                            sich mehr als MaxNumBuddies drauf
**                            befinden, und gibt in diesem Fall eine
**                            Warnung aus.
**      11-Jun-98   floh    + Beamgate Open wird jetzt zyklisch ausgegeben,
**                            solange noch nicht full
*/
{
    ULONG i;

    /*** teste Keysektoren für jedes Gate ***/
    for (i=0; i<ywd->Level->NumGates; i++) {

        struct Gate *g = &(ywd->Level->Gate[i]);
        ULONG status = WTYPE_OpenedGate;
        ULONG j;

        if (g->sec->Owner != ywd->URBact->Owner) status = WTYPE_ClosedGate;
        else for (j=0; j<g->num_keysecs; j++) {
            if (g->keysec[j].sec) {
                if (g->keysec[j].sec->Owner != ywd->URBact->Owner) {
                    status = WTYPE_ClosedGate;
                    break;
                };
            };
        };

        /*** hat sich Status des aktuellen Beamgates verändert? ***/
        if (status != g->sec->WType) {

            struct createbuilding_msg cbm;

            /*** Gate-Zustand wurde geändert! ***/
            cbm.job_id    = g->sec->Owner;
            cbm.owner     = g->sec->Owner;
            cbm.immediate = TRUE;
            cbm.sec_x     = g->sec_x;
            cbm.sec_y     = g->sec_y;
            cbm.flags     = 0;

            if (status == WTYPE_OpenedGate){
                struct logmsg_msg lm;
                cbm.bp  = g->opened_bp;
            }else{
                struct logmsg_msg lm;
                cbm.bp  = g->closed_bp;
                lm.bact = NULL;
                lm.pri  = 65;
                lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_GATECLOSED,"TRANSPORTER GATE CLOSED!");
                lm.code = LOGMSG_GATECLOSED;
                _methoda(ywd->world, YWM_LOGMSG, &lm);
            };
            _methoda(ywd->world, YWM_CREATEBUILDING, &cbm);

            /*** Gate-Sektor neu patchen ***/
            g->sec->WType  = status;
            g->sec->WIndex = i;
        };

        /*** MaxNumBuddies Test ***/
        if (WTYPE_OpenedGate == status) {
            /*** teste alle Vehikel im Sektor ***/
            ULONG b_count = yw_CountVehiclesInSector(ywd,g->sec);
            if (b_count >= ywd->Level->MaxNumBuddies) {
                /*** Grenze ist überschritten ***/
                LONG td = ywd->TimeStamp - ywd->GateFullMsgTimeStamp;
                if (td > 20000) {
                    struct logmsg_msg lm;
                    lm.bact = NULL;
                    lm.pri  = 10;
                    lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_BEAMGATE_FULL,"WARNING: BEAM GATE FULL!");
                    lm.code = LOGMSG_BEAMGATE_FULL;
                    _methoda(ywd->world,YWM_LOGMSG,&lm);
                    ywd->GateFullMsgTimeStamp = ywd->TimeStamp;
                };
            } else {
                LONG td = ywd->TimeStamp - ywd->GateFullMsgTimeStamp;
                if (td > 40000) {
                    struct logmsg_msg lm;
                    lm.bact = NULL;
                    lm.pri  = 49;
                    lm.msg  = ypa_GetStr(ywd->LocHandle,STR_LMSG_GATEOPENED,"TRANSPORTER GATE OPENED!");
                    lm.code = LOGMSG_GATEOPENED;
                    _methoda(ywd->world,YWM_LOGMSG,&lm);
                    ywd->GateFullMsgTimeStamp = ywd->TimeStamp;
                };
            };                    
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
ULONG yw_BGParser(struct ScriptParser *p)
/*
**  FUNCTION
**      GameShell-Background-Pic-Parser [begin_bg .. end].
**      Benötigt <struct ypaworld_data *> in p->store[0].
**
**  CHANGED
**      29-Aug-96   floh    created
**      17-Jan-97   floh    + neues Keyword [name] ersetzt
**                            [disable_name][enable_name][finish_name]
**      06-Nov-97   floh    + neue Keywords: [background_map][rollover_map]
**                            [finished_map][enabled_map][mask_map]
**      21-Feb-98   floh    + mehr Backgroundmaps für die neue Shell
**      04-May-98   floh    + brief_map, debrief_map
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw, "begin_bg")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];
        struct WorldInfo *ln = ywd->LevelNet;

        /*** Überlauf-Schutz Anzahl BG-Pics ***/
        if (ln->NumBG >= MAXNUM_BG) {
            _LogMsg("world.ini: Too many BG pics (max=%d)!\n",MAXNUM_BG);
            return(PARSE_BOGUS_DATA);
        };

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0){
            ln->NumBG++;
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** Mission-Selection-Screen ***/
        }else if (stricmp(kw,"background_map")==0)
            strcpy(ln->BgMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"rollover_map")==0)
            strcpy(ln->RolloverMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"finished_map")==0)
            strcpy(ln->FinishedMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"enabled_map")==0)
            strcpy(ln->EnabledMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"mask_map")==0)
            strcpy(ln->MaskMaps[ln->NumBG].name,data);

        /*** Tutorial-Screen ***/
        else if (stricmp(kw,"tut_background_map")==0)
            strcpy(ln->TutBgMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"tut_rollover_map")==0)
            strcpy(ln->TutRolloverMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"tut_mask_map")==0)
            strcpy(ln->TutMaskMaps[ln->NumBG].name,data);

        /*** Hauptmenü-Beackground ***/
        else if (stricmp(kw,"menu_map")==0)
            strcpy(ln->MenuMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"input_map")==0)
            strcpy(ln->InputMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"settings_map")==0)
            strcpy(ln->SettingsMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"network_map")==0)
            strcpy(ln->NetworkMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"locale_map")==0)
            strcpy(ln->LocaleMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"save_map")==0)
            strcpy(ln->SaveMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"about_map")==0)
            strcpy(ln->AboutMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"help_map")==0)
            strcpy(ln->HelpMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"brief_map")==0)
            strcpy(ln->BriefMaps[ln->NumBG].name,data);
        else if (stricmp(kw,"debrief_map")==0) 
            strcpy(ln->DebriefMaps[ln->NumBG].name,data);    

        /*** size_x ***/
        else if (stricmp(kw,"size_x")==0)
            ln->BgMaps[ln->NumBG].w = strtol(data,NULL,0);

        /*** size_y ***/
        else if (stricmp(kw,"size_y")==0)
            ln->BgMaps[ln->NumBG].h = strtol(data,NULL,0);

        /*** UNKNOWN KEYWORD ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseWorldIni(struct ypaworld_data *ywd,
                      UBYTE *script)
/*
**  FUNCTION
**      Parst den angegebenen Filenamen mit folgenden
**      akzeptierten Kontext-Blöcken:
**
**          begin_bg    -> Background-Pic Definition
**          add_level   -> Level-Aufzählung für Level-Map
**
**  INPUTS
**      ywd     - LID des Welt-Objects
**      script  - Filename des world.ini Scripts
**
**  CHANGED
**      29-Aug-96   floh    created
**      09-Dec-97   floh    + yw_ParseColors()
**      25-Jan-98   floh    + yw_ParseMovieData()
**      04-Apr-98   floh    + yw_WorldMiscParser()
*/
{
    struct ScriptParser parsers[5];
    memset(parsers,0,sizeof(parsers));

    parsers[0].parse_func = yw_BGParser;
    parsers[0].store[0]   = (ULONG) ywd;

    parsers[1].parse_func = yw_ParseColors;
    parsers[1].store[0]   = (ULONG) ywd;

    parsers[2].parse_func = yw_ParseMovieData;
    parsers[2].store[0]   = (ULONG) ywd;

    parsers[3].parse_func = yw_ParseSuperItemData;
    parsers[3].store[0]   = (ULONG) ywd;

    parsers[4].parse_func = yw_WorldMiscParser;
    parsers[4].store[0]   = (ULONG) ywd;

    return(yw_ParseScript(script,5,parsers,PARSEMODE_SLOPPY));
}



