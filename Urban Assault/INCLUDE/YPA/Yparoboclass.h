#ifndef YPA_ROBOCLASS_H
#define YPA_ROBOCLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/yparoboclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/14 17:08:59 $
**  $Locker:  $
**  $Author: floh $
**
**  Roboclass. Klasse für höchste Form der Objekte, die in der Lage sind,
**  strategisch zu handeln und andere Objekte dafür nutzen.
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

#ifndef ADE_ADE_CLASS_H     /* für ArgStack/PubStack-Entry-Zeug */
#include "ade/ade_class.h"
#endif

#ifndef BASECLASS_BASECLASS_H
#include "baseclass/baseclass.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

#ifndef YPA_BACTCLASS_H
#include "ypa/ypabactclass.h"
#endif


/*-------------------------------------------------------------------
**  NAME
**      yparobo.class --
**
**
**  FUNCTION
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**
**      yparobo.class
**      ~~~~~~~~~~~~~
**
**          YRM_ALLOWVTYPES / YRM_FORBIDVTYPES
**              Erlaubt bzw. verbietet dem Robo das Bauen gewisser VTypes
**
**          YRM_GETCOMMANDER
**              Ermittelt den Commander zu gewissen Informationen
**
**          YRM_SEARCHROBO
**              Sucht einen Robo, der wir nicht selbst sind. In der Struktur
**              muß die Weltgröße, der zuletztgefundene Robo und der Suchbereich
**              übergeben werden. Wenn ret == TRUE, dann ist der Bakterien-
**              pointer gültig.
**
**          YRM_SEARCHNEXTROBO
**              Sucht zeilenweise nächstliehgenden Robo. Zeile wird in 
**              yrd->ActLine gemerkt. ret = TRUE->wert ok
**              ret = FALSE und Pointer == NULL --> konnte bis jetzt nix
**              finden, pointer == 1L --> konnte im ganzen Bereich nix finden!
**
**          YRM_EXTERMAKENCOMMAND
**              Erzeugt ein Geschwader ohne daß der Robo der meinung ist, er
**              müßte sowas tun, z.B. vor Spielbeginn. Man übergibt den VisProto,
**              die Anzahl und die position, an der die leute erzeugt werden und
**              die auch als Hauptziel zugewiesen wird. Mit Flags kann ich 
**              entscheiden, ob dieses geschwader als unbrauchbar deklariert 
**              werden soll und ob ich testen soll, ob ich das überhaupt bauen darf.
**              Energie wird derzeit nicht berücksichtigt.
**
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**      yparobo.class
**      ~~~~~~~~~~~~~
**          YRA_Brain (G)   <struct brain_part *>
**              Gibt Adresse des Gehirns zurück (&(yrd->brain))
**          
**          (Kollision analog ZeppelinClass)
**          YRA_MPtoMP   (ISG) <BOOL> [def = TRUE]
**              Intersection von bisheriger zu neu berechneter Position
**
**          YRA_MPSphere (ISG) <BOOL> [def = FALSE]
**              Kugeltest um neu berechnete Position mit bact.radius
**
**          YRA_TPtoTP   (ISG) <BOOL> [def = FALSE]
**              Intersections von den Testpoints zu den neu berechneten
**              Testpoints
**
**          YRA_TPSphere (ISG) <BOOL> [def = FALSE]
**              Kugeltests um alle neu berechneten Testpoints
**
**          YRA_TestPoint (I) <struct flt_triple *>
**              Übergibt einen testpoint. Dabei wird der interne Zähler 
**              hochgezählt. Maximal können 10 Attribute übergeben werden,
**              alle nachfolgenden werden ignoriert.
**              Mit Get liefert das Attr. den Anfang aller Testpoints
**              (flt_triple - Pointer)
**
**          YRA_TPCount (G) <ULONG>
**              Gibt die derzeitige Zahl von Testpoints zurück.
**              Nur getten, NIE SETTEN!!!!
**
**          YRA_Buoyancy (ISG) <ULONG>
**              Auftrieb. Denn bei diesem Modell ist act_force für
**              die Geschwindigkeit, nicht aber für die Flughöhe verant-
**              wortlich.
**
**          YRA_BattVehicle / YRA_BattBuilding
**              bei set und init kann man den Prozentwert angeben, der die
**              maximale Batteriefüllung (prozentual) vom Maximum angibt
**              Sinnvoll zum Malen des Balkens
**
**          YRA_VBActual / YRA_BBactual
**              Gibt den aktuellen Füllstand (absolut) an.
**
**          YRA_VBMaximum / YRA_BBMaximum
**              Gibt den maximalen Füllstand (absolut - klar ) an.
**
**          YRA_GunArray
**              Adresse des Bereiches, wo die Kanonen stehen
**
**          YRA_EP....
**              Sind jetzt prozentwerte, mit denen die priorität zur auswahl
**              des nächsten Jobs multipliziert wird. 0..das nicht machen
**              100...das ist ganz wichtig
*/
#define YPAROBO_CLASSID "yparobo.class"

#define NUM_HELP            4           // Zahl der Merkzellen

/*-----------------------------------------------------------------*/
#define YRM_BASE                (YBM_BASE+METHOD_DISTANCE)
#define YRM_GETCOMMANDER        (YRM_BASE)
#define YRM_ALLOWVTYPES         (YRM_BASE+1)
#define YRM_FORBIDVTYPES        (YRM_BASE+2)
#define YRM_SEARCHROBO          (YRM_BASE+3)
#define YRM_GETENEMY            (YRM_BASE+4)
#define YRM_EXTERNMAKECOMMAND   (YRM_BASE+5)
#define YRM_LOGMSG              (YRM_BASE+6)



/*-----------------------------------------------------------------*/
#define YRA_BASE            (YBA_BASE+ATTRIB_DISTANCE)
#define YRA_Buoyancy        (YRA_BASE)          // ISG (ULONG)
#define YRA_Extension       (YRA_BASE+1)        // IS struct RoboExtension *
#define YRA_EPConquer       (YRA_BASE+2)
#define YRA_EPDefense       (YRA_BASE+3)
#define YRA_EPRadar         (YRA_BASE+4)
#define YRA_EPPower         (YRA_BASE+5)
#define YRA_EPSafety        (YRA_BASE+6)
#define YRA_CommandCount    (YRA_BASE+7)        // SG
#define YRA_BattVehicle     (YRA_BASE+8)        // ISG  absolut
#define YRA_BattBuilding    (YRA_BASE+9)        //
#define YRA_BattBeam        (YRA_BASE+10)
#define YRA_FillModus       (YRA_BASE+11)
#define YRA_WaitSway        (YRA_BASE+12)       // IS
#define YRA_WaitRotate      (YRA_BASE+13)       // IS
#define YRA_GunArray        (YRA_BASE+14)       // G
#define YRA_EPChangePlace   (YRA_BASE+15)
#define YRA_EPReconnoitre   (YRA_BASE+16)
#define YRA_EPRobo          (YRA_BASE+17)
#define YRA_RoboState       (YRA_BASE+18)       // G
#define YRA_ViewAngle       (YRA_BASE+19)       // IS, Achtung! in Grad!!
#define YRA_SafDelay        (YRA_BASE+20)       // Um Anfangswerte deklarieren
#define YRA_PowDelay        (YRA_BASE+21)       // zu koennen
#define YRA_RadDelay        (YRA_BASE+22)
#define YRA_CplDelay        (YRA_BASE+23)
#define YRA_DefDelay        (YRA_BASE+24)
#define YRA_ConDelay        (YRA_BASE+25)
#define YRA_RobDelay        (YRA_BASE+26)
#define YRA_RecDelay        (YRA_BASE+27)
#define YRA_LoadFlags       (YRA_BASE+28)       // (G) FLOH 28-Apr-98
#define YRA_LossFlags       (YRA_BASE+29)       // (G) FLOH 28-Apr-98 
#define YRA_AbsReload       (YRA_BASE+30)       // (G) FLOH 20-May-98

/*-------------------------------------------------------------------
**  Defaults für Attribute
*/

/*** Anteile für den autonomen Robo ***/
#define YRA_EPConquer_DEF       (30)        // Wichtigkeit Eroberung
#define YRA_EPDefense_DEF       (90)        // Wichtigkeit Verteidigung
#define YRA_EPRadar_DEF         (30)        // Wichtigkeit Radar
#define YRA_EPPower_DEF         (50)        // Wichtigkeit Kraftwerk
#define YRA_EPSafety_DEF        (50)        // Wichtigkeit Flak
#define YRA_EPReconnoitre_DEF   (80)        // Wichtigkeit Aufklärung
#define YRA_EPChangePlace_DEF   (100)       // Wichtigkeit Platzwechsel
#define YRA_EPRobo_DEF          (100)       // Wichtigkeit Robobekämpfung

/*** weils überschrieben wird ***/
#define YBA_YourLastSeconds_Robo_DEF (3000)    // für Robos (war 20000)

/*-------------------------------------------------------------------
**  Strukturen
*/


struct gun_data {

    struct flt_triple pos;      // Position der Kanone
    struct flt_triple dir;      // Richtung der Kanone (NORMIERT!!!!)
    Object *go;                 // Objekt der Kanone
    char   name[ 32 ];          // Name
    UBYTE  gun;                 // ProtoID  der Kanone
};

#define NUMBER_OF_GUNS      8


/*** Sozusagen Robo-Bact-Struktur. Mit Tag übergeben. Non getable ***/

struct RoboExtension {

    /*** Für den Viewer ***/
    struct extra_viewer Viewer;

    /*** Die Kanonen ***/
    struct gun_data gun[ NUMBER_OF_GUNS ];
    UBYTE  number_of_guns;      // used guns!

    struct flt_triple dock_pos; // relative Position des Docks

    struct ExtCollision coll;   // Robo hat erweiterte Kollision
};            


#define MAXNUM_ROBOATTACKER     (16)
struct roboattacker {

    ULONG   attacker_id;
    ULONG   robo_id;
};

/*-----------------------------------------------------------------*/
struct yparobo_data {

    /*** allgemeines ***/
    Object      *world;             // für schnellen Zugriff
    struct ypaworld_data *ywd;
    struct Bacterium *bact;         // für schnelleren Zugriff

    /*** physikalische Attribute ***/
    FLOAT       buoyancy;           // Auftriebskraft, analog zepp.class
    struct ExtCollision coll;       // eine ExtColl.-Struktur
    FLOAT       merke_y;            // für das Schwanken...

    /*** Zeugs ***/
    UBYTE       flags;
    UBYTE       waitflags;          // das machen Autonome, wenn sie nix zu tun haben

    /*** Intelligenz ***/
    ULONG       RoboState;          // spezielle RoboZustände (ROBO_... )
    ULONG       NewAI;              // mit ausgebremster Suche
    ULONG       TimeLine;           // Wartezeit fuer 0%, in MILLISEKUNDEN!
    
    /*** Strategie ***/
    UBYTE       ep_Conquer;         // Waren früher energieanteile, sind heute
    UBYTE       ep_Radar;           // Prozentwerte für die Prioritätsberechnung
    UBYTE       ep_Power;           // 0..tues nicht, 100..wichtig
    UBYTE       ep_Defense;
    UBYTE       ep_Safety;
    UBYTE       ep_Reconnoitre;
    UBYTE       ep_ChangePlace;
    UBYTE       ep_Robo;

    /*** Zum Dock ***/
    LONG        dock_energy;        // Wenn das unter 0 ist, dann ist das der-
                                    // zeitige DockGeschwader aufgefüllt
    ULONG       dock_count;         // Um im Dock Geschwader zu realisieren
    ULONG       dock_user;          // derzeitiger Nutzer des Docks (CommandID)
    LONG        dock_time;          // Blockadezeit
    struct flt_triple dtpos;        // dieses Ziel bekommt der geschwaderführer
    struct Bacterium  *dtbact;      // nach dem fertigen Erzeugen des Geschwaders
    UBYTE       dttype;             // wenn ROBO_SETDOCKTARGET gesetzt war
                                    // is' immer Hauptziel
    UBYTE       dock_aggr;          // diese wird dem Commander nach dem Erzeugen
                                    // des geschw. übergeben
    ULONG       dtCommandID;        // Zur Identifizierung von Bakterien
    struct flt_triple dock_pos;     // Erzeugungsposition für vehicle relativ zu Local-0

    /*** CheckWorld-Sachen ***/
    LONG         chk_Radar_Value;   // Der beste Platz für ein Radar
    struct Cell *chk_Radar_Sector;
    LONG         chk_Radar_Pos;     // Dort ist es passiert
    LONG         chk_Radar_Count;
    LONG         chk_Radar_Time;    // Damals habe ich das gefunden (globaltime)
    LONG         chk_Radar_Delay;   // solange test verzoegern

    LONG         chk_Safety_Value;  // Der beste Platz für eine Flakstellung
    struct Cell *chk_Safety_Sector;
    LONG         chk_Safety_Pos;
    LONG         chk_Safety_Count;
    LONG         chk_Safety_Time;
    LONG         chk_Safety_Delay;   

    LONG         chk_Power_Value;   // Der beste Platz für ein Kraftwerk
    struct Cell *chk_Power_Sector;
    LONG         chk_Power_Pos;
    LONG         chk_Power_Count;
    LONG         chk_Power_Time;
    LONG         chk_Power_Delay;   

    LONG         chk_Enemy_Value;   // das gefährlichste Geschwader
    struct Cell *chk_Enemy_Sector;
    LONG         chk_Enemy_Pos;
    ULONG        chk_Enemy_CommandID;
    LONG         chk_Enemy_Time;
    LONG         chk_Enemy_Div;     // zur Ausbremsung der Tests
    LONG         chk_Enemy_Delay;   

    LONG         chk_Terr_Value;    // das eroberungswürdigste Gebiet
    struct Cell *chk_Terr_Sector;
    LONG         chk_Terr_Pos;
    LONG         chk_Terr_Count;
    LONG         chk_Terr_Time;
    LONG         chk_Terr_Delay;   

    LONG         chk_Place_Value;   // bessere Position
    struct Cell *chk_Place_Sector;
    LONG         chk_Place_Pos;
    LONG         chk_Place_Count;
    LONG         chk_Place_Time;
    LONG         chk_Place_Delay;   

    LONG         chk_Recon_Value;   // das unbekannteste, also aufklärungs-
    struct Cell *chk_Recon_Sector;  // würdigste Gebiet
    LONG         chk_Recon_Pos;
    LONG         chk_Recon_Count;
    LONG         chk_Recon_Time;
    LONG         chk_Recon_Delay;   

    LONG         chk_Robo_Value;    // das gefährlichste Geschwader
    struct Cell *chk_Robo_Sector;
    LONG         chk_Robo_Pos;
    ULONG        chk_Robo_CommandID;
    LONG         chk_Robo_Time;
    LONG         chk_Robo_Delay;   

    LONG         VehicleSlot_Pos;   // das ist der Slot zum Bauen von Vehiclen,
    struct Cell *VehicleSlot_Sector;// der also das Dock benötigt, in Kind mer-
    LONG         VehicleSlot_Kind;  // ken wir uns die Art als ROBO_BUILD-Flag
    LONG         VehicleSlot_CommandID;

    LONG         BuildSlot_Pos;     // das ist der Slot zum Bauen von gebäuden
    struct Cell *BuildSlot_Sector;  // und dem Fliehen, der also die Bewegung
    LONG         BuildSlot_Kind;    // benötigt

    struct Cell *FirstSector;       // erleichtet Testphasen

    LONG         observer_time;
    LONG         enemy_time;        // nach dieser Zeit mal wieder gucken,
                                    // wer mir helfen kann


    /*** Kampf ***/
    struct gun_data gun[ NUMBER_OF_GUNS ];         // 8 Kanonen

    /*** User-Batterien, Maximum wird von Max(s.struct_Bact.) bestimmt ***/
    LONG        BattVehicle;        // derzeitige Energie für Vehicle
    LONG        BattBuilding;       // Batterie fürs Bauen
    LONG        BattBeam;           // Batterie fürs Bauen

    UBYTE       FillModus;          // sollen all diese Batterien gefüllt wern?
    UBYTE       LoadFlags;          // alle Batterien, die gerade aufgeladen werde
    UBYTE       LossFlags;          // alle Batterien, die gerade Energie verlieren
    UBYTE       Pad;
    LONG        AbsReload;          // Energie-Reload pro Zeiteinheit

    /*** AI-Batterien / Sparbuecher ***/
    LONG         BuildSpare;        // Sparbuch nur fuer Baujobs!
    LONG         VehicleSpare;      // Sparbuch nur fuer RoboAngriff

    /*** Beam-Daten ***/
    LONG         BeamInTime;        // Zähler für "Vorbeamen"
    struct flt_triple BeamPos;      // da soll es hingehen
    LONG         beam_fx_time;      // Flackerzaehler


    /*** Spezialzeug zur Roboattacke ***/
    struct roboattacker rattack[ MAXNUM_ROBOATTACKER ];
    LONG   controlattack_time;
    LONG   clearattack_time;
};

/*-------------------------------------------------------------------
**  Definitionen für yparobo_data.flags
*/

#define YRF_VIEW_FLIGHTDIR          1   // versucht immer, sich in Flugrichtung
                                        // zu drehen

#define RWF_SWAY                (1<<0)  // schwanken
#define RWF_ROTATE              (1<<1)  // rotieren

/*** Fillmodi ***/
#define YRF_Fill_System         (1<<0)  // Basisbatterie, also Energy
#define YRF_Fill_Build          (1<<1)  // zum Bauen
#define YRF_Fill_Vehicle        (1<<2)  // für Bewegliches
#define YRF_Fill_Beam           (1<<3)  // Hin und Her
#define YRF_Fill_Nothing        0       // nix aufnehmen
#define YRF_Fill_All            (YRF_Fill_System|YRF_Fill_Build|YRF_Fill_Beam|YRF_Fill_Vehicle)


/*** Die RoboStates. Das sind Flags ähnlich ExtraState speziell für den Robo ***/
#define ROBO_SAFETYZONE     (1L<<0) // meine Sicherheitszone ist erobert
#define ROBO_ROBOFOUND      (1L<<1) // habe erstmal einen Robo gefunden
#define ROBO_DOCKINUSE      (1L<<2) // Dock wird gerade genutzt, frag später nochmal
#define ROBO_SETDOCKTARGET  (1L<<3) // Setze Dock-Ziel nach Erstellung eines G.

#define ROBO_BUILDRADAR     (1L<<4) // baue gerade Radar
#define ROBO_BUILDPOWER     (1L<<5) // baue gerade Kraftwerk
#define ROBO_BUILDCONQUER   (1L<<6) // baue gerade Geschwader für Eroberung
#define ROBO_BUILDDEFENSE   (1L<<7) // baue gerade Geschwader für Verteidigung
#define ROBO_BUILDSAFETY    (1L<<8) // baue gerade Flakstellung

#define ROBO_DEFENSEREADY   (1L<<9) // diese Flags geben an, daß der Test
#define ROBO_CONQUERREADY  (1L<<10) // mindestens einmal nach den Rücksetzen
#define ROBO_RADARREADY    (1L<<11) // durch ist
#define ROBO_POWERREADY    (1L<<12)
#define ROBO_SAFETYREADY   (1L<<13)

#define ROBO_USERROBO      (1L<<14) // Ich bin der Robo des Spielers   HACK!!!
#define ROBO_INDANGER      (1L<<15) // Feinde in der Nähe

#define ROBO_FOUNDPLACE    (1L<<16) // besseren Platz gefunden (...READY)
#define ROBO_CHANGEPLACE   (1L<<17) // bewege mich dorthin     (...BUILD)
#define ROBO_RECONREADY    (1L<<18) // Aufklärung beendet
#define ROBO_BUILDRECON    (1L<<19) // baue Aufklärer
#define ROBO_ROBOREADY     (1L<<20) // Robosuche fertig
#define ROBO_BUILDROBO     (1L<<21) // baue Robobekämpfungseinheiten

#define ROBO_MOVE          (1L<<22) // Um Move auszubremsen wegen Effekten
#define ROBO_USEVHCLSPARE  (1L<<23) // nutze Zusatzbatterie fuer Geschwaderbau

/*-----------------------------------------------------------------------
** Strukturen
*/


struct getcommander_msg {

    ULONG            flags;         // Wonach suchen

    /*** evtl. Ziel ***/
    union {
        struct Bacterium *bact;
        struct Cell *sector; } target;
    ULONG target_type;
    struct flt_triple targetpos;
    ULONG       PrimCommandID;      // wenn 0, dann Bakterienpointer vergleichen

    /*** evtl. CommandID ***/
    ULONG       CommandID;

    /*** evtl. Entfernung zum (oben übergebenen) Hauptziel ***/
    FLOAT       distance;

    /*** Dann wird folgendes zurückgegeben ***/
    struct Bacterium *combact;
    Object           *com;
};

#define GC_COMMANDID        (1L<<0)     // nach CommandID suchen
#define GC_PRIMTARGET       (1L<<1)     // Hauptziel
#define GC_PRIMSECTORDIST   (1L<<2)


struct allocforce_msg {

    LONG            energy;         // gegnerische Stärke
    FLOAT           distance;       // Entfernung zum gegner
    ULONG           aggression;     // für diese Aufgabe brauchen wir Leute
                                    // dieser Aggression

    /*** zu übergebendes Ziel ***/
    ULONG           target_type;
    struct flt_triple target_pos;
    struct Bacterium *target_bact;
    ULONG           commandID;

    /*** Auswahlkriterien / siehe ACF_#? ***/
    ULONG           forbidden;      // das darf nicht sein
    ULONG           necessary;      // notwendig
    ULONG           good;           // wünschenswert
    ULONG           bad;            // besser nicht
    ULONG           job;            // für den neuen Aufruf, siehe unten
};


/*** Charakteristika für vehicleauswahl ***/
#define ACF_UFO             (1L<<0) // UFO-Klasse
#define ACF_HELICOPTER      (1L<<1)
#define ACF_TANK            (1L<<2)
#define ACF_CAR             (1L<<3)
#define ACF_FLYER           (1L<<4)
#define ACF_WEAPON          (1L<<5) // ist bewaffnet
#define ACF_SEARCHMISSILE   (1L<<6) // besitzt Lenkwaffe
#define ACF_BOMB            (1L<<7) // besitzt Bombe
#define ACF_FLYINGHIGH      (1L<<8) // fliegt hoch
#define ACF_AGILE           (1L<<9) // hohes Maxrot
#define ACF_FOOTPRINT      (1L<<10) // hoher Footprint

#define JOB_FIGHTROBO       1L      // bekämpfe Robo
#define JOB_FIGHTTANK       2L      // Kampf gegen Bodenfahrzeuge
#define JOB_FIGHTFLYER      3L      // Kampf gegen hochfliegende Luftzeuge
#define JOB_FIGHTHELICOPTER 4L      // Kampf gegen Hubschrauber
#define JOB_RECONNOITRE     5L      // Aufklärung
#define JOB_CONQUER         6L      // Landeroberung


struct searchrobo_msg {

    ULONG           flags;          // s.u.
    FLOAT           found_x;        // wo haben wir was gefunden
    FLOAT           found_z;
    struct Bacterium *robo;         // wen haben wir gefunden
    struct Bacterium *lastrobo;     // den zuletzt gefundenen
};

#define SR_OWNTERR      (1L<<0)     // suche nur auf eigenem Gebiet
#define SR_NEXT         (1L<<1)     // suche nächstliegenden
#define SR_SUCC         (1L<<2)     // wenn lastrobo sinnvoll, dann Nachfolger
                                    // sonst lh_Head
#define SR_KNOWNTERR    (1L<<3)     // suche auf bekanntem Gebiet

struct externmakecommand_msg {

    ULONG   vproto;                 // wer?
    ULONG   number;                 // wieviel?
    struct  flt_triple pos;         // wo?
    ULONG   flags;                  // wie?
    ULONG   *varray;                // wenn != NULL, dann Prototypen von dort
};

#define EMC_UNUSABLE    (1L<<0)     // setze alle auf "unbrauchbar"
#define EMC_EVER        (1L<<1)     // wird nicht mehr unterstützt

struct bact_message {

    struct  Bacterium *sender;  // wer
    LONG    ID;                 // was
    LONG    para1;              // die Parameter
    LONG    para2;
    LONG    para3;
    LONG    pri;                // die priorität, wird an YWM_LOGMSG weitergegeben
    ULONG   time;               // wird nur von YRM_LOGMSG benötigt
};


/*---------------------------------------------------------------------------
** Maximal (Richt-) Werte für die Objektauswahl des Robos
*/
#define YRF_AC_ENERGY   50000.0
#define YRF_AC_MAXROT     1.2
#define YRF_AC_SPEED    200.0
#define YRF_AC_RANGE    (64.0 * SECTOR_SIZE)
#define YRF_AC_SHIELD    70.0
#define YRF_AC_HEIGHT   200.0
#define YRF_AC_VIEW       1.0



/*---------------------------------------------------------------------------
**  weitere Vereinbarungen
*/
#define VTYPE_DONE            (0xFFFFFFFF)    // Ende für Allow/ForbidVTypes
#define MinimalProjectEnergy  (15000)         // Mindeststärke für Project
#define MaximalProjectEnergy  (200000)        // Maximalstärke für Project
#define YR_MinEnergy          (0.2)           // das bleibt als reserve im Haupttank
                                              // (*max_energy)
#define YR_DeltaDockTime      (2000)          // Docktime nach Objekt
#define YR_DestV              (15000)         // mult. mit dof.v ist das der Abzug
                                              // bei Kollision mit der Welt, sofern
#define YR_DestF              (10)            // das hier nicht mult. mit act_force
                                              // nicht größer ist
#define YR_CreationTimeA      (0.2)           // damit multiplizieren wir max_force und
                                              // haben die "Entstehungsdauer"
#define YR_CreationTimeU      (0.2)           // the same fürn User
#define YR_DANGER_DISTANCE    (2 * SECTOR_SIZE) // in dieser Entf. panic
#define YR_Enemy_Time         (20000)         // für yrd->enemy_time
#define YR_PowerRatio         (0.9)           // bei soviel Ratio KW bauen
#define YR_SecsPerFlak        (10)            // soviel Sektoren für eine Flak
#define YR_SecsPerRadar       (64)            // soviel unbekannte Sektoren für eine Schüssel

/*** kritische Roboenergy ***/
#define CRITICAL_ROBO_ENERGY    10000.0       // dem Robo geht es schlecht, noch
                                              // keine grenze --> YR_MinRoboEnergy
                                              // nur Hilfe für Auswahl
// Faktor für Energie-Verbrauch bei Vehikel-Erzeugung
// Berechnung: faktor = 1 + CREATE_ENERGY_FACTOR * Anzahl_Vehicle
#define CREATE_ENERGY_AUTONOM (0.05)

// bei Usern bleibt es wie es ist
#define CREATE_ENERGY_FACTOR  (2)

// Dauer des Effektes vor dem Beamen
#define BEAM_IN_TIME        (1500)

// BasisWerte für Energy eines Geschwaders zur Bekämpfung von...
// ein normales Vehicle hat 10000, 20000 sind schon gut
#define BASIC_RECON_ENERGY      (10)
#define BASIC_ROBO_ENERGY       (120000)
#define BASIC_CONQUER_ENERGY    (20000)
#define BASIC_DEFENSE_ENERGY    (40000)

/*-----------------------------------------------------------------*/
#endif

