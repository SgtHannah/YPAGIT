#ifndef ADE_PARTICLECLASS_H
#define ADE_PARTICLECLASS_H
/*
**  $Source: PRG:VFM/Include/ade/particleclass.h,v $
**  $Revision: 38.8 $
**  $Date: 1996/11/10 21:15:03 $
**  $Locker:  $
**  $Author: floh $
**
**  Die Partikel-System-ADE-Klasse...
**
**  (C) Copyright 1995-1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"
#endif

#ifndef SKELETON_SKELETONCLASS_H
#include "skeleton/skeletonclass.h"
#endif

#ifndef ADE_ADE_CLASS_H
#include "ade/ade_class.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      particle.class -- Partikel-System in einer ADE-Subklasse
**
**  FUNCTION
**      Mit der particle.class hat man ein visuelles Partikel-
**      System zur Verfügung, daß sich überall dort rankleben
**      läßt, wo man halt ein ADE rankleben kann :-)))
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ade.class
**      ~~~~~~~~~
**      particle.class
**      ~~~~~~~~~~~~~~
**      PTLM_ACCEL
**          Msg: struct ptl_vector_msg
**          Ret: ---
**
**          Definiert einen Beschleunigungs-Vektor, dem die Partikel
**          ausgesetzt sind. In der <msg> wird der
**          Anfangs-Zustand und End-Zustand des Vektors übergeben,
**          zwischen den beiden Zuständen wird über die Lebensdauer
**          eines Kontexts interpoliert.
**
**      PTLM_GETACCEL
**          Msg: struct ptl_vector_msg
**          Ret: ---
**
**          Schreibt in die bereitgestellte Message den aktuellen
**          Status des Beschleuniguns-Vektors.
**
**      PTLM_MAGNIFY
**          Msg: struct ptl_vector_msg
**          Ret: ---
**
**          Diese Methode definiert einen Richtungsvektor,
**          entlang welchem neugeborene Partikel ausgestoßen
**          werden. Das funktioniert so:
**
**          Ein neugeborenes Partikel bekommt zuerst einen
**          (mehr oder weniger) zufälligen Start-Vektor aus einer
**          internen Zufallszahlen-Tabelle zugewiesen. Jede dieser
**          Zufalls-Komponenten liegt irgendwo zwischen -1.0 und +1.0.
**          Auf diesen Zufallsvektor werden die Komponenten des
**          Magnifier-Vektors aufaddiert.
**          Der resultierende Vektor wird normalisiert und
**          und mit PTLA_StartSpeed multipliziert... fertig.
**
**          In der <msg> wird der Anfangs- und Endzustand des Vektors
**          übergeben, über den über die Lebensdauer des Kontexts
**          interpoliert wird.
**
**          Der Magnifier-Vektor ist immer im lokalen Koordinaten-System
**          definiert.
**
**      PTLM_GETMAGNIFY
**          Msg: struct ptl_vector_msg
**          Ret: ---
**
**          Schreibt in die bereitgestellte Message den aktuellen
**          Status des Magnifier-Vektors.
**
**      PTLM_ADDADE
**          Msg: Object *new_ade
**          Ret: TRUE/FALSE
**
**          Hängt ein neues ADE an das aktuelle Ende des ADE-Arrays
**          (nur, wenn noch eins reinpaßt). Kommt TRUE zurück,
**          wenn alles OK, FALSE, wenn ADE-Array voll.
**
**      PTLM_REMADE
**          Msg: ULONG num
**          Ret: Object *
**
**          Entfernt das per Nummer definierte ADE
**          aus dem ADE-Array, die nachfolgenden ADEs rücken auf.
**          Der Objekt-Pointer des entfernten ADEs wird zurück-
**          gegeben, das ADE wird *nicht* disposed.
**
**      PTLM_ADEUP
**          Msg: ULONG num
**          Ret: ---
**
**          Das per <num> definierte ADE wird mit seinem Vorgänger
**          vertauscht.
**
**      PTLM_ADEDOWN
**          Msg: ULONG num
**          Ret: ---
**
**          Das per <num> definierte ADE wird mit seinem Nachfolger
**          vertauscht.
**
**
**  ATTRIBUTES
**
**      PTLA_StartSpeed  (ISG) LONG
**          Definiert die Anfangsgeschwindigkeit eines Partikels
**          in coords/sec.
**
**      PTLA_NumContexts (ISG) LONG
**          Anzahl "Kontexte" für dieses Object. Das sind einzelne
**          "Particle-Spaces", die dieselben Parameter besitzen,
**          aber eine unterschiedliche Position auf der Timeline.
**
**          Ein (S) dieses Attrs bewirkt eine Neuallokierung
**          interner Datenstrukturen, also ...
**
**      PTLA_ContextLifetime (ISG) LONG
**          Definiert die Gesamt-Lebensdauer eines Contexts in
**          1/1000(!) sec. Nach Ablauf dieser Zeit wird
**          der Context wieder frei.
**
**      PTLA_StartGeneration (ISG) LONG
**          Definiert den Zeitpunkt, an dem der Context anfängt,
**          die ersten Partikel zu generieren (in millisec seit
**          Context-Geburt).
**
**      PTLA_StopGeneration (ISG) LONG
**          Definiert den Zeitpunkt, an dem der Context aufhört,
**          Partikel zu generieren (in millisec seit Kontext-
**          Geburt). Um "normale" Ergebnisse zu erhalten, sollte
**          <PTLA_StopGeneration> mindestens <PTLA_Lifetime>
**          vor <PTLA_ContextLifetime> liegen, damit das
**          letzte Partikel noch korrekt "verrauchen" kann.
**
**      PTLA_BirthRate  (ISG) LONG
**          Ausstoßrate des Partikelsystems in Partikel/sec.
**
**          Ein (S) dieses Attrs bewirkt eine Neuallokierung
**          interner Datenstrukturen, also ...
**
**      PTLA_Lifetime   (ISG) LONG
**          Definiert die Lebensdauer eines Partikels in
**          1/1000(!) sec.
**
**          Eine (S) dieses Attrs bewirkt eine Neuallokierung
**          interner Datenstrukturen, also ...
**
**      PTLA_ADE     (ISG) <Object *>
**          *** existiert nur noch aus historischen Gründen, ***
**          *** siehe PTLA_ADEArray                          ***
**          Pointer auf ein ADE-Object, kann sowohl ein Polygon-
**          als auch ein Point-ADE sein. Ein Polygon-ADE muß
**          allerdings einen 4-eckigen Polygon akzeptieren!
**          Das ADE-Object wird zur Visualisierung der einzelnen
**          Partikel verwendet.
**
**      PTLA_ADEArray   (ISG) <Object **>
**          Zeigt auf ein NULL-terminiertes(!) Array mit bis zu
**          8 ADE-Object-Pointern. Die ADEs werden *nacheinander*
**          zur Visualisierung benutzt, der Start-Zeitindex
**          für ein beliebiges ADE ist mit
**              (lifetime/num_ades) * act_ade
**          festgelegt. Sprich: Ist nur ein ADE angegeben, ist dieses
**          über die gesamte Lebenszeit eines Partikels gültig,
**          mehrere ADEs verteilen sich gleichmäßig über die
**          Partikel-Lifetime. That's all.
**
**          Das veraltete PTLA_ADE Attribut verhält sich aus
**          Backward-Compatibilty Gründen genauso wie ein
**          PTLA_ADEArray mit nur einem ADE.
**
**          WICHTIG: Die "vorherige" ADE-Belegung wird mit jedem
**          neuen set(PTLA_ADEArray) komplett disposed!
**
**      PTLA_StartSize  (ISG) LONG
**      PTLA_EndSize    (ISG) LONG
**          Größe der Partikel im 3D-Space, zwischen der Anfangs-
**          und End-Größe wird linear interpoliert.
**
**      PTLA_Noise  (ISG) LONG
**          Definiert Stärke des "Rauschens", mit dem die
**          aktuelle Position "verwischt" wird. So funktionierts:
**              pos = rand[-1..+1] * PTLA_Noise;
*/

/*-----------------------------------------------------------------*/
#define PARTICLE_CLASSID "particle.class"
/*-----------------------------------------------------------------*/
#define PTLM_BASE           (ADEM_BASE+METHOD_DISTANCE)
#define PTLM_ACCEL          (PTLM_BASE)
#define PTLM_MAGNIFY        (PTLM_BASE+1)
#define PTLM_GETACCEL       (PTLM_BASE+2)
#define PTLM_GETMAGNIFY     (PTLM_BASE+3)
#define PTLM_ADDADE         (PTLM_BASE+4)
#define PTLM_REMADE         (PTLM_BASE+5)
#define PTLM_ADEUP          (PTLM_BASE+6)
#define PTLM_ADEDOWN        (PTLM_BASE+7)
/*-----------------------------------------------------------------*/
#define PTLA_Base               (ADEA_Base+ATTRIB_DISTANCE)
#define PTLA_StartSpeed         (PTLA_Base)
#define PTLA_NumContexts        (PTLA_Base+1)
#define PTLA_ContextLifetime    (PTLA_Base+2)
#define PTLA_BirthRate          (PTLA_Base+3)
#define PTLA_Lifetime           (PTLA_Base+4)
#define PTLA_ADE                (PTLA_Base+5)
#define PTLA_StartSize          (PTLA_Base+6)
#define PTLA_EndSize            (PTLA_Base+7)
#define PTLA_StartGeneration    (PTLA_Base+8)
#define PTLA_StopGeneration     (PTLA_Base+9)
#define PTLA_Noise              (PTLA_Base+10)
#define PTLA_ADEArray           (PTLA_Base+11)
/*-------------------------------------------------------------------
**  Default-Attribut-Werte
*/
#define PTLA_StartSpeed_DEF         (50)
#define PTLA_NumContexts_DEF        (1)
#define PTLA_ContextLifetime_DEF    (1000)
#define PTLA_BirthRate_DEF          (10)
#define PTLA_Lifetime_DEF           (3000)
#define PTLA_StartSize_DEF          (30)
#define PTLA_EndSize_DEF            (30)
#define PTLA_StartGeneration_DEF    (0)
#define PTLA_StopGeneration_DEF     (1000)

/*-------------------------------------------------------------------
**  perverse Konstanten
*/
#define PTL_MAXNUMADES      (8)

/*-----------------------------------------------------------------*/
struct ptc_vec {
    FLOAT x,y,z;
};

struct Particle {       // Definition eines einzelnen Partikels
    FLOAT px,py,pz;     // aktuelle Position [x,y,z]
    FLOAT vx,vy,vz;     // aktuelle Richtung/Speed
};

struct Context {        // ein Partikel-Context
    struct Particle *Start;    // Pointer auf Particle-Array (Ringbuffer)
    struct Particle *End;      // Pointer auf Ende des Particle-Array
    struct Particle *Oldest;   // Ptr auf ältestes gültiges Particle
    struct Particle *Latest;   // Ptr auf jüngstes gültiges Particle

    ULONG OldestAge;        // Alter des momentan ältesten Partikels

    LONG Timer;
    ULONG Id;               // Kontext-ID, extern bereitgestellt, != 0
    ULONG Timestamp;        // Time-Stamp der Kontext-Geburt (millisec)
    ULONG Age;              // aktuelles Kontext-Alter (millisec)
};

/*-----------------------------------------------------------------*/
struct particle_data {

    /*** allgemein ***/
    Object *Skeleton;           // "Fake Skeleton" für eingebettetes ADE
    struct Skeleton *Sklt;      // dessen Skeleton-Struktur

    ULONG Flags;                // siehe unten
    ULONG PointNum;

    /*** ADEs ***/
    ULONG NumADEs;              // 0..PTL_MAXNUMADES
    LONG ADETimeDiff;           // == (Lifetime/NumADEs)
    Object *ADEArray[PTL_MAXNUMADES+1];   // NULL-terminiert

    /*** konstante Vektoren und Kollision ***/
    struct ptc_vec StartAccel;     // Start-Zustand des Accel-Vektors
    struct ptc_vec EndAccel;
    struct ptc_vec AddAccel;       // zur Interpolation

    struct ptc_vec StartMagnify;   // Start-Zustand des Magnifier Vektors
    struct ptc_vec EndMagnify;
    struct ptc_vec AddMagnify;     // zur Interpolation

    /*** Partikel-Konstanten ***/
    LONG Lifetime;              // Particle-Lifetime in millisec
    LONG Birthrate;             // in Particles/sec!!!
    LONG Threshold;             // alle soviele Millisec wird ein Ptl geboren

    FLOAT StartSize;            // Radius eines Partikels im 3D-Space
    FLOAT EndSize;
    FLOAT AddSize;
    FLOAT StartSpeed;           // Anfangs-Geschwindigkeit eines Partikels
    FLOAT Noise;                // Stärke des "Positions-Rauschens"

    /*** Context-Konstanten und Context-Array ***/
    LONG NumContexts;           // Anzahl Kontexte im ContextArray
    LONG CtxLifetime;           // Kontext-Lifetime in millisec
    LONG CtxStartGen;           // Zeitpunkt für Partikel-Generation-Start
    LONG CtxStopGen;            // Zeitpunkt für Partikel-Generation-Stop

    struct Context *CtxStart;   // Ptr auf Context-Array (Ringbuffer)
    struct Context *CtxEnd;     // Ptr auf Ende des Context-Array
    struct Context *CtxNext;    // Pointer auf nächsten (freien) Kontext
};

/*-------------------------------------------------------------------
**  Definitionen für particle_data.Flags
*/
#define PTLF_DepthFade  (1<<0)

/*-----------------------------------------------------------------*/
struct ptl_vector_msg {
    struct ptc_vec start_vec;
    struct ptc_vec end_vec;
};

struct ptl_addade_msg {
    Object *ade;
    ULONG num;
};

/*-------------------------------------------------------------------
**  IFF-Definitionen
*/
#define PARTIFF_VERSION 1

#define PARTIFF_FORMID  MAKE_ID('P','T','C','L')
#define PARTIFF_Attrs   MAKE_ID('A','T','T','S')

struct part_IFFAttr {
    UWORD version;

    // (PARTIFF_VERSION == 1)
    struct ptc_vec start_accel;
    struct ptc_vec end_accel;
    struct ptc_vec start_magnify;
    struct ptc_vec end_magnify;

    LONG collide;                   // unbenutzt, aber zu spät...
    LONG start_speed;               // PTLA_StartSpeed
    LONG num_contexts;              // PTLA_NumContexts
    LONG ctx_lifetime;              // PTLA_ContextLifetime
    LONG ctx_startgen;              // PTLA_StartGeneration
    LONG ctx_stopgen;               // PTLA_StopGeneration
    LONG birthrate;                 // PTLA_BirthRate
    LONG lifetime;                  // PTLA_Lifetime
    LONG start_size;                // PTLA_StartSize
    LONG end_size;                  // PTLA_EndSize
    LONG noise;                     // PTLA_Noise
};

/*
**  Nach dem Attrs-Chunk folgen 0..PTL_MAXNUMADES OBJT Chunks
*/

/*-----------------------------------------------------------------*/
#endif


