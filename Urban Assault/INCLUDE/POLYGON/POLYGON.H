#ifndef POLYGON_POLYGON_H
#define POLYGON_POLYGON_H
/*
**  $Source: PRG:VFM/Include/polygon/polygon.h,v $
**  $Revision: 38.9 $
**  $Date: 1996/11/10 21:16:27 $
**  $Locker:  $
**  $Author: floh $
**
**  Allgemeine Polygon-Definitionen.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef SKELETON_SKLT_H
#include "skeleton/sklt.h"
#endif

struct PolygonInfo {
    struct Skeleton *Sklt;  // gültige Skeleton-Struktur
    UWORD PolyNum;          // ...in Sklt's AreaPool
    UWORD Flags;            // Polygonflags
    fp3d *Pool;             // Pointer auf (Viewer3D)Pool
    struct VFMOutline *Outline; // optionale Outline, wird gemerged mit Polygon
    Point2D *Polygon;           // zeigt auf kompletten Polygon
    LONG *DistPool;             // Pool mit "wahren" Punkt-Entfernungen
};

/*-----------------------------------------------------------------*/
#endif


