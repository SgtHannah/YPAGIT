/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_profile.c,v $
**  $Revision: 38.1 $
**  $Date: 1997/10/25 16:39:50 $
**  $Locker:  $
**  $Author: floh $
**
**  yw_profile.c -- Profiling-Funktionen...
**                  Windows-Gummizelle!
**
**  (C) Copyright 1997 by A.Weissflog
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
unsigned long yw_GetTickCount(void)
/*
**  FUNCTION
**      Gummizellen-Wrapper für GetTickCount().
**      Genauigkeit ist 1/10 Millisekunde.
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
        return(_tick_div(cnt.LowPart,cnt.HighPart,(freq.LowPart/10000)));
    } else return(0);
}

/*-----------------------------------------------------------------*/
unsigned long yw_StartProfile(void)
/*
**  FUNCTION
**      Startet einen Profile-Vorgang...
**
**  INPUTS
**      large_int - zeigt auf ULONG Array mit 2 Einträgen,
**                  large_int[0] -> Low Part
**                  large_int[1] -> Hi Part
**
**  CHANGED
**      04-Jul-97   floh    created
*/
{
    return(yw_GetTickCount());
}

/*-----------------------------------------------------------------*/
unsigned long yw_EndProfile(unsigned long start_value)
/*
**  FUNCTION
**      Beendet einen Profile-Vorgang... zurückgegeben wird die
**      Differenz zum Start-Wert in 1/10 Millisekunden.
**
**  CHANGED
**      04-Jul-97   floh    created
*/
{
    return(yw_GetTickCount() - start_value);
}

