/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_tooltip.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 16:29:40 $
**  $Locker:  $
**  $Author: floh $
**
**  "Tooltip-System".
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "input/inputclass.h"
#include "input/idevclass.h"
#include "ypa/ypatooltips.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypagameshell.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine

extern UBYTE LW_QuickLogBuf[];  // aus yw_logwin.c
extern struct key_info GlobalKeyTable[];    // aus yw_gsinit.c

/*-----------------------------------------------------------------*/
BOOL yw_InitTooltips(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Allokiert und initialisiert die Default-Strings für das
**      Tooltip-System, muß innerhalb OM_NEW
**      aufgerufen werden.
**
**  CHANGED
**      17-Sep-96   floh    created
**      28-Sep-96   floh    + TOOLTIP_GUI_CTRL2LM
**      20-Sep-96   floh    + Shell-Tooltips von AF
**      23-Jul-97   floh    + TOOLTIP_GUI_FINDER_NUMVHCLS
**      05-Aug-97   floh    + TOOLTIP_GUI_HELP
**      05-Sep-97   floh    + Tooltips für Debriefing
*/
{
    ywd->DefTooltips = (UBYTE **)
        _AllocVec(MAX_NUM_TOOLTIPS * sizeof(UBYTE *),MEMF_PUBLIC|MEMF_CLEAR);
    if (ywd->DefTooltips) {

        UBYTE **t = ywd->DefTooltips;

        t[TOOLTIP_NONE]        = NULL;
        t[TOOLTIP_GUI_ORDER]   = "GIVE ORDERS TO SELECTED SQUADRON";
        t[TOOLTIP_GUI_FIGHT]   = "SWITCH INTO DEFENSE GUNS";
        t[TOOLTIP_GUI_NEW]     = "CREATE A NEW SQUADRON LEADER";
        t[TOOLTIP_GUI_ADD]     = "ADD A DRONE TO SELECTED SQUADRON";
        t[TOOLTIP_GUI_CONTROL] = "TAKE OVER CONTROL OF A DRONE";
        t[TOOLTIP_GUI_BUILD]   = "CREATE A NEW BUILDING";
        t[TOOLTIP_GUI_BEAM]    = "BEAM TO SOME OTHER PLACE";

        t[TOOLTIP_GUI_MAP]     = "OPEN/CLOSE THE MAP WINDOW";
        t[TOOLTIP_GUI_FINDER]  = "OPEN/CLOSE THE FINDER WINDOW";
        t[TOOLTIP_GUI_LOG]     = "OPEN/CLOSE THE MESSAGE LOG WINDOW";
        t[TOOLTIP_GUI_ENERGY]  = "OPEN/CLOSE THE ENERGY WINDOW";

        t[TOOLTIP_GUI_AGGR_0]  = "AGGR SETTING: ESCAPE NOW";
        t[TOOLTIP_GUI_AGGR_1]  = "AGGR SETTING: ATTACK PRIMARY TARGET ONLY";
        t[TOOLTIP_GUI_AGGR_2]  = "AGGR SETTING: SEARCH AND DESTROY NEARBY ENEMY DRONES";
        t[TOOLTIP_GUI_AGGR_3]  = "AGGR SETTING: ATTACK ALL ENEMY TARGETS ON YOUR WAY";
        t[TOOLTIP_GUI_AGGR_4]  = "AGGR SETTING: GO AMOK";

        t[TOOLTIP_GUI_TOROBO]      = "SWITCH BACK INTO HOST STATION";
        t[TOOLTIP_GUI_TOCMD]       = "SWITCH INTO COMMANDER OF SQUAD";
        t[TOOLTIP_GUI_NEXTUNIT]    = "SWITCH INTO NEXT UNIT OF SQUAD";
        t[TOOLTIP_GUI_NEXTCMDR]    = "SWITCH INTO NEXT SQUADRON COMMANDER";
        t[TOOLTIP_GUI_EXIT]        = "PAUSE OR CANCEL CURRENT MISSION";

        t[TOOLTIP_GUI_MAP_LAND]    = "SHOW/HIDE SECTOR DETAIL LAYER";
        t[TOOLTIP_GUI_MAP_OWNER]   = "SHOW/HIDE SECTOR OWNER LAYER";
        t[TOOLTIP_GUI_MAP_EXTINFO] = "SHOW/HIDE EXTENDED INFO LAYER";
        t[TOOLTIP_GUI_MAP_NOLOCK]  = "FREE MAP SCROLLING";
        t[TOOLTIP_GUI_MAP_LOCKVWR] = "MAP SCROLLING LOCKED ON CURRENT VIEWER";
        t[TOOLTIP_GUI_MAP_LOCKSEL] = "MAP SCROLLING LOCKED ON SELECTED SQUADRON";
        t[TOOLTIP_GUI_MAP_ZOOMOUT] = "ZOOM OUT";
        t[TOOLTIP_GUI_MAP_ZOOMIN]  = "ZOOM IN";
        t[TOOLTIP_GUI_MAP_SIZE]    = "TOGGLE MAP SIZE";

        t[TOOLTIP_GUI_FINDER_ACTION_WAIT]   = "SQUADRON ACTION: AWAITING ORDER";
        t[TOOLTIP_GUI_FINDER_ACTION_FIGHT]  = "SQUADRON ACTION: FIGHT ENEMY";
        t[TOOLTIP_GUI_FINDER_ACTION_GOTO]   = "SQUADRON ACTION: ON THE WAY";
        t[TOOLTIP_GUI_FINDER_ACTION_ESCAPE] = "SQUADRON ACTION: ESCAPING";
        t[TOOLTIP_GUI_FINDER_NUMVHCLS]      = "NUMBER OF VEHICLES IN SELECTED SQUAD";
        t[TOOLTIP_GUI_HELP]                 = "SITUATION ANALYZER";
        t[TOOLTIP_GUI_ONLINEHELP]           = "ONLINE HELP";

        t[TOOLTIP_GUI_ENERGY_RELOAD]    = "ENERGY RELOAD LEVEL";
        t[TOOLTIP_GUI_ENERGY_SYSAKKU]   = "MAIN SYSTEM BATTERY LEVEL";
        t[TOOLTIP_GUI_ENERGY_VHCLAKKU]  = "DRONE CREATION BATTERY LEVEL";
        t[TOOLTIP_GUI_ENERGY_BUILDAKKU] = "BUILDING CREATION BATTERY LEVEL";
        t[TOOLTIP_GUI_ENERGY_BEAMAKKU]  = "BEAM TRANSPORTER BATTERY LEVEL";

        t[TOOLTIP_GUI_MODE]      = "SELECT ANOTHER ACTION MODE";
        t[TOOLTIP_GUI_WEAPONS]   = "SELECT A DEFENSE GUN";
        t[TOOLTIP_GUI_VEHICLES]  = "SELECT A DRONE TYPE";
        t[TOOLTIP_GUI_BUILDINGS] = "SELECT A BUILDING TYPE";
        t[TOOLTIP_GUI_HUD]       = "SHOW/HIDE HEAD UP DISPLAY";
        t[TOOLTIP_GUI_CTRL2LM]   = "CONTROL TO LAST MESSAGE SENDER";

        t[TOOLTIP_ACTION_SELECT]      = "MAKE SQUADRON THE SELECTED ONE";
        t[TOOLTIP_ACTION_GOTO]        = "ADVANCE TO THIS LOCATION";
        t[TOOLTIP_ACTION_ATTACK_SEC]  = "ATTACK THIS SECTOR";
        t[TOOLTIP_ACTION_ATTACK_VHCL] = "ATTACK THIS DRONE";
        t[TOOLTIP_ACTION_NEW]         = "CREATE A NEW SQUADRON LEADER";
        t[TOOLTIP_ACTION_ADD]         = "ADD A NEW DRONE TO SELECTED SQUAD";
        t[TOOLTIP_ACTION_CONTROL]     = "TAKE OVER HAND CONTROL OF THIS DRONE";
        t[TOOLTIP_ACTION_BUILD]       = "CREATE A NEW BUILDING HERE";
        t[TOOLTIP_ACTION_BEAM]        = "BEAM HOST STATION TO THIS LOCATION";

        t[TOOLTIP_ERROR_NOTARGET]       = "ERROR: NO VALID TARGET GIVEN!";
        t[TOOLTIP_ERROR_NOROOM]         = "ERROR: NOT ENOUGH ROOM HERE!";
        t[TOOLTIP_ERROR_NOENERGY]       = "ERROR: CURRENTLY NOT ENOUGH ENERGY!";
        t[TOOLTIP_ERROR_NOTCONQUERED]   = "ERROR: SECTOR MUST BE CONQUERED!";
        t[TOOLTIP_ERROR_TOOFAR]         = "ERROR: LOCATION TOO FAR AWAY!";
        t[TOOLTIP_ERROR_TOOCLOSE]       = "ERROR: LOCATION TOO CLOSE TO HOST STATION!";
        t[TOOLTIP_ERROR_JUSTBUILDING]   = "ERROR: BUILDING CREATION SYSTEM BUSY!";
        t[TOOLTIP_ERROR_VHCLSINSECTOR]  = "ERROR: MUST BE NO DRONES IN TARGET SECTOR!";
        t[TOOLTIP_ERROR_TOOJAGGY]       = "ERROR: TARGET LOCATION TOO JAGGY!";
        t[TOOLTIP_ERROR_UNKNOWNAREA]    = "ERROR: LOCATION NOT IN SENSOR AREA!";

        t[TOOLTIP_SHELL_OCINPUT]        = "OPEN/CLOSE INPUTREQUESTER";
        t[TOOLTIP_SHELL_OCVIDEO]        = "OPEN/CLOSE VIDEOREQUESTER";
        t[TOOLTIP_SHELL_OCSOUND]        = "OPEN/CLOSE SOUNDREQUESTER";
        t[TOOLTIP_SHELL_OCDISK]         = "OPEN/CLOSE IN-OUT-REQUESTER";
        t[TOOLTIP_SHELL_OCLOCALE]       = "OPEN/CLOSE LANGUAGEREQUESTER";
        t[TOOLTIP_SHELL_PL_PLAY]        = "PAUSE/CONTINUE MISSION BRIEFING";
        t[TOOLTIP_SHELL_PL_STOP]        = "STOP MISSION BRIEFING";
        t[TOOLTIP_SHELL_PL_LOAD]        = "LOAD LAST SAVEGAME";
        t[TOOLTIP_SHELL_PL_FASTFORWARD] = "WIND FORWARD MISSION BRIEFING";
        t[TOOLTIP_SHELL_PL_SETBACK]     = "RESET MISSION BRIEFING";
        t[TOOLTIP_SHELL_PL_GAME]        = "PLAY THE LEVEL";
        t[TOOLTIP_SHELL_QUITGAME]       = "QUIT THAT ORGY OF DESTRUCTION";
        t[TOOLTIP_SHELL_QUALIFIER]      = "PRESS FOR ADDITIONAL SELECTION";
        t[TOOLTIP_SHELL_ENTERINPUT]     = "ENTER NEW KEY IF BUTTON IS PRESSED";
        t[TOOLTIP_SHELL_PLAYLEFT]       = "YOU HEAR THE SOUND AT LEFT";
        t[TOOLTIP_SHELL_PLAYRIGHT]      = "YOU HEAR THE SOUND AT RIGHT";
        t[TOOLTIP_SHELL_OCNET]          = "OPEN/CLOSE NETWORKREQUESTER";
        t[TOOLTIP_SHELL_HELP]           = "GIVE ME MORE INFORMATION";
        t[TOOLTIP_SHELL_PL_TOTITLE]     = "GO BACK TO START PAGE";
        t[TOOLTIP_SHELL_PL_TOPLAY]      = "GO BACK TO LEVELSELECTION";
        t[TOOLTIP_SHELL_NETCANCEL]      = "CLOSE NETWORK REQUESTER";
        t[TOOLTIP_SHELL_NETHELP]        = "???";
        t[TOOLTIP_SHELL_OKPROVIDER]     = "APPLY THIS PROVIDER";
        t[TOOLTIP_SHELL_OKPLAYER]       = "APPLY THIS PLAYERNAME";
        t[TOOLTIP_SHELL_OKLEVEL]        = "CREATE GAME WITH THIS LEVEL";
        t[TOOLTIP_SHELL_OKSESSION]      = "JOIN THIS SESSION";
        t[TOOLTIP_SHELL_STARTLEVEL]     = "START GAME";
        t[TOOLTIP_SHELL_BACKTOPROVIDER] = "GO BACK TO PROVIDER SELECTION";
        t[TOOLTIP_SHELL_BACKTOPLAYER]   = "GO BACK TO ENTER A NEW NAME";
        t[TOOLTIP_SHELL_BACKTOSESSION]  = "GO BACK TO SESSION SELECTION";
        t[TOOLTIP_SHELL_NEWLEVEL]       = "CHOOSE A NEW LEVEL";
        t[TOOLTIP_SHELL_SEND]           = "SEND MESSAGE MTO ALL PLAYERS";
        t[TOOLTIP_SHELL_USERRACE]       = "SELECT RESISTANCE AS RACE";
        t[TOOLTIP_SHELL_KYTERNESER]     = "SELECT GHORKOV AS RACE";
        t[TOOLTIP_SHELL_MYKONIER]       = "SELECT MYKONIANS AS RACE";
        t[TOOLTIP_SHELL_TAERKASTEN]     = "SELECT TAERKASTEN AS RACE";
        t[TOOLTIP_SHELL_READY]          = "MARK GAME AS STARTABLE";
        t[TOOLTIP_SHELL_NOTREADY]       = "MARK GAME AS NOT STARTBALE";
        t[TOOLTIP_SHELL_LOAD]           = "LOAD SELECTED PLAYER";
        t[TOOLTIP_SHELL_SAVE]           = "SAVE ACTUAL PLAYER";
        t[TOOLTIP_SHELL_DELETE]         = "DELETE SELECTED PLAYER";
        t[TOOLTIP_SHELL_NEWGAME]        = "CREATE A NEW PLAYER";
        t[TOOLTIP_SHELL_OKLOAD]         = "LOAD PLAYER WITH THIS NAME";
        t[TOOLTIP_SHELL_OKSAVE]         = "SAVE ACTUAL PLAYER UNDER THIS NAME";
        t[TOOLTIP_SHELL_OKDELETE]       = "DELETE PLAYER WITH THIS NAME";
        t[TOOLTIP_SHELL_OKCREATE]       = "CREATE PLAYER WITH THIS NAME";
        t[TOOLTIP_SHELL_DISKCANCEL]     = "CLOSE PLAYER REQUESTER";
        t[TOOLTIP_SHELL_CANCELLOAD]     = "STOP LOADING PLAYER";
        t[TOOLTIP_SHELL_CANCELSAVE]     = "STOP SAVING PLAYER";
        t[TOOLTIP_SHELL_CANCELDELETE]   = "STOP DELETING PLAYER";
        t[TOOLTIP_SHELL_CANCELCREATE]   = "STOP CREATING PLAYER";
        t[TOOLTIP_SHELL_SETTINGSCANCEL] = "CLOSE SETTINGS REQUESTER";
        t[TOOLTIP_SHELL_SETTINGSOK]     = "APPLY NEW SETTINGS";
        t[TOOLTIP_SHELL_RESOLUTION]     = "PRESS TO CHANGE SCREEN RESOLUTION";
        t[TOOLTIP_SHELL_SOUNDSWITCH]    = "SWITCH FX-SOUND";
        t[TOOLTIP_SHELL_SOUNDLR]        = "TOGGLE CHANNEL ASSIGNMENT";
        t[TOOLTIP_SHELL_FARVIEW]        = "CHANGE HORIZON DEPTH";
        t[TOOLTIP_SHELL_HEAVEN]         = "SWITCH ON/OFF SKY";
        t[TOOLTIP_SHELL_FILTERING]      = "SWITCH ON/OFF FILTERING";
        t[TOOLTIP_SHELL_CDSOUND]        = "ENABLE/DISABLE CD SOUND";
        t[TOOLTIP_SHELL_SOFTMOUSE]      = "USE SOFTWARE OR HARDWARE MOUSEPOINTER";
        t[TOOLTIP_SHELL_ENEMYINDICATOR] = "ALLOW/FORBID ENEMYINDICATORS";
        t[TOOLTIP_SHELL_FXVOLUME]       = "CHANGE FX VOLUME";
        t[TOOLTIP_SHELL_CDVOLUME]       = "CHANGE CD VOLUME";
        t[TOOLTIP_SHELL_FXSLIDER]       = "CHANGE NUMBER OF EXPLODE EFFECTS";
        t[TOOLTIP_SHELL_INPUTOK]        = "APPLY INPUT CHANGES";
        t[TOOLTIP_SHELL_INPUTCANCEL]    = "IGNORE INPUT CHANGES";
        t[TOOLTIP_SHELL_INPUTRESET]     = "RESET TO DEFAULT KEYS";
        t[TOOLTIP_SHELL_JOYSTICK]       = "USE JOYSTICK";
        t[TOOLTIP_SHELL_FORCEFEEDBACK]  = "USE FORCEFEEDBACK FOR JOSTICK";
        t[TOOLTIP_SHELL_SWITCHOFF]      = "REMOVE KEY FROM ACTION";
        t[TOOLTIP_SHELL_LOCALEOK]       = "USE SELECTED LANGUAGE";
        t[TOOLTIP_SHELL_LOCALECANCEL]   = "CLOSE LOCALE REQUESTER";
        t[TOOLTIP_SHELL_PLAY]           = "GO TO LEVEL SELECTION MAP";
        t[TOOLTIP_SHELL_TUTORIAL]       = "GO TO TRAINING AREA";

        t[TOOLTIP_SHELL_DB_PAUSE]           = "PAUSE DEBRIEFING";
        t[TOOLTIP_SHELL_DB_STOP]            = "EXIT DEBRIEFING";
        t[TOOLTIP_SHELL_DB_REWIND]          = "REWIND DEBRIEFING";
        t[TOOLTIP_SHELL_DB_TIME_LOC]        = "PLAYING TIME - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_TIME_GLOB]       = "PLAYING TIME - OVERALL";
        t[TOOLTIP_SHELL_DB_KILLS_LOC]       = "KILLS - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_KILLS_GLOB]      = "KILLS - OVERALL";
        t[TOOLTIP_SHELL_DB_LOSS_LOC]        = "LOSSES - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_LOSS_GLOB]       = "LOSSES - OVERALL";
        t[TOOLTIP_SHELL_DB_SECS_LOC]        = "SECTORS CONQUERED - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_SECS_GLOB]       = "SECTORS CONQUERED - OVERALL";
        t[TOOLTIP_SHELL_DB_SCORE_LOC]       = "SCORE - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_SCORE_GLOB]      = "SCORE - OVERALL";
        t[TOOLTIP_SHELL_DB_POWER_LOC]       = "POWERSTATIONS CAPTURED - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_POWER_GLOB]      = "POWERSTATIONS CAPTURED - OVERALL";
        t[TOOLTIP_SHELL_DB_UPGRADE_LOC]     = "TECH UPGRADES CAPTURED - THIS LEVEL";
        t[TOOLTIP_SHELL_DB_UPGRADE_GLOB]    = "TECH UPGRADES CAPTURED - OVERALL";

        t[TOOLTIP_SHELL_DB_RACE]        = "SHOW STATISTICS FOR THIS RACE";
        t[TOOLTIP_SHELL_EXITDEBRIEFING] = "EXIT DEBRIEFING";

        return(TRUE);
    };

    /*** Fehler-Ende ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_KillTooltips(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Deinitialisiert Tooltip-System. Muß aus OM_DISPOSE
**      aufgerufen werden.
**
**  CHANGED
**      17-Sep-96   floh    created
*/
{
    if (ywd->DefTooltips) {
        _FreeVec(ywd->DefTooltips);
        ywd->DefTooltips = NULL;
    };
}

/*-----------------------------------------------------------------*/
void yw_Tooltip(struct ypaworld_data *ywd, ULONG tip_id)
/*
**  FUNCTION
**      Setzt den angegeben Tooltip als aktuellen für diesen
**      Frame. Falls bereits ein höherpriorisierter
**      (== höhere ID-Nummer) eingetragen ist, wird
**      der Tooltip allerdings ignoriert.
**
**  CHANGED
**      17-Sep-96   floh    created
**      03-Jan-96   floh    löscht jetzt automatisch das Feld
**                          ywd->TooltipHotkey
*/
{
    if (tip_id > ywd->Tooltip) {
        ywd->Tooltip       = tip_id;
        ywd->TooltipHotkey = 0;
    };
}

/*-----------------------------------------------------------------*/
void yw_TooltipHotkey(struct ypaworld_data *ywd, ULONG tip_id, ULONG hotkey)
/*
**  FUNCTION
**      Tooltip mit zusätzlicher automatischer Auflösung
**      des zugehörigen Hotkeys. Der Hotkey wird
**      dann innerhalb yw_RenderTooltip() ermittelt
**      und mit angezeigt.
**
**  INPUTS
**      ywd     - LID des Weltobjects
**      tip_id  - gewünschter Tooltip
**      hotkey  - ID des gewünschten Hotkeys (> 0x80)
**
**  CHANGED
**      03-Jan-97   floh    created
*/
{
    if (tip_id > ywd->Tooltip) {
        ywd->Tooltip       = tip_id;
        ywd->TooltipHotkey = hotkey;
    };
}

/*-----------------------------------------------------------------*/
UBYTE *yw_Keycode2Str(struct ypaworld_data *ywd, ULONG k)
/*
**  FUNCTION
**      Wandelt Keycode in String um (garantiert Großbuchstaben,
**      und in eckigen Klammern eingeschlossen).
**
**  CHANGED
**      03-Jan-97   floh    created
**      26-Oct-97   floh    + benutzt jetzt AF's GlobalKeyTable
*/
{
    static UBYTE str_buf[64];
    str_buf[0] = 0;
    if (ywd->gsr) {
        if (GlobalKeyTable[k].name) {
            sprintf(str_buf,"[%s]",GlobalKeyTable[k].name);
            return(str_buf);
        };
    };
    return(NULL);
}

/*-----------------------------------------------------------------*/
void yw_RenderTooltip(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Rendered den in diesem Frame eingetragenen
**      Tooltip und setzt den aktuell eingestellten
**      Tooltip wieder zurück(!).
**
**  CHANGED
**      17-Sep-96   floh    created
**      03-Jan-97   floh    + Support für optionale Tooltip-Hotkey.
**      09-Jun-97   floh    + Tooltips etwas niedriger positioniert
**      11-Oct-97   floh    + Positionierung jetzt per ywd->LowerTabu
**      24-Nov-97   floh    + Tooltips DBCS enabled
**      10-Dec-97   floh    + DBCS-Textfarbe
*/
{
    if ((ywd->Tooltip != TOOLTIP_NONE) && (!ywd->MouseBlanked)) {

        /*** wir benutzen zum Rendern den QuickLogBuf ***/
        UBYTE *str = LW_QuickLogBuf;
        UBYTE *tip = ypa_GetStr(ywd->LocHandle,(STR_TIP_NONE+ywd->Tooltip),ywd->DefTooltips[ywd->Tooltip]);
        struct rast_text rt;
        WORD ypos = -(ywd->LowerTabu+ywd->FontH+(ywd->FontH>>2));
        UBYTE *hk_str = NULL;

        /*** Hotkey angegeben? ***/
        if (ywd->TooltipHotkey) {

            Object *io;
            struct inp_delegate_msg idm;
            struct idev_queryhotkey_msg iqm;

            _IE_GetAttrs(IET_Object,&io,TAG_DONE);

            iqm.keycode = 0;    // gesucht
            iqm.hotkey  = ywd->TooltipHotkey;
            idm.type   = ITYPE_KEYBOARD;
            idm.num    = 0;
            idm.method = IDEVM_QUERYHOTKEY;
            idm.msg    = &iqm;
            _methoda(io,IM_DELEGATE,&idm);

            if (iqm.keycode != 0) {
                /*** <keycode> in String translatieren ***/
                hk_str = yw_Keycode2Str(ywd,iqm.keycode);
                if (hk_str) {
                    ypos = -(ywd->LowerTabu+2*ywd->FontH+(ywd->FontH>>2));                
                };
            };
        };

        new_font(str,FONTID_TRACY);
        pos_brel(str,0,ypos);
        if (hk_str) {
            dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_LIST),yw_Green(ywd,YPACOLOR_TEXT_LIST),yw_Blue(ywd,YPACOLOR_TEXT_LIST));        
            str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_TRACY],str,hk_str,ywd->DspXRes);
            new_line(str);
        };
        dbcs_color(str,yw_Red(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Green(ywd,YPACOLOR_TEXT_TOOLTIP),yw_Blue(ywd,YPACOLOR_TEXT_TOOLTIP));
        str = yw_TextCenteredSkippedItem(ywd->Fonts[FONTID_TRACY],str,tip,ywd->DspXRes);
        eos(str);
        rt.string = LW_QuickLogBuf;
        rt.clips  = NULL;
        _methoda(ywd->GfxObject,RASTM_Text,&rt);
    };

    /*** ... und Tooltip löschen ***/
    ywd->Tooltip = TOOLTIP_NONE;
    ywd->TooltipHotkey = 0;
}

