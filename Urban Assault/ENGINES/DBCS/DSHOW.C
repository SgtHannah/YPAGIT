/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  dshow.c -- DirectShow Movie Player. Kochbuch-Code...
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DSHOW_PADDEDCELL (1)
//#ifdef _MSC_VER
//#define IErrorLog void
//#endif
#include "misc/dshow.h"

/*-----------------------------------------------------------------**
**                                                                 **
**  dshow.c Interface Routinen                                     **
**                                                                 **
**-----------------------------------------------------------------*/

IGraphBuilder *pigb = NULL;
IVideoWindow  *pivw = NULL;
IMediaControl *pimc = NULL;
IMediaEventEx *pime = NULL;
unsigned long dshow_StopReceived = FALSE;

#define OATRUE  (-1)
#define OAFALSE (0)

/*-----------------------------------------------------------------*/
void dshow_Cleanup(void)
/*
**  CHANGED
**      23-Jan-98   floh    created
*/
{
    if (pime) {
        pime->lpVtbl->Release(pime);
        pime = NULL;
    };
    if (pimc) {
        pimc->lpVtbl->Release(pimc);
        pimc = NULL;
    };
    if (pivw) {
        pivw->lpVtbl->Release(pivw);
        pivw = NULL;
    };
    if (pigb) {
        pigb->lpVtbl->Release(pigb);
        pigb = NULL;
    };
}

/*-----------------------------------------------------------------*/
void dshow_StopMovie(void)
/*
**  CHANGED
**      23-Jan-98   floh    created
*/
{
    if (pimc) {
        dshow_StopReceived = TRUE;
    };
}

/*-----------------------------------------------------------------*/
long FAR PASCAL dshow_WinProc(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
/*
**  FUNCTION
**      Mini-Ersatz-WinProc, wenn der Movie-Player aktiv ist.
**      Ignoriert alles, außer Tastendruck und sonstige
**      Abbruchbedingungen.
**
**  CHANGED
**      28-Jan-98   floh    created
**      26-Feb-98   floh    + ein paar neue Messages getrappt
*/
{
    switch(message) {

        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
            /*** Movieplayer stoppen ***/
            dshow_StopMovie();
            break;

        case WM_ERASEBKGND:
            /*** Background-Löschen abwürgen ***/
            return(1);
            break;

        case WM_MEDIAEVENT:
            {
                LONG evCode,evParam1,evParam2;

                /*** Event(s) abholen ***/
                while (SUCCEEDED(pime->lpVtbl->GetEvent(pime,
                       &evCode,&evParam1,&evParam2,0)))
                {
                    switch(evCode) {
                        case EC_COMPLETE:
                        case EC_USERABORT:
                        case EC_ERRORABORT:
                            dshow_StopMovie();
                            break;
                    };
                };
            };
    };

    /*** den Default-Handler aufrufen ***/
    return(DefWindowProc(hWnd,message,wParam,lParam));
}

/*-----------------------------------------------------------------*/
unsigned long dshow_PlayMovie(char *fname, HWND hwnd)
/*
**  FUNCTION
**      Movie-Player mit DirectShow.
**
**  CHANGED
**      22-Jan-98   floh    created
*/
{
    HRESULT hr;
    RECT rc;
    #define MAXPATH (512)
    WCHAR path[MAXPATH];
    WNDPROC oldWinProc = NULL;
    MSG msg;

    dshow_StopReceived = FALSE;

    hr = CoCreateInstance(&CLSID_FilterGraph,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_IGraphBuilder,
                          (void **)&pigb);
    if (FAILED(hr)) {
        MessageBox(hwnd,"Filtergraph creation failed.","dshow.c",MB_ICONERROR);
        return(FALSE);
    };

    MultiByteToWideChar(CP_ACP,0,fname,-1,path,MAXPATH);
    hr = pigb->lpVtbl->RenderFile(pigb,path,NULL);
    if (FAILED(hr)) {
        MessageBox(hwnd,"RenderFile() failed.","dshow.c",MB_ICONERROR);
        dshow_Cleanup();
        return(FALSE);
    };

    hr = pigb->lpVtbl->QueryInterface(pigb, &IID_IVideoWindow, (void **)&pivw);
    pivw->lpVtbl->put_Owner(pivw, (OAHWND)hwnd);
    pivw->lpVtbl->put_WindowStyle(pivw, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
    pivw->lpVtbl->put_WindowStyleEx(pivw, WS_EX_TOPMOST);
    pivw->lpVtbl->put_AutoShow(pivw,-1);
    // won't send WM_ messages when fullscreen activated
    // pivw->lpVtbl->put_FullScreenMode(pivw,-1);
    GetClientRect(hwnd,&rc);
    pivw->lpVtbl->SetWindowPosition(pivw,rc.left,rc.top,rc.right,rc.bottom);
    pivw->lpVtbl->HideCursor(pivw,OATRUE);
    pivw->lpVtbl->put_MessageDrain(pivw,(OAHWND)hwnd);
    pivw->lpVtbl->SetWindowForeground(pivw,-1);

    hr = pigb->lpVtbl->QueryInterface(pigb, &IID_IMediaControl, (void **)&pimc);
    hr = pigb->lpVtbl->QueryInterface(pigb, &IID_IMediaEventEx, (void **)&pime);
    hr = pime->lpVtbl->SetNotifyWindow(pime, (OAHWND)hwnd, WM_MEDIAEVENT, 0);
    pimc->lpVtbl->Run(pimc);
    oldWinProc = SetWindowLong(hwnd,GWL_WNDPROC,dshow_WinProc);

    //while (!dshow_StopReceived) {
    //    WaitMessage();
    //    if (GetMessage(&msg,NULL,0,0)) {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    };
    //};

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			WaitMessage();
		}
		if (dshow_StopReceived)
			break;
	}



    /*** originale WinProc wieder installieren ***/
    SetWindowLong(hwnd,GWL_WNDPROC,oldWinProc);
    pimc->lpVtbl->Stop(pimc);
    pivw->lpVtbl->put_Owner(pivw, NULL);
    dshow_Cleanup();
    return(TRUE);
}

