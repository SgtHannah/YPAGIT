#ifdef __NETWORK__

#ifndef NETWORK_NETWORKFLAGS_H
#define NETWORK_NETWORKFLAGS_H

/* -----------------------------------------------------------------
** Weil ich die networkclass.h in der WinBox nicht habe, aber dort
** auch die gesamten Deklarationen benötige, schaufle ich sie in ein
** Extra-Include, welches systemunabhängig ist.
** ---------------------------------------------------------------*/
/*** Vereinbarungen ***/
#define STANDARD_NAMELEN        (64)

#define MAXNUM_PROVIDERS        (64)
#define PROVIDER_NAMELEN        STANDARD_NAMELEN
#define MAXNUM_SESSIONS         (64)
#define SESSION_NAMELEN         (STANDARD_NAMELEN * 2)      // wegen den vielen Zusatzinfos
#define MAXNUM_PLAYERS          (4)
#define PLAYER_NAMELEN          STANDARD_NAMELEN


/* ------------------------------------------------------------------------
** globale Player-Flags. Überall, wo Playereigenschaften mittels
** Flags beschrieben werden, sollten diese Flags benutzt werden.
** D.h beim Erzeugen, wie beim Merken so auch bei der Informationsrückgabe.
** ----------------------------------------------------------------------*/
#define NWFP_OWNPLAYER         (1L<<0)     // Player existiert auf meiner M.

/*** Flags für getPlayerData ***/
#define GPD_ASKNUMBER       0
#define GPD_ASKNAME         1

/*** Adressearten für SendData ***/
#define MSG_UNKNOWN         0
#define MSG_PLAYER          1
#define MSG_ALL             2

/*** Session-Status-Flags für GETSESSION ***/
#define SESSION_NONE        0
#define SESSION_CREATED     1
#define SESSION_JOINED      2

/*** Messagearten für ReceiveMessage ***/
#define MK_NORMAL           0       // data enthält kundenspezifischen Block
#define MK_CREATEPLAYER     1       // Es kam ein CP rein, in data steht der name
#define MK_DESTROYPLAYER    2       // es kam die Abmeldung eines Players
#define MK_HOST             3       // derjenige wird Host
#define MK_SYSUNKNOWN       4       // Systemmessage, die uns aber nicht int.
#define MK_SETSESSIONDESC   5       // nvfdkjvkjhid iuddvkjfdvg ikjfd

/*** Flags für guaranteed_mode ***/
#define NWGM_Normal         0       // wie bisher, die Aussage in der Message ent.
#define NWGM_Guaranteed     1       // immer garantiert
#define NWGM_NotGuaranteed  2       // nie garantiert

// the connection types
#define NWFC_NONE           0   // nothing selected yet
#define NWFC_TCPIP          1   // the only one
#define NWFC_IPX            2   // is dead
#define NWFC_SERIAL         3   // may be it works
#define NWFC_MODEM          4   // very slow...
#define NWFC_LOBBY          5

// die sessionstatus flags
#define WSS_SESSION_NONE    (0) // nix
#define WSS_SESSION_JOINED  (1) // bin einer angeschlossen
#define WSS_SESSION_CREATED (2) // habe selbst was eroeffnet


#endif  // von NETWORK_FLAGS
#endif  // von __NETWORK__




