/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_listview.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/03/21 20:30:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen für die GameShell
**
**  (C) Copyright 1995 by A.Flemming
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
#include "input/inputclass.h"
#include "input/idevclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guilist.h"
#include "ypa/ypagui.h"
#include "ypa/ypagameshell.h"
#include "visualstuff/ov_engine.h"
#include "network/networkflags.h"
#include "network/networkclass.h"
#include "audio/cdplay.h"
#include "bitmap/winddclass.h"

WORD merke_scr_x, merke_scr_y;

extern void yw_LoadSaveGameFromMap( struct GameShellReq *GSR );

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_input_engine
_extern_use_ov_engine
_extern_use_audio_engine

extern struct key_info   GlobalKeyTable[ 256 ];
extern struct video_info GlobalVideoTable[ 256 ];
extern UBYTE  **GlobalLocaleHandle;

/*** externe Designdaten ***/
extern WORD ReqDeltaX;
extern WORD ReqDeltaY;
extern WORD IReqWidth;
extern WORD SReqWidth;
extern WORD VReqWidth;
extern WORD LReqWidth;
extern WORD DReqWidth;
extern WORD NReqWidth;
extern WORD NListWidth;
extern WORD gadgetwidth;

/*** Prototypen, nix als Prototypen... ***/
#include "yw_protos.h"
#include "yw_gsprotos.h"
#include "yw_netprotos.h"

#define GET_X_COORD(x) (FLOAT_TO_INT((((FLOAT)x)/640.0)*((FLOAT)ywd->DspXRes)))
#define GET_Y_COORD(y) (FLOAT_TO_INT((((FLOAT)y)/480.0)*((FLOAT)ywd->DspYRes)))

/*** Einheitszeug für Audiozeuch ***/
struct flt_triple null_vec = {0.0, 0.0, 0.0};
struct flt_m3x3      e_dir = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

/*** Screenmodes selbst überwachen? ***/
#ifdef __WINDOWS__
BOOL CheckScreenModes = FALSE;
#else
BOOL CheckScreenModes = TRUE;
#endif

_dispatcher( void, yw_YWM_TRIGGERGAMESHELL, struct GameShellReq *GSR )
{
/*
**  FUNCTION
**
**  INPUT       die tgs-Message enthält Pointer auf eine normale trigger-msg
**              und .... noch viel mehr
**
**  CHANGED     af 14-Apr-96 created
**              25-May-98   floh    Maus setzen rausgenommen
*/

    struct ypaworld_data *ywd;
    BOOL   playing_network = FALSE;
    struct disp_pointer_msg dpm;
    ULONG  shell_mode;

    ywd = INST_DATA( cl, o);

    /*** LastAction löschen ***/
    GSR->GSA.LastAction = A_NOACTION;

    /*** mit Sound anfangen ***/
    _StartAudioFrame( GSR->frame_time, &null_vec, &null_vec, &e_dir );

    /*** spezielle Soundabarbeitung ***/
    yw_PlayShellSamples( GSR );

    _OVE_GetAttrs(OVET_Object,&(ywd->GfxObject),TAG_DONE);
    _methoda(ywd->GfxObject,DISPM_Begin,0);

    /*** ShellMode merken ***/
    shell_mode = GSR->shell_mode;
    GSR->shell_mode_changed = FALSE;

    /*** HandleInput ***/
    yw_HandleGameShell(ywd,GSR);
    
    /*** Shellmode geaendert? ***/
    if( GSR->shell_mode != shell_mode )
        GSR->shell_mode_changed = TRUE;

    /*** n Floh sei Levelzeuch ***/
    yw_TriggerShellBg(ywd,GSR,GSR->input);

    /*** Tooltips und Shell-GUI darstellen ***/
    _methoda(ywd->GfxObject,RASTM_Begin2D,NULL);
    yw_RenderTooltip(ywd);
    
    /*** Layout ***/
    yw_LayoutGameShell(ywd,GSR);
    _methoda(ywd->GfxObject,RASTM_End2D,NULL);

    #ifdef __NETWORK__
    if( ywd->playing_network ) {

        ywd->flush_time -= GSR->frame_time;

        if( ywd->flush_time <= 0 ) {

            /*** Buffer für MSG_ALL-Messages leeren ***/
            struct flushbuffer_msg fb;
            fb.sender_kind         = MSG_PLAYER;
            fb.sender_id           = GSR->NPlayerName;
            fb.receiver_kind       = MSG_ALL;
            fb.receiver_id         = NULL;
            fb.guaranteed          = TRUE;
            _methoda( GSR->ywd->nwo, NWM_FLUSHBUFFER, &fb);

            ywd->flush_time = 100;
            }
        }
    #endif

    /*** Soundaufarbeitung ***/
    _RefreshSoundCarrier( &(GSR->ShellSound1) );
    _RefreshSoundCarrier( &(GSR->ShellSound2) );
    _RefreshSoundCarrier( &(GSR->ChatSound) );
    _EndAudioFrame();

    /*** Für das Einfaden die Palette schwärzen ***/
    if( GSR->JustOpened ) yw_BlackOut( ywd );

    /*** Rendern abschließen ***/
    _methoda(ywd->GfxObject,DISPM_End,NULL);

    /*** Einfaden ? ***/
    if( GSR->JustOpened ) {

        GSR->JustOpened = FALSE;
        yw_FadeIn( ywd );
        }

    /*** von diesem einen Screenshot anfertigen? ***/
    if(yw_CheckCheatKey(GSR->ywd, GSR->input, KEYCODE_NUM_MUL)) yw_ScreenShot( GSR->ywd );

    /* ----------------------------------------------------
    ** Blanker? wenn keine Netzwerkvorbereitungen, d.h. wir
    ** sind schon ueber provider hinaus. 
    ** --------------------------------------------------*/
    playing_network = FALSE;
    if( NM_PROVIDER != GSR->n_selmode) playing_network = TRUE;
    
    /*** Meldung, falls jemand bescheisst ***/
    if( NM_MESSAGE == GSR->n_selmode) {
        yw_TellAboutCheckSum( GSR->ywd );
        yw_CheckCDStatus( GSR );
        }
    
    if( yw_WasInputEvent( GSR->input ) ) GSR->last_input_event = GSR->global_time;
    if( ((GSR->global_time - GSR->last_input_event ) > GSR->wait_til_demo ) &&
        (SHELLMODE_TITLE == GSR->shell_mode) )
        GSR->GSA.LastAction = A_DEMO;

    /*** Der erste Nachspielframe sollte vorbei sein ***/
    GSR->aftergame = FALSE;

    /*** Hilfe aufrufen ***/
    if( ywd->Url ) {

        struct yw_onlinehelp_msg oh;
        oh.url = ywd->Url;
        _methoda( o, YWM_ONLINEHELP, &oh );
        ywd->Url = NULL;
        }
}



void yw_HandleGameShell( struct ypaworld_data *ywd, struct GameShellReq *GSR )
{
/*
** Hier werde ich nochmal was schreiben...ähem...ja doch...irgendwann...
** Hier werden Inputereignisse ausgewertet
** Naja, soviel wollte ich auch nicht wieder schreiben...
*/

    ULONG  ret, who, n;
    struct switchpublish_msg    sp;
    struct setbuttonstate_msg   sbs;
    struct setstring_msg        ss;
    struct selectbutton_msg     sb;
    struct switchbutton_msg     swb;
    char   d_name[ 300 ], helpstring[ 300 ];
    struct setbuttonpos_msg sbp;
    LONG   count;
    struct bt_propinfo *prop;
    struct snd_cdcontrol_msg cd;
    UBYTE  erster[8];
    struct LevelNode *l;
    struct snd_cdvolume_msg cdv;
    struct fileinfonode *node;
    struct getsessionname_msg   gsn;
    struct getplayerdata_msg    gpd;
    BOOL    clear_input = FALSE;
    
    
    /*** Buttonsounds sind allgemein ***/
    if( GSR->input->ClickInfo.flags & CIF_BUTTONDOWN )
        _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_BUTTON );
    
    #ifdef __NETWORK__
    /* -----------------------------------------------------------------
    ** Wenn schon ein Provider gewählt wurde. Messages müssen hier immer
    ** bearbeitet werden!
    ** ---------------------------------------------------------------*/
    if( NM_PROVIDER != GSR->n_selmode )
        yw_HandleNetMessages( GSR->ywd ); // Evtl. in Rahmenprogramm auslagern...

    /*** Wenn wir im Sessionmodus sind, fragen wir ab und zu mal nach ***/
    if( NM_SESSIONS == GSR->n_selmode) {
        if( NWFC_MODEM  == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL)) {

            /*** nur nach Initialisierung fragen ***/
            if(GSR->modem_ask_session)
                _methoda( ywd->nwo, NWM_ASKSESSIONS, NULL );
            }
        else {

            /*** immer fragen. Seriell nur auf Wunsch... ***/
            if( (NWFC_SERIAL != _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL)) ||
                (KEYCODE_SPACEBAR == GSR->input->NormKey) )
                _methoda( ywd->nwo, NWM_ASKSESSIONS, NULL );    
            }
        }
    #endif

/// "Vorbereitungen"

    /* -------------------------------------------------------
    **                Schalten der Gadgets
    ** -----------------------------------------------------*/

    /*** erstmal alles aus- und dann nach Bedarf wieder einschalten ***/
    swb.visible = 0L;
    swb.number = GSID_PL_LOAD;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_PL_SETBACK;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_PL_QUIT;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_PL_GAME;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_PL_FASTFORWARD;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_INPUT;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_VIDEO;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_DISK;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_LOCALE;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_QUIT;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_GAME;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_HELP;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_PL_GOTOLOADSAVE;
    _methoda( GSR->UBalken, BTM_DISABLEBUTTON, &swb );

    #ifdef __NETWORK__
    swb.number = GSID_NET;
    _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
    #endif
    
    /*** erstmal Quit und Start auf ihre normalen Posionen setzen ***/
    sbp.number = GSID_PL_GAME;
    sbp.x      = 0;
    sbp.y      = -1;
    sbp.w      = -1;
    sbp.h      = -1;
    _methoda( GSR->UBalken, BTM_SETBUTTONPOS, &sbp);
    sbp.number = GSID_PL_QUIT;
    sbp.x      = (WORD)(ywd->DspXRes - gadgetwidth);   
    _methoda( GSR->UBalken, BTM_SETBUTTONPOS, &sbp);
    sbp.number = GSID_PL_SETBACK;
    sbp.x      = (WORD)(gadgetwidth + ReqDeltaX);   
    _methoda( GSR->UBalken, BTM_SETBUTTONPOS, &sbp);
    ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_SHELL_GOBACK, "GO BACK");
    ss.pressed_text   = NULL;
    ss.number         = GSID_PL_QUIT;
    _methoda( GSR->UBalken, BTM_SETSTRING, &ss );  

    if( yw_MBActive( ywd ) ) {

        /*** Gadgets (de)aktivieren ***/
        if( (ywd->Level->Status != LEVELSTAT_DEBRIEFING) &&
            (FALSE              == GSR->aftergame) ) {

            swb.number = GSID_PL_GAME;
            _methoda( GSR->UBalken, BTM_ENABLEBUTTON, &swb );
            
            /*** Start auf Endeposition verschieben ***/
            sbp.number = GSID_PL_GAME;
            sbp.x      = (WORD)(ywd->DspXRes - gadgetwidth);   
            _methoda( GSR->UBalken, BTM_SETBUTTONPOS, &sbp);
            
            /*** Quit auf Startposition verschieben ***/
            sbp.number = GSID_PL_QUIT;
            sbp.x      = 0;   
            _methoda( GSR->UBalken, BTM_SETBUTTONPOS, &sbp);
            }

        if( ywd->Level->Status == LEVELSTAT_DEBRIEFING ) {
        
            /*** auch nur noch im Debriefing von noeten ***/
            swb.number = GSID_PL_SETBACK;
            _methoda( GSR->UBalken, BTM_ENABLEBUTTON, &swb );
            
            /*** nach links schieben ***/
            sbp.number = GSID_PL_SETBACK;
            sbp.x      = 0;   
            _methoda( GSR->UBalken, BTM_SETBUTTONPOS, &sbp);
            
            /*** im Debriefing bekommt Back einen anderen Text ***/
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_SHELL_EXITDEBRIEFING, "CONTINUE");
            ss.pressed_text   = NULL;
            ss.number         = GSID_PL_QUIT;
            _methoda( GSR->UBalken, BTM_SETSTRING, &ss );  
           }
        swb.number = GSID_PL_QUIT;
        _methoda( GSR->UBalken, BTM_ENABLEBUTTON, &swb );


        /*** Fenster schließen ***/
        sp.modus = SP_NOPUBLISH;
        _methoda( GSR->binput, BTM_SWITCHPUBLISH, &sp );
        _methoda( GSR->bvideo, BTM_SWITCHPUBLISH, &sp );
        _methoda( GSR->bdisk,  BTM_SWITCHPUBLISH, &sp );
        _methoda( GSR->blocale,BTM_SWITCHPUBLISH, &sp );
        #ifdef __NETWORK__
        _methoda( GSR->bnet,   BTM_SWITCHPUBLISH, &sp );
        #endif
        yw_CloseReq(GSR->ywd, &(GSR->imenu.Req) );
        yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req) );
        yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );
        yw_CloseReq(GSR->ywd, &(GSR->lmenu.Req) );
        #ifdef __NETWORK__
        yw_CloseReq(GSR->ywd, &(GSR->nmenu.Req) );
        #endif
        }
    else {

        /*** Je nach ShellModus Balken einschalten ***/
        if( SHELLMODE_TITLE == GSR->shell_mode ) {

            swb.number = GSID_INPUT;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            swb.number = GSID_VIDEO;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            swb.number = GSID_DISK;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            swb.number = GSID_LOCALE;
            if( GSR->num_languages <= 1 )
                _methoda( GSR->Titel, BTM_DISABLEBUTTON, &swb );
            else
                _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            swb.number = GSID_QUIT;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            swb.number = GSID_HELP;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            swb.number = GSID_GAME;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );

            #ifdef __NETWORK__
            swb.number = GSID_NET;
            _methoda( GSR->Titel, BTM_ENABLEBUTTON, &swb );
            #endif

            /* ----------------------------------------------------------------
            ** Das Load-gadget stellen wir nur dar, wenn es was zu laden gibt.
            ** Wir müssen irgendwie den Level erfragen, sehen, ob Files da
            ** sind. Den test machen wir nur einmal und merken uns das Resultat
            ** Achtung, das Loadgadget ist nicht mehr im MBActive-Teil
            ** --------------------------------------------------------------*/
            }
        else {

            /*** Load und ExtraQuit in PLAY und TUTORIAL ***/
            if( (SHELLMODE_PLAY     == GSR->shell_mode) ||
                (SHELLMODE_TUTORIAL == GSR->shell_mode) ) {

                if( !(GSR->exist_savegame) ) {

                    /*** Testen ***/
                    if( yw_ExistSaveGame( 0, GSR->UserName ) )
                        GSR->exist_savegame = 1;
                    else
                        GSR->exist_savegame = 2;
                    }

                swb.number = GSID_PL_LOAD;
                if( GSR->exist_savegame == 1 ) {
                    _methoda( GSR->UBalken, BTM_ENABLEBUTTON, &swb );
                    }

                swb.number = GSID_PL_QUIT;
                _methoda( GSR->UBalken, BTM_ENABLEBUTTON, &swb );
                swb.number = GSID_PL_GOTOLOADSAVE;
                _methoda( GSR->UBalken, BTM_ENABLEBUTTON, &swb );
                }
            }
        }

///

    /* -----------------------------------------------------------------------
    **                          DER CONFIRMREQUESTER
    ** ---------------------------------------------------------------------*/
    if( CONFIRM_NONE != GSR->confirm_modus ) 
        clear_input = TRUE;
    
    if( ret = _methoda( GSR->confirm, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe? ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {
        
            case GS_CONFIRMOK:
            
                /*** Je nach Modus ***/
                switch( GSR->confirm_modus ) {
                
                    case CONFIRM_LOADFROMMAP:
            
                        yw_LoadSaveGameFromMap( GSR );
                        break;
                        
                    case CONFIRM_NETSTARTALONE:
                    
                        yw_StartNetGame( GSR );
                        break;
                        
                    case CONFIRM_SAVEANDOVERWRITE:
                    
                        yw_EAR_Save( GSR );
                        break;
                        
                    case CONFIRM_MORECDS:
                    
                        /*** nur Bestaetigung. Nix machen ***/
                        yw_CloseConfirmRequester( GSR );
                        break;
                    }
                
                yw_CloseConfirmRequester( GSR );
                break;
                
            case GS_CONFIRMCANCEL:
               
                /*** noch spezielles ***/
                switch( GSR->confirm_modus ) {
                
                    case CONFIRM_SAVEANDOVERWRITE:
                        
                        GSR->D_InputMode = DIM_NONE;
                        break;
                    }
            
                yw_CloseConfirmRequester( GSR );
                break;    
            }
        }
        
    /*** Die tasten ***/
    if( CONFIRM_NONE != GSR->confirm_modus ) {
    
        switch( GSR->input->HotKey ) {
        
            case HOTKEY_QUIT:   
               
                /*** noch spezielles ***/
                switch( GSR->confirm_modus ) {
                
                    case CONFIRM_SAVEANDOVERWRITE:
                        
                        GSR->D_InputMode = DIM_NONE;
                        break;
                    }
            
                yw_CloseConfirmRequester( GSR );
                break;
            }
            
        switch( GSR->input->NormKey ) {
        
            case KEYCODE_RETURN:
            
                switch( GSR->confirm_modus ) {

                    case CONFIRM_LOADFROMMAP:
            
                        yw_LoadSaveGameFromMap( GSR );
                        break;
                        
                    case CONFIRM_NETSTARTALONE:
                    
                        yw_StartNetGame( GSR );
                        break;
                        
                    case CONFIRM_SAVEANDOVERWRITE:
                    
                        yw_EAR_Save( GSR );
                        break;
                        
                    case CONFIRM_MORECDS:
                    
                        /*** nur Bestaetigung. Nix machen ***/
                        yw_CloseConfirmRequester( GSR );
                        break;
                    }

                yw_CloseConfirmRequester( GSR );
                break;
            }
        } 
        
    /*** Input loeschen ***/
    if( clear_input ) {
    
        GSR->input->ClickInfo.flags = 0;
        GSR->input->NormKey  = 0;
        GSR->input->ContKey  = 0;
        GSR->input->AsciiKey = 0;
        GSR->input->HotKey   = 0;
        }

    /* -----------------------------------------------------------------------
    **                             DIE TITELSEITE
    ** ---------------------------------------------------------------------*/

/// "Tasten der Titelleiste"
    /*** Die tasten fuer die Titelleiste ***/
    if( SHELLMODE_TITLE == GSR->shell_mode ) {

        switch( GSR->input->HotKey ) {

            case HOTKEY_QUIT:
                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
                GSR->GSA.LastAction  = A_QUIT;
                _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_QUIT );
                break;

            case HOTKEY_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_ROOT,
                                       "help\\start.html");
                break;
            }
        }
///

/// "Gadgets der Titelleiste"
    /*** Haben gadgets der Titelseite was zu melden? ***/
    if( ret = _methoda( GSR->Titel, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe? ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case GS_QUIT:

                /* ------------------------------------------------
                ** Also raus hier.
                ** ----------------------------------------------*/
                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );

                /*** nach außen melden ***/
                GSR->GSA.LastAction  = A_QUIT;
                _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_QUIT );

                break;

            case GS_GAME_OPEN:

                /*** Inputrequester öffnen ***/
                sp.modus = SP_PUBLISH;
                _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
                GSR->shell_mode = SHELLMODE_PLAY;
                break;

            case GS_INPUT_OPEN:

                /*** Inputrequester öffnen ***/
                yw_OpenInput( GSR );
                break;

            case GS_VIDEO_OPEN:

                yw_OpenSettings( GSR );
                break;

            case GS_DISK_OPEN:

                yw_OpenDisk( GSR );
                GSR->d_fromwhere = 0;
                break;

            case GS_LOCALE_OPEN:

                yw_OpenLocale( GSR );
                break;

            case GS_NET_OPEN:

                yw_OpenNetwork( GSR );
                break;

            case GS_HELP_OPEN:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_ROOT,
                                       "help\\start.html");
                break;
            }
        }
///

    /* --------------------------------------------------------------------
    **                  DER SPIELSCREEN UND DER TUTORIALSCREEN
    ** ------------------------------------------------------------------*/

/// "tasten des Spielscreens"
    /*** Die tasten ***/
    if( (SHELLMODE_PLAY     == GSR->shell_mode) ||
        (SHELLMODE_TUTORIAL == GSR->shell_mode) ) {

        if( HOTKEY_QUIT == GSR->input->HotKey ) {

            if( yw_MBActive( ywd ) ) {

                /*** Shellmode nicht aendern, nur MB ausschalten ***/
                yw_MBCancel( ywd );

                /*** ShellSound wieder einschalten ***/
                if( GSR->ywd->Prefs.Flags & YPA_PREFS_CDSOUNDENABLE ) {

                    cd.command = SND_CD_STOP;
                    _ControlCDPlayer( &cd );

                    if( GSR->shelltrack ) {

                        cd.command   = SND_CD_SETTITLE;
                        cd.para      = GSR->shelltrack;
                        cd.min_delay = ywd->gsr->shell_min_delay;
                        cd.max_delay = ywd->gsr->shell_max_delay;
                        _ControlCDPlayer( &cd );
                        cd.command = SND_CD_PLAY;
                        _ControlCDPlayer( &cd );
                        }
                    }

                /*** Bei bedarf zu Titelscreen ***/
                if( GSR->backtotitle )
                    yw_BackToTitle( GSR );
                }
            else {

                /*** Zurück zum Titelscreen ***/
                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
                sp.modus = SP_PUBLISH;
                _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
                GSR->shell_mode = SHELLMODE_TITLE;
                }
            }
        }
///

/// "Gadgets des Spielscreens"
    /*** Hat unterer Statusbalken was zu melden? ***/
    if( ret = _methoda( GSR->UBalken, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe? ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case GS_QUIT:

                /* ------------------------------------------------
                ** Also raus hier. Wir müssen alles schließen und
                ** melden die Aktion nach außen 
                ** NEU: CancleButton beendet bei laufendem Mission-
                ** briefing erstmal selbiges
                ** ----------------------------------------------*/
                if( yw_MBActive( ywd ) ) {

                    /*** Titelliste zuschalten ***/

                    /*** Abbruch des Missionbriefings ***/
                    yw_MBCancel( ywd );

                    /*** ShellSound wieder einschalten ***/
                    if( GSR->ywd->Prefs.Flags & YPA_PREFS_CDSOUNDENABLE ) {

                        cd.command = SND_CD_STOP;
                        _ControlCDPlayer( &cd );

                        if( GSR->shelltrack) {

                            cd.command   = SND_CD_SETTITLE;
                            cd.para      = GSR->shelltrack;
                            cd.min_delay = ywd->gsr->shell_min_delay;
                            cd.max_delay = ywd->gsr->shell_max_delay;
                            _ControlCDPlayer( &cd );
                            cd.command = SND_CD_PLAY;
                            _ControlCDPlayer( &cd );
                            }
                        }

                    /*** Bei bedarf zu Titelscreen ***/
                    if( GSR->backtotitle )
                        yw_BackToTitle( GSR );
                    }
                else {

                    sp.modus = SP_NOPUBLISH;
                    _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
                    sp.modus = SP_PUBLISH;
                    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
                    GSR->shell_mode = SHELLMODE_TITLE;
                    }

                break;

            case GS_PL_GAME:

                /*** Letzte Arbeiten ***/
                GSR->mb_stopped = FALSE;

                GSR->exist_savegame = 0;

                #ifdef __NETWORK__
                ywd->playing_network = FALSE;
                #endif

                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );

                /*** Spiel starten ***/
                yw_MBStart( ywd );
                break;

            case GS_PL_LOAD:

                /*** Letzte Arbeiten ***/
                if( yw_CheckOlderSaveGame( GSR ) ) 
                    yw_OpenConfirmRequester( GSR, CONFIRM_LOADFROMMAP, 
                        ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_LOADANDOVERWRITE, 
                                    "DO YOU WANT TO LOAD >>>OLDER<<< SAVEGAME?"),0 );
                else
                    yw_OpenConfirmRequester( GSR, CONFIRM_LOADFROMMAP, 
                        ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_LOAD, 
                                    "DO YOU WANT TO LOAD INGAME SAVEGAME?"), 0 );
                break;
                
            case GS_PL_SETBACK:

                GSR->mb_stopped = FALSE;

                /*** Rücksetzen ***/
                yw_MBRewind( ywd );
                break;

            case GS_PL_FASTFORWARD:

                /*** Vorspulen. Is nich mehr ***/
                break;

            case GS_PL_ENDFASTFORWARD:

                /*** damit nach dem Spulen der orig. Zustand wieder da ist ***/
                if( GSR->mb_stopped )
                    yw_MBPause( ywd );
                else
                    yw_MBForward( ywd );
                break;
                
            case GS_PL_GOTOLOADSAVE:
            
                yw_OpenDisk( GSR );
                GSR->d_fromwhere = 1;
                break;
            }
        }

///

    /* --------------------------------------------------------------------
    **                  DER INPUTREQUESTER
    ** ------------------------------------------------------------------*/

/// "Die tasten für den InputRequester"
    /*** Gehen Tasten an uns? ***/
    if( SHELLMODE_INPUT == GSR->shell_mode ) {

        /*** Hotkeys? VOR Normkey, da changemode dann zurueckgesetzt! ***/
        if( GSR->input->HotKey && (GSR->input_changemode == FALSE) ) {

            switch( GSR->input->HotKey ) {

                case HOTKEY_HELP:


                    ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_INPUTCONFIG,
                                           "help\\19.html");
                    break;
                }
            }

        if( GSR->input->NormKey ) {

            /* ------------------------------------------------------------
            ** Es gibt 2 Modi, zum einen die normale Listview-Navigation
            ** und zum anderen der Modus zum Aendern einer Aktion. Im
            ** 2. Modus muessen wir die ListView-Cursor-Navigation explizit
            ** ausschalten
            ** ----------------------------------------------------------*/
            if( GSR->input_changemode ) {

                GSR->imenu.Flags &= ~LISTF_KeyboardInput;

                /* -----------------------------------------
                ** Eine normale Taste. unterstützen wir die? 
                ** ---------------------------------------*/
                if( GlobalKeyTable[ GSR->input->NormKey ].config ) {

                    /*** Auswerten ***/
                    if( GSR->i_selfirst ) {

                        GSR->inp[ GSR->i_actualitem ].pos = GSR->input->NormKey;
                    
                        /*** Markierung umschalten ***/
                        if( GSR->inp[ GSR->i_actualitem ].kind == GSI_SLIDER ) {

                            /*** Status umschalten ***/
                            GSR->i_selfirst = FALSE;
                            }

                        GSR->input_changemode = FALSE;
                        GSR->inp[ GSR->i_actualitem ].done = 0;
                        }
                    else {

                        GSR->inp[ GSR->i_actualitem ].neg = GSR->input->NormKey;
                    
                        /*** Status umschalten ***/
                        GSR->inp[ GSR->i_actualitem ].done &= ~IF_SECOND;
                        GSR->i_selfirst                     = TRUE;
                        }
                    }

                /*** Die taste löschen ***/
                GSR->input->NormKey = 0;
                }
            else {

                GSR->imenu.Flags |= LISTF_KeyboardInput;

                /*** Normaler CursorNavigationsmode ***/
                switch( GSR->input->NormKey ) {

                    case KEYCODE_RETURN:

                        /* ---------------------------------------------
                        ** Als "zu aendernd" kennzeichnen und changemode
                        ** einschalten
                        ** -------------------------------------------*/
                        GSR->inp[ GSR->i_actualitem ].done = (IF_FIRST|IF_SECOND);
                        GSR->input_changemode = TRUE;
                        if( GSR->inp[ GSR->i_actualitem ].kind == GSI_SLIDER )
                            GSR->i_selfirst = FALSE;
                        break;

                    case KEYCODE_ESCAPE:

                        yw_CancelInput( GSR );
                        break;

                    case KEYCODE_BS:
                    case KEYCODE_DEL:

                        /*** Loeschen eines Eintrages ***/

                        /*** Derzeit Slider noch rausfiltern ***/
                        if( GSR->inp[ GSR->i_actualitem ].kind != GSI_SLIDER )
                            GSR->inp[ GSR->i_actualitem ].pos = 0;
                        break;
                    }
                }
            }
        }
///

/// "Gadgetaktualisierungen"
    /* -----------------------------------------------------------------------
    ** Nacharbeit: Wenn QuealifierTasten als normale tasten verwendet werden, 
    ** dann die Gadgets aus, damit sie nicht mehr als Qualifier genutzt werden
    ** können! 
    ** Bei Hotkeys schalten wir alles aus!
    ** ---------------------------------------------------------------------*/
    if( GSR->inp[ GSR->i_actualitem ].kind != GSI_SLIDER ) {

        swb.visible = 0L;

        /*** ausschalten ***/
        swb.number = GSID_SWITCHOFF;
        _methoda( GSR->binput, BTM_ENABLEBUTTON, &swb);
        }
    else {

        swb.visible = 0L;
        
        swb.number = GSID_SWITCHOFF;
        _methoda( GSR->binput, BTM_DISABLEBUTTON, &swb);
        }
///

/// "Auswertung der InputGadgets"

    /*** Hat InputRequester was zu melden ? ***/
    if( ret = _methoda( GSR->binput, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case GS_INPUTCANCEL:

                yw_CancelInput( GSR );
                break;

            case GS_USEJOYSTICK:

                GSR->new_joystick   = TRUE;
                GSR->input_changed |= ICF_JOYSTICK;
                break;

            case GS_NOJOYSTICK:

                GSR->new_joystick   = FALSE;
                GSR->input_changed |= ICF_JOYSTICK;
                break;

            case GS_ALTJOYSTICK_YES:

                GSR->new_altjoystick   = TRUE;
                GSR->input_changed    |= ICF_ALTJOYSTICK;
                break;

            case GS_ALTJOYSTICK_NO:

                GSR->new_altjoystick   = FALSE;
                GSR->input_changed    |= ICF_ALTJOYSTICK;
                break;

            case GS_USEFORCEFEEDBACK:

                GSR->new_forcefeedback = TRUE;
                GSR->input_changed    |= ICF_FORCEFEEDBACK;
                break;

            case GS_NOFORCEFEEDBACK:

                GSR->new_forcefeedback = FALSE;
                GSR->input_changed    |= ICF_FORCEFEEDBACK;
                break;

            case GS_INPUTOK:

                /*** Joystickeinstellungen uebernehmen ***/
                if( GSR->input_changed & ICF_JOYSTICK ) {

                    GSR->joystick = GSR->new_joystick;
                    if( GSR->joystick )
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_JOYDISABLE;
                    else
                        GSR->ywd->Prefs.Flags |=  YPA_PREFS_JOYDISABLE;
                    }

                if( GSR->input_changed & ICF_ALTJOYSTICK ) {

                    GSR->altjoystick = GSR->new_altjoystick;
                    if( GSR->new_altjoystick )
                        GSR->ywd->Prefs.Flags |=  YPA_PREFS_JOYMODEL2;
                    else
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_JOYMODEL2;
                    }

                if( GSR->input_changed & ICF_FORCEFEEDBACK ) {

                    if( GSR->new_forcefeedback )
                        GSR->ywd->Prefs.Flags &= ~YPA_PREFS_FFDISABLE;
                    else
                        GSR->ywd->Prefs.Flags |=  YPA_PREFS_FFDISABLE;
                    }

                GSR->input_changed = 0L;

                /*** Alle tasten setzen und requester schließen ***/
                yw_SetAllKeys( GSR->ywd->world, GSR );
                yw_NoticeKeys( GSR );

                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->binput, BTM_SWITCHPUBLISH, &sp );
                sp.modus = SP_PUBLISH;
                _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
                yw_CloseReq(GSR->ywd, &(GSR->imenu.Req) );

                GSR->shell_mode = SHELLMODE_TITLE;

                break;

            case GS_INPUTRESET:

                /*** Alle tasten rücksetzen. Nicht schließen ***/
                yw_RestoreStandardKeys( GSR );

                break;

            case GS_SWITCHOFF:

                /*** Zur Position 0 gibt es kein ".config" ***/
                GSR->inp[ GSR->i_actualitem ].pos = 0;
                break;

            case GS_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_INPUTCONFIG,
                                       "help\\19.html");
                GSR->input_changemode = FALSE;
                break;
            }
        }
///

/// "Auswerten des Input-Menüs"
    /*** Das Menü des EingabeRequesters (VOR dem Req?) ***/
    if( SHELLMODE_INPUT == GSR->shell_mode ) {

        LONG old_actualitem;

        /*** Auswertung ***/
        yw_ListHandleInput( ywd, &(GSR->imenu), GSR->input );

        old_actualitem    = GSR->i_actualitem;
        GSR->i_actualitem = GSR->imenu.Selected + 1;

        if( GSR->imenu.Flags & LISTF_Select ) {

            /* -----------------------------------------------
            ** Evtl. noch bestehenden "input_changemode"  aus-
            ** schalten.
            ** ---------------------------------------------*/
            GSR->inp[ old_actualitem ].done = 0;
            GSR->i_selfirst                 = TRUE;
            GSR->input_changemode           = FALSE;
            }
        
        
        /*** Muß halt so sein ***/
        yw_ListLayout( ywd, &(GSR->imenu) );
        }
///


    /*--------------------------------------------------------------------
    **               DER SETTINGSREQUESTER
    **------------------------------------------------------------------*/

/// "Auswertung der Settingstasten"
    /*** tasten ***/
    if( SHELLMODE_SETTINGS == GSR->shell_mode ) {

        switch( GSR->input->NormKey ) {

            case KEYCODE_ESCAPE:

                yw_CancelSettings( GSR );
                GSR->shell_mode = SHELLMODE_TITLE;
                break;

            case KEYCODE_RETURN:

                /*** Wenn Menu offen, dann auswaehlen, sonst OK-Funktion ***/
                if( (GSR->vmenu.Req.flags & REQF_Closed) &&
                    (GSR->d3dmenu.Req.flags & REQF_Closed) ) {

                    /*** Requester mit OK schliessen ***/
                    yw_OKSettings( GSR );
                    GSR->shell_mode = SHELLMODE_TITLE;
                    }

                if( !(GSR->vmenu.Req.flags & REQF_Closed) ) {

                    /*** Videomode auswaehlen ***/
                    if( !(GSR->vmenu.Req.flags & REQF_Closed) )
                        yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req) );

                    /*** Wenn zu, dann auch gadget "entpressen" ***/
                    if( GSR->vmenu.Req.flags & REQF_Closed ) {

                        sbs.state = SBS_UNPRESSED;
                        sbs.who   = GSID_VMENU_BUTTON;
                        _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
                        }

                    GSR->settings_changed |=  SCF_MODE;
                    yw_AppearVideoMode( GSR, FALSE );
                    }

                if( !(GSR->d3dmenu.Req.flags & REQF_Closed) ) {
                    }

                break;
            }

        switch( GSR->input->HotKey ) {

            case HOTKEY_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_SETTINGS,
                                       "help\\110.html");
                break;
            }
        }
///

/// "Auswertung der Settings Gadgets"

    if( ret = _methoda( GSR->bvideo, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case GS_SETTINGSCANCEL:

                yw_CancelSettings( GSR );
                break;

            case GS_SETTINGSOK:

                yw_OKSettings( GSR );
                break;

            case GS_CDSOUND_YES:

                GSR->new_sound_flags  |=  SF_CDSOUND;
                GSR->settings_changed |=  SCF_CDSOUND;
                break;

            case GS_CDSOUND_NO:

                GSR->new_sound_flags  &= ~SF_CDSOUND;
                GSR->settings_changed |=  SCF_CDSOUND;
                break;

            case GS_SOUND_LR:

                GSR->settings_changed |=  SCF_INVERT;
                GSR->new_sound_flags  &= ~SF_INVERTLR;

                break;

            case GS_SOUND_RL:

                GSR->new_sound_flags  |=  SF_INVERTLR;
                GSR->settings_changed |=  SCF_INVERT;

                break;

            case GS_SOUND_HITFXVOLUME:

                /*** Drücken des Sliders --> Sound einschalten ***/
                _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_VOLUME );
                GSR->settings_changed |= SCF_FXVOLUME;
                break;

            case GS_SOUND_HOLDFXVOLUME:

                /*** Halten des Sliders --> Lautstärke ***/
                break;

            case GS_SOUND_RELEASEFXVOLUME:

                /*** Loslassen des Sliders --> Sound ausschalten ***/
                _EndSoundSource( &(GSR->ShellSound1), SHELLSOUND_VOLUME );
                break;

            case GS_SOUND_HITCDVOLUME:

                /*** Drücken des Sliders --> Track ist ShellTrack ***/
                GSR->settings_changed |= SCF_CDVOLUME;
                break;

            case GS_SOUND_HOLDCDVOLUME:

                /*** Halten des Sliders ***/
                break;

            case GS_SOUND_RELEASECDVOLUME:

                /*** Loslassen des Sliders  ***/
                break;

            // Videospezifische Sachen
            case GS_VMENU_OPEN:

                yw_OpenReq(ywd, &(GSR->vmenu.Req) );
                _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_MENUOPEN );

                GSR->input->ClickInfo.flags &= ~CIF_MOUSEDOWN;
                break;

            case GS_VMENU_CLOSE:

                yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req) );
                break;

            case GS_3DMENU_OPEN:

                yw_OpenReq(ywd, &(GSR->d3dmenu.Req) );
                _StartSoundSource( &(GSR->ShellSound1), SHELLSOUND_MENUOPEN );

                GSR->input->ClickInfo.flags &= ~CIF_MOUSEDOWN;
                break;

            case GS_3DMENU_CLOSE:

                yw_CloseReq(GSR->ywd, &(GSR->d3dmenu.Req) );
                break;

            case GS_FARVIEW:

                GSR->new_video_flags  |=  VF_FARVIEW;
                GSR->settings_changed |=  SCF_FARVIEW;
                break;

            case GS_NOFARVIEW:

                GSR->new_video_flags  &= ~VF_FARVIEW;
                GSR->settings_changed |=  SCF_FARVIEW;
                break;

            case GS_HEAVEN:

                GSR->new_video_flags  |=  VF_HEAVEN;
                GSR->settings_changed |=  SCF_HEAVEN;
                break;

            case GS_NOHEAVEN:

                GSR->new_video_flags  &= ~VF_HEAVEN;
                GSR->settings_changed |=  SCF_HEAVEN;
                break;


            case GS_16BITTEXTURE_YES:

                GSR->new_video_flags  |=  VF_16BITTEXTURE;
                GSR->settings_changed |=  SCF_16BITTEXTURE;
                break;

            case GS_16BITTEXTURE_NO:

                GSR->new_video_flags  &= ~VF_16BITTEXTURE;
                GSR->settings_changed |=  SCF_16BITTEXTURE;
                break;


            case GS_DRAWPRIMITIVE_YES:

                GSR->new_video_flags  |=  VF_DRAWPRIMITIVE;
                GSR->settings_changed |=  SCF_DRAWPRIMITIVE;
                break;

            case GS_DRAWPRIMITIVE_NO:

                GSR->new_video_flags  &= ~VF_DRAWPRIMITIVE;
                GSR->settings_changed |=  SCF_DRAWPRIMITIVE;
                break;


            case GS_SOFTMOUSE:

                GSR->new_video_flags  |=  VF_SOFTMOUSE;
                GSR->settings_changed |=  SCF_SOFTMOUSE;
                break;

            case GS_NOSOFTMOUSE:

                GSR->new_video_flags  &= ~VF_SOFTMOUSE;
                GSR->settings_changed |=  SCF_SOFTMOUSE;
                break;

            case GS_FXSLIDERHIT:

                GSR->settings_changed |= SCF_FXNUMBER;
                break;

            case GS_ENEMY_YES:

                GSR->new_enemyindicator = TRUE;
                GSR->settings_changed  |= SCF_ENEMYINDICATOR;
                break;

            case GS_ENEMY_NO:

                GSR->new_enemyindicator = FALSE;
                GSR->settings_changed  |= SCF_ENEMYINDICATOR;
                break;

            case GS_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_SETTINGS,
                                       "help\\110.html");
                break;
            }
        }
///

/// "Auswertung des VideoMenüs"
    /*** Das Menü des Videorequesters ***/
    if( (SHELLMODE_SETTINGS == GSR->shell_mode) &&
        (!(GSR->vmenu.Req.flags & REQF_Closed)) ) {
    
        FLOAT restbreite;

        restbreite = VReqWidth - 3 * ReqDeltaX - ywd->PropW;

        yw_ListHandleInput( ywd, &(GSR->vmenu), GSR->input );

        if( GSR->vmenu.Flags & LISTF_SelectDone ) {

            BOOL   remotestart = FALSE;

            #ifdef __NETWORK__
            if( ywd->gsr->remotestart ) remotestart = TRUE;
            #endif

            /*** Das muss dann bei "OK" beachtet werden ***/
            GSR->settings_changed |=  SCF_MODE;

            /* ----------------------------------------------------
            ** 1. Müssen wir überhaupt was ändern? --> Act != sel
            ** 2. Dürfen wir überhaupt was ändern? --> !remotestart
            ** --------------------------------------------------*/
            yw_AppearVideoMode( GSR, remotestart );
            }

        /*** Wenn zu, dann auch gadget "entpressen" ***/
        if( GSR->vmenu.Req.flags & REQF_Closed ) {

            sbs.state = SBS_UNPRESSED;
            sbs.who   = GSID_VMENU_BUTTON;
            _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
            }
        
        /*** Muß halt so sein ***/
        yw_ListLayout( ywd, &(GSR->vmenu) );
        }
///

/// "Auswertung des D3DMenüs"
    /*** Das Menü des Videorequesters ***/
    if( (SHELLMODE_SETTINGS == GSR->shell_mode) &&
        (!(GSR->d3dmenu.Req.flags & REQF_Closed)) ) {
    
        FLOAT restbreite;

        restbreite = VReqWidth - 3 * ReqDeltaX - ywd->PropW;

        yw_ListHandleInput( ywd, &(GSR->d3dmenu), GSR->input );

        if( GSR->d3dmenu.Flags & LISTF_SelectDone ) {

            BOOL   remotestart = FALSE;

            #ifdef __NETWORK__
            if( ywd->gsr->remotestart ) remotestart = TRUE;
            #endif

            /*** Das muss dann bei "OK" beachtet werden ***/
            GSR->settings_changed |=  SCF_3DDEVICE;

            yw_Appear3DDevice( GSR, remotestart );
            }

        /*** Wenn zu, dann auch gadget "entpressen" ***/
        if( GSR->d3dmenu.Req.flags & REQF_Closed ) {

            sbs.state = SBS_UNPRESSED;
            sbs.who   = GSID_3DMENU_BUTTON;
            _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
            }
        
        /*** Muß halt so sein ***/
        yw_ListLayout( ywd, &(GSR->d3dmenu) );
        }
///

/// "Nacharbeit für Slider"
    /* -------------------------------------------------------------
    ** Nacharbeit, wir holen uns die Werte der Slider und setzen die
    ** zugehörigen textgadgets
    ** -----------------------------------------------------------*/

    // VideoSlider
    sb.number = GSID_FXSLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    
    sprintf( helpstring, "%d\0", prop->value );
    ss.pressed_text = ss.unpressed_text = helpstring;
    ss.number = GSID_FXNUMBER;
    _methoda( GSR->bvideo, BTM_SETSTRING, &ss );
    GSR->new_destfx = prop->value;

    // Sound-Slider
    sb.number = GSID_FXVOLUMESLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    
    sprintf( helpstring, "%d\0", prop->value );
    ss.pressed_text = ss.unpressed_text = helpstring;
    ss.number = GSID_FXVOLUMENUMBER;
    _methoda( GSR->bvideo, BTM_SETSTRING, &ss );
    GSR->new_fxvolume = prop->value;
    _AE_SetAttrs( AET_MasterVolume, (LONG)(GSR->new_fxvolume), TAG_DONE );

    sb.number = GSID_CDVOLUMESLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    
    sprintf( helpstring, "%d\0", prop->value );
    ss.pressed_text = ss.unpressed_text = helpstring;
    ss.number = GSID_CDVOLUMENUMBER;
    _methoda( GSR->bvideo, BTM_SETSTRING, &ss );
    GSR->new_cdvolume = prop->value;
    cdv.volume        = GSR->new_cdvolume;
    _SetCDVolume( &cdv );
///


    /* ------------------------------------------------------------------
    **                       DER DISKREQUESTER
    ** ----------------------------------------------------------------*/
    
/// "Tasten für den DiskRequester"

    /*** Tasten? --> Stringadget ***/
    if( SHELLMODE_DISK == GSR->shell_mode ) {

        /* -------------------------------------------------------------------
        ** Mit dem neuen Konzept, wo das Stringgadget nur zu bestimmten Zeiten
        ** aktiv ist, kann ich es machen, die Cursortasten zur Listennavi-
        ** gation zu nehmen. Ansonsten bleibt alles beim alten, ledig-
        ** lich die Entertaste wird als Abschluß des Stringgadgets und damit
        ** als "NEW" ausgewertet.
        ** -----------------------------------------------------------------*/

        if( GSR->input->NormKey || GSR->input->AsciiKey ) {

            if( DIM_NONE != GSR->D_InputMode ) {

                int    len;
                struct switchbutton_msg sb;
                BOOL   own_input;

                sb.visible = 1L;
                if( GSR->ywd->UseSystemTextInput )
                    own_input = FALSE;
                else
                    own_input = TRUE;

                /* --------------------------------------------
                ** Was sind denn das für noch für SonderTasten?
                ** Im UseSystemtextInput-Mode gilt nur enter
                ** ------------------------------------------*/
                switch( GSR->input->NormKey ) {

                    case KEYCODE_CURSOR_LEFT:

                        /*** Cursor links ***/
                        if( (GSR->DCursorPos > 0) && own_input )
                            GSR->DCursorPos--;
                        break;

                    case KEYCODE_CURSOR_RIGHT:

                        /*** Cursor nach rechts ***/
                        len = strlen( GSR->D_Name );
                        if( (GSR->DCursorPos < len) && own_input )
                            GSR->DCursorPos++;
                        break;

                    case KEYCODE_DEL:

                        /*** Löschen Zeichen nach Cursor ***/
                        if( (GSR->DCursorPos < strlen( GSR->D_Name)) &&
                            (own_input) ) {

                            strcpy( &(GSR->D_Name[ GSR->DCursorPos ]), 
                                    &(GSR->D_Name[ GSR->DCursorPos + 1]) );
                            }
                        break;

                   

                    case KEYCODE_BS:

                        /*** Zeichen vor Cursor löschen ***/
                        if( (GSR->DCursorPos > 0) && own_input ) {

                            strcpy( &(GSR->D_Name[ GSR->DCursorPos - 1 ]),
                                    &(GSR->D_Name[ GSR->DCursorPos ]) );
                            GSR->DCursorPos--;
                            }
                        break;

                    case KEYCODE_RETURN:

                        /*** Je nach Modus... ***/
                        switch( GSR->D_InputMode ) {

                            case DIM_KILL:

                                yw_EAR_Kill( GSR );
                                break;

                            case DIM_LOAD:

                                yw_EAR_Load( GSR );
                                break;

                            case DIM_SAVE:
                            
                                /*** Existiert das schon? ***/
                                if( GSR->d_actualitem )
                                    yw_OpenConfirmRequester( GSR, CONFIRM_SAVEANDOVERWRITE,
                                    ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_SAVEANDOVERWRITE,
                                    "DO YOU WANT TO OVERWRITE THIS PLAYER STATUS?"),0);
                                else
                                    yw_EAR_Save( GSR );
                                break;

                            case DIM_CREATE:

                                yw_EAR_Create( GSR );
                                break;
                            }
                        break;

                    case KEYCODE_ESCAPE:

                        /*** Schaltet zurück auf Nicht-InputMode ***/
                        GSR->D_InputMode = DIM_NONE;
                        break; 
                    }


                /*** allgemeine Buchstabentaste? ***/
                if( (strlen( GSR->D_Name ) < USERNAMELEN) &&
                    (GSR->input->AsciiKey > 31) && (own_input)) {

                    char ascii_in[10], ascii_out[10];

                    /* ---------------------------------------------------------------
                    ** Was nicht darstellbar ist, sollte auch nicht als * ausgegeben
                    ** werden. Folglich jagen wir es durch Floh's Filterroutine und
                    ** ignorieren alles, was als Stern rauskam, aber nicht als solcher
                    ** rein ist!
                    ** -------------------------------------------------------------*/
                    sprintf( ascii_in, "%c\0", GSR->input->AsciiKey );
                    yw_LocStrCpy( ascii_out, ascii_in );
                    if( (!( (ascii_in[0] != '*') && (ascii_out[0] == '*') )) &&
                        (yw_IsOKForFilename( ascii_in[0] )) ) {

                        /*** Buchstabe einsortieren ***/
                        strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
                        strncpy( &(d_name[ GSR->DCursorPos ]), ascii_out, 1 );
                        strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &( GSR->D_Name[ GSR->DCursorPos]) );
                        strcpy( GSR->D_Name, d_name );

                        GSR->DCursorPos++;  // Stringgadget unten neu
                        }
                    }

                }
            else {

                /*** Im None-Input-Mode ***/

                switch( GSR->input->NormKey ) {

                    case KEYCODE_ESCAPE:

                        yw_CancelDisk( GSR );
                        break;
                    }

                /*** Im None-Inputmode kann HelpHotkey erfragt werden ***/
                switch( GSR->input->HotKey ) {

                    case HOTKEY_HELP:

                        ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_SAVEGAME,
                                               "help\\18.html");
                        break;
                    }
                }


            /*** Falls was ausgewählt, richtig positionieren ***/
            if( GSR->d_actualitem )
                yw_PosSelected( &(GSR->dmenu), GSR->d_actualitem - 1 );
            }
        }

    /* -------------------------------------------------------
    ** Wir haben den Gadgetinhalt verändert. Falls wir einen
    ** Namen eingegeben haben, der schon existiert, dann wird
    ** dieser zur optischen Rückkopplung in der Liste
    ** gehighlighted.
    ** NEU: Ich mache diesen test jetzt immer, weil Namen auch
    ** nur gesetzt nwerden können (nicht an tasten gebunden)
    ** -----------------------------------------------------*/
    GSR->d_actualitem = 0;
    for( node = (struct fileinfonode *) GSR->flist.mlh_Head, n = 1;
         node->node.mln_Succ;
         node = (struct fileinfonode *) node->node.mln_Succ, n++ )
         
         if( stricmp( node->username, GSR->D_Name ) == 0 ) {
             GSR->d_actualitem = n;
             break;
             }

///
    
/// "Gadgets des DiskRequesters"
    /*** Hat Ein-/AusgabeRequester was zu melden ? ***/
    if( ret = _methoda( GSR->bdisk, BTM_HANDLEINPUT, GSR->input ) ) {

        struct fileinfonode *new_node;
        int    new_count;
        char  *new_user;

        /*** Hilfe ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case PRA_RQ_CLOSE:

                yw_CancelDisk( GSR );
                break;

            case GS_DISKCANCEL:

                /*** Schaltet zurück auf Nicht-InputMode ***/
                if( GSR->D_InputMode != DIM_NONE )
                    GSR->D_InputMode = DIM_NONE;
                else
                    yw_CancelDisk( GSR );
                break;

            case GS_DISKOK:

                /*** Je nach Modus... ***/
                switch( GSR->D_InputMode ) {

                    case DIM_KILL:

                        yw_EAR_Kill( GSR );
                        break;

                    case DIM_LOAD:

                        yw_EAR_Load( GSR );
                        break;

                    case DIM_SAVE:
                      if( GSR->d_actualitem )
                            yw_OpenConfirmRequester( GSR, CONFIRM_SAVEANDOVERWRITE,
                            ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_SAVEANDOVERWRITE,
                            "DO YOU WANT TO OVERWRITE THIS PLAYER STATUS?"),0);
                        else
                            yw_EAR_Save( GSR );
                        break;

                    case DIM_CREATE:

                        yw_EAR_Create( GSR );
                        break;
                    }
                break;

            case GS_DELETE:

                GSR->D_InputMode = DIM_KILL;

                if( !GSR->d_actualitem )
                    strcpy(GSR->D_Name, UNKNOWN_NAME );
                GSR->DCursorPos   = strlen( GSR->D_Name );
                strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
                strncpy( &(d_name[ GSR->DCursorPos ]), CURSORSIGN, 1 );
                strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
                ss.unpressed_text = d_name;
                ss.pressed_text   = NULL;
                ss.number         = GSID_DISKSTRING;
                _methoda( GSR->bdisk, BTM_SETSTRING, &ss );
                break;

            
            case GS_LOAD:

                GSR->D_InputMode = DIM_LOAD;

                if( !GSR->d_actualitem )
                    strcpy(GSR->D_Name, UNKNOWN_NAME );
                GSR->DCursorPos   = strlen( GSR->D_Name );
                strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
                strncpy( &(d_name[ GSR->DCursorPos ]), CURSORSIGN, 1 );
                strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
                ss.unpressed_text = d_name;
                ss.pressed_text   = NULL;
                ss.number         = GSID_DISKSTRING;
                _methoda( GSR->bdisk, BTM_SETSTRING, &ss );

                break;

            case GS_NEWGAME:

                /* -------------------------------------------------
                ** Je nach InputModus bereiten wir die texteingabe
                ** vor (klassisch) oder fragen gleich ab (japanisch)
                ** Texteingabe ist nur bei NEW und SAVE wirklich
                ** notwendig.
                ** -----------------------------------------------*/
                GSR->D_InputMode = DIM_CREATE;

                /*** Um versehentliches Loeschen zu vermeiden Standardname ***/
                new_user  = ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_NEWUSER,
                                        "NEW GAME");
                new_count = 0;
                new_node = (struct fileinfonode *) GSR->flist.mlh_Head;
                while( new_node->node.mln_Succ ) {
                    if( strnicmp( new_user, new_node->username, strlen(new_user))==0) {

                        /*** Nummer ermitteln und vergleichen ***/
                        if( new_count < atoi( &(new_node->username[ strlen(new_user) ])) )
                            new_count = atoi( &(new_node->username[ strlen(new_user) ]));
                        }
                    new_node = (struct fileinfonode *) new_node->node.mln_Succ;
                    }
                sprintf( GSR->D_Name, "%s%d", new_user, new_count + 1 );

                if( GSR->ywd->UseSystemTextInput ) {

                    struct windd_gettext gt;

                    gt.title_text   = ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_ENTERNAME, "ENTER NAME");
                    gt.ok_text      = ypa_GetStr( GlobalLocaleHandle, STR_OK, "OK");
                    gt.cancel_text  = ypa_GetStr( GlobalLocaleHandle, STR_CANCEL, "CANCEL");
                    gt.default_text = GSR->D_Name;
                    gt.timer_func   = NULL;
                    _methoda( GSR->ywd->GfxObject, WINDDM_GetText, &gt );
                    if( gt.result ) {

                        /*** result ist uebergebener Text ***/
                        strcpy( GSR->D_Name, gt.result );
                        }
                    else {

                        /*** Abgebrochen ***/
                        GSR->D_InputMode = DIM_NONE;
                        }
                    }
                else {

                    GSR->DCursorPos   = strlen( GSR->D_Name );
                    strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
                    strncpy( &(d_name[ GSR->DCursorPos ]), CURSORSIGN, 1 );
                    strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
                    ss.unpressed_text = d_name;
                    ss.pressed_text   = NULL;
                    ss.number         = GSID_DISKSTRING;
                    _methoda( GSR->bdisk, BTM_SETSTRING, &ss );
                    }
                break;

            case GS_SAVE:

                GSR->D_InputMode = DIM_SAVE;

                if( GSR->ywd->UseSystemTextInput ) {

                    struct windd_gettext gt;

                    gt.title_text   = ypa_GetStr( GlobalLocaleHandle, STR_DGADGET_ENTERNAME, "ENTER NAME");
                    gt.ok_text      = ypa_GetStr( GlobalLocaleHandle, STR_OK, "OK");
                    gt.cancel_text  = ypa_GetStr( GlobalLocaleHandle, STR_CANCEL, "CANCEL");
                    gt.default_text = GSR->D_Name;
                    gt.timer_func   = NULL;
                    _methoda( GSR->ywd->GfxObject, WINDDM_GetText, &gt );
                    if( gt.result ) {

                        /*** result ist uebergebener Text ***/
                        strcpy( GSR->D_Name, gt.result );
                        }
                    else {

                        /*** Abgebrochen ***/
                        GSR->D_InputMode = DIM_NONE;
                        }
                    }
                else {

                    if( !GSR->d_actualitem )
                        strcpy(GSR->D_Name, UNKNOWN_NAME );
                    GSR->DCursorPos   = strlen( GSR->D_Name );
                    strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
                    strncpy( &(d_name[ GSR->DCursorPos ]), CURSORSIGN, 1 );
                    strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
                    ss.unpressed_text = d_name;
                    ss.pressed_text   = NULL;
                    ss.number         = GSID_DISKSTRING;
                    _methoda( GSR->bdisk, BTM_SETSTRING, &ss );
                    }

                break;

            case GS_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_SAVEGAME,
                                       "help\\18.html");
                GSR->D_InputMode = DIM_NONE;
                break;

                break;                                   
            }
        }
///

/// "Menü des DiskRequesters"
    /*** Das Menü, also die Fileliste / 0 heißt nix ausgewählt!!!!! ***/
    if( SHELLMODE_DISK == GSR->shell_mode ) {

        int    n;
        struct fileinfonode *nd;

        yw_ListHandleInput( ywd, &(GSR->dmenu), GSR->input );

        if( (GSR->dmenu.Flags     & LISTF_Select) ||
            (GSR->input->NormKey == KEYCODE_CURSOR_UP) ||
            (GSR->input->NormKey == KEYCODE_CURSOR_DOWN) ) {

            /*** Es wurde eine Action zur Änderung ausgewählt ***/
            GSR->d_actualitem = GSR->dmenu.Selected + 1;

            /*** Sperre, falls Selected mal querschießt ***/
            if( 1 > GSR->d_actualitem ) GSR->d_actualitem = 1;
            if( GSR->dmenu.NumEntries < GSR->d_actualitem )
                GSR->d_actualitem = GSR->dmenu.NumEntries;

            /*** Name suchen ***/
            n = GSR->d_actualitem - 1;
            nd = (struct fileinfonode *) GSR->flist.mlh_Head;
            while( n-- ) {

                if( nd->node.mln_Succ )
                    nd = (struct fileinfonode *) nd->node.mln_Succ;
                else {
                    
                    /*** Da war ein Fehler ***/
                    nd = (struct fileinfonode *) nd->node.mln_Succ;
                    GSR->d_actualitem = 0;
                    break;
                    }
                }


            strcpy( GSR->D_Name, nd->username );
            GSR->DCursorPos = strlen( GSR->D_Name );
            }                                                                    
        
        /*** Muß halt so sein ***/
        yw_ListLayout( ywd, &(GSR->dmenu) );
        }

///

/// "Nacharbeit zum Diskrequester"
    /*** Nacharbeit: Je nach InputMode ***/
    if( DIM_NONE == GSR->D_InputMode ) {

        /*** 4 Auswahl-Gadgets an, OK und CANCEl weg ***/
        swb.visible = 0;
        swb.number  = GSID_DISKOK;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        swb.number  = GSID_SAVE;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );
        swb.number  = GSID_LOAD;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );
        swb.number  = GSID_NEWGAME;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );
        swb.number  = GSID_DISKSTRING;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        
        /*** Delete nur wenn nicht der eigene ***/
        swb.number  = GSID_DELETE;
        if( stricmp( GSR->D_Name, GSR->UserName ) == 0 )
            _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        else
            _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );

        /*** Text ohne Cursor ***/
        strcpy( d_name, GSR->D_Name );
        }
    else {

        swb.number  = GSID_DISKOK;
        _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );
        swb.visible = 0;
        swb.number  = GSID_SAVE;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        swb.number  = GSID_LOAD;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        swb.number  = GSID_DELETE;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        swb.number  = GSID_NEWGAME;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
        swb.number  = GSID_DISKSTRING;
        _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );

        /* ------------------------------------------------------------
        ** Wenn der Username in D_Name  steht und wir im KillMode sind,
        ** dann OK-Gadgetausschalten
        ** ----------------------------------------------------------*/
        if( DIM_KILL == GSR->D_InputMode ) {

            swb.number = GSID_DISKOK;
            swb.visible = 0;

            if( GSR->d_actualitem ) {

                /*** ist es zufaellig der derzeitige? ***/
                if( stricmp( GSR->D_Name, GSR->UserName ) == 0 )
                    _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
                else
                    _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );
                }
            else
                _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
                
            }

        if( (DIM_LOAD == GSR->D_InputMode) &&
            (0        == GSR->d_actualitem) ) {

            swb.number = GSID_DISKOK;
            swb.visible = 0;
            _methoda( GSR->bdisk, BTM_DISABLEBUTTON, &swb );
            }

        if( (DIM_SAVE == GSR->D_InputMode) ||
            (DIM_CREATE ==  GSR->D_InputMode) ) {
            
            swb.number = GSID_DISKSTRING;
            _methoda( GSR->bdisk, BTM_ENABLEBUTTON, &swb );
            }
            
            
        /*** Blinken des Cursors ***/
        #ifdef __DBCS__

        /*** Bei UseSystemTextInput kein Cursor ***/
        if( GSR->ywd->UseSystemTextInput ) {

            strcpy( d_name, GSR->D_Name);
            }
        else {

            strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
            strncpy( &(d_name[ GSR->DCursorPos ]), "_", 1 );
            strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
            }
        #else
        if( (GSR->global_time / 300) & 1 ) {

            /*** mit vollem Cursor ***/
            strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
            strncpy( &(d_name[ GSR->DCursorPos ]), CURSORSIGN, 1 );
            strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
            }
        else {

            /*** mit leerem Cursor ***/
            strncpy( d_name, GSR->D_Name, GSR->DCursorPos );
            strncpy( &(d_name[ GSR->DCursorPos ]), CURSORSIGN_E, 1 );
            strcpy(  &(d_name[ GSR->DCursorPos + 1 ]), &(GSR->D_Name[ GSR->DCursorPos ]) );
            } 
        #endif    
        }

    /*** Stringgadget aktualisieren ***/
    ss.pressed_text   = NULL;
    ss.unpressed_text = d_name;
    ss.number         = GSID_DISKSTRING;
    _methoda( GSR->bdisk, BTM_SETSTRING, &ss );
///

    /* ------------------------------------------------------------------
    **                       DER LOCALEREQUESTER
    ** ----------------------------------------------------------------*/
    
/// "Tasten fuer Locale"
    if( SHELLMODE_LOCALE == GSR->shell_mode ) {

        switch( GSR->input->NormKey ) {

            case KEYCODE_ESCAPE:

                yw_CancelLocale( GSR );
                break;

            case KEYCODE_RETURN:

                yw_OKLocale( GSR );
                break;
            }

        switch( GSR->input->HotKey ) {

            case HOTKEY_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_LANGUAGE,
                                       "help\\111.html");
                break;
            }
        }
///

/// "Gadgets für den LocaleRequester"
    /*** Nur so ein Aufruf wegen Close, Move und so'n Zeuch ***/
    if( ret = _methoda( GSR->blocale, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case GS_LOCALEOK:

                yw_OKLocale( GSR );
                break;

            case GS_LOCALECANCEL:
            case PRA_RQ_CLOSE:

                yw_CancelLocale( GSR );
                break;

            case GS_HELP:

                ywd->Url = ypa_GetStr( GlobalLocaleHandle, STR_HELP_LANGUAGE,
                           "help\\111.html");
                break;

            }
        }
///
    
/// "Menü des LocaleRequesters"
    /*** Bis jetzt nur ein Menü ***/
    if( SHELLMODE_LOCALE == GSR->shell_mode ) {

        int    n;
        struct localeinfonode *nd;

        yw_ListHandleInput( ywd, &(GSR->lmenu), GSR->input );

        if( GSR->lmenu.Flags & LISTF_Select ) {

            GSR->locale_changed |= LCF_LANGUAGE;

            /*** Ausgewählte Node suchen ***/
            n  = GSR->lmenu.Selected;
            nd = (struct localeinfonode *) GSR->localelist.mlh_Head;
            while( n-- ) nd = (struct localeinfonode *) nd->node.mln_Succ;

            GSR->new_lsel = nd;
            }
        
        
        /*** Muß halt so sein ***/
        yw_ListLayout( ywd, &(GSR->lmenu) );
        }

///

    /* ------------------------------------------------------------------
    **                       DER ABOUTREQUESTER
    ** ----------------------------------------------------------------*/

/// "Gadgets des Aboutrequesters"
    if( ret = _methoda( GSR->babout, BTM_HANDLEINPUT, GSR->input ) ) {

        /*** Hilfe ***/
        who = (ret >> 16);

        ret = ((ret << 16) >> 16);

        switch( ret ) {

            case PRA_RQ_CLOSE:

                /*** Der EA-Requester wurde geschlossen ***/
                GSR->shell_mode   = SHELLMODE_TITLE;
                sp.modus = SP_NOPUBLISH;
                _methoda( GSR->babout, BTM_SWITCHPUBLISH, &sp );

                break;
            }
        }
///

/// "Tasten zum Aboutrequester"
    /*** Tasten. Es müssen alle requester geschlossen sein! ***/
    if( SHELLMODE_ABOUT == GSR->shell_mode ) {

        /*** taste zum Abbruch? ***/
        if( (KEYCODE_ESCAPE == GSR->input->NormKey) ||
            (KEYCODE_RETURN == GSR->input->NormKey) ) {

            /*** Wieder zurück auf TITLE_MODE ***/
            GSR->shell_mode = SHELLMODE_TITLE;

            sp.modus = SP_NOPUBLISH;
            _methoda( GSR->babout, BTM_SWITCHPUBLISH, &sp );
            sp.modus = SP_PUBLISH;
            _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
            }
        }

    /*** Zum Aufbau muessen wir im TITLEMODE sein ***/
    if( SHELLMODE_TITLE == GSR->shell_mode ) {

        /*** Wieviel zeit ist vergangen? ***/
        if( (GSR->a_lettercount == 0) ||
            ((GSR->global_time - GSR->a_waittime) < 700) ) {

            switch( GSR->a_lettercount ) {

                case 0: /*** A akzeptieren ***/
                        if(yw_CheckCheatKey(GSR->ywd,GSR->input,KEYCODE_A)) {
                            GSR->a_waittime = GSR->global_time;
                            GSR->a_lettercount++;
                            }
                        else {
                            if( GSR->input->NormKey != 0 )
                                GSR->a_lettercount = 0L;
                            }
                        break;

                case 1: /*** m merken ***/
                        if(yw_CheckCheatKey(GSR->ywd,GSR->input,KEYCODE_M)) {
                            GSR->a_lettercount++;
                            GSR->a_waittime = GSR->global_time;
                            }
                        else {
                            if( GSR->input->NormKey != 0 )
                                GSR->a_lettercount = 0L;
                            }
                        break;

                case 2: /*** o okkupieren ***/
                        if(yw_CheckCheatKey(GSR->ywd,GSR->input,KEYCODE_O)) {
                            GSR->a_waittime = GSR->global_time;
                            GSR->a_lettercount++;
                            }
                        else {
                            if( GSR->input->NormKey != 0 )
                                GSR->a_lettercount = 0L;
                            }
                        break;

                case 3: /*** k kaufen ***/
                        if(yw_CheckCheatKey(GSR->ywd,GSR->input,KEYCODE_K)) {

                            yw_OpenAbout( GSR );
                            _StartSoundSource( &(GSR->ShellSound2), SHELLSOUND_SECRET );
                            }
                        else {
                            if( GSR->input->NormKey != 0 )
                                GSR->a_lettercount = 0L;
                            }
                        break;
                }
            }
        else {
            GSR->a_lettercount = 0L;
            }
        }
    else {
        GSR->a_lettercount = 0L;
        }
///


    /* ------------------------------------------------------------------
    **                       DER NETWORKREQUESTER
    ** ----------------------------------------------------------------*/
#ifdef __NETWORK__

/// "Netzvorbereitungen"
    /*** Vorbereitung ***/
    switch( GSR->n_selmode ) {

        case NM_PROVIDER:

            GSR->N_InputMode    = FALSE;
            GSR->nmenu_yoffset  = (WORD)(3 * (ywd->FontH + ReqDeltaY));
            GSR->nmenu_draw     = TRUE;
            GSR->nmenu.MaxShown = NUM_NET_SHOWN;
            break;

        case NM_SESSIONS:

            GSR->N_InputMode    = FALSE;
            GSR->nmenu_yoffset  = (WORD)(3 * (ywd->FontH + ReqDeltaY));
            GSR->nmenu_draw     = TRUE;
            GSR->nmenu.MaxShown = NUM_NET_SHOWN;
            break;

        case NM_LEVEL:

            GSR->N_InputMode    = FALSE;
            GSR->nmenu_yoffset  = (WORD)(3 * (ywd->FontH + ReqDeltaY));
            GSR->nmenu_draw     = TRUE;
            GSR->nmenu.MaxShown = NUM_NET_SHOWN;
            break;

        case NM_PLAYER:

            GSR->N_InputMode    = TRUE;
            GSR->nmenu_draw     = FALSE;
            GSR->nmenu.MaxShown = NUM_NET_SHOWN;
            break;

        case NM_MESSAGE:

            GSR->N_InputMode    = TRUE;
            GSR->nmenu_yoffset  = (WORD)((5.5+MAXNUM_PLAYERS) * ywd->FontH) + 2 * ReqDeltaY;
            GSR->nmenu_draw     = TRUE;
            GSR->nmenu.MaxShown = NUM_NET_SHOWN - MAXNUM_PLAYERS - 2;
            break;
        }
///

/// "gadgets für den NetzwerkRequester"
    /*** Nur so ein Aufruf wegen Close, Move und so'n Zeuch ***/
    if( ret = _methoda( GSR->bnet, BTM_HANDLEINPUT, GSR->input ) ) {

        struct getsession_msg gs;
        struct sendmessage_msg sm;
        struct ypamessage_text tm;
        struct ypamessage_race rm;

        /*** Hilfe ***/
        who = (ret >> 16);
        if( who ) yw_GameShellToolTips( GSR, who );

        ret = ((ret << 16) >> 16);

        if( (GS_USERRACE == ret) || (GS_KYTERNESER == ret) ||
            (GS_MYKONIER == ret) || (GS_TAERKASTEN == ret) ) {

            /*** Vorbereitung, die für alle gleich sind ***/
            rm.generic.message_id = YPAM_RACE;
            rm.generic.owner      = 0;
            sm.data               = (char *) &rm;
            sm.data_size          = sizeof( rm );
            sm.receiver_kind      = MSG_ALL;
            sm.receiver_id        = NULL;
            sm.guaranteed         = TRUE;
            }

        switch( ret ) {

            case PRA_RQ_CLOSE:
            case GS_NETCANCEL:

                /*** Der NW-Requester wurde geschlossen ***/
                yw_CloseNetRequester( GSR );
                break;

            /* -----------------------------------------------------
            ** Die Rassengadgets können ebenfalls global abgehandelt
            ** werden, da sie nur im Messagemodus aktiv sind. Vor-
            ** bereitungen wurden schon oben abgehandelt, weil viele
            ** Sachen gleich sind.
            ** ---------------------------------------------------*/
            case GS_USERRACE:

                /*** Userrasse ein, schaltet bisherige Rasse aus ***/
                rm.freerace     =  GSR->SelRace;
                GSR->FreeRaces |=  GSR->SelRace;
                rm.newrace      =  FREERACE_USER;
                GSR->SelRace    =  FREERACE_USER;
                GSR->FreeRaces &= ~FREERACE_USER;
                _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm );
                break;

            case GS_KYTERNESER:

                /*** Kyterneser ein, schaltet bisherige Rasse aus ***/
                rm.freerace     =  GSR->SelRace;
                GSR->FreeRaces |=  GSR->SelRace;
                rm.newrace      =  FREERACE_KYTERNESER;
                GSR->SelRace    =  FREERACE_KYTERNESER;
                GSR->FreeRaces &= ~FREERACE_KYTERNESER;
                _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm );
                break;

            case GS_MYKONIER:

                /*** Mykonier ein, schaltet bisherige Rasse aus ***/
                rm.freerace     =  GSR->SelRace;
                GSR->FreeRaces |=  GSR->SelRace;
                rm.newrace      =  FREERACE_MYKONIER;
                GSR->SelRace    =  FREERACE_MYKONIER;
                GSR->FreeRaces &= ~FREERACE_MYKONIER;
                _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm );
                break;

            case GS_TAERKASTEN:

                /*** Taerkasten ein, schaltet bisherige Rasse aus ***/
                rm.freerace     =  GSR->SelRace;
                GSR->FreeRaces |=  GSR->SelRace;
                rm.newrace      =  FREERACE_TAERKASTEN;
                GSR->SelRace    =  FREERACE_TAERKASTEN;
                GSR->FreeRaces &= ~FREERACE_TAERKASTEN;
                _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm );
                break;
            }

        /*** Die unteren gadgets sind kontextsensitiv ***/
        switch( GSR->n_selmode ) {

            case NM_PROVIDER:

                switch( ret ) {

                    case GS_NETOK:

                        /*** Den ausgewählten provider schalten ***/
                        yw_OKProvider( GSR );
                        break;

                    case GS_NETBACK:

                        /*** Gibt es hier nicht ***/
                        break;

                    case GS_NETNEW:

                        /*** Gibt es hier nicht ***/
                        break;

                    case GS_HELP:

                        ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                               STR_HELP_MULTI_SELPROV,
                                               "help\\13.html");
                        break;
                    }

                break;

            case NM_PLAYER:

                switch( ret ) {

                    case GS_NETOK:

                        if( GSR->N_Name[ 0 ] != 0 ) {

                            /* -------------------------------------------
                            ** Hat die gleiche Wirkung wie Enter. Schließt
                            ** die Namenseingabe ab. Die Spielererzeugung
                            ** wird bei Open/JoinSession gemacht.
                            ** -----------------------------------------*/
                            strcpy( GSR->NPlayerName, GSR->N_Name );

                            GSR->n_selmode        = NM_SESSIONS;
                            GSR->NSel             = -1;
                            GSR->nmenu.FirstShown = 0;
                            GSR->N_Name[ 0 ]      = 0;
                            
                            yw_OpenReq(GSR->ywd, &(GSR->nmenu.Req));
                            }

                        break;

                    case GS_NETNEW:

                        /*** Hat nur bei SystemTextInput Sinn ***/
                        if( ywd->UseSystemTextInput ) {

                            struct windd_gettext gt;

                            gt.title_text   = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPLAYER, "ENTER CALLSIGN");
                            gt.ok_text      = ypa_GetStr( GlobalLocaleHandle, STR_OK, "OK");
                            gt.cancel_text  = ypa_GetStr( GlobalLocaleHandle, STR_CANCEL, "CANCEL");
                            gt.default_text = GSR->N_Name;
                            gt.timer_func   = NULL;
                            _methoda( GSR->ywd->GfxObject, WINDDM_GetText, &gt );
                            if( gt.result ) {

                                /*** result ist uebergebener Text ***/
                                strncpy( GSR->N_Name, gt.result, STANDARD_NAMELEN );
                                GSR->N_Name[ STANDARD_NAMELEN-1 ] = 0;
                                }
                            }
                        break;

                    case GS_HELP:

                        ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                               STR_HELP_MULTI_ENTERNAME,
                                               "help\\14.html");
                        break;
                    }

                break;

            case NM_SESSIONS:

                switch( ret ) {

                    case GS_NETOK:

                        yw_OKSessions( GSR );
                        break;

                    case GS_NETNEW:

                        /* ---------------------------------------------------
                        ** Der Sessionname setzt sich aus Player(Host)
                        ** und Levelname zusammen. Wir brauchen ihn also nicht
                        ** einzugeben
                        ** -------------------------------------------------*/
                        GSR->n_selmode        = NM_LEVEL;
                        GSR->is_host          = TRUE;
                        GSR->NSel             = -1;
                        GSR->nmenu.FirstShown = 0;

                        break;

                    case GS_HELP:

                        ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                               STR_HELP_MULTI_SELSESSION,
                                               "help\\15.html");
                        break;
                    }

                break;

            case NM_LEVEL:

                switch( ret ) {

                    case GS_NETOK:

                        yw_OKLevel( GSR );
                        break;

                    case GS_HELP:

                        ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                               STR_HELP_MULTI_SELLEVEL,
                                               "help\\16.html");
                        break;

                    }

                break;

            case NM_MESSAGE:

                switch( ret ) {

                    case GS_NETREADY:

                        {
                            /*** Fertig-Meldung ***/
                            struct ypamessage_readytostart rts;
                            struct sendmessage_msg sm;
                            struct flushbuffer_msg fb;
                            
                            /*** mich im Playermenue suchen ***/
                            gpd.number  = 0;
                            gpd.askmode = GPD_ASKNUMBER;
                            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {

                                /*** Nicht ganz wasserdicht...***/
                                if(stricmp( gpd.name, GSR->NPlayerName)==0)
                                    break;

                                gpd.number++;
                                }

                            /*** "Fertig"-Message losschicken ***/
                            rts.ready_to_start = 1;
                            GSR->ReadyToStart  = 1;
                            GSR->player2[ gpd.number ].ready_to_start = 1;

                            rts.generic.message_id = YPAM_READYTOSTART;
                            rts.generic.owner      = 0; // weil noch nicht fest

                            sm.data                = (char *) &rts;
                            sm.data_size           = sizeof( rts );
                            sm.receiver_kind       = MSG_ALL;
                            sm.receiver_id         = NULL;
                            sm.guaranteed          = TRUE;
                            _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);

                            fb.sender_kind         = MSG_PLAYER;
                            fb.sender_id           = GSR->NPlayerName;
                            fb.receiver_kind       = MSG_ALL;
                            fb.receiver_id         = NULL;
                            fb.guaranteed          = TRUE;
                            _methoda( GSR->ywd->nwo, NWM_FLUSHBUFFER, &fb);
                            }
                        break;

                    case GS_NETSTOP:

                        {
                            /*** Fertig-Meldung ***/
                            struct ypamessage_readytostart rts;
                            struct sendmessage_msg sm;
                            struct flushbuffer_msg fb;
                            
                            /*** mich im Playermenue suchen ***/
                            gpd.number  = 0;
                            gpd.askmode = GPD_ASKNUMBER;
                            while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {

                                /*** Nicht ganz wasserdicht...***/
                                if(stricmp( gpd.name, GSR->NPlayerName)==0)
                                    break;

                                gpd.number++;
                                }

                            /*** "Noch-Warten-Message" losschicken ***/
                            rts.ready_to_start = 0;
                            GSR->ReadyToStart  = 0;
                            GSR->player2[ gpd.number ].ready_to_start = 0;

                            rts.generic.message_id = YPAM_READYTOSTART;
                            rts.generic.owner      = 0; // weil noch nicht fest

                            sm.data                = (char *) &rts;
                            sm.data_size           = sizeof( rts );
                            sm.receiver_kind       = MSG_ALL;
                            sm.receiver_id         = NULL;
                            sm.guaranteed          = TRUE;
                            _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);

                            fb.sender_kind         = MSG_PLAYER;
                            fb.sender_id           = GSR->NPlayerName;
                            fb.receiver_kind       = MSG_ALL;
                            fb.receiver_id         = NULL;
                            fb.guaranteed          = TRUE;
                            _methoda( GSR->ywd->nwo, NWM_FLUSHBUFFER, &fb);
                            }
                        break;

                    case GS_NETOK:

                        /* ---------------------------------------------------
                        ** Das Spiel beginnt.Bin ich Host, so schicke ich eine
                        ** Message los, daß das Spiel gestartet wird
                        ** -------------------------------------------------*/
                        if( GSR->is_host ) {

                            /*** Möglichkeit zum starten ***/
                            char *t;
                            
                            /*** Wenn nur ein Spieler, dann Sicherheitsabfrage ***/
                            if( 1 < _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL ) ) {
                                
                                /*** CD Check ***/
                                if( t = yw_NotEnoughCDs( GSR ) ) 
                                    yw_OpenConfirmRequester( GSR, CONFIRM_MORECDS, t, 1 );
                                else
                                    yw_StartNetGame( GSR );
                                }
                            else
                                yw_OpenConfirmRequester( GSR, CONFIRM_NETSTARTALONE,
                                    ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_NETSTARTALONE,
                                                "DO YOU REALLY WANT TO START WITHOUT OTHER PLAYERS?"), 0);      
                            }
                        break;

                    case GS_NETBACK:

                        /* -------------------------------------------
                        ** Zurück zu den Levels, um einen neuen auszuwaehlen.
                        ** Das sit nur dem Host erlaubt. Die Session darf nicht
                        ** geschlossen werden, okLevel muss seolbst daruf achten,
                        ** es eine neue Session ist oder ob sich nur der Level
                        ** aendert.
                        ** Hierbei gibt es keine Unterschiede zwischen Lobby und
                        ** normalem Start.
                        ** -----------------------------------------*/
                        if( GSR->is_host ) {
                            
                            GSR->n_selmode   = NM_LEVEL;
                            GSR->NSel              = -1;
                            GSR->nmenu.FirstShown  = 0;
    
                            /*** Messagebuffer und Stringgadget leeren ***/
                            GSR->act_messageline   = 0;
                            GSR->lastsender[0]     = 0;
                            GSR->N_Name[0]         = 0;
                            }
                        break;

                    case GS_NETSEND:

                        /* ------------------------------------------------
                        ** Im Falle von SystemInput fragen wir vorher die
                        ** zu sendende Message ab, denn wir haben ja keinen
                        ** normalen textinput.
                        ** ----------------------------------------------*/
                        if( ywd->UseSystemTextInput ) {

                            struct windd_gettext gt;

                            gt.title_text   = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_ENTERMESSAGE, "ENTER MESSAGE");
                            gt.ok_text      = ypa_GetStr( GlobalLocaleHandle, STR_OK, "OK");
                            gt.cancel_text  = ypa_GetStr( GlobalLocaleHandle, STR_CANCEL, "CANCEL");
                            gt.default_text = GSR->N_Name;
                            gt.timer_func   = NULL;
                            _methoda( GSR->ywd->GfxObject, WINDDM_GetText, &gt );
                            if( gt.result ) {

                                /*** result ist uebergebener Text ***/
                                strncpy( GSR->N_Name, gt.result, STANDARD_NAMELEN );
                                GSR->N_Name[ STANDARD_NAMELEN-1 ] = 0;
                                }
                            }


                        if( GSR->N_Name[ 0 ] != 0 ) {

                            WORD number;

                            tm.generic.message_id = YPAM_TEXT;
                            tm.generic.owner      = 0; // weil noch nicht feststehend
                            strcpy( tm.text, GSR->N_Name );

                            sm.data               = (char *) &tm;
                            sm.data_size          = sizeof( tm );
                            sm.sender_kind        = MSG_PLAYER;
                            sm.sender_id          = GSR->NPlayerName;
                            sm.receiver_kind      = MSG_ALL;
                            sm.receiver_id        = NULL;
                            sm.guaranteed         = TRUE;
                            _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);

                            /*** Message auch bei mir anzeigen ***/
                            yw_AddMessageToBuffer( GSR->ywd, GSR->NPlayerName, tm.text );

                            GSR->N_Name[ 0 ]      = 0;
                            GSR->NCursorPos       = 0;

                            /*** und, wenn Nummer, Sample spielen ***/
                            number = strtol( tm.text, NULL, 0 );
                            if( number > 0 )
                                yw_LaunchChatSample( ywd, number );
                            }

                        break;

                    case GS_HELP:

                        ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                               STR_HELP_MULTI_STARTSCREEN,
                                               "help\\17.html");
                        break;
                    }

                break;
            }
        }
///
    
/// "Menü des NetzwerkRequesters"
    /*** Bis jetzt nur ein Menü ***/
    if( SHELLMODE_NETWORK == GSR->shell_mode ) {

        struct setstring_msg ss;
        struct getprovidername_msg gpn;
        struct getsessionname_msg gsn;
        LONG   derzeit_y;

        /*** Vorbereitung für die Auswertung ***/
        _get( GSR->bnet, BTA_BoxY, &derzeit_y );
        GSR->nmenu.Req.req_cbox.rect.y = derzeit_y + GSR->nmenu_yoffset;

        yw_ListHandleInput( ywd, &(GSR->nmenu), GSR->input );

        if( (GSR->nmenu.Flags     & LISTF_Select) ||
            (GSR->input->NormKey == KEYCODE_CURSOR_UP) ||
            (GSR->input->NormKey == KEYCODE_CURSOR_DOWN) ) {

            ss.number       = GSID_NETSTRING;
            ss.pressed_text = NULL;
            GSR->NSel       = GSR->nmenu.Selected;

            /*** Auswertung erfolgt z.T. je nach Modus. ***/
            switch( GSR->n_selmode ) {

                case NM_PROVIDER:

                    gpn.number = GSR->NSel;
                    GSR->N_InputMode = FALSE;
                    break;

                case NM_SESSIONS:

                    gsn.number = GSR->NSel;
                    GSR->N_InputMode = FALSE;

                    break;

                case NM_LEVEL:

                    /* -----------------------------------------------------------
                    ** der n-selte Eintrag, der folgenden Bedingungen entspricht:
                    **   mehr oder gleichviele Spieler wie bisher gejoined
                    **        sofern nicht ein Spieler, weil dann erst neu ausgew.
                    **   weniger als 3 bei Slow Connection
                    ** ---------------------------------------------------------*/
                    {
                        ULONG i, players;
                        struct LevelNode *level;
                        LONG    offset;

                        offset = 0;

                        players = _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL );
                            
                        for( i = 0; i < MAXNUM_LEVELS; i++ ) {
                                               
                            level = &(GSR->ywd->LevelNet->Levels[ GSR->netlevel[ i ].number ]);

                            if( (players > 1) && (level->num_players < players) )
                                continue;

                            if( offset == GSR->NSel ) {
                                
                                GSR->NLevelName   = GSR->netlevel[ i ].name;
                                GSR->NLevelOffset = GSR->netlevel[ i ].number;
//                                yw_LocStrCpy( GSR->N_Name, GSR->NLevelName);
                                break;    
                                }
                            offset++;
                            }         
                            
                    }
                    break;

                case NM_PLAYER:

                    /*** Es läßt sich sowieso kein Player auswählen ... ***/
                    break;
                }

            /*** Name im String-Gadget setzen ***/
            GSR->NCursorPos = strlen( GSR->N_Name );
            }                                                                   
        

        /*** Muß halt so sein ***/
        yw_ListLayout( ywd, &(GSR->nmenu) );
        }

///
    
/// "Tasten für den NetRequester"

    /*** Tasten? --> Stringadget ***/
    if( SHELLMODE_NETWORK == GSR->shell_mode ) {

        /* -------------------------------------------------------------------
        ** Mit dem neuen Konzept, wo das Stringgadget nur zu bestimmten Zeiten
        ** aktiv ist, kann ich es machen, die Cursortasten zur Listennavi-
        ** gation zu nehmen. Ansonsten bleibt alles beim alten, ledig-
        ** lich die Entertaste wird als Abschluß des Stringgadgets und damit
        ** als "NEW" ausgewertet.
        ** -----------------------------------------------------------------*/

        if( GSR->input->NormKey || GSR->input->AsciiKey || GSR->input->HotKey ) {

            int    len;
            struct switchbutton_msg sb;
            struct sendmessage_msg sm;
            struct ypamessage_text tm;
            BOOL   own_input;

            char   n_name[ STANDARD_NAMELEN + 1 ];

            if( GSR->N_InputMode ) {

                int max_chars;
                
                sb.visible = 1L;
                if( ywd->UseSystemTextInput )
                    own_input = FALSE;
                else
                    own_input = TRUE;
                    
                /*** 40 'W's passen ins Feld, also nur 39 Zeichen plus Cursor ***/
                if( NM_PLAYER == GSR->n_selmode )
                   max_chars = 32;
                else
                   max_chars = 38;
                   
                /*** allgemeine Buchstabentaste? ***/
                if( (strlen( GSR->N_Name ) < (max_chars)) &&
                    (GSR->input->AsciiKey > 31) && (own_input) ) {

                    char ascii_in[10], ascii_out[10];

                    /* ---------------------------------------------------------------
                    ** Was nicht darstellbar ist, sollte auch nicht als * ausgegeben
                    ** werden. Folglich jagen wir es durch Floh's Filterroutine und
                    ** ignorieren alles, was als Stern rauskam, aber nicht als solcher
                    ** rein ist!
                    ** -------------------------------------------------------------*/
                    sprintf( ascii_in, "%c\0", GSR->input->AsciiKey );
                    yw_LocStrCpy( ascii_out, ascii_in );
                    if( !( (ascii_in[0] != '*') && (ascii_out[0] == '*') ) ) {

                        /*** Buchstabe einsortieren ***/
                        strncpy( n_name, GSR->N_Name, GSR->NCursorPos );
                        strncpy( &(n_name[ GSR->NCursorPos ]), ascii_out, 1 );
                        strcpy(  &(n_name[ GSR->NCursorPos + 1 ]), &( GSR->N_Name[ GSR->NCursorPos]) );
                        strcpy( GSR->N_Name, n_name );

                        GSR->NCursorPos++;  // Stringgadget unten neu
                        }
                    }

                /*** Was sind denn das für noch für SonderTasten? ***/
                switch( GSR->input->NormKey ) {

                    case KEYCODE_CURSOR_LEFT:

                        /*** Cursor links ***/
                        if( (GSR->NCursorPos > 0) && own_input )
                            GSR->NCursorPos--;
                        break;

                    case KEYCODE_CURSOR_RIGHT:

                        /*** Cursor nach rechts ***/
                        len = strlen( GSR->N_Name );
                        if( (GSR->NCursorPos < len) && own_input )
                            GSR->NCursorPos++;
                        break;

                    case KEYCODE_DEL:

                        /*** Löschen Zeichen nach Cursor ***/
                        if( (GSR->NCursorPos < strlen( GSR->N_Name )) && own_input ) {

                            strcpy( &(GSR->N_Name[ GSR->NCursorPos ]),
                                    &(GSR->N_Name[ GSR->NCursorPos + 1]) );
                            }
                        break;

                    case KEYCODE_BS:

                        /*** Zeichen vor Cursor löschen ***/
                        if( (GSR->NCursorPos > 0) && own_input ) {

                            strcpy( &(GSR->N_Name[ GSR->NCursorPos - 1 ]),
                                    &(GSR->N_Name[ GSR->NCursorPos ]) );
                            GSR->NCursorPos--;
                            }
                        break;

                    }
                }

            /* -----------------------------------------------------
            ** Manche Tasten gelten immer!. Achtung, was schon durch
            ** den Inputmode abhandelt wird, mache ich nicht doppelt
            ** ---------------------------------------------------*/
            switch( GSR->input->NormKey ) {

                case KEYCODE_RETURN:

                    /*** Eigentlich müssen wir den Modus abfragen... ***/
                    switch( GSR->n_selmode ) {

                        case NM_PROVIDER:

                            yw_OKProvider( GSR );
                            break;

                        case NM_LEVEL:

                            yw_OKLevel( GSR );
                            break;

                        case NM_SESSIONS:

                            /* ------------------------------------
                            ** Wenn kein level da ist, heisst enter
                            ** neuen level auswaehlen
                            ** ----------------------------------*/
                            if( GSR->nmenu.NumEntries == 0 ) {

                                GSR->n_selmode        = NM_LEVEL;
                                GSR->is_host          = TRUE;
                                GSR->NSel             = -1;
                                GSR->nmenu.FirstShown = 0;
                                }
                            else
                                yw_OKSessions( GSR );
                            break;

                        case NM_PLAYER:

                            /* ----------------------------------------
                            ** OK, die Namenseingabe ist abgeschlossen,
                            ** genauso, als ob ich OK gedrückt hätte.
                            ** Nur Name merken
                            ** --------------------------------------*/
                            if( GSR->N_Name[ 0 ] != 0 ) {

                                strcpy( GSR->NPlayerName, GSR->N_Name );

                                GSR->n_selmode        = NM_SESSIONS;
                                GSR->NSel             = -1;
                                GSR->nmenu.FirstShown = 0;
                                GSR->N_Name[ 0 ]      = 0;
                                
                                yw_OpenReq(GSR->ywd, &(GSR->nmenu.Req));
                                }

                            break;

                        case NM_MESSAGE:

                            /*** Eine Message an alle Losschicken ***/
                            if( GSR->N_Name[ 0 ] != 0 ) {

                                WORD number;

                                tm.generic.message_id = YPAM_TEXT;
                                tm.generic.owner      = 0;
                                strcpy( tm.text, GSR->N_Name );

                                sm.data                = (char *) &tm;
                                sm.data_size           = sizeof( tm );
                                sm.receiver_kind       = MSG_ALL;
                                sm.receiver_id         = NULL;
                                sm.guaranteed          = TRUE;
                                _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);

                                /*** Message auch bei mir anzeigen ***/
                                yw_AddMessageToBuffer( GSR->ywd, GSR->NPlayerName, tm.text );

                                GSR->N_Name[ 0 ]      = 0;
                                GSR->NCursorPos       = 0;

                                /*** und, wenn Nummer, Sample spielen ***/
                                number = strtol( tm.text, NULL, 0 );
                                if( number > 0 )
                                    yw_LaunchChatSample( ywd, number );
                                }

                            break;
                            }

                    break;

                case KEYCODE_ESCAPE:

                    yw_CloseNetRequester( GSR );
                    break;
                }


            switch( GSR->input->HotKey ) {

                case HOTKEY_HELP:

                    /*** je nach derzeitigem Modus/Screen ***/
                    if( !GSR->N_InputMode ) {

                        switch( GSR->n_selmode ) {

                            case NM_PROVIDER:

                                ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                                       STR_HELP_MULTI_SELPROV,
                                                       "help\\13.html");
                                break;

                            case NM_SESSIONS:

                                ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                                       STR_HELP_MULTI_SELSESSION,
                                                       "help\\15.html");
                                break;

                            case NM_PLAYER:

                                ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                                       STR_HELP_MULTI_ENTERNAME,
                                                       "help\\14.html");
                                break;

                            case NM_LEVEL:

                                ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                                       STR_HELP_MULTI_SELLEVEL,
                                                       "help\\16.html");
                                break;

                            case NM_MESSAGE:

                                ywd->Url = ypa_GetStr( GlobalLocaleHandle,
                                                       STR_HELP_MULTI_STARTSCREEN,
                                                       "help\\17.html");
                                break;
                            }
                        }

                    break;
                }
            
            /*** Falls was ausgewählt, richtig positionieren ***/
            if( GSR->NSel != -1 )
                yw_PosSelected( &(GSR->nmenu), GSR->NSel );

            /*** Taste für den rest der Menschheit löschen ***/
            GSR->input->NormKey = 0;
            }
        }

///
    
/// "Zusatzarbeit für Netzrequester"
    /* -------------------------------------------------
    ** Nacharbeit zu den gadgets des Netzwerk-requesters
    ** -----------------------------------------------*/

    /* --------------------------------------------------------
    ** Wenn ich Host bin darf ich volle Sessions locken, welche
    ** ich natürlich nicht wieder unlocke, wenn das Spiel schon
    ** gestartet ist!!!!
    ** ------------------------------------------------------*/
    if( (GSR->is_host) && (GSR->n_selmode == NM_MESSAGE) &&
        (GSR->GSA.LastAction != A_NETSTART) ) {

        struct locksession_msg ls;

        if( GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].num_players <=
            _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL ) ) {

            /*** Block, wenn nicht schon getan ***/
            if( !GSR->blocked ) {
                ls.block = 1;
                _methoda( GSR->ywd->nwo, NWM_LOCKSESSION, &ls );
                GSR->blocked = TRUE;
                }
            }
        else {

            if( GSR->blocked ) {
                ls.block = 0;
                _methoda( GSR->ywd->nwo, NWM_LOCKSESSION, &ls );
                GSR->blocked = FALSE;
                }
            }
        }

    /* --------------------------------------------------------
    ** Voreinstellungen, die im Nachhinein überschrieben werden
    ** ------------------------------------------------------*/

    /*** Die Buttons ***/
    swb.number  = GSID_NETOK;
    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
    swb.number  = GSID_NETBACK;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_NETNEW;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_NETCANCEL;
    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
    swb.number  = GSID_NETSEND;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_LEVELNAME;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_LEVELTEXT;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );

    /*** Default ***/
    ss.number = GSID_NETOK;
    ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_OK,"OK");
    ss.pressed_text   = NULL;
    _methoda( GSR->bnet, BTM_SETSTRING, &ss );

    /*** Rasse defaultmäßig ausschalten ***/
    swb.number = GSID_SELRACE;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_USERRACE;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_KYTERNESER;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_MYKONIER;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_TAERKASTEN;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_NETREADY;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number = GSID_NETREADYTEXT;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );

    /*** defaultmaessig Namensgadgets aus, bei Message wieder an ***/
    swb.visible = 0;
    swb.number  = GSID_PLAYERNAME1;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERNAME2;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERNAME3;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERNAME4;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERSTATUS1;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERSTATUS2;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERSTATUS3;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    swb.number  = GSID_PLAYERSTATUS4;
    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    
    /* -------------------------------------------------------------
    ** Hilfstext fuer Sessionupdate bei serieller Verbindung. Diesen
    ** Button missbrauchen wir auch zur Anzeige der TCP/IP adresse 
    ** -----------------------------------------------------------*/
    if( ((NM_SESSIONS == GSR->n_selmode) &&
        (NWFC_SERIAL == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL ))) ||
        (NM_PROVIDER == GSR->n_selmode) ) {
        
        if( NM_PROVIDER == GSR->n_selmode) {
        
            char m[300];
            if( GSR->ywd->local_addressstring[ 0 ] )
                sprintf( m, "%s  %s", ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_TCPIP, "YOUR TCP/IP ADDRESS"),
                         GSR->ywd->local_addressstring );
            else
                strcpy( m, " ");
            ss. unpressed_text = m;
            }
        else {
        
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_REFRESHSESSIONS,
                                            "PRESS SPACEBAR TO UPDATE SESSION LIST");
            }
            
        ss.pressed_text   = NULL;
        ss.number = GSID_REFRESHSESSIONS;
        _methoda( GSR->bnet, BTM_SETSTRING, &ss );    
            
        swb.number  = GSID_REFRESHSESSIONS;
        _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
        }
    else {
    
        swb.number  = GSID_REFRESHSESSIONS;
        _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
        }

    /*** Im Inputmode Cursor blinken ***/
    if( (GSR->N_InputMode && (FALSE == ywd->UseSystemTextInput)) ||
        (NM_PLAYER == GSR->n_selmode) ) {
        
        /*** Textgadget freischalten ***/
        swb.number  = GSID_NETSTRING;
        _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
        
        /*** Im Playermodus unter Titel, sonst unterlistview ***/
        sbp.x      = -1;
        sbp.number = GSID_NETSTRING;
        if( NM_PLAYER == GSR->n_selmode ) {
        
            sbp.w = NListWidth;
            sbp.y = 3 * (ywd->FontH + ReqDeltaY);
            }
        else {
        
            sbp.w = (WORD)(0.8 * NListWidth);
            sbp.y = (WORD)((NUM_NET_SHOWN + 2) * (ywd->FontH + ReqDeltaY));
            }
        _methoda( GSR->bnet, BTM_SETBUTTONPOS, &sbp );
        
        if( FALSE == ywd->UseSystemTextInput ) {
        
            strncpy( d_name, GSR->N_Name, GSR->NCursorPos );
            strncpy( &(d_name[ GSR->NCursorPos ]), "_", 1 );
            strcpy(  &(d_name[ GSR->NCursorPos + 1 ]), &(GSR->N_Name[ GSR->NCursorPos ]) );
            }
        else 
            strcpy( d_name, GSR->N_Name );
            
        ss.number         = GSID_NETSTRING;
        ss.unpressed_text = d_name;
        ss.pressed_text   = NULL;
        _methoda( GSR->bnet, BTM_SETSTRING, &ss );
        }
    else {
    
        /*** Textgadget abschalten, weil dort keine Eingabe ***/
        swb.number  = GSID_NETSTRING;
        _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
        }    
        
    /*** Im Playermodus NEW nach oben (fuer SytemtextInput) ***/
    sbp.number = GSID_NETNEW;
    sbp.x      = -1;
    sbp.w      = -1; 
    if( NM_PLAYER == GSR->n_selmode ) {
    
        sbp.y = 4 * (ywd->FontH + ReqDeltaY); 
        }
    else {
    
        sbp.y = (WORD)((NUM_NET_SHOWN + 3.2) * (ywd->FontH + ReqDeltaY));
        }    
    _methoda( GSR->bnet, BTM_SETBUTTONPOS, &sbp );
    
    switch( GSR->n_selmode ) {

        case NM_PROVIDER:

            /*** Mittleres gadget ausschalten ***/
            swb.visible = 0L;
            swb.number  = GSID_NETBACK;
            _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );

            /*** Titel ist select provider ***/
            ss.number = GSID_NETHEADLINE;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPROVIDER,"SELECT PROVIDER");
            ss.pressed_text   = NULL;
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE2;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPROVIDER2,"2");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE3;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPROVIDER3,"3");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            break;

        case NM_SESSIONS:

            /*** Mittleres gadget einschalten ***/
            ss.number = GSID_NETNEW;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_NEW,"NEW");
            ss.pressed_text   = NULL;
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            swb.number  = GSID_NETNEW;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );

            /*** Titel ist select session ***/
            ss.number = GSID_NETHEADLINE;
            ss.pressed_text   = NULL;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLSESSIONS,"SELECT SESSION");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE2;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLSESSIONS2,"5");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE3;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLSESSIONS3,"6");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );

            /*** Überhaupt was da? ***/
            gsn.number = 0;
            if( !_methoda( GSR->ywd->nwo, NWM_GETSESSIONNAME, &gsn ) ) {
                
                /*** nix da, aber bei Modem heisst das "Horchen" ***/
                if( NWFC_MODEM == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL ) ) {
                    
                    /*** mache einen Search-Button ***/
                    ss.number = GSID_NETOK;
                    ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_SEARCH,"SEARCH");
                    ss.pressed_text   = NULL;
                    _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                    }
                else {
                    
                   swb.number  = GSID_NETOK;
                   _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
                   }
                }
            else {

                ss.number = GSID_NETOK;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_JOIN,"JOIN");
                ss.pressed_text   = NULL;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                }

            break;

        case NM_LEVEL:

            /*** bei remotestart kein BackButton ***/
            if( GSR->remotestart ) {

                swb.number  = GSID_NETBACK;
                _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
                }

            /*** Titel ist select provider ***/
            ss.number = GSID_NETHEADLINE;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLLEVEL,"SELECT LEVEL");
            ss.pressed_text   = NULL;
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE2;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLLEVEL2,"8");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE3;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLLEVEL3,"9");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            break;

        case NM_PLAYER:

            ss.number = GSID_NETHEADLINE;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPLAYER,"ENTER PLAYER");
            ss.pressed_text   = NULL;
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE2;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPLAYER2,"11");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            ss.number = GSID_NETHEADLINE3;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLPLAYER3,"12");
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );

            /* --------------------------------------------------------
            ** Wir sind noch bei der Namenseingabe. Da brauchen wir
            ** NEW nur falls wir SystemInput nehmen (als Start das req)
            ** ------------------------------------------------------*/
            if( ywd->UseSystemTextInput ) {

                swb.number  = GSID_NETNEW;
                _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                
                ss.number         = GSID_NETNEW;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_ENTERNAME,"CHANGE");
                ss.pressed_text   = NULL;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                }
           break;

        case NM_MESSAGE:

            /* ----------------------------------------------------
            ** wir warten auf das Spiel. Als Host brauchen
            ** wir den Startgame-Button. Sonst warten wir, bis eine
            ** "Los-Gehts"-message von außen kommt.
            ** Oder wir verschicken einfach so 'n paar Messages...
            ** --------------------------------------------------*/

            /*** OK-Button als SEND kennzeichnen ***/
            swb.number  = GSID_NETSEND;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            ss.number = GSID_NETSEND;
            ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_SEND,"SEND");
            ss.pressed_text   = NULL;
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );

            /*** Levelanzeige freischalten und ausfuellen ***/    
            swb.number  = GSID_LEVELNAME;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_LEVELTEXT;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            if( GSR->NLevelOffset )
                ss.unpressed_text = GSR->NLevelName;
            else
                ss.unpressed_text = " ";        
            ss.number = GSID_LEVELNAME;
            ss.pressed_text   = NULL;
            _methoda( GSR->bnet, BTM_SETSTRING, &ss );
            
            /*** nur hosts haben BackButton fuer Levelauswahl***/
            if( GSR->is_host ) {

                swb.number  = GSID_NETBACK;
                _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                }
                
            /* -----------------------------------------------
            ** Wenn wir uns ready geschalten haben, dann gibts
            ** auch keinen backButton.
            ** ---------------------------------------------*/    
            if( (GSR->ReadyToStart) && (!(GSR->is_host)) ) {

                swb.number  = GSID_NETBACK;
                _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
                }

            /*** text einschalten ***/
            swb.number = GSID_SELRACE;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                
            /*** freie Rassengadgets für Owner aktivieren ***/ 
            if( (0 < GSR->NLevelOffset) && (GSR->NLevelOffset < MAXNUM_LEVELS) ) {
            
                UBYTE  warschonda;
               
                l = &(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ]);
                swb.number = GSID_USERRACE;
                if( l->races & 2 )
                    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                else
                    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    
                swb.number = GSID_KYTERNESER;
                if( l->races & 64 )
                    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                else
                    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    
                swb.number = GSID_MYKONIER;
                if( l->races & 8 )
                    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                else
                    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    
                swb.number = GSID_TAERKASTEN;
                if( l->races & 16 )
                    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                else
                    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
    
                /*** Rassenauswahl "pressen" ***/
                sbs.who = 0;
                switch( GSR->SelRace ) {
    
                    case FREERACE_USER:         sbs.who = GSID_USERRACE;   break;
                    case FREERACE_KYTERNESER:   sbs.who = GSID_KYTERNESER; break;
                    case FREERACE_MYKONIER:     sbs.who = GSID_MYKONIER;   break;
                    case FREERACE_TAERKASTEN:   sbs.who = GSID_TAERKASTEN; break;
                    }
    
                if( sbs.who ) {
                    sbs.state = SBS_PRESSED;
                    _methoda( GSR->bnet, BTM_SETSTATE, &sbs );
                    }
    
                /* -------------------------------------------------
                ** Kollisionstest mit Auswahl anderer.natuerlich nur
                ** Sinn, wenn ein Level ausgewaehlt wurde. das kann
                ** bei Lobbyzeug durchaus noch nicht geschehen sein.
                ** -----------------------------------------------*/
            
                gpd.number     = 0;
                gpd.askmode    = GPD_ASKNUMBER;
                GSR->Startable = 1;
                GSR->Welcomed  = 1;
                warschonda     = 0;
    
                /*** Verbotene Rassen "waren auch schon da"... ***/
                if( !(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].races & 2) )
                    warschonda |= FREERACE_USER;
                if( !(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].races & 64) )
                    warschonda |= FREERACE_KYTERNESER;
                if( !(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].races & 8) )
                    warschonda |= FREERACE_MYKONIER;
                if( !(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].races & 16) )
                    warschonda |= FREERACE_TAERKASTEN;
    
                while( _methoda( GSR->ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {
    
                    UBYTE foundrace;
    
                    /*** Macht der Trouble? ***/
                    if( gpd.flags & NWFP_OWNPLAYER)
                        foundrace = GSR->SelRace;
                    else
                        foundrace = GSR->player2[ gpd.number ].race;
    
                    if( warschonda & foundrace ) {
                        GSR->player2[ gpd.number ].trouble = 1;
                        GSR->Startable                     = 0;
    
                        /*** auch den ersten markieren ***/
                        switch( foundrace ) {
                            case FREERACE_USER:
                                GSR->player2[ erster[0] ].trouble = 1; break;
                            case FREERACE_KYTERNESER:
                                GSR->player2[ erster[1] ].trouble = 1; break;
                            case FREERACE_MYKONIER:
                                GSR->player2[ erster[2] ].trouble = 1; break;
                            case FREERACE_TAERKASTEN:
                                GSR->player2[ erster[3] ].trouble = 1; break;
                            }
                        }
                    else {
                        GSR->player2[ gpd.number ].trouble = 0;
    
                        /*** den 1. merken. Flexibel ist das nicht!!! ***/
                        switch( foundrace ) {
                            case FREERACE_USER:       erster[0] = gpd.number; break;
                            case FREERACE_KYTERNESER: erster[1] = gpd.number; break;
                            case FREERACE_MYKONIER:   erster[2] = gpd.number; break;
                            case FREERACE_TAERKASTEN: erster[3] = gpd.number; break;
                            }
                        }
    
                    /*** merken ***/
                    warschonda |= foundrace;
    
                    gpd.number++;
                    }
                }    


            /* ------------------------------------------------------
            ** Nun testen, ob von allen eine "Los-Gehts-Message" kam.
            ** Dabei mißachten wir uns selbst, weil wir ja Host sind.
            ** Ist dem nicht so, wird startable ge-FALSE-d
            ** ----------------------------------------------------*/
            gpd.number     = 0;
            gpd.askmode    = GPD_ASKNUMBER;
            while( _methoda( GSR->ywd->nwo, NWM_GETPLAYERDATA, &gpd ) ) {

                if( !(gpd.flags & NWFP_OWNPLAYER ) ) {

                    if( !GSR->player2[ gpd.number ].ready_to_start )
                        GSR->Startable = 0;

                    if( !GSR->player2[ gpd.number ].welcomed ) 
                        GSR->Welcomed  = 0;
                    }

                gpd.number++;
                }

            if( GSR->is_host ) {

                /*** Die Überschrift ***/
                ss.number = GSID_NETHEADLINE;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLSTART,"START GAME OR ENTER MESSAGE TO THE PLAYERS");
                ss.pressed_text   = NULL;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                ss.number = GSID_NETHEADLINE2;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLSTART2,"14");
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                ss.number = GSID_NETHEADLINE3;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLSTART3,"15");
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );

                /*** Den New-Button mit Inschrift Start freischalten ***/
                ss.number = GSID_NETOK;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_NETSTART,"START");
                ss.pressed_text   = NULL;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );

                if( !GSR->Startable ) {

                    swb.visible = 0L;
                    swb.number  = GSID_NETOK;
                    _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
                    }
                }
            else {

                /*** Die Überschrift ***/
                ss.number = GSID_NETHEADLINE;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLWAIT,"WAIT FOR START OR SEND MESSAGES");
                ss.pressed_text   = NULL;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                ss.number = GSID_NETHEADLINE2;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLWAIT2,"17");
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );
                ss.number = GSID_NETHEADLINE3;
                ss.unpressed_text = ypa_GetStr( GlobalLocaleHandle, STR_NGADGET_HLWAIT3,"18");
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );

                /* -----------------------------------------
                ** Ready-Button freischalten, wenn von allen
                ** eine WELCOME-message eingetroffen ist
                ** ---------------------------------------*/
                if( GSR->Welcomed ) {

                    swb.number  = GSID_NETREADY;
                    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                    swb.number  = GSID_NETREADYTEXT;
                    _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
                    }

                /*** kein ok ***/
                swb.number  = GSID_NETOK;
                _methoda( GSR->bnet, BTM_DISABLEBUTTON, &swb );
                }

            /*** Stringgadgets fuer Name freischalten ***/
            swb.number  = GSID_PLAYERNAME1;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERNAME2;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERNAME3;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERNAME4;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERSTATUS1;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERSTATUS2;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERSTATUS3;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );
            swb.number  = GSID_PLAYERSTATUS4;
            _methoda( GSR->bnet, BTM_ENABLEBUTTON, &swb );

            /*** Setzen der Inhalte ***/
            gpd.number  = 0;
            gpd.askmode = GPD_ASKNUMBER;
            count       = 0;
            while( count < 4 ) {

                LONG merke_number;
                char status[10];
                BOOL is_player;

                is_player = _methoda( GSR->ywd->nwo, NWM_GETPLAYERDATA, &gpd );

                switch( count ) {

                    case 0: ss.number             = GSID_PLAYERNAME1;
                            merke_number          = GSID_PLAYERSTATUS1;
                            if( is_player )
                                ss.unpressed_text = gpd.name;
                            else
                                ss.unpressed_text = " ";
                            break;

                    case 1: ss.number             = GSID_PLAYERNAME2;
                            merke_number          = GSID_PLAYERSTATUS2;
                            if( is_player )
                                ss.unpressed_text = gpd.name;
                            else
                                ss.unpressed_text = " ";
                            break;

                    case 2: ss.number             = GSID_PLAYERNAME3;
                            merke_number          = GSID_PLAYERSTATUS3;
                            if( is_player )
                                ss.unpressed_text = gpd.name;
                            else
                                ss.unpressed_text = " ";
                            break;

                    case 3: ss.number             = GSID_PLAYERNAME4;
                            merke_number          = GSID_PLAYERSTATUS4;
                            if( is_player )
                                ss.unpressed_text = gpd.name;
                            else
                                ss.unpressed_text = " ";
                            break;
                    }

                ss.pressed_text = NULL;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );

                if( is_player ) {

                    UBYTE race;

                    if( gpd.flags & NWFP_OWNPLAYER )
                        race = GSR->SelRace;
                    else
                        race =  GSR->player2[ gpd.number ].race;

                    /*** Rasse ***/
                    switch( race ) {
                        case FREERACE_USER:
                             status[0] = 'P'; break;
                        case FREERACE_KYTERNESER:
                             status[0] = 'R'; break;
                        case FREERACE_MYKONIER:
                             status[0] = 'T'; break;
                        case FREERACE_TAERKASTEN:
                             status[0] = 'V'; break;
                        default:
                             status[0] = ' '; break;
                        }

                    /*** trouble mit Rassenauswahl ***/
                    if( GSR->player2[ gpd.number ].trouble ) {

                        if( (GSR->global_time / 300) & 1)
                            status[1] = 'f';
                        else
                            status[1] = ' ';
                        }
                    else status[1] = ' ';

                    /*** schon fertig? ***/
                    if( GSR->player2[ gpd.number ].ready_to_start )
                        status[2] = 'h';
                    else
                        status[2] = ' ';
                        
                    /*** CD eingelegt ***/
                    if( GSR->player2[ gpd.number ].cd ) 
                        status[3] = 'i';
                    else
                        status[3] = ' ';

                    /*** Stringabschlusss ***/
                    status[4]     = ' ';
                    status[5]     = 0;
                    }
                else {
                    status[0]     = ' ';
                    status[1]     = ' ';
                    status[2]     = ' ';
                    status[3]     = ' ';
                    status[4]     = ' ';
                    status[5]     = 0;
                    }

                ss.number         = merke_number;
                ss.unpressed_text = status;
                _methoda( GSR->bnet, BTM_SETSTRING, &ss );

                gpd.number++;
                count++;
                }

            break;
        }

///

#endif

}


/// "cancel fuer Settings"
void yw_CancelSettings( struct GameShellReq *GSR )
{
    struct setbuttonstate_msg sbs;
    struct selectbutton_msg   sb;
    struct switchpublish_msg  sp;
    struct setstring_msg      ss;
    struct video_node        *vnode;
    struct bt_propinfo       *prop;

    GSR->shell_mode       = SHELLMODE_TITLE;
    GSR->settings_changed = 0L;

    vnode = (struct video_node *) GSR->videolist.mlh_Head;
    while( vnode->node.mln_Succ ) {

        if( GSR->ywd->GameRes == vnode->modus )
            break;
        vnode = (struct video_node *) vnode->node.mln_Succ;
        }
    GSR->new_modus = GSR->ywd->GameRes;

    /*** Videomode-Gadget setzen ***/
    ss.number         = GSID_VMENU_BUTTON;
    ss.unpressed_text = vnode->name;
    ss.pressed_text   = NULL;
    _methoda( GSR->bvideo, BTM_SETSTRING, &ss );

    /*** 3D-Gadget setzen ***/
    ss.number         = GSID_3DMENU_BUTTON;
    ss.unpressed_text = GSR->d3dname;
    ss.pressed_text   = NULL;
    _methoda( GSR->bvideo, BTM_SETSTRING, &ss );

    GSR->new_d3dname  = GSR->d3dname;
    GSR->new_d3dguid  = GSR->d3dguid;


    /*** Kanalvertauschung ***/
    if( GSR->sound_flags & SF_INVERTLR )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_SOUND_LR;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** CD-Sound ***/
    if( GSR->sound_flags & SF_CDSOUND )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_CDSOUND;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** Sichttiefe ***/
    if( GSR->video_flags & VF_FARVIEW )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_FARVIEW;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** Himmel ***/
    if( GSR->video_flags & VF_HEAVEN )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_HEAVEN;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** 16Bit Texture Schalter ***/
    if( GSR->video_flags & VF_16BITTEXTURE )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_16BITTEXTURE;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** OpenGL? ***/
    if( GSR->video_flags & VF_DRAWPRIMITIVE )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_DRAWPRIMITIVE;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** Softmouse ***/
    if( GSR->video_flags & VF_SOFTMOUSE )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_SOFTMOUSE;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** Enemyindikator ***/
    if( GSR->enemyindicator )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_ENEMYINDICATOR;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    /*** DestFX   ruecksetzen ***/
    sb.number = GSID_FXSLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    prop->value = GSR->destfx;
    _methoda( GSR->bvideo, BTM_REFRESH, &sb );

    /*** FXVolume ruecksetzen ***/
    sb.number = GSID_FXVOLUMESLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    prop->value = GSR->fxvolume;
    _methoda( GSR->bvideo, BTM_REFRESH, &sb );

    /*** CDVolume ruecksetzen ***/
    sb.number = GSID_CDVOLUMESLIDER;
    prop = (struct bt_propinfo *) _methoda( GSR->bvideo, BTM_GETSPECIALINFO, &sb );
    prop->value = GSR->cdvolume;
    _methoda( GSR->bvideo, BTM_REFRESH, &sb );

    /*** Der VideoRequester wurde geschlossen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bvideo, BTM_SWITCHPUBLISH, &sp );
    if( !(GSR->vmenu.Req.flags & REQF_Closed) )
        yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req) );
    if( !(GSR->d3dmenu.Req.flags & REQF_Closed) )
        yw_CloseReq(GSR->ywd, &(GSR->d3dmenu.Req) );

    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );

    sbs.state = SBS_UNPRESSED;
    sbs.who   = GSID_VMENU_BUTTON;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
    sbs.who   = GSID_3DMENU_BUTTON;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
}
///


/// "Cancel fuer DiskReq. (mit schliessen)"
void yw_CancelDisk( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;
    struct setbuttonstate_msg sbs;

    /*** Der EA-Requester wurde geschlossen ***/
    sp.modus        = SP_NOPUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );

    if( 0 == GSR->d_fromwhere ) {
        GSR->shell_mode     = SHELLMODE_TITLE;
        sp.modus = SP_PUBLISH;
        _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
        }
    else {
        GSR->shell_mode     = SHELLMODE_PLAY;
        sp.modus = SP_PUBLISH;
        _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
        }

    sbs.state = SBS_UNPRESSED;
    sbs.who   = GSID_VMENU_BUTTON;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    GSR->D_InputMode = DIM_NONE;
}
///


/// "Cancel fuer Input"
void yw_CancelInput( struct GameShellReq *GSR )
{
    struct setbuttonstate_msg sbs;
    struct switchpublish_msg sp;

    /*** Alles rücksetzen ***/
    GSR->shell_mode = SHELLMODE_TITLE;
    yw_RestoreKeys( GSR );

    /*** Rücksetzen des gadgets! ***/
    sbs.who   = GSID_INPUT;
    sbs.state = SBS_UNPRESSED;
    _methoda( GSR->UBalken, BTM_SETSTATE, &sbs );

    GSR->input_changed = 0L;

    if( GSR->joystick )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_JOYSTICK;
    _methoda( GSR->binput, BTM_SETSTATE, &sbs );

    if( GSR->altjoystick )
        sbs.state = SBS_PRESSED;
    else
        sbs.state = SBS_UNPRESSED;
    sbs.who = GSID_ALTJOYSTICK;
    _methoda( GSR->binput, BTM_SETSTATE, &sbs );

    if( GSR->ywd->Prefs.Flags & YPA_PREFS_FFDISABLE )
        sbs.state = SBS_UNPRESSED;
    else
        sbs.state = SBS_PRESSED;
    sbs.who = GSID_FORCEFEEDBACK;
    _methoda( GSR->binput, BTM_SETSTATE, &sbs );

    /*** Der InputRequester wurde geschlossen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->binput, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->imenu.Req) );

    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
}
///


/// "Cancel fuer Lokale"
void yw_CancelLocale( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;

    /*** new_sel wieder sel ***/
    GSR->shell_mode     = SHELLMODE_TITLE;
    GSR->new_lsel       = GSR->lsel;
    GSR->locale_changed = 0L;

    /*** Der EA-Requester wurde geschlossen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->blocale, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->lmenu.Req) );

    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
}
///


/// "OK fuer Settings"
void yw_OKSettings( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;
    struct snd_cdvolume_msg  cdv;
    struct snd_cdcontrol_msg cd;
    struct setbuttonstate_msg sbs;
    struct ypaworld_data *ywd;
    BOOL   setvideomode;
    BOOL   set3dmode;
    struct setgamevideo_msg sgv;
    ULONG  old_modus;

    ywd = GSR->ywd;

    setvideomode = FALSE;
    set3dmode    = FALSE;

    sgv.forcesetvideo = FALSE;

    /*** Videomode uebernehmen ***/
    if( (GSR->settings_changed & SCF_MODE) &&
        (GSR->new_modus != GSR->ywd->GameRes) &&
        (GSR->new_modus != 0L)) {

        old_modus         = GSR->ywd->GameRes;
        GSR->ywd->GameRes = GSR->new_modus;

        /*** Realisierung ***/
        setvideomode = TRUE;
        }

    /*** 3D Device uebernehmen ***/
    if( (GSR->settings_changed & SCF_3DDEVICE) &&
        (GSR->new_d3dguid != NULL) &&
        (stricmp(GSR->new_d3dguid, GSR->d3dguid)) ) {

        struct windd_device wdm;

        strcpy( GSR->d3dname, GSR->new_d3dname);
        strcpy( GSR->d3dguid, GSR->new_d3dguid);

        /*** Device setzen ***/
        wdm.name  = GSR->d3dname;
        wdm.guid  = GSR->d3dguid;
        wdm.flags = 0;
        _methoda( GSR->ywd->GfxObject, WINDDM_SetDevice, &wdm );
        
        /* ----------------------------------------------------
        ** Die Aenderung des 3D Devices aendert die videoListe.
        ** zur sicherheit immer auf 640x480 stellen.
        ** --------------------------------------------------*/
        old_modus         = GFX_GAME_DEFAULT_RES;
        GSR->ywd->GameRes = GFX_GAME_DEFAULT_RES;

        /*** Zum Uebernehmen auch videomode setzen ***/
        set3dmode = TRUE;
        sgv.forcesetvideo = TRUE;
        }
        
    /*** 16BitTexture ***/
    if( GSR->settings_changed & SCF_16BITTEXTURE ) {
        if( GSR->new_video_flags & VF_16BITTEXTURE ) {
            GSR->video_flags |=  VF_16BITTEXTURE;
            _set(GSR->ywd->GfxObject,WINDDA_16BitTextures,TRUE);
            }
        else {
            GSR->video_flags &= ~VF_16BITTEXTURE;
            _set(GSR->ywd->GfxObject,WINDDA_16BitTextures,FALSE);
            }
        old_modus = GSR->ywd->GameRes;
        set3dmode = TRUE;
        sgv.forcesetvideo = TRUE;
        }

    /*** DrawPrimitive ***/
    if( GSR->settings_changed & SCF_DRAWPRIMITIVE ) {
        if( GSR->new_video_flags & VF_DRAWPRIMITIVE ) {
            GSR->video_flags |= VF_DRAWPRIMITIVE;
            _set(GSR->ywd->GfxObject,WINDDA_UseDrawPrimitive,TRUE);
            }
        else {
            GSR->video_flags &= ~VF_DRAWPRIMITIVE;
            _set(GSR->ywd->GfxObject,WINDDA_UseDrawPrimitive,FALSE);
            }
        old_modus = GSR->ywd->GameRes;
        set3dmode = TRUE;
        sgv.forcesetvideo = TRUE;
        }

    if( (setvideomode && GSR->ywd->OneDisplayRes) ||
        (set3dmode) ) {

        /* ----------------------------------------------------------------
        ** jetzt kommt ein Hack, denn ich darf die Auflösung
        ** trotz des setzens  nicht veraendern, wenn nicht explizit OneDM
        ** angegeben ist. Achtung nach dem Oeffnen der Shell ist die Video-
        ** liste neu!
        ** --------------------------------------------------------------*/
        int i = 0;
        struct video_node *vn;
        
        if( (!GSR->ywd->OneDisplayRes) && setvideomode )
            sgv.modus = old_modus;
        else
            sgv.modus = GSR->ywd->GameRes;

        _methoda( GSR->ywd->world, YWM_SETGAMEVIDEO, &sgv );
        
        /*** Was immer auch gesetzt wurde, Liste und Gadget neu setzen ***/
        vn = (struct video_node *) GSR->videolist.mlh_Head;
        while( vn->node.mln_Succ ) {
        
            if( vn->modus == GSR->ywd->GameRes ) {
            
                struct setstring_msg ss;
                
                GSR->v_actualitem = i;
                ss.number          = GSID_VMENU_BUTTON;
                ss.unpressed_text  = vn->name;
                ss.pressed_text    = NULL;
                _methoda( GSR->bvideo, BTM_SETSTRING, &ss );
                break;
                }
            i++;
            vn = (struct video_node *) vn->node.mln_Succ;
            }
        }

    /*** CD Sound ***/
    if( GSR->settings_changed & SCF_CDSOUND ) {

        if( GSR->new_sound_flags & SF_CDSOUND ) {
            GSR->sound_flags |= SF_CDSOUND;
            ywd->Prefs.Flags |= YPA_PREFS_CDSOUNDENABLE;

            /*** Einschalten ... ***/
            cd.command = SND_CD_SWITCH;
            cd.para    = 1;
            _ControlCDPlayer( &cd );

            /*** ...und CD-Sound starten ***/
            if( GSR->shelltrack ) {

                cd.command   = SND_CD_SETTITLE;
                cd.para      = GSR->shelltrack;
                cd.min_delay = ywd->gsr->shell_min_delay;
                cd.max_delay = ywd->gsr->shell_max_delay;
                _ControlCDPlayer( &cd );
                cd.command = SND_CD_PLAY;
                _ControlCDPlayer( &cd );
                }
            }
        else {
            GSR->sound_flags &= ~SF_CDSOUND;
            ywd->Prefs.Flags &= ~YPA_PREFS_CDSOUNDENABLE;

            /*** erst stoppen (!) ... ***/
            cd.command = SND_CD_STOP;
            _ControlCDPlayer( &cd );

            /*** und dann Ausschalten ***/
            cd.command = SND_CD_SWITCH;
            cd.para    = 0;
            _ControlCDPlayer( &cd );
            }
        }

    /*** Kanal vertauschen ***/
    if( GSR->settings_changed & SCF_INVERT ) {

        if( GSR->new_sound_flags & SF_INVERTLR ) {
            GSR->sound_flags |=  SF_INVERTLR;
            _AE_SetAttrs( AET_ReverseStereo, TRUE, TAG_DONE );
            }
        else {
            GSR->sound_flags &= ~SF_INVERTLR;
            _AE_SetAttrs( AET_ReverseStereo, FALSE, TAG_DONE );
            }
        }

    /*** Sichttiefe ***/
    if( GSR->settings_changed & SCF_FARVIEW ) {

        if( GSR->new_video_flags & VF_FARVIEW ) {
            GSR->video_flags |= VF_FARVIEW;
            yw_SetFarView( ywd->world, TRUE );
            }
        else {
            GSR->video_flags &= ~VF_FARVIEW;
            yw_SetFarView( ywd->world, FALSE );
            }
        }

    /*** Himmel ***/
    if( GSR->settings_changed & SCF_HEAVEN ) {

        if( GSR->new_video_flags & VF_HEAVEN ) {
            GSR->video_flags |=  VF_HEAVEN;
            _set( ywd->world, YWA_RenderHeaven, TRUE );
            }
        else {
            GSR->video_flags &= ~VF_HEAVEN;
            _set( ywd->world, YWA_RenderHeaven, FALSE );
            }
        }

    /*** Softmouse ***/
    if( GSR->settings_changed & SCF_SOFTMOUSE ) {

        if( GSR->new_video_flags & VF_SOFTMOUSE ) {
            GSR->video_flags      |=  VF_SOFTMOUSE;
            GSR->ywd->Prefs.Flags |=  YPA_PREFS_SOFTMOUSE;
            // FIXME_FLOH
            _set(GSR->ywd->GfxObject,WINDDA_CursorMode,WINDD_CURSORMODE_SOFT);
            }
        else {
            GSR->video_flags      &= ~VF_SOFTMOUSE;
            GSR->ywd->Prefs.Flags &= ~YPA_PREFS_SOFTMOUSE;
            // FIXME_FLOH
            _set(GSR->ywd->GfxObject,WINDDA_CursorMode,WINDD_CURSORMODE_HW);
            }
        }

    /*** Enemyindikator ***/
    if( GSR->settings_changed & SCF_ENEMYINDICATOR ) {

        GSR->enemyindicator = GSR->new_enemyindicator;
        if( GSR->new_enemyindicator ) {
            GSR->ywd->Prefs.Flags |=  YPA_PREFS_INDICATOR;
            }
        else {
            GSR->ywd->Prefs.Flags &= ~YPA_PREFS_INDICATOR;
            }
        }

    /*** FX-Slider ***/
    if( GSR->settings_changed & SCF_FXNUMBER ) {

        GSR->destfx = GSR->new_destfx;
        yw_SetDestFX( GSR );
        }

    /*** CD-Volume ***/
    if( GSR->settings_changed & SCF_CDVOLUME ) {

        GSR->cdvolume = GSR->new_cdvolume;
        cdv.volume    = GSR->new_cdvolume;
        _SetCDVolume( &cdv );
        }

    /*** FX-Volume ***/
    if( GSR->settings_changed & SCF_FXVOLUME ) {

        GSR->fxvolume = GSR->new_fxvolume;
        _AE_SetAttrs( AET_MasterVolume, (LONG)(GSR->fxvolume), TAG_DONE );
        }

    GSR->settings_changed = 0L;
    GSR->shell_mode       = SHELLMODE_TITLE;

    /*** Alles schliessen ***/
    sp.modus          =  SP_NOPUBLISH;
    _methoda( GSR->bvideo, BTM_SWITCHPUBLISH, &sp );
    if( !(GSR->vmenu.Req.flags & REQF_Closed) )
        yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req) );
    if( !(GSR->d3dmenu.Req.flags & REQF_Closed) )
        yw_CloseReq(GSR->ywd, &(GSR->d3dmenu.Req) );

    sbs.state = SBS_UNPRESSED;
    sbs.who   = GSID_VMENU_BUTTON;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );
    sbs.who   = GSID_3DMENU_BUTTON;
    _methoda( GSR->bvideo, BTM_SETSTATE, &sbs );

    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
}
///


/// "OK fuer Locale"
void yw_OKLocale( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;
    struct localeinfonode *nd;
    int    n;

    /*** Nochmal uebernehmen, weil evtl. per Taste ausgewaehlt ***/
    n  = GSR->lmenu.Selected;
    nd = (struct localeinfonode *) GSR->localelist.mlh_Head;
    while( n-- ) nd = (struct localeinfonode *) nd->node.mln_Succ;

    GSR->new_lsel = nd;

    /* --------------------------------------------------
    ** Sprache uebernehmen, wegen tastenauswahl nicht auf
    ** LCF_LOCALE testen.
    ** ------------------------------------------------*/
    if( (GSR->new_lsel != GSR->lsel) &&
        (GSR->new_lsel != NULL) ) {

        GSR->lsel = GSR->new_lsel;
        _methoda( GSR->ywd->world, YWM_SETGAMELANGUAGE, GSR );
        }

    GSR->locale_changed = 0L;

    /*** new_sel wieder sel ***/
    GSR->shell_mode     = SHELLMODE_TITLE;
    GSR->new_lsel       = GSR->lsel;
    GSR->locale_changed = 0L;

    /*** Der EA-Requester wurde geschlossen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->blocale, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->lmenu.Req) );

    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );

}
///


/// "OK fuer Providerauswahl"
void yw_OKProvider( struct GameShellReq  *GSR )
{
    if( GSR->NSel >= 0 ) {

        struct getprovidername_msg gpn;
        struct setprovider_msg     sp;

        gpn.number = GSR->NSel;

        if( _methoda( GSR->ywd->nwo, NWM_GETPROVIDERNAME, &gpn ) ) {

            ULONG typ;

            sp.name = gpn.name;

            #ifdef __WINDOWS__
            /*** für Windows ***/
            _methoda( GSR->ywd->GfxObject, WINDDM_EnableGDI, NULL);
            #endif

            if( _methoda( GSR->ywd->nwo, NWM_SETPROVIDER, &sp ) ) {

                GSR->n_selmode        = NM_PLAYER;
                GSR->NSel             = -1; // nix ausg.
                GSR->nmenu.FirstShown = 0;
                
                /*** fuer Player Listview ausschalten ***/
                yw_CloseReq(GSR->ywd, &(GSR->nmenu.Req));
                
                /*** playername schon mal hoilen ***/
                strcpy( GSR->N_Name, GSR->NPlayerName ); 
                GSR->NCursorPos       = strlen(GSR->N_Name);
                }

            #ifdef __WINDOWS__
            _methoda( GSR->ywd->GfxObject, WINDDM_DisableGDI, NULL);
            #endif

            /* -----------------------------------------------------
            ** Datenrate einstellen. Dabei nach Typ, weil Baudraten-
            ** abfrage nicht funktioniert
            ** ---------------------------------------------------*/
            typ = _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL );
            if( (NWFC_MODEM == typ) || (NWFC_SERIAL == typ) ) {

                /*** Das ist zu langsam ***/
                GSR->flush_time_normal  = FLUSH_TIME_REDUCED;
                GSR->update_time_normal = UPDATE_TIME_REDUCED;
                }
            else {

                /*** Das ist schnell ***/
                GSR->flush_time_normal  = FLUSH_TIME_NORMAL;
                GSR->update_time_normal = UPDATE_TIME_NORMAL;
                }
            }
        }
}
///


/// "OK fuer Levelauswahl"
void yw_OKLevel( struct GameShellReq *GSR )
{
    /*** OK fuer Levelauswahl ***/

    /* ----------------------------------------------------
    ** Erzeugen einer neuen oder Aendern einer bestehenden
    ** Session. Wenn wir einen Sessionnamen erfragen
    ** koennen, so gibt es bereits eine Session, deren
    ** Namen wir aendern und wo wir eine Message zur Level-
    ** aenderung losschicken (die in jedem Falle schneller
    ** als eine DPlay-interne Sessionnamensaenderung sein
    ** duerfte.
    ** --------------------------------------------------*/
    if( GSR->NSel < 0 ) return;

    if( _methoda(GSR->ywd->nwo, NWM_GETSESSIONSTATUS, NULL)) {
    
        struct LevelNode *l;
        BOOL   race_selected = FALSE;
        ULONG  plys, pl;
        struct ypamessage_changelevel cl;
        struct sendmessage_msg sm;
        struct flushbuffer_msg fb;
        char   sname[300];
        struct setsessionname_msg ssn;
           
        /*** Rassen analog Lobby fest verteilen ***/
        l = &(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ]);
        plys = _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL);
        pl   = 0;
        
        GSR->n_selmode        = NM_MESSAGE;
        GSR->N_Name[ 0 ]      = 0;
        GSR->NCursorPos       = 0;
        GSR->nmenu.FirstShown = 0;
        GSR->NSel             = -1;
        
        /* ---------------------------------------------
        ** Rassen festklopfen. Alle nich vorhandenen und
        ** von den vorhandenen die, fuer die ein Spieler
        ** da ist, ausschalten. Als host nehme ich mir
        ** die erste 
        ** -------------------------------------------*/
        if( l->races & 2 ) {
            GSR->FreeRaces &= ~FREERACE_USER;
            GSR->SelRace    =  FREERACE_USER;
            race_selected   =  TRUE;
            plys--;
            GSR->player2[ pl++ ].race = FREERACE_USER;
            }
        else  GSR->FreeRaces &= ~FREERACE_USER;   
            
        if( l->races & 64 ) {
            if( plys > 0 ) {
                GSR->FreeRaces  &= ~FREERACE_KYTERNESER;
                GSR->player2[ pl++ ].race = FREERACE_KYTERNESER;
                plys--;
                if( !race_selected ) {
                    GSR->SelRace  = FREERACE_KYTERNESER;
                    race_selected = TRUE;
                    }
                }
            }
        else  GSR->FreeRaces  &= ~FREERACE_KYTERNESER;      
                     
        if( l->races & 8 ) {
            if( plys > 0 ) {
                GSR->FreeRaces  &= ~FREERACE_MYKONIER;
                GSR->player2[ pl++ ].race = FREERACE_MYKONIER;
                plys--;
                if( !race_selected ) {
                    GSR->SelRace  = FREERACE_MYKONIER;
                    race_selected = TRUE;
                    }
                }
            }
        else  GSR->FreeRaces  &= ~FREERACE_MYKONIER;       
                    
        if( l->races & 16 ) {
            if( plys > 0 ) {
                GSR->FreeRaces  &= ~FREERACE_TAERKASTEN;
                plys--;
                GSR->player2[ pl++ ].race = FREERACE_TAERKASTEN;
                if( !race_selected ) {
                    GSR->SelRace  = FREERACE_TAERKASTEN;
                    race_selected = TRUE;
                    }
                }
            }
        else  GSR->FreeRaces  &= ~FREERACE_TAERKASTEN;                
        
        /*** Message dazu verschicken ***/
        strncpy( cl.hostname, GSR->NPlayerName, STANDARD_NAMELEN );
        cl.levelnum           = GSR->NLevelOffset;
        cl.generic.message_id = YPAM_CHANGELEVEL;
        cl.generic.owner      = 0; // weil nicht feststehend

        sm.data                = (char *) &cl;
        sm.data_size           = sizeof( cl );
        sm.receiver_kind       = MSG_ALL;
        sm.receiver_id         = NULL;
        sm.guaranteed          = TRUE;
        _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);
        
        /*** nach dem Aendern des levels Checksumme neu berechnen ***/
        yw_SendCheckSum( GSR->ywd, GSR->NLevelOffset );

        fb.sender_kind         = MSG_PLAYER;
        fb.sender_id           = GSR->NPlayerName;
        fb.receiver_kind       = MSG_ALL;
        fb.receiver_id         = NULL;
        fb.guaranteed          = TRUE;
        _methoda( GSR->ywd->nwo, NWM_FLUSHBUFFER, &fb);
    
        /*** Sessionname aendern ***/
        sprintf( sname, "%d%s%s%s%s\0", GSR->NLevelOffset,
                 SEPHOSTFROMSESSION, GSR->NPlayerName,
                 SEPHOSTFROMSESSION, GSR->ywd->Version );
        ssn.name = sname;
        _methoda( GSR->ywd->nwo, NWM_SETSESSIONNAME, &ssn );

        }
    else {
                
        /*** OK, ganz normal eine neue Session oeffnen ***/
        struct createplayer_msg cp;

        /*** Im Remotastartmodus nur merken und weiter ***/
        if( GSR->remotestart ) {
        
            struct sendmessage_msg sm;
            struct ypamessage_lobbyinit li;
            struct flushbuffer_msg fb;
            struct LevelNode *l;
            BOOL   race_selected = FALSE;
            ULONG  plys, pl;
               
            l = &(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ]);
            plys = _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL);
            pl   = 0;
            
            GSR->n_selmode        = NM_MESSAGE;
            GSR->N_Name[ 0 ]      = 0;
            GSR->NCursorPos       = 0;
            GSR->nmenu.FirstShown = 0;
            
            /* ---------------------------------------------
            ** Rassen festklopfen. Alle nich vorhandenen und
            ** von den vorhandenen die, fuer die ein Spieler
            ** da ist, ausschalten. Als host nehme ich mir
            ** die erste 
            ** -------------------------------------------*/
            if( l->races & 2 ) {
                GSR->FreeRaces &= ~FREERACE_USER;
                GSR->SelRace    =  FREERACE_USER;
                race_selected   =  TRUE;
                plys--;
                GSR->player2[ pl++ ].race = FREERACE_USER;
                }
            else  GSR->FreeRaces &= ~FREERACE_USER;   
                
            if( l->races & 64 ) {
                if( plys > 0 ) {
                    GSR->FreeRaces  &= ~FREERACE_KYTERNESER;
                    GSR->player2[ pl++ ].race = FREERACE_KYTERNESER;
                    plys--;
                    if( !race_selected ) {
                        GSR->SelRace  = FREERACE_KYTERNESER;
                        race_selected = TRUE;
                        }
                    }
                }
            else  GSR->FreeRaces  &= ~FREERACE_KYTERNESER;      
                         
            if( l->races & 8 ) {
                if( plys > 0 ) {
                    GSR->FreeRaces  &= ~FREERACE_MYKONIER;
                    GSR->player2[ pl++ ].race = FREERACE_MYKONIER;
                    plys--;
                    if( !race_selected ) {
                        GSR->SelRace  = FREERACE_MYKONIER;
                        race_selected = TRUE;
                        }
                    }
                }
            else  GSR->FreeRaces  &= ~FREERACE_MYKONIER;       
                        
            if( l->races & 16 ) {
                if( plys > 0 ) {
                    GSR->FreeRaces  &= ~FREERACE_TAERKASTEN;
                    plys--;
                    GSR->player2[ pl++ ].race = FREERACE_TAERKASTEN;
                    if( !race_selected ) {
                        GSR->SelRace  = FREERACE_TAERKASTEN;
                        race_selected = TRUE;
                        }
                    }
                }
            else  GSR->FreeRaces  &= ~FREERACE_TAERKASTEN;                
             
            /* ----------------------------------
            ** Wenn wir hier vorbeikommen, sind
            ** wir host und duerfen eine INIT-Msg
            ** an das Volk losschicken
            ** --------------------------------*/
            strncpy( li.hostname, GSR->NPlayerName, STANDARD_NAMELEN );
            li.levelnum           = GSR->NLevelOffset;
            li.generic.message_id = YPAM_LOBBYINIT;
            li.generic.owner      = 0; // weil nicht feststehend

            sm.data                = (char *) &li;
            sm.data_size           = sizeof( li );
            sm.receiver_kind       = MSG_ALL;
            sm.receiver_id         = NULL;
            sm.guaranteed          = TRUE;
            _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);
            
            fb.sender_kind         = MSG_PLAYER;
            fb.sender_id           = GSR->NPlayerName;
            fb.receiver_kind       = MSG_ALL;
            fb.receiver_id         = NULL;
            fb.guaranteed          = TRUE;
            _methoda( GSR->ywd->nwo, NWM_FLUSHBUFFER, &fb);

            return;
            }

        if( GSR->is_host ) {

            /* ------------------------------------------
            ** Die Session existiert noch nicht. Wir
            ** basteln aus allen Namen den Sessionnamen
            ** und versuchen diese zu öffnen. Wenn
            ** dies erfolgreich war, gehts normal weiter,
            ** weil immer eine Session aktiv ist.
            ** NEU: Nummer statt Namen wegen Localizing!
            ** Neu: Jetzt auch mit Versionstring
            ** ----------------------------------------*/
            struct opensession_msg os;
            char   sname[ 300 ];
            ULONG   r;

            sprintf( sname, "%d%s%s%s%s\0", GSR->NLevelOffset,
                     SEPHOSTFROMSESSION, GSR->NPlayerName,
                     SEPHOSTFROMSESSION, GSR->ywd->Version );

            os.name       = sname;
            os.maxplayers = 4;  // irgendwoher genauer!!!

            /*** bei Modem Bildschirm umschalten ***/
            if( NWFC_MODEM == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL ))
                _methoda( GSR->ywd->GfxObject, WINDDM_EnableGDI, NULL);

            r = _methoda( GSR->ywd->nwo, NWM_OPENSESSION, &os );
            
            if( NWFC_MODEM == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL ))
                _methoda( GSR->ywd->GfxObject, WINDDM_DisableGDI, NULL);

            if( !r )    
               return;
            }

        /*** Jetzt kann der Spieler erzeugt werden ***/
        cp.name  = GSR->NPlayerName;
        cp.flags = NWFP_OWNPLAYER;
        if( _methoda( GSR->ywd->nwo, NWM_CREATEPLAYER, &cp ) ) {

            LONG np, cd = 0;

            /*** Ab jetzt müssen wir Messages empfangen ***/
            GSR->ywd->playing_network = TRUE;

            GSR->NSel             = -1;
            GSR->n_selmode        = NM_MESSAGE;
            GSR->N_Name[ 0 ]      = 0;
            GSR->NCursorPos       = 0;
            GSR->nmenu.FirstShown = 0;

            /*** Name in player2 merken ***/
            _get( GSR->ywd->nwo, NWA_NumPlayers, &np );
            yw_StrUpper( GSR->player2[ np-1 ].name, cp.name);

            /*** Pauschal bekommt er die erste freie Rasse ***/
            yw_DoRaceInit( GSR );
            
            /*** Als Host bin ich immer "ready to start" ***/
            GSR->ReadyToStart                 = 1;
            GSR->player2[np-1].ready_to_start = 1;
            
            /* ----------------------------------------------------
            ** CD handling. Als Host nur registrieren. weil ich
            ** der erste bin, brauche ich keine Message zu schicken
            ** --------------------------------------------------*/
            cd = yw_CheckCD( FALSE, FALSE, NULL, NULL );
            
            GSR->player2[np-1].cd  = (UBYTE) cd; 
            GSR->cd                = (UBYTE) cd;
            GSR->last_cdcheck = GSR->global_time;
            }
        }
}
///


/// "OK fuer Sessionauswahl"
void yw_OKSessions( struct GameShellReq *GSR )
{

    struct ypaworld_data *ywd;
    ywd = GSR->ywd;

    if( (NWFC_MODEM == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL )) &&
        (GSR->NSel   < 0) ) {

        ULONG ret;
        
        /* ------------------------------------------
        ** Modem, aber noch keine Session ausgwaehlt.
        ** Somit sollten wir mal nachfragen
        ** ----------------------------------------*/
        #ifdef __WINDOWS__
        if( NWFC_MODEM == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL ))
            _methoda( GSR->ywd->GfxObject, WINDDM_EnableGDI, NULL);
        #endif

        ret = _methoda( GSR->ywd->nwo, NWM_ASKSESSIONS, NULL );

        /* ------------------------------------------------------------
        ** von nun an ist es im Modemfalle erlaubt, nach sessions zu
        ** fragen. Das natuerlich nur, wenn alles korrekt initialisiert
        ** wurde.
        ** ----------------------------------------------------------*/
        if( ret )
            GSR->modem_ask_session = TRUE;
               
        #ifdef __WINDOWS__
        if( NWFC_MODEM == _methoda( GSR->ywd->nwo, NWM_GETPROVIDERTYPE, NULL ))
            _methoda( GSR->ywd->GfxObject, WINDDM_DisableGDI, NULL);
        #endif
        }
    else {
        
        /* ------------------------------------------
        ** Es wurde eine Session ausgewählt. Wir sind
        ** demzufolge nicht Host
        ** ----------------------------------------*/
        struct getsessionname_msg gsn;
        
        gsn.number = GSR->NSel;
        if( _methoda( GSR->ywd->nwo, NWM_GETSESSIONNAME, &gsn)){

            struct joinsession_msg js;
            struct getplayerdata_msg gpd;
            js.name = gsn.name;

            /*** Es ist eine neue Session ***/
            if( _methoda( GSR->ywd->nwo, NWM_JOINSESSION, &js)) {

                char buffer[ 200 ];
                struct createplayer_msg cp;
                struct ypamessage_cd cdm;
                struct sendmessage_msg sm;

                GSR->N_Name[ 0 ] = 0;
                GSR->NCursorPos  = 0;

                /* ------------------------------------
                ** Aus dem Sessionnamen die levelnummer
                ** ermitteln
                ** ----------------------------------*/
                strcpy( buffer, js.name );
                if( strtok( buffer, SEPHOSTFROMSESSION ))
                    GSR->NLevelOffset = atol( buffer );
                else
                    GSR->NLevelOffset = 0;
                    
                GSR->NLevelName = ypa_GetStr( GlobalLocaleHandle,
                                  STR_NAME_LEVELS + GSR->NLevelOffset,
                                  GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ].title);
                GSR->nmenu.FirstShown = 0;

                /* ------------------------------------------
                ** Zu allen Spielern Namen merken. Gleich-
                ** zeitig gucken wir, ob mein Name schon
                ** da ist. Wenn ja "addieren" wir ein zeichen
                ** ----------------------------------------*/
                gpd.number  = 0;
                gpd.askmode = GPD_ASKNUMBER;
                while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {

                    yw_StrUpper( ywd->gsr->player2[ gpd.number ].name,
                                 gpd.name );

                    /*** Nicht ganz wasserdicht...***/
                    if(stricmp( gpd.name, GSR->NPlayerName)==0)
                        strcat( GSR->NPlayerName,"X");

                    gpd.number++;
                    }

                /*** Jetzt Spieler erzeugen ***/
                cp.name  = GSR->NPlayerName;
                cp.flags = NWFP_OWNPLAYER;
                if( _methoda( GSR->ywd->nwo, NWM_CREATEPLAYER, &cp ) ) {

                    LONG np, cd = 0;

                    /*** Ab jetzt müssen wir Messages empfangen ***/
                    GSR->ywd->playing_network = TRUE;

                    GSR->is_host          = FALSE;
                    GSR->NSel             = -1;
                    GSR->n_selmode        = NM_MESSAGE;
                    GSR->N_Name[ 0 ]      = 0;
                    GSR->NCursorPos       = 0;
                    GSR->nmenu.FirstShown = 0;

                    /*** Name in player2 merken ***/
                    _get( ywd->nwo, NWA_NumPlayers, &np );
                    yw_StrUpper( GSR->player2[ np-1 ].name, cp.name);

                    yw_DoRaceInit( GSR );
                    
                    /* --------------------------------------------------
                    ** CD handling. Als Host nur registrieren. weil ich
                    ** mich einer Session anschliesse, muss ich die Leute
                    ** ueber meinen CD Status informieren.
                    ** ------------------------------------------------*/
                    cd = yw_CheckCD( FALSE, FALSE, NULL, NULL );
                    
                    GSR->player2[np-1].cd  = (UBYTE) cd; 
                    GSR->cd                = (UBYTE) cd;
                    GSR->last_cdcheck = GSR->global_time;

                    cdm.cd                 = GSR->cd;
                    cdm.generic.message_id = YPAM_CD;
                    cdm.generic.owner      = 0; // weil nicht feststehend
        
                    sm.data                = (char *) &cdm;
                    sm.data_size           = sizeof( cdm );
                    sm.receiver_kind       = MSG_ALL;
                    sm.receiver_id         = NULL;
                    sm.guaranteed          = TRUE;
                    _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);
                    }

                /*** ReadyButton zuruecksetzen ***/
                gpd.number  = 0;
                gpd.askmode = GPD_ASKNUMBER;
                while( _methoda( ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {

                    /*** Nicht ganz wasserdicht...***/
                    if(stricmp( gpd.name, GSR->NPlayerName)==0)
                        break;

                    gpd.number++;
                    }

                GSR->ReadyToStart  = 0;
                GSR->player2[ gpd.number ].ready_to_start = 0;
                
                /*** Jetzt, wo alles feststeht, CheckSumme uebermitteln ***/
                yw_SendCheckSum( GSR->ywd, GSR->NLevelOffset );
                }
            else {
                
                yw_MessageBox( GSR->ywd, 
                               ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_HEADLINE,
                               "YPA ERROR MESSAGE"),
                               ypa_GetStr( GlobalLocaleHandle, STR_YPAERROR_NOJOIN,
                               "SESSION NOT LONGER AVAILABLE") );
                
                /*** erneut nachfragen ***/
                _methoda( GSR->ywd->nwo, NWM_ASKSESSIONS, NULL ); 
                }
            }
        }
}
///


/// "Oeffnen der Requester"
void yw_OpenInput( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->binput, BTM_SWITCHPUBLISH, &sp );

    GSR->shell_mode = SHELLMODE_INPUT;

    yw_CloseReq(GSR->ywd, &(GSR->imenu.Req));
    yw_OpenReq(GSR->ywd, &(GSR->imenu.Req));
}


void yw_OpenSettings( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->bvideo, BTM_SWITCHPUBLISH, &sp );

    GSR->shell_mode = SHELLMODE_SETTINGS;

    if( !(GSR->vmenu.Req.flags & REQF_Closed) ) {

        yw_CloseReq(GSR->ywd, &(GSR->vmenu.Req));
        yw_OpenReq(GSR->ywd, &(GSR->vmenu.Req));
        }

    /*** Im Menue Selected einstellen ***/
    GSR->vmenu.Selected = GSR->v_actualitem;
}


void yw_OpenDisk( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;
    struct fileinfonode *nd;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );

    GSR->shell_mode = SHELLMODE_DISK;

    /*** Spielzeit fuer aktuellen User erneuern ***/
    nd = (struct fileinfonode *) GSR->flist.mlh_Head;
    while( nd->node.mln_Succ ) {
        if( stricmp( nd->username, GSR->UserName ) == 0 ) {
            nd->global_time = GSR->ywd->GlobalStats[ 1 ].Time;
            break;
            }
        nd = (struct fileinfonode *) nd->node.mln_Succ;
        }

    yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req));
    yw_OpenReq(GSR->ywd, &(GSR->dmenu.Req));
}


void yw_OpenLocale( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;
    struct localeinfonode *lin;
    int    n;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->blocale, BTM_SWITCHPUBLISH, &sp );

    GSR->shell_mode = SHELLMODE_LOCALE;

    yw_CloseReq(GSR->ywd, &(GSR->lmenu.Req));
    yw_OpenReq(GSR->ywd, &(GSR->lmenu.Req));

    n  = 0;
    lin = (struct localeinfonode *) GSR->localelist.mlh_Head;
    while( lin->node.mln_Succ ) {
        if( GSR->lsel == lin ) break;
        n++;
        lin = (struct localeinfonode *) lin->node.mln_Succ;
        }
    GSR->lmenu.Selected = n;
}


void yw_OpenNetwork( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->bnet, BTM_SWITCHPUBLISH, &sp );

    GSR->shell_mode = SHELLMODE_NETWORK;

    yw_CloseReq(GSR->ywd, &(GSR->nmenu.Req));
    yw_OpenReq(GSR->ywd, &(GSR->nmenu.Req));
}


void yw_OpenAbout( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->babout, BTM_SWITCHPUBLISH, &sp );

    GSR->shell_mode = SHELLMODE_ABOUT;
}
///


/// "Videomode setzen"
void yw_AppearVideoMode( struct GameShellReq *GSR, BOOL remotestart )
{

    struct video_node *vnode;
    struct setstring_msg ss;
    int    i;

    if( (GSR->v_actualitem != GSR->vmenu.Selected) &&
        (FALSE == remotestart) ) {

        /* -------------------------------------------------------------------
        ** Es wurde eine Action zur Änderung ausgewählt. Um raus-
        ** zukriegen, was es nun war, müssen wir das Menu genauso durchgrasen,
        ** wie wir es aufgebaut haben.
        ** -----------------------------------------------------------------*/
        GSR->v_actualitem = GSR->vmenu.Selected;
        
        /*** Suchen der zugehörigen Node ***/
        vnode = (struct video_node *) GSR->videolist.mlh_Head;
        i = GSR->v_actualitem;
        while( i-- ) vnode = (struct video_node *) vnode->node.mln_Succ;

        /*** in i-1 steht jetzt der Modus ***/
        GSR->new_modus     = vnode->modus;

        /*** auch öffentlich machen ***/
        ss.number          = GSID_VMENU_BUTTON;
        ss.unpressed_text  = vnode->name;
        ss.pressed_text    = NULL;
        _methoda( GSR->bvideo, BTM_SETSTRING, &ss );
        }
}
///


/// "3D Device setzen"
void yw_Appear3DDevice( struct GameShellReq *GSR, BOOL remotestart )
{

    struct setstring_msg ss;
    int    i;
    char  *actual, *selname, *selguid;
    struct windd_device wdm;

    /*** Aktuelles und  gewuenschtes suchen ***/
    wdm.name  = NULL;
    wdm.guid  = NULL;
    wdm.flags = 0;
    i         = 0;
    do {

        _methoda( GSR->ywd->GfxObject, WINDDM_QueryDevice, &wdm );
        if( wdm.name ) {

            /*** Aktuelles? ***/
            if( wdm.flags & WINDDF_IsCurrentDevice )
                actual = wdm.name;

            /*** selektiertes? ***/
            if( i == GSR->d3dmenu.Selected ) {
                selname = wdm.name;
                selguid = wdm.guid;
                }
            }
        i++;
        }  while( wdm.name );

    if( FALSE   == remotestart ) {

        GSR->new_d3dguid = selguid;
        GSR->new_d3dname = selname;
        
        /*** auch öffentlich machen ***/
        ss.number         = GSID_3DMENU_BUTTON;
        ss.unpressed_text = selname;
        ss.pressed_text   = NULL;
        _methoda( GSR->bvideo, BTM_SETSTRING, &ss );
        }
}
///


/// "EARequester-Routinen"
void yw_EAR_Kill( struct GameShellReq *GSR )
{

    /* -----------------------------------------------------------
    ** Einen SpielerEintrag löschen. Der Name steht in GSR->D_Name
    ** ---------------------------------------------------------*/
    char   fn[ 300 ];
    int    n;
    struct fileinfonode *node, *node2;
    BOOL   has_succ;
    struct switchpublish_msg sp;

    /* -----------------------------------------------------------------------
    ** GSR->d_actualitem wird automatisch aktualisiert. Ist es auf 0, so
    ** entspricht D_Name keinem Eintrag. Dann gibt es aber auch nix zu löschen
    ** ---------------------------------------------------------------------*/
    if( !GSR->d_actualitem ) return;

    /*** Für taste nochmal nen test ***/
    if( stricmp( GSR->D_Name, GSR->UserName ) == 0 ) return;

    /*** Die Node holen ***/
    node = (struct fileinfonode *) GSR->flist.mlh_Head;
    n    = GSR->d_actualitem - 1;
    while( n-- ) node = (struct fileinfonode *) node->node.mln_Succ;

    /*** Hat Node einen Nachfolger? ***/
    if( node->node.mln_Succ->mln_Succ ) {
        node2 = (struct fileinfonode *) node->node.mln_Succ;
        has_succ = TRUE;
        }
    else {
        node2 = (struct fileinfonode *) node->node.mln_Pred;
        has_succ = FALSE;
        }

    /* --------------------------------------------
    ** Alles Files löschen. Dazu gehört das gesamte
    ** Verzeichnis! Die Angabe eines Patterns
    ** funktioniert nicht. Also jedes einzeln lö.
    ** Achtung, Leerzeichen beachten!
    ** ------------------------------------------*/
    sprintf( fn, "save:%s", node->username);

    yw_KillAllFilesInDir( fn );
    _FRemDir( fn );

    /*** Node freigeben und Test, ob es das letzte war ***/
    _Remove( (struct Node *) node );
    GSR->dmenu.NumEntries--;

    if( GSR->flist.mlh_Head == (struct MinNode *)&(GSR->flist.mlh_Tail) ) {

        GSR->d_actualitem = 0;
        strcpy( GSR->D_Name, UNKNOWN_NAME );
        }
    else {

        /*** was ist das neue aktive? ***/
        if( !has_succ ) GSR->d_actualitem--;
        strcpy( GSR->D_Name, node2->username );
        }

    /*** Stringgadget und Namen aktualisieren ***/
    GSR->DCursorPos = strlen( GSR->D_Name );

    if( GSR->d_actualitem )
        yw_PosSelected( &(GSR->dmenu), GSR->d_actualitem - 1 );

    /*** alte Node noch freigeben ***/
    _FreeVec( node );

    /*** inputMode ausschalten ***/
    GSR->D_InputMode = DIM_NONE;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );
    if( 0 == GSR->d_fromwhere ) {
        GSR->shell_mode     = SHELLMODE_TITLE;
        sp.modus = SP_PUBLISH;
        _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
        }
    else {
        GSR->shell_mode     = SHELLMODE_PLAY;
        sp.modus = SP_PUBLISH;
        _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
        }
}


void yw_EAR_Load( struct GameShellReq *GSR )
{

    /* ----------------------------------------------------------------
    ** Lädt Usereinstellungen. Tut dies aber nur, wenn (das automatisch
    ** aktualisierte) GSR->d_actualitem auf etwas zeigt (was sich dann
    ** laden läßt.
    ** --------------------------------------------------------------*/
    struct fileinfonode *finode;
    struct saveloadsettings_msg sls;
    LONG cn = GSR->d_actualitem - 1;
    char filename[ 300 ];
    struct switchpublish_msg sp;

    if( !GSR->d_actualitem ) return;

    /*** Node suchen ***/
    finode = (struct fileinfonode *) GSR->flist.mlh_Head;
    while( cn-- ) finode = (struct fileinfonode *) finode->node.mln_Succ;

    sprintf( filename, "%s/user.txt", finode->username );
    sls.filename = filename;
    sls.username = finode->username;
    sls.mask     = (DM_SCORE|DM_VIDEO|DM_SOUND|DM_SHELL|
                    DM_INPUT|DM_BUILD|DM_BUDDY|DM_USER);
    sls.gsr      = GSR;
    sls.flags    = SLS_OPENSHELL;
    _methoda( GSR->ywd->world, YWM_LOADSETTINGS, &sls );

    /*** Name wird ja nicht mehr gescannt ***/
    yw_LocStrCpy( GSR->UserName, finode->username);
    yw_LocStrCpy( GSR->D_Name, finode->username);

    /*** inputMode ausschalten ***/
    GSR->D_InputMode = DIM_NONE;
    GSR->exist_savegame = 0;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );
    GSR->shell_mode     = SHELLMODE_PLAY;
    sp.modus = SP_PUBLISH;
    _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
}


void yw_EAR_Save( struct GameShellReq *GSR )
{
    /* ------------------------------------------------------------------
    ** Abspeichern unter einem Namen, der in D_Name steht. Dabei wird der
    ** aktuelle Spieler NICHT verändert.
    ** Dazu lege ich ein Verzeichnis mit Userfile an und kopiere alle
    ** #?.fin und #?.sgm-Files.
    ** ----------------------------------------------------------------*/
    APTR   dir;
    struct ncDirEntry entry;
    struct saveloadsettings_msg sls;
    struct fileinfonode *node;
    char   fn[ 300 ];
    struct switchpublish_msg sp;

    /*** neue Node, wenn dieser Spieler noch nicht existiert ***/
    if( !GSR->d_actualitem ) {

        /*** neue Node anlegen ***/
        if( node = (struct fileinfonode *) _AllocVec(
            sizeof( struct fileinfonode ),
            MEMF_PUBLIC | MEMF_CLEAR ) ) {

            char dirname[ 300 ];

            /*** Node ausfüllen ***/
            strncpy( node->username, GSR->D_Name,
                     USERNAMELEN );

            /*** Einklinken ***/
            _AddTail( (struct List *)( &(GSR->flist)),
                      (struct Node *)node );

            /*** Verzeichnis anlegen ***/
            sprintf( dirname, "save:%s", GSR->D_Name);

            if( !_FMakeDir( dirname )) {

                _LogMsg("Unable to create directory %s\n",
                         dirname );
                return;
                }

            /*** Diverses ***/
            GSR->dmenu.NumEntries++;

            /*** Was ist nun actualitem ? ***/
            GSR->d_actualitem = GSR->dmenu.NumEntries;
            }
        else {

            _LogMsg("Warning: No Memory!\n");
            return;
            }
        }
    else {

        /* ------------------------------------------------------------------
        ** Zu diesem Spieler existiert eine Node und ein Verzeichnis. Folg-
        ** lich löschen wir alle Files darin (später kopieren wir alles rüber)
        ** natuerlich nur, wenn UNTER EINEM ANDEREN NAMEN ABGESPEICHERT werden
        ** soll!
        ** ----------------------------------------------------------------*/
        if( stricmp( GSR->D_Name, GSR->UserName ) != 0 ) {
        
            sprintf( fn, "save:%s", GSR->D_Name);
            yw_KillAllFilesInDir( fn );
            }
        }

    /*** Nur mal so die Einstellungen abspeichern. ***/
    sprintf( fn, "%s/user.txt", GSR->D_Name );

    /*** Alles abspeichern ***/
    sls.filename = fn;
    sls.username = GSR->D_Name;
    sls.gsr      = GSR;
    sls.mask     = (DM_USER|DM_INPUT|DM_SOUND|DM_SHELL|
                    DM_SCORE|DM_VIDEO|DM_BUILD|DM_BUDDY);
    sls.flags    = 0;
    if( !_methoda( GSR->ywd->world, YWM_SAVESETTINGS, &sls ))
        _LogMsg("Warning! Error while saving user data for %s\n",
                 GSR->D_Name );

    /* ---------------------------------------------------------------
    ** Nun vom Verzeichnis UserName nach D_Name kopieren, wenn es denn
    ** verschiedene Namen sind
    ** -------------------------------------------------------------*/
    sprintf( fn, "save:%s", GSR->UserName ); // denn von Username her auslesen
    if( stricmp( GSR->D_Name, GSR->UserName ) ) {

        if( dir = _FOpenDir( fn ) ) {

            /*** Nun alle auslesen ***/
            while( _FReadDir( dir, &entry ) ) {

                if( entry.attrs & NCDIR_DIRECTORY )
                    continue;

                if( strstr( entry.name, ".sgm") ||
                    strstr( entry.name, ".SGM") ||
                    strstr( entry.name, ".rst") ||
                    strstr( entry.name, ".RST") ||
                    strstr( entry.name, ".fin") ||
                    strstr( entry.name, ".FIN") ) {

                    /* -------------------------------------------------
                    ** nach D_Name kopieren. Weil wir sowas nicht haben,
                    ** öffnen wir beide Files und schreiben von einem
                    ** in das andere
                    ** -----------------------------------------------*/
                    char von[ 300 ], nach[ 300 ];

                    sprintf( von,  "%s/%s", fn, entry.name );
                    sprintf( nach, "save:%s/%s", GSR->D_Name, entry.name );
                    yw_CopyFile( von, nach );
                    }
                }

            _FCloseDir( dir );
            }
        }
    GSR->D_InputMode = DIM_NONE;
    
    /*** Dieser wird nun aktiv ***/
    strncpy( GSR->UserName, GSR->D_Name, USERNAMELEN );

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );
    if( 0 == GSR->d_fromwhere ) {
        GSR->shell_mode     = SHELLMODE_TITLE;
        sp.modus = SP_PUBLISH;
        _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
        }
    else {
        GSR->shell_mode     = SHELLMODE_PLAY;
        sp.modus = SP_PUBLISH;
        _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
        }
}


void yw_EAR_Create( struct GameShellReq *GSR )
{
    /* -----------------------------------------------------
    ** Sichern des bisherigen Standes/Spielers. dazu suche
    ** ich die Node des jetzigen Spielers und sichere alles.
    ** ---------------------------------------------------*/
    int    n;
    char   fn[ 300 ];
    struct saveloadsettings_msg sls;
    struct switchpublish_msg sp;
    struct fileinfonode *node;

    /*** Filename basten ***/
    sprintf( fn, "%s/user.txt", GSR->UserName );

    /*** Alles abspeichern ***/
    sls.filename = fn;
    sls.username = GSR->UserName;
    sls.gsr      = GSR;
    sls.mask     = (DM_USER|DM_INPUT|DM_SOUND|DM_SHELL|
                    DM_SCORE|DM_VIDEO|DM_BUILD|DM_BUDDY);
    sls.flags    = 0;
    if( !_methoda( GSR->ywd->world, YWM_SAVESETTINGS, &sls ))
        _LogMsg("Warning! Error while saving user data for %s\n",
                 GSR->UserName );

    /* ----------------------------------------------------
    ** Es ist absolut verwirrend, unter einem bestehenden
    ** Namen einen neuen User anzulegen. Also überschreiben
    ** wir ihn.
    ** Dazu reicht es, actualitem zu untersuchen, weil ich
    ** das stets aktualisiere.
    ** --------------------------------------------------*/
    if( !GSR->d_actualitem ) {

        /*** neue Node anlegen ***/
        if( node = (struct fileinfonode *) _AllocVec(
            sizeof( struct fileinfonode ),
            MEMF_PUBLIC | MEMF_CLEAR ) ) {

            char dirname[ 300 ];

            /*** Node ausfüllen ***/
            strncpy( node->username, GSR->D_Name,
                     USERNAMELEN );

            /*** Einklinken ***/
            _AddTail( (struct List *)( &(GSR->flist)),
                      (struct Node *)node );

            /*** Verzeichnis anlegen ***/
            sprintf( dirname, "save:%s", GSR->D_Name);

            if( !_FMakeDir( dirname )) {

                _LogMsg("Unable to create directory %s\n",
                         dirname );
                return;
                }

            /*** Diverses ***/
            strncpy( GSR->UserName, GSR->D_Name,
                     USERNAMELEN );
            GSR->dmenu.NumEntries++;

            /*** Was ist nun actualitem ? ***/
            GSR->d_actualitem = GSR->dmenu.NumEntries;
            }
        else {

            _LogMsg("Warning: No Memory!\n");
            return;
            }
        }
    else {

        /* ------------------------------------------------------------------
        ** Zu diesem Spieler existiert eine Node und ein Verzeichnis. Folg-
        ** lich löschen wir alle Files darin (später kopieren wir alles rüber
        ** ----------------------------------------------------------------*/

        sprintf( fn, "save:%s", GSR->D_Name);

        yw_KillAllFilesInDir( fn );
        }

    /* ------------------------------------------------
    ** Wir haben den neuen Spieler, folglich können
    ** wir mit Aufräumarbeiten beginnen. Diese mache
    ** ich jedoch nur, wenn ein neuer Spieler angelegt
    ** wird. beim kopieren lasse ich alles, wie es ist.
    **
    ** Zuerst killen wir die Buddies
    ** ----------------------------------------------*/
    yw_ClearBuddyArray( GSR->ywd );

    /* ----------------------------------------------
    ** Dann müssen die Prototyp-Arrays auf Vordermann
    ** begracht werden.
    ** --------------------------------------------*/
    yw_KillPrototypeArrays( GSR->ywd );
    if( !yw_InitPrototypeArrays( GSR->ywd )) {

        _LogMsg("Warning, error while parsing prototypes\n");
        return;
        }

    /*** Spielstand rücksetzen ***/
    for( n = 0; n < MAXNUM_LEVELS; n++ ) {

        if( (LNSTAT_INVALID != GSR->ywd->LevelNet->Levels[ n ].status) &&
            (LNSTAT_NETWORK != GSR->ywd->LevelNet->Levels[ n ].status) )
             GSR->ywd->LevelNet->Levels[ n ].status =
                  LNSTAT_DISABLED;
        }

    /*** den ersten enablen wir ***/
    GSR->ywd->LevelNet->Levels[ 1 ].status = LNSTAT_ENABLED;

    /*** Tutorial-Level freischalten ***/
    GSR->ywd->LevelNet->Levels[ TUTORIAL_LEVEL_1 ].status = LNSTAT_ENABLED;
    GSR->ywd->LevelNet->Levels[ TUTORIAL_LEVEL_2 ].status = LNSTAT_ENABLED;
    GSR->ywd->LevelNet->Levels[ TUTORIAL_LEVEL_3 ].status = LNSTAT_ENABLED;

    /*** Maximale Roboenergie runtersetzen ***/
    GSR->ywd->MaxRoboEnergy = 0;
    GSR->ywd->MaxReloadConst = 0;

    /*** GlobalStats löschen ***/
    memset( GSR->ywd->GlobalStats, 0, sizeof( GSR->ywd->GlobalStats ));

    /*** inputMode ausschalten ***/
    GSR->D_InputMode = DIM_NONE;

    /*** Maximale Fahrzeugzahl fuer Beamgate ruecksetzen ***/
    GSR->ywd->Level->MaxNumBuddies = MAXNUM_STARTBUDDIES;

    /*** "Contact"-Flags ruecksetzen ***/
    memset( GSR->ywd->Level->RaceTouched, 0, sizeof( GSR->ywd->Level->RaceTouched ));

    GSR->exist_savegame = 0;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bdisk, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->dmenu.Req) );
    GSR->shell_mode     = SHELLMODE_PLAY;
    sp.modus = SP_PUBLISH;
    _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
}


void yw_KillAllFilesInDir( char *fn )
{
    /* --------------------------------------------------------------
    ** Löscht alle Files dieses Directories, aber nicht das Directory
    ** selbst
    ** ------------------------------------------------------------*/
    APTR dir;
    struct ncDirEntry entry;

    if( dir = _FOpenDir( fn ) ) {

        /*** Nun alle auslesen ***/
        while( _FReadDir( dir, &entry ) ) {

            char file[ 200 ];

            #ifdef __WINDOWS__
            if( entry.attrs & NCDIR_DIRECTORY ) {

                /* --------------------------------------------------
                ** Scheint eine Spielstandsdatei zu sein. Bei Windows
                ** müssen wir die Verzeichnisse . und .. noch raus-
                ** filtern
                ** ------------------------------------------------*/
                if( ( strcmp( entry.name, "." )  == 0 ) ||
                    ( strcmp( entry.name, ".." ) == 0 ) )
                    continue;
                }
            #endif

            /*** löschen ***/
            sprintf( file, "%s/%s", fn, entry.name );
            _FDelete( file );
            }

        _FCloseDir( dir );
        }
}


void yw_CopyFile( char *von, char *nach )
{
    /*** kvhfdkjdu fdjh fdxvbfxkjbvdshgv dh fdxmnvxjnv,m ***/
    FILE *V, *N;
    char zeile[ 300 ];

    if( !(V = _FOpen( von,  "r" ))) return;
    if( !(N = _FOpen( nach, "w" ))) { _FClose( V ); return; }

    while( fgets( zeile, 299, V ) )
           fputs( zeile, N);

    _FClose( N );
    _FClose( V );
}

///


void yw_BackToTitle( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;

    yw_DBDoGlobalScore( GSR->ywd );
    yw_KillDebriefing( GSR->ywd );

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    GSR->shell_mode = SHELLMODE_TITLE;
    GSR->backtotitle = FALSE;
}



/// "yw_WasKeyJustUsed"
BOOL yw_WasKeyJustUsed( struct GameShellReq *GSR, WORD keycode, WORD modus )
{
/* ------------------------------------------------------------------------
** testet, ob die Taste schon einmal als Key (!) verwendet wurde. Arg abge-
** speckt,
** ----------------------------------------------------------------------*/

    int   i;

    if( !keycode ) return( FALSE );  // erleichtert uns draußen die Arbeit

    for( i = 1; i <= NUM_INPUTEVENTS; i++ ) {

        /*** eigene nicht testen ***/
        if( GSR->i_actualitem == i ) continue;

        /*** Die Tasten simple ohne Qualifier ***/
        if( modus & CHECK_KEY ) {

            /*** erstmal normal ***/
            if( GSR->inp[ i ].pos == keycode)
                return( TRUE );

            /*** dann erweiterter Sonderfall Slider ***/
            if( GSI_SLIDER == GSR->inp[ i ].kind )
                if( GSR->inp[ i ].neg == keycode)
                    return( TRUE );
            }
        }

    /*** Hamma nich gefunden ***/
    return( FALSE );
}
///


/// "yw_SetAllKeys"
void yw_SetAllKeys( Object * world, struct GameShellReq *GSR )
{
    /* -------------------------------------------------------
    ** Setzt alle tasten, weil sich was wesentliches, z.B. der
    ** Qualifiermodus, geändert hat
    ** NEU: Hotkeys vorher ausschalten
    ** -----------------------------------------------------*/

    int war, ist;
    Object *input_object = NULL;

    war = GSR->i_actualitem;
    _IE_GetAttrs( IET_Object, &input_object, TAG_DONE );

    /* ----------------------------------------------------------
    ** Zuerst ALLE Hotkeys loeschen, nicht nur die im Inputarray.
    ** Dann neue Tasten setzen. Wieviele Hotkeys gibt es genau?
    ** --------------------------------------------------------*/
    for( ist = 0; ist <= IDEV_NUMHOTKEYS; ist++ ) {

        struct idev_sethotkey_msg sk;
        struct inp_delegate_msg del;

        sk.id     = "nop";
        sk.hotkey = ist;

        del.type   = ITYPE_KEYBOARD;
        del.num    = 0;
        del.method = IDEVM_SETHOTKEY;
        del.msg    = &sk;
        _methoda( input_object, IM_DELEGATE, &del);
        }

    for( ist = 1; ist <= NUM_INPUTEVENTS; ist++ ) {

        GSR->i_actualitem = ist;

        _methoda( world, YWM_SETGAMEINPUT, GSR );
        }

    GSR->i_actualitem = war;
}
///


/// "yw_SetFarView"
void yw_SetFarView( Object *world, BOOL really )
{
    /*** Setzt die Sichtweite ***/
    if( really ) {

        _set( world, YWA_VisSectors,   VISSECTORS_FAR );
        _set( world, YWA_NormVisLimit, NORMVISLIMIT_FAR );
        _set( world, YWA_NormDFadeLen, NORMDFADELEN_FAR );
        }
    else {

        _set( world, YWA_VisSectors,   VISSECTORS_NEAR );
        _set( world, YWA_NormVisLimit, NORMVISLIMIT_NEAR );
        _set( world, YWA_NormDFadeLen, NORMDFADELEN_NEAR );
        }
}
///


/// "yw_WasInputEvent"
BOOL yw_WasInputEvent( struct VFMInput *input )
{
    int    mouse_move;
    ULONG  click;

    click = input->ClickInfo.flags & (~(1L));

    if( (input->ClickInfo.act.scrx == merke_scr_x) &&
        (input->ClickInfo.act.scry == merke_scr_y) )
        mouse_move = FALSE;
    else
        mouse_move = TRUE;

    merke_scr_x = input->ClickInfo.act.scrx;
    merke_scr_y = input->ClickInfo.act.scry;

    if( (input->ContKey ) ||
        (input->NormKey ) ||
        (input->HotKey  ) ||
        (mouse_move     ) ||
        (click) )
        return( TRUE );
    else
        return( FALSE );
}
///

#ifdef __NETWORK__

/// "CleanupNetworkData"
void yw_CleanupNetworkData( struct ypaworld_data *ywd )
{
    /* ------------------------------------------------------------------
    ** Aufräumroutine für Netzwerk. Schließt sowohl Session und meldet
    ** Spieler ab, erledigt aber auch Shellaufräumarbeiten.
    **
    ** Achtung, GSR ist im DESIGNMODE und winyppsn nicht ausgefüllt,
    ** Obwohl ein solcher Aufruf einer Netzsache nicht erfolgen dürfte...
    **
    ** NEU: Weil die Routine im Falle eines Remotestart nicht mehr
    ** aufgerufen wird und es einen zweiten Start dann nicht geben wird,
    ** schalten wir remotestart prophilaktisch mit aus. Das Schließen/
    ** Öffnen der Shell während remotestart muß verboten werden.
    ** ----------------------------------------------------------------*/
    int i;
    struct flushbuffer_msg fb;

    /* ------------------------------------------------------------
    ** Wie auch immer Buffer flushen, damit zu toten Spielern keine
    ** Messages mehr losgeschickt werden
    ** ----------------------------------------------------------*/
    fb.sender_kind   = MSG_PLAYER;
    fb.sender_id     = ywd->gsr->NPlayerName;
    fb.receiver_kind = MSG_ALL;
    fb.receiver_id   = NULL;
    fb.guaranteed    = TRUE;
    _methoda( ywd->nwo, NWM_FLUSHBUFFER, &fb);

    if( ywd->gsr ) {

        struct GameShellReq *GSR;
        
        GSR = ywd->gsr;

        /*** Wenn schon ein Spieler existiert, dann killen ***/
        if( (GSR->n_selmode == NM_MESSAGE) ||
            (GSR->ywd->netgamestartet) )
            yw_DestroyPlayer( GSR->ywd, GSR->NPlayerName );

        /*** Session schließen, wenn von mir geöffnet ***/
        if( ((NM_MESSAGE == GSR->n_selmode) && (GSR->is_host)) ||
            (GSR->ywd->netgamestartet) )
            _methoda( GSR->ywd->nwo, NWM_CLOSESESSION, NULL );
            
        _methoda( GSR->ywd->nwo, NWM_RESET, NULL );

        GSR->n_selmode            = NM_PROVIDER;
        GSR->ywd->playing_network = FALSE;
        GSR->nmenu.FirstShown     = 0;
        GSR->NSel                 = -1;
        GSR->N_InputMode          = FALSE;
        GSR->is_host              = FALSE;
        GSR->blocked              = FALSE;
        GSR->NPlayerOwner         = 0;
        GSR->NLevelOffset         = 0;
        GSR->modem_ask_session    = FALSE;

        for( i=0; i<MAXNUM_PLAYERS; i++ ) {

            /*** player2-Sachen ***/
            GSR->player2[i].msg[0]             = 0;
            GSR->player2[i].race               = 0;
            GSR->player2[i].ready_to_start     = 0;
            GSR->player2[i].name[0]            = 0;
            GSR->player2[i].welcomed           = 0;
            GSR->player2[i].waiting_for_update = 0;
            GSR->player2[i].checksum           = 0; 
            }

        for( i=0; i<8; i++ ) {

            /*** Player-Sachen ***/
            GSR->player[i].name[0]         = 0;
            }

        GSR->ReadyToStart        = FALSE;
        GSR->ywd->netgamestartet = FALSE;
        GSR->act_messageline     = 0;
        GSR->lastsender[0]       = 0;
        GSR->disconnected        = FALSE;
        GSR->trouble_count       = 0;
        GSR->network_trouble     = NETWORKTROUBLE_NONE;
        GSR->network_trouble_count = 0;
        GSR->tacs_time           = 0;
        GSR->dont_send           = FALSE;
        
        GSR->FreeRaces = (FREERACE_KYTERNESER | FREERACE_MYKONIER |
                          FREERACE_TAERKASTEN);
        GSR->SelRace   = FREERACE_USER;     // pauschal...

        /*** Ausschalten, weil es einen 2. Remotetart nicht gibt ***/
        GSR->remotestart = 0;
        }
yw_NetLog("netcleanup:      ende\n");
}
///


/// "yw_CloseNetRequester"
void yw_CloseNetRequester( struct GameShellReq *GSR )
{
    /* ---------------------------------------------------------
    ** Schließt den Netzrequester. Ist inner extra Routine, weil
    ** etwas Mehrarbeit anfällt, wegen Session schließen usw.
    ** -------------------------------------------------------*/
    struct setbuttonstate_msg sbs;
    struct switchpublish_msg sp;

    GSR->shell_mode   = SHELLMODE_TITLE;
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->bnet, BTM_SWITCHPUBLISH, &sp );
    sp.modus = SP_PUBLISH;
    _methoda( GSR->Titel, BTM_SWITCHPUBLISH, &sp );
    yw_CloseReq(GSR->ywd, &(GSR->nmenu.Req) );

    /*** Rücksetzen des gadgets! ***/
    sbs.who   = GSID_NET;
    sbs.state = SBS_UNPRESSED;
    _methoda( GSR->UBalken, BTM_SETSTATE, &sbs );

    sbs.who   = GSID_NETREADY;
    sbs.state = SBS_UNPRESSED;
    _methoda( GSR->bnet, BTM_SETSTATE, &sbs );

    yw_CleanupNetworkData( GSR->ywd );
}
///


/// "Rassenzeug für Netzwerk"
UBYTE yw_GetFirstFreeRace( struct GameShellReq *GSR )
{
    /*** leifert die erste rasse zurück, die ein ausgewählter Level bietet ***/
    struct LevelNode *l;
    UBYTE  race = 0;

    l = &( GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ] );

    if( l->races & 2 )
        race = FREERACE_USER;
    else
        if( l->races & 64 )
            race = FREERACE_KYTERNESER;
        else
            if( l->races & 8 )
                race = FREERACE_MYKONIER;
            else
                if( l->races & 16 )
                    race = FREERACE_TAERKASTEN;

    return( race );
}


void yw_DoRaceInit( struct GameShellReq *GSR )
{
    struct LevelNode *l;

    /*** nur die Einschalten, die es gibt ***/
    l = &(GSR->ywd->LevelNet->Levels[ GSR->NLevelOffset ]);
    GSR->FreeRaces = 0;

    if( l->races & 2 )
        GSR->FreeRaces |= FREERACE_USER;
    if( l->races & 64 )
        GSR->FreeRaces |= FREERACE_KYTERNESER;
    if( l->races & 8 )
        GSR->FreeRaces |= FREERACE_MYKONIER;
    if( l->races & 16 )
        GSR->FreeRaces |= FREERACE_TAERKASTEN;

    /*** Pauschal bekommt er die erste freie Rasse ***/
    GSR->SelRace = yw_GetFirstFreeRace( GSR );
    GSR->FreeRaces &= ~GSR->SelRace;
}
///
#endif


BOOL yw_IsOKForFilename( char ascii )
{
    /*** testet, ob das Zeichen in Filenamen verwendet werden kann ***/
    switch( ascii ) {
        case '.':
        case 92 :               // "/"
        case 47 :               // "\"
        case '*':
        case '?':
        case '"':
        case '|':
        case '<':
        case '>':
        case ':':
            return( FALSE );
        default:
            return( TRUE );
        }
}

void yw_LoadSaveGameFromMap( struct GameShellReq *GSR )
{

    struct switchpublish_msg sp;
    
    GSR->mb_stopped = FALSE;
    GSR->exist_savegame = 0;
    GSR->ywd->playing_network = FALSE;

    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->UBalken, BTM_SWITCHPUBLISH, &sp );

    /*** Spiel starten ***/
    GSR->GSA.LastAction = A_LOAD;
    GSR->GSA.ActionParameter[0] = 0;    // FIXME: hat nix zu bedeuten
}



void yw_CloseConfirmRequester( struct GameShellReq *GSR )
{
    struct switchpublish_msg sp;
    struct switchbutton_msg sb;

    sb.number  = GSID_CONFIRMOK;
    _methoda( GSR->confirm, BTM_DISABLEBUTTON, &sb );
    sb.number  = GSID_CONFIRMCANCEL;
    _methoda( GSR->confirm, BTM_DISABLEBUTTON, &sb );

    /*** nicht darstellen ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( GSR->confirm, BTM_SWITCHPUBLISH, &sp );
    
    GSR->confirm_modus = CONFIRM_NONE;
}



void yw_OpenConfirmRequester( struct GameShellReq *GSR, ULONG modus, char *text, ULONG type )
{
/*
**  FUNCTION    Oeffnet einen Requester um eine Aktion zu bestaetigen. Dieser
**              muss permanent bearbeitet werden, was aber Handle... macht.
**
**  INPUT       der Modus zeigt an, wohin nach Beenden des Requesters verzweigt
**              wird.
**              texts ist die eine (oder mehrere) Textzeile...
**        
**  OUTPUT
**
**  CHANGED   
*/
    struct switchpublish_msg sp;
    struct switchbutton_msg sb;
    struct setstring_msg ss;
    struct setbuttonpos_msg sbp;
    struct ypaworld_data *ywd;
    
    GSR->confirm_modus = modus;
    ywd = GSR->ywd;

    sb.number  = GSID_CONFIRMOK;
    _methoda( GSR->confirm, BTM_ENABLEBUTTON, &sb );
    
    if( 0 == type ) {
    
        /*** dann 2ButtonRequester ***/
        sb.number  = GSID_CONFIRMCANCEL;
        _methoda( GSR->confirm, BTM_ENABLEBUTTON, &sb );

        sbp.number = GSID_CONFIRMOK;
        sbp.x      = GET_X_COORD(160);
        sbp.y      = -1;
        sbp.w      = -1;
        sbp.h      = -1;
        _methoda( GSR->confirm, BTM_SETBUTTONPOS, &sbp);
        sbp.number = GSID_CONFIRMCANCEL;
        sbp.x      = GET_X_COORD(400); 
        _methoda( GSR->confirm, BTM_SETBUTTONPOS, &sbp);
        }
    else {
    
        sbp.number = GSID_CONFIRMOK;
        sbp.x      = GET_X_COORD(280);
        sbp.y      = -1;
        sbp.w      = -1;
        sbp.h      = -1;
        _methoda( GSR->confirm, BTM_SETBUTTONPOS, &sbp);
        }

    ss.unpressed_text = text;
    ss.pressed_text   = NULL;
    ss.number         = GSID_CONFIRMTEXT;
    _methoda( GSR->confirm, BTM_SETSTRING, &ss );  

    /*** nicht darstellen ***/
    sp.modus = SP_PUBLISH;
    _methoda( GSR->confirm, BTM_SWITCHPUBLISH, &sp );
}  


BOOL yw_CheckOlderSaveGame( struct GameShellReq *GSR )
{
    FILE *f;
    char filename[ 300 ];
    
    sprintf( filename, "save:%s/sgisold.txt", GSR->UserName );
    
    if( f = _FOpen( filename, "r" ) ) {
    
        _FClose( f );
        return( TRUE );
        }
    else
        return( FALSE );
}                         


void yw_StartNetGame( struct GameShellReq *GSR )
{

    struct ypamessage_loadgame yms;
    struct sendmessage_msg sm;
    struct locksession_msg ls;
    struct flushbuffer_msg fb;

    yms.generic.message_id = YPAM_LOADGAME;
    yms.generic.owner      = 0; // weil nicht feststehend
    yms.level              = GSR->NLevelOffset;

    sm.data                = (char *) &yms;
    sm.data_size           = sizeof( yms );
    sm.receiver_kind       = MSG_ALL;
    sm.receiver_id         = NULL;
    sm.guaranteed          = TRUE;
    _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);

    fb.sender_kind         = MSG_PLAYER;
    fb.sender_id           = GSR->NPlayerName;
    fb.receiver_kind       = MSG_ALL;
    fb.receiver_id         = NULL;
    fb.guaranteed          = TRUE;
    _methoda( GSR->ywd->nwo, NWM_FLUSHBUFFER, &fb);


    GSR->GSA.LastAction = A_NETSTART;
    GSR->GSA.ActionParameter[ 0 ] = GSR->NLevelOffset;
    GSR->GSA.ActionParameter[ 1 ] = GSR->NLevelOffset;

    GSR->nmenu.FirstShown = 0;

    /*** Session sperren ***/
    ls.block = TRUE;
    _methoda( GSR->ywd->nwo, NWM_LOCKSESSION, &ls );

    /*** Letzte Infos rausschreiben ***/
    yw_PrintNetworkInfoStart( GSR );
}  


void yw_CheckCDStatus( struct GameShellReq *GSR )
{
/* ------------------------------------------------------
** testet zyklisch, ob eine CD eingelegt ist. Somit kann
** waehrend des spieles noch eine Cd eingelegt werden.
** ausserdem ist man beim Starten des programmes manchmal
** schneller als das CD-Rom ueberprueft werden kann.
** ----------------------------------------------------*/
    
    LONG cd;
    struct ypamessage_cd cdm;
    struct sendmessage_msg sm;
    struct getplayerdata_msg gpd;
    
    /*** ist es wirklich schon so spaet? ***/  
    if( (GSR->global_time - GSR->last_cdcheck) < 1500 )
        return;
        
    GSR->last_cdcheck = GSR->global_time;
    
    /*** testen ... ***/
    cd = yw_CheckCD( FALSE, FALSE, NULL, NULL );    
    
    /*** ... Eintragen ... ***/
    gpd.number  = 0;
    gpd.askmode = GPD_ASKNUMBER;
    while( _methoda( GSR->ywd->nwo, NWM_GETPLAYERDATA, &gpd) ) {

        /*** Nicht ganz wasserdicht...***/
        if(stricmp( gpd.name, GSR->NPlayerName)==0) {
            GSR->player2[ gpd.number ].cd = (UBYTE) cd;
            break;
            }

        gpd.number++;
        }
    
    GSR->cd                = (UBYTE) cd;
    
    /*** ... und merken ***/
    cdm.cd                 = GSR->cd;
    cdm.generic.message_id = YPAM_CD;
    cdm.generic.owner      = 0; // weil nicht feststehend

    sm.data                = (char *) &cdm;
    sm.data_size           = sizeof( cdm );
    sm.receiver_kind       = MSG_ALL;
    sm.receiver_id         = NULL;
    sm.guaranteed          = TRUE;
    _methoda( GSR->ywd->world, YWM_SENDMESSAGE, &sm);
}


char *yw_NotEnoughCDs( struct GameShellReq *GSR )
{
    /* ----------------------------------------------
    ** Sieht nach, ob die CDs reichen. Gibt NULL oder
    ** gleich den ErrorString zurueck 
    ** --------------------------------------------*/
    ULONG num_cds, num_players, i;
        
    num_players = _methoda( GSR->ywd->nwo, NWM_GETNUMPLAYERS, NULL );
    
    num_cds = 0;
    for( i = 0; i < num_players; i++ )
        if( GSR->player2[ i ].cd )
            num_cds++;
            
    if( (num_players > 3) && (num_cds < 2) )
        return( ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_NEED2CD, 
                 "YOU NEED 2 CD TO START 4 PLAYER GAME")); 
        
    if( num_cds < 1 )
        return( ypa_GetStr( GlobalLocaleHandle, STR_CONFIRM_NEEDCD, 
                 "YOU NEED A CD TO START THIS GAME")); 

    return( NULL );
}        
        
