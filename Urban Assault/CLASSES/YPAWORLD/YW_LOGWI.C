/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_logwin.c,v $
**  $Revision: 38.10 $
**  $Date: 1998/01/06 16:23:03 $
**  $Locker: floh $
**  $Author: floh $
**
**  Der Logwindow-Requester, der die Meldungen von auﬂen
**  verwaltet.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <string.h>
#include <stdio.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/bacterium.h"
#include "ypa/guilogwin.h"
#include "ypa/guimap.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_audio_engine
_extern_use_ov_engine

extern struct YPAEnergyBar EB;

struct YPALogWin LW;
UBYTE LW_QuickLogBuf[1024];     // siehe yw_RenderQuickLog()

/*-----------------------------------------------------------------*/
BOOL yw_InitLogWin(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert das LogWindow.
**
**  CHANGED
**      06-May-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      10-Aug-96   floh    + Listview mit Custom-Close-Gadget.
**      13-Sep-96   floh    + Prefs-Handling
**      25-Sep-96   floh    + Bugfixes im Prefs-Handling bei
**                            verschiedenen Display-Auflˆsungen
**      28-Sep-96   floh    + revised & updated
**                          + Bugfix: Y-Prefs-Position wurde falsch
**                            abgehandelt, weil yw_ListSetRect()
**                            das aber sowieso mit abhandelt, habe
**                            ich es rausgenommen.
**      13-Dec-96   floh    + falls (ywd->TimeStamp != 0) wird nicht
**                            die WELCOME-Message angezeigt, sondern
**                            "GAME CONTINUED AT TIME INDEX hh:mm:ss"
**      02-Jan-96   floh    + Fehler in der Game Continued Message
**                            entfernt (Locale Message darf jetzt auch
**                            keine %d mehr enthalten!).
**      22-Jul-97   floh    + VBorder-Fixes
**      11-Oct-97   floh    + Prefs-Ausdehnungskorrektur jetzt mit
**                            ywd->UpperTabu und ywd->LowerTabu
*/
{
    BOOL retval = FALSE;
    ULONG min_w = ywd->Fonts[FONTID_DEFAULT]->fchars['0'].width * 10;

    memset(&(LW),0,sizeof(LW));
    if (yw_InitListView(ywd, &(LW.l),
             LIST_Title, ypa_GetStr(ywd->LocHandle,STR_WIN_LOGWIN,"MESSAGE LOG"),
             LIST_Resize,        TRUE,
             LIST_NumEntries,    1,
             LIST_ShownEntries,  6,
             LIST_FirstShown,    0,
             LIST_Selected,      0,
             LIST_MaxShown,      24,        // Maximal-Wert!
             LIST_MinShown,      3,
             LIST_DoIcon,        FALSE,
             LIST_EntryHeight,   ywd->FontH,
             LIST_EntryWidth,    200,
             LIST_MaxEntryWidth, 32000,
             LIST_MinEntryWidth, min_w,
             LIST_Enabled,       TRUE,
             LIST_VBorder,       ywd->EdgeH,
             LIST_CloseChar,     'J',
             TAG_DONE))
    {
        struct logmsg_msg lm;

        /*** Initialisierung ***/
        if (ywd->TimeStamp == 0) {
            /*** YWM_CREATELEVEL-Initialisierung ***/
            strcpy(LW.line_buf[0].line,ypa_GetStr(ywd->LocHandle,STR_WELCOME, "WELCOME TO YOUR PERSONAL AMOK!"));
        } else {
            /*** YWM_LOADGAME-Initialisierung ***/
            UBYTE *loc_str;
            UBYTE time_str[32];
            LONG sec  = ywd->TimeStamp>>10;
            LONG min  = sec / 60;   // Alter in Minuten
            LONG hour = min / 60;   // Alter in Stunden
            loc_str = ypa_GetStr(ywd->LocHandle,STR_CONTINUED,"GAME CONTINUED AT TIME INDEX %s.");
            sprintf(time_str, "<%02d:%02d:%02d>",hour%24,min%60,sec%60);
            sprintf(LW.line_buf[0].line,loc_str,time_str);
        };

        LW.last_pri = 127;
        LW.line_buf[0].time_stamp = ywd->TimeStamp+1; // darf nicht 0 sein
        LW.lm_senderid  = 0;
        LW.lm_timestamp = ywd->TimeStamp;
        LW.lm_code      = LOGMSG_NOP;

        /*** Dynamic Layout Zeug ***/
        LW.skip_pixels = 6 * ywd->Fonts[LW_ItemFont]->fchars['0'].width +
                         2 * ywd->Fonts[LW_ItemFont]->fchars[':'].width +
                         12;

        /*** Prefs-Handling ***/
        if (ywd->Prefs.valid) {

            struct YPAWinPrefs *p = &(ywd->Prefs.WinLog);

            /*** Ausdehnungs-Korrektur ***/
            if (p->rect.w > ywd->DspXRes) p->rect.w = ywd->DspXRes;
            if (p->rect.h > (ywd->DspYRes-ywd->UpperTabu-ywd->LowerTabu)) p->rect.h =ywd->DspYRes-ywd->UpperTabu-ywd->LowerTabu;
            LW.l.ActEntryWidth = p->rect.w-ywd->PropW;
            LW.l.ShownEntries  = (p->rect.h-ywd->FontH-LW.l.UpperVBorder-LW.l.LowerVBorder)/LW.l.EntryHeight;
            yw_ListSetRect(ywd,&(LW.l),p->rect.x,p->rect.y);
        };

        retval = TRUE;
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_KillLogWin(Object *o, struct ypaworld_data *ywd)
/*
**  FUNCTION
**
**  CHANGED
**      06-May-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    yw_KillListView(ywd, &(LW.l));
}

/*-----------------------------------------------------------------*/
UBYTE *yw_LWBuildEmptyItem(struct ypaworld_data *ywd, UBYTE *str)
/*
**  FUNCTION
**      Erzeugt ein leeres Item...
**
**  CHANGED
**      06-May-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    /*** Nonselected-Item-Font ***/
    new_font(str,LW_ItemFont);
    put(str,'{');   // linker Rand
    lstretch_to(str,(LW.l.ActEntryWidth - ywd->EdgeW));
    put(str,' ');
    put(str,'}');   // rechter Rand
    new_line(str);
    return(str);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_LWBuildItem(struct ypaworld_data *ywd, UBYTE *str, ULONG lnum)
/*
**  FUNCTION
**      Rendert ein "normales" LogWin-Item.
**
**  INPUTS
**      ywd     - Ptr auf LID des Welt-Objects
**      str     - Output-Stream
**      lnum    - Nummer der sichtbaren Zeile
**
**  OUTPUT
**      modifizierter Output-Stream-Pointer
**
**  CHANGED
**      10-May-96   floh    created
**                          + TimeStamp-Handling
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      24-Nov-97   floh    + vorlaeufig auf yw_TextBuildClippedItem
**                            umgestellt.
**      28-Feb-98   floh    + Zeitanzeige eliminiert
*/
{
    LONG act = LW.first + LW.l.FirstShown + lnum;
    WORD w   = LW.l.ActEntryWidth;
    struct LW_Line *l;

    if (act < 0) act += LW_NumLines;
    else if (act >= LW_NumLines) act -= LW_NumLines;

    l = &(LW.line_buf[act]);

    put(str,'{');   // linker Rand

    /*** und der ItemText itself (clipped of course) ***/
    str = yw_TextBuildClippedItem(ywd->Fonts[LW_ItemFont],
          str, l->line, w-2*ywd->EdgeW, ' ');

    put(str,'}');   // rechter Rand
    new_line(str);

    return(str);
}

/*-----------------------------------------------------------------*/
void yw_LWLayoutItems(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Layoutet den ItemBlock im LogWindow.
**
**  CHANGED
**      06-May-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      10-Aug-96   floh    oberen Rand an neues Layout angepaﬂt
*/
{
    ULONG i;
    UBYTE *str = LW.l.Itemblock;

    /*** oberen Rand rendern ***/
    str = yw_LVItemsPreLayout(ywd, &(LW.l), str, LW_ItemFont, "{ }");

    /*** alle Items zeichnen ***/
    for (i=0; i<LW.l.ShownEntries; i++) {
        str = yw_LWBuildItem(ywd, str, i);
    };

    /*** unteren Rand rendern ***/
    str = yw_LVItemsPostLayout(ywd, &(LW.l), str, LW_ItemFont, "xyz");

    /*** EOS ***/
    eos(str);

    /*** dat wars ***/
}

/*-----------------------------------------------------------------*/
void yw_HandleInputLW(struct ypaworld_data *ywd, struct VFMInput *ip)
/*
**  FUNCTION
**      Input-Handler des LogWindows.
**
**  CHANGED
**      06-May-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      15-Aug-97   floh    ruft jetzt yw_ListHandleInput() auch im
**                          geschlossenen Zustand auf, um kontinuierliche
**                          Prozesse korrekt zu lˆschen
*/
{
    if (!(LW.l.Req.flags & (REQF_Iconified|REQF_Closed))) {

        /*** Listview-Input-Handling, und Layout ***/
        yw_ListHandleInput(ywd, &(LW.l), ip);
        yw_ListLayout(ywd, &(LW.l));

        /*** Itemblock layouten ***/
        yw_LWLayoutItems(ywd);
    } else {
        /*** kontinuierliche Prozesse abbrechen ***/
        yw_ListHandleInput(ywd, &(LW.l), ip);
    };
}

/*-----------------------------------------------------------------*/
struct LW_Line *yw_LWNewLine(struct YPALogWin *w)
/*
**  FUNCTION
**      Schaltet neue aktuelle Zeile im Ringbuffer des LogWin,
**      das heiﬂt, <last> wird ver‰ndert, und wenn eine Kollision
**      auftritt, <first> ebenfalls.
**
**  CHANGED
**      10-May-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      09-Aug-96   floh    Bugfix, die <first> und <last> Indizes
**                          verursachten einen Array-Overflow
**                          (-> "Schrott-Linie")
*/
{
    w->last++;
    if (w->last >= LW_NumLines) w->last = 0;
    if (w->last == w->first) {
        w->first++;
        if (w->first >= LW_NumLines) w->first = 0;
    };

    w->l.NumEntries++;
    if (w->l.NumEntries > LW_NumLines) w->l.NumEntries = LW_NumLines;

    return(&(w->line_buf[w->last]));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_LOGMSG, struct logmsg_msg *msg)
/*
**  CHANGED
**      10-May-96   floh    created
**                          + TimeStamp-Handling
**      28-May-96   floh    + Soundausgabe je nach Priorit‰t:
**                              0..100 GUI_NOISE_ALERT1
**                            100..200 GUI_NOISE_ALERT2
**                            200..255 GUI_NOISE_ALERT3
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      10-Aug-96   floh    Falls Priorit‰t der letzten Msg < 5,
**                          wird die letzte Msg ¸berschrieben,
**                          das hat den Effekt, daﬂ die Msg nicht
**                          dauerhaft ins LogWindow aufgenommen wird.
**      28-Sep-96   floh    + die jeweils letzte g¸ltige Message
**                            wird gesondert aufgehoben (f¸r den
**                            Control-2-LastMessage-Button)
**      13-Aug-97   floh    + der Quicklog-Countdown wird pro Zeile
**                            um 5 sec hochgez‰hlt
**                          + <lm_numlines> wird initialisiert
**      22-Sep-97   floh    + Filter f¸r unerlaubte Buchstaben rausgehauen.
**      24-Sep-97   floh    + yw_StartVoiceOver()
**      20-May-98   floh    + wenn es sich um eine VoiceOver Only Message
**                            handelte, klappte das gesamte LastMessage
**                            Handling nicht.
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    /*** VoiceOver starten ***/
    if (msg->code != LOGMSG_NOP) {
        yw_StartVoiceOver(ywd,msg->bact,msg->pri,msg->code);
    };
    
    /*** LastMessage aufheben ***/
    if (msg->bact) LW.lm_senderid = msg->bact->ident;
    else           LW.lm_senderid = 0;
    LW.lm_timestamp = ywd->TimeStamp;
    LW.lm_code      = msg->code;

    /*** h‰ngt auch ein Text dran??? ***/
    if (msg->msg) {

        struct YPALogWin *w = &LW;
        struct LW_Line *line;
        UBYTE *from = msg->msg;
        UBYTE c;
        ULONG pos;

        /*** Linebuffer zur¸cksetzen, oder "Newline"? ***/
        if (w->last_pri < 5) {
            w->last = w->last_log;
            line = &(w->line_buf[w->last]);
        } else {
            line = yw_LWNewLine(w);
            w->last_log = w->last;
        };
        w->last_pri = msg->pri;
        w->quick_log_counter = 5000;
        w->lm_numlines = 1;

        /*** ein etwas intelligenteres Multiline-strcpy() ***/
        pos = 0;
        if (msg->bact) line->sender_id = msg->bact->ident;
        else           line->sender_id = 0;
        line->time_stamp = ywd->TimeStamp;
        line->count_down = LW_Countdown;

        while (c = *from++) {
            if ('\n' == c) {

                /*** momentane Zeile terminieren und neue Zeile ***/
                line->line[pos]   = 0;
                pos = 0;

                line = yw_LWNewLine(w);
                w->quick_log_counter += 5000;
                w->lm_numlines++;
                if (msg->bact) line->sender_id = msg->bact->ident;
                else           line->sender_id = 0;
                line->time_stamp = 0;  // Timecode in dieser Zeile nicht anzeigen
                line->count_down = LW_Countdown;

            } else if (pos < (LW_NumChars-3)) {
                line->line[pos++] = c;
            };
        };

        /*** String terminieren ***/
        line->line[pos] = 0;

        /*** FirstShown korrigieren ***/
        w->l.FirstShown = w->l.NumEntries - w->l.ShownEntries;
        if (w->l.FirstShown < 0) w->l.FirstShown = 0;
    };
}

/*-----------------------------------------------------------------*/
void yw_PutRoboTypeIcon(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Falls der User weniger als 5 Sekunden im Robo sitzt, wird
**      ein blinkendes TypeIcon in der linken oberen Ecke angezeigt.      
**
**  CHANGED
**      26-Nov-97   floh    created
*/
{
    /*** Type-Icon wenn in Robo zur¸ck ***/
    if ((ywd->UVBact == ywd->URBact) &&
        ((ywd->TimeStamp - ywd->UserVehicleTimeStamp) < 5000) &&
        ((ywd->TimeStamp / 500) & 1))
    {
        struct drawtext_args dt;
        UBYTE *str = LW_QuickLogBuf;
        UBYTE c;

        /*** Font + Positionierung ***/
        c = MAP_CHAR_ROBO_BIG + ywd->URBact->Owner;
        new_font(str,FONTID_MAPCUR_16);
        pos_brel(str,ywd->FontH,EB.bar_height+ywd->FontH);
        put(str,c);
        eos(str);

        /*** _DrawText ***/
        dt.string = LW_QuickLogBuf;
        dt.clips  = NULL;
        _DrawText(&dt);
    };
}

/*-----------------------------------------------------------------*/
void yw_RenderQuickLog(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendert die letzte Message (so sie j¸nger
**      als 5 Sekunden ist) in den oberen Teil des Displays,
**      damit man weiﬂ was los ist, ohne das man das
**      LogWindow dauernd offen haben muﬂ.
**
**  CHANGED
**      24-Jun-96   floh    created
**      29-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      10-Aug-96   floh    ‹bernimmt jetzt auch die Kontrolle ¸ber
**                          die Anzeige des TypeIcons, in dem
**                          der User sitzt.
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + WP_Array nicht mehr global
**      13-Aug-97   floh    + Multilines werden linksb¸ndig,
**                            nicht zentriert, angezeigt
**                          + benutzt jetzt den QuickLogCountdown
**                            um zu ermitteln, wie lange die letzte
**                            QuickLogMsg angezeigt werden soll
**      25-Nov-97   floh    + DBCS-Support
**      02-Dec-97   floh    + Beschleuniger fuer viele Zeilen eingebaut.
**      06-Feb-98   floh    + Beschleuniger etwas runtergedreht
**                          + auf 10 Zeilen max begrenzt, weil es einen
**                            Absturz gab, wenn das Display voll war.
**      29-May-98   floh    + Quicklog-Meldungen kommen jetzt am linken Rand
*/
{
    /*** Alter der letzten Message ermitteln ***/
    ULONG act_log;     // = LW.last_log;
    ULONG i;
    UBYTE *str = LW_QuickLogBuf;
    struct drawtext_args dt;
    LONG time_diff, time_stamp;
    ULONG first_hit = FALSE;
    // old: WORD xpos = ywd->DspXRes/5;
    WORD xpos = 16;

    /*** Tech Upgrade, oder Logmsgs anzeigen? ***/
    if (!yw_RenderTechUpgrade(ywd)) {

        /*** maximale Anzahl Zeilen, die dargestellt werden... ***/
        LONG lines;
        if (LW.first > LW.last) {
            lines = (LW_NumLines - LW.first) + LW.last;
        } else {
            lines = (LW.last - LW.first);
        };
        if (lines > 10) lines=10;
        act_log = LW.last - lines;
        if (act_log < 0) act_log = LW_NumLines + act_log;

        /*** fuer jede Zeile... ***/
        new_font(str,FONTID_TRACY);
        ypos_brel(str,EB.bar_height + (ywd->FontH>>1));
        time_diff = time_stamp = 0;
        do {
            
            struct LW_Line *act_line;
            LONG rel_width;

            /*** Ringbuffer-Ueberlauf abfangen und Ptr auf aktuelle Zeile ***/
            if (act_log >= LW_NumLines) act_log=0;
            act_line = &(LW.line_buf[act_log]);

            if (act_line->count_down > 0) {
                /*** eine gueltige Zeile! ***/
                if (!first_hit) {
                    /*** das ist die erste anzuzeigende Zeile ***/
                    long num_lines;
                    if (act_log <= LW.last) num_lines=LW.last-act_log;
                    else                    num_lines=(LW_NumLines-act_log)+LW.last;
                    num_lines++;
                    num_lines /= 2;
                    if (num_lines==0) num_lines=1;
                    first_hit = TRUE;
                    act_line->count_down -= (ywd->FrameTime * num_lines);
                };
                         
                if (act_line->time_stamp) {
                    /*** erste Zeile einer Multiline Message ***/
                    time_stamp = act_line->time_stamp;
                } else {
                    /*** eine weitere Zeile einer Multiline Message ***/
                    time_stamp += 200;
                };

                /*** TimeDiff der aktuellen Zeile (kann negativ sein!) ***/
                time_diff = ywd->TimeStamp - time_stamp;

                if (time_diff < 200) {
                    /*** Message ist juenger als 200 Millisec, reinfliessen lassen ***/
                    rel_width = (time_diff * 100) / 200;
                    if (rel_width < 0)        rel_width = 0;
                    else if (rel_width > 100) rel_width = 100;
                } else {
                    /*** sonst als volle Zeile ***/
                    rel_width = 100;
                };

                /*** String layouten ***/
                if (rel_width > 0) {
                    xpos_brel(str,xpos);
                    dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_MESSAGE),yw_Green(ywd,YPACOLOR_TEXT_MESSAGE),yw_Blue(ywd,YPACOLOR_TEXT_MESSAGE));
                    str = yw_TextRelWidthItem(ywd->Fonts[FONTID_TRACY],str,act_line->line,rel_width,YPACOLF_ALIGNLEFT);
                    new_line(str);
                };
            };

        } while (act_log++ != LW.last);
        
        /*** EOS ***/
        eos(str);
        
        /*** _DrawText ***/
        dt.string = LW_QuickLogBuf;
        dt.clips  = NULL;
        _DrawText(&dt);
    };
}

