#ifndef INPUT_AMITIMERCLASS_H
#define INPUT_AMITIMERCLASS_H
/*
**  $Source: PRG:VFM/Include/input/amitimerclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1996/03/11 12:28:54 $
**  $Locker:  $
**  $Author: floh $
**
**  ACHTUNG, die amitimer.class ist Amiga-spezifisch!
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DEVICES_TIMER_H
#include "devices/timer.h"
#endif

#ifndef INPUT_ITIMERCLASS_H
#include "input/itimerclass.h"
#endif


/*-------------------------------------------------------------------
**  NAME
**      amitimer.class -- Time-Source-Klasse für den Amiga.
**
**  FUNCTION
**      Die Klasse benutzt die EClock des timer.device.
**
**  METHODS
**      siehe "input/itimerclass.h"
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define AMITIMER_CLASSID "drivers/input/amitimer.class"
/*-----------------------------------------------------------------*/
#define AMITIM_BASE     (ITIM_BASE+METHOD_DISTANCE)

/*-------------------------------------------------------------------
**  LID der amitimer.class
*/
struct amitimer_data {
    struct EClockVal NewEClock;
    struct EClockVal OldEClock;
};

/*-----------------------------------------------------------------*/
#endif
