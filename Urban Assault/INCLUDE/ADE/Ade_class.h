#ifndef ADE_ADE_CLASS_H
#define ADE_ADE_CLASS_H
/*
**  $Source: PRG:VFM/Include/ade/ade_class.h,v $
**  $Revision: 38.28 $
**  $Date: 1996/08/31 00:32:28 $
**  $Locker:  $
**  $Author: floh $
**
**  ade_class-private Definitionen
**
**  29-Sep-93   floh    ade_Flags eingeführt
**  01-Oct-93   floh    - ade_data-Struktur mit ObjectNodes ausgestattet
**                      - IFF-Definitionen erledigt
**  02-Oct-93   floh    Revisionskontrolle an RCS übergeben.
**
**  (C) Copyright 1993 by A.Weißflog
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

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef NUCLEUS_NUCLEUS_H
#include "nucleus/nucleus2.h"
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      ade.class -- Stammklasse aller Atomic Data Elements
**
**  FUNCTION
**      Atomic Data Elements sind "nicht-teilbare" LowLevel-Objekte,
**      die in irgendeiner Weise an 3D-Koordinaten gebunden sind.
**      Ein typisches ADE ist z.B. ein Flächen-Objekt.
**      ADEs werden normalerweise durch ein base.class-Object
**      zu einem komplexen 3D-Objekt zusammengefaßt.
**      ADEs sind "nicht autonom", daß heißt, allein für sich sind
**      sie relativ nutzlos, da sie auf extern bereitgestellte Puffer
**      und Objekte angewiesen sind, die sie sich mit anderen ADEs teilen.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OM_NEW
**      OM_DISPOSE
**      OM_GET
**      OM_SET
**      OM_NEWFROMIFF
**      OM_SAVETOIFF
**
**      ade.class
**      ~~~~~~~~~
**      ADEM_CONNECT
**          Msg:    struct adeconnect_msg
**          Ret:    void
**
**          Das ADE wird an eine beliebige doppelt verkettete
**          Liste angehängt. In der Message wird der Pointer
**          auf die Liste übergeben, die Verkettung erfolgt
**          mit Hilfe der internen Node "ade_Private".
**
**      ADEM_PUBLISH    + >struct publish_msg<
**          Msg:    struct publish_msg
**          Ret:    void
**
**          !!! ACHTUNG !!! Neues Publish-Konzept!
**          Das alte Konzept mit der Publish-Liste hatte einen
**          entscheidenden Haken, der die Wiederverwendbarkeit von
**          ADEs unmöglich machte. Jedes ADE konnte sich nämlich
**          logischer- aber dummerweise nur EINMAL in die Liste
**          einsortieren.
**
**          Das neue Publish-Konzept arbeitet dagegen nicht mit einer
**          Publish-Liste, sondern mit zwei Stacks, in denen ein
**          ADE bei jedem ADEM_PUBLISH "einen Abdruck hinterläßt".
**          Und das geht so:
**
**          Die beiden genannten Stacks heißen "Publish Stack" und
**          "Args Stack". Der Publish-Stack besteht aus einzelnen
**          <struct pubstack_entry>'s. Jede dieser Strukturen hält eine
**          Priorität (normalerweise die größte Entfernung des ADEs
**          zum Beobachter) und einen Pointer auf das zugehörige
**          Element im Args-Stack.
**          Der Args-Stack hält alle Informationen, die letztenendes
**          zur Darstellung (etc.) notwendig sind, also welche
**          Routine aufgerufen werden soll und welche Parameter
**          dafür zu übergeben sind.
**
**          In der Publish-Phase pusht jedes ADE, das sich publishen
**          will, je ein ausgefülltes Element auf den Publish-Stack und
**          auf den Args-Stack. Wenn die Publish-Phase abgeschlossen
**          ist, läßt man einen qsort() über den Publish-Stack, womit
**          dieser "tiefensortiert" wird. Dann rasselt man den sortierten
**          Publish-Stack durch und kann via dem Arg-Stack-Pointer
**          jedes Publish-Elements die zur Darstellung notwendigen
**          Routinen in der richtigen Reihenfolge und mit den richtigen
**          Parametern aufrufen... fertig!
**
**          *** WICHTIG ***
**          Die ADEM_PUBLISH-Methode muß die "neuen" PubStack- und
**          ArgStack-Pointer in die übergebene <msg> zurückschreiben,
**          falls sie sich verändert haben!!! Es ist also wichtig,
**          für einen ADEM_PUBLISH-Aufruf _methoda() zu verwenden,
**          weil die <msg> sonst auf dem Stack aufgebaut wird und
**          nach dem Methoden-Aufruf verloren ist. Damit wären aber
**          auch die (evtl.) modifizierten Stack-Pointer weg.
**
**  ATTRIBUTES
**      nucleus.class
**      ~~~~~~~~~~~~~
**      ---
**      ade.class
**      ~~~~~~~~~
**      ADEA_Global3DPool   - (S) Pixel3D *             *** OBSOLETE ***
**      ADEA_Viewer3DPool   - (S) Pixel3D *             *** OBSOLETE ***
**      ADEA_Viewer2DPool   - (S) Pixel2D *             *** OBSOLETE ***
**      ADEA_VisLimit       - (ISG) ULONG (max. +32767) *** OBSOLETE ***
**
**      ADEA_BackCheck      - (ISG) BOOL
**              Manche ADEs sind des "Back Face Culling's" mächtig. Also
**              "Flächenrückenuntersuchung". Mit diesem Attribut kann
**              es ein- und ausgeschaltet werden. Viele Flächen-ADEs
**              ignorieren dieses Attribut aber, weil der entsprechende
**              Polygon-Füller nicht "verkehrt herum" füllen kann.
**
**      ADEA_DepthFade      - (ISG) BOOL
**              Dieses Flag schaltet das DepthFading ein/aus. Dieser Begriff
**              ist nicht zu eng zu sehen, z.B. bedeutet es bei visuellen
**              ADEs, daß mit wachsender Entfernung das ADE zur Hintergrund-
**              farbe hin verblaßt, bei Audio-ADEs kann hiermit der
**              Surround-Sound ein-/ausgeschaltet werden.
**
**      ADEA_ExtSkeleton    *** OBSOLETE ***
**
**      ADEA_Point  - (ISG) ULONG
**              Bei einem "punkt-orientierten" ADE wird hiermit die
**              Nummer des 3D-Punkts im 3D-Pool des zugehörigen
**              Skeletons definiert (siehe ADEA_Skeleton).
**
**      ADEA_Polygon    - (ISG) ULONG
**              Bei einem "polygon-orientierten" ADE wird hiermit die
**              Nummer der Flächen-Definition im "Area-Array" des
**              zugehörigen Skeletons definiert (siehe ADEA_Skeleton).
**
**      ADEA_Detail         *** OBSOLETE ***
**      ADEA_DetailLimit    *** OBSOLETE ***
**
**      ADEA_PrivateNode    - (G) <struct ObjectNode *>
**              Dieses Attribut ist nur gettable und repräsentiert
**              den Pointer auf die interne ade_Private-Node.
**              Bitte nicht benutzen, wenn sich's nicht verhindern
**              läßt!!!
**
**  NOTE
**      Die meisten ADEA_Attribute werden vom ade.class-Dispatcher in
**      der ade.class-LID aufbewahrt, und bei OM_GET zurückgegeben.
**      Außerdem wird beim Laden des ADEs von Disk immer ein OM_SET
**      auf die entsprechenden Attribute angewendet. Damit müssen
**      die konkreten ADE-Subklassen diese Attribute nicht selbst im
**      IFF-Chunk ablegen. Selbstverständlich sollten zeitkritische
**      Attribute (solche die zur Laufzeit benötigt werden) bei den
**      entsprechenden Methoden von der konkreten ADE-Subklasse abgefangen
**      und in der eigenen LID abgelegt werden. Die konkrete Subklasse
**      braucht sich aber bei OM_GET oder OM_SAVETOIFF nicht um diese
**      abgefangenen Attribute zu kümmern, da dies bereits die
**      ade.class erledigt.
*/

/*-------------------------------------------------------------------
**  Die Klassen-ID der ade_class
*/
#define ADE_CLASSID "ade.class"

/*-------------------------------------------------------------------
**  Die Locale Instance Data der ade_class
*/
struct ade_data {
    struct ObjectNode ade_Private;

    UWORD ade_Flags;            /* verschiedenes */
    UWORD ade_Point;            /* ADEA_Point */
    UWORD ade_Polygon;          /* ADEA_Polygon */
};

/*-------------------------------------------------------------------
**  ade_Flags-Definitionen
*/
#define ADEF_InListPrivate  (1<<0)  /* ade_Private ist Node in einer Liste */
                                    /* -> das heißt, falls ADEM_CONNECT    */
                                    /* dieses Flag gesetzt vorfindet, muß  */
                                    /* vorher die Node Remove()'d werden.  */
#define ADEF_DepthFade      (1<<1)  /* ADEA_DepthFade */
#define ADEF_BackCheck      (1<<2)  /* ADEA_BackCheck */

/*-------------------------------------------------------------------
**  Definition der neuen MethodID's
**  (Superclass = nucleusclass)
*/
#define ADEM_BASE               (OM_BASE+METHOD_DISTANCE)

#define ADEM_CONNECT            (ADEM_BASE)
#define ADEM_PUBLISH            (ADEM_BASE+1)
#define ADEM_EXPRESS            (ADEM_BASE+2)   /* OBSOLETE */
#define ADEM_SETPRI             (ADEM_BASE+3)   /* OBSOLETE */

/*-------------------------------------------------------------------
**  Definition der Attribute
**  (I) = Initialsierbar
**  (G) = Gettable
**  (S) = Settable
**
**  05-Jan-94:  Die neuen Common ADE Attributes definieren Attribute,
**              die für alle (vorerst: alle visuellen) ADEs Gültigkeit
**              besitzen (also quasi virtuelle Attribute).
**              Achtung: Nicht alle konkreten ADEs unterstützen
**              alle Attribute, nicht unterstützte Attribute
**              werden aber garantiert ignoriert und weitergereicht.
*/
#define ADEA_Base           (OMA_BASE+ATTRIB_DISTANCE)
#define ADEA_Global3DPool   (ADEA_Base)     // OBSOLETE
#define ADEA_Viewer3DPool   (ADEA_Base+1)   // OBSOLETE
#define ADEA_Viewer2DPool   (ADEA_Base+2)   // OBSOLETE

#define ADEA_BackCheck      (ADEA_Base+3)   // (ISG) <BOOL>
#define ADEA_DepthFade      (ADEA_Base+4)   // (ISG) <BOOL>
#define ADEA_VisLimit       (ADEA_Base+5)   // OBSOLETE

#define ADEA_ExtSkeleton    (ADEA_Base+6)   // OBSOLETE
#define ADEA_Point          (ADEA_Base+7)   // (ISG) <ULONG>
#define ADEA_Polygon        (ADEA_Base+8)   // (ISG) <ULONG>

#define ADEA_Detail         (ADEA_Base+9)   // OBSOLETE
#define ADEA_DetailLimit    (ADEA_Base+10)  // OBSOLETE

#define ADEA_PrivateNode    (ADEA_Base+11)  /* (G) <struct ObjectNode *> */

/*-------------------------------------------------------------------
**  Default-Werte für ein paar Attribute
*/
#define ADEA_BackCheck_DEF      FALSE
#define ADEA_DepthFade_DEF      FALSE
#define ADEA_Detail_DEF         DETAIL_DEFAULT
#define ADEA_DetailLimit_DEF    DETAIL_NONE

/*-------------------------------------------------------------------
**  Definition der einzelnen Elementar-Strukturen für
**  Publish Stack und Args Stack.
**
**  Ein <struct argstack_entry> hält einen Pointer auf eine
**  Darstellungs-Funktion und einen zugehörigen Argument-Block.
**  Die <draw_func> bekommt als Argument einen Pointer auf den
**  Args-Block des Elements, wobei beides natürlich zusammenpassen muß!
**
**  Der <struct pubstack_entry> ist so kurz wie möglich gehalten
**  (wegen dem qsort()) und hält außer der Entfernung zum Beobachter
**  einen Pointer auf das zugehörige Element im Args-Stack. Weil dieser
**  Pointer beim Sortieren "mitgenommen" wird, kann das zugehörige
**  Args-Stack-Element jederzeit sicher erreicht werden :-)
**
**  Die konkreten Stacks sind werden übrigens NICHT von der
**  ade.class verwaltet, sondern müssen extern bereitgestellt werden.
*/
struct argstack_entry {
    void (*draw_func) (void *);
};  /* ab hier dann der Args-Block... */

struct pubstack_entry {
    LONG depth;                     /* Entfernung zum Beobachter */
    struct argstack_entry *args;    /* Pointer in den Args-Stack */
};

/*-------------------------------------------------------------------
**  Custom-Messages:
*/
struct adeconnect_msg {
    struct List *list;
};

struct publish_msg {

    ULONG owner_id;             // 32-Bit ID (Serien-Nummer) des ADE-Besitzers

    /* Publish-Stuff */
    struct pubstack_entry *pubstack;  // Back-To-Front-PubStack
    struct argstack_entry *argstack;  // Pointer auf Top Of Args Stack

    /* Zeit */
    ULONG time_stamp;           // dieses Frames
    ULONG frame_time;           // in Ticks (== 1/1000 sec)

    /* Geometrie und Environment-Information */
    tform *viewer;              // TForm des aktuellen Viewers
    tform *owner;               // TForm des Besitzer dieses ADEs
    Object *sklto;              // Skeleton-Object des Besitzers
    struct Skeleton *sklt;      // ...dessen Skeleton-Struktur
    FLOAT min_z;                // Entfernung der Front-Clipping-Plane
    FLOAT max_z;                // Entfernung der Back-Clipping-Plane

    /* Licht */
    FLOAT dfade_start;
    FLOAT dfade_length;
    LONG ambient_light;

    /* Feedback */
    ULONG ade_count;            // Anzahl ADEs bearbeitet
};

/*-------------------------------------------------------------------
**  IFF-Zeugs
*/
#define ADEIFF_VERSION (1)  /* siehe ADEIFF_IFFAttr-Chunk-Struktur! */

#define ADEIFF_FORMID       MAKE_ID('A','D','E',' ')
#define ADEIFF_Attributes   MAKE_ID('A','T','T','R')    /* OBSOLETE */
/*
**  Der ADEIFF_Attributes Chunk ist nicht mehr gültig und
**  wird vollständig ignoriert, eine Rückwärts-Kompatiblität
**  wäre wegen den verschobenen Attribut-IDs sowieso innerhalb
**  der ade.class schwierig zu realisieren, ich werde allerdings
**  über einen externen Konverter nachdenken...
*/
#define ADEIFF_IFFAttr  MAKE_ID('S','T','R','C')
/* 
**  Dieser Chunk hat folgende festgelegte (und nach hinten
**  erweiterbare Struktur):
*/
struct ade_IFFAttr {
    UWORD version;      /* für Backward-Compatibility */

    /* ADEIFF_VERSION == 1 */
    UBYTE detaillimit;  /* ADEA_DetailLimit */
    UBYTE flags;        /* ADEF_BackCheck | ADEF_DepthFade */
    UWORD point_nr;     /* ADEA_Point */
    UWORD polygon_nr;   /* ADEA_Polygon */
    UWORD vis_limit;    /* ADEA_VisLimit (OBSOLETE!!!) */

    /* ADEIFF_VERSION == 2 */   /* für Erweiterungen... */
};
/*
**  OM_NEWFROMIFF baut daraus eine Attribut-TagListe und
**  schickt diese per OM_SET an sich selbst.
*/

/*-----------------------------------------------------------------*/
#endif
