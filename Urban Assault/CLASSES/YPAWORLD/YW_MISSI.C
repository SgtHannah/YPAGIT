/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_mission.c,v $
**  $Revision: 38.6 $
**  $Date: 1998/01/06 16:24:22 $
**  $Locker: floh $
**  $Author: floh $
**
**  Mission-Briefing-Routinen.
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
#include "nucleus/math.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "bitmap/ilbmclass.h"
#include "ypa/ypaworldclass.h"
#include "audio/cdplay.h"

#include "yw_protos.h"

#ifdef __WINDOWS__
extern unsigned long wdd_DoDirect3D;
#endif

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_tform_engine
_extern_use_input_engine
_extern_use_audio_engine

/*** HACK: Pointer auf Speicherbereiche der base.class, siehe OM_NEW ***/

#ifdef AMIGA
extern __far struct pubstack_entry *yw_PubStack;
extern __far                 UBYTE *yw_ArgStack;
extern __far                 UBYTE *yw_EOArgStack;
#else
extern struct pubstack_entry *yw_PubStack;
extern                 UBYTE *yw_ArgStack;
extern                 UBYTE *yw_EOArgStack;
#endif

void yw_MBKillSet(struct ypaworld_data *);

#define DOSLOT_LINES    (1<<0)
#define DOSLOT_TEXT     (1<<1)

#define GET_X_COORD(x) (((x/640.0)*2.0)-1.0)
#define GET_Y_COORD(y) (((y/480.0)*2.0)-1.0)

#define MID_OF(x0,x1)   ((x0+x1)*0.5)
#define WIDTH_OF(x0,x1) (x1-x0)

#define BOUNDRECT_MAP_X0    GET_X_COORD(21)     // (-0.947)
#define BOUNDRECT_MAP_X1    GET_X_COORD(330)    // (+0.034)
#define BOUNDRECT_MAP_Y0    GET_Y_COORD(16)     // (-0.929)
#define BOUNDRECT_MAP_Y1    GET_Y_COORD(308)    // (+0.283)

#define BOUNDRECT_SLOT_X0   GET_X_COORD(4)      // (-0.689)
#define BOUNDRECT_SLOT_X1   GET_X_COORD(319)    // (-0.234)
#define BOUNDRECT_SLOT_Y0   GET_Y_COORD(324)    // (+0.379)
#define BOUNDRECT_SLOT_Y1   GET_Y_COORD(445)    // (+0.862)

#define BOUNDRECT_TEXT_X0   GET_X_COORD(368)    // (+0.147)
#define BOUNDRECT_TEXT_X1   GET_X_COORD(623)    // (+0.935)
#define BOUNDRECT_TEXT_Y0   GET_Y_COORD(43)    // (-0.833)
#define BOUNDRECT_TEXT_Y1   GET_Y_COORD(436)    // (+0.833)

struct YPAListReq MBR;

/*-----------------------------------------------------------------*/
ULONG yw_MissionMapParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst 0..4 begin_mbmap Kontext-Blöcke welche
**      Informationen über die zu verwendende Mission-
**      Briefing-Map in bis zu 4 Pixelauflösungen enthält.
**
**  CHANGED
**      16-Oct-96   floh    created
**      17-Jan-97   floh    + Cleanup
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct LevelDesc *ld = p->target;
    struct BGPicture *bg = &(ld->brfmap[ld->num_brfmaps]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw,"begin_mbmap")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        if (ld->num_brfmaps >= MAXNUM_BG) {
            _LogMsg("Mission Briefing Init: Too many bg maps (max=%d)!\n",MAXNUM_BG);
            return(PARSE_BOGUS_DATA);
        };

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0) {
            ld->num_brfmaps++;
            p->status = PARSESTAT_READY;
            return(PARSE_LEFT_CONTEXT);

        /*** name ***/
        }else if (stricmp(kw,"name")==0)
            strcpy(bg->name,data);

        /*** size_x ***/
        else if (stricmp(kw,"size_x")==0)
            bg->w = strtol(data,NULL,0);

        /*** size_y ***/
        else if (stricmp(kw,"size_y")==0)
            bg->h = strtol(data,NULL,0);

        /*** UNKNOWN KEYWORD ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** all-ok ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

/*-----------------------------------------------------------------*/
void yw_MBKillSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Selektive Set-Kill-Routine, räumt nur die
**      Sachen auf, die von yw_MBLoadSet()
**      verändert werden.
**
**  CHANGED
**      18-Oct-96   floh    created
*/
{
    Object *gfxo;

    yw_KillVPSet(ywd);      // VisProtos
    yw_KillLegoSet(ywd);    // Legos
    yw_KillSlurpSet(ywd);   // Slurps

    memset(ywd->SubSects,0,MAXNUM_SUBSECTS*sizeof(struct SubSectorDesc));
    memset(ywd->Sectors,0,MAXNUM_SECTORS*sizeof(struct SectorDesc));

    if (ywd->SetObject) {
        _dispose(ywd->SetObject);
        ywd->SetObject = NULL;
    };

    /*** DISPM_EndSession ***/
    _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
    _methoda(gfxo,DISPM_EndSession,NULL);
}

/*-----------------------------------------------------------------*/
BOOL yw_MBLoadSet(struct ypaworld_data *ywd, ULONG set_num)
/*
**  FUNCTION
**      Lädt und initialisiert das VisProto-Set und
**      Lego-Set für's Missionsbriefing. Alle
**      anderen Sachen, die normalerweise zum Set
**      gehören, werden nicht angefaßt (darin
**      unterscheidet sich die Routine vom
**      "normalen" yw_LoadSet().
**
**  CHANGED
**      18-Oct-96   floh    created
**      15-Jan-97   floh    + Cleanup Mauspointer
**      18-May-98   floh    + DISPM_BeginSession war hier schon
**                            zu spaet, weil beim Triggern
**                            generell durch das 3D-Rendering
**                            gegangen wurde (konnte crashen)
*/
{
    Object *gfxo;
    struct disp_pointer_msg dpm;
    UBYTE old_path[256];
    UBYTE set_path[256];

    /*** Mousepointer disablen, Texturcache flushen  ***/
    _OVE_GetAttrs(OVET_Object,&gfxo,TAG_DONE);
    dpm.pointer = ywd->MousePtrBmp[YW_MOUSE_DISK];
    dpm.type    = DISP_PTRTYPE_DISK;
    _methoda(gfxo,DISPM_SetPointer,&dpm);

    /*** Set-Pfad einstellen ***/
    strcpy(old_path,_GetAssign("rsrc"));
    sprintf(set_path,"data:set%d:",set_num);
    _SetAssign("rsrc",set_path);

    /*** Set-Object laden ***/
    if (ywd->SetObject = yw_LoadSetObject()) {

        struct MinList *ls;
        struct MinNode *nd;
        ULONG j;
        APTR sdf;

        /*** Set-Description-File öffnen ***/
        if (sdf = _FOpen("rsrc:scripts/set.sdf", "r")) {

            _get(ywd->SetObject, BSA_ChildList, &ls);
            for (j=0,nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ,j++) {

                Object *child = ((struct ObjectNode *)nd)->Object;

                switch(j) {
                    case 0:
                        /*** visproto.base ***/
                        if (!yw_LoadVPSet(ywd,child)) return(FALSE);
                        break;

                    case 1:
                        /*** lego.base ***/
                        if (!yw_LoadLegoSet(ywd,sdf,child))  return(FALSE);
                        if (!yw_ReadSubSectorTypes(ywd,sdf)) return(FALSE);
                        if (!yw_ReadSectorTypes(ywd,sdf))    return(FALSE);
                        break;

                    case 2:
                        /*** slurp.base ***/
                        if (!yw_LoadSlurpSet(ywd,child)) return(FALSE);
                        break;
                };
            };
            _FClose(sdf);

        } else {
            _LogMsg("Briefing: no set description file.\n");
            return(FALSE);
        };

    } else {
        _LogMsg("Briefing: No fat base object\n");
        return(FALSE);
    };

    /*** alten Resource-Pfad wiederherstellen ***/
    _SetAssign("rsrc",old_path);

    /*** MousePointer enablen ***/
    dpm.pointer = ywd->MousePtrBmp[YW_MOUSE_POINTER];
    dpm.type    = DISP_PTRTYPE_NORMAL;
    _methoda(gfxo,DISPM_SetPointer,&dpm);

    /*** und zurück ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL yw_MBInitListView(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert das Listview fuer die Textanzeige im 
**      Missionbriefing.
**
**  CHANGED
**      04-May-98   floh    created
*/
{
    BOOL retval = FALSE;
    WORD x0,x1,y0,y1,w,h;
    WORD num_shown;
    
    x0 = ((BOUNDRECT_TEXT_X0+1.0)*0.5) * ywd->DspXRes;
    x1 = ((BOUNDRECT_TEXT_X1+1.0)*0.5) * ywd->DspXRes;
    y0 = ((BOUNDRECT_TEXT_Y0+1.0)*0.5) * ywd->DspYRes;
    y1 = ((BOUNDRECT_TEXT_Y1+1.0)*0.5) * ywd->DspYRes;
    w = (x1-x0);
    num_shown = (y1-y0)/ywd->FontH;
    h = num_shown * ywd->FontH;
    
    memset(&MBR,0,sizeof(MBR));
    if (yw_InitListView(ywd, &(MBR),
        LIST_NumEntries,    1,
        LIST_ShownEntries,  num_shown,
        LIST_MinShown,      num_shown,
        LIST_MaxShown,      num_shown,
        LIST_DoIcon,        FALSE,
        LIST_EntryHeight,   ywd->FontH,
        LIST_EntryWidth,    w,
        LIST_VBorder,       ywd->EdgeH,
        TAG_DONE))
    {
        yw_ListSetRect(ywd,&MBR,x0,y0);
        yw_OpenReq(ywd,&(MBR.Req));
        return(TRUE);
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_MBKillListView(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Gegenstueck zu yw_MBInitListView().
**
**  CHANGED
**      08-Apr-98   floh    created
*/
{
    yw_CloseReq(ywd,&(MBR.Req));
    yw_KillListView(ywd,&(MBR));
}

/*-----------------------------------------------------------------*/
BOOL yw_InitMissionBriefing(struct ypaworld_data *ywd, ULONG lnum)
/*
**  FUNCTION
**      Initialisiert Mission-Briefing für den angebenen
**      Level.
**
**  INPUTS
**      ywd     - LID der Weltklasse
**      lnum    - Nummer des gewünschten Levels
**
**  RESULTS
**      TRUE    - aok
**      FALSE   - Ooops... -> Missionbriefing überspringen
**                und versuchen Level zu starten
**
**  CHANGED
**      17-Oct-96   floh    created
**      19-Oct-96   floh    Slot-Positionen auf Bildschirm
**                          werden initialisiert
**      20-Oct-96   floh    + TypeMap wird ausgefüllt
**      28-Oct-96   floh    + TimerStatus initialisiert
**      17-Jan-97   floh    + Cleanup
**      26-Feb-97   floh    + Missionmap wird mit BMA_Texture
**                            Flag initialisiert
**      25-Mar-97   floh    Missionmap erhält das Modifier-Attr
**                          BMA_TxtBlittable
**      29-May-97   floh    + CD-Audio-Support im Missionbriefing
**      26-Jun-97   floh    + TypeMap wird nach <ywd> geladen
**      27-Jan-98   floh    + Tutorial-Movie-Sachen
**      02-Feb-98   floh    + yw_MBKillSet() wird jetzt hier erledigt.
**      07-Apr-98   floh    + FirstContact Flags werden ausgewertet
**      04-May-98   floh    + initialisiert neue BgMap
**                          + initialisiert Text-Listview
**      07-May-98   floh    + Listview wurde im Fehlerfall nicht
**                            korrekt aufgeraeumt
**      18-May-98   floh    + loescht jetzt zuerst den Screen.
**                          + stoppt CD Player
*/
{
    struct MissionBriefing *mb = &(ywd->Mission);
    struct snd_cdcontrol_msg cd;

    memset(mb,0,sizeof(struct MissionBriefing));
    mb->MouseOverElm = -1;
    mb->ElmSlot      = -1;
    
    /*** CD Player starten ***/
    cd.command = SND_CD_STOP;
    _ControlCDPlayer(&cd);
    
    /*** Text Listview initialisieren ***/
    if (!yw_MBInitListView(ywd)) return(FALSE);    
    
    /*** Texturcache flushen ***/    
    _methoda(ywd->GfxObject,DISPM_BeginSession,NULL);
    _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
    _methoda(ywd->GfxObject,RASTM_Clear,NULL);
    _methoda(ywd->GfxObject,RASTM_End2D,NULL);

    /*** aktuelle Level-Info-Struktur sichern ***/
    mb->LevelBackup   = *ywd->Level;
    mb->Status        = MBSTATUS_INVALID;
    mb->TimerStatus   = TMBSTAT_PLAYING;
    mb->TimeStamp     = 0;
    mb->TextTimeStamp = 0;

    /*** LevelInfo-Struktur "patchen" ***/
    ywd->Level->Num      = lnum;
    ywd->Level->Status   = LEVELSTAT_BRIEFING;
    ywd->Level->NumGates = 0;
    memset(&(ywd->Level->Gate),0,sizeof(ywd->Level->Gate));
    memset(&(ywd->gem),0,sizeof(ywd->gem));

    /*** Level-Description-File parsen ***/
    _SetAssign("rsrc","data:");
    if (yw_ParseLDF(ywd,&(mb->LevelDesc),ywd->LevelNet->Levels[lnum].ldf)) {

        /*** alles ok soweit? ***/
        if ((mb->LevelDesc.flags & LDESCF_ALL_OK) == LDESCF_ALL_OK) {

            struct LevelDesc *ld = &(mb->LevelDesc);
            
            /*** Missionbriefing-Text holen, falls existent ***/
            mb->MissionText = ypa_GetStr(ywd->LocHandle,STR_MISSION_TEXTS+ywd->Level->Num,"<NO INFO AVAILABLE>");

            /*** entscheiden, welche Missionbriefing-Map laden ***/
            if (ld->num_brfmaps > 0) {

                ULONG i;
                UBYTE *name;

                /*** FirstContact Handling ***/
                for (i=0; i<ld->num_robos; i++) {
                    ULONG owner = ld->robos[i].owner;
                    if (!ywd->Level->RaceTouched[owner]) {
                        LONG movie_id;
                        switch(ld->robos[i].owner) {
                            case 2: movie_id=MOV_KYT_INTRO;  break;
                            case 3: movie_id=MOV_MYK_INTRO;  break;
                            case 4: movie_id=MOV_TAER_INTRO; break;
                            case 5: movie_id=MOV_SULG_INTRO; break;
                            case 6: movie_id=MOV_KYT_INTRO;  break;
                            default: movie_id=-1;
                        };
                        if ((movie_id != -1) && (ywd->MovieData.Valid[movie_id])) {
                            ywd->Level->RaceTouched[owner] = TRUE;
                            strcpy(mb->Movie,&(ywd->MovieData.Name[movie_id]));
                            mb->Status = MBSTATUS_START_MOVIE;
                            break;
                        };
                    };
                };

                /*** normales Level-Intro-Movie-Handling ***/
                if (ywd->Level->Movie[0] && (MBSTATUS_INVALID == mb->Status)) {
                    strcpy(mb->Movie,ywd->Level->Movie);
                    mb->Status = MBSTATUS_START_MOVIE;
                };
                
                /*** lade Hintergrund Map ***/
                _SetAssign("rsrc","levels:");
                name = &(ywd->LevelNet->BriefMaps[0].name);
                if (name[0]) {
                    mb->BackgroundMap = _new("ilbm.class",
                                             RSA_Name,         name,
                                             BMA_Texture,      TRUE,
                                             BMA_TxtBlittable, TRUE,
                                             TAG_DONE);
                };
    
                /*** lade Missionbriefing Map ***/                
                name = ld->brfmap[0].name;
                _SetAssign("rsrc","mbpix:");
                mb->MissionMap = _new("ilbm.class",
                                      RSA_Name,         name,
                                      BMA_Texture,      TRUE,
                                      BMA_TxtBlittable, TRUE,
                                      TAG_DONE);
                if (mb->MissionMap) {

                    /*** TypeMap ... ***/
                    if (!ywd->TypeMap) {
                        /*** TypeMap war externer File... ***/
                        name = ld->typ_map;
                        ywd->TypeMap = _new("ilbm.class",RSA_Name,name,TAG_DONE);
                    };
                    if (ywd->TypeMap) {

                        ULONG j;

                        /*** TypeMap definiert Level-Size ***/
                        _get(ywd->TypeMap,BMA_Bitmap,&(mb->TypeBmp));
                        ywd->MapSizeX = mb->TypeBmp->Width;
                        ywd->MapSizeY = mb->TypeBmp->Height;
                        ywd->WorldSizeX = ywd->MapSizeX * SECTOR_SIZE;
                        ywd->WorldSizeY = ywd->MapSizeY * SECTOR_SIZE;

                        /*** initialisiere Slot-Positionen ***/
                        mb->InfoSlot[0].Rect.xmin = BOUNDRECT_SLOT_X0;                        
                        mb->InfoSlot[0].Rect.xmax = BOUNDRECT_SLOT_X1;                       
                        mb->InfoSlot[0].Rect.ymin = BOUNDRECT_SLOT_Y0;                       
                        mb->InfoSlot[0].Rect.ymax = BOUNDRECT_SLOT_Y1;                       

                        /*** falls kein Movie, Briefing sofort starten ***/
                        if (MBSTATUS_INVALID == mb->Status) mb->Status=MBSTATUS_MAPLOADED;

                        /*** aktuelles Set killen ***/
                        yw_MBKillSet(ywd);
                        return(TRUE);
                    };
                    /*** ab hier Fehlerbehandlung! ***/
                    _dispose(mb->MissionMap);
                    mb->MissionMap = NULL;
                };
            };
        };
    };

    /*** fehlerhaft zurück ***/
    yw_MBKillListView(ywd);
    *ywd->Level = mb->LevelBackup;
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_KillMissionBriefing(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Aufräumfunktion für das MissionBriefing.
**
**  CHANGED
**      17-Oct-96   floh    created
**      18-Oct-96   floh    + TypeMap-Killing
**      10-Sep-97   floh    + ywd->Level->Status wird auf LEVELSTAT_SHELL
**                            gesetzt
**      17-Sep-97   floh    + stoppt jetzt CD-Player
**      01-Mar-98   floh    + CD-Player-Stop raus, wird jetzt extern
**                            gemacht
*/
{
    struct MissionBriefing *mb = &(ywd->Mission);
    struct snd_cdcontrol_msg cd;

    /*** evtl. geladenes Set killen ***/
    yw_MBKillSet(ywd);

    /*** Type-Map killen ***/
    if (ywd->TypeMap) {
        _dispose(ywd->TypeMap);
        ywd->TypeMap = NULL;
    };

    /*** Mission-Map killen ***/
    if (mb->MissionMap) {
        _dispose(mb->MissionMap);
        mb->MissionMap = NULL;
    };
    
    /*** BgMap killen ***/
    if (mb->BackgroundMap) {
        _dispose(mb->BackgroundMap);
        mb->BackgroundMap = NULL;
    };    

    /*** Struktur ungültig markieren ***/
    mb->Status = MBSTATUS_INVALID;

    /*** Textlistview killen ***/
    yw_MBKillListView(ywd);
}

/*-----------------------------------------------------------------*/
void yw_MBWorld2Screen(struct ypaworld_data *ywd,
                       struct MissionBriefing *mb, 
                       FLOAT in_x, FLOAT in_z, 
                       FLOAT *out_x, FLOAT *out_y)
/*
**  FUNCTION
**      Wandelt die X/Z-Komponente einer Welt-Koordinate in
**      eine gueltige Screen-Koordinate (-1.0 .. +1.0) um.
**
**  INPUTS
**      in_x,in_z   - Welt-Koordinaten
**
**      RESULTS
**      *out_x, *out_z  - Screen-Koordinaten
**
**  CHANGED
**      28-Nov-97   floh    created
*/
{
    struct rast_rect *map = &(mb->MapBltEnd);
    FLOAT rel_x = in_x / ywd->WorldSizeX;
    FLOAT rel_z = -in_z / ywd->WorldSizeY;
    *out_x = map->xmin + (rel_x * (map->xmax - map->xmin));
    *out_y = map->ymin + (rel_z * (map->ymax - map->ymin));
}

/*-----------------------------------------------------------------*/
void yw_MBGetSlotRect(struct ypaworld_data *ywd, 
                      struct MissionBriefing *mb,
                      ULONG snum, FLOAT pos_x, FLOAT pos_z)
/*
**  FUNCTION
**      Fuellt das Screenrechteck des Slots mit sinnvollen
**      Werten, die relativ zu den Weltkoordinaten pos_x/pos_z
**      liegen. Probiert eine von 4 Varianten, versucht dabei,
**      keine Kollision mit den Bildschirm-Rand (garantiert)
**      und keine Kollision mit anderen Slot-Rects (nicht
**      garantiert) zu erreichen.
**
**  INPUTS
**
**  CHANGED
**      28-Nov-97   floh    created      
*/
{
    ULONG i,j;
    struct rast_rect r[4];
    ULONG collision[4];
    FLOAT mx,my;
    yw_MBWorld2Screen(ywd,mb,pos_x,pos_z,&mx,&my);

    /*** berechne alle 4 Varianten ***/
    for (i=0; i<4; i++) {
        FLOAT clip;
        switch(i) {
            case 0:
            case 1:
                r[i].xmin=mx+0.05; r[i].xmax=r[i].xmin+MBSLOT_WIDTH;
                break;
            case 2:
            case 3:
                r[i].xmax=mx-0.05; r[i].xmin=r[i].xmax-MBSLOT_WIDTH;
                break;
        };
        switch(i) {
            case 0:
            case 2:
                r[i].ymax=my-0.05; r[i].ymin=r[i].ymax-MBSLOT_HEIGHT;
                break;
            case 1:
            case 3:
                r[i].ymin=my+0.05; r[i].ymax=r[i].ymin+MBSLOT_HEIGHT;
                break;
        };
        /*** Kollision mit Screen-Rand korrigieren ***/
        if ((clip = -1.0 - r[i].xmin) > 0.0) {
            r[i].xmin += clip;  r[i].xmax += clip;
        } else if ((clip = 1.0 - r[i].xmax) < 0.0) {
            r[i].xmin += clip;  r[i].xmax += clip;
        };
        if ((clip = -1.0 - r[i].ymin) > 0.0) {
            r[i].ymin += clip;  r[i].ymax += clip;
        } else if ((clip = 1.0 - r[i].ymax) < 0.0) {
            r[i].ymin += clip;  r[i].ymax += clip;
        };  
    };
    
    /*** Kollision mit anderen Slot-Rects? ***/
    for (i=0; i<4; i++) {
        collision[i] = 0;
        for (j=0; j<MAXNUM_INFOSLOTS; j++) {
            if ((j != snum) && (mb->InfoSlot[j].Type != MBTYPE_NONE)) {
                struct rast_rect *r0 = &(r[i]);
                struct rast_rect *r1 = &(mb->InfoSlot[j].Rect);
                ULONG x_col = FALSE;
                ULONG y_col = FALSE;
                if (((r0->xmin > r1->xmin) && (r0->xmin < r1->xmax)) ||
                    ((r0->xmax > r1->xmin) && (r0->xmax < r1->xmax)))
                {
                    x_col = TRUE;
                };
                if (((r0->ymin > r1->ymin) && (r0->ymin < r1->ymax)) ||
                    ((r0->ymax > r1->ymin) && (r0->ymax < r1->ymax)))
                {
                    y_col = TRUE;
                };
                if (x_col && y_col) {
                    collision[i] = TRUE;
                    break;
                };
            };
        };
    };
    
    /*** nimm die 1.Variante ohne Kollision ***/
    for (i=0; i<4; i++) {
        if (!collision[i]) break;
    };
    if (i==4) i=0;

    /*** Ergebnis nach Rect des InfoSlots schreiben ***/
    mb->InfoSlot[snum].Rect = r[i];
}

/*-----------------------------------------------------------------*/
void yw_MBPutTitles(struct ypaworld_data *ywd,struct MissionBriefing *mb)
/*
**  FUNCTION
**      Rendert Level-Titel in linke obere Ecke,
**      aktuellen MissionsBriefing-Titel in rechte
**      obere Ecke, Rendering passiert "sofort".
**
**  CHANGED
**      21-Oct-96   floh    created
**      07-Apr-97   floh    Title leicht umpositioniert
**      28-Nov-97   floh    + Missionbriefing-Texte DBCS-enabled
**      01-Dec-97   floh    + keine Anzeige der Missionbriefing-Stufe mehr
**      13-Dec-97   floh    + DBCS-Farbe
*/
{
    UBYTE str_buf[256];
    UBYTE *str = str_buf;
    struct rast_text rt;
    UBYTE *name;
    LONG x0,x1,y0,y1,w;     
    
    x0 = BOUNDRECT_MAP_X0 * (ywd->DspXRes>>1);
    x1 = BOUNDRECT_MAP_X1 * (ywd->DspXRes>>1);
    y0 = BOUNDRECT_MAP_Y0 * (ywd->DspYRes>>1);
    y1 = BOUNDRECT_MAP_Y1 * (ywd->DspYRes>>1);
    w  = (x1-x0);

    /*** Level-Titel immer und sofort anzeigen ***/
    name = ypa_GetStr(ywd->LocHandle,STR_NAME_LEVELS+ywd->Level->Num,ywd->Level->Title);
    new_font(str,FONTID_MAPCUR_4);
    pos_abs(str,x0,y0+4);
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_BRIEFING),yw_Green(ywd,YPACOLOR_TEXT_BRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_BRIEFING));
    str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_MAPCUR_4],str,name,w);

    /*** String(s) rendern und fertig***/
    eos(str);
    rt.string = str_buf;
    rt.clips  = NULL;
    _methoda(ywd->GfxObject,RASTM_Text,&rt);
}

/*-----------------------------------------------------------------*/
void yw_MBPutMarkers(struct ypaworld_data *ywd,
                     struct MissionBriefing *mb)
/*
**  FUNCTION
**      Zeichnet die Positions-Markierer für die
**      Mission-Briefing-Elemente, diese bleiben
**      ja bestehen, sind deshalb abgekoppelt
**      von den InfoSlots.
**
**  CHANGED
**      22-Oct-96   floh    created
*/
{
    UBYTE str_buf[1024];
    UBYTE *str = str_buf;
    UBYTE act_fnt = 255;    // impossible
    struct rast_rect *map = &(mb->MapBltEnd);
    struct rast_text rt;
    ULONG i;

    for (i=0; i<mb->ElmStoreIndex; i++) {

        struct MBElm *m = &(mb->ElmStore[i]);
        FLOAT rel_x,rel_z;
        WORD xpos,ypos;
        WORD size = (ywd->Fonts[m->fnt_id]->height)>>1;
        ULONG j;
        ULONG render_me = TRUE;        
        
        /*** falls identisch zu einem der Info-Slots, blinken lassen ***/
        for (j=0; j<MAXNUM_INFOSLOTS; j++) {
            struct MBInfoSlot *slot = &(mb->InfoSlot[j]);
            if ((slot->Type  != MBTYPE_NONE) &&
                (slot->Type  == m->type)     &&
                (slot->Id    == m->id)       &&
                (slot->Pos.x == m->pos_x)    &&
                (slot->Pos.z == m->pos_z)    &&
                ((mb->TimeStamp/200) & 1))
            {
                render_me = FALSE;
                break;
            };
        };
        
        if (render_me) {        

            /*** neuer Font? ***/
            if (m->fnt_id != act_fnt) {
                act_fnt = m->fnt_id;
                new_font(str,act_fnt);
            };

            /*** Element-Pos relativ zu Map... ***/
            rel_x = m->pos_x / ywd->WorldSizeX;
            rel_z = -m->pos_z / ywd->WorldSizeY;

            /*** ... und zu Display ***/
            rel_x = map->xmin + (rel_x * (map->xmax - map->xmin));
            rel_z = map->ymin + (rel_z * (map->ymax - map->ymin));

            /*** Integer-Koords... ***/
            xpos = ((WORD)(rel_x * ((FLOAT)(ywd->DspXRes>>1)))) - size;
            ypos = ((WORD)(rel_z * ((FLOAT)(ywd->DspYRes>>1)))) - size;
            pos_abs(str,xpos,ypos);
            put(str,m->chr);
        };
    };

    /*** komplett... ***/
    eos(str);
    rt.string = str_buf;
    rt.clips  = NULL;
    _methoda(ywd->GfxObject,RASTM_Text,&rt);
}

/*-----------------------------------------------------------------*/
void yw_MBRenderVProto(struct ypaworld_data *ywd,
                       ULONG snum,
                       LONG x_ang,
                       struct flt_triple *pos,
                       struct basepublish_msg *bpm)
/*
**  CHANGED
**      22-Oct-96   floh    created
**      12-Apr-97   floh    + VP_Array nicht mehr global
*/
{
    struct MissionBriefing *mb = &(ywd->Mission);
    struct MBInfoSlot *slot = &(mb->InfoSlot[snum]);
    struct flt_vector_msg fvm;
    struct lng_vector_msg lvm;
    Object *o;
    LONG rot;

    /*** Objekt ermitteln und vorbereiten ***/
    o = ywd->VisProtos[ywd->VP_Array[slot->Id].TypeNormal].o;
    _set(o,BSA_VisLimit,16000);
    _set(o,BSA_DFadeLength,100);

    /*** Position setzen ***/
    fvm.mask = VEC_X|VEC_Y|VEC_Z;
    fvm.x    = pos->x;
    fvm.y    = pos->y;
    fvm.z    = pos->z;
    _methoda(o,BSM_POSITION,&fvm);

    /*** Winkel setzen ***/
    rot = bpm->frame_time / 5;
    if (snum < 3) {
        /*** linke Seite ***/
        slot->Ang += rot;
        if (slot->Ang >= 360) slot->Ang-=360;
    } else {
        /*** rechte Seite ***/
        slot->Ang -= rot;
        if (slot->Ang < 0) slot->Ang+=360;
    };

    // OBSOLETE -> noch aus 6-Slot-Zeiten    
    // switch(snum%3) {
    //     case 0: lvm.x = x_ang + 65; break;    // obere Reihe
    //     case 1: lvm.x = x_ang + 45; break;    // mittlere Reihe
    //     case 2: lvm.x = x_ang + 10; break;    // unten ist ok
    // };
    lvm.mask = VEC_X|VEC_Y|VEC_Z;
    lvm.x = x_ang + 10;    
    lvm.y = slot->Ang;
    lvm.z = 0;
    _methoda(o,BSM_ANGLE,&lvm);

    /*** und publishen ***/
    _methoda(o,BSM_PUBLISH,bpm);
}

/*-----------------------------------------------------------------*/
void yw_MBRenderSector(struct ypaworld_data *ywd,
                       ULONG snum,
                       LONG x_ang,
                       struct flt_triple *pos,
                       struct basepublish_msg *bpm)
/*
**  CHANGED
**      23-Oct-96   floh    created
*/
{
    struct MissionBriefing *mb = &(ywd->Mission);
    struct MBInfoSlot *slot = &(mb->InfoSlot[snum]);
    struct lng_vector_msg lvm;
    Object *o;
    LONG rot,ix,iy,lim,correct;
    tform *tf;

    struct SectorDesc *sd = &(ywd->Sectors[slot->Id]);

    /*** ein beliebiges Object als "Schablone" ***/
    o = ywd->VisProtos[0].o;

    rot = bpm->frame_time / 5;
    if (snum < 3) {
        /*** linke Seite ***/
        slot->Ang += rot;
        if (slot->Ang >= 360) slot->Ang-=360;
    } else {
        /*** rechte Seite ***/
        slot->Ang -= rot;
        if (slot->Ang < 0) slot->Ang+=360;
    };

    // OBSOLETE, noch aus 6-Slot-Zeiten
    // switch(snum%3) {
    //     case 0: lvm.x = x_ang + 65; break;    // obere Reihe
    //     case 1: lvm.x = x_ang + 45; break;    // mittlere Reihe
    //     case 2: lvm.x = x_ang + 10; break;    // unten ist ok
    // };
    lvm.mask = VEC_X|VEC_Y|VEC_Z;
    lvm.x    = x_ang + 10;
    lvm.y    = slot->Ang;
    lvm.z    = 0;
    _methoda(o,BSM_ANGLE,&lvm);
    _get(o,BSA_TForm,&tf);

    if (sd->SecType == SECTYPE_COMPACT) {
        lim=1; correct=0;
    } else {
        lim=3; correct=-1;
    };

    for (iy=0; iy<lim; iy++) {
        for (ix=0; ix<lim; ix++) {

            FLOAT dx,dy,dz;
            struct flt_vector_msg fvm;
            struct flt_m3x3 *m = &(tf->loc_m);
            ULONG lnum;
            Object *lego;

            /*** SubSektor positionieren ***/
            dx = (FLOAT)((ix+correct)*SLURP_WIDTH);
            dy = 0.0;
            dz = (FLOAT)((iy+correct)*SLURP_WIDTH);

            fvm.mask = VEC_X|VEC_Y|VEC_Z;
            fvm.x = pos->x + m->m11*dx + m->m12*dy + m->m13*dz;
            fvm.y = pos->y + m->m21*dx + m->m22*dy + m->m23*dz;
            fvm.z = pos->z + m->m31*dx + m->m32*dy + m->m33*dz;

            /*** Lego-Nummer ***/
            lnum = ywd->Sectors[slot->Id].SSD[ix][iy]->limit_lego[DLINDEX_FULL];
            lego = ywd->Legos[lnum].lego;
            _set(lego,BSA_Static,FALSE);
            _set(lego,BSA_VisLimit,16000);
            _set(lego,BSA_DFadeLength,100);
            _methoda(lego,BSM_ANGLE,&lvm);
            _methoda(lego,BSM_POSITION,&fvm);
            _methoda(lego,BSM_PUBLISH,bpm);
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBRenderObject(struct ypaworld_data *ywd,
                       struct MissionBriefing *mb,
                       struct VFMInput *ip,
                       ULONG snum, LONG rel_dt)
/*
**  FUNCTION
**      Erledigt Rendering des 3D-Objects.
**
**  INPUTS
**      ywd
**      mb
**      ip
**      snum    - Slot-Nummer
**      rel_dt  - relative Zeit-Differenz seit "Einblendung"
**                des 3D-Objects.
**
**  CHANGED
**      22-Oct-96   floh    created
**      23-Oct-96   floh    + Zoom In innerhalb 2.0 Sekunden
**      25-Oct-96   floh    + TimeCodes durch Defines ersetzt
**      03-Apr-97   floh    + Bugfix: basepublish_msg wurde jedesmal
**                            komplett neu initialisiert -> dummer Fehler,
**                            weil auf manchen Gfx-Treibern absturzträchtig
**      12-Apr-97   floh    + VP_Array nicht mehr global
*/
{
    struct MBInfoSlot *slot = &(mb->InfoSlot[snum]);
    tform vwr;

    /*** BasePublish-Msg ausfüllen ***/
    mb->bpm.frame_time  = ip->FrameTime;
    mb->bpm.global_time = mb->TimeStamp;
    mb->bpm.owner_id    = 1;   // != 0, der 22-Sep-95-Bug.

    /*** Viewer initialisieren ***/
    memset(&vwr,0,sizeof(vwr));
    vwr.scl.x = 1.0;
    vwr.scl.y = 1.0;
    vwr.scl.z = 1.0;
    vwr.loc_m.m11=1.0; vwr.loc_m.m12=0.0; vwr.loc_m.m13=0.0;
    vwr.loc_m.m21=0.0; vwr.loc_m.m22=1.0; vwr.loc_m.m23=0.0;
    vwr.loc_m.m31=0.0; vwr.loc_m.m32=0.0; vwr.loc_m.m33=1.0;
    _SetViewer(&vwr);
    _TFormToGlobal(&vwr);

    if (slot->Type != MBTYPE_NONE) {

        struct rast_rect *r = &(slot->Rect);
        struct flt_triple vec;
        FLOAT min_dist,max_dist,act_dist;
        LONG min_ang,max_ang,act_ang;

        /*** durch diesen hohlen Vektor muß er kommen! ***/
        vec.x = (r->xmax + r->xmin)/2.0;
        vec.y = (r->ymax + r->ymin)/2.0;
        switch(slot->Type) {
            case MBTYPE_VPROTO:
                min_dist = ywd->VP_Array[slot->Id].Radius * 7;
                max_dist = ywd->VP_Array[slot->Id].Radius * 32;
                break;
            case MBTYPE_SECTOR:
                min_dist = 600.0 * 6;
                max_dist = 600.0 * 16;
                break;
        };

        /*** Start- / End-Winkel ***/
        min_ang = 0;
        max_ang = 90;

        /*** Zoom ***/
        if (rel_dt < MBT_DUR_ZOOM) {
            act_dist=max_dist+((min_dist-max_dist)*rel_dt)/MBT_DUR_ZOOM;
            act_ang =max_ang +((min_ang -max_ang) *rel_dt)/MBT_DUR_ZOOM;
        } else {
            act_dist=min_dist;
            act_ang =min_ang;
        };

        /*** Object-3D-Position ***/
        vec.x *= act_dist;
        vec.y *= act_dist;
        vec.z = act_dist;

        /*** 3D-Objekt(e) publishen ***/
        switch(slot->Type) {
            case MBTYPE_VPROTO:
                yw_MBRenderVProto(ywd,snum,act_ang,&vec,&(mb->bpm));
                break;

            case MBTYPE_SECTOR:
                yw_MBRenderSector(ywd,snum,act_ang,&vec,&(mb->bpm));
                break;
        };
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_MBGetInfoSlot(struct ypaworld_data *ywd, struct MissionBriefing *mb, FLOAT x, FLOAT z)
/*
**  FUNCTION
**      Ermittelt die Nummer des "zuständigen" InfoSlots
**      für die angegebene Weltkoordinate.
**
**  CHANGED
**      18-Oct-96   floh    created
**      04-May-98   floh    es gibt nur noch einen Slot
*/
{
    FLOAT rel_x = x / ywd->WorldSizeX;
    FLOAT rel_z = -z / ywd->WorldSizeY;
    ULONG slot = 0;
    return(slot);
    
    /*** OBSOLETE *** OBSOLETE *** OBSOLETE ***/
    /*** plaziert Infoslot-Rechteck dynamisch ***/
    //ULONG slot;
    //ULONG i;
    //ULONG max_age_slot = 0;
    //for (i=0; i<MAXNUM_INFOSLOTS; i++) {
    //    struct MBInfoSlot *s = &(mb->InfoSlot[i]);
    //    if (s->Type == MBTYPE_NONE) {
    //        /*** ein freier! ***/
    //        break;
    //    } else {
    //        /*** besetzt, Alter testen ***/
    //        if (s->StartTime <= mb->InfoSlot[max_age_slot].StartTime) {
    //            max_age_slot = i;
    //        };
    //    };
    //};
    ///*** alle besetzt? ***/
    //if (MAXNUM_INFOSLOTS == i) slot = max_age_slot;
    //else                       slot = i;
    //
    ///*** Slot-Rechteck initialisieren ***/
    //yw_MBGetSlotRect(ywd,mb,slot,x,z);
}

/*-----------------------------------------------------------------*/
void yw_MBEraseSlot(struct ypaworld_data *ywd,
                    struct MissionBriefing *mb,
                    ULONG snum)
/*
**  FUNCTION
**      Löscht den angegebenen Slot
**
**  CHANGED
**      20-Oct-96   floh    created
*/
{
    struct MBInfoSlot *slot = &(mb->InfoSlot[snum]);
    slot->Type = MBTYPE_NONE;
    slot->Id   = 0;
}

/*-----------------------------------------------------------------*/
ULONG yw_MBFillSlot(struct ypaworld_data *ywd,
                    struct MissionBriefing *mb,
                    FLOAT pos_x, FLOAT pos_z,
                    UBYTE color, UBYTE *text,
                    UBYTE fnt_id, UBYTE chr,
                    ULONG type, ULONG id,
                    ULONG fill_elm)
/*
**  FUNCTION
**      Reserviert einen passenden Info-Slot und füllt diesen
**      mit dem übergebenen "Ereignis".
**
**  INPUT
**      ywd
**      mb
**      pos_x,pos_z     - Position in Welt-Koordinaten
**      color           - Legenden-Linien-Farbe
**      text            - Erklärungs-Text
**      fnt_id          - für Map-Markierung! (nicht den Text!)
**      chr             - ditto
**      type            - MBTYPE_NONE | MBTYPE_VPROTO | MBTYPE_SEC
**      id              - VehicleProto-Nummer oder Sector-Typ
**      fill_elm        - TRUE, falls der Slot in der "MB-History"
**                        mitgeschrieben werden soll
**
**  RETURNS
**      slot_num
**
**  CHANGED
**      19-Oct-96   floh    created
**      20-Oct-96   floh    ugh, hatte Slot gelöscht, dabei natürlich
**                          die statischen Parameter überschrieben :-/
**      22-Oct-96   floh    + <fnt_id> und <chr> nebst zugehörigen Code.
**      23-Oct-97   floh    + Soundausgabe "Wuuusch"
*/
{
    ULONG snum = yw_MBGetInfoSlot(ywd,mb,pos_x,pos_z);
    struct MBInfoSlot *slot  = &(mb->InfoSlot[snum]);
    struct MBElm      *mbelm = &(mb->ElmStore[mb->ElmStoreIndex]);

    /*** Infoslot ausfüllen ***/
    slot->StartTime = mb->TimeStamp;
    slot->Pos.x     = pos_x;
    slot->Pos.z     = pos_z;
    slot->Color     = color;
    slot->Type      = type;
    slot->Id        = id;
    memset(slot->Text,0,sizeof(slot->Text));
    strcpy(slot->Text,text);

    /*** ElmStore mit Map-Markierung auffüllen ***/
    if (fill_elm) {
        mbelm->pos_x  = pos_x;
        mbelm->pos_z  = pos_z;
        mbelm->fnt_id = fnt_id;
        mbelm->chr    = chr;
        mbelm->color  = color;
        mbelm->type   = type;
        mbelm->id     = id;
        memset(&(mbelm->text),0,sizeof(mbelm->text));
        strcpy(mbelm->text,text);

        mb->ElmStoreIndex++;
        if (mb->ElmStoreIndex >= MAXNUM_MBELEMENTS) {
            mb->ElmStoreIndex = MAXNUM_MBELEMENTS - 1;
        };
    };

    /*** Sound ***/
    if (ywd->gsr) {
        _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_OBJECTAPPEAR);
    };

    return(snum);
}

/*-----------------------------------------------------------------*/
void yw_MBDoSlot2D(struct ypaworld_data *ywd,
                   struct MissionBriefing *mb,
                   struct VFMInput *ip,
                   ULONG snum, ULONG what)
/*
**  FUNCTION
**      Rendert den 2D-Part des Slots.
**
**  CHANGED
**      13-Dec-97   floh    + ausgegliedert aus yw_MBDoSlot(), weil
**                            dort illegaler Weise 2D- und 3D-Rendering
**                            gemixt wurde!
*/
{
    struct MBInfoSlot *slot = &(mb->InfoSlot[snum]);

    /*** Slot überhaupt aktiviert? ***/
    if (slot->Type != MBTYPE_NONE) {

        struct rast_intrect ri;
        ULONG dt = mb->TimeStamp-slot->StartTime;
        FLOAT rel_x,rel_z;

        /*** Map-relative Pos des Ereignisses (0.0 .. 1.0) ***/
        rel_x = slot->Pos.x / ywd->WorldSizeX;
        rel_z = -slot->Pos.z / ywd->WorldSizeY;

        /*** Integer-Koords des Slots ***/
        ri.xmin = slot->Rect.xmin * (ywd->DspXRes>>1);
        ri.ymin = slot->Rect.ymin * (ywd->DspYRes>>1);
        ri.xmax = slot->Rect.xmax * (ywd->DspXRes>>1);
        ri.ymax = slot->Rect.ymax * (ywd->DspYRes>>1);

        /*** Step 1: zeichne Linien Karte -> Legende ***/
        if ((dt > MBT_START_LINES) && (what & DOSLOT_LINES)) {

            struct rast_pens rp;
            struct rast_line rl0;
            struct rast_line rl1;
            struct rast_rect *map = &(mb->MapBltEnd);

            /*** Legenden-Linien ***/
            rl1.x0 = map->xmin + (rel_x * (map->xmax - map->xmin));
            rl1.x1 = rl1.x0;
            rl1.y0 = map->ymin + (rel_z * (map->ymax - map->ymin));
            rl1.y1 = slot->Rect.ymax;
            rl0.y0 = slot->Rect.ymax;
            rl0.y1 = slot->Rect.ymax;
            if (snum < 3) {
                /*** linke Seite ***/
                rl0.x0 = slot->Rect.xmin;
                rl0.x1 = rl1.x0;
            } else {
                /*** rechte Seite ***/
                rl0.x0 = rl1.x0;
                rl0.x1 = slot->Rect.xmax;
            };
            #ifdef __WINDOWS__
                if (wdd_DoDirect3D) {
                    rp.fg_pen  = 0x0000A0A0;
                    rp.fg_apen = 0x00007070;
                    rp.bg_pen  = -1;
                } else {
                    rp.fg_pen  = slot->Color;
                    rp.fg_apen = slot->Color;
                    rp.bg_pen  = -1;
                };
            #else
                rp.fg_pen  = slot->Color;
                rp.fg_apen = slot->Color;
                rp.bg_pen  = -1;
            #endif
            _methoda(ywd->GfxObject,RASTM_SetPens,&rp);
            _methoda(ywd->GfxObject,RASTM_Line,&rl0);
            #ifdef __WINDOWS__
                if (wdd_DoDirect3D) {
                    rp.fg_pen  = 0x00004040;
                    rp.fg_apen = 0x00007070;
                    rp.bg_pen  = -1;
                };
                _methoda(ywd->GfxObject,RASTM_SetPens,&rp);
            #endif
            _methoda(ywd->GfxObject,RASTM_Line,&rl1);
        };

        /*** Step 2: Text rendern ***/
        if ((dt > MBT_START_TEXT) && (slot->Text) && (what & DOSLOT_TEXT)) {

            UBYTE str_buf[128];
            UBYTE *str = str_buf;
            WORD xpos,ypos;
            struct rast_text rt;
            LONG rel_dt = dt-MBT_START_TEXT;
            ULONG fhgt = ywd->Fonts[FONTID_MAPCUR_4]->height;
            ULONG align;
            LONG rel_width;

            if (rel_dt < MBT_DUR_TEXT) {
                rel_width = (100 * rel_dt)/MBT_DUR_TEXT;
            } else {
                rel_width = 100;
            };

            xpos = ((slot->Rect.xmin+slot->Rect.xmax)*0.5) * (ywd->DspXRes>>1);
            ypos = (slot->Rect.ymax * (ywd->DspYRes>>1)) - fhgt - 1;

            new_font(str,FONTID_MAPCUR_4);
            pos_abs(str,xpos,ypos);
            dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_BRIEFING),yw_Green(ywd,YPACOLOR_TEXT_BRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_BRIEFING));
            str = yw_TextRelWidthItem(ywd->Fonts[FONTID_MAPCUR_4],str,slot->Text,rel_width,YPACOLF_ALIGNCENTER);
            eos(str);

            rt.string = str_buf;
            rt.clips  = NULL;
            _methoda(ywd->GfxObject,RASTM_Text,&rt);
        };
    };
}


/*-----------------------------------------------------------------*/
void yw_MBDoSlot3D(struct ypaworld_data *ywd,
                   struct MissionBriefing *mb,
                   struct VFMInput *ip,
                   ULONG snum)
/*
**  FUNCTION
**      Rendert den 3D-Part des Slots.
**
**  CHANGED
**      19-Oct-96   floh    created
**      20-Oct-96   floh    + FGPen bei Linien wurde nicht gesetzt
**      21-Oct-96   floh    + Textausgabe (falls slot->Title gesetzt)
**      22-Oct-96   floh    + 3D-Object-Rendering
**      25-Oct-96   floh    + Timing-Hardcodes durch Defines ersetzt
**                          + Legenden-Linien nicht mehr schräg
**      02-Apr-97   floh    + Linien-Farben-Patch fuer Direct3D
**      13-Dec-97   floh    + 2D-Part ausgelagert und umbenannt nach 
**                            yw_MBDoSlot3D()    
*/
{
    struct MBInfoSlot *slot = &(mb->InfoSlot[snum]);

    /*** Slot ueberhaupt aktiviert? ***/
    if (slot->Type != MBTYPE_NONE) {

        ULONG dt = mb->TimeStamp-slot->StartTime;

        /*** Step 3: 3D-Object rendern ***/
        if (dt > MBT_START_3D) {
            LONG rel_dt = dt-MBT_START_3D;
            yw_MBRenderObject(ywd,mb,ip,snum,rel_dt);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_MBMapLoaded(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  FUNCTION
**      Handler für MBSTATUS_MAPLOADED.
**
**  CHANGED
**      18-Oct-96   floh    created
**      23-Oct-97   floh    + Wuuusch Sound
**      07-Apr-98   floh    + CD Player Control
**      04-May-98   floh    + Bounding Rect ist jetzt woanders
*/
{
    struct LevelNode *ln = &(ywd->LevelNet->Levels[ywd->Level->Num]);
    FLOAT x_scale,y_scale;
    struct snd_cdcontrol_msg cd;

    /*** CD Player starten ***/
    cd.command = SND_CD_STOP;
    _ControlCDPlayer(&cd);
    if( ywd->gsr->missiontrack ) {
        cd.command   = SND_CD_SETTITLE;
        cd.para      = ywd->gsr->missiontrack;
        cd.min_delay = ywd->gsr->mission_min_delay;
        cd.max_delay = ywd->gsr->mission_max_delay;
        _ControlCDPlayer(&cd);
        cd.command = SND_CD_PLAY;
        _ControlCDPlayer(&cd);
    };

    /*** Vorbereitung auf Map-Skalierung ***/
    mb->Status    = MBSTATUS_MAPSCALING;
    mb->StartTime = mb->TimeStamp;
    _get(mb->MissionMap,BMA_Bitmap,&(mb->MapBlt.src));

    mb->MapBltStart.xmin = MID_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1);
    mb->MapBltStart.ymin = MID_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1);
    mb->MapBltStart.xmax = mb->MapBltStart.xmin;
    mb->MapBltStart.ymax = mb->MapBltStart.ymin;
    
    /*** Map-Größe ist abhängig von Level-Ausdehnung ***/
    if (ywd->MapSizeX > ywd->MapSizeY) {
        x_scale = 1.0;
        y_scale = ((FLOAT)ywd->MapSizeY)/((FLOAT)ywd->MapSizeX);
    } else if (ywd->MapSizeY > ywd->MapSizeX) {
        x_scale = ((FLOAT)ywd->MapSizeX)/((FLOAT)ywd->MapSizeY);
        y_scale = 1.0;
    } else {
        x_scale = y_scale = 1.0;
    };
    mb->MapBltEnd.xmin = MID_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1) - (WIDTH_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1)*0.5*x_scale);
    mb->MapBltEnd.ymin = MID_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1) - (WIDTH_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1)*0.5*y_scale);
    mb->MapBltEnd.xmax = mb->MapBltEnd.xmin + WIDTH_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1)*x_scale; 
    mb->MapBltEnd.ymax = mb->MapBltEnd.ymin + WIDTH_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1)*y_scale;
    mb->MapBlt.from.xmin = -1.0;
    mb->MapBlt.from.ymin = -1.0;
    mb->MapBlt.from.xmax = +1.0;
    mb->MapBlt.from.ymax = +1.0;
    mb->MapBlt.to = mb->MapBltStart;
}

/*-----------------------------------------------------------------*/
void yw_MBMapScaling(struct ypaworld_data *ywd,
                     struct VFMInput *ip,
                     struct MissionBriefing *mb)
/*
**  FUNCTION
**      Handler für MBSTATUS_MAPSCALING.
**
**  CHANGED
**      18-Oct-96   floh    created
*/
{
    struct rast_rect *r0  = &(mb->MapBltStart);
    struct rast_rect *r1  = &(mb->MapBltEnd);
    struct rast_blit *blt = &(mb->MapBlt);
    LONG dt = mb->TimeStamp - mb->StartTime;

    if (dt < MBT_DUR_MAPSCALE) {

        FLOAT scl = ((FLOAT)dt)/MBT_DUR_MAPSCALE;

        /*** aktuelle Blit-Koords ermitteln ***/
        blt->to.xmin = r0->xmin + ((r1->xmin - r0->xmin)*scl);
        blt->to.ymin = r0->ymin + ((r1->ymin - r0->ymin)*scl);
        blt->to.xmax = r0->xmax + ((r1->xmax - r0->xmax)*scl);
        blt->to.ymax = r0->ymax + ((r1->ymax - r0->ymax)*scl);

    } else {
        /*** fertig skaliert, To-Blt-Koord fixieren ***/
        mb->MapBlt.to = mb->MapBltEnd;
        mb->Status = MBSTATUS_MAPDONE;
    };
}

/*-----------------------------------------------------------------*/
void yw_MBMapDone(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  FUNCTION
**      Handler für MBSTATUS_MAPDONE.
**
**  CHANGED
**      18-Oct-96   floh    created
*/
{
    /*** alles klar... also nächste Briefing-Stufe aktivieren ***/
    mb->FillElms      = TRUE;
    mb->ElmStoreIndex = 0;
    
    /*** Text-Rendering startet parallel ***/    
    mb->TextTimeStamp = mb->TimeStamp;

    /*** falls noch nicht passiert, Set laden ***/
    if (!ywd->SetObject) {
        if (!yw_MBLoadSet(ywd,mb->LevelDesc.set_num)) {
            /*** Flucht nach vorn... ***/
            mb->Status = MBSTATUS_STARTLEVEL;
            return;
        };
    };
    mb->Status = MBSTATUS_L1_START;
}

/*-----------------------------------------------------------------*/
void yw_MBL1Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
**      22-Oct-96   floh    setzt ElmStoreIndex zurück auf 0
**      15-Jan-97   floh    + Titel Localized
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) yw_MBEraseSlot(ywd,mb,i);

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->MaxElm    = 1;
    mb->Status    = MBSTATUS_L1_RUNNING;
    mb->Title     = ypa_GetStr(ywd->LocHandle,STR_MB_YOUAREHERE,"YOU ARE HERE");
    mb->MouseOverElm = -1;
    mb->ElmSlot      = -1;
}

/*-----------------------------------------------------------------*/
void yw_MBL1Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      12-Apr-97   floh    + VP_Array nicht mehr global
*/
{
    LONG dt = mb->TimeStamp - mb->StartTime;

    if (dt < MBT_START_NEXT) {

        if (mb->ActElm != 0) {

            /*** UserRobo... ***/
            struct NLRoboDesc *r = &(mb->LevelDesc.robos[0]);
            UBYTE *name = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+r->vhcl_proto,
                          ywd->VP_Array[r->vhcl_proto].Name);
            UBYTE chr = r->owner + MB_CHAR_ROBO;

            /*** InfoSlot mit User-Robo füllen ***/
            mb->ActElm = 0;
            yw_MBFillSlot(ywd,mb,r->pos.x,r->pos.z,
                          MB_LINE_COLOR, name,
                          MB_MARKER_FONT, chr,
                          MBTYPE_VPROTO, r->vhcl_proto,
                          mb->FillElms);
        };
    } else {
        /*** nächster Stage ***/
        mb->Status = MBSTATUS_L1_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL1Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
*/
{
    /*** weiter mit Beamgates ***/
    mb->Status = MBSTATUS_L7_START;
}

/*-----------------------------------------------------------------*/
void yw_MBL2Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      15-Jan-97   floh    + Titel Localized
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) {
        if (mb->ElmSlot!=i) yw_MBEraseSlot(ywd,mb,i);
    };

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->MaxElm    = 0;
    mb->Title     = ypa_GetStr(ywd->LocHandle,STR_MB_PRIMARY,"PRIMARY TARGETS");

    /*** Anzahl Elemente ist Gesamtanzahl Keysektoren ***/
    for (i=0; i<ywd->Level->NumGates; i++) {
        struct Gate *g = &(ywd->Level->Gate[i]);
        if (g->mb_status != BRIEFSTAT_UNKNOWN) {
            mb->MaxElm += g->num_keysecs;
        };
    };
    if (mb->MaxElm == 0) mb->Status = MBSTATUS_L2_DONE;
    else                 mb->Status = MBSTATUS_L2_RUNNING;
}

/*-----------------------------------------------------------------*/
void yw_MBL2Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      29-Oct-96   floh    Bug: Sektor-Position wurde falsch ermittelt
**      01-Dec-97   floh    + Keysector haben jetzt einen Namen
**      16-Feb-98   floh    + neue Symbole
*/
{
    LONG dt;
    LONG act_elm;

    /*** alle Keysektoren aller BeamGates abhandeln ***/
    dt = mb->TimeStamp - mb->StartTime;

    /*** welches Element müßte an der Reihe sein? ***/
    act_elm = (dt / MBT_START_NEXT);

    if (act_elm < mb->MaxElm) {
        if (act_elm != mb->ActElm) {

            ULONG i,j,k;

            /*** ein neues aktivieren ***/
            mb->ActElm = act_elm;

            /*** richtigen Gate-Keysector neu einstellen ***/
            for (k=0,i=0; i<ywd->Level->NumGates; i++) {

                struct Gate *g = &(ywd->Level->Gate[i]);
                if (g->mb_status != BRIEFSTAT_UNKNOWN) {
                    for (j=0; j<g->num_keysecs; j++,k++) {

                        if (k == act_elm) {

                            struct KeySector *ks = &(g->keysec[j]);
                            FLOAT pos_x = ks->sec_x*SECTOR_SIZE+SECTOR_SIZE/2;
                            FLOAT pos_z = -(ks->sec_y*SECTOR_SIZE+SECTOR_SIZE/2);
                            UBYTE sec_type;
                            UBYTE *body = mb->TypeBmp->Data;
  
                            /*** SecType muß aus Type-Map geholt werden ***/
                            sec_type = body[ks->sec_y*mb->TypeBmp->Width+ks->sec_x];
                            yw_MBFillSlot(ywd,mb,pos_x,pos_z,
                                          MB_LINE_COLOR, ypa_GetStr(ywd->LocHandle,STR_MB_KEYSECTOR,"KEY SECTOR"),
                                          MB_MARKER_FONT, MB_CHAR_GATEKEYSEC,
                                          MBTYPE_SECTOR, sec_type,
                                          mb->FillElms);
                        };
                    };
                };
            };
        };

    } else {

        /*** alles durch, Primary Target Stage beenden ***/
        mb->Status = MBSTATUS_L2_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL2Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
*/
{
    /*** weiter mit Techupgrades ***/
    mb->Status = MBSTATUS_L3_START;
}

/*-----------------------------------------------------------------*/
void yw_MBL3Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      15-Jan-97   floh    + Titel Localized
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) {
        if (mb->ElmSlot!=i) yw_MBEraseSlot(ywd,mb,i);
    };

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->MaxElm    = 0;
    mb->Title     = ypa_GetStr(ywd->LocHandle,STR_MB_UPGRADE,"TECHNOLOGY UPGRADES");

    /*** Anzahl Elemente ist Gesamtanzahl Wundersteine ***/
    for (i=0; i<MAXNUM_WUNDERSTEINS; i++) {
        struct Wunderstein *g = &(ywd->gem[i]);
        if ((g->active)&&(g->mb_status!=BRIEFSTAT_UNKNOWN)) mb->MaxElm++;
    };
    if (mb->MaxElm == 0) mb->Status = MBSTATUS_L3_DONE;
    else                 mb->Status = MBSTATUS_L3_RUNNING;
}

/*-----------------------------------------------------------------*/
void yw_MBL3Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      12-Apr-97   floh    + BP_Array nicht mehr global
**      01-Dec-97   floh    + Tech Upgrades haben jetzt einen Namen
**      16-Feb-98   floh    + neues Symbol
*/
{
    LONG dt;
    LONG act_elm;

    /*** alle Keysektoren aller BeamGates abhandeln ***/
    dt = mb->TimeStamp - mb->StartTime;

    /*** welches Element müßte an der Reihe sein? ***/
    act_elm = (dt / MBT_START_NEXT);

    if (act_elm < mb->MaxElm) {
        if (act_elm != mb->ActElm) {

            struct Wunderstein *g;
            FLOAT pos_x,pos_z;
            UBYTE sec_type;
            ULONG i;


            /*** ein neues aktivieren ***/
            mb->ActElm = act_elm;

            /*** ignoriere BRIEFSTAT_UNKNOWN Elemente ***/
            for (i=0; i<MAXNUM_WUNDERSTEINS; i++) {
                g = &(ywd->gem[i]);
                if ((g->active) &&
                    (g->mb_status != BRIEFSTAT_UNKNOWN) &&
                    (act_elm-- == 0))
                {
                    break;
                };
            };

            /*** g jetzt richtiger Wunderstein ***/
            pos_x = g->sec_x*SECTOR_SIZE + SECTOR_SIZE/2;
            pos_z = -(g->sec_y*SECTOR_SIZE + SECTOR_SIZE/2);
            sec_type = ywd->BP_Array[g->bproto].SecType;
            yw_MBFillSlot(ywd,mb,pos_x,pos_z,
                          MB_LINE_COLOR, ypa_GetStr(ywd->LocHandle,STR_MB_TECHUPGRADE,"TECH UPGRADE"),
                          MB_MARKER_FONT, MB_CHAR_GEM,
                          MBTYPE_SECTOR, sec_type,
                          mb->FillElms);
        };

    } else {
        /*** alles durch, Primary Target Stage beenden ***/
        mb->Status = MBSTATUS_L3_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL3Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      20-Oct-96   floh    created
*/
{
    /*** weiter mit Feindrobos ***/
    mb->Status = MBSTATUS_L4_START;
}

/*-----------------------------------------------------------------*/
void yw_MBL4Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      15-Jan-97   floh    + Titel Localized
**                          + BRIEFSTAT_UNKNOWN Handling
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) {
        if (mb->ElmSlot!=i) yw_MBEraseSlot(ywd,mb,i);
    };

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->Title     = ypa_GetStr(ywd->LocHandle,STR_MB_ENEMYROBO,"ENEMY DEFENSE STATIONS");
    mb->MaxElm = 0;
    for (i=1; i<mb->LevelDesc.num_robos; i++) {
        struct NLRoboDesc *r = &(mb->LevelDesc.robos[i]);
        if (r->mb_status != BRIEFSTAT_UNKNOWN) mb->MaxElm++;
    };
    if (mb->MaxElm == 0) mb->Status = MBSTATUS_L4_DONE;
    else                 mb->Status = MBSTATUS_L4_RUNNING;
}

/*-----------------------------------------------------------------*/
void yw_MBL4Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      15-Jan-97   floh    + BRIEFSTAT_UNKNOWN Handling
**      12-Apr-97   floh    + VP_Array nicht mehr global
**      03-Dec-97   floh    + Vehikel-Namen lokalisiert
**      16-Feb-98   floh    + unterschiedliche Symbole für unterschiedliche
**                            Rassen
*/
{
    LONG dt;
    LONG act_elm;

    /*** alle feindlichen Robos abhandeln ***/
    dt = mb->TimeStamp - mb->StartTime;

    /*** welches Element müßte an der Reihe sein? ***/
    act_elm = (dt / MBT_START_NEXT);

    if (act_elm < mb->MaxElm) {
        if (act_elm != mb->ActElm) {

            struct NLRoboDesc *r = &(mb->LevelDesc.robos[act_elm+1]);
            ULONG i;
            UBYTE *name;
            UBYTE chr;

            /*** ein neues aktivieren ***/
            mb->ActElm = act_elm;

            /*** ignoriere BRIEFSTAT_UNKNOWN Elemente ***/
            for (i=1; i<(mb->LevelDesc.num_robos); i++) {
                r = &(mb->LevelDesc.robos[i]);
                if ((r->mb_status != BRIEFSTAT_UNKNOWN) &&
                    (act_elm-- == 0))
                {
                    break;
                };
            };

            /*** neues Element aktivieren ***/
            name = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+r->vhcl_proto,
                   ywd->VP_Array[r->vhcl_proto].Name);
            chr  = r->owner + MB_CHAR_ROBO;
            yw_MBFillSlot(ywd,mb,r->pos.x,r->pos.z,
                          MB_LINE_COLOR, name,
                          MB_MARKER_FONT, chr,
                          MBTYPE_VPROTO, r->vhcl_proto,
                          mb->FillElms);
        };

    } else {
        /*** alles durch, Primary Target Stage beenden ***/
        mb->Status = MBSTATUS_L4_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL4Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
*/
{
    /*** weiter mit Feindgeschwader ***/
    mb->Status = MBSTATUS_L5_START;
}

/*-----------------------------------------------------------------*/
void yw_MBL5Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      15-Jan-97   floh    + Titel Localized
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) {
        if (mb->ElmSlot!=i) yw_MBEraseSlot(ywd,mb,i);
    };

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->MaxElm    = 0;
    mb->Title     = ypa_GetStr(ywd->LocHandle,STR_MB_ENEMYFORCE,"ENEMY FORCES");

    /*** Anzahl Elemente ist Anzahl vorplazierter feindlicher Squads ***/
    for (i=0; i<mb->LevelDesc.num_squads; i++) {
        struct NLSquadDesc *s = &(mb->LevelDesc.squad[i]);
        if ((s->owner != 1) && (s->mb_status != BRIEFSTAT_UNKNOWN)) {
            mb->MaxElm++;
        };
    };
    if (mb->MaxElm == 0) mb->Status = MBSTATUS_L5_DONE;
    else                 mb->Status = MBSTATUS_L5_RUNNING;
}

/*-----------------------------------------------------------------*/
void yw_MBL5Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      12-Apr-97   floh    + VP_Array nicht mehr global
**      03-Dec-97   floh    + Name lokalisiert
**      16-Feb-98   floh    + unterschiedliche Symbole für unterschiedliche
**                            Rassen
**      20-May-98   floh    + Vehikel-Namen waren auf 32 begrenzt
*/
{
    LONG dt;
    LONG act_elm;

    /*** alle feindlichen Squads abhandeln ***/
    dt = mb->TimeStamp - mb->StartTime;

    /*** welches Element müßte an der Reihe sein? ***/
    act_elm = (dt / MBT_START_NEXT);

    if (act_elm < mb->MaxElm) {
        if (act_elm != mb->ActElm) {

            ULONG i,j;
            struct NLSquadDesc *s = NULL;

            /*** neues Element aktivieren ***/
            mb->ActElm = act_elm;

            /*** suche richtiges Squad, ignoriere eigene Squads! ***/
            for (i=0,j=0; i<mb->LevelDesc.num_squads; i++) {
                s = &(mb->LevelDesc.squad[i]);
                if ((s->owner != 1) && (s->mb_status != BRIEFSTAT_UNKNOWN)) {
                    if (j == act_elm) break;
                    j++;
                };
            };

            if (s) {
                UBYTE text[128];
                UBYTE *name = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+s->vproto,ywd->VP_Array[s->vproto].Name);
                UBYTE chr;
                sprintf(text,"%d %s",s->num,name);
                chr = s->owner + MB_CHAR_SQUAD;
                yw_MBFillSlot(ywd,mb,s->pos.x,s->pos.z,
                              MB_LINE_COLOR, text,
                              MB_MARKER_FONT, chr,
                              MBTYPE_VPROTO, s->vproto,
                              mb->FillElms);
            };
        };

    } else {
        /*** alles durch, Primary Target Stage beenden ***/
        mb->Status = MBSTATUS_L5_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL5Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
*/
{
    /*** weiter mit User-Squads ***/
    mb->Status = MBSTATUS_L6_START;
}


/*-----------------------------------------------------------------*/
void yw_MBL6Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      15-Jan-97   floh    + Titel Localized
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) {
        if (mb->ElmSlot!=i) yw_MBEraseSlot(ywd,mb,i);
    };

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->MaxElm    = 0;
    mb->Status    = MBSTATUS_L6_RUNNING;
    mb->Title     = "FRIENDLY SUPPORT FORCES";

    /*** Anzahl Elemente ist Anzahl vorplazierter User-Squads ***/
    for (i=0; i<mb->LevelDesc.num_squads; i++) {
        struct NLSquadDesc *s = &(mb->LevelDesc.squad[i]);
        if ((s->owner == 1) && (s->mb_status != BRIEFSTAT_UNKNOWN)) {
            mb->MaxElm++;
        };
    };
    if (mb->MaxElm == 0) mb->Status = MBSTATUS_L6_DONE;
    else                 mb->Status = MBSTATUS_L6_RUNNING;
}

/*-----------------------------------------------------------------*/
void yw_MBL6Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      12-Apr-97   floh    + VP_Array nicht mehr global
**      03-Dec-97   floh    + Name lokalisiert
**      16-Feb-98   floh    + neues Symbol
*/
{
    LONG dt;
    LONG act_elm;

    /*** alle User-Squads abhandeln ***/
    dt = mb->TimeStamp - mb->StartTime;

    /*** welches Element müßte an der Reihe sein? ***/
    act_elm = (dt / MBT_START_NEXT);

    if (act_elm < mb->MaxElm) {
        if (act_elm != mb->ActElm) {

            ULONG i,j;
            struct NLSquadDesc *s = NULL;

            /*** neues Element aktivieren ***/
            mb->ActElm = act_elm;

            /*** suche richtiges Squad, ignoriere feindliche Squads! ***/
            for (i=0,j=0; i<mb->LevelDesc.num_squads; i++) {
                s = &(mb->LevelDesc.squad[i]);
                if ((s->owner == 1) && (s->mb_status != BRIEFSTAT_UNKNOWN)) {
                    if (j == act_elm) break;
                    j++;
                };
            };

            if (s) {
                UBYTE text[128];
                UBYTE *name = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+s->vproto,ywd->VP_Array[s->vproto].Name);
                UBYTE chr;
                sprintf(text,"%d %s",s->num,name);
                chr = s->owner + MB_CHAR_SQUAD;
                yw_MBFillSlot(ywd,mb,s->pos.x,s->pos.z,
                              MB_LINE_COLOR, text,
                              MB_MARKER_FONT, chr,
                              MBTYPE_VPROTO, s->vproto,
                              mb->FillElms);
            };
        };

    } else {
        /*** alles durch, Primary Target Stage beenden ***/
        mb->Status = MBSTATUS_L6_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL6Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
*/
{
    /*** wieder von vorn ***/
    mb->Status = MBSTATUS_L1_START;

    /*** im nächsten Durchlauf Elms-Array in Ruhe lassen ***/
    mb->FillElms = FALSE;
}

/*-----------------------------------------------------------------*/
void yw_MBL7Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      15-Jan-97   floh    + Titel Localized
*/
{
    ULONG i;

    /*** InfosSlots löschen ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) {
        if (mb->ElmSlot!=i) yw_MBEraseSlot(ywd,mb,i);
    };

    /*** MB-Parameter initialisieren ***/
    mb->StartTime = mb->TimeStamp;
    mb->ActElm    = -1;
    mb->MaxElm    = 0;
    mb->Status    = MBSTATUS_L7_RUNNING;
    mb->Title     = ypa_GetStr(ywd->LocHandle,STR_MB_GATES,"TRANSPORTER GATES");

    for (i=0; i<ywd->Level->NumGates; i++) {
        struct Gate *g = &(ywd->Level->Gate[i]);
        if (g->mb_status != BRIEFSTAT_UNKNOWN) mb->MaxElm++;
    };
    if (mb->MaxElm == 0) mb->Status = MBSTATUS_L7_DONE;
    else                 mb->Status = MBSTATUS_L7_RUNNING;
}

/*-----------------------------------------------------------------*/
void yw_MBL7Running(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      25-Oct-96   floh    Timing-Hardcodes durch Defines ersetzt
**      01-Nov-96   floh    + BRIEFSTAT_UNKNOWN Handling
**      12-Apr-97   floh    + BP_Array nicht mehr global
**      01-Dec-97   floh    + Beamgates haben jetzt einen Namen
**      16-Feb-98   floh    + neues Symbol
*/
{
    LONG dt;
    LONG act_elm;

    /*** alle Beam Gates ***/
    dt = mb->TimeStamp - mb->StartTime;

    /*** welches Element müßte an der Reihe sein? ***/
    act_elm = (dt / MBT_START_NEXT);

    if (act_elm < mb->MaxElm) {
        if (act_elm != mb->ActElm) {

            ULONG i;
            struct Gate *g;
            FLOAT pos_x,pos_z;
            ULONG sec_type;

            mb->ActElm = act_elm;

            /*** ignoriere BRIEFSTAT_UNKNOWN ***/
            for (i=0; i<ywd->Level->NumGates; i++) {
                g = &(ywd->Level->Gate[i]);
                if ((g->mb_status != BRIEFSTAT_UNKNOWN) &&
                    (act_elm-- == 0))
                {
                    break;
                };
            };
            pos_x = g->sec_x*SECTOR_SIZE+SECTOR_SIZE/2;
            pos_z = -(g->sec_y*SECTOR_SIZE+SECTOR_SIZE/2);
            sec_type = ywd->BP_Array[g->closed_bp].SecType;

            /*** neues Element aktivieren ***/
            yw_MBFillSlot(ywd,mb,pos_x,pos_z,
                          MB_LINE_COLOR, ypa_GetStr(ywd->LocHandle,STR_MB_BEAMGATE,"BEAM GATE"),
                          MB_MARKER_FONT, MB_CHAR_GATE,
                          MBTYPE_SECTOR, sec_type,
                          mb->FillElms);
        };

    } else {
        /*** alles durch, Primary Target Stage beenden ***/
        mb->Status = MBSTATUS_L7_DONE;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_MBL7Done(struct ypaworld_data *ywd,
                 struct MissionBriefing *mb)
/*
**  CHANGED
**      21-Oct-96   floh    created
**      28-Oct-96   floh    Briefing beginnt jetzt von vorn.
*/
{
    /*** weiter mit Keysektoren ***/
    mb->Status = MBSTATUS_L2_START;
}

/*-----------------------------------------------------------------*/
void yw_MB3DRenderLoop(struct ypaworld_data *ywd,
                       struct MissionBriefing *mb,
                       struct VFMInput *ip)
/*
**  FUNCTION
**      Rendert die 3D-Objekte im Missionbriefing.
**
**  CHANGED
**      03-Apr-97   floh    bugfixed und ausgelagert
**      13-Dec-97   floh    Bugfix: rendert jetzt nur noch die 3D-
**                          Objekte
*/
{
    ULONG i;
    ULONG act_elm,num_elm;

    /*** 3D-Rendering-Loop ***/
    _methoda(ywd->GfxObject,RASTM_Begin3D,NULL);

    /*** basepublish_msg initialisieren ***/
    mb->bpm.frame_time  = 1;
    mb->bpm.global_time = 1;
    mb->bpm.pubstack    = yw_PubStack;
    mb->bpm.argstack    = (struct argstack_entry *) yw_ArgStack;
    mb->bpm.eoargstack  = (struct argstack_entry *) yw_EOArgStack;
    mb->bpm.ade_count   = 0;
    mb->bpm.maxnum_ade  = 1200;
    mb->bpm.owner_id    = 1;
    mb->bpm.min_z       = 17.0;
    mb->bpm.max_z       = 32000.0;

    /*** 3D-Objekte publishen... ***/
    for (i=0; i<MAXNUM_INFOSLOTS; i++) yw_MBDoSlot3D(ywd,mb,ip,i);

    /*** ...und rendern ***/
    num_elm = mb->bpm.pubstack - yw_PubStack;
    if (num_elm > 1) {
        qsort(yw_PubStack, num_elm, sizeof(struct pubstack_entry), yw_cmp);
    };
    for (act_elm=0; act_elm<num_elm; act_elm++) {
        void *drawargs = yw_PubStack[act_elm].args + 1;
        #ifdef AMIGA
            void __asm (*drawfunc) (__a0 void *);
            drawfunc = (void __asm (*) (__a0 void *))
                       yw_PubStack[act_elm].args->draw_func;
        #else
            void (*drawfunc) (void *);
            drawfunc = yw_PubStack[act_elm].args->draw_func;
        #endif
        drawfunc(drawargs);
    };

    /*** 3D-Rendering beendet ***/
    _methoda(ywd->GfxObject,RASTM_End3D,NULL);
}

/*-----------------------------------------------------------------*/
void yw_MBHandleInput(struct ypaworld_data *ywd,
                      struct VFMInput *ip,
                      struct MissionBriefing *mb)
/*
**  FUNCTION
**      Map-Marker, über denen die Maus ist, werden
**      bevorzugt gezeigt.
**
**  CHANGED
**      07-Apr-97   floh    created
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    LONG hit_index = -1;

    /*** Maus über einem Map-Marker??? ***/
    if (NULL == ci->box) {
        ULONG i;
        FLOAT mx = ((FLOAT)ci->act.scrx) / ((FLOAT)ywd->DspXRes);
        FLOAT my = ((FLOAT)ci->act.scry) / ((FLOAT)ywd->DspYRes);
        for (i=0; i<mb->ElmStoreIndex; i++) {

            struct MBElm *m = &(mb->ElmStore[i]);
            struct rast_rect *map = &(mb->MapBltEnd);
            FLOAT rel_x,rel_z;

            /*** Element-Pos relativ zu Map... ***/
            rel_x = m->pos_x / ywd->WorldSizeX;
            rel_z = -m->pos_z / ywd->WorldSizeY;

            /*** ... und zu Display (-1.0 .. +1.0) ***/
            rel_x = map->xmin + (rel_x * (map->xmax - map->xmin));
            rel_z = map->ymin + (rel_z * (map->ymax - map->ymin));

            /*** nach (0.0 .. 1.0) skalieren ***/
            rel_x = (rel_x + 1.0) * 0.5;
            rel_z = (rel_z + 1.0) * 0.5;

            /*** ist Maus drüber??? ***/
            if ((mx > (rel_x-0.025)) && (mx < (rel_x+0.025)) &&
                (my > (rel_z-0.025)) && (my < (rel_z+0.025)))
            {
                hit_index = i;
            };
        };

        /*** Maus über einem neuen(!) Marker? ***/
        if ((hit_index != -1) && (hit_index != mb->MouseOverElm)) {
            struct MBElm *m = &(mb->ElmStore[hit_index]);
            mb->MouseOverElm = hit_index;
            mb->ElmSlot = yw_MBFillSlot(ywd,mb,m->pos_x,m->pos_z,
                          m->color, m->text, m->fnt_id, m->chr,
                          m->type, m->id, FALSE);
        } else if ((hit_index!=mb->MouseOverElm) && (mb->MouseOverElm!=-1)) {
            /*** Maus ist runter, Slot abbrechen ***/
            yw_MBEraseSlot(ywd,mb,mb->ElmSlot);
            mb->MouseOverElm = -1;
            mb->ElmSlot      = -1;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_MBRLayoutItems(struct ypaworld_data *ywd, struct MissionBriefing *mb)
/*
**  CHANGED
**      04-May-98   floh    created
*/
{
    UBYTE *str = MBR.Itemblock;
    if (mb->TextTimeStamp > 0) {

        BOOL last_line = FALSE;
        UBYTE *msg_ptr;
        LONG time_diff = mb->TimeStamp - mb->TextTimeStamp;
        ULONG act_line,num_lines,wait_time;
        UBYTE c;        
        
        /*** zaehle Anzahl Zeilen im Missionbriefing ***/
        num_lines = 0;
        msg_ptr   = mb->MissionText;
        do {
            c = *msg_ptr++;
            if ((c == '\n') || (c == 0)) num_lines++;
        } while (c != 0);
        
        /*** Listview Parameter updaten ***/
        MBR.NumEntries = num_lines;
        
        /*** oberen Rand ***/    
        str = yw_LVItemsPreLayout(ywd, &(MBR), str, FONTID_MAPCUR_4, "   ");
        
        /*** fuer jede Zeile... ***/
        msg_ptr  = mb->MissionText;
        act_line = 0;
        do {
            UBYTE buf_line[512];
            UBYTE *buf_ptr = buf_line;
            LONG rel_width;
            
            /*** isoliere aktuelle Zeile ***/        
            do {
                c = *msg_ptr++;
                if (c == 0) last_line=TRUE;
                if (c == '\n') c=0;
                *buf_ptr++ = c;
            } while (c != 0);

            /*** Reinfliess-Verhalten ***/
            if (time_diff < 500) {
                /*** diese Zeile reinfliessen lassen ***/
                rel_width = (time_diff * 100) / 500;
                if (rel_width < 0)        rel_width = 0;
                else if (rel_width > 100) rel_width = 100;
            } else {
                /*** eine volle Zeile ***/
                rel_width = 100;
            };
            time_diff -= 500;

            /*** ist die Zeile sichtbar? ***/
            if ((act_line>=MBR.FirstShown) && (act_line < (MBR.FirstShown+MBR.ShownEntries))) {
                /*** String layouten ***/
                if (rel_width > 0) {
                    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Green(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Blue(ywd,YPACOLOR_TEXT_TOOLTIP));
                    str = yw_TextRelWidthItem(ywd->Fonts[FONTID_MAPCUR_4],str,buf_line,rel_width,YPACOLF_ALIGNLEFT);
                    new_line(str);
                };
            };
            act_line++;
        } while (!last_line);

        /*** unteren Rand ***/
        str = yw_LVItemsPostLayout(ywd, &(MBR), str, FONTID_MAPCUR_4, "   ");
    };
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_MBRHandleInput(struct ypaworld_data *ywd, 
                       struct VFMInput *ip,
                       struct MissionBriefing *mb)
/*
**  CHANGED
**      04-May-98   floh    created
*/
{
    /*** Listview-Input-Handling, und Layout ***/
    yw_ListHandleInput(ywd,&(MBR),ip);
    yw_ListLayout(ywd,&(MBR));

    /*** Itemblock layouten ***/
    yw_MBRLayoutItems(ywd,mb);
}

/*-----------------------------------------------------------------*/
void yw_MBRRender(struct ypaworld_data *ywd)
/*
**  CHANGED
**      04-May-98   floh    created
*/
{
    struct rast_text rt;
    rt.string = MBR.Req.req_string;
    rt.clips  = MBR.Req.req_clips;
    _methoda(ywd->GfxObject,RASTM_Text,&rt);
}

/*-----------------------------------------------------------------*/
void yw_TriggerMissionBriefing(struct ypaworld_data *ywd,
                               struct GameShellReq *gsr,
                               struct VFMInput *ip)
/*
**  FUNCTION
**      Übernimmt den gesamten Ablauf des Missionbriefings,
**      das Laden der Map hat bereits in yw_InitMissionBriefing()
**      stattgefunden. Als Ende-Bedingung ist MBSTAT_STARTLEVEL
**      bzw. MBSTAT_CANCEL definiert.
**
**  CHANGED
**      17-Oct-96   floh    created
**      28-Oct-96   floh    + TimerStatus
**      15-Jan-97   floh    + Marker und Titel werden jetzt nach
**                            den Info-Slots gerendert, damit die
**                            Marker die Linien überschreiben
**      26-Mar-97   floh    + RASTM_Begin2D/End2D/Begin3D/End3D
**      13-Dec-97   floh    + BUGFIX: Missionbriefing sollte jetzt
**                            auf allen Karten korrekt aussehen.
**                            Ich habe illegalerweise 2D-Operationen
**                            zwischen Begin3D/End3D gemixt...
**      04-May-98   floh    + rendert jetzt keine Linien mehr 
**                            zwischen Marker und 3D-Slot
**                          + yw_MBRHandleInput()
*/
{
    struct MissionBriefing *mb = &(ywd->Mission);
    ULONG i;

    if (mb->Status == MBSTATUS_START_MOVIE) {

        /*** Tutorial Movie anstatt Missionbriefing ***/
        yw_PlayMovie(ywd,&(mb->Movie));
        mb->Status = MBSTATUS_MAPLOADED;

    } else {

        /*** Standard-Missionbriefing ***/
        switch(mb->TimerStatus) {

            case TMBSTAT_PLAYING:
                mb->TimeStamp += ip->FrameTime;
                break;

            case TMBSTAT_PAUSED:
                ip->FrameTime = 1;
                mb->TimeStamp += ip->FrameTime;
                break;

            case TMBSTAT_FFWRD:
                mb->TimeStamp += ip->FrameTime;
                if (mb->Status == MBSTATUS_TEXT_RUNNING) {
                    mb->Status = MBSTATUS_TEXT_DONE;
                } else if (mb->ActElm != -1) {
                    switch(mb->Status) {
                        case MBSTATUS_L1_RUNNING:
                        case MBSTATUS_L2_RUNNING:
                        case MBSTATUS_L3_RUNNING:
                        case MBSTATUS_L4_RUNNING:
                        case MBSTATUS_L5_RUNNING:
                        case MBSTATUS_L6_RUNNING:
                        case MBSTATUS_L7_RUNNING:
                            /*** nächstes Missionbriefing-Element aktivieren ***/
                            mb->TimeStamp = mb->StartTime + MBT_START_NEXT*(mb->ActElm+1);
                            break;
                    };
                };
                mb->TimerStatus = TMBSTAT_PLAYING;
                break;

            case TMBSTAT_RWND:
                mb->Status      = MBSTATUS_TEXT_START;
                mb->TimerStatus = TMBSTAT_PLAYING;
                break;
        };

        /*** Aktion ist abhängig vom gegenwärtigen Status (wird durch MouseOverElm overridden) ***/
        switch(mb->Status) {

            /*** Map hochskalieren ***/
            case MBSTATUS_MAPLOADED:
                yw_MBMapLoaded(ywd,ip,mb);
                break;
            case MBSTATUS_MAPSCALING:
                yw_MBMapScaling(ywd,ip,mb);
                break;
            case MBSTATUS_MAPDONE:
                yw_MBMapDone(ywd,ip,mb);
                break;

            /*** YOU ARE HERE ***/
            case MBSTATUS_L1_START:
                yw_MBL1Start(ywd,ip,mb);
                break;
            case MBSTATUS_L1_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL1Running(ywd,ip,mb);
                break;
            case MBSTATUS_L1_DONE:
                yw_MBL1Done(ywd,mb);
                break;

            /*** PRIMARY TARGETS ***/
            case MBSTATUS_L2_START:
                yw_MBL2Start(ywd,ip,mb);
                break;
            case MBSTATUS_L2_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL2Running(ywd,ip,mb);
                break;
            case MBSTATUS_L2_DONE:
                yw_MBL2Done(ywd,mb);
                break;

            /*** SECONDARY TARGETS ***/
            case MBSTATUS_L3_START:
                yw_MBL3Start(ywd,ip,mb);
                break;
            case MBSTATUS_L3_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL3Running(ywd,ip,mb);
                break;
            case MBSTATUS_L3_DONE:
                yw_MBL3Done(ywd,mb);
                break;

            /*** ENEMY BATTLE STATIONS ***/
            case MBSTATUS_L4_START:
                yw_MBL4Start(ywd,ip,mb);
                break;
            case MBSTATUS_L4_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL4Running(ywd,ip,mb);
                break;
            case MBSTATUS_L4_DONE:
                yw_MBL4Done(ywd,mb);
                break;

            /*** ENEMY SUPPLY FORCES ***/
            case MBSTATUS_L5_START:
                yw_MBL5Start(ywd,ip,mb);
                break;
            case MBSTATUS_L5_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL5Running(ywd,ip,mb);
                break;
            case MBSTATUS_L5_DONE:
                yw_MBL5Done(ywd,mb);
                break;

            /*** USER SUPPLY FORCES ***/
            case MBSTATUS_L6_START:
                yw_MBL6Start(ywd,ip,mb);
                break;
            case MBSTATUS_L6_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL6Running(ywd,ip,mb);
                break;
            case MBSTATUS_L6_DONE:
                yw_MBL6Done(ywd,mb);
                break;

            /*** BEAM GATES ***/
            case MBSTATUS_L7_START:
                yw_MBL7Start(ywd,ip,mb);
                break;
            case MBSTATUS_L7_RUNNING:
                if (mb->MouseOverElm == -1) yw_MBL7Running(ywd,ip,mb);
                break;
            case MBSTATUS_L7_DONE:
                yw_MBL7Done(ywd,mb);
                break;
        };

        /*** Input auswerten ***/
        yw_MBRHandleInput(ywd,ip,mb);
        yw_MBHandleInput(ywd,ip,mb);

        /*** 2D-Hintergrund rendern ***/
        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
        if (mb->BackgroundMap) {
            struct rast_blit bg_blt;
            _get(mb->BackgroundMap,BMA_Bitmap,&bg_blt.src);
            bg_blt.from.xmin = -1.0;
            bg_blt.from.xmax = +1.0;
            bg_blt.from.ymin = -1.0;
            bg_blt.from.ymax = +1.0;
            bg_blt.to.xmin = -1.0;
            bg_blt.to.xmax = +1.0;
            bg_blt.to.ymin = -1.0;
            bg_blt.to.ymax = +1.0;
            _methoda(ywd->GfxObject,RASTM_ClippedBlit,&bg_blt);
        };
        _methoda(ywd->GfxObject,RASTM_ClippedBlit,&(mb->MapBlt));
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);

        /*** 3D-Objekte rendern ***/
        yw_MB3DRenderLoop(ywd,mb,ip);

        /*** Text und Stuff noch rendern... ***/
        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
        yw_MBPutMarkers(ywd,mb);
        yw_MBPutTitles(ywd,mb);
        yw_MBRRender(ywd);        
        for (i=0; i<MAXNUM_INFOSLOTS; i++) yw_MBDoSlot2D(ywd,mb,ip,i,DOSLOT_TEXT);
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_MBActive(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Returniert TRUE, falls Missionsbriefing gerade
**      läuft, FALSE, falls der normale Shell-Background
**      aktiv ist.
**
**  CHANGED
**      28-Oct-96   floh    created
**      29-Oct-97   floh    testet jetzt auf ungleich LEVELSTAT_SHELL
*/
{
    if (ywd->Level->Status == LEVELSTAT_SHELL) return(FALSE);
    else                                       return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_MBCancel(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Bricht Missionbriefing ab, kehrt zur normalen
**      Shell-BG-Funktionalität zurück.
**
**  CHANGED
**      28-Oct-96   floh    created
*/
{
    ywd->Mission.Status = MBSTATUS_CANCEL;
}

/*-----------------------------------------------------------------*/
void yw_MBStart(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Bricht MissionsBriefing ab, startet den Level.
**
**  CHANGED
**      28-Oct-96   floh    created
*/
{
    ywd->Mission.Status = MBSTATUS_STARTLEVEL;
}

/*-----------------------------------------------------------------*/
void yw_MBContinue(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Briefing weiterführen (PLAY-Taste auf CD-Player)
**
**  CHANGED
**      28-Oct-96   floh    created
*/
{
    ywd->Mission.TimerStatus = TMBSTAT_PLAYING;
}

/*-----------------------------------------------------------------*/
void yw_MBPause(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Briefing anhalten.
**
**  CHANGED
**      28-Oct-96   floh    created
*/
{
    ywd->Mission.TimerStatus = TMBSTAT_PAUSED;
}

/*-----------------------------------------------------------------*/
void yw_MBForward(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Schalte auf Fast Forward, muss jeden Frame neu
**      angewendet werden, solange User auf Taste
**      bleibt.
**
**  CHANGED
**      28-Oct-96   floh    created
*/
{
    ywd->Mission.TimerStatus = TMBSTAT_FFWRD;
}

/*-----------------------------------------------------------------*/
void yw_MBRewind(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      MissionBriefing beginnt von vorn.
**      Ein "richtiges" Zurückspulen läßt sich leider nicht
**      machen.
**
**  CHANGED
**      28-Oct-96   floh    created
*/
{
    ywd->Mission.TimerStatus = TMBSTAT_RWND;
}

