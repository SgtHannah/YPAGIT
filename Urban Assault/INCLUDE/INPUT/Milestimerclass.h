#ifndef INPUT_MILESTIMERCLASS_H
#define INPUT_MILESTIMERCLASS_H
/*
**  $Source: PRG:VFM/Include/input/milestimerclass.h,v $
**  $Revision: 38.3 $
**  $Date: 1996/11/21 01:42:43 $
**  $Locker:  $
**  $Author: floh $
**
**  TimeSource-Klasse für Benutzung mit der Miles-AIL. Weil
**  die Miles-Libraries ihr eigenes Timing haben (und noch
**  dazu Timed-Callback-Service) anbieten, muß/darf ich
**  nicht auch noch an der Timer-Hardware rummachen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_ITIMERCLASS_H
#include "input/itimerclass.h"
#endif

#include "thirdparty/mss.h"

/*-------------------------------------------------------------------
**  NAME
**      milestimer.class -- Miles-AIL kompatible TimeSource-Klasse für
**                          DOS.
**
**  FUNCTION
**      Benutzt die "Process Service Functions" des Miles-Treiber-
**      Systems für Timing-Information. Eine bereits initialisierte
**      miles.engine ist Voraussetzung!
**
**  METHODS
**
**  ATTRIBUTES
*/
/*-----------------------------------------------------------------*/
#define MLSTIMER_CLASSID "drivers/input/milestimer.class"
/*-----------------------------------------------------------------*/
#define MTIM_BASE   (ITIM_BASE+METHOD_DISTANCE)
#define MTIA_BASE   (ITIM_BASE+METHOD_DISTANCE)
/*-----------------------------------------------------------------*/
struct milestimer_data {
    ULONG timer_store;
};

/*-----------------------------------------------------------------*/
#endif

