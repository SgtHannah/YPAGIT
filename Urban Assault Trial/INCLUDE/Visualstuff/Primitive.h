#ifndef VISUALSTUFF_PRIMITIVE_H
#define VISUALSTUFF_PRIMITIVE_H
/*
**  $Source: PRG:VFM/Include/visualstuff/primitive.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/05/24 14:19:56 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Lowlevel-Gfx-Primitives.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-----------------------------------------------------------------*/
struct VFMOutline {
    FLOAT x;        /* 0.0 <= x < 1.0 */
    FLOAT y;        /* 0.0 <= y < 1.0 */
};

struct VFMRect {
    LONG xmin,ymin;
    LONG xmax,ymax;
};

/*-----------------------------------------------------------------*/
#endif
