#ifndef YPA_YPAWORLDCLASS_H
#define YPA_YPAWORLDCLASS_H
/*
**  $Source: PRG:VFM/Include/ypa/ypaworldclass.h,v $
**  $Revision: 38.39 $
**  $Date: 1998/01/14 17:09:42 $
**  $Locker: floh $
**  $Author: floh $
**
**  Die ypaworld.class ist die Welt-Klasse des Spiels. Ahmen.
**
**  (C) Copyright 1995 by A.Weissflog & A.Flemming
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

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef BITMAP_BITMAPCLASS_H
#include "bitmap/bitmapclass.h"
#endif

#ifndef VISUALSTUFF_FONT_H
#include "visualstuff/font.h"
#endif

#ifndef SKELETON_SKELETONCLASS_H
#include "skeleton/skeletonclass.h"
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

#ifndef BASECLASS_BASECLASS_H
#include "baseclass/baseclass.h"
#endif

#ifndef BITMAP_DISPLAYCLASS_H
#include "bitmap/displayclass.h"
#endif

#ifndef YPA_YPAGUI_H
#include "ypa/ypagui.h"
#endif

#ifndef YPA_BACTERIUM_H
#include "ypa/bacterium.h"
#endif

#ifndef YPA_BACTCLASS_H
#include "ypa/ypabactclass.h"
#endif

#ifndef YPA_YPAVEHICLES_H
#include "ypa/ypavehicles.h"
#endif

#ifndef YPA_YPARECORD_H
#include "ypa/yparecord.h"
#endif

#ifndef YPA_YPALOCALE_H
#include "ypa/ypalocale.h"
#endif

#ifndef YPA_GUIHUD_H
#include "ypa/guihud.h"
#endif

#ifndef YPA_GAMESHELL_H
#include "ypa/ypagameshell.h"
#endif

#ifndef YPA_YPAHISTORY_H
#include "ypa/ypahistory.h"
#endif

#ifndef YPA_YPAPLAYERSTATS_H
#include "ypa/ypaplayerstats.h"
#endif

#ifndef YPA_YPAVOICEOVER_H
#include "ypa/ypavoiceover.h"
#endif

#ifndef YPA_YPACOLOR_H
#include "ypa/ypacolor.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      ypaworld.class -- Welt-Klasse für Your Personal Amok
**                        (Arbeitstitel)
**
**  FUNCTION
**      Die Welt-Klasse verwaltet die Spiel-Welt als Sektor-Map,
**      sowie die Energie-Beziehungen zwischen den Spiel-Elementen.
**      Wesentliche Bestandteile sind der Zellautomat und Routinen
**      zur Level-Initialisierung. Yo!
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      base.class
**      ~~~~~~~~~~
**      ypaworld.class
**      ~~~~~~~~~~~~~~
**      YWM_MODSECTORENERGY
**          Msg:    struct energymod_msg
**          Ret:    ---
**
**          'Außerplanmäßige' Energie-Modifikation eines Sektors
**          von außen, z.B. infolge von Beschuß.
**          Übergeben wird eine absolute 3D-Position, die innerhalb
**          der Methode in eine Sektor- bzw. Subsektor-ID umgerechnet
**          wird. Die Energie wird nicht direkt abgezogen, sondern
**          durch den prozentualen "Rüstungsschutz" des betroffenen
**          Legos verringert.
**
**      YWM_GETSECTORINFO
**          Msg:    struct getsectorinfo_msg
**          Ret:    ULONG
**
**          In der Msg wird die 3D-Position des Objects im Welt-Koordinaten-
**          System übergeben. YWM_GETSECTORINFO errechnet daraus
**          folgende Sachen:
**
**              - x,y Koordinate in Sektor-Map
**              - Pointer auf <struct Cell> in Sektor-Map
**              - relative x,z Position innerhalb des Sektors
**                (ohne die Höhe!)
**
**          Diese Daten werden in die Message zurückgeschrieben.
**
**          Falls die übergebene Position ungültig ist (außerhalb
**          der Sektor-Map), wird eine 0L zurückgegeben, sonst
**          eine 1L.
**
**      YWM_SETVIEWER
**          Msg:    struct Bacterium
**          Ret:    ---
**
**          Signalisiert dem Welt-Object einen neuen Viewer
**          (in Form einer Bacterium-Struktur).
**
**      YWM_GETVIEWER
**          Msg:    ---
**          Ret:    <struct Bacterium *>
**
**          Returniert den Bacterium-Pointer des aktuellen Viewers.
**
**      YWM_GETHEIGHT
**          Msg:    struct getheight_msg
**          Ret:    ---
**
**          Bietet Low-Level-Support für Kollisions-Check gegen
**          'Umwelt' (sprich Sektor-Map). Übergeben wird ein
**          Sektor-Pointer, sowie eine Position innerhalb
**          des Sektors (nämlich die relative x/z-Position).
**          Die Methode ermittelt die 'Boden-Höhe' an dieser
**          Position (fehlendes Y, Ausrichtung beachten), und
**          den Polygon, der 'getroffen' wird. Die Rückgabe-Werte
**          werden in die Msg zurückgeschrieben.
**          Siehe <struct getheight_msg> Definition.
**
**      YWM_GETVISPROTO
**          Msg:    ULONG Type
**          Ret:    Object * [instance of base.class]
**
**          Gibt einen Pointer auf den Visual Prototype des
**          angeforderten Typs durch. Returniert NULL, falls
**          angeforderter Prototype nicht existiert!
**
**      YWM_ADDCOMMANDER
**          Msg:    Object  [instance of ypabact.class]
**          Ret:    ---
**
**          Hängt ein neues Bakterien-Object an die Commander-Liste
**          des Welt-Objects. Das Welt-Object wendet innerhalb
**          YWM_ADDUNIT ein YBM_NEWMASTER mit einem <Object *> == 1L
**          (als Zeichen dafür, daß es sich um das Welt-Object
**          handelt) und einen Pointer auf den CommanderList-Header.
**          In jedem Frame schickt das Welt-Object ein YBA_TR_LOGIC
**          an die Commanders, bei einem Dispose des Welt-Objects
**          werden die Commanders vom Welt-Object disposed.
**
**          Bitte beachten, daß die Methode nur mit _methoda()
**          benutzt werden kann!
**
**      YWM_NEWLEVEL [OBSOLETE!!! -> YWM_CREATELEVEL benutzen!]
**          Msg:    struct newlevel_msg
**          Ret:    1 -> alles OK
**                  0 -> Ooops, Fehler
**
**          Initialisiert einen neuen Level durch Einlesen und
**          Auswerten von 3 Level-Description-Bitmaps für
**          Höhen-, Eigentümer- und Sektortyp-Information.
**
**      YWM_KILLLEVEL
**          Msg:    ---
**          Ret:    ---
**
**          Gegenstück zu YWM_NEWLEVEL und YWM_CREATELEVEL. Normalerweise 
**          braucht man YWM_KILLLEVEL nicht aufzurufen, weil YWM_NEWLEVEL
**          das vorsichtshalber selbst nochmal macht.
**
**      YWM_CREATELEVEL
**          Msg:    struct createlevel_msg
**          Ret:    1 -> alles OK
**                  0 -> ein Fehler
**
**          Neuer Ersatz für YWM_NEWLEVEL, alles weitere ist
**          in der Message-Definition erklärt.
**
**      YWM_INTERSECT
**          Msg:    struct intersect_msg
**          Ret:    msg ausgefüllt...
**
**          Testet einen 3D-Vektor auf Intersection mit
**          dem Gelände. Besonderheiten: Falls mehrere Intersections
**          auftreten, wird die dem Startpunkt am nächsten liegende
**          zurückgegeben (kleinstes <t>). Intersections mit
**          "Flächen-Rücken" werden ignoriert. Man kann also "von hinten"
**          durch eine Fläche durchfliegen. Das ist (a) effizienter
**          zu berechnen, (b) kann man sich damit nicht "auf der
**          falschen Seite der Welt" verfangen, wenn man aus irgendwelchen
**          Gründen dorthin gelangt ist...
**
**          In:     
**              msg.pnt     -> globale 3D-Start-Koordinate des Test-Vektors
**              msg.vec     -> Richtung und Länge des Test-Vektors
**
**          Out:
**              msg.pnt     -> relative 3D-Position zum getesteten Skeleton
**              msg.vec     -> unverändert
**              msg.insect  -> TRUE: Intersection aufgetreten, dann:
**              msg.t       -> t-Value der Intersection
**              msg.ipnt    -> genauer 3D-Point der Intersection
**              msg.pnum    -> Nummer des Polygons in <msg.sklt>, mit dem
**                             die "näheste" Intersection auftrat.
**              msg.sklt    -> Pointer auf Skeleton-Struktur. Diese Skeleton-
**                             Struktur ist nur gültig bis zur nächsten
**                             Anwendung von YWM_INTERSECT, weil für die
**                             Slurps temporäre Sklt-Strukturen berechnet
**                             werden müssen!
**
**          Da die Welt intern in "Subsektoren" und "Slurps" aufgeteilt
**          ist, kann sich der Test-Vektor theoretisch über mehrere dieser 
**          Bereiche erstrecken. Damit die Testerei nicht ausartet, wird 
**          maximal gegen den Sub-Bereich, in den der Startpunkt des Sektors
**          liegt und gegen den Sub-Bereich, in den der Endpunkt liegt
**          getestet. Liegen beide Punkte im selben Sub-Bereich, wird
**          natürlich nur gegen diesen getestet.
**
**      YWM_INSPHERE
**          Msg:    struct insphere_msg
**          Ret:    msg ausgefüllt
**
**          Testet, ob ausgehend von einem 3D-Punkt eine Intersection
**          mit der Umwelt innerhalb eines gegebenen Radius auftritt.
**          Das ist kein *exakter* Kugel-Intersection-Test, aber
**          in etwa damit vergleichbar. Zurückgegeben werden alle
**          festgestellten Intersections (siehe <struct insphere_msg>
**          und <struct Insect>.
**
**          In:
**              msg.pnt         -> 3D-Punkt
**              msg.radius      -> Test-Radius
**              msg.chain       -> Pointer auf leeren Puffer, in denen
**                                 <struct Insect>'s reingeschrieben werden,
**                                 für jede Intersection eine Struktur
**              msg.max_insects -> soviele Intersections werden max. ausgewertet
**                                 (msg.chain-Buf so groß bemessen!)
**              msg.dof         -> hält aktuelle Flugrichtung, wegen
**                                 "Flächenrücken-Unterdrückung)
**
**          Out:
**              msg.num_insects -> festgestellte Anzahl Intersections.
**
**      YWM_ADDREQUESTER
**          Msg:    struct YPAReq
**          Ret:    ---
**
**          Hängt eine neue Requester-Struktur in die interne Requester-
**          Liste des Welt-Objects. Der Requester wird an das Ende
**          der Liste gehängt. Das Welt-Object handelt Iconify,
**          Move Requester und Requester To Front automatisch ab,
**          außerdem stellt die Welt-Klasse die Requester automatisch
**          dar. Um den Rest muß sich der Besitzer des Requesters
**          selber kümmern.
**
**          Folgende Standard-ClickButton-IDs sind zu benutzen:
**              YPAGUI_ICONIFY
**              YPAGUI_DRAGBAR
**              YPAGUI_ICON
**
**          Zumindest diese Buttons muß jeder Requester unterstützen!
**
**      YWM_REMREQUESTER
**          Msg:    struct YPAReq
**          Ret:    ---
**
**          Entfernt diesen Requester aus interner Requester-Liste
**          des Welt-Objects.
**
**      YWM_MODEMENUCONTROL
**          *** OBSOLETE *** OBSOLETE *** OBSOLETE ***
**
**      YWM_UNITMENUCONTROL
**          *** OBSOLETE *** OBSOLETE *** OBSOLETE ***
**
**      YWM_OBTAINVEHICLE
**          Msg:    <struct obtainvehicle_msg>
**          Ret:    <Object *>
**
**          ACHTUNG: Diese Methode ist mit Einführung von 
**          YWM_CREATEVEHICLE und YWM_CREATEWEAPON auf einen
**          niedrigeren Level abgerutscht und sollte nicht mehr
**          von Robos verwendet werden (nur noch von Vehikeln,
**          die autonome Waffen anfordern).
**
**          Hiermit wird ein neue Bakterie vom Welt-Objekt angefordert.
**          Übergeben werden muß ein Vehikel-Typ (siehe ypavehicles.h)
**          und eine Init-Position. Das gewünschte Objekt wird bevorzugt aus
**          dem internen Toten-Cache reinkarniert, ansonsten wird
**          es per _new() erzeugt.
**          Das Objekt muß extern in die Kommando-Struktur eingebunden
**          werden.
**          Das Objekt *ist*bereits* in die richtige Sektor-Bakterien-Liste
**          integriert (wird bestimmt über die Position).
**
**      YWM_RELEASEVEHICLE
**          Msg:    <Object *>  es macht also nur methoda() Sinn!!!
**          Ret:    ---
**
**          Nimmt das per Msg definierte Objekt in den internen
**          Toten-Cache auf. Das Objekt muß aus der Kommando-
**          Struktur isoliert sein, also:
**
**              bact.master ist NULL
**              bact.slave_list ist leer (keine Untergebenen)
**              bact.slave_node ist in keine Liste eingeklinkt
**
**          Das Objekt *MUSS* aber noch in seiner Sektor-Bakterien-
**          Liste eingeklinkt sein (weil es ja weiterhin dargestellt
**          wird).
**
**          Das Welt-Objekt verwendet zum Einklinken in den Toten-
**          Cache die <slave_node> indirekt, indem es ein
**          normales YBM_NEWMASTER auf das Objekt anwendet.
**
**          Die Methode ist zuständig für alle Objekte, die
**          per YWM_OBTAINVEHICLE, YWM_CREATEWEAPON und 
**          YWM_CREATEVEHICLE erzeugt wurden.
**
**      YWM_ISVISIBLE
**          Msg:    <struct Bacterium>
**          Ret:    BOOL
**
**          Testet, ob sich das angegebene Bakterium zur Zeit im
**          Sichtbereich des aktuellen Viewers befindet.
**          Returniert TRUE, falls das Bakterium prinzipiell
**          sichtbar ist, FALSE, falls es definitiv unsichtbar ist.
**
**      YWM_CREATEVEHICLE
**          Msg:    <struct createvehicle_msg>
**          Ret:    <Object *>
**
**          Das ist die High-Level-Version von YWM_OBTAINVEHICLE.
**          Übergeben wird eine "Vehicle-Nummer", also ein direkter
**          Index in das neue Vehicle-Array (nicht das VProto-Array!).
**          Das Welt-Objekt wird dann mittels YWM_OBTAINVEHICLE
**          das entsprechende Objekt erzeugen und zurückgeben. Die
**          neue Methode wurde eingeführt, weil im VisualPrototype
**          Array sowohl Waffen als auch Vehikel liegen (was ja
**          auch Logo ist). Im neuen Vehicle-Array liegen mehr
**          strategische Informationen über ein Fahrzeug, die der
**          Robo benutzen kann, wenn er selbst ein Fahrzeug erzeugen
**          will.
**          Die normale Verfahrensweise beim Erzeugen eines neuen
**          Fahrzeugs wäre für einen AI-Robo also: die interne
**          Vehicle-Liste des Welt-Objects scannen und einen Fahrzeug-
**          Typ heraussuchen, dabei das Footprint-Feld beachten!
**          Dann mit YWM_CREATEVEHICLE das Fahrzeug anfordern, das
**          neue Fahrzeug für n Sekunden in den Genesis-Zustand
**          schalten.
**
**          Achtung: Es ist doch nicht möglich, den Genesis-Zustand
**          im Welt-Objekt abzuwickeln, weil es ja eine kontinuierliche
**          Aktion ist, die über das normale Zustands-Handling abgehandelt
**          werden muß (visuell: rotieren und skalieren, logisch:
**          Energie-Happen zu jedem Frame...). 
**
**          Problem: wie wird dem neuen Objekt mitgeteilt, daß der
**          User noch auf dem "Create-Button" hängt, also noch mehr
**          Energie in das neue Objekt pumpen will? Umständlich...
**          ist das überhaupt nötig? -> Besser wäre, wenn neue
**          Objekte generell mit voller Ladung erzeugt werden...
**
**          Siehe YWA_VehicleArray!
**
**      YWM_CREATEWEAPON
**          Msg:    <struct createweapon_msg>
**          Ret:    <Object *>
**
**          Funktioniert exakt wie YWM_CREATEVEHICLE, nur das eine
**          autonome Waffe erzeugt wird. Das Welt-Objekt wird die
**          Waffe intern mittels YWM_OBTAINVEHICLE erzeugen und einen
**          Pointer darauf zurückgeben. YWM_CREATEWEAPON darf nur
**          verwendet werden, wenn die entsprechende Waffe im Weapon-
**          Array das WF_Autonom Flag gesetzt hat (bei nicht-
**          autonomen Waffen muß nur gecheckt werden, ob der Robo
**          diese Waffe verwenden darf (per Footprint).
**
**          Wichtig: Wird nur von der AI eines Robos angewendet!
**          Normale Vehikel verwenden nach wie vor hardgecodet
**          YWM_OBTAINVEHICLE...
**
**          siehe YWA_WeaponArray!
**
**      YWM_CREATEBUILDING
**          Msg:    <struct createbuilding_msg>
**          Ret:    BOOL
**
**          Teilt dem Welt-Objekt mit, das ein bestimmter Sektor
**          mit einem Gebäude bebaut werden soll. Der Gebäude-Typ
**          entspricht dem Index ins BuildArray, wo das Gebäude
**          näher beschrieben ist. Wenn alle Bedingungen erfüllt sind,
**          beginnt das Welt-Objekt mit dem Bau-Prozeß, der sich
**          über längere Zeit hinziehen wird. Falls NICHT gebaut werden
**          konnte, returniert die Methode FALSE (Gründe können sein:
**          Ziel-Sektor nicht frei oder nicht erobert, zu viele
**          Bau-Prozesse in Gang etc...).
**
**          Der Sektor wird neutral und ohne Energie bleiben,
**          bis der Bauprozeß beendet ist.
**
**          Um die Anzahl der parallel laufenden Bauprozesse niedrig
**          zu halten, werden Bauprozesse eines Robos solange abgewiesen,
**          bis der vorhergehende Bauprozeß des selben Robos beendet
**          ist. Es können also maximal MAXNUM_ROBOS Bauprozesse 
**          parallel laufen.
**
**          Neu: In der <createbuilding_msg> existiert ein
**          neues Feld <immediate>. Ist dieses TRUE, wird das
**          Gebäude sofort "hingesetzt", also nicht hochskaliert.
**
**          Bitte beachten, daß zwei unterschiedliche "Owner-Infos"
**          existieren: <job_id> und <owner>.
**          Das <job_id> Feld wird zum Locking eine Bauen-Jobs
**          in der Welt verwendet. Kommt ein Bauauftrag mit
**          derselben <job_id> rein, wird dieser abgewiesen,
**          bis der entsprechende Job-Slot wieder frei ist.
**          Die <job_id> kann dem <owner> Feld entsprechen
**          (das wäre eine einfache Lösung), wenn wir allerdings
**          mehrere Robos desselben Owner zulassen, können
**          bei identischer <job_id> und <owner_id> Bauanfragen
**          gegenseitig behindern. Auf alle Fälle ist
**          YWM_CREATEBUILDING für den Fall der Fälle vorbereitet.
**
**      YWM_INTERSECT2
**          Msg:    struct intersect_msg
**          Ret:    msg ausgefüllt...
**
**          Alternative Intersect-Methode, zu ungenau für
**          Erdfahrzeuge (der bekannte Diagonal-Fehler),
**          kann aber dafür beliebig lange Test-Vektoren
**          abhandeln. Habe ich mir hauptsächlich geschrieben
**          für den Mouse-Check, schon getestet und für Fahrzeuge
**          als unfähig befunden...
**
**      YWM_INBACT
**          Msg:    struct inbact_msg
**          Ret:    void
**
**          Testet, ob der Vektor ein Bakterium schneidet. Es
**          wird das Bakterium zurückgegeben, das dem Start-Punkt
**          am nächsten liegt. Der Vektor darf sich über mehrere
**          Sektoren erstrecken.
**
**      YWM_MODSECTORENERGY2
**          Msg:    struct mse2_msg
**          Ret:    void
**
**          Neue Version von ModSector-Energy, etwas effizienter
**          etwas mehr beeinflussbarer durch Flags, sonst
**          wie YWM_MODSECTORENERGY. Der Unterschied ist, daß
**          nicht mit direkten 3D-Koordinaten, sondern mit
**          Sektor- und Subsektor-Koordinaten gearbeitet wird.
**
**          Zum Ermitteln der Sektor- und Subsektor-Koordinaten
**          schaue man sich bitte im YWM_MODSECTORENERGY-Quelltext
**          um (also die alte 3D-Punkt-Version).
**
**              mse2_msg.sec_x      - Sektor-X-Koordinate
**              mse2_msg.sec_y      - Sektor-Y-Koordinate
**              mse2_msg.subsec_x   - Subsektor-X-Koord [0..2]
**              mse2_msg.subsec_y   - Subsektor-Y-Koord [0..2]
**              mse2_msg.energy     - Energie-Menge, die *abgezogen* wird
**                                    (kann natürlich auch negativ sein)
**              mse2_msg.flags:
**                  MSE2F_IgnoreShild   - Schild ignorieren
**                  MSE2F_Overkill      - gesamten Sektor auf 0!
**
**          Die Methode realisiert alle Nebenwirkungen wie
**          die 3D-Punkt-Version.
**          Falls es sich um einen Kompakt-Sektor handelt, werden
**          <subsec_x> und <subsec_y> ignoriert.
**
**      YWM_VISOR
**          Msg:    struct visor_msg
**          Ret:    void
**
**          Kontrolliert das von der Welt-Klasse gerenderte
**          Visier. Die Methode muß innerhalb der Trigger-Logic-
**          Schleife genau einmal (nämlich vom Viewer-Fahrzeug)
**          auf die Welt-Klasse angewendet werden. In der Message wird
**          je ein Visiertyp für das MG (wird immer in Screen-Mitte
**          gezeichnet) und für die autonome Waffe angegeben
**          (jeweils VISORTYPE_NONE, wenn das jeweilige Visier
**          nicht existiert). Für die autonome Waffe wird außerdem
**          die Position des Visiers als FLOAT [-1.0 .. +1.0]
**          angegeben.
**
**      YWM_INITGAMESHELL
**          Msg:
**          Ret: BOOL
**
**          Initialisiert das grafische Interface der GameShell. Rückkehr-
**          wert ist TRUE, wenn diese Sache erfolgreich war. Das gegenstück dazu
**          ist FREEGAMESHELL. Die GameShell-struktur ist kein teil der Welt,
**          muß also extern alloziert und durch FREE... vor dem Disposen der welt
**          wieder freigegeben werden.
**          So ist die gameshell weitestgehend von der Welt entkoppelt.
**
**      YWM_TRIGGERGAMESHELL
**
**          TriggerMethode für die GAmeShell. Wird alternativ zu TRIGGER der
**          Welt aufgerufen.
**
**      YWM_OPENGAMESHELL, YWM_CLOSEGAMESHELL
**
**          Darstellung der GameShell erlauben/abbrechen
**
**      YWM_REALIZESHELLACTION
**
**          Erledigt die Aufgaben, die in der Shell ausgewählt wurden.
**          Um eine Trennung und anderweitige Verwendung zu garantieren, wird
**          eine spezielle Message übergeben, die allerdings auch in der
**          GSR-Struktur enthalten ist (logischerweise)
**
**      YWM_LOGMSG
**          Msg:    struct logmsg_msg
**          Ret:    void
**
**          Hiermit darf jeder Messages ins LogWindow der Welt-Klasse
**          schreiben. Bevorzugt sollten das aber nur die Commander
**          machen, damit die Sache nicht überhand nimmt.
**          Wenn die Msg von einer Bakterie kommt, MUSS ein Bakterien-
**          Pointer mit übergeben werden. Es kann außerdem eine
**          Priorität angegeben werden, diese wird aber unter
**          Umständen nur für einen Signal-Ton verwendet werden.
**          Siehe auch <struct yw_logmsg_msg>.
**
**          Für die Priorität gelten folgende Grenzwerte:
**              0..100  - "Mild Alert", eine Standard-Message
**            100..200  - "Yellow Alert", eine wichtige Message
**            200..255  - "Red Alert", eine verdammt wichtige Message
**
**          *** WICHTIG ***
**          Die Msg sollte nur Großbuchstaben und die üblichen
**          Sonderzeichen enthalten, alles andere wird abgefangen!!!
**
**          Ach so: Die Msg kann per '\n' in mehrere Zeilen unterteilt
**          werden, sollte aber ansonsten keine derartigen Formatierungs-
**          zeichen enthalten. Pro Zeile sollten max. 64 Buchstaben
**          enthalten sein (aber je weniger, je besser, weil das
**          LogWindow in der Regel nicht sehr groß sein wird!).
**
**      YWM_INITPLAYER
**          Msg:    struct initplayer_msg
**          Ret:    1 -> alles OK
**                  0 -> ein Fehler
**
**          Ersatz für YWM_CREATELEVEL im Player-Modus. Angegeben
**          wird der Name eines #?.scn Files. Die Methode
**          initialisiert daraus den kompletten Level
**          (dazu wendet sie ein YWM_CREATELEVEL an),
**          und erzeugt die "virtuelle Kamera". Dann wird
**          der 1.Frame geladen um einen definierten
**          Ausgangs-Zustand herzustellen. Wenn die
**          Methode TRUE zurückkommt, benutzt man einmal
**          pro YWM_TRIGGERPLAYER, wenn YWA_LevelFinished
**          auf TRUE geht, wendet man ein YWM_KILLPLAYER
**          an, und fertig.
**
**      YWM_TRIGGERPLAYER
**          Msg:    struct trigger_msg
**          Ret:    ---
**
**          YWM_TRIGGERPLAYER ersetzt BSM_TRIGGER im
**          Player-Modus (also wenn eine Sequenz abgespielt
**          wird).
**
**      YWM_KILLPLAYER
**          Msg:    ---
**          Ret:    ---
**
**          Ersatz für YWM_KILLLEVEL im Player-Modus (in fact,
**          die Methode macht derzeit nichts weiter als ein
**          YWM_KILLLEVEL).
**
**      YWM_PLAYERCONTROL
**          Msg:    struct playercontrol_msg
**          Ret:    ---
**
**          Kontrolliert Player-Verhalten (quasi das Bedieninterface
**          des Players):
**
**              PLAYER_STOP   - auf aktuellem Keyframe stop
**              PLAYER_PLAY   - abspielen mit Keyframe-Interpolation
**              PLAYER_GOTO   - Goto Key-Frame-Nummer (STOP oder PLAY
**                              bleibt normal aktiv)
**                              spezielle Werte: -1 -> letzter Frame
**              PLAYER_NEXT   - springe auf nächsten Keyframe
**              PLAYER_PREV   - springe auf vorherigen Keyframe
**              PLAYER_RECORD - wie PLAY, nur mit Aufzeichnung,
**                              PCONTROL_STOP stoppt die Aufzeichnung
**              PLAYER_RELPOS - Relativ-Positionierung (arg ist Anzahl Frames
**                              vom aktuellen)
**
**          Das Ergebnis von YWM_PLAYERCONTROL wird erst in 
**          YWM_TRIGGERPLAYER sichtbar, welches zyklisch weiterhin
**          angewendet wird (unabhängig vom eingestellten PCONTROL-Modus!)
**
**      YWM_SETLANGUAGE
**          Msg:    struct setlanguage_msg
**          Ret:    ULONG [TRUE/FALSE]
**
**          Initialisiert das Locale-System für eine neue
**          Sprache. Aus dem übergebenen Language-ID-String
**          (z.B. "deutsch") wird ein Filename folgender
**          Form gebaut:
**
**              "levels/locale/deutsch.lng"
**
**          Dieses Script wird durch den Locale-Parser
**          gejagt. Falls was schiefgeht, kommt FALSE
**          zurück. In diesem Fall ist das Locale-
**          System zurückgesetzt, das heißt,
**          ypa_GetStr() wird immer den Default-String
**          zurückliefern!
**
**      YWM_BEAMNOTIFY
**          Msg:    struct beamnotify_msg
**          Ret:    ---
**
**          Teilt der Weltklasse mit, daß ein ***User-Vehikel***
**          ein offenes BeamGate betreten hat. Es dürfen
**          nur Fahrzeuge gemeldet werden, die in den
**          nächsten Level mitgenommen werden können
**          (also keine Feinde, Flaks oder autonome Waffen!).
**
**          Der User-Robo wird ebenfalls ganz normal über diese
**          Methode abgehandelt. Innerhalb YWM_BEAMNOTIFY
**          findet ein Test statt, ob der User-Robo reinkam,
**          in diesem Fall wird das interne LevelFinished Flag
**          gesetzt, das heißt, nach "Fertigstellung" des aktuellen
**          Frames wird der Level beendet.
**
**          Visuelle BeamGate-Betreten-Effekte müssen *vor* dem
**          Anwenden der YWM_BEAMNOTIFY-Message erledigt worden sein.
**          Das Fahrzeug MUSS SICH NOCH AUF DEM BEAM-GATE-SEKTOR
**          befinden! Weil ich per Bact->Sector->WType
**          herausfinden muß, über welches Beamgate der
**          Level verlassen wurde.
**
**      YWM_GETRLDRATIO
**          Msg:    struct getrldratio_msg
**          Ret:    ---
**
**          Returniert das Energie-Reload-Ratio für einen
**          gegebenen Owner. Dieser Wert ist grob das
**          Verhältnis der eroberten Kraftwerke zu der
**          Anzahl der eroberten Level. Der Rückgabewert
**          ist eine FLOAT Zahl zwischen 0.0 und 1.0
**          (wird in msg zurückgeschrieben).
**
**      YWM_NOTIFYDEADROBO
**          Msg:    struct notifydeadrobo_msg
**          Ret:    ---
**
**          MUSS auf die Weltklasse angewendet werden, wenn
**          ein Robo tot geht (irgendwo vor YWM_RELEASEVEHICLE).
**          Die Weltklasse erfährt hiermit, von welchem Owner
**          der Robo gekillt wurde, um das Land des toten
**          Owners auf den Killer umzuschichten.
**          Die Methode darf nur angewendet werden, wenn der
**          Robo durch Fremdeinwirkung (autonome Waffe oder MG)
**          getötet wurde!
**          Anmerkung: Falls ein zweiter Robo desselben Owners
**          existiert, wird das Land erst umgeschichtet,
**          wenn auch dieser Robo tot geht, diese Entscheidung
**          trifft die Methode aber selbst.
**
**      YWM_NETWORKLEVEL
**          Msg: struct createlevel_msg;
**          ret: BOOL success
**          Ist das Gegenstück zu CREATELEVEL für ein Netzwerkspiel
**
**      YWM_FORCEFEEDBACK
**          Msg: struct yw_forcefeedback_msg
**          Ret: ---
**
**          Hiermit melden Bakterien dem Weltobjekt einen
**          ForceFeedback-Effekt.
**
**      YWM_NOTIFYWEAPONHIT
**          Msg: struct yw_notifyweaponhit_msg
**          Ret: ---
**
**          Hiermit meldet ein User-Vehicle(!), daß es von einer
**          Rakete getroffen wurde (auch die Robos, wenn's geht!).
**          Die Message enthält Infos über die Stärke des Treffers
**          (mit Schild schon abgezogen!).
**
**      YWM_ADVANCEDCREATELEVEL
**          Msg: struct createlevel_msg
**          Ret: ---
**
**          Kapselt YWM_CREATELEVEL, lädt, falls vorhanden, einen
**          Finalsavegame-File, und legt einen Restart-File an.
**
**      YWM_NOTIFYHISTORYEVENT
**          Msg: (siehe ypahistory.h)
**              struct ypa_HistNewFrame
**              struct ypa_HistConSec
**              struct ypa_HistVhclKill
**          Ret: ---
**
**          Teilt dem History-Event-Recorder ein aufzeichnungs-
**          würdiges Ereignis mit. Siehe <ypa/ypahistory.h>
**          für eine genaue Beschreibung der erwarteten Messages.
**
**      YWM_ONLINEHELP
**          Msg: struct yw_onlinehelp_msg
**          Ret: ---
**
**          Startet die Online-Hilfe. Blockiert, bis YPA wieder
**          die aktive Applikation ist.
**
**  ATTRIBUTES
**      YWA_MapMaxX     (IG)    [ULONG]
**      YWA_MapMaxY     (IG)    [ULONG]
**          Definiert MAXIMALE Größe der Sektor-Map für die
**          Lebensdauer des Welt-Objects. Muß ausreichend
**          bemessen sein, um den größten Level im Spiel
**          aufnehmen zu können. OM_NEW allokiert einen
**          Speicher-Block für diese maximale Sektor-Map-Größe.
**
**      YWA_MapSizeX    (G)     [ULONG]
**      YWA_MapSizeY    (G)     [ULONG]
**          Enthalten die Map-Größe des akt. Levels in Sektoren.
**          Darf nur abgefragt werden!
**
**      YWA_SectorSizeX (G)     [FLOAT]
**      YWA_SectorSizeY (G)     [FLOAT]
**          Gibt Größe eines Sektors im 3D-Raum zurück.
**
**      YWA_WorldDescription (I)    [UBYTE *]
**          Definiert Filenamen des Welt-Beschreibungs-Files.
**          MUSS bei OM_NEW angegeben werden.
**
**      YWA_NormVisLimit     (ISG) [ULONG] def=1500
**      YWA_NormDFadeLen     (ISG) [ULONG] def=600
**      YWA_HeavenVisLimit   (ISG) [ULONG] def=8000
**      YWA_HeavenDFadeLen   (ISG) [ULONG] def=6000
**          Beschreibt Depthfading-Parameter für "normale" Sektoren
**          (und Slurps) und den Himmel.
**
**      YWA_HeavenHeight    (ISG) [LONG]  def=-3500
**          Definiert Höhe des Himmels (negative Werte -> Höher).
**
**      YWA_RenderHeaven    (ISG) [BOOL]  def=TRUE
**          Zeichne Himmel ja/nein.
**
**      YWA_DoEnergyCycle (ISG) [BOOL] def=TRUE
**          Energie-Zyklus an/aus.
**
**      YWA_VisSectors      (ISG)    [ULONG] def=3
**          Definiert die Anzahl der sichtbaren Sektoren um den
**          Viewer herum. Intern wird sichergestellt, daß diese
**          ungerade ist (|=1). YWA_NormVisLimit und YWA_NormDFadeLen
**          werden *nicht* beeinflußt (also aufpassen, das diese
**          Attribute auf sinnvolle Werte gesetzt werden).
**
**      YWA_VTypes  (G)
**          Returniert Pointer auf internes Visualproto-Array.
**
**      YWA_UserRobo      (SG) [Object *]
**      YWA_UserVehicle   (SG) [Object *]
**          Mit diesen beiden Attributen wird dem Welt-Objekt
**          mitgeteilt, welchen Robo der User steuert
**          (YWA_UserRoboObject) und in welchem Vehikel
**          der User gerade sitzt (YWA_UserVehicleObject).
**
**          Diese beiden Informationen werden vom Welt-Objekt
**          zur korrekten Darstellung der Oberfläche benötigt.
**
**          YWA_UserRoboObject muß nur neu gesetzt werden, wenn
**          der User das Robo-Objekt wechselt (das ist ja eigentlich
**          nicht legal).
**
**          YWA_UserVechicleObject muß immer dann gesetzt werden,
**          wenn der User das Vehikel wechselt, in dem er
**          gerade sitzt.
**
**          Implementierungs-Vorschlag:
**
**          ypabact.class:
**              OM_SET: YBA_UserInput -> TRUE, 
**                      _set(World, YWA_UserVehicle, mich);
**
**
**          yparobo.class:
**              OM_SET: YBA_UserInput -> TRUE
**                      _set(World, YWA_UserRobo, mich);
**
**          Das ist zwar etwas redundant (immer wenn der User von
**          einem Fahrzeug in den Robo zurückschaltet, wird
**          unötigerweise YWA_UserRobo gesetzt), funktioniert
**          aber immer und vollkommen automatisch...
**
**      YWA_WeaponArray (G) [struct WeaponProto *]
**          Returniert einen Pointer auf das interne Array
**          der Waffen-Prototypen. Das sind nur die Waffen,
**          die ein Robo erzeugen kann, für die Fahrzeuge
**          ist das ja hardgecodet (hier werde ich im
**          GUI wohl oder übel einen Hack anwenden müssen:
**          wenn der User gerade im Robo sitzt (UserRobo == UserVehicle)
**          wird das Weapon-Array im Waffen-Menü angezeigt, ansonsten
**          ein BactClassID-spezifischer Inhalt).
**
**          siehe Methode YWM_CREATEWEAPON
**
**          -> normale Vehikel sollten ihre autonomen Waffen
**          nach wie vor direkt per YWM_OBTAINVEHICLE erzeugen,
**          nur die Robos sollten YWM_CREATEWEAPON verwenden.
**
**      YWA_BuildArray  (G) [struct BuildProto *]
**          Returniert einen Pointer auf das interne Array
**          der Gebäude-Prototypen. Funktioniert genau wie
**          YWA_WeaponArray, das Build-Array wird (wie der
**          gesamte Build-Modus) disabled, wenn der User
**          nicht im Robo sitzt.
**
**          siehe Methode YWM_CREATEBUILDING
**
**      YWA_VehicleArray (G) [struct VehicleProto *]
**          Returniert einen Pointer auf das VehicleArray.
**          Das ist ein "neues" Array, auf demselben Level wie
**          das Weapon-Array und das Build-Array. Statt dem
**          Visual-Prototype Array sollte ein Robo das VehicleArray
**          scannen. Dort sind einmal "aussagekräftigere" Daten
**          zu finden als im Vehicle-Array, andernseits liegen
**          im VehicleArray ja auch autonome Waffen rum etc.
**
**          siehe Methode YWM_CREATEVEHICLE
**
**      YWA_LevelFinished (SG) [BOOL]
**          Wird TRUE, wenn User den Level beendet hat.
**
**      YWA_LocaleHandle (G) [APTR]
**          Handle für ypaGetStr() Macro.
**
**      YWA_DspXRes (SG) [ULONG]
**      YWA_DspYRes (SG) [ULONG]
**          Display-Auflösung, mit der die ypaworld.class
**          arbeitet.
**
**      YWA_LevelInfo (G) [struct LevelInfo *]
**          Returniert Pointer auf eingebettete LevelInfo-Struktur.
**
**      YWA_NumDestFX (G) [ULONG]
**          Begrenzung für Zerstörungseffekte bei Gebäuden und
**          Vehicle.
**
**      YWA_Version (I) [UBYTE *]
**          Definiert einen Versions-String, der über ywd->Version
**          erreichbar ist. Der String muß static sein.
*/

/*-------------------------------------------------------------------
**  ClassID der ypaworld.class
*/
#define YPAWORLD_CLASSID    "ypaworld.class"

/*-------------------------------------------------------------------
**  Methoden
*/
#define YWM_BASE             (BSM_BASE+METHOD_DISTANCE)

#define YWM_MODSECTORENERGY    (YWM_BASE+1)
#define YWM_GETSECTORINFO      (YWM_BASE+2)
#define YWM_SETVIEWER          (YWM_BASE+3)
#define YWM_GETHEIGHT          (YWM_BASE+4)
#define YWM_GETVISPROTO        (YWM_BASE+5)
#define YWM_ADDCOMMANDER       (YWM_BASE+6)
#define YWM_NEWLEVEL           (YWM_BASE+7)
#define YWM_INTERSECT          (YWM_BASE+8)
#define YWM_INSPHERE           (YWM_BASE+9)
#define YWM_GETVIEWER          (YWM_BASE+10)
#define YWM_ADDREQUESTER       (YWM_BASE+11)
#define YWM_REMREQUESTER       (YWM_BASE+12)
#define YWM_UNITMENUCONTROL    (YWM_BASE+13)
#define YWM_MODEMENUCONTROL    (YWM_BASE+14)
#define YWM_OBTAINVEHICLE      (YWM_BASE+15)
#define YWM_RELEASEVEHICLE     (YWM_BASE+16)
#define YWM_ISVISIBLE          (YWM_BASE+17)
#define YWM_CREATEVEHICLE      (YWM_BASE+18)
#define YWM_CREATEWEAPON       (YWM_BASE+19)
#define YWM_CREATEBUILDING     (YWM_BASE+20)
#define YWM_INTERSECT2         (YWM_BASE+21)
#define YWM_INBACT             (YWM_BASE+22)
#define YWM_KILLLEVEL          (YWM_BASE+23)
#define YWM_MODSECTORENERGY2   (YWM_BASE+24)
#define YWM_VISOR              (YWM_BASE+25)
#define YWM_INITGAMESHELL      (YWM_BASE+26)    // AF
#define YWM_FREEGAMESHELL      (YWM_BASE+27)    // AF
#define YWM_OPENGAMESHELL      (YWM_BASE+28)    // AF
#define YWM_CLOSEGAMESHELL     (YWM_BASE+29)    // AF
#define YWM_TRIGGERGAMESHELL   (YWM_BASE+30)    // AF
#define YWM_LOGMSG             (YWM_BASE+31)
#define YWM_REALIZESHELLACTION (YWM_BASE+32)    // AF
#define YWM_CREATELEVEL        (YWM_BASE+33)
#define YWM_INITPLAYER         (YWM_BASE+34)
#define YWM_TRIGGERPLAYER      (YWM_BASE+35)
#define YWM_KILLPLAYER         (YWM_BASE+36)
#define YWM_PLAYERCONTROL      (YWM_BASE+37)
#define YWM_SETLANGUAGE        (YWM_BASE+38)
#define YWM_REFRESHGAMESHELL   (YWM_BASE+39)    // AF
#define YWM_BEAMNOTIFY         (YWM_BASE+40)
#define YWM_LOADGAME           (YWM_BASE+41)    // AF
#define YWM_SAVEGAME           (YWM_BASE+42)    // AF
#define YWM_SAVESETTINGS       (YWM_BASE+43)    // AF
#define YWM_LOADSETTINGS       (YWM_BASE+44)    // AF
#define YWM_SETGAMEINPUT       (YWM_BASE+45)    // AF
#define YWM_SETGAMEVIDEO       (YWM_BASE+46)    // AF
#define YWM_SETGAMELANGUAGE    (YWM_BASE+47)    // AF
#define YWM_GETRLDRATIO        (YWM_BASE+48)
#define YWM_NOTIFYDEADROBO     (YWM_BASE+49)
#define YWM_NETWORKLEVEL       (YWM_BASE+51)    // AF
#define YWM_FORCEFEEDBACK      (YWM_BASE+52)
#define YWM_SENDMESSAGE        (YWM_BASE+53)
#define YWM_NOTIFYWEAPONHIT    (YWM_BASE+54)
#define YWM_ADVANCEDCREATELEVEL (YWM_BASE+55)
#define YWM_NOTIFYHISTORYEVENT  (YWM_BASE+56)
#define YWM_ONLINEHELP          (YWM_BASE+57)

/*-------------------------------------------------------------------
**  Attribute
*/
#define YWA_BASE             (BSA_BASE+ATTRIB_DISTANCE)

#define YWA_MapMaxX             (YWA_BASE)     /* (IG) */
#define YWA_MapMaxY             (YWA_BASE+1)   /* (IG) */
#define YWA_MapSizeX            (YWA_BASE+2)   /* (G) */
#define YWA_MapSizeY            (YWA_BASE+3)   /* (G) */
#define YWA_SectorSizeX         (YWA_BASE+4)   /* (G) */
#define YWA_SectorSizeY         (YWA_BASE+5)   /* (G) */
#define YWA_WorldDescription    (YWA_BASE+6)   /* (I) */
#define YWA_NormVisLimit        (YWA_BASE+7)   /* (I) */
#define YWA_NormDFadeLen        (YWA_BASE+8)   /* (I) */
#define YWA_HeavenVisLimit      (YWA_BASE+9)   /* (I) */
#define YWA_HeavenDFadeLen      (YWA_BASE+10)  /* (I) */
#define YWA_HeavenHeight        (YWA_BASE+11)  /* (ISG) */
#define YWA_RenderHeaven        (YWA_BASE+12)  /* (ISG) */
#define YWA_DoEnergyCycle       (YWA_BASE+13)  /* (ISG) */
#define YWA_VisSectors          (YWA_BASE+14)  /* (ISG) */
#define YWA_VTypes              (YWA_BASE+15)  /* (G) */
#define YWA_UserRobo            (YWA_BASE+16)  /* (SG) */
#define YWA_UserVehicle         (YWA_BASE+17)  /* (SG) */
#define YWA_WeaponArray         (YWA_BASE+18)  /* (G) */
#define YWA_BuildArray          (YWA_BASE+19)  /* (G) */
#define YWA_VehicleArray        (YWA_BASE+20)  /* (G) */
#define YWA_LevelFinished       (YWA_BASE+21)  /* (SG) */
#define YWA_DspXRes             (YWA_BASE+22)  /* (SG) */
#define YWA_DspYRes             (YWA_BASE+23)  /* (SG) */
#define YWA_LocaleHandle        (YWA_BASE+24)  /* (G) */
#define YWA_GateStatus          (YWA_BASE+25)  /* (G) */
#define YWA_LevelInfo           (YWA_BASE+26)  /* (G) */
#define YWA_NumDestFX           (YWA_BASE+27)  /* (G) */
#define YWA_NetObject           (YWA_BASE+28)  /* (G) */
#define YWA_Version             (YWA_BASE+29)  /* (I) */
#define YWA_DontRender          (YWA_BASE+30)  /* (S) */

/*-------------------------------------------------------------------
**  Default-Werte für Attribute
*/
#define YWA_MapMaxX_DEF             (64)
#define YWA_MapMaxY_DEF             (64)
#define YWA_NormVisLimit_DEF        (1400)
#define YWA_NormDFadeLen_DEF        (600)
#define YWA_HeavenVisLimit_DEF      (4200)
#define YWA_HeavenDFadeLen_DEF      (1100)
#define YWA_HeavenHeight_DEF        (-550)
#define YWA_RenderHeaven_DEF        (TRUE)
#define YWA_DoEnergyCycle_DEF       (TRUE)
#define YWA_VisSectors_DEF          (5)
#define YWA_NumDestFX_DEF           (NUM_DESTFX)

/*-------------------------------------------------------------------
**  allgemeine Konstanten und Macros
*/

/*** "berechnet Sub-Koordinate" aus einer absoluten x- oder z-3D-Position ***/
#define GET_SUB(x) ((LONG)(x)+((LONG)SLURP_WIDTH/2)) / ((WORD)SLURP_WIDTH)

/*** max. Anzahls ***/
#define MAXNUM_FONTS        (92)    // max. Anzahl Fonts
#define MAXNUM_LEGOS        (256)   // max. Anzahl Bausteine
#define MAXNUM_SUBSECTS     (256)   // max. Anzahl Subsektoren
#define MAXNUM_SECTORS      (256)   // max. Anzahl Sektoren
#define MAXNUM_VISPROTOS    (512)   // max. Anzahl Visual Prototypes
#define MAXNUM_OWNERS       (8)     // max. Anzahl Mitspieler
#define MAXNUM_KRAFTWERKS   (256)   // max. Anzahl Kraftwerke
#define MAXNUM_ROBOS        (8)     // bitte nicht ändern
#define MAXNUM_JOBS         MAXNUM_ROBOS
#define MAXNUM_WUNDERSTEINS (8)     // max. Anzahl Wundersteine je Level
#define MAXNUM_SQUADS       (96)    // max. Anzahl vorplazierte Geschwader
#define MAXNUM_GATES        (8)     // max. Anzahl Level-Ausgänge
#define MAXNUM_TARGETS      (8)     // max. Anzahl Ziel-Level
#define MAXNUM_BUDDIES      (128)   // max. Anzahl mitnehmbarer Fahrzeuge
#define MAXNUM_KEYSECS      (16)    // max. Anzahl Keysectors für jedes Gate
#define MAXNUM_LEVELS       (256)
#define MAXNUM_BG           (4)     // max. Anzahl BG-Pic-Auflösungen
#define MAXNUM_INFOSLOTS    (1)     // für's MissionBriefing
#define MAXNUM_MBELEMENTS   (128)   // für's MissionBriefing
#define MAXNUM_EXPLODEFX    (NUM_DESTFX) // max. Anzahl Explode-FX (siehe bacterium.h)
#define MAXNUM_SUPERITEMS   (8)     // max. Anzahl Superitems pro Level
#define MAXNUM_DEFINEDROBOS (16)    // max. Anzahl Robos definiert in Scripts
#define MAXNUM_STARTBUDDIES (5)     // soviele Buddies dürfen am Anfang mitgenommen werden

/*** Font-IDs ***/
#define FONTID_DEFAULT      (0)     // Default-Font
#define FONTID_MAPROBO      (1)     // Icon-Font (obsolete)
#define FONTID_ICONDOWN     (2)     // Icon Pressed (obsolete)
#define FONTID_DEFBLUE      (3)
#define FONTID_DEFWHITE     (4)
#define FONTID_MENUBLUE     (5)
#define FONTID_MENUGRAY     (6)
#define FONTID_MENUWHITE    (7)
#define FONTID_GADGET       (8)
#define FONTID_MENUDOWN     (9)
#define FONTID_MAPBTNS      (10)
#define FONTID_MAPHORZ      (11)
#define FONTID_MAPVERT      (12)
#define FONTID_MAPVERT1     (13)
#define FONTID_GAUGE4       (14)
#define FONTID_TRACY        (15)    // transparenter Font, helle Bukwuis

#define FONTID_MAPCUR_4     (16)    // für Sektor-Cursors, 4x4
#define FONTID_MAPCUR_8     (17)    // ditto, 8x8
#define FONTID_MAPCUR_16    (18)    // ditto, 16x16
#define FONTID_MAPCUR_32    (19)    // ditto, 32x32
#define FONTID_MAPCUR_64    (20)    // ditto, 64x64

#define FONTID_ICON_NB      (21)    // Icon-Set, Normal, Big
#define FONTID_ICON_PB      (22)    // Icon-Set, Pressed, Big
#define FONTID_ICON_DB      (23)    // Icon-Set, Disabled, Big
#define FONTID_ICON_NS      (24)    // Icon-Set, Normal, Small
#define FONTID_ICON_PS      (25)    // Icon-Set, Pressed, Small
#define FONTID_TYPE_NB      (26)    // Type-Icons, Normal, Big
#define FONTID_TYPE_PB      (27)    // Type-Icons, Pressed, Big
#define FONTID_TYPE_NS      (28)    // Type-Icons, Normal, Small
#define FONTID_TYPE_PS      (29)    // Type-Icons, Pressed, Small

#define FONTID_ENERGY       (30)    // Energie-Window-Font
#define FONTID_LTRACY       (31)    // spezieller Lowres-Transparenz-Font

#define FONTID_LEGO16X16_0  (40)    // Font für 16x16 Lego-Map
#define FONTID_LEGO8X8_0    (41)    // Font für 8x8 Lego-Map
#define FONTID_LEGO4X4_0    (42)    // Font für 4x4 Lego-Map

#define FONTID_SEC4X4       (43)    // Sektoren, 8X8
#define FONTID_SEC8X8       (44)    // Sektoren, 4X4

#define FONTID_OWNER_4      (50)    // Owner-Font, 4x4
#define FONTID_OWNER_8      (51)    //             8x8
#define FONTID_OWNER_16     (52)    //             16x16
#define FONTID_OWNER_32     (53)    //             32x32
#define FONTID_OWNER_64     (54)    //             64x64

#define FONTID_BRIGHT_4     (55)    // Brightness-Font, 4x4
#define FONTID_BRIGHT_8     (56)    //                  8x8
#define FONTID_BRIGHT_16    (57)    //                  16x16
#define FONTID_BRIGHT_32    (58)    //                  32x32

#define FONTID_BACT3X3      (59)    // Bakterien in Map, 3x3
#define FONTID_BACT5X5      (60)    //                   5x5
#define FONTID_BACT7X7      (61)    //                   7x7
#define FONTID_BACT9X9      (62)    //                   9x9

/*** Geometrics ***/
#define SECTOR_HEIGHTSCALE  (-100.0)
#define SLURP_WIDTH         (300.0)
#define INNER_SECTOR_SIZE   (3 * SLURP_WIDTH)
#define SECTOR_SIZE         (INNER_SECTOR_SIZE+SLURP_WIDTH)

/*** Groundtypes ***/
#define NUM_GTYPES      (6)         // Anzahl Bodentypen
#define GTYPE_GRASS     (0)         // Bodentyp Grass
#define GTYPE_STONE     (1)         // Bodentyp Stein
#define GTYPE_YPPSN     (2)         // Bodentyp Yppsn ;-)
#define GTYPE_FOOBAR    (3)         // Bodentyp FooBar

/*** Sector-Aufbau-Typen ***/
#define SECTYPE_3X3     (0)     // 9 kleine Subsektoren...
#define SECTYPE_COMPACT (1)     // ein einziger großer Subsektor

/*** Energy-Stuff ***/
#define ETYPE_NORMAL    (0)     // normal, verbraucht und leitet Strom
#define ETYPE_POWER     (1)     // Kraftwerk, verbraucht und erzeugt Strom
#define ETYPE_EMPTY     (2)     // leer, verbraucht nix, leitet nix(?)

#define ENERGY_CONSUMPTION (27)        // Norm-Energie-Verbrauch milliwatt/millisec
#define ENERGY_FULLAKKU    ((1<<24)*3) // == 50.3kW
#define ENERGY_LOWLIMIT    (10000000)  // == 10kW ab hier wird Sektor inaktiv

/*** DestRange-Limit-Indexe ***/
#define DLINDEX_FULL      (0)     // immer == ENERGY_FULLAKKU
#define DLINDEX_DAMAGED   (1)     // 1. Zerstörungs-Stufe
#define DLINDEX_DESTROYED (2)     // 2. Zerstörungs-Stufe
#define DLINDEX_EMPTY     (3)     // immer 0(!)

/*** Soundsource-IDs für GUI-Sounds ***/
#define GUI_NUM_NOISES     (6)
#define GUI_NOISE_ACK      (0)     // Acknowledged (on Maus-Klick)
#define GUI_NOISE_NAK      (1)     // Not Acknowledged (on Maus-Klick)
#define GUI_NOISE_SELECT   (2)     // Spezieller Select-Sound
#define GUI_NOISE_ALERT1   (3)     // Mild Alert
#define GUI_NOISE_ALERT2   (4)     // Yellow Alert
#define GUI_NOISE_ALERT3   (5)     // Red Alert

/*** Status eines Briefing-Elements ***/
#define BRIEFSTAT_KNOWN   (0)       // alle Daten bekannt
#define BRIEFSTAT_UNKNOWN (1)       // vollständig unbekannt
#define BRIEFSTAT_HIDDEN  (2)       // getarnt, nur Position bekannt

/*** Default-RoboReloadConst (entspricht Energie in Drak) ***/
#define YPA_ROBORELOADCONST_DEF (550000)

/*-------------------------------------------------------------------
**  Mousepointer-Defs
*/
#define NUM_MOUSE_POINTERS  (11)

#define YW_MOUSE_POINTER    (0)     // Default-Pointer
#define YW_MOUSE_CANCEL     (1)     // Cancel-Pointer
#define YW_MOUSE_SELECT     (2)     // Select-Pointer
#define YW_MOUSE_ATTACK     (3)     // Attack-Pointer
#define YW_MOUSE_GOTO       (4)     // Goto-Pointer
#define YW_MOUSE_DISK       (5)     // Disk-Access (allgemein Wait)
#define YW_MOUSE_NEW        (6)     // New-Pointer
#define YW_MOUSE_ADD        (7)     // Add-Pointer
#define YW_MOUSE_CONTROL    (8)     // Control-Pointer
#define YW_MOUSE_BEAM       (9)     // Beam-Pointer
#define YW_MOUSE_BUILD      (10)    // Build-Pointer

/*-------------------------------------------------------------------
**  Low-Level-Datenstrukturen für Aufbau der Welt aus Sektoren
*/

/*** ein Sektor-Baustein... ***/
struct Lego {
    Object *lego;               // visuell, instance of base.class
    Object *colobj;             // Collision-Skeleton, instance of skeleton.class
    struct Skeleton *colsklt;   // Ptr auf embedded Skeleton von <colobj>
    struct Skeleton *altsklt;   // zuständiges Cheat-Kollissions-Skeleton
    UBYTE shield;               // prozentualer Schild, 0..100%
    UBYTE page;                 // Page-Nummer des Fonts, 0..3
    UBYTE chr;                  // Font-Char-Num des Legos
    UBYTE pad;
    UBYTE fx_vp[MAXNUM_EXPLODEFX];  // für FX-Explosion, 0 unused
    fp3d fx_pos[MAXNUM_EXPLODEFX];  // 3d-Koords für FX-Explosionen
};

/*** eine Subsektor-Definition (1x1 oder 3x3 Größe) ***/
struct SubSectorDesc {
    LONG init_energy;           // 1 oder 0
    UBYTE limit_lego[4];        // 0=FULL, 1=DAMAGED, 2=DESTROYED, 3=EMPTY
    ULONG repair_proto;         // VisProto-ID der Repair-Hülle
};

/*** eine fertige Sektor-Typ-Description ***/
struct SectorDesc {
    UBYTE SecType;              // (SECTYPE_COMPACT | SECTYPE_3x3)
    UBYTE GroundType;           // (GTYPE_GRASS | GTYPE_STONE | GTYPE_YPPSN)
    UBYTE EType;                // (ETYPE_NORMAL | ETYPE_POWER | ETYPE_EMPTY)
    UBYTE Chr;                  // Font-Char in FONTID_SEC8X8 und FONTID_SEC4X4
    struct SubSectorDesc *SSD[3][3]; // 3*3 Subsektoren-Ptr, oder 1
};

/*** ein Sektor im Cell-Array ***/
struct Cell {
    struct MinNode PathNode;    // fuer Wegfindungsalg. Node bitte
    UBYTE  PosX;                // bitte am Anfang lassen
    UBYTE  PosY;
    UBYTE  Obstacle;            // 0...gut passierbar, SPV_Forbidden...verboten
    UBYTE  PathFlags;           // siehe TankClass
    FLOAT  FPathValue;          // Wertigkeit fuer weg hierher (f-funktion)
    FLOAT  GPathValue;          // Wertigkeit fuer weg zum Ziel (g-funktion)
    struct MinList TreeList;
    struct MinNode TreeNode;
    struct Cell   *TreeUp;

    UBYTE Owner;                // aktueller Eigentümer, 0 = EMPTY
    UBYTE Type;                 // Index in Sectors-Array
    UBYTE SType;                // Kopie von SectorDesc.SecType
    UBYTE EnergyFactor;         // Stromstärke im Sektor
    UBYTE SubEnergy[3][3];      // 0..255 "Energie-Gehalt"
    UBYTE FootPrint;            // Ich-War-Hier-Flags für 8 Eigentümer
    UBYTE WType;                // Welt-Klassen-Privat
    UBYTE WIndex;               // Welt-Klassen-Privat
    struct MinList BactList;    // Liste aller Bacts im Sektor
    FLOAT Height;               // Sektor-Höhe
    FLOAT AvrgHeight;           // Durschnittshöhe für je 4 Sektoren
                                // für linke obere Ecke dieses Sektors
};

/*** Defs für Cell.WType ***/
#define WTYPE_None            (0) // ein Standard-Sektor
#define WTYPE_JobLocked       (1) // gelockt durch einen Job, Index ist Job-Index
#define WTYPE_Kraftwerk       (2) // Sektor ist Kraftwerk, Index -> KW-Array-Index
#define WTYPE_Building        (3) // irgendwas anderes gebautes, WIndex -> BProtoNum
#define WTYPE_Wunderstein     (4) // ein Wunderstein, WIndex -> gems[Index]
#define WTYPE_ClosedGate      (5) // ein geschlossenes Beam-Gate
#define WTYPE_OpenedGate      (6) // ein offenes BeamGate
#define WTYPE_DeadWunderstein (7) // ein deaktivierter Wunderstein
#define WTYPE_SuperItem       (8) // ein Superitem

/*** ein Kraftwerk im Kraftwerks-Array ***/

// Kraftwerke:
// ~~~~~~~~~~~
// Ein Kraftwerk versorgt seine Umgebung mit einer bestimmten
// Stromstärke. Diese Stromstärke ist direkt proportional zum 
// Zerstörungsgrad des Kraftwerks:
//
//      factor = (factor_backup * sec->SubEnergy[all]) / 256;
//

struct KraftWerk {
    WORD x,y;                   // Sektor-Position...
    struct Cell *sector;        // ...und direkter Sektor-Pointer
    ULONG factor_backup;        // der ursprüngliche Energie-Faktor
    ULONG factor;               // der aktuelle Energie-Faktor dieses KWs
};

/*** statische Visual-Prototype-Daten ***/
struct VisProto {
    Object *o;
    struct tform *tform;
};

/*** ein Element in der Energymap ***/
struct EMapElm {
    UBYTE owner;
    UBYTE factor;
};

/*** ein Job, für jeden Robo ist ein Job-Slot reserviert ***/
struct BuildJob {
    ULONG lock;         // TRUE, wenn gerade besetzt
    ULONG age;          // aktuelles Alter des Jobs in millisec
    ULONG duration;     // Job wird so alt werden
    LONG sec_x, sec_y;  // Sektor-Koordinaten für diesen Job
    UBYTE owner;        // auf diesen Owner setzen, wenn fertig
    UBYTE bp;           // BuildProto zum Bauen
};

/*** ein "Wunderstein", davon gibt's bis zu 8 Stück gleichzeitig ***/
struct Wunderstein {
    UWORD active;       // TRUE/FALSE
    UWORD bproto;       // Buildproto-Nummer zur "Sektor-Visualisierung"
    UWORD sec_x,sec_y;  // Sector-X/Y-Koordinate
    ULONG mb_status;    // Mission-Briefing-Status (BRIEFSTAT_#?)
    UBYTE script_name[64];  // wird ausgeführt, wenn Wunderstein gefunden
                            // may be NULL (dann action_1st_line/last_line!)
    UBYTE msg[128];         // Message an den User
    UWORD nw_vproto_num[4]; // netzwerkrelevante protos
    UWORD nw_bproto_num[4];
    UWORD action_1st_line;  // 1.Zeile Aktions-Block, falls kein ext.Script
    UWORD action_last_line; // letzte Zeile Aktions-Block
    ULONG type;             // 0, oder LOGMSG_TECH_#?
};

/*** wird beim Wunderstein-Aktivieren ausgefuellt ***/
struct Touchstone {
    LONG gem;               // welche Wunderstein-Nummer
    LONG time_stamp;        // wann passierte es
    LONG vp_num;            // VehicleProto-Nummer, die betroffen war
    LONG wp_num;            // WeaponProto-Nummer, die betroffen war
    LONG bp_num;            // Buildproto-Nummer, die betroffen war
};

/*** eine Init-Geschwader-Struktur für die Level-Initialisierung ***/
struct NLSquadDesc {
    BOOL active;            // TRUE/FALSE
    BOOL useable;           // TRUE: Robo gibt Kommandos
    ULONG owner;            // Owner halt...
    ULONG vproto;           // Vehicle-Proto
    ULONG num;              // Anzahl der Vehicle
    struct flt_triple pos;  // Position
    ULONG mb_status;    // Mission-Briefing-Status (BRIEFSTAT_#?)
};

/*** Robo Description-Struktur für Robo-Initialisierung ***/
struct NLRoboDesc {
    UWORD owner;        // dieser Owner
    UWORD vhcl_proto;   // diesen VehicleProto benutzen
    fp3d  pos;          // Position, Y ist relativ zu Oberfläche
    ULONG energy;       // Start-Energie (nur Autonome)
    UBYTE conquer;      // Conquer-Budget (nur Autonome)
    UBYTE radar;        // Aufklärungs-Budget (nur Autonome)
    UBYTE power;        // Nachschub-Budget (nur Autonome)
    UBYTE defense;      // Verteidigungs-Budget (nur Autonome)
    LONG robo_reload_const; // Reload-Konstante fuer diesen Robo (optional)

    /*** neu 8100 000C ***/
    UBYTE safety;       // Wichtigkeit des FlakBaus
    UBYTE recon;        // Wichtigkeit der Aufklärung
    UBYTE place;        // Wichtigkeit des Platzwechsels
    UBYTE robo;         // Wichtigkeit des Roboangriffs

    ULONG mb_status;    // Mission-Briefing-Status (BRIEFSTAT_#?)
    WORD  viewangle;    // 0...360° für die Roboblickrichtung

    LONG  saf_delay;    // Verzoegerung, hier koennen Anfangswerte
    LONG  pow_delay;    // deklariert werden
    LONG  rad_delay;
    LONG  cpl_delay;
    LONG  def_delay;
    LONG  con_delay;
    LONG  rec_delay;
    LONG  rob_delay;
};

/*** für Shell-Hintergrund und Briefing-Map ***/
struct BGPicture {
    UWORD w,h;
    UBYTE name[32]; // relativ zu "levels:"
};

/*** Level-Description-Struktur für Level-Initialisierung     ***/
struct LevelDesc {
    ULONG flags;            // siehe unten
    ULONG set_num;
    ULONG event_loop;       // optionale Eventloop fuer Eventcatcher
    ULONG size_x,size_y;    // X/Y-Groesse
    ULONG slow_conn;

    UBYTE sky_name[64];     // Filename für Himmels-Objekt
    UBYTE typ_map[64];      // Filename für Type-Map
    UBYTE own_map[64];      // Filename für Owner-Map
    UBYTE hgt_map[64];      // Filename für Höhen-Map
    UBYTE blg_map[64];      // Filename für Building-Map

    ULONG num_robos;
    struct NLRoboDesc robos[8];     // bis zu 8 Robos pro Level, 0 ist User-Robo
    ULONG num_squads;
    struct NLSquadDesc squad[MAXNUM_SQUADS];    // bis zu MAXNUM_SQUDS vorplaziert

    UBYTE pal_slot[8][64];  // Paletten-Slot-Filenamen

    ULONG num_brfmaps;      // Anzahl Briefingmaps
    struct BGPicture brfmap[MAXNUM_BG];

    ULONG num_debrfmaps;    // Anzahl Debriefing-Maps
    struct BGPicture debrfmap[MAXNUM_BG];
};

/*** LevelDesc.flags ***/
#define LDESCF_SETNUM_OK        (1<<0)
#define LDESCF_SKY_OK           (1<<1)
#define LDESCF_TYPMAP_OK        (1<<2)
#define LDESCF_OWNMAP_OK        (1<<3)
#define LDESCF_HGTMAP_OK        (1<<4)
#define LDESCF_BLGMAP_OK        (1<<5)
#define LDESCF_ROBOS_OK         (1<<6)  // mindestens 1 Robo vorhanden

#define LDESCF_ALL_OK           ((1<<7)-1) // AND-Maske für alle Flags

/*** universelle Script-Parser-Beschreibungs-Struktur ***/
struct ScriptParser;
struct ScriptParser {
    ULONG (*parse_func)(struct ScriptParser *);
    void  *target;      // Parse-Func-spezifisch: auszufüllende Struktur
    UBYTE *keyword;
    UBYTE *data;
    APTR  fp;           // das Filehandle des Scripts
    ULONG line;         // aktuelle Zeilen-Nummer
    ULONG status;
    ULONG store[8];     // User-Data-Zeugs
};

/*** Werte für ScriptParser.status ***/
#define PARSESTAT_READY     (0)     // momentan außerhalb eines Context
#define PARSESTAT_RUNNING   (1)     // momentan innerhalb eines Context

/*** Parse-Modes ***/
#define PARSEMODE_EXACT     (1<<0)  // stoppt bei unbekannten Kontextblöcken
#define PARSEMODE_SLOPPY    (1<<1)  // ignoriert unbekannte Kontextblöcke
#define PARSEMODE_NOINCLUDE (1<<2)  // ignoriert Includes

/*** Returnvalues für ScriptParser.parse_func() ***/
#define PARSE_ALL_OK            (0)     // alles OK
#define PARSE_ENTERED_CONTEXT   (1)     // gerade begin-end Context betreten
#define PARSE_LEFT_CONTEXT      (2)     // gerade begin-end Context verlassen
#define PARSE_UNKNOWN_KEYWORD   (3)     // unbekanntes Keyword
#define PARSE_BOGUS_DATA        (4)     // kein oder kaputtes Data-Word
#define PARSE_UNEXPECTED_EOF    (5)     // EOF innerhalb eines Context-Block
#define PARSE_EOF               (6)     // EOF außerhalb eines Kontext-Block

/*-------------------------------------------------------------------
**  <struct Slurp> hält ein Slurp-base.class-Object und einen
**  Pointer auf dessen Skeleton-Struktur.
**
**  Regel für die Filenamen der Slurp-Objects:
**
**      S00V.base
**      |||+- Richtungs-Anzeiger (H->Horizontal, V->Vertikal)
**      ||+-- Bodentyp 2
**      |+--- Bodentyp 1
**      +---- Identifier "Slurp-Object"
*/
struct Slurp {
    Object *o;
    struct Skeleton *sklt;
};

/*-------------------------------------------------------------------
**  Visier-Zeugs...
*/
struct visor_msg {
    ULONG mg_type;      // Visier-Typ für festes MG
    ULONG gun_type;     // Visier-Typ für "autonome Waffe"
    FLOAT x,y;          // Display-Pos für MG-Visier
    FLOAT gun_x,gun_y;  // Display-Pos für autonome Waffe
    struct Bacterium *target;
};

#define VISORTYPE_NONE      (0)     // kein Visier
#define VISORTYPE_MG        (1)     // "normales" MG-Visier
#define VISORTYPE_GRENADE   (2)     // für "Granaten-Kanone"
#define VISORTYPE_ROCKET    (3)     // für ungelenkte Rakete
#define VISORTYPE_MISSILE   (4)     // für gelenkte Rakete
#define VISORTYPE_BOMB      (5)     // für Bomben

/*-------------------------------------------------------------------
**  Level-Finish-Zeugs
**  ~~~~~~~~~~~~~~~~~~
**  Ist auch NACH einem YWM_KILLLEVEL noch gültig und
**  enthält Daten über den soeben beendeten Level.
*/
struct KeySector {
    ULONG sec_x,sec_y;
    struct Cell *sec;   // KANN NULL SEIN, WENN X/Y ungültig!
};

struct Gate {
    struct Cell *sec;   // Beam-Gate ist auf diesem Sektor
    ULONG sec_x,sec_y;  // dessen Sektor-Koordinaten
    ULONG closed_bp;    // Buildproto für "Closed" Zustand
    ULONG opened_bp;    // Buildproto für "Opened" Zustand
    ULONG num_targets;  // Anzahl Targets in <targets[]>
    ULONG targets[MAXNUM_TARGETS];  // bis zu 8 Target-Level, zu denen das Gate führt
    ULONG num_keysecs;              // Anzahl Keysektoren
    struct KeySector keysec[MAXNUM_KEYSECS];  //
    ULONG mb_status;    // Mission-Briefing-Status (BRIEFSTAT_#?)
};

struct SuperItem {
    ULONG type;         // siehe unten
    ULONG status;       // siehe unten
    ULONG time_diff;    // Zeit von Aktivierung bis Auslösung

    struct Cell *sec;   // Superitem ist auf diesem Sektor
    ULONG sec_x,sec_y;  // dessen Sektor-Koordinaten
    ULONG inactive_bp;  // ist nicht aktiviert
    ULONG active_bp;    // ist aktiv
    ULONG trigger_bp;   // ist beendet
    ULONG num_keysecs;  // Anzahl Keysektoren
    struct KeySector keysec[MAXNUM_KEYSECS];    // die Keysektoren
    ULONG mb_status;

    ULONG active_timestamp;     // Timestamp der Aktivierung
    ULONG trigger_timestamp;    // Timestamp des Triggerns
    ULONG activated_by;         // dieser User hat die Bombe zuletzt aktiviert

    LONG countdown;             // wird von <time_diff> runtergezählt
    LONG last_ten_sec;          // für Countdown-Piepser
    LONG last_sec;
    LONG radius;                // aktueller Radius der Ausbreitungs-Welle

    /*** Superbomb-Specifics ***/
    LONG last_radius;           // Radius des letzten Schaden-Anrichtens
};

#define SI_TYPE_NONE        (0) // dieses Superitem ist ungültig
#define SI_TYPE_BOMB        (1) // die Superbombe
#define SI_TYPE_WAVE        (2) // die Stoudson-Welle

#define SI_STATUS_INACTIVE  (0) // wartet auf Aktivierung
#define SI_STATUS_ACTIVE    (1) // wurde aktiviert, zählt runter
#define SI_STATUS_FROZEN    (2) // ist eingefroren
#define SI_STATUS_TRIGGERED (3) // wurde getriggert

struct BuddyInfo {
    LONG CommandID;    // Geschwader-Zugehörigkeits-ID
    UWORD TypeID;      // VehicleProto-Nummer
    UWORD Created;     // != 0, wenn schon erzeugt
    LONG Energy;       // letzter Energie-Gehalt dieses "Dings"
};

//
// die Welt besteht aus einem Netzwerk von Level-Nodes
//
struct LevelNode {
    ULONG status;       // siehe unten
    FLOAT x0,y0,x1,y1;  // disp-res-unabhängige Button-Definition (0.0 .. 1.0)
    UBYTE target[8];    // 0 ist Terminator, wird bei Level Finished gefüllt!
    UBYTE ldf[64];      // Filename Level-Description-File, rel. zu LEVELS:
    UBYTE title[64];    // wird in Shell als Tooltip angezeigt
    struct rast_rect r; // Blit-Koordinaten

    /*** für Netzwerk-Levelvorbereitung ***/
    UBYTE num_players;
    UBYTE races;        // für jeden owner ein Bit aufgeodert
    UBYTE size_x;       // in Sektoren
    UBYTE size_y;
    ULONG slow_conn;    // fuer langsame Verbindungen geeignet
};

// LevelNode.status
#define LNSTAT_INVALID  (0)     // nicht initialisiert
#define LNSTAT_DISABLED (1)     // noch nicht aufgeschlossen
#define LNSTAT_ENABLED  (2)     // ist aufgeschlossen
#define LNSTAT_FINISHED (3)     // wurde gewonnen
#define LNSTAT_NETWORK  (4)     // nur für netzwerk

struct WorldInfo {
    ULONG NumBG;                // Anzahl BG-Pics

    /*** die diversen Background-Map-Filenamen in verschiedenen Auflösungen ***/
    struct BGPicture BgMaps[MAXNUM_BG];
    struct BGPicture RolloverMaps[MAXNUM_BG];
    struct BGPicture FinishedMaps[MAXNUM_BG];
    struct BGPicture EnabledMaps[MAXNUM_BG];
    struct BGPicture MaskMaps[MAXNUM_BG];

    struct BGPicture TutBgMaps[MAXNUM_BG];
    struct BGPicture TutRolloverMaps[MAXNUM_BG];
    struct BGPicture TutMaskMaps[MAXNUM_BG];

    struct BGPicture MenuMaps[MAXNUM_BG];     // Hauptmenü
    struct BGPicture InputMaps[MAXNUM_BG];    // Input-Settings
    struct BGPicture SettingsMaps[MAXNUM_BG]; // Audio/Video-Settings
    struct BGPicture NetworkMaps[MAXNUM_BG];  // Multiplayer-Settings
    struct BGPicture LocaleMaps[MAXNUM_BG];   // Locale-Settings
    struct BGPicture SaveMaps[MAXNUM_BG];     // Savegame-Settings
    struct BGPicture AboutMaps[MAXNUM_BG];
    struct BGPicture HelpMaps[MAXNUM_BG];     // Online-Hilfe
    struct BGPicture BriefMaps[MAXNUM_BG];
    struct BGPicture DebriefMaps[MAXNUM_BG];

    struct LevelNode Levels[MAXNUM_LEVELS];
    Object *BgMap;
    Object *MaskMap;
    Object *RolloverMap;
    Object *FinishedMap;
    Object *EnabledMap;

    /*** Level-Select-FX ***/
    ULONG MouseOverLevel;   // zoomt ein kleines bissel ran...
};

/*** Fonts und Chars für Shell-BG-Level-Marker ***/
#define WI_FONT             (FONTID_TYPE_NB)
#define WI_DISABLED_CHAR    ('H')
#define WI_ENABLED_CHAR     ('I')
#define WI_FINISHED_CHAR    ('J')
#define WI_HILITE_CHAR      ('K')

//
// Beschreibung des gerade geladenen Levels
//
struct LevelInfo {
    UBYTE Title[64];    // Titel bzw. Name des Levels
    ULONG Status;       // siehe unten (LEVELSTAT_#?)
    ULONG Num;          // Nummer des aktuellen Levels
    ULONG Mode;         // LEVELMODE_#? (SeqPlayer etc)
    ULONG BeamGate;     // über dieses BeamGate wurde Level verlassen
    ULONG AmbienceTrack;// Hintergrundgeräusch für das Spiel
    ULONG ambience_min_delay;   // min und max-Wert der Pause
    ULONG ambience_max_delay;   // default ist 0 0
    ULONG OwnerMask;    // welche Rassen waren im Spiel
    ULONG UserMask;     // wer davon war User???

    ULONG RSysBatt;     // System-Batterie, User-Robo
    ULONG RBeamBatt;    // Beam-Batterie, User-Robo
    ULONG RBuildBatt;   // Build-Batterie, User-Robo
    ULONG RVhclBatt;    // Vehicle-Batterie, User-Robo

    ULONG MaxNumBuddies;    // wieviele Buddies darf der User mitnehmen?
    ULONG NumBuddies;       // Anzahl der mitgenommenen "Buddies"
    struct BuddyInfo Buddies[MAXNUM_BUDDIES];

    ULONG NumGates;                 // Anzahl BeamGates im Level
    struct Gate Gate[MAXNUM_GATES]; // die BeamGate-Definitionen

    ULONG NumItems;     // Anzahl Superitems im Level
    struct SuperItem Item[MAXNUM_SUPERITEMS];

    ULONG RaceTouched[MAXNUM_OWNERS];

    UBYTE Movie[256];        // falls definiert, wird Movie statt MB gespielt
    UBYTE WinMovie[256];     // wird abgespielt, wenn Level gewonnen
    UBYTE LoseMovie[256];    // wird abgespielt, wenn Level verloren
};

#define LEVELSTAT_PLAYING    (0)     // Spiel läuft
#define LEVELSTAT_FINISHED   (1)     // über Beamgate verlassen
#define LEVELSTAT_ABORTED    (2)     // Level wurde abgebrochen
#define LEVELSTAT_PAUSED     (3)     // Pausenmode aktiviert
#define LEVELSTAT_RESTART    (4)
#define LEVELSTAT_BRIEFING   (5)     // MissionBriefing läuft gerade
#define LEVELSTAT_SAVE       (6)
#define LEVELSTAT_LOAD       (7)
#define LEVELSTAT_SHELL      (8)
#define LEVELSTAT_DEBRIEFING (9)    // Debriefing läuft gerade

/*-------------------------------------------------------------------
**  YPA-Game-Pref-Struktur. Alle "Preferences-Einstellungen"
**  im Spiel selbst werden von hier gelesen und
**  nach hier zurückgeschrieben (die Struktur ist
**  eingebettet in die LID des Welt-Objects.
*/
struct YPAWinPrefs {
    struct ClickRect rect;  // Position und Ausdehnung
};

struct YPAGamePrefs {
    ULONG valid;            // TRUE: auslesen ist erlaubt
    struct YPAWinPrefs WinMap;
    struct YPAWinPrefs WinFinder;
    struct YPAWinPrefs WinLog;
    struct YPAWinPrefs WinEnergy;
    #ifdef __NETWORK__
    struct YPAWinPrefs WinMessage;
    #endif
    ULONG MapLayers;        // angeschaltete Map-Layer
    ULONG MapZoom;          // Zoom-Level der Map
    ULONG Flags;
};

#define YPA_PREFS_MENUOPEN      (1<<0)      // Mode-Menu bleibt offen
#define YPA_PREFS_MENUTEXT      (1<<1)      // Mode-Menu als Text
#define YPA_PREFS_JOYDISABLE    (1<<2)      // Joystick disabled
#define YPA_PREFS_FFDISABLE     (1<<3)      // Force-Feedback-Disable
#define YPA_PREFS_SOUNDENABLE   (1<<4)      // Sound ist enabled
#define YPA_PREFS_INDICATOR     (1<<5)      // zeichne Indikatoren in Welt
#define YPA_PREFS_SOFTMOUSE     (1<<6)      // zeichne Software-Mousepointer
#define YPA_PREFS_FILTERING     (1<<7)      // Kaffee filtern
#define YPA_PREFS_CDSOUNDENABLE (1<<8)      // CD-Sound ist enabled
#define YPA_PREFS_JOYMODEL2     (1<<9)      // alternatives Joystick-Modell

/*-------------------------------------------------------------------
**  Mission-Briefing-Parameter
*/

/*** Definition eines Mission-Briefing-Info-Slots ***/
struct MBInfoSlot {
    ULONG Type;                 // siehe unten
    ULONG Id;                   // Visproto oder Sector-Type
    LONG Ang;                   // Winkel fürs Rotieren
    ULONG StartTime;            // ... letzte Aktivierung des Slots
    struct rast_rect Rect;      // Position und Ausdehnung auf Screen
    struct flt_triple Pos;      // 3D-Position in Weltkoordinaten (nur x,z)
    UBYTE Color;                // Farbe der Linie von Blob zu Info-Slot
    UBYTE Text[31];             // Info-Slot "Unterschrift"
};

#define MBTYPE_NONE     (0)
#define MBTYPE_SECTOR   (1)
#define MBTYPE_VPROTO   (2)

#define MBSLOT_WIDTH    (0.3)   // Hoehe eines Slots in Screencoords
#define MBSLOT_HEIGHT   (0.4)   // Breite eines Slots in Screencoords

struct MBElm {
    FLOAT pos_x,pos_z;  // 3D-Welt-Koordinaten!
    UWORD type;         // wie in MBInfoSlot
    UWORD id;           // wie in MBInfoSlot
    UBYTE fnt_id;       // Font-ID
    UBYTE chr;          // Buchstabe
    UBYTE color;        // wie in MBInfoSlot
    UBYTE text[31];     // wie in MBInfoSlot
};

/*** Mission(de)briefing Wireframes ***/
#define MBNUM_WIREFRAMES (4)
#define MBWIRE_VHCLCREATE   (0)
#define MBWIRE_VHCLKILL     (1)
#define MBWIRE_WPNEXPLODE   (2)
#define MBWIRE_CONSEC       (3)

/*** Techupgrade-Beschreibung fuer Debriefing ***/
struct DBTechUpgrade {
    UWORD type;             // siehe unten
    WORD  vp_num;           // Prototype-Nummer des betroffenen Vehicles
    WORD  wp_num;
    WORD  bp_num;
};    

/*** Debriefing: maximale Anzahl Debriefing-Techupgrades ***/
#define DB_MAXNUM_TECHUPGRADES (5)

/*** Missionbriefing/Debriefing Struktur ***/
struct MissionBriefing {
    Object *MissionMap;             // die Missions-Briefing-Map
    Object *BackgroundMap;          // die Hintergrund-Map
    struct VFMBitmap *TypeBmp;      // von TypeMap
    struct LevelInfo LevelBackup;   // wird nach Briefing wiederhergestellt
    struct LevelDesc LevelDesc;     // wird durch Parsen des LDF ausgefüllt

    ULONG Status;                   // see below
    ULONG TimerStatus;              // siehe unten
    LONG ActElm;                    // im aktuellen Status
    LONG MaxElm;                    // maximale Elemente im akt. Status
    LONG StartTime;                 // ...der aktuellen Stufe
    LONG TimeStamp;                 // rel. zu InitMissionBriefing()
    LONG TextTimeStamp;             // Timestamp fuer Textrendering
    UBYTE *Title;                   // NULL oder Ptr auf String
    UBYTE *MissionText;             // optionaler Missiontext

    struct MBInfoSlot InfoSlot[MAXNUM_INFOSLOTS];  // Infoslots zum Rendern

    struct rast_blit MapBlt;        // Parameter für's Map-Blitting
    struct rast_rect MapBltStart;   // Start-Koordinaten für Map-Blitting
    struct rast_rect MapBltEnd;     // End-Koordinaten für Map-Blitting

    ULONG FillElms;                 // TRUE: Elm-Array wird aufgefüllt
    ULONG ElmStoreIndex;            // aktueller Index für ElmStore
    LONG MouseOverElm;              // Mouse Over Elm
    LONG ElmSlot;                   // Slot des Mouse Over Elements.
    struct MBElm ElmStore[MAXNUM_MBELEMENTS]; // Store für "Pünktchen"

    struct basepublish_msg bpm;

    /*** Debriefing Specifics ***/
    ULONG IsHiColor;                // TRUE, wenn HiColor aktiviert
    ULONG ZoomFromBeamGate;         // TRUE -> aus BeamGate herauszoomen
    Object *WireObject[MBNUM_WIREFRAMES];
    struct Skeleton *WireSkeleton[MBNUM_WIREFRAMES];
    Object *OwnMap;         // Kopie der ywd->OwnerMapBU
    Object *TypMap;         // Kopie der ywd->TypeMapBU
    struct VFMBitmap *OwnMapBmp;
    struct VFMBitmap *TypMapBmp;
    ULONG ActOwner;                 // aktueller Debriefing Owner
    ULONG ActFrameTimeStamp;        // TimeStamp des aktuell gescannten Frames
    struct ypa_PlayerStats GlobalStats[MAXNUM_ROBOS];   // Kopie von ywd->GlobalStats
    struct ypa_PlayerStats LocalStats[MAXNUM_ROBOS];    // für diesen Levels
    UBYTE Movie[256];               // falls Movie gespielt werden soll, hier Filename

    ULONG ActTechUpgrade;
    struct DBTechUpgrade TechUpgrades[DB_MAXNUM_TECHUPGRADES];
};

/*** MissionBriefing.TimerStatus ***/
#define TMBSTAT_PLAYING     (0)     // normales Abspielen
#define TMBSTAT_PAUSED      (1)     // angehalten
#define TMBSTAT_FFWRD       (2)     // fast forward
#define TMBSTAT_RWND        (3)     // rewind

/*** MissionBriefing.Status ***/
#define MBSTATUS_INVALID        (0)     // kein MissionBriefing aktiviert
#define MBSTATUS_STARTLEVEL     (1)     // los geht's
#define MBSTATUS_CANCEL         (2)     // Briefing abbrechen
#define MBSTATUS_LOADLEVEL      (3)     // Savegame dieses Levels laden

#define MBSTATUS_MAPLOADED      (4)     // Map ist geladen
#define MBSTATUS_MAPSCALING     (5)     // Map wird gerade skaliert
#define MBSTATUS_MAPDONE        (6)     // Map ist fertig skaliert

#define MBSTATUS_L1_START       (7)     // You Are Here
#define MBSTATUS_L1_RUNNING     (8)
#define MBSTATUS_L1_DONE        (9)

#define MBSTATUS_L2_START       (10)     // Primary Targets
#define MBSTATUS_L2_RUNNING     (11)
#define MBSTATUS_L2_DONE        (12)

#define MBSTATUS_L3_START       (13)    // Secondary Targets (Wundersteine)
#define MBSTATUS_L3_RUNNING     (14)
#define MBSTATUS_L3_DONE        (15)

#define MBSTATUS_L4_START       (16)    // Enemy Host Stations
#define MBSTATUS_L4_RUNNING     (17)
#define MBSTATUS_L4_DONE        (18)

#define MBSTATUS_L5_START       (19)    // Enemy Supply Forces
#define MBSTATUS_L5_RUNNING     (20)
#define MBSTATUS_L5_DONE        (21)

#define MBSTATUS_L6_START       (22)    // User Supply Forces
#define MBSTATUS_L6_RUNNING     (23)
#define MBSTATUS_L6_DONE        (24)

#define MBSTATUS_L7_START       (25)    // Beam Gates
#define MBSTATUS_L7_RUNNING     (26)
#define MBSTATUS_L7_DONE        (27)

#define MBSTATUS_START_MOVIE    (28)    // starte Movieplayer

#define MBSTATUS_TEXT_START     (29)    // starte Textdisplay
#define MBSTATUS_TEXT_RUNNING   (30)    // Textdisplay läuft
#define MBSTATUS_TEXT_DONE      (31)    // Textdisplay erledigt

/*** Start-Timecodes für je 1 Missionbriefing-Element ***/
#define MBT_START_LINES  (50)      // wann Linien zeichnen
#define MBT_START_TEXT   (50)      // wann Text zeichnen
#define MBT_START_3D     (50)      // wann mit 3D-Object beginnen
#define MBT_START_NEXT   (2500)    // wann ist nächstes Event dran

/*** Zeitdauer-Codes (nur wo sinnvoll) ***/
#define MBT_DUR_MAPSCALE (600)      // solange dauert Map-Skalierung
#define MBT_DUR_TEXT     (500)      // Dauer Text-"tippen"
#define MBT_DUR_ZOOM     (500)      // Dauer des 3D-Zoom
#define MBT_DUR_MISSIONTEXT (15000) // Dauer des Missionbriefing-Texts

#define MB_LINE_COLOR   (25)
#define MB_MARKER_FONT  (FONTID_TYPE_NB)
#define MB_CHAR_ROBO        (128)
#define MB_CHAR_SQUAD       (136)
#define MB_CHAR_GATE        (145)
#define MB_CHAR_GATEKEYSEC  (146)
#define MB_CHAR_GEM         (144)

/*-------------------------------------------------------------------
**  Profile Stuff
*/
#define PROF_NUM_ITEMS (8)

#define PROF_FRAME          (0)
#define PROF_FRAMETIME      (1)
#define PROF_SETFOOTPRINT   (2)
#define PROF_GUILAYOUT      (3)
#define PROF_TRIGGERLOGIC   (4)
#define PROF_RENDERING      (5)
#define PROF_NETWORK        (6)
#define PROF_POLYGONS       (7)

/*-------------------------------------------------------------------
**  Filenamen für Movies
*/
#define NUM_MOVIES      (9)

#define MOV_GAME_INTRO  (0)
#define MOV_WIN_EXTRO   (1)
#define MOV_LOSE_EXTRO  (2)
#define MOV_USER_INTRO  (3)
#define MOV_KYT_INTRO   (4)
#define MOV_TAER_INTRO  (5)
#define MOV_MYK_INTRO   (6)
#define MOV_SULG_INTRO  (7)
#define MOV_BLACK_INTRO (8)

struct ypa_MovieData {
    UBYTE Valid[NUM_MOVIES];
    UBYTE Name[NUM_MOVIES][256];
};

/*-------------------------------------------------------------------
**  Display-Auflösungs-Definitionen
*/
#define GFX_SHELL_RESOLUTION    ((640<<12)|(480))
#define GFX_GAME_DEFAULT_RES    ((640<<12)|(480))

/*-------------------------------------------------------------------
**  Local Instance Data
*/
struct ypaworld_data {

    /*** wichtige Pointer ***/
    Object *world;
    struct GameShellReq *gsr;       // auszufüllen in YWM_INITGAMESHELL

    /*** Pointer auf aktuelle trigger_msg ***/
    struct trigger_msg *TriggerMsg; // ausgefüllt in BSM_TRIGGER!

    /*** Sektor-Daten ***/
    LONG MapMaxX, MapMaxY;      // maximale Map-Größe
    LONG MapSizeX, MapSizeY;    // Map-Größe aktueller Level
    struct Cell *CellArea;      // die Sektor-Map an sich

    FLOAT SectorSizeX;          // die Größe eines Sektors
    FLOAT SectorSizeY;

    FLOAT WorldSizeX;           // Gesamt-Größe der Welt in Koords.
    FLOAT WorldSizeY;

    struct EMapElm *EnergyMap;      // 64*64-Array für Energie-Versorgungs-Status
    struct KraftWerk *KraftWerks;   // das Kraftwerk-Array
    ULONG FirstFreeKraftWerk;       // Index des ersten freien Kraftwerks...

    ULONG ActKraftWerk;         // Counter für Floodfiller

    /*** Set-Data ***/
    LONG ActSet;                // momentan geladenes Set, 0 -> keins
    Object *SetObject;          // Die Definition Vooooon FETT!

    /*** Listen und Arrays ***/
    struct MinList CmdList;     // die Liste der 'Commander-Units'
    struct MinList DeathCache;  // der Toten-Cache...

    struct VisProto      *VisProtos;
    struct Lego          *Legos;
    struct SubSectorDesc *SubSects;
    struct SectorDesc    *Sectors;
    struct VehicleProto  *VP_Array;
    struct WeaponProto   *WP_Array;
    struct BuildProto    *BP_Array;
    struct RoboExtension *EXT_Array;

    struct BuildJob BuildJobs[MAXNUM_JOBS];

    UWORD Energy2Lego[256];     // Transformations-Tabelle
    UBYTE DistTable[64*64];     // Distanz-Tabelle für Kraftwerks-Füller

    /*** Visual Stuff ***/
    struct Bacterium *Viewer;
    struct flt_triple ViewerPos;    // "wahre" Viewer-Position
    struct flt_m3x3   ViewerDir;    // "wahre" Viewer-Matrix
    Object *Heaven;
    ULONG VisSectors;
    Object *BeeBox;             // globales Bounding-Box-base.class Object
    Object *AltCollSubObject;     // alternatives Subsektoren-Würfel-Koll-Sklt
    Object *AltCollCompactObject; // alternatives Kompaktsektor-Würfel-Koll-Sklt
    struct Skeleton *AltCollSubSklt;     // dessen Skeleton
    struct Skeleton *AltCollCompactSklt; // dessen Skeleton
    Object *TracyRemap;         // Transparenz-Remap-Tabelle
    Object *ShadeRemap;         // Shading-Remap-Tabelle
    Object *GfxObject;          // Object der Gfx-Engine
    ULONG DontRender;           // wenn TRUE, nur Logik berechnen
    ULONG UseSystemTextInput;   // TRUE, wenn Standard-Requester nehmen...

    /*** Slurp-Objects-Arrays ***/
    struct Slurp HSlurp[NUM_GTYPES][NUM_GTYPES];
    struct Slurp VSlurp[NUM_GTYPES][NUM_GTYPES];

    /*** Slurp-Kollisions-Objects ***/
    struct Slurp SideSlurp;
    struct Slurp CrossSlurp;

    /*** Misc ***/
    ULONG NormVisLimit;         // Depthfading-Params für "normale" Objects
    ULONG NormDFadeLen;
    ULONG HeavenVisLimit;       // Depthfading-Params für Himmels-Objects
    ULONG HeavenDFadeLen;
    LONG HeavenHeight;          // Höhe des Himmels
    ULONG RenderHeaven;         // Himmel zeichnen, ja/nein
    ULONG DoEnergyCycle;        // Energy-Cycle ja/nein
    ULONG MasterVolume;         // der Audio-Engine
    ULONG SaveGameExists;       // wird beim Öffnen des Pausen-Req gesetzt
    ULONG RestartSaveGameExists;    // ditto
    ULONG GamePaused;           // gesetzt, wenn im GamePaused Zustand
    LONG GamePausedTimeStamp;   // GamePaused begann hier

    LONG TimeStamp;             // wird in BSM_TRIGGER hochaddiert
    LONG FrameTime;             // FrameTime des aktuellen Frames
    LONG FrameCount;            // einfacher FrameCounter
    UBYTE *Version;             // Versions-String

    /*** GUI ***/
    ULONG GUI_Ok;               // GUI ist momentan initialisiert
    struct VFMFont *Fonts[MAXNUM_FONTS];
    struct MinList ReqList;     // aktuelle Liste *aller* Requester
    UWORD DspXRes, DspYRes;     // Display-X-Res, Display-Y-Res in Pixel
    BOOL Dragging;              // TRUE: about to drag (Window-Pos)
    struct YPAReq *DragReq;     // Store für Dragging Requester
    WORD DragReqX, DragReqY;    // Drag-Mouse-Requester-Offset
    ULONG DragLock;             // TRUE, wenn IRGENDEIN Drag-Prozeß im Gange,
                                // dann wird der Mausptr nach default gezwungen
    ULONG ControlLock;          // TRUE, wenn Maus für Vhcl-Kontrolle verwendet
    ULONG Tooltip;              // aktueller Tooltip
    ULONG TooltipHotkey;        // zugehöriger Tooltip-Hotkey
    UBYTE **DefTooltips;        // String-Pointer-Array mit Default-Strings
    struct ypa_Color Colors[YPACOLOR_NUM];  // Farbdefinitionen fuers GUI

    LONG GateFullMsgTimeStamp;  // wann wurde letzte Beamgate-Full-Message gesendet?
    LONG UserInRoboTimeStamp;   // reset, wenn User sich in Robo schaltet
    LONG EnemySectorTimeStamp;  // reset, wenn Enemy Sektor entered
    LONG UserVehicleTimeStamp;  // TimeStamp des letzten YWA_UserVehicle
    ULONG UserVehicleCmdId;     // CmdId des User-Vehicles
    LONG WeaponHitTimeStamp;    // TimeStamp des letzten WeaponHit
    FLOAT WeaponHitPower;       // Power des letzten Weaponhit
    LONG PowerAttackTimeStamp;  // Powerstation under Attack Msg Timestamp

    /*** Dynamic Layout-Stuff ***/
    LONG FontH;                 // Standard-Font-Höhe
    LONG CloseW;                // Breite des Standard-Close-Gadget
    LONG PropW,PropH;           // Breite/Höhe Standard-Proportional-Gadgets
    LONG EdgeW,EdgeH;           // Breite/Höhe Kanten
    LONG VPropH;                // Font-Höhe des Vertikal-Prop-Fonts
    LONG IconBW;                // Breite, großer Icon-Font
    LONG IconBH;                // Höhe, großer Icon-Font
    LONG IconSW;                // Breite, kleiner Icon-Font
    LONG IconSH;                // Höhe, kleiner Icon-Font
    LONG UpperTabu;             // Höhe obere "Tabu-Zone" für Requester
    LONG LowerTabu;             // Höhe untere "Tabu-Zone" für Requester

    /*** GUI-Select ***/
    ULONG FrameFlags;           // siehe unten...
    ULONG Action;               // Action, die bei Click ausgelöst würde
    struct Cell *SelSector;     // falls YWFF_MouseSectorHit
    UWORD SelSecX,SelSecY;      // falls YWFF_MouseSectorHit
    fp3d SelSecIPos;            // Intersection! falls YWFF_MouseSectorHit
    fp3d SelSecMPos;            // Mittelpunkt! falls YWFF_MouseSectorHit
    fp3d SelMouseVec;           // normalisierter "3D-Mouse-Vector" (immer)
    struct Bacterium *SelBact;  // falls YWFF_MouseBactHit
    FLOAT SelDist;              // entweder Bact-Dist oder Intersection-Dist
                                // nur 3D!
    LONG ClickControlTimeStamp; // Timestamp des letzten Vehikel-Klicks
    LONG ClickControlX;         // ...dessen Screen-X-Position
    LONG ClickControlY;         // ...dessen Screen-Y-Position
    struct Bacterium *ClickControlBact; // ...dessen Bact Pointer
    ULONG WayPointMode;         // TRUE -> Waypoint-Modus an, sonst aus
    ULONG NumWayPoints;         // Anzahl bisheriger Waypoints Waypoint-Modus
    struct flt_triple FirstWayPoint;

    Object *MousePointer[NUM_MOUSE_POINTERS];
    struct VFMBitmap *MousePtrBmp[NUM_MOUSE_POINTERS];

    ULONG MouseBlanked;         // Mouse-Blanker gerade aktiv
    WORD BMouseX,BMouseY;       // Mouse-Position bei Blank

    struct trigger_logic_msg TLMsg;   // die bekommen die Robos...

    /*** Debug-Info ***/
    UWORD DebugInfo;            // TRUE: DebugInfo drucken/verstecken
    UWORD PolysAll;             // Polygone bearbeitet
    UWORD PolysDrawn;           // Polygone gezeichnet
    UWORD FramesProSec;         // Frames Pro Sekunde

    /*** Daten von und für Robo und User ***/
    ULONG UserSitsInRoboFlak;   // TRUE, wenn User in Robo-Geschütz sitzt
    ULONG UVSecOwnerBU;         // letzter SectorOwner, User-Vehicle
    Object *UserRobo;
    Object *UserVehicle;
    struct Bacterium *URBact;   // User Robo Bact
    struct Bacterium *UVBact;   // User Vehicle Bact
    struct MinList *URSlaves;   // Slave-Liste des aktuellen Robos
    LONG SectorCount[MAXNUM_ROBOS];     // für jeden Owner Anzahl Sektoren
    FLOAT RatioCache[MAXNUM_ROBOS];     // Energy-Reload-Ratio-Cache
    FLOAT RoughRatio[MAXNUM_ROBOS];     // Energy-Reload-Ratio-Cache (>1)

    struct Bacterium *CmdrRemap[512];    // Remap-Tabelle für GUI

    LONG ActCmdID;          // Command-ID des aktuellen Geschwaders
    LONG ActCmdr;           // Index! des aktuellen Commanders, -1 keiner
    LONG NumCmdrs;          // aktuelle Anzahl Untergebener des Player-Robo
    LONG NumSlaves;         // Anzahl Untergebener des ActCmdr
    ULONG LastOccupiedID;   // Bacterium.ident des letzten okkupierten Vehicles

    /*** Snapshot-, SeqDig-, Scene-Recorder-Data ***/
    ULONG snap_num;         // Nummer des aktuellen Snapshots
    ULONG seq_digger;       // TRUE-> SeqDigging on, sonst off
    ULONG seq_num;          // Sequenz-Nummer
    ULONG frame_num;        // Frame-Nummer in Sequenz

    struct YPASeqData *in_seq;
    struct YPASeqData *out_seq;

    /*** Visier-Data ***/
    struct visor_msg visor; // siehe unten

    /*** Wunderstein-Data ***/
    struct Wunderstein gem[MAXNUM_WUNDERSTEINS];    // Wundersteine im Level
    struct Touchstone  touch_stone;                 // Infos ueber zuletzt aktivieren WStein
    UBYTE gem_info[512];    // Script-Buffer für Wunderstein-Erklärung

    /*** überlebt auch ein Killevel! ***/
    struct WorldInfo *LevelNet;     // globales Level-Netzwerk
    struct LevelInfo *Level;        // momentan geladener Level, Pointer!
    struct MissionBriefing Mission;         // Mission-Briefing-Params
    struct HistoryHeader *History;  // siehe ypa/ypahistory.h
    ULONG SBombWallVProto;      // VehicleProto für Wand-Visualisierung
    ULONG SBombCenterVProto;    // VehicleProto für Zentrum-Visualisierung
    ULONG DoDebriefing;         // Debriefing anzeigen, wird in KILLLEVEL erledigt
    ULONG WasNetworkSession;    // Kopie von ywd->playing_network fuer Debriefing

    /*** Locale-Stuff ***/
    UBYTE LocaleLang[32];       // momentane Sprache (String, def = "default")
    UBYTE *LocaleStrBuf;        // hier liegen alle Strings hintereinander
    UBYTE *LocalePtr;           // nächste freie Position im Puffer
    UBYTE *LocaleEOB;           // LastChar+1 des LocaleStrBuf
    UBYTE **LocaleArray;        // statische Pointer in <LocaleStrBuf>
    APTR LocHandle;             // abstraktes LocaleHandle

    /*** Level-Designer-Data ***/
    Object *TypeMap;
    Object *OwnerMap;
    Object *BuildMap;
    Object *HeightMap;
    UBYTE *TypeMapName;
    UBYTE *OwnerMapName;
    UBYTE *BuildMapName;
    UBYTE *HeightMapName;
    Object *TypeMapBU;          // Backup-Type-Map fürs Debriefing
    Object *OwnerMapBU;         // Backup-Owner-Map fürs Debriefing

    /*** HUD Data ***/
    struct YPAHud Hud;

    /*** Preferences ***/
    struct YPAGamePrefs Prefs;
    ULONG  NumDestFX;

    /*** Level-Statistix ***/
    LONG NumSquads[MAXNUM_ROBOS];       // Anzahl Commander (ohne Flaks)
    LONG NumVehicles[MAXNUM_ROBOS];     // Anzahl Slaves (ohne Flaks)
    LONG NumFlaks[MAXNUM_ROBOS];        // Anzahl Flaks im Level
    LONG NumRobos[MAXNUM_ROBOS];        // Robos...
    LONG NumWeapons[MAXNUM_ROBOS];      // Waffen...
    LONG AllSquads, MaxSquads;
    LONG AllVehicles, MaxVehicles;
    LONG AllFlaks, MaxFlaks;
    LONG AllRobos, MaxRobos;
    LONG AllWeapons, MaxWeapons;

    /*** ForceFeedback-Stuff (Windows only) ***/
    #ifdef __WINDOWS__
    Object *InputObject;
    LONG    FFTimeStamp;    // TimeStamp des letzten FF-Updates
    LONG    FFEngineType;   // aktueller FF-Engine-Type des Vehicles
    FLOAT   FFPeriod;       // Period-Factor für relativen Speed
    FLOAT   FFMagnitude;    // Engine-Magnitude des User-Vhcls.
    void   *ActShakeFX;     // der aktuelle Shake-FX
    #endif

    /*** Netzwerk ***/
    #ifdef __NETWORK__
    Object *nwo;            // Das Objekt, wir brauchen nur eins!
    LONG    update_time;
    ULONG   playing_network;
    ULONG   netgamestartet;
    LONG    flush_time;
    LONG    r_bpersec;
    LONG    s_bpersec;
    ULONG   infooverkill;
    LONG    netstarttime;
    ULONG   interpolate;    // zum Umschalten der Modi
    char    local_address[ 4 ];                  // daten zur maschine, wenn nicht verfuegbar, ist alles 0.
    char    local_name[ STANDARD_NAMELEN ];
    char    local_addressstring[ STANDARD_NAMELEN ];    // Adresse als String aufbereitet
    ULONG   exclusivegem;                               // sollen Wundersteine im netzspiel exclusiv sein? default ja

    /*** Fuer Chat ***/
    BOOL    chat_system;    // nutze WindowsReq fuer Chat
    UBYTE   chat_owner;     // an den owner geht es (8==alle)
    UBYTE   chat_p[3];
    char    *chat_name;     // an den Spieler geht es
    #endif

    /*** Profiling ***/
    ULONG NumFrames;
    ULONG Profile[PROF_NUM_ITEMS];
    ULONG ProfMax[PROF_NUM_ITEMS];
    ULONG ProfMin[PROF_NUM_ITEMS];
    ULONG ProfAll[PROF_NUM_ITEMS];

    /*** globale Playerstats -> beim Userladen rücksetzen ***/
    struct ypa_PlayerStats GlobalStats[MAXNUM_ROBOS];
    LONG MaxRoboEnergy;     // die höchste Energie, die ein UserRobo hatte (bereits geviertelt!)
    LONG MaxReloadConst;    // die daran gekoppelte ReloadConst 

    /*** VoiceOver Stuff ***/
    struct vo_VoiceOver *VoiceOver;

    /*** aktueller Joystick-Status ***/
    ULONG DoJoystick;       // wird bei Joystick-Änderung angemacht
    FLOAT PrevJoyX;         // alter Status von JoyX
    FLOAT PrevJoyY;         // alter Status von JoyY
    FLOAT PrevJoyZ;         // alter Status von JoyZ
    FLOAT PrevJoyHatX;
    FLOAT PrevJoyHatY;

    /*** Movie Data ***/
    struct ypa_MovieData MovieData;

    /*** Help URL ***/
    UBYTE *Url;

    /*** Display Resolution Stuff ***/
    ULONG OneDisplayRes;        // TRUE, wenn 1 Display-Res fürs ganze Spiel
    ULONG ShellRes;             // DisplayID für Shell
    ULONG GameRes;              // DisplayID für Spiel

    /*** EventCatcher ***/
	struct ypa_EventCatcher *EventCatcher;
};

#define NETSTARTTIME    250000      // soviele Millisekunden warten wir auf
                                    // die Spieler...

/*-------------------------------------------------------------------
**  Das folgende Macro returniert die aktuell gültige Lego-
**  Nummer für eine SubSektor-Koordinate [sx,sy] in einem
**  Sektor <sec>
*/
#define GET_LEGONUM(ywd,sec,sx,sy) (ywd->Sectors[sec->Type].SSD[sx][sy]->limit_lego[ywd->Energy2Lego[sec->SubEnergy[sx][sy]]])

/*-------------------------------------------------------------------
**  Defs für ypaworld_data.FrameFlags:
**      (1) wird mit jedem neuen Frame gelöscht
**      (2) yw_HandleGUIInput() setzt YWFF_MouseOverGUI, YWFF_MouseOverMap
**      (3) falls YWFF_MouseOverMap, setzt yw_RenderMap() YWFF_MouseOverSector
**          und evtl. YWFF_MouseOverBact (sowie Ausfüllen aller dazu bekannten
**          Daten in <ywd>).
**      (4) yw_CheckMouseSelect() übernimmt dann die endgültige Aufbereitung
**          der potentiell selektierten Sachen (auch abhängig vom
**          eingestellten Modus im Statusbar).
*/
#define YWFF_DoSectorCheck   (1<<0)  // Modus-abhängig
#define YWFF_DoBactCheck     (1<<1)  // Modus-abhängig
#define YWFF_MouseOverGUI    (1<<2)  // Mouse ist über GUI
#define YWFF_MouseOverMap    (1<<3)  // Mouse ist speziell über Map-Interior
#define YWFF_MouseOverSector (1<<4)  // Mouse ist über einem Sektor
#define YWFF_MouseOverBact   (1<<5)  // Mouse ist über einem Bacterium
#define YWFF_MouseOverFinder (1<<6)  // Mouse ist über einem Finder-Button
#define YWFF_GimmeLeader     (1<<7)  // Modifikator bei YWFF_DoBactCheck

/*-------------------------------------------------------------------
**  Aktionen, die vom User ausgelöst werden können
*/
#define YW_ACTION_NONE      (0)
#define YW_ACTION_FIRE      (1)
#define YW_ACTION_GOTO      (2)
#define YW_ACTION_NEW       (3)
#define YW_ACTION_ADD       (4)
#define YW_ACTION_CONTROL   (5)
#define YW_ACTION_BUILD     (6)
#define YW_ACTION_MERGE     (7)
#define YW_ACTION_SELECT    (8)
#define YW_ACTION_PANIC     (9)
#define YW_ACTION_APILOT    (10)     // Robo: bitte bewegen

#define YW_ACTION_WAYPOINT_START (11)   // erster gültiger Waypoint
#define YW_ACTION_WAYPOINT_ADD   (12)   // weiterer Waypoint
#define YW_ACTION_WAYPOINT_CYCLE (13)   // bei Cycle...

#define YW_ACTION_SETSECTOR (14)    // Designer: Sektor setzen
#define YW_ACTION_SETHEIGHT (15)    // Designer: Sektor-Höhe ändern
#define YW_ACTION_SETOWNER  (16)    // Designer: Sektor-Owner ändern

#define YW_ACTION_CAMERAPOS     (17)    // Cutter: Kamera neu positionieren
#define YW_ACTION_CAMERALINK    (18)    // Cutter: Kamera an Bact linken

#define YW_ACTION_ONBOARD   (19)    // passives Mitfahren

/*-------------------------------------------------------------------
**  Msg- und Support-Strukturen
*/
struct getsectorinfo_msg {
    FLOAT abspos_x, abspos_z;   // pos_x und pos_z vorgegeben!!!
    ULONG sec_x, sec_y;         // wird ermittelt
    struct Cell *sector;        // wird ermittelt
    FLOAT relpos_x, relpos_z;   // wird ermittelt
};

struct getheight_msg {
    struct Cell *sector;    // z.B. mit YWM_GETSECTORINFO ermitteln!
    fp3d pos;               // in: X und Z, out: fehlendes Y
    LONG poly;              // out: Nummer des getroffenen Polys, -1: no hit
    struct Skeleton *sklt;  // out: Pointer auf Kollision-Skeleton
};

struct newlevel_msg {
    ULONG pos_x, pos_y;     // Position in Welt-Level-Map
};

struct createlevel_msg {
    ULONG level_num;        // lineare Nummer des Levels
    ULONG level_mode;       // LEVELMODE_#?
};
#define LEVELMODE_NORMAL    (0) // normal-Modus (garantiert 0)
#define LEVELMODE_SEQPLAYER (1) // für Sequenz-Player!

struct intersect_msg {
    fp3d pnt;               // 3D-Start-Punkt des Test-Vektors
    fp3d vec;               // Richtung und Länge des Test-Vektors
    ULONG insect;           // 1 -> Intersection, 0 -> keine Intersection
    FLOAT t;                // falls Treffer, hier der t-Value
    fp3d ipnt;              // falls Treffer, hier der genaue Schnittpunkt
    ULONG pnum;             // falls Treffer, hier die Poly-Nummer
    struct Skeleton *sklt;  // falls Treffer, hier Ptr. auf Skeleton-Struktur
    ULONG flags;            // siehe unten
};
#define INTERSECTF_CHEAT    (1<<0)  // aktiviere Intersection-Cheat

struct Insect {
    fp3d ipnt;              // Point Of Intersection
    struct Plane pln;       // Ebenen-Parameter des Treffer-Polygons
};

struct insphere_msg {
    fp3d pnt;               // In: 3D-Punkt
    fp3d dof;               // In: normalisierter Richtungs-Vektor
    FLOAT radius;           // In: Test-Radius
    struct Insect *chain;   // In: Pointer auf Buf mit leeren <struct Insect>
    ULONG max_insects;      // In: maximale Anzahl Intersections
    ULONG num_insects;      // Out: tatsächliche Anzahl Intersections
    ULONG flags;
};

struct inbact_msg {
    struct Bacterium *me;   // In: Pointer auf "Checker", wird nicht gecheckt
    fp3d pnt;               // In: 3D-Punkt
    fp3d vec;               // In: Vektor
    struct Bacterium *bhit; // getroffene Bakterie
    FLOAT dist;             // Entfernung zu dieser
};

struct energymod_msg {
    fp3d pnt;               // In: absoluter 3D-Punkt
    LONG energy;            // In: Energie-Betrag in Milli-Watt
    ULONG owner;            // In: 255 -> selbst berechnen, sonst übernehmen
    struct Bacterium *hitman;   // In: der, der die Waffe abgeballert hat
};

struct obtainvehicle_msg {
    ULONG vtype;            // Vehicle-Typ, siehe "ypa/ypavehicles.h"
    FLOAT x,y,z;            // gewünschte Anfangs-Position
};

struct createvehicle_msg {
    ULONG vp;           // VP_XXX Konstante aus "ypa/ypavehicles.h"
    FLOAT x,y,z;        // Initial-Position
};

struct createweapon_msg {
    ULONG wp;           // WP_XXX Konstante aus "ypa/ypavehicles.h"
    FLOAT x,y,z;        // Initial-Position
};

struct createbuilding_msg {
    ULONG job_id;           // [1..MAXNUM_ROBOS]
    ULONG owner;            // wird in fertigen Sektor eingetragen
    ULONG bp;               // BP_XXX Konstante aus "ypa/ypavehicles.h"
    ULONG immediate;        // TRUE -> sofort bauen
    ULONG sec_x, sec_y;     // Sektor-Position, auf der gebaut werden soll
    ULONG flags;            // siehe unten
};
#define CBF_NOBACTS (1<<0)  // nur Immediate Mode: es werden keine Bacts mitgebaut

struct mse2_msg {       // siehe YWM_MODSECTORENERGY2
    ULONG sec_x;
    ULONG sec_y;
    ULONG subsec_x;
    ULONG subsec_y;
    LONG energy;
    ULONG flags;
};

#define MSE2F_IgnoreShield  (1<<0)
#define MSE2F_Overkill      (1<<1)

struct logmsg_msg {
    struct Bacterium *bact;     // darf NULL sein
    ULONG pri;                  // Priorität, noch nicht näher def.
    UBYTE *msg;                 // die Msg itself als C-String
    ULONG code;     // LOGMSG_#? Code für spezielle Messages
};

// die LOGMSG-Codes unter 32 entsprechen ***garantiert***
// ID_LMSG_#? Codes aus ypa/ypastrategy.h!
#define LOGMSG_NOP              (0)
#define LOGMSG_DONE             (1)
#define LOGMSG_CONQUERED        (2)
#define LOGMSG_FIGHTSECTOR      (3)
#define LOGMSG_REMSECTOR        (4)
#define LOGMSG_ENEMYDESTROYED   (5)
#define LOGMSG_FOUNDROBO        (6)
#define LOGMSG_FOUNDENEMY       (7)
#define LOGMSG_LASTDIED         (8)
#define LOGMSG_ESCAPE           (9)
#define LOGMSG_RELAXED          (10)
#define LOGMSG_ROBODEAD         (11)
#define LOGMSG_USERROBODEAD     (12)
#define LOGMSG_USERROBODANGER   (13)

#define LOGMSG_CREATED          (14)
#define LOGMSG_ORDERMOVE        (15)
#define LOGMSG_ORDERATTACK      (16)
#define LOGMSG_CONTROL          (17)
#define LOGMSG_REQUESTSUPPORT   (18)
#define LOGMSG_ENEMYESCAPES     (19)

#define LOGMSG_ENEMYGROUND      (20)
#define LOGMSG_WUNDERSTEIN      (21)
#define LOGMSG_ENEMYSECTOR      (22)
#define LOGMSG_GATEOPENED       (23)
#define LOGMSG_GATECLOSED       (24)
#define LOGMSG_TECH_WEAPON      (25)
#define LOGMSG_TECH_ARMOR       (26)
#define LOGMSG_TECH_VEHICLE     (27)
#define LOGMSG_TECH_BUILDING    (28)
#define LOGMSG_TECH_LOST        (29)
#define LOGMSG_POWER_DESTROYED  (30)
#define LOGMSG_FLAK_DESTROYED   (31)
#define LOGMSG_RADAR_DESTROYED  (32)
#define LOGMSG_POWER_ATTACK     (33)
#define LOGMSG_FLAK_ATTACK      (34)
#define LOGMSG_RADAR_ATTACK     (35)
#define LOGMSG_POWER_CREATED    (36)
#define LOGMSG_FLAK_CREATED     (37)
#define LOGMSG_RADAR_CREATED    (38)

#define LOGMSG_HOST_WELCOMEBACK     (39)
#define LOGMSG_HOST_ENERGY_CRITICAL (40)
#define LOGMSG_HOST_ONLINE          (41)
#define LOGMSG_HOST_SHUTDOWN        (42)
#define LOGMSG_ANALYSIS_AVAILABLE   (43)
#define LOGMSG_ANALYSIS_WORKING     (44)
#define LOGMSG_POWER_CAPTURED       (45)

#define LOGMSG_BEAMGATE_FULL        (46)

#define LOGMSG_EVENTMSG_CREATE          (47)
#define LOGMSG_EVENTMSG_CONTROL         (48)
#define LOGMSG_EVENTMSG_DESTROYROBO     (49)
#define LOGMSG_EVENTMSG_MAP             (50)
#define LOGMSG_EVENTMSG_OPT_CONTROL     (51)
#define LOGMSG_EVENTMSG_OPT_AGGR        (52)
#define LOGMSG_EVENTMSG_DESTROYALL      (53)
#define LOGMSG_EVENTMSG_POWERSTATION    (54)
#define LOGMSG_EVENTMSG_KEYSECTORS      (55)
#define LOGMSG_EVENTMSG_BEAMGATE        (56)
#define LOGMSG_EVENTMSG_WELCOME_1       (57)
#define LOGMSG_EVENTMSG_WELCOME_2       (58)
#define LOGMSG_EVENTMSG_WELCOME_3       (59)
#define LOGMSG_EVENTMSG_COMPLETE_1      (60)
#define LOGMSG_EVENTMSG_COMPLETE_2      (61)
#define LOGMSG_EVENTMSG_COMPLETE_3      (63)
#define LOGMSG_EVENTMSG_COMMAND         (64)
#define LOGMSG_EVENTMSG_HOWTOBEAM       (65)
#define LOGMSG_EVENTMSG_READYFORWAR     (66)

#define LOGMSG_POWER_LOST               (67)
#define LOGMSG_FLAK_LOST                (68)
#define LOGMSG_RADAR_LOST               (69)

#define LOGMSG_SUPERBOMB_ACTIVATED      (70)
#define LOMGSG_SUPERBOMB_TRIGGERED      (71)
#define LOGMSG_SUPERBOMB_FROZEN         (72)
#define LOGMSG_SUPERBOMB_DEACTIVATED    (73)
#define LOGMSG_SUPERWAVE_ACTIVATED      (74)
#define LOGMSG_SUPERWAVE_TRIGGERED      (75)
#define LOGMSG_SUPERWAVE_FROZEN         (76)
#define LOGMSG_SUPERWAVE_DEACTIVATED    (77)

#define LOGMSG_TECH_RADAR               (78)
#define LOGMSG_TECH_BUILDANDVEHICLE     (79)
#define LOGMSG_TECH_GENERIC             (80)
#define LOGMSG_KEYSECTOR_LOST           (81)
#define LOGMSG_KEYSECTOR_CAPTURED       (82)

#define LOGMSG_NETWORK_USER_LEFT        (83)
#define LOGMSG_NETWORK_KYT_LEFT         (84)
#define LOGMSG_NETWORK_TAER_LEFT        (85)
#define LOGMSG_NETWORK_MYK_LEFT         (86)
#define LOGMSG_NETWORK_USER_KILLED      (87)
#define LOGMSG_NETWORK_KYT_KILLED       (88)
#define LOGMSG_NETWORK_TAER_KILLED      (89)
#define LOGMSG_NETWORK_MYK_KILLED       (90)
#define LOGMSG_NETWORK_YOUWIN           (91)

#define LOGMSG_NETWORK_PARASITE_STOPPED (92)

struct initplayer_msg {
    UBYTE *seq_filename;        // Filename eines SEQN-Files
};

struct playercontrol_msg {
    ULONG mode;                 // siehe unten
    LONG arg0;                  // abhängig von Code
};

#define PLAYER_NOP      (0)
#define PLAYER_STOP     (1)
#define PLAYER_PLAY     (2)
#define PLAYER_GOTO     (3)     // arg0 -> FrameNummer, -1 -> last frame
#define PLAYER_NEXT     (4)
#define PLAYER_PREV     (5)
#define PLAYER_RECORD   (6)
#define PLAYER_RELPOS   (7)

#define CAMERA_FREESTATIV   (16)
#define CAMERA_LINKSTATIV   (17)
#define CAMERA_ONBOARD1     (18)
#define CAMERA_ONBOARD2     (19)
#define CAMERA_PLAYER       (20)

struct setlanguage_msg {
    UBYTE *lang;            // z.B. "deutsch", "english" ... etc.
};

struct beamnotify_msg {
    struct Bacterium *b;
};

struct saveloadgame_msg {   // AF
    struct GameShellReq *gsr;
    char   *name;           // neu, muß nun ausgefüllt werden
};

struct saveloadsettings_msg {   // AF
    char *filename;     // ohne Pfad, relativ zu USER
    char *username;
    ULONG mask;
    struct GameShellReq *gsr;
    ULONG  flags;       // siehe unten
};

struct setgamevideo_msg {

    ULONG modus;
    BOOL  forcesetvideo;    // auch setzen, wenn gleicher Modus...
};

#define SLS_OPENSHELL   1

struct getrldratio_msg {
    ULONG owner;        // In: für welchen Owner?
    FLOAT ratio;        // Out: Energie-Reload-Verhältnis (0.0 .. 1.0)
    FLOAT rough_ratio;  // Out: ebenso, nicht aber auf 1 begrenzt
};

struct notifydeadrobo_msg {
    struct Bacterium *victim;       // Bact-Pointer des Opfers
    ULONG killer_id;                // Owner-ID des Killers (!= 0 !!!)
};

struct yw_forcefeedback_msg {
    ULONG type;             // siehe unten
    FLOAT power;            // 0.0 .. 1.0
    FLOAT dir_x,dir_y;      // Richtungs-Vektor
};

#define YW_FORCE_MISSILE_LAUNCH (0)     // Rakete
#define YW_FORCE_GRENADE_LAUNCH (1)     // Granate
#define YW_FORCE_BOMB_LAUNCH    (2)     // Bombe ausgeklinkt
#define YW_FORCE_MG_ON          (3)     // MG anschalten
#define YW_FORCE_MG_OFF         (4)     // MG ausschalten
#define YW_FORCE_COLLISSION     (5)     // Kollission aufgetreten
#define YW_FORCE_WEAPONHIT      (6)     // direkter Waffen-Treffer

struct yw_notifyweaponhit_msg {
    FLOAT power;
};

struct yw_onlinehelp_msg {
    UBYTE *url;         // String, welcher die zu launchende URL beschreibt
};
/*-----------------------------------------------------------------*/
#endif

