/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_enwin.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 16:18:49 $
**  $Locker:  $
**  $Author: floh $
**
**  Energie-Balken-Fenster.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypatooltips.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

extern struct YPAStatusReq SR;

struct YPAListReq ER;

/*-----------------------------------------------------------------*/
BOOL yw_InitER(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Energie-Balken-Window.
**
**  CHANGED
**      20-Aug-96   floh    created
**      25-Sep-96   floh    Bugfix im Prefs-Handling: nur noch die
**                          Position, nicht die Ausdehnung wird
**                          überschrieben!
**      08-Apr-97   floh    Höhen/Seiten-Verhältnis des
**                          Energie-Fensters verändert
*/
{
    LONG width;
    BOOL retval  = FALSE;
    UBYTE *title = ypa_GetStr(ywd->LocHandle, STR_WIN_ENERGY, "ENERGY");

    /*** Größen-Verhältnis ca. 4 (Breite) zu 4 (Höhe) ***/
    width = (ywd->Fonts[FONTID_ENERGY]->height*5*4) / 4;

    if (yw_InitListView(ywd, &ER,
        LIST_Title,        title,
        LIST_Resize,       FALSE,
        LIST_NumEntries,   5,
        LIST_ShownEntries, 5,
        LIST_FirstShown,   0,
        LIST_Selected,     0,
        LIST_MaxShown,     5,
        LIST_MinShown,     5,
        LIST_DoIcon,       FALSE,
        LIST_EntryHeight,  ywd->Fonts[FONTID_ENERGY]->height,
        LIST_EntryWidth,   width,
        LIST_Enabled,      TRUE,
        LIST_VBorder,      ywd->EdgeH,
        LIST_StaticItems,  TRUE,
        LIST_CloseChar,    'K',
        TAG_DONE))
    {
        /*** initialisiere Position und Ausdehnung nach Prefs ***/
        if (ywd->Prefs.valid) {
            struct YPAWinPrefs *p = &(ywd->Prefs.WinEnergy);
            yw_ListSetRect(ywd,&ER,p->rect.x,p->rect.y);
        };

        retval = TRUE;
    };

    /*** Ende ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillER(struct ypaworld_data *ywd)
/*
**  CHANGED
**      20-Aug-96   floh    created
*/
{
    yw_KillListView(ywd,&ER);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_ERLayoutBar(struct ypaworld_data *ywd,
                      UBYTE *str,
                      UBYTE type_char,
                      ULONG filled_width,
                      ULONG needed_width,
                      ULONG empty_width)
/*
**  FUNCTION
**      Universeller Energie-Balken-Renderer.
**      FONTID_ENERGY muß bereits eingestellt sein!
**
**  INPUTS
**      ywd            - Ptr auf LID des Welt-Objects
**      type_char      - Buchstabe für Batterie-Typ
**      filled_width   - Breite des Füllstand-Balkens in Pixel
**      needed_width   - Breite des Needed-Balkens in Pixel (oder 0)
**
**  CHANGED
**      20-Aug-96   floh    created
*/
{
    put(str,'a');       // linker Fensterrand
    put(str,'h');       // Leerzeichen
    put(str,type_char); // Batterie-Typ
    put(str,'c');       // linker Energie-Balken-Rand
    if (filled_width > 0) {
        stretch(str,filled_width);
        put(str,'f');
    };
    if (needed_width > 0) {
        stretch(str,needed_width);
        put(str,'g');
    };
    if (empty_width > 0) {
        stretch(str,empty_width);
        put(str,'e');
    };
    put(str,'d');       // rechter Energie-Balken-Rand
    put(str,'h');       // Leerzeichen
    put(str,'b');       // rechter Fenster-Rand
    new_line(str);

    return(str);
}

/*-----------------------------------------------------------------*/
void yw_ERLayoutItems(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet den gesamten ItemBlock des Energie-Windows.
**
**  CHANGED
**      20-Aug-96   floh    created
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + BP_Array nicht mehr global
*/
{
    UBYTE *str = ER.Itemblock;
    LONG inner_w, filled_w, needed_w, rest_w;
    UBYTE type_char;
    ULONG fill_modus, b_sys, b_vehicle, b_build, b_beam, b_max;
    LONG load_stat;     // -1 -> Entladung, +1 Aufladung, 0 keine Änderung
    struct getrldratio_msg grm;
    LONG efact,reload;

    /*** Bitmaske, aktive, inaktive Batterien ***/
    _method(ywd->UserRobo, OM_GET,
            YRA_FillModus,    &fill_modus,
            YRA_BattVehicle,  &b_vehicle,
            YRA_BattBuilding, &b_build,
            YRA_BattBeam,     &b_beam,
            TAG_DONE);
    b_sys = ywd->URBact->Energy;
    b_max = ywd->URBact->Maximum;

    /*** oberen Rand rendern ***/
    str = yw_LVItemsPreLayout(ywd, &ER, str, FONTID_DEFAULT, "{ }");

    /*** Font initialisieren ***/
    newfont_flush(str,FONTID_ENERGY);

    /*** innere Gesamtbreite des Energie-Balkens ***/
    inner_w = ER.ActEntryWidth -
              4 * ywd->EdgeW -
              ywd->Fonts[FONTID_ENERGY]->fchars['A'].width -
              2 * ywd->Fonts[FONTID_ENERGY]->fchars['h'].width;

    /*** Stoudson-Absorber ***/
    grm.owner = ywd->UVBact->Sector->Owner;
    _methoda(ywd->world,YWM_GETRLDRATIO,&grm);
    efact  = ywd->UVBact->Sector->EnergyFactor;
    reload = (LONG) (efact * grm.ratio);
    if (ywd->UVBact->Owner == ywd->UVBact->Sector->Owner) {
        if (reload == 0) {
            type_char = 'S'; // keine Änderung
            load_stat = 0;
        } else {
            type_char = 'Q'; // Aufladung
            load_stat = +1;
        };
    } else {
        if (reload == 0) {
            type_char = 'S'; // keine Änderung
            load_stat = 0;
        } else {
            type_char = 'R'; // Entladung
            load_stat = -1;
            reload    = efact;  // bei Entladung keine Effizienz-Anzeige
        };
    };
    needed_w = ((efact-reload)*inner_w)/256;
    filled_w = (efact*inner_w)/256 - needed_w;
    rest_w   = inner_w - filled_w - needed_w;
    str = yw_ERLayoutBar(ywd,str,type_char,filled_w,needed_w,rest_w);

    /*** System-Batterie ***/
    if (fill_modus & YRF_Fill_System) {
        switch(load_stat) {
            case -1:    type_char = 'B'; break;     // Entladung
            case  0:    type_char = 'C'; break;     // keine Änderung
            case +1:    type_char = 'A'; break;     // Aufladung
        };
    } else {
        type_char = 'D';    // Systembatterie inaktiv (Cant Happen)
    };
    needed_w = 0;
    filled_w = (b_sys * inner_w)/b_max;
    rest_w = inner_w - filled_w - needed_w;
    str = yw_ERLayoutBar(ywd,str,type_char,filled_w,needed_w,rest_w);

    /*** Vehicle-Batterie ***/
    if (fill_modus & YRF_Fill_Vehicle) {
        switch(load_stat) {
            case -1:    type_char = 'F'; break;     // Entladung
            case  0:    type_char = 'G'; break;     // keine Änderung
            case +1:    type_char = 'E'; break;     // Aufladung
        };
    } else {
        type_char = 'H';    // inaktiv
    };
    needed_w = 0;
    if (SR.ActiveMode & (STAT_MODEF_NEW|STAT_MODEF_ADD)) {
        if (SR.ActVehicle != -1) {
            struct VehicleProto *vp = &(ywd->VP_Array[SR.VPRemap[SR.ActVehicle]]);
            needed_w = (vp->Energy*CREATE_ENERGY_FACTOR*inner_w)/b_max;
        };
    };
    filled_w = (b_vehicle * inner_w)/b_max;
    if (needed_w > filled_w) {
        needed_w = filled_w;
        filled_w = 0;
    } else filled_w -= needed_w;
    rest_w = inner_w - filled_w - needed_w;
    str = yw_ERLayoutBar(ywd,str,type_char,filled_w,needed_w,rest_w);

    /*** Building-Batterie ***/
    if (fill_modus & YRF_Fill_Build) {
        switch(load_stat) {
            case -1:    type_char = 'J'; break;     // Entladung
            case  0:    type_char = 'K'; break;     // keine Änderung
            case +1:    type_char = 'I'; break;     // Aufladung
        };
    } else {
        type_char = 'L';    // inaktiv
    };
    needed_w = 0;
    if (SR.ActiveMode & (STAT_MODEF_BUILD)) {
        if (SR.ActBuilding != -1) {
            struct BuildProto *bp = &(ywd->BP_Array[SR.BPRemap[SR.ActBuilding]]);
            needed_w = (bp->CEnergy*inner_w)/b_max;
        };
    };
    filled_w = (b_build * inner_w)/b_max;
    if (needed_w > filled_w) {
        needed_w = filled_w;
        filled_w = 0;
    } else filled_w -= needed_w;
    rest_w = inner_w - filled_w - needed_w;
    str = yw_ERLayoutBar(ywd,str,type_char,filled_w,needed_w,rest_w);

    /*** Beam-Batterie ***/
    if (fill_modus & YRF_Fill_Beam) {
        switch(load_stat) {
            case -1:    type_char = 'N'; break;     // Entladung
            case  0:    type_char = 'O'; break;     // keine Änderung
            case +1:    type_char = 'M'; break;     // Aufladung
        };
    } else {
        type_char = 'P';    // inaktiv
    };
    needed_w = 0;
    if (SR.ActiveMode & (STAT_MODEF_AUTOPILOT)) {
        /*** HACK! abhängig von yw_select.c/yw_BeamCheck()! ***/
        needed_w = (ywd->TLMsg.energy*inner_w)/b_max;
    };
    filled_w = (b_beam * inner_w)/b_max;
    if (needed_w > filled_w) {
        needed_w = filled_w;
        filled_w = 0;
    } else filled_w -= needed_w;
    rest_w = inner_w - filled_w - needed_w;
    str = yw_ERLayoutBar(ywd,str,type_char,filled_w,needed_w,rest_w);

    /*** unterer Rand ***/
    str = yw_LVItemsPostLayout(ywd, &ER, str, FONTID_DEFAULT, "xyz");

    /*** EOS ***/
    eos(str);
}

/*-----------------------------------------------------------------*/
void yw_HandleInputER(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Input-Handler des ER, man beachte, daß der ER
**      ja eigentlich ein umfunktioniertes Listview ist.
**
**  CHANGED
**      20-Aug-96   floh    created
**      18-Sep-96   floh    + Tooltips
*/
{
    if (!(ER.Req.flags & (REQF_Iconified|REQF_Closed))) {

        struct ClickInfo *ci = &(ip->ClickInfo);

        /*** GUI auswerten ***/
        if (ci->box == &(ER.Req.req_cbox)) {

            /*** beachte: System-Batterie läßt sich nicht deaktivieren! ***/
            if (ci->flags & CIF_BUTTONDOWN) {

                ULONG fill_modus;
                _get(ywd->UserRobo, YRA_FillModus, &fill_modus);

                switch(ci->btn) {

                    /*** Vehikel-Batterie ***/
                    case (LV_NUM_STD_BTN + 2):
                        fill_modus ^= YRF_Fill_Vehicle;
                        break;

                    /*** Build-Batterie ***/
                    case (LV_NUM_STD_BTN + 3):
                        fill_modus ^= YRF_Fill_Build;
                        break;

                    /*** Beam-Batterie ***/
                    case (LV_NUM_STD_BTN + 4):
                        fill_modus ^= YRF_Fill_Beam;
                        break;
                };
                _set(ywd->UserRobo, YRA_FillModus, fill_modus);
            };

            /*** Tooltips ***/
            switch(ci->btn) {
                case (LV_NUM_STD_BTN + 0): yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_RELOAD); break;
                case (LV_NUM_STD_BTN + 1): yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_SYSAKKU); break;
                case (LV_NUM_STD_BTN + 2): yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_VHCLAKKU); break;
                case (LV_NUM_STD_BTN + 3): yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_BUILDAKKU); break;
                case (LV_NUM_STD_BTN + 4): yw_Tooltip(ywd,TOOLTIP_GUI_ENERGY_BEAMAKKU); break;
            };
        };

        /*** Listview-Handling (yw_ListHandleInput() nicht nötig) ***/
        yw_ListHandleInput(ywd, &ER, ip);
        yw_ListLayout(ywd,&ER);

        /*** Itemblock layouten ***/
        yw_ERLayoutItems(ywd);
    };
}

