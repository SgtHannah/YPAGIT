#ifndef YPA_GAMESHELL_H
#define YPA_GAMESHELL_H
/*
**  $Source: PRG:VFM/Include/ypa/ypagameshell.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:25:34 $
**  $Locker:  $
**  $Author: floh $
**
**  Vereinbarungen für die GameShell
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef YPA_GUILIST_H
#include "ypa/guilist.h"
#endif

#ifndef YPA_YPAGUI_H
#include "ypa/ypagui.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

#ifndef YPA_YPATOOLTIPS_H
#include "ypa/ypatooltips.h"
#endif

#ifndef REQUESTER_BUTTONCLASS_H
#include "requester/buttonclass.h"
#endif

#ifndef REQUESTER_REQUESTERCLASS_H
#include "requester/requesterclass.h"
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

#ifndef AUDIO_SAMPLACLASS_H
#include "audio/sampleclass.h"
#endif

#ifdef AMIGA
  #ifndef _SYS_DIR_H
  #include <sys/dir.h>
  #endif
#endif

#ifdef MSDOS
  #ifndef _DIRECT_H_INCLUDED
  #include <direct.h>
  #endif
#endif

#ifdef __WINDOWS__
    #ifndef _DIRECT_H_INCLUDED
    #include <direct.h>
    #endif
#endif

/*** Zum testen ***/
#ifdef AMIGA
#define __NETWORK__
#endif

#ifdef __NETWORK__
    #ifndef YPA_MESSAGES_H
    #include <ypa/ypamessages.h>
    #endif

    #ifndef NETWORK_NETWORKCLASS_H
    #include <network/networkclass.h>
    #endif
#endif


/* --------------------------------------------------------------------
** In der GameShell-Struktur verwalten wir alle Sachen, die abgefragt
** werden können. Parallel sind in dieser Struktur alle Werte und
** zugehörige Requester vereint, die zur Manipulation dieser Werte
** beitragen. Außerdem wird hieran die trigger-Message mit angekoppelt.
** Somit kann diese Struktur an alle Routinen zur Initialisierung,
** Auswertung, Manipulation und Ausgabe weitergegeben werden.
** ------------------------------------------------------------------*/


struct input_info {

    WORD    kind;       // in INIT ausfüllen (slider, button...)
    WORD    number;     // Nummer zu oben ^^^
    WORD    pos;        // tastencode
    WORD    neg;        // tastencode
    WORD    bu_pos;     // Hier merke ich mir die alten Einstellungen
    WORD    bu_neg;
    WORD    st_pos;     // Hier merke ich mir die Standardeinstellungen
    WORD    st_neg;
    WORD    done;       // was ist zu aendern? Flags: IF_FIRST & IF_SECOND
    WORD    p;
    UBYTE   *menuname;  // Name im Menü
};

#define IF_FIRST        (1)
#define IF_SECOND       (2)

#define GSI_BUTTON      1           //
#define GSI_SLIDER      2           //
#define GSI_HOTKEY      3           //

#define GS_UPTEXT           "UP"
#define GS_DOWNTEXT         "DOWN"
#define GS_LEFTTEXT         "LEFT"
#define GS_RIGHTTEXT        "RIGHT"
#define GS_MORETEXT         "MORE"
#define GS_LESSTEXT         "LESS"

#define GS_DRVDIRTEXT       "DRIVE DIR"
#define GS_DRVSPEEDTEXT     "DRIVE SPEED"
#define GS_FLYDIRTEXT       "FLY DIR"
#define GS_FLYSPEEDTEXT     "FLY SPEED"
#define GS_FLYHEIGHTTEXT    "FLY HEIGHT"
#define GS_STOPTEXT         "STOP"
#define GS_FIRETEXT         "FIRE"
#define GS_FIREVIEWTEXT     "FIRE VIEW"
#define GS_FIREGUNTEXT      "FIRE GUN"
#define GS_GUNHEIGHTTEXT    "GUN HEIGHT"
#define GS_VISIERTEXT       "VISIER"
#define GS_ORDERTEXT        "ORDER"
#define GS_FIGHTTEXT        "FIGHT"
#define GS_NEWTEXT          "NEW"
#define GS_ADDTEXT          "ADD"
#define GS_CONTROLTEXT      "CONTROL"
#define GS_AUTOPILOTTEXT    "AUTOPILOT"
#define GS_MAPTEXT          "MAP"
#define GS_FINDERTEXT       "FINDER"
#define GS_LANDSCAPETEXT    "LANDSCAPE"
#define GS_OWNERTEXT        "OWNER"
#define GS_HEIGHTTEXT       "HEIGHT"
#define GS_NOLOCKTEXT       "NO MAPLOCK"
#define GS_LOCKVIEWERTEXT   "LOCK VIEWER"
#define GS_LOCKSQUADTEXT    "LOCK SQUAD"
#define GS_ZOOMINTEXT       "ZOOM IN"
#define GS_ZOOMOUTTEXT      "ZOOM OUT"
#define GS_MAPMINITEXT      "MAP MINI"
#define GS_MAPMAXTEXT       "MAP MAX"
#define GS_NEXTCOMTEXT      "NEXT COM"
#define GS_TOROBOTEXT       "TO ROBO"
#define GS_NEXTMANTEXT      "NEXT MAN"
#define GS_TOCOMMANDERTEXT  "TO COMMANDER"
#define GS_QUITTEXT         "QUIT"
#define GS_LOGWINTEXT       "LOGWIN"
#define GS_NEXTITEMTEXT     "NEXT ITEM"
#define GS_PREVITEMTEXT     "PREV ITEM"
#define GS_ENERGYTEXT       "ENERGY WINDOW"
#define GS_HUDTEXT          "HEADUP DISPLAY"
#define GS_LASTMSGTEXT      "JUMP TO LASTMSG-SENDER"
#define GS_PAUSETEXT        "PAUSE"
#define GS_TOUSERTEXT       "MESSAGE TO RESISTANCE"
#define GS_TOKYTERNESERTEXT "MESSAGE TO GHORKOV"
#define GS_TOMYKONIERTEXT   "MESSAGE TO MYKONIER"
#define GS_TOTAERKASTENTEXT "MESSAGE TO TAERKASTEN"
#define GS_TOALLTEXT        "MESSAGE TO ALL PLAYERS"
#define GS_AGGR1TEXT        "AGGR: COME BACK"
#define GS_AGGR2TEXT        "AGGR: FIGHT TARGET"
#define GS_AGGR3TEXT        "AGGR: FIGHT ENEMIES TOO"
#define GS_AGGR4TEXT        "AGGR: CONQUER ALL ENEMY AREA TOO"
#define GS_AGGR5TEXT        "AGGR: GO AMOK"
#define GS_WAYPOINT         "SELECT WAYPOINT"
#define GS_HELPTEXT         "HELP"
#define GS_LASTOCCUPIEDTEXT  "GOTO LAST OCCUPIED VEHICLE"
#define GS_MAKECOMMANDERTEXT "MAKE CURRENT VEHICLE COMMANDER"
#define GS_ANALYZERTEXT     "SITUATION ANALYZER"

#define USERNAMELEN         32      // so lang darf ein Username sein
#define NUM_INPUTEVENTS     45      // soviele Einträge hat das Input-Menü
#define MESSAGE_TIME        1500    // solange bleibt eine Meldung da
#define MAX_LANGUAGELEN     32      // soviele Zeichen darf die Sprache enthalten
#define SESSIONNAMELEN      1024    // Länge bezogen auf größe des Gadgets!
#define MAXNUM_MESSAGELINES 32

struct fileinfonode {

    struct MinNode node;    // zum Einklinken in die Dir-Liste
    LONG   global_time;
    UBYTE  owner;

    char    username[ USERNAMELEN + 2 ];    // so heißt das File
};

struct rsaction_msg {

    ULONG   LastAction;             // was wird gewünscht?
    ULONG   ActionParameter[ 5 ];   // wie wird es gewünscht?
};

struct reqpos {

    WORD    x;                      // damit merkt sich die shell bei
    WORD    y;                      // close die Position
};


struct localeinfonode {

    struct MinNode node;
    UBYTE  language[ MAX_LANGUAGELEN + 2 ];
};

struct playerdata {                 // Owner ist offset

    char  name[ STANDARD_NAMELEN ]; // Sein Name
    char  msg[ STANDARD_NAMELEN ];  // Seine letzte Message
    UBYTE ready_to_play;            // nicht 0, wenn zugehöriger Player mit
                                    // laden fertig ist
    UBYTE race;                     // was spielt er
    UBYTE was_killed;               // wurde rausgeschmissen. Kann aber sein, daß
                                    // derjenige das noch nicht weiß
    UBYTE status;                   // derzeitiger status des Spielers (NWS_...)

    /*** Für Debugging und Tests der Übertragung ***/
    UBYTE no_answer;                // 1, wenn zu lange keine Message kam...
    LONG  lastmsgtime;              // dann kam die letzte Message
    ULONG timestamp;                // wann von diesem die msg losgeschickt w.
    ULONG msgcount;                 // muß fortlaufend sein, für tests

    LONG  trouble_count;
    LONG  latency;                  // derzeitige  latency in ms
};

#define WASKILLED_NORMAL    (1<<0)  // wurde getoetet, normal eben
#define WASKILLED_SHOWIT    (1<<1)  // zeige das bitte auch mit an

#define NWS_NOTHERE         0       // dieser Owner spielt nicht mit
#define NWS_INGAME          1       // normal ohne Probleme im Spiel
#define NWS_TROUBLE         2       // zu diesem derzeit keine Verbindung
#define NWS_LEFTGAME        3       // hat Spiel normal verlassen
#define NWS_REMOVED         4       // wurde rausgeschmissen


struct playerdata2 {                // interne Nummer ist offset

    char   msg[ STANDARD_NAMELEN ]; // offset entsprechend der engine
                                    // weil Eigentümer noch nicht init.
    UBYTE  race;                    // die rassen meiner Mitspieler
    UBYTE  trouble;                 // seine Rasse habe ich auch
    UBYTE  owner;                   // wird in PlaceRobos entsprechend den
                                    // gewählten Rassen gesetzt
    UBYTE  ready_to_start;          // von mir aus kanns losgehen
    UBYTE  welcomed;
    UBYTE  cd; 
    UBYTE  p[2];
    LONG   waiting_for_update;      // ist eine Zeit, weil der Request ja verloren
                                    // gehen kann und dann fragen wir erneut
    LONG   checksum;                // derzeitige checksumme der files                                 
    char   name[ STANDARD_NAMELEN ];// jetzt doch, um nach dem Killen den Spieler
                                    // noch zu finden
};

struct netlevel {

    WORD    number;
    char   *name;
};


#define SEPHOSTFROMSESSION      "|"
#define TIME_CHECK_LATENCY      (2000)
#define MAX_LATENCY             (7000)  // Latency fuer Hin- und Rueckweg 
#define LATENCY_COUNT_HOST      (200)   // Achtung, Frametime ist 1, so ist es Zahl der Fr.
#define LATENCY_COUNT_CLIENT    (500)
#define ENDTROUBLE_COUNT        (3000)  // solange Grund fuer Loesung anzeigen
#define KICKOFF_YOU_COUNT       (10000)
#define KICKOFF_PLAYER_COUNT    (15000)
#define WAITINGFORPLAYER_TIME   (20000) // ab dann Message, dass kein Kontakt, wenn nicht Host

struct GameShellReq {

    BOOL   ShellOpen;           // tut gut, das zu wissen...
    BOOL   JustOpened;          // wegen Aufhellen, da muß ja erstmal was da sein
    BOOL   FirstScoreLoad;      // Hack, damit wir beim ersten Mal nicht speichern
    BOOL   aftergame;           // wird shell nach einem Spiel geoffnet?
    BOOL   force_frametime_1;   // damit (weil Spiel und ypa gsr kennen) wird ypa
                                // angehalten, die Frametime 1 zu erzwingen
    char   UserName[ USERNAMELEN + 2 ];

    struct ypaworld_data *ywd;

    /*** TriggerMessage ***/
    struct VFMInput *input;
    LONG   frame_time;
    LONG   global_time;

    /*** Informationen zu allen Requestern ***/
    ULONG  shell_mode;
    BOOL   shell_mode_changed;  // wird waehrend Trigger gesetzt
    BOOL   backtotitle;         // nach Debriefing in Titelleiste?

    /*** Sound 2x, da mehrere Sounds ***/
    struct SoundCarrier ShellSound1;     // die Carrierstruktur
    Object *so1[ MAX_SOUNDSOURCES ];     // die Objekte
    struct SoundCarrier ShellSound2;     // die Carrierstruktur
    Object *so2[ MAX_SOUNDSOURCES ];     // die Objekte

    #ifdef __NETWORK__
    struct SoundCarrier ChatSound;       // für die netz-Chat-Samples
    Object *ChatObject;
    #endif

    /* -------------------------------------------------------------
    ** Die Objekte. Dabei werden die Button- und Requesterobjekte
    ** extra verwalten, damit wir die Button noch schön modifizieren
    ** können. Aufpassen, daß nur die Button freigegeben werden, die
    ** kein Requesterobjekt haben!
    ** -----------------------------------------------------------*/
    Object *UBalken;            // ButtonObjekt
    BOOL    mb_stopped;         // FALSE: MB läuft gerade

    /*** Titelbalken extra ***/
    Object *Titel;

    /* -------------------------------------------------------------
    **             Daten zum InputRequester
    ** -----------------------------------------------------------*/

    Object *binput;             // das Buttonobjekt
    struct  YPAListReq imenu;   // das benötigte Menü
    LONG    i_actualitem;       // was isn grade ausgewählt?
    LONG    i_selfirst;         // fürs Umschalten bei Slidern
    BOOL    joystick;           // soll joystick unterstützt werden?
    BOOL    new_joystick;
    BOOL    altjoystick;        // alternatives Modell
    BOOL    new_altjoystick;
    BOOL    new_forcefeedback;
    BOOL    input_changemode;
    WORD    imenu_xoffset;      // Versatz des menüs bzgl. 0,0
    WORD    imenu_yoffset;      //
    WORD    ib_xoffset;         // Versatz des buttons bzgl. 0,0
    WORD    ib_yoffset;         //
    ULONG   input_changed;      // diese daten muessen bei OK beachtet werden

    /* -------------------------------------------------------------
    **             Daten zum Spiel-Requester
    ** -----------------------------------------------------------*/

    Object *bvideo;             // das Buttonobjekt

    // Video
    struct  YPAListReq vmenu;     // Game: das benötigte Menü
    //struct  video_node *vsel;     // Game: ausgewählter Modus, == V_..., siehe unten
    //struct  video_node *new_vsel; // Game: ausgewählter Modus, == V_..., siehe unten
    ULONG   new_modus;
    struct  MinList videolist;    // Liste mit Einträgen möglicher Videomodi
    LONG    v_actualitem;         // Game: was isn grade ausgewählt?
    struct  YPAListReq d3dmenu;   // Game: das benötigte Menü
    char    d3dguid[100];         // ausgew. 3Ddevice(GUID), merken auch nach laden
    char    d3dname[300];         // ausgew. 3Ddevice name fuer Oberflaeche
    char   *new_d3dguid;          // pointer auf neues
    char   *new_d3dname;          // pointer auf neues
    WORD    destfx;               // number destruction fx
    WORD    new_destfx;
    WORD    video_flags;
    WORD    new_video_flags;
    WORD    vb_xoffset;           // Versatz des buttons bzgl. 0,0
    WORD    vb_yoffset;           //
    //BOOL    forcesetvideo;        // setzen, auch wenn es der gleiche ist

    // Sound
    WORD    sound_flags;
    WORD    new_sound_flags;
    WORD    fxvolume;
    WORD    new_fxvolume;
    WORD    cdvolume;
    WORD    new_cdvolume;

    // Spiel
    BOOL    enemyindicator;     // Enemy-Indikator nutzen
    BOOL    new_enemyindicator;

    ULONG   settings_changed;   // diese daten muessen bei OK beachtet werden

    /* -------------------------------------------------------------
    **             Daten zum DiskRequester
    ** -----------------------------------------------------------*/

    Object *bdisk;              // das Buttonobjekt
    struct  YPAListReq dmenu;   // das Menu ist sozusagen der dateirequester
    LONG    d_actualitem;
    char    D_Name[ 300 ];      // der Name, der gerade aktuell ist
    WORD    DCursorPos;         // wo ist gerade der Cursor
    WORD    D_InputMode;        // 0 nichts, sonst Art (siehe DIM_....)
    APTR    UserDir;            // das Direktory mit den Spielständen
    struct  MinList flist;      // Liste mit den Fileinfonodes
    UBYTE   d_mask;             // Maske, welche Contexte bearbeitet werden sollen
    UBYTE   FoundContent;       // was wurde beim parsen gefunden?
    WORD    dmenu_xoffset;      // Versatz des menüs bzgl. ButtonObject
    WORD    dmenu_yoffset;      //
    WORD    db_xoffset;         // Versatz des buttons bzgl. 0,0
    WORD    db_yoffset;         //
    WORD    d_fromwhere;        // 0 aus dem Titel, sonst aus map aufgerufen

    /* -------------------------------------------------------------
    **             Daten zum LocaleRequester
    ** -----------------------------------------------------------*/

    Object *blocale;                    // das Buttonobjekt
    struct  MinList localelist;         // Nodes sind Sprachen
    struct  YPAListReq lmenu;           //
    struct  localeinfonode *lsel;       // ausgewählte Sprache
    struct  localeinfonode *new_lsel;   // ausgewählte Sprache
    WORD    lmenu_xoffset;              // Versatz des menüs bzgl. ButtonObject
    WORD    lmenu_yoffset;              //
    WORD    lb_xoffset;                 // Versatz des buttons bzgl. 0,0
    WORD    lb_yoffset;
    LONG    locale_changed;
    ULONG   num_languages;

    /* --------------------------------------------------------------
    **              Daten zum About-Requester
    ** ------------------------------------------------------------*/
    Object *babout;
    LONG    a_waittime;             // seit letzter Taste
    LONG    a_lettercount;          // eingegebene Buchstaben
    WORD    ab_xoffset;         // Versatz des buttons bzgl. 0,0
    WORD    ab_yoffset;         //

    #ifdef __NETWORK__
    /* -------------------------------------------------------------
    **              Daten zum Netzwerkerequester
    ** -----------------------------------------------------------*/
    Object *bnet;
    struct  YPAListReq nmenu;
    WORD    nmenu_xoffset;          // Versatz des menüs bzgl. ButtonObject
    WORD    nmenu_yoffset;          //
    WORD    nb_xoffset;             // Versatz des buttons bzgl. 0,0
    WORD    nb_yoffset;             //
    BOOL    nmenu_draw;             // soll Menue gezeichnet werden?
    WORD    n_selmode;              // was wird gerade im Listview dargestellt
    WORD    NSel;                   // Nummer des gerade selekteten
    BOOL    N_InputMode;            // wird das Stringgadget derzeit bedient
    char    N_Name[ STANDARD_NAMELEN + 2 ];  // der Name, der gerade im Gadget steht
    WORD    NCursorPos;             // wo ist gerade der Cursor
    WORD    NLevelOffset;           // falls ich einen level starte, stehen
    char   *NLevelName;             // dort die Daten bis zu OpenSession
    char    NPlayerName[ STANDARD_NAMELEN ]; // denke ich, daß ich das brauche
    ULONG   NCheckSum;              // meine eigene Checksumme
    LONG    tacs_time;              // wenn das runter auf 0 ist, ne Warnung ausgeben
                                    // wessen checksumme nicht stimmt.
    UBYTE   NPlayerOwner;           // von welchem Eigentümer bin ich in dem Spiel?
    UBYTE   FreeRaces;              // wer ist noch frei?
    UBYTE   SelRace;                // was wurde von mir gewählt
    UBYTE   Startable;              // Es kann losgehen
    BOOL    is_host;                // eröffnen wir eine Session?
    BOOL    modem_ask_session;      // Modem darf nun regelmaessing sessionliste upd.    
    LONG    session_time;           // für Abfrage der Sessions
    ULONG   msgcount;
    UBYTE   Welcomed;               // Von allen Welcome bekommen->Ready erlaubt
    UBYTE   ReadyToStart;           // Ich bin bereit...
    UBYTE   remotestart;            // wurde von Lobby gestartet
    BOOL    disconnected;           // die Verbindung wurde getrennt
    BOOL    blocked;                // session gelocked waehrend Shell??
                                    // nur Flag fuer Host!!!
    LONG    trouble_count;          // Solange > 0: Troublesymbol zeichnen

    WORD    act_messageline;
    char    messagebuffer[MAXNUM_MESSAGELINES][STANDARD_NAMELEN];
    char    lastsender[ STANDARD_NAMELEN ];

    struct  netlevel netlevel[128]; // weil MAXNUM_LEVEL hier noch nicht definiert ist
    WORD    num_netlevels;
    
    BOOL    dont_send;              // dann werden keine Messages mehr verschickt.
                                    // nur zum Behandeln von Problemen
    UBYTE   network_trouble_owner;  // zum merken fuer spaeteres aufraeumen                              
    char    network_trouble_name[ STANDARD_NAMELEN ];                                
    WORD    network_trouble;        // der derzeitige Problemstatus, siehe ypamessages.h
    WORD    network_allok;          // Art der Loesung des problems
    ULONG   update_time_normal;     // wird bei providerwahl ausgefuellt
    ULONG   flush_time_normal;
    ULONG   kickoff_time;           // nach dieser Zeit darf der Host jemanden
                                    // rausschmeissen, wenn er solange nix hoert
    LONG    latency_check;          // Zaehler fuer Latency Messages
    LONG    network_trouble_count;  // Zaehler fuer Problembearbeitung (max. Dauer)
    LONG    network_allok_count;    // Solange art der Loesung anzeigen
    LONG    corpse_check;           // Zaehler zum testen nicht beseitigter schattenvehicle 
    LONG    sendscore;              // Zaehler, nachdem mal wieder der score abgeglichen werden muss                               

    /*** Playerdata 8 weil Eigentümer auch Offset !!! ***/
    struct playerdata player[ 8 ];  // offset ist der Eigentümer!
    struct playerdata2 player2[ MAXNUM_PLAYERS ]; // offset ist interne Nummer
    #endif

    /* -------------------------------------------------------------
    **             Daten zum Confirmrequester                                   
    ** -----------------------------------------------------------*/
    Object *confirm;
    ULONG   confirm_modus;
    char   *confirm_texts;
    
    /* -------------------------------------------------------------
    **             Diverses
    ** -----------------------------------------------------------*/

    /*** Actionen, die für außen bestimmt sind ***/
    struct  rsaction_msg GSA;   // das wird nach außen geleitet

    struct  input_info inp[ NUM_INPUTEVENTS + 1 ];   // Die Informationen
                                // zu allen Eingabeereignissen,
                                // die wir auswerten. Das Offset in das Array
                                // entspricht  der Menü-Nummer ( I_.... s.u.)

    /*** wer war am Ende aktiver Requester und soll es am Anfang wieder sein? ***/
    WORD    active_req;
    WORD    exist_savegame;

    /*** Tracknummern ***/
    WORD    shelltrack;
    WORD    missiontrack;
    WORD    loadingtrack;
    WORD    debriefingtrack;
    ULONG   shell_min_delay;
    ULONG   shell_max_delay;
    ULONG   mission_min_delay;
    ULONG   mission_max_delay;
    ULONG   loading_min_delay;
    ULONG   loading_max_delay;
    ULONG   debriefing_min_delay;
    ULONG   debriefing_max_delay;

    /*** für Demos und Screenblanker ***/
    char    fn[ 32 ][ 256 ];    // Puffer für Filenamen
    WORD    fn_number;          // Anzahl derzeitiger Filenamen im Puffer
    UBYTE   cd;                 // Kopie fuer schnellen Zugriff von player2
    LONG    last_cdcheck;       // wann wurde das letzte mal nachgesehen?
    ULONG   last_input_event;   // TimeStamp für das letzte Eingabeereignis
    ULONG   wait_til_demo;      // solange warten, bis Blanker startet

    /* --------------------------------------------------------------------
    **                             Netzwerkstatistik
    ** ------------------------------------------------------------------*/
    LONG    transfer_sendcount;     // wieviel im Intervall empfangen/gesendet
    LONG    transfer_rcvcount;
    LONG    transfer_rcvtime;       // zaehler fuer Intervall
    LONG    transfer_sbps;          // momentaner Durchschnitt
    LONG    transfer_rbps;
    LONG    transfer_rbps_min;      // min und max der datenrate
    LONG    transfer_rbps_max;
    LONG    transfer_rbps_avr;      // Langzeitdurchschnitt
    LONG    transfer_sbps_min;
    LONG    transfer_sbps_max;
    LONG    transfer_sbps_avr;
    LONG    transfer_gcount;        // sende-Messagezaehler
    LONG    transfer_pckt;          // paketgroesse, bezieht sich auf Senden
    LONG    transfer_pckt_min;      // min und max dafuer
    LONG    transfer_pckt_max;
    LONG    transfer_pckt_count;    // Pakete im Intervall
    LONG    transfer_pckt_avr;      // langzeit durchschnitt

};


/*** GameShell-Actionen, die nach außen gemeldet werden (-->struct gs_action) ***/
#define A_NOACTION          0L  // war nüscht los....
#define A_QUIT              1L  // das Spiel verlassen
#define A_PLAY              2L  // weiterspielen, para 0 und 1 haben Levelidentifaktion
#define A_LOAD              3L  // in 0 Levelnummer
#define A_NETSTART          4L  // Start eines Netzwerk-Levels
#define A_DEMO              5L  // startet demo/screenblanker-modus

#define DIM_NONE            0L  // keine Eingabe
#define DIM_SAVE            1L  // zum Speichern
#define DIM_LOAD            2L  // zum Laden
#define DIM_CREATE          3L  // zum Erzeugen eines neuen Users
#define DIM_KILL            4L  // zum weglöschen....

/*** Die netreq-Modi ***/
#define NM_PROVIDER         0
#define NM_SESSIONS         1
#define NM_PLAYER           2
#define NM_LEVEL            3
#define NM_MESSAGE          4

/*** shellmodi, was also gerade angezeigt wird ***/
#define SHELLMODE_TITLE     1   // Titelbild
#define SHELLMODE_INPUT     2   // Inputrequester
#define SHELLMODE_SETTINGS  3   // die restlichen Einstellungen
#define SHELLMODE_TUTORIAL  4   // die Uebungsmissionen
#define SHELLMODE_PLAY      5   // das normale Spiel
#define SHELLMODE_NETWORK   6   // Netzwerkeinstellungen
#define SHELLMODE_LOCALE    7   // der LocaleRequester ist offen
#define SHELLMODE_ABOUT     8   // der geheime Requester
#define SHELLMODE_DISK      9   // Spielerauswahl
#define SHELLMODE_HELP     10   // nur fuer den Fall der Faelle

/*** Confirm-Flags ***/
#define CONFIRM_NONE            0   // zur Zeit nicht aktiviert
#define CONFIRM_LOADFROMMAP     1   // wirklich das Savegame von der Map laden?
#define CONFIRM_NETSTARTALONE   2   // Netzspiel allein starten
#define CONFIRM_SAVEANDOVERWRITE 3  // Savegame in DiskReq ueberschreiben
#define CONFIRM_MORECDS         4   // CDs reichen nicht


/* ---------------------------------------------------------------------
** OBSOLET: Sollte nur noch für die Vehicleoffsets genommen werden,
** will heißen, für Eigentümer 2 werden Vehicle mit offset 4 * ... frei-
** geschalten.
** -------------------------------------------------------------------*/
#define OWNER_OF_1          1
#define OWNER_OF_2          4
#define OWNER_OF_3          5
#define OWNER_OF_4          3
#define OWNER_OF_5          2
#define OWNER_OF_6          6
#define OWNER_OF_7          7


/*** Welche Rassen sind noch frei? ***/
#define FREERACE_USER       (1<<0)
#define FREERACE_KYTERNESER (1<<1)
#define FREERACE_MYKONIER   (1<<2)
#define FREERACE_TAERKASTEN (1<<3)


/*** Bezeichner für die einzelnen Requester ***/
#define REQ_UBALKEN         (1L<<0)     // unterer Balken
#define REQ_OBALKEN         (1L<<1)     // unterer Balken
#define REQ_INPUT           (1L<<2)     // Input-Preferences
#define REQ_SOUND           (1L<<3)     // Sound-Preferences
#define REQ_VIDEO           (1L<<4)     // Video-Preferences
#define REQ_DISK            (1L<<5)     // Ein- Ausgabe-Requester
#define REQ_PLAY            (1L<<6)     // PlayMap
#define REQ_LOCALE          (1L<<7)     // LanguageRequester
#define REQ_ABOUT           (1L<<8)     // AboutRequester
#define REQ_NET             (1L<<9)     // Der Netzrequester

/*** Soundflags ***/
#define SF_INVERTLR         (1<<0)      // Vertauschung der Links-Rechts-Verteilung
#define SF_PLAYRIGHT        (1<<1)      // damit er weiß, daß er nach Left noch
                                        // was zu spielenn hat
#define SF_PLAYINVERT       (1<<2)      // wegen pos. verschieben etc.
#define SF_CDSOUND          (1<<4)

/*** videoflags ***/
#define VF_FARVIEW          (1<<0)      // Weitsicht
#define VF_HEAVEN           (1<<1)      // Himmel zeichnen
#define VF_SOFTMOUSE        (1<<2)
#define VF_DRAWPRIMITIVE    (1<<3)
#define VF_16BITTEXTURE     (1<<4)      // 

/*** Was hat sich im Inputrequester veraendert? ***/
#define ICF_JOYSTICK        (1L<<0)
#define ICF_FORCEFEEDBACK   (1L<<1)
#define ICF_ALTJOYSTICK     (1L<<2)

/*** Was hat sich im Settingsrequester veraendert? ***/
#define SCF_MODE            (1L<<0)
#define SCF_INVERT          (1L<<1)
#define SCF_16BITTEXTURE    (1L<<2)
#define SCF_HEAVEN          (1L<<3)
#define SCF_FARVIEW         (1L<<4)
#define SCF_ENEMYINDICATOR  (1L<<5)
#define SCF_FXNUMBER        (1L<<6)
#define SCF_FXVOLUME        (1L<<7)
#define SCF_CDVOLUME        (1L<<8)
#define SCF_CDSOUND         (1L<<9)
#define SCF_DRAWPRIMITIVE   (1L<<10)
#define SCF_SOFTMOUSE       (1L<<11)
#define SCF_3DDEVICE        (1L<<12)

/*** Was hat sich im Localerequester veraendert? ***/
#define LCF_LANGUAGE        (1L<<0)


/*** Jedes Gadget der ButtonClass hat eine ID ***/
#define GSID_DISK           1001    // globaler Disk-Requester
#define GSID_UNTENINFO      1002    // der Rest des unteren Balkens für Infos
#define GSID_INPUT          1003    // InputPreferences
#define GSID_VIDEO          1004    // VideoPreferences
#define GSID_SOUND          1005    // SoundPreferences
#define GSID_PLAY           1006    // Level-Map
#define GSID_QUIT           1007    // das Schließgadget
#define GSID_LOCALE         1008    // das Schließgadget
#define GSID_PL_SETBACK     1011    // An Anfang zurückspringen
#define GSID_PL_FASTBACK    1012    // Rückspulen
#define GSID_PL_FASTFORWARD 1013    // Z1013: schnell vorwärts
#define GSID_PL_GAME        1014    // Spielen
#define GSID_PL_LOAD        1015    // Laden
#define GSID_NET            1016    // Netzwerk
#define GSID_HELP           1017
#define GSID_GAME           1018
#define GSID_PL_QUIT        1019    // das QuitIcon
#define GSID_PL_GOTOLOADSAVE 1020   

#define GSID_JOYSTICK       1050    // Z 1013, Joystickschalter
#define GSID_INPUTOK        1051    // Sachen übernehmen
#define GSID_INPUTHELP      1052    // Hilfe
#define GSID_INPUTRESET     1053    // Sachen rücksetzen
#define GSID_INPUTCANCEL    1054    // Sachen nicht übernehmen
#define GSID_FORCEFEEDBACK  1055    // Forcefeedback
#define GSID_SWITCHOFF      1056    // Hotkey-Ausschalter
#define GSID_INPUTHEADLINE  1057
#define GSID_INPUTHEADLINE2 1058
#define GSID_INPUTHEADLINE3 1059
#define GSID_INPUTHEADLINE4 1060
#define GSID_ALTJOYSTICK    1061    // alternatives Joystick model

#define GSID_DISKSTRING     1100    // das Stringgadget zum Dateirequester
#define GSID_LOAD           1101    // LOAD-Button
#define GSID_DELETE         1102    // Delete
#define GSID_NEWGAME        1103    // neues Spiel
#define GSID_SAVE           1104    // Extra Savebutton
#define GSID_DISKOK         1105
#define GSID_DISKCANCEL     1106
#define GSID_DISKHELP       1107
#define GSID_DISKHEADLINE   1108
#define GSID_DISKHEADLINE2  1109
#define GSID_DISKHEADLINE3  1110
#define GSID_DISKHEADLINE4  1111

#define GSID_16BITTEXTURE   1150    // soll es überhaupt Sound geben?
#define GSID_SOUND_LR       1151    // Schalter für links-recht-Verteilung
#define GSID_FXVOLUMESLIDER 1152    // der Volumeslider
#define GSID_FXVOLUMENUMBER 1153    // Textgadget mit aktueller Lautstärke
#define GSID_CDVOLUMESLIDER 1154    // der Volumeslider für die CD
#define GSID_CDVOLUMENUMBER 1155    // Textgadget mit aktueller Lautstärke
#define GSID_VMENU_BUTTON   1156    // das gadget mit der Res. und zum Öffnen des Menus
#define GSID_FARVIEW        1157    // Weitsicht-Schalter
#define GSID_FXNUMBER       1158    // Textgadget für Zahl der effekte
#define GSID_FXSLIDER       1159    // Slider für Zahl der effekte
#define GSID_HEAVEN         1160    // Checkmark für Himmel
#define GSID_SETTINGSOK     1161    // Settings übernehmen
#define GSID_SETTINGSCANCEL 1162    // Settings nicht übernehmen
#define GSID_ENEMYINDICATOR 1163    // Zeichne Zeuch in die 3D-Welt
#define GSID_CDSOUND        1164
#define GSID_SOFTMOUSE      1165
#define GSID_DRAWPRIMITIVE  1166
#define GSID_SETTINGSHELP   1167
#define GSID_SETTINGSHEADLINE  1168
#define GSID_SETTINGSHEADLINE2 1169
#define GSID_SETTINGSHEADLINE3 1170
#define GSID_SETTINGSHEADLINE4 1171
#define GSID_3DMENU_BUTTON  1172

#define GSID_NETSTRING      1200    // das Netzwerkstringgadget
#define GSID_NETOK          1201
#define GSID_NETNEW         1202    // evtl. mit variablem Inhalt
#define GSID_NETCANCEL      1203
#define GSID_NETHEADLINE    1204    // die Überschrift des Requesters
#define GSID_NETBACK        1205    // zurück
#define GSID_USERRACE       1206    // userrasse auswählen
#define GSID_KYTERNESER     1207    // die Kyterneser auf 2. Position
#define GSID_MYKONIER       1208    // Mykonier...
#define GSID_TAERKASTEN     1209    // taerkasten, soso...
#define GSID_PLAYERNAME1    1210    // Stringgadget fuer Name des ersten Players
#define GSID_PLAYERNAME2    1211
#define GSID_PLAYERNAME3    1212
#define GSID_PLAYERNAME4    1213
#define GSID_PLAYERSTATUS1  1214    // Status des ersten Players
#define GSID_PLAYERSTATUS2  1215
#define GSID_PLAYERSTATUS3  1216
#define GSID_PLAYERSTATUS4  1217
#define GSID_NETHELP        1218
#define GSID_NETREADY       1219
#define GSID_SELRACE        1220
#define GSID_NETREADYTEXT   1221
#define GSID_NETHEADLINE2   1222
#define GSID_NETHEADLINE3   1223
#define GSID_NETHEADLINE4   1224
#define GSID_NETSEND        1225
#define GSID_LEVELNAME      1226
#define GSID_LEVELTEXT      1227           
#define GSID_REFRESHSESSIONS 1228

#define GSID_LOCALEOK       1250
#define GSID_LOCALECANCEL   1251
#define GSID_LOCALEHELP     1252
#define GSID_LOCALEHEADLINE 1253
#define GSID_LOCALEHEADLINE2 1254
#define GSID_LOCALEHEADLINE3 1255
#define GSID_LOCALEHEADLINE4 1256

#define GSID_CONFIRMOK      1300
#define GSID_CONFIRMCANCEL  1301
#define GSID_CONFIRMTEXT    1302
#define GSID_CONFIRMTEXT2   1303

#define GSID_OBJECT_INPUT   2000    // das gesamte Inputobjekt, also dessen ID im Req.
#define GSID_OBJECT_PLAY    2001    // das gesamte Playobjekt,  also dessen ID im Req.
#define GSID_OBJECT_VIDEO   2002    // das gesamte Settingsobjekt, also dessen ID im Req.
#define GSID_OBJECT_DISK    2003    // das gesamte Diskobjekt,  also dessen ID im Req.
#define GSID_OBJECT_LOCALE  2004    // das gesamte Localeobjekt,also dessen ID im Req.
#define GSID_OBJECT_ABOUT   2005    // das gesamte Aboutobjekt, also dessen ID im Req.
#define GSID_OBJECT_NET     2006    // das gesamte Netzobject,  also dessen ID im Req.


/*** Jedes gadget kann Nachrichten senden ***/
#define GS_DISK_OPEN            1001    // DiskR. öffnen
#define GS_DISK_CLOSE           1002    // DiskR. schließen
#define GS_PLAY_OPEN            1003    // LevelMap öffnen
#define GS_PLAY_CLOSE           1004    // LevelMap schließen
#define GS_VIDEO_OPEN           1005    // VideoRequester öffnen
#define GS_VIDEO_CLOSE          1006    // VideoRequester schließen
#define GS_INPUT_OPEN           1007    // InputRequester öffnen
#define GS_INPUT_CLOSE          1008    // InputRequester schließen
#define GS_SOUND_OPEN           1009    // SoundRequester öffnen
#define GS_SOUND_CLOSE          1010    // SoundRequester schließen
#define GS_LOCALE_OPEN          1011    // SoundRequester öffnen
#define GS_LOCALE_CLOSE         1012    // SoundRequester schließen
#define GS_QUIT                 1013    // Raus hier!
#define GS_PL_SETBACK           1016    // An Anfang zurückspringen
#define GS_PL_FASTBACK          1017    // Rückspulen   // OBSOLET
#define GS_PL_FASTFORWARD       1018    // Z1013: schnell vorwärts
#define GS_PL_GAME              1019    // Spielen
#define GS_PL_ENDFASTFORWARD    1020    // Spulen Abschluß
#define GS_PL_LOAD              1021    // Laden
#define GS_NET_OPEN             1022    // NetzRequester öffnen
#define GS_NET_CLOSE            1023    // NetzRequester schließen
#define GS_GAME_OPEN            1024    // gamescreen zeigen
#define GS_HELP_OPEN            1025    // Hilfe aufrufen
#define GS_PL_GOTOLOADSAVE      1026

#define GS_USEJOYSTICK          1050
#define GS_NOJOYSTICK           1051
#define GS_INPUTOK              1052
#define GS_INPUTRESET           1053
#define GS_INPUTCANCEL          1054
#define GS_NOFORCEFEEDBACK      1055
#define GS_USEFORCEFEEDBACK     1056
#define GS_SWITCHOFF            1057
#define GS_ALTJOYSTICK_YES      1058
#define GS_ALTJOYSTICK_NO       1059

#define GS_VMENU_OPEN           1100    // Öffnet Resolution-Menu
#define GS_VMENU_CLOSE          1101    // machts wieder zu
#define GS_FARVIEW              1102    // Weitsicht, also 5 Sektoren
#define GS_NOFARVIEW            1103    // doch net
#define GS_PALETTEFX            1104    // palettenfx
#define GS_NOPALETTEFX          1105
#define GS_HEAVEN               1106    // Himmel zeichnen
#define GS_NOHEAVEN             1107
#define GS_FXSLIDERHIT          1108
#define GS_FXSLIDERRELEASED     1109
#define GS_FXSLIDERHOLD         1110
#define GS_SOUND_LR             1111    // Verteilung Links  rechts
#define GS_SOUND_RL             1112    // Verteilung rechts links
#define GS_16BITTEXTURE_YES     1113    //  na klar!
#define GS_16BITTEXTURE_NO      1114    //  ach nö!
#define GS_SOUND_HITFXVOLUME    1115    // Slider angefaßt
#define GS_SOUND_HOLDFXVOLUME   1116    // Slider gehalten
#define GS_SOUND_RELEASEFXVOLUME 1117   // Slider losgelassen
#define GS_SOUND_HITCDVOLUME    1118    // Slider angefaßt
#define GS_SOUND_HOLDCDVOLUME   1119    // Slider gehalten
#define GS_SOUND_RELEASECDVOLUME 1120   // Slider losgelassen
#define GS_CHANNELSLIDERHIT     1121
#define GS_CHANNELSLIDERRELEASED 1122
#define GS_CHANNELSLIDERHOLD    1123
#define GS_SETTINGSOK           1124
#define GS_SETTINGSCANCEL       1125
#define GS_ENEMY_YES            1126
#define GS_ENEMY_NO             1127
#define GS_CDSOUND_YES          1128
#define GS_CDSOUND_NO           1129
#define GS_DRAWPRIMITIVE_YES    1130
#define GS_DRAWPRIMITIVE_NO     1131
#define GS_SOFTMOUSE            1132
#define GS_NOSOFTMOUSE          1133
#define GS_3DMENU_OPEN          1134
#define GS_3DMENU_CLOSE         1135
#define GS_SAVED3D_YES          1136
#define GS_SAVED3D_NO           1137
#define GS_D3DPRIM_YES          1138
#define GS_D3DPRIM_NO           1139

#define GS_LOAD                 1160    // los, laden
#define GS_DELETE               1161    // Löschen des Eintrages
#define GS_NEWGAME              1162    // Neues Spielfile erzeugen
#define GS_SAVE                 1163    // extra abspeichern
#define GS_DISKOK               1164    // requester wird mit OK verlassen
#define GS_DISKCANCEL           1165    // requester wird mit CANCEL verlassen

#define GS_NETOK                1200    // ok-Button losgelassen
#define GS_NETNEW               1201
#define GS_NETCANCEL            1202
#define GS_NETBACK              1203    // eine Stufe zurück
#define GS_USERRACE             1204
#define GS_KYTERNESER           1205
#define GS_MYKONIER             1206
#define GS_TAERKASTEN           1207
#define GS_NETREADY             1208
#define GS_NETSTOP              1209
#define GS_NETSEND              1210

#define GS_HELP                 1250    // allgemeine Hilfsanforderung
#define GS_BUTTONDOWN           1251    // wenn Ton ohne Funktionalität

#define GS_LOCALEOK             1300
#define GS_LOCALECANCEL         1301

#define GS_CONFIRMOK            1350
#define GS_CONFIRMCANCEL        1351

/*** Die Masken, was geladen bzw. gespeichert werden soll ***/
#define DM_USER             (1<<0)      // allgemeine Daten
#define DM_INPUT            (1<<1)      // Input-daten
#define DM_VIDEO            (1<<2)      // Video-Daten
#define DM_SOUND            (1<<3)      // Sound-Daten
#define DM_SCORE            (1<<4)      // Spielstand
#define DM_SHELL            (1<<5)      // Shelldaten
#define DM_BUILD            (1<<6)      // veränderte vehicledaten
#define DM_BUDDY            (1<<7)      // buddies

/*** Nummern der Menüeinträge des InputRequesters ***/
#define I_PAUSE             1L
#define I_QUIT              2L
#define I_DRV_DIR           3L
#define I_DRV_SPEED         4L
#define I_GUN_HEIGHT        5L
#define I_FLY_HEIGHT        6L
#define I_FLY_SPEED         7L
#define I_FLY_DIR           8L
#define I_STOP              9L
#define I_FIRE              10L
#define I_FIREVIEW          11L
#define I_FIREGUN           12L
#define I_MAKECOMMANDER     13L
#define I_HUD               14L
#define I_AUTOPILOT         15L
#define I_ORDER             16L
#define I_NEW               17L
#define I_ADD               18L
#define I_FINDER            19L
#define I_AGGR1             20L
#define I_AGGR2             21L
#define I_AGGR3             22L
#define I_AGGR4             23L
#define I_AGGR5             24L
#define I_MAP               25L
#define I_WAYPOINT          26L
#define I_LANDSCAPE         27L
#define I_OWNER             28L
#define I_HEIGHT            29L
#define I_MAPMINI           30L
#define I_LOCKVIEWER        31L
#define I_ZOOMIN            32L
#define I_ZOOMOUT           33L
#define I_LOGWIN            34L
#define I_CONTROL           35L
#define I_LASTOCCUPIED      36L
#define I_FIGHT             37L
#define I_TOROBO            38L
#define I_TOCOMMANDER       39L
#define I_NEXTMAN           40L
#define I_NEXTCOM           41L
#define I_LASTMSG           42L
#define I_TOALL             43L
#define I_HELP              44L
#define I_ANALYZER          45L

/*** Die Shellsounds 1. SoundSource ***/
#define SHELLSOUND_VOLUME           0   // für'n Lautstärkeregler
#define SHELLSOUND_RIGHT            1   // links-rechts-Konvertierung
#define SHELLSOUND_LEFT             2   // links-rechts-Konvertierung
#define SHELLSOUND_BUTTON           3   // BDown, keine Action
#define SHELLSOUND_QUIT             4   // Raus hier
#define SHELLSOUND_SLIDER           5   // allgemeiner Slider-HOLD-sound
#define SHELLSOUND_WELCOME          6   // Sample nach Start
#define SHELLSOUND_MENUOPEN         7   // menü geöffnet
#define SHELLSOUND_OVERLEVEL        8   // Maus über Level
#define SHELLSOUND_LEVELSELECT      9   // Level selectiert
#define SHELLSOUND_TEXTAPPEAR       10  // Untermalung für Ausschriften
#define SHELLSOUND_OBJECTAPPEAR     11  //  "          für Objekte im MB
#define SHELLSOUND_SECTORCONQUERED  12  // DB: Sektor erobert
#define SHELLSOUND_VHCLDESTROYED    13  // DB: Fahrzeug zerstört
#define SHELLSOUND_BLDGCONQUERED    14  // DB: Gebäude erobert
#define SHELLSOUND_TIMERCOUNT       15  // DB: für das Höchzählen der Spielzeit

/*** Die 2. Shellsounds ***/
#define SHELLSOUND_SELECT           0
#define SHELLSOUND_ERROR            1   // allgemeine Fehlermeldung
#define SHELLSOUND_ATTENTION        2   // wnimanije
#define SHELLSOUND_SECRET           3   // beim Öffnen des versteckten requesters
#define SHELLSOUND_PLASMA                       4       // beim Aufnehmen eines plasmabatzens

/*** weitere Vereinbarungen ***/

#define STAT_IMENU_WIDTH    240
#define STAT_VMENU_WIDTH    100
#define STAT_DMENU_WIDTH    116
#define STAT_LMENU_WIDTH    85
#define NUM_INPUT_SHOWN     8       // Anzahl maximal sichtbarer Einträge
#define NUM_VIDEO_SHOWN     4       // Anzahl maximal sichtbarer Einträge
#define NUM_D3D_SHOWN       4       // Anzahl maximal sichtbarer Einträge
#define NUM_DISK_SHOWN      10      // Anzahl maximal sichtbarer Einträge
#define NUM_LOCALE_SHOWN    10       // Anzahl maximal sichtbarer Einträge
#define NUM_NET_SHOWN       12      // Anzahl maximal sichtbarer Einträge
#define MAX_NUM_CHANNELS    8       // wieviele ganäle dun mir unterstützn?


#define UNKNOWN_NAME    "NEWUSER"   // Anfang für den DiskRequester
#define CURSORSIGN      "h"         // das Cursorzeichgen für Stringgadgets
#define CURSORSIGN_E    "i"         // "leerer Cursor"
#define GAMELOADED      "["         // User geladen (Diskrequester)

/*** Tutorial Level ***/
#define TUTORIAL_LEVEL_1        (25)
#define TUTORIAL_LEVEL_2        (26)
#define TUTORIAL_LEVEL_3        (27)

/*-----------------------------------------------------------------*/

/*
**  Um zu tasten auch die darstellbaren Strings und andere
**  Informationen zu bekommen, bauen wir eine große Tabelle
**  auf
*/

struct key_info {

    char *name;         // als was soll sie dargestellt werden
    char *config;       // so äußert sich das im ConfigFile/String für IObject
    UBYTE ascii;        // was hat sie denn für einen ASCII-Code
    UBYTE pad[3];
};

struct video_node {

    struct MinNode node;

    char   name[ 32 ];  // Name des Modus
    ULONG  modus;       // DisplayID
    WORD   res_x;       // Auflösung, vorerst nur Platz dafür reservieren
    WORD   res_y;
};


/*** Für die Routine WasKeyJustUsed gelten folgende Testflags: ***/
#define CHECK_QUAL      (1 << 0)    // teste  Qualifier
#define CHECK_KEY       (1 << 1)    // teste  Tasten allein


/*** Defaultsettings für VideoDaten ***/
#define VISSECTORS_NEAR     (5)                    // zu beachtende Sektoren
#define NORMVISLIMIT_NEAR   YWA_NormVisLimit_DEF   // Sichtweite
#define NORMDFADELEN_NEAR   YWA_NormDFadeLen_DEF   // DepthfadingGrenze
#define VISSECTORS_FAR      (9)
#define NORMVISLIMIT_FAR    (YWA_NormVisLimit_DEF+(YWA_NormVisLimit_DEF/2))
#define NORMDFADELEN_FAR    (YWA_NormDFadeLen_DEF+(YWA_NormDFadeLen_DEF/2))

/*** spezielle Flags für ExtraViewer --> Abspeichern und Laden ***/
#define EV_No       0       // kein ExtraViewer
#define EV_RoboGun  1       // wir saßen inner Bordkanone


/*** Das Treiberzeichen, um die Files systemunabhängig zu halten ***/
#define KEYBORD_DRIVER_SIGN     "$"
#define NICHTSTRING             "-"     // Taste wird nicht unterstützt

/*** Wann schaltet sich der Blanker ein? ***/
#define WAIT_TIL_DEMO           (200000)

/*** Treiberdefinitionen ***/
#ifdef AMIGA
    #define KEYBORD_DRIVER  "amiga:"
#endif

#ifdef MSDOS
    #define KEYBORD_DRIVER  "dkey:"
#endif

#ifdef __WINDOWS__
    #define KEYBORD_DRIVER  "winp:"
#endif



#endif

