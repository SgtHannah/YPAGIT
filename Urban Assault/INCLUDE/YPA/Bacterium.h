#ifndef YPA_BACTERIUM_H
#define YPA_BACTERIUM_H
/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  Definition der Bakterien-Struktur jetzt abgesplittet
**  von automatus.h.
**
**  (C) Copyright 1995 by Andreas Flemming
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

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

/*-------------------------------------------------------------------
**  ein "gesplitteter Vektor"
*/
struct uvec {
    FLOAT x,y,z;        /* Richtung (Unit-Vektor) */
    FLOAT v;            /* Betrag des Vektors */
};

#ifndef PI
#define PI (3.14159265358979323846)
#endif


/*-------------------------------------------------------------------
**  Eine OBNode hält gleichzeitig einen Pointer auf das Object
**  und die Bakterien-Struktur.
*/
struct OBNode {
    struct MinNode nd;
    Object *o;
    struct Bacterium *bact;
};


/*-------------------------------------------------------------------
**  Physikalische Konstanten der Welt
*/
#define METER_SIZE  (6.0)       /* 1 m == 6 Koordinaten-Einheiten */
#define GRAVITY     (9.80665)   /* Gravitations-Konstante */


/*===================================================================
**  Die <struct Bacterium> wurde erheblich erweitert. Direkt aus
**  der Bakterien-Struktur sind jetzt eine ganze Menge Informationen
**  direkt erhältlich, ohne den Umweg über das Object.
*/
struct Bacterium;       // Prototype

struct BactNode {
    struct MinNode node;
    struct Bacterium *bact;
};


struct extra_viewer {

    /*** Position ist relativ, Matrix absolut ***/
    struct flt_triple relpos;   // Position und
    struct flt_m3x3   dir;      // Ausrichtung

    /*** Denn die Matrix wird jedesmal aus der bact-dir und den Drehwinkeln ermittelt ***/
    FLOAT       horiz_angle;        // Drehung des Viewers
    FLOAT       vert_angle;
    FLOAT       max_up;             // Begrenzung bzgl. der Robo-z-Achse
    FLOAT       max_down;
    FLOAT       max_side;
};    


struct extra_vproto {

    /*** Der Einfachheit halber nur pos und scal. ***/
    FLOAT               scale;
    struct flt_triple   pos; 
    struct flt_m3x3     dir;
    ULONG               flags;
    Object              *vis_proto;       // wie immer
    struct tform        *vp_tform;
};

#define EVF_Active      (1<<0)
#define EVF_Scale       (1<<1)
    

#define PLASMA_ENERGY   (0.2)           // mximal soviel vom maximum
#define PLASMA_TIME             (0.7)           // * maximum soviele millisekunden
#define PLASMA_MINTIME  (10000)         // mindestens jedoch solange
#define PLASMA_MAXTIME  (25000)         // und hoechstens solange
#define PLASMA_SCALE    (0.75)          // beginne mit dieser groesse

struct DestEffect {

    UBYTE       flags;              // s.u.
    UBYTE       proto;              // Prototyp des vehicles, 0 heißt nix!!!
    struct flt_triple relspeed;     // Geschwindigkeit relativ zum lokalen KS
};

#define DEF_Death       (1<<0)  // im Death-Zustand aktivieren
#define DEF_Megadeth    (1<<1)  // im Megadeth-Zustand aktivieren
#define DEF_Create      (1<<2)  //  "
#define DEF_Beam        (1<<3)  //  "
#define DEF_Relative    (1<<4)  // Objektgeschwindigkeit aufaddieren

#define NUM_DESTFX      (16)
#define MAXNUM_VPFX     (32)    // Visprotos für effekte (EXTRA_SPECIALEFFECT)
#define MAXNUM_WAYPOINTS (32)   // zahl der möglichen Wegpunkte
#define MAXNUM_EXTRAVP  (3)     // soviele zusaetzliche VP's kanns geben


#define No_ComID        (0xFFFFFFFF)    // die Situation bezieht sich auf einen
                                        // Sektor, nicht auf ein Geschwader

struct Bacterium {

    /*** --- Verwaltung --- ***/
    struct MinNode SectorNode;      // zum Einklinken in Sektor-Liste
    Object *BactObject;             // in dieses Object eingebettet (bact.class)
    LONG   SectX, SectY;            // ergibt sich aus <pos>
    struct Cell *Sector;            // Pointer auf SectX/SectY-Sektor
    FLOAT  WorldX;                  // die korrekten Weltgrößen. Werden in 
    FLOAT  WorldZ;                  // ObtainObject gesetzt!!!
    WORD   WSecX;                   // Weltgröße in Sectoren
    WORD   WSecY;
    ULONG  BactClassID;
    ULONG  ident;                   // globaler Identifier
    UBYTE  TypeID;                  // Offset im WC_Array VT_...
    UBYTE  backup_flags;            // merken, wenn woanders was ausg. wurde
    ULONG  CommandID;               // Kennung, wenn ich Geschw.Führer bin

    /*** --- Geschwaderbildung --- ***/
    Object *robo;                   // wird von roboclass nach obtainvehicle gesetzt
    Object *master;                 // 1L für Welt-Object, sonst ypabact.class
    struct Bacterium *master_bact;  // NULL bei Welt-Object!
    struct MinList slave_list;
    struct OBNode slave_node;


    /*** --- Audio --- ***/
    struct SoundCarrier sc;         // Struktur für Soundzeug
    ULONG  sound;                   // hier merken wir uns die kontinuierlichen
                                    // Sounds in Bits (1L << VP_NOISE....)
    LONG   bu_volume;               // für NORMAL-Zustand was zum merken
    LONG   bu_pitch;
    FLOAT  max_pitch;

    /*** --- Energie und deren Verluste --- ***/
    LONG   Energy;
    LONG   Maximum;
    LONG   RoboReloadConst;         // Energie-Aufladungs-Richtwert fuer Robos statt Maximum
    WORD   Transfer;                // nur, damit World-Class erstmal compiliert
    UBYTE  Shield;                  // prozentualer Rüstungsschutz

    /*** --- Strategie --- ***/
    UBYTE FWay;                     // wie erfolgt der Anflug, drüber, zurück oder zufall
    UBYTE View;                     // Sichtweite in Sektoren -->FootPrint
    UBYTE Owner;                    // 1..8
    UBYTE Aggression;               // Aggression  0 ... 100
    UBYTE MainState;                // Zustand, siehe Action Types
    ULONG ExtraState;               // beschreibt MainState genauer
    FLOAT ManValue;                 // ManöverWert, um gewisse Aktionen zeitlich
                                    // auszudehnen

    /*** --- Ziele --- ***/
    UBYTE PrimTargetType;           // siehe Target Types
    UBYTE SecTargetType;            // siehe Target Types
    ULONG PrimCommandID;            // um Geschwader nicht zu verlieren
    ULONG SecCommandID;             // nur für Messagesystem!

    union {
        struct Cell *Sector;
        struct Bacterium *Bact;
    } PrimaryTarget;
    struct flt_triple PrimPos;

    union {
        struct Cell *Sector;
        struct Bacterium *Bact;
    } SecondaryTarget;
    struct flt_triple SecPos;
    FLOAT  adist_sector;            // früher ABSTAND_SECTOR
    FLOAT  adist_bact;              // früher ABSTAND_BACT
    FLOAT  sdist_sector;            // hier wird abgebremst und evtl.
    FLOAT  sdist_bact;              // zurückgeflogen

    struct flt_triple waypoint[ MAXNUM_WAYPOINTS ]; // Position der Wegpunkte
    WORD   count_waypoints;         // derzeit aktueller Wegpunkt
    WORD   num_waypoints;           // Zahl der Wegpunkte
    ULONG  mt_commandid;            // wenn das != 0 ist, dann ist das letzte
    UBYTE  mt_owner;                // wegpunktziel ein Bacterium!


    LONG   found_enemy;             // CommandID für zuletzt gefundenes
                                    // Feindgeschwader für meldungen
    LONG   fe_time;                 // noch ein zeitindex dafür

    /*** --- physikalische Eigenschaften --- ***/
    FLOAT mass;             // Masse des Objects in kg
    FLOAT max_force;        // maximale Schubkraft in N (kg*m*s^-2)
                            // Richtung der Schubkraft abhängig von Flug-Modell
    FLOAT air_const;        // Luftwiederstands-Konstante in kg*s^-1
    FLOAT bu_air_const;     // Speicher, denn bei Manövern wird ac hochgesetzt
    FLOAT max_rot;          // Maximale Drehung um eine Achse


    /*** --- Low-Level-Geometrics --- ***/
    struct extra_viewer Viewer;     // Flag ExtraViewer entscheidet
    FLOAT  act_force;               // aktuell eingestellte Schubkraft
    struct uvec dof;                // Direction Of Flight
                                    // dof.v = Geschw. in (m*s^-1)
    struct flt_triple accel;        // fuer Netzwerk

    struct flt_triple pos;          // aktuelle Global-Position
    struct flt_triple old_pos;      // BackUp für direkte Kollision
    struct flt_triple tar_vec;      // 3D-Vector auf Target
    struct flt_triple tar_unit;     // tar_vec als Unit-Vektor
    struct flt_m3x3 dir;            // aktuelle Orientation (!= dof!!!)
    FLOAT  pref_height;             // bevorzugte Flughöhe, positiv(!) über 
                                    // Sektorhöhe
    FLOAT  max_user_height;         // Maximum der Höhe für den User
    FLOAT  scale_x;                 // für'n CREATE-Zustand, zum Aufblasen
    FLOAT  scale_y;
    FLOAT  scale_z;
    
    /*** --- Visuelles --- ***/
    Object *vis_proto_normal;       // wie immer
    struct tform *vp_tform_normal;
    Object *vis_proto_fire;         // schießend (Bordkanone)
    struct tform *vp_tform_fire;
    Object *vis_proto_wait;         // wartend, Antrieb aus
    struct tform *vp_tform_wait;
    Object *vis_proto_dead;         // tot mit brennen etc.
    struct tform *vp_tform_dead;
    Object *vis_proto_megadeth;     // tot nach Aufschlag
    struct tform *vp_tform_megadeth;
    Object *vis_proto_create;       // tot nach Aufschlag
    struct tform *vp_tform_create;
    ULONG  vpactive;                // derzeitiger visueller Zustand 
    struct extra_vproto extravp[ MAXNUM_EXTRAVP ];
    ULONG  extravp_logic;           // wie sind die VPs fuer die Logic zu interpretieren?

    struct DestEffect DestFX[ NUM_DESTFX ]; // Effekte bei zerstörung

    /* -------------------------------------------------------------------------
    ** die direkten Werte für die Kollision und Bodenanpassung etc. sind derzeit
    ** wegen der testphasen nach Viewer und Nichtviewer getrennt. Evtl. werden
    ** diese später durch allgemeingültige Konstanten oder Faktoren ersetzt.
    ** -----------------------------------------------------------------------*/
    FLOAT  radius;                  // ungefährer Objektradius für Koll.
    FLOAT  viewer_radius;           // ungefährer Objektradius für Koll.(Viewer)
    FLOAT  over_eof;                // Auch fliegende Objekte müssen mal landen!
    FLOAT  viewer_over_eof;         // für den Viewer

    struct flt_triple relpos;       // Relativ-Position zu Sektor

    /*** TForm für Transformer-Engine ***/
    struct tform tf;                // zur Zeit noch notwendig

    /*** Zeitaufteilung ***/
    LONG   internal_time;           // Alle zeiten beziehen sich nicht auf die
                                    // Welt- sondern die eigene global_time!!!
    LONG   time_ai1;                // Diese Werte merken sich, wann die zuge-
    LONG   time_ai2;                // hörige Routine das letzte Mal aufgerufen
    LONG   time_test1;              // wurde
    LONG   time_test2;              // ai: Koordinatenaufnahme, test: Eignung
    LONG   time_search2;            // für Ziel, search: Teilzielsuche Bact
    LONG   time_search3;            // für Ziel, search: Teilzielsuche Sector
    LONG   scale_time;              // Zähler für Scalierungseffekte
    LONG   bf_time;                 // BreakFree-Test?
    LONG   bf2_time;                // interner Zähler
    LONG   bfb_time;                // solange keine Bactcollision
    LONG   newtarget_time;          // nach dieser zeit im Wartezustand Ziele suchen
    LONG   assess_time;             // nach dieser zeit Ziel wieder überpr.
    LONG   wait_for_coll;           // wird zur Ausbremsung von bewegungen genutzt (der-
                                    // zeit nur tanks)

    LONG   sliderx_time;            // für Trägheitseffekt bei Handsteuerung
    LONG   slidery_time;
    LONG   dead_entry;              // da wurde ich gekillt. Damit ich nicht zu lange als Leiche rumhänge...
    LONG   startbeam;               // da betrat der Robo das Tor...
    LONG   time_energy;             // nach dieser zeit Energieneuberechnung (wegen Rundungsfehlern)

    struct flt_triple mpos;

    /*** Bewaffnung ***/
    UBYTE   auto_ID;                // Typ der Waffen
    UBYTE   auto_info;              // Kopie der Flags aus dem Prototyp
    UBYTE   mg_shot;                // Proto-ID des Einschusses, ist eine Rakete.
                                    // 0xFF heißt kein MG!!
    UBYTE   num_weapons;            // soviele Waffen werden verteilt von -x
                                    // nach +x abgeschossen
    struct  MinList auto_list;      // alle Waffensysteme, die zu triggern sind
    ULONG   last_weapon;            // GlobalTime des letzten Schusses
    struct  flt_triple firepos;     // relative Abschußposition
    FLOAT   gun_angle;              // Summand für y-Komponente der Abschuß-
    FLOAT   gun_angle_user;         // richtung. Bei Tank modifizierbar
    FLOAT   gun_leftright;          // Kanone links und rechts, Joystick only
    FLOAT   gun_radius;             // Breite des "Strahls", sollte klein sein
    FLOAT   gun_power;              // Die Energie ziehe ich anderen pro zeit ab
    ULONG   mg_time;                // letztes Einschußloch
    ULONG   salve_count;            // Zaehler fuer Salve
    BOOL    kill_after_shot;        // ich sterbe nach dem ersten Schuss

    /*** Handsteuerung ***/
    FLOAT   HC_Speed;               // Schnelligkeit von Richtungsänderungen

    /*** Diverses ***/
    struct  Bacterium *killer;      // Dort hinterläßt der Killer sich
    UBYTE   killer_owner;
    WORD    reccount;               // Abprallzähler für crashende vehicle
    LONG    at_ret;                 // Merkbuffer für Zielart --> assess_time

    /*** Netzwerk ***/
    LONG    last_frame;             // da war letztes Update
    struct  flt_m3x3   d_matrix;    // Änderung der Matrix
    struct  flt_m3x3   old_dir;     // Für Änderung der Matrix
    struct  flt_triple d_speed;     // Änderung der Geschwindigkeit

    /*** SpecialEffext ***/
    FLOAT   scale_start;
    FLOAT   scale_speed;            // Änderung des Factors pro sec.
    FLOAT   scale_accel;            // Änderung des scale_speeds pro sec. (Add!)
    LONG    scale_duration;         // Dauer der Änderung.
    LONG    scale_count;            // zählt von 0 bis duration
    LONG    scale_delay;            // Soweit wird das Scalen verzögert
    Object *vp_proto_fx[ MAXNUM_VPFX ];       // wie immer
    struct tform *vp_tform_fx[ MAXNUM_VPFX ];
};

/*-------------------------------------------------------------------
**  KONSTANTEN
*/

/*** Hauptzustände, exclusiv ***/

#define ACTION_NOP              (0)     // nicht definiert
#define ACTION_NORMAL           (1)     // Normal-Zustand
#define ACTION_DEAD             (2)     // inaktiv
#define ACTION_WAIT             (3)     // kein Ziel, gestattet Füllaktionen
#define ACTION_CREATE           (4)     // im Dock, nur für Commander
#define ACTION_BEAM             (5)     // werde gerade weggeschleudert

/*** Visueller Zustand ***/

#define VP_JUSTNORMAL           (1)
#define VP_JUSTDEATH            (2)
#define VP_JUSTMEGADETH         (3)
#define VP_JUSTCREATE           (4)
#define VP_JUSTBEAM             (5)
#define VP_JUSTWAIT             (6)
#define VP_JUSTFIRE             (7)

/*** Nebenzustände, inclusiv ( # - not longer supported!) ***/

#define EXTRA_FIGHTP            (1L<<0)    // PrimT bekämpfend
#define EXTRA_FIGHTS            (1L<<1)    // SecT bekämpfend
#define EXTRA_FORMATION         (1L<<2)    // Formation bildend
#define EXTRA_MANEUVER          (1L<<3)    // Ausweichverhalten  #
#define EXTRA_LEFT              (1L<<4)    // Ausweichen
#define EXTRA_RIGHT             (1L<<5)    // Ausweichen
#define EXTRA_MOVE              (1L<<6)    // Bewegen
#define EXTRA_OVER              (1L<<7)    // nach oben fliegen!
#define EXTRA_FIRE              (1L<<8)    // schießend
#define EXTRA_LANDED            (1L<<9)    // Landevorgang ist abgeschlossen
#define EXTRA_LOGICDEATH        (1L<<10)   // bereits korrekt abgemeldet, 
                                           // logischer Tod eingetreten
#define EXTRA_MEGADETH          (1L<<11)   // Aufschlag-VP
#define EXTRA_EMPTYAKKU         (1L<<12)   // Bin zur zeit erschöpft  #
#define EXTRA_APPROACH          (1L<<13)   // im Zielanflug/suchen pos.
#define EXTRA_ESCAPE            (1L<<14)   // ich fliehe zur Zeit
#define EXTRA_HC_XLEFT          (1L<<15)   // der SliderX zeigt zur zeit nach l.
#define EXTRA_HC_YUP            (1L<<16)   // der SliderY zeigt zur zeit nach o.
#define EXTRA_BACTCRASH         (1L<<17)   // bin mit einem Bacterium zamnandergestoßen
                                           // bzw. bin noch nicht weg --> für CrashSound
#define EXTRA_LANDCRASH         (1L<<18)   // och so, ne
#define EXTRA_UNUSABLE          (1L<<19)   // darf nicht verwendet/verplant werden
#define EXTRA_SCALE             (1L<<20)   // die Skalierung soll in Trigger aus-
                                           // gewertet werden
#define EXTRA_SHAKE             (1L<<21)
#define EXTRA_DONTRENDER        (1L<<22)   // nicht zeichnen, vor allem für
                                           // tote netzwerk-Viewer
#define EXTRA_ISVIEWER          (1L<<23)   // ist Viewer auf anderer Maschine
                                           // setzen durch extra Message
#define EXTRA_SPECIALEFFECT     (1L<<24)   // andere Behandlung in CRASHBACT.
#define EXTRA_CLEANUP           (1L<<25)   // ich bin in einer Aufräumphase, es
                                           // sollten also nicht die TodMessages
                                           // u.a. gezeigt werden
#define EXTRA_DOINGWAYPOINT     (1L<<26)   // bin im Modus mit mehreren Wegpunkten
#define EXTRA_WAYPOINTCYCLE     (1L<<27)   // WaypointModus Cycle
#define EXTRA_NOMESSAGES        (1L<<28)   // keine Meldungen für zerstörte geschwader
#define EXTRA_SETDIRECTLY       (1L<<29)   // hat nur im Netzwerk sinn, sagt, dass nicht
                                           // interpoliert wird, wird von InsertVD sofort wieder zurueckgesetzt
#define EXTRA_ATTACK            (1L<<30)   // wir fahren auf ein zu bekaempfendes Ziel zu 

/*** Bacterium.Prim/SecTargetType ***/
#define TARTYPE_NONE            (0)     // aktuell kein Target
#define TARTYPE_SECTOR          (1)     // Target ist Sektor
#define TARTYPE_BACTERIUM       (2)     // Target ist Bacterium
#define TARTYPE_FORMATION       (3)     // Target ist Punkt in Formation
#define TARTYPE_SIMPLE          (4)     // tar_unit hat die Richtung und darf
                                        // nicht geändert werden!
/*** Wenn das Hauptziel nicht an die Slaves weitergegeben werden soll...***/
#define TARTYPE_SECTOR_NR       (5)     // non rekursiv
#define TARTYPE_BACTERIUM_NR    (6)

/*** Mögliche FWay's ***/
#define FlyBack             (1)         // fliegt Ziel an und dann wieder zurück
#define FlyOver             (2)         // fliegt drüber weg um Anlauf zu nehmen
#define FlyDifferent        (3)         // wechselt zufällig zwischen beiden Modi


/*** Konstanten für Motorgeräusche ***/
#define MSF_HELICOPTER      (1.2)
#define MSF_TANK            (1.1)
#define MSF_FLYER           (1.2)
#define MSF_UFO             (1.2)


/*** Flags, wie die extravp's fuer die Logik zu interpretieren sind ***/
#define EVLF_PLASMA         (1)         // es ist ein Plasmaeffekt
#define EVLF_BEAM           (2)         // es ist ein Robobeameffekt

/*** Wie lange kann ich maximal tot sein ***/
#define DEATH_TIME          5000        // zu bact.dead_entry

#endif


