/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_bgshell.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/02/23 01:36:33 $
**  $Locker: floh $
**  $Author: floh $
**
**  yw_bgshell.c -- Shell-Hintergrund-Handler (Bg-Pics und Levelauswahl)
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
#include "engine/engine.h"
#include "nucleus/math.h"
#include "visualstuff/ov_engine.h"
#include "bitmap/rasterclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypagameshell.h"

#include "yw_protos.h"
#include "yw_gsprotos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_audio_engine

/*-----------------------------------------------------------------*/
void yw_MaskGetBlitCoords(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Geht für jede Levelnummer die Masken-Background-Map
**      durch und ermittelt das kleinste Bounding-Rect als
**      Blitkoordinaten.
**
**  CHANGED
**      06-Nov-97   floh    created
*/
{
    if (ywd->LevelNet->MaskMap) {
        struct VFMBitmap *bmp;
        LONG i,x,y;
        struct rast_intrect lcoords[MAXNUM_LEVELS];
        UBYTE *mask_ptr;

        /*** Minimax Koordinaten nach Anfangszustand ***/
        for (i=0; i<MAXNUM_LEVELS; i++) {
            lcoords[i].xmin = 10000;
            lcoords[i].xmax = -10000;
            lcoords[i].ymin = 10000;
            lcoords[i].ymax = -10000;
        };
        _get(ywd->LevelNet->MaskMap,BMA_Bitmap,&bmp);
        for (y=0; y<bmp->Height; y++) {
            mask_ptr = ((UBYTE *)bmp->Data) + y*bmp->BytesPerRow;
            for (x=0; x<bmp->Width; x++) {
                UBYTE mask_val = *mask_ptr++;
                if (mask_val < MAXNUM_LEVELS) {
                    struct rast_intrect *r = &(lcoords[mask_val]);
                    if (x < r->xmin) r->xmin = x;
                    if (x > r->xmax) r->xmax = x;
                    if (y < r->ymin) r->ymin = y;
                    if (y > r->ymax) r->ymax = y;
                };
            };
        };

        /*** jetzt alle ermittelten Minimax-Koordinaten übernehmen ***/
        for (i=0; i<MAXNUM_LEVELS; i++) {
            struct LevelNode *l = &(ywd->LevelNet->Levels[i]);
            if ((l->status != LNSTAT_INVALID) &&
                (l->status != LNSTAT_NETWORK) &&
                (lcoords[i].xmin != 10000))
            {
                l->r.xmin = ((((FLOAT)lcoords[i].xmin)/((FLOAT)bmp->Width))*2.0)-1.0;
                l->r.xmax = ((((FLOAT)lcoords[i].xmax)/((FLOAT)bmp->Width))*2.0)-1.0;
                l->r.ymin = ((((FLOAT)lcoords[i].ymin)/((FLOAT)bmp->Height))*2.0)-1.0;
                l->r.ymax = ((((FLOAT)lcoords[i].ymax)/((FLOAT)bmp->Height))*2.0)-1.0;
            } else {
                l->r.xmin = 0.0;
                l->r.xmax = 0.0;
                l->r.ymin = 0.0;
                l->r.ymax = 0.0;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_KillBgPicObjects(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt alle geladenen Background-Maps.
**
**  CHANGED
**      21-Feb-98   floh    created
*/
{
    struct WorldInfo *ln = ywd->LevelNet;
    if (ln->BgMap) {
        _dispose(ln->BgMap); ln->BgMap=NULL;
    };
    if (ln->RolloverMap) {
        _dispose(ln->RolloverMap); ln->RolloverMap=NULL;
    };
    if (ln->FinishedMap) {
        _dispose(ln->FinishedMap); ln->FinishedMap=NULL;
    };
    if (ln->EnabledMap) {
        _dispose(ln->EnabledMap); ln->EnabledMap=NULL;
    };
    if (ln->MaskMap) {
        _dispose(ln->MaskMap); ln->MaskMap=NULL;
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadBgPicObjects(struct ypaworld_data *ywd, ULONG shell_mode)
/*
**  FUNCTION
**      Lädt alle für einen bestimmten ShellMode benötigten
**      Hintergrund-Bilder.
**
**  CHANGED
**      21-Feb-98   floh    created
**      26-Feb-98   floh    + lädt zuerst, killt danach, damit
**                            werden identische Bilder in Null-
**                            zeit geladen
**      25-May-98   floh    + so umgeschrieben, dass fuer alle
**                            "normalen" Shell-Screens 2 Maps
**                            gleichzeitig geladen sind.
*/
{
    ULONG all_ok = TRUE;

    if (ywd->LevelNet->NumBG > 0) {

        UBYTE old_path[255];
        LONG min_dist,min_index;
        ULONG i;
        UBYTE  *bg_name         = NULL;
        UBYTE  *mask_name       = NULL;
        UBYTE  *rollover_name   = NULL;
        UBYTE  *finished_name   = NULL;
        UBYTE  *enabled_name    = NULL;
        Object *bg_object       = NULL;
        Object *mask_object     = NULL;
        Object *rollover_object = NULL;
        Object *finished_object = NULL;
        Object *enabled_object  = NULL;

        /*** Resource-Path umbiegen ***/
        strcpy(old_path, _GetAssign("rsrc"));
        _SetAssign("rsrc","levels:");

        /*** passendste Pic-Size finden ***/
        min_index = 0;
        min_dist  = (1<<16);    // Big Num
        for (i=0; i<ywd->LevelNet->NumBG; i++) {
            LONG dw = ywd->LevelNet->BgMaps[i].w - ywd->DspXRes;
            LONG dh = ywd->LevelNet->BgMaps[i].h - ywd->DspYRes;
            LONG dist = nc_sqrt((float)(dw*dw + dh*dh));
            if (dist < min_dist) {
                min_dist  = dist;
                min_index = i;
            };
        };

        /*** die Namen der benötigten Maps rausfummeln ***/
        switch(shell_mode) {
            case SHELLMODE_TITLE:
            case SHELLMODE_INPUT:
            case SHELLMODE_SETTINGS:
            case SHELLMODE_NETWORK:
            case SHELLMODE_LOCALE:
            case SHELLMODE_ABOUT:
            case SHELLMODE_DISK:
            case SHELLMODE_HELP:
                bg_name       = ywd->LevelNet->MenuMaps[min_index].name;
                rollover_name = ywd->LevelNet->SettingsMaps[min_index].name;
                break;

            case SHELLMODE_TUTORIAL:
                bg_name       = ywd->LevelNet->TutBgMaps[min_index].name;
                rollover_name = ywd->LevelNet->TutRolloverMaps[min_index].name;
                mask_name     = ywd->LevelNet->TutMaskMaps[min_index].name;
                break;

            case SHELLMODE_PLAY:
                bg_name       = ywd->LevelNet->BgMaps[min_index].name;
                rollover_name = ywd->LevelNet->RolloverMaps[min_index].name;
                finished_name = ywd->LevelNet->FinishedMaps[min_index].name;
                enabled_name  = ywd->LevelNet->EnabledMaps[min_index].name;
                mask_name     = ywd->LevelNet->MaskMaps[min_index].name;
                break;
        };

        /*** die benötigten Maps reinladen ***/
        if (bg_name) {
            bg_object = _new("ilbm.class",
                             RSA_Name, bg_name,
                             BMA_Texture, TRUE,
                             BMA_TxtBlittable, TRUE,
                             TAG_DONE);
            if (!bg_object) {
                _LogMsg("world.ini: Could not load %s\n",bg_name); all_ok=FALSE;
            };
        };
        if (rollover_name) {
            rollover_object = _new("ilbm.class",
                                   RSA_Name, rollover_name,
                                   BMA_Texture, TRUE,
                                   BMA_TxtBlittable, TRUE,
                                   TAG_DONE);
            if (!rollover_object) {
                _LogMsg("world.ini: Could not load %s\n",rollover_name); all_ok=FALSE;
            };
        };
        if (finished_name) {
            finished_object = _new("ilbm.class",
                                   RSA_Name, finished_name,
                                   BMA_Texture, TRUE,
                                   BMA_TxtBlittable, TRUE,
                                   TAG_DONE);
            if (!finished_object) {
                _LogMsg("world.ini: Could not load %s\n",finished_name); all_ok=FALSE;
            };
        };
        if (enabled_name) {
            enabled_object = _new("ilbm.class",
                                  RSA_Name, enabled_name,
                                  BMA_Texture, TRUE,
                                  BMA_TxtBlittable, TRUE,
                                  TAG_DONE);
            if (!enabled_object) {
                _LogMsg("world.ini: Could not load %s\n",enabled_name); all_ok=FALSE;
            };
        };
        if (mask_name) {
            /*** die Maske als 8-Bit! ***/
            mask_object = _new("ilbm.class", RSA_Name, mask_name, TAG_DONE);
            if (!mask_object) {
                _LogMsg("world.ini: Could not load %s\n",mask_name); all_ok=FALSE;
            };
        };

        /*** Resource-Path zurückbiegen ***/
        _SetAssign("rsrc",old_path);

        /*** alles ok? ***/
        if (!all_ok) {
            if (bg_object) { _dispose(bg_object); bg_object=NULL; };
            if (rollover_object) { _dispose(rollover_object); rollover_object=NULL; };
            if (finished_object) { _dispose(finished_object); finished_object=NULL; };
            if (enabled_object)  { _dispose(enabled_object); enabled_object=NULL; };
            if (mask_object) { _dispose(mask_object); mask_object=NULL; };
        };

        /*** alte Pic-Objekte killen ***/
        yw_KillBgPicObjects(ywd);

        /*** Object-Pointer eintragen ***/
        ywd->LevelNet->BgMap       = bg_object;
        ywd->LevelNet->MaskMap     = mask_object;
        ywd->LevelNet->RolloverMap = rollover_object;
        ywd->LevelNet->FinishedMap = finished_object;
        ywd->LevelNet->EnabledMap  = enabled_object;
    };

    return(all_ok);
}

/*-----------------------------------------------------------------*/
void yw_InitShellBgMode(struct ypaworld_data *ywd, ULONG shell_mode)
/*
**  FUNCTION
**      Initialisiert einen neuen Shell-Background-Modus.
**
**  INPUTS
**      new_shellmode - SHELLMODE_#? des neuen Modus
**      old_shellmode - SHELLMODE_#? des vorherigen Modus
**
**  CHANGED
**      21-Feb-98   floh    created
**      20-Jun-98   floh    + TipOfTheDay Handling
*/
{
    ULONG tod;
    
    /*** ... und die neuen Pics laden ***/
    yw_LoadBgPicObjects(ywd,shell_mode);

    /*** Mode-spezifisches Handling ***/
    switch (shell_mode) {
        case SHELLMODE_PLAY:
        case SHELLMODE_TUTORIAL:
            /*** Single Player Level Selection ***/
            ywd->FirstContactOwner = 0;
            ywd->Mission.Status = MBSTATUS_INVALID;
            ywd->LevelNet->MouseOverLevel = 0;
            yw_MaskGetBlitCoords(ywd);
            /*** hole TipOfTheDay ***/
            ywd->TipOfTheDay = yw_GetIntEnv(ywd,"tod.def");    
            tod = ywd->TipOfTheDay+1;
            if ((STR_TIPOFDAY_FIRST + tod) > STR_TIPOFDAY_LAST) tod = 0;
            yw_PutIntEnv(ywd,"tod.def",tod);
            break;
    };
}

/*-----------------------------------------------------------------*/
void yw_KillShellBgMode(struct ypaworld_data *ywd, ULONG shell_mode)
/*
**  FUNCTION
**      Killt einen Shell-Background-Modus.
**
**  CHANGED
**      21-Feb-98   floh    created
*/
{
    /*** zuerst mal alle herumliegenden Bg-Pic-Objekte killen ***/
    yw_KillBgPicObjects(ywd);

    /*** Shellmode-spezifisches Handling ***/
    switch(shell_mode) {
        case SHELLMODE_PLAY:
        case SHELLMODE_TUTORIAL:
            /*** falls Briefing oder Debriefing noch aktiv, killen ***/
            if (LEVELSTAT_BRIEFING == ywd->Level->Status) {            
                yw_KillMissionBriefing(ywd);
            } else if (LEVELSTAT_DEBRIEFING == ywd->Level->Status) {
                yw_KillDebriefing(ywd);
            };
            break;
            
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_InitShellBgHandling(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Neue Initialisierung-Routine für Shell-Background-Handling,
**      muss aus OPENGAMESHELL aufgerufen werden.
**
**  CHANGED
**      21-Feb-98   floh    created
**      20-Jun-98   floh    + TipOfTheDay Handling
*/
{
    Object *gfxo;
    struct rast_intrect clip;

    /*** ClipRegion neu setzen ***/
    _OVE_GetAttrs(OVET_Object, &gfxo, TAG_DONE);
    clip.xmin = -(ywd->DspXRes>>1);
    clip.xmax = ywd->DspXRes>>1;
    clip.ymin = -(ywd->DspYRes>>1);
    clip.ymax = ywd->DspYRes>>1;
    _methoda(gfxo,RASTM_IntClipRegion,&clip);

    /*** Menü-Mode initialisieren ***/
    yw_InitShellBgMode(ywd,ywd->gsr->shell_mode);
    
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillShellBgHandling(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Räumt Shell-Background-Handling auf, muß aus
**      CLOSEGAMESHELL aufgerufen werden.
**
**  CHANGED
**      21-Feb-98   floh    created
**      20-Jun-98   floh    + TipOfTheDay Handling
*/
{
    /*** den aktuellen Shellmode killen ***/
    yw_KillShellBgMode(ywd,ywd->gsr->shell_mode);
}

/*-----------------------------------------------------------------*/
void yw_CheckMOL(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Testet, ob Maus über einem Level ist, und modifiziert
**      entsprechend ywd->LevelNet->MouseOverLevel.
**
**  CHANGED
**      17-Jan-97   floh    created
**      22-Feb-97   floh    simuliert einen Mausklick per
**                          Keycode 1..0 (entspricht den
**                          ersten 10 initialisierten Levels
**      15-Oct-97   floh    + LNSTAT_NETWORK werden jetzt ignoriert
**      23-Oct-97   floh    + Sound bei MouseOverLevel
**      06-Nov-97   floh    + testet jetzt in der Masken-
**                            Bitmap, der alte Test ist erstmal weiterhin
**                            gültig
**      21-Feb-98   floh    + kein Support mehr für Button-Level-Auswahl
**      26-Feb-98   floh    + wird nicht mehr durch Maus-Über-Clickbox,
**                            sondern nur noch durch Maus-Über-Button
**                            blockiert
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    BOOL mol_hit = FALSE;
    ULONG mol = 0;
    FLOAT mx = ((FLOAT)ci->act.scrx) / ((FLOAT)ywd->DspXRes);
    FLOAT my = ((FLOAT)ci->act.scry) / ((FLOAT)ywd->DspYRes);

    if (ywd->LevelNet->MaskMap) {
        /*** Mouse über einer Masken-Farbe? ***/
        if (-1 == ci->btn) {
            struct VFMBitmap *bmp;
            LONG x,y;
            UBYTE *ptr;
            _get(ywd->LevelNet->MaskMap,BMA_Bitmap,&bmp);
            x = bmp->Width  * mx;
            y = bmp->Height * my;
            ptr = ((UBYTE *)bmp->Data) + (y*bmp->BytesPerRow) + x;
            if ((mol = *ptr) && (mol < MAXNUM_LEVELS)) {
                struct LevelNode *ln = &(ywd->LevelNet->Levels[mol]);
                if ((ln->status != LNSTAT_INVALID)  &&
                    (ln->status != LNSTAT_DISABLED) &&
                    (ln->status != LNSTAT_NETWORK))
                {
                    mol_hit = TRUE;
                };
            };
        };
    };

    if (mol_hit) {
        /*** Sound-Ausgabe bei Over-Level-Changed ***/
        if (mol != ywd->LevelNet->MouseOverLevel) {
            if (ywd->gsr) {
                _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_OVERLEVEL);
            };
        };
        ywd->LevelNet->MouseOverLevel=mol;
    } else {
        ywd->LevelNet->MouseOverLevel=0;
    };
}

/*-----------------------------------------------------------------*/
void yw_BlitLevels(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert alle Levels, entsprechend ihrem aktuellen
**      Status.
**
**  CHANGED
**      30-Aug-96   floh    created
**      17-Jan-97   floh    + jetzt per _DrawText().
**      22-Feb-97   floh    + der 256-Byte Stringpuffer hatte bei mehr
**                            als 28 Levels einen Overflow und verursachte
**                            einen Absturz
**                          + der Titel des highlighted Levels wird
**                            als Fake-Tooltip angezeigt.
**      06-Nov-97   floh    + zusätzlich Masken-Blit-Rendering
**      24-Nov-97   floh    + Leveltitel DBCS Enabled
**      21-Feb-98   floh    + Support für Button-Levelauswahl ist endgültig
**                            raus
**      20-Jun-98   floh    + Tip Of The Day Handling
*/
{
    if (ywd->LevelNet->BgMap && ywd->LevelNet->MaskMap &&
        ywd->LevelNet->RolloverMap && ywd->LevelNet->FinishedMap &&
        ywd->LevelNet->EnabledMap)
    {

        ULONG i;
        struct rast_blit blt;
        UBYTE *tod;

        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);

        /*** die Hintergrund-Map ***/
        _get(ywd->LevelNet->BgMap, BMA_Bitmap, &(blt.src));
        blt.from.xmin = blt.to.xmin = -1.0;
        blt.from.ymin = blt.to.ymin = -1.0;
        blt.from.xmax = blt.to.xmax = +1.0;
        blt.from.ymax = blt.to.ymax = +1.0;
        _methoda(ywd->GfxObject,RASTM_Blit,&blt);

        /*** Masken-Blitting ***/
        for (i=0; i<MAXNUM_LEVELS; i++) {

            struct LevelNode *ln  = &(ywd->LevelNet->Levels[i]);

            /*** gültige Masken-Blit-Koordinaten? ***/
            if (ln->r.xmin != ln->r.xmax) {
                struct VFMBitmap *src = NULL;
                switch(ln->status) {
                    case LNSTAT_ENABLED:
                        if (ywd->LevelNet->MouseOverLevel == i) {
                            /*** Rollover-Zustand ***/
                            _get(ywd->LevelNet->RolloverMap,BMA_Bitmap,&src);
                        } else {
                            /*** Enabled-Zustand ***/
                            _get(ywd->LevelNet->EnabledMap,BMA_Bitmap,&src);
                        };
                        break;
                    case LNSTAT_FINISHED:
                        /*** Finished-Zustand ***/
                        _get(ywd->LevelNet->FinishedMap,BMA_Bitmap,&src);
                        break;
                };
                if (src) {
                    struct rast_maskblit blt;
                    blt.src = src;
                    _get(ywd->LevelNet->MaskMap,BMA_Bitmap,&(blt.mask));
                    blt.mask_key  = i;
                    blt.from.xmin = blt.to.xmin = ln->r.xmin;
                    blt.from.ymin = blt.to.ymin = ln->r.ymin;
                    blt.from.xmax = blt.to.xmax = ln->r.xmax;
                    blt.from.ymax = blt.to.ymax = ln->r.ymax;
                    _methoda(ywd->GfxObject,RASTM_MaskBlit,&blt);
                };
            };
        };

        /*** den Highlighted Level Titel als Fake Tooltip ***/
        if (ywd->LevelNet->MouseOverLevel > 0) {
            struct LevelNode *ln = &(ywd->LevelNet->Levels[ywd->LevelNet->MouseOverLevel]);
            UBYTE str_buf[256];
            UBYTE *str = str_buf;
            struct rast_text rt;
            if ((ln->status   != LNSTAT_INVALID) && (ln->title[0] != 0))
            {
                UBYTE *name;

                /*** selbe Positionierung wie Tooltips ***/
                name = ypa_GetStr(ywd->LocHandle,STR_NAME_LEVELS+ywd->LevelNet->MouseOverLevel,ln->title);
                new_font(str,FONTID_TRACY);
                pos_brel(str,0,(-(ywd->IconBH+ywd->FontH+4)));
                dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Green(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Blue(ywd,YPACOLOR_TEXT_TOOLTIP));
                str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_TRACY],str,name,ywd->DspXRes);
            };
            
            /*** String abschliessen und rendern ***/
            eos(str);
            rt.string = str_buf;
            rt.clips  = NULL;
            _methoda(ywd->GfxObject,RASTM_Text,&rt);
        };
        
        /*** TipOfTheDay ***/
        tod = ypa_GetStr(ywd->LocHandle,STR_TIPOFDAY_FIRST+ywd->TipOfTheDay," ");
        yw_PutTOD(ywd, ywd->GfxObject, tod, ywd->DspXRes/20, ywd->DspXRes/20);
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);
    };
}

/*-----------------------------------------------------------------*/
void yw_BlitLevelsTutorial(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Blitter für Tutorial-Screen. Dort gibts keine
**      Enabled- und Finished-Maps, deshalb...
**
**  CHANGED
**      26-Feb-98   floh    created
*/
{
    if (ywd->LevelNet->BgMap && ywd->LevelNet->MaskMap &&
        ywd->LevelNet->RolloverMap)
    {
        ULONG i;
        struct rast_blit blt;

        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);

        /*** die Hintergrund-Map ***/
        _get(ywd->LevelNet->BgMap, BMA_Bitmap, &(blt.src));
        blt.from.xmin = blt.to.xmin = -1.0;
        blt.from.ymin = blt.to.ymin = -1.0;
        blt.from.xmax = blt.to.xmax = +1.0;
        blt.from.ymax = blt.to.ymax = +1.0;
        _methoda(ywd->GfxObject,RASTM_Blit,&blt);

        /*** Masken-Blitting ***/
        for (i=0; i<MAXNUM_LEVELS; i++) {

            struct LevelNode *ln  = &(ywd->LevelNet->Levels[i]);

            /*** gültige Masken-Blit-Koordinaten? ***/
            if (ln->r.xmin != ln->r.xmax) {
                struct VFMBitmap *src = NULL;
                switch(ln->status) {
                    case LNSTAT_FINISHED:
                    case LNSTAT_ENABLED:
                        if (ywd->LevelNet->MouseOverLevel == i) {
                            /*** Rollover-Zustand ***/
                            _get(ywd->LevelNet->RolloverMap,BMA_Bitmap,&src);
                        };
                        break;
                };
                if (src) {
                    struct rast_maskblit blt;
                    blt.src = src;
                    _get(ywd->LevelNet->MaskMap,BMA_Bitmap,&(blt.mask));
                    blt.mask_key  = i;
                    blt.from.xmin = blt.to.xmin = ln->r.xmin;
                    blt.from.ymin = blt.to.ymin = ln->r.ymin;
                    blt.from.xmax = blt.to.xmax = ln->r.xmax;
                    blt.from.ymax = blt.to.ymax = ln->r.ymax;
                    _methoda(ywd->GfxObject,RASTM_MaskBlit,&blt);
                };
            };
        };

        /*** den Highlighted Level Titel als Fake Tooltip ***/
        if (ywd->LevelNet->MouseOverLevel > 0) {
            struct LevelNode *ln = &(ywd->LevelNet->Levels[ywd->LevelNet->MouseOverLevel]);
            UBYTE str_buf[256];
            UBYTE *str = str_buf;
            struct rast_text rt;
            if ((ln->status   != LNSTAT_INVALID) && (ln->title[0] != 0))
            {
                UBYTE *name;

                /*** selbe Positionierung wie Tooltips ***/
                name = ypa_GetStr(ywd->LocHandle,STR_NAME_LEVELS+ywd->LevelNet->MouseOverLevel,ln->title);
                new_font(str,FONTID_TRACY);
                pos_brel(str,0,(-(ywd->IconBH+ywd->FontH+4)));
                dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Green(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Blue(ywd,YPACOLOR_TEXT_TOOLTIP));
                str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_TRACY],str,name,ywd->DspXRes);
            };

            /*** String abschließen und rendern ***/
            eos(str);
            rt.string = str_buf;
            rt.clips  = NULL;
            _methoda(ywd->GfxObject,RASTM_Text,&rt);
        };
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);
    };
}

/*-----------------------------------------------------------------*/
void yw_ShellBgBlt(struct ypaworld_data *ywd, Object *bmpo)
/*
**  FUNCTION
**      Einfacher Shell-Background-Blitter.
**
**  CHANGED
**      22-Feb-98   floh    created
*/
{
    if (bmpo) {
        struct rast_blit blt;
        _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
        _get(bmpo, BMA_Bitmap, &(blt.src));
        blt.from.xmin = blt.to.xmin = -1.0;
        blt.from.ymin = blt.to.ymin = -1.0;
        blt.from.xmax = blt.to.xmax = +1.0;
        blt.from.ymax = blt.to.ymax = +1.0;
        _methoda(ywd->GfxObject,RASTM_Blit,&blt);
        _methoda(ywd->GfxObject,RASTM_End2D,NULL);
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerShellBgGameTut(struct ypaworld_data *ywd,
                              struct GameShellReq *gsr,
                              struct VFMInput *ip)
/*
**  CHANGED
**      21-Feb-98   floh    created
**      06-May-98   floh    LevelSelect-Sounds rausgenommen
*/
{
    if ((LEVELSTAT_FINISHED == ywd->Level->Status) ||
        (LEVELSTAT_ABORTED  == ywd->Level->Status))
    {
        /*** Endemovie spielen ***/
        if (LEVELSTAT_FINISHED == ywd->Level->Status) {
            if (ywd->Level->WinMovie[0]) {
                yw_PlayMovie(ywd,&(ywd->Level->WinMovie));
            };
        };
        if (LEVELSTAT_ABORTED == ywd->Level->Status) {
            if (ywd->Level->LoseMovie[0] && ywd->UserRoboDied) {
                yw_PlayMovie(ywd,&(ywd->Level->LoseMovie));
            };
        };
   
        ywd->Mission.Status = MBSTATUS_INVALID;
        ywd->Level->Status   = LEVELSTAT_SHELL;

        /*** Debriefing wenn gewonnen, oder im Netzwerkspiel ***/
        if (ywd->DoDebriefing) yw_InitDebriefing(ywd);

    }else if (ywd->Mission.Status == MBSTATUS_INVALID){

        /*** Levelauswahl-Map rendern ***/
        struct ClickInfo *ci = &(ip->ClickInfo);

        /*** Maus über einem Level? ***/
        yw_CheckMOL(ywd,ip);

        /*** Hintergrund-Map und Levels blitten ***/
        if (SHELLMODE_TUTORIAL == ywd->gsr->shell_mode) {
            yw_BlitLevelsTutorial(ywd);
        } else {
            yw_BlitLevels(ywd);
        };

        /*** auf Left/Right-Klick Missionsbriefing/Level starten ***/
        if (ywd->LevelNet->MouseOverLevel > 0) {
            if (ci->flags & CIF_MOUSEDOWN) {
                /*** Level schon beendet? -> kein Missionbriefing ***/
                ULONG ln_stat = ywd->LevelNet->Levels[ywd->LevelNet->MouseOverLevel].status;
                if (ln_stat == LNSTAT_FINISHED) {
                    /*** Level sofort starten ***/
                    gsr->GSA.LastAction = A_PLAY;
                    gsr->GSA.ActionParameter[0] = ywd->LevelNet->MouseOverLevel;
                    gsr->GSA.ActionParameter[1] = ywd->LevelNet->MouseOverLevel;
                } else {
                    /*** Missionbriefing  ***/
                    if (!yw_InitMissionBriefing(ywd,ywd->LevelNet->MouseOverLevel)) {
                        /*** also den Level sofort starten ***/
                        gsr->GSA.LastAction = A_PLAY;
                        gsr->GSA.ActionParameter[0] = ywd->LevelNet->MouseOverLevel;
                        gsr->GSA.ActionParameter[1] = ywd->LevelNet->MouseOverLevel;
                    };
                };
            }else if (ci->flags & CIF_RMOUSEDOWN){
                /*** Right-Click: Level sofort starten ***/
                gsr->GSA.LastAction = A_PLAY;
                gsr->GSA.ActionParameter[0] = ywd->LevelNet->MouseOverLevel;
                gsr->GSA.ActionParameter[1] = ywd->LevelNet->MouseOverLevel;
            };
        };
    }else if (LEVELSTAT_BRIEFING == ywd->Level->Status){

        /*** MISSIONBRIEFING LÄUFT ***/

        /*** Game per RETURN starten ***/
        if (ip->NormKey == KEYCODE_RETURN) {
            ywd->Mission.Status = MBSTATUS_STARTLEVEL;
        };

        /*** Missionsbriefing-Status abfragen ***/
        switch(ywd->Mission.Status) {

            case MBSTATUS_STARTLEVEL:
                /*** Level wird genommen ***/
                gsr->GSA.LastAction = A_PLAY;
                gsr->GSA.ActionParameter[0] = ywd->Level->Num;
                gsr->GSA.ActionParameter[1] = ywd->Level->Num;
                yw_KillMissionBriefing(ywd);
                break;

            case MBSTATUS_CANCEL:
                yw_KillMissionBriefing(ywd);
                ywd->Level->Status = LEVELSTAT_SHELL;
                /*** nächster Durchlauf -> Satellitenmap-Handling ***/
                break;

            default:
                /*** das "normale" Mission-Briefing-Handling ***/
                yw_TriggerMissionBriefing(ywd,gsr,ip);
                break;
        };

    }else if (LEVELSTAT_DEBRIEFING == ywd->Level->Status){

        /*** DEBRIEFING LÄUFT ***/

        /*** Debriefing-Status abfragen ***/
        switch(ywd->Mission.Status) {

            case MBSTATUS_CANCEL:
                yw_DBDoGlobalScore(ywd);
                yw_KillDebriefing(ywd);
                /*** nächster Durchlauf -> Satellitenmap-Handling ***/
                break;

            default:
                /*** das "normale" Mission-Briefing-Handling ***/
                yw_TriggerDebriefing(ywd,gsr,ip);
                break;
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerShellBg(struct ypaworld_data *ywd,
                       struct GameShellReq *gsr,
                       struct VFMInput *ip)
/*
**  FUNCTION
**      Trigger-Routine für Shell-Background-Handling.
**      Beinhaltet NICHT das Rendering!
**
**  CHANGED
**      21-Feb-98   floh    created
**      25-May-98   floh    + umgeschrieben fuer 2 Hintergrund-
**                            Maps auf einmal...
*/
{
    /*** muß neuer Shell-BG initialisiert werden? ***/
    if (gsr->shell_mode_changed) {
        yw_InitShellBgMode(ywd,gsr->shell_mode);
    };

    /*** Mode-spezifisches Triggern ***/
    switch (ywd->gsr->shell_mode) {
        case SHELLMODE_PLAY:
        case SHELLMODE_TUTORIAL:
            /*** ziemlich komplex... deshalb eigene Routine ***/
            yw_TriggerShellBgGameTut(ywd,gsr,ip);
            break;
            
        case SHELLMODE_TITLE:
            /*** den Title-Screen blitten ***/
            yw_ShellBgBlt(ywd,ywd->LevelNet->BgMap);   
            break;         

        default:
            /*** sonst einfach den aktuellen Hintergrund blitten ***/
            yw_ShellBgBlt(ywd,ywd->LevelNet->RolloverMap);
            break;
    };
}




