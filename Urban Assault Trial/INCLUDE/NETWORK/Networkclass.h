#ifdef __NETWORK__

#ifndef NETWORK_NETWORKCLASS_H
#define NETWORK_NETWORKCLASS_H
/*
**  $Source: $
**  $Revision: 38.3 $
**  $Date: 1995/08/17 23:45:25 $
**  $Locker:  $
**  $Author:  $
**
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include <nucleus/nucleusclass.h>
#endif

#ifndef NETWORK_NETWORKFLAGS_H
#include <network/networkflags.h>
#endif

/*
** Die Netzwerk-Klasse ist da Frontend systemspeifischer Netzwerk-
** Realisierungen. Sie richtet sich im Design im wesentlichen nach
** DirectPlay.
**
** Ein Netzwerkobjekt verwaltet die Namen der Provider, Sessions,
** Gruppen und Spieler. Da dies sehr speziell von der jeweiligen
** Netzwerkrealisation abhängt (was ein deutsch für!), hat die
** Netzwerkklasse keine LID, sondern erst die darunterliegenden
** Klassen. Da weiterhin vfm nix mit den WINDOWS-spezifischen
** Typendeklarationen, die man zur DirectX-Programmierung benötigt,
** anfangen kann, muß eh das ganze Windows-Zeug von VFM separiert
** werden. Was die Netzwerkklasse nun macht, ist die Definition
** von Methoden, die wahrscheinlich alle erst auf der darunterliegenden
** Ebene realisiert werden.
**
** NEU: Das ganze gruppenhandling ist gestorben!
**
** Methoden:
**
**      OM_NEW
**      Erzeugt ein NetzwerkObjekt. Sowas sollte nur einmal im Spiel passieren.
**      Außerdem rufe ich hier auch noch ASKPROVIDERS auf, um das Array der
**      Provider gleich mal zu initialisieren.
**
**      NWM_ASKPROVIDERS
**      Fragt die Provider ab. Diese merken wir uns in einem Array, welches
**      aus dafür spezialisierten Strukturen besteht. Diese Daten sind
**      Windows-Box-spezifisch, stehen also in der win_data. Das Array
**      wird bei jedem Aufruf neu beschrieben.
**
**      NWM_GETPROVIDERNAME
**      Da die Daten zu den Providern inner Windows-Schachtel gekapselt
**      sind, müssen wir sie erfragen. Wir bekommen jedesmal einen namen
**      zurück, solange noch welche da sind. Wir übergeben die Nummer, die
**      wir extern hochzählen.
**
**      NWM_SETPROVIDER
**      Es wird ein Provider gesetzt. übergeben wird der Name, den man mit
**      GETPROVIDERNAME erfragen kann. War bereits ein provider eingestellt,
**      so veranlaßt die Routine ein Aufräumen. War die Sache erfolgreich,
**      so wird TRUE zurückgegeben.
**
**      NWM_GETPROVIDER
**      War ein provider eingestellt, also richtig ausgewählt, so
**      wird hier sein Name zurückgegeben.
**
**      NWM_ASKSESSIONS
**
**      NWM_GETSESSIONNAME
**
**      NWM_JOINSESSION
**
**      NWM_OPENSESSION
**
**      NWM_GETSESSION
**
**      NWM_CLOSESESSION
**
**      NWM_CREATEPLAYER
**      Erzeugt einen Player in unserer internen Liste. Dabei gibt es 2
**      Modi: Einmal können wir selbst einen Player erzeugen, andererseits
**      kann es notwendig sein, ihn lediglich bei uns anzumelden. Grund
**      für die etwas umständliche Sache ist die tatsache, daß ich alle
**      Player für den Datenaustausch kennen muß, ich aber nur die erzeuge,
**      die auch auf meiner Maschine laufen.
**
**      NWM_DESTROYPLAYER
**      Löscht einen Player mit samt seinen Informationen aus meinem Array
**      und gibt ihn, sofern er auf meiner Maschine erzeugt wurde, frei.
**
**      NWM_ENUMPLAYERS
**      Fragt alle Player ab. Das sollte nur einmal am Anfang nach dem entern
**      einer Session gemacht werden. Alle weiteren Player werden mittels
**      CREATEPLAYER eingetragen!
**      Die Arbeitsreihenfolge könnte so sein: ENUMPLAYERS, CREATEPLAYER
**      (eigener), Systemmessages abfragen, Message erhalten, die mir
**      sagt, daß ein Player erzeugt wurde, Den dann bei mir merken!
**      NEU: EnumPlayers und Enumgroups werden jetzt bei OPEN/JOINSESSION
**      aufgerufen!
**
**      NWM_GETPLAYERDATA
**      Gibt alle daten zu diesem Player zurück. Sind die daten verfügbar,
**      so kommt TRUE zurück. Man kann die Daten nach Nummer (für schnelles
**      Abfragen aller) als auch über den Namen (für gezieltes Suchen) er-
**      mitteln.
**
**      NWM_CHECKREMOTESTART
**      Testet, ob die Informationen zum Start einer Netzsession
**      anderweitig übergeben werden. Das ist zum Beispiel beim
**      DPlay-Lobby-Konzept so. Wenn so etwas festgestellt wurde, so etabliert
**      die Routine die Verbindung und gibt remote = TRUE zurück. In diesem Falle
**      wird die remotestart_msg ausgefüllt. Andernfalls dürfen die
**      Werte in der Struktur nicht ausgewertet werden.
**      Geht etwas bei der Abfrage schief, liefert die Methode FALSE zurück.
**
**      NWM_ASKLOCALMACHINE
**      Wenn gueltige Netzwerkeinstellungen fuer die Maschine gefunden wurden,
**      so kommt die Routine mit TRUE zurueck und in der Struktur stehen
**      gueltige Werte.
**
**      NWM_GETPROVIDERTYPE
**      Gives information about the selected provider or connection. See flags
**      NWFC_...  in networkflags.h  
**
**      NWM_DIAGNOSIS
**      Gibt Infos ueber den derzeitigen Stand der Netzwerkarbeit. Siehe dazu
**      Message.
**
*/



#define NETWORK_CLASSID "network.class"

/*** Methods ***/
#define NWM_BASE                (OM_BASE + METHOD_DISTANCE)
#define NWM_ASKPROVIDERS        (NWM_BASE)
#define NWM_GETPROVIDERNAME     (NWM_BASE + 1)
#define NWM_SETPROVIDER         (NWM_BASE + 2)
#define NWM_GETPROVIDER         (NWM_BASE + 3)

#define NWM_ASKSESSIONS         (NWM_BASE + 4)
#define NWM_GETSESSIONNAME      (NWM_BASE + 5)
#define NWM_JOINSESSION         (NWM_BASE + 6)
#define NWM_OPENSESSION         (NWM_BASE + 7)
#define NWM_GETSESSION          (NWM_BASE + 8)
#define NWM_CLOSESESSION        (NWM_BASE + 9)
#define NWM_GETSESSIONSTATUS    (NWM_BASE + 10)
#define NWM_SETSESSIONNAME      (NWM_BASE + 11)

#define NWM_CREATEPLAYER        (NWM_BASE + 12)
#define NWM_DESTROYPLAYER       (NWM_BASE + 13)
#define NWM_ENUMPLAYERS         (NWM_BASE + 14)
#define NWM_GETPLAYERDATA       (NWM_BASE + 15)

#define NWM_SENDMESSAGE         (NWM_BASE + 16)
#define NWM_RECEIVEMESSAGE      (NWM_BASE + 17)
#define NWM_FLUSHBUFFER         (NWM_BASE + 18)

#define NWM_GETCAPSINFO         (NWM_BASE + 19)
#define NWM_LOCKSESSION         (NWM_BASE + 20)
#define NWM_RESET               (NWM_BASE + 21)
#define NWM_GETNUMPLAYERS       (NWM_BASE + 22)

#define NWM_CHECKREMOTESTART    (NWM_BASE + 23)

#define NWM_ASKLOCALMACHINE     (NWM_BASE + 24)
#define NWM_SETSESSIONIDENT     (NWM_BASE + 25)
#define NWM_GETPROVIDERTYPE     (NWM_BASE + 26)

#define NWM_DIAGNOSIS           (NWM_BASE + 27)


/*** Attribute ***/
#define NWA_BASE                (OMA_BASE + ATTRIB_DISTANCE)
#define NWA_NumPlayers          (NWA_BASE)
#define NWA_GuaranteedMode      (NWA_BASE+1)


/*** Vereinbarungen, einige sind vielleicht systemübergreifend interresant ***/
#define STANDARD_NAMELEN        (64)


/*** Das Button/Eingabe-Feld, also die Local Instance Data dieser Klasse ***/
struct network_data {

  ULONG dummy;
};

/*** -------------------------- Zu den Providern -------------------- ***/
struct getprovidername_msg {

    char *name;
    int  number;
};


struct setprovider_msg {

    char *name;
};


struct getprovider_msg {

    char *name;
};


/*** --------------------------- Zu den Sessions ---------------------- ***/
struct getsessionname_msg {

    char *name;
    int  number;
};


struct joinsession_msg {

    char *name;
};


struct opensession_msg {

    char *name;
    unsigned long maxplayers;
};


struct getsession_msg {

    char *name;
    ULONG status;
};

struct setsessionname_msg {

    char *name;
};    


/*** ------------------------- Zu den Playern -------------------------- ***/

/* ------------------------------------------------------------------------
** globale Player-Flags. Überall, wo Playereigenschaften mittels
** Flags beschrieben werden, sollten diese Flags benutzt werden.
** D.h beim Erzeugen, wie beim Merken so auch bei der Informationsrückgabe.
** ----------------------------------------------------------------------*/
#define NWFP_OWNPLAYER         (1L<<0)     // Player existiert auf meiner M.


struct createplayer_msg {

    ULONG   flags;
    char   *name;
    ULONG   data1;              // für Daten, die von außen kommen
    ULONG   data2;
};


struct destroyplayer_msg {

    char *name;
};


struct getplayerdata_msg {

    ULONG   askmode;
    ULONG   number;
    char    *name;           //
    ULONG   flags;
    ULONG   data1;
    ULONG   data2;
};

/*** ------------------------- Zu den Messages ----------------------------***/

struct sendmessage_msg {

    void   *data;           // zu verschickende Daten
    ULONG   data_size;      // Größe des extern allozierten datenblocks
    char   *sender_id;      // Name des Absenders (zur Identifikation)
    ULONG   sender_kind;    // Was ist der Sender? s.Flags
    char   *receiver_id;    // Adresse des Empfängers
    ULONG   receiver_kind;  // Was ist der Empfänger? s.Flags
    ULONG   guaranteed;     // versuche, garaniert zu verschicken
};


struct receivemessage_msg {

    void *data;             // Pointer auf den Datenblock
    ULONG size;
    char *receiver_id;      // Pointer auf einen String zur Ident. des Empf.
    ULONG receiver_kind;    // Art des Empf. bei SM_TOALL und SM_UNKNOWN ist
                            // receiver_id == NULL
    char *sender_id;        // Name des Senders, kann NULL sein
    ULONG sender_kind;      // Art des Senders
    ULONG message_kind;     // Wie soll das interprtiert werden? siehe flags
};


struct flushbuffer_msg {

    char   *sender_id;      // Name des Absenders (zur Identifikation)
    ULONG   sender_kind;    // Was ist der Sender? s.Flags
    char   *receiver_id;    // Adresse des Empfängers
    ULONG   receiver_kind;  // Was ist der Empfänger? s.Flags
    ULONG   guaranteed;     // versuche, garaniert zu verschicken
};


/*** --------------------------- Diverses ------------------------------ ***/
struct getcapsinfo_msg {

    ULONG   baud;
    ULONG   maxbuffersize;
};


struct locksession_msg {

    ULONG   block;          // 0: aktuelle Session wieder joinable, 1 sperren
};


struct localmachine_msg {

    char address[ 4 ];              // die adresse, direkt aus der hosten-Struktur
    char name[ STANDARD_NAMELEN ];  // der name des Rechners
    char address_string[ STANDARD_NAMELEN ];    // Die Adresse als String aufbereitet.    
};


struct sessionident_msg {

    char *check_item;
};    

    

/*** ---------------------------- RemoteControl-Zeug -------------------- ***/
struct checkremotestart_msg {               // reduziert, ohne CustomProps

    char    name[ STANDARD_NAMELEN ];       // Name des Spielers
    UBYTE   is_host;                        // ist er Host?
    UBYTE   remote;                         // ja, ist ferngesteuert
};


struct diagnosis_msg {

    ULONG   num_send;           // Anzahl der Messages in der Sendqueue
    ULONG   num_recv;           // Anzahl der Messages in der Recvqueue
    ULONG   max_recv;           // maximal sind soviele Messages auf einmal reingekommen
    ULONG   max_send;           // soviele sind maximal mit einem Mal versendet worden.
    ULONG   tme_send;           // solange dauerte der Sendeaufruf
    ULONG   max_tme_send;       // solange dauerte der laengste Sendeaufruf
    ULONG   num_bug_send;       // Anzahl bisheriger Sendefehler
};
    
#endif  // von NETWORK_CLASS
#endif  // von __NETWORK__




