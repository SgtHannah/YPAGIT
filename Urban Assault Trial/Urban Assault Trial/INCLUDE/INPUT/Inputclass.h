#ifndef INPUT_INPUTCLASS_H
#define INPUT_INPUTCLASS_H
/*
**  $Source: PRG:VFM/Include/input/inputclass.h,v $
**  $Revision: 38.4 $
**  $Date: 1996/08/31 00:34:23 $
**  $Locker:  $
**  $Author: floh $
**
**  input.class Header
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_INPUT_H
#include "input/input.h"
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      input.class -- zentrale Input-Collector-Klasse
**
**  FUNCTION
**      Ein input.class Object ist der einzelne zentrale Input-
**      Collector.
**      Es erzeugt alle benötigten systemspezifischen
**      Input-Provider-Objekte über ein einfaches und offenes
**      String-Protokoll und bündelt die einzelnen Input-Quellen
**      in eine einzelne Zugriffs-Methode.
**
**      Die Abfrage des Input-Status erfolgt durch die
**      Methode IM_GETINPUT. Dieser Methode wird ein
**      Pointer auf eine auszufüllende VFMInput-Struktur
**      übergeben. Dann geht's etwa so weiter:
**
**      1) Mit Hilfe des TimeSource-Objects die verstrichene
**         Zeit seit letztem Aufruf ermitteln.
**      2) Per IWIMPM_HASFOCUS auf das WIMP-Handler-Objekt
**         ermitteln, ob die Applikation gerade den Input-
**         Focus besitzt.
**      3) Falls Input-Focus:
**          3.1) Keyboard-Information abfragen.
**          3.2) WIMP-Information abfragen [aka GUI-Info ;-)]
**          3.3) die Input-Xpression-Lists aller Buttons auswerten
**          3.4) die Input-Xpression-Lists aller Sliders auswerten
**
**      Input Xpressions
**      ~~~~~~~~~~~~~~~~
**      Ein input.class Object verwaltet bis zu 32 "Soft-Buttons"
**      und "Soft-Sliders". Ein Soft-Button ist ein simpler
**      nicht entprellter Binär-Switch, ein Soft-Slider wird
**      durch eine FLOAT-Zahl im Bereich [-1.0 .. +1.0] definiert.
**
**      Input Expressions definieren, wie die Input-Hardware
**      auf einen Soft-Button oder Soft-Slider abgebildet werden.
**      Es können beliebig viele "reale" Eingabe-Elemente auf
**      einen einzigen Soft-Button/Slider gemappt werden, dabei
**      beschreibt die Input Expression, welche Real-Elemente
**      benutzt werden, und wie diese zu verknüpfen sind.
**
**      Eine IX (Input Expression) wird aus folgenden
**      Tokens aufgebaut:
**
**      "driver:id"  - Beschreibt ein Eingabe-Element durch den
**                     Drivertyp, dann durch Doppelpunkt getrennt
**                     eine Driver-spezifische ID, die das Element
**                     genau definiert. Das Eingabe-Element kann
**                     ein Button oder Slider sein. Der Drivertyp
**                     muß eine Klasse in "drivers/input/" beschreiben
**                     (<driver> ist Klassen-Name, ohne ".class" Postfix).
**
**      "&" - definiert, daß der Status des nächsten Eingabe-Elements
**            mit dem bisherigem Ergebnis AND-verknüpft wird.
**      "|" - definiert, daß der Status des nächsten Eingabe-Elements
**            mit dem bisherigem Ergebnis OR-verknüpft wird.
**      "~" - invertiert den Status des nächsten Eingabe-Elements
**            (bei Buttons NOT, bei Sliders NEG)
**      "§" - das nächste Eingabe-Element ist ein Button, soll aber
**            als Slider behandelt werden.
**
**      Special Feature:
**      Ein Soft-Slider wird nur aktiviert, wenn das Ergebnis
**      aller Hard-Buttons in der IX TRUE ist.
**      Treten mehrere Slider in einer Input-Expression
**      auf, werden diese generell aufaddiert.
**
**      Ein paar fiktive Input-Expressions:
**
**      "dosmouse:lmb & dosmouse:rmb dosmouse:x"
**      Die X-Maus-Achse wird benutzt, das Ergebnis ist aber nur
**      gültig, wenn die linke UND die rechte Maustaste nieder-
**      gedrückt ist.
**
**      "dosmouse:y"
**      Die Y-Maus-Achse wird in jedem Fall benutzt
**
**      "dosmouse:lmb|dosmouse:rmb dosmouse:x"
**      LMB ODER RMB müssen gedrückt sein, dann wird die X-Achse
**      der Maus durchgeschleift.
**
**      "~dosmouse:mmb & ~dosmouse:rmb dosjoyst:y"
**      MMB und RMB dürfen NICHT gedrückt sein, dann wird die Y-Achse
**      des Joysticks benutzt.
**
**      "~doskeyb:lalt & ~doskeyb:ctrl ~§doskeyb:left §doskeyb:right"
**      Die linke Alt-Taste und die Ctrl-Taste dürfen nicht
**      gedrückt sein, die linke und rechte Cursor-Tasten "simulieren"
**      einen Slider.
**
**
**  METHODS
**      IM_SETHANDLER
**          Msg:    struct inp_handler_msg
**          Ret:    ULONG [TRUE/FALSE]
**
**          Hiermit wird eines der Treiber-Objekte,
**          die ein input.class Objekt intern erzeugt,
**          definiert. Angegeben wird, welches Object
**          (WIMP-, Timer-, Keyboard, Button- oder Slider-
**          Handler), bei Button- und Slider-Objects
**          zusätzlich eine Nummer [je 0..31], und
**          ein ID-String, der das zu benutzende Gerät
**          beschreibt.
**
**          Der ID-String ist bei WIMP-, Timer- und
**          Keyboard ein einzelner String, der sofort
**          als Klassen-Name verwendet wird, um das
**          Objekt zu erzeugen.
**
**          Bei Button- und Slider-Objects ist es eine
**          Input-Expression, wie oben beschrieben.
**
**          WIMP-Object: bitte beachten, daß IM_SETHANDLER *NICHT*
**          IWIMPA_Environment initialisiert, das muß per IM_DELEGATE
**          von außen erledigt werden!
**
**      IM_GETINPUT
**          Msg:    struct VFMInput
**          Ret:    ---
**
**          Die fette Input-Collector-Methode. Die Message wird
**          "nach bestem Wissen und Gewissen" ausgefüllt.
**
**      IM_DELEGATE
**          Msg:    struct inp_delegate_msg
**          Ret:    Return-Value der aufgerufenen Methode
**
**          Delegiert eine Methode auf eines der eingebetteten
**          Objects. Das Ziel-Object sollte mit der Methode
**          natürlich auch was anfangen können. Einfach
**          mal die inp_delegate_msg angucken, dann wird alles
**          klar.
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define INPUT_CLASSID   "input.class"
/*-----------------------------------------------------------------*/
#define IM_BASE       (OM_BASE+METHOD_DISTANCE)

#define IM_SETHANDLER (IM_BASE)
#define IM_GETINPUT   (IM_BASE+1)
#define IM_DELEGATE   (IM_BASE+2)

/*-----------------------------------------------------------------*/
#define IA_BASE       (OMA_BASE+ATTRIB_DISTANCE)

/*-------------------------------------------------------------------
**  Support-Strukturen und Konstanten
*/

/*** Anzahl unterstützter Buttons und Slider ***/
#define INUM_BUTTONS    (32)
#define INUM_SLIDERS    (32)

/*** Objekt-ID für die eingebetteten Input-Handler-Objects ***/
#define ITYPE_NONE      (0)     // wird ignoriert
#define ITYPE_WIMP      (1)     // der WIMP (aka GUI) Handler
#define ITYPE_TIMER     (2)     // TimeSource-Object
#define ITYPE_KEYBOARD  (3)     // das Keyboard-Handler-Object
#define ITYPE_BUTTON    (4)     // ein Button-Object
#define ITYPE_SLIDER    (5)     // ein Slider-Object

/*** folgende Strukturen für die Expression-Liste für jedes Inp-Element ***/
struct IXHeader {
    struct MinList ls;
};

struct IXToken {
    struct MinNode nd;      // mehrere Input-Tokens verkettbar
    Object *o;              // dieses Object wird gefragt
    ULONG flags;            // siehe unten
    ULONG type;             // (IDEVTYPE_BUTTON || IDEVTYPE_SLIDER)
    UBYTE id[32];           // Device Driver ID (String)
    UBYTE sub_id[32];       // Driver-spezifische Sub-ID (String)
};

#define IXF_And         (1<<0)      // Token '&'
#define IXF_Or          (1<<1)      // Token '|'
#define IXF_Invert      (1<<2)      // Token '~'
#define IXF_ForceSlider (1<<3)      // Token '§'

struct IXResult {
    ULONG bool_res;         // Boolsches Resultat aller IDEVTYPE_BUTTON
    FLOAT flt_res;          // Resultat aller Slider
};

/*-------------------------------------------------------------------
**  LID der input.class
*/
struct input_data {
    ULONG flags;        // siehe unten

    ULONG time_stamp;   // wird bei jedem IM_GETINPUT aktualisiert

    Object *timer;      // das Time-Source-Object
    Object *wimp;       // das WIMP-Handler-Object
    Object *keyb;       // das Keyboard-Handler-Object

    /*** Input Expression Lists (Liste von IXTokens) ***/
    struct IXHeader btn_ixl[INUM_BUTTONS];
    struct IXHeader sld_ixl[INUM_SLIDERS];
};

/*-------------------------------------------------------------------
**  Message-Strukturen
*/
struct inp_handler_msg {
    ULONG type;         // ITYPE_#?
    ULONG num;          // nur für Slider und Buttons
    UBYTE *id;          // Type-spezifischer ID-String
};

struct inp_delegate_msg {
    ULONG type;         // an welches Object delegieren? ITYPE_#?
    ULONG num;          // nur für Slider und Buttons
    ULONG method;       // die Methode, die delegiert werden soll
    void *msg;          // Pointer auf Msg für die Methode
};

/*-----------------------------------------------------------------*/
#endif

