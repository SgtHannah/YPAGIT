#ifndef MISC_DSHOW_H
#define MISC_DSHOW_H
/*
**  $Source:$
**  $Revision:$
**  $Date:$
**  $Locker:$
**  $Author:$
**
**      misc/dshow.h -- DirectShow related stuff, nach dem cplay.c
**                                              DirectShow Beispiel       
**
**  (C) Copyright 1997 by A.Weissflog
*/
#ifdef DSHOW_PADDEDCELL
        #ifndef _INC_WINDOWS
        #include <windows.h>
        #endif

        #ifndef _INC_WINDOWSX
        #include <windowsx.h>
        #endif

        #ifndef _MSC_VER
        #define IErrorLog void
        #define BEGIN_INTERFACE
        #define END_INTERFACE
        #endif
     
        #ifndef __strmif_h__
        #include <strmif.h>
        #endif

        #include <evcode.h>
        #include <uuids.h>
        #include <control.h>
#endif
/*-----------------------------------------------------------------*/
unsigned long dshow_PlayMovie(char *, HWND);
void dshow_StopMovie(void);

#define WM_MEDIAEVENT (WM_USER+46)
/*-----------------------------------------------------------------*/
#endif
