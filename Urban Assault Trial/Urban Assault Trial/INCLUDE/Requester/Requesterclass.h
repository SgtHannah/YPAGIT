#ifndef REQUESTER_REQUESTERCLASS_H
#define REQUESTER_REQUESTERCLASS_H
/*
**  $Source: PRG:VFM/Include/input/clickbox.h,v $
**  $Revision: 38.3 $
**  $Date: 1995/08/17 23:45:25 $
**  $Locker:  $
**  $Author: floh $
**
**  Struktur-Definitionen für die RequesterClass. Die RequesterClass
**  verwaltet komplexere Strukturen aus ButtonObjekten
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

#ifndef REQUESTER_BUTTONCLASS_H
#include <requester/buttonclass.h>
#endif


/*
**  Die Requesterklasse verwaltet ein komplexeres Eingabefeld aus mehreren
**  ButtonObjekten. Zusätzlich bietet sie einen Rahmen an, an dem man
**  Requester verschieben, iconifizieren u.a. kann.
**
**  Es verlangt 2 Default-Button-Objekte, eines für den Rahmen und eines
**  für das Icon. Wie die aussehen, ist egal. Wichtig ist, daß sie gewisse
**  Standardwerte zurückschicken
**
**  Active hat vorerst nur optische Bedeutung, weil wir keine Tasten auswerten.
**  Weil ich nicht wissen kann, wann ein anderer Requester aktiv wird, muß
**  alles von außen geschalten werden!
**
**  Methoden
**
**      RQM_MOVEREQUESTER/POSREQUESTER
**          setzt einen Requester auf eine Position (POS) bzw. verschiebt ihn
**          (MOVE), beidesmal wird eine Message mit einer Position/ eines Wegstückes
**          übergeben, die Flags werden zur zeit noch nicht ausgewertet.
**
**  Attribute
**
**      RQA_Active:
**          
**          Optische Äußerung des Requesters als aktiv (z.B. für Tasten...)
**
**      RQA_Chars
**
**          Hiermit lassen sich ALLE Sonderzeichen für den Requester
**          verändern. Eine Angabe ist nicht notwendig, denn dann werden die
**          Defaultwerte genommen. Die Reihenfolge im String ist wie folgt:
**          R(ahmen)Up, RDown, RLeft, RRight, RUpLeft, RUpRight, RDownLeft
**          RDownRight, RBackRound
**          zuerst nacheinander alle inaktiven, dann alle aktiven, siehe dazu
**          auch rq_attrs.c
**
**      RQA_Open/RQA_Iconified
**
**          geben Aussage über den Zustand des Requesters.
**
**      RQA_Requester
**
**          Das RequesterObjekt (z.B. mit Close- und MoveButton), muß
**          angegeben werden. Gibt die Größe an. Ist ein ButtonClass-Objekt
**
**      RQA_Icon
**
**          das Icon(Button)Objekt, so sieht das Ding iconifiziert aus.
**
**      RQA_Font
**
**          der Font für den Rahmen u.s.w.
**
**      RQA_PosX/RQA_PosY
**
**          Hierbei wird die Position des RequesterObjektes (rqd->requester)
**          zurückgegeben, welches das einzig repräsentative Dingsda ist
**
**      RQA_MoveBorderL/R/U/D
**          Rand für die Bewegung des Requesters (an den ScreenRand ran)
**
*/

#define REQUESTER_CLASSID "requester.class"

/*** Methods ***/
#define RQM_BASE                (OM_BASE + METHOD_DISTANCE)
#define RQM_HANDLEINPUT         (RQM_BASE)
#define RQM_PUBLISH             (RQM_BASE + 1)
#define RQM_SWITCHPUBLISH       (RQM_BASE + 2)
#define RQM_NEWBUTTONOBJECT     (RQM_BASE + 3)
#define RQM_REMOVEBUTTONOBJECT  (RQM_BASE + 4)
#define RQM_GETOFFSET           (RQM_BASE + 5)
#define RQM_POSREQUESTER        (RQM_BASE + 6)
#define RQM_MOVEREQUESTER       (RQM_BASE + 7)


/*** Attribute ***/
#define RQA_BASE                (OMA_BASE + ATTRIB_DISTANCE)
#define RQA_Requester           (RQA_BASE)          // I
#define RQA_Icon                (RQA_BASE + 1)      // I
#define RQA_Iconified           (RQA_BASE + 2)      // G
#define RQA_Open                (RQA_BASE + 3)      // G
#define RQA_Active              (RQA_BASE + 4)      // ISG
#define RQA_Chars               (RQA_BASE + 5)      // SG
#define RQA_Font                (RQA_BASE + 6)      // ISG
#define RQA_PosX                (RQA_BASE + 7)      // G
#define RQA_PosY                (RQA_BASE + 8)      // G
#define RQA_MoveBorderU         (RQA_BASE + 9)      // ISG
#define RQA_MoveBorderD         (RQA_BASE + 10)     // ISG
#define RQA_MoveBorderL         (RQA_BASE + 11)     // ISG
#define RQA_MoveBorderR         (RQA_BASE + 12)     // ISG


/*** Default-Werte ***/



/*** Standard-Messages. ab 100, denn davor liegen die Button ***/
#define PRA_RQ_PRESSMOVE        101     // Move-Balken gedrückt
#define PRA_RQ_RELEASEMOVE      102     // selbigen losgelassen
#define PRA_RQ_CLOSE            103
#define PRA_RQ_ICONIFY          104     // zum Icon machen
#define PRA_RQ_ICONPRESSED      105     // extra, wegen Move-Icon
#define PRA_RQ_ICONRELEASED     106     // wieder normaler Requester


/*** Das Eingabe-Feld, also die Local Instance Data dieser Klasse ***/

#define NUM_BUTTONOBJECTS   10

struct buttonobject {

    Object *o;
    ULONG   ID;
};


struct requester_data {

    Object *requester;      // extern erzeugtes Button-Object
    Object *icon;           // ebenso

    WORD    flags;

    WORD    number;         // Anzahl der zusätzlichen ButtonObjekte
    struct buttonobject button[ NUM_BUTTONOBJECTS ];

    /*** video ***/
    WORD   screen_x;
    WORD   screen_y;

    WORD   last_mousex;
    WORD   last_mousey;

    ULONG  iconify_backup;  // welche sind offen (für nach Wiederherstellung)

    /*** special font ***/
    UBYTE  rq_font;

    /*** Grenzemn für die Bewegung ***/
    WORD   moveborder_u;
    WORD   moveborder_d;
    WORD   moveborder_l;
    WORD   moveborder_r;

    /*** spezielle zeichen, inactiver Requester ***/
    UBYTE  ch_rup;          // für den Rahmen
    UBYTE  ch_rdown;        // bezieht sich auf den Stringfont
    UBYTE  ch_rupleft;
    UBYTE  ch_rupright;
    UBYTE  ch_rdownleft;
    UBYTE  ch_rdownright;
    UBYTE  ch_rleft;
    UBYTE  ch_rright;
    UBYTE  ch_rbackround;   // der Hintergrund der Rahmenfläche

    /*** aktiver Requester ***/
    UBYTE  ac_rup;          // für den Active-Rahmen
    UBYTE  ac_rdown;        // bezieht sich auf den Stringfont
    UBYTE  ac_rupleft;
    UBYTE  ac_rupright;
    UBYTE  ac_rdownleft;
    UBYTE  ac_rdownright;
    UBYTE  ac_rleft;
    UBYTE  ac_rright;
    UBYTE  ac_rbackround;   // der Hintergrund der Rahmenfläche
};


#define RQC_CH_RUP          'o'
#define RQC_CH_RUPLEFT      'n'
#define RQC_CH_RUPRIGHT     'p'
#define RQC_CH_RLEFT        'l'
#define RQC_CH_RRIGHT       'm'
#define RQC_CH_RDOWNLEFT    'q'
#define RQC_CH_RDOWNRIGHT   's'
#define RQC_CH_RDOWN        'r'
#define RQC_CH_RBACKROUND   't'
#define RQC_AC_RUP          'b'
#define RQC_AC_RUPLEFT      'a'
#define RQC_AC_RUPRIGHT     'c'
#define RQC_AC_RLEFT        'a'
#define RQC_AC_RRIGHT       'c'
#define RQC_AC_RDOWNLEFT    'd'
#define RQC_AC_RDOWNRIGHT   'f'
#define RQC_AC_RDOWN        'e'
#define RQC_AC_RBACKROUND   'g'


struct selectbo_msg {

    ULONG number;
};


struct reqswitchpublish_msg {

    ULONG   modus;      // ein- oder ausschalten
    ULONG   who;        // bei Buttonobjekten: Nummer
};


struct newbuttonobject_msg {

    Object *bo;     // das zu verwaltende Buttonobject
    ULONG   ID;     // wie heißt das Buttonobject bei uns?
};


struct movereq_msg {

    LONG    x;
    LONG    y;
    LONG    flags;
};


#define RSP_REQPUBLISH          1   // Requester aufklappen
#define RSP_REQNOPUBLISH        2   // Requester vollkommen weg
#define RSP_ICONIFY             3   // Requester al Icon
#define RSP_BTPUBLISH           4   // Buttonobjekte zuschalten
#define RSP_BTNOPUBLISH         5   // Buttonobjekte wegschalten

/* -----------------------------
** Flag- und Aktionsdefinitionen
** ---------------------------*/

#define RF_ICONIFIED    (1<<0)      // bin gerade das Icon
#define RF_MOVE         (1<<1)      // Movebutton ist gedrückt
#define RF_PUBLISH      (1<<2)      // soll dargestellt werden
#define RF_ACTIVE       (1<<3)      // soll als aktiv dargestellt werden, irgendwie,
                                    // derzeit: von außen --> activ
                                    // dann Rahmen zeichnen


#endif




