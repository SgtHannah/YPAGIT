/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_debrief.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 16:17:43 $
**  $Locker:  $
**  $Author: floh $
**
**  yw_debrief.c -- Routinen fürs Debriefing, orientiert sich
**                  in der grundlegenden Philosophie stark am
**                  Missionbriefing.
**
**  (C) Copyright 1997 by A.Weissflog
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
#include "ypa/guimap.h"
#include "ypa/ypaworldclass.h"
#include "audio/cdplay.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_tform_engine
_extern_use_input_engine
_extern_use_audio_engine

#ifdef __WINDOWS__
extern unsigned long wdd_DoDirect3D;
#endif

UBYTE *YPA_DBWireNames[MBNUM_WIREFRAMES] = {
    "wireless/db_genes.sklt",   // Placeholder VhclCreate
    "wireless/db_death.sklt",   // Placeholder VhclKill
    "wireless/db_death.sklt",   // Placeholder WpnExplode
    "wireless/db_sec.sklt",     // Placeholder ConSec
};

/*** Zeitgeber-Konstanten ***/
#define MB_TIMESCALE    (60)        // 60-fache Beschleunigung
#define MBFXTIME_VHCLCREATE (750 * MB_TIMESCALE)
#define MBFXTIME_VHCLKILL   (500 * MB_TIMESCALE)
#define MBFXTIME_WPNEXPLODE (500 * MB_TIMESCALE)
#define MBFXTIME_CONSEC     (500 * MB_TIMESCALE)

/*** Boundingrect Konstanten ***/
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

struct __dbkills_qsort_struct {
    LONG owner;
    LONG kills;
};
struct __dbscore_qsort_struct {
    LONG owner;
    LONG score;
};

/*-----------------------------------------------------------------*/
ULONG yw_DebriefingMapParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parst 0..4 begin_dbmap Kontext-Blöcke welche
**      Informationen über die zu verwendende Debriefing-Map in bis
**      zu 4 Pixelauflösungen enthält.
**
**  CHANGED
**      09-Sep-97   floh    created
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;

    struct LevelDesc *ld = p->target;
    struct BGPicture *bg = &(ld->debrfmap[ld->num_debrfmaps]);

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        if (stricmp(kw,"begin_dbmap")==0) {
            p->status = PARSESTAT_RUNNING;
            return(PARSE_ENTERED_CONTEXT);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else {

        if (ld->num_debrfmaps >= MAXNUM_BG) {
            _LogMsg("Debriefing Init: Too many bg maps (max=%d)!\n",MAXNUM_BG);
            return(PARSE_BOGUS_DATA);
        };

        /*** momentan innerhalb eines Context ***/
        if (stricmp(kw,"end")==0) {
            ld->num_debrfmaps++;
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
void yw_KillDebriefing(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Aufräumen für das Debriefing.
**
**  CHANGED
**      26-Aug-97   floh    created
**      01-Sep-97   floh    Type- und Ownermap werden nicht mehr
**                          gekillt, weil jetzt die Backups der
**                          Maps verwendet werden, welches vollkommen
**                          "außerhalb" passiert.
**      02-Sep-97   floh    + FX-WireFrames-Objects
**                          + Killt die privaten Kopien der OwnMapBU und
**                            TypeMapBU
**      17-Sep-97   floh    + stoppt jetzt CD-Player
**      01-Mar-98   floh    + CD-Player-Stop wieder rausgelöscht,
**                            wird jetzt extern gemacht
**      05-May-98   floh    + BackgroundMap wird gekillt
*/
{
    ULONG i;
    struct MissionBriefing *mb = &(ywd->Mission);
    struct snd_cdcontrol_msg cd;

    /*** Map-Kopien killen ***/
    if (mb->OwnMap) {
        _dispose(mb->OwnMap); mb->OwnMap = NULL;
    };
    if (mb->TypMap) {
        _dispose(mb->TypMap); mb->TypMap = NULL;
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

    /*** Wireframes raus ***/
    for (i=0; i<MBNUM_WIREFRAMES; i++) {
        if (mb->WireObject[i]) {
            _dispose(mb->WireObject[i]);
            mb->WireObject[i]   = NULL;
            mb->WireSkeleton[i] = NULL;
        };
    };

    /*** Struktur ungültig markieren ***/
    mb->Status = MBSTATUS_INVALID;
    ywd->Level->Status = LEVELSTAT_SHELL;
    
    _LogMsg("-> Debriefing left...\n");    
}

/*-----------------------------------------------------------------*/
BOOL yw_InitDebriefing(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert das Debriefing für den zuletzt
**      gespielten Level.
**
**  CHANGED
**      26-Aug-97   floh    created
**      01-Sep-97   floh    TypeMap und OwnerMap werden nicht mehr
**                          reingeladen, sondern es werden die
**                          TypeMapBU und OwnerMapBU Backup-Objekte
**                          verwendet, die in YWM_ADVANCEDCREATELEVEL
**                          erzeugt werden.
**      02-Sep-97   floh    + Wireframes werden geladen
**      03-Sep-97   floh    + HiColor Flag
**      10-Sep-97   floh    + falls es ein Netzwerk-Spiel war, werden
**                            die GlobalStats des Players gelöscht.
**      30-Sep-97   floh    + yw_ParseLDF() ersetzt durch yw_ParseLDFDebriefing(),
**                            dieses ignoriert sämtliche Prototype-Defs und
**                            überschreibt damit bei Netzwerk-Spielen die
**                            User-Settings nicht mehr.
**      06-May-98   floh    + Initialisierung der BackgroundMap
*/
{
    ULONG i;
    struct rast_rect cr;
    struct MissionBriefing *mb = &(ywd->Mission);
    memset(mb,0,sizeof(struct MissionBriefing));
    
    _LogMsg("-> Debriefing entered...\n");
        
    if (ywd->OwnerMapBU && ywd->TypeMapBU) {

        if (LEVELSTAT_FINISHED==ywd->Level->Status) mb->ZoomFromBeamGate=TRUE;
        else                                        mb->ZoomFromBeamGate=FALSE;

        /*** ClipRect initialisieren ***/
        cr.xmin = -1.0;
        cr.ymin = -1.0;
        cr.xmax = +1.0;
        cr.ymax = +1.0;
        _methoda(ywd->GfxObject,RASTM_ClipRegion,&cr);

        /*** aktuelle LevelInfos sichern ***/
        mb->Status      = MBSTATUS_INVALID;
        mb->TimerStatus = TMBSTAT_PLAYING;
        mb->TimeStamp   = 0;
        mb->ActOwner    = 1;    // FIXME: Netzwerk-Owner!?!?!?!

        /*** LevelInfo-Struktur "patchen" ***/
        ywd->Level->Status = LEVELSTAT_DEBRIEFING;

        /*** Effekt-Wireframes laden ***/
        _SetAssign("rsrc","data:");
        for (i=0; i<MBNUM_WIREFRAMES; i++) {
            mb->WireObject[i] = _new("sklt.class",RSA_Name,YPA_DBWireNames[i],TAG_DONE);
            if (mb->WireObject[i]) {
                _get(mb->WireObject[i],SKLA_Skeleton,&(mb->WireSkeleton[i]));
            };
        };

        /*** Owner- und TypeMap-Bmps ***/
        if (ywd->OwnerMapBU) {
            mb->OwnMap = yw_CopyBmpObject(ywd->OwnerMapBU,"copy2_of_ownmap");
            _get(mb->OwnMap,BMA_Bitmap,&(mb->OwnMapBmp));
        };
        if (ywd->TypeMapBU) {
            mb->TypMap = yw_CopyBmpObject(ywd->TypeMapBU,"copy2_of_typmap");
            _get(mb->TypMap,BMA_Bitmap,&(mb->TypMapBmp));
        };

        /*** HiColor Flag ***/
        mb->IsHiColor = FALSE;
        #ifdef __WINDOWS__
        if (wdd_DoDirect3D) mb->IsHiColor = TRUE;
        #endif

        /*** Level-Description-File parsen ***/
        if (yw_ParseLDFDebriefing(ywd,&(mb->LevelDesc),ywd->LevelNet->Levels[ywd->Level->Num].ldf)) {

            struct LevelDesc *ld = &(mb->LevelDesc);
            struct BGPicture *bgp;

            /*** gibt es Debriefing-Map-Definitionen? ***/
            if (ld->num_debrfmaps > 0) bgp = ld->debrfmap;
            else                       bgp = ld->brfmap;

            /*** entscheiden, welche Missionbriefing-Map laden ***/
            if (bgp[0].name[0]) {

                LONG min_dist,min_index;
                ULONG i;
                UBYTE *name;
                
                /*** lade Hintergrund-Map ***/
                _SetAssign("rsrc","levels:");
                name = &(ywd->LevelNet->DebriefMaps[0].name);
                if (name[0]) {
                    mb->BackgroundMap = _new("ilbm.class",
                                             RSA_Name,         name,
                                             BMA_Texture,      TRUE,
                                             BMA_TxtBlittable, TRUE,
                                             TAG_DONE);
                };
                
                /*** lade Debriefing-Map ***/                
                _SetAssign("rsrc","mbpix:");
                name = bgp[0].name;
                mb->MissionMap = _new("ilbm.class",
                                      RSA_Name,         name,
                                      BMA_Texture,      TRUE,
                                      BMA_TxtBlittable, TRUE,
                                      TAG_DONE);
                if (mb->MissionMap) {
                    _get(ywd->TypeMapBU,BMA_Bitmap,&(mb->TypeBmp));
                    ywd->MapSizeX = mb->TypeBmp->Width;
                    ywd->MapSizeY = mb->TypeBmp->Height;
                    ywd->WorldSizeX = ywd->MapSizeX * SECTOR_SIZE;
                    ywd->WorldSizeY = ywd->MapSizeY * SECTOR_SIZE;
                    mb->Status = MBSTATUS_MAPLOADED;
                    return(TRUE);
                };
            };
        };
    };

    /*** fehlerhaft zurück ***/
    yw_KillDebriefing(ywd);
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_DBMapLoaded(struct ypaworld_data *ywd,
                    struct VFMInput *ip,
                    struct MissionBriefing *mb)
/*
**  FUNCTION
**      Debriefing-Handler für MBSTATUS_MAPLOADED.
**
**  CHANGED
**      26-Aug-97   floh    created
**      23-Oct-97   floh    + Wuuusch Sound
**      16-Mar-98   floh    + u.U. waren die Start-Blit-Koordinaten
**                            ungültig
**      08-Apr-98   floh    + AudioCD-Control jetzt hier
**      05-May-98   floh    + neue Koordinaten...
*/
{
    FLOAT x_size,y_size;
    FLOAT x_pos,y_pos;
    FLOAT x_scale,y_scale;
    ULONG sec_x,sec_y;
    struct snd_cdcontrol_msg cd;

    /*** AudioTrack für Debriefing ***/
    //cd.command = SND_CD_STOP;
    //_ControlCDPlayer(&cd);
    //if( ywd->gsr && ywd->gsr->debriefingtrack ) {
    //    cd.command   = SND_CD_SETTITLE;
    //    cd.para      = ywd->gsr->debriefingtrack;
    //    cd.min_delay = ywd->gsr->debriefing_min_delay;
    //    cd.max_delay = ywd->gsr->debriefing_max_delay;
    //    _ControlCDPlayer(&cd);
    //    cd.command = SND_CD_PLAY;
    //    _ControlCDPlayer(&cd);
    //};

    /*** Vorbereitung für Map-Blitting ***/
    mb->Status    = MBSTATUS_MAPSCALING;
    mb->StartTime = mb->TimeStamp;
    _get(mb->MissionMap,BMA_Bitmap,&(mb->MapBlt.src));

    /*** Größe eines Sektors ***/
    if (ywd->MapSizeX > ywd->MapSizeY) {
        x_scale = 1.0;
        y_scale = ((FLOAT)ywd->MapSizeY)/((FLOAT)ywd->MapSizeX);
    } else if (ywd->MapSizeY > ywd->MapSizeX) {
        x_scale = ((FLOAT)ywd->MapSizeX)/((FLOAT)ywd->MapSizeY);
        y_scale = 1.0;
    } else {
        x_scale = y_scale = 1.0;
    };
    x_size = 1.0 / ((float)ywd->MapSizeX);
    y_size = 1.0 / ((float)ywd->MapSizeY);

    /*** Position des Finish-Beamgates ***/
    if (mb->ZoomFromBeamGate) {
        sec_x = ywd->Level->Gate[ywd->Level->BeamGate].sec_x;
        sec_y = ywd->Level->Gate[ywd->Level->BeamGate].sec_y;
    } else {
        sec_x = ywd->MapSizeX>>1;
        sec_y = ywd->MapSizeY>>1;
    };
    x_pos = -1.0 + (( ((float)sec_x) / ((float)ywd->MapSizeX) ) * 2.0);
    y_pos = -1.0 + (( ((float)sec_y) / ((float)ywd->MapSizeY) ) * 2.0);

    /*** Source: MapBltStart -> [-1.0,-1.0][+1.0,+1.0] ***/
    mb->MapBltStart.xmin = x_pos - x_size;
    mb->MapBltStart.ymin = y_pos - y_size;
    mb->MapBltStart.xmax = x_pos + x_size;
    mb->MapBltStart.ymax = y_pos + y_size;
    yw_ClipFloatRect(&(mb->MapBltStart));

    /*** Target: [-1.0,-1.0][+1.0,+1.0] -> MapBltEnd ***/
    mb->MapBltEnd.xmin = MID_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1) - (WIDTH_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1)*0.5*x_scale);
    mb->MapBltEnd.ymin = MID_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1) - (WIDTH_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1)*0.5*y_scale);
    mb->MapBltEnd.xmax = mb->MapBltEnd.xmin + WIDTH_OF(BOUNDRECT_MAP_X0,BOUNDRECT_MAP_X1)*x_scale; 
    mb->MapBltEnd.ymax = mb->MapBltEnd.ymin + WIDTH_OF(BOUNDRECT_MAP_Y0,BOUNDRECT_MAP_Y1)*y_scale;
    yw_ClipFloatRect(&(mb->MapBltEnd));

    /*** MapBlt Struktur initialisieren ***/
    mb->MapBlt.from = mb->MapBltStart;
    mb->MapBlt.to   = mb->MapBltEnd;    

    /*** Wuuusch Sound ***/
    if (ywd->gsr) {
        _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_OBJECTAPPEAR);
    };
}

/*-----------------------------------------------------------------*/
void yw_DBMapScaling(struct ypaworld_data *ywd,
                     struct VFMInput *ip,
                     struct MissionBriefing *mb)
/*
**  FUNCTION
**      Debriefing-Handler für MBSTATUS_MAPSCALING.
**
**  CHANGED
**      26-Aug-97   floh    created
*/
{
    struct rast_rect *r0  = &(mb->MapBltStart);
    struct rast_rect *r1  = &(mb->MapBltEnd);
    struct rast_blit *blt = &(mb->MapBlt);
    LONG dt = mb->TimeStamp - mb->StartTime;

    if (dt < MBT_DUR_MAPSCALE) {

        FLOAT scl = ((FLOAT)dt)/MBT_DUR_MAPSCALE;

        /*** Blitkoordinaten interpolieren ***/
        blt->from.xmin = r0->xmin + ((-1.0 - r0->xmin)*scl);
        blt->from.ymin = r0->ymin + ((-1.0 - r0->ymin)*scl);
        blt->from.xmax = r0->xmax + ((+1.0 - r0->xmax)*scl);
        blt->from.ymax = r0->ymax + ((+1.0 - r0->ymax)*scl);
        mb->MapBlt.to = mb->MapBltEnd;

    } else {
        /*** fertig skaliert, To-Blt-Koord fixieren ***/
        mb->MapBlt.from.xmin = -1.0;
        mb->MapBlt.from.ymin = -1.0;
        mb->MapBlt.from.xmax = +1.0;
        mb->MapBlt.from.ymax = +1.0;
        mb->MapBlt.to = mb->MapBltEnd;
        mb->Status = MBSTATUS_MAPDONE;
    };
}

/*-----------------------------------------------------------------*/
void yw_DBMapDone(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  FUNCTION
**      Debriefing-Handler für MBSTATUS_MAPDONE.
**
**  CHANGED
**      26-Aug-97   floh    created
*/
{
    mb->Status = MBSTATUS_L1_START;
}

/*-----------------------------------------------------------------*/
void yw_Score(struct ypaworld_data *ywd, UBYTE *inst,
              struct ypa_PlayerStats *stat)
/*
**  FUNCTION
**      Updated <stats> je nach <inst>.
**
**  CHANGED
**      08-Sep-97   floh    created
**      19-May-98   floh    + YPAHIST_TECHUPGRADE ist neu
**                          + Scoring jetzt "richtig"
*/
{
    UBYTE cmd = *inst;

    switch (cmd) {
        case YPAHIST_CONSEC:
            {
                struct ypa_HistConSec *hcs_inst = (struct ypa_HistConSec *) inst;
                ULONG owner = hcs_inst->new_owner;
                stat[owner].SecCons++;
                stat[owner].Score += SCORE_SECTOR;
            };
            break;
            
        case YPAHIST_VHCLKILL:
            {
                struct ypa_HistVhclKill *hvk_inst = (struct ypa_HistVhclKill *) inst;
                ULONG killer_owner = (hvk_inst->owners>>3) & 0x7;
                ULONG victim_owner = (hvk_inst->owners) & 0x7;
                ULONG code = hvk_inst->owners & 0xC0;                
                stat[killer_owner].Kills++;
                if (code == 0x80) {
                    /*** ein User hat eine AI gekillt ***/
                    stat[killer_owner].Score += SCORE_USERAIKILL;
                    stat[killer_owner].UserKills++;
                    _LogMsg("-> User AI Kill scored for %d.\n",killer_owner);
                } else if (code == 0xC0) {
                    /*** User hat einen User gekillt ***/
                    stat[killer_owner].Score += SCORE_USERUSERKILL;
                    stat[killer_owner].UserKills++;
                    _LogMsg("-> User User Kill scored for %d.\n",killer_owner);
                } else {
                    /*** AI/AI-, oder AI hat einen User gekillt ***/
                    stat[killer_owner].Score += SCORE_AIAIKILL;
                };                
                if (hvk_inst->vp & (1<<15)) {
                    /*** Opfer war ein Robo ***/
                    stat[killer_owner].Score += SCORE_HOSTKILL;
                    _LogMsg("-> Host Station Kill scored for %d.\n",killer_owner);
                };
            };
            break;

        case YPAHIST_VHCLCREATE:
            break;

        case YPAHIST_POWERSTATION:
            {
                struct ypa_HistConSec *hcs_inst = (struct ypa_HistConSec *) inst;
                ULONG owner = hcs_inst->new_owner;
                stat[owner].Power++;
                stat[owner].Score += SCORE_POWERSTATION;
                _LogMsg("-> Power station scored for %d.\n",owner);
            };
            break;

        case YPAHIST_TECHUPGRADE:
            {
                struct ypa_HistTechUpgrade *hcs_inst = (struct ypa_HistTechUpgrade *) inst;
                ULONG new_owner = hcs_inst->new_owner;
                stat[new_owner].Techs++;
                stat[new_owner].Score += SCORE_TECHUPGRADE;
                _LogMsg("-> Tech upgrade scored for %d.\n",new_owner);
            };
            break;
    };
}

/*-----------------------------------------------------------------*/
void yw_DBGetSecSize(struct ypaworld_data *ywd,
                     struct MissionBriefing *mb,
                     FLOAT *x_size, FLOAT *y_size)
/*
**  FUNCTION
**      Returniert die Größe eines Debriefing-Sektors in
**      Screen-Koordinaten. Das Ergebnis wird nach <x_size>
**      und <y_size> zurückgeschrieben.
**
**  CHANGED
**      02-Sep-97   floh    created
*/
{
    *x_size = (FLOAT)((mb->MapBltEnd.xmax-mb->MapBltEnd.xmin)/ywd->MapSizeX);
    *y_size = (FLOAT)((mb->MapBltEnd.ymax-mb->MapBltEnd.ymin)/ywd->MapSizeY);
}

/*-----------------------------------------------------------------*/
void yw_DBGetSecPos(struct ypaworld_data *ywd,
                    struct MissionBriefing *mb,
                    ULONG sec_x, ULONG sec_y,
                    FLOAT x_size, FLOAT y_size,
                    FLOAT *x_pos, FLOAT *y_pos)
/*
**  FUNCTION
**      Returniert den Mittelpunkt eines Debriefing
**      Sektors in Screenkoordinaten.
**
**  CHANGED
**      02-Sep-97   floh    created
*/
{
    *x_pos = mb->MapBltEnd.xmin + sec_x*x_size + x_size*0.5;
    *y_pos = mb->MapBltEnd.ymin + sec_y*y_size + y_size*0.5;
}

/*-----------------------------------------------------------------*/
void yw_DBRenderOwners(struct ypaworld_data *ywd,
                       struct MissionBriefing *mb)
/*
**  FUNCTION
**      Rendert den aktuellen Zustand der OwnerMap in
**      die Debriefing-Map.
**
**  CHANGED
**      01-Sep-97   floh    created
**      22-Apr-98   floh    + Linienfarben waren noch nicht an neues
**                            Colorhandling angepasst
*/
{
    UBYTE *own_map;
    ULONG secx,secy;
    FLOAT x_pos,y_pos;
    FLOAT x_size,y_size;
    FLOAT tile_xsize, tile_ysize;

    /*** zuerst alle Sektoren in Fullsize rendern ***/
    own_map = mb->OwnMapBmp->Data;
    _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);

    /*** Größe und Startposition eines Sektors in Float ***/
    yw_DBGetSecSize(ywd,mb,&x_size,&y_size);
    tile_xsize = x_size/10.0;
    tile_ysize = y_size/10.0;
    for (secy=0,y_pos=mb->MapBltEnd.ymin+y_size*0.5; secy<ywd->MapSizeY; secy++,y_pos+=y_size) {
        for (secx=0,x_pos=mb->MapBltEnd.xmin+x_size*0.5; secx<ywd->MapSizeX; secx++,x_pos+=x_size) {

            UBYTE owner = *own_map++;

            /*** neutrale Sektoren werden ignoriert ***/
            if ((owner != 0) &&
                (secx != 0) && (secy != 0) &&
                (secx != (ywd->MapSizeX-1)) &&
                (secy != (ywd->MapSizeY-1)))
            {
                struct rast_line l0;
                struct rast_line l1;
                struct rast_pens pens;
                ULONG color;

                l0.x0 = x_pos-tile_xsize; l0.y0 = y_pos;
                l0.x1 = x_pos+tile_xsize; l0.y1 = y_pos;
                l1.x0 = x_pos; l1.y0 = y_pos-tile_ysize;
                l1.x1 = x_pos; l1.y1 = y_pos+tile_ysize;

                /*** Pens ***/
                color = yw_GetColor(ywd,YPACOLOR_OWNER_0+owner);                
                pens.fg_pen  = color;
                pens.fg_apen = color;
                pens.bg_pen  = -1;
                _methoda(ywd->GfxObject,RASTM_SetPens,&pens);
                _methoda(ywd->GfxObject,RASTM_Line,&l0);
                _methoda(ywd->GfxObject,RASTM_Line,&l1);
            };

        };
    };
    _methoda(ywd->GfxObject,RASTM_End2D,NULL);
}

/*-----------------------------------------------------------------*/
void yw_DBHandleConSec(struct ypaworld_data *ywd,
                       struct MissionBriefing *mb,
                       struct ypa_HistConSec *act_inst,
                       LONG real_time, LONG scan_time)
/*
**  CHANGED
**      27-Aug-97   floh    created
**      23-Oct-97   floh    + Sound
**      22-Apr-98   floh    + Linienfarben noch nicht an neues
**                            Colorhandling angepasst
*/
{
    ULONG sec_x = act_inst->sec_x;
    ULONG sec_y = act_inst->sec_y;
    ULONG owner = act_inst->new_owner;
    UBYTE *own_map = mb->OwnMapBmp->Data;
    LONG dt = real_time - scan_time;

    /*** Sektor in die Ownermap eintragen und Score hochzählen ***/
    own_map[mb->OwnMapBmp->Width * sec_y + sec_x] = owner;
    if (scan_time == mb->ActFrameTimeStamp) {
        yw_Score(ywd, (UBYTE *) act_inst, mb->LocalStats);
        if (ywd->gsr) {
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_SECTORCONQUERED);
        };
    };

    /*** neue Sektoren "nachglühen" lassen ***/
    if ((dt < MBFXTIME_CONSEC) && (owner != 0)) {
        struct Skeleton *sklt;
        if (sklt = mb->WireSkeleton[MBWIRE_CONSEC]) {

            ULONG color;
            FLOAT scale;
            FLOAT x_pos,y_pos,x_size,y_size;
            FLOAT tx,ty,sx,sy,m11,m12,m21,m22;

            yw_DBGetSecSize(ywd,mb,&x_size,&y_size);
            yw_DBGetSecPos(ywd,mb,sec_x,sec_y,x_size,y_size,&x_pos,&y_pos);
            scale = 1.0 - (((FLOAT)dt) / MBFXTIME_CONSEC);
            if (scale < 0.0)      scale = 0.0;
            else if (scale > 1.0) scale = 1.0;
            tx = x_pos;
            ty = y_pos;
            sx = x_size * scale;
            sy = y_size * scale;
            m11 = 1.0; m12 = 0.0;
            m21 = 0.0; m22 = 1.0;
            color = yw_GetColor(ywd,YPACOLOR_OWNER_0+owner);                
            yw_VectorOutline(ywd,sklt,tx,ty,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_DBHandleVhclKill(struct ypaworld_data *ywd,
                         struct MissionBriefing *mb,
                         struct ypa_HistVhclKill *act_inst,
                         LONG real_time, LONG scan_time)
/*
**  CHANGED
**      27-Aug-97   floh    created
**      23-Oct-97   floh    + Sound
*/
{
    ULONG victim_owner = (act_inst->owners) & 0x7;
    FLOAT pos_x = act_inst->pos_x / 256.0;
    FLOAT pos_y = act_inst->pos_z / 256.0;
    LONG dt = real_time - scan_time;

    /*** Score updaten ***/
    if (scan_time == mb->ActFrameTimeStamp) {
        yw_Score(ywd, (UBYTE *) act_inst, mb->LocalStats);
        if (ywd->gsr) {
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_VHCLDESTROYED);
        };

    };

    /*** Explosion visualisieren ***/
    if (dt < (4*MBFXTIME_VHCLKILL)) {
        struct Skeleton *sklt;
        ULONG vp_num;        
        struct VehicleProto *vp;        
        
        /*** Wireframe ermitteln ***/
        //vp_num = act_inst->vp & ~(1<<15);
        //vp = &(ywd->VP_Array[vp_num]);
        //if (vp->wireframe_object) _get(vp->wireframe_object,SKLA_Skeleton,&sklt);
        //else sklt = mb->WireSkeleton[MBWIRE_VHCLKILL];
        
        sklt = mb->WireSkeleton[MBWIRE_VHCLKILL];
        if (sklt) {

            ULONG color;
            FLOAT scale;
            FLOAT x_pos,y_pos,x_size,y_size;
            FLOAT tx,ty,sx,sy,m11,m12,m21,m22;

            yw_DBGetSecSize(ywd,mb,&x_size,&y_size);
            x_size *= 0.5;
            y_size *= 0.5;
            x_pos = mb->MapBltEnd.xmin + ((mb->MapBltEnd.xmax-mb->MapBltEnd.xmin)*pos_x);
            y_pos = mb->MapBltEnd.ymin + ((mb->MapBltEnd.ymax-mb->MapBltEnd.ymin)*pos_y);
            if (dt < MBFXTIME_VHCLKILL) {
                scale = 1.0 - (((FLOAT)dt) / MBFXTIME_VHCLKILL);
                if (scale < 0.1)      scale = 0.1;
                else if (scale > 1.0) scale = 1.0;
            } else {
                /*** restliche Zeit auf voller Größe lassen ***/
                scale = 0.1;
            };
            tx = x_pos;
            ty = y_pos;
            sx = x_size * scale;
            sy = y_size * scale;
            m11 = 1.0; m12 = 0.0;
            m21 = 0.0; m22 = 1.0;
            color = yw_GetColor(ywd,YPACOLOR_OWNER_0+victim_owner);                
            yw_VectorOutline(ywd,sklt,tx,ty,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_DBHandleVhclCreate(struct ypaworld_data *ywd,
                           struct MissionBriefing *mb,
                           struct ypa_HistVhclCreate *act_inst,
                           LONG real_time, LONG scan_time)
/*
**  CHANGED
**      27-Aug-97   floh    created
**      23-Oct-97   floh    + Sound
**      22-Apr-98   floh    + Linienfarben waren noch nicht an
**                            neues Colorhandling angepasst
*/
{
    ULONG owner = act_inst->owner;
    FLOAT pos_x = act_inst->pos_x / 256.0;
    FLOAT pos_y = act_inst->pos_z / 256.0;
    LONG dt = real_time - scan_time;

    /*** Genesis visualisieren ***/
    if (dt < MBFXTIME_VHCLCREATE) {
        struct Skeleton *sklt;
        if (sklt = mb->WireSkeleton[MBWIRE_VHCLKILL]) {

            ULONG color;
            FLOAT scale;
            FLOAT x_pos,y_pos,x_size,y_size;
            FLOAT tx,ty,sx,sy,m11,m12,m21,m22;

            yw_DBGetSecSize(ywd,mb,&x_size,&y_size);
            x_size *= 0.125;
            y_size *= 0.125;
            x_pos = mb->MapBltEnd.xmin + ((mb->MapBltEnd.xmax-mb->MapBltEnd.xmin)*pos_x);
            y_pos = mb->MapBltEnd.ymin + ((mb->MapBltEnd.ymax-mb->MapBltEnd.ymin)*pos_y);
            if (dt < MBFXTIME_VHCLCREATE) {
                scale = ((FLOAT)dt) / MBFXTIME_VHCLCREATE;
                if (scale < 0.0)      scale = 0.0;
                else if (scale > 1.0) scale = 1.0;
            };
            tx = x_pos;
            ty = y_pos;
            sx = x_size * scale;
            sy = y_size * scale;
            m11 = 1.0; m12 = 0.0;
            m21 = 0.0; m22 = 1.0;
            color = yw_GetColor(ywd,YPACOLOR_OWNER_0+owner);                
            yw_VectorOutline(ywd,sklt,tx,ty,m11,m12,m21,m22,sx,sy,color,NULL,NULL);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_DBHandlePowerStation(struct ypaworld_data *ywd,
                             struct MissionBriefing *mb,
                             struct ypa_HistConSec *act_inst,
                             LONG real_time, LONG scan_time)
/*
**  CHANGED
**      09-Sep-97   floh    created
**      23-Oct-97   floh    + Sound
*/
{
    /*** Score updaten ***/
    if (scan_time == mb->ActFrameTimeStamp) {
        yw_Score(ywd, (UBYTE *) act_inst, mb->LocalStats);
        if (ywd->gsr) {
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BLDGCONQUERED);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_DBRegisterTechupgrade(struct ypaworld_data *ywd,
                              struct MissionBriefing *mb,
                              struct ypa_HistTechUpgrade *act_inst) 
/*
**  CHANGED
**      19-May-98   floh    created
*/
{
    ULONG type   = act_inst->type;
    ULONG vp_num = act_inst->vp_num;
    ULONG bp_num = act_inst->bp_num;
    ULONG wp_num = act_inst->wp_num;
    ULONG exists = FALSE;
    ULONG i;    
    
    /*** existiert dieses Techupgrade schon? ***/
    for (i=0; i<mb->ActTechUpgrade; i++) {
        struct DBTechUpgrade *dbtu = &(mb->TechUpgrades[i]);
        if ((dbtu->type == type) &&
            (dbtu->vp_num == vp_num) &&
            (dbtu->bp_num == bp_num) &&
            (dbtu->wp_num == wp_num))
        { 
            exists = TRUE;
            break;
        };
    };
    if (!exists) {
        /*** existiert noch nicht, hinten ran damit ***/
        struct DBTechUpgrade *dbtu;
        if (mb->ActTechUpgrade >= DB_MAXNUM_TECHUPGRADES) {
            mb->ActTechUpgrade = DB_MAXNUM_TECHUPGRADES-1;
        };
        dbtu = &(mb->TechUpgrades[mb->ActTechUpgrade]);
        dbtu->type   = type;
        dbtu->vp_num = vp_num;
        dbtu->bp_num = bp_num;
        dbtu->wp_num = wp_num;
        mb->ActTechUpgrade++;
    };
}        

/*-----------------------------------------------------------------*/
void yw_DBHandleTechUpgrade(struct ypaworld_data *ywd,
                            struct MissionBriefing *mb,
                            struct ypa_HistTechUpgrade *act_inst,
                            LONG real_time, LONG scan_time)
/*
**  CHANGED
**      09-Sep-97   floh    created
**      23-Oct-97   floh    + Sound
*/
{
    /*** Score updaten ***/
    if (scan_time == mb->ActFrameTimeStamp) {
        yw_Score(ywd, (UBYTE *) act_inst, mb->LocalStats);
        yw_DBRegisterTechupgrade(ywd,mb,act_inst);
        if (ywd->gsr) {
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BLDGCONQUERED);
        };
    };
}

/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutKillsTitle(struct ypaworld_data *ywd,
                             struct MissionBriefing *mb,
                                    UBYTE *str, LONG w)
/*
**  CHANGED
**      08-May-98   floh    created
**      18-May-98   floh    + Texte localized
*/
{
    struct ypa_ColumnItem col[3];
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Green(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_DEBRIEFING));    

    /*** Column-Layout initialisieren ***/        
    col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_KILLS,"KILLS");
    col[0].width        = w * 0.4;
    col[0].font_id      = FONTID_TRACY;
    col[0].space_chr    = ' ';
    col[0].prefix_chr   = 0;
    col[0].postfix_chr  = 0;
    col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
     
    col[1].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_KILLSBYPLAYER,"BY PLAYER");
    col[1].width        = w * 0.3;
    col[1].font_id      = FONTID_TRACY;
    col[1].space_chr    = ' ';
    col[1].prefix_chr   = 0;
    col[1].postfix_chr  = 0;
    col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;

    col[2].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_KILLSALL,"ALL");
    col[2].width        = w * 0.3;
    col[2].font_id      = FONTID_TRACY;
    col[2].space_chr    = ' ';
    col[2].prefix_chr   = 0;
    col[2].postfix_chr  = 0;
    col[2].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
    str = yw_BuildColumnItem(ywd,str,3,col);
    
    return(str);
}

/*-----------------------------------------------------------------*/
int __dbkills_qsort_hook(struct __dbkills_qsort_struct *elm1,
                         struct __dbkills_qsort_struct *elm2)
{
    return(elm2->kills-elm1->kills);
}
/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutKills(struct ypaworld_data *ywd,
                        struct MissionBriefing *mb,
                        UBYTE *str, LONG w)
/*
**  FUNCTION
**      Layoutet den Multiplayer und SinglePlayer-Score.
**
**  CHANGED
**      08-May-98   floh    created
*/
{

    struct __dbkills_qsort_struct kills_array[MAXNUM_OWNERS];
    ULONG i,num_owners;    
    
    /*** initialisiere und sortiere Owner-Array ***/    
    num_owners=0;
    for (i=0; i<MAXNUM_OWNERS; i++) {
        if (ywd->Level->OwnerMask & (1<<i)) {
            kills_array[num_owners].owner=i;
            kills_array[num_owners].kills=mb->LocalStats[i].Kills;
            num_owners++;
        };
    };
    qsort(kills_array,num_owners,sizeof(struct __dbkills_qsort_struct),__dbkills_qsort_hook);
    
    /*** layoute jede Zeile ***/
    for (i=0; i<num_owners; i++) {
        UBYTE *name;
        ULONG c_index;
        struct ypa_ColumnItem col[3];
        UBYTE buf_0[32],buf_1[32],buf_2[32];
        switch(kills_array[i].owner) {
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
        col[0].width        = w * 0.4;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
         
        if (ywd->WasNetworkSession || (kills_array[i].owner==1)) sprintf(buf_0,"%d",mb->LocalStats[kills_array[i].owner].UserKills);
        else                                                     sprintf(buf_0,"-");
        col[1].string       = buf_0;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;

        sprintf(buf_1,"%d",mb->LocalStats[kills_array[i].owner].Kills);         
        col[2].string       = buf_1;
        col[2].width        = w * 0.3;
        col[2].font_id      = FONTID_TRACY;
        col[2].space_chr    = ' ';
        col[2].prefix_chr   = 0;
        col[2].postfix_chr  = 0;
        col[2].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;

        str = yw_BuildColumnItem(ywd,str,3,col);
        new_line(str);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutScoreTitle(struct ypaworld_data *ywd,
                             struct MissionBriefing *mb,
                             UBYTE *str, LONG w)
/*
**  CHANGED
**      08-May-98   floh    created
*/
{
    struct ypa_ColumnItem col[1];
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Green(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_DEBRIEFING));    

    /*** Column-Layout initialisieren ***/        
    col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_SCORE,"SCORE");
    col[0].width        = w;
    col[0].font_id      = FONTID_TRACY;
    col[0].space_chr    = ' ';
    col[0].prefix_chr   = 0;
    col[0].postfix_chr  = 0;
    col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
    str = yw_BuildColumnItem(ywd,str,1,col);
    return(str);
}

/*-----------------------------------------------------------------*/
int __dbscore_qsort_hook(struct __dbscore_qsort_struct *elm1,
                         struct __dbscore_qsort_struct *elm2)
{
    return(elm2->score-elm1->score);
}
/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutScore(struct ypaworld_data *ywd,
                        struct MissionBriefing *mb,
                        UBYTE *str, LONG w)
/*
**  FUNCTION
**      Layoutet den Multiplayer und SinglePlayer-Score.
**
**  CHANGED
**      08-May-98   floh    created
**      19-May-98   floh    + scored jetzt richtig
*/
{
    if (ywd->WasNetworkSession) {
    
        struct __dbscore_qsort_struct score_array[MAXNUM_OWNERS];
        ULONG i,num_owners;    

        /*** Score-Title layouten ***/
        str = yw_DBLayoutScoreTitle(ywd,mb,str,w);
        new_line(str);
        
        /*** initialisiere und sortiere Owner-Array ***/    
        num_owners=0;
        for (i=0; i<MAXNUM_OWNERS; i++) {
            if (ywd->Level->OwnerMask & (1<<i)) {
                score_array[num_owners].owner=i;
                score_array[num_owners].score=mb->LocalStats[i].Score;
                num_owners++;
            };
        };
        qsort(score_array,num_owners,sizeof(struct __dbscore_qsort_struct),__dbscore_qsort_hook);
        
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
             
            sprintf(buf_0,"%d",mb->LocalStats[score_array[i].owner].Score);         
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
    } else {

        struct ypa_ColumnItem col[2];
        UBYTE buf_0[32];

        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Green(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_DEBRIEFING));    

        /*** Score Overall ***/
        col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_SCORETHISMISSION,"SCORE THIS MISSION:");
        col[0].width        = w * 0.7;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        sprintf(buf_0,"%d",mb->LocalStats[1].Score);
        col[1].string       = buf_0;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        str = yw_BuildColumnItem(ywd,str,2,col);
        new_line(str);

        /*** Score Overall ***/
        col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_SCOREOVERALL,"SCORE OVERALL:");
        col[0].width        = w * 0.7;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        sprintf(buf_0,"%d",mb->GlobalStats[1].Score + mb->LocalStats[1].Score);
        col[1].string       = buf_0;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        str = yw_BuildColumnItem(ywd,str,2,col);
        new_line(str);
    };                
        
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutTime(struct ypaworld_data *ywd,
                       struct MissionBriefing *mb,
                       UBYTE *str, LONG w)
/*
**  CHANGED
**      08-May-98   floh    created
**      19-May-98   floh    + muesste jetzt GlobalTime korrekt anzeigen
*/
{
    if (ywd->WasNetworkSession) {

        struct ypa_ColumnItem col[2];
        UBYTE buf_0[32];
        LONG secs,mins,hours;

        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Green(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_DEBRIEFING));    

        col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_PLAYINGTIME,"PLAYING TIME:");
        col[0].width        = w * 0.7;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;

        secs  = mb->ActFrameTimeStamp>>10;
        mins  = secs / 60;
        hours = mins / 60;
        sprintf(buf_0,"%02d:%02d:%02d",hours,mins%60,secs%60);
        col[1].string       = buf_0;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        str = yw_BuildColumnItem(ywd,str,2,col);
        new_line(str);

    } else {

        struct ypa_ColumnItem col[2];
        UBYTE buf_0[32];
        LONG secs,mins,hours;

        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Green(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_DEBRIEFING));    

        /*** Playing Time This Level ***/
        col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TIMETHISMISSION,"PLAYING TIME THIS MISSION:");
        col[0].width        = w * 0.7;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;

        secs  = mb->ActFrameTimeStamp>>10;
        mins  = secs / 60;
        hours = mins / 60;
        sprintf(buf_0,"%02d:%02d:%02d",hours,mins%60,secs%60);
        col[1].string       = buf_0;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        str = yw_BuildColumnItem(ywd,str,2,col);
        new_line(str);

        /*** Playing Time Overall ***/
        col[0].string       = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TIMEOVERALL,"PLAYING TIME OVERALL:");
        col[0].width        = w * 0.7;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        secs  = (mb->GlobalStats[1].Time + mb->ActFrameTimeStamp)>>10;
        mins  = secs / 60;
        hours = mins / 60;
        sprintf(buf_0,"%02d:%02d:%02d",hours,mins%60,secs%60);
        col[1].string       = buf_0;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        str = yw_BuildColumnItem(ywd,str,2,col);
        new_line(str);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutScoreTable(struct ypaworld_data *ywd,
                             struct MissionBriefing *mb,
                             UBYTE *str)
/*
**  FUNCTION
**      Rendert die Scores-Tabelle.
**
**  CHANGED
**      03-Sep-97   floh    created
**      05-Sep-97   floh    + Sector-Conquer-Counter
**      08-Sep-97   floh    + Blittet jetzt die Rassen-Symbole korrekt
**      09-Sep-97   floh    + Score flog raus, Powerstations und
**                            Techupgrades rein.
**      08-May-98   floh    + Score Layout umgearbeitet.
*/
{
    WORD x,y,w,h;
    x = BOUNDRECT_TEXT_X0 * (ywd->DspXRes>>1);
    y = BOUNDRECT_TEXT_Y0 * (ywd->DspYRes>>1);
    w = (BOUNDRECT_TEXT_X1-BOUNDRECT_TEXT_X0) * (ywd->DspXRes>>1);
    h = (BOUNDRECT_TEXT_Y1-BOUNDRECT_TEXT_Y0) * (ywd->DspYRes>>1);
    
    /*** und los ***/
    new_font(str,FONTID_TRACY);
    pos_abs(str,x,y);
    
    /*** Kills Block ***/
    str = yw_DBLayoutKillsTitle(ywd,mb,str,w);
    new_line(str);
    str = yw_DBLayoutKills(ywd,mb,str,w);
    new_line(str);
    
    /*** Multiplayer Score Block ***/
    str = yw_DBLayoutScore(ywd,mb,str,w);
    new_line(str);
    
    /*** Playing Time ***/
    str = yw_DBLayoutTime(ywd,mb,str,w);    
    
    /*** dat war's ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutSingleTechupgrade(struct ypaworld_data *ywd,
                                    struct MissionBriefing *mb,
                                    struct DBTechUpgrade *dbtu,
                                    UBYTE *str, LONG w)
/*
**  CHANGED
**      19-May-98   floh    created
**      26-May-98   floh    + Gebaeude-Namen Multiplayer-tauglich gemacht...
**      09-Jun-98   floh    + Weapon-Techupgrades wurden nicht immer richtig
**                            angezeigt
**      28-Jun-98   floh    + Combined Techupgrades wurden falsch angezeigt.
*/
{
    UBYTE buf_0[256];
    UBYTE *type_str, *name_str, *value_str;
    LONG type   = dbtu->type;
    LONG vp_num = dbtu->vp_num;
    LONG bp_num = dbtu->bp_num;
    LONG wp_num = dbtu->wp_num;
    ULONG i;
    struct VehicleProto *vp = NULL;
    struct WeaponProto  *wp = NULL;
    struct BuildProto   *bp = NULL;    
    
    /*** zu einem Weapon-Upgrade das Vehikel suchen? ***/
    if ((!vp_num) && wp_num) {
        for (i=0; i<NUM_VEHICLEPROTOS; i++) {
            if (ywd->VP_Array[i].Weapons[0] == wp_num) {
                vp_num = i;
                break;
            };
        };
        if (!vp_num) {
            /*** diese Waffe verwendet niemand ***/
            return(str);
        };
    };
    
    /*** zu einem Weapon-Upgrade die Waffe suchen? ***/
    if ((!wp_num) && vp_num) {
        wp_num = ywd->VP_Array[vp_num].Weapons[0];
        if (wp_num == -1) wp_num=0;
    };    

    /*** zugehoerige Prototype-Pointer ***/    
    if (vp_num) vp = &(ywd->VP_Array[vp_num]);
    if (bp_num) bp = &(ywd->BP_Array[bp_num]);
    if (wp_num) wp = &(ywd->WP_Array[wp_num]);

    /*** WICHTIG: alle Strings als Leerstring initialisieren ***/
    type_str  = " ";
    value_str = " ";
    name_str  = " ";
        
    /*** name_str ***/
    if (vp)      name_str = ypa_GetStr(ywd->LocHandle,STR_NAME_VEHICLES+vp_num,vp->Name);
    else if (bp) {
        if (ywd->WasNetworkSession) name_str = ypa_GetStr(ywd->LocHandle,STR_NAME_NETWORK_BUILDINGS+bp_num,bp->Name);
        else                        name_str = ypa_GetStr(ywd->LocHandle,STR_NAME_BUILDINGS+bp_num,bp->Name);
    };    
    
    /*** type_str und value_str ***/
    switch(dbtu->type) {
        case YPAHIST_TECHTYPE_WEAPON:
            if (wp && vp) {
                type_str  = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_WEAPON,"WEAPON UPGRADE:");
                if (vp->NumWeapons > 1) {
                    sprintf(buf_0,"(%d x%d)", wp->Energy/100, vp->NumWeapons);
                } else {
                    sprintf(buf_0,"(%d)", wp->Energy/100);
                };
                value_str = buf_0;
            };
            break;
            
        case YPAHIST_TECHTYPE_ARMOR:
            if (vp) {
                type_str = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_ARMOR,"ARMOR UPGRADE:");
                sprintf(buf_0,"%d%%", vp->Shield);
                value_str = buf_0;
            };
            break;
            
        case YPAHIST_TECHTYPE_VEHICLE:
            if (vp) {
                type_str  = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_VEHICLE,"NEW VEHICLE TECH:");
            };
            break;
            
        case YPAHIST_TECHTYPE_BUILDING:
            if (bp) {
                type_str = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_BUILDING,"NEW BUILDING TECH:");
            };
            break;
            
        case YPAHIST_TECHTYPE_RADAR:
            if (vp) {
                type_str = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_RADAR,"RADAR UPGRADE:");
            };
            break;
            
        case YPAHIST_TECHTYPE_BUILDANDVEHICLE:
            if (vp && bp) {
                UBYTE *name;
                if (ywd->WasNetworkSession) name = ypa_GetStr(ywd->LocHandle,STR_NAME_NETWORK_BUILDINGS+bp_num,bp->Name);
                else                        name = ypa_GetStr(ywd->LocHandle,STR_NAME_BUILDINGS+bp_num,bp->Name);
                type_str = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_BUILDANDVEHICLE,"COMBINED UPGRADE:");
                sprintf(buf_0,"%s",name);
                value_str = buf_0;
            };
            break;
            
        case YPAHIST_TECHTYPE_GENERIC:
            type_str = ypa_GetStr(ywd->LocHandle,STR_DEBRIEF_TU_GENERIC,"GENERIC TECH UPGRADE");
            break;
    };                        
        
    /*** type_str Spalte ***/
    if (type_str && name_str && value_str) {        

        struct ypa_ColumnItem col[3];

        col[0].string       = type_str;
        col[0].width        = w * 0.5;
        col[0].font_id      = FONTID_TRACY;
        col[0].space_chr    = ' ';
        col[0].prefix_chr   = 0;
        col[0].postfix_chr  = 0;
        col[0].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        col[1].string       = name_str;
        col[1].width        = w * 0.3;
        col[1].font_id      = FONTID_TRACY;
        col[1].space_chr    = ' ';
        col[1].prefix_chr   = 0;
        col[1].postfix_chr  = 0;
        col[1].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;
        
        col[2].string       = value_str;
        col[2].width        = w * 0.2;
        col[2].font_id      = FONTID_TRACY;
        col[2].space_chr    = ' ';
        col[2].prefix_chr   = 0;
        col[2].postfix_chr  = 0;
        col[2].flags        = YPACOLF_TEXT|YPACOLF_ALIGNLEFT;

        str = yw_BuildColumnItem(ywd,str,3,col);
        new_line(str);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_DBLayoutTechUpgrades(struct ypaworld_data *ywd,
                               struct MissionBriefing *mb,
                               UBYTE *str)
/*
**  FUNCTION
**      Layoutet die eroberten Techupgrades im unteren
**      linken Text-Block.
**
**  CHANGED
**      19-May-98   floh    created
*/
{
    WORD x,y,w,h;
    ULONG i;
    x = BOUNDRECT_SLOT_X0 * (ywd->DspXRes>>1);
    y = BOUNDRECT_SLOT_Y0 * (ywd->DspYRes>>1);
    w = (BOUNDRECT_SLOT_X1-BOUNDRECT_SLOT_X0) * (ywd->DspXRes>>1);
    h = (BOUNDRECT_SLOT_Y1-BOUNDRECT_SLOT_Y0) * (ywd->DspYRes>>1);
    
    new_font(str,FONTID_TRACY);
    pos_abs(str,x,y);
    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Green(ywd,YPACOLOR_TEXT_DEBRIEFING),yw_Blue(ywd,YPACOLOR_TEXT_DEBRIEFING));    
    
    for (i=0; i<mb->ActTechUpgrade; i++) {
        struct DBTechUpgrade *dbtu = &(mb->TechUpgrades[i]);
        str = yw_DBLayoutSingleTechupgrade(ywd,mb,dbtu,str,w);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_DBPutTitles(struct ypaworld_data *ywd,struct MissionBriefing *mb)
/*
**  CHANGED
**      19-Jun-98   floh    created
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
void yw_DBL1Start(struct ypaworld_data *ywd,
                  struct VFMInput *ip,
                  struct MissionBriefing *mb)
/*
**  CHANGED
**      26-Aug-97   floh    created
**      03-Sep-97   floh    + PlayerStats werden initialisiert
*/
{
    struct VFMBitmap *src_bmp;
    struct VFMBitmap *tar_bmp;

    mb->StartTime = mb->TimeStamp;
    mb->Status    = MBSTATUS_L1_RUNNING;
    mb->ActFrameTimeStamp = 0;

    /*** kopiere OwnerMaps in die eigenen Kopie-Objekte ***/
    _get(ywd->OwnerMapBU,BMA_Bitmap,&src_bmp);
    _get(mb->OwnMap,BMA_Bitmap,&tar_bmp);
    memcpy(tar_bmp->Data,src_bmp->Data,src_bmp->Width*src_bmp->Height);
    _get(ywd->TypeMapBU,BMA_Bitmap,&src_bmp);
    _get(mb->TypMap,BMA_Bitmap,&tar_bmp);
    memcpy(tar_bmp->Data,src_bmp->Data,src_bmp->Width*src_bmp->Height);

    /*** PlayerStats initialisieren ***/
    memcpy(&(mb->GlobalStats),&(ywd->GlobalStats),sizeof(mb->GlobalStats));
    memset(&(mb->LocalStats),0,sizeof(mb->LocalStats));

    /*** Techupgrade-Stack zuruecksetzen ***/
    mb->ActTechUpgrade = 0;
    memset(&(mb->TechUpgrades),0,sizeof(mb->TechUpgrades));
}

/*-----------------------------------------------------------------*/
void yw_DBL1RunningDone(struct ypaworld_data *ywd,
                        struct VFMInput *ip,
                        struct MissionBriefing *mb)
/*
**  FUNCTION
**      Handelt sowohl Running, als auch Done ab!!!
**
**  CHANGED
**      26-Aug-97   floh    created
**      03-Sep-97   floh    + rendert Score
**      09-Sep-97   floh    + handelt jetzt auch Power-Station-
**                            Conquers und Tech Upgrades ab
**      19-May-98   floh    + YPAHIST_TECHUPGRADE neu
**      11-Jun-98   floh    + fuer Multiplayersessions sollte der
**                            End-Score jetzt korrekt angepasst werden 
**      28-Jun-98   floh    + ooops, die Wireframe-Effekt-Linien wurden 
**                            ausserhalb RASTM_Begin2D/RASTM_End2D
**                            gezeichnet, dass hat sich erst durch den
**                            strikteren RASTM_End2D Code gezeigt, der
**                            den Display-Pointer loescht...  
*/
{
    LONG real_time = mb->TimeStamp - mb->StartTime;
    LONG scan_time = 0;
    UBYTE str_buf[2048];
    struct rast_text rt;
    UBYTE *str = str_buf;

    struct MinList *ls;
    struct MinNode *nd;
    BOOL quit = FALSE;

    /*** aktuellen Status der OwnerMap ***/
    yw_DBRenderOwners(ywd,mb);

    /*** Score + Techupgrades rendern ***/
    str = yw_DBLayoutScoreTable(ywd,mb,str);
    str = yw_DBLayoutTechUpgrades(ywd,mb,str);
    eos(str);
    rt.string = str_buf;
    rt.clips  = NULL;
    _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
    _methoda(ywd->GfxObject,RASTM_Text,&rt);

    /*** History-Auswertung nur, wenn noch running ***/
    if (mb->Status == MBSTATUS_L1_RUNNING) {

        /*** alle HistoryBuffer parsen ***/
        ls = &(ywd->History->ls);
        for (nd=ls->mlh_Head; (nd->mln_Succ && (!quit)); nd=nd->mln_Succ) {

            struct HistoryBuffer *hbuf = (struct HistoryBuffer *) nd;
            UBYTE *act_inst = hbuf->inst_begin;
            ULONG size = 0;
            BOOL scanning = TRUE;

            /*** Scanne aktuellen Instruktions-Buffer bis Ende ***/
            while (scanning) {
                UBYTE cmd = *act_inst;
                switch(cmd) {
                    case YPAHIST_INVALID:
                        /*** nächster Puffer please ***/
                        scanning = FALSE;
                        size = 0;
                        break;

                    case YPAHIST_NEWFRAME:
                        size = sizeof(struct ypa_HistNewFrame);
                        scan_time = ((struct ypa_HistNewFrame *)act_inst)->time_stamp;
                        if (scan_time >= mb->ActFrameTimeStamp) {
                            mb->ActFrameTimeStamp = scan_time;
                        };
                        if (scan_time >= real_time) {
                            scanning = FALSE;
                            quit = TRUE;
                        };
                        break;

                    case YPAHIST_CONSEC:
                        size = sizeof(struct ypa_HistConSec);
                        yw_DBHandleConSec(ywd, mb,
                                          (struct ypa_HistConSec *)act_inst,
                                          real_time, scan_time);
                        break;

                    case YPAHIST_VHCLKILL:
                        size = sizeof(struct ypa_HistVhclKill);
                        yw_DBHandleVhclKill(ywd, mb,
                                            (struct ypa_HistVhclKill *)act_inst,
                                            real_time, scan_time);
                        break;

                    case YPAHIST_VHCLCREATE:
                        size = sizeof(struct ypa_HistVhclCreate);
                        yw_DBHandleVhclCreate(ywd, mb,
                                              (struct ypa_HistVhclCreate *)act_inst,
                                              real_time, scan_time);
                        break;

                    case YPAHIST_POWERSTATION:
                        size = sizeof(struct ypa_HistConSec);
                        yw_DBHandlePowerStation(ywd, mb,
                                                (struct ypa_HistConSec *)act_inst,
                                                real_time, scan_time);
                        break;

                    case YPAHIST_TECHUPGRADE:
                        size = sizeof(struct ypa_HistTechUpgrade);
                        yw_DBHandleTechUpgrade(ywd, mb,
                                               (struct ypa_HistTechUpgrade *)act_inst,
                                               real_time, scan_time);
                        break;
                };
                act_inst += size;
            };
        };
        if (!quit) {
            /*** History-Aufzeichnung war zuende, also L1_DONE setzen ***/
            mb->Status = MBSTATUS_L1_DONE;
            /*** Multiplayer Score korrekt alignen ***/
            if (ywd->WasNetworkSession) {
                ULONG i;
                for (i=0; i<MAXNUM_OWNERS; i++) mb->LocalStats[i] = ywd->IngameStats[i];
            };
        };
    };
    
    /*** Level Titel ueber alles drueber rendern ***/
    yw_DBPutTitles(ywd,mb);
    _methoda(ywd->GfxObject,RASTM_End2D,NULL);
}

/*-----------------------------------------------------------------*/
void yw_DBDoGlobalScore(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Stellt sicher, daß der GlobalScore des Users
**      beim Beenden des Debriefings korrekt hochgezählt wird.
**
**  CHANGED
**      08-Sep-97   floh    created
**      09-May-98   floh    + GlobalStats werden nur noch beeinflusst
**                            wenn es sich um eine Singleplayer-
**                            Session handelte
**      19-May-98   floh    + neues YPAHIST_TECHUPGRADE Handling
*/
{
    if (!ywd->WasNetworkSession) {
        ULONG i;
        ULONG timer = 0;
        struct MinList *ls;
        struct MinNode *nd;

        /*** alle HistoryBuffer parsen ***/
        _LogMsg("-> DoGlobalScore() entered\n");
        ls = &(ywd->History->ls);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

            struct HistoryBuffer *hbuf = (struct HistoryBuffer *) nd;
            UBYTE *act_inst = hbuf->inst_begin;
            ULONG size = 0;
            BOOL scanning = TRUE;

            /*** Scanne aktuellen Instruktions-Buffer bis Ende ***/
            while (scanning) {
                UBYTE cmd = *act_inst;
                switch(cmd) {
                    case YPAHIST_INVALID:
                        /*** nächster Puffer please ***/
                        scanning = FALSE;
                        size = 0;
                        break;

                    case YPAHIST_NEWFRAME:
                        size  = sizeof(struct ypa_HistNewFrame);
                        timer = ((struct ypa_HistNewFrame *)act_inst)->time_stamp;
                        break;

                    case YPAHIST_CONSEC:
                        size = sizeof(struct ypa_HistConSec);
                        yw_Score(ywd,act_inst,ywd->GlobalStats);
                        break;

                    case YPAHIST_VHCLKILL:
                        size  = sizeof(struct ypa_HistVhclKill);
                        yw_Score(ywd,act_inst,ywd->GlobalStats);
                        break;

                    case YPAHIST_VHCLCREATE:
                        size = sizeof(struct ypa_HistVhclCreate);
                        yw_Score(ywd,act_inst,ywd->GlobalStats);
                        break;

                    case YPAHIST_POWERSTATION:
                        size = sizeof(struct ypa_HistConSec);
                        yw_Score(ywd,act_inst,ywd->GlobalStats);
                        break;

                    case YPAHIST_TECHUPGRADE:
                        size = sizeof(struct ypa_HistTechUpgrade);
                        yw_Score(ywd,act_inst,ywd->GlobalStats);
                        break;
                };
                act_inst += size;
            };
        };
        for (i=0; i<MAXNUM_ROBOS; i++) ywd->GlobalStats[i].Time += timer;
        _LogMsg("-> DoGlobalScore() left\n");
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerDebriefing(struct ypaworld_data *ywd,
                          struct GameShellReq *gsr,
                          struct VFMInput *ip)
/*
**  FUNCTION
**      Übernimmt den Ablauf des Mission-Debriefing.
**      Als Endebedingung wird MBSTAT_CANCEL erwartet.
**      Zur Manipulation werden die "üblichen" MissionBriefing-
**      Funktionen yw_MBPause(), yw_MBCancel(), etc.
**      verwendet (aus yw_mission.c).
**
**  CHANGED
**      26-Aug-97   floh    created
*/
{
    struct MissionBriefing *mb = &(ywd->Mission);
    if (ywd->History) {

        /*** TimerStatus auswerten ***/
        switch(mb->TimerStatus) {
            case TMBSTAT_PLAYING:
                if (mb->Status == MBSTATUS_L1_RUNNING) {
                    /*** während Missionbriefing, Fast Forward ***/
                    mb->TimeStamp += ip->FrameTime * MB_TIMESCALE;
                } else if (mb->Status == MBSTATUS_L1_DONE) {
                    /*** Timer einfrieren ***/
                } else {
                    mb->TimeStamp += ip->FrameTime;
                };
                break;
            case TMBSTAT_PAUSED:
                ip->FrameTime = 1;
                break;
            case TMBSTAT_RWND:
                mb->Status      = MBSTATUS_L1_START;
                mb->TimerStatus = TMBSTAT_PLAYING;
                break;
        };

        /*** Background-Maps rendern ***/
        if (mb->Status != MBSTATUS_MAPLOADED) {
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
        };

        /*** Aktion ist abhängig vom gegenwärtigen Status ***/
        switch(mb->Status) {
            case MBSTATUS_MAPLOADED:
                yw_DBMapLoaded(ywd,ip,mb);
                break;
            case MBSTATUS_MAPSCALING:
                yw_DBMapScaling(ywd,ip,mb);
                break;
            case MBSTATUS_MAPDONE:
                yw_DBMapDone(ywd,ip,mb);
                break;
            case MBSTATUS_L1_START:
                yw_DBL1Start(ywd,ip,mb);
                break;
            case MBSTATUS_L1_RUNNING:
                yw_DBL1RunningDone(ywd,ip,mb);
                break;
            case MBSTATUS_L1_DONE:
                yw_DBL1RunningDone(ywd,ip,mb);
                break;
        };

    } else {
        mb->Status = MBSTATUS_CANCEL;
    };
}

