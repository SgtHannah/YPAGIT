#ifndef YPA_YPAVEHICLES_H
#define YPA_YPAVEHICLES_H
/*
**  $Source: PRG:VFM/Include/ypa/ypavehicles.h,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 14:31:26 $
**  $Locker:  $
**  $Author: floh $
**
**  Vehikel-Typen und der Death-Cache.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif 

#ifndef NUCLEUS_NUCLEUS2_H
#include "nucleus/nucleus2.h"
#endif

#ifndef YPA_YPADESIGN_H
#include "ypa/ypadesign.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

/*-------------------------------------------------------------------
**  Damit ich keine strcmp()'s machen muß, ist jeder
**  Bakterien-Klasse eine ID zugeordnet.
*/
#define BCLID_NOCLASS       (0)     // nicht definiert
#define BCLID_YPABACT       (1)     // "ypabact.class"
#define BCLID_YPATANK       (2)     // "ypatank.class"
#define BCLID_YPAROBO       (3)     // "yparobo.class"
#define BCLID_YPAMISSY      (4)     // "ypamissile.class"
#define BCLID_YPAZEPP       (5)     // "ypazepp.class"
#define BCLID_YPAFLYER      (6)     // "ypaflyer.class"
#define BCLID_YPAUFO        (7)     // "ypaufo.class"
#define BCLID_YPACAR        (8)     // "ypacar.class"
#define BCLID_YPAGUN        (9)     // "ypagun.class"
#define BCLID_YPAHOVERCRAFT (10)    // "ypahovercraft.class"

/*-----------------------------------------------------------------*/
struct SoundInit {
    UBYTE smp_name[32];
    UBYTE ext_smp_name[MAX_FRAGMENTS][32];

    Object *smp_object;     // zugehöriges sample.class Object
    Object *ext_smp_obj[MAX_FRAGMENTS]; // sample.class Objects für Ext Sample Def

    WORD volume;            // Original-Lautstärke [0..127]
    WORD pitch;             // Frequenz-Differenz in Hertz

    struct PaletteFX pfx;       // Paletten-FX-Definition
    struct ShakeFX   shk;       // Shake-FX-Definition
    struct ExtSample ext_smp;   // Extended-Sample-Definition
};

#define VP_NUM_NOISES  (12)

#define VP_NOISE_NORMAL     (0)     // loop
#define VP_NOISE_FIRE       (1)     // loop
#define VP_NOISE_WAIT       (2)     // loop
#define VP_NOISE_GENESIS    (3)     // loop
#define VP_NOISE_EXPLODE    (4)     // one shot
#define VP_NOISE_CRASHLAND  (5)     // one shot
#define VP_NOISE_CRASHVHCL  (6)     // one shot
#define VP_NOISE_GOINGDOWN  (7)     // loop
#define VP_NOISE_COCKPIT    (8)     // loop (alternativ)
#define VP_NOISE_BEAMIN     (9)     // one shot
#define VP_NOISE_BEAMOUT    (10)    // one shot
#define VP_NOISE_BUILD      (11)    // one shot, nur sinnv. bei Robos

#define WP_NUM_NOISES   (3)

#define WP_NOISE_NORMAL (0)         // kontinuierlich
#define WP_NOISE_LAUNCH (1)
#define WP_NOISE_HIT    (2)

/*** OBSOLETE??? ***/
#define BP_NUM_NOISES   (1)
#define BP_NOISE_WORKING    (0)

/*-------------------------------------------------------------------
**  Eine VehicleProto-Struktur beschreibt jetzt ein konkretes 
**  Vehikle (und nur noch Vehikel, Waffen werden weiter unten
**  abgehandelt).
*/
struct VehicleProto {

    /*** allgemeine Typ-Informationen ***/
    UBYTE BactClassID;      // wie gehabt
    UBYTE FootPrint;        // wer darf dieses Vehicle bauen
    BYTE  Weapons[6];       // bis zu 6 Weapon-Types, (-1) für leerer Slot
    BYTE  MG_Shot;          // Einschuss des MG's
    UBYTE TypeIcon;         // Type-Icon in finder.font
    UBYTE Name[127];         // Typen-Bezeichnung

    /*** visuelle Information (VisProto IDs) ***/
    UWORD TypeNormal;
    UWORD TypeFire;
    UWORD TypeDead;
    UWORD TypeWait;
    UWORD TypeMegaDeth;
    UWORD TypeGenesis;
    struct DestEffect DestFX[NUM_DESTFX];   // aus bacterium.h

    /*** Audio-Informationen ***/
    struct SoundInit Noise[VP_NUM_NOISES];
    ULONG  VOType;      // 1=Light Tank, 2=Medium Tank...
    FLOAT  MaxPitch;    // Maximalfaktor fuer Pitch, 0 heisst nimm klassenspezifisch

    /*** taktisch strategische Informationen ***/
    UWORD Range;        // mittlere Reichweite in 3D-Koords
    UWORD Speed;        // mittlere Geschwindigkeit in Koords/sec

    ULONG Shield;       // wie gehabt
    ULONG Energy;       // wie gehabt
    ULONG CEnergy;      // zusätzlich benötigte Erschaffungs-Energie
    FLOAT ADist_Sector;
    FLOAT ADist_Bact;
    FLOAT SDist_Sector;
    FLOAT SDist_Bact;
    UBYTE View;         // FootPrint-Bereich in Sektoren
    UBYTE nix[3];

    /*** physikalische und geometrische Eigenschaften ***/
    FLOAT Mass;
    FLOAT MaxForce;
    FLOAT AirConst;
    FLOAT MaxRot;
    FLOAT PrefHeight;
    FLOAT Radius;
    FLOAT OverEOF;
    FLOAT ViewerRadius;
    FLOAT ViewerOverEOF;
    
    FLOAT GunAngle;     // Summand zur y-Komponente der Abschußrichtung
    FLOAT FireRelX;     // relative Abschußposition, x wird mal aufaddiert
    FLOAT FireRelY;     // mal abgezogen (seitliches Schießen)
    FLOAT FireRelZ;
    UWORD NumDestFX;                        // Anzahl Destruction-FX
    UWORD NumWeapons;   // für Mehrfachabschuß

    FLOAT GunPower;
    FLOAT GunRadius;
    BOOL  KillAfterShot;

    /*** Special Effects ***/
    FLOAT scale_start;      // Scale-Factor-Startwert
    FLOAT scale_speed;      // Änderung des Factors pro Sec (Add)
    FLOAT scale_accel;      // Anderung von scale_speed pro Sec (Add)
    LONG  scale_duration;   // Dauer der Änderung in Millisec
    UWORD scale_type[MAXNUM_VPFX];  // VisProto-Nummer

    /*** Eignung für bestimmte Aufgaben ***/
    UBYTE   JobFightTank;
    UBYTE   JobFightHelicopter;
    UBYTE   JobFightFlyer;
    UBYTE   JobFightRobo;
    UBYTE   JobConquer;
    UBYTE   JobReconnoitre;

    /*** 2D-Vektor-Outlines ***/
    Object *wireframe_object;   // skeleton.class

    /*** Universal Attribute ***/
    struct TagItem *LastTag;        // zeigt auf TAG_DONE
    struct TagItem MoreAttrs[8];
};

/*-------------------------------------------------------------------
**  Ein Weapon-Prototype, wie im Weapon-Array zu finden...
**  (Die Schußfrequenz ist Eigenschaft der Waffe...wenn's mal mehrere
**  werden!)
*/
struct WeaponProto {

    /*** allgemeine Typ-Informationen ***/
    UBYTE BactClassID;  // wie gehabt
    UBYTE FootPrint;    // wer darf dieses Vehicle bauen
    UWORD Flags;        // siehe unten...
    UBYTE TypeIcon;     // Type-Icon in finder.font(?)
    UBYTE Name[127];     // Typen-Bezeichnung

    /*** visuelle Information (VisProto IDs) ***/
    UWORD TypeNormal;
    UWORD TypeFire;
    UWORD TypeDead;
    UWORD TypeWait;
    UWORD TypeMegaDeth;
    UWORD TypeGenesis;
    ULONG NumDestFX;                        // Anzahl Destruction-FX
    struct DestEffect DestFX[NUM_DESTFX];   // aus bacterium.h

    /*** Audio-Information ***/
    struct SoundInit Noise[WP_NUM_NOISES];

    /*** taktisch strategische Informationen ***/
    ULONG Range;        // mittlere Reichweite in 3D-Koords
    ULONG Speed;        // mittlere Geschwindigkeit in Koords/sec

    ULONG Energy;       // -> kommt auch nach Bact-Struktur
    ULONG CEnergy;      // zusätzlich benötigte Erschaffungs-Energie

    ULONG LifeTime;     // mittlere Lifetime der Rakete in millisec
    ULONG LifeTimeNT;   // kann als Extrawert fuer Raketen ohne Ziel angegeben werden
    ULONG DriveTime;    // Brenndauer des Triebwerkes
    ULONG DelayTime;    // Zünder-Verzögerung in Millisec...
    FLOAT ADist_Sector;
    FLOAT ADist_Bact;
    ULONG ShotTime;     // Zeit bis zum nächsten Schuß
    ULONG ShotTime_User;// wenn der User drinnen sitzt...
    ULONG SalveShots;       // Maximalzahl von Schuessen, die mit ShotTime verzoegert werden
    ULONG SalveDelay;       // Verzoegerung danach
    
    /*** spezielle Keywords zur Differenzierung ***/
    FLOAT EnergyHeli;   // gegen Hubschrauber
    FLOAT EnergyTank;   // gegen tanks und cars                             
    FLOAT EnergyFlyer;  // gegen flyer und UFOs
    FLOAT EnergyRobo;   // gegen Robos
    FLOAT RadiusHeli;
    FLOAT RadiusTank;
    FLOAT RadiusFlyer;
    FLOAT RadiusRobo;

    /*** physikalische und geometrische Eigenschaften ***/
    FLOAT Mass;
    FLOAT MaxForce;
    FLOAT AirConst;
    FLOAT MaxRot;
    FLOAT PrefHeight;
    FLOAT Radius;
    FLOAT OverEOF;
    FLOAT ViewerRadius;
    FLOAT ViewerOverEOF;
    FLOAT StartSpeed;       // Offset zur Geschwindigkeit des Schützen

    /*** 2D-Vektor-Outlines ***/
    Object *wireframe_object;       // skeleton.class

    /*** Universal Attribute ***/
    struct TagItem *LastTag;        // zeigt auf TAG_DONE
    struct TagItem MoreAttrs[8];
};

/*** Flags für WeaponProto.Flags ***/
#define WPF_Autonom         (1<<0)  // es ist eine autonome Waffe
#define WPF_Driven          (1<<1)  // die Waffe hat einen Antrieb
#define WPF_Searching       (1<<2)  // die Waffe ist selbstsuchend
#define WPF_FireAndForget   (1<<3)  // die Waffe weicht Hindernissen aus
#define WPF_Impulse         (1<<4)  // die Waffe hat nur einen Anfangs-Impuls

/*-------------------------------------------------------------------
**  Ein Building-Prototype, wie im BuildArray zu finden...
*/

/*** stationäre Bakterie ***/
struct StatBact {
    ULONG vp;           // eine VehicleProto-Nummer, 0 ist NICHT!
                        // erlaubt.
    fp3d pos;           // 3D-Position RELATIV ZU SEKTOR-MITTE!!!
    fp3d vec;           // Richtungs-Vektor
};

struct BuildProto {

    UBYTE SecType;      // Sektor-Typ des guten Stücks...
    UBYTE FootPrint;    // wer darf das Teil bauen?
    UBYTE BaseType;     // Basis-Typ des Bauwerks (siehe unten)
    UBYTE Power;        // "Stärke" des Bauwerks
    UBYTE TypeIcon;     // Type-Icon in finder.font(?)
    UBYTE Name[127];

    LONG  CEnergy;      // die zur Erschaffung benötigte Gesamt-Energie

    struct SoundInit Noise[BP_NUM_NOISES];

    struct StatBact SBact[8];  // maximal 8 stationäre Bakterien
};

/*-------------------------------------------------------------------
**  Die hier definierten Base-Types stehen in BuildProto.BaseType
**  und definierten jeweils einen "Grundtyp" für ein
**  Gebäude. Eine genauere Unterteilung ist dann jeweils
**  in BuildProto.Power zu finden. Das dürfte den AI-Robos
**  auch die Suche nach einem geeignetem Gebäude erleichtern.
**
**  Der einzige BaseType mit Funktionalität ist das Kraftwerk.
**  Bei den Radarstationen wird die Footprint-Reichweite
**  der 1.(!!!) stationären Bakterie (das muß immer die Radarschüssel
**  sein) auf die Reichweite der Radarstation gehackt, das
**  passiert innerhalb YWM_CREATEBUILDING.
**
**  Trotz der Tatsache, daß alle anderen Bauwerke keine
**  Sonderfunktionalität haben, sollten trotzdem
**  sinnvolle Basis-Typen definiert werden, um den
**  AI-Robos die Suche zu erleichtern. Das muß nochmal
**  bequatscht werden.
*/
#define BUILD_BASE_NONE         (0)     // KEIN gültiger Buildproto, ignorieren
#define BUILD_BASE_KRAFTWERK    (1)     // BuildProto.Power
#define BUILD_BASE_RADARSTATION (2)     // BuildProto.Power->Reichweite
#define BUILD_BASE_DEFCENTER    (3)     // Verteidiguns-Zentrum

/*-----------------------------------------------------------------*/
#endif

