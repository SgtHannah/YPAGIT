#ifndef SKELETON_SKELETONCLASS_H
#define SKELETON_SKELETONCLASS_H
/*
**  $Source: PRG:VFM/Include/skeleton/skeletonclass.h,v $
**  $Revision: 38.8 $
**  $Date: 1996/11/10 20:54:14 $
**  $Locker:  $
**  $Author: floh $
**
**  Die skeleton.class steht auf derselben Ebene wie die
**  bitmap.class, allerdings stellt sie statt Bitmaps
**  3D-Skeletons bereit (Punkt-Pools zu Polygonen verbunden).
**
**  (C) Copyright 1994,1995,1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef NUCLEUS_NUCLEUSCLASS_H
#include "nucleus/nucleusclass.h"
#endif

#ifndef SKELETON_SKLTN_H
#include "skeleton/sklt.h"
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"     // für VFMOutline Def
#endif

#ifndef RESOURCES_RSRCCLASS_H
#include "resources/rsrcclass.h"
#endif

#ifndef BITMAP_RASTERCLASS_H
#include "bitmap/rasterclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      skeleton.class -- Stammklasse für alle Skeleton-Anbieter
**
**  FUNCTION
**      Die skeleton.class ist funktionell ähnlich der bitmap.class.
**      Die dort gegebenen Informationen können auch auf die
**      skeleton.class angewendet werden.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OM_NEW
**      OM_GET
**
**      rsrc.class
**      ~~~~~~~~~~
**      RSM_CREATE
**
**          RSM_CREATE ist in der Lage, eine Skeleton-Struktur
**          "vorzuallokieren". Dazu muß mindestens das Attribut
**          SKLA_NumPoints definiert sein. Allokiert werden
**          nur die Elemente, für die auch Informationen per
**          Attribut vorhanden sind:
**
**              SKLA_NumPoints          - Skeleton mit Point-Pool
**              SKLA_NumSensorPoints    - zusätzlich Sensor-Pool
**              SKLA_NumPolygons        - Anzahl Polygone, nur in
**                                        Zusammenhang mit 
**                                        SKLA_NumPolyPoints gültig!
**              SKLA_DoPlanes           - kann zusätzlich zu
**                                        SKLA_NumPolygons und
**                                        SKLA_NumPolyPoints angegeben
**                                        werden, wenn ein Bereich für
**                                        die Ebenen-Parameter reserviert
**                                        werden soll.
**
**      RSM_FREE
**
**      skeleton.class
**      ~~~~~~~~~~~~~~
**      SKLM_GETSKELETON
**          Msg:    ---
**          Ret:    struct Skeleton *
**
**          Returniert einen Pointer auf die eingebettete
**          Skeleton-Struktur.
**
**      SKLM_LOCAL2VWR
**          Msg:    struct local2vwr_msg
**          Ret:    BOOL vis_code (TRUE -> Pool sichtbar)
**
**          Transformiert die lokalen 3D-Punkte in sklt->Pool
**          ins Viewer-Koordinaten-System, wobei die
**          resultierenden Koordinaten im TransformPool
**          abgelegt werden. Es wird abgebrochen, sobald
**          feststeht, daß das Objekt nicht im Viewer-Bereich
**          sein wird. Auf Wunsch wird eine Voruntersuchung
**          an den Sensor-Points gemacht.
**
**      SKLM_EXTRACTPOLY
**          Msg:    struct sklt_extract_msg
**          Ret:    <void *>
**
**          Extrahiert, clippt, beleuchtet und projiziert
**          einen Polygon des Skeleton. Die Methode bricht
**          ab, sobald feststeht, daß der Polygon unsichtbar
**          ist.
**
**          Die Puffer der rast_poly-Struktur müssen mindestens 2x
**          NumPoints des Polys halten können.
**
**          poly->xyz muß mit einem Pointer auf einen (großen!)
**          Buffer ausgestattet sein, poly->uv und poly->b
**          werden dann mit in diesem einen Buffer reingebogen!
**
**          Der konkrete Output ist:
**              poly->xyz
**              poly->uv  [falls SKLF_UV gesetzt]
**              poly->b   [falls SKLF_B gesetzt]
**
**          Wenn SKLF_DFADE gesetzt ist, wird beim Lighting die
**          Entfernung zum Viewer beachtet.
**
**          Zurückgegeben wird ein Pointer auf die erste freie
**          Adresse im [x,y,z][u,v][b] Puffer, oder NULL,
**          wenn der Polygon unsichtbar ist.
**
**      SKLM_CREATE_SENSORPOOL  (INTERNAL USE ONLY!!!)
**          msg:    struct create_sensorpool_msg;
**          ret:    BOOL
**
**          Allokiert den Sensor-Pool für das eingebettete Skeleton.
**          Diese Funktion darf NUR innerhalb von RSM_CREATE angewendet
**          werden!!!
**
**      SKLM_CREATE_POLYGONS    (INTERNAL USE ONLY!!!)
**          msg:    struct create_polygons_msg;
**          ret.    BOOL
**
**          Allokiert Polygon-Strukturen für das eingebettete Skeleton.
**          Diese Funktion darf NUR innerhalb von RSM_CREATE angewendet
**          werden!!!
**
**      SKLM_REFRESHPLANE
**          msg:    struct refreshplane_msg
**          ret:    ---
**
**          Berechnet die Ebenen-Parameter A,B,C,D des angegebenen
**          Polygons neu. A,B,C entsprechen dabei dem normalisierten
**          Normalen-Vektor des Polygons (allerdings im Model-Space).
**
**  ATTRIBUTES
**  ~~~~~~~~~~
**      SKLA_Skeleton (G) - <struct Skeleton *>
**          Pointer auf <struct Skeleton>, die vom Skeleton-Object
**          als Resource verwaltet wird.
**
**      SKLA_NumPoints (IG) - ULONG
**          Anzahl der Punkte, die im 3D-Pool definiert sind.
**          Die Größe des Pools in Byte bekommt man durch Multiplikation 
**          mit sizeof(Pixel3D). Dieses Attribut existiert für den Fall, 
**          wenn ein Client-Object Puffer-Speicher für die 
**          Koordinaten-Transformation anlegen möchte, die den 3D-Pool 
**          des Skeleton-Objects als Quelle haben.
**
**          Automatisch zum Speicherbereich für die 3D-Punkte wird
**          ein gleichgroßer Bereich als Ziel-Buffer für die
**          transformierten Viewer-3D-Koordinaten angelegt.
**
**      SKLA_NumSensorPoints (IG) - ULONG
**          Wie SKLA_NumPoints, nur für die optionalen
**          Sensor-Punkte. ***WICHTIG*** die Anzahl der
**          Sensor-Punkte *MUSS* kleiner oder gleich SKLA_NumPoints
**          sein!!!
**
**      SKLA_NumPolygons (IG) - ULONG
**          Anzahl der Polygone im Skeleton, falls (I) MUSS
**          zusätzlich ein SKLA_NumPolyPoints angegeben werden!
**
**      SKLA_NumPolyPoints (I) - ULONG
**          Gesamt-Anzahl aller Eck-Punkte aller Polygone. Nur
**          wichtig fuer Initialisierung.
**
**-----------------------------------------------------------------*/
#define SKLM_BASE               (RSM_BASE+METHOD_DISTANCE)

#define SKLM_GETSKELETON        (SKLM_BASE)
#define SKLM_CREATE_SENSORPOOL  (SKLM_BASE+1)
#define SKLM_CREATE_POLYGONS    (SKLM_BASE+2)
#define SKLM_REFRESHPLANE       (SKLM_BASE+3)
#define SKLM_LOCAL2VWR          (SKLM_BASE+4)
#define SKLM_EXTRACTPOLY        (SKLM_BASE+5)
/*-----------------------------------------------------------------*/
#define SKLA_BASE               (RSA_BASE+ATTRIB_DISTANCE)

#define SKLA_Skeleton           (SKLA_BASE)     // (G)
#define SKLA_NumPoints          (SKLA_BASE+1)   // (IG)
#define SKLA_NumSensorPoints    (SKLA_BASE+2)   // (IG)
#define SKLA_NumPolygons        (SKLA_BASE+3)   // (IG)
#define SKLA_NumPolyPoints      (SKLA_BASE+4)   // (I)
/*-------------------------------------------------------------------
**  ClassID der skeleton.class
*/
#define SKELETON_CLASSID "skeleton.class"

/*-----------------------------------------------------------------*/
struct skeleton_data {
    struct Skeleton *skeleton;
};

/*-----------------------------------------------------------------*/
struct local2vwr_msg {
    tform *local;           // TForm des Objects
    tform *viewer;          // TForm des Viewers
    FLOAT min_z;            // Entfernung der Clipping-Frontplane
    FLOAT max_z;            // Entfernung der Clipping-Backplane
};

struct create_sensorpool_msg {
    struct Skeleton *sklt;
    LONG num_sensorpnts;
};

struct create_polygons_msg {
    struct Skeleton *sklt;
    LONG num_polys;
    LONG num_polypnts;
};

struct refreshplane_msg {
    ULONG pnum;
};

struct sklt_extract_msg {
    ULONG pnum;             // Nummer des Polygons
    ULONG flags;            // siehe unten
    struct rast_poly *poly; // [x,y,z],[u,v],[b] werden gefüllt!
    struct VFMOutline *uv;  // Source-Outline (NULL, falls keine vorhanden)
    FLOAT min_z;            // Entfernung der Front-Clipping-Plane
    FLOAT max_z;            // Entfernung der Back-Clipping-Plane
    FLOAT shade;            // Konstanter Shade-Wert, [0.0 .. 1.0]
    FLOAT dfade_start;
    FLOAT dfade_len;
};
#define SKLF_UV     (1<<0)  // behandle [u,v] (msg->poly->uv gültig!)
#define SKLF_BRIGHT (1<<1)  // behandle [b] (msg->poly->b gültig!)
#define SKLF_DFADE  (1<<2)  // apply depth fading

struct sklt_clip {
    fp3d *s_poly;
    fp3d *t_poly;
    struct VFMOutline *s_uv;
    struct VFMOutline *t_uv;
    ULONG code;
    FLOAT min_z;
    FLOAT max_z;
};

/*-----------------------------------------------------------------*/
#endif


