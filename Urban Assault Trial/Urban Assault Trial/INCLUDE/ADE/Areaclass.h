#ifndef ADE_AREACLASS_H
#define ADE_AREACLASS_H
/*
**  $Source: PRG:VFM/Include/ade/areaclass.h,v $
**  $Revision: 38.20 $
**  $Date: 1996/01/20 17:13:59 $
**  $Locker:  $
**  $Author: floh $
**
**  Die Area-Class zeichnet ist das Universal-ADE für
**  "normale" Flächen.
**  (22-Sep-94) - Mit den neuen Polygon-Füllern wird auch die
**  area.class clean gemacht.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef POLYGON_POLYGON_H
#include "polygon/polygon.h"
#endif

#ifndef ADE_ADE_CLASS_H
#include "ade/ade_class.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      area.class  - allgemeine Universal-Flächen-Klasse
**
**  FUNCTION
**      Die area.class ist der "Leim" zwischen der virtuellen
**      3D-Welt und dem realen 2D-Polygon-Füller DrawPolygon() 
**      in der OVE.
**
**      Programmierer & Designer:
**          Die area.class macht jetzt praktisch keine Tests
**          mehr, ob eine Attribut-Konstellation sonderlich
**          sinnvoll ist. Die meisten Attribute füllen
**          nur interne Slots auf. Ob diese Slots
**          bei Aktivierung sinnvolle Werte enthalten
**          (vor allem in Kombination), ist ein paralleler Prozeß...
**
**  METHODS
**      keine eigenen Methoden
**
**  ATTRIBUTES
**      AREAA_Map   - (ISG)
**          Auswahl des zu verwendenden Mapping-Algorithmus
**          Dabei bedeutet:
**              MAP_NONE
**              MAP_LINEAR
**              MAP_DEPTH   (aka perspektiv-korrigiert)
**
**      AREAA_Txt   - (ISG)
**          Textur-Mapping an/aus
**              TXT_NONE
**              TXT_MAPPED
**
**      AREAA_Shade - (ISG)
**          Kontrolliert Shading-Verhalten der Fläche:
**              SHADE_NONE
**              SHADE_FLAT  [nicht implementiert]
**              SHADE_LINE  [obsolete]
**              SHADE_GRADIENT  [normales Verlaufs--Shading]
**
**      AREAA_Tracy - (ISG)
**          Transparenz-Verhalten:
**              TRACY_NONE
**              TRACY_CLEAR [Null-Farbe ist 100% transparent]
**              TRACY_FLAT  [nicht implementiert]
**              TRACY_MAPPED    [nicht implementiert]
**
**      AREAA_TracyMode - (ISG) [*** OBSOLETE *** OBSOLETE ***]
**
**      AREAA_TxtBitmap - (ISG) Object * [Instance of bitmap.class]
**          Falls das Attribut AREAA_Txt den Wert TXT_MAPPED hat, muß
**          über das Attribut AREAA_TxtMap ein Pointer auf ein
**          existierendes Object der bitmap.class angegeben
**          werden. Dieses bitmap.class Object geht komplett in den
**          Besitz der area.class über, darf also von diesem
**          nach Belieben disposed werden.
**          Das bitmap.class Object muß eine Bitmap UND eine
**          Outline enthalten. Andernfalls kann (lies: <wird>) es zu
**          unkontrolliertem Verhalten des area.class Objects kommen.
**          Siehe auch AREAA_TracyBitmap.
**
**      AREAA_TracyBitmap   - (ISG) Object * [instance of bitmap.class]
**          Falls das Attribut AREAA_Tracy auf TRACY_MAPPED steht,
**          definiert das Attribut AREAA_TracyBitmap einen Pointer
**          auf ein bitmap.class Object, von dem sich das area.class
**          Object alle Informationen zum Tracy-Mapping holt
**          (Bitmap-Daten und Outline). Die Outline wird ignoriert,
**          falls Texture-Mapping mit Transparenz-Mapping kombiniert
**          wird ((AREAA_Txt == TXT_MAPPED) && (AREAA_Tracy == TRACY_MAPPED)).
**          In diesem Fall liefert das AREAA_TxtBitmap Object die
**          Outline zum Mappen.
**
**      AREAA_ColorValue    - (ISG) ULONG [0..255]
**          Zugriff auf den "Color Slot" der Fläche. Wird
**          DrawPolygon() überreicht.
**
**      AREAA_ShadeValue    - (ISG) ULONG [0..255]
**          Der Shade-Value halt. Wird DrawPolygon() übergeben.
**          Allerdings modifiziert eingeschaltetes DepthFading
**          diesen Wert!
**
**      AREAA_TracyValue    - (ISG) ULONG [0..255]
**          Siehe AREAA_ColorValue und AREAA_ShadeValue...
**
**      [AREAA_Stamp] - (ISG) BOOL *** OBSOLETE ***
**          Dieses Attribut existiert nur noch aus historischen
**          Gründen.
**          Intern wird's so umgebogen:
**              AREAA_Stamp == TRUE:  AREAA_Tracy = TRACY_NONE
**              AREAA_Stamp == FALSE: AREAA_Tracy = TRACY_CLEAR
**
**      [AREAA_Texture] - (ISG) BOOL *** OBSOLETE ***
**          Dieses Attribut existiert nur noch aus historischen
**          Gründen.
**          Intern geht's so:
**              AREAA_Texture == TRUE:  AREAA_Map = MAP_LINEAR
**                                      AREAA_Txt = TXT_MAPPED
**              AREAA_Texture == FALSE: AREAA_Map = MAP_NONE
**                                      AREAA_Txt = TXT_NONE
**          *** Dieses Attribut sollte generell nicht mehr verwendet
**          *** werden, insbesondere nicht in Zusammenhang mit den
**          *** neuen "Polygon-Behaviour-Attributes" [AREAA_Map, etc...]
**
**      AREAA_Blob1 - (S) <leave me alone>
**          Dieses Attribut wird intern vom Save-Dispatcher
**          verwendet und faßt diverse Attribute des Objects
**          in einem ULONG zusammen:
**              Upper 16 Bit    -> verschiedene binäre Flags.
**              Lower 16 Bit    -> reserviert für Polygon-Flags.
**          Dieses Vorgehen spart etliche Tags beim Abspeichern
**          der Attribute des Objects als Attribut-TagList ins
**          Disk-Object.
**          Alle <Blob>-Attribute wirken XOR-ierend auf "die anderen"
**          Attribute, die durch das Blob-Attribut zusammengefaßt
**          wurden. Das spart viele viele GetTagData()'s.
**
**      AREAA_Blob2 - (S) <leave me alone>
**          Siehe auch AREAA_Blob1.
**          AREAA_Blob2 faßt alle "..Value"-Attribute in ein
**          LONG zusammen:
**              AREAA_Blob2 = (AREAA_ColorValue<<16)|
**                            (AREAA_TracyValue<<8)|
**                            (AREAA_ShadeValue<<0);
**
**      AREAA_PolygonInfo   - (G) (struct PolygonInfo *)
**          Dieses Attribut ist nur gettable und returniert einen
**          Pointer auf die eingebettete PolygonInfo-Struktur
**          des area.class Objects. YOU SHOULD KNOW WHAT YOU'RE
**          DOING IF YOU USE THIS ONE!!!!
*/

/*-------------------------------------------------------------------
**  Die Klassen-ID
*/
#define AREA_CLASSID "area.class"

/*-------------------------------------------------------------------
**  Die Locale Instance Data der AreaClass
*/
struct area_data {
    Object *TxtBitmap;      /* AREAA_TxtBitmap */
    Object *TracyBitmap;    /* AREAA_TracyBitmap */

    ULONG Flags;            /* siehe unten */

    UBYTE ColorValue;       /* AREAA_ColorValue */
    UBYTE TracyValue;       /* AREAA_TracyValue */
    UBYTE ShadeValue;       /* AREAA_ShadeValue */
    UBYTE pad;              /* leer... */

    struct PolygonInfo PolyInfo;    /* siehe "polygon/polygon.h" */
};
/*-------------------------------------------------------------------
**  A word about flags:
**  ~~~~~~~~~~~~~~~~~~~
**  Das <Flags>-Feld enthält nur ein paar Area-spezifische
**  Flags, die Polygon-Flags selbst sind in
**  <PolygonInfo.Flags>!!!
*/
#define AREAF_DepthFade     (1<<0)
#define AREAF_BackCheck     (1<<1)

/*-------------------------------------------------------------------
**  Methoden
*/
#define AREAM_BASE          (ADEM_BASE+METHOD_DISTANCE)

/*-------------------------------------------------------------------
**  Attribute
*/
#define AREAA_Base          (ADEA_Base+ATTRIB_DISTANCE)

#define AREAA_TxtBitmap     (AREAA_Base)
#define AREAA_ColorValue    (AREAA_Base+1)
#define AREAA_Obsolete1     (AREAA_Base+2)  // OBSOLETE
#define AREAA_Obsolete2     (AREAA_Base+3)  // OBSOLETE
#define AREAA_Map           (AREAA_Base+4)
#define AREAA_Txt           (AREAA_Base+5)
#define AREAA_Shade         (AREAA_Base+6)
#define AREAA_Tracy         (AREAA_Base+7)
#define AREAA_TracyMode     (AREAA_Base+8)  // OBSOLETE
#define AREAA_TracyBitmap   (AREAA_Base+9)
#define AREAA_ShadeValue    (AREAA_Base+10)
#define AREAA_TracyValue    (AREAA_Base+11)
#define AREAA_Blob1         (AREAA_Base+12)
#define AREAA_Blob2         (AREAA_Base+13)
#define AREAA_PolygonInfo   (AREAA_Base+14)

/*** obsolete ***/
#define AREAA_Bitmap        AREAA_TxtBitmap     /* OBSOLETE */
#define AREAA_BaseColor     AREAA_ColorValue    /* OBSOLETE */
#define AREAA_Stamp         (AREAA_Base+1)      /* Compatibility */
#define AREAA_Texture       (AREAA_Base+2)      /* Compatibility */

/*-------------------------------------------------------------------
**  Konstanten-Definitionen für ein paar Attribute
*/
/*** AREAA_Map ***/
#define MAP_NONE    0
#define MAP_LINEAR  1
#define MAP_DEPTH   2

/*** AREAA_Txt ***/
#define TXT_NONE    0
#define TXT_MAPPED  1

/*** AREAA_Shade ***/
#define SHADE_NONE      0
#define SHADE_FLAT      1
#define SHADE_LINE      2
#define SHADE_GRADIENT  3

/*** AREAA_Tracy ***/
#define TRACY_NONE      0
#define TRACY_CLEAR     1
#define TRACY_FLAT      2
#define TRACY_MAPPED    3

/*** AREAA_TracyMode ***/
#define TRACYMODE_DARKEN    0
#define TRACYMODE_LIGHTEN   1

/*-------------------------------------------------------------------
**  Default-Attribut-Werte
*/
#define AREAA_ColorValue_DEF    1
#define AREAA_ShadeValue_DEF    0
#define AREAA_TracyValue_DEF    0

#define DEF_PolyFlags (PLGF_NONMAPPED|PLGF_NOTEXTURE|PLGF_NOSHADE|PLGF_NOTRACY|PLGF_DARKEN)

/*-------------------------------------------------------------------
**  IFF-Stuff
*/
#define AREAIFF_VERSION     1

#define AREAIFF_FORMID      MAKE_ID('A','R','E','A')
#define AREAIFF_Attributes  MAKE_ID('A','T','T','R') /* OBSOLETE! */
#define AREAIFF_IFFAttr     MAKE_ID('S','T','R','C')
struct area_IFFAttr {
    UWORD version;
    /* AREAIFF_VERSION == 1 */
    ULONG blob1;    /* AREAA_BLOB1 */
    ULONG blob2;    /* AREAA_BLOB2 */
};
/*-----------------------------------------------------------------**
**  Bitmap-Objects werden in folgender Reihenfolge 
**  rausgehauen:
**      TxtBitmap
**      TracyBitmap
**  Dabei können beide, gar keins oder ein beliebiges dieser
**  beiden vorhanden sein. Einfach die Polygon-Flags
**  auswerten, dann bekommt man schon raus, mit welchem
**  Object man es gerade zu tun hat. Das setzt natürlich voraus,
**  daß der OM_SAVETOIFF-Dispatcher die Bitmap-Objects
**  nur einbettet, wenn die zugehörigen Map-Flags auch gesetzt
**  sind.
**
**  Backward Comp:
**      Falls ein Bitmap-Object vorhanden ist, aber
**      keine Mapping-Flags gesetzt sind, heißt das,
**      daß das hier noch ein "altes" Disk-Object ist,
**      in diesem Fall: als TxtBitmap laden, folgende
**      Polygon-Flags "reinodern":
**          (PLGF_DEPTHMAPPED | PLGF_TEXTUREMAPED)
*/
/*-----------------------------------------------------------------*/
#endif

