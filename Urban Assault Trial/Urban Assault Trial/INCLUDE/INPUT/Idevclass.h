#ifndef INPUT_IDEVCLASS_H
#define INPUT_IDEVCLASS_H
/*
**  $Source: PRG:VFM/Include/input/idevclass.h,v $
**  $Revision: 38.11 $
**  $Date: 1998/01/06 12:59:29 $
**  $Locker: floh $
**  $Author: floh $
**
**  Die idev.class ist Superklasse aller primitiven
**  Input-Device-Treiber-Klassen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef NUCLEUS_NUCLEUS2_H
#include "nucleus/nucleus2.h"
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef INPUT_INPUT_H
#include "input/input.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      idev.class -- Superklasse aller Input-Treiber-Klassen
**                    Subklasse der nucleus.class
**
**  FUNCTION
**      Dient als einheitliches Interface für die Bereitstellung
**      von Input-Information. Für jede spezielle Input-Hardware
**      existiert eine Subklasse der idev.class, welche diese
**      Hardware in ein Universal-Protokoll kapselt.
**
**      Um die Input-Informationen möglichst universell 
**      darzustellen, arbeitet die idev.class mit 3 "virtuellen"
**      Gerätetypen: Slidern, Buttons und Keys.
**
**      Ein Slider ist ein 1-dimensionaler Zahlenbereich von
**      -1.0 bis +1.0, die minimale Auflösung entspricht der
**      Genauigkeit einer FLOAT Zahl (32 Bit IEEE).
**
**      Ein Button ist ein einfacher Binär-Schalter,
**      bis zu 32 Buttons können parallel betätigt sein.
**
**      Eine Key ist eine normale Taste, entweder entprellt
**      oder "roh". Es kann immer nur 1 Taste betätigt sein.
**
**      Aufgabe einer speziellen Subklasse der idev.class ist
**      es, ein reales Eingabegerät auf eine Anzahl von
**      Slidern und Buttons abzubilden. Ein normaler 2-Tasten-
**      Analog-Joystick würde z.B. 2 Slider und 2 Buttons
**      bereitstellen.
**
**      Einem einzelnen Objekt kann aber immer nur genau 1 Eingabe-
**      Element zugeordnet werden (1 Button oder 1 Slider). 
**      Das Objekt ist dann auf dieses Element fixiert und
**      "horcht" nur noch auf Aktivierung dieses Elements.
**      Das hat sich nach n Anläufen als die einfachste
**      und sogar effizienteste Methode erwiesen. Also
**      keine Diskussion ;-)
**
**  METHODS
**      IDEVM_GETBUTTON
**          Msg:    struct idev_status_msg
**          Ret:    ---
**
**          Falls das Objekt auf einen Button gemappt wurde,
**          erhält man durch Anwendung dieser Methode den
**          Status des Buttons zurück. TRUE heißt "Button gedrückt",
**          sonst FALSE.
**
**          Ist das Objekt nicht auf einen Button gemappt, kommt
**          immer FALSE zurück.
**
**      IDEVM_GETSLIDER
**          Msg:    struct idev_status_msg
**          Ret:    ---
**
**          Falls das Object auf einen Slider gemappt wurde,
**          schreibt es den Zustand des Sliders nach msg.status.
**          Falls das Objekt nicht auf einen Slider gemappt ist,
**          wird immer 0.0 zurückgeschrieben.
**
**          Falls das Element auf einen Button gemappt wurde,
**          wird 0.0 in die msg geschrieben, wenn der
**          Button nicht gedrückt ist, sonst 1.0. Damit kann
**          ein Button einen Slider simulieren.
**
**      IDEVM_GETKEY
**          Msg:    struct idev_getkey_msg
**          Ret:    --- 
**
**          Falls die entsprechende Subklasse der idev.class 
**          Tastatur-Information anbietet, kann man per
**          IDEVM_GETKEY den Status der Tastatur abfragen.
**          In die Msg wird die aktuelle ContKey und NormKey
**          zurückgeschrieben, die ContKey bleibt solange
**          gültig, wie die Taste tatsächlich gedrückt ist,
**          die NormKey wird spätestens nach der 1.Abfrage gelöscht
**          (entprellt)
**
**      IDEVM_SETHOTKEY
**          Msg:    struct idev_sethotkey_msg
**          Ret:    ULONG [IDEVTYPE_NONE | IDEVTYPE_HOTKEY]
**
**          Falls das Objekt Tastatur-Information anbietet (per
**          IDEVM_GETKEY) sollte es auch IDEVM_SETHOTKEY-Methoden
**          akzeptieren. Dem Objekt wird hiermit mitgeteilt,
**          welcher Input-Code (siehe IDEVM_SETID) als
**          Hotkey aufgefasst werden soll, und natürlich,
**          welche Hotkey gemeint ist. Wenn der Input-Code
**          vom Objekt akzeptiert wurde, sollte es
**          bei IDEVM_GETKEY das <hotkey> Feld ausfüllen
**          wenn die Hotkey gedrückt wurde.
**          Die Methode returniert auf IDEVTYPE_NONE, wenn die
**          Hotkey-Nummer zu groß ist (>= IDEV_NUMHOTKEY).
**
**      IDEVM_SETID
**          Msg:    struct idev_setid_msg
**          Ret:    ULONG
**
**          Definiert den Input-Code, auf welches das
**          Object "hören" soll. Der Code wird definiert
**          durch einen Klasse-spezifischen String (z.B. "lalt").
**          Das Object returniert den Typ des Strings:
**
**              IDEVTYPE_NONE   - unbekannt
**              IDEVTYPE_BUTTON - es handelt sich um einen Button
**              IDEVTYPE_SLIDER - es handelt sich um einen Slider
**
**          Das Object wird dann auf IDEVM_GETBUTTON bzw. IDEVM_GETSLIDER
**          entsprechend seinem aktuellen Status antworten.
**
**      IDEVM_RESET
**          Msg:    struct idev_reset_msg
**          Ret:    ULONG
**
**          Universelle "Reset-Methode", zum Beispiel zur
**          Rekalibrierung. Es ist Sache der konkreten Subklassen,
**          ob sie die Methode unterstützen, oder ignorieren.
**
**      IDEVM_QUERYHOTKEY
**          Msg:    struct idev_queryhotkey_msg
**          Ret:    ---
**
**          In der Msg wird entweder eine Hotkey-Nummer
**          oder ein Keycode übergeben, zurückgeliefert
**          wird der jeweils korrespondierende Wert
**          oder 0, falls keine passende Hotkey-Definition
**          existiert.
**
**      IDEVM_FFCONTROL
**          Msg:    struct idev_ffcontrol_msg
**          Ret:    ---
**
**          Forcefeedback-Control-Methode. Siehe <idev_ffcontrol_msg>
**          für mehr Info.
**
**  ATTRIBUTES
**
**      IDEVA_Debug     (IS) [BOOL=FALSE]
**          Wenn TRUE, aktiviert die Klassen ihren
**          Debug-Modus, wie immer der auch aussieht.
*/

/*-----------------------------------------------------------------*/
#define IDEV_CLASSID    "idev.class"
/*-----------------------------------------------------------------*/
#define IDEVM_BASE      (OM_BASE+METHOD_DISTANCE)

#define IDEVM_GETBUTTON     (IDEVM_BASE)
#define IDEVM_GETSLIDER     (IDEVM_BASE+1)
#define IDEVM_GETKEY        (IDEVM_BASE+2)
#define IDEVM_SETID         (IDEVM_BASE+3)
#define IDEVM_SETHOTKEY     (IDEVM_BASE+4)
#define IDEVM_RESET         (IDEVM_BASE+5)
#define IDEVM_QUERYHOTKEY   (IDEVM_BASE+6)
#define IDEVM_FFCONTROL     (IDEVM_BASE+7)

/*-----------------------------------------------------------------*/
#define IDEVA_BASE      (OMA_BASE+ATTRIB_DISTANCE)

#define IDEVA_Debug         (IDEVA_BASE)

/*-------------------------------------------------------------------
**  Misc Stuff
*/
#define IDEVTYPE_NONE       (0)
#define IDEVTYPE_BUTTON     (1)
#define IDEVTYPE_SLIDER     (2)
#define IDEVTYPE_HOTKEY     (3)

#define IDEV_NUMHOTKEYS (48)

struct idev_status_msg {
    ULONG time_stamp;       // aktueller Timestamp
    ULONG btn_status;       // TRUE -> Button gedrückt
    FLOAT sld_status;       // [-1.0 .. +1.0]
};

struct idev_sethotkey_msg {
    UBYTE *id;
    ULONG hotkey;           // 1..IDEV_NUMHOTKEYS [0 wird ignoriert]
};

struct idev_queryhotkey_msg {
    LONG keycode;           // Keycode, oder 0, falls ungültig
    LONG hotkey;            // Hotkey, oder 0, falls ungültig
};

struct idev_getkey_msg {
    ULONG cont_key;
    ULONG norm_key;
    ULONG hot_key;          // Bit 7 gesetzt!, Rest ist Hotkey-Nummer
    ULONG ascii_key;        // auf ASCII gemappte Key
};

struct idev_setid_msg {
    UBYTE *id;              // ID-String
};

struct idev_reset_msg {
    ULONG rtype;
};

#define IRTYPE_NOP         (0)
#define IRTYPE_CENTER      (1)
#define IRTYPE_RECAL_RIGHT (2)
#define IRTYPE_RECAL_LEFT  (3)
#define IRTYPE_RECAL_UP    (4)
#define IRTYPE_RECAL_DOWN  (5)

struct idev_ffcontrol_msg {
    LONG type;              // siehe unten
    LONG mode;              // siehe unten
    FLOAT power;            // Stärke/Magnitude des Effekts [0.0 .. 1.0]
    FLOAT pitch;            // Frequenz-Verschiebung [0.0 .. 1.0]
    FLOAT dir_x,dir_y;      // Richtung, aus der der Effekt kommt
};

#define IDEV_FFMODE_START       (0)
#define IDEV_FFMODE_END         (1)
#define IDEV_FFMODE_MODIFY      (2)

#define IDEV_FFTYPE_ALL             (0)
#define IDEV_FFTYPE_ENGINE_TANK     (1)     // Panzer-Motor
#define IDEV_FFTYPE_ENGINE_PLANE    (2)     // Flugzeug-Engine
#define IDEV_FFTYPE_ENGINE_HELI     (3)     // Helikopter
#define IDEV_FFTYPE_MAXROT          (4)
#define IDEV_FFTYPE_MGUN            (5)
#define IDEV_FFTYPE_MISSLAUNCH      (6)
#define IDEV_FFTYPE_GRENLAUNCH      (7)
#define IDEV_FFTYPE_BOMBLAUNCH      (8)
#define IDEV_FFTYPE_COLLISSION      (9)
#define IDEV_FFTYPE_SHAKE           (10)

struct idev_xy {
    WORD x,y;
};

/*** Support-Struktur für Button- und Slider-Remapping ***/
struct idev_remap {
    UBYTE *id;          // ID-String des Buttons
    ULONG type;         // IDEVTYPE_#?
    LONG code;          // interner ID-Code
    ULONG status;       // aktueller Status des Elements
};

/*-----------------------------------------------------------------*/
#endif

