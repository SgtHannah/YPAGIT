#ifndef SKELETON_SKLT_H
#define SKELETON_SKLT_H
/*
**  $Source: PRG:VFM/Include/skeleton/sklt.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/11/10 20:52:49 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition der grundlegenden 3D-Form-Definitions-Struktur
**  >Skeleton<.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

/*-------------------------------------------------------------------
**  Jedes Skeleton (siehe unten) kann einen optionalen Plane-
**  Parameter-Pool besitzen. Dort steht für jede Area des Skeletons
**  deren Ebenen-Parameter (0 = Ax + By + Cz + D), also A,B,C und D.
*/
struct Plane {
    FLOAT A,B,C,D;
};

/*-------------------------------------------------------------------
**  Die Skeleton-Struktur beinhaltet einen 3D-Pool, die zugehörige
**  Bounding-Box, die Flächen-Definition und ein paar
**  Verwaltungs-Werte.
**
**  Der "Area-Pointer-Pointer" bedarf einer näheren Erklärung.
**  Alle Oberflächen sind im Area-Array als Pointer auf eine
**  Polygon-Definition vertreten. Die Polygon-Definition besteht
**  aus WORDs, die jeweils die  Nummer des jeweiligen Eckpunktes im
**  Pool definieren. Eine Polygon-Definition ist durch ein -1.w
**  begrenzt. Der Polygon muß im Uhrzeigersinn von der sichtbaren
**  Seite aus definiert sein.
**  Eine Fläche wird von außen angesprochen durch die Position
**  ihres Polygon-Pointers im Area-Array.
**
**  News 16-Jan-96:
**  ~~~~~~~~~~~~~~~
**  Mit der neuen skeleton.class wurden viele Aufgaben der
**  Transformer-Engine in die skeleton.class verlagert.
**  Gleichzeitig haben sich ein paar interne Formate geändert:
**
**      sklt->Pool und sklt->SensorPool werden NICHT MEHR DURCH
**      EIN ENDE-ELEMENT BEGRENZT!!! Die Anzahl der Punkte in
**      den Pools definieren ausschließlich NumPoolPoints und
**      NumSensorPoints!
**
**      Eine Area (Polygon) wird nicht mehr durch ein Ende-Element
**      begrenzt, sondern die erste Nummer der Area gibt an,
**      wieviele Punkte der Polygon besitzt.
*/
struct Skeleton {
    fp3d *Pool;             // fp3d-Koordinaten-Pool
    UWORD NumPoolPoints;    // Anz.Koords. in >pool< einschl. Ende-Element
    UWORD NumSensorPoints;  // <= NumPoolPoints!!!!!
    WORD **Areas;
    ULONG NumAreas;
    fp3d *SensorPool;       // optionaler Pool von Sensor-Koordinaten */
    FLOAT Radius;           // größte Distanz in <Pool>
    struct Plane *PlanePool;
    fp3d *TransformPool;        // Zielbuffer fuer Koord-Transformation
};

/*-------------------------------------------------------------------
**  IFF-Definitionen für das SKLT-Format.
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**  Die oberste Form-ID ist ein MC2, danach kommt erst die
**  SKLT-Form.
**
**  Der TYPE-Chunk gibt das genaue Format des POOL-Chunks
**  an.
**
**  TYPE_1 -> die Koordinaten sind als WORD im Pool abgelegt.
**
**  Der POOL-Chunk enthält die eigentlichen 3D-Koordinaten, das
**  genaue Format des POOL-Chunks ist im TYPE-Chunk festgelegt
**
**  Der POLY-Chunk enthält die Flächen-Definitionen. Der Chunk besteht
**  aus WORDs, die den Punktnummern der Polygon-Eckpunkte im POOL-Chunk
**  entspricht. Eine Polygon-Definition ist mit -1.w abgeschlossen,
**  Darauf folgt die nächste Flächen-Definition, bis der Chunk
**  alle ist.
**
**  Der SENSOR-Chunk liegt im selben Format vor, wie der
**  POOL-Chunk.
**
**  SKLTIFF_NEWPOOL:
**      FLOAT x,y,z;    aufeinanderfolgend
**
**  SKLTIFF_NEWSENSOR   genauso
**
**  SKLTIFF_NEWPOLY:
**      ULONG num_polys;
**      for each poly:
**          UWORD num_points
**          UWORD p0,p1,p2...
**          end
*/
#define SKLTIFF_FORMID          MAKE_ID('S','K','L','T')
#define SKLTIFF_TYPE            MAKE_ID('T','Y','P','E')    // OBSOLETE
#define SKLTIFF_POOL            MAKE_ID('P','O','O','L')    // OBSOLETE
#define SKLTIFF_POLY            MAKE_ID('P','O','L','Y')    // OBSOLETE
#define SKLTIFF_SENSOR          MAKE_ID('S','E','N','S')    // ONSOLETE
#define SKLTIFF_PLANE           MAKE_ID('P','L','A','N')    // OBSOLETE

#define SKLTIFF_NEWPOOL         MAKE_ID('P','O','O','2')
#define SKLTIFF_NEWPOLY         MAKE_ID('P','O','L','2')
#define SKLTIFF_NEWSENSOR       MAKE_ID('S','E','N','2')

/*-------------------------------------------------------------------
**  Die Pool-Type-Definitionen:
*/
#define SKLT_PoolType1  (1)
struct SKLT_PoolType1Element {
    WORD x,y,z;
};

struct sklt_NewPoolPoint {
    FLOAT x,y,z;
};

/*-----------------------------------------------------------------*/
#endif


