#ifndef REQUESTER_BUTTONCLASS_H
#define REQUESTER_BUTTONCLASS_H
/*
**  $Source: PRG:VFM/Include/input/clickbox.h,v $
**  $Revision: 38.3 $
**  $Date: 1995/08/17 23:45:25 $
**  $Locker:  $
**  $Author: floh $
**
**  Struktur-Definitionen für die ButtonClass. Die ButtonClass erlaubt
**  das Erstellen von simplen AbfrageBoxen. Diese können Bestandteile
**  von Requestern sein
**
**  (C) Copyright 1995 by A.Weissflog & Andreas Flemming
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef INPUT_CLICKBOX_H
#include <input/clickbox.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include <nucleus/nucleusclass.h>
#endif





/*
**
**  Zentral bei dem Input-Konzept ist eine ClickBox-Struktur. Die InputEngine kann
**  damit was anfangen, somit ist es logisch, das System darauf aufzubauen. Es gibt
**  also eine Requester-Klasse, die eine ClickBox und all ihre Informationen
**  dazu verwaltet.
**
**  Eine ClickBox besteht aus (maximal) 32 Buttons. Zu diesen Buttons müssen wir
**  folgende Informationen verwalten:
**
**      Backup von Position und Größe (-->inaktiv schalten)
**      den Namen
**      Attribute
**      Signale
**
**  Signale sind die Messages, die man haben möchte, wenn ein Button gedrückt
**  wurde (o.a.)
**
**  Wie wird so etwas initialisiert? Wir übergeben eine Button-Init-Struktur, die
**  alle Daten für einen button enthält. Intern wird dann ein Zähler hochgezählt,
**  und sofern der Button noch akzeptiert werden kann, werden die Daten kopiert.
**
**  Weiterhin muß die Klasse Methoden zum Darstellen, Auswerten und Verändern des
**  Requesters anbieten.
**
**  Spezielle zeichen (Rahmen, spezielle gadgets...) können extern angegeben 
**  werden
**
**
**
**
**  Methoden
**
**      BT_NEWBUTTON
**
**      fügt einen Button hinzu. Übergeben wird eine newbutton_msg. Darin enthalten
**      sind ungefähr die gleichen Informationen, wie sie letztendlich in der
**      bt_button-Struktur stehen. Da wären:
**          - Die Größe und Position
**          - Der name, welcher maximal 31 Zeichen umfassen darf
**          - der Inaktiv-Name, der dann dargestellt werden soll, wenn das
**            Gadget nicht anwählbar ist. Hier darf NULL angegeben werden, was
**            bedeutet, daß eine interne Sache genommen wird.
**          - der Shortcut ist ein Charakter, keine Groß-Klein-Unterscheidung
**            liefert button_released
**          - release- und press-data. Werden bei Press und Release gemeldet.
**            Achtung, hier sind auch interne Sachen wie Close möglich. Deshalb
**            eigene Definitionen ab 1000.
**            Die internen Sachen stehen unten irgendwo rum...
**
**      neu: jeder Button hat 3 eigene Fonts, die nun nicht mehr nach der Art,
**           sondern nur noch nach dem Zustand unterschieden werden:
**           ungedrückt, gedrückt und disabled
**           somit reduzieren sich auch die Strings auf gedrückt und ungedrückt
**
**      BT_ENABLEBUTTON
**
**          schaltet einen Button aktiv, heißt, er kann gedrückt werden
**
**      BT_DISABLEBUTTON
**
**          verbietet sein drücken. Optisch äußert sich das dadurch, daß
**          der inactiv-String gezeichnet wird
**
**      BT_REMOVEBUTTON
**
**          entfernt (unwiderbringlich) eine Button, weiß nicht, ob sowas Sinn
**          haben wird?? ja, doch !!
**
**      BT_HANDLEINPUT
**
**          Verwertet die übergebene VFMInput-Struktur, liefert den pressed-,
**          released-Wert oder 0, wenn nix war. Behandelt auch die internen
**          Sachen.
**
**      BT_SWITCHPUBLISH
**
**          folgende Darstellungsarten werden unterstützt:
**              offen, geschlossen,
**          In der Message gebe man die Pre-Release-Aktionen an, die auch bei
**          user_pressed/user_released akzeptiert werden.
**          modus 1 und modus 2 sind sich gegenseitig ausschliessende
**          Darstellungsarten, wobei eine die iconifzierte sein kann, aber
**          nicht muß
**
**      BT_PUBLISH
**
**          Stellt sich dar
**
**      BT_SETSTRING
**
**          setzt alle strings neu. es kann bei inaktiv und pressed der Name 
**          angegeben werden
**
**      BT_GETSPECIALINFO
**
**          gibt Pointer auf die Specialinfo des Buttons zurück, dessen ID
**          mittels einer selectbutton_msg übergeben wurde.
**
**      BTM_REFRESH
**
**          initialisiert Neuberechnung diverser Werte, nachdem man zum Beispiel
**          in einer Specialinfo rumgeschrieben hat, was das Objekt ja nicht mit-
**          kriegt. Ebenfalls selectbutton_msg.
**
**  Attribute
**
**      BTA_Box....
**
**          Sind die Position und Ausdehnung des Feldes, zu dem alle weiteren
**          positionen relativ sind! Müssen bei NEW angegeben werden. Ist sozusagen
**          struct rect der ClickBox.
**
**      BTA_...Font
**
**          Die Fontattribute geben VFM-Fontnamen an. Notwendig, weil
**          String, pressed und unpresssed unterschiedlich aussehen
**
**      BTA_Chars
**
**          Dieser String enthält die Zeichen, die für Spezialfälle verwendet werden
**          sollen, also Rahmen etc. Da es Defaultwerte gibt, muß dieses Attribut
**          nicht angegeben werden, sollte aber dann immer vollständig wie folgt
**          sein:
**          b(utton)open, bclose, bfill
**
**
**
**  Signale MXBUTTON    GADGET     STRING   SWITCH
**
**  pressed     x          x          -       x
**  released    -          x          -       x
**  hold        -          x          -       -
**
*/

#define BUTTON_CLASSID "button.class"

/*** Methods ***/
#define BTM_BASE                (OM_BASE + METHOD_DISTANCE)
#define BTM_NEWBUTTON           (BTM_BASE)
#define BTM_REMOVEBUTTON        (BTM_BASE + 1)
#define BTM_ENABLEBUTTON        (BTM_BASE + 2)
#define BTM_DISABLEBUTTON       (BTM_BASE + 3)
#define BTM_SWITCHPUBLISH       (BTM_BASE + 4)
#define BTM_HANDLEINPUT         (BTM_BASE + 5)
#define BTM_PUBLISH             (BTM_BASE + 6)
#define BTM_SETSTRING           (BTM_BASE + 7)
#define BTM_GETOFFSET           (BTM_BASE + 8)
#define BTM_SETSTATE            (BTM_BASE + 9)
#define BTM_GETSPECIALINFO      (BTM_BASE + 10)
#define BTM_REFRESH             (BTM_BASE + 11)
#define BTM_SETBUTTONPOS        (BTM_BASE + 12)


/*** Attribute ***/
#define BTA_BASE                (OMA_BASE + ATTRIB_DISTANCE)
#define BTA_BoxX                (BTA_BASE + 3)                  // IG
#define BTA_BoxY                (BTA_BASE + 4)                  // IG
#define BTA_BoxW                (BTA_BASE + 5)                  // IG
#define BTA_BoxH                (BTA_BASE + 6)                  // IG
#define BTA_InstData            (BTA_BASE + 7)                  // G
#define BTA_Chars               (BTA_BASE + 8)                  // IS

/*** Default-Werte ***/

#define BT_NAMELEN      512

struct bt_button {

    WORD    x, y, w, h;             // Backups der ButtonPos und -Größe
    char    unpressed_text[ BT_NAMELEN ];    // der Name (DrawText-String 
                                             // wird vor dem Zeichnen
                                             // zusammengebastelt)
    char    pressed_text[ BT_NAMELEN ];

    WORD    flags;                  // Flags für den Button
    WORD    shortcut;
    ULONG   user_pressed;           // gewünschtes Signal bei button-pressed
    ULONG   user_released;          // gewünschtes Signal bei button-release
    ULONG   user_hold;              // liefern nur die Gadgets
    WORD    modus;                  // Was ist der Button
    WORD    ID;                     // wer bin ich?
    LONG    *specialinfo;           // Pointer auf zusatzstruktur

    LONG    red;                    // farben fuer DBCS-Version
    LONG    green;
    LONG    blue;
    
    /*** die Fonts ***/
    UBYTE  pressed_font;
    UBYTE  unpressed_font;
    UBYTE  disabled_font;
};


/*** Die Modi, die es für einen Button gibt ***/
#define BM_GADGET       1   // pressed bei MDOWN, release bei MUP, wenn vorher pressed
#define BM_SWITCH       2   // pressed bei MUP nach MDOWN, released nach MUP nach
                            // MDOWN, wenn pressed war, auf deutsch: Schalter
#define BM_STRING       3   // keine Messages
#define BM_MXBUTTON     4   // nur pressed, wenn gedrückt. schaltet alle anderen mx-
                            // buttons dann aus.
#define BM_SLIDER       5   // Slidergadget



/*** Das Button/Eingabe-Feld, also die Local Instance Data dieser Klasse ***/
struct button_data {

    struct ClickBox click;          // dies ist die Struktur für die InputEngine
    struct bt_button *button[MAXNUM_CLICKBUTTONS];  // die Buttons, siehe oben
    WORD   number;                  // Anzahl der Buttons
    WORD   flags;


    /*** Spezielle Zeichen ***/
    UBYTE  ch_bopen;                // Buttoneröffnung
    UBYTE  ch_bclose;               // Buttonabschluss
    UBYTE  ch_bfill;                // Buttonfüllung
    UBYTE  ch_bslideropen;          // Slidereröffnung
    UBYTE  ch_bsliderback;          // Sliderhintergrund, Rest ist normaler Button
    UBYTE  ch_bsliderclose;         // Sliderabschluß

    /*** video ***/
    WORD   screen_x;
    WORD   screen_y;
};


/*** Default-Zeichen ***/
#define BTC_CH_BOPEN        'a'
#define BTC_CH_BCLOSE       'c'
#define BTC_CH_BFILL        32 // 'b'
#define BTC_CH_BSLIDEROPEN  'a'
#define BTC_CH_BSLIDERBACK  115        // s
#define BTC_CH_BSLIDERCLOSE 'c'





/*** Die Methoden-Messages ***/
struct newbutton_msg {

    LONG    modus;              // für welchen Req-Modus, 0 oder 1
    LONG    x, y, w, h;         //
    char    *unpressed_text;    // der Name (DrawText-String wird vor dem Zeichnen
                                // zusammengebastelt)
    char    *pressed_text;

    ULONG   shortcut;
    ULONG   user_pressed;       // gewünschtes Signal bei button-pressed
    ULONG   user_released;      // gewünschtes Signal bei button-release
    ULONG   user_hold;          // bei hold
    ULONG   ID;                 // die ID des Buttons
    ULONG   flags;              // Flags siehe oben bei Button-flags
    LONG    *specialinfo;       // Zusatzstruktur, Daten werden kopiert
    
    /*** die Fonts ***/
    UBYTE  pressed_font;
    UBYTE  unpressed_font;
    UBYTE  disabled_font;

    /*** Die farben fuer DBCS ***/
    LONG    red;
    LONG    green;
    LONG    blue;
};

/*** specialinfos ***/
struct bt_propinfo {

    WORD    value;              // Werte des gadgets
    WORD    max;
    WORD    min;

    WORD    start;              // Äußerung in der Buttongeometrie
    WORD    end;

    WORD    what;               // welcher Teil wurde getroffen?
    WORD    mvalue;             // merkt sich ursprünglichen Wert bei HIT_SLIDER
    WORD    down_x;             // Screen-x-Koordinate bei MouseDown
};


#define HIT_NOTHING     0       // nix getroffen
#define HIT_LESS        1       // teil vor Slider getroffen
#define HIT_SLIDER      2       // Slider getroffen
#define HIT_MORE        3       // teil nach Slider getroffen


/*** Universalstruktur zur Auswahl eines Buttons ***/
struct selectbutton_msg {

    ULONG   number;         // zu removender Button
};


struct switchpublish_msg {

    ULONG   modus;
};


struct switchbutton_msg {

    ULONG   number;
    ULONG   visible;        // wirkt nur bei DISABLE
};

/*** die verschiedenen Modi, derzeit sind es volle 2 Stück!!! ***/
#define SP_PUBLISH      1
#define SP_NOPUBLISH    2



struct setstring_msg {

    ULONG  number;     // button-number
    char  *unpressed_text;
    char  *pressed_text;
};


struct setbuttonstate_msg {

    ULONG   who;        // ID
    ULONG   state;      // wie soll der Status sein?
};

#define SBS_PRESSED     1       // gedrückt
#define SBS_UNPRESSED   2       // losgelassen
                                // evtl. hier noch Sperrung etc.

struct setbuttonpos_msg {

    ULONG   number;
    WORD    x;                  // -1 bedeutet nicht aendern
    WORD    y;
    WORD    w;
    WORD    h;                  // der Vollstaendigkeit halber. zur zeit keine Unterst.
};

            
/* -----------------------------
** Flag- und Aktionsdefinitionen
** ---------------------------*/

/*** interne Gadget-Aktionen, ab 0 ***/
#define PRA_CLOSE       1L      // schließe den requester
#define PRA_OPEN        2L      // öffnet ihn
#define PRA_CBOXHIT     3L      // clickbox getroffen, wird von Buttontreffern überlagert



/*** die flags der Buttons ***/
#define BT_PRESSED      (1<<0)  // ist zur Zeit gedrückt
#define BT_DISABLED     (1<<1)  // zur Zeit inaktiv, also nicht anwählbar
#define BT_DOWN         (1<<2)  // Mouse ging hier nieder
#define BT_PUBLISH      (1<<3)  // soll dargestellt werden

#define BT_BORDER       (1<<4)  // mit Rahmen, üblich
#define BT_CENTRE       (1<<5)  // text zentrieren 
#define BT_TEXT         (1<<6)  // definiert dieses ausdruecklich als text (fuer Truetype)
#define BT_NOPRESS      (1<<7)  // fuer DBCS, eigentlich gedrueckt, aber nicht so zeichnen
#define BT_RIGHTALIGN   (1<<8)  // Text rechts zentrieren.

/*** die flags der Buttonliste ***/
#define BF_PUBLISH      (1<<0)  // wird dargestellt


#define MIN_SLIDER_WIDTH    6

#endif




