#ifndef YPA_BACTCLASS_H
#define YPA_BACTCLASS_H
/*
**  $Source: PRG:MovingCubesII/Include/ypa/ypabactclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1995/06/08 23:10:23 $
**  $Locker: floh $
**  $Author: floh $
**
**  Ok, hier meine Version der ypabact.class, erstmal Schmalspur...
**
**  (C) Copyright 1995 by A.Weissflog & A.Flemming
**  Hauptsächlich basierend auf A.F.'s bacteria.class.
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

#ifndef YPA_YPAVEHICLES_H
#include "ypa/ypavehicles.h"
#endif

#ifndef YPA_YPAKEYS_H
#include "ypa/ypakeys.h"
#endif

#ifndef YPA_YPASTRATEGY_H
#include "ypa/ypastrategy.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      ypabact.class -- Bacteria-Klasse für Your Personal Amok,
**                       Superklasse aller Vehikel im Spiel.
**
**  FUNCTION
**      Kommunikation mit Welt-Object, Bildung und Verwaltung
**      von Vehikel-Gruppen (Geschwader), grundsätzliche
**      Wegfindungs- und Bewegungs-Algorithmen.
**      Vererbungsfreundlich, konkrete Abwandlungen (wie bestimmte
**      Vehikel-Typen) werden durch Subklassen realisiert.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**      YBM_TR_LOGIC
**          Msg:    struct trigger_logic_msg
**          Ret:    ---
**
**          Wird einmal pro Frame auf jedes Bakterien-Object angewendet.
**          Dient quasi als Signal-Methode, das Object entscheidet
**          selbst, was es innerhalb dieser Methode macht.
**
**      YBM_TR_VISUAL
**          Msg:    struct basepublish_msg (von base.class)
**          Ret:    ---
**
**          Wird auf Object angewendet, wenn es sich darstellen
**          soll. Vorher *MUSS* bereits YBM_TR_LOGIC angewendet worden
**          sein. Das Object 'kopiert' seine Position und Orientierung
**          in das 'zuständige' Visual Prototype Object und wendet
**          ein BSM_PUBLISH darauf an. Der Rest ist Sache des
**          Welt-Objects.
**
**      YBM_SETTARGET
**          Msg:    struct settarget_msg
**          Ret:    ---
**
**          Setzt primäres oder sekundäres Target. Target darf ein
**          eigener oder feindlicher Sektor oder eine Unit sein.
**          Die konkreten Aktionen zum und beim Erreichen des
**          Targets ist hauptsächlich davon abhängig, welcher Typ
**          das Target ist und ob es befreundet oder feindlich ist
**          (das entscheiden dann aber die AI-Methoden).
**
**      YBM_HANDLEINPUT
**          Msg:    struct trigger_logic_msg
**          Ret:    ---
**
**          Input-Handler für Object. Wird anhand der AI-Methoden
**          aktiviert, wenn YBA_UserInput TRUE ist.
**
**      YBM_AI_LEVEL1
**          Msg:    ---
**          Ret:    ---
**
**          Abstraktester Level des Weg-Findungs-Algorithmus.
**          Schaltet einen anderen Action-Typ aktiv, falls das
**          Primär-Ziel erreicht ist, oder aus irgendwelchen
**          Gründen keins vorhanden ist. Falls irgendwann mal
**          der Fall gebraucht wird, selbstständig ein Primär-
**          Ziel auszuwählen, sollte das auch hier passieren.
**
**          Als Ergebnis dieser Methode wird der <tar_vec> in der
**          Bacteria-Struktur auf das Primär-Ziel ausgerichtet.
**
**      YBM_AI_LEVEL2
**          Msg:    ---
**          Ret:    ---
**
**          Ist für das Sekundär-Ziel zuständig. Falls ein
**          Sekundär-Ziel existiert, wird der <tar_vec> auf dieses
**          umgebogen (das Primär-Ziel verliert dadurch seine Bedeutung).
**          Außerdem wird entschieden, ob ein neuer Action-Typ
**          aktiviert wird (zum Beispiel irgendein Kampf-Verhalten).
**          Falls kein Sekundär-Ziel (mehr) existiert, versucht die
**          Routine erstmal mit Hilfe des Agressions-Wertes ein
**          neues Sekundär-Ziel in der Umgebung zu finden. Mißlingt
**          dies auch, bleibt der <tar_vec> auf das Primär-Ziel
**          ausgerichtet.
**
**      YBM_AI_LEVEL3
**          Msg:    ---
**          Ret:    ---
**
**          Der niedrigste Level der Bewegungs-Kontrolle. Hier
**          werden folgende grundlegenden Sachen erledigt:
**
**          (1) Kollisions-Kontrolle auf aktuelle Position
**          (2) vorhersagende Kollisions-Kontrolle
**              (beeinflußt direkt <dir> und <tar_vec>).
**          (3) <dir> an den <tar_vec> angleichen
**          (4) 'Korrekturen', also Parameter so verändern,
**              das bevorzugte Flughöhe und Geschwindigkeit
**              etc. erreicht werden. Das betrifft Änderungen
**              an <dir> und <act_speed>.
**
**          Dazu folgendes:
**          (2) hat größere Priorität als (3) und (4). Falls eine Kollision
**          in Flugrichtung vorausgesagt wird, wird <dir> direkt
**          modifiziert. In diesem Fall findet *KEINE* Angleichung
**          von <dir> and <tar_vec> statt, weil das die vorhergehende
**          Korrektur aufheben könnte. In diesem Fall wird auch
**          (4) ignoriert, weil die Korrekturen die Kollisions-Verhütung
**          gefährlich beeinflussen können!
**
**          Falls eine Kollision seitlich voraus angezeigt wird, wird
**          <dir> zwar in die entgegengesetzte Richtung modifiziert,
**          (3) und (4) werden aber trotzdem ausgeführt, weil keine
**          unmittelbare Gefahr besteht. GENAU! Das ist es!
**
**          Da hier die für Move benötigte Flugkraft berechnet wird und
**          diese von der Objektausrichtung abhängt, ist es Aufgabe von
**          AI_3 die Objektdrehung zu realisieren. Man kann sagen, AI_3
**          realisiert letztendlich die Bewegung in ihrer Gesamtheit und
**          nutzt dabei Move, welches die Position aus den Kräften
**          berechnet. Somit ist das Kraftmodell in eine extra methode
**          exportiert. Move ist aber dabei von dem von AI_3 genommenen
**          Flugmodell abhängig.
**
**      YBM_ADDSLAVE
**          Msg: Object
**          Ret: ---
**
**          Hängt ein neues Bakterien-Object in die Slave-Liste.
**          Der neue Master (auf den die Methode angewendet wurde),
**          baut eine <struct newmaster_msg> zusammen und wendet
**          sie auf den neuen Slave an. Dieser klinkt sich dann
**          selbst ein (funktioniert wie BSM_ADDCHILD bei base.class).
**
**          Bitte beachten, daß die Methode nur mit _methoda()
**          invoked werden kann!
**
**      YBM_NEWMASTER
**          Msg: struct newmaster_msg
**          Ret:
**
**          Diese Methode wird normalerweise nicht direkt auf das
**          Object angewendet. Sie wird von einem neuen Master
**          auf seinen neuen Slave angewendet mit Informationen,
**          wie und wo er sich in die slave_list des Masters
**          einklinken soll.
**          Das Objekt entfernt sich selbständig aus seiner alten
**          Liste.
**
**          *** SONDERFALL ***
**          Diese Methode wird auch vom Welt-Object auf Commander-Objects
**          angewendet, in diesem Fall sieht die newmaster_msg so aus:
**
**          msg.master = 1L         -> also KEIN gültiger Object-Pointer!
**          msg.master_bact = NULL  -> weil Welt-Object kein Bakterien-Object
**          msg.slave_list          -> Listenheader im Welt-Object
**
**      YBM_MOVE
**          Msg: struct move_msg
**          Ret: ---
**
**          Realisiert die Bewegung, nicht aber die Objektdrehung.
**          Die Move-Message enthält zur Zeit nur die FrameTime. Dies
**          ist das Kraftmodell und kann somit auch von HandleInput
**          aufgerufen werden.
**
**      YBM_SETTARGET
**          Msg: settarget_msg
**          Ret: ---
**
**          Setzt ein Ziel. Es muß übergeben werden in der Message:
**          die Art (Sektor/Bacterium)
**          die Priorität (Prim/Sec)
**          Pointer oder Position
**          Aus praktischen Gründen ist es zur Zeit so, daß bei Bacterium
**          nur der Pointer und bei Sektoren nur die x,z-Position ausgewertet
**          wird.
**
**      YBM_FIGHTSECTOR / YBM_FIGHTBACT
**          Msg:
**          Ret:
**
**          Methoden zum Bekämpfen von Sektoren oder Bakterien.
**
**      YBM_HANDLEEVENTS
**          Msg:
**          Ret:
**          Nimmt sich alle Ereignisse der eigenen brain_list vor und bearbeitet
**          sie. Das heißt bei den primitiven Klassen vor allem das Zusammen-
**          fassen und die Weitergabe des höchstpriorisierten, beim Robo ist
**          es die richtige Bearbeitung. Zeitliche Aufteilungen bezüglich
**          des Zusammenfassens sind denkbar.
**
**      YBM_SETPOSITION
**          Setzt eine Position und, wenn SetGround, dann mit Ausrichtung
**          an den Boden. Übernimmt Einklinken in SektorListe.
**
**      YBM_SETSTATE
**          Setzt den Status, was man eigentlich auch einfacher haben könnte,
**          aber die Routine setzt auch gleich die VisProtos, wenn sie noch 
**          NICHT gesetzt waren. Wenn zum Beispiel einem Objekt in der Luft die
**          Energie ausgeht, dann setzt trigger es auf tot und meldet es ab
**          (EXTRA_LOGICDEATH). Dann sind wir im ACTION_DEAD und fallen, aber
**          erst, wenn  wir aufschlagen, darf der VP gesetzt werden (mit dieser)
**          Methode. Somit ist auch sichergestellt, daß wir auf dem Boden nicht
**          nochmal explodieren, wenn es schon in der Luft geknallt hat.
**
**      YBM_FIREMISSILE
**          Alloziert eine Rakete (allgemein: ein ypamissile-Object) und 
**          feuert sie entsprechend den übergeben Parametern ab.
**
**      YBM_DIE
**          Wird nur von TriggerLogic aufgerufen und übernimmt das logische
**          Sterben (Abmelden, Nachfolger benenen, Raketen wegschmeißen,...)
**
**      YBM_IMPULSE
**          Wendet auf ein Objekt einen Kraftstoß an. Übergeben wird das
**          Zentrum der Explosion und der Kraftstoß. Ein Kraftstoß berechnet
**          sich aus Kraft und Dauer der Wirkung, also F*t. Ich denke, typische
**          Werte für t sind 0.1 sec, so daß der Kraftstoß bei 1000N eben 100
**          wäre.
**
**      YBM_CANYOUDOIT
**          Übergeben wird eine Energie und ein Flag, ob die Entscheidung
**          vor oder während eines Kampfes steht. TRUE heißt, wir machen
**          die Sache (weiter)
**
**      YBM_GENERALENERGY
**          Erledigt den Energieabzug für die Bewegung und den normalen
**          Energiefluß in Abhängigkeit vom Sektoreigentümer. Wird von
**          MOVE aufgerufen. Bodenfahrzeuge sollten die Methode überladen,
**          um den Bodentyp zu beachten.
**
**      YBM_SUMPARAMETER
**          Summiert einen bestimmten Parameter für sich und alle Untergebenen.
**          Ist sinnvoll, wenn man schnell einen Überblick über den Zustand 
**          eines Geschwaders haben will. Was unterstützt wird, siehe Flags
**          unten (PARA_....)
**
**      YBM_ANNOUNCEEVENT
**          Versucht, ein Ereignis in die Eigene Brainlist einzuklinken.
**          Das muß nicht immer möglich sein (geringe Priorität, gerade
**          in Arbeit, etc. ... ). Man übergibt das Ereignis und Daten dazu
**          und erhält ein TRUE, wenn es angenommen wurde.
**
**      YBM_REDUCEENERGY
**          Außerplanmäßiger Energieabzug. flags entscheidet über Art und Weise.
**          Die möglichen Flags sind unten bei der zugehörigen msg erklärt.
**
**      YBM_CRASHTEST
**          Testet, ob resultierende geschwindigkeit bezogen auf eine übergebene
**          Flächennormale größer als CRASH_SPEED ist.
**
**      YBM_RECOIL
**          Das Abrallen. Erklärt sich aus untenstehender Msg selbst, lediglich
**          das Abrutschen ist etwas komplizierter. Ist dof.y negativ, so wird es 
**          mit mul_y multipliziert, andernfall dadurch dividiert. Damit wird
**          dof.y nach unten gebogen. Es werden nur Werte <1.0 und >0.0 
**          akzeptiert.
**
**      YBM_CHECKLANDPOS
**          TRUE, wenn die derzeitige Position nicht auf einem Slurp liegt.
**
**      YBM_GETSECTARGET
**          sucht  im Umfeld (1 Sector) meiner oder einer anderen Position nach
**          möglichen Zielen. Die getsecttar_msg wird einmal initialisiert und
**          dann fortlaufend verwendet. In ihr merke ich den letzten untersuchten
**          Kandidaten. --> Jedem Anfragenden seinen Feind!
**
**      YBM_GETBESTSUB
**          Übergeben wird eine Position und die Methode korrigiert diese so,
**          daß sie auf den energiereichsten Subsektor (3x3) oder den Mittelpunkt
**          (COMPACT) zeigt.
**
**      YBM_REINCARNATE
**          Viele Objekte setzen diverse Flags (nicht im bacterium) und andere
**          Sachen, die bei der Wiederverwendung so nicht sein sollten. Dies
**          ist sozusagen eine Aufräummethode, die die Welt bei Obtainvehicle
**          anwendet und damit sicherstellt, daß alles seinen sozialistischen
**          Gang geht.
**
**      YBM_STOPMACHINE
**          Ausrichten und anhalten
**
**      YBM_CHECKAUTOFIREPOS
**          Testet, ob es günstig ist zu ballern.
**
**      YBM_VISIER
**          Dient der Zielaufnahme für den User
**
**      YBM_FIREYOURGUNS
**          Ist das Kanonengegenstück zu FireMissile. Setzt beim Viewer
**          auch die Einschüsse. Wirkt nur auf Bakterien
**
**      YBM_HANDLEVISCHILDREN
**          Bearbeitet die Visprotos in gewünschter Weise (zur Zeit ankoppeln und
**          Removen der Children)
**
**      YBM_TESTSECTARGET
**          dient bei der Nebenzielsuche  für zusätzliche Tests, kann auch genutzt
**          werden, ein Ziel wieder abzumelden. Sinnvoll für Guns bzgl. der Winkel
**          Übergeben wird das Bacterium des Feindes
**
**      YBM_DOWHILEBEAM
**          Übernimmt Effekte und abmelden beim Beamen.
**
**      YBM_DOWHILEDEATH
**          Übernimmt Effekte und abmelden beim Deathen ...  naja, also
**.         während man so tot ist.
**
**      YBM_RUNEFFECT
**          Startet einen  Vehicle-Effekt. Dies geschieht beim Umschalten
**          in einen Zustand. Falls man diese methode extra anwenden möchte,
**          so sollte der Zustand vorher gesetzt werden, da alle Informationen
**          aus der bakterienstruktur (speziell DestFX) genommen werden.
**          siehe auch dort und condition.c
**
**      YBM_CHECKPOSITION
**          testet und korrigiert die position auf Viewertauglichkeit.
**          Dies lohnt sich beim Setzen des Viewermodus, um nicht in
**          Flächen zu stecken oder ähnliches...
**
**      YBM_CORRECTPOSITION
**          testet, ob eine position außerhalb der Welt ist und setzt sie
**          wieder rein. Das war früher bei jedem MOVE dabei, weil wir das
**          aber häufiger brauchen, als ich dachte, gibts itze ne Methode
**          dafür
**
**
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ypabact.class
**      ~~~~~~~~~~~~~
**      YBA_World   (I) <Object *> [instance of ypaworld.class]
**          Pointer auf Welt-Object, MUSS beim Erzeugen
**          angegeben werden!
**
**
**      YBA_TForm (G) <struct tform *>
**          Returniert Pointer auf eingebettete TForm. Diese wird
**          jeweils mit der Ausrichtung bzw. Blickrichtung des
**          Objects ausgefüllt und kann mittels _SetViewer()
**          jederzeit zum Viewer-Object gemacht werden. Der
**          Inhalt des TForm-Objects wird außerdem in den
**          Visual Prototype kopiert, um diesen zu positionieren.
**
**      YBA_Bacterium (G) <struct Bacterium *>
**          Returniert einen Pointer die eingebettete
**          Bacterium-Struktur.
**
**      YBA_Viewer (ISG) <BOOL> [def = FALSE]
**          Definiert Viewer-Status des Objects. Bevor ein
**          neuer Viewer aktiviert wird, muß der vorherige
**          ein _set(old_viewer, YBA_Viewer, FALSE) erhalten!
**          Der Viewer meldet sich automatisch bei der
**          Transformer-Engine an.
**          Ansonsten ist der einzige Unterschied zu einem
**          'normalen' Object, daß das Viewer-Object den
**          Visual Prototype NICHT darstellt.
**
**      YBA_UserInput (ISG) <BOOL> [def = FALSE]
**          Das Object soll Input vom User akzeptieren
**          (dadurch wird die künstliche Intelligenz praktisch
**          deaktiviert).
**          Bevor ein neues Object den Input-Focus bekommt, muß
**          der vorherige Input-Handler ein 
**          _set(old_input, YBA_UserInput, FALSE) bekommen!.
**
**      YBA_CheckVisExactly (ISG) <BOOL> [def = FALSE]
**          Wenn TRUE, dann werden sichtbare Objekte exakter bezüglich
**          ihrer Kollisionsvermeidung bearbeitet. Dient der Anpassung eines 
**          Spieles an die vorhandene Rechenleistung.
**
**      YBA_BactCollision (ISG) <BOOL>
**          Schaltet die direkte Kollisionskontrolle für Bakterien ein/aus.
**
**      YBA_AirConst (ISG)
**          Widerspricht zwar den "Reinigungsbemühungen", aber hier wird gleich
**          der bu_Air_const-Wert mit gesetzt, was man sonst nicht vergessen
**          darf.
**
**      YBA_VisProto (G)
**          Nur get für den pointer, alles andere mit SETSTATE. Benötigt man,
**          um festzustellen, wie man zur Zeit aussieht.
**
**      YBA_Aggression
**          Normalerweise kann man in die Bakterienstruktur reinschreiben, 
**          allerdings gehe ich davon aus, daß alle Leute in einem Geschwader
**          die gleichen Aggr. haben. Wenn man hierbei mit SET arbeitet,
**          wird der Wert nach unten durchgereicht.
**
**      YBA_ExtraViewer
**          Setzt ein Flag, ob die daten aus bact->Viewer beachtet werden sollen
**
**
** Die Bakterienattribute der Bacterium-Struktur sollte man besser über
** den Strukturpointer (YBA_Bacterium) nutzen. Es besteht die Möglichkeit, 
** daß im Zuge einer Entrümpelungsaktion alle Extra-Atrribute, die in der 
** Bakterienstruktur auftauchen, rausgeschmissen werden!
*/
#define YPABACT_CLASSID "ypabact.class"

/*-----------------------------------------------------------------*/
#define YBM_BASE            (OM_BASE+METHOD_DISTANCE)

#define YBM_TR_LOGIC            (YBM_BASE+1)
#define YBM_TR_VISUAL           (YBM_BASE+2)
#define YBM_SETTARGET           (YBM_BASE+3)
#define YBM_AI_LEVEL1           (YBM_BASE+4)
#define YBM_AI_LEVEL2           (YBM_BASE+5)
#define YBM_AI_LEVEL3           (YBM_BASE+6)
#define YBM_HANDLEINPUT         (YBM_BASE+7)
#define YBM_ADDSLAVE            (YBM_BASE+8)
#define YBM_NEWMASTER           (YBM_BASE+9)
#define YBM_MOVE                (YBM_BASE+10)
#define YBM_FIGHTBACT           (YBM_BASE+11)
#define YBM_FIGHTSECTOR         (YBM_BASE+12)
#define YBM_DIE                 (YBM_BASE+13)
#define YBM_SETSTATE            (YBM_BASE+14)
#define YBM_FIREMISSILE         (YBM_BASE+15)
#define YBM_SETPOSITION         (YBM_BASE+16)
#define YBM_SUMPARAMETER        (YBM_BASE+17)
#define YBM_GENERALENERGY       (YBM_BASE+18)
#define YBM_IMPULSE             (YBM_BASE+19)
#define YBM_MODVEHICLEENERGY    (YBM_BASE+20)
#define YBM_CRASHTEST           (YBM_BASE+21)
#define YBM_CRASHBACTERIUM      (YBM_BASE+22)
#define YBM_BACTCOLLISION       (YBM_BASE+23)
#define YBM_RECOIL              (YBM_BASE+24)
#define YBM_CHECKLANDPOS        (YBM_BASE+25)
#define YBM_GETSECTARGET        (YBM_BASE+26)
#define YBM_GETBESTSUB          (YBM_BASE+27)
#define YBM_CHECKFORCERELATION  (YBM_BASE+28)
#define YBM_GIVEENEMY           (YBM_BASE+29)
#define YBM_GETFORMATIONPOS     (YBM_BASE+30)
#define YBM_CANYOUDOIT          (YBM_BASE+31)       // o?
#define YBM_REINCARNATE         (YBM_BASE+32)
#define YBM_STOPMACHINE         (YBM_BASE+33)
#define YBM_ASSEMBLESLAVES      (YBM_BASE+34)
#define YBM_DOWHILECREATE       (YBM_BASE+35)
#define YBM_TESTDESTROY         (YBM_BASE+36)
#define YBM_CHECKAUTOFIREPOS    (YBM_BASE+37)
#define YBM_SETFOOTPRINT        (YBM_BASE+38)
#define YBM_MERGE               (YBM_BASE+39)
#define YBM_BREAKFREE           (YBM_BASE+40)
#define YBM_FIREYOURGUNS        (YBM_BASE+41)
#define YBM_VISIER              (YBM_BASE+42)
#define YBM_HANDLEVISCHILDREN   (YBM_BASE+43)
#define YBM_HOWDOYOUDO          (YBM_BASE+44)
#define YBM_ORGANIZE            (YBM_BASE+45)
#define YBM_ASSESSTARGET        (YBM_BASE+46)
#define YBM_TESTSECTARGET       (YBM_BASE+47)
#define YBM_DOWHILEBEAM         (YBM_BASE+48)
#define YBM_RUNEFFECTS          (YBM_BASE+49)
#define YBM_CHECKPOSITION       (YBM_BASE+50)
#define YBM_CORRECTPOSITION     (YBM_BASE+51)
#define YBM_TR_NETLOGIC         (YBM_BASE+52)
#define YBM_SHADOWTRIGGER       (YBM_BASE+53)
#define YBM_RELEASEVEHICLE      (YBM_BASE+54)
#define YBM_SETSTATE_I          (YBM_BASE+55)
#define YBM_MODSECTORENERGY     (YBM_BASE+56)
#define YBM_DOWHILEDEATH        (YBM_BASE+57)
#define YBM_SHADOWTRIGGER_I     (YBM_BASE+58)
#define YBM_SHADOWTRIGGER_E     (YBM_BASE+59)
#define YBM_FINDPATH            (YBM_BASE+60)
#define YBM_SETWAY              (YBM_BASE+61)




/*-----------------------------------------------------------------*/
#define YBA_BASE        (OMA_BASE+ATTRIB_DISTANCE)

#define YBA_World           (YBA_BASE+1)    // (I) (Object *)
#define YBA_TForm           (YBA_BASE+2)    // (G) <struct tform *>
#define YBA_Bacterium       (YBA_BASE+3)    // (G) <struct Bacterium *>
#define YBA_Viewer          (YBA_BASE+4)    // (ISG) BOOL
#define YBA_UserInput       (YBA_BASE+5)    // (ISG) BOOL
#define YBA_CheckVisExactly (YBA_BASE+6)    // (ISG) Exakter Test für Koll.
#define YBA_BactCollision   (YBA_BASE+7)    // (ISG) Test für Koll.untereinander
#define YBA_AttckList       (YBA_BASE+8)    // (G) (struct MinList *)
#define YBA_AirConst        (YBA_BASE+9)    // (ISG) ULONG
#define YBA_LandWhileWait   (YBA_BASE+10)   // (ISG) (BOOL)
#define YBA_YourLS          (YBA_BASE+11)   // (ISG) (ULONG)
#define YBA_VisProto        (YBA_BASE+12)   // (ISG) (Object *)
#define YBA_Aggression      (YBA_BASE+13)   // (ISG) 0..100 siehe oben!
#define YBA_ExtCollision    (YBA_BASE+14)   // (G) struct ExtCollision *
#define YBA_VPTForm         (YBA_BASE+15)   // ISG
#define YBA_ExtraViewer     (YBA_BASE+16)   // ISG BOOL
#define YBA_PtAttckNode     (YBA_BASE+17)   // G
#define YBA_StAttckNode     (YBA_BASE+18)   // G
#define YBA_AlwaysRender    (YBA_BASE+19)   // ISG


/*-------------------------------------------------------------------
**  Defaults für Attribute
*/
#define YBA_Mass_DEF            (400.0)
#define YBA_MaxForce_DEF        (5000.0)    // ausreichend für DEF-Masse bei 1g
#define YBA_AirConst_DEF        (500.0)     // mal probieren...
#define YBA_PrefHeight_DEF      (150.0)     // mal probieren...
#define YBA_MaxRot_DEF          (0.5)       // mal probieren...
#define YBA_Radius_DEF          (20)        // Objektradius für Kollision
#define YBA_Viewer_Radius_DEF   (40)
#define YBA_OverEOF_DEF         (10.0)      // über der Eof
#define YBA_Viewer_OverEOF_DEF  (40.0)
#define YBA_Energy_DEF          (10000)     // damit erstmal ohne Attr was funkt.
#define YBA_Shield_DEF          (0)         // keine Panzerung
#define YBA_HCSpeed_DEF         (0.7)       // für Handsteuerung
#define YBA_YourLastSeconds_DEF (3000)      // für alle
#define YBA_Aggression_DEF      (AGGR_SECBACT)  // prozentual
#define YBA_Maximum_DEF         (10000)     // Volle Batterie
#define YBA_Transfer_DEF        (0.5)       // Transfer fürs Aufladen
#define YBA_MaxUserHeight_DEF   (1600)
#define YBA_ADist_Sector_DEF    (800.0)     // Strecke nach Ziel bevor neuer Anflug
#define YBA_ADist_Bact_DEF      (650.0)
#define YBA_SDist_Sector_DEF    (200.0)     // Strecke vor Ziel für Rückflug
#define YBA_SDist_Bact_DEF      (100.0)
#define YBA_GunPower_DEF        (4000.0)    // 4/10 von 'ner Rakete
#define YBA_GunRadius_DEF       (5.0)       // wenig!




/*-----------------------------------------------------------------------
** Die erweiterte Kollision betrachtet number Points, deren relative
** Koordinaten in points stehen. Für jede Kugel kann ein Radius angegeben
** werden. Ist nder radius 0, so wird der Punkt bei Kollisionen nicht
** beachtet, ist dem Objekt aber erstmal bekannt, so daß er zum Beispiel
** für Teststrahlen genommen werden kann.
*/

#define MAX_COLLISION_POINTS    (16)

struct CollisionPoint {

    FLOAT  radius;              // der Radius
    struct flt_triple point;    // relative Koordinaten
    struct flt_triple old;      // vorherige Position, sollte absolut sein
};

struct ExtCollision {

    FLOAT  last_angle;                                    // letzte y-Drehung
    UBYTE  number;                                        // Anzahl der Testpunkte
    struct CollisionPoint points[ MAX_COLLISION_POINTS ]; // die Punkte höchstselbst
};


/*-----------------------------------------------------------------*/
struct ypabact_data {
    struct Bacterium bact;      // siehe "ypa/automatus.h"

    LONG map_size_x;            // Ausdehnung der Welt
    LONG map_size_y;

    ULONG flags;                // siehe unten
    Object *world;              // instance of ypaworld.class
    struct ypaworld_data *ywd;

    /*** Darstellung ***/
    Object *vis_proto;          // instance of base.class
    struct tform *vp_tform;     // TForm des Visual Prototypes


    struct MinList attck_list;  // Liste der Angreifer
    struct OBNode pt_attck_node;
    struct OBNode st_attck_node;

    /*** Sterbezeit ***/
    LONG   YourLastSeconds;         // Zeit für finale Explosion etc.
};

/*-------------------------------------------------------------------
**  Definitionen für ypabact_data.flags
*/
#define YBF_Viewer          (1L<<0)      // Object ist Viewer-Object
#define YBF_UserInput       (1L<<1)      // Object hat User-Input-Focus
#define YBF_CheckVisExactly (1L<<2)      // Exakte Kollisionsverhütung sicht-
                                         // barer Objekte
#define YBF_BactCollision   (1L<<3)      // KollTest untereinander
#define YBF_LandWhileWait   (1L<<4)      // Landen in Wartephase (z.Zt. nur Zepp)
#define YBF_ExtraViewer     (1L<<5)      // nur Flag, ausfüllen direkt in Bactstruktur
#define YBF_AlwaysRender    (1L<<6)      // immer darstellen, auch wenn der User drin
                                         // sitzt (Robos) und mit Sicherheit ein gutes Gefühl


/*-------------------------------------------------------------------
**  Die erweiterte trigger_logic_msg:
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
struct trigger_logic_msg {
    ULONG   global_time;
    ULONG   frame_time;
    struct VFMInput *input;
    ULONG   slave_count;        // Nummer des Untergebenen in 'Formation'
    ULONG   user_info;          // für spezielle Verwendung, um Informationen
                                // mit dem Triggern zu übergeben.
    ULONG user_action;          // siehe oben
    ULONG proto_id;
    struct Bacterium *selbact;
    struct Cell *tarsec;
    ULONG tarsec_x, tarsec_y;
    fp3d tarpoint;
    struct Bacterium *tarbact;
    LONG energy;                // NEU!
};




struct settarget_msg {
    ULONG target_type;      // TARTYPE_SECTOR|TARTYPE_BACTERIUM
    ULONG priority;         // 0->Primary, 1->Secondary
    union {
        struct Cell *sector;
        struct Bacterium *bact;
    } target;
    struct flt_triple pos;  // z.B für Sektoren
};

struct newmaster_msg {
    Object *master;         // inst of ypabact.class oder 1L wenn Welt-Object
    struct Bacterium *master_bact;  // == NULL bei Welt-Object
    struct MinList *slave_list;
};


struct move_msg {

    FLOAT   t;          // frame_time / 1000
    FLOAT   schub_x;    // wenn die Kraftrichtung nicht aus
    FLOAT   schub_y;    // der Objektlage ermittelbar ist...
    FLOAT   schub_z;
    UBYTE   flags;
};

#define MOVE_NOFORCE        1
#define MOVE_STOP           2   // bei Bremsmanöver...

struct getsectar_msg {

    struct Bacterium *Me;       // dös bin ich selbst
    struct Bacterium *SecTarget;// Wenn was gefunden wurde...
    ULONG  flags;
    FLOAT  pos_x;               // Wo soll gesucht werden?
    FLOAT  pos_z;
};

#define GST_MYPOS   (1L<<0)     // pos_x und pos_z nicht auswerten, eigene 
                                // Pos nehmen


struct fight_msg {

    struct flt_triple   pos;
    
    union {
        struct Cell *sector;
        struct Bacterium *bact;
    } enemy;
    ULONG priority;

    FLOAT time;                 // in sec. (frame_time )
    ULONG global_time;
};


struct setstate_msg {

    ULONG   main_state;         // Der HauptStatus, 0 --> ignorieren
    ULONG   extra_on;           // Extra ausschalten   0 --> ign.
    ULONG   extra_off;          // Extra einschalten   0 --> ign.
};


struct firemissile_msg {

    struct  flt_triple dir;     // Anfangsrichtung
    struct  flt_triple start;   // Startpunkt relativ zum lokalen KS
    
    ULONG   target_type;        // nur bei Search-raketen ausfüllen
    union {
        struct Cell *sector;
        struct Bacterium *bact;
    } target;
    struct  flt_triple target_pos;
    
    ULONG   wtype;              // Der gewünschte Typ
    ULONG   global_time;
    ULONG   flags;
};

#define FIRE_VIEW       (1L<<0) // Wir gucken aus der Rakete raus
#define FIRE_CORRECT    (1L<<1) // Das Sektorziel wird auch y-Komponentenmäßig b.

struct setposition_msg {

    FLOAT   x,y,z;              // Die Position höchstselbst
    ULONG   flags;              // Das "WIE" im Umgang mit den Werten
};

#define YBFSP_SetGround         (1<<0)   // Gefährt an Boden anpassen,wenn sinnvoll
                                         // -->zur Zeit nur tank.class
#define YBFSP_DontCorrect       (1<<1)   // keinen test auf sinnvolle position

struct setevent_msg {

    ULONG   event;                  // was soll angemeldet werden?
    WORD    sec_x, sec_y;           // wo war das?
    ULONG   para[4];                // Parameter dazu
};


struct sumparameter_msg {

    ULONG   para;                   // was soll aufaddiert werden
    LONG    value;
};

#define PARA_ENERGY         1
#define PARA_SHIELD         2
#define PARA_NUMBER         3       // Anzahl
#define PARA_MAXENERGY      4
#define PARA_ATTACKER       5


struct should_I_do_it_msg {

    ULONG   priority;               // In Bezug auf welches Ziel sollen die 
                                    // Untersuchung gemacht werden?
                                    // 0: P    1: S
};


struct impulse_msg {

    LONG                impulse;    // Kraftstoß im Zentrum ( der Explosion )
                                    // irgendwas->austesten->nicht real!
    struct flt_triple   pos;        // Position (der Explosion)
    struct uvec         miss;       // Flugrichtung der Miss, äh Rakete
    FLOAT               miss_mass;  // Masse der rakete
};

struct modvehicleenergy_msg {

    LONG        energy;             // was sol abgezogen werden
    struct Bacterium *killer;       // wer war das
};

struct crash_msg {

    ULONG   flags;                  // siehe unten
    ULONG   frame_time;             // in 1000stel Sekunden
};

#define CRASH_CRASH     (1L<<0)     // Abstürzen, sonst Landen
#define CRASH_SPIN      (1L<<1)     // Trudeln beim Abstürzen


struct bactcollision_msg {

    ULONG   frame_time;             // in 1000stel  Sekunden
};


struct recoil_msg {

    struct flt_triple vec;          // der Ebenenvektor
    FLOAT  mul_v;                   // damit wird dof.v multipliziert
    FLOAT  mul_y;                   // mul oder div mit dof.y -->Abrutschen
    FLOAT  time;                    // in sec
};


struct checkforcerelation_msg {

    struct flt_triple pos;          // Wo untersuchen
    LONG   my_energy;               // von der Methode auszufüllen
    LONG   his_energy;
    ULONG  flags;
};

#define CFR_MYPOS        (1L<<0)    // pos ist ungültig, nimm Objektposition
#define CFR_NOSECTOR     (1L<<1)    // betrachte den Sektor nicht
#define CFR_MYSECTORENERGY (1L<<2)  // normalerweise addiere ich die 
                                    // Sektorenergie nicht, wenn es mein Sektor
                                    // ist, weil das sonst Verwirrung bzgl. der
                                    // Feinde stiften kann. Wenn doch, dann Flag

struct giveenemy_msg {

    struct flt_triple pos;          // wo suchen
    ULONG  owner;                   // sein Eigentümer
    Object *master;                 // der Chef als Objekt
    struct Bacterium *mbact;        // der Chef als Bakterium
    ULONG  CommandID;               // seine CommandID
    ULONG  flags;
};

#define GE_MYPOS        (1L<<0)     // Pos ist ungültig, nimm Objektposition


struct getformationpos_msg {

    ULONG           count;          // Pos. im geschwader
    struct flt_triple pos;          // absolut
    struct flt_triple rel;          // relativ;
};


struct canyoudoit_msg {

    LONG        energy;             // dagegen kämpfen wir
    ULONG       flags;
};

#define CYDI_WHILE_FIGHTING     (1L<<0)     // test ist im Kampf, nicht davor


struct checklandpos_msg {

    ULONG       flags;
    FLOAT       distance;
};

#define CLP_NOSLURP         (1L<<0)   // LandPos soll kein Slurp sein
#define CLP_DISTANCE        (1L<<1)   // LandPos max. dist. vom Ziel weg sein
#define CLP_NOBUILDING      (1L<<2)   // LandPos soll kein Gebäude sein
#define CLP_COMMANDERONLY   (1L<<31)  // Test nur als Commander ausführen


struct checkautofirepos_msg {

    struct flt_triple   target_pos;     // wo ist das Ziel?
    ULONG               target_type;    // was ist das Ziel?
    FLOAT               radius;         // für bactAuswertung: wie groß is'n das Ziel so ungefähr, also nicht genau, aber, wenn man mal so, also ich weiß nicht, jetzt ist es auch schon nach 1....
};


struct fireyourguns_msg {

    struct flt_triple dir;      // in die Richtung feuern wir
    FLOAT  time;                // die frame_time in sec.
    ULONG  global_time;
};


struct visier_msg {

    ULONG       flags;          // wen wir suchen
    struct flt_triple dir;      // in welche Richtung wir suchen
    struct Bacterium *enemy;
};

#define VISIER_ENEMY    (1L<<0)     // feinde
#define VISIER_FRIEND   (1L<<1)     // freunde
#define VISIER_NEUTRAL  (1L<<2)     // neutrale


struct handlevischildren_msg {

    ULONG   job;
};

#define HVC_CONNECTCHILDREN         1   // koppelt Kinder an Mutter
#define HVC_REMOVECHILDREN          2   // koppelt se wieder ab


struct organize_msg {

    ULONG  mode;
    struct Bacterium *specialbact;
};

#define ORG_NEWCHIEF            1   // Object wird an specialbact als slave übergeben
#define ORG_BECOMECHIEF         2   // Objekt wird Chef des geschwaders specialbact

/*** Siehe zu den neuen Modi im Quelltext, da sie sich unterscheiden vom v. ***/
#define ORG_NEWCOMMAND          3   // Objekt wird neuer Commander
#define ORG_ADDSLAVE            4   // Geschwader bekommt neuen Slave
#define ORG_CHOOSECOMMANDER     5   // Geschwader sucht sich neuen Chef

/*** Zum Freischwimmen u.ä. ***/
#define ORG_NEARTOCOM           6   // Zielnächster SLAVE(!) wird neuer Com.

struct assesstarget_msg {

    ULONG   target_type;            // Was will ich untersuchen, TARTYPE_...
    ULONG   priority;               // isses das HZ oder NZ
};

/*** AT-Rückkehrwerte ***/
#define AT_REMOVE               0L  // abmelden, also nichts damit machen
#define AT_GOTO                 1L  // zum hinfahren geeignet
#define AT_FIGHT                2L  // zum Bekämpfen geeignet
#define AT_IGNORE               3L  // lassen, aber nicht bearbeiten


struct die_msg {

    BOOL    normal;                 // normaler Tod, mit Meldung, andern-
                                    // falls "Aufräumtod"
};


struct howdoyoudo_msg {

    FLOAT value;                    // der exakte Wert, für weitere Verwendung
};


struct runeffects_msg {

    UBYTE   effects;                // zu welchem Zustand? nutze DEF_...
    UBYTE   p[3];
};


struct findpath_msg {

    // von wo?
    FLOAT   from_x;
    FLOAT   from_z;

    // nach?
    FLOAT   to_x;               // FLOAT, weil ich den Zielpunkt
    FLOAT   to_z;               // unverfaelscht zurueckgeben muss!

    // ergibt welche Ziele?
    WORD    num_waypoints;
    WORD    flags;
    struct  flt_triple waypoint[ MAXNUM_WAYPOINTS ];
};


/*** Flags fuer Sektoren ***/
#define SPF_InClose     (1<<0)      // In Closelist
#define SPF_InOpen      (1<<1)      // In Openlist

/*** Werte fuer Pfadfindung ***/
#define SPV_WallHeight      500.0   // das ist ein Wall
#define SPV_CritHeight      300.0   // da wird der Cross-Slurp kritisch
#define SPV_Forbidden       100     // Obstacle-Wert fuer unpassierbar

/*** Flags fuer findpath_msg ***/
#define WPF_Normal          (1<<0)  // normal Zielsetzung
#define WPF_StartWayPoint   (1<<1)  // Beginn einer Wegpunkt-kette
#define WPF_AddWayPoint     (1<<2)  // zusaetzlicher Wegpunkt


/*---------------------------------------------------------------------
** diverse Vereibarungen 
*/

#define YPS_PLANE       0.6   //0.72        // "Steilheit"-Test, 0.71 sind ca.45 Grad
                                    // vor allem für Boden-Subklasses

#define THINK_YPS       (2000.0)    // außerhalb keine y-Komponente für tar_vec
                                    // muß schon groß sein!

#define CRASH_SPEED     (15.0)      // diese Geschwindigkeit muß ein Vehicle
                                    // haben, um beim Aufprall bei Energy <= 0
                                    // zu explodieren und um überhaupt Energie zu v.

#define NOISE_SPEED     (7.0)       // die Geschwindigkeit muß ein Objekt haben,
                                    // um beim Aufprall etwas hören zu lassen
                                    // muß hoch sein, weil es in Startphasen und 
                                    // kleineren Abprallserien viel rumpelt

#define NO_AUTOWEAPON   255         // autoID: keine Raketen etc.

#define NO_MACHINEGUN   255         // MG_Shot: keine Bordkanone etc.

#define FOLLOW_DISTANCE (400.0)     // wenn ich als Chef soweit weg bin, folgen sie mir
                                    // für AI1 das doppelte!

#define OR_ANGLE        (0.003)     // wenn Abweichung von globaler y-Achse größer
                                    // ist, dann Ausrichtung

#define TAR_DISTANCE    (300.0)     // innerhalb derer gehen wir in WAIT über, wenn
                                    // wir dürfen ( -->siehe auch yb_fight.c)

#define IS_SPEED        (0.1)       // darunter ist es de facto Stillstand

#define BEAM_TIME       (3000)      // solange dauert ein Beameffekt

#define BEAM_MAX        (30)        // Verzerrungsfaktor

#define ENERGY_PRO_DEST (300)       // soviel mal mehr haben wir Zerstörungsleistung
                                    // als Energieverlust (beim Schießen)

#define BF_TIME         (5000)      // nach dieser zeit ein test auf Festhängen

#define FAR_FROM_SEC_TARGET     ( 3 * SECTOR_SIZE ) // dann advance-te nebenziel-
                                                    // tests bzgl. Vehiclezahl

#define GRAV_FACTOR     (4.0)       // Soviel mal mehr GravKraft gibts beim
                                    // Fallen

#define FE_TIME         (45000)     // Ausbremsung für Feindsicht-Meldung
                                    // siehe bact->fe_time
/*-----------------------------------------------------------------*/
#endif

