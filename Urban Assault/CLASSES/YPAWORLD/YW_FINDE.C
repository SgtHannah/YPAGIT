/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_finder.c,v $
**  $Revision: 38.14 $
**  $Date: 1998/01/06 16:19:00 $
**  $Locker: floh $
**  $Author: floh $
**
**  Der neue ultimative Finder-m‰ﬂige Geschwader-Requester :-)
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guifinder.h"
#include "ypa/guilogwin.h"
#include "ypa/ypatooltips.h"
#include "ypa/guimap.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_audio_engine

struct YPAFinder FR;        // Finder-Req-Struktur
extern struct YPALogWin LW;
extern struct YPAStatusReq SR;
extern struct YPAMapReq MR;

/*-----------------------------------------------------------------*/
BOOL yw_InitFinder(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert Finder-Requester.
**
**  CHANGED
**      30-Nov-95   floh    created
**      26-Dec-95   floh    kein Icon mehr
**      19-Mar-96   floh    so, los geht's...
**      27-Jul-96   floh    revised & updated (Dynamic Layout Support)
**      10-Aug-96   floh    + keine Aggr- und Sektor-Owner-Anzeige mehr
**                            im Finder
**                          + Listview mit Custom-Close-Gadget
**      13-Sep-96   floh    + Prefs-Handling
**      25-Sep-96   floh    + Bugfixes im Prefs-Handling bei
**                            verschiedenen Display-Auflˆsungen
**      28-Sep-96   floh    + Bugfix: Y-Prefs-Position wurde falsch
**                            abgehandelt, weil yw_ListSetRect()
**                            das aber sowieso mit abhandelt, habe
**                            ich es rausgenommen.
**                          + je 1 Leerzeichen zwischen Aggressions-
**                            Buttons, Aktions-Icon und Anzahl
**                            Vehikel
**      11-Oct-97   floh    + Prefs-Korrektur per ywd->UpperTabu und
**                            ywd->LowerTabu
**      30-May-98   floh    + Prefs-Handling raus, macht jetzt die
**                            Status-Req-Initialisierung
*/
{
    BOOL retval = FALSE;
    LONG start_aggr = ywd->Fonts[FONTID_DEFAULT]->fchars['{'].width +
                      ywd->Fonts[FONTID_DEFAULT]->fchars[' '].width;
    LONG start_icon = start_aggr +
                      5*ywd->Fonts[FONTID_ICON_NS]->fchars['1'].width +
                      ywd->Fonts[FONTID_DEFAULT]->fchars[' '].width;
    LONG min_entry_width = start_icon +
                           ywd->Fonts[FONTID_TYPE_NS]->fchars['a'].width +
                           4*ywd->Fonts[FONTID_DEFAULT]->fchars['A'].width +
                           ywd->Fonts[FONTID_DEFAULT]->fchars[' '].width +
                           8;   // Sicherheits-Zone

    if (yw_InitListView(ywd, &(FR.l),
             LIST_Title, ypa_GetStr(ywd->LocHandle,STR_WIN_FINDER,"FINDER"),
             LIST_Resize,        TRUE,
             LIST_NumEntries,    0,
             LIST_ShownEntries,  12,
             LIST_FirstShown,    0,
             LIST_Selected,      0,
             LIST_MaxShown,      24,        // Maximal-Wert!
             LIST_MinShown,      3,
             LIST_DoIcon,        FALSE,
             LIST_EntryHeight,   ywd->FontH,
             LIST_EntryWidth,    min_entry_width,
             LIST_MaxEntryWidth, 32000,
             LIST_MinEntryWidth, min_entry_width,
             LIST_Enabled,       TRUE,
             LIST_UpperVBorder,  ywd->EdgeH,
             LIST_LowerVBorder,  ywd->FontH + ywd->EdgeH,
             LIST_CloseChar,     'I',
             TAG_DONE))
    {
        struct VFMFont *f = ywd->Fonts[FONTID_TYPE_NS];

        /*** Dynamic Layout ***/
        FR.type_icon_width = f->fchars['A'].width; // 1.TypeIcon
        FR.leader_pix_off = ywd->EdgeW +               // linker Rand
                            f->fchars['a'].width +     // 1.Aktions-Icon
                            f->fchars['@'].width;      // 3-Pixel-Leerzeichen
        FR.bunch_pix_off  = FR.leader_pix_off +
                            FR.type_icon_width +
                            f->fchars['@'].width;
        FR.lborder_icon = start_icon;
        FR.lborder_aggr = start_aggr;

        retval = TRUE;
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillFinder(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Der Finder-Requester muﬂ korrekt gekillt werden...
**
**  CHANGED
**      30-Nov-95   floh    created
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
*/
{
    /*** eingebettete Listview-Struktur deinitialisieren ***/
    yw_KillListView(ywd, &(FR.l));
}

/*-----------------------------------------------------------------*/
void yw_FinderRemap(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      F¸llt die Bakterien-Remap-Tabelle des Finders
**      mit den Commanders aus der "normalen" Commander-
**      Remap-Tabelle.
**
**      Nach FR.l.NumEntries wird die Anzahl der gescannten
**      Bakterien geschrieben
**
**  CHANGED
**      19-Mar-96   floh    created
**      20-Mar-96   floh    Bedeutend vereinfacht, weil keine
**                          aufgeklappten Geschwader mehr
**                          beachtet werden m¸ssen.
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      08-Dec-97   floh    + 1.Zeile im Finder ist jetzt die Hoststation
**      22-Mar-98   floh    + FirstShown wurde nicht angepasst, wenn
**                            ans Ende gescrollt und die Geschwader
**                            weggeplatzt wurden.
*/
{
    LONG cmdr_count;
    LONG first_shown;

    /*** erstmal prophylaktisch lˆschen... ***/
    memset(&(FR.b_map), 0, sizeof(FR.b_map));

    /*** ... und FirstShown evtl. anpassen ***/
    if ((FR.l.FirstShown+FR.l.ShownEntries)>=(ywd->NumCmdrs+1)) {
       FR.l.FirstShown = (ywd->NumCmdrs+1)-FR.l.ShownEntries;
       if (FR.l.FirstShown < 0) FR.l.FirstShown = 0;
    };

    /*** f¸r jeden Commander... ***/
    cmdr_count = 0;
    first_shown = FR.l.FirstShown;
    for (cmdr_count=0; cmdr_count < (ywd->NumCmdrs+1); cmdr_count++) {
        
        struct Bacterium *cmdr; 
        if (cmdr_count == 0) cmdr = ywd->URBact;
        else                 cmdr = ywd->CmdrRemap[cmdr_count-1];

        /*** sichtbar? ***/
        if (cmdr_count >= first_shown) {
            LONG ix = cmdr_count - first_shown;
            if (ix < FR.l.ShownEntries) FR.b_map[ix] = cmdr;
        };
    };
    
    /*** NumEntries updaten ***/
    FR.l.NumEntries = cmdr_count;

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
UBYTE yw_FRGetTypeIcon(struct ypaworld_data *ywd, struct Bacterium *b)
/*
**  FUNCTION
**      Guckt per TypeID-Index im VehicleProto-Array nach
**      und ermittelt so das TypeIcon der Bakterie. Ist
**      dieses 0, wird 'A' zur¸ckgegeben.
**
**  CHANGED
**      21-Mar-96   floh    created
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      12-Apr-97   floh    VP_Array nicht mehr global
*/
{
    UBYTE c = ywd->VP_Array[b->TypeID].TypeIcon;
    if (c == 0) c='A';
    return(c);
}

/*-----------------------------------------------------------------*/
UBYTE yw_FRGetActionIcon(struct Bacterium *b)
/*
**  FUNCTION
**      Ermittelt Aktions-Icon f¸r das Bakterium, bei welchem
**      es sich um einen Commander handeln sollte.
**
**  CHANGED
**      21-Mar-96   floh    created
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      08-Oct-97   floh    + im Wait-Zustand wird jetzt auch
**                            der Escape-Zustand abgefragt
**
**  NOTE
**      FR_ACTION_FIGHTPRIM und FR_ACTION_FIGHTSEC noch
**      nicht implementiert.
*/
{
    UBYTE c = FR_ACTION_NONE;

    switch(b->MainState) {

        case ACTION_WAIT:
            if (b->ExtraState & EXTRA_ESCAPE) {
                c = FR_ACTION_ESCAPE;
            } else {
                c = FR_ACTION_WAIT;
            };
            break;

        case ACTION_NORMAL:
            if (b->ExtraState & EXTRA_ESCAPE) {
                c = FR_ACTION_ESCAPE;
            } else if (b->SecTargetType != TARTYPE_NONE) {
                c = FR_ACTION_GOTOSEC;
            } else if (b->PrimTargetType != TARTYPE_NONE) {
                c = FR_ACTION_GOTOPRIM;
            };
            break;
    };
    return(c);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_FRBuildCmdrItem(struct ypaworld_data *ywd, 
                          struct Bacterium *cmdr,
                          UBYTE *str)
/*
**  FUNCTION
**      Baut ein geclipptes Finder-Item auf.
**
**  INPUTS
**      ywd  - LID des Welt-Objects
**      cmdr - Bakterien-Pointer des gew¸nschten Commanders
**      str  - Pointer auf Output-Stream
**
**  RESULTS
**      str - Modifizierter Output-Stream-Pointer
**
**  CHANGED
**      19-Mar-96   floh    created
**      20-Mar-96   floh    jetzt ohne PopUp-Gadget am Anfang
**      21-Mar-96   floh    TypeIcons jetzt korrekt
**      21-Mar-96   floh    Bacts im CREATE-Zustand werden fortan
**                          ignoriert
**      23-Mar-96   floh    FIXME: Patch f¸r Aggressions-"Overflow" bei 100
**      27-May-96   floh    + AF's letztes Update
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      10-Aug-96   floh    - Location und Aggr-Anzeige
**      14-Oct-96   floh    + ignoriert jetzt Fahrzeuge im ACTION_BEAM
**                            Zustand
**      29-Jul-97   floh    + rendert einen blinkenden Cursor ¸ber
**                            den Viewer
**                          + rendert einen blinkenden Cursor ¸ber den
**                            Melder der letzten Message
**      08-Dec-97   floh    + wird nur noch Selected dargestellt, wenn
**                            Beamzustand nicht aktiviert
**      20-May-98   floh    + Last-Message-Indikator gefixt...
**      12-Jun-98   floh    + jetzt mit Lifemeter ueber der Selbact
*/
{
    UBYTE str_buf[64];
    ULONG i = 0;
    struct MinList *ls;
    struct MinNode *nd;
    ULONG fnt_id;
    struct Bacterium *lm_bact;
    ULONG act_x = 0;
    ULONG sel_bact_x = 0;
    struct Bacterium *sel_bact = NULL;
    
    /*** gibt es derzeit eine Selbact? ***/
    if (ywd->FrameFlags & YWFF_MouseOverBact) sel_bact = ywd->SelBact;    
    
    /*** hole Sender der letzten Message ***/
    lm_bact = ywd->LastMessageSender;    
    
    /*** Action-Item und Leerzeichen ***/
    str_buf[i++] = yw_FRGetActionIcon(cmdr);
    str_buf[i++] = '@';
    act_x = FR.leader_pix_off;

    /*** String erzeugen (max. 64 Zeichen) ***/
    if ((cmdr == ywd->UVBact) && ((ywd->TimeStamp/300)&1)) {
        /*** falls Viewer, Indikator blinken ***/
        str_buf[i++] = '!';
    }else if ((lm_bact==cmdr) && ((ywd->TimeStamp/300)&1)) {
        /*** falls Last-Message-Originator, Indikator blinken lassen ***/
        str_buf[i++] = '"';
    }else{
        str_buf[i++] = yw_FRGetTypeIcon(ywd,cmdr);
    };
    str_buf[i++] = '@';
    
    /*** ist dieser die Selbact? ***/
    if (cmdr == sel_bact) sel_bact_x = act_x;

    /* Der Rest sind die Slaves */
    act_x = FR.bunch_pix_off; 
    ls = &(cmdr->slave_list);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct Bacterium *b = ((struct OBNode *)nd)->bact;

        /*** nur "untote" Bakterien beachten ***/
        if ((b->MainState != ACTION_DEAD)   &&
            (b->MainState != ACTION_CREATE) &&
            (b->MainState != ACTION_BEAM))
        {
            if ((b == ywd->UVBact) && ((ywd->TimeStamp/300)&1)) {
                /*** falls Viewer, Indikator blinken ***/
                str_buf[i++] = '!';
            }else if ((lm_bact==b) && ((ywd->TimeStamp/300)&1)) {
                /*** falls Last-Message-Originator, Indikator blinken lassen ***/
                str_buf[i++] = '"';
            }else{
                str_buf[i++] = yw_FRGetTypeIcon(ywd,b);
            };
            if (i >= 60) break; // damit kein Overflow auftritt
        };
        if (sel_bact == b) sel_bact_x = act_x;
        act_x += FR.type_icon_width;
    };
    str_buf[i++] = 0;

    /*** Font-ID (Selected/Nonselected) ***/
    if ((ywd->ActCmdr != -1) && 
        (ywd->CmdrRemap[ywd->ActCmdr] == cmdr) && 
        (!(SR.ActiveMode & STAT_MODEF_AUTOPILOT))) 
    {
        /*** Selected ***/
        fnt_id = STAT_MENUDOWN_FONT;
    } else {
        /*** Non-Selected ***/
        fnt_id = STAT_MENUUP_FONT;
    };

    /*** Hintergrund ***/
    new_font(str,STAT_MENUUP_FONT);
    put(str,'{');
    new_font(str,fnt_id);
    lstretch_to(str,FR.l.ActEntryWidth - ywd->EdgeW);
    put(str,' ');
    new_font(str,STAT_MENUUP_FONT);
    put(str,'}');

    /*** Cursor auf Start des str_buf-Strings ***/
    xpos_rel(str,-(FR.l.ActEntryWidth - 2*ywd->EdgeW + 1));

    /*** und das Geschwader (geclippt) ***/
    new_font(str,FONTID_TYPE_NS);
    str = yw_BuildClippedItem(ywd->Fonts[FONTID_TYPE_NS],
                              str, str_buf,
                              FR.l.ActEntryWidth - 2*ywd->EdgeW,
                              '@');
    
    /*** evtl. Lebensbalken ueber die Selbact ***/
    if (sel_bact_x && (sel_bact_x < (FR.l.ActEntryWidth - FR.type_icon_width - ywd->EdgeW))) {
        UBYTE chr;
        FLOAT nrg = ((FLOAT)sel_bact->Energy) / ((FLOAT)sel_bact->Maximum);
        xpos_rel(str,-(FR.l.ActEntryWidth-2*ywd->EdgeW));
        xpos_rel(str,sel_bact_x);
        if      (nrg <= 0.25) chr=128;
        else if (nrg <= 0.5)  chr=129;
        else if (nrg <= 0.75) chr=130;
        else                  chr=131;
        put(str,chr);
    };
    new_line(str);

    /*** das war's auch schon... ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_FRBuildRoboItem(struct ypaworld_data *ywd, 
                          struct Bacterium *cmdr,
                          UBYTE *str)
/*
**  FUNCTION
**      Baut Finder-Item fuer den Robo auf.
**
**  INPUTS
**      ywd  - LID des Welt-Objects
**      cmdr - Bakterien-Pointer des gew¸nschten Commanders
**      str  - Pointer auf Output-Stream
**
**  RESULTS
**      str - Modifizierter Output-Stream-Pointer
**
**  CHANGED
**      08-Dec-97   floh    created
*/
{
    UBYTE str_buf[64];
    ULONG i = 0;
    ULONG fnt_id;

    /* Space, Anf¸hrer, Space */
    if ((cmdr == ywd->UVBact) && ((ywd->TimeStamp/300)&1)) {
        /*** falls Viewer, Indikator blinken ***/
        str_buf[i++] = '!';
    }else if ((LW.lm_senderid == cmdr->ident) &&
              ((ywd->TimeStamp - LW.line_buf[LW.last_log].time_stamp)<10000) &&
              ((ywd->TimeStamp/300)&1))
    {
        /*** falls Last-Message-Originator, Indikator blinken lassen ***/
        str_buf[i++] = '"';
    }else{
        str_buf[i++] = yw_FRGetTypeIcon(ywd,cmdr);
    };
    str_buf[i++] = 0;

    /*** Font-ID (Selected/Nonselected) (abhaengig von Beam-Zustand) ***/
    if (SR.ActiveMode & STAT_MODEF_AUTOPILOT) {
        /*** Selected ***/
        fnt_id = STAT_MENUDOWN_FONT;
    } else {
        /*** Non-Selected ***/
        fnt_id = STAT_MENUUP_FONT;
    };

    /*** Hintergrund ***/
    new_font(str,STAT_MENUUP_FONT);
    put(str,'{');
    new_font(str,fnt_id);
    lstretch_to(str,FR.l.ActEntryWidth - ywd->EdgeW);
    put(str,' ');
    new_font(str,STAT_MENUUP_FONT);
    put(str,'}');

    /*** Cursor auf Start des str_buf-Strings ***/
    xpos_rel(str,-(FR.l.ActEntryWidth - 2 * ywd->EdgeW + 1));

    /*** und das Geschwader ***/
    new_font(str,FONTID_TYPE_NS);
    str = yw_BuildClippedItem(ywd->Fonts[FONTID_TYPE_NS],
                              str, str_buf,
                              FR.l.ActEntryWidth - 2*ywd->EdgeW,
                              '@');
    new_line(str);

    /*** das war's auch schon... ***/
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_FRBuildEmptyItem(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Erzeugt ein leeres Item...
**
**  CHANGED
**      19-Mar-96   floh    created
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
*/
{
    WORD w = FR.l.ActEntryWidth - ywd->EdgeW;

    /*** Nonselected-Item-Font ***/
    new_font(str,STAT_MENUUP_FONT);
    put(str,'{');           // linker Rand
    lstretch_to(str,w);
    put(str,' ');           // Mitte
    put(str,'}');           // rechter Rand
    new_line(str);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_FRLayoutLowerBorder(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Layoutet den unteren Rand des Finders, mit
**      Status-Infos und Aggressions-Icons.
**
**  CHANGED
**      22-Jul-97   floh    created
**      01-Dec-97   floh    Anzahl Fahrzeuge im Statusbalken DBCS enabled.
**      08-Dec-97   floh    falls Beamen aktiviert, wird nichts gerendert 
*/
{
    WORD w = FR.l.ActEntryWidth - ywd->EdgeW;
    if ((ywd->ActCmdr != -1) && (!(SR.ActiveMode & STAT_MODEF_AUTOPILOT))) {
        LONG aggr = ywd->CmdrRemap[ywd->ActCmdr]->Aggression;
        UBYTE num_buf[32];
        new_font(str,FONTID_DEFAULT);
        put(str,'{');   // linker Rand
        put(str,' ');   // 1 Leerzeichen

        /*** Aggressions-Icons ***/
        new_font(str,FONTID_ICON_PS);
        put(str,'1');   // Aggr 0
        if (aggr < 25) { new_font(str,FONTID_ICON_NS); };
        put(str,'2');   // Aggr 1
        if ((aggr >= 25) && (aggr < 50)) { new_font(str,FONTID_ICON_NS); };
        put(str,'3');   // Aggr 2
        if ((aggr >= 50) && (aggr < 75)) { new_font(str,FONTID_ICON_NS); };
        put(str,'4');   // Aggr 3
        if ((aggr >= 75) && (aggr < 100)) { new_font(str,FONTID_ICON_NS); };
        put(str,'5');

        /*** Anzahl Slaves im Selected Squad ***/
        new_font(str,FONTID_DEFAULT);
        sprintf(num_buf," %d",ywd->NumSlaves+1);
        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_DEFAULT),yw_Green(ywd,YPACOLOR_TEXT_DEFAULT),yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT));
        str = yw_TextBuildClippedItem(ywd->Fonts[FONTID_DEFAULT],str,num_buf,
              4*ywd->Fonts[FONTID_DEFAULT]->fchars['A'].width,' ');
        put(str,' ');

        /*** den Rest rendern ***/
        new_font(str,FONTID_DEFAULT);
        lstretch_to(str,w);
        put(str,' ');
        put(str,'}');

    } else {
        /*** kein Squad ausgew‰hlt, einen leeren Balken rendern ***/
        new_font(str,FONTID_DEFAULT);
        put(str,'{');           // linker Rand
        lstretch_to(str,w);
        put(str,' ');           // Mitte
        put(str,'}');           // rechter Rand
    };

    /*** rendere den unteren Standard-Rand ***/
    new_line(str);
    w = FR.l.ActEntryWidth;
    newfont_flush(str,FONTID_TYPE_NS);
    off_vert(str,(ywd->Fonts[FONTID_TYPE_NS]->height-ywd->EdgeH));
    put(str,'&');   // Ecke links unten
    lstretch_to(str,(w-ywd->EdgeW));
    put(str,'/');   // Rand unten
    put(str,'=');   // Ecke rechts unten
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_FRPutDragIcon(struct ypaworld_data *ywd, UBYTE *str, 
                        LONG icon_x, LONG icon_y)
/*
**  CHANGED
**      12-Jun-98   floh    created
*/
{
    /*** falls geclippt, nicht zeichnen ***/
    if ((icon_x > 0) && (icon_y > 0) &&
        ((icon_x + FR.type_icon_width) < ywd->DspXRes) &&
        ((icon_y + ywd->FontH) < ywd->DspYRes))
    {
        WORD xpos = icon_x - (ywd->DspXRes>>1);
        WORD ypos = icon_y - (ywd->DspYRes>>1);
        new_font(str,FONTID_TYPE_NS);
        pos_abs(str,xpos,ypos);
        put(str,yw_FRGetTypeIcon(ywd,FR.start_drag));
    };
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_FRLayoutItems(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Malt den ItemBlock im Finder.
**
**  CHANGED
**      19-Mar-96   floh    created
**      26-Mar-96   floh    + gedraggtes Type-Icon
**      27-May-96   floh    + Commander-Check jetzt per (master == robo)
**      27-Jul-96   floh    + revised & updated (Dynamic Layout)
**      10-Aug-96   floh    + oberen Rand an neues Layout angepaﬂt
**      07-Jul-97   floh    + Drag-Icon wird nur gezeichnet, wenn
**                            tatsaechlich eine Mausbewegung passiert
**                            ist.
**      12-Jun-98   floh    + Multidragging
*/
{
    struct ClickInfo *ci = &(ip->ClickInfo);
    UBYTE *str = FR.l.Itemblock;
    ULONG i;

    /*** oberen Rand rendern ***/
    str = yw_LVItemsPreLayout(ywd, &(FR.l), str, FONTID_DEFAULT, "{ }");
    new_font(str,FONTID_TYPE_NS);

    /*** Items rendern ***/
    for (i=0; i<FR.l.ShownEntries; i++) {
        struct Bacterium *act_bact = FR.b_map[i];
        if (act_bact) {
            if (act_bact == ywd->URBact) {
                /*** der Robo ***/
                str = yw_FRBuildRoboItem(ywd, act_bact, str);
            } else if (act_bact->master == act_bact->robo) {
                /*** ein Commander ***/
                str = yw_FRBuildCmdrItem(ywd, act_bact, str);
            } else {
                /*** can't happen ***/
                str = yw_FRBuildEmptyItem(ywd, str);
            };
        } else {
            /*** "Phantom-Item" ***/
            str = yw_FRBuildEmptyItem(ywd, str);
        };
    };

    /*** unteren Rand rendern ***/
    str = yw_FRLayoutLowerBorder(ywd,str);

    /*** evtl. Drag-Type-Icon rendern ***/
    if ((FR.flags & FRF_Dragging) &&
        ((FR.start_x!=ci->act.scrx) || (FR.start_y!=ci->act.scry)))
    {
        str = yw_FRPutDragIcon(ywd,str,FR.icon_x,FR.icon_y);    

        /*** bei Multidragging aktuelles Item 2x zeichnen ***/
        if (FR.flags & FRF_IsMultiDrag) {
            str = yw_FRPutDragIcon(ywd, str,
                     FR.icon_x + (FR.type_icon_width>>2), 
                     FR.icon_y + (ywd->FontH>>2));
        };
    };

    /*** EOS ***/
    eos(str);

    /*** dat wars ***/
}

/*-----------------------------------------------------------------*/
void yw_LayoutFR(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Pre-Input-Layouter des Finders, ziemlich leer...
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      01-Dec-95   floh    created
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
*/
{
    if (!(FR.l.Req.flags & (REQF_Iconified|REQF_Closed))) {
        /*** derzeit leer (vielleicht eh alles per HandleInput!) ***/
    };
}

/*-----------------------------------------------------------------*/
struct Bacterium *yw_FRGetBactUnderMouse(struct ypaworld_data *ywd, LONG item_num, LONG x)
/*
**  FUNCTION
**      Guckt im ¸bergebenen Finder-Item an der x-Koordinate
**      nach, welches Typ-Icon dort zu finden ist. Diesem
**      wird dann seine Bakterien-Struktur zugewiesen und
**      returniert. Wird vom Select-Zeugs (in yw_select.c)
**      verwendet, wenn exakt eine Bakterie ausgew‰hlt
**      werden muﬂ [im Gegensatz zur Auswahl eines Commanders,
**      das ist ja easy :-)].
**
**  INPUTS
**      item_num    - Item-Nummer im Finder-Listview (FirstShown
**                    ist dabei == 0!!!)
**      x           - (Mouse-) X-Koordinate innerhalb des Item-Buttons
**
**  RESULTS
**      Ein Bakterien-Pointer (der Leader, oder ein Untergebener)
**      oder NULL, wenn die Mouse nicht ¸ber einem Typ-Icon ist.
**
**  CHANGED
**      20-Mar-96   floh    created
**      21-Mar-96   floh    tote Bacts werden ignoriert
**      26-Mar-96   floh    F¸llt jetzt FR.squad_num und
**                          FR.unit_num als "Abfall-Information" aus,
**                          z.B. ganz n¸tzlich f¸r Drag'n'Drop im Finder.
**      27-May-96   floh    AF's letztes Update
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      14-Oct-96   floh    + ignoriert Fahrzeuge im ACTION_BEAM Zustand
**      08-Dec-97   floh    + Achtung!, kann jetzt auch einen Robo-Pointer 
**                            zurueckliefern!
*/
{
    struct Bacterium *cmdr = FR.b_map[item_num];

    if (cmdr) {
        if (cmdr == ywd->URBact) {
            /*** der Robo... ***/
            FR.squad_num = -1;
            FR.unit_num  = -1;
            return(cmdr);
        } else {
            /*** ein normaler Commander ***/
            FR.squad_num = item_num + FR.l.FirstShown - 1;
            FR.unit_num  = -1;

            /*** ist es der Commander itself? ***/
            if ((x > FR.leader_pix_off) && (x < (FR.leader_pix_off+FR.type_icon_width))) {
                return(cmdr);
            };

            /*** sonst evtl. ein Untergebener... ***/
            x -= FR.bunch_pix_off;
            if (x >= 0) {

                LONG slave_num = x / FR.type_icon_width;
                struct MinList *ls;
                struct MinNode *nd;

                FR.unit_num = slave_num;

                /*** Slave an Position <slave_num> zur¸ckgeben... ***/
                ls = &(cmdr->slave_list);
                for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

                    struct Bacterium *b = ((struct OBNode *)nd)->bact;

                    /*** nur "untote" Bakterien beachten ***/
                    if ((b->MainState != ACTION_DEAD)  &&
                        (b->MainState != ACTION_CREATE)&&
                        (b->MainState != ACTION_BEAM)) {
                        if (slave_num-- == 0) return(b);
                    };
                };
            };
        };
    };

    /*** kein Erfolg... ***/
    FR.squad_num = -1;
    FR.unit_num  = -1;
    return(NULL);
}

/*-----------------------------------------------------------------*/
BOOL yw_FRStartDragging(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Leitet einen Drag-Vorgang im Finder ein. Falls alles
**      glatt geht, werden folgende Felder im Finder initialisiert:
**
**          FR.drag_squad_num   -> gefilterte Geschwader-Nummer des Draggers
**          FR.drag_unit_num    -> gefilterte Unit-Nummer des Draggers
**          FR.start_drag       -> Bact-Pointer des Draggers itself
**          FR.orig_x,orig_y    -> Store-Koords f¸r Dragging
**          FR.icon_x,icon_y    -> entspricht hier [orig_x,orig_y]
**
**      Bedingung: der Mousepointer muss ueber dem Finder sein
**      und ein CIF_RMOUSEDOWN-Event muss aufgetreten sein.
**
**      Die Routine kommt TRUE zur¸ck, wenn ein Drag-Vorgang
**      stattfinden kann, sonst FALSE. Die angegebenen Parameter
**      sind nur bei return(TRUE) g¸ltig.
**
**  CHANGED
**      26-Mar-96   floh    created
**      27-May-96   floh    Commander-Check jetzt per (master == robo)
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      14-Aug-97   floh    wenn im Control-Modus, wird Dragging generell
**                          NICHT aktiviert
**      08-Dec-97   floh    + Robo laesst sich nicht draggen
**      11-Dec-97   floh    + zentriert Map auf ausgewaehlter Bact
**      13-Dec-97   floh    + Bugfix: Map zentrierte auch bei linkem Maus-
**                            Klick
*/
{
    /*** ist Mouse ¸ber einem g¸ltigen Item und Control aus? ***/
    if ((ci->btn >= LV_NUM_STD_BTN) && (!(SR.ActiveMode & STAT_MODEF_CONTROL))) {

        struct Bacterium *b;
        LONG item,entry;

        item  = ci->btn - LV_NUM_STD_BTN;
        entry = item + FR.l.FirstShown;

        if (entry < FR.l.NumEntries) {

            /*** ist ein Bakterium unter dem Mousepointer? ***/
            if (b = yw_FRGetBactUnderMouse(ywd, item, ci->act.btnx)) {

                /*** Map zentrieren ***/
                if ((!(MR.req.flags & REQF_Closed)) && (ci->flags & CIF_RMOUSEDOWN)) {
                    MR.midx = b->pos.x;
                    MR.midz = b->pos.z;
                    MR.lock_mode  = MAP_LOCK_NONE;
                    yw_CheckMidPoint(ywd,MR.xscroll.size-MR.BorLeftW,MR.yscroll.size);
                };

                /*** Draggen ***/
                if (b != ywd->URBact) {
                    /*** ok, kann los gehen ***/
                    FR.drag_squad_num = FR.squad_num;
                    FR.drag_unit_num  = FR.unit_num;
                    FR.start_drag     = b;

                    /*** Ausgleich f¸r "Pseudo-Buttons" ***/
                    FR.start_x = ci->act.scrx;
                    if (b->master == b->robo) {
                        FR.orig_x = -(ci->act.btnx - FR.leader_pix_off);
                    } else {
                        FR.orig_x = -((ci->act.btnx-FR.bunch_pix_off)%FR.type_icon_width);
                    };
                    FR.orig_y  = -ci->act.btny;
                    FR.start_y = ci->act.scry;
                    return(TRUE);
                };
            };
        };
    };

    /*** Fehler-Ende ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL yw_FRDoDragging(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Testet, ob die beim Dragging-Start ausgew‰hlte Bakterie
**      noch g¸ltig ist, wenn dem so ist, kommt die
**      Routine TRUE zur¸ck (die Icon-Position muﬂ auﬂerhalb
**      ermittelt werden).
**
**  CHANGED
**      26-Mar-96   floh    created
**      27-May-96   floh    AF's letztes Update
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      14-Oct-96   floh    + ignoriert Fahrzeuge im ACTION_BEAM Zustand
*/
{
    struct Bacterium *cmdr = ywd->CmdrRemap[FR.drag_squad_num];
    if (cmdr) {

        struct MinList *ls;
        struct MinNode *nd;

        /*** erst Testen, ob der Commander itself ***/
        if ((cmdr == FR.start_drag) &&
            (cmdr->MainState != ACTION_DEAD)   &&
            (cmdr->MainState != ACTION_CREATE) &&
            (cmdr->MainState != ACTION_BEAM))
        {
            /*** Erfolg ***/
            return(TRUE);
        };

        /*** es kann aber auch ein Slave gewesen sein... ***/
        ls = &(cmdr->slave_list);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

            struct Bacterium *b = ((struct OBNode *)nd)->bact;

            /*** Test gegen die Drag-Bakterie (nur Untote beachten) ***/
            if ((b == FR.start_drag)          &&
                (b->MainState != ACTION_DEAD) &&
                (b->MainState != ACTION_CREATE) &&
                (b->MainState != ACTION_BEAM))
            {
                /*** Erfolg! ***/
                return(TRUE);
            };
        };
    };

    /*** Bakterie ist verschollen... ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_FREndDragging(struct ypaworld_data *ywd, struct ClickInfo *ci)
/*
**  FUNCTION
**      Beendet den aktuellen Drag-Vorgang und loest u.U.
**      den Transfer aus...
**
**  CHANGED
**      26-Mar-96   floh    created
**      05-Mar-96   floh    debugging...
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      10-Apr-97   floh    + Drag-Bact -> New Command funktioniert jetzt
**      08-Dec-97   floh    + falls Ziel die Robo-Zeile ist, Fehler
**      12-Jun-98   floh    + Multidragging Implementierung
*/
{
    ULONG is_cmdr = (FR.start_drag->robo==FR.start_drag->master);
    struct organize_msg om;

    /*** Maus ueber gueltiger Finder-Position? ***/
    if ((ci->box==&(FR.l.Req.req_cbox)) && (ci->btn>=LV_NUM_STD_BTN)) {

        LONG item,entry;
        item  = ci->btn - LV_NUM_STD_BTN;
        entry = item + FR.l.FirstShown;
        if (entry < FR.l.NumEntries) {

            /*** ueber gueltigem Geschwader losgelassen: umschichten ***/
            struct Bacterium *cmdr = FR.b_map[item];
            
            if (cmdr && (cmdr != ywd->URBact)) {
            
                /*** falls ueber selben Squad losgelassen, nix machen ***/
                if (is_cmdr) {
                    if (cmdr == FR.start_drag) return;
                } else {
                    if (cmdr == FR.start_drag->master_bact) return;
                };
            
                /*** MultiDrag oder SingleDrag? ***/
                if (FR.flags & FRF_IsMultiDrag) {
                    /*** Multidragging ***/                
                    if (is_cmdr) {                
                        /*** bei einem Commander werden alle Untergebenen automatisch mitgenommen ***/
                        om.mode        = ORG_NEWCHIEF;
                        om.specialbact = cmdr;
                        _methoda(FR.start_drag->BactObject, YBM_ORGANIZE, &om);
                    } else {
                        /*** eine Einzelbakterie nimmt alle Nachfolger mit ***/
                        struct MinNode *nd;
                        struct MinNode *next;
                        nd = &(FR.start_drag->slave_node.nd);                        
                        while (nd->mln_Succ) {
                            Object *o = ((struct OBNode *)nd)->o;
                            next = nd->mln_Succ;                 
                            om.mode        = ORG_NEWCHIEF;
                            om.specialbact = cmdr;
                            _methoda(o, YBM_ORGANIZE, &om);
                            nd = next;
                        };          
                    };
                } else {
                    /*** Singledragging ***/
                    om.mode        = ORG_ADDSLAVE;
                    om.specialbact = FR.start_drag;
                    _methoda(cmdr->BactObject, YBM_ORGANIZE, &om);
                };

                /*** Ziel-Geschwader selecten und Tabellen remappen ***/
                ywd->ActCmdID = cmdr->CommandID;
                yw_FinderRemap(ywd);
                yw_RemapCommanders(ywd);

                /*** Ende ***/
                return;
            };
        };
    };

    /*** ab hier: Drag ging in "leeren" Bereich, oder in die Robo-Zeile ***/
    if (FR.flags & FRF_IsMultiDrag) {

        /*** Multidragging (macht hier nur Sinn, wenn kein Commander) ***/
        if (!is_cmdr) {    
            struct MinNode *nd;
            struct MinNode *next;
            Object *first_object = NULL;
            nd = &(FR.start_drag->slave_node.nd);
            while (nd->mln_Succ) {
                Object *o = ((struct OBNode *)nd)->o;
                struct Bacterium *b = ((struct OBNode *)nd)->bact;
                next = nd->mln_Succ;
                if (!first_object) {
                    /*** das erste Vehicle wird Commander ***/
                    first_object   = o;
                    om.mode        = ORG_NEWCOMMAND;
                    om.specialbact = NULL;
                    _methoda(first_object,YBM_ORGANIZE,&om);
                } else {
                    /*** alle anderen werden an die FirstBact rangehaengt ***/
                    om.mode        = ORG_ADDSLAVE;
                    om.specialbact = b;
                    _methoda(first_object, YBM_ORGANIZE, &om);
                };
                nd = next;
            };
        };
    } else {        
        /*** Single-Drag ***/
        om.mode        = ORG_NEWCOMMAND;
        om.specialbact = NULL;
        _methoda(FR.start_drag->BactObject,YBM_ORGANIZE,&om);
    };

    /*** neues Geschwader selecten und alle Tabellen remappen ***/
    ywd->ActCmdID = FR.start_drag->CommandID;
    yw_FinderRemap(ywd);
    yw_RemapCommanders(ywd);

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void yw_FRDoStatusTooltip(struct ypaworld_data *ywd, struct Bacterium *b)
/*
**  FUNCTION
**      Sucht sich den richtigen Tooltip raus f¸r
**      den Fall das die Maus ¸ber einem der Status-
**      Icons im Finder ist und meldet diesen an.
**
**  CHANGED
**      23-Jul-97   floh    created
*/
{
    switch(b->MainState) {
        case ACTION_WAIT:
            yw_Tooltip(ywd,TOOLTIP_GUI_FINDER_ACTION_WAIT);
            break;

        case ACTION_NORMAL:
            if (b->ExtraState & EXTRA_ESCAPE) {
                yw_Tooltip(ywd,TOOLTIP_GUI_FINDER_ACTION_ESCAPE);
            } else if (b->SecTargetType != TARTYPE_NONE) {
                yw_Tooltip(ywd,TOOLTIP_GUI_FINDER_ACTION_FIGHT);
            } else if (b->PrimTargetType != TARTYPE_NONE) {
                yw_Tooltip(ywd,TOOLTIP_GUI_FINDER_ACTION_GOTO);
            };
            break;
    };
}

/*-----------------------------------------------------------------*/
void yw_HandleInputFR(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Input-Handler des Finders.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      01-Dec-95   floh    created
**      10-May-96   floh    DragLock-Handling
**      27-Jul-96   floh    revised & updated (Dynamic Layout)
**      15-Aug-97   floh    + lˆscht jetzt alle kontinuierlichen Aktionen,
**                            wenn der Finder geschlossen ist, um die
**                            undefinierte "Der Finder wurde mir unter dem
**                            Pointer weggezogen, w‰hrend ich gerade was
**                            gemacht habe" zu eliminieren
**      23-Oct-97   floh    + Sound
**      07-Dec-97   floh    + Robo in Finder uebernommen
**      12-Jun-98   floh    + Multi-Dragging
*/
{
    if (!(FR.l.Req.flags & (REQF_Iconified|REQF_Closed))) {

        struct ClickInfo *ci = &(ip->ClickInfo);

        /*** Commander auf Finder-Items mappen ***/
        yw_FinderRemap(ywd);

        /*** falls Dragging... ***/
        if (FR.flags & FRF_Dragging) {
            /*** Drag-Bakterie noch existent? ***/
            if (yw_FRDoDragging(ywd,ci)) {

                /*** Dragging h‰lt an? ***/
                if ((ci->flags & (CIF_RMOUSEHOLD|CIF_MOUSEHOLD))==
                    (CIF_RMOUSEHOLD|CIF_MOUSEHOLD))
                {
                    /*** beide Maustasten gleichzeitig gedr¸ckt: ***/
                    /*** Dragselect abbrechen                    ***/
                    FR.flags &= ~(FRF_Dragging|FRF_IsMultiDrag);
                } else if (ci->flags & (CIF_RMOUSEHOLD|CIF_MOUSEHOLD)) {
                    /*** eine der beiden Maustasten:      ***/
                    /*** aktuelle Icon-Position ermitteln ***/
                    FR.icon_x = FR.orig_x + ci->act.scrx;
                    FR.icon_y = FR.orig_y + ci->act.scry;
                } else {
                    /*** irgendwas anderes: Dragging beenden ***/
                    yw_FREndDragging(ywd,ci);
                    FR.flags &= ~(FRF_Dragging|FRF_IsMultiDrag);
                };
            } else {
                /*** die Geschwader-Struktur hat sich ver‰ndert, verdammt... ***/
                FR.flags &= ~(FRF_Dragging|FRF_IsMultiDrag);
            };

        } else if (ci->box == &(FR.l.Req.req_cbox)) {

            /*** Online-Hilfe? ***/
            if ((ci->btn == LV_BTN_HELP) && (ci->flags & CIF_BUTTONUP)) {
                ywd->Url = ypa_GetStr(ywd->LocHandle,STR_HELP_INGAMEFINDER,"help\\l15.html");
            };

            /*** Click in Lower Border Button? ***/
            if ((ci->btn == LV_BTN_LOWVBORDER) &&
                (ywd->ActCmdr != -1) &&
                (!(SR.ActiveMode & STAT_MODEF_AUTOPILOT)))
            { 
                /*** ein Aggressions-Button? ***/
                LONG btn = (ci->act.btnx - FR.lborder_aggr) /
                           ywd->Fonts[FONTID_ICON_NS]->fchars['1'].width;
                if ((btn>=0) && (btn<=4) && (ci->flags & CIF_BUTTONDOWN)) {
                    LONG aggr = btn * 25;
                    struct Bacterium *cmdr = ywd->CmdrRemap[ywd->ActCmdr];
                    _set(cmdr->BactObject,YBA_Aggression,aggr);
                    if (ywd->gsr) {
                        _StartSoundSource(&(ywd->gsr->ShellSound1),SHELLSOUND_BUTTON);
                    };
                };

                /*** Aggr-Tooltips abhandeln ***/
                switch(btn) {
                    case 0: yw_TooltipHotkey(ywd,TOOLTIP_GUI_AGGR_0,HOTKEY_AGGR1); break;
                    case 1: yw_TooltipHotkey(ywd,TOOLTIP_GUI_AGGR_1,HOTKEY_AGGR2); break;
                    case 2: yw_TooltipHotkey(ywd,TOOLTIP_GUI_AGGR_2,HOTKEY_AGGR3); break;
                    case 3: yw_TooltipHotkey(ywd,TOOLTIP_GUI_AGGR_3,HOTKEY_AGGR4); break;
                    case 4: yw_TooltipHotkey(ywd,TOOLTIP_GUI_AGGR_4,HOTKEY_AGGR5); break;
                };

                /*** Tooltip f¸r Status-Icon und Vehicle Anzahl ***/
                if (ci->act.btnx >= FR.lborder_icon) {
                    yw_Tooltip(ywd,TOOLTIP_GUI_FINDER_NUMVHCLS);
                };
            };

            /*** Drag-Vorgang einleiten? ***/
            if (ci->flags & (CIF_RMOUSEDOWN|CIF_MOUSEDOWN)) {
                if (yw_FRStartDragging(ywd,ci)) {
                    /*** yo ***/
                    FR.flags &= ~(FRF_Dragging|FRF_IsMultiDrag);
                    FR.flags |= FRF_Dragging;
                    if (ci->flags & CIF_RMOUSEDOWN) FR.flags |= FRF_IsMultiDrag; 
                    FR.icon_x  = FR.orig_x + ci->act.scrx;
                    FR.icon_y  = FR.orig_y + ci->act.scry;
                };
            };

            /*** Tooltip-Handling fuer die aktuelle Geschwader-Aktion ***/
            if (FR.l.MouseItem != -1) {
                LONG item = FR.l.MouseItem - FR.l.FirstShown;
                if ((item >= 0) && (item < FR.l.MaxShown)) {
                    struct Bacterium *b = FR.b_map[item];
                    if (b && (ci->act.btnx < FR.leader_pix_off)) {
                        yw_FRDoStatusTooltip(ywd,b);
                    };
                };
            };
        };

        /*** Listview-Input-Handling, und Layout ***/
        yw_ListHandleInput(ywd, &(FR.l), ip);
        yw_ListLayout(ywd, &(FR.l));

        /*** Itemblock layouten ***/
        yw_FRLayoutItems(ywd,ip);
    } else {
        /*** wenn Finder zu, alle kontinuierlichen Aktionen lˆschen ***/
        FR.flags &= ~FRF_Dragging;
        yw_ListHandleInput(ywd, &(FR.l), ip);
    };
}


