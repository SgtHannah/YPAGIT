/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  plattformunabhängiger Main-Teil für YPA. Zu linken mit den jeweiligen
**  xxxmain.o für die spezielle Plattform
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <exec/types.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "engine/engine.h"
#include "visualstuff/ov_engine.h"
#include "transform/te.h"
#include "audio/audioengine.h"
#include "bitmap/ilbmclass.h"
#include "bitmap/rasterclass.h"

#include "ypa/ypaworldclass.h"
#include "ypa/ypagameshell.h"
#include "input/ie.h"
#include "requester/requesterclass.h"

ULONG ypa_Init( void );
ULONG ypa_DoFrame( void );
void  ypa_Kill( void );

UBYTE *Version = __DATE__ "  "__TIME__;

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
_use_nucleus
_use_ov_engine
_use_tform_engine
_use_input_engine
_use_audio_engine
#else
struct GET_Nucleus *NUC_GET = NULL;
struct GET_Engine *OVE_GET  = NULL;
struct GET_Engine *TE_GET   = NULL;
struct GET_Engine *IE_GET   = NULL;
struct GET_Engine *AE_GET   = NULL;
#endif

struct GameShellReq GSR;
Object *World;
struct VFMInput Ip;
struct trigger_msg Trigger;

ULONG GameStatus, MovieNumber = 0;
BOOL  AccPlayer   = FALSE;
char *AccName     = NULL;
BOOL  RestoreData = FALSE;

#define GS_NONE     (0)     // weder Level, noch Shell geladen
#define GS_SHELL    (1)     // Shell offen
#define GS_LEVEL    (2)     // Spiel läuft gerade
#define GS_PLAYER   (3)     // Demo-Player läuft gerade

void ypa_ParseSnapDir( void );

#ifdef __TRIAL__
ULONG ypa_DoSplashScreen  = FALSE;
Object *ypa_SplashScreenBmp = NULL;
Object *ypa_GfxObject       = NULL;
#endif 

/*-----------------------------------------------------------------*/
void kill(void)
{
    if (TE_GET)     _CloseEngine(MID_ENGINE_TRANSFORM);
    if (IE_GET)     _CloseEngine(MID_ENGINE_INPUT);
    if (AE_GET)     _CloseEngine(MID_ENGINE_OUTPUT_AUDIO);
    if (OVE_GET)    _CloseEngine(MID_ENGINE_OUTPUT_VISUAL);
    if (NUC_GET)    nc_CloseNucleus();
}

/*-----------------------------------------------------------------*/
BOOL init(void)
{
    ULONG xres,yres,mode,mode_info;

    World = NULL;
    GameStatus = NULL;
    memset(&GSR,0,sizeof(GSR));
    memset(&Ip,0,sizeof(Ip));
    memset(&Trigger,0,sizeof(Trigger));

    /* Nucleus öffnen */
    NUC_GET = nc_OpenNucleus();
    if (!NUC_GET) { puts("Couldn't open Nucleus!"); kill(); return(FALSE); };

    #ifdef __WINDOWS__
        _ConfigOverride("gfx.display  = win3d.class");
        _ConfigOverride("gfx.display2 = windd.class");

        _ConfigOverride("gfx.engine     = gfx.engine");
        _ConfigOverride("tform.engine   = tform_ng.engine");
        _ConfigOverride("input.engine   = input.engine");
        _ConfigOverride("input.wimp     = winp");
        _ConfigOverride("input.keyboard = winp");

        _ConfigOverride("input.slider[10] = winp:mousex");
        _ConfigOverride("input.slider[11] = winp:mousey");
        _ConfigOverride("input.slider[12] = winp:joyx winp:joyrudder");
        _ConfigOverride("input.slider[13] = winp:joyy");
        _ConfigOverride("input.slider[14] = winp:joythrottle");
        _ConfigOverride("input.slider[15] = winp:joyhatx");
        _ConfigOverride("input.slider[16] = winp:joyhaty");

        _ConfigOverride("input.button[16] = winp:joyb0");
        _ConfigOverride("input.button[17] = winp:joyb1");
        _ConfigOverride("input.button[18] = winp:joyb2");
        _ConfigOverride("input.button[19] = winp:joyb3");
        _ConfigOverride("input.button[20] = winp:joyb4");
        _ConfigOverride("input.button[21] = winp:joyb5");
        _ConfigOverride("input.button[22] = winp:joyb6");
        _ConfigOverride("input.button[23] = winp:joyb7");
    #endif


    /* Engines öffnen (Windows: Display VOR Audio und Input!!!) */
    OVE_GET = _OpenEngine(MID_ENGINE_OUTPUT_VISUAL);
    AE_GET  = _OpenEngine(MID_ENGINE_OUTPUT_AUDIO);
    IE_GET  = _OpenEngine(MID_ENGINE_INPUT);
    TE_GET  = _OpenEngine(MID_ENGINE_TRANSFORM);

    if (!AE_GET)  { puts("Couldn't open audio engine!"); kill(); return(FALSE); };
    if (!OVE_GET) { puts("Couldn't open gfx engine!"); kill(); return(FALSE); };
    if (!TE_GET)  { puts("Couldn't open tform engine!"); kill(); return(FALSE); };
    if (!IE_GET)  { puts("Couldn't open input engine!"); kill(); return(FALSE); };

    OVE_GET->GetAttrs(OVET_XRes, &xres,
                      OVET_YRes, &yres,
                      OVET_Mode, &mode,
                      OVET_ModeInfo, &mode_info,
                      TAG_DONE);

    IE_GET->SetAttrs(IET_VideoX, xres,
                     IET_VideoY, yres,
                     IET_VideoMode, mode,
                     IET_ModeInfo,  mode_info,
                     TAG_DONE);

    /* fertig */
    return(TRUE);
}

/*====================================================================**
**                                                                    **
**  Routinen für User-Settings saven und restaurieren                 **
**                                                                    **
**====================================================================*/

BOOL GetSettings( void )
{
    /* -----------------------------------------------------------
    ** Defaultuser ermitteln. dazu suchen wir das File user.def in
    ** levels:. Existiert das nicht, gehen wir von "standard" aus.
    ** Das kann ich hier machen, weil die Fileliste schon aufge-
    ** baut wurde.
    **
    ** NEU: UserSettings sind nun ein Verzeichnisname. Dieses
    ** Verzeichnis enthält alle spielerspezifischen Files...
    ** ---------------------------------------------------------*/
    FILE   *F, *F2;
    char   fn[ 300 ], dn[ 300 ];
    struct fileinfonode *fnode;
    int    n;
    struct saveloadsettings_msg sls;


    if( F = _FOpen("env:user.def", "r" ) ) {

        /*** auslesen ***/
        _FGetS( dn, 300, F );

        /*** Existiert das File? ***/
        sprintf( fn, "save:%s/user.txt\0", dn );
        if( F2 = _FOpen( fn, "r" ) ) {

            /*** ok... ***/
            _FClose( F2 );
            strcpy( GSR.UserName, dn );
            sprintf( fn, "%s/user.txt\0", dn );
            }
        else {

            /*** Existiert nicht. Standard nehmen ***/
            _LogMsg("Warning: default user file doesn't exist (%s)\n",fn);
            strcpy( fn, "sdu7/user.txt\0" );
            strcpy( GSR.UserName, "SDU7\0" );
            }
        _FClose( F );
        }
    else {

        strcpy( fn, "sdu7/user.txt" );
        strcpy( GSR.UserName, "SDU7" );
        _LogMsg("Warning: No default user set\n");
        }

    /*** Usernummer ist obsolet. Trotzdem D_ActualItem setzen? ***/
    fnode            = (struct fileinfonode *) GSR.flist.mlh_Head;
    n                = 1;
    GSR.d_actualitem = -1;
    while( fnode->node.mln_Succ ) {

        if( stricmp( fnode->username, GSR.UserName ) == 0 ) {
            GSR.d_actualitem = n;
            break;
            }
        n++;
        fnode = (struct fileinfonode *) fnode->node.mln_Succ;
        }

    /*** InitDaten lesen ***/
    sls.gsr      = &GSR;
    sls.filename = fn;
    sls.username = GSR.UserName;
    sls.mask     = DM_INPUT|DM_VIDEO|DM_SHELL|DM_SOUND|DM_SCORE|
                   DM_BUILD|DM_BUDDY|DM_USER;
    sls.flags    = SLS_OPENSHELL;
    if( !_methoda( World, YWM_LOADSETTINGS, &sls ) ) return( FALSE );


    return( TRUE );
}


/*-----------------------------------------------------------------------*/
void SaveSettings( void )
{
    char   fn[ 300 ];
    FILE   *F;
    struct saveloadsettings_msg sls;

    /*** Abspeichern der Default-settings ***/
    sprintf( fn, "%s/user.txt", GSR.UserName );
    sls.filename = fn;
    sls.username = GSR.UserName;
    sls.gsr      = &GSR;
    sls.mask     = DM_SCORE|DM_USER|DM_INPUT|DM_VIDEO|DM_SHELL|DM_SOUND|
                   DM_BUILD|DM_BUDDY;
    sls.flags    = 0;
    _methoda( World, YWM_SAVESETTINGS, &sls );

    /*** Abspeichern des letzten Spielers ***/
    if( F = _FOpen( "env:user.def", "w") ) {

        strcpy( fn, GSR.UserName );
        _FWrite( fn, strlen( fn ), 1, F );
        _FClose( F );
        }
}

/*====================================================================**
**                                                                    **
**  Routinen für Demo Screenblanker                                   **
**                                                                    **
**====================================================================*/

void ypa_ParseSnapDir()
{
    struct ncDirEntry entry;
    APTR   dir;

    if( dir = _FOpenDir( "env/snaps/" ) ) {

        /*** Nun alle auslesen ***/
        while( _FReadDir( dir, &entry ) ) {

            if( (!(entry.attrs & NCDIR_DIRECTORY)) && (GSR.fn_number < 32) ) {

                /*** Ist es ein würdiger Eintrag? ***/
                if( strnicmp(entry.name, "demo", 4) == 0 ) {

                    /*** Name merken ***/
                    sprintf( GSR.fn[ GSR.fn_number ], "env/snaps/%s", entry.name );
                    GSR.fn_number++;
                    }
                }
            }

        /*** Verzeichnis schließen ***/
        _FCloseDir( dir );
        }
}

char *ypa_GetAccFileName( ULONG global_time )
{
    /*** wählt zufällig einen Namen aus ***/
    ULONG old_number;
    old_number = MovieNumber;

    if( GSR.fn_number ) {

        MovieNumber = global_time % GSR.fn_number;

        /*** Nichts doppelt ***/
        if( old_number == MovieNumber ) {

            if( MovieNumber > 0 )
                MovieNumber--;
            else
                if( MovieNumber < (GSR.fn_number-1) )
                    MovieNumber++;
            }

        return( GSR.fn[ MovieNumber ] );
        }
    else return( NULL );
}


BOOL ypa_SaveBuildInfo( struct GameShellReq *GSR )
{
    struct saveloadsettings_msg sls;

    /*** Speichert Buildinfo fuer Rest. nach Abbruch oder Netzwerk ***/
    sls.gsr      = GSR;
    sls.filename = "settings.tmp";
    sls.username = GSR->UserName;
    sls.mask     = DM_BUILD|DM_BUDDY|DM_USER;
    sls.flags    = 0;
    return( (BOOL) _methoda( World, YWM_SAVESETTINGS, &sls ) );
}


/*-----------------------------------------------------------------*/
BOOL ypa_HandlePlayer(struct trigger_msg *trigger)
{
    struct playercontrol_msg pcm;
    BOOL   running = TRUE;
    struct ypaworld_data *ywd = GSR.ywd;

    /* --------------------------------------------------------------
    ** Im Normalfall wird der Player getriggert und fängt nach dem
    ** Ende wieder von vorn an. Wenn wir im Zufallsmodus sind, dann
    ** müssen wir testen, ob eine Sequenz zu Ende ist, um rechtzeitig
    ** die neue anzukündigen.
    ** ------------------------------------------------------------*/
    if( (ywd->in_seq->frame.id >= (ywd->in_seq->num_frames-3)) &&
        (AccPlayer) ) {

        /*** Ein neues File bitte... ***/
        AccName = ypa_GetAccFileName( trigger->global_time );

        /*** Player schließen ***/
        _methoda( World, YWM_KILLPLAYER, NULL );

        /*** Ein neuer Anfang ***/
        if(_method(World, YWM_INITPLAYER, (ULONG)AccName)) {
            GameStatus = GS_PLAYER;
            AccPlayer = TRUE;
        } else {
            GameStatus = GS_SHELL;
            _GetInput( &Ip );
            return( FALSE );
        }
    }

    _methoda(World, YWM_TRIGGERPLAYER, trigger);

    pcm.mode = PLAYER_NOP;
    pcm.arg0 = 0;
    switch(trigger->input->NormKey) {
        case KEYCODE_P:
            pcm.mode = PLAYER_PLAY;
            break;
        case KEYCODE_S:
            pcm.mode = PLAYER_STOP;
            break;
        case KEYCODE_R:
            pcm.mode = PLAYER_GOTO;
            pcm.arg0 = 0;
            break;
        case KEYCODE_N:
            pcm.mode = PLAYER_NEXT;
            break;
        case KEYCODE_B:
            pcm.mode = PLAYER_PREV;
            break;
        case KEYCODE_M:
            pcm.mode = PLAYER_RELPOS;
            pcm.arg0 = 10;
            break;
        case KEYCODE_V:
            pcm.mode = PLAYER_RELPOS;
            pcm.arg0 = -10;
            break;
        case KEYCODE_ESCAPE:
        case KEYCODE_SPACEBAR:
            running = FALSE;
            break;
    };
    if (pcm.mode != PLAYER_NOP) {
        _methoda(World, YWM_PLAYERCONTROL, &pcm);
    };

    return(running);
}

#ifdef __TRIAL__
/*=================================================================**
**                                                                 **
**  Exit-Splash-Screen                                             **
**                                                                 **
**=================================================================*/

ULONG ypa_BeginSplashScreen(void)
{
    _OVE_GetAttrs(OVET_Object, &ypa_GfxObject, TAG_DONE);

    /*** Screen loeschen ***/
    _methoda(ypa_GfxObject, DISPM_Begin, NULL);
    _methoda(ypa_GfxObject, DISPM_End, NULL);

    /*** Bmp laden ***/
    _SetAssign("rsrc","data:mc2res");
    ypa_SplashScreenBmp = _new("ilbm.class", RSA_Name, "exit.ilb",
                               BMA_Texture,      TRUE,
                               BMA_TxtBlittable, TRUE,
                               TAG_DONE);
    if (ypa_SplashScreenBmp) {
        ypa_DoSplashScreen = TRUE; 
        return(TRUE);
    } else {
        return(FALSE);
    };
}
/*-----------------------------------------------------------------*/
void ypa_EndSplashScreen(void)
{
    if (ypa_SplashScreenBmp) {
        _dispose(ypa_SplashScreenBmp);
        ypa_SplashScreenBmp = NULL;
    };
    ypa_GfxObject = NULL;
}
/*-----------------------------------------------------------------*/
void ypa_BlitSplashScreen(void)
{
    Object *pic  = ypa_SplashScreenBmp;
    Object *gfxo = ypa_GfxObject;

    if (pic && gfxo) {
        struct rast_blit blt;
        struct disp_pointer_msg dpm;
        dpm.pointer = NULL;
        dpm.type    = DISP_PTRTYPE_NONE;
        _methoda(gfxo, DISPM_SetPointer, &dpm);
        _get(pic, BMA_Bitmap, &(blt.src));
        blt.from.xmin = blt.to.xmin = -1.0;
        blt.from.ymin = blt.to.ymin = -1.0;
        blt.from.xmax = blt.to.xmax = +1.0;
        blt.from.ymax = blt.to.ymax = +1.0;
        if (gfxo) {
            _methoda(gfxo, DISPM_Begin, NULL);
            _methoda(gfxo, RASTM_Begin2D, NULL);
            _methoda(gfxo, RASTM_Blit, &blt);
            _methoda(gfxo, RASTM_End2D, NULL);
            _methoda(gfxo, DISPM_End, NULL);
        };
    };
}
#endif

/*=================================================================**
**                                                                 **
**  Welt-Objekt erzeugen                                           **
**                                                                 **
**=================================================================*/

BOOL CreateWorld( void )
{

    /*** Welt-Object erzeugen ***/
    World = _new("ypaworld.class",
                 YWA_Version, Version,
                 TAG_DONE);

    if (World) {
        /*** Interface vorbereiten! ***/
        if(!_methoda(World, YWM_INITGAMESHELL, &GSR)) {
            _LogMsg("Unable to init shell structure\n");
            return( FALSE );
        };
        if(!GetSettings()) {
            _LogMsg("Unable to init game with default settings\n");
            return( FALSE );
        };
        if(!GSR.ShellOpen) {
            if(!_methoda(World, YWM_OPENGAMESHELL, &GSR)) {
                _LogMsg("Error: Unable to open Gameshell\n");
                return(FALSE);
            };
        };
        GameStatus = GS_SHELL;
        ypa_ParseSnapDir();

    } else return(FALSE);

    return(TRUE);
}

/*=================================================================**
**                                                                 **
**  Pro-Frame-Routinen                                             **
**                                                                 **
**=================================================================*/

ULONG ypa_DoIngameFrame(void)
{
    ULONG running = TRUE;

    struct LevelInfo *l;
    char   filename[ 200 ];

    /*** Es läuft das Spiel ***/
    _methoda(World, BSM_TRIGGER, &Trigger);

    _get(World, YWA_LevelInfo, &l);
    switch(l->Status) {

        struct saveloadgame_msg slg;

        case LEVELSTAT_FINISHED:
        case LEVELSTAT_ABORTED:
            /*** Spiel abbrechen und Shell starten ***/
            _methoda(World,YWM_KILLLEVEL,NULL);

            /*** Wenn mit Cancel verlassen oder Netzwerk ***/
            if( RestoreData ||
                (LEVELSTAT_ABORTED == l->Status)) {

                struct saveloadsettings_msg sls;
                sls.gsr      = &GSR;
                sls.filename = "settings.tmp";
                sls.username = GSR.UserName;
                sls.mask     = DM_BUILD|DM_BUDDY|DM_USER;
                sls.flags    = 0;                   // keijne Shell öffnen
                if( !_methoda( World, YWM_LOADSETTINGS, &sls ) ) return( FALSE );
                RestoreData = FALSE;
                }

            /*** Bei Fernsteuerung Abbruch ***/
            if(GSR.remotestart) running = FALSE;

            /* -------------------------------------------------
            ** Im Netzwerkfalle folgendes: PLAY einschalten, um
            ** Debriefing zu erhalten und dann zurueck auf TITLE
            ** schalten
            ** -----------------------------------------------*/
            if( SHELLMODE_NETWORK == GSR.shell_mode ) {

                GSR.shell_mode  = SHELLMODE_PLAY;
                GSR.backtotitle = TRUE;
                }
            else
                GSR.backtotitle = FALSE;

            GameStatus = GS_NONE;
            Trigger.global_time  = 0;
            GSR.last_input_event = 0;
            GSR.aftergame        = TRUE;
            if(!_methoda( World, YWM_OPENGAMESHELL, &GSR)) {
                _LogMsg("GameShell-Error!!!\n");
                _methoda( World, YWM_FREEGAMESHELL, &GSR );
                return(FALSE);
            };
            GameStatus = GS_SHELL;

            /*** FrameTime rücksetzen ***/
            _GetInput( &Ip );
            break;

        case LEVELSTAT_RESTART:

            /*** Spiel abbrechen, und Restart-File laden ***/
            _methoda(World,YWM_KILLLEVEL,NULL);
            slg.gsr  = &GSR;
            slg.name = filename;
            sprintf( filename, "save:%s/%d.rst\0", GSR.UserName, l->Num );
            if( !_methoda( World, YWM_LOADGAME, &slg ) ) {
                _LogMsg("Warning, load error\n");
            };
            /*** FrameTime rücksetzen ***/
            _GetInput( &Ip );
            break;

        case LEVELSTAT_SAVE:

            /*** Spielstand kurz mal abspeichern für Slot 0  ***/
            slg.gsr  = &GSR;
            slg.name = filename;
            sprintf( filename, "save:%s/%d.sgm\0", GSR.UserName, 0);
            if( !_methoda( World, YWM_SAVEGAME, &slg ) ) {
                _LogMsg("Warning, Save error\n");
            };
            /*** FrameTime rücksetzen ***/
            _GetInput( &Ip );
            break;

        case LEVELSTAT_LOAD:

            /*** Spielstand neu laden für Slot 0 ***/
            _methoda( World, YWM_KILLLEVEL, NULL );

            slg.gsr  = &GSR;
            slg.name = filename;
            sprintf( filename, "save:%s/%d.sgm\0", GSR.UserName, 0);
            if( !_methoda( World, YWM_LOADGAME, &slg ) ) {
                _LogMsg("Warning, load error\n");
            };
            /*** FrameTime rücksetzen ***/
            _GetInput( &Ip );
            break;
    }; // switch()

    return(running);
}

/*-----------------------------------------------------------------*/
ULONG ypa_DoShellFrame(void)
{
    ULONG running = TRUE;
    char  filename[ 200 ];

    #ifdef __TRIAL__
    if (ypa_DoSplashScreen) {
        ypa_BlitSplashScreen();
        if (Ip.NormKey != 0) {
            ypa_EndSplashScreen();
            return(FALSE);
        } else return(TRUE);
    };
    #endif

    GSR.frame_time   = Trigger.frame_time;
    GSR.global_time  = Trigger.global_time;
    GSR.input = &Ip;

    _methoda( World, YWM_TRIGGERGAMESHELL, &GSR );

    /*** Was war der Rückkehrwert für uns? ***/
    if( GSR.GSA.LastAction == A_QUIT ) {
        #ifdef __TRIAL__
            if (!ypa_BeginSplashScreen()) running = FALSE;
        #else 
            running = FALSE;
        #endif
        }

    /*** einen neuen Level starten ***/
    if(GSR.GSA.LastAction == A_PLAY) {
        struct createlevel_msg clm;
        SaveSettings();
        _methoda( World, YWM_CLOSEGAMESHELL, &GSR );
        GameStatus = GS_NONE;

        /*** Fuer Abbruch mit Cancel ***/
        if( !ypa_SaveBuildInfo( &GSR )) return( FALSE );

        clm.level_num  = GSR.GSA.ActionParameter[0];
        clm.level_mode = LEVELMODE_NORMAL;
        if( !_methoda(World, YWM_ADVANCEDCREATELEVEL, &clm)) {
            /*** Na sowas, der Level geht nicht! ***/
            _LogMsg("Sorry, unable to init this level!\n");
            _methoda( World, YWM_FREEGAMESHELL, &GSR );
            running = FALSE;
        };
        GameStatus = GS_LEVEL;
        _GetInput( &Ip );
        }

    /*** letzten Spielstand des Users laden ***/
    if (GSR.GSA.LastAction == A_LOAD) {
        struct LevelInfo *l;
        struct saveloadgame_msg slg;
        GameStatus = GS_NONE;
        _get(World, YWA_LevelInfo, &l);
        SaveSettings();
        _methoda( World, YWM_CLOSEGAMESHELL, &GSR );
        slg.gsr  = &GSR;
        slg.name = filename;

        /*** Fuer Abbruch mit Cancel ***/
        if( !ypa_SaveBuildInfo( &GSR )) return( FALSE );

        sprintf( filename, "save:%s/%d.sgm", GSR.UserName, 0 );
        if( !_methoda( World, YWM_LOADGAME, &slg ) ) {
            _LogMsg("Error while loading level (level %d, User %s\n",
                     l->Num, GSR.UserName );
            _methoda( World, YWM_FREEGAMESHELL, &GSR );
            running = FALSE;
        };
        GameStatus = GS_LEVEL;
        _GetInput(&Ip);
        }

    /*** einen Netzwerk-Level starten ***/
    #ifdef __NETWORK__
    if(GSR.GSA.LastAction == A_NETSTART) {
        struct createlevel_msg cl;

        SaveSettings();

        /*** Buildinfos immer restaurieren ***/
        if( !ypa_SaveBuildInfo( &GSR )) return( FALSE );
        RestoreData = TRUE;

        _methoda( World, YWM_CLOSEGAMESHELL, &GSR );
        GameStatus = GS_NONE;
        cl.level_mode = LEVELMODE_NORMAL;
        cl.level_num  = GSR.GSA.ActionParameter[0];
        if( !_methoda(World, YWM_NETWORKLEVEL, &cl ) )
        {
            /*** Na sowas, der Level geht nicht! ***/
            _LogMsg("Sorry, unable to init this level for network!\n");
            _methoda( World, YWM_FREEGAMESHELL, &GSR );
            running = FALSE;
        };
        GameStatus = GS_LEVEL;

        /*** Frame ausbremsen ***/
        _GetInput( &Ip );
        }
    #endif

    /*** den Movie-Player starten ***/
    if(GSR.GSA.LastAction == A_DEMO) {

        char   *f;
        if( !(f = ypa_GetAccFileName( Trigger.global_time ) ) ) {
            GSR.last_input_event = Trigger.global_time;
            return( TRUE );
            }

        /*** Immer restaurieren ***/
        if( !ypa_SaveBuildInfo( &GSR )) return( FALSE );
        RestoreData = TRUE;

        _methoda( World, YWM_CLOSEGAMESHELL, &GSR );
        GameStatus = GS_NONE;

        if (_method(World, YWM_INITPLAYER, (ULONG)f)) {
            GameStatus = GS_PLAYER;
            AccPlayer  = TRUE;
        } else {
            _LogMsg("Sorry, unable to init player!\n");
            Trigger.global_time  = 0;
            GSR.last_input_event = 0;
            if(!_methoda( World, YWM_OPENGAMESHELL, &GSR)) {
                _LogMsg("GameShell-Error!!!\n");
                _methoda( World, YWM_FREEGAMESHELL, &GSR );
                return( FALSE );
                }
            GameStatus = GS_SHELL;
        };
        _GetInput( &Ip );
    };

    return(running);
}

/*-----------------------------------------------------------------*/
ULONG ypa_DoPlayerFrame(void)
{
    ULONG running = TRUE;
    ULONG player_running;

    player_running = ypa_HandlePlayer(&Trigger);

    /*** Wurde Escape gedrückt? Dann raus hier! ***/
    if(!player_running) {

        /*** Dann müssen wir die Shell öffnen ***/
        _methoda( World, YWM_KILLPLAYER, NULL );

        /*** Was heißt hier restore...eigentlich immer !!! ***/
        if( RestoreData ) {

            struct saveloadsettings_msg sls;
            sls.gsr      = &GSR;
            sls.filename = "settings.tmp";
            sls.username = GSR.UserName;
            sls.mask     = DM_BUILD|DM_BUDDY|DM_USER;
            sls.flags    = 0;                   // keijne Shell öffnen
            if( !_methoda( World, YWM_LOADSETTINGS, &sls ) ) return( FALSE );
            RestoreData = FALSE;
            }

        GameStatus = GS_NONE;
        Trigger.global_time  = 0;
        GSR.last_input_event = 0;
        if(!_methoda( World, YWM_OPENGAMESHELL, &GSR)) {
            _LogMsg("GameShell-Error!!!\n");
            _methoda( World, YWM_FREEGAMESHELL, &GSR );
            return( FALSE );
        };
        GameStatus = GS_SHELL;
        GSR.ywd->Level->Status = LEVELSTAT_SHELL;

        /*** Wegen FrameTime UND langen Ladezeiten GetInput ***/
        memset(&Ip,0,sizeof(Ip));
        _GetInput( &Ip );
        }
    return(running);
}

/*-----------------------------------------------------------------*/
ULONG ypa_DoFrame()
{
    ULONG running = TRUE;

    memset(&Ip,0,sizeof(Ip));
    _GetInput(&Ip);
    
    /*** FrameTime + 1 erzwingen?. Flag wird immer wieder neu gesetzt ***/
    if( GSR.force_frametime_1 ) {
        Ip.FrameTime          = 1;
        GSR.force_frametime_1 = FALSE;
        }

    Ip.FrameTime += 1;
    Trigger.frame_time   = Ip.FrameTime;
    Trigger.global_time += Ip.FrameTime;
    Trigger.input        = &Ip;

    /*** OK, nun die non-Player-Sachen, wie eh und je... ***/
    switch(GameStatus) {
        case GS_LEVEL:  running = ypa_DoIngameFrame(); break;
        case GS_SHELL:  running = ypa_DoShellFrame();  break;
        case GS_PLAYER: running = ypa_DoPlayerFrame(); break;
    };

    /*** mit "Running"-Status zurück ***/
    return(running);
}

/*-----------------------------------------------------------------*/
ULONG ypa_Init()
{
    if (init()) {
        if (CreateWorld()) return(TRUE);
        else kill();
    };
    return(FALSE);
}
/*-----------------------------------------------------------------*/
void ypa_Kill()
{
    if (World) {
        if (GameStatus == GS_LEVEL) {
            _methoda(World,YWM_KILLLEVEL,NULL);
            _methoda(World,YWM_FREEGAMESHELL,&GSR);
        } else if (GameStatus == GS_SHELL) {
            SaveSettings();
            _methoda(World,YWM_CLOSEGAMESHELL,&GSR);
            _methoda(World,YWM_FREEGAMESHELL,&GSR);
        };
        _dispose(World);
    };
    kill();
}

