/*
**  $Source: PRG:VFM/Classes/_WinTimerClass/wint_wbox.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 15:12:10 $
**  $Locker:  $
**  $Author: floh $
**
**  Windows-Gummizelle für winp.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include "stdlib.h"
#include "windows.h"
#include "windowsx.h"

/*** Watcom spezifisch ***/
#ifdef _MSC_VER
__inline unsigned long _tick_div(long l, long h, long f)
{
	_asm
	{
		mov eax, l;
		mov edx, h;
		mov ebx, f;
		div ebx;
	}
}

#else
extern unsigned long _tick_div(long low, long high, long freq);
#pragma aux _tick_div parm [eax][edx][ebx] = \
    "div ebx" \
    value [eax] \
    modify [eax edx] ;
#endif

/*-----------------------------------------------------------------*/
unsigned long wit_GetTickCount(void)
/*
**  FUNCTION
**      Gummizellen-Wrapper für GetTickCount().
**
**  CHANGED
**      21-Nov-96   floh    created
**      17-Feb-97   floh    benutzt optional QueryPerformanceCounter()
*/
{
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
        /*** Performance-Counter existiert, alles klar... ***/
        LARGE_INTEGER cnt;
        QueryPerformanceCounter(&cnt);
        return(_tick_div(cnt.LowPart,cnt.HighPart,(freq.LowPart>>10)));
    } else {
        /*** kein Performance-Counter, Fallback auf GetTickCount() ***/
        return(GetTickCount());
    };
}

