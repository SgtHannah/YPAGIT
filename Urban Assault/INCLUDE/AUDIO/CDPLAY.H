#ifndef AUDIO_CDPLAY_H
#define AUDIO_CDPLAY_H
/*
**  $Source: PRG:VFM/Include/audio/cdplay.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 12:50:38 $
**  $Locker:  $
**  $Author: floh $
**
**  
**  (C) Copyright 1997 by A.Flemming
*/


/*** Die daten zum abspielen von CD's ***/
struct cd_data {

    unsigned long   timestamp;          // die globale zeit;
    unsigned long   on;                 // wirklich eingeschaltet?

    /*** Abspielen ***/
    unsigned long   track;              // Nummer des CD-tracks
    unsigned long   starts;             // start und ende eines CD-Tracks
    unsigned long   ends;
    unsigned long   begins;             // Beginn des Abspielens bzgl. timestamp

    /*** Verzoegerung ***/
    unsigned long   wait;               // ich warte zwischen 2 Tracks
    unsigned long   waitbegins;         // hier startete das Warten
    unsigned long   delay;              // Dauer des Wartens
    unsigned long   min_delay;          // min und max der Verzeogerung
    unsigned long   max_delay;          // beim Wiederholen
};


struct snd_cdcontrol_msg {

    unsigned long   command;
    unsigned long   para;
    unsigned long   min_delay;      // Pausen, bei SETTITLE
    unsigned long   max_delay;      // einstellen!
};

struct snd_cdtrigger_msg {

    unsigned long global_time;
    unsigned long playitagainsam;   // Titel nach Ablauf erneut starten
};

struct snd_cdvolume_msg {

    unsigned long volume;
};

/*** CD-Kommandos ***/
#define SND_CD_STOP         (1)     // Stop-taste
#define SND_CD_PLAY         (2)     // Play-taste
#define SND_CD_RESUME       (3)     // Pause raus
#define SND_CD_PAUSE        (4)     // Pause drücken
#define SND_CD_NEXT         (5)     // näxter Titel, sofern es den gibt
#define SND_CD_PREV         (6)     // vorheriger Titel, sofern es den gibt
#define SND_CD_SETTITLE     (7)     // dieser Titel (nr in para), Nummer
                                    // beginnt bei 1 (sofern es den gibt)
#define SND_CD_SWITCH       (8)     // schaltet CDPlayer ein oder aus (para==0)

/*** Prototypes ***/
unsigned long audio_InitCDPlayer( void );
void          audio_FreeCDPlayer( void );
unsigned long audio_ControlCDPlayer( struct snd_cdcontrol_msg *);
unsigned long audio_TriggerCDPlayer( struct snd_cdtrigger_msg *);
unsigned long audio_SetCDVolume( struct snd_cdvolume_msg *);

/*-----------------------------------------------------------------*/
#endif

