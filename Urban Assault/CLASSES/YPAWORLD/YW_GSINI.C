/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen fnr die GameShell
**
**  © Copyright 1996 by A.Flemming
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "nucleus/nucleus2.h"
#include "nucleus/syntax.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "ypa/ypagameshell.h"
#include "bitmap/displayclass.h"
#include "audio/cdplay.h"
#include "input/winpclass.h"
#include "bitmap/winddclass.h"

#include "yw_protos.h"
#include "yw_gsprotos.h"

/*-----------------------------------------------------------------*/
// globale Vereinbarungen
_extern_use_nucleus
_extern_use_audio_engine
_extern_use_input_engine
_extern_use_ov_engine

struct key_info GlobalKeyTable[ 256 ];
UBYTE  **GlobalLocaleHandle;

#define YW_NUM_CONFIG_ITEMS (4)
struct ConfigItem yw_ConfigItems[YW_NUM_CONFIG_ITEMS] = {
    {"netgame.exclusivegem", CONFIG_BOOL, TRUE},
    {"net.waitstart", CONFIG_INTEGER, 150000},
    {"net.kickoff", CONFIG_INTEGER, 20000},
    {"game.debug", CONFIG_BOOL, FALSE}
};
/*** Designdaten sind global ***/
WORD ReqDeltaX;
WORD ReqDeltaY;
WORD IReqWidth;
WORD VReqWidth;
WORD DReqWidth;
WORD LReqWidth;
WORD AReqWidth;
WORD NReqWidth;
WORD NListWidth;
WORD IListWidth;
WORD DListWidth;

#define GET_X_COORD(x) (FLOAT_TO_INT((((FLOAT)x)/640.0)*((FLOAT)ywd->DspXRes)))
#define GET_Y_COORD(y) (FLOAT_TO_INT((((FLOAT)y)/480.0)*((FLOAT)ywd->DspYRes)))

/*** Prototypen, nix als Prototypen... ***/
#include "yw_protos.h"
#include "yw_gsprotos.h"


/*------------------------------------------------------------------
** Die GameShell ben÷tigt 2 Methoden, eine Initialisierungs-Methode,
** die das Interface bastelt, und eine Trigger-Methode, die das
** Zeichnen und die Auswertung der Eingaben nbernimmt. Wir verwenden
** hierbei Routinen der yw_listview.c
** ---------------------------------------------------------------*/
char   BalkenString[2000];

_dispatcher( BOOL, yw_YWM_INITGAMESHELL, struct GameShellReq *GSR )
{
/*
**  FUNCTION    Initialisiert die GameShell. Wird nur einmal am Anfang
**              des Spieles gemacht
**
**  RESULT      TRUE, wenn alles ok war
**
**  CHANGD      af 14-Apr-96 created
**              localized und default-init der inputengine
*/

    struct ypaworld_data *ywd;
    struct input_info *ii;
    int    i;
    #ifdef __NETWORK__
    struct checkremotestart_msg crs;
    #endif
    
    ywd = INST_DATA( cl, o );

    /*** Allgemeiner Austausch ***/
    GSR->ywd = ywd;
    ywd->gsr = GSR;

    // FLOH CHANGE
    ywd->Level->Status = LEVELSTAT_SHELL;

    /*** Shellmode auf Titel, Open muss evtl. unterscheiden ***/
    GSR->shell_mode = SHELLMODE_TITLE;
    
    /*** Wie sollen Wundersteine im Netz behandelt werden? ***/
    _GetConfigItems(NULL,yw_ConfigItems,YW_NUM_CONFIG_ITEMS);
    ywd->exclusivegem = (BOOL)yw_ConfigItems[0].data;

/// "Diverse Sachen"

    /* -------------------------------------------
    ** Diverses, was mit wenig Worten erledigt ist
    ** -----------------------------------------*/
    _get( o, YWA_LocaleHandle, &GlobalLocaleHandle );

    _NewList( (struct List *) &( GSR->flist ) );
    _NewList( (struct List *) &( GSR->videolist ) );
    _NewList( (struct List *) &( GSR->localelist ) );
    yw_InitKeyTable( ywd );
    yw_InitVideoList( GSR );

    yw_ScanUserDirectory( GSR, "save:", o );

    yw_ScanLocaleDirectory( GSR, "locale", o );
    
    strcpy(GSR->D_Name, UNKNOWN_NAME );
    GSR->DCursorPos = strlen( GSR->D_Name );

    GSR->FirstScoreLoad = TRUE;
    GSR->d_actualitem   = 0;
    GSR->i_actualitem   = 1;  // mal sehen, dann eben wieder 1...

    /*** Soundzeug ***/
    GSR->ShellSound1.vec.x = GSR->ShellSound2.vec.x =  0.0;
    GSR->ShellSound1.vec.y = GSR->ShellSound2.vec.y =  0.0;
    GSR->ShellSound1.vec.z = GSR->ShellSound2.vec.z =  0.0;
    GSR->ShellSound1.pos.x = GSR->ShellSound2.pos.x =  0.0;
    GSR->ShellSound1.pos.y = GSR->ShellSound2.pos.y =  0.0;
    GSR->ShellSound1.pos.z = GSR->ShellSound2.pos.z =  0.0;
    for( i = 0; i < MAX_SOUNDSOURCES; i++ ) {

        GSR->ShellSound1.src[ i ].volume = GSR->ShellSound2.src[ i ].volume = 127;
        GSR->ShellSound1.src[ i ].pitch  = GSR->ShellSound2.src[ i ].pitch  = 0;
        }

    _InitSoundCarrier( &(GSR->ShellSound1) );
    _InitSoundCarrier( &(GSR->ShellSound2) );
    _InitSoundCarrier( &(GSR->ChatSound) );

    if( !yw_ParseShellScript( GSR )) {

        _LogMsg("Error: Unable to load from Shell.ini\n");
        return( FALSE );
        }

    GSR->v_actualitem = 0;
///

/// "Initialisieren des Input-Arrays"
    /*** Nun fnllen wir das Input-Info-feld aus (Namen erst bei Opneshell!) ***/
    ii = &( GSR->inp[ I_DRV_DIR ]);
    ii->kind     = GSI_SLIDER;
    ii->number   = SL_DRV_DIR;
    ii->pos      = KEYCODE_CURSOR_RIGHT;
    ii->neg      = KEYCODE_CURSOR_LEFT;

    ii = &( GSR->inp[ I_DRV_SPEED ]);
    ii->kind     = GSI_SLIDER;
    ii->number   = SL_DRV_SPEED;
    ii->pos      = KEYCODE_CURSOR_UP;
    ii->neg      = KEYCODE_CURSOR_DOWN;

    ii = &( GSR->inp[ I_FLY_DIR ]);
    ii->kind     = GSI_SLIDER;
    ii->number   = SL_FLY_DIR;
    ii->pos      = KEYCODE_CURSOR_RIGHT;
    ii->neg      = KEYCODE_CURSOR_LEFT;

    ii = &( GSR->inp[ I_FLY_HEIGHT ]);
    ii->kind     = GSI_SLIDER;
    ii->number   = SL_FLY_HEIGHT;
    ii->pos      = KEYCODE_CURSOR_UP;
    ii->neg      = KEYCODE_CURSOR_DOWN;

    ii = &( GSR->inp[ I_FLY_SPEED ]);
    ii->kind     = GSI_SLIDER;
    ii->number   = SL_FLY_SPEED;
    ii->pos      = KEYCODE_CTRL;
    ii->neg      = KEYCODE_LSHIFT;

    ii = &( GSR->inp[ I_GUN_HEIGHT ]);
    ii->kind     = GSI_SLIDER;
    ii->number   = SL_GUN_HEIGHT;
    ii->pos      = KEYCODE_A;
    ii->neg      = KEYCODE_Y;

    ii = &( GSR->inp[ I_FIRE ]);
    ii->kind     = GSI_BUTTON;
    ii->number   = 0;               // Achtung! BT_... sind Masken!
    ii->pos      = KEYCODE_SPACEBAR;

    ii = &( GSR->inp[ I_FIREVIEW ]);
    ii->kind     = GSI_BUTTON;
    ii->number   = 1;
    ii->pos      = KEYCODE_TAB;

    ii = &( GSR->inp[ I_FIREGUN ]);
    ii->kind     = GSI_BUTTON;
    ii->number   = 2;
    ii->pos      = KEYCODE_RETURN;

    ii = &( GSR->inp[ I_STOP ]);
    ii->kind     = GSI_BUTTON;
    ii->number   = 3;
    ii->pos      = KEYCODE_NUM_0;

    ii = &( GSR->inp[ I_HUD ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_HUD - 128;
    ii->pos      = KEYCODE_V;

    ii = &( GSR->inp[ I_NEW ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_NEW - 128;
    ii->pos      = KEYCODE_N;

    ii = &( GSR->inp[ I_ADD ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_ADD - 128;
    ii->pos      = KEYCODE_A;

    ii = &( GSR->inp[ I_ORDER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_ORDER - 128;
    ii->pos      = KEYCODE_O;

    ii = &( GSR->inp[ I_FIGHT ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_FIGHT - 128;
    ii->pos      = KEYCODE_SPACEBAR;

    ii = &( GSR->inp[ I_CONTROL ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_CONTROL - 128;
    ii->pos      = KEYCODE_C;

    ii = &( GSR->inp[ I_AUTOPILOT ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_AUTOPILOT - 128;
    ii->pos      = KEYCODE_G;

    ii = &( GSR->inp[ I_MAP ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP - 128;
    ii->pos      = KEYCODE_M;

    ii = &( GSR->inp[ I_FINDER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_FINDER - 128;
    ii->pos      = KEYCODE_F;

    ii = &( GSR->inp[ I_LANDSCAPE ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_LAND - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_OWNER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_OWNER - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_HEIGHT ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_HEIGHT - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_LOCKVIEWER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_LOCKVWR - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_ZOOMIN ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_ZOOMIN - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_ZOOMOUT ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_ZOOMOUT - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_MAPMINI ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAP_MINIMIZE - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_NEXTCOM ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_NEXTCMDR - 128;
    ii->pos      = KEYCODE_F1;

    ii = &( GSR->inp[ I_TOROBO ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_TOROBO - 128;
    ii->pos      = KEYCODE_F2;

    ii = &( GSR->inp[ I_NEXTMAN ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_NEXTUNIT - 128;
    ii->pos      = KEYCODE_F3;

    ii = &( GSR->inp[ I_TOCOMMANDER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_TOCMDR - 128;
    ii->pos      = KEYCODE_F4;

    ii = &( GSR->inp[ I_QUIT ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_QUIT - 128;
    ii->pos      = KEYCODE_ESCAPE;

    ii = &( GSR->inp[ I_LOGWIN ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_LOGWIN - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_NEXTITEM ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_NEXTITEM - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_PREVITEM ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_PREVITEM - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_LASTMSG ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_CTRL2LM - 128;
    ii->pos      = KEYCODE_BS;

    ii = &( GSR->inp[ I_PAUSE ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_PAUSE - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_TOALL ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_TOALL - 128;
    ii->pos      = KEYCODE_NUM_5;

    ii = &( GSR->inp[ I_AGGR1 ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_AGGR1 - 128;
    ii->pos      = KEYCODE_1;

    ii = &( GSR->inp[ I_AGGR2 ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_AGGR2 - 128;
    ii->pos      = KEYCODE_2;

    ii = &( GSR->inp[ I_AGGR3 ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_AGGR3 - 128;
    ii->pos      = KEYCODE_3;

    ii = &( GSR->inp[ I_AGGR4 ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_AGGR4 - 128;
    ii->pos      = KEYCODE_4;

    ii = &( GSR->inp[ I_AGGR5 ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_AGGR5 - 128;
    ii->pos      = KEYCODE_5;

    ii = &( GSR->inp[ I_WAYPOINT ]);
    ii->kind     = GSI_BUTTON;
    ii->number   = 4;
    ii->pos      = KEYCODE_LSHIFT;

    ii = &( GSR->inp[ I_HELP ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_HELP - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_LASTOCCUPIED ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_LASTOCCUPIED - 128;
    ii->pos      = 0;

    ii = &( GSR->inp[ I_MAKECOMMANDER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_MAKECOMMANDER - 128;
    ii->pos      = 0;
    
    ii = &( GSR->inp[ I_ANALYZER ]);
    ii->kind     = GSI_HOTKEY;
    ii->number   = HOTKEY_ANALYZER - 128;
    ii->pos      = 0;    
///

/// "Tasten mit Defaultwerten setzen"
    /*** Nun alle Tasten setzen ***/
    yw_SetAllKeys( o, GSR );
    yw_NoticeKeys( GSR );
    yw_NoticeStandardKeys( GSR );
///

    GSR->d_mask = DM_SCORE|DM_INPUT|DM_VIDEO|DM_SOUND|DM_SHELL|DM_BUILD|DM_BUDDY;

/// "Netzspezifisches..."
    #ifdef __NETWORK__
    /* --------------------------------------------------------------
    ** Testen, ob Fernsteuerung. Wenn dem so war, dann öffnen wir den
    ** Netzrequester und stllen in auf NM_MESSAGE.
    ** ------------------------------------------------------------*/
    if( _methoda( ywd->nwo, NWM_CHECKREMOTESTART, &crs ) ) {

        /*** Fernsteuerung ? ***/
        if( crs.remote ) {

            struct getplayerdata_msg gpd;
            
            /* -----------------------------------------------------------
            ** Wichtig, darf beim Laden von Usereinstellungen nicht ueber-
            ** schrieben werden!
            ** ---------------------------------------------------------*/    
            yw_LocStrCpy( ywd->gsr->NPlayerName, crs.name );

            if( crs.is_host )
                ywd->gsr->is_host = 1;
            else
                ywd->gsr->is_host = 0;
            ywd->gsr->remotestart = 1;
 
            GSR->NLevelOffset = 0;
            GSR->shell_mode   = SHELLMODE_NETWORK;
                      
            /* ----------------------------------
            ** Wenn host, dann ready_to_start und
            ** in den LevelMode
            ** --------------------------------*/
            if( GSR->is_host ) {
            
                gpd.number  = 0;
                gpd.askmode = GPD_ASKNUMBER;
                while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {
                 
                    /*** Nicht ganz wasserdicht...***/
                    if( stricmp( gpd.name, GSR->NPlayerName)==0)
                        break;
                    gpd.number++;
                    }
                    
                GSR->player2[ gpd.number ].ready_to_start = 1;
                
                GSR->n_selmode = NM_LEVEL;
                }                         
            else {
            
                /*** Client, dann in den MessageMode ***/
                GSR->n_selmode = NM_MESSAGE;
                }

            /*** Namen aller Spieler merken... ***/   
            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {

                strncpy( ywd->gsr->player2[ gpd.number ].name,
                             gpd.name, STANDARD_NAMELEN );
                gpd.number++;
                }

            /*** Lobby ist langsam ***/
            GSR->flush_time_normal  = FLUSH_TIME_REDUCED;
            GSR->update_time_normal = UPDATE_TIME_REDUCED;
            }
        else
            ywd->gsr->remotestart = 0;
        }
    else {

        /*** Da ging was schief ***/
        _LogMsg("Error while remote start check\n");
        return( FALSE );
        }
    #endif
///

    GSR->wait_til_demo = WAIT_TIL_DEMO;

    /*** Das IntroMovie spielen, aber nur, wenn es kein LobbyStart war ***/
    if( 0 == GSR->remotestart )
        yw_PlayIntroMovie(ywd);


    return( TRUE );
}


BOOL yw_LoadKeyTable( struct GameShellReq *GSR )
{
    /* --------------------------------------------------------------
    ** Versucht ein File namens"..." zu Parsen. Nur das. Um selbige
    ** Einstellungen zum Beispiel zu Standardeinstellungen zu machen,
    ** muß noch NoticeStandardKeys aufgerufen werden
    ** ------------------------------------------------------------*/

    char filename[ 300 ];
    struct ScriptParser parser;

    /* -------------------------------------------------------------------
    ** Load wird von Restore aufgerufen, welches per Hand aufgerufen wird.
    ** Somit dürfte immer eine Sprache eingestellt sein.
    ** -----------------------------------------------------------------*/
    sprintf( filename, "levels:settings/%s/input.def", GSR->ywd->LocaleLang );
    parser.parse_func = yw_ParseInputData;
    parser.store[0]   = (ULONG) GSR->ywd->world;
    parser.target     = GSR;

    return( yw_ParseScript( filename, 1, &parser, PARSEMODE_SLOPPY ) );
}



void yw_NoticeKeys( struct GameShellReq *GSR )
{
    /*** merkt alle tasten in den backup-Buffern ***/
    ULONG i;

    for( i = 1; i <= NUM_INPUTEVENTS; i++ ) {

        GSR->inp[ i ].bu_pos = GSR->inp[ i ].pos;
        GSR->inp[ i ].bu_neg = GSR->inp[ i ].neg;
        }
}


void yw_NoticeStandardKeys( struct GameShellReq *GSR )
{
    /*** merkt alle tasten in den backup-Buffern ***/
    ULONG i;

    for( i = 1; i <= NUM_INPUTEVENTS; i++ ) {

        GSR->inp[ i ].st_pos = GSR->inp[ i ].pos;
        GSR->inp[ i ].st_neg = GSR->inp[ i ].neg;
        }
}


void yw_RestoreKeys( struct GameShellReq *GSR )
{
    /*** Setzt alle tasten zurück und aktualisiert dabei ..._text ***/
    ULONG i;

    for( i = 1; i <= NUM_INPUTEVENTS; i++ ) {

        GSR->inp[ i ].pos     = GSR->inp[ i ].bu_pos;
        GSR->inp[ i ].neg     = GSR->inp[ i ].bu_neg;
        }
}


void yw_RestoreStandardKeys( struct GameShellReq *GSR )
{
    /* -----------------------------------------------------------------
    ** Setzt alle tasten auf die Standard-belegung. Wir versuchen, ein
    ** Locale-spezifisches Standard-File zu lesen. ist das nicht da oder
    ** läßt es sich nicht laden, nehmen wir eben die Einstellungen, die
    ** von InitGameShell noch in st... stehen
    ** ---------------------------------------------------------------*/
    ULONG i;

    if( yw_LoadKeyTable( GSR ) ) {

        /*** Dann wenigstens als Standard merken ***/
        yw_NoticeStandardKeys( GSR );
        }

    /*** Egal ob Laden geklappt hat: Tasten vom st Feld ins Originale...***/
    for( i = 1; i <= NUM_INPUTEVENTS; i++ ) {

        GSR->inp[ i ].pos     = GSR->inp[ i ].st_pos;
        GSR->inp[ i ].neg     = GSR->inp[ i ].st_neg;
        }
}


_dispatcher( BOOL, yw_YWM_OPENGAMESHELL, struct GameShellReq *GSR )
{
/*
**  FUNCTION    _ffnet die GameShell, wird vor Arbeit mit selbiger
**              aufgerufen. Ich schalte dabei nur den unteren Balken
**              sichtbar.
**
**  INPUT       die GSR-Struktur...
**
**  CHANGED     15-Apr-96   af created
*/
    struct switchpublish_msg sp;
    struct newbutton_msg nb;
    struct setbuttonstate_msg sbs;
    struct ypaworld_data *ywd = INST_DATA( cl, o);
    char   sys_string[ 300 ];
    BOOL   button_ok;
    WORD   num_dev, sel_dev, restbreite, i, y_dist;
    struct MinNode *n;
    struct bt_propinfo pi;
    struct input_info *ii;
    struct setlanguage_msg sl;
    struct disp_pointer_msg dpm;
    struct snd_cdcontrol_msg cd;
    struct switchbutton_msg sb;
    WORD   bwidth, bheight, bstart_x, bstart_y, ok_x, ok_y, ok_w;
    WORD   cancel_x, cancel_y, cancel_w, help_x, help_y, help_w;
    struct windd_device wdm;

    /*** spezielle lokale Variable fnr das aufl÷sungsabhSngige Design ***/
    WORD   gadgetwidth, checkmarkwidth;

    /*
    ** Die Fonts:
    ** Stringgadget normal FONTID_MAPCUR_4
    ** Pressed             FONTID_MAPCUR_32
    ** unpressed           FONTID_MAPCUR_16
    ** disabled            FONTID_ENERGY
    */

    /*** vorsichtshalber UserName "großbuchstabig" machen ***/
    yw_LocStrCpy( sys_string, GSR->UserName );
    strcpy( GSR->UserName, sys_string );

    /* ----------------------------------------------------------------
    ** Videomode setzen, wenn  erwuenscht. Weil SETVIDEOMODE (wie lange
    ** eigentlich noch?) sich auf die GSR-daten bezieht, die aber nun
    ** nur noch Spielspezifisch sind, muessen wir sie vorher merken und
    ** dann wieder ruecksetzen.
    ** --------------------------------------------------------------*/
    if( !GSR->ywd->OneDisplayRes ) {

        struct setgamevideo_msg sgv;

        sgv.modus         = ywd->ShellRes;
        sgv.forcesetvideo = FALSE;

        _methoda( GSR->ywd->world, YWM_SETGAMEVIDEO, &sgv );
        }

    /*** Set laden ***/
    if( !yw_LoadSet( ywd, 46 ) ) {

        _LogMsg("Unable to load set for shell\n");
        return( FALSE );
        }

    /*** Schwärzen, weil LOADSET die Palette setzt ***/
    yw_BlackOut( ywd );

    /*** diverse Initialisierungen ***/
    GSR->exist_savegame   = 0;
    GSR->i_selfirst       = TRUE;
    GSR->last_input_event = 0;
    GSR->ywd->UpperTabu   = 0;
    GSR->input_changemode = FALSE;
    GSR->ywd->Url         = NULL;
    GSR->blocked          = FALSE;

    /*** Globales Language-Handle setzen ***/
    if( GSR->lsel ) {

        sl.lang = GSR->lsel->language;
        if( !_methoda( o, YWM_SETLANGUAGE, &sl )) {

            _LogMsg("Warning: Catalogue not found\n");
            }
        }
    else _LogMsg("Warning: No Language selected, use default set\n");

    /*** Netzlevel sortieren (jetzt ist Sprache aktuell) ***/
    yw_SortNetworkLevels( GSR );

    /*** FIXME_FLOH: Level-Select-Zeugs initialisieren ***/
    if (!yw_InitShellBgHandling(ywd)) {
        _LogMsg("Could not init level select stuff!\n");
        return(FALSE);
    };

    /*** GafikObject holen ***/
    _OVE_GetAttrs( OVET_Object, &(ywd->GfxObject), TAG_DONE );
    if( NULL == ywd->GfxObject ) {

        /*** raus hier, da kein GfxObject da ist ***/
        _LogMsg("No GfxObject in OpengameShell!\n");
        return( FALSE );
        }

    /*** Mauspointer setzen ***/
    dpm.pointer = ywd->MousePtrBmp[ YW_MOUSE_POINTER ];
    dpm.type    = DISP_PTRTYPE_NORMAL;
    _methoda( ywd->GfxObject, DISPM_SetPointer, &dpm );
    yw_InitVideoList( GSR );

    /*** aktuellen D3D-Mode erfragen ***/
    wdm.name  = NULL;
    wdm.guid  = NULL;
    wdm.flags = 0;
    num_dev   = 0;
    do {

        _methoda( ywd->GfxObject, WINDDM_QueryDevice, &wdm );
        if( wdm.name ) {

            if( wdm.flags & WINDDF_IsCurrentDevice ) {

                strcpy( GSR->d3dguid, wdm.guid );
                strcpy( GSR->d3dname, wdm.name );
                sel_dev = num_dev;
                }
            num_dev++;
            }
        } while( wdm.name );

    /*** Filtering und Softmouse neu setzen ***/
    if( GSR->video_flags & VF_SOFTMOUSE )
        _set(GSR->ywd->GfxObject,WINDDA_CursorMode,WINDD_CURSORMODE_SOFT);
    else
        _set(GSR->ywd->GfxObject,WINDDA_CursorMode,WINDD_CURSORMODE_HW);


    /*** Tabelle neu ausfüllen wegen localizing, Tasten bleiben ***/
    yw_InitKeyTable( ywd );
       
/// "Ausfüllen der texte für Eingabeereignisse"
    /*** Sprachspezifische Vorbereitung des Inputmenns ***/
    ii = &( GSR->inp[ I_DRV_DIR ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_DRVDIR ,GS_DRVDIRTEXT);

    ii = &( GSR->inp[ I_DRV_SPEED ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_DRVSPEED, GS_DRVSPEEDTEXT);

    ii = &( GSR->inp[ I_FLY_DIR ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FLYDIR, GS_FLYDIRTEXT);

    ii = &( GSR->inp[ I_FLY_HEIGHT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FLYHEIGHT, GS_FLYHEIGHTTEXT);

    ii = &( GSR->inp[ I_FLY_SPEED ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FLYSPEED, GS_FLYSPEEDTEXT);

    ii = &( GSR->inp[ I_GUN_HEIGHT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_GUNHEIGHT, GS_GUNHEIGHTTEXT);

    ii = &( GSR->inp[ I_FIRE ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FIRE, GS_FIRETEXT);

    ii = &( GSR->inp[ I_FIREVIEW ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FIREVIEW, GS_FIREVIEWTEXT);

    ii = &( GSR->inp[ I_FIREGUN ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FIREGUN, GS_FIREGUNTEXT);

    ii = &( GSR->inp[ I_STOP ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_STOP, GS_STOPTEXT);

    ii = &( GSR->inp[ I_HUD ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_HUD, GS_HUDTEXT);

    ii = &( GSR->inp[ I_NEW ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_NEW, GS_NEWTEXT);

    ii = &( GSR->inp[ I_ADD ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_ADD, GS_ADDTEXT);

    ii = &( GSR->inp[ I_ORDER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_ORDER, GS_ORDERTEXT);

    ii = &( GSR->inp[ I_FIGHT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FIGHT, GS_FIGHTTEXT);

    ii = &( GSR->inp[ I_CONTROL ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_CONTROL, GS_CONTROLTEXT);

    ii = &( GSR->inp[ I_AUTOPILOT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_AUTOPILOT, GS_AUTOPILOTTEXT);

    ii = &( GSR->inp[ I_MAP ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_MAP, GS_MAPTEXT);

    ii = &( GSR->inp[ I_FINDER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_FINDER, GS_FINDERTEXT);

    ii = &( GSR->inp[ I_LANDSCAPE ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_MAPLANDSCAPE, GS_LANDSCAPETEXT);

    ii = &( GSR->inp[ I_OWNER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_MAPOWNER, GS_OWNERTEXT);

    ii = &( GSR->inp[ I_HEIGHT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_MAPHEIGHT, GS_HEIGHTTEXT);

    ii = &( GSR->inp[ I_LOCKVIEWER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_LOCKVIEWER, GS_LOCKVIEWERTEXT);

    ii = &( GSR->inp[ I_ZOOMIN ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_ZOOMIN, GS_ZOOMINTEXT);

    ii = &( GSR->inp[ I_ZOOMOUT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_ZOOMOUT, GS_ZOOMOUTTEXT);

    ii = &( GSR->inp[ I_MAPMINI ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_MAPMINI, GS_MAPMINITEXT);

    ii = &( GSR->inp[ I_NEXTCOM ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_NEXTCOM, GS_NEXTCOMTEXT);

    ii = &( GSR->inp[ I_TOROBO ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_TOROBO, GS_TOROBOTEXT);

    ii = &( GSR->inp[ I_NEXTMAN ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_NEXTMAN, GS_NEXTMANTEXT);

    ii = &( GSR->inp[ I_TOCOMMANDER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_TOCOMMANDER, GS_TOCOMMANDERTEXT);

    ii = &( GSR->inp[ I_QUIT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_QUIT, GS_QUITTEXT);

    ii = &( GSR->inp[ I_LOGWIN ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_LOGWIN, GS_LOGWINTEXT);

    ii = &( GSR->inp[ I_NEXTITEM ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_NEXTITEM, GS_NEXTITEMTEXT);

    ii = &( GSR->inp[ I_PREVITEM ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_PREVITEM, GS_PREVITEMTEXT);

    ii = &( GSR->inp[ I_LASTMSG ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_LASTMSG, GS_LASTMSGTEXT);

    ii = &( GSR->inp[ I_PAUSE ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_PAUSE, GS_PAUSETEXT);

    ii = &( GSR->inp[ I_TOALL ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_TOALL, GS_TOALLTEXT);

    ii = &( GSR->inp[ I_AGGR1 ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_AGGR1, GS_AGGR1TEXT);

    ii = &( GSR->inp[ I_AGGR2 ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_AGGR2, GS_AGGR2TEXT);

    ii = &( GSR->inp[ I_AGGR3 ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_AGGR3, GS_AGGR3TEXT);

    ii = &( GSR->inp[ I_AGGR4 ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_AGGR4, GS_AGGR4TEXT);

    ii = &( GSR->inp[ I_AGGR5 ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_AGGR5, GS_AGGR5TEXT);

    ii = &( GSR->inp[ I_WAYPOINT ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_WAYPOINT, GS_WAYPOINT);

    ii = &( GSR->inp[ I_HELP ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_HELP, GS_HELPTEXT);

    ii = &( GSR->inp[ I_LASTOCCUPIED ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_LASTOCCUPIED, GS_LASTOCCUPIEDTEXT);

    ii = &( GSR->inp[ I_MAKECOMMANDER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_MAKECOMMANDER, GS_MAKECOMMANDERTEXT);

    ii = &( GSR->inp[ I_ANALYZER ]);
    ii->menuname = ypa_GetStr( GlobalLocaleHandle, STR_IMENU_ANALYZER, GS_ANALYZERTEXT);
///

/// "Voreinstellungen fnr Ausdehnung und Startposition der Requester"
    /* -----------------------------------------------------------------------
    ** Nun geht es an die Ausdehnung. Die H÷he kann ich aus den Fonth÷hen
    ** u.s.w. ermitteln, aber die Breite wird ein Problem. Ich lege diese hier
    ** einfach mal fest. Sollte das alles noch fontsenstitiv werden, kann
    ** ich die Breite im Nachhinein noch ermitteln
    ** ---------------------------------------------------------------------*/

    if( ywd->DspXRes >= 512 ) {

        /*** Idee: Breite vielleicht direkt abhSngig von DspXRes ***/
        ReqDeltaY = 3;            ReqDeltaX = 3;              checkmarkwidth = 16;
        IReqWidth = 450;
        VReqWidth = 380;
        DReqWidth = 390;
        LReqWidth = 280;
        AReqWidth = 500;
        #ifdef __NETWORK__
        NReqWidth = 480;
        #endif
        }
    else {

        ReqDeltaY = 2;            ReqDeltaX = 2;              checkmarkwidth = 8;
        IReqWidth = 250;
        VReqWidth = 210;
        DReqWidth = 220;
        LReqWidth = 200;
        AReqWidth = 300;
        #ifdef __NETWORK__
        NReqWidth = 270;
        #endif
        }

    /*** typische Buttongroesse ***/
    bwidth   = (WORD)(0.7 * ywd->DspXRes);
    bheight  = (WORD)(0.8 * ywd->DspYRes);
    bstart_x = (ywd->DspXRes - bwidth)/2;
    bstart_y = ywd->FontH; // (ywd->DspYRes - bheight)/2;

    /* ----------------------------------------------------------------
    ** In der Hoehe zentrieren. Masstab ist 512, also was drueber geht,
    ** wird halbiert und aufaddiert
    ** --------------------------------------------------------------*/
    if( ywd->DspXRes >= 512 )
        bstart_y += ((ywd->DspYRes - 384) / 2);

    /*** Pos der Standardbutton innerhalb(!) des Buttons ***/
    ok_x     = 0;
    if( ywd->DspXRes >= 512 )
        ok_y     = bheight - ywd->FontH - ((ywd->DspYRes - 384) / 2);
    else
        ok_y     = bheight - ywd->FontH;
    ok_w     = (bwidth - 2 * ReqDeltaX) / 3;
    help_x   = 2 * ok_w + 2 * ReqDeltaX;
    help_y   = ok_y;
    help_w   = ok_w;
    cancel_x = ok_w + ReqDeltaX;
    cancel_y = ok_y;
    cancel_w = ok_w;


///

/// "Titelleiste"
    /* ---------------------------------------------------------------
    ** -------------------------------------------------------------*/

    GSR->Titel = _new("button.class",BTA_BoxX, 0,
                                     BTA_BoxY, 0,
                                     BTA_BoxW, ywd->DspXRes,
                                     BTA_BoxH, ywd->DspYRes,
                                     TAG_DONE );
    
    if( !GSR->Titel ) {
        
        _LogMsg("Unable to create Titel-Button-Object\n");
        return( FALSE );
        }

    button_ok = FALSE;
    y_dist    = (WORD)((0.8 * ywd->DspYRes - 10 * ywd->FontH)/10);
    y_dist   += ywd->FontH;

    /*** ist am Ende... ***/
    nb.pressed_font  = FONTID_MAPCUR_32;    // frueher ICON_NB
    nb.unpressed_font= FONTID_MAPCUR_16;
    nb.disabled_font = FONTID_ENERGY;
    nb.modus         = BM_GADGET;
    nb.x             = GET_X_COORD(213);
    nb.y             = GET_Y_COORD(110);
    nb.w             = ywd->DspXRes/3;
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_PLAY, "GAME");
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = GS_BUTTONDOWN;
    nb.user_released = GS_GAME_OPEN;
    nb.user_hold     = 0;
    nb.ID            = GSID_GAME;
    nb.flags         = BT_BORDER|BT_CENTRE|BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
    
    if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

     nb.y             = GET_Y_COORD(148);
     nb.shortcut      = 0;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_NETWORK, "NETWORK");
     nb.pressed_text  = NULL;
     nb.user_pressed  = GS_BUTTONDOWN;
     nb.user_released = GS_NET_OPEN;
     nb.user_hold     = 0;
     nb.ID            = GSID_NET;
    
     if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

      nb.x             = GET_X_COORD(213);     // FIXME FLOH
      nb.y             = GET_Y_COORD(208);
      nb.w             = ywd->DspXRes/3;       // FIXME FLOH
      nb.shortcut      = 0;
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_INPUT,"INPUT");
      nb.pressed_text  = NULL;
      nb.user_pressed  = GS_BUTTONDOWN;
      nb.user_released = GS_INPUT_OPEN;
      nb.user_hold     = 0;
      nb.ID            = GSID_INPUT;
       
      if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

       nb.y             = GET_Y_COORD(246);
       nb.shortcut      = 0;
       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_SETTINGS, "SETTINGS");
       nb.pressed_text  = NULL;
       nb.user_pressed  = GS_BUTTONDOWN;
       nb.user_released = GS_VIDEO_OPEN;
       nb.user_hold     = 0;
       nb.ID            = GSID_VIDEO;
        
       if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

        nb.y             = GET_Y_COORD(284);
        nb.shortcut      = 0;
        nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_DISK, "PLAYER");
        nb.pressed_text  = NULL;
        nb.user_pressed  = GS_BUTTONDOWN;
        nb.user_released = GS_DISK_OPEN;
        nb.user_hold     = 0;
        nb.ID            = GSID_DISK;
        
        if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

         nb.x             = GET_X_COORD(570);      // FIXME FLOH
         nb.y             = GET_Y_COORD(460);      // FIXME FLOH
         nb.w             = GET_X_COORD(64);       // FIXME FLOH
         nb.shortcut      = 0;
         nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_LOCALE, "LOCALE");
         nb.pressed_text  = NULL;
         nb.user_pressed  = GS_BUTTONDOWN;
         nb.user_released = GS_LOCALE_OPEN;
         nb.user_hold     = 0;
         nb.ID            = GSID_LOCALE;
         
         if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

          nb.x             = GET_X_COORD(213);     // FIXME FLOH
          nb.y             = GET_Y_COORD(344);
          nb.w             = ywd->DspXRes/3;       // FIXME FLOH
          nb.shortcut      = 0;
          nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_HELP, "HELP");
          nb.pressed_text  = NULL;
          nb.user_pressed  = GS_BUTTONDOWN;
          nb.user_released = GS_HELP_OPEN;
          nb.user_hold     = 0;
          nb.ID            = GSID_HELP;
          
          if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

           nb.y             = GET_Y_COORD(382);
           nb.shortcut      = 0;
           nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_TITLE_QUIT, "QUIT");
           nb.pressed_text  = NULL;
           nb.user_pressed  = GS_BUTTONDOWN;
           nb.user_released = GS_QUIT;
           nb.user_hold     = 0;
           nb.ID            = GSID_QUIT;
           
           if( _methoda( GSR->Titel, BTM_NEWBUTTON, &nb ) ) {

            button_ok = TRUE;
            }
           }
          }
         }
        }
       }
      }
     }


    if( !button_ok ) {

        _LogMsg("Unable to add button to Titel\n");
        return( FALSE );
        }

    if( GSR->num_languages <= 1 ) {

        sb.number  = GSID_LOCALE;
        _methoda( GSR->Titel, BTM_DISABLEBUTTON, &sb );
        }

    /*** nicht darstellen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    
///

/// "Unterer Statusbalken"
    /* ---------------------------------------------------------------
    ** -------------------------------------------------------------*/

    gadgetwidth = ywd->DspXRes / 6;

    GSR->UBalken = _new("button.class",BTA_BoxX, 0,
                                       BTA_BoxY, ywd->DspYRes - ywd->FontH,
                                       BTA_BoxW, ywd->DspXRes,
                                       BTA_BoxH, ywd->FontH,
                                       TAG_DONE );
    
    if( !GSR->UBalken ) {
        
        _LogMsg("Unable to create Button-Object\n");
        return( FALSE );
        }

    button_ok = FALSE;

    nb.pressed_font  = FONTID_MAPCUR_32;    // frueher ICON_NB
    nb.unpressed_font= FONTID_MAPCUR_16;
    nb.disabled_font = FONTID_ENERGY;
    nb.modus         = BM_GADGET;
    nb.x             = gadgetwidth + ReqDeltaX;
    nb.y             = 0;
    nb.w             = gadgetwidth;
    nb.shortcut      = 0;
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SHELL_SETBACK, "REWIND");
    nb.pressed_text  = NULL;
    nb.user_pressed  = GS_BUTTONDOWN;
    nb.user_released = GS_PL_SETBACK;
    nb.user_hold     = 0;
    nb.ID            = GSID_PL_SETBACK;
    nb.flags         = BT_TEXT|BT_CENTRE|BT_BORDER;
    
    if( _methoda( GSR->UBalken, BTM_NEWBUTTON, &nb ) ) {

     nb.x             = 2 * (gadgetwidth + ReqDeltaX);
     nb.shortcut      = 0;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SHELL_STEPFORWARD, "STEP FORWARD");
     nb.pressed_text  = NULL;
     nb.user_pressed  = 0;
     nb.user_released = GS_PL_ENDFASTFORWARD;
     nb.user_hold     = GS_PL_FASTFORWARD;
     nb.ID            = GSID_PL_FASTFORWARD;
     
     if( _methoda( GSR->UBalken, BTM_NEWBUTTON, &nb ) ) {

      nb.x             = 0;
      nb.shortcut      = 0;
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SHELL_STARTGAME, "START GAME");
      nb.pressed_text  = NULL;
      nb.user_pressed  = GS_BUTTONDOWN;
      nb.user_released = GS_PL_GAME;
      nb.user_hold     = 0;
      nb.ID            = GSID_PL_GAME;
      
      if( _methoda( GSR->UBalken, BTM_NEWBUTTON, &nb ) ) {

       nb.x             = (WORD)(ywd->DspXRes - 3 * gadgetwidth - 2 * ReqDeltaX);
       nb.shortcut      = 0;
       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SHELL_GOTOLOADSAVE, "GOTO LOADSAVE");
       nb.pressed_text  = NULL;
       nb.user_pressed  = GS_BUTTONDOWN;
       nb.user_released = GS_PL_GOTOLOADSAVE;
       nb.user_hold     = 0;
       nb.ID            = GSID_PL_GOTOLOADSAVE;
       
       if( _methoda( GSR->UBalken, BTM_NEWBUTTON, &nb ) ) {

        nb.x             = (WORD)(ywd->DspXRes - 2 * gadgetwidth - 2 * ReqDeltaX);
        nb.shortcut      = 0;
        nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SHELL_LOADGAME, "LOAD GAME");
        nb.pressed_text  = NULL;
        nb.user_pressed  = GS_BUTTONDOWN;
        nb.user_released = GS_PL_LOAD;
        nb.user_hold     = 0;
        nb.ID            = GSID_PL_LOAD;
       
        if( _methoda( GSR->UBalken, BTM_NEWBUTTON, &nb ) ) {

         nb.x             = (WORD)(ywd->DspXRes - 1 * gadgetwidth);
         nb.shortcut      = 0;
         nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SHELL_GOBACK, "GO BACK");
         nb.pressed_text  = NULL;
         nb.user_pressed  = GS_BUTTONDOWN;
         nb.user_released = GS_QUIT;
         nb.user_hold     = 0;
         nb.ID            = GSID_PL_QUIT;   // also Icon-Quit!
        
         if( _methoda( GSR->UBalken, BTM_NEWBUTTON, &nb ) ) {

          button_ok = TRUE;
          }
         }
        }
       }
      }
     }

    if( !button_ok ) {

        _LogMsg("Unable to add button to sub-bar\n");
        return( FALSE );
        }

    /*** Loadgadget ausschalten? ***/
    if( GSR->exist_savegame != 1 ) {

        /*** denn dann wurde schon ein test gemacht ***/
        sb.visible = 0L;
        sb.number  = GSID_PL_LOAD;
        _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &sb );
        }

    /*** MB-Gadgets erstmal ausschalten ***/
    sb.visible = 0L;
    sb.number  = GSID_PL_GAME;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PL_FASTFORWARD;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PL_SETBACK;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &sb );

    /*** nicht darstellen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
    
///

    /*** ConfirmRequester ***/
    
    /* --------------------------------------------------------
    ** Mit der gesamten Groesse loesen wir das problem der Maus
    ** eingaben, die auch andere Sachen beeinflussen koennen
    ** ------------------------------------------------------*/
    GSR->confirm = _new("button.class",BTA_BoxX, 0,
                                       BTA_BoxY, 0,
                                       BTA_BoxW, ywd->DspXRes,
                                       BTA_BoxH, ywd->DspYRes,
                                       TAG_DONE );
      
    if( !GSR->confirm ) {
        
        _LogMsg("Unable to create Confirm-Button-Object\n");
        return( FALSE );
        }

    button_ok = FALSE;

    /*** ist am Ende... ***/
    nb.pressed_font  = FONTID_MAPCUR_32;    // frueher ICON_NB
    nb.unpressed_font= FONTID_MAPCUR_16;
    nb.disabled_font = FONTID_ENERGY;
    nb.modus         = BM_GADGET;
    nb.x             = GET_X_COORD(160);
    nb.y             = GET_Y_COORD(245);
    nb.w             = GET_X_COORD(80);
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_OK, "OK");
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = GS_BUTTONDOWN;
    nb.user_released = GS_CONFIRMOK;
    nb.user_hold     = 0;
    nb.ID            = GSID_CONFIRMOK;
    nb.flags         = BT_BORDER|BT_CENTRE|BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);

    if( _methoda( GSR->confirm, BTM_NEWBUTTON, &nb ) ) {

     nb.x             = GET_X_COORD(400);
     nb.shortcut      = 0;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_CANCEL, "CANCEL");
     nb.pressed_text  = NULL;
     nb.user_pressed  = GS_BUTTONDOWN;
     nb.user_released = GS_CONFIRMCANCEL;
     nb.user_hold     = 0;
     nb.ID            = GSID_CONFIRMCANCEL;   // also Icon-Quit!
    
     if( _methoda( GSR->confirm, BTM_NEWBUTTON, &nb ) ) {

      nb.pressed_font  = FONTID_MAPCUR_4;
      nb.unpressed_font= FONTID_MAPCUR_4;
      nb.disabled_font = FONTID_MAPCUR_4;
      nb.modus         = BM_STRING;
      nb.x             = GET_X_COORD(160);
      nb.y             = GET_Y_COORD(225);
      nb.w             = GET_X_COORD(320);
      nb.unpressed_text= " ";
      nb.pressed_text  = NULL;
      nb.shortcut      = 0;
      nb.user_pressed  = 0;
      nb.user_released = 0;
      nb.user_hold     = 0;
      nb.ID            = GSID_CONFIRMTEXT;
      nb.flags         = BT_TEXT|BT_CENTRE;
      nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
      nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
      nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);

      if( _methoda( GSR->confirm, BTM_NEWBUTTON, &nb ) ) {
       
       button_ok = TRUE;
       }
      }
     } 


    sb.number  = GSID_CONFIRMOK;
    _methoda( GSR->confirm, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_CONFIRMCANCEL;
    _methoda( GSR->confirm, BTM_DISABLEBUTTON, &sb );

    /*** nicht darstellen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->confirm, BTM_SWITCHPUBLISH, &sp );

 
/// "Der EingabeRequester"
    button_ok = FALSE;


    /*** Nun folgt das spezielle ButtonObjekt fnr den InputRequester ***/
    IListWidth = bwidth - ywd->PropW;
    if (!yw_InitListView(ywd, &(GSR->imenu),
                         LIST_Resize,         FALSE,
                         LIST_NumEntries,     NUM_INPUTEVENTS,
                         LIST_ShownEntries,   NUM_INPUT_SHOWN,
                         LIST_FirstShown,     0,
                         LIST_Selected,       0,
                         LIST_MaxShown,       NUM_INPUT_SHOWN,
                         LIST_DoIcon,         FALSE,
                         LIST_EntryHeight,    ywd->FontH,
                         LIST_EntryWidth,     IListWidth,
                         LIST_Enabled,        TRUE,
                         LIST_VBorder,        ywd->EdgeH,
                         LIST_ImmediateInput, FALSE,
                         LIST_KeyboardInput,  TRUE,
                         TAG_DONE)) {

                         _LogMsg("Unable to create Input-ListView\n");
                         return(FALSE);
                         }

    GSR->ib_xoffset = bstart_x;
    GSR->ib_yoffset = bstart_y;
    GSR->binput = _new("button.class",BTA_BoxX, GSR->ib_xoffset,
                                      BTA_BoxY, GSR->ib_yoffset,
                                      BTA_BoxW, ywd->DspXRes - GSR->ib_xoffset,
                                      BTA_BoxH, ywd->DspYRes - GSR->ib_yoffset,
                                      TAG_DONE );
    if( !GSR->binput ) {
    
        _LogMsg("Unable to create Input-Button\n");
        return( FALSE );
        }

    /*** Menü anpassen ***/
    GSR->imenu.Req.req_cbox.rect.x = bstart_x;
    GSR->imenu.Req.req_cbox.rect.y = bstart_y + 4 * (ywd->FontH + ReqDeltaY);

    button_ok = FALSE;

    /* ---------------------------------------------------------------
    ** Alle Gadgetbreiten beziehen sich auf die Gesamtbreite, so sie
    ** nicht fest wie Checkmarks sind. Wir ermitteln also den Rest aus
    ** der breite der Checkmarks und den ZwischenrSumen und teilen das
    ** auf.
    ** -------------------------------------------------------------*/


    /* --------------------------------------------------------------------
    ** Die gadgets müssen aufeinander angepaßt werden, folglich orientieren
    ** wir uns mit restbreite an der "klareren" nächsten Zeile.
    ** ------------------------------------------------------------------*/
    restbreite = IListWidth - 3 * ReqDeltaX - 2 * checkmarkwidth + ywd->PropW;

    nb.pressed_font  = FONTID_MAPCUR_4;
    nb.unpressed_font= FONTID_MAPCUR_4;
    nb.disabled_font = FONTID_MAPCUR_4;
    nb.modus         = BM_STRING;
    nb.x             = 0;
    nb.y             = 0;
    nb.w             = bwidth;
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_HEADLINE,"INPUT SETTINGS");
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = 0;
    nb.user_released = 0;
    nb.user_hold     = 0;
    nb.ID            = GSID_INPUTHEADLINE;
    nb.flags         = BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
    
    if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

     nb.x             = 0;
     nb.y             = ywd->FontH + ReqDeltaY;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_HEADLINE2,"2");
     nb.pressed_text  = NULL;
     nb.user_hold     = 0;
     nb.ID            = GSID_INPUTHEADLINE2;
     nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);

     if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

      nb.x             = 0;
      nb.y             = 2 * (ywd->FontH + ReqDeltaY);
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_HEADLINE3,"3");
      nb.pressed_text  = NULL;
      nb.user_hold     = 0;
      nb.ID            = GSID_INPUTHEADLINE3;
      
      if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

       nb.x             = 0;
       nb.y             = 3 * (ywd->FontH + ReqDeltaY);
       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_HEADLINE4,"4");
       nb.pressed_text  = NULL;
       nb.user_hold     = 0;
       nb.ID            = GSID_INPUTHEADLINE4;
       
       if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

        nb.pressed_font  = FONTID_MAPCUR_32;
        nb.unpressed_font= FONTID_MAPCUR_16;
        nb.disabled_font = FONTID_ENERGY;
        nb.modus         = BM_SWITCH;
        nb.x             = bwidth/6;
        nb.y             = (NUM_INPUT_SHOWN + 6) * ywd->FontH + 6 * ReqDeltaY;
        nb.w             = checkmarkwidth;
        nb.unpressed_text= "g";
        nb.pressed_text  = "g";
        nb.shortcut      = 0;
        nb.user_pressed  = GS_USEJOYSTICK;
        nb.user_released = GS_NOJOYSTICK;
        nb.user_hold     = 0;
        nb.ID            = GSID_JOYSTICK;
        nb.flags         = 0;    // kein Rand, nicht zentriert
         
        if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

         nb.pressed_font  = FONTID_MAPCUR_4;
         nb.unpressed_font= FONTID_MAPCUR_4;
         nb.disabled_font = FONTID_MAPCUR_4;
         nb.modus         = BM_STRING;
         nb.x             = (WORD)( bwidth/6 + ReqDeltaX + checkmarkwidth );
         nb.w             = (WORD)( bwidth/2 - ReqDeltaX);
         nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_JOYSTICK,"JOYSTICK");
         nb.pressed_text  = NULL;
         nb.shortcut      = 0;
         nb.user_pressed  = 0;
         nb.user_released = 0;
         nb.user_hold     = 0;
         nb.ID            = 2;
         nb.flags         = BT_TEXT;
         nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
         nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
         nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
         
         if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

          nb.pressed_font  = FONTID_MAPCUR_32;
          nb.unpressed_font= FONTID_MAPCUR_16;
          nb.disabled_font = FONTID_ENERGY;
          nb.modus         = BM_SWITCH;
          nb.x             = (WORD)(bwidth/2) + ReqDeltaX;
          nb.w             = checkmarkwidth;
          nb.unpressed_text= "g";
          nb.pressed_text  = "g";
          nb.shortcut      = 0;
          nb.user_pressed  = GS_ALTJOYSTICK_YES;
          nb.user_released = GS_ALTJOYSTICK_NO;
          nb.user_hold     = 0;
          nb.ID            = GSID_ALTJOYSTICK;
          nb.flags         = 0;    // kein Rand, nicht zentriert
         
          if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

           nb.pressed_font  = FONTID_MAPCUR_4;
           nb.unpressed_font= FONTID_MAPCUR_4;
           nb.disabled_font = FONTID_MAPCUR_4;
           nb.modus         = BM_STRING;
           nb.x             = (WORD)( bwidth/2 + 2 * ReqDeltaX + checkmarkwidth );
           nb.w             = (WORD)( bwidth/2 - ReqDeltaX);
           nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_ALTJOYSTICK,"ALTERNATE JOYSTICK MODEL");
           nb.pressed_text  = NULL;
           nb.shortcut      = 0;
           nb.user_pressed  = 0;
           nb.user_released = 0;
           nb.user_hold     = 0;
           nb.ID            = 2;
           nb.flags         = BT_TEXT;
           nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
           nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
           nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
         
           if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

            nb.pressed_font  = FONTID_MAPCUR_32;
            nb.unpressed_font= FONTID_MAPCUR_16;
            nb.disabled_font = FONTID_ENERGY;
            nb.modus         = BM_SWITCH;
            nb.x             = (WORD)(bwidth/3);
            nb.y             = (NUM_INPUT_SHOWN + 7) * ywd->FontH + 7 * ReqDeltaY;
            nb.w             = checkmarkwidth;
            nb.unpressed_text= "g";
            nb.pressed_text  = "g";
            nb.shortcut      = 0;
            nb.user_pressed  = GS_USEFORCEFEEDBACK;
            nb.user_released = GS_NOFORCEFEEDBACK;
            nb.user_hold     = 0;
            nb.ID            = GSID_FORCEFEEDBACK;
            nb.flags         = 0;    // kein Rand, nicht zentriert
          
            if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

             nb.pressed_font  = FONTID_MAPCUR_4;
             nb.unpressed_font= FONTID_MAPCUR_4;
             nb.disabled_font = FONTID_MAPCUR_4;
             nb.modus         = BM_STRING;
             nb.x             = (WORD)( bwidth/3 + ReqDeltaX + checkmarkwidth );
             nb.w             = (WORD)( bwidth/2 );
             nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_FORCEFEEDBACK,"DISABLE FORCE FEEDBACK");
             nb.pressed_text  = NULL;
             nb.shortcut      = 0;
             nb.user_pressed  = 0;
             nb.user_released = 0;
             nb.user_hold     = 0;
             nb.ID            = 2;
             nb.flags         = BT_TEXT;
           
             if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

              /*** neue zeile erfordert neue "Restbreite" ***/
              restbreite = IListWidth - 2 * ReqDeltaX + ywd->PropW;

              nb.pressed_font  = FONTID_MAPCUR_32;
              nb.unpressed_font= FONTID_MAPCUR_16;
              nb.disabled_font = FONTID_ENERGY;
              nb.modus         = BM_GADGET;
              nb.x             = bwidth/6;
              nb.y             = (NUM_INPUT_SHOWN + 5) * ywd->FontH + 5 * ReqDeltaY;
              nb.w             = (WORD)( bwidth/3 - ReqDeltaX );
              nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_IGADGET_SWITCHOFF,"SWITCH OFF");                           ;
              nb.pressed_text  = NULL;
              nb.shortcut      = 0;
              nb.user_pressed  = GS_BUTTONDOWN;
              nb.user_released = GS_SWITCHOFF;
              nb.user_hold     = 0;
              nb.ID            = GSID_SWITCHOFF;
              nb.flags         = BT_BORDER | BT_CENTRE | BT_TEXT;
              nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
              nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
              nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);

              if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

               nb.x             = (WORD)(bwidth/2) + ReqDeltaX;
               nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_RESET,"RESET");                           ;
               nb.pressed_text  = NULL;
               nb.shortcut      = 0;
               nb.user_released = GS_INPUTRESET;
               nb.user_hold     = 0;
               nb.ID            = GSID_INPUTRESET;
           
               if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

                nb.modus         = BM_GADGET;
                nb.x             = ok_x;
                nb.y             = ok_y;
                nb.w             = ok_w;
                nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_OK,"OK");                           ;
                nb.pressed_text  = NULL;
                nb.shortcut      = 0;
                nb.user_pressed  = GS_BUTTONDOWN;
                nb.user_released = GS_INPUTOK;
                nb.user_hold     = 0;
                nb.ID            = GSID_INPUTOK;
              
                if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

                 nb.x             = help_x;
                 nb.y             = help_y;
                 nb.w             = help_w;
                 nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_HELP,"HELP");                           ;
                 nb.pressed_text  = NULL;
                 nb.shortcut      = 0;
                 nb.user_released = GS_HELP;
                 nb.user_hold     = 0;
                 nb.ID            = GSID_INPUTHELP;
               
                 if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

                  nb.x             = cancel_x;
                  nb.y             = cancel_y;
                  nb.w             = cancel_w;
                  nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_CANCEL,"CANCEL");                           ;
                  nb.pressed_text  = NULL;
                  nb.shortcut      = 0;
                  nb.user_released = GS_INPUTCANCEL;
                  nb.user_hold     = 0;
                  nb.ID            = GSID_INPUTCANCEL;
              
                  if( _methoda(GSR->binput, BTM_NEWBUTTON, &nb ) ) {

                   button_ok = TRUE;
                   }
                  }
                 }
                }
               }
              }
             }
            }
           }
          }
         }
        }
       }
      }
     }

    if( !button_ok ) {

        _LogMsg("Unable to add input-button\n");
        return( FALSE);
        }


    /** Button ausschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->binput, BTM_SWITCHPUBLISH, &sp );

///

/// "Der SettingsRequester"
    
    /*** Vorbereitung VideoMenu ***/
    i = 0;
    n = GSR->videolist.mlh_Head;
    while( n->mln_Succ ) { n = n->mln_Succ; i++; }

    /*** Restbreite schon hier fnr Listviewbreite ***/
    restbreite = bwidth - 3 * ReqDeltaX - ywd->PropW;

    if (!yw_InitListView(ywd, &(GSR->vmenu),
                         LIST_Resize,       FALSE,
                         LIST_NumEntries,   i,
                         LIST_ShownEntries, NUM_VIDEO_SHOWN,
                         LIST_FirstShown,   0,
                         LIST_Selected,     0,
                         LIST_MaxShown,     NUM_VIDEO_SHOWN,
                         LIST_DoIcon,       FALSE,
                         LIST_EntryHeight,  ywd->FontH,
                         LIST_EntryWidth,   (LONG)(0.6 * restbreite),
                         LIST_Enabled,      TRUE,
                         LIST_VBorder,      ywd->EdgeH,
                         LIST_ImmediateInput, TRUE,
                         LIST_KeyboardInput,TRUE,
                         TAG_DONE)) {

                         _LogMsg("Unable to create Game-Video-Menu\n");
                         return(FALSE);
                         }

    if (!yw_InitListView(ywd, &(GSR->d3dmenu),
                         LIST_Resize,       FALSE,
                         LIST_NumEntries,   num_dev,
                         LIST_ShownEntries, NUM_D3D_SHOWN,
                         LIST_FirstShown,   0,
                         LIST_Selected,     sel_dev,
                         LIST_MaxShown,     NUM_D3D_SHOWN,
                         LIST_DoIcon,       FALSE,
                         LIST_EntryHeight,  ywd->FontH,
                         LIST_EntryWidth,   (LONG)(0.6 * restbreite),
                         LIST_Enabled,      TRUE,
                         LIST_VBorder,      ywd->EdgeH,
                         LIST_ImmediateInput, TRUE,
                         LIST_KeyboardInput,TRUE,
                         TAG_DONE)) {

                         _LogMsg("Unable to create D3D-Menu\n");
                         return(FALSE);
                         }

    GSR->vb_xoffset = bstart_x;
    GSR->vb_yoffset = bstart_y;
    GSR->bvideo = _new("button.class",BTA_BoxX, GSR->vb_xoffset,
                                      BTA_BoxY, GSR->vb_yoffset,
                                      BTA_BoxW, ywd->DspXRes - GSR->vb_xoffset,
                                      BTA_BoxH, ywd->DspYRes - GSR->vb_yoffset,
                                      TAG_DONE );
    if( !GSR->bvideo ) {
    
        _LogMsg("Unable to create Video-Button\n");
        return( FALSE );
        }

    button_ok = FALSE;
    GSR->vmenu.Req.req_cbox.rect.x   = bstart_x + ReqDeltaX + (WORD)(0.4*restbreite);
    GSR->vmenu.Req.req_cbox.rect.y   = bstart_y + 6 * ywd->FontH + 6 * ReqDeltaY;
    GSR->d3dmenu.Req.req_cbox.rect.x = bstart_x + ReqDeltaX + (WORD)(0.4*restbreite);
    GSR->d3dmenu.Req.req_cbox.rect.y = bstart_y + 7 * ywd->FontH + 7 * ReqDeltaY;

    /*** Bildschirm-Aufloesung ***/
    nb.pressed_font  = FONTID_MAPCUR_4;
    nb.unpressed_font= FONTID_MAPCUR_4;
    nb.disabled_font = FONTID_MAPCUR_4;
    nb.modus         = BM_STRING;
    nb.x             = 0;
    nb.y             = 0;
    nb.w             = bwidth;
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_HEADLINE,"GAME SETTINGS");
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = 0;
    nb.user_released = 0;
    nb.user_hold     = 0;
    nb.ID            = GSID_SETTINGSHEADLINE;
    nb.flags         = BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
    
    if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

     nb.x             = 0;
     nb.y             = ywd->FontH + ReqDeltaY;
     nb.w             = bwidth;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_HEADLINE2,"2");
     nb.pressed_text  = NULL;
     nb.ID            = GSID_SETTINGSHEADLINE2;
     nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
     
     if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

      nb.x             = 0;
      nb.y             = 2 * (ywd->FontH + ReqDeltaY);
      nb.w             = bwidth;
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_HEADLINE3,"3");
      nb.pressed_text  = NULL;
      nb.ID            = GSID_SETTINGSHEADLINE3;
      
      if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

       nb.x             = 0;
       nb.y             = 3 * (ywd->FontH + ReqDeltaY);
       nb.w             = bwidth;
       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_HEADLINE4,"4");
       nb.pressed_text  = NULL;
       nb.ID            = GSID_SETTINGSHEADLINE4;
       
       if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

        nb.pressed_font  = FONTID_MAPCUR_4;
        nb.unpressed_font= FONTID_MAPCUR_4;
        nb.disabled_font = FONTID_MAPCUR_4;
        nb.modus         = BM_STRING;
        nb.x             = 0;
        nb.y             = 5 * (ywd->FontH + ReqDeltaY);
        nb.w             = (WORD)( 0.4 * restbreite );
        nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_RESSHELL,"RESOLUTION SHELL");
        nb.pressed_text  = NULL;
        nb.shortcut      = 0;
        nb.user_pressed  = 0;
        nb.user_released = 0;
        nb.user_hold     = 0;
        nb.ID            = 2;
        nb.flags         = BT_BORDER | BT_TEXT;
        nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
        nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
        nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
        
        if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

         struct video_node *vn = (struct video_node *) GSR->videolist.mlh_Head;
         while( vn->node.mln_Succ ) {
             if( vn->modus == ywd->GameRes )
                 break;
             vn = (struct video_node *) vn->node.mln_Succ;
             }

         nb.pressed_font  = FONTID_MAPCUR_32;
         nb.unpressed_font= FONTID_MAPCUR_16;
         nb.disabled_font = FONTID_ENERGY;
         nb.modus         = BM_SWITCH;
         nb.x             = (WORD)(ReqDeltaX + 0.4 * restbreite);
         nb.w             = (WORD)(0.6 * restbreite);
         nb.unpressed_text= vn->name;
         nb.pressed_text  = NULL;
         nb.shortcut      = 0;
         nb.user_pressed  = GS_VMENU_OPEN;
         nb.user_released = GS_VMENU_CLOSE;
         nb.user_hold     = 0;
         nb.ID            = GSID_VMENU_BUTTON;
         nb.flags         = BT_BORDER | BT_CENTRE | BT_TEXT;
         nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
         nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
         nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
         
         if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

          nb.pressed_font  = FONTID_MAPCUR_4;
          nb.unpressed_font= FONTID_MAPCUR_4;
          nb.disabled_font = FONTID_MAPCUR_4;
          nb.modus         = BM_STRING;
          nb.x             = 0;
          nb.y             = 6 * (ywd->FontH + ReqDeltaY);
          nb.w             = (WORD)( 0.4 * restbreite );
          nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_3DDEVICE,"SELECT 3D DEVICE");
          nb.pressed_text  = NULL;
          nb.shortcut      = 0;
          nb.user_pressed  = 0;
          nb.user_released = 0;
          nb.user_hold     = 0;
          nb.ID            = 2;
          nb.flags         = BT_BORDER | BT_TEXT;
          nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
          nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
          nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
          
          if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

           nb.pressed_font  = FONTID_MAPCUR_32;
           nb.unpressed_font= FONTID_MAPCUR_16;
           nb.disabled_font = FONTID_ENERGY;
           nb.modus         = BM_SWITCH;
           nb.x             = (WORD)(ReqDeltaX + 0.4 * restbreite);
           nb.w             = (WORD)(0.6 * restbreite);
           nb.unpressed_text= GSR->d3dname;
           nb.pressed_text  = NULL;
           nb.shortcut      = 0;
           nb.user_pressed  = GS_3DMENU_OPEN;
           nb.user_released = GS_3DMENU_CLOSE;
           nb.user_hold     = 0;
           nb.ID            = GSID_3DMENU_BUTTON;
           nb.flags         = BT_BORDER | BT_CENTRE | BT_TEXT;
           nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
           nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
           nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
           
           if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

            /*** Sichtweite ***/
            restbreite = VReqWidth - 6 * ReqDeltaX - 2 * checkmarkwidth;
        
            nb.pressed_font  = FONTID_MAPCUR_32;
            nb.unpressed_font= FONTID_MAPCUR_16;
            nb.disabled_font = FONTID_ENERGY;
            nb.modus         = BM_SWITCH;
            nb.x             = 0;
            nb.y             = 7 * (ywd->FontH + ReqDeltaY);
            nb.w             = checkmarkwidth;
            nb.unpressed_text= "g";
            nb.pressed_text  = "g";
            nb.shortcut      = 0;
            nb.user_pressed  = GS_FARVIEW;
            nb.user_released = GS_NOFARVIEW;
            nb.user_hold     = 0;
            nb.ID            = GSID_FARVIEW;
            nb.flags         = 0;
           
            if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
        
             nb.pressed_font  = FONTID_MAPCUR_4;
             nb.unpressed_font= FONTID_MAPCUR_4;
             nb.disabled_font = FONTID_MAPCUR_4;
             nb.modus         = BM_STRING;
             nb.x             = checkmarkwidth + ReqDeltaX;
             nb.w             = restbreite / 2;
             nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_FARVIEW,"FAR VIEW");
             nb.pressed_text  = NULL;
             nb.shortcut      = 0;
             nb.user_pressed  = 0;
             nb.user_released = 0;
             nb.user_hold     = 0;
             nb.ID            = 2;
             nb.flags         = BT_TEXT;
             nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
             nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
             nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
           
             if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
             
              /*** Himmel ***/
              nb.pressed_font  = FONTID_MAPCUR_32;
              nb.unpressed_font= FONTID_MAPCUR_16;
              nb.disabled_font = FONTID_ENERGY;
              nb.modus         = BM_SWITCH;
              nb.x             = checkmarkwidth + restbreite/2 + 3 * ReqDeltaX;
              nb.w             = checkmarkwidth;
              nb.unpressed_text= "g";
              nb.pressed_text  = "g";
              nb.shortcut      = 0;
              nb.user_pressed  = GS_HEAVEN;
              nb.user_released = GS_NOHEAVEN;
              nb.user_hold     = 0;
              nb.ID            = GSID_HEAVEN;
              nb.flags         = 0;
             
              if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
          
               nb.pressed_font  = FONTID_MAPCUR_4;
               nb.unpressed_font= FONTID_MAPCUR_4;
               nb.disabled_font = FONTID_MAPCUR_4;
               nb.modus         = BM_STRING;
               nb.x             = 2*checkmarkwidth + restbreite/2 + 4 * ReqDeltaX;
               nb.w             = restbreite / 2;
               nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_HEAVEN,"HEAVEN");
               nb.pressed_text  = NULL;
               nb.shortcut      = 0;
               nb.user_pressed  = 0;
               nb.user_released = 0;
               nb.user_hold     = 0;
               nb.ID            = 2;
               nb.flags         = BT_TEXT;
             
               if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
              
                /*** Software Mousepointer ***/
                nb.pressed_font  = FONTID_MAPCUR_32;
                nb.unpressed_font= FONTID_MAPCUR_16;
                nb.disabled_font = FONTID_ENERGY;
                nb.modus         = BM_SWITCH;
                nb.x             = 0;
                nb.y             = 8 * (ywd->FontH + ReqDeltaY);
                nb.w             = checkmarkwidth;
                nb.unpressed_text= "g";
                nb.pressed_text  = "g";
                nb.shortcut      = 0;
                nb.user_pressed  = GS_SOFTMOUSE;
                nb.user_released = GS_NOSOFTMOUSE;
                nb.user_hold     = 0;
                nb.ID            = GSID_SOFTMOUSE;
                nb.flags         = 0;
               
                if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
            
                 nb.pressed_font  = FONTID_MAPCUR_4;
                 nb.unpressed_font= FONTID_MAPCUR_4;
                 nb.disabled_font = FONTID_MAPCUR_4;
                 nb.modus         = BM_STRING;
                 nb.x             = checkmarkwidth + ReqDeltaX;
                 nb.w             = restbreite / 2;
                 nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_SOFTMOUSE,"SW MOUSEPOINTER");
                 nb.pressed_text  = NULL;
                 nb.shortcut      = 0;
                 nb.user_pressed  = 0;
                 nb.user_released = 0;
                 nb.user_hold     = 0;
                 nb.ID            = 2;
                 nb.flags         = BT_TEXT;

                 /*** Filtering ***/
                 if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
              
                  nb.pressed_font  = FONTID_MAPCUR_32;
                  nb.unpressed_font= FONTID_MAPCUR_16;
                  nb.disabled_font = FONTID_ENERGY;
                  nb.modus         = BM_SWITCH;
                  nb.x             = checkmarkwidth + restbreite/2 + 3 * ReqDeltaX;
                  nb.w             = checkmarkwidth;
                  nb.unpressed_text= "g";
                  nb.pressed_text  = "g";
                  nb.shortcut      = 0;
                  nb.user_pressed  = GS_DRAWPRIMITIVE_YES;
                  nb.user_released = GS_DRAWPRIMITIVE_NO;
                  nb.user_hold     = 0;
                  nb.ID            = GSID_DRAWPRIMITIVE;
                  nb.flags         = 0;
                 
                  if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
              
                   nb.pressed_font  = FONTID_MAPCUR_4;
                   nb.unpressed_font= FONTID_MAPCUR_4;
                   nb.disabled_font = FONTID_MAPCUR_4;
                   nb.modus         = BM_STRING;
                   nb.x             = 2*checkmarkwidth + restbreite/2 + 4 * ReqDeltaX;
                   nb.w             = restbreite / 2;
                   nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_DRAWPRIMITIVE, "OPENGL LIKE (:-)");
                   nb.pressed_text  = NULL;
                   nb.shortcut      = 0;
                   nb.user_pressed  = 0;
                   nb.user_released = 0;
                   nb.user_hold     = 0;
                   nb.ID            = 2;
                   nb.flags         = BT_TEXT;
                 
                   if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                    /*** Sound allgemein ***/
                    nb.modus         = BM_STRING;
                    nb.x             = checkmarkwidth + ReqDeltaX;
                    nb.y             = 9 * (ywd->FontH + ReqDeltaY);
                    nb.w             = restbreite/2;
                    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_16BITTEXTURE,"USE 16BIT TEXTURE");
                    nb.pressed_text  = NULL;
                    nb.shortcut      = 0;
                    nb.user_pressed  = 0;
                    nb.user_released = 0;
                    nb.user_hold     = 0;
                    nb.ID            = 0;
                    nb.flags         = BT_TEXT; // BT_BORDER;
                    
                    if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                     nb.pressed_font  = FONTID_MAPCUR_32;
                     nb.unpressed_font= FONTID_MAPCUR_16;
                     nb.disabled_font = FONTID_ENERGY;
                     nb.modus         = BM_SWITCH;
                     nb.x             = 0;
                     nb.w             = checkmarkwidth;
                     nb.unpressed_text= "g";
                     nb.pressed_text  = "g";
                     nb.shortcut      = 0;
                     nb.user_pressed  = GS_16BITTEXTURE_YES;
                     nb.user_released = GS_16BITTEXTURE_NO;
                     nb.user_hold     = 0;
                     nb.ID            = GSID_16BITTEXTURE;
                     nb.flags         = 0;

                      if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                      /*** CD Sound ***/
                      nb.pressed_font  = FONTID_MAPCUR_4;
                      nb.unpressed_font= FONTID_MAPCUR_4;
                      nb.disabled_font = FONTID_MAPCUR_4;
                      nb.modus         = BM_STRING;
                      nb.x             = 2 * checkmarkwidth + 4 * ReqDeltaX + restbreite/2;
                      nb.w             = restbreite/2;
                      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_USECDSOUND,"ENABLE CD AUDIO");
                      nb.pressed_text  = NULL;
                      nb.shortcut      = 0;
                      nb.user_pressed  = 0;
                      nb.user_released = 0;
                      nb.user_hold     = 0;
                      nb.ID            = 0;
                      nb.flags         = BT_TEXT; // BT_BORDER;
                      
                      if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                       nb.pressed_font  = FONTID_MAPCUR_32;
                       nb.unpressed_font= FONTID_MAPCUR_16;
                       nb.disabled_font = FONTID_ENERGY;
                       nb.modus         = BM_SWITCH;
                       nb.x             = restbreite/2 + checkmarkwidth + 3 * ReqDeltaX;
                       nb.w             = checkmarkwidth;
                       nb.unpressed_text= "g";
                       nb.pressed_text  = "g";
                       nb.shortcut      = 0;
                       nb.user_pressed  = GS_CDSOUND_YES;
                       nb.user_released = GS_CDSOUND_NO;
                       nb.user_hold     = 0;
                       nb.ID            = GSID_CDSOUND;
                       nb.flags         = 0;

                       if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                        /*** Enemyindikatoren ***/
                        nb.pressed_font  = FONTID_MAPCUR_32;
                        nb.unpressed_font= FONTID_MAPCUR_16;
                        nb.disabled_font = FONTID_ENERGY;
                        nb.modus         = BM_SWITCH;
                        nb.x             = 0;
                        nb.y             = 10 * (ywd->FontH + ReqDeltaY);
                        nb.w             = checkmarkwidth;
                        nb.unpressed_text= "g";
                        nb.pressed_text  = "g";
                        nb.shortcut      = 0;
                        nb.user_pressed  = GS_ENEMY_YES;
                        nb.user_released = GS_ENEMY_NO;
                        nb.user_hold     = 0;
                        nb.ID            = GSID_ENEMYINDICATOR;
                        nb.flags         = 0;

                        if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                         nb.pressed_font  = FONTID_MAPCUR_4;
                         nb.unpressed_font= FONTID_MAPCUR_4;
                         nb.disabled_font = FONTID_MAPCUR_4;
                         nb.modus         = BM_STRING;
                         nb.x             = checkmarkwidth + ReqDeltaX;
                         nb.w             = restbreite/2 - checkmarkwidth;
                         nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_ENEMY,"ENEMY INDICATOR");
                         nb.pressed_text  = NULL;
                         nb.shortcut      = 0;
                         nb.user_pressed  = 0;
                         nb.user_released = 0;
                         nb.user_hold     = 0;
                         nb.ID            = 0;
                         nb.flags         = BT_TEXT; // BT_BORDER;
                         
                         if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                          /*** Kanal vertauschen ***/
                          nb.pressed_font  = FONTID_MAPCUR_4;
                          nb.unpressed_font= FONTID_MAPCUR_4;
                          nb.disabled_font = FONTID_MAPCUR_4;
                          nb.modus         = BM_STRING;
                          nb.x             = 2 * checkmarkwidth + 4 * ReqDeltaX + restbreite/2;
                          nb.w             = restbreite/2;
                          nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_INVERTLR,"INVERT LEFT-RIGHT DIVISION ");
                          nb.pressed_text  = NULL;
                          nb.shortcut      = 0;
                          nb.user_pressed  = 0;
                          nb.user_released = 0;
                          nb.user_hold     = 0;
                          nb.ID            = 0;
                          nb.flags         = BT_TEXT; // BT_BORDER;
                          
                          if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                           nb.pressed_font  = FONTID_MAPCUR_32;
                           nb.unpressed_font= FONTID_MAPCUR_16;
                           nb.disabled_font = FONTID_ENERGY;
                           nb.modus         = BM_SWITCH;
                           nb.x             = restbreite/2 + checkmarkwidth + 3 * ReqDeltaX;
                           nb.w             = checkmarkwidth;
                           nb.unpressed_text= "g";
                           nb.pressed_text  = "g";
                           nb.shortcut      = 0;
                           nb.user_pressed  = GS_SOUND_RL;
                           nb.user_released = GS_SOUND_LR;
                           nb.user_hold     = 0;
                           nb.ID            = GSID_SOUND_LR;
                           nb.flags         = 0;

                           if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
                               
                            /*** Zerstoerungseffekte ***/
                            restbreite = VReqWidth - 5 * ReqDeltaX;
                           
                            nb.pressed_font  = FONTID_MAPCUR_4;
                            nb.unpressed_font= FONTID_MAPCUR_4;
                            nb.disabled_font = FONTID_MAPCUR_4;
                            nb.modus         = BM_STRING;
                            nb.x             = 0;
                            nb.y             = 11 * (ywd->FontH + ReqDeltaY);
                            nb.w             = (WORD)(0.3 * restbreite);
                            nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_VGADGET_DESTFX,"DESTRUCTION FX");
                            nb.pressed_text  = NULL;
                            nb.shortcut      = 0;
                            nb.user_pressed  = 0;
                            nb.user_released = 0;
                            nb.user_hold     = 0;
                            nb.flags         = BT_TEXT;
                            nb.ID            = 2;
                            
                            if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
                            
                             pi.min   = 0;
                             pi.max   = NUM_DESTFX;
                             pi.value = 8;
                           
                             nb.pressed_font  = FONTID_MAPCUR_16;
                             nb.unpressed_font= FONTID_MAPCUR_16;
                             nb.disabled_font = FONTID_ENERGY;
                             nb.modus         = BM_SLIDER;
                             nb.x             = (WORD)(0.3  * restbreite + ReqDeltaX);
                             nb.w             = (WORD)(0.55 * restbreite);
                             nb.unpressed_text= " ";
                             nb.pressed_text  = NULL;
                             nb.shortcut      = 0;
                             nb.user_pressed  = GS_FXSLIDERHIT;;
                             nb.user_released = GS_FXSLIDERRELEASED;
                             nb.user_hold     = GS_FXSLIDERHOLD;
                             nb.ID            = GSID_FXSLIDER;
                             nb.flags         = 0;
                             nb.specialinfo   = (LONG *) &pi;
                            
                             if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {
                           
                              nb.pressed_font  = FONTID_MAPCUR_4;
                              nb.unpressed_font= FONTID_MAPCUR_4;
                              nb.disabled_font = FONTID_MAPCUR_4;
                              nb.modus         = BM_STRING;
                              nb.x             = (WORD)(0.85 * restbreite + ReqDeltaX);
                              nb.w             = (WORD)(0.15 * restbreite);
                              nb.unpressed_text= " 4";
                              nb.pressed_text  = NULL;
                              nb.shortcut      = 0;
                              nb.user_pressed  = 0;
                              nb.user_released = 0;
                              nb.user_hold     = 0;
                              nb.ID            = GSID_FXNUMBER;
                              nb.flags         = BT_BORDER | BT_CENTRE | BT_TEXT;
                            
                              if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                               /*** Lautstaerke ***/
                               nb.modus         = BM_STRING;
                               nb.x             = 0;
                               nb.y             = 12 * (ywd->FontH + ReqDeltaY);
                               nb.w             = (WORD)(0.3 * restbreite);
                               nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_VOLUME,"FX VOLUME");
                               nb.pressed_text  = NULL;
                               nb.shortcut      = 0;
                               nb.user_pressed  = 0;
                               nb.user_released = 0;
                               nb.user_hold     = 0;
                               nb.ID            = 2;
                               nb.flags         = BT_TEXT; // BT_BORDER;
                               
                               if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                pi.min   = 1;
                                pi.max   = 127;
                                pi.value = 100;

                                nb.pressed_font  = FONTID_MAPCUR_16;
                                nb.unpressed_font= FONTID_MAPCUR_16;
                                nb.disabled_font = FONTID_ENERGY;
                                nb.modus         = BM_SLIDER;
                                nb.x             = (WORD)(0.3  * restbreite + ReqDeltaX);
                                nb.w             = (WORD)( 0.55 * restbreite );
                                nb.unpressed_text= " ";
                                nb.pressed_text  = NULL;
                                nb.shortcut      = 0;
                                nb.user_pressed  = GS_SOUND_HITFXVOLUME;
                                nb.user_released = GS_SOUND_RELEASEFXVOLUME;
                                nb.user_hold     = GS_SOUND_HOLDFXVOLUME;
                                nb.ID            = GSID_FXVOLUMESLIDER;
                                nb.flags         = 0; // BT_BORDER;
                                nb.specialinfo   = (LONG *) &pi;
                                
                                if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                 nb.pressed_font  = FONTID_MAPCUR_4;
                                 nb.unpressed_font= FONTID_MAPCUR_4;
                                 nb.disabled_font = FONTID_MAPCUR_4;
                                 nb.modus         = BM_STRING;
                                 nb.x             = (WORD)(0.85 * restbreite + 2 * ReqDeltaX);
                                 nb.w             = (WORD)(0.15 * restbreite);
                                 nb.unpressed_text= "4";
                                 nb.pressed_text  = NULL;
                                 nb.shortcut      = 0;
                                 nb.user_pressed  = 0;
                                 nb.user_released = 0;
                                 nb.user_hold     = 0;
                                 nb.ID            = GSID_FXVOLUMENUMBER;
                                 nb.flags         = BT_CENTRE | BT_TEXT; // BT_BORDER;
                                   
                                 if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                  /*** CD - Lautstaerke ***/
                                  nb.modus         = BM_STRING;
                                  nb.x             = 0;
                                  nb.y             = 13 * (ywd->FontH + ReqDeltaY);
                                  nb.w             = (WORD)(0.3 * restbreite);
                                  nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_SGADGET_CDVOLUME,"CD VOLUME");
                                  nb.pressed_text  = NULL;
                                  nb.shortcut      = 0;
                                  nb.user_pressed  = 0;
                                  nb.user_released = 0;
                                  nb.user_hold     = 0;
                                  nb.ID            = 2;
                                  nb.flags         = BT_TEXT; // BT_BORDER;
                                  
                                  if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                   pi.min   = 1;
                                   pi.max   = 127;
                                   pi.value = 100;

                                   nb.pressed_font  = FONTID_MAPCUR_16;
                                   nb.unpressed_font= FONTID_MAPCUR_16;
                                   nb.disabled_font = FONTID_ENERGY;
                                   nb.modus         = BM_SLIDER;
                                   nb.x             = (WORD)(0.3  * restbreite + ReqDeltaX);
                                   nb.w             = (WORD)( 0.55 * restbreite );
                                   nb.unpressed_text= " ";
                                   nb.pressed_text  = NULL;
                                   nb.shortcut      = 0;
                                   nb.user_pressed  = GS_SOUND_HITCDVOLUME;
                                   nb.user_released = GS_SOUND_RELEASECDVOLUME;
                                   nb.user_hold     = GS_SOUND_HOLDCDVOLUME;
                                   nb.ID            = GSID_CDVOLUMESLIDER;
                                   nb.flags         = 0; // BT_BORDER;
                                   nb.specialinfo   = (LONG *) &pi;
                                   
                                   if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                    nb.pressed_font  = FONTID_MAPCUR_4;
                                    nb.unpressed_font= FONTID_MAPCUR_4;
                                    nb.disabled_font = FONTID_MAPCUR_4;
                                    nb.modus         = BM_STRING;
                                    nb.x             = (WORD)(0.85 * restbreite + 2 * ReqDeltaX);
                                    nb.w             = (WORD)(0.15 * restbreite);
                                    nb.unpressed_text= "4";
                                    nb.pressed_text  = NULL;
                                    nb.shortcut      = 0;
                                    nb.user_pressed  = 0;
                                    nb.user_released = 0;
                                    nb.user_hold     = 0;
                                    nb.ID            = GSID_CDVOLUMENUMBER;
                                    nb.flags         = BT_CENTRE | BT_TEXT; // BT_BORDER;
                                    
                                    if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                     restbreite = VReqWidth - 3 * ReqDeltaX;

                                     nb.pressed_font  = FONTID_MAPCUR_32;
                                     nb.unpressed_font= FONTID_MAPCUR_16;
                                     nb.disabled_font = FONTID_ENERGY;
                                     nb.modus         = BM_GADGET;
                                     nb.x             = ok_x;
                                     nb.y             = ok_y;
                                     nb.w             = ok_w;
                                     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_OK,"OK");
                                     nb.pressed_text  = NULL;
                                     nb.shortcut      = 0;
                                     nb.user_pressed  = 0;
                                     nb.user_released = GS_SETTINGSOK;
                                     nb.user_hold     = 0;
                                     nb.ID            = GSID_SETTINGSOK;
                                     nb.flags         = BT_CENTRE | BT_TEXT| BT_BORDER;
                                     nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
                                     nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
                                     nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);

                                     if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                      nb.x             = help_x;
                                      nb.y             = help_y;
                                      nb.w             = help_w;
                                      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_HELP,"HELP");
                                      nb.pressed_text  = NULL;
                                      nb.shortcut      = 0;
                                      nb.user_pressed  = 0;
                                      nb.user_released = GS_HELP;
                                      nb.user_hold     = 0;
                                      nb.ID            = GSID_SETTINGSHELP;

                                      if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                       nb.x             = cancel_x;
                                       nb.y             = cancel_y;
                                       nb.w             = cancel_w;
                                       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_CANCEL,"CANCEL");
                                       nb.pressed_text  = NULL;
                                       nb.shortcut      = 0;
                                       nb.user_pressed  = 0;
                                       nb.user_released = GS_SETTINGSCANCEL;
                                       nb.user_hold     = 0;
                                       nb.ID            = GSID_SETTINGSCANCEL;

                                       if( _methoda(GSR->bvideo, BTM_NEWBUTTON, &nb ) ) {

                                        button_ok = TRUE;
                                        }
                                       }
                                      }
                                     }
                                    }
                                   }
                                  }
                                 }
                                }
                               }
                              }
                             }
                            }
                           }
                          }
                         }
                        }
                       }
                      }
                     }
                    }
                   }
                  }
                 }
                }
               }
              }
             }
            }
           }
          }
         }
        }
       }
      }
     }


    if( !button_ok ) {

        _LogMsg("Unable to add video-button\n");
        return( FALSE );
        }

    /*** Soundflags schalten ***/
    sbs.who = GSID_SOUND_LR;
    if( GSR->sound_flags & SF_INVERTLR )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /** Button ausschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bvideo, BTM_SWITCHPUBLISH, &sp );
///

/// "Der EinAusgabeRequester"
    DListWidth = bwidth; 
    if (!yw_InitListView(ywd, &(GSR->dmenu),
                         LIST_Resize,       FALSE,
                         LIST_NumEntries,   yw_HowMuchNodes(
                                            (struct List *)(&(GSR->flist))),
                         LIST_ShownEntries, NUM_DISK_SHOWN,
                         LIST_FirstShown,   0,
                         LIST_Selected,     0,
                         LIST_MaxShown,     NUM_DISK_SHOWN,
                         LIST_DoIcon,       FALSE,
                         LIST_EntryHeight,  ywd->FontH,
                         LIST_EntryWidth,   DListWidth,
                         LIST_Enabled,      TRUE,
                         LIST_VBorder,      ywd->EdgeH,
                         LIST_ImmediateInput, FALSE,
                         LIST_KeyboardInput,TRUE,
                         TAG_DONE)) {

                         _LogMsg("Unable to create disk-listview\n");
                         return(FALSE);
                         }

    GSR->db_xoffset = bstart_x;
    GSR->db_yoffset = bstart_y;
    GSR->bdisk = _new("button.class",BTA_BoxX, GSR->db_xoffset,
                                     BTA_BoxY, GSR->db_yoffset,
                                     BTA_BoxW, ywd->DspXRes - GSR->db_xoffset,
                                     BTA_BoxH, ywd->DspYRes - GSR->db_yoffset,
                                     TAG_DONE );
    if( !GSR->bdisk ) {
    
        _LogMsg("Unable to create disk-buttonobject\n");
        return( FALSE );
        }

    /*** Menü anpassen ***/
    GSR->dmenu_xoffset = bstart_x;
    GSR->dmenu_yoffset = bstart_y + 4 * (ywd->FontH + ReqDeltaY);
    GSR->dmenu.Req.req_cbox.rect.x = GSR->dmenu_xoffset;
    GSR->dmenu.Req.req_cbox.rect.y = GSR->dmenu_yoffset;

    button_ok = FALSE;
    yw_LocStrCpy(GSR->D_Name, GSR->UserName );
    GSR->DCursorPos = strlen( GSR->D_Name );
    if( GSR->D_InputMode )
        sprintf( sys_string, "%s%s\0", GSR->D_Name, CURSORSIGN );
    else
        sprintf( sys_string, "%s\0", GSR->D_Name );

    nb.pressed_font  = FONTID_MAPCUR_4;
    nb.unpressed_font= FONTID_MAPCUR_4;
    nb.disabled_font = FONTID_MAPCUR_4;
    nb.modus         = BM_STRING;
    nb.x             = 0;
    nb.y             = 0;
    nb.w             = bwidth;
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_HEADLINE,"LOAD, CREATE OR DELETE PLAYER");
    nb.pressed_text  = NULL;
    nb.ID            = GSID_DISKHEADLINE;
    nb.flags         = BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
    
    if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

     nb.x             = 0;
     nb.y             = ywd->FontH + ReqDeltaY;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_HEADLINE2,"2");
     nb.pressed_text  = NULL;
     nb.ID            = GSID_DISKHEADLINE2;
     nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
     
     if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

      nb.x             = 0;
      nb.y             = 2 * (ywd->FontH + ReqDeltaY);
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_HEADLINE3,"3");
      nb.pressed_text  = NULL;
      nb.ID            = GSID_DISKHEADLINE3;
      
      if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

       nb.x             = 0;
       nb.y             = 3 * (ywd->FontH + ReqDeltaY);
       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_HEADLINE4,"4");
       nb.pressed_text  = NULL;
       nb.ID            = GSID_DISKHEADLINE4;
       
       if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

        nb.pressed_font  = FONTID_MAPCUR_8;
        nb.unpressed_font= FONTID_MAPCUR_8;
        nb.disabled_font = FONTID_MAPCUR_8;
        nb.modus         = BM_STRING;
        nb.x             = 0;
        nb.y             = (NUM_DISK_SHOWN + 4) * ywd->FontH + 6 * ReqDeltaY;
        nb.w             = bwidth;
        nb.unpressed_text= sys_string;
        nb.pressed_text  = NULL;
        nb.shortcut      = 0;
        nb.user_pressed  = 0;
        nb.user_released = 0;
        nb.user_hold     = 0;
        nb.ID            = GSID_DISKSTRING;
        nb.flags         = BT_BORDER | BT_CENTRE | BT_TEXT;
        
        if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

         restbreite = bwidth - 3 * ReqDeltaX;

         nb.pressed_font  = FONTID_MAPCUR_32;
         nb.unpressed_font= FONTID_MAPCUR_16;
         nb.disabled_font = FONTID_ENERGY;
         nb.modus         = BM_GADGET;
         nb.x             = (WORD)(0.25 * restbreite + ReqDeltaX);
         nb.y             = (NUM_DISK_SHOWN + 5) * ywd->FontH + 7 * ReqDeltaY;
         nb.w             = (WORD)(0.25 * restbreite);
         nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_LOAD,"LOAD");
         nb.pressed_text  = NULL;
         nb.shortcut      = 0;
         nb.user_pressed  = GS_BUTTONDOWN;
         nb.user_released = GS_LOAD;
         nb.user_hold     = 0;
         nb.ID            = GSID_LOAD;
         nb.flags         = BT_BORDER | BT_CENTRE | BT_TEXT;
         nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
         nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
         nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
         if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

          nb.x             = (WORD)(0.75 * restbreite + 3 * ReqDeltaX);
          nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_DELETE,"DELETE");
          nb.pressed_text  = NULL;
          nb.user_released = GS_DELETE;
          nb.ID            = GSID_DELETE;
          if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

           nb.x             = 0;
           nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_NEWGAME,"NEW GAME");
           nb.pressed_text  = NULL;
           nb.user_released = GS_NEWGAME;
           nb.ID            = GSID_NEWGAME;
           if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

            nb.x             = (WORD)(0.5 * restbreite + 2 * ReqDeltaX);
            nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_SAVE,"SAVE");
            nb.pressed_text  = NULL;
            nb.user_released = GS_SAVE;
            nb.ID            = GSID_SAVE;
            if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

             nb.x             = ok_x;
             nb.y             = ok_y;
             nb.w             = ok_w;
             nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_OK,"OK");
             nb.pressed_text  = NULL;
             nb.shortcut      = 0;
             nb.user_pressed  = GS_BUTTONDOWN;
             nb.user_released = GS_DISKOK;
             nb.user_hold     = 0;
             nb.ID            = GSID_DISKOK;
             if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

              nb.y             = help_y;
              nb.w             = help_w;
              nb.x             = help_x;
              nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_HELP,"HELP");
              nb.pressed_text  = NULL;
              nb.user_released = GS_HELP;
              nb.ID            = GSID_DISKHELP;
              if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

               nb.y             = cancel_y;
               nb.w             = cancel_w;
               nb.x             = cancel_x;
               nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_CANCEL,"CANCEL");
               nb.pressed_text  = NULL;
               nb.user_released = GS_DISKCANCEL;
               nb.ID            = GSID_DISKCANCEL;
               if( _methoda(GSR->bdisk, BTM_NEWBUTTON, &nb ) ) {

                button_ok = TRUE;
                }
               }
              }
             }
            }
           }
          }
         }
        }
       }
      }
     }

    if( !button_ok ) {

        _LogMsg("Unable to add button to disk-buttonobject\n");
        }


    /** Button ausschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );

    /*** OK und CANCEL erstmal ausschalten ***/
    sb.visible = 0;
    sb.number  = GSID_DISKOK;
    _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &sb );
///

/// "Der Spracheinsteller"
    if (!yw_InitListView(ywd, &(GSR->lmenu),
                         LIST_Resize,       FALSE,
                         LIST_NumEntries,   NUM_LOCALE_SHOWN,       // may vary
                         LIST_ShownEntries, NUM_LOCALE_SHOWN,       // may vary
                         LIST_FirstShown,   0,
                         LIST_Selected,     0,
                         LIST_MaxShown,     NUM_LOCALE_SHOWN,
                         LIST_DoIcon,       FALSE,
                         LIST_EntryHeight,  ywd->FontH,
                         LIST_EntryWidth,   (LONG)(bwidth - ywd->PropW),
                         LIST_Enabled,      TRUE,
                         LIST_VBorder,      ywd->EdgeH,
                         LIST_ImmediateInput, FALSE,
                         LIST_KeyboardInput,TRUE,
                         TAG_DONE)) {

                         _LogMsg("Unable to create local-listview\n");
                         return(FALSE);
                         }

    GSR->lb_xoffset = bstart_x;
    GSR->lb_yoffset = bstart_y;
    GSR->blocale = _new("button.class",BTA_BoxX, GSR->lb_xoffset,
                                       BTA_BoxY, GSR->lb_yoffset,
                                       BTA_BoxW, ywd->DspXRes - GSR->lb_xoffset,
                                       BTA_BoxH, ywd->DspYRes - GSR->lb_yoffset,
                                       TAG_DONE );
    if( !GSR->blocale ) {
    
        _LogMsg("Unable to create locale-buttonobject\n");
        return( FALSE );
        }

    GSR->lmenu.Req.req_cbox.rect.x = bstart_x;
    GSR->lmenu.Req.req_cbox.rect.y = bstart_y + 4 * (ywd->FontH + ReqDeltaY);

    button_ok = FALSE;

    nb.pressed_font  = FONTID_MAPCUR_4;
    nb.unpressed_font= FONTID_MAPCUR_4;
    nb.disabled_font = FONTID_ENERGY;
    nb.modus         = BM_STRING;
    nb.x             = 0;
    nb.y             = 0;
    nb.w             = bwidth;
    nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_LGADGET_HEADLINE,"SELECT NEW LANGUAGE");
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = 0;
    nb.user_released = 0;
    nb.user_hold     = 0;
    nb.ID            = GSID_LOCALEHEADLINE;
    nb.flags         = BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);

    if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

     nb.x             = 0;
     nb.y             = ywd->FontH + ReqDeltaY;
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_LGADGET_HEADLINE2,"2");
     nb.pressed_text  = NULL;
     nb.ID            = GSID_LOCALEHEADLINE2;
     nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
     nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);

     if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

      nb.x             = 0;
      nb.y             = 2 * (ywd->FontH + ReqDeltaY);
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_LGADGET_HEADLINE3,"3");
      nb.pressed_text  = NULL;
      nb.ID            = GSID_LOCALEHEADLINE3;

      if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

       nb.x             = 0;
       nb.y             = 3 * (ywd->FontH + ReqDeltaY);
       nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_LGADGET_HEADLINE4,"4");
       nb.pressed_text  = NULL;
       nb.ID            = GSID_LOCALEHEADLINE4;

       if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

        nb.pressed_font  = FONTID_MAPCUR_32;
        nb.unpressed_font= FONTID_MAPCUR_16;
        nb.disabled_font = FONTID_ENERGY;
        nb.modus         = BM_GADGET;
        nb.x             = ok_x - (GSR->lb_xoffset - bstart_x);
        nb.y             = ok_y - (GSR->lb_yoffset - bstart_y);
        nb.w             = ok_w;
        nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_OK,"OK");
        nb.pressed_text  = NULL;
        nb.shortcut      = 0;
        nb.user_pressed  = 0;
        nb.user_released = GS_LOCALEOK;
        nb.user_hold     = 0;
        nb.ID            = GSID_LOCALEOK;
        nb.flags         = BT_CENTRE | BT_TEXT | BT_BORDER;
        nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
        nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
        nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);

        if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

         nb.x             = help_x - (GSR->lb_xoffset - bstart_x);
         nb.y             = help_y - (GSR->lb_yoffset - bstart_y);
         nb.w             = help_w;
         nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_HELP,"HELP");
         nb.pressed_text  = NULL;
         nb.shortcut      = 0;
         nb.user_pressed  = 0;
         nb.user_released = GS_HELP;
         nb.user_hold     = 0;
         nb.ID            = GSID_LOCALEHELP;

         if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

          nb.x             = cancel_x - (GSR->lb_xoffset - bstart_x);
          nb.y             = cancel_y - (GSR->lb_yoffset - bstart_y);
          nb.w             = cancel_w;
          nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_CANCEL,"CANCEL");
          nb.pressed_text  = NULL;
          nb.shortcut      = 0;
          nb.user_pressed  = 0;
          nb.user_released = GS_LOCALECANCEL;
          nb.user_hold     = 0;
          nb.ID            = GSID_LOCALECANCEL;

          if( _methoda(GSR->blocale, BTM_NEWBUTTON, &nb ) ) {

           button_ok = TRUE;
           }
          }
         }
        }
       }
      }
     }

    if( !button_ok ) {

        _LogMsg("Unable to add locale-button\n");
        return( FALSE );
        }

    button_ok = FALSE;

    /** Button ausschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->blocale, BTM_SWITCHPUBLISH, &sp );
///

/// "Der AboutRequester"

    GSR->ab_xoffset = 0;
    GSR->ab_yoffset = bstart_y;
    GSR->babout = _new("button.class",BTA_BoxX, GSR->ab_xoffset,
                                      BTA_BoxY, GSR->ab_yoffset,
                                      BTA_BoxW, ywd->DspXRes - GSR->ab_xoffset,
                                      BTA_BoxH, ywd->DspYRes - GSR->ab_yoffset,
                                      TAG_DONE );
    if( !GSR->babout ) {
    
        _LogMsg("Unable to create sound-buttonobject\n");
        return( FALSE );
        }

    /*** Die button zum Sound ***/
    button_ok = FALSE;

    restbreite = AReqWidth - 4 * ReqDeltaX;

    nb.pressed_font  = FONTID_MAPCUR_4;
    nb.unpressed_font= FONTID_MAPCUR_4;
    nb.disabled_font = FONTID_MAPCUR_4;
    nb.modus         = BM_STRING;
    nb.x             = 0;    
    nb.y             = (ywd->FontH + ReqDeltaY);
    nb.w             = restbreite;
    nb.unpressed_text= "zusammengeschraubt von:";
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = 0;
    nb.user_released = 0;
    nb.user_hold     = 0;
    nb.ID            = 2;
    nb.flags         = BT_CENTRE | BT_TEXT; // BT_BORDER;
    
    if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

     nb.y             = 2 * (ywd->FontH + ReqDeltaY);
     nb.unpressed_text= "        Bernd Beyreuther, Andreas Flemming, Stefan Karau,";
     
     if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

      nb.y             = 3 * (ywd->FontH + ReqDeltaY);
      nb.unpressed_text= "        Sylvius Lack, Steffen Priebus, Henrik Volkening,";
      
      if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

       nb.y             = 4 * (ywd->FontH + ReqDeltaY);
       nb.unpressed_text= "        Stefan Warias und Andre Weissflog";
       
       if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

        nb.y             = 5 * (ywd->FontH + ReqDeltaY);
        nb.unpressed_text= "unter Mithilfe von: ";
        
        if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

         nb.y             = 6 * (ywd->FontH + ReqDeltaY);
         nb.unpressed_text= "        D.Koebelin, N.Nitsch U.Kapp und A.Blechschmidt";
         
         if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

          nb.y             = 7 * (ywd->FontH + ReqDeltaY);
          nb.unpressed_text= "und den Werkzeugen:";
          
          if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

           nb.y             = 8 * (ywd->FontH + ReqDeltaY);
           nb.unpressed_text= "        GoldEd - dPaint - SAS/C";
           
           if( _methoda(GSR->babout, BTM_NEWBUTTON, &nb ) ) {

            button_ok = TRUE;
            }
           }
          }
         }
        }
       }
      }
     }

    if( !button_ok ) {

        _LogMsg("Unable to add about-button\n");
        return( FALSE);
        }


    /** Button  ausschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->babout, BTM_SWITCHPUBLISH, &sp );
///

#ifdef __NETWORK__
/// "Der Netzwerkrequester"
    NListWidth = bwidth - ywd->PropW;
    
    if (!yw_InitListView(ywd, &(GSR->nmenu),
                         LIST_Resize,       FALSE,
                         LIST_NumEntries,   NUM_NET_SHOWN,       // may vary
                         LIST_ShownEntries, NUM_NET_SHOWN,       // may vary
                         LIST_FirstShown,   0,
                         LIST_Selected,     0,
                         LIST_MaxShown,     NUM_NET_SHOWN,
                         LIST_DoIcon,       FALSE,
                         LIST_EntryHeight,  ywd->FontH,
                         LIST_EntryWidth,   NListWidth,
                         LIST_Enabled,      TRUE,
                         LIST_VBorder,      ywd->EdgeH,
                         LIST_ImmediateInput, FALSE,
                         LIST_KeyboardInput,TRUE,
                         TAG_DONE)) {

                         _LogMsg("Unable to create network-listview\n");
                         return(FALSE);
                         }

    GSR->nb_xoffset = bstart_x;
    GSR->nb_yoffset = bstart_y - ywd->FontH;    // weil wenig Platz ist...
    GSR->bnet = _new("button.class",BTA_BoxX, GSR->nb_xoffset,
                                    BTA_BoxY, GSR->nb_yoffset,
                                    BTA_BoxW, ywd->DspXRes - GSR->nb_xoffset,
                                    BTA_BoxH, ywd->DspYRes - GSR->nb_yoffset,
                                    TAG_DONE );
    if( !GSR->bnet ) {
    
        _LogMsg("Unable to create network-buttonobject\n");
        return( FALSE );
        }

    /*** Menü anpassen ***/
    GSR->nmenu_xoffset = GSR->nb_xoffset;
    GSR->nmenu_yoffset = GSR->nb_yoffset + (WORD)(3 * (ywd->FontH + ReqDeltaY));
    GSR->nmenu.Req.req_cbox.rect.x = GSR->nmenu_xoffset;
    GSR->nmenu.Req.req_cbox.rect.y = GSR->nmenu_yoffset;

    button_ok = FALSE;
    restbreite = NListWidth - 3 * ReqDeltaX; // an unteren Buttons orientieren
    
    nb.pressed_font  = FONTID_MAPCUR_8;
    nb.unpressed_font= FONTID_MAPCUR_8;
    nb.disabled_font = FONTID_MAPCUR_8;
    nb.modus         = BM_STRING;
    nb.x             = 0; //(WORD)(0.05 * NListWidth);
    nb.y             = (WORD)((NUM_NET_SHOWN + 2) * (ywd->FontH + ReqDeltaY));
    nb.w             = (WORD)(0.8 * NListWidth);     // Ändern
    nb.unpressed_text= "???";
    nb.pressed_text  = NULL;
    nb.shortcut      = 0;
    nb.user_pressed  = 0;
    nb.user_released = 0;
    nb.user_hold     = 0;
    nb.ID            = GSID_NETSTRING;
    nb.flags         = BT_TEXT;
    nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
    nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
    nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);

    if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {
        
     nb.pressed_font  = FONTID_MAPCUR_32;
     nb.unpressed_font= FONTID_MAPCUR_16;
     nb.disabled_font = FONTID_ENERGY;
     nb.modus         = BM_GADGET;
     nb.x             = (WORD)(0.8 * NListWidth) + ReqDeltaX;
     nb.w             = (WORD)(0.2 * NListWidth) - ReqDeltaX;  
     nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_SEND,"SEND");
     nb.pressed_text  = NULL;
     nb.user_released = GS_NETSEND;
     nb.user_hold     = 0;
     nb.ID            = GSID_NETSEND;
     nb.flags         = BT_CENTRE | BT_TEXT | BT_BORDER;
     nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
     nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
     nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
     
     if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

      /*** Gbr ist sozusagen maximale Gadgetbreite ***/
      WORD gbr = (WORD)(0.25 * (0.25 * restbreite - 3 * ReqDeltaX));
      struct VFMFont *fnt = _GetFont( FONTID_GADGET );

      nb.pressed_font  = FONTID_MAPCUR_4;
      nb.unpressed_font= FONTID_MAPCUR_4;
      nb.disabled_font = FONTID_MAPCUR_4;
      nb.modus         = BM_STRING;
      nb.y             = (WORD)((NUM_NET_SHOWN + 3) * (ywd->FontH + ReqDeltaY));
      nb.x             = 0; // (WORD)(0.1 * NListWidth);
      nb.w             = (WORD)(0.4 * NListWidth - 2 * ReqDeltaX);
      nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_SELRACE,"SELECT YOUR RACE");
      nb.pressed_text  = NULL;
      nb.user_pressed  = 0;
      nb.ID            = GSID_SELRACE;
      nb.flags         = BT_TEXT | BT_RIGHTALIGN;
      nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
      nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
      nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
    
      if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

       nb.pressed_font  = FONTID_GADGET;
       nb.unpressed_font= FONTID_GADGET;
       nb.disabled_font = FONTID_ENERGY;
       nb.modus         = BM_MXBUTTON;
       nb.x            += (nb.w + 2 * ReqDeltaX);
       nb.w             = fnt->fchars[ 'A' ].width;
       nb.unpressed_text= "A";
       nb.pressed_text  = "B";
       nb.user_pressed  = GS_USERRACE;
       nb.ID            = GSID_USERRACE;
       nb.flags         = 0;
     
       if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

        nb.x            += (gbr + ReqDeltaX);
        nb.unpressed_text= "C";
        nb.pressed_text  = "D";
        nb.user_pressed  = GS_KYTERNESER;
        nb.ID            = GSID_KYTERNESER;
     
        if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

         nb.x            += (gbr + ReqDeltaX);
         nb.unpressed_text= "E";
         nb.pressed_text  = "F";
         nb.user_pressed  = GS_MYKONIER;
         nb.ID            = GSID_MYKONIER;
     
         if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

          nb.x            += (gbr + ReqDeltaX);
          nb.unpressed_text= "G";
          nb.pressed_text  = "H";
          nb.user_pressed  = GS_TAERKASTEN;
          nb.ID            = GSID_TAERKASTEN;
     
          if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {
              
           nb.pressed_font  = FONTID_MAPCUR_32;
           nb.unpressed_font= FONTID_MAPCUR_16;
           nb.disabled_font = FONTID_ENERGY;
           nb.modus         = BM_GADGET;
           nb.x            += (gbr + 2 * ReqDeltaX);
           nb.w             = (WORD)(NListWidth) - nb.x;
           nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_BACK,"BACK");
           nb.pressed_text  = NULL;
           nb.user_pressed  = GS_BUTTONDOWN;
           nb.user_released = GS_NETBACK;
           nb.user_hold     = 0;
           nb.ID            = GSID_NETBACK;
           nb.flags         = BT_CENTRE|BT_BORDER|BT_TEXT;
           nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
           nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
           nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
        
           if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {
           
            nb.pressed_font  = FONTID_MAPCUR_4;
            nb.unpressed_font= FONTID_MAPCUR_4;
            nb.disabled_font = FONTID_MAPCUR_4;
            nb.modus         = BM_STRING;
            nb.x             = 0;    
            nb.y             = 0;
            nb.w             = NListWidth;
            nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPROVIDER,"SELECT PROVIDER");
            nb.pressed_text  = NULL;
            nb.user_pressed  = 0;
            nb.user_released = 0;
            nb.user_hold     = 0;
            nb.ID            = GSID_NETHEADLINE;
            nb.flags         = BT_TEXT;
         
            if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

             nb.x             = 0;    
             nb.y             = ywd->FontH + ReqDeltaY;
             nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPROVIDER2,"2");
             nb.pressed_text  = NULL;
             nb.ID            = GSID_NETHEADLINE2;
             nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
             nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
             nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
           
             if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

              nb.x             = 0;    
              nb.y             = 2 * (ywd->FontH + ReqDeltaY);
              nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPROVIDER3,"3");
              nb.pressed_text  = NULL;
              nb.ID            = GSID_NETHEADLINE3;
           
              if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

               nb.pressed_font  = FONTID_MAPCUR_32;
               nb.unpressed_font= FONTID_MAPCUR_16;
               nb.disabled_font = FONTID_ENERGY;
               nb.modus         = BM_GADGET;
               nb.x             = (WORD)(0.3 * NListWidth);
               nb.y             = (WORD)((NUM_NET_SHOWN + 3.2) * (ywd->FontH + ReqDeltaY));
               nb.w             = (WORD)(0.4 * NListWidth);
               nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_NEW,"NEW");
               nb.pressed_text  = NULL;
               nb.user_pressed  = GS_BUTTONDOWN;
               nb.user_released = GS_NETNEW;
               nb.user_hold     = 0;
               nb.ID            = GSID_NETNEW;
               nb.flags         = BT_CENTRE|BT_BORDER|BT_TEXT;
               nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
               nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
               nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
                          
               if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                nb.x             = ok_x;
                nb.y             = ok_y + ywd->FontH;
                nb.w             = ok_w;
                nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_OK,"NEXT");
                nb.pressed_text  = NULL;
                nb.user_released = GS_NETOK;
                nb.user_hold     = 0;
                nb.ID            = GSID_NETOK;
              
                if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                 nb.x             = help_x;
                 nb.y             = help_y + ywd->FontH;
                 nb.w             = help_w;
                 nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_HELP,"HELP");
                 nb.pressed_text  = NULL;
                 nb.user_released = GS_HELP;
                 nb.user_hold     = 0;
                 nb.ID            = GSID_NETHELP;
               
                 if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                  nb.x             = cancel_x;
                  nb.y             = cancel_y + ywd->FontH;
                  nb.w             = cancel_w;
                  nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_CANCEL,"CANCEL");
                  nb.pressed_text  = NULL;
                  nb.user_released = GS_NETCANCEL;
                  nb.user_hold     = 0;
                  nb.ID            = GSID_NETCANCEL;
                
                  if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                   WORD breite;

                   if( ywd->DspXRes >= 512 )
                       breite = 4 * checkmarkwidth;
                   else
                       breite = 6 * checkmarkwidth;

                   nb.pressed_font  = FONTID_MAPCUR_4;
                   nb.unpressed_font= FONTID_MAPCUR_4;
                   nb.disabled_font = FONTID_MAPCUR_4;
                   nb.modus         = BM_STRING;
                   nb.x             = breite + checkmarkwidth;
                   nb.y             = 4 * (ywd->FontH + ReqDeltaY);
                   nb.w             = NListWidth - breite - checkmarkwidth;
                   nb.unpressed_text= " ";
                   nb.pressed_text  = NULL;
                   nb.user_pressed  = 0;
                   nb.user_released = 0;
                   nb.user_hold     = 0;
                   nb.ID            = GSID_PLAYERNAME1;
                   nb.flags         = BT_TEXT;
                   nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
                   nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
                   nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);

                   if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                    nb.y             = 5 * (ywd->FontH + ReqDeltaY);
                    nb.ID            = GSID_PLAYERNAME2;

                    if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                     nb.y             = 6 * (ywd->FontH + ReqDeltaY);
                     nb.ID            = GSID_PLAYERNAME3;

                     if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                      nb.y             = 7 * (ywd->FontH + ReqDeltaY);
                      nb.ID            = GSID_PLAYERNAME4;

                      if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                       nb.pressed_font  = FONTID_GADGET;
                       nb.unpressed_font= FONTID_GADGET;
                       nb.disabled_font = FONTID_GADGET;
                       nb.modus         = BM_STRING;
                       nb.x             = 0;
                       nb.y             = 4 * (ywd->FontH + ReqDeltaY);
                       nb.w             = breite;
                       nb.unpressed_text= " ";
                       nb.pressed_text  = NULL;
                       nb.user_pressed  = 0;
                       nb.user_released = 0;
                       nb.user_hold     = 0;
                       nb.ID            = GSID_PLAYERSTATUS1;
                       nb.flags         = 0;
                       nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
                       nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
                       nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);

                       if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                        nb.y             = 5 * (ywd->FontH + ReqDeltaY);
                        nb.ID            = GSID_PLAYERSTATUS2;
 
                        if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                         nb.y             = 6 * (ywd->FontH + ReqDeltaY);
                         nb.ID            = GSID_PLAYERSTATUS3;

                         if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                          nb.y             = 7 * (ywd->FontH + ReqDeltaY);
                          nb.ID            = GSID_PLAYERSTATUS4;

                          if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                           /*** Ready-Checkmark ***/
                           nb.pressed_font  = FONTID_MAPCUR_32;
                           nb.unpressed_font= FONTID_MAPCUR_16;
                           nb.disabled_font = FONTID_ENERGY;
                           nb.modus         = BM_SWITCH;
                           nb.x             = ok_x;              //(WORD)(bwidth/3);
                           nb.y             = ok_y + ywd->FontH; //(WORD)((NUM_NET_SHOWN + 3.2 ) * (ywd->FontH + ReqDeltaY));
                           nb.w             = checkmarkwidth;
                           nb.unpressed_text= "g";
                           nb.pressed_text  = "g";
                           nb.shortcut      = 0;
                           nb.user_pressed  = GS_NETREADY;
                           nb.user_released = GS_NETSTOP;
                           nb.user_hold     = 0;
                           nb.ID            = GSID_NETREADY;
                           nb.flags         = 0;    // kein Rand, nicht zentriert
 
                           if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                            nb.pressed_font  = FONTID_MAPCUR_4;
                            nb.unpressed_font= FONTID_MAPCUR_4;
                            nb.disabled_font = FONTID_MAPCUR_4;
                            nb.modus         = BM_STRING;
                            nb.x            += (ReqDeltaX + checkmarkwidth);
                            nb.w             = ok_w - checkmarkwidth - ReqDeltaX;
                            nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_NETREADY,"READY");
                            nb.pressed_text  = NULL;
                            nb.shortcut      = 0;
                            nb.user_pressed  = 0;
                            nb.user_released = 0;
                            nb.user_hold     = 0;
                            nb.ID            = GSID_NETREADYTEXT;
                            nb.flags         = BT_TEXT;

                            if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                             nb.pressed_font  = FONTID_MAPCUR_4;
                             nb.unpressed_font= FONTID_MAPCUR_4;
                             nb.disabled_font = FONTID_MAPCUR_4;
                             nb.modus         = BM_STRING;
                             nb.x             = 0;    
                             nb.y             = 3 * (ywd->FontH + ReqDeltaY);
                             nb.w             = (WORD)(0.3 * NListWidth);
                             nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_LEVELTEXT,"YOU PLAY");
                             nb.pressed_text  = NULL;
                             nb.user_pressed  = 0;
                             nb.user_released = 0;
                             nb.user_hold     = 0;
                             nb.ID            = GSID_LEVELTEXT;
                             nb.flags         = BT_TEXT;
                             nb.red           = yw_Red(ywd,YPACOLOR_TEXT_BUTTON);
                             nb.green         = yw_Green(ywd,YPACOLOR_TEXT_BUTTON);
                             nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_BUTTON);
                          
                             if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {
    
                              nb.modus         = BM_STRING;
                              nb.x             = (WORD)(0.3 * NListWidth);    
                              nb.w             = (WORD)(0.7 * NListWidth);
                              nb.unpressed_text= "...";
                              nb.ID            = GSID_LEVELNAME;
                              nb.flags         = BT_TEXT;
                         
                              if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {

                               nb.pressed_font  = FONTID_MAPCUR_4;
                               nb.unpressed_font= FONTID_MAPCUR_4;
                               nb.disabled_font = FONTID_MAPCUR_4;
                               nb.modus         = BM_STRING;
                               nb.x             = 0;    
                               nb.y             = (WORD)((NUM_NET_SHOWN + 2) * (ywd->FontH + ReqDeltaY));
                               nb.w             = (WORD)(NListWidth);
                               nb.unpressed_text= ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_REFRESHSESSIONS,"PRESS SPACEBAR TO UPDATE SESSION LIST");
                               nb.pressed_text  = NULL;
                               nb.user_pressed  = 0;
                               nb.user_released = 0;
                               nb.user_hold     = 0;
                               nb.ID            = GSID_REFRESHSESSIONS;
                               nb.flags         = BT_TEXT;
                               nb.red           = yw_Red(ywd,YPACOLOR_TEXT_DEFAULT);
                               nb.green         = yw_Green(ywd,YPACOLOR_TEXT_DEFAULT);
                               nb.blue          = yw_Blue(ywd,YPACOLOR_TEXT_DEFAULT);
                              
                               if( _methoda(GSR->bnet, BTM_NEWBUTTON, &nb ) ) {
                               
                                button_ok = TRUE;
                                }
                               }
                              }
                             }
                            }
                           }
                          }
                         }
                        }
                       }
                      }
                     }
                    }
                   }
                  }
                 }
                }
               }
              }
             }
            }
           }
          }
         }
        }
       }
      }
     }

    if( !button_ok ) {

        _LogMsg("Unable to add network-button\n");
        return( FALSE );
        }

    sb.visible = 0;
    sb.number  = GSID_PLAYERNAME1;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERNAME2;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERNAME3;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERNAME4;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERSTATUS1;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERSTATUS2;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERSTATUS3;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_PLAYERSTATUS4;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &sb );

    /** Button ausschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bnet, BTM_SWITCHPUBLISH, &sp );
///
#endif


    /*** Balken "÷ffnen" ***/
    switch( GSR->shell_mode ) {

        case SHELLMODE_PLAY:
        case SHELLMODE_TUTORIAL:
            sp.modus = SP_PUBLISH;
            _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );

            /*** Nach den Spiel Debriefing einstellungen ***/
            if( GSR->aftergame ) {

                struct switchbutton_msg swb;

                swb.visible = 0;
                swb.number  = GSID_PL_GAME;
                _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
                swb.number = GSID_PL_FASTFORWARD;
                _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
                }
            break;

        case SHELLMODE_NETWORK:

            sp.modus = SP_PUBLISH;
            _methoda( GSR->bnet, BTM_SWITCHPUBLISH, &sp );
            break;

        default:
            sp.modus = SP_PUBLISH;
            _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
            break;
        }

    /*** Aktualisieren der OberflSche ***/
    _methoda( o, YWM_REFRESHGAMESHELL, GSR );

    /*** Welcome-Sample anschieben ***/
    _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_WELCOME );

    GSR->ShellOpen  = TRUE;
    GSR->JustOpened = TRUE;

    /* -------------------------------------------------------------------
    ** Achtung! Cleanup... löscht alle Einstellungen, die Checkremotestart
    ** evtl. schon ausgefüllt hat. Deshalb verlassen wir uns darauf, daß
    ** ein remotestart ein 2. Mal aus der Shell nicht möglich ist. Somit
    ** können wir in Cleanup... remotestart zurücksetzen. Weil beim Öffnen
    ** und Schließen der Shell ebenfalls die Sachen zurückgesetzt werden,
    ** müssen wir im Remotestart die Sachen verbieten
    ** S C H E I S S - L O B B Y (paßt eben nicht hierein)
    ** -----------------------------------------------------------------*/
    if( GSR->remotestart ) {

        /* -------------------------------------------------------------
        ** Wenn wir Im Fernstart bis hierher kommen, dann öffnen wir den
        ** Netzrequester in dem ausgewählten Modus.
        ** -----------------------------------------------------------*/
        yw_OpenNetwork( GSR );
        GSR->ywd->playing_network = TRUE;

        GSR->FreeRaces = (FREERACE_KYTERNESER | FREERACE_MYKONIER |
                          FREERACE_TAERKASTEN);
        GSR->SelRace   = FREERACE_USER;     // pauschal...
        }
    else {

        yw_CleanupNetworkData( GSR->ywd );
        GSR->n_selmode = NM_PROVIDER;
        }
    GSR->NSel      = -1;


    /*** Ein Lied! ***/
    if( GSR->ywd->Prefs.Flags & YPA_PREFS_CDSOUNDENABLE ) {

        if( GSR->aftergame ) {

            cd.command   = SND_CD_SETTITLE;
            cd.min_delay = ywd->gsr->debriefing_min_delay;
            cd.max_delay = ywd->gsr->debriefing_max_delay;
            cd.para      = GSR->debriefingtrack;
            }
        else {

            cd.command   = SND_CD_SETTITLE;
            cd.min_delay = ywd->gsr->shell_min_delay;
            cd.max_delay = ywd->gsr->shell_max_delay;
            cd.para      = GSR->shelltrack;
            }

        _ControlCDPlayer( &cd );
        cd.command = SND_CD_PLAY;
        _ControlCDPlayer( &cd );
        }

    return( TRUE );
}


_dispatcher( void, yw_YWM_CLOSEGAMESHELL, struct GameShellReq *GSR )
{
/*
**  FUNCTION    Es werden alle Requester der GameShell geschlossen.
**
**  CHANGED     15-Apr-96   af created
**              29-Mai-96   af mir vergSssn mol net, de listen zuzumachen
**                          un abzemelden....
*/
    struct switchpublish_msg    sp;
    struct ypaworld_data *ywd = INST_DATA( cl, o);

    /*** Schlie_en nur, wenn Shell offen war ***/
    if( !GSR->ShellOpen ) return;

/// "Schließen und freigeben aller Requester- und Buttonobjekte"
    /*** Balken schlie_en und Objekt freigeben ***/
    if( GSR->confirm ) {
    
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->confirm, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->confirm );
        }

    GSR->confirm = NULL;

    if( GSR->UBalken ) {
    
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->UBalken );
        }

    GSR->UBalken = NULL;

    if( GSR->Titel ) {
    
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->Titel );
        }

    GSR->Titel = NULL;

    if( GSR->binput ) {

        if( !(GSR->imenu.Req.flags & REQF_Closed) )
            yw_CloseReq(GSR->ywd, &(GSR->imenu.Req) );
        yw_KillListView( ywd, &(GSR->imenu) );
        
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->binput, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->binput );
        GSR->binput = NULL;
        }
    
    if( GSR->bvideo ) {

        if( !(GSR->vmenu.Req.flags & REQF_Closed) )
            yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req) );
        yw_KillListView( ywd, &(GSR->vmenu) );
        if( !(GSR->d3dmenu.Req.flags & REQF_Closed) )
            yw_CloseReq(GSR->ywd, &(GSR->d3dmenu.Req) );
        yw_KillListView( ywd, &(GSR->d3dmenu) );

        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->bvideo, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->bvideo );
        GSR->bvideo = NULL;
        }
    
    if( GSR->bdisk ) {

        if( !(GSR->dmenu.Req.flags & REQF_Closed) )
            yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );
        yw_KillListView( ywd, &(GSR->dmenu) );
        
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->bdisk );
        GSR->bdisk = NULL;
        }
    
    if( GSR->blocale ) {

        if( !(GSR->lmenu.Req.flags & REQF_Closed) )
            yw_CloseReq(GSR->ywd, &(GSR->lmenu.Req) );
        yw_KillListView( ywd, &(GSR->lmenu) );
        
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->blocale, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->blocale );
        GSR->blocale = NULL;
        }
    
    if( GSR->babout ) {
        
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->babout, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->babout );
        GSR->babout = NULL;
        }
    
    #ifdef __NETWORK__
    if( GSR->bnet ) {

        if( !(GSR->nmenu.Req.flags & REQF_Closed) )
            yw_CloseReq(GSR->ywd, &(GSR->nmenu.Req) );
        yw_KillListView( ywd, &(GSR->nmenu) );
        
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->bnet, BTM_SWITCHPUBLISH, &sp );
        _dispose( GSR->bnet );
        GSR->bnet = NULL;
        }
    #endif
///
    
    /*** WAS noch alles? ***/

    _FlushAudio();

    /*** FIXME_FLOH: Level-Select-Zeugs killen ***/
    yw_KillShellBgHandling(ywd);

    /*** Set freigeben ***/
    yw_KillSet( ywd );

    /*** Ausklingen lassen ***/
    yw_FadeOut( ywd );

    GSR->ShellOpen = FALSE;
}


_dispatcher( void, yw_YWM_FREEGAMESHELL, struct GameShellReq *GSR )
{
/*
**  FUNCTION    Gibt die GameShell frei, hei_t hier, da_ sie abgemeldet
**              wird, also nach QUIT.
**              Disposed auch alle Objekte
**
**  CHANGED     15-Apr-96   8100 000C created
*/
    struct ypaworld_data *ywd;
    struct Node *node;
    int    i;
    
    ywd = INST_DATA( cl, o);

    yw_CleanupNetworkData( GSR->ywd );

/// "Listen aufräumen"
    /*** Liste mit Filenodes freigeben ***/
    while( node = _RemHead( (struct List *) &(GSR->flist) ) )
        _FreeVec( (APTR) node );

    /*** Liste mit Videonodes freigeben ***/
    while( node = _RemHead( (struct List *) &(GSR->videolist) ) )
        _FreeVec( (APTR) node );

    /*** Liste mit Localenodes freigeben ***/
    while( node = _RemHead( (struct List *) &(GSR->localelist) ) )
        _FreeVec( (APTR) node );

///

/// "Soundsachen aufräumen"
    _FlushAudio();

    /*** Wech mit de Samples ***/
    for( i = 0; i < MAX_SOUNDSOURCES; i++ ) {

        if( GSR->so1[ i ] ) {
            _dispose( GSR->so1[ i ] );  GSR->so1[ i ] = NULL; }

        if( GSR->so2[ i ] ) {
            _dispose( GSR->so2[ i ] );  GSR->so2[ i ] = NULL; }
        }

    if( GSR->ChatObject ) {
        _dispose( GSR->ChatObject );  GSR->ChatObject = NULL; }

    _KillSoundCarrier( &(GSR->ShellSound1) );
    _KillSoundCarrier( &(GSR->ShellSound2) );
    _KillSoundCarrier( &(GSR->ChatSound) );
///
}


_dispatcher( void, yw_YWM_REFRESHGAMESHELL, struct GameShellReq *GSR )
{
/*
**  FUNCTION    Nach dem _ffnen und dem Laden neuer Infos, ist es notwendig,
**              die OberflSche an die neuen Daten anzupassen. Mu_ die Gameshell
**              geschlossen werden, so wird dies explizit von den bearbeitenden
**              Routinen gemacht.
**
**  INPUT       die GSR, aus der die daten gezogen werden
**
**  CHANGED     gewittert das nun oder nicht ?!
*/

    struct setbuttonstate_msg sbs;
    struct setstring_msg ss;
    struct selectbutton_msg sb;
    struct bt_propinfo *prop;
    struct video_node *vn;

    /* ------------------------------------------------
    ** Nun werden die Daten zu den Gadgets aktualisiert 
    ** ----------------------------------------------*/

/// "Die IO-Gadgets"
    
    if( GSR->d_actualitem ) {

        struct switchbutton_msg sb;

        sb.visible = 1L;
        yw_PosSelected( &(GSR->dmenu), GSR->d_actualitem - 1 );

        sb.number = GSID_LOAD;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &sb );
        sb.number = GSID_DELETE;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &sb );
        sb.number = GSID_NEWGAME;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &sb );
        }
    else {

        struct switchbutton_msg sb;

        sb.visible = 1L;
        sb.number = GSID_LOAD;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &sb );
        sb.number = GSID_DELETE;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &sb );
        sb.number = GSID_NEWGAME;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &sb );
        }               
///

/// "Die Settings-Gadgets"
    if( GSR->sound_flags & SF_INVERTLR )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_SOUND_LR;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
    
    if( GSR->sound_flags & VF_16BITTEXTURE )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_16BITTEXTURE;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
    
    if( GSR->sound_flags & SF_CDSOUND )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_CDSOUND;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
    
    sb.number = GSID_FXVOLUMESLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    prop->value = GSR->fxvolume;
    _methoda( GSR->bvideo, BTM_REFRESH, &sb );
    
    sb.number = GSID_CDVOLUMESLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    prop->value = GSR->cdvolume;
    _methoda( GSR->bvideo, BTM_REFRESH, &sb );

    if( GSR->enemyindicator )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_ENEMYINDICATOR;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    if( GSR->video_flags & VF_FARVIEW )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_FARVIEW;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    if( GSR->video_flags & VF_HEAVEN )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_HEAVEN;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    if( GSR->video_flags & VF_SOFTMOUSE )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_SOFTMOUSE;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    if( GSR->video_flags & VF_DRAWPRIMITIVE )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_DRAWPRIMITIVE;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    vn = (struct video_node *) GSR->videolist.mlh_Head;
    while( vn->node.mln_Succ ) {
        if( vn->modus == GSR->ywd->GameRes )
            break;
        vn = (struct video_node *) vn->node.mln_Succ;
        }

    ss.pressed_text = ss.unpressed_text = vn->name;
    ss.number = GSID_VMENU_BUTTON;
    _methoda( GSR->bvideo, BTM_SETSTRING, &ss );

    sb.number = GSID_FXSLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    prop->value = GSR->destfx;
    _methoda( GSR->bvideo, BTM_REFRESH, &sb );

///

    /*** Inputrequester: AktualItem holen und darstellen ***/
    yw_ActualizeInputRequester( GSR );

}

/*-----------------------------------------------------------------*/
void yw_InitKeyTable( struct ypaworld_data *ywd )
/*
**  FUNCTION
**
**  CHANGED
**      26-Oct-97   floh    drinrumgepfuscht (neue Tasten + lokalisiert)
*/
{
    memset(GlobalKeyTable,0,sizeof(GlobalKeyTable));

/// "Ausfüllen der Tabelle"
    GlobalKeyTable[ 0 ].name =                     "*";  // Geht das ????
    GlobalKeyTable[ 0 ].ascii =                    '*';
    GlobalKeyTable[ 0 ].config =                   "nop";

    GlobalKeyTable[KEYCODE_ESCAPE].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_ESCAPE,"ESC");
    GlobalKeyTable[KEYCODE_ESCAPE].ascii  = 0;
    GlobalKeyTable[KEYCODE_ESCAPE].config = "esc";

    GlobalKeyTable[KEYCODE_SPACEBAR].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_SPACEBAR,"SPACE");
    GlobalKeyTable[KEYCODE_SPACEBAR].ascii  = ' ';
    GlobalKeyTable[KEYCODE_SPACEBAR].config = "space";

    GlobalKeyTable[KEYCODE_CURSOR_UP].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_CURSOR_UP,"UP");
    GlobalKeyTable[KEYCODE_CURSOR_UP].ascii  = 0;
    GlobalKeyTable[KEYCODE_CURSOR_UP].config = "up";

    GlobalKeyTable[KEYCODE_CURSOR_DOWN].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_CURSOR_DOWN,"DOWN");
    GlobalKeyTable[KEYCODE_CURSOR_DOWN].ascii  = 0;
    GlobalKeyTable[KEYCODE_CURSOR_DOWN].config = "down";

    GlobalKeyTable[KEYCODE_CURSOR_LEFT].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_CURSOR_LEFT,"LEFT");
    GlobalKeyTable[KEYCODE_CURSOR_LEFT].ascii  = 0;
    GlobalKeyTable[KEYCODE_CURSOR_LEFT].config = "left";

    GlobalKeyTable[KEYCODE_CURSOR_RIGHT].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_CURSOR_RIGHT,"RIGHT");
    GlobalKeyTable[KEYCODE_CURSOR_RIGHT].ascii  = 0;
    GlobalKeyTable[KEYCODE_CURSOR_RIGHT].config = "right";

    GlobalKeyTable[KEYCODE_F1].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F1,"F1");
    GlobalKeyTable[KEYCODE_F1].ascii  = 0;
    GlobalKeyTable[KEYCODE_F1].config = "f1";

    GlobalKeyTable[KEYCODE_F2].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F2,"F2");
    GlobalKeyTable[KEYCODE_F2].ascii  = 0;
    GlobalKeyTable[KEYCODE_F2].config = "f2";

    GlobalKeyTable[KEYCODE_F3].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F3,"F3");
    GlobalKeyTable[KEYCODE_F3].ascii  = 0;
    GlobalKeyTable[KEYCODE_F3].config = "f3";

    GlobalKeyTable[KEYCODE_F4].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F4,"F4");
    GlobalKeyTable[KEYCODE_F4].ascii  = 0;
    GlobalKeyTable[KEYCODE_F4].config = "f4";

    GlobalKeyTable[KEYCODE_F5].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F5,"F5");
    GlobalKeyTable[KEYCODE_F5].ascii  = 0;
    GlobalKeyTable[KEYCODE_F5].config = "f5";

    GlobalKeyTable[KEYCODE_F6].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F6,"F6");
    GlobalKeyTable[KEYCODE_F6].ascii  = 0;
    GlobalKeyTable[KEYCODE_F6].config = "f6";

    GlobalKeyTable[KEYCODE_F7].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F7,"F7");
    GlobalKeyTable[KEYCODE_F7].ascii  = 0;
    GlobalKeyTable[KEYCODE_F7].config = "f7";

    GlobalKeyTable[KEYCODE_F8].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F8,"F8");
    GlobalKeyTable[KEYCODE_F8].ascii  = 0;
    GlobalKeyTable[KEYCODE_F8].config = "f8";

    GlobalKeyTable[KEYCODE_F9].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F9,"F9");
    GlobalKeyTable[KEYCODE_F9].ascii  = 0;
    GlobalKeyTable[KEYCODE_F9].config = "f9";

    GlobalKeyTable[KEYCODE_F10].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F10,"F10");
    GlobalKeyTable[KEYCODE_F10].ascii  = 0;
    GlobalKeyTable[KEYCODE_F10].config = "f10";

    GlobalKeyTable[KEYCODE_F11].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F11,"F11");
    GlobalKeyTable[KEYCODE_F11].ascii  = 0;
    GlobalKeyTable[KEYCODE_F11].config = "f11";

    GlobalKeyTable[KEYCODE_F12].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F12,"F12");
    GlobalKeyTable[KEYCODE_F12].ascii  = 0;
    GlobalKeyTable[KEYCODE_F12].config = "f12";

    GlobalKeyTable[KEYCODE_BS].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_BS,"BACK");
    GlobalKeyTable[KEYCODE_BS].ascii  = 0;
    GlobalKeyTable[KEYCODE_BS].config = "bs";

    GlobalKeyTable[KEYCODE_TAB].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_TAB,"TAB");
    GlobalKeyTable[KEYCODE_TAB].ascii  = 0;
    GlobalKeyTable[KEYCODE_TAB].config = "tab";

    GlobalKeyTable[KEYCODE_CLEAR].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_CLEAR,"CLEAR");
    GlobalKeyTable[KEYCODE_CLEAR].ascii  = 0;
    GlobalKeyTable[KEYCODE_CLEAR].config = "clear";

    GlobalKeyTable[KEYCODE_RETURN].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_RETURN,"RETURN");
    GlobalKeyTable[KEYCODE_RETURN].ascii  = 0;
    GlobalKeyTable[KEYCODE_RETURN].config = "return";

    GlobalKeyTable[KEYCODE_CTRL].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_CTRL,"CTRL");
    GlobalKeyTable[KEYCODE_CTRL].ascii  = 0;
    GlobalKeyTable[KEYCODE_CTRL].config = "ctrl";

    GlobalKeyTable[KEYCODE_LSHIFT].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_SHIFT,"SHIFT");
    GlobalKeyTable[KEYCODE_LSHIFT].ascii  = 0;
    GlobalKeyTable[KEYCODE_LSHIFT].config = "rshift";

    GlobalKeyTable[KEYCODE_LALT].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_ALT,"ALT");
    GlobalKeyTable[KEYCODE_LALT].ascii  = 0;
    GlobalKeyTable[KEYCODE_LALT].config = "alt";

    GlobalKeyTable[KEYCODE_PAUSE].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_PAUSE,"PAUSE");
    GlobalKeyTable[KEYCODE_PAUSE].ascii  = 0;
    GlobalKeyTable[KEYCODE_PAUSE].config = "pause";

    GlobalKeyTable[KEYCODE_PAGEUP].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_PAGEUP,"PGUP");
    GlobalKeyTable[KEYCODE_PAGEUP].ascii  = 0;
    GlobalKeyTable[KEYCODE_PAGEUP].config = "pageup";

    GlobalKeyTable[KEYCODE_PAGEDOWN].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_PAGEDOWN,"PGDOWN");
    GlobalKeyTable[KEYCODE_PAGEDOWN].ascii  = 0;
    GlobalKeyTable[KEYCODE_PAGEDOWN].config = "pagedown";

    GlobalKeyTable[KEYCODE_END].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_END,"END");
    GlobalKeyTable[KEYCODE_END].ascii  = 0;
    GlobalKeyTable[KEYCODE_END].config = "end";

    GlobalKeyTable[KEYCODE_HOME].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_HOME,"HOME");
    GlobalKeyTable[KEYCODE_HOME].ascii  = 0;
    GlobalKeyTable[KEYCODE_HOME].config = "home";

    GlobalKeyTable[KEYCODE_SELECT].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_SELECT,"SELECT");
    GlobalKeyTable[KEYCODE_SELECT].ascii  = 0;
    GlobalKeyTable[KEYCODE_SELECT].config = "select";

    GlobalKeyTable[KEYCODE_EXECUTE].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXECUTE,"EXEC");
    GlobalKeyTable[KEYCODE_EXECUTE].ascii  = 0;
    GlobalKeyTable[KEYCODE_EXECUTE].config = "execute";

    GlobalKeyTable[KEYCODE_SNAPSHOT].name = ypa_GetStr(ywd->LocHandle,STR_KEY_SNAPSHOT,"PRINT");
    GlobalKeyTable[KEYCODE_SNAPSHOT].ascii = 0;
    GlobalKeyTable[KEYCODE_SNAPSHOT].config = "snapshot";

    GlobalKeyTable[KEYCODE_INS].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_INS,"INS");
    GlobalKeyTable[KEYCODE_INS].ascii  = 0;
    GlobalKeyTable[KEYCODE_INS].config = "ins";

    GlobalKeyTable[KEYCODE_DEL].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_DEL,"DEL");
    GlobalKeyTable[KEYCODE_DEL].ascii  = 0;
    GlobalKeyTable[KEYCODE_DEL].config = "del";

    GlobalKeyTable[KEYCODE_1].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_1,"1");           
    GlobalKeyTable[KEYCODE_1].ascii  = '1';
    GlobalKeyTable[KEYCODE_1].config = "1";

    GlobalKeyTable[KEYCODE_2].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_2,"2");
    GlobalKeyTable[KEYCODE_2].ascii  = '2';
    GlobalKeyTable[KEYCODE_2].config = "2";

    GlobalKeyTable[KEYCODE_3].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_3,"3");
    GlobalKeyTable[KEYCODE_3].ascii  = '3';
    GlobalKeyTable[KEYCODE_3].config = "3";

    GlobalKeyTable[KEYCODE_4].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_4,"4");
    GlobalKeyTable[KEYCODE_4].ascii  = '4';
    GlobalKeyTable[KEYCODE_4].config = "4";

    GlobalKeyTable[KEYCODE_5].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_5,"5");
    GlobalKeyTable[KEYCODE_5].ascii  = '5';
    GlobalKeyTable[KEYCODE_5].config = "5";

    GlobalKeyTable[KEYCODE_6].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_6,"6");
    GlobalKeyTable[KEYCODE_6].ascii  = '6';
    GlobalKeyTable[KEYCODE_6].config = "6";

    GlobalKeyTable[KEYCODE_7].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_7,"7");
    GlobalKeyTable[KEYCODE_7].ascii  = '7';
    GlobalKeyTable[KEYCODE_7].config = "7";

    GlobalKeyTable[KEYCODE_8].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_8,"8");
    GlobalKeyTable[KEYCODE_8].ascii  = '8';
    GlobalKeyTable[KEYCODE_8].config = "8";

    GlobalKeyTable[KEYCODE_9].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_9,"9");
    GlobalKeyTable[KEYCODE_9].ascii  = '9';
    GlobalKeyTable[KEYCODE_9].config = "9";

    GlobalKeyTable[KEYCODE_0].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_0,"0");
    GlobalKeyTable[KEYCODE_0].ascii  = '0';
    GlobalKeyTable[KEYCODE_0].config = "0";

    GlobalKeyTable[KEYCODE_A].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_A,"A");
    GlobalKeyTable[KEYCODE_A].ascii  = 'A';
    GlobalKeyTable[KEYCODE_A].config = "a";

    GlobalKeyTable[KEYCODE_B].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_B,"B");
    GlobalKeyTable[KEYCODE_B].ascii  = 'B';
    GlobalKeyTable[KEYCODE_B].config = "b";

    GlobalKeyTable[KEYCODE_C].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_C,"C");
    GlobalKeyTable[KEYCODE_C].ascii  = 'C';
    GlobalKeyTable[KEYCODE_C].config = "c";

    GlobalKeyTable[KEYCODE_D].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_D,"D");
    GlobalKeyTable[KEYCODE_D].ascii  = 'D';
    GlobalKeyTable[KEYCODE_D].config = "d";

    GlobalKeyTable[KEYCODE_E].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_E,"E");
    GlobalKeyTable[KEYCODE_E].ascii  = 'E';
    GlobalKeyTable[KEYCODE_E].config = "e";

    GlobalKeyTable[KEYCODE_F].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_F,"F");
    GlobalKeyTable[KEYCODE_F].ascii  = 'F';
    GlobalKeyTable[KEYCODE_F].config = "f";

    GlobalKeyTable[KEYCODE_G].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_G,"G");
    GlobalKeyTable[KEYCODE_G].ascii  = 'G';
    GlobalKeyTable[KEYCODE_G].config = "g";

    GlobalKeyTable[KEYCODE_H].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_H,"H");
    GlobalKeyTable[KEYCODE_H].ascii  = 'H';
    GlobalKeyTable[KEYCODE_H].config = "h";

    GlobalKeyTable[KEYCODE_I].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_I,"I");
    GlobalKeyTable[KEYCODE_I].ascii  = 'I';
    GlobalKeyTable[KEYCODE_I].config = "i";

    GlobalKeyTable[KEYCODE_J].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_J,"J");
    GlobalKeyTable[KEYCODE_J].ascii  = 'J';
    GlobalKeyTable[KEYCODE_J].config = "j";

    GlobalKeyTable[KEYCODE_K].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_K,"K");
    GlobalKeyTable[KEYCODE_K].ascii  = 'K';
    GlobalKeyTable[KEYCODE_K].config = "k";

    GlobalKeyTable[KEYCODE_L].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_L,"L");
    GlobalKeyTable[KEYCODE_L].ascii  = 'L';
    GlobalKeyTable[KEYCODE_L].config = "l";

    GlobalKeyTable[KEYCODE_M].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_M,"M");
    GlobalKeyTable[KEYCODE_M].ascii  = 'M';
    GlobalKeyTable[KEYCODE_M].config = "m";

    GlobalKeyTable[KEYCODE_N].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_N,"N");
    GlobalKeyTable[KEYCODE_N].ascii  = 'N';
    GlobalKeyTable[KEYCODE_N].config = "n";

    GlobalKeyTable[KEYCODE_O].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_O,"O");
    GlobalKeyTable[KEYCODE_O].ascii  = 'O';
    GlobalKeyTable[KEYCODE_O].config = "o";

    GlobalKeyTable[KEYCODE_P].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_P,"P");
    GlobalKeyTable[KEYCODE_P].ascii  = 'P';
    GlobalKeyTable[KEYCODE_P].config = "p";

    GlobalKeyTable[KEYCODE_Q].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_Q,"Q");
    GlobalKeyTable[KEYCODE_Q].ascii  = 'Q';
    GlobalKeyTable[KEYCODE_Q].config = "q";

    GlobalKeyTable[KEYCODE_R].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_R,"R");
    GlobalKeyTable[KEYCODE_R].ascii  = 'R';
    GlobalKeyTable[KEYCODE_R].config = "r";

    GlobalKeyTable[KEYCODE_S].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_S,"S");
    GlobalKeyTable[KEYCODE_S].ascii  = 'S';
    GlobalKeyTable[KEYCODE_S].config = "s";

    GlobalKeyTable[KEYCODE_T].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_T,"T");
    GlobalKeyTable[KEYCODE_T].ascii  = 'T';
    GlobalKeyTable[KEYCODE_T].config = "t";

    GlobalKeyTable[KEYCODE_U].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_U,"U");
    GlobalKeyTable[KEYCODE_U].ascii  = 'U';
    GlobalKeyTable[KEYCODE_U].config = "u";

    GlobalKeyTable[KEYCODE_V].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_V,"V");
    GlobalKeyTable[KEYCODE_V].ascii  = 'V';
    GlobalKeyTable[KEYCODE_V].config = "v";

    GlobalKeyTable[KEYCODE_W].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_W,"W");
    GlobalKeyTable[KEYCODE_W].ascii  = 'W';
    GlobalKeyTable[KEYCODE_W].config = "w";

    GlobalKeyTable[KEYCODE_X].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_X,"X");
    GlobalKeyTable[KEYCODE_X].ascii  = 'X';
    GlobalKeyTable[KEYCODE_X].config = "x";

    GlobalKeyTable[KEYCODE_Y].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_Y,"Y");
    GlobalKeyTable[KEYCODE_Y].ascii  = 'Y';
    GlobalKeyTable[KEYCODE_Y].config = "y";

    GlobalKeyTable[KEYCODE_Z].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_Z,"Z");
    GlobalKeyTable[KEYCODE_Z].ascii  = 'Z';
    GlobalKeyTable[KEYCODE_Z].config = "z";

    GlobalKeyTable[KEYCODE_NUM_0].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_0,"NUM 0");
    GlobalKeyTable[KEYCODE_NUM_0].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_0].config = "num0";

    GlobalKeyTable[KEYCODE_NUM_1].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_1,"NUM 1");
    GlobalKeyTable[KEYCODE_NUM_1].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_1].config = "num1";

    GlobalKeyTable[KEYCODE_NUM_2].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_2,"NUM 2");
    GlobalKeyTable[KEYCODE_NUM_2].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_2].config = "num2";

    GlobalKeyTable[KEYCODE_NUM_3].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_3,"NUM 3");
    GlobalKeyTable[KEYCODE_NUM_3].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_3].config = "num3";

    GlobalKeyTable[KEYCODE_NUM_4].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_4,"NUM 4");
    GlobalKeyTable[KEYCODE_NUM_4].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_4].config = "num4";

    GlobalKeyTable[KEYCODE_NUM_5].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_5,"NUM 5");
    GlobalKeyTable[KEYCODE_NUM_5].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_5].config = "num5";

    GlobalKeyTable[KEYCODE_NUM_6].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_6,"NUM 6");
    GlobalKeyTable[KEYCODE_NUM_6].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_6].config = "num6";

    GlobalKeyTable[KEYCODE_NUM_7].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_7,"NUM 7");
    GlobalKeyTable[KEYCODE_NUM_7].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_7].config = "num7";

    GlobalKeyTable[KEYCODE_NUM_8].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_8,"NUM 8");
    GlobalKeyTable[KEYCODE_NUM_8].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_8].config = "num8";

    GlobalKeyTable[KEYCODE_NUM_9].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_9,"NUM 9");
    GlobalKeyTable[KEYCODE_NUM_9].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_9].config = "num9";

    GlobalKeyTable[KEYCODE_NUM_MUL].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_MUL,"MUL");
    GlobalKeyTable[KEYCODE_NUM_MUL].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_MUL].config = "nummul";

    GlobalKeyTable[KEYCODE_NUM_PLUS].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_PLUS,"ADD");
    GlobalKeyTable[KEYCODE_NUM_PLUS].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_PLUS].config = "numplus";

    GlobalKeyTable[KEYCODE_NUM_DOT].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_DOT,"DOT");
    GlobalKeyTable[KEYCODE_NUM_DOT].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_DOT].config = "numdot";

    GlobalKeyTable[KEYCODE_NUM_MINUS].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_MINUS,"SUB");
    GlobalKeyTable[KEYCODE_NUM_MINUS].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_MINUS].config = "numminus";

    GlobalKeyTable[KEYCODE_ENTER].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_ENTER,"ENTER");
    GlobalKeyTable[KEYCODE_ENTER].ascii  = 0;
    GlobalKeyTable[KEYCODE_ENTER].config = "enter";

    GlobalKeyTable[KEYCODE_NUM_DIV].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_NUM_DIV,"DIV");
    GlobalKeyTable[KEYCODE_NUM_DIV].ascii  = 0;
    GlobalKeyTable[KEYCODE_NUM_DIV].config = "numdiv";

    GlobalKeyTable[KEYCODE_EXTRA_1].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_1,",");
    GlobalKeyTable[KEYCODE_EXTRA_1].ascii  = ',';
    GlobalKeyTable[KEYCODE_EXTRA_1].config = "extra1";

    GlobalKeyTable[KEYCODE_EXTRA_2].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_2,".");
    GlobalKeyTable[KEYCODE_EXTRA_2].ascii  = '.';
    GlobalKeyTable[KEYCODE_EXTRA_2].config = "extra2";

    GlobalKeyTable[KEYCODE_EXTRA_3].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_3,"-");
    GlobalKeyTable[KEYCODE_EXTRA_3].ascii  = '-';
    GlobalKeyTable[KEYCODE_EXTRA_3].config = "extra3";

    GlobalKeyTable[KEYCODE_EXTRA_4].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_4,"<");
    GlobalKeyTable[KEYCODE_EXTRA_4].ascii  = '<';
    GlobalKeyTable[KEYCODE_EXTRA_4].config = "extra4";

    GlobalKeyTable[KEYCODE_EXTRA_5].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_5,"Ü");
    GlobalKeyTable[KEYCODE_EXTRA_5].ascii  = (UBYTE)'ü';
    GlobalKeyTable[KEYCODE_EXTRA_5].config = "extra5";

    GlobalKeyTable[KEYCODE_EXTRA_6].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_6,"+");
    GlobalKeyTable[KEYCODE_EXTRA_6].ascii  = '+';
    GlobalKeyTable[KEYCODE_EXTRA_6].config = "extra6";

    GlobalKeyTable[KEYCODE_EXTRA_7].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_7,"Ö");
    GlobalKeyTable[KEYCODE_EXTRA_7].ascii  = (UBYTE)'ö';
    GlobalKeyTable[KEYCODE_EXTRA_7].config = "extra7";

    GlobalKeyTable[KEYCODE_EXTRA_8].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_8,"Ä");
    GlobalKeyTable[KEYCODE_EXTRA_8].ascii  = (UBYTE)'ä';
    GlobalKeyTable[KEYCODE_EXTRA_8].config = "extra8";

    GlobalKeyTable[KEYCODE_EXTRA_9].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_9,"#");
    GlobalKeyTable[KEYCODE_EXTRA_9].ascii  = '#';
    GlobalKeyTable[KEYCODE_EXTRA_9].config = "extra9";

    GlobalKeyTable[KEYCODE_EXTRA_10].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_10,"BLUB");
    GlobalKeyTable[KEYCODE_EXTRA_10].ascii  = '`';
    GlobalKeyTable[KEYCODE_EXTRA_10].config = "extra10";

    GlobalKeyTable[KEYCODE_EXTRA_11].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_EXTRA_11,"BLOB");
    GlobalKeyTable[KEYCODE_EXTRA_11].ascii  = '^';
    GlobalKeyTable[KEYCODE_EXTRA_11].config = "extra11";

    GlobalKeyTable[WINP_CODE_MMB].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_MMB,"MIDDLE MOUSE");
    GlobalKeyTable[WINP_CODE_MMB].ascii  = 0;
    GlobalKeyTable[WINP_CODE_MMB].config = "mmb";

    GlobalKeyTable[WINP_CODE_JB0].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB0,"JOYB0");
    GlobalKeyTable[WINP_CODE_JB0].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB0].config = "joy_b0";

    GlobalKeyTable[WINP_CODE_JB1].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB1,"JOYB1");
    GlobalKeyTable[WINP_CODE_JB1].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB1].config = "joy_b1";

    GlobalKeyTable[WINP_CODE_JB2].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB2,"JOYB2");
    GlobalKeyTable[WINP_CODE_JB2].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB2].config = "joy_b2";

    GlobalKeyTable[WINP_CODE_JB3].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB3,"JOYB3");
    GlobalKeyTable[WINP_CODE_JB3].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB3].config = "joy_b3";

    GlobalKeyTable[WINP_CODE_JB4].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB4,"JOYB4");
    GlobalKeyTable[WINP_CODE_JB4].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB4].config = "joy_b4";

    GlobalKeyTable[WINP_CODE_JB5].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB5,"JOYB5");
    GlobalKeyTable[WINP_CODE_JB5].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB5].config = "joy_b5";

    GlobalKeyTable[WINP_CODE_JB6].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB6,"JOYB6");
    GlobalKeyTable[WINP_CODE_JB6].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB6].config = "joy_b6";

    GlobalKeyTable[WINP_CODE_JB7].name   = ypa_GetStr(ywd->LocHandle,STR_KEY_JB7,"JOYB7");
    GlobalKeyTable[WINP_CODE_JB7].ascii  = 0;
    GlobalKeyTable[WINP_CODE_JB7].config = "joy_b7";
///

}

/// "yw_InitVideoList"
void yw_InitVideoList( struct GameShellReq *GSR )
{
    /* --------------------------------------------------------------
    ** Wir erfragen vom "VideoObjekt" alle Modi und erzeugen zu jedem
    ** Modus eine Node.
    ** Wir muessen uns den jetzigen Modus merken und dazu dann die
    ** neue Node suchen, weil ein neuer treiber auch eine neue Liste
    ** hat
    ** ------------------------------------------------------------*/

    ULONG   i, next_id, n;
    struct  disp_query_msg dqm;
    struct  video_node *node;

    /*** erstmal aufraeumen ***/
    while( node = (struct video_node *) _RemHead( (struct List *) &(GSR->videolist) ) )
        _FreeVec( (APTR) node );
    
    /*** Zur Sicherheit, neuerdings gibt es wieder Abstuerze ***/
    _OVE_GetAttrs( OVET_Object, &(GSR->ywd->GfxObject), TAG_DONE );

    /* -------------------------------------------------------------------
    ** Modi abfragen 
    ** (das folgende Schleifenkonstruct ist vom Floh und echt interessant) 
    ** -----------------------------------------------------------------*/
    next_id = 0;
    dqm.id  = 0;
    n       = 0;
    do {

        next_id = _methoda( GSR->ywd->GfxObject, DISPM_Query, &dqm );

        /*** Node dazu allozieren ***/
        if( node = _AllocVec( sizeof( struct video_node ), MEMF_CLEAR ) ) {

            /*** Ausfnllen ***/
            node->modus = dqm.id;
            node->res_x = dqm.w;
            node->res_y = dqm.h;

            n++;
            i = 0;
            while( dqm.name[ i ] && (i < 32) ) {

                node->name[ i ] = toupper( dqm.name[ i ] );
                i++;
                }

            /*** Einklinken ***/
            _AddTail( (struct List *) &( GSR->videolist ),
                      (struct Node *) node );
            }
        } while( dqm.id = next_id );   

}
///


/// "yw_ScanUserDirectory"
void yw_ScanUserDirectory( struct GameShellReq *GSR, char *dirname, Object *World )
{
/*
**  FUNCTION    Scannt ein Direktory und gibt eine Liste mit fileinfo-
**              Strukturen zurück.
**              NEU: Für jeden User existiert ein Verzeichnis!!!!
**
*/

    struct fileinfonode *infonode;
    struct ncDirEntry entry;
    struct ypa_PlayerStats gstats[ MAXNUM_ROBOS ];
    LONG   mre,mrc;
    char   nbuffer[ 300 ];

    /* ---------------------------------------------------------------
    ** Buffern der Daten, die wir beim Parsen überschreiben. Sollte in
    ** DM_USER noch etwas dazukommen, muß es hier geparts werden!
    ** -------------------------------------------------------------*/
    memcpy( gstats, GSR->ywd->GlobalStats, sizeof( gstats ) );
    mre = GSR->ywd->MaxRoboEnergy;
    mrc = GSR->ywd->MaxReloadConst;
    strcpy( nbuffer, GSR->NPlayerName );

    if( GSR->UserDir = _FOpenDir( dirname ) ) {

        /*** Nun alle auslesen ***/
        while( _FReadDir( GSR->UserDir, &entry ) ) {

            /*** 0.usr wird ausgeklammert ***/
            if( entry.attrs & NCDIR_DIRECTORY ) {

                /* --------------------------------------------------
                ** Scheint eine Spielstandsdatei zu sein. Bei Windows
                ** müssen wir die Verzeichnisse . und .. noch raus-
                ** filtern
                ** ------------------------------------------------*/
                #ifdef __WINDOWS__
                if( ( strcmp( entry.name, "." )  == 0 ) ||
                    ( strcmp( entry.name, ".." ) == 0 ) )
                    continue;
                #endif

                /*** Node dafnr allozieren ***/
                if( infonode = _AllocVec( sizeof( struct fileinfonode ), 
                                          MEMF_CLEAR | MEMF_PUBLIC ) ) {

                    struct ScriptParser parser;
                    char   filename[ 300 ];

                    /* ------------------------------------------
                    ** Node ausfüllen. Aus '_' wird bei LocStrCpy
                    ** automatisch ein Space
                    ** ----------------------------------------*/
                    yw_LocStrCpy( infonode->username, entry.name );

                    /*** So, die Node ist ausgefnllt, also klinken wir sie ein ***/
                    _AddTail( (struct List *) &( GSR->flist ), 
                              (struct Node *) infonode );

                    /*** File wegen Zeit parsen ***/
                    parser.parse_func = yw_ParseUserData;
                    parser.store[0]   = (ULONG) GSR->ywd->world;
                    parser.store[1]   = (ULONG) GSR->ywd;
                    sprintf( filename, "%s/%s/user.txt\n", dirname, entry.name );

                    if( !yw_ParseScript( filename, 1, &parser, PARSEMODE_SLOPPY ) )
                        _LogMsg("Warning, cannot parse %s for time scanning\n",
                                 filename );

                    /*** Pauschal Userowner 1 ***/
                    infonode->owner       = 1;
                    infonode->global_time = GSR->ywd->GlobalStats[ 1 ].Time;
                    }
                }
            }

        _FCloseDir( GSR->UserDir );
        }
    else {

        _LogMsg("Unknown Game-Directory %s\n", dirname );
        }

    /*** Rückschreiben der GlobalStats ***/
    memcpy( GSR->ywd->GlobalStats, gstats, sizeof( gstats ) );
    GSR->ywd->MaxReloadConst = mrc;
    GSR->ywd->MaxRoboEnergy = mre;
    strcpy( GSR->NPlayerName, nbuffer );
}
///


/// "yw_ScanLocaleDirectory"
void yw_ScanLocaleDirectory( struct GameShellReq *GSR, char *dirname, Object *World )
{
/*
**  FUNCTION    Scannt ein Direktory und gibt eine Liste mit localeinfo-
**              Strukturen zurnck
**
*/

    struct localeinfonode *infonode, *found_english = NULL;
    struct ncDirEntry entry;
    ULONG  pos, i;
    APTR   dir;

    GSR->num_languages = 0;

    if( dir = _FOpenDir( dirname ) ) {

        /*** Nun alle auslesen ***/
        while( _FReadDir( dir, &entry ) ) {

            /* -------------------------------------------------------------
            ** Ist es ein File, welches ich wnnsche? Weil sich WATCOM
            ** und SAS in ihren Strukturen absolut widersprechen und ich nur
            ** das Namensfeld gemeinsam habe, suche ich nach ".LNG" im
            ** Namen. Anders geht das nicht. Scheiße
            **
            ** NEU: auch DLLs beachten. Nur einen Eintrag pro Sprache zu-
            ** lassen.
            ** -----------------------------------------------------------*/

            /*** Endungen übeprüfen ***/
            pos = 0L;
            pos = (ULONG) strstr( entry.name, ".LNG" );
            if( !pos ) pos = (ULONG) strstr( entry.name, ".lng" );
            if( !pos ) pos = (ULONG) strstr( entry.name, ".dll" );
            if( !pos ) pos = (ULONG) strstr( entry.name, ".DLL" );
            
            if( (!(entry.attrs & NCDIR_DIRECTORY)) &&  pos ) {

                char language_name[ 255 ];
                BOOL just_here;

                /*** Wie heißt die Sprache? ***/
                i = 0;
                while( (i < MAX_LANGUAGELEN) &&
                       (entry.name[ i ] >  32) &&
                       (entry.name[ i ] != 46) ) {

                    language_name[ i ] = toupper( entry.name[ i ] );
                    i++;
                    }
                language_name[ i ] = 0;

                /*** Existiert schon so ein Eintrag? ***/
                just_here = FALSE;
                infonode = (struct localeinfonode *)GSR->localelist.mlh_Head;
                while( infonode->node.mln_Succ ) {

                    if( stricmp( infonode->language, language_name ) == 0 ) {
                        just_here = TRUE;
                        break;
                        }
                    infonode = (struct localeinfonode *)infonode->node.mln_Succ;
                    }

                if( !just_here ) {

                    GSR->num_languages++;

                    /*** Das is'n NEUES Locale-File ***/
                    if( infonode = _AllocVec( sizeof( struct localeinfonode ),
                                              MEMF_PUBLIC | MEMF_CLEAR ) ) {

                        /*** Ausfnllen ***/
                        strcpy( infonode->language, language_name );

                        /*** Einklinken ***/
                        _AddTail( (struct List *) &(GSR->localelist),
                                  (struct Node *) infonode );

                        if( stricmp( infonode->language, "language") == 0 )
                            found_english = infonode;
                        }
                    }
                }
            }

        /*** Verzeichnis schließen ***/
        _FCloseDir( dir );
        }
    else {

        _LogMsg("Unknown Locale-Directory %s\n", dirname );
        }

    /*** Erste Sache Default ***/
    if( found_english ) {
    
        /*** English wird defaultsprache, kann spSter nberschrieben werden ***/
        GSR->lsel = found_english;
        }
}
///


/// "yw_ParseShellScript"
BOOL yw_ParseShellScript( struct GameShellReq *GSR )
{
/*  ---------------------------------------------------------------
**  Lädt alle Samples, die die Shell benötigt. Diese liegen im File
**  shell.ini in levels
**
**  NEU: world.ini und shell.ini sind nun eins. Ich lasse aber
**  trotzdem die Namen.
**  -------------------------------------------------------------*/

    struct ScriptParser parser[ 2 ];

    parser[0].parse_func = yw_ParseShellSounds;
    parser[0].target     = GSR;
    parser[1].parse_func = yw_ParseShellTracks;
    parser[1].target     = GSR;

    if( !yw_ParseScript( "data:world.ini", 2, parser, PARSEMODE_SLOPPY ) ) return( FALSE );

    return( TRUE );
}

///


/// "yw_ParseShellSounds"
ULONG yw_ParseShellSounds( struct ScriptParser *parser )
{
/* --------------------------------------------------------------
** Liest nur den Namen, der in new_user...end eingeschlossen ist.
** target ist ein String, in den wir schreiben können.
** target == NULL bedeutet ignorieren, also nicht bearbeiten
** ------------------------------------------------------------*/

    struct GameShellReq *GSR = NULL;
    if( parser->target ) GSR = (struct GameShellReq *) (parser->target);

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_shellsounds") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                ULONG r;

                if( PARSE_UNKNOWN_KEYWORD != (r = yw_InitShellSounds( GSR, parser ))) {

                    return( r );
                    }
                else {

                if( PARSE_UNKNOWN_KEYWORD != (r = yw_InitShellVolumes( GSR, parser ))) {

                    return( r );
                    }
                else {

                if( PARSE_UNKNOWN_KEYWORD != (r = yw_InitShellPitches( GSR, parser ))) {

                    return( r );
                    }
                else {

                    /*** Unbekannt ***/
                    return( PARSE_UNKNOWN_KEYWORD );
                    } } }
                }

            return( PARSE_ALL_OK );
            }
        }
}


LONG yw_InitShellSounds( struct GameShellReq *GSR, struct ScriptParser *parser)
{
    struct SoundCarrier *Sound;
    Object **so;
    int    ID;
    char   old_path[ 300 ];

    if( stricmp( parser->keyword, "quit_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_QUIT;
        }
    else {

    if( stricmp( parser->keyword, "volume_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_VOLUME;
        }
    else {

    if( stricmp( parser->keyword, "button_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_BUTTON;
        }
    else {

    if( stricmp( parser->keyword, "left_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_LEFT;
        }
    else {

    if( stricmp( parser->keyword, "right_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_RIGHT;
        }
    else {

    if( stricmp( parser->keyword, "slider_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_SLIDER;
        }
    else {

    if( stricmp( parser->keyword, "welcome_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_WELCOME;
        }
    else {

    if( stricmp( parser->keyword, "menuopen_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_MENUOPEN;
        }
    else {

    if( stricmp( parser->keyword, "overlevel_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_OVERLEVEL;
        }
    else {

    if( stricmp( parser->keyword, "levelselect_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_LEVELSELECT;
        }
    else {

    if( stricmp( parser->keyword, "textappear_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_TEXTAPPEAR;
        }
    else {

    if( stricmp( parser->keyword, "objectappear_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_OBJECTAPPEAR;
        }
    else {

    if( stricmp( parser->keyword, "sectorconquered_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_SECTORCONQUERED;
        }
    else {

    if( stricmp( parser->keyword, "vhcldestroyed_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_VHCLDESTROYED;
        }
    else {

    if( stricmp( parser->keyword, "bldgconquered_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_BLDGCONQUERED;
        }
    else {

    if( stricmp( parser->keyword, "timercount_sample") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_TIMERCOUNT;
        }
    else {

    if( stricmp( parser->keyword, "select_sample") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_SELECT;
        }
    else {

    if( stricmp( parser->keyword, "error_sample") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_ERROR;
        }
    else {

    if( stricmp( parser->keyword, "attention_sample") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_ATTENTION;
        }
    else {

    if( stricmp( parser->keyword, "secret_sample") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_SECRET;
        }
    else {

    if( stricmp( parser->keyword, "plasma_sample") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_PLASMA;
        }
    else {
    
        return( PARSE_UNKNOWN_KEYWORD );
        } } } } } } } } } } } } } } } } } } } } }


    /*** Nun Sample reinladen ***/
    strcpy( old_path, _GetAssign("rsrc"));
    _SetAssign("rsrc", "data:");

    /*** Objekt laden ***/
    if( !( so[ ID ] = _new("wav.class", RSA_Name, parser->data, TAG_DONE)))
          return( PARSE_BOGUS_DATA );

    /*** Sample holen ***/
    _get( so[ ID ], SMPA_Sample, &(Sound->src[ ID ].sample));

    /*** Nacharbeit ***/
    if( ( (ID == SHELLSOUND_VOLUME)      && (so == GSR->so1) ) ||
        ( (ID == SHELLSOUND_SLIDER)      && (so == GSR->so1) ) ||
        ( (ID == SHELLSOUND_TEXTAPPEAR)  && (so == GSR->so1) ) ||
        ( (ID == SHELLSOUND_TIMERCOUNT)  && (so == GSR->so1) ) )
        Sound->src[ ID ].flags |= AUDIOF_LOOPDALOOP;

    _SetAssign("rsrc",old_path);
    return( PARSE_ALL_OK );
}


LONG yw_InitShellVolumes( struct GameShellReq *GSR, struct ScriptParser *parser)
{
    struct SoundCarrier *Sound;
    Object **so;
    int    ID;

    if( stricmp( parser->keyword, "quit_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_QUIT;
        }
    else {

    if( stricmp( parser->keyword, "volume_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_VOLUME;
        }
    else {

    if( stricmp( parser->keyword, "button_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_BUTTON;
        }
    else {

    if( stricmp( parser->keyword, "left_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_LEFT;
        }
    else {

    if( stricmp( parser->keyword, "right_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_RIGHT;
        }
    else {

    if( stricmp( parser->keyword, "slider_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_SLIDER;
        }
    else {

    if( stricmp( parser->keyword, "welcome_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_WELCOME;
        }
    else {

    if( stricmp( parser->keyword, "menuopen_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_MENUOPEN;
        }
    else {

    if( stricmp( parser->keyword, "overlevel_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_OVERLEVEL;
        }
    else {

    if( stricmp( parser->keyword, "levelselect_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_LEVELSELECT;
        }
    else {

    if( stricmp( parser->keyword, "textappear_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_TEXTAPPEAR;
        }
    else {

    if( stricmp( parser->keyword, "objectappear_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_OBJECTAPPEAR;
        }
    else {

    if( stricmp( parser->keyword, "sectorconquered_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_SECTORCONQUERED;
        }
    else {

    if( stricmp( parser->keyword, "vhcldestroyed_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_VHCLDESTROYED;
        }
    else {

    if( stricmp( parser->keyword, "bldgconquered_volume") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_BLDGCONQUERED;
        }
    else {

    if( stricmp( parser->keyword, "timercount_volume") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_TIMERCOUNT;
        }
    else {

    if( stricmp( parser->keyword, "select_volume") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_SELECT;
        }
    else {

    if( stricmp( parser->keyword, "error_volume") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_ERROR;
        }
    else {

    if( stricmp( parser->keyword, "attention_volume") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_ATTENTION;
        }
    else {

    if( stricmp( parser->keyword, "secret_volume") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_SECRET;
        }
    else {

    if( stricmp( parser->keyword, "plasma_volume") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_PLASMA;
        }
    else {
        return( PARSE_UNKNOWN_KEYWORD );
        } } } } } } } } } } } } } } } } } } } } }

    /*** Volume setzen ***/
    Sound->src[ ID ].volume = atoi( parser->data );

    return( PARSE_ALL_OK );
}


LONG yw_InitShellPitches( struct GameShellReq *GSR, struct ScriptParser *parser)
{
    struct SoundCarrier *Sound;
    Object **so;
    int    ID;

    if( stricmp( parser->keyword, "quit_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_QUIT;
        }
    else {

    if( stricmp( parser->keyword, "volume_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_VOLUME;
        }
    else {

    if( stricmp( parser->keyword, "button_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_BUTTON;
        }
    else {

    if( stricmp( parser->keyword, "left_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_LEFT;
        }
    else {

    if( stricmp( parser->keyword, "right_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_RIGHT;
        }
    else {

    if( stricmp( parser->keyword, "slider_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_SLIDER;
        }
    else {

    if( stricmp( parser->keyword, "welcome_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_WELCOME;
        }
    else {

    if( stricmp( parser->keyword, "menuopen_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_MENUOPEN;
        }
    else {

    if( stricmp( parser->keyword, "overlevel_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_OVERLEVEL;
        }
    else {

    if( stricmp( parser->keyword, "levelselect_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_LEVELSELECT;
        }
    else {

    if( stricmp( parser->keyword, "textappear_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_TEXTAPPEAR;
        }
    else {

    if( stricmp( parser->keyword, "objectappear_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_OBJECTAPPEAR;
        }
    else {

    if( stricmp( parser->keyword, "sectorconquered_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_SECTORCONQUERED;
        }
    else {

    if( stricmp( parser->keyword, "vhcldestroyed_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_VHCLDESTROYED;
        }
    else {

    if( stricmp( parser->keyword, "bldgconquered_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_BLDGCONQUERED;
        }
    else {


    if( stricmp( parser->keyword, "timercount_pitch") == 0 ) {

        so = GSR->so1; Sound = &(GSR->ShellSound1); ID = SHELLSOUND_TIMERCOUNT;
        }
    else {

    if( stricmp( parser->keyword, "select_pitch") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_SELECT;
        }
    else {

    if( stricmp( parser->keyword, "error_pitch") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_ERROR;
        }
    else {

    if( stricmp( parser->keyword, "attention_pitch") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_ATTENTION;
        }
    else {

    if( stricmp( parser->keyword, "secret_pitch") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_SECRET;
        }
    else {

    if( stricmp( parser->keyword, "plasma_pitch") == 0 ) {

        so = GSR->so2; Sound = &(GSR->ShellSound2); ID = SHELLSOUND_PLASMA;
        }
    else {
        return( PARSE_UNKNOWN_KEYWORD );
        } } } } } } } } } } } } } } } } } } } } }

    /*** Pitch setzen ***/
    Sound->src[ ID ].pitch = atoi( parser->data );

    return( PARSE_ALL_OK );
}
///


/// "yw_ParseShellTracks"
ULONG yw_ParseShellTracks( struct ScriptParser *parser )
{
/* --------------------------------------------------------------
** Liest nur den Namen, der in new_user...end eingeschlossen ist.
** target ist ein String, in den wir schreiben können.
** target == NULL bedeutet ignorieren, also nicht bearbeiten
** ------------------------------------------------------------*/

    struct GameShellReq *GSR = NULL;
    if( parser->target ) GSR = (struct GameShellReq *) (parser->target);

    if( PARSESTAT_READY == parser->status ) {

        /*** Wir sind bereit für etwas neues ***/
        if( stricmp( parser->keyword, "begin_shelltracks") == 0 ) {

            /*** Das ist etwas für uns ***/
            parser->status = PARSESTAT_RUNNING;
            return( PARSE_ENTERED_CONTEXT );
            }
        else {

            /*** Damit kann ich nichts anfangen ***/
            return( PARSE_UNKNOWN_KEYWORD );
            }
        }
    else {

        /*** Wir bearbeiten das schon. Ist es das Ende? ***/
        if( stricmp( parser->keyword, "end") == 0 ) {

            /*** Das Ende naht! ***/
            parser->status = PARSESTAT_READY;
            return( PARSE_LEFT_CONTEXT );
            }
        else {

            /*** Es sollte also ein  normales keyword sein ***/
            if( parser->target ) {

                char data[ 300 ], *p;
                strcpy( data, parser->data );

                if( stricmp( parser->keyword, "shelltrack") == 0 ) {

                    /*** Track für Shellhintergrund ***/
                    GSR->shell_min_delay = 0;
                    GSR->shell_max_delay = 0;
                    p = strtok( data, " \t_\n");
                    GSR->shelltrack = strtol( p, NULL, 0 );
                    if( p = strtok( NULL, " \t_\n") ) {
                        GSR->shell_min_delay = strtol( p, NULL, 0 );
                        if( p = strtok( NULL, " \t_\n") )
                            GSR->shell_max_delay = strtol( p, NULL, 0 );
                        }
                    }
                else {

                if( stricmp( parser->keyword, "missiontrack") == 0 ) {

                    /*** Track für das Mission-Briefing ***/
                    GSR->mission_min_delay = 0;
                    GSR->mission_max_delay = 0;
                    p = strtok( data, " \t_\n");
                    GSR->missiontrack = strtol( p, NULL, 0 );
                    if( p = strtok( NULL, " \t_\n") ) {
                        GSR->mission_min_delay = strtol( p, NULL, 0 );
                        if( p = strtok( NULL, " \t_\n") )
                            GSR->mission_max_delay = strtol( p, NULL, 0 );
                        }
                    }
                else {

                if( stricmp( parser->keyword, "debriefingtrack") == 0 ) {

                    /*** Track für die spielauswertung ***/
                    GSR->debriefing_min_delay = 0;
                    GSR->debriefing_max_delay = 0;
                    p = strtok( data, " \t_\n");
                    GSR->debriefingtrack = strtol( p, NULL, 0 );
                    if( p = strtok( NULL, " \t_\n") ) {
                        GSR->debriefing_min_delay = strtol( p, NULL, 0 );
                        if( p = strtok( NULL, " \t_\n") )
                            GSR->debriefing_max_delay = strtol( p, NULL, 0 );
                        }
                    }
                else {

                if( stricmp( parser->keyword, "loadingtrack") == 0 ) {

                    /*** Track zur Überbrückung von Ladezeiten ***/
                    GSR->loading_min_delay = 0;
                    GSR->loading_max_delay = 0;
                    p = strtok( data, " \t_\n");
                    GSR->loadingtrack = strtol( p, NULL, 0 );
                    if( p = strtok( NULL, " \t_\n") ) {
                        GSR->loading_min_delay = strtol( p, NULL, 0 );
                        if( p = strtok( NULL, " \t_\n") )
                            GSR->loading_max_delay = strtol( p, NULL, 0 );
                        }
                    }
                else return( PARSE_UNKNOWN_KEYWORD );
                } } } }

            return( PARSE_ALL_OK );
            }
        }
}

/// "HowMuchNodes"


LONG yw_HowMuchNodes( struct List *list )
{
    /*** Gibt die Anzahl der Einträge zurück ***/
    LONG        count = 0;
    struct Node *node = list->lh_Head;

    while( node->ln_Succ ) {

        count++;
        node = node->ln_Succ;
        }

    return( count );
}


///

