#ifndef VISUALSTUFF_CHUNKY8_H
#define VISUALSTUFF_CHUNKY8_H
/*
**  $Source: PRG:VFM/Include/visualstuff/chunky8.h,v $
**  $Revision: 38.1 $
**  $Date: 1995/05/31 00:10:31 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Chunky8-Grafik-Routinen.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Struktur zum Aufbau einer Span-Table. Für jede Scan-Line eines
**  Polygons wird eine Start- und eine End-Spantable gebaut.
*/
struct Span {
    UWORD x,y;          /* Display-Position */
    ULONG color;        /* Farbwert */
    ULONG mapx, mapy;   /* Mapping-Koordinaten, Linear- und Depth-Mapping */
    ULONG actz;         /* Depth-Mapping: 1/z0 -> 1/z1 */
};

/*-----------------------------------------------------------------*/
#endif




