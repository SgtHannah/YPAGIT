#ifndef NUCLEUS_MATH_H
#define NUCLEUS_MATH_H
/*
**  $Source: PRG:VFM/Include/nucleus/math.h,v $
**  $Revision: 38.3 $
**  $Date: 1996/08/14 00:45:32 $
**  $Locker: floh $
**  $Author: floh $
**
**  mathematische Routinen für nucleus (TM)
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef MATH_H
#include <math.h>
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

/*--------------------------------------------------------------------*/

#ifndef NOMATH_FUNCTIONS
static FLOAT NC_ACOS( FLOAT x )
{
    if( x >  1.0 )  x =  1.0;
    if( x < -1.0 )  x = -1.0;
    return( (FLOAT) acos( (double) x ) );
}


static FLOAT NC_ASIN( FLOAT x )
{
    if( x >  1.0 )  x =  1.0;
    if( x < -1.0 )  x = -1.0;
    return( (FLOAT) asin( (double) x ) );
}


static FLOAT NC_SQRT( FLOAT x )
{
    if( x < 0.0 )   x = 0.0;
    return( (FLOAT) sqrt( (double) x ) );
}


static void nc_m_mul_m(struct flt_m3x3 *m1, struct flt_m3x3 *m2, struct flt_m3x3 *m)
/*
**  FUNCTION
**      Multipliziert zwei 3x3-Matrizen.
**
**      m = m1*m2
**
**  CHANGED
**      10-Jun-95   floh    created
*/
{
    m->m11 = m1->m11*m2->m11 + m1->m12*m2->m21 + m1->m13*m2->m31;
    m->m12 = m1->m11*m2->m12 + m1->m12*m2->m22 + m1->m13*m2->m32;
    m->m13 = m1->m11*m2->m13 + m1->m12*m2->m23 + m1->m13*m2->m33;

    m->m21 = m1->m21*m2->m11 + m1->m22*m2->m21 + m1->m23*m2->m31;
    m->m22 = m1->m21*m2->m12 + m1->m22*m2->m22 + m1->m23*m2->m32;
    m->m23 = m1->m21*m2->m13 + m1->m22*m2->m23 + m1->m23*m2->m33;

    m->m31 = m1->m31*m2->m11 + m1->m32*m2->m21 + m1->m33*m2->m31;
    m->m32 = m1->m31*m2->m12 + m1->m32*m2->m22 + m1->m33*m2->m32;
    m->m33 = m1->m31*m2->m13 + m1->m32*m2->m23 + m1->m33*m2->m33;
}


/*
** Die Makros wegen dem FLOAT-double-Casting. Der Einfachheit halber
** nenne ich die Makros wie die Funktionen und die Ur-Fkt. selber groß-
** buchstabig, obwohl das gegen die Konvention
*/

#define nc_sqrt(x)      NC_SQRT( (FLOAT) (x) )
#define nc_asin(x)      NC_ASIN( (FLOAT) (x) )
#define nc_acos(x)      NC_ACOS( (FLOAT) (x) )

#define nc_sgn(x)       ((x<0)?(-1):(1))

#endif

/*----------------------------------------------------------------------*/
#ifdef _LittleEndian_
/* use IEEE conversion trick, little-endian - normalize and extract integer from mantissa */
static double __tempDouble;
#define FLOAT_TO_INT(f) (__tempDouble = (f) + 6755399441055744.0, *(long *)&__tempDouble)
#else
/* use IEEE conversion trick, big-endian - normalize and extract integer from mantissa */
static double __tempDouble;
#define FLOAT_TO_INT(f) (__tempDouble = (f) + 6755399441055744.0, *(((long *)&__tempDouble)+1))
#endif

/*----------------------------------------------------------------------*/
#endif

