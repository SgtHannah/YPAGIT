#ifndef BITMAP_BITMAPCLASS_H
#define BITMAP_BITMAPCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/bitmapclass.h,v $
**  $Revision: 38.15 $
**  $Date: 1998/01/06 12:53:41 $
**  $Locker:  $
**  $Author: floh $
**
**  Die bitmap.class ist die RootClass aller Klassen, die
**  Bitmaps bereitstellen und manipulieren. Sie ist Subklasse
**  der resource.class und erbt damit alle Resource-Verwaltungs-
**  Fähigkeiten.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef RESOURCES_RSRCCLASS_H
#include "resources/rsrcclass.h"
#endif

#ifndef BITMAP_BITMAP_H
#include "bitmap/bitmap.h"
#endif

/*-------------------------------------------------------------------
**
**  NAME
**      bitmap.class -- Stammklasse für Bitmap-behandelnde Klassen
**
**  FUNCTION
**      Die bitmap.class ist die Stammklasse aller Klassen,
**      die irgendwie mit VFMBitmaps umgehen.
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
**
**      bitmap.class
**      ~~~~~~~~~~~~
**
**      BMM_BMPOBTAIN
**          Msg:    struct bmpobtain_msg
**          Ret:    ausgefüllte Msg
**
**          Neue Standard-Methode (07-Jul-95) zur Ermittlung der
**          aktuellen Bitmap-Informationen des Objects.
**
**          In: 
**              msg.time_stamp  -> globaler Timestamp des aktuellen Frames
**              msg.frame_time  -> Zeitdauer des aktuellen Frames
**          Out:
**              msg.bitmap      -> Ptr auf Bitmap-Struktur
**              msg.outline     -> optionaler Pointer auf Outline
**
**          *** WICHTIG ***
**          Die Outline-Information wird im "neuen" Outline-Format
**          zurückgegeben, als FLOAT-Zahlen zwischen 0.0 und 1.0.
**          Intern werden jetzt alle Outlines in diesem Format
**          gehalten, aus Backward-Compatibilty-Gründen sind die
**          bisher benutzten Methoden und Attribute so umgeschrieben,
**          das sie das alte Format akzeptieren/zurückliefern.
**
**
**  ATTRIBUTES
**      BMA_Bitmap (G) - <struct VFMBitmap *>
**          Returniert einen Pointer auf die eingebettete
**          VFMBitmap-Struktur
**
**      BMA_Outline (ISG) - <Pixel2D *> 
**          (S) OBSOLETE!
**
**          Definiert die Outline des Bitmap-Objekts. Das ist
**          ein 2D-Polygon, der über die Bitmap gespannt
**          ist (und zum Beispiel zur [u,v] Definition
**          beim Txt-Mapping verwendet wird).
**
**      BMA_Width   (IG) - <ULONG>
**          Breite der Bitmap in Pixel.
**
**      BMA_Height  (IG) - <ULONG>
**          Höhe der Bitmap in Pixel.
**
**      BMA_Type *** OBSOLETE *** OBSOLETE *** OBSOLETE ***
**
**      BMA_Body    (IG) - <UBYTE *>
**          Pointer auf die Bitmap-Daten selbst. Wird dieses
**          Attribut bei _new() angegeben, wird
**          kein eigener Bereich allokiert, sondern der
**          angegebene benutzt. In diesem Fall muß aber der
**          Erzeuger des Buffers für seine korrekte Freigabe
**          sorgen, nachdem das Bitmap-Objekt disposed wurde!
**
**      BMA_HasColorMap  (IG) - <BOOL>
**          Falls TRUE wird eine ColorMap allokiert und an die Bitmap 
**          gehängt. Mit (G) kann man abfragen, ob eine ColorMap 
**          existiert.
**
**      BMA_ColorMap    (ISG)  - <UBYTE *>
**          Dient zum Schreiben bzw. Lesen der gesamten ColorMap.
**          Der erwartete Pointer zeigt auf ein UBYTE[3]-Array
**          mit 256 Elementen zu je 3 8-Bit-Werten für R,G,B
**          (in dieser Reihenfolge). Der Inhalt des Arrays wird kopiert.
**          Falls der Status von (G) BMA_HasColorMap <FALSE> ist,
**          passiert NICHTS!
**
**      BMA_Texture (IG) - <BOOL>
**          Die Bitmap soll als Textur verwendet werden. Das hat
**          Auswirkungen, sobald ein primäres raster.class
**          (== Display Treiber Object) existiert, weil dieses
**          ein Custom-Textur-Format verwenden kann. Ein Bitmap-
**          Objekt, welches mit dem BMA_Texture Attribut erzeugt
**          wurde, besitzt nicht unbedingt das Standard-Textur-
**          Format und darf nicht direkt manipuliert werden!
**
**      BMA_TxtBlittable    (IG) - <BOOL>
**          Textur soll im Display-Pixelformat vorliegen,
**          muß zusätzlich zu BMA_Texture als Modifier
**          angegeben werden!
**
**  NOTE
**      Die bitmap.class untersützt kein _load()/_save(),
**      das bleibt Sache der spezifischeren Subklassen.
**
**-----------------------------------------------------------------*/
#define BMM_BASE        (RSM_BASE+METHOD_DISTANCE)

#define BMM_GETBITMAP   (BMM_BASE)          // OBSOLETE
#define BMM_GETOUTLINE  (BMM_BASE+1)        // OBSOLETE
#define BMM_BMPOBTAIN   (BMM_BASE+2)
/*-----------------------------------------------------------------*/
#define BMA_BASE            (RSA_BASE+ATTRIB_DISTANCE)

#define BMA_Bitmap          (BMA_BASE)      // (G)
#define BMA_Outline         (BMA_BASE+1)    // (ISG)
#define BMA_Width           (BMA_BASE+2)    // (IG)
#define BMA_Height          (BMA_BASE+3)    // (IG)
#define BMA_Type            (BMA_BASE+4)    // *** OBSOLETE ***
#define BMA_Body            (BMA_BASE+5)    // (IG)
#define BMA_HasColorMap     (BMA_BASE+6)    // (IG)
#define BMA_ColorMap        (BMA_BASE+7)    // (ISG)
#define BMA_Texture         (BMA_BASE+8)    // (IG)
#define BMA_TxtBlittable    (BMA_BASE+9)    // (IG)

/*-----------------------------------------------------------------*/
#define BITMAP_CLASSID "bitmap.class"

/*-----------------------------------------------------------------*/
struct bitmap_data {
    struct VFMBitmap *bitmap;       // Ptr auf VFMBitmap
    struct VFMOutline *outln;       // optional
    ULONG flags;                    // siehe unten
};  

/*-------------------------------------------------------------------
**  Message-Strukturen
*/
struct frametime_msg {
    ULONG frame_time;
};

struct bmpobtain_msg {
    LONG time_stamp;            /* In */
    LONG frame_time;            /* In */
    struct VFMBitmap *bitmap;   /* Out */
    struct VFMOutline *outline; /* optional out */
};

/*-----------------------------------------------------------------*/
#endif

