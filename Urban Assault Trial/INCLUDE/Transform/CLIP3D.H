#ifndef TRANSFORM_CLIP3D_H
#define TRANSFORM_CLIP3D_H
/*
**  $Source: PRG:VFM/Include/transform/clip3d.h,v $
**  $Revision: 38.1 $
**  $Date: 1994/01/24 17:25:53 $
**  $Locker:  $
**  $Author: floh $
**
**  3D-Clipper-Definitionen
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  3D-Clip-Extra-Flags, die angeben, welche Extra-Clip-Funktionen
**  von den 3D-Clip-Funktionen ausgeführt werden sollen.
**
**  Definiert sind die Flags und deren Bitnummern.
*/
#define CLIPEXTRAB_XY		0
#define CLIPEXTRAB_BRIGHTNESS	1
#define CLIPEXTRAB_TRUECOLOR	2

#define CLIPEXTRAF_XY		(1<<CLIPEXTRAB_XY)
#define CLIPEXTRAF_BRIGHTNESS	(1<<CLIPEXTRAB_BRIGHTNESS)
#define CLIPEXTRAF_TRUECOLOR	(1<<CLIPEXTRAB_TRUECOLOR)

/*-----------------------------------------------------------------*/
#endif

