/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_enbar.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 16:18:12 $
**  $Locker:  $
**  $Author: floh $
**
**  yw_enbar.c -- neuer Energie-Balken.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypatooltips.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_audio_engine

extern struct YPAStatusReq SR;

BYTE EB_ReqString[512] = {NULL};

struct ClickButton EB_Reload;
struct ClickButton EB_System;
struct ClickButton EB_Vehicle;
struct ClickButton EB_Beam;

struct YPAEnergyBar EB;

/*-----------------------------------------------------------------*/
BOOL yw_InitEB(struct ypaworld_data *ywd)
/*
**  CHANGED
**      09-Oct-97   floh    created
**      12-Dec-97   floh    + Build-Balken rausgeflogen
*/
{
    ULONG btn;

    /*** initialisiere Requester ***/
    memset(&EB,0,sizeof(EB));
    EB.bar_height = ywd->Fonts[FONTID_ENERGY]->height;
    EB.icon_width = ywd->Fonts[FONTID_ENERGY]->fchars['A'].width;
    EB.bar_width  = ywd->Fonts[FONTID_ENERGY]->fchars['T'].width + EB.icon_width;
    EB.bar_space  = (ywd->DspXRes-(EB_NUMBUTTONS*EB.bar_width))/EB_NUMBUTTONS;
    EB.bar_start  = EB.bar_space/2;
    EB.fuel_bar_start = 2;
    EB.fuel_bar_width = EB.bar_width-EB.icon_width-2*EB.fuel_bar_start;

    EB.req.flags = 0;
    EB.req.req_cbox.rect.x = 0;
    EB.req.req_cbox.rect.y = 0;
    EB.req.req_cbox.rect.w = ywd->DspXRes;
    EB.req.req_cbox.rect.h = EB.bar_height;

    EB.req.req_cbox.num_buttons = EB_NUMBUTTONS;

    EB.req.req_cbox.buttons[EBBTN_RELOAD]  = &EB_Reload;
    EB.req.req_cbox.buttons[EBBTN_SYSTEM]  = &EB_System;
    EB.req.req_cbox.buttons[EBBTN_VEHICLE] = &EB_Vehicle;
    EB.req.req_cbox.buttons[EBBTN_BEAM]    = &EB_Beam;

    for (btn=0; btn<EB_NUMBUTTONS; btn++) {
        struct ClickButton *cb = EB.req.req_cbox.buttons[btn];
        cb->rect.x = EB.bar_start + (btn*(EB.bar_width+EB.bar_space));
        cb->rect.y = 0;
        cb->rect.w = EB.bar_width;
        cb->rect.h = EB.bar_height;
    };
    EB.req.req_string = EB_ReqString;

    /*** EnergyBar einklinken ***/
    _AddClickBox(&(EB.req.req_cbox), IE_CBX_ADDHEAD);
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillEB(struct ypaworld_data *ywd)
/*
**  CHANGED
**      09-Oct-97   floh    created
*/
{
    _RemClickBox(&(EB.req.req_cbox));
}

/*-----------------------------------------------------------------*/
void yw_RenderEB(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert den EnergyBar. Der EnergyBar wird nicht
**      per YWM_ADDREQUESTER eingeklinkt und hat deshalb
**      sein eigenes Rendering.
**
**  CHANGED
**      09-Oct-97   floh    created
*/
{
    struct rast_text rt;
    rt.string = EB.req.req_string;
    rt.clips  = EB.req.req_clips;
    _methoda(ywd->GfxObject,RASTM_Text,&rt);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_EBPutBar(struct ypaworld_data *ywd,
                   UBYTE *str,
                   WORD xpos, WORD ypos,
                   UBYTE icon_chr,
                   UBYTE fuel_chr_1,
                   UBYTE fuel_chr_2,
                   FLOAT fuel_width_1,
                   FLOAT fuel_width_2,
                   UBYTE *overlay_text)
/*
**  FUNCTION
**      Generic Energybar Layouter.
**
**
**  INPUTS
**      ywd
**      str
**      xpos,ypos     - Position auf Screen
**      icon_chr      - Char für Icon
**      fuel_chr1     - Char für Füllbalken, voll
**      fuel_chr2     - Char für Füllbalken, Abstufung
**      fuel_width_1  - relative Breite des Füllbalkens [0.0 .. 1.0]
**      fuel_width_2  - relative Breite der Abstufung [0.0 .. 1.0]
**      overlay_text  - Overlay-String für Zahlen-Angaben etc...,
**                      kann NULL sein
**
**  CHANGED
**      09-Oct-97   floh    created
**      28-Oct-97   floh    + Overlay-Text verwendet jetzt den FONTID_LTRACY
**                            Font
*/
{
    LONG rw1 = (LONG)(fuel_width_1 * EB.fuel_bar_width);
    LONG rw2 = (LONG)(fuel_width_2 * EB.fuel_bar_width);

    if ((rw1 == 0) && (fuel_width_1 > 0.0)) rw1=1;
    if ((rw2 == 0) && (fuel_width_2 > 0.0)) rw2=1;

    /*** Icon und Hintergrundbalken ***/
    new_font(str,FONTID_ENERGY);
    pos_abs(str,xpos,ypos);
    put(str,icon_chr);
    put(str,'T');

    /*** Füllbalken ***/
    if ((rw1 > 0) || (rw2 > 0)) {
        xpos_abs(str,xpos+EB.icon_width+EB.fuel_bar_start);
        if (rw1 > 0) {
            if (rw1 > 1) {
                stretch_to(str,rw1);
            };
            put(str,fuel_chr_1);
        };
        if (rw2 > rw1) {
            if (rw2 > 1) {
                stretch_to(str,rw2);
            };
            put(str,fuel_chr_2);
        };
    };

    /*** Overlay-Text ***/
    if (overlay_text) {
        new_font(str,FONTID_LTRACY);
        xpos_abs(str,xpos+EB.icon_width+4);
        ypos_brel(str,0);
        strcpy(str,overlay_text);
        str += strlen(overlay_text);
    };
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_EBPutReloadBar(struct ypaworld_data *ywd,
                         UBYTE *str, WORD xpos, WORD ypos,
                         LONG load_stat, LONG efact, LONG reload,
                         LONG abs_real_load, LONG abs_full_load)
/*
**  CHANGED
**      09-Oct-97   floh    created
**      20-May-98   floh    jetzt mit absoluten Zahlen
*/
{
    UBYTE icon_chr;
    UBYTE fuel_chr_1,fuel_chr_2;
    FLOAT fuel_width_1,fuel_width_2;
    UBYTE str_buf[32];
    UBYTE *overlay_text;
    FLOAT reload_rel,efact_rel;

    switch(load_stat) {
        case -1:    icon_chr='R'; break;
        case  0:    icon_chr='S'; break;
        case +1:    icon_chr='Q'; break;
    };
    fuel_chr_1=0;
    fuel_chr_2='9';
    switch(ywd->URBact->Sector->Owner) {
        case 1: fuel_chr_1='1'; break;
        case 2: fuel_chr_1='2'; break;
        case 3: fuel_chr_1='3'; break;
        case 4: fuel_chr_1='4'; break;
        case 5: fuel_chr_1='5'; break;
        case 6: fuel_chr_1='6'; break;
        case 7: fuel_chr_1='7'; break;
    };
    if (efact > 0) {
        efact_rel  = ((FLOAT)efact) / 255.0;
        reload_rel = ((FLOAT)reload) / ((FLOAT)efact);
        fuel_width_1 = reload_rel * efact_rel;
        fuel_width_2 = efact_rel;
    }else{
        reload_rel = 0.0;
        efact_rel  = 0.0;
        fuel_width_1 = 0.0;
        fuel_width_2 = 0.0;
    };
    if (load_stat < 0) {
        sprintf(str_buf,"%d",-abs_real_load);
    } else {
        // FIXME: bis die neuen LTracy-Zeichen kommen...
        if ((abs_real_load == 0) || (reload_rel >= 1.0)) sprintf(str_buf,"%d",abs_real_load);
        else                                             sprintf(str_buf,"%d/%d%%",abs_real_load,(int)(reload_rel*100));
    };
    overlay_text = str_buf;    
    str=yw_EBPutBar(ywd,str,xpos,ypos,icon_chr,
                    fuel_chr_1,fuel_chr_2,fuel_width_1,fuel_width_2,
                    overlay_text);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_EBPutSystemBar(struct ypaworld_data *ywd,
                         UBYTE *str, WORD xpos, WORD ypos,
                         ULONG fill_modus, LONG load_stat, 
                         LONG b_sys, LONG b_max)
/*
**  CHANGED
**      09-Oct-97   floh    created
**      29-Apr-98   floh    + kann jetzt auch gesperrt sein
**      30-Apr-98   floh    + blinkt jetzt, wenn unter 20%
*/
{
    UBYTE icon_chr;
    UBYTE fuel_chr_1;
    FLOAT fuel_width_1;
    UBYTE str_buf[32];
    UBYTE *str_buf_ptr = str_buf;
    if (fill_modus & YRF_Fill_System) {
        switch(load_stat) {
            case -1:    icon_chr='B'; break;
            case  0:    icon_chr='C'; break;
            case +1:    icon_chr='A'; break;
        };
    } else icon_chr='D';
    
    /*** falls System-Batterie unter 20% blinken ***/
    if ((b_sys < (b_max/5)) && ((ywd->TimeStamp/300) & 1)) {
        fuel_chr_1  = '9';
        str_buf_ptr = NULL;
    } else {
        fuel_chr_1 = '8';
    };
    fuel_width_1 = ((FLOAT)b_sys) / ((FLOAT)b_max);
    sprintf(str_buf,"%d",b_sys/100);
    str=yw_EBPutBar(ywd,str,xpos,ypos,icon_chr,
                    fuel_chr_1,0,fuel_width_1,(FLOAT)0.0,
                    str_buf_ptr);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_EBPutVehicleBar(struct ypaworld_data *ywd,
                          UBYTE *str, WORD xpos, WORD ypos,
                          ULONG fill_modus, LONG load_stat,
                          LONG b_vehicle, LONG b_max)
/*
**  CHANGED
**      09-Oct-97   floh    created
*/
{
    UBYTE icon_chr;
    UBYTE fuel_chr_1,fuel_chr_2;
    FLOAT fuel_width_1,fuel_width_2;
    UBYTE str_buf[32];

    if (fill_modus & YRF_Fill_Vehicle) {
        switch(load_stat) {
            case -1:    icon_chr='F'; break;
            case  0:    icon_chr='G'; break;
            case +1:    icon_chr='E'; break;
        };
    } else {
        icon_chr='H';
    };
    fuel_chr_1   = '8';
    fuel_chr_2   = '9';
    fuel_width_1 = ((FLOAT)b_vehicle)/((FLOAT)b_max);
    fuel_width_2 = 0.0;
    if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD)) {
        if (SR.ActVehicle != -1) {
            struct VehicleProto *vp = &(ywd->VP_Array[SR.VPRemap[SR.ActVehicle]]);
            fuel_width_2 = ((FLOAT)(vp->Energy*CREATE_ENERGY_FACTOR))/((FLOAT)b_max);
        };
    };
    if (fuel_width_2 > fuel_width_1) {
        fuel_width_2 = fuel_width_1;
        fuel_width_1 = 0.0;
    } else {
        FLOAT tmp = fuel_width_1;
        fuel_width_1 -= fuel_width_2;
        fuel_width_2 = tmp;
    };
    sprintf(str_buf,"%d",b_vehicle/100);
    str=yw_EBPutBar(ywd,str,xpos,ypos,icon_chr,
                    fuel_chr_1,fuel_chr_2,fuel_width_1,fuel_width_2,
                    str_buf);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_EBPutBeamBar(struct ypaworld_data *ywd,
                       UBYTE *str, WORD xpos, WORD ypos,
                       ULONG fill_modus, LONG load_stat,
                       LONG b_beam, LONG b_max)
/*
**  CHANGED
**      09-Oct-97   floh    created
*/
{
    UBYTE icon_chr;
    UBYTE fuel_chr_1,fuel_chr_2;
    FLOAT fuel_width_1,fuel_width_2;
    UBYTE str_buf[32];

    if (fill_modus & YRF_Fill_Beam) {
        switch(load_stat) {
            case -1:    icon_chr='N'; break;
            case  0:    icon_chr='O'; break;
            case +1:    icon_chr='M'; break;
        };
    } else {
        icon_chr='P';
    };
    fuel_chr_1   = '8';
    fuel_chr_2   = '9';
    fuel_width_1 = ((FLOAT)b_beam)/((FLOAT)b_max);
    fuel_width_2 = 0.0;
    if (SR.ActiveMode & STAT_MODEF_AUTOPILOT) {
        /*** HACK! abhängig von yw_select.c/yw_BeamCheck()! ***/
        fuel_width_2 = ((FLOAT)(ywd->TLMsg.energy))/((FLOAT)b_max);
    };
    if (fuel_width_2 > fuel_width_1) {
        fuel_width_2 = fuel_width_1;
        fuel_width_1 = 0.0;
    } else {
        FLOAT tmp = fuel_width_1;
        fuel_width_1 -= fuel_width_2;
        fuel_width_2 = tmp;
    };
    sprintf(str_buf,"%d",b_beam/100);
    str=yw_EBPutBar(ywd,str,xpos,ypos,icon_chr,
                    fuel_chr_1,fuel_chr_2,fuel_width_1,fuel_width_2,
                    str_buf);
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_LayoutEB(struct ypaworld_data *ywd)
/*
**  CHANGED
**      09-Oct-97   floh    created
**      16-Oct-97   floh    + EB wird nicht mehr angezeigt, wenn
**                            Hoststation toooot...
**      12-Dec-97   floh    + Build Balken ist raus
**      20-May-98   floh    + Reload-Bar mit Absolut-Nummern
*/
{
    BYTE *str = EB_ReqString;
    if (ywd->URBact->MainState != ACTION_DEAD) {

        WORD xpos = EB.req.req_cbox.rect.x - (ywd->DspXRes>>1);
        WORD ypos = EB.req.req_cbox.rect.y - (ywd->DspYRes>>1);
        WORD next_bar = (EB.bar_width + EB.bar_space);
        ULONG fill_modus,b_sys,b_vehicle,b_beam,b_max,load_flags,loss_flags;
        LONG load_stat,sys_stat,vhcl_stat,beam_stat; 
        struct getrldratio_msg grm;
        LONG efact,reload;
        LONG abs_real_load, abs_full_load;

        /*** Daten besorgen ***/
        _method(ywd->UserRobo, OM_GET,
                YRA_FillModus,    &fill_modus,
                YRA_BattVehicle,  &b_vehicle,
                YRA_BattBeam,     &b_beam,
                YRA_LoadFlags,    &load_flags,
                YRA_LossFlags,    &loss_flags,
                TAG_DONE);
        b_sys = ywd->URBact->Energy;
        b_max = ywd->URBact->Maximum;
        grm.owner = ywd->URBact->Sector->Owner;
        _methoda(ywd->world,YWM_GETRLDRATIO,&grm);
        efact  = ywd->URBact->Sector->EnergyFactor;
        reload = (LONG) (efact * grm.ratio);
        _get(ywd->UserRobo,YRA_AbsReload,&abs_full_load);
        abs_real_load = (LONG) (((FLOAT)abs_full_load) * grm.ratio);
        abs_full_load = (abs_full_load * 10) / 100; // Energie-Punkte pro 10 Sekunden
        abs_real_load = (abs_real_load * 10) / 100; // ditto         
        
        if (ywd->URBact->Owner == ywd->URBact->Sector->Owner) {
            if (reload == 0) load_stat = 0;
            else             load_stat = +1;
        } else {
            if (reload == 0) load_stat = 0;
            else             { load_stat = -1; reload=efact; };
        };
        
        if (load_flags & YRF_Fill_System)       sys_stat=+1;
        else if (loss_flags & YRF_Fill_System)  sys_stat=-1;
        else                                    sys_stat=0;
        if (load_flags & YRF_Fill_Vehicle)      vhcl_stat=+1;
        else if (loss_flags & YRF_Fill_Vehicle) vhcl_stat=-1;
        else                                    vhcl_stat=0;
        if (load_flags & YRF_Fill_Beam)         beam_stat=+1;
        else if (loss_flags & YRF_Fill_Beam)    beam_stat=-1;
        else                                    beam_stat=0; 

        /*** Energie-Balken immer vorn ***/
        _RemClickBox(&(EB.req.req_cbox));
        _AddClickBox(&(EB.req.req_cbox),IE_CBX_ADDHEAD);
        xpos += EB.bar_start;
        str = yw_EBPutReloadBar(ywd,str,xpos,ypos,load_stat,efact,reload,abs_real_load,abs_full_load);
        xpos += next_bar;
        str = yw_EBPutSystemBar(ywd,str,xpos,ypos,fill_modus,sys_stat,b_sys,b_max);
        xpos += next_bar;
        str = yw_EBPutVehicleBar(ywd,str,xpos,ypos,fill_modus,vhcl_stat,b_vehicle,b_max);
        xpos += next_bar;
        str = yw_EBPutBeamBar(ywd,str,xpos,ypos,fill_modus,beam_stat,b_beam,b_max);
    };
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_HandleInputEB(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  CHANGED
**      09-Oct-97   floh    created
**      23-Oct-97   floh    + Sound
**      29-Apr-98   floh    + System-Batterie laesst sich jetzt sperren
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    if (ci->box == &(EB.req.req_cbox)) {

        if ((ywd->gsr) && (ci->btn!=-1) && (ci->flags&CIF_BUTTONDOWN)) {
            _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
        };

        if (ci->flags & CIF_BUTTONUP) {
            ULONG fill_modus;
            _get(ywd->UserRobo,YRA_FillModus,&fill_modus);
            switch(ci->btn) {
                case EBBTN_SYSTEM:
                    fill_modus ^= YRF_Fill_System;
                    break;
                case EBBTN_VEHICLE:
                    fill_modus ^= YRF_Fill_Vehicle;
                    break;
                case EBBTN_BEAM:
                    fill_modus ^= YRF_Fill_Beam;
                    break;
            };
            _set(ywd->UserRobo,YRA_FillModus,fill_modus);
        };
        /*** Tooltips ***/
        switch(ci->btn) {
            case EBBTN_RELOAD:  yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_RELOAD); break;
            case EBBTN_SYSTEM:  yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_SYSAKKU); break;
            case EBBTN_VEHICLE: yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_VHCLAKKU); break;
            case EBBTN_BEAM:    yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_BEAMAKKU); break;
        };
    };
    yw_LayoutEB(ywd);
}

