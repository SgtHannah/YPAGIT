#ifdef __NETWORK__

#ifndef NETWORK_WINDPCLASS_H
#define NETWORK_WINDPCLASS_H
/*
**  $Source: $
**  $Revision: 38.3 $
**  $Date: 1995/08/17 23:45:25 $
**  $Locker:  $
**  $Author:  $
**
*/

#ifndef NETWORK_NETWORKFLAGS_H
#include <network/networkflags.h>
#endif

#include "network/winlist.h"

#ifdef WINDP_WINBOX

    /*** Windows-spezifische Includes ***/
    #ifndef _INC_WINDOWS
    #include <windows.h>
    #endif

    #ifndef _INC_WINDOWSX
    #include <windowsx.h>
    #endif

    #ifndef __DPLAY_INCLUDED_
    #include <dplay.h>
    #endif

    #ifndef __DPLOBBY_INCLUDED_
    #include <dplobby.h>
    #endif

    #ifdef INITGUID
    DEFINE_GUID( GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    #endif
    
#else

    #ifndef EXEC_TYPES_H
    #include <exec/types.h>
    #endif

    #ifndef EXEC_NODES_H
    #include <exec/nodes.h>
    #endif

    #ifndef NUCLEUS_NUCLEUSCLASS_H
    #include <nucleus/nucleusclass.h>
    #endif

    #ifndef NETWORK_NETWORKCLASS_H
    #include <network/networkclass.h>
    #endif

#endif

/*
** Die Windows-Spezifische Netzwerkrealisation, sprich die DirectPlay-Klasse.
** Um alle Windowsspezischen Datentypen von vfm fernzuhalten, wird WINDP_WINBOX
** benutzt. Ist dies definiert, erscheinen die Strukturen windows-spezifisch.
** Um keine Probleme mit der Größe zu bekommen, definiere ich nur einen
** Pointer in der LID, der auf die Daten, die eh bloß DirectPlay verwendet,
** zeigt. Unter dem Public-Mode ist es einfach nur ein void-Pointer, unter
** Winbox zeigt er auf eine definierte Struktur. DIESE WILL ICH NIE DIREKT
** AUSLESEN UND WENN ICH DOCH MAL WILL, DANN WILL ICH EBEN BESSER NICHT! Sprich
** alle Daten werden über Methoden ausgelesen.
**
** Sollten doch noch VFM-spezifische daten (also was sich unter WINDOWS nicht
** lohnt) auftreten, halte ich dafür einen Pointer auf eine Struktur bereit,
** die unter WINBOX wiederum nicht ausgelesen werden darf. Hier nehme ich aber
** keine Methoden, sondern definiere alles bereits so, daß Daten dieser Klasse,
** die in der WinBox benötigt werden, nicht in der VFM-Struktur stehen!
**
** Methoden:
**
*/



#define WINDP_CLASSID "windp.class"

/*** Methods ***/
#define WINDPM_BASE                (NWM_BASE + METHOD_DISTANCE)


/*** Attribute ***/
#define WINDPA_BASE                (NWA_BASE + ATTRIB_DISTANCE)



#ifdef WINDP_WINBOX

    #ifdef __BETA__
    // die Beta 2 GUID
    // {017C7740-FAD4-11d1-9C03-00A0C90832CD}
    DEFINE_GUID(YPA_GUID, 0x17c7740, 0xfad4, 0x11d1, 0x9c, 0x3, 0x0, 0xa0, 0xc9, 
                          0x8, 0x32, 0xcd);
    #else
    
        #ifdef __TRIAL__
        // fuer die Trial Version
        // {017C7741-FAD4-11d1-9C03-00A0C90832CD}
        DEFINE_GUID(YPA_GUID, 0x17c7741, 0xfad4, 0x11d1, 0x9c, 0x3, 0x0, 0xa0, 
                              0xc9, 0x8, 0x32, 0xcd);
        #else
        // Die globale Applikation-GUID
        // {381f0620-fc68-11d0-8af9-0020aff04466}
        DEFINE_GUID( YPA_GUID, 0x381f0620, 0xfc68, 0x11d0, 0x8a, 0xf9, 0x0,
                               0x20, 0xaf, 0xf0, 0x44, 0x66 );
        #endif
    #endif

#endif


/*** Das Button/Eingabe-Feld, also die Local Instance Data dieser Klasse ***/
#ifdef WINDP_WINBOX

    #define WDP_NORMALBLOCKSIZE        (1024) // DP spinnt neuerdings (65536)
    #define WDP_RECEIVEBUFFERSIZE      (65536)

    /*** DPLAY-Spezifischen daten ***/
    struct provider_entry {

        char   name[ PROVIDER_NAMELEN ];
        GUID   guid;
        LPVOID connection; // neu, Info fuer Initializeconnection, kommt von EnumConnections      
        };

    struct session_entry {

        char name[ SESSION_NAMELEN ];
        char password[ SESSION_NAMELEN ];
        GUID guid;
        DPSESSIONDESC2 desc;        // zum joinen merken. Die einzigen Pointer
                                    // sind name und Password und die habe ich
                                    // extra noch gemerkt (s.o.)
        };

    struct player_entry {

        char     name[ PLAYER_NAMELEN ];
        DPID     dpid;    // von Player
        unsigned long data;
        unsigned long flags;
        };

    /*** Player-Flags ***/
    #define WDPF_OWNPLAYER      (1L<<0)     // Player wurde auf eigener M. erz.

    struct windp_win_data {

        /*** Das ProviderArray ***/
        unsigned long num_providers;
        struct provider_entry provider[ MAXNUM_PROVIDERS ];

        /*** Der aktuell eingestellte Provider ***/
        LPDIRECTPLAY    dpo1;
        LPDIRECTPLAY3A  dpo2;
        unsigned long   act_provider;
        unsigned long   conn_type;

        /*** Das SessionArray ***/
        unsigned long num_sessions;
        struct session_entry session[ MAXNUM_SESSIONS ];

        /*** Die aktuelle Session ***/
        unsigned long act_session;        // nur sinnvoll wenn session_joined
        struct session_entry own_session; // nur sinnvoll wenn session_created
        unsigned long session_joined;
        unsigned long session_created;
        
        /*** Sessiontest ***/
        char versionident[ STANDARD_NAMELEN * 2 ];
        BOOL versioncheck;

        /*** Das PlayerArray ***/
        struct player_entry player[ MAXNUM_PLAYERS ];
        unsigned long num_players;

        /*** Messagezeug ***/
        char *normalblock;      // initialisierter Empfangsblock
        char *bigblock;         // Behelfsblock, wenn Buffer zu klein
        unsigned long normalsize;   // die Größen der Blöcke
        unsigned long bigsize;
        unsigned long guaranteed;   // unterstützt garantiertes Verschicken
        char *sendbuffer;
        unsigned long sendbuffersize;
        unsigned long sendbufferoffset; // der Füllstand des Buffers
        unsigned long guaranteed_mode;
        struct WinList recv_list;
        struct WinList send_list;
        HANDLE          mevent;       // fuer receive/WaitForSingleObject
        unsigned long debug;        // Flag ob ein debug script angelegt werden soll
        };

    /*** Die LID aus Sicht der WinBox ***/
    struct windp_data {

        struct  windp_win_data *win_data;
        LPVOID  vfm_data;
    };


    /*** Die Thread-Kommunikationsstruktur ***/
    struct SRThread {

        /*** Allgemeine daten ***/
        struct windp_win_data *win_data;
        int             thread;
        unsigned long   threadid;

        /*** Daten zum Senden ***/
        unsigned long   send;           // bitte senden
        char           *send_buffer;    // der Sendbuffer
        unsigned long   send_size;      // wieviel ist zu senden
        char           *s_sender_id;    // das bin ich
        unsigned long   s_sender_kind;
        char           *s_receiver_id;  // an den geht es
        unsigned long   s_receiver_kind;
        unsigned long   guaranteed;

        /*** daten zum Empfangen ***/
        unsigned long   received;       // es ist was da
        char           *recv_data;      // Hilfspointer
        char           *recv_buffer;    // der Empfangsbuffer
        unsigned long   recv_size;      // wieviel kam rein?
        char           *r_sender_id;    // wer hat gesendet?
        unsigned long   r_sender_kind;
        char           *r_receiver_id;  // an wen ging es?
        unsigned long   r_receiver_kind;
        unsigned long   message_kind;
        
        /*** statistische Daten zur Untersuchung des Zustandes ***/
        unsigned long   num_send;       // derzeit in der Sendliste
        unsigned long   num_recv;       // derzeit in der EmpfListe
        unsigned long   max_recv;       // maximal kamen soviele gleichzeitig rein
        unsigned long   max_send;       // maximal habe ich soviele mit einem mal versendet
        unsigned long   tme_send;       // solange dauert zur Zeit ein Sendeaufruf
        unsigned long   max_tme_send;   // solangew dauerte der laengste sendeaufruf
        unsigned long   num_bug_send;   // Anzahl bisheriger Sendefehler
        
        /*** daten zum Beenden ***/
        unsigned long   quit;           // bitte thread beenden
    };

    /* -----------------------------------------------------------
    ** Nodes zum halten einer empfangenen Message. Es kommt der
    ** Windows-Header (WinNode), dann ein Flag, welches den "Lese-
    ** zustand" angibt (wird beim ersten Receive gestezt und beim
    ** naechsten Receive, wo die naechste Node angefordert wird,
    ** kann die als gesesen markierte freigegeben werden.
    ** Danach kommen die Daten.
    ** ---------------------------------------------------------*/
    struct message_node {

        struct WinNode node;
        int    read;
        int    message_kind;
        char   receiver_id[ STANDARD_NAMELEN ];
        char   sender_id[ STANDARD_NAMELEN ];
        unsigned long sender_kind;
        unsigned long receiver_kind;
        unsigned long recv_size;        // trotz des Namens senden und empf.
        unsigned long guaranteed;
        unsigned long data;
    };



#else

    /*** Die VFM-Spezifischen Daten ***/
    struct windp_vfm_data {

        ULONG dummy;    // weiler soonst spinnt...
        };

    /*** Die LID aus VFM-Sicht ***/
    struct windp_data {

        void *win_data;
        struct windp_vfm_data *vfm_data;
    };

#endif

/*** No Lobby Custom Properties anymore ***/

#endif  // von WINDP_CLASS
#endif  // von __NETWORK__




