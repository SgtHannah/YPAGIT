#ifndef POLYGON_POLYGONFLAGS_H
#define POLYGON_POLYGONFLAGS_H
/*
**  $Source: PRG:VFM/Include/polygon/polygonflags.h,v $
**  $Revision: 38.1 $
**  $Date: 1994/10/19 00:50:40 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition der Polygon-Flags.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include "exec/types.h"
#endif

/*-------------------------------------------------------------------
**  Die Polygonflags definieren das Aussehen des Polygons,
**  sprich, welche Technik zur Visualiserung benutzt
**  werden soll [DrawPolygon()].
*/
#define PLGF_SCANLINE   (1<<0)  /* 1->horizontal, 0->vertical */
#define PLGF_MAPBIT1    (1<<1)  /* map bits: 00 -> non mapped */
#define PLGF_MAPBIT2    (1<<2)  /* map bits: 01 -> linear mapped */
                                /* map bits: 11 -> depth mapped */

#define PLGF_TXTBIT1    (1<<3)  /* 1->texture mapped, 0->not texture mapped*/

#define PLGF_SHADEBIT1  (1<<4)  /* shade bits: 00 -> not shaded  */
#define PLGF_SHADEBIT2  (1<<5)  /* shade bits: 01 -> flat shaded */
                                /* shade bits: 10 -> line shaded */
                                /* shade bits: 11 -> gradient shaded */

#define PLGF_TRACYBIT1  (1<<6)  /* tracy bits: 00 -> no transparency */
#define PLGF_TRACYBIT2  (1<<7)  /* tracy bits: 01 -> clear transparency */
                                /* tracy bits: 10 -> flat transparency */
                                /* tracy bits: 11 -> mapped transparency */
#define PLGF_TRACYBIT3  (1<<8)  /* 1->lighten 0->darken */

/*-------------------------------------------------------------------
**  Folgende Bitkombinationen sollten bevorzugt benutzt werden
**  (anstatt der oberen "rohen" Bits).
**  Sie können durch einfaches Zusammen-ODERn kombiniert werden
*/
#define PLGF_HORIZONTAL     (PLGF_SCANLINE)
#define PLGF_VERTICAL       (0)

#define PLGF_NONMAPPED      (0)
#define PLGF_LINEARMAPPED   (PLGF_MAPBIT1)
#define PLGF_DEPTHMAPPED    (PLGF_MAPBIT1|PLGF_MAPBIT2)

#define PLGF_NOTEXTURE      (0)
#define PLGF_TEXTUREMAPPED  (PLGF_TXTBIT1)

#define PLGF_NOSHADE        (0)
#define PLGF_FLATSHADE      (PLGF_SHADEBIT1)
#define PLGF_LINESHADE      (PLGF_SHADEBIT2)
#define PLGF_GRADSHADE      (PLGF_SHADEBIT1|PLGF_SHADEBIT2)

#define PLGF_NOTRACY        (0)
#define PLGF_CLEARTRACY     (PLGF_TRACYBIT1)
#define PLGF_FLATTRACY      (PLGF_TRACYBIT2)
#define PLGF_TRACYMAPPED    (PLGF_TRACYBIT1|PLGF_TRACYBIT2)

#define PLGF_DARKEN         (0)
#define PLGF_LIGHTEN        (PLGF_TRACYBIT3)

/*-----------------------------------------------------------------*/
#endif



