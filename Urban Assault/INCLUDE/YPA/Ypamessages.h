#ifdef __NETWORK__

#ifndef YPA_MESSAGES_H
#define YPA_MESSAGES_H
/*
**  $Source: PRG:VFM/Include/ypa/ypamessages.h,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/14 16:53:31 $
**  $Locker:  $
**  $Author: floh $
**
**  Die Messages, die über das Netz geschossen werden
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

#ifndef NETWORK_NETWORKCLASS_H
#include "network/networkclass.h"
#endif

#ifndef YPA_YPAROBOCLASS_H
#include "ypa/yparoboclass.h"
#endif

/*** Vereinbarungen ***/

// 2 Updates pro Message ist Verschwendung....
#define UPDATE_TIME_NORMAL  200         // nach dieser zeit gehen meine V. übers Netz
#define FLUSH_TIME_NORMAL   200         // Nach dieser Zeit wird Buffer gefl.
#define UPDATE_TIME_REDUCED 400         // fuer geringe Bandbreite
#define FLUSH_TIME_REDUCED  400
#define UPDATE_TIME_TROUBLE 1400        // fuer problemfaelle
#define FLUSH_TIME_TROUBLE  1500

#define BAUD_FAST           60000       // ab da ist es schnell


/*** Allgemein zum ermitteln... ***/
#define YPAM_BASE           (1000)           // für Messagekind
struct ypamessage_generic {

    ULONG   message_id;
    ULONG   timestamp;                      // Wann losgeschickt
    ULONG   msgcount;                       // Zur Kontrolle ob was fehlt
    UBYTE   owner;
    UBYTE   p[3];
};

#define YPAM_LOADGAME       YPAM_BASE        // Spiel wird gestartet
struct ypamessage_loadgame {

    struct ypamessage_generic generic;

    //++++++++++++++++
    ULONG   level;          // die levelnummer
};

#define YPAM_NEWVEHICLE     (YPAM_BASE+1)
struct ypamessage_newvehicle {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++
    struct flt_triple   pos;
    ULONG               master;     // id des Chefs;
    ULONG               ident;      // meine ID
    ULONG               command_id;
    UBYTE               vkind;      // was bin ich? Das erleichtert das einordnen
    UBYTE               type;       // vehicleproto-id
    UBYTE               p[2];
};

#define NV_ROBO         0   // sinnlos, nur der Vollständigkeit halber
#define NV_COMMANDER    1   // ich werde Chef
#define NV_SLAVE        2   // ich werde Untergebener


#define YPAM_DESTROYVEHICLE     (YPAM_BASE+2)
struct ypamessage_destroyvehicle {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++
    ULONG   ident;              // wen trifft es?
    UBYTE   class;              // die Klasse zur Unterscheidung Waffe/vehicle
    UBYTE   p[3];
};


#define YPAM_NEWCHIEF           (YPAM_BASE+3)
struct ypamessage_newchief {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++
    ULONG   ident;              // wen trifft es?
    ULONG   new_master;         // wer bekommt meine Slaves?
};


#define YPAM_NEWWEAPON          (YPAM_BASE+4)
struct ypamessage_newweapon {           // raketen werden nun normal getriggert
                                        // dazu benoetigen sie auch ein paar
    struct ypamessage_generic generic;  // mehr informationen.

    //++++++++++++++++++++++++++
    struct flt_triple   pos;        // Position der Waffe
    ULONG  ident;                   // ident der Waffe
    ULONG  rifleman;                // Schütze
    struct flt_triple dir;          // Ausrichtung des Einschusses als geschw.
    ULONG  target;                  // ident des targets;
    UBYTE  flags;
    UBYTE  type;                    // Art der Waffe (also TypeID)
    UBYTE  target_type;             // Art des Zieles der Rakete
    UBYTE  target_owner;            // eigentuemer des Zieles
    struct flt_triple target_pos;   // Position des Zieles
};

#define NW_GROUND       (1<<0)      // am Boden
#define NW_SHOT         (1<<1)      // Einschuß allgemein


#define YPAM_SETSTATE           (YPAM_BASE+5)
struct ypamessage_setstate {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++
    ULONG   ident;
    ULONG   extra_on;
    ULONG   extra_off;
    UBYTE   main_state;
    UBYTE   class;
    UBYTE   p[2];
};


// ++++++++++++++++++++++++++ Vehicle-Datenübertragung +++++++++++++++++++++++
#define MAXNUM_TRANSFEREDVEHICLES   (1024)
struct vehicledata {

    //struct flt_triple pos;          // Position
    WORD   pos_x;                   // durch 2 geteilte position (sonst
    WORD   pos_y;                   // passt die nicht rein!)
    WORD   pos_z;
    WORD   p;
    UBYTE  roll;                    // MatrixVerschlüsselung
    UBYTE  pitch;
    UBYTE  yaw;
    UBYTE  specialinfo;

    ULONG  ident;
    LONG   energy;
};

// Vehicledata ist die minimale variante fuer die
// interpolation. bei der Extrapolation kommt noch die
// geschwindigkeit hinzu.
struct vehicledata_i {

    struct vehicledata i;
};

struct vehicledata_e {

    struct vehicledata e;
    struct flt_triple speed;
};

#define SIF_DONTRENDER          (1<<0)  // EXTRA_DONTRENDER muß übers Netz
#define SIF_LANDED              (1<<1)  // Gegenstück zu Extra_Landed
#define SIF_YPAGUN              (1<<2)  // ist ne gun....
#define SIF_YPAMISSY            (1<<3)  // missile, ne
#define SIF_YPAROBO             (1<<4)  // das n robo
#define SIF_SETDIRECTLY         (1<<5)  // mache keine inter-/extrapolation

struct vehicledata_header {

    UWORD   number;                 // Anzahl der enthaltenen vehicle
    UWORD   size;
    ULONG   diff_sendtime;          // zeit zwischen 2 M. von Senderseite aus!
};


#define YPAM_VEHICLEDATA_I        (YPAM_BASE+6)
struct ypamessage_vehicledata_i {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    struct vehicledata_header head;
    struct vehicledata_i block[ MAXNUM_TRANSFEREDVEHICLES ];
};

#define YPAM_VEHICLEDATA_E        (YPAM_BASE+7)
struct ypamessage_vehicledata_e {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    struct vehicledata_header head;
    struct vehicledata_e block[ MAXNUM_TRANSFEREDVEHICLES ];
};


#define YPAM_ORGANIZE           (YPAM_BASE+8)
struct ypamessage_organize {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++
    ULONG   ident;
    ULONG   master;
    ULONG   slave;
    ULONG   command_id;
    WORD    modus;          // entscheidet, wie obiges interpretiert wird
                            // verwendet defines der ORGANIZE-Methode
    UBYTE   p[2];
};


#define YPAM_DIE                (YPAM_BASE+9)    // logisches Sterben --> YBM_DIE
struct ypamessage_die {

    struct ypamessage_generic generic;

    //+++++++++++++++++++++++++++++++++++
    ULONG   ident;              // wen trifft es?
    ULONG   new_master;         // wer bekommt meine Slaves?
    UBYTE   landed;             // 1, wenn megadeth eingeschalten werden soll
    UBYTE   class;              // welcher Art ist der Sterbende
    UBYTE   killerowner;        // vorerst brauchen wir den gesamten Killer nicht
    UBYTE   p;
};


#define YPAM_VEHICLEENERGY      (YPAM_BASE+10)   // addiert Energie auf !
struct ypamessage_vehicleenergy {               // für Red. negative Werte nehmen

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++
    ULONG   ident;              // wen trifft es
    LONG    energy;             // mit wieviel
    LONG    killer;             // wer reduzierte denn da?
    UBYTE   killerowner;
    UBYTE   p[3];
};


#define YPAM_SECTORENERGY       (YPAM_BASE+11)
struct ypamessage_sectorenergy {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++
    struct flt_triple pos;
    LONG   energy;
    UBYTE  sectorowner;         // mse kann auf unt. maschinen zu unterschiedlichen
                                // Resultaten kommen. Deshalb gebe ich den Owner mit
                                // rüber, auch wenn das nicht immer hilft
    UBYTE  p[3];
};


#define YPAM_STARTBUILD         (YPAM_BASE+12)  // startet einen Bauauftrag
struct ypamessage_startbuild {                  // für mich auf den anderen
                                                // mich
    struct ypamessage_generic generic;

    //+++++++++++++++++++++++++++++++++++
    LONG   bproto;                              // was bauen?
    WORD   sec_x;                               // wo  bauen?
    WORD   sec_y;
};


struct bVhcl {

    ULONG   ident;
    struct  flt_triple basis;                   // wohin zeigt es?
    struct  flt_triple pos;                     // wo ist es?
    WORD    vproto;                             // was ist es?
    UBYTE   p[2];
};


#define YPAM_BUILDINGVEHICLE    (YPAM_BASE+13)  // Beim bauen können stationäre
struct ypamessage_buildingvehicle {             // Vehicle erzeugt werden. Die
                                                // dürfen aber nur vom OrgRobo
    struct ypamessage_generic generic;          // erzeugt werden und müssen dann
                                                // gespiegelt werden. Das Bauen
    //+++++++++++++++++++++++++++++++++++       // selbst kann jeder für sich
    struct bVhcl vehicle[ 8 ];                  // machen. Deshalb 2 Messages
};


#define YPAM_VIEWER             (YPAM_BASE+14)
struct ypamessage_viewer {

    struct ypamessage_generic generic;

    //+++++++++++++++++++++++++++++++++++
    ULONG       ident;
    ULONG       rifleman;                       // für Raketen
    UBYTE       class;
    UBYTE       viewer;                         // 1 ein, 0 aus
    UBYTE       p[2];
};


#define YPAM_SYNCGAME           (YPAM_BASE+15)  // Sorgt am Spielanfang für einen
struct ypamessage_syncgame {                    // Datenaustausch. Sagt also:
                                                // Ich bin mit Laden fertig und
    struct ypamessage_generic generic;          // mein Robo hat diesen Ident.
                                                // Der Host kann nun den Start
    //+++++++++++++++++++++++++++++++++++       // solange verzögern, bis von
    ULONG       robo_ident;                     // allen eine solche Message kam
    ULONG       gun[ NUMBER_OF_GUNS ];          // IDs der Bordbewaffnung
};


#define YPAM_ROBODIE            (YPAM_BASE+16)
struct ypamessage_robodie {                     // Extra-Sterbe-Message für
                                                // den Robo. Wir brauchen ja
    struct ypamessage_generic generic;          // bloß den Eigentümer

    //+++++++++++++++++++++++++++++++++++
    LONG        killer;
    UBYTE       killerowner;
    UBYTE       p[3];
};


#define MAXNUM_DEBUGMSGITEMS    MAXNUM_TRANSFEREDVEHICLES
#define YPAM_DEBUG              (YPAM_BASE+17)
struct ypamessage_debug {                       // Sendet die geschwaderstrukrur
                                                // zu einem Robo: ID_Robo, ID_Co.
                                                // ID_Slave, ID_Slave, DBGMSG_NEWCOMMANDER,
    struct ypamessage_generic generic;          // und irgendwann mal DBGMSG_END

    //+++++++++++++++++++++++++++++++++++
    ULONG data[ MAXNUM_DEBUGMSGITEMS ];
};

#define DBGMSG_END                      (0)
#define DBGMSG_NEWCOMMANDER             (1)

#define SI_UNKNOWN          (0)
#define SI_COMMANDER        (1)
#define SI_ROBO             (2)
#define SI_SLAVE            (3)


#define YPAM_TEXT               (YPAM_BASE+18)
struct ypamessage_text {

    struct ypamessage_generic generic;

    //+++++++++++++++++++++++++++++++++++
    char text[ STANDARD_NAMELEN ];              // so lang sind nun mal bei uns die Texte
};


#define YPAM_REMOVEPLAYER       (YPAM_BASE+19)
struct ypamessage_removeplayer {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++++
    char   name[ STANDARD_NAMELEN ];
};

#define YPAM_GEM                (YPAM_BASE+20)  // gem wurde von anderen
struct ypamessage_gem {                         // erobert. Somit bei
                                                // mir ausschalten
    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++++
    WORD   gemoffset;
    WORD   enable;                              // TRUE: bei anderen wieder
                                                // einschalten (normal), sonst
                                                // als Verlust kennzeichnen
};


#define YPAM_RACE               (YPAM_BASE+21)  // Rasse gewählt, folglich
struct ypamessage_race {                        // bei mir ein- und ausschalten

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    WORD   freerace;                            // diese rasse wird frei
    WORD   newrace;                             // seine neue --> ausschalten
};


#define YPAM_WELCOME            (YPAM_BASE+22)  // diese message bekommt der
struct ypamessage_welcome {                     // von den Spielern, die schon
                                                // erzeugt wurden. Ein Spieler
    struct ypamessage_generic generic;          // schickt diese los, wenn er
                                                // ein CREATEPLAYER empfängt
    //++++++++++++++++++++++++++++++++
    WORD   myrace;                              // hallo, hier sind rasse und
    UBYTE  ready_to_start;                      // startstatus 
    UBYTE  cd;                                  // ey, ich hab ne cd
};


#define YPAM_READYTOSTART       (YPAM_BASE+23)  // Bin ich fertig zum Start?
struct ypamessage_readytostart {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    UBYTE  ready_to_start;                      // 1, wenn es losgehen kann, o
    UBYTE  p[3];                                // mal noch warten...
};


#define YPAM_REQUESTUPDATE      (YPAM_BASE+24)  // Fordere ein Update, weil
struct ypamessage_requestupdate {               // bei mir irgendwas im Arsch ist
                                                // Ist sozusagen die Einleitung
    struct ypamessage_generic generic;          // einer Reparaturmaßnahme

    //++++++++++++++++++++++++++++++++
};


#define YPAM_UPDATE             (YPAM_BASE+25)
#define MAXNUM_UPDATEITEMS      MAXNUM_TRANSFEREDVEHICLES

struct vhclupd_entry {

    UBYTE  kind;
    UBYTE  mainstate;
    UBYTE  vproto;
    UBYTE  roll;
    UBYTE  pitch;
    UBYTE  yaw;
    UBYTE  p[2];

    ULONG  extrastate;
    ULONG  ident;
    LONG   energy;
    struct flt_triple pos;
};

#define UPD_END         0
#define UPD_ROBO        1
#define UPD_CMDR        2
#define UPD_SLAVE       3
#define UPD_MISSILE     4
#define UPD_RGUN        5

struct ypamessage_update_basic {

    ULONG  size;                // weil messagegroesse variabel ist
    ULONG  entries;
};


struct ypamessage_update {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    struct ypamessage_update_basic basic;
    struct vhclupd_entry data[ MAXNUM_UPDATEITEMS ];
};


#define YPAM_IMPULSE            (YPAM_BASE+26)
struct ypamessage_impulse {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    LONG                ident;          // Kennung der expl. Rakete

    struct flt_triple   pos;            // daten der rakete. Bezeichnung wie für
    LONG                impulse;        // impulse-Message.
    FLOAT               miss_mass;
    struct uvec         miss;
};

#define YPAM_LOGMSG             (YPAM_BASE+27)
struct ypamessage_logmsg {

    struct ypamessage_generic generic;  // manchmal können auf Schattenmaschinen
                                        // Ereignisse für meldungen nicht fest-
    //++++++++++++++++++++++++++++++++  // gestellt werden. Dann schicken wir
                                        // einfach eine Message
    LONG   sender;                      // Sender-ident und Owner des Senders
    UBYTE  senderowner;                 // zum ermitteln des Bact.
    UBYTE  p[3];

    LONG   pri;
    LONG   ID;
    LONG   para1;
    LONG   para2;
    LONG   para3;
};

#define YPAM_NEWORG             (YPAM_BASE+28)
#define MAXNUM_NEWORGSLAVES     (500)

struct ypamessage_neworg_basic {

    ULONG   modus;                          // für DebugZwecke der orgModus
    ULONG   commander;                      // du wirst neuer Commander
    ULONG   command_id;                     // du bekommst die ID
    ULONG   num_slaves;
    ULONG   size;                           // Messagesize, weil variabel
};

struct ypamessage_neworg {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++
    struct ypamessage_neworg_basic basic;

    ULONG   slaves[ MAXNUM_NEWORGSLAVES ];  // das werden deine Slaves
};          


#define YPAM_LOBBYINIT          (YPAM_BASE+29) // Sinn ist es, dass bei einer Lobbyinit    
struct ypamessage_lobbyinit {                  // Daten, die die Clients so nicht kriegen
                                               // koennen, public gemacht werden.
    struct ypamessage_generic generic;         // Fuer "READY" brauchen wir den Host
                                               // fuer das Starten die levelnummer. All
    // +++++++++++++++++++++++++++++++         // die Daten hat der Cliebnt nicht. Der
                                               // kriegt nicht mal nen Sessionnamen...
    char    hostname[ STANDARD_NAMELEN ];      // die rassen sind zur Zeit noch OptionalS
    ULONG   levelnum;
    char    races[4];
};            


#define YPAM_STARTPLASMA        (YPAM_BASE+30)
struct ypamessage_startplasma {

    struct ypamessage_generic generic;
    
    //+++++++++++++++++++++++++++++++++
    
    ULONG   time;                               // solange wirds dauern
    FLOAT   scale;                              // damit gehts los  
    ULONG   ident;
    struct  flt_triple pos;                     // weil die position des Schattens
    struct  flt_m3x3 dir;                       // zu diesem Zeitpunkt arg anders sein kann
};


#define YPAM_ENDPLASMA          (YPAM_BASE+31)
struct ypamessage_endplasma {

    struct ypamessage_generic generic;
    
    //++++++++++++++++++++++++++++++++++ 
    ULONG   ident;
};


#define YPAM_EXTRAVP            (YPAM_BASE+32)
struct ypamessage_extravp {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++
    ULONG   ident;                              // um wen gehts?
    ULONG   numvp;                              // wieviele sind zu beachten? ACHTUNG,
                                                // geht nacheinander weg!
    struct extra_vproto vp[ MAXNUM_EXTRAVP ];   // die Visprotoeintraege wie in der bactStruktur
                                                // vp wird mit  VPJUST... uebergeben und dann korrigiert
};


#define YPAM_STARTBEAM          (YPAM_BASE+33)
struct ypamessage_startbeam {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++++++++                                                                                                    
    struct flt_triple BeamPos;
    ULONG             ident;
};


#define YPAM_ENDBEAM            (YPAM_BASE+34)
struct ypamessage_endbeam {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++++++++                                                                                                    
    ULONG             ident;
};

#define YPAM_ANNOUNCEQUIT       (YPAM_BASE+35)
struct ypamessage_announcequit {

    struct ypamessage_generic generic;

    //++++++++++++++++++++++++++++++++++++++++                                                                                                    
};

#define YPAM_CHANGELEVEL        (YPAM_BASE+36) // Sinn ist es, dass bei einer Lobbyinit    
struct ypamessage_changelevel {               // Daten, die die Clients so nicht kriegen
                                               // koennen, public gemacht werden.
    struct ypamessage_generic generic;        // Fuer "READY" brauchen wir den Host
                                               // fuer das Starten die levelnummer. All
    // +++++++++++++++++++++++++++++++         // die Daten hat der Cliebnt nicht. Der
                                               // kriegt nicht mal nen Sessionnamen...
    char    hostname[ STANDARD_NAMELEN ];     // die rassen sind zur Zeit noch OptionalS
    ULONG   levelnum;
    char    races[4];
};            


#define YPAM_CHECKSUM           (YPAM_BASE+37) // liefert nach joinen oder levelaenderung 
struct ypamessage_checksum {                  // die Daten der Files an alle anderen.
                                               // auch nach welcome-message
    struct ypamessage_generic generic;        // 
                                               
    // +++++++++++++++++++++++++++++++         
                                               
    ULONG   checksum;
};            

#define YPAM_REQUESTLATENCY     (YPAM_BASE+38) // fordert eine Antwort auf eine Latency
struct ypamessage_requestlatency {            // anfrage
                                               // timestamp, als ich das losgeschickt habe
    struct ypamessage_generic generic;        // ist einziger Parameter
                                               // Owner sollte fuer beide Messages ausgefuellt
    // +++++++++++++++++++++++++++++++         // sein
                                               
    ULONG   time_stamp;
};            

#define YPAM_ANSWERLATENCY      (YPAM_BASE+39) // Antwort mit dem in request uebergebenen
struct ypamessage_answerlatency {             // timestamp
                                               // 
    struct ypamessage_generic generic;        // 
                                               
    // +++++++++++++++++++++++++++++++         
                                               
    ULONG   time_stamp;
};            


#define YPAM_STARTTROUBLE       (YPAM_BASE+40) // eine Problemloesung wurde eingeleitet
struct ypamessage_starttrouble {              // uebergeben wird der Grund, der auch in
                                               // gsr->network_trouble gemerkt wird
    struct ypamessage_generic generic;        // 
                                               
    // +++++++++++++++++++++++++++++++         
                                               
    ULONG   trouble;
};            

#define NETWORKTROUBLE_NONE         (0)     // is doch gar nuescht los
#define NETWORKTROUBLE_LATENCY      (1)     // Latency Probleme, Host entscheidet
#define NETWORKTROUBLE_HELPSTARTED  (2)     // ich Klops habe die Hilfe aufgemacht
#define NETWORKTROUBLE_KICKOFF_YOU  (3)     // du bsit rausgeschmissen worden
#define NETWORKTROUBLE_KICKOFF_PLAYER (4)   // ein anderer muss raus
#define NETWORKTROUBLE_WAITINGFORPLAYER (5)


#define YPAM_ENDTROUBLE         (YPAM_BASE+41) // Probleme haben aufgehoert
struct ypamessage_endtrouble {                // reason gibt Grund an
                                               // 
    struct ypamessage_generic generic;        // 
                                               
    // +++++++++++++++++++++++++++++++         
                                               
    ULONG   reason;
}; 

#define ENDTROUBLE_ALLOK        (1)     // itze gehts wieder
#define ENDTROUBLE_TIMEDOUT     (2)     // aufgegeben, keine Chance das in den griff zu kr. 
#define ENDTROUBLE_UNKNOWN      (3)     // kein spezieller Wert --> somit kein text          


#define YPAM_CD                 (YPAM_BASE+42)  // mein derzeitiger "CD-Bestitz-Status"
struct ypamessage_cd {                         // hat sich geaendert   
                                                
    struct ypamessage_generic generic;
    
    //+++++++++++++++++++++++++++++++++++++
    
    UBYTE   cd;                                 // 0 oder nicht 0, je nachdem
    UBYTE   p[3];
};
    
#endif
#endif

