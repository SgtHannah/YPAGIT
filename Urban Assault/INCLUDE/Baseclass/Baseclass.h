#ifndef BASECLASS_BASECLASS_H
#define BASECLASS_BASECLASS_H
/*
**  $Source: PRG:VFM/Include/baseclass/baseclass.h,v $
**  $Revision: 38.26 $
**  $Date: 1998/01/06 12:52:00 $
**  $Locker:  $
**  $Author: floh $
**
**  Die base.class ist die neue Rootclass für ein hochflexibles
**  Weltmodell :-)
**
**  (C) Copyright 1994,1995,1996 by A.Weissflog
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

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef ADE_ADE_CLASS_H
#include "ade/ade_class.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      base.class  -- Rootclass für 3d-Objects
**
**  FUNCTION
**      Ein base.class Object besteht hauptsächlich aus einem
**      Skeleton-Object, einer Liste von ADE-Objects und
**      einer TForm als Lage-Definition im 3D-Raum.
**
**      base.class Objects können selbst zu einem nach unten
**      gerichteten Baum verkoppelt werden.
**
**      Bitte beachten, daß einige Features hauptsächlich aus
**      Kompatiblitäts-Gründen mit früheren Versionen existieren.
**      Die base.class wird eben langsam alt und wird irgendwann
**      mal ersetzt.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**
**      base.class
**      ~~~~~~~~~~
**      BSM_TRIGGER
**      ===========
**          Msg:    struct trigger_msg
**          Ret:     ---
**
**          Die BSM_TRIGGER Methode wird einmal pro Frame auf das
**          Welt-Object (also das oberste Object im "Weltenbaum")
**          angewendet. Mehr muß die Applikation nicht machen, damit
**          eine Welt "funktioniert" :-)
**
**          Um die Welt von der Framerate abzukoppeln, wird in der
**          >trigger_msg< die aktuelle Absolut-Zeit seit Beginn
**          der Existenz der Welt und die Zeit-Differenz zum
**          letzten Frame in >Ticks< mitgegeben. Sämtliche 
**          kontinuierliche Prozesse werden mit diesen Werten 
**          synchronisiert.
**
**          Man beachte, daß BSM_TRIGGER nur auf das aktuelle Weltobject
**          wirkt. Falls diese Methode modifiziert werden soll, hat
**          es also nur Sinn, hier "globale Sachen" unterzubringen, die
**          das Welt-Object zu erledigen hat. BSM_TRIGGER selbst wird
**          NICHT an die Child-Objects des Welt-Objects hinuntergereicht,
**          sondern innerhalb BSM_TRIGGER wendet das Object die internen 
**          Methoden BSM_MOTION, BSM_DOCOLLISION und BSM_PUBLISH AUF SICH
**          SELBST AN, erst diese sorgen selbst dafür, daß "der Rest der
**          Welt davon erfährt". Damit lassen sich die einzelnen
**          spezialisierten Prozesse besser optimieren, weil sich nicht
**          alles auf die gesamte Welt auswirken muß.
**
**
**      BSM_MOTION  [*** INTERN *** INTERN *** INTERN ***]
**      ==========
**          Msg:    struct trigger_msg
**          Ret:    ---
**
**          Das "Default-Verhalten" von BSM_MOTION sieht so aus:
**              0) Falls das Object ein Input-Empfänger ist 
**                 (BSA_HandleInput = TRUE), wendet das Object die
**                 Methode BSM_HANDLEINPUT auf sich selbst an (siehe
**                 dort).
**              1) Falls lineare Bewegung eingeschaltet ist, wird der
**                 lineare Bewegungsvektor [vx,vy,vz] auf die lokale
**                 Position [x,y,z] aufaddiert (synchronisiert mit
**                 aktuellen FrameTime) [siehe BSM_VECTOR].
**              2) Falls Rotation eingeschaltet ist, wird [rx,ry,rz]
**                 auf [ax,ay,az] addiert, auch synchronisiert mit der
**                 Frametime [siehe BSM_ROTATION].
**              3) BSM_MOTION wird auf alle Children verteilt. Damit
**                 überflutet BSM_MOTION rekursiv die gesamte Welt.
**                 Genau aus diesem Grund sollte BSM_MOTION auch
**                 relativ effizient gehalten werden!
**
**      BSM_HANDLEINPUT [*** INTERN *** INTERN *** INTERN *** ]
**      ===============
**          Msg:    struct trigger_msg
**          Ret:    ---
**          WIRD AUSSCHLIESSLICH VON BSM_MOTION AUFGERUFEN!
**
**          Diese Methode wertet die in der trigger_msg via
**          trigger_msg->input übergebene VFMInput-Struktur aus
**          und modifiziert abhängig davon die eigenen Object-
**          Attribute.
**          Es wird NICHT gecheckt, ob BSF_HandleInput gesetzt ist,
**          wenn BSM_HANDLEINPUT auf ein Object angewendet wird, kann
**          es sicher sein, daß es auch was mit den Input-Informationen
**          anfangen soll.
**
**      BSM_DOCOLLISION     [*** OBSOLETE *** OBSOLETE *** OBSOLETE *** ]
**      BSM_CHECKCOLLISION  [*** OBSOLETE *** OBSOLETE *** OBSOLETE *** ]
**      BSM_HANDLECOLLISION [*** OBSOLETE *** OBSOLETE *** OBSOLETE *** ]
**
**      BSM_PUBLISH
**      ===========
**          Msg:    struct basepublish_msg
**          Ret:    BOOL
**
**          Sichtbarkeits-Untersuchung, Koordinaten-Transformation,
**          ADEs publishen, falls sichtbar, BSM_PUBLISH rekursiv auf
**          Childs anwenden. Returniert TRUE, falls sichtbar, FALSE,
**          falls nicht sichtbar.
**
**      BSM_ADDCHILD
**      ============
**          Msg:    struct addchild_msg
**          Ret:    ---
**      BSM_NEWMOTHER   [ *** INTERN *** INTERN *** INTERN *** ]
**      =============
**          Msg:    struct newmother_msg
**          Ret:    ---
**
**          Das gesamte Mother-Child-Handling wird über diese beiden
**          Methoden abgewickelt:
**
**              _method(mother, BSM_ADDCHILD, child);
**
**          Die neue <mother> wird dann auf das <child> die Methode
**          BSM_NEWMOTHER anwenden, in der Msg wird der <Object *> der
**          Mother und diverse interne Pointer übergeben:
**
**              _method(child, BSM_NEWMOTHER, mother,
**                                            &(mother->TForm),
**                                            &(mother->ChildList));
**
**          Das Child setzt sein <child->Mother> auf <mother> und macht
**          ein AddTail() in die Child-List der Mother.
**          Falls die Mother ein "darstellbares" Object ist, also
**          ein gültiges <base_data.Skeleton> hat, sollte sie
**          in <newmother_msg.mother_tform> einen Pointer auf die
**          eigene eingebettete TForm-Struktur mitgeben. Das neue
**          Child kann dann diesen Pointer als <mother> in seine
**          eigene TForm eintragen, womit eine "echte" geometrische
**          Mother-Child-Hierarchie realisiert werden kann.
**
**          Wenn ein Child direkt seine Mother wechseln soll, reicht ein
**
**              _method(new_mother, BSM_ADDCHILD, child);
**
**          weil BSM_NEWMOTHER zuerst nachguckt, ob das Object noch in
**          einer anderen Mother-List hängt, und sich daraus zuerst
**          removed. Wenn das <Child> feststellt, daß es das aktuelle
**          MainChild in seiner Hierarchie ist, unternimmt es außerdem
**          folgende Schritte:
**
**              _method(old_mother, BSM_MAIN, -1L);   // abkoppeln
**              _method(new_mother, BSM_MAIN, my_object_pointer);
**
**          Damit bleibt der Main-Zweig automatisch auf dem neuesten Stand.
**
**
**      BSM_MAIN    [ *** INTERN *** INTERN *** INTERN *** ]
**      ========
**          Msg:    struct main_msg 
**          Ret:    ---
**
**          Diese Methode übernimmt das gesamte interne Main-Child-Handling.
**          Von außen benutze man bitte die Methode BSM_SETMAINOBJECT,
**          um das Main-Object zu bestimmen (diese muß auf das Root-
**          Object einer Hierarchie angewendet werden). Dieses stellt
**          dann (unter anderem durch Verwendung von BSM_MAIN) sicher,
**          daß das vorherige Main-Object korrekt abgemeldet wird.
**
**          Folgende "Spezial-Werte" sind für <main_msg.main_child> definiert:
**
**              0L  -> Object, auf das BSM_MAIN angewendet wurde,
**                     ist das MainObject. Dieses Object wird an seine
**                     Mother die Methode BSM_MAIN mit einem Pointer
**                     auf sich selbst schicken, damit wird
**                     der Main-Zweig bis hinauf zum Welt-Object aufgebaut.
**              Object * -> internal use only (see above)...
**              -1L -> wenn ein MainChild seine Mother verläßt (siehe
**                     BSM_ADDCHILD), schickt es ein 
**                     DoMethod(mother, BSM_MAIN, -1L) an seine Mother.
**                     Diese löscht ihren MainChild-Pointer und schickt
**                     die Methode wiederum an ihre Mother. Damit wandert
**                     der gesamte Main-Zweig automatisch mit dem
**                     MainObject mit.
**
**          Sobald ein Object die Methode BSM_MAIN bekommt, setzt es
**          bei einem <msg.main_child != -1> das interne BSF_MainChild-Flag.
**          Andernfalls wird das Flag gelöscht. Nur zur Information ;-)
**
**      BSM_SETMAINOBJECT
**      =================
**          Msg:    setmainobject_msg
**                  (was einfach ein Object * ist)
**          Ret:    Object *
**                  Pointer auf vorheriges Main-Object (oder NULL)
**
**          Dies ist die "öffentliche" Methode zur Bestimmung des
**          Main-Objects (das ist das Object, aus dem der Beobachter
**          rausguckt). Zur Bestimmung des Main-Objects wendet man
**          einfach folgende Methode auf das ROOT(!)-Object an:
**
**              Object *old = _method(root,BSM_SETMAINOBJECT,main_object)
**
**          DAS NEUE MAIN-OBJECT MUSS CHILD ODER SUB..CHILD DES
**          ROOT-OBJECTS SEIN!
**
**          BSM_SETMAINOBJECT stellt sicher, daß das vorherige Main-Object
**          korrekt abgemeldet wird, so daß der Main-Zweig intakt bleibt.
**
**          Ein DoMethod(root,BSM_SETMAINOBJECT,NULL) ist komplett ok,
**          in diesem Fall ist KEIN Object das MainObject. Wenn man
**          nur mal schnell wissen will, was das aktuelle MainObject ist,
**          ist folgendes Fragment legal (wenn auch nicht sehr elegant):
**
**          Object *main_o = DoMethod(root,BSM_SETMAINOBJECT,NULL);
**          if (main_o) DoMethod(root,BSM_SETMAINOBJECT,main_o);
**
**      BSM_POSITION
**      ============
**          Msg:    flt_vector_msg
**          Ret:    ---
**
**          Setzt die Position [x,y,z] des Objects. In der <mask>
**          der flt_vector_msg wird angegeben, welche Koordinaten
**          ignoriert und welche gesetzt werden sollen:
**
**          DoMethod(o,BSM_POSITION,(VEC_X|VEC_Y),100.0,200.0,0.0);
**
**          Bitte beachten, daß die numerischen Werte als FLOAT's
**          erwartet werden!
**
**          BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**
**      BSM_VECTOR
**      ==========
**          Msg:    flt_vector_msg
**          Ret:    ---
**
**          Setzt den Bewegungs-Vektor [vx,vy,vz] des Objects. In der
**          <mask> der vector_msg wird angegeben, welche Raum-Dimensionen
**          ignoriert bzw. modifiziert werden sollen:
**
**          DoMethod(o,BSM_VECTOR,(VEC_X|VEC_Z),0.46,0.0,1.23);
**
**          Bitte beachten, daß die numerischen Werte als FLOAT's
**          erwartet werden!
**          Falls das <mask>-Bit VEC_FORCE_ZERO gesetzt ist, wird die
**          Bewegung vollständig ausgeschaltet, es findet dann auch
**          keine aktive Kollisions-Kontrolle mehr statt
**          (via BSF_MoveMe-Flag).
**
**          BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**
**      BSM_ANGLE
**      =========
**          Msg:    lng_vector_msg
**          Ret:    ---
**
**          Setzt die Achswinkel [ax,ay,az] des Objects. In der
**          <mask> der vector_msg wird angegeben, welche Achswinkel
**          gesetzt bzw. ignoriert werden sollen:
**
**          DoMethod(o,BSM_ANGLE,(VEC_Y|VEC_Z),0,123,90);
**
**          Bitte beachten, daß die numerischen Werte als LONG
**          erwartet werden und zwischen 0 und 359 liegen müssen!
**
**          BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**
**      BSM_ROTATION
**      ============
**          Msg:    lng_vector_msg
**          Ret:    ---
**
**          Setzt die neue Achsrotation [rx,ry,rz] des Objects. In
**          der <mask> der vector_msg wird angegeben, welche
**          Rotations-Achsen ignoriert bzw. gesetzt werden sollen:
**
**          DoMethod(o,BSM_ROTATION,(VEC_X),12,0,0);
**
**          Die Zahlenwerte geben die Rotation in Grad/Sekunde(!!!)
**          an (und nicht in Ticks!).
**          Bitte beachten, daß die numerischen Werte als LONG
**          erwartet werden und zwischen  0 und 359 liegen
**          müssen!
**          Falls das <mask>-Bit VEC_FORCE_ZERO gesetzt ist, wird die
**          Rotation vollständig ausgeschaltet, es findet dann auch
**          keine aktive Kollisions-Kontrolle mehr statt 
**          (via BSF_RotateMe-Flag).
**
**          BITTE UNBEDINGT DEN TEXT ZUR <struct vector_msg> DURCHLESEN!
**
**      BSM_SCALE
**      =========
**          Msg:    flt_vector_msg
**          Ret:    ---
**
**          Setzt die Raum-Skalierer [sx,sy,sz] des Objects. In
**          der <mask> der vector_msg wird angegeben, welche
**          Raumdimensionen ignoriert werden sollen.
**          Falls VEC_RELATIVE gesetzt ist, bitte beachten,
**          das die relative Modifikation multiplizierend wirkt!
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      base.class
**      ~~~~~~~~~~
**      BSA_Skeleton (ISG)  <Object *> [instance of skeleton.class]
**          Definiert das für dieses base.class-Object gültige 
**          skeleton.class-Object.
**
**      BSA_ADE (IS) <Object *> [instance of ade.class]
**          Hänge ein ade.class-Object an die interne ADE-Liste.
**          Alle angehängten ADEs gehen in den Besitz des
**          base.class-Objects über und werden bei Bedarf
**          disposed.
**
**      BSA_FollowMother (ISG) <BOOL> [def = TRUE]
**          Definiert, ob das Object seiner Mother folgen soll,
**          sich also in deren Koordinaten-System einbetten soll.
**          Dieses Attribut beeinflußt das TForm-Flag <TFF_FollowMother>
**          in der eingebetteten TForm-Struktur direkt. Ob das Objekt
**          tatsächlich seiner Mother folgen kann, hängt außerdem
**          davon ab, ob seine Mother eine "geometrische Existenz"
**          ist, also mindestens über ein Skeleton-Object verfügt.
**          (siehe BSM_ADDCHILD und BSM_NEWMOTHER).
**
**      BSA_CollisionMode (ISG) <const> [def = COLLISION_NONE]
**          [*** OBSOLETE *** OBSOLETE *** OBSOLETE ***]
**
**      BSA_X,  BSA_Y,  BSA_Z   (G)   <FLOAT>
**      BSA_VX, BSA_VY, BSA_VZ  (G)   <FLOAT>
**      BSA_SX, BSA_SY, BSA_SZ  (G)   <FLOAT>
**          Diese Attribute sind nur GETTABLE!
**          Zum Setzen bitte BSM_POSITION, BSM_VECTOR
**          und BSM_SCALE benutzen!
**
**      BSA_AX, BSA_AY, BSA_AZ  (G)   <LONG>
**      BSA_RX, BSA_RY, BSA_RZ  (G)   <LONG>
**          Diese Attribute sind nur GETTABLE!
**          Setzen von Achswinkel und Rotation bitte
**          BSM_ANGLE und BSM_ROTATION benutzen!
**
**      BSA_VisLimit    (ISG) <ULONG> [0..32767] def=4096
**          Beschreibt die "Sichbarkeitsgrenze" für dieses Object.
**          Sobald ein base.class Object mit "geometrischer Existenz"
**          weiter als BSA_VisLimit vom Beobachter weg ist, wird es
**          ignoriert. Dient außerdem zum Berechnen der DepthFading-
**          Tiefe (für ADEs mit eingeschaltetem Depthfading). Der
**          berechnete <dfade_shifter> wird beim ADEM_PUBLISH mit
**          der <publish_msg> mitgeschickt (siehe "ade/ade_class.h").
**
**      BSA_DFadeLength (ISG) <ULONG> [0..32768] def=300
**          "Länge" des Depthfadings, definiert zusammen mit
**          BSA_VisLimit das genaue DFade-Verhalten.
**
**      BSA_AmbientLight    (ISG)   <ULONG> (ColorValue)
**          [NICHT IMPLEMENTIERT]
**
**      BSA_PublishAll  (ISG)   <BOOL> def=TRUE
**          [*** OBSOLETE *** OBSOLETE *** OBSOLETE ***]
**
**      BSA_TerminateCollision  (ISG)   <BOOL> def=FALSE
**          [*** OBSOLETE *** OBSOLETE *** OBSOLETE ***]
**
**      BSA_HandleInput (ISG)   <BOOL> def=FALSE
**          Wenn dieses Attribut TRUE ist, wird das Object die beim
**          BSM_TRIGGER übergebenen Input-Informationen auswerten und
**          sich danach verhalten.
**          ACHTUNG: Es ist durchaus möglich, mehrere Objects gleichzeitig
**          den Input auswerten zu lassen. Inwieweit das sinnvoll ist,
**          kommt auf die Applikation an. Wenn es NICHT erwünscht ist,
**          dann muß der aktuelle Input-Auswerter erst ein
**          OM_SET(BSA_HandleInput,FALSE) bekommen, ehe ein neuer
**          Input-Auswerter aktiviert wird!
**
**      BSA_ADEList     (G)     <struct List *>
**      BSA_TForm       (G)     <tform *>
**      BSA_ChildList   (G)     <struct List *>
**      BSA_ChildNode   (G)     <struct ObjectNode *>
**      BSA_ColMsg [*** OBSOLETE ***]
**      BSA_PubMsg      (G)     <struct publish_msg *>
**
**          Diese Attribute returnieren Pointer auf ausgewählte
**          interne Strukturen in den Eingeweiden des Objects.
**          Hüte Dich davor, oh naiver Zauberlehrling, diese
**          Strukturen direkt zu manipulieren! Selbst Read-Access
**          ist schon verdächtig! Die Tags sind hauptsächlich zur
**          internen Benutzung für die Inter-Class-Level-Kommunikation
**          gedacht (you're not expected to know this).
**
**      BSA_MainChild   (G)     <BOOL>
**          Dieses Attribute ist nur gettable und wird TRUE,
**          wenn sich das Object innerhalb des Main-Zweigs
**          befindet (BSF_MainChild ist gesetzt).
**
**      BSA_MainObject  (G)     <BOOL>
**          Dieses Attribut ist nur gettable und wird TRUE,
**          wenn das Object das Main-Object ist
**          (BSF_MainObject ist gesetzt).
**
**      BSA_PubStack   (G) <struct pubstack_entry *>
**      BSA_ArgStack   (G) <UBYTE *>
**      BSA_EOArgStack    (G) <UBYTE *>
**          Returniert Pointer auf die Klassen-globalen internen
**          Arg- und Publish-Stacks BSA_EOArgStack ist End Of ArgStack.
**          (Pointer darf nicht dereferenziert werden!).
**          NUR ZUR BENUTZUNG DURCH SUBKLASSEN DER base.class VORGESEHEN!!!
**
**      BSA_Static      (ISG) <BOOL>
**          Zeigt mit TRUE an, daß das Objekt bei der Local-2-Global
**          Transformation nicht *rotiert* werden muß (Translation
**          ist ok). Das ist z.B. bei vielen "Landschaftselementen" :-/
**          der Fall. Die Sache spart 9 Multiplikationen pro 3D-
**          Punkt. ACHTUNG: das Attribut ist flüchtig, geht also
**          bei einem _save()/_load() verloren.
**
**      BSA_EmbedRsrc   (ISG) <BOOL>
**          Wenn TRUE, erzeugt das Objekt bei einem OM_SAVETOIFF
**          einen zusätzlichen Chunk für ein eingebettetes
**          embed.class Object, das on the fly erzeugt wird.
**          Dieses Objekt enthält alle Shared Resources, die
**          zur Zeit im Speicher liegen.
**          Das Attribut ist nicht persistent.
*/

/*-------------------------------------------------------------------
**  ClassID der base.class
*/
#define BASE_CLASSID    "base.class"

/*-------------------------------------------------------------------
**  Methoden
*/
#define BSM_BASE            (OM_BASE+METHOD_DISTANCE)

#define BSM_TRIGGER         (BSM_BASE)

#define BSM_ADDCHILD        (BSM_BASE+1)
#define BSM_NEWMOTHER       (BSM_BASE+2)
#define BSM_MAIN            (BSM_BASE+3)

#define BSM_POSITION        (BSM_BASE+4)
#define BSM_VECTOR          (BSM_BASE+5)
#define BSM_ANGLE           (BSM_BASE+6)
#define BSM_ROTATION        (BSM_BASE+7)
#define BSM_SCALE           (BSM_BASE+8)

#define BSM_MOTION          (BSM_BASE+9)
#define BSM_DOCOLLISION     (BSM_BASE+10)       // OBSOLETE
#define BSM_CHECKCOLLISION  (BSM_BASE+11)       // OBSOLETE
#define BSM_HANDLECOLLISION (BSM_BASE+12)       // OBSOLETE
#define BSM_PUBLISH         (BSM_BASE+13)

#define BSM_HANDLEINPUT     (BSM_BASE+14)

#define BSM_SETMAINOBJECT   (BSM_BASE+15)

/*-------------------------------------------------------------------
**  Attribute
*/
#define BSA_BASE            (OMA_BASE+ATTRIB_DISTANCE)

#define BSA_Skeleton        (BSA_BASE)          // (ISG)
#define BSA_ADE             (BSA_BASE+1)        // (IS)
#define BSA_FollowMother    (BSA_BASE+2)        // (ISG)
#define BSA_CollisionMode   (BSA_BASE+3)        // OBSOLETE
#define BSA_VisLimit        (BSA_BASE+4)        // (ISG)
#define BSA_AmbientLight    (BSA_BASE+5)        // (ISG)
#define BSA_PublishAll      (BSA_BASE+6)        // (ISG)
#define BSA_TerminateCollision  (BSA_BASE+7)    // OBSOLETE
#define BSA_HandleInput     (BSA_BASE+8)        // (ISG)
#define BSA_X               (BSA_BASE+9)        // (G)
#define BSA_Y               (BSA_BASE+10)       // (G)
#define BSA_Z               (BSA_BASE+11)       // (G)
#define BSA_VX              (BSA_BASE+12)       // (G)
#define BSA_VY              (BSA_BASE+13)       // (G)
#define BSA_VZ              (BSA_BASE+14)       // (G)
#define BSA_AX              (BSA_BASE+15)       // (G)
#define BSA_AY              (BSA_BASE+16)       // (G)
#define BSA_AZ              (BSA_BASE+17)       // (G)
#define BSA_RX              (BSA_BASE+18)       // (G)
#define BSA_RY              (BSA_BASE+19)       // (G)
#define BSA_RZ              (BSA_BASE+20)       // (G)
#define BSA_SX              (BSA_BASE+21)       // (G)
#define BSA_SY              (BSA_BASE+22)       // (G)
#define BSA_SZ              (BSA_BASE+23)       // (G)
#define BSA_ADEList         (BSA_BASE+24)       // (G)
#define BSA_TForm           (BSA_BASE+25)       // (G)
#define BSA_ChildList       (BSA_BASE+26)       // (G)
#define BSA_ChildNode       (BSA_BASE+27)       // (G)
#define BSA_ColMsg          (BSA_BASE+28)       // OBSOLETE
#define BSA_PubMsg          (BSA_BASE+29)       // (G)
#define BSA_MainChild       (BSA_BASE+30)       // (G)
#define BSA_MainObject      (BSA_BASE+31)       // (G)
#define BSA_PubStack        (BSA_BASE+32)       // (G)
#define BSA_ArgStack        (BSA_BASE+33)       // (G)
#define BSA_EOArgStack      (BSA_BASE+34)       // (G)
#define BSA_DFadeLength     (BSA_BASE+35)       // (ISG)
#define BSA_Static          (BSA_BASE+36)       // (ISG)
#define BSA_EmbedRsrc       (BSA_BASE+37)       // (ISG)

/*-------------------------------------------------------------------
**  Default-Werte für (I)-Attribute
*/
#define BSA_FollowMother_DEF    (TRUE)
#define BSA_CollisionMode_DEF   (COLLISION_NONE)    /*** OBSOLETE ***/
#define BSA_VisLimit_DEF        (4096)
#define BSA_AmbientLight_DEF    (255)
#define BSA_PublishAll_DEF      (TRUE)
#define BSA_TerminateCollision_DEF  (FALSE)         /*** OBSOLETE ***/
#define BSA_HandleInput_DEF     (FALSE)

#define BSA_DFadeLength_DEF     (600)

/*-------------------------------------------------------------------
**  Definitionen für collision_msg.precision
**  [*** OBSOLETE *** OBSOLETE *** OBSOLETE ***]
*/
#define COLLISION_NONE      0   /* ignoriere Kollisionskontrolle */
#define COLLISION_PROXIMITY 1   /* einfacher Entfernungs-Check */
#define COLLISION_BBOX      2   /* Bounding-Box-Check */
#define COLLISION_DEEP      3   /* genauer Test (z.B. mit Sensor-Pool) */

/*-------------------------------------------------------------------
**  Locale Instance Data der base.class
*/
struct base_data {
    /*-- allgemeines Zeug --*/
    ULONG Id;                   /* eindeutige Object-ID */
    ULONG Flags;                /* 32 Bits für Object-Flags */
    ULONG TimeStamp;            /* Synch für 'multiples Publishing' */

    /*-- visuell --*/
    Object *Skeleton;           /* darf NULL sein */

    struct MinList ADElist;     /* nur gültig, wenn Skeleton existiert */

    /*-- Koordinaten-System-Definition --*/
    tform TForm;                /* Lagedefinition im Mother-System      */
                                /* nur gültig, wenn Skeleton existiert! */

    /*-- Welt-Struktur --*/
    Object *Mother;             /* NULL, wenn Welt-Object */
    Object *MainChild;          /* Pointer auf MainChild, may be zero!    */
                                /* das MainChild MUSS sich in der eigenen */
                                /* ChildList befinden! */
    Object *MainObject;         /* Pointer aufs eigentliche MainObject. */
                                /* NICHT UNBEDINGT IDENTISCH MIT MAIN-CHILD! */
    struct MinList ChildList;   /* die Kids dieses Objects */
    struct ObjectNode ChildNode;    /* mit dieser Node in ChildList der */
                                    /* Mother verkettet */

    /*-- ADE-Handling --*/
    ULONG VisLimit;
    struct publish_msg PubMsg;      /* eingebettete ADEM_PUBLISH-Msg */

    /*-- temporäre Daten --*/
    Object *EmbedObj;           /* temporäres embed.class Object */
};

/*-------------------------------------------------------------------
**  Flagbits für base_data.flags
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**  BSF_MoveMe      -> kontrolliert von BSM_VECTOR
**  BSF_RotateMe    -> kontrolliert von BSM_ROTATION
**  BSF_MainChild   -> gesetzt, wenn Bestandteil des Main-Zweiges
**  BSF_MainObject  -> gesetzt, wenn am ENDE des Main-Zweiges
**  BSF_PublishAll  -> == BSA_PublishAll
**  BSF_TerminateCollision  -> == BSA_TerminateCollision [*** OBSOLETE ***]
**  BSF_HandleInput         -> == BSA_HandleInput
**  BSF_EmbedRsrc           -> == BSA_EmbedRsrc
*/
#define BSF_MoveMe              (1<<0)
#define BSF_RotateMe            (1<<1)
#define BSF_MainChild           (1<<2)
#define BSF_PublishAll          (1<<3)
#define BSF_TerminateCollision  (1<<4)
#define BSF_HandleInput         (1<<5)
#define BSF_MainObject          (1<<6)  /* nicht zu verwechseln mit BSF_MainChild! */
#define BSF_EmbedRsrc           (1<<7)

/*-------------------------------------------------------------------
**  Messages für base.class-Methoden (außer collision_msg)
*/
struct newmother_msg {
    Object *mother;
    tform *mother_tform;
    struct MinList *child_list;
};

struct addchild_msg {
    Object *child;
};

struct main_msg {           /* SIEHE BSM_MAIN!!! */
    Object *main_child;
    Object *main_object;
};

struct setmainobject_msg {
    Object *main_object;
};

struct basepublish_msg {
    ULONG frame_time;                   // aus trigger_msg
    ULONG global_time;
    struct pubstack_entry *pubstack;    // Front-To-Back-PubStack
    struct argstack_entry *argstack;
    struct argstack_entry *eoargstack;      // End Of ...
    ULONG ade_count;                    // von/für trigger_msg
    ULONG maxnum_ade;                   // soviel paßt in die Stacks
    ULONG owner_id;                     // eindeutige 32-Bit-ID, non-persistent!
    FLOAT min_z;                        // Min-Clipping-Plane
    FLOAT max_z;                        // Max-Clipping-Plane
};

/*-------------------------------------------------------------------
**  TRIGGER MSG
*/
struct trigger_msg {
    ULONG global_time;      /* Ticks seit bestehen der Welt   */
    ULONG frame_time;       /* Ticks seit letzter BSM_TRIGGER */
    struct VFMInput *input; /* ausgefüllt von input_xxx.engine */
    ULONG ade_count;        /* Feedback: Anzahl ADEs durchgerattert */
    ULONG ade_drawn;        /* Anzahl "gezeichneter" ADEs */
};

/***-----------------------------------------------------------------
****    Die Vector Message
****    ~~~~~~~~~~~~~~~~~~
****    Die Mask-Bits sind außerordentlich WICHTIG!
****    Eine [x|y|z] Koordinate wird erst übernommen, wenn auch
****    das zugehörige Masken-Bit gesetzt ist!
****    Die Koordinaten können als FLOATs oder als LONGs übergeben
****    werden (über eine Union). Welches Format notwendig ist,
****    hängt vom jeweiligen Methoden-Dispatcher ab.
****
****    VEC_FORCE_ZERO  MUSS immer gesetzt werden, wenn ALLE
****    Koordinaten bewußt auf NULL gesetzt werden sollen. In diesem
****    Fall löscht das Object interne Flags, die Bewegung, Rotation
****    und/oder Kollisions-Kontrolle vollständig unterdrücken
****    (diese Flags sind BFM_MoveMe und/oder BSF_RotateMe).
****
****    Wenn VEC_RELATIVE gesetzt ist, werden die übergebenen
****    Koordinaten nicht als absolut interpretiert, sondern auf
****    die vorherigen Koordinaten aufaddiert.
****
****    Es existieren jetzt zwei verschiedene Vector-Message
****    Formate:
****        flt_vector_msg -> mit FLOAT-Koordinaten
****        lng_vector_msg -> mit LONG-Koordinaten
***/

struct flt_vector_msg {
    ULONG mask;             /* siehe unten */
    FLOAT x,y,z;
};

struct lng_vector_msg {
    ULONG mask;             /* siehe unten */
    LONG x,y,z;
};

/*** Bit-Definitionen für vector_msg.mask ***/
#define VEC_X   (1<<0)
#define VEC_Y   (1<<1)
#define VEC_Z   (1<<2)
#define VEC_FORCE_ZERO  (1<<3)
#define VEC_RELATIVE    (1<<4)

/*-------------------------------------------------------------------
**  IFF-Definitionen
*/
#define BASEIFF_VERSION (1)     /* siehe BASEIFF_IFFAttr-Chunk! */

#define BASEIFF_FORMID      MAKE_ID('B','A','S','E')
#define BASEIFF_IFFAttr     MAKE_ID('S','T','R','C')
#define BASEIFF_ADE         MAKE_ID('A','D','E','S')    /* FORM */
#define BASEIFF_Children    MAKE_ID('K','I','D','S')    /* FORM */

/*
**  BASEIFF_IFFAttr
**      Hält die Struktur base_IFFAttr. Bitte das Versions-Handling
**      genau beachten!
**
**  BASEIFF_ADE
**      Enthält alle ADEs, die an dem base.class Object hängen.
**      Nicht vorhanden, falls keine ADEs vorhanden sind.
**
**  BASEIFF_Children
**      Enthält alle Children des base.class Objects. Nicht
**      vorhanden, falls keine Kinder vorhanden sind.
**
**  Ein OBJT-Form *vor* dem BASEIFF_IFFAttrs Chunk ist ein
**  optionales embed.class Object, *nach* dem BASEIFF_IFFAttrs
**  Chunk das ebenfalls optionale Skeleton-Objekt.
**
**  attr_flags  hält jeweils ein gesetztes Bit, falls das zugehörige
**  boolsche Attribut TRUE ist. Man beachte, daß die attr_flags NICHT
**  den BSF_xxx-Flags entsprechen!
*/
struct base_IFFAttr {
    UWORD version;      /* for future combatibilty */

    /* ADEIFF_VERSION == 1 */
    FLOAT x,y,z;        /* für BSM_POSITION */
    FLOAT vx,vy,vz;     /* für BSM_VECTOR */
    FLOAT sx,sy,sz;     /* für BSM_SCALE */
    WORD ax,ay,az;      /* für BSM_ANGLE (Format beachten!) */
    WORD rx,ry,rz;      /* für BSM_ROTATION (Format beachten!) */

    UWORD attr_flags;   /* IAF_xxx (siehe unten */
    UWORD col_mode;     /* BSA_CollisionMode [OBSOLETE] */
    ULONG vis_limit;    /* BSA_VisLimit */
    ULONG ambience;     /* BSA_AmbientLight */
};

#define IAF_MoveMe          (1<<0)
#define IAF_RotateMe        (1<<1)
#define IAF_MainObject      (1<<2)
#define IAF_PublishAll      (1<<3)
#define IAF_TermCol         (1<<4)
#define IAF_HandleInput     (1<<5)
#define IAF_FollowMother    (1<<6)

/*-----------------------------------------------------------------*/
#endif
