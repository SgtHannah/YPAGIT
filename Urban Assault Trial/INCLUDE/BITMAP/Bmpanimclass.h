#ifndef BITMAP_BMPANIMCLASS_H
#define BITMAP_BMPANIMCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/bmpanimclass.h,v $
**  $Revision: 38.6 $
**  $Date: 1996/01/15 00:35:25 $
**  $Locker:  $
**  $Author: floh $
**
**  Die bmpanim.class realisiert Bitmap- und Outline-Animation
**  als Subklasse der bitmap.class.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"
#endif

#ifndef BITMAP_BITMAPCLASS_H
#include "bitmap/bitmapclass.h"
#endif

/*-------------------------------------------------------------------
**
**  NAME
**      bmpanim.class -- Bitmap- und Outline-Animation in einem
**                       Bitmap-Object
**
**  FUNCTION
**      Die bmpanim.class integriert Bitmap- und Outline-Animation
**      mit "Timecode"-Kontrolle in ein normal ansprechbares
**      Bitmap-Objekt (die bmpanim.class ist Subklasse der
**      bitmap.class). Zum Einladen der Bildinformation kann
**      eine Loaderklasse angegeben werden, die ebenfalls Subklasse
**      der Bitmap-Klasse sein muß, damit ist die Sache maximal
**      flexibel.
**
**      Die Klasse ist nicht gerade auf Komfort ausgelegt... es
**      sind nur wenige Attribute settable oder gettable.
**      Der Grund dafür ist, daß fast alle Attribute intern in einer
**      komplett anderen Form abgelegt werden. BANIMA_FilenamePool,
**      BANIMA_OutlinePool und BANIMA_TimeLine werden intern
**      so miteinander verschmolzen, daß der Overhead beim
**      "Abspielen" der Animation (via BMM_BMPOBTAIN) so gering
**      wie möglich bleibt.
**      Würde für all diese Parameter OM_GET unterstützt werden,
**      müßten sie aus der internen Repräsentation wieder
**      in die Attribut-Form zurückübersetzt werden... und das
**      wäre definitiv zu viel Voodoo.
**
**      Für interaktive Tools wird es am besten sein, alle Parameter
**      intern in einer gut editierbaren Form aufzubewahren und
**      nur bei Bedarf ein bmpanim.class-Object neu zu erzeugen,
**      zu saven und gleich wieder zu disposen.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OM_NEW
**      OM_DISPOSE
**      OM_GET
**      OM_SET
**      OM_SAVETOIFF
**      OM_NEWFROMIFF
**
**      rsrc.class
**      ~~~~~~~~~~
**      RSM_CREATE
**      RSM_FREE
**      RSM_SAVE
**
**      bitmap.class
**      ~~~~~~~~~~~~
**      BMM_BMPOBTAIN
**
**      bmpanim.class
**      ~~~~~~~~~~~~~
**      ---
**
**
**  ATTRIBUTES  
**      <default=none> ...bedeutet, daß dieses Attribut zwingend 
**                     vorhanden sein muß!
**
**      bitmap.class
**      ~~~~~~~~~~~~
**      BMA_Bitmap  (IG)   - <struct VFMBitmap *>
**          Bitte beachten, daß mit OM_GET(BMA_Bitmap) KEINE
**          Bitmap-Animation realisiert wird! Hier wird immer
**          nur ein Pointer auf die Bitmap des ersten Frames
**          zurückgeliefert!
**
**      bmpanim.class
**      ~~~~~~~~~~~~~
**      BANIMA_AnimID   (IG)    - <UBYTE *>     <default=NONE>
**          Pointer auf String mit eindeutiger ID für diese
**          Animations-Sequenz. Diese ID wird für's Resource-
**          Sharing benötigt.
**          Intern wird dieses Attribut als RSA_Name "weiterbehandelt",
**          ein OM_GET(RSA_Name) bringt das selbe Ergebnis wie ein
**          OM_GET(BANIMA_AnimID).
**
**      BANIMA_LoaderClass  (IG)    - <UBYTE *> <default=NONE>
**          Pointer auf String, mit dem die Loader-Klasse für
**          die einzelnen Frame-Objects definiert wird (für jeden
**          Frame wird intern ein Bitmap-Object angelegt.
**          Zum Beispiel:   ...BANIMA_LoaderClass, "ilbm.class",...
**
**      BANIMA_FilenamePool (I)     - <UBYTE **>    <default=NONE>
**          Pointer auf Array von String-Pointern (mit NULL-Pointer
**          terminiert), in dem alle für die Animations-Sequenz
**          benötigten Bilddaten-Files über ihre Filenamen definiert
**          sind. Die Files müssen über die mit BANIMA_LoaderClass
**          definierte Klasse geladen werden können.
**          Jeder Filename darf nur einmal vorkommen.
**          Folgendes Quelltext-Fragment ergibt z.B. einen gültigen
**          String-Pool:
**                      UBYTE *fn_pool[] = {
**                          "Roxanne1.ilbm",
**                          "Roxanne2.ilbm",
**                          "Roxanne3.ilbm",
**                          NULL,
**                      };
**          Sowohl das Pointer-Array als auch die Strings werden
**          in interne Speicherbereiche kopiert, sie können also
**          nach der Übergabe des Attributes gekillt werden.
**
**      BANIMA_OutlinePool  (I)     - <Pixel2D **>  <default=NONE>
**          Pointer auf Array von Outline-Pointern (mit NULL-Pointer
**          terminiert. Jede Outline sollte unique sein.
**          Siehe auch BANIMA_FilenamePool.
**
**      BANIMA_AnimType     (ISG)   - <ANIMTYPE_CYCLE | ANIMTYPE_PINGPONG>
**                                    <default=ANIMTYPE_CYCLE>
**          Definiert, ob die Animationssequenz als Cycle- oder
**          als PingPong-Sequenz abgespielt werden soll.
**
**      BANIMA_NumFrames    (IG)    - ULONG     <default=NONE>
**          Anzahl der Einzel-Frames dieser Animations-Sequenz.
**
**      BANIMA_Sequence     (I)     - <struct BmpAnimFrame *> <default=NONE>
**          Hier ist die eigentliche Animations-Sequenz definiert.
**          BANIMA_Sequence ist ein Array von Strukturen, die jeweils
**          einen Frame der Sequenz beschreiben. Dazu wird
**          per Array-Index auf den BANIMA_FilenamePool für den
**          für diesen Frame benötigten Picture-File und auf
**          den BANIMA_OutlinePool für die benötigte Outline
**          zugegriffen.
**          Die Struktur <struct BmpAnimFrame> beschreibt den
**          Aufbau eines einzelnen Frames der Animation:
**
**          struct BmpAnimFrame {
**              ULONG frame_time;    Verweildauer des Frames in millisec
**              UWORD fnpool_index;  Array-Index in Filename-Pool
**              UWORD olpool_index;  Array-Indes in Outline-Pool
**          };
**-----------------------------------------------------------------*/

#define BMPANIM_CLASSID "bmpanim.class"

/*------------------------------------------------------------------*/
#define BANIMM_BASE     (BMM_BASE+METHOD_DISTANCE)

/*------------------------------------------------------------------*/
#define BANIMA_BASE     (BMA_BASE+ATTRIB_DISTANCE)

#define BANIMA_AnimID       (BANIMA_BASE)   /* (IG) */
#define BANIMA_LoaderClass  (BANIMA_BASE+1) /* (IG) */
#define BANIMA_FilenamePool (BANIMA_BASE+2) /* (I)  */
#define BANIMA_OutlinePool  (BANIMA_BASE+3) /* (I)  */
#define BANIMA_AnimType     (BANIMA_BASE+4) /* (I)  */
#define BANIMA_NumFrames    (BANIMA_BASE+5) /* (IG) */
#define BANIMA_Sequence     (BANIMA_BASE+6) /* (I)  */

/*--------------------------------------------------------------------
**  Definitionen für BANIMA_AnimType
*/
#define ANIMTYPE_CYCLE      (0)
#define ANIMTYPE_PINGPONG   (1)

/*--------------------------------------------------------------------
**  Folgende Struktur definiert einen einzelnen Frame in der
**  Animations-Sequenz. In einem Array mit <BANIMA_NumFrames> Feldern
**  werden diese beim OM_NEW mittels BANIMA_Sequence übergeben.
*/
struct BmpAnimFrame {
    ULONG frame_time;   /* Verweildauer des Frames in millisec */
    UWORD fnpool_index; /* Array-Index in Filename-Pool */
    UWORD olpool_index; /* Array-Index in Outline-Pool */
};

/*--------------------------------------------------------------------
**  INTERNAL FACTS
**  ~~~~~~~~~~~~~~
**  Weil die bmpanim.class auch Subklasse der resource.class
**  ist, bietet sich an, sämtliche Speicherplatz-intensiven
**  Read-Only-Parameter als <Shared Resource> verwalten zu lassen,
**  und nur die Object-spezifischen Sachen in der LID zu halten.
**  Zur Zeit hält die bmpanim.class folgende Attribute in
**  einer Shared Resource:
**      - den Filename-Pool
**      - den Outline-Pool
**      - ein Bitmap-Object-Pool (für jeden Pict-File ein Object)
**      - den Namen der LoaderClass
**      - die NumFrames
**      - die eigentliche Sequenz-Definition
**      - und die AnimID "rekarniert" als RA_Name
**
**  Faktisch alle diese Attribute liegen in einem internen
**  Format vor, daß nicht den "öffentlichen" Attribut-Formaten
**  entspricht.
*/

/*-------------------------------------------------------------------
**  Die interne Definition eines Frames (diese sind in einem
**  ReadOnly-Array zusammengefaßt, das Teil der Shared Resource
**  ist).
**
**  Der VFMBitmap-Pointer wird per OM_GET(BMA_BITMAP) vom über den
**  <pict_index> definierten Bitmap-Object im Bitmap-Pool
**  ermittelt.
**
**  <filename_index> und <outline_index> sind die Nummern des
**  zum Frame gehörenden Picture-Filenamen im Filename-Pool 
**  und der Outline im Outline-Pool, das ist redundante
**  Information zum effizienten Abspeichern des bmpanim.class-Objects.
*/
struct internal_frame {
    struct VFMOutline *outline;     // direkter Pointer in Outline-Pool
    struct VFMBitmap *bitmap;
    LONG frame_time;                // Verweilzeit in millisec
    WORD pict_index;
    WORD outline_index;
};

/*-------------------------------------------------------------------
**  Für jeden Pict-File im Filename-Pool wird eine
**  <struct object_info> im Object-Pool erzeugt.
*/
struct object_info {
    Object *bitmap_object;
    struct VFMBitmap *bitmap;
    UBYTE *pict_filename;       /* Pointer in Filename-Pool */
};

/*-------------------------------------------------------------------
**  Der <sequence_header> wird als Shared Resource verwaltet
**  und enthält alle Read Only Parameter der Animations-Sequenz.
**
**  Der <filename_pool> ist ein einziger Speicherblock, der
**  alle Picture-Filenamen für die Sequenz enthält, abgetrennt
**  durch ein Null-Byte. Der gesamte Pool ist durch ein
**  Null-Word begrenzt (also zwei Null-Bytes). Das ist notwendig
**  für die ANSI-Funktion strbpl().
**
**  Der <outline_pool> ist aufgebaut wie der <filename_pool>,
**  allerdings ist die Begrenzung zwischen den Outline
**  mittels einem normalen Ende-Pixel2D-Element (Pixel2D.flags = P2DF_Done)
**  definiert. Die Gesamt-Größe des Outline-Pools ist (indirekt)
**  angegeben durch <ne_olpool>
**
**  Der <object_pool> enthält für jeden Pict-Filename eine
**  ausgefüllte <struct object_info>.
**
**  Die <loader_class> ist ein normaler C-String, der in einen
**  eigenen Puffer kopiert wurde.
**
**  Die <sequence> ist ein Array von <struct internal_frame>'s,
**  die die Animations-Sequenz definierten. Die Größe des
**  Arrays ergibt sich aus (sizeof(struct internal_frame)*num_frames).
**
**  <endof_sequence> zeigt auf das Element NACH dem letzten Element
**  (deshalb darf über diesen Pointer nie gelesen oder geschrieben
**  werden!). Er dient nur als Vergleichs-Pointer, ob beim Durchsteppen
**  der <sequence> bereits das Ende erreicht ist.
*/
struct sequence_header {
    UBYTE *filename_pool;
    struct VFMOutline *outline_pool;
    struct object_info *object_pool;
    UBYTE *loader_class;
    struct internal_frame *sequence;
    struct internal_frame *endof_sequence;
    UWORD num_frames;
    UWORD sizeof_fnpool;    /* Gesamtgröße Filename-Pool in Bytes */
    UWORD ne_olpool;        /* Anzahl Elemente in Outline-Pool inkl.Ende-Elemente! */
    UWORD ne_objpool;       /* Anzahl Einträge im Object-Pool */
};

/*--------------------------------------------------------------------
**  Die LID der bmpanim.class
*/
struct bmpanim_data {
    struct sequence_header *SeqHeader;  // Pointer auf Shared Resource
    struct internal_frame *ActFrame;    // aktueller Frame
    LONG TimeStamp;                 // TimeStamp des aktuellen Global-Frames
    LONG FrameTimeOverflow;     // wieviele millisec wurden angeknabbert?
    WORD AnimType;              // ANIMTYPE_PINGPONG|ANIMTYPE_SEQUENCE
    WORD FrameAdder;            // +/- sizeof(struct internal_frame)
};

/*=================================================================**
**  DISK STUFF                                                     **
**=================================================================*/

/*--------------------------------------------------------------------
**  Die bmpanim.class unterstützt das Resourcesharing im Filesystem.
**  Anstatt die Shared Resource Daten in JEDES Diskobject einzubetten,
**  werden diese unter RA_Name als Filename ins MC2resources:rsrcpool
**  Directory geschrieben (das ist vollständig für die resource.class
**  und deren Subklassen reserviert, siehe "resourceclass/resourceclass.h".
**
**  Das Verfahren hat zur Zeit noch eine kleine Anomalie:
**  JEDES Diskobject wird seine Shared Resource speichern, damit
**  werden die Resource-Files so oft überschrieben, wie Clients
**  für die jeweilige Resource existieren. Das tritt aber nur
**  beim OM_SAVETOIFF auf, bei einem OM_NEWFROMIFF werden die
**  Resource-Files nicht mal angerührt, sobald die zugehörige
**  Resource bereits in der internen Resource-Liste der resource.class
**  existiert!
**
**  Fileformat der bmpanim.class Resource-Files:
**
**  UWORD sizeof_loaderclass / Größe des Loaderclass-Files (strlen())
**  UBYTE[] loaderclass;    / die Loaderclass als C-String
**  UWORD sizeof_fnpool;    / Gesamt-Größe des Filenamepools
**  UBYTE[] fnpool;         / der gesamte Filenamepool, als Block
**  UWORD ne_olpool;        / Gesamt-Anzahl Elemente in Outline-Pool, inklusive Ende-Elemente
**      UWORD ne_ol;        / Anzahl Elemente aktuelle Outline
**      UBYTE x,y[];        / alle Koordinaten der Outline
**
**      UWORD ne_ol;        / nächste Outline im Pool...
**      ....
**
**  UWORD num_frames;       / Anzahl Frames in der Sequence
**      struct BmpAnimFrame sequence[]  /die Sequenz selbst.
**
**
**  Revision 12-Jan-96:
**  ~~~~~~~~~~~~~~~~~~~
**  Das o.g. Dateiformat wird jetzt generell in einen
**  FORM VANM eingebettet, ansonsten ändert sich aber nichts.
**  Das "alte" rohe Format wird nach wie vor akzeptiert
**  (für eine Übergangs-Zeit). Die Änderung wurde durch
**  die Einführung von RSM_SAVE notwendig, das generall
**  IFF Files benötigt, um alle Features nutzen zu können
**  (z.B. Collection-Files).
*/

/*-------------------------------------------------------------------
**  Komprimiertes Format eines Outline-Pixel2D-Elements
**  im Resource-File
*/
struct ol_atom {
    UBYTE x,y;
};

/*-------------------------------------------------------------------
**  IFF STUFF
*/
#define BANIMIFF_VERSION    1

#define BANIMIFF_FORMID MAKE_ID('B','A','N','I')
#define BANIMIFF_IFFAttr  MAKE_ID('S','T','R','C')
/*  Hält folgende Struktur: */
struct banim_IFFAttr {
    UWORD version;
    /* BMPANIM_VERSION == 1 */
    UWORD animID_offset;  /* Start des AnimID-Strings ab Chunk-Anfang */
    UWORD anim_type;    /* BANIMA_AnimType */
};
/* Nach dieser Struktur kommt der AnimID-String. Benutze
** banim_IFFAttr.animID_offset, um String-Amfang zu finden!!!
*/

/*-------------------------------------------------------------------
**  FORM ID für Bmpanim-Resourcefile-Format:
*/
#define BMPANIM_VANM    MAKE_ID('V','A','N','M')
#define BMPANIM_DATA    MAKE_ID('D','A','T','A')

/*-----------------------------------------------------------------*/
#endif

