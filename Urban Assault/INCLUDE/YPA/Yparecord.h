#ifndef YPA_YPARECORD_H
#define YPA_YPARECORD_H
/*
**  $Source: PRG:VFM/Include/ypa/yparecord.h,v $
**  $Revision: 38.3 $
**  $Date: 1997/01/20 22:54:00 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für den Scene-Recorder/Cutter in YPA.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef IFF_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

/*-------------------------------------------------------------------
**  Background
**  ~~~~~~~~~~
**  Eine Sequenz-Beschreibung ist aufgebaut aus einem
**  Sequenz-Header mit Informationen über die folgende
**  Sequenz. In die Sequenz eingebettet ein Frame-
**  Header mit Informationen für je einen Frame.
**  Im Frame eingebettet sind "Event-Streams". Das sind
**  jeweils für ein definiertes Event eine Anzahl
**  von Parametern für diesen Typ von Event.
**
**  Das Sequence-Description-Fileformat ist ein
**  eigenes IFF-Format.
*/

/*** Event-Definitionen ***/
#define RCEVENT_None        (0)
#define RCEVENT_Sequence    (1)
#define RCEVENT_Frame       (2)
#define RCEVENT_ObjectInfo  (3)     // kontinuierliche Object-Info
#define RCEVENT_AudioInfo   (4)     // Object-Informationen für Audio
#define RCEVENT_ModEnergy   (5)     // ein ModSectorEnergy-Event
#define RCEVENT_Build       (6)     // ein Sektor-Build-Event

/*** Support-Strukturen ***/
struct rc_Sequence {
    WORD id;       // Sequence-Nummer
    WORD level;    // Level-ID, in dem die Sequence spielt
};

struct rc_Frame {
    LONG id;           // aktuelle Frame-Nummer
    LONG time_stamp;   // Timestamp dieses Frames
    LONG viewer;       // Object-Index des Viewers in diesem Frame
};

struct rc_ObjectInfo {
    ULONG id;               // <bact->ident> Wert
    struct flt_triple p;    // 3*4 Bytes Position im 3D-Raum
    BYTE roll,pitch,yaw;    // Orientation in Euler-Angles (radians)
    UBYTE state;            // aktueller visueller Zustand, siehe unten
    UBYTE type;             // Vehicle oder Weapon
    UBYTE proto;            // Vehicle- oder Weapon-Proto-Nummer
};
#define RCSTATE_None        (0)     // nicht definiert
#define RCSTATE_Normal      (1)     // visuell...
#define RCSTATE_Fire        (2)     // visuell...
#define RCSTATE_Wait        (3)     // visuell...
#define RCSTATE_Dead        (4)     // visuell...
#define RCSTATE_MegaDeth    (5)     // visuell...
#define RCSTATE_Create      (6)     // visuell...

#define RCTYPE_None     (0)
#define RCTYPE_Missy    (1)     // aka Weapon
#define RCTYPE_Vehicle  (2)

struct rc_AudioInfo {
    UWORD active;               // 16 Bits für aktive SoundSources
    WORD norm_pitch;            // Pitch des Motorengeräuschs
};

struct rc_ModEnergy {
    struct flt_triple p;    // Punkt der Energie-Modifikation
    LONG energy;            // Stärke der Energie-Modifikation
};

/*-------------------------------------------------------------------
**  Integritäts-Problem
**  ~~~~~~~~~~~~~~~~~~~
**  Folgendes Problem: Die Objekte in der Geschwaderhierarchie
**  behalten bekanntermaßen ihre Position in den Listen
**  nicht bei, sondern verschiedene Ereignisse bewirken
**  ein Umschichten innerhalb des Geschwader-Trees. Wenn
**  ich Sachen mitschneide, bin ich aber auf eine
**  statische Reihenfolge der Objekte von Frame zu Frame
**  angewiesen (das macht das Abspielen und Bearbeiten effizienter).
**  Lösung:
**      Innerhalb jedes Frames wird die Geschwaderstruktur
**      durchgegangen, und die Daten in der Reihenfolge
**      der aktuellen Geschwaderstruktur mitgeschnitten.
**      Wenn das erledigt ist, wird der ObjectInfo-Puffer
**      nach dem <id> Wert sortiert. Fertig.
*/

/*-------------------------------------------------------------------
**  Globale Struktur, hält eine Sequenz-Beschreibung komplett.
*/
struct YPASeqData {

    /*** Recorder & Player ***/
    struct IFFHandle *iff;
    struct rc_Sequence seq;     // Parameter der aktuellen Sequenz
    struct rc_Frame frame;      // Parameter des aktuellen Frames

    struct Bacterium     **buf_store;
    struct rc_ObjectInfo *buf_objinf;
    struct rc_AudioInfo  *buf_audinf;
    struct rc_ModEnergy  *buf_modeng;
    UBYTE *buf_audcod;

    ULONG max_store;
    ULONG max_modeng;

    ULONG act_store;
    ULONG act_modeng;
    ULONG act_audcod;

    /*** Recorder-only ***/
    ULONG active;               // TRUE -> aktiviert, sonst inaktiv
    LONG  delay;                // Timer zwischen zwei Frame-Snaps

    /*** Player-only ***/
    struct flt_triple cam_pos;  // Position der virtuellen Kamera
    struct flt_m3x3 cam_mx;     // Matrix der virtuellen Kamera
    ULONG num_frames;           // Player only: NumFrames
    ULONG flags;                // siehe unten
    ULONG player_mode;          // nur PLAYER_STOP, PLAY, RECORD!
    ULONG camera_mode;          // CAMERA_#?
    ULONG camera_link;          // Bact-ID, an die Kamera gelinkt ist
    UBYTE fname[64];            // Player only: Filename
};

#define RC_DELAY_TIME   (250)       // alle 250 millisec ein Snap

#define RCF_Flush   (1<<0)      // ein Flush-Frame (keine Interpolation)

/*-------------------------------------------------------------------
**  IFF Definitionen
*/

#define RCIFF_SeqHeader     MAKE_ID('S','E','Q','N')    // Form
#define RCIFF_SeqInfo       MAKE_ID('S','I','N','F')    // Chunk
#define RCIFF_FrameHeader   MAKE_ID('F','R','A','M')    // Form
#define RCIFF_FrameInfo     MAKE_ID('F','I','N','F')    // Chunk
#define RCIFF_ObjectInfo    MAKE_ID('O','I','N','F')    // Chunk
#define RCIFF_AudioInfo     MAKE_ID('A','I','N','F')    // Chunk
#define RCIFF_ModEnergy     MAKE_ID('M','O','D','E')    // Chunk
#define RCIFF_Build         MAKE_ID('B','U','L','D')    // Chunk
#define RCIFF_Flush         MAKE_ID('F','L','S','H')    // Chunk (immer empty)

/*
**  Chunks:
**  ~~~~~~~
**  RCIFF_SeqInfo       == <struct rc_Sequence>
**  RCIFF_FrameInfo     == <struct rc_Frame>
**  RCIFF_AddObject     == <struct rc_AddObject[]>
**  RCIFF_ObjectInfo    == <struct rciff_ObjectInfo[]> (!!!)
**  RCIFF_AudioInfo     == <struct rc_AudioInfo[]> (Runlength-Encoded!)
**  RCIFF_ModEnergy     == <struct rc_ModEnergy[]>
*/
/*-----------------------------------------------------------------*/
#endif
