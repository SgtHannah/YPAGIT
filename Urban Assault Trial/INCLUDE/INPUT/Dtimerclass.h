#ifndef INPUT_DTIMERCLASS_H
#define INPUT_DTIMERCLASS_H
/*
**  $Source: PRG:VFM/Include/input/dtimerclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/03/11 15:57:37 $
**  $Locker:  $
**  $Author: floh $
**
**  Time-Source-Klasse für PCs unter DOS.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_ITIMERCLASS_H
#include "input/itimerclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      dtimer.class -- Time-Source-Klasse für DOS-PCs
**
**  FUNCTION
**      Die Klasse schnappt sich den Timer-Interrupt und
**      beschleunigt von 18.2 Hz auf 200 Hz.
**
**  METHODS
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define DTIMER_CLASSID "drivers/input/dtimer.class"
/*-----------------------------------------------------------------*/
#define DTIM_BASE   (ITIM_BASE+METHOD_DISTANCE)
#define DTIA_BASE   (ITIA_BASE+ATTRIB_DISTANCE)
/*-------------------------------------------------------------------
**  LID der dtimer.class
*/
struct dtimer_data {
    ULONG timer_store;
};

/*-----------------------------------------------------------------*/
#endif

