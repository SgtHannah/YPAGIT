#ifndef INPUT_WINTIMERCLASS_H
#define INPUT_WINTIMERCLASS_H
/*
**  $Source: PRG:VFM/Include/input/wintimerclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/11/21 01:41:11 $
**  $Locker:  $
**  $Author: floh $
**
**  Timer-Klasse unter Windows.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef INPUT_ITIMERCLASS_H
#include "input/itimerclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      wintimer.class -- Time-Source-Klasse unter Windows.
**
**  FUNCTION
**      Die Klasse benutzt GetTickCount(). Falls der Zeit-
**      unterschied zum letzten Aufruf >2000 millisec
**      ist, wird ein Tickcount von 1 zurückgegeben (das
**      ist die einfachste Art und Weise mit "inaktiven"
**      Perioden umzugehen.
**
**  METHODS
**
**  ATTRIBUTES
*/

/*-----------------------------------------------------------------*/
#define WINTIMER_CLASSID "drivers/input/wintimer.class"
/*-----------------------------------------------------------------*/
#define WINTIM_BASE     (ITIM_BASE+METHOD_DISTANCE)

/*-----------------------------------------------------------------*/
struct wintimer_data {
    ULONG old_tickcount;
};

/*-----------------------------------------------------------------*/
#endif




