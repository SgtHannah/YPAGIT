#ifndef ADE_AMESHCLASS_H
#define ADE_AMESHCLASS_H
/*
**  $Source: PRG:VFM/Include/ade/ameshclass.h,v $
**  $Revision: 38.5 $
**  $Date: 1996/01/20 17:13:52 $
**  $Locker:  $
**  $Author: floh $
**
**  Die amesh.class ist Subklasse der area.class und faßt
**  eine Anzahl Flächen in ein ADE zusammen. Bedingung ist,
**  daß bestimmte Attribute der Einzelflächen identisch sind.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef ADE_AREACLASS_H
#include "ade/areaclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      amesh.class - ganze "Area Meshes" in einem ADE
**
**  FUNCTION
**      Die amesh.class bietet die Funktionalität der area.class
**      für eine beliebig große Anzahl von Einzelflächen. Die Flächen
**      müssen allerdings in bestimmten Attributen übereinstimmen,
**      nämlich in folgenden:
**
**          ADEA_DepthFade
**
**          AREAA_Map
**          AREAA_Txt
**          AREAA_Shade
**          AREAA_Tracy
**          AREAA_TracyMode
**          AREAA_TxtBitmap   -> identische Klasse, identischer ILBM-File
**          AREAA_TracyBitmap -> identische Klasse, identischer ILBM-File
**
**      Alle anderen Attribute, die ein area.class Object definieren,
**      dürfen für die einzelnen Flächen verschieden sein.
**
**      Hauptziel der amesh.class ist die Einsparung von Speicherplatz
**      auf Disk und im Speicher, außerdem werden die Ladezeiten
**      drastisch reduziert, die Darstellung der einzelnen Flächen
**      ist geringfügig schneller, als bei einzelnen ADEs.
**
**  METHODS
**      keine eigenen Methoden
**
**  ATTRIBUTES
**      AMESHA_NumPolygons  - (IG) (LONG) [default = NONE]
**          Definiert die Anzahl der Einzel-Polygone für dieses
**          amesh.class Object.
**
**      AMESHA_AttrsPool    - (IS) (struct amesh_attrs *) [default = NONE]
**          Pointer auf Array mit Strukturen, in denen für jeden
**          Polygon folgende Attribute definiert sind:
**
**              ADEA_Polygon        (WORD)
**              AREAA_ColorValue    (UBYTE)
**              AREAA_ShadeValue    (UBYTE)
**              AREAA_TracyValue    (UBYTE)
**
**          Die Größe des Arrays ergibt sich aus AMESHA_NumPolygons.
**          Der Pool wird in einen eigenen Speicherblock kopiert.
**
**      AMESHA_OutlinePool  - (IS) (struct VFMOutline **) [default = NULL]
**          Pointer auf optionalen Outline-Pool. Dieser Pool muß
**          nur vorhanden sein, falls die Einzelpolygone
**          gemappt sind. Der Pool ist ein Array aus VFMOutline-Pointern,
**          die jeweils auf eine vollständige Outline zeigen.
**          Die Größe des Arrays ergibt sich aus AMESHA_NumPolygons.
**
**          Der Pool wird zusammen mit allen Outlines in einen eigenen
**          Speicherblock kopiert.
*/

/*-------------------------------------------------------------------
**  Class ID der amesh.class
*/
#define AMESH_CLASSID "amesh.class"

/*-------------------------------------------------------------------
**  Die Locale Instance Data der amesh.class
*/
struct amesh_data {
    Object *TxtBitmap;      /* gefiltert aus AREAA_TxtBitmap */
    Object *TracyBitmap;    /* gefiltert aus AREAA_TracyBitmap */

    UWORD NumPolygons;      /* == AMESHA_NumPolygons */
    UWORD Flags;            /* AMESHF_DepthFade */

    struct amesh_attrs *AttrsPool;    /* == AMESHA_AttrsPool   */
    struct VFMOutline **OutlinePool;  /* == AMESHA_OutlinePool */
    struct PolygonInfo *PolyInfo;     /* Ptr in LID der area.class */
};

/*-------------------------------------------------------------------
**  Methoden IDs
*/
#define AMESHM_BASE         (AREAM_BASE+METHOD_DISTANCE)

/*-------------------------------------------------------------------
**  Attribut IDs
*/
#define AMESHA_Base         (AREAA_Base+ATTRIB_DISTANCE)

#define AMESHA_NumPolygons  (AMESHA_Base)
#define AMESHA_AttrsPool    (AMESHA_Base+1)
#define AMESHA_OutlinePool  (AMESHA_Base+2)

/*-------------------------------------------------------------------
**  Defs für <amesh_data.Flags>
*/
#define AMESHF_DepthFade    (1<<0)

/*-------------------------------------------------------------------
**  <amesh_attrs> Struktur, faßt numerische Attribute für jeweils
**  einen Polygon zusammen.
*/
struct amesh_attrs {
    WORD poly_num;      /* ADEA_Polygon */
    UBYTE color_val;    /* AREAA_ColorValue */
    UBYTE shade_val;    /* AREAA_ShadeValue */
    UBYTE tracy_val;    /* AREAA_TracyValue */
    UBYTE pad;          /* Pad-Byte */
};

/*-------------------------------------------------------------------
**  IFF-Definitionen
*/
#define AMESHIFF_FORMID     MAKE_ID('A','M','S','H')
#define AMESHIFF_Attrs      MAKE_ID('A','T','T','S')
/*
**  AMESHIFF_Attrs besteht aus lauter <struct amesh_attrs>,
**  für jeden Einzelpolygon eine Struktur. 
**  Dieser Chunk ist IMMER vorhanden.
**  AMESHA_NumPolygons ergibt sich aus der Chunk-Größe dividiert
**  durch sizeof(struct amesh_attrs).
*/

#define AMESHIFF_Outlines   MAKE_ID('O','L','P','L')
/*
**  Chunk-Aufbau:
**  Outline[0] {
**      WORD num_elements
**      UBYTE x,y
**      UBYTE x,y
**      [...]
**  }
**  Outline[1] {
**      WORD num_elements
**      BYTE x,y
**      [...]
**  }
**  [...]
**
**  Es sind so viele Outlines im Chunk, wie AMESHA_NumPolygons.
**  Der Outline-Chunk ist optional. Wenn vorhanden, folgt er immer
**  HINTER dem AMESHIFF_Attrs Chunk.
*/
struct amesh_ol_atom {
    UBYTE x,y;
};

/*-----------------------------------------------------------------*/
#endif



