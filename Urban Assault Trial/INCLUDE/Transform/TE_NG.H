#ifndef TRANSFORM_TE_NG_H
#define TRANSFORM_TE_NG_H
/*
**  $Source: PRG:VFM/Include/transform/te_ng.h,v $
**  $Revision: 38.3 $
**  $Date: 1995/01/02 23:48:07 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für tform_NG.engine
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef MATH_H
#include <math.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef TRANSFORM_TE_H
#include "transform/te.h"
#endif

/*-------------------------------------------------------------------
**  Einige C-Compiler kenne M_PI nicht (ich will ja keine
**  Namen nennen...)!
*/
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

/*-------------------------------------------------------------------
**  die Struktur eines Elements in der Sincos_Table.
*/
struct Sincos_atom {
    FLOAT sinus;
    FLOAT cosinus;
};

/*-------------------------------------------------------------------
**  Die Variablen-Basis der tform_NG.engine
*/
struct TE_Base {
    FLOAT ZoomX, ZoomY;
    FLOAT MaxZ;
    FLOAT MinZ;
    tform *Viewer;
    struct Sincos_atom *Sincos_Table;
};

/*-----------------------------------------------------------------*/
#endif

