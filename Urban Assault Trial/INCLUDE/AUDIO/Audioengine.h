#ifndef AUDIO_AUDIOENGINE_H
#define AUDIO_AUDIOENGINE_H
/*
**  $Source: PRG:VFM/Include/audio/audioengine.h,v $
**  $Revision: 38.8 $
**  $Date: 1998/01/06 12:50:27 $
**  $Locker:  $
**  $Author: floh $
**
**  Allgemeine Defs + Specs der Audio-Engine.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef AUDIO_SAMPLE_H
#include "audio/sample.h"
#endif

/*-------------------------------------------------------------------
**  Background
**  ~~~~~~~~~~
**
**  Die AudioEngine konzentriert sich ausschließlich auf die
**  Bereitstellung einer 3D-Audio-Umwelt mit den "Audio-Phänomenen"
**  entfernungsabhängige Lautstärke, Stereoposition und
**  Doppler-Effekt.
**
**  <struct SoundCarrier>
**  ---------------------
**  Die wichtigste Datenstruktur der AudioEngine ist die
**  SoundCarrier-Struktur. Ein SoundCarrier ist die
**  Repräsentation eines "3D-Objects" für die AudioEngine,
**  mit einer globalen 3D-Position, einem globalen Geschwindigkeits-
**  Vektor und bis zu 16 "SoundSources".
**
**  <struct SoundSource>
**  --------------------
**  Eine SoundSource-Struktur kapselt jeweils einen einzelnen
**  Sound-Effekt. Wie gesagt können an einem SoundCarrier bis
**  zu 16 unabhängige SoundSources hängen, die alle die Position
**  und Geschwindigkeit des übergeordneten SoundCarriers besitzen.
**
**  Aktiver/inaktiver Zustand
**  -------------------------
**  SoundSources besitzen (unabhängig voneinander) die beiden
**  Zustände "aktiv" und "inaktiv". Die AudioEngine wird
**  versuchen, alle aktiven SoundSources auch "hörbar" zu
**  machen, allerdings können nur die n höchstpriorisierten
**  SoundSources parallel abgespielt werden (weil ja nur
**  eine endliche Anzahl Audio-Kanäle gemixt werden können).
**
**  Lautstärke und Priorität
**  ------------------------
**  Die Priorität einer SoundSource ergibt sich aus ihrer
**  resultierenden Lautstärke. Die resultierende Lautstärke
**  ergibt sich wiederum aus Vorgabe-Lautstärke und Entfernung.
**
**  One-Shot-Sounds/Loop-Da-Loop-Sound
**  --------------------------------
**  Es gibt zwei grundlegende Arten von SoundSources, One-Shot
**  und Loop-Da-Loop.
**  One-Shot-Sounds sind "einmalige" Toneffekte, wie Explosionen,
**  die sich nach ihrer Aktivierung und ihrer Spieldauer
**  selbst terminieren.
**  Loop-Da-Loop-Sounds werden dagegen durch dauernde Wiederholung
**  solange abgespielt, bis sie explizit "von außen" beendet werden.
**  Typische Loop-Da-Loop-Sounds sind z.B. Motoren- oder Triebwerk-
**  Geräusche.
**
**  SoundCarrier Refreshing
**  -----------------------
**  Weil sich die SoundCarrier ja relativ zum Viewer bewegen, muß
**  die AudioEngine zyklisch über den Zustand eines SoundCarriers
**  auf dem laufenden gehalten werden. Das passiert normalerweise 
**  einmal pro Carrier und Frame mit der Routine _RefreshSoundCarrier().
**  Bei einem _RefreshSoundCarrier() sollte zumindest Position und
**  Geschwindigkeits-Vektor aktualisiert sein, optional kann
**  auch die Vorgabe-Lautstärke und Tonhöhe geändert werden.
**  Die AudioEngine paßt dann bei allen aktiven SoundSources des 
**  Carriers Tonhöhe, Lautstärke und Stereo-Position
**  an:
**      Position -> ergibt entfernungsabhängige Lautstärke-Verschiebung
**                  und Stereo-Position
**
**      Geschwindigkeits-Vektor -> ergibt Tonhöhen-Verschiebung (aka
**                  Doppler-Effekt).
**
**      resultierende Lautstärke -> ergibt die resultierende
**                  Priorität einer SoundSource.
**
**  Other Stuff
**  -----------
**  Weil SoundSources jederzeit unterbrochen/abgebrochen werden
**  können, muß die AudioEngine für jede SoundSource Timing-Information
**  mitschreiben, damit jederzeit Offsets in die Sound-Samples
**  berechnet werden können (es wäre ja blöd, wenn niederpriorisierte
**  Sounds, die durch höherpriorisierte unterbrochen werden, wieder von
**  vorn anfangen zu spielen, oder dort wo sie unterbrochen worden
**  sind, oder überhaupt nicht weitermachen... das ist so auch ziemlich 
**  das komplexeste Problem mit dem die Audio-Engine fertig werden muß).
**
**  Funktionen und Vorgehensweise
**  -----------------------------
**  Zuerst benötigt man eine SoundCarrier-Struktur, an diese
**  hängt man nach der Initialisierung bis zu 16 einzelne
**  Sound-Effekte. Die SoundCarrier-Struktur muß statisch sein,
**  also zum Beispiel in eine Local-Instance-Data eingebunden.
**
**  Die Initialisierung einer SoundCarrier-Struktur erfolgt per
**  _InitSoundCarrier(). Damit ist die Routine aufnahmebereit
**  für das Ausfüllen der SoundSource-Strukturen.
**
**  Eine SoundSource wird initialisiert, indem sie mit folgenden 
**  Werten ausgefüllt wird:
**
**      SoundSource.sample -> Ptr auf VFMSample-Struktur
**      SoundSource.flags  -> AUDIOF_LOOPDALOOP oder 0
**      SoundSource.volume -> Vorgabe-Lautstärke [0..127]
**      SoundSource.pitch  -> Vorgabe-Tonhöhen-Shift [0 für Original-SampleRate]
**
**  Eine SoundSource wird per _StartSoundSource() aktiviert, und
**  per _EndSoundSource() wieder deaktiviert. Solange die
**  SoundSource aktiv ist, wird die AudioEngine versuchen, sie
**  in den Audio-Out-Strom zu mixen. Bei One-Shot-Sounds muß kein
**  _EndSoundSource() angegeben werden, weil diese nach dem Abspielen
**  von selbst terminieren. Loop-Da-Loop-Sounds MÜSSEN mit einem
**  _EndSoundSource() beendet werden, sobald der entsprechende Ton
**  nicht mehr "erklingen" soll.
**
**  Die AudioEngine muß in jedem Frame auf dem Laufenden gehalten
**  werden, dazu muß folgende Prozedur eingehalten werden:
**
**  1) _StartAudioFrame() das passiert EINMAL ganz am Anfang eines
**     Frames. Hiermit erzählt man der AudioEngine Position,
**     Orientierung und Geschwindigkeit des aktuellen Viewers.
**
**  2) Für ***JEDEN*** SoundCarrier ein _RefreshSoundCarrier(),
**     mit angepaßten Paremetern für Position und Geschwindigkeit,
**     sowie evtl. Vorgabe-Lautstärke und Tonhöhe.
**
**  3) Danach ein _EndAudioFrame(). Das ist für die AudioEngine
**     das Signal, die N höchstpriorisierten SoundSources
**     dieses Frames zur Audio-Out-Buchse rauszuschießen.
**
**  Wenn eine SoundCarrier-Struktur ungültig wird (zum Beispiel
**  innerhalb eines OM_DISPOSE) muß sie vorher per _KillSoundCarrier()
**  "unschädlich" gemacht werden. Wenn die Routine zurückkehrt, kann
**  die SoundCarrier-Struktur ohne Probleme ins Nirvana geschickt
**  werden.
**
**  14-Aug-96
**  ---------
**  Die SoundSource-Struktur wurde ääähh..."multimedial"
**  erweitert, jede SoundSource kann jetzt auch einen
**  Paletten-FX tragen, später evtl. einen Shake-FX.
**
**  Zusammenfassung der genannten AudioEngine Routinen
**  --------------------------------------------------
**  _InitSoundCarrier()
**  _KillSoundCarrier()
**
**  _StartSoundSource()
**  _EndSoundSource()
**
**  _StartAudioFrame()
**  _RefreshSoundCarrier()
**  _EndAudioFrame()
*/

struct SoundCarrier;

/*** Konstanten ***/
#define AUDIOF_LOOPDALOOP   (1<<0)  // ist ein LoopDaLoop-Sound, sonst One-Shot
#define AUDIOF_ACTIVE       (1<<1)  // ist momentan aktiviert
#define AUDIOF_PLAYING      (1<<2)  // wird momentan "richtig" gespielt

#define AUDIOF_HASPFX       (1<<3)  // Paletten-FX attached
#define AUDIOF_PFXACTIVE    (1<<4)  // Paletten-FX momentan aktiv
#define AUDIOF_PFXPLAYING   (1<<5)  // Paletten-FX wird momentan ausgegeben

#define AUDIOF_HASSHAKE     (1<<6)  // Shake-FX attached
#define AUDIOF_SHKACTIVE    (1<<7)  // Shake momentan aktiv
#define AUDIOF_SHKPLAYING   (1<<8)  // Shake wird momentan ausgegeben

#define AUDIOF_HASEXTSMP    (1<<9)  // besitzt Extended Sample Definition

#define MAX_SOUNDSOURCES (16)

#define MAX_SHAKE_X (0.2)   // in rad!
#define MAX_SHAKE_Y (0.2)
#define MAX_SHAKE_Z (0.1)

#define MAX_AUDIO_DISTANCE  (6000.0) // maximale Entfernung für Krach

#define MAX_FRAGMENTS   (8) // max. Anzahl Sample-Fragmente

#define AUDIO_MIN_FREQ  (2000)  // minimale resultierende Grundfrequenz
#define AUDIO_MAX_FREQ  (44100) // maximale resultierende Grundfrequenz
#define AUDIO_MIN_VOL   (4)     // minimale resultierende Lautstärke

/*** Paletten-FX ***/
struct PaletteFX {
    UWORD time;         // Lifetime millisec
    UWORD slot;         // Slot-Nummer
    FLOAT mag0;         // Start-Magnitude
    FLOAT mag1;         // End-Magnitude
};

/*** Shake-FX ***/
struct ShakeFX {
    UWORD time;         // Lifetime in millisec
    UWORD slot;         // derzeit nicht genutzt
    FLOAT mag0;         // Start-Magnitude
    FLOAT mag1;         // End-Magnitude
    FLOAT mute;         // Dämpfungs-Faktor
    FLOAT x,y,z;        // Shake-Stärke pro Dimension in Radiant
};

/*** Extended Sample-Definition (Read Only!) ***/
struct SampleFragment {
    struct VFMSample *sample;   // die Sample-Daten
    UWORD loop;         // Loop-Counter, 0->unendlich
    WORD  volume;       // relative Lautstärke
    LONG  pitch;        // relativer Pitch
    ULONG offset;       // Start-Offset in 11kHz Samples (freq-unabhängig!)
    ULONG len;          // Länge in Samples (freq-unabhängig!)
    ULONG real_offset;  // Start-Offset in "echten" Samples
    ULONG real_len;     // Länge in "echten" Samples
};

struct ExtSample {
    ULONG count;        // Anzahl Samplefragment-Definitionen
    struct SampleFragment frag[MAX_FRAGMENTS];
};

struct SoundSource {

    /*** public ***/
    struct VFMSample *sample;   // Pointer auf Roh-Sample
    struct PaletteFX *pfx;      // Pointer(!) auf Paletten-FX-Definition
    struct ShakeFX   *shk;      // Pointer(!) auf Shake-FX-Definition
    struct ExtSample *ext_smp;  // Pointer(!) auf Extended Sample Definition

    WORD volume;        // globale Audio-Lautstärke, [0..127]
    UWORD flags;        // siehe unten
    LONG pitch;         // globaler Frequenz-Shift

    /*** private ***/
    struct SoundCarrier *sc;    // Backward-Pointer auf eigenen SoundCarrier
    UWORD pri;          // resultierende Priorität
    UWORD frag_act;         // Ext Smp Def: aktuelles Fragment

    ULONG start_time;   // TimeStamp des letzten _StartSoundSource()
    FLOAT pfx_actmag;   // aktuelle Magnitude des Paletten-FX
    FLOAT shk_actmag;   // aktuelle Magnitude des Shake

    WORD res_volume;    // resultierende Lautstärke
    WORD res_pan;       // resultierende Stereo-Position
    LONG res_pitch;     // resultierende Tonhöhe (in Samples/sec)
};

struct SoundCarrier {
    struct flt_triple pos;  // globale 3D-Position
    struct flt_triple vec;  // globaled Geschw.-Vektor (Koords/sec)
    struct SoundSource src[MAX_SOUNDSOURCES]; // eingebettete SoundSources
};

/*-----------------------------------------------------------------**
**  CONFIG STUFF und AudioEngine-Attribut-Tags                     **
**-----------------------------------------------------------------*/

#define AET_BASE        (TAG_USER+0x4000)

#define AET_GET_SPEC        (AET_BASE+1)

#define AET_Channels        (AET_BASE+2)    // ULONG, (SG)
#define AET_MasterVolume    (AET_BASE+3)    // ULONG, (SG)
#define AET_NumPalFX        (AET_BASE+4)    // ULONG, (SG)
#define AET_ReverseStereo   (AET_BASE+5)    // BOOL, (SG)

#define AECONF_Channels         "audio.channels"        // INTEGER
#define AECONF_MasterVolume     "audio.volume"          // INTEGER
#define AECONF_PaletteFX        "audio.num_palfx"       // INTEGER
#define AECONF_ReverseStereo    "audio.rev_stereo"      // BOOL

/*-----------------------------------------------------------------*/
#endif

