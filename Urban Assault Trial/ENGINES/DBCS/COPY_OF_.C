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
#include "misc/dshow.h"

static Media media;

/*-----------------------------------------------------------------**
**                                                                 **
**  cplay.c Support Routinen                                       **
**                                                                 **
**-----------------------------------------------------------------*/
BOOL CanPlay(void)
{
    return((media.state==Stopped) || (media.state==Paused));
}
/*-----------------------------------------------------------------*/
BOOL CanStop(void)
{
    return((media.state==Playing) || (media.state==Paused));
}
/*-----------------------------------------------------------------*/
BOOL CanPause(void)
{
    return((media.state==Playing) || (media.state==Stopped));
}
/*-----------------------------------------------------------------*/
BOOL IsInitialized(void)
{
    return(media.state != Uninitialized);
}
/*-----------------------------------------------------------------*/
void ChangeStateTo( State newState )
{
    media.state = newState;
}
/*-----------------------------------------------------------------*/
BOOL InitMedia(void)
{
    ChangeStateTo(Uninitialized);
    media.hGraphNotifyEvent = NULL;
    media.pGraph = NULL;
    return(TRUE);
}
/*-----------------------------------------------------------------*/
void DeleteContents()
{
    if (media.pGraph != NULL) {
	    media.pGraph->lpVtbl->Release(media.pGraph);
	    media.pGraph = NULL;
    };
    media.hGraphNotifyEvent = NULL;
    ChangeStateTo(Uninitialized);
}
/*-----------------------------------------------------------------*/
BOOL CreateFilterGraph(void)
{
    IMediaEvent *pME;
    HRESULT hr;

    if (media.pGraph == NULL) {
        hr = CoCreateInstance(&CLSID_FilterGraph, NULL,                         
    			  CLSCTX_INPROC_SERVER,         
    			  &IID_IGraphBuilder,           
    			  (void **) &media.pGraph);  
        if (hr != S_OK) {
	        media.pGraph = NULL;
	        return(FALSE);
        };

        hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph, &IID_IMediaEvent, (void **) &pME);
        if (hr != S_OK) {
	        DeleteContents();
	        return(FALSE);
        };

        hr = pME->lpVtbl->GetEventHandle(pME, (OAEVENT*) &media.hGraphNotifyEvent);
        pME->lpVtbl->Release( pME );
        if (hr != S_OK) {
	        DeleteContents();
	        return(FALSE);
        };
        return(TRUE);
    };
    return(FALSE);
}
/*-----------------------------------------------------------------*/
BOOL RenderFile(LPSTR szFileName)
{
    HRESULT hr;
    WCHAR wPath[MAX_PATH];
    DeleteContents();
    if (!CreateFilterGraph()) {
        MessageBox(NULL,"CreateFilterGraph() failed","Bla",MB_OK); 
        return(FALSE);
    };
    MultiByteToWideChar(CP_ACP,0,szFileName,-1,wPath,MAX_PATH);
    hr = media.pGraph->lpVtbl->RenderFile(media.pGraph,wPath,NULL);
    if (hr != S_OK) {
        MessageBox(NULL,"RenderFile() failed","Bla",MB_OK); 
        return(FALSE);
    };
    return(TRUE);

}
/*-----------------------------------------------------------------*/
void OnMediaPlay(void)
{
    if (CanPlay()) {
	    HRESULT hr;
	    IMediaControl *pMC;
        hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph, 
             &IID_IMediaControl, (void **) &pMC);
        if (hr == S_OK) {
	        hr = pMC->lpVtbl->Run(pMC);
	        pMC->lpVtbl->Release(pMC);
	        if (hr == S_OK) ChangeStateTo( Playing );
        };
    };
}
/*-----------------------------------------------------------------*/
void OnMediaPause(void)
{
    if (CanPause()) {
	    HRESULT hr;
	    IMediaControl *pMC;
	    hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph, 
	         &IID_IMediaControl, (void **) &pMC);
	    if (hr == S_OK) {
	        hr = pMC->lpVtbl->Pause(pMC);
	        pMC->lpVtbl->Release(pMC);
	        if(hr == S_OK) ChangeStateTo(Paused);
	    }
    };
}
/*-----------------------------------------------------------------*/
void OnMediaAbortStop(void)
{
    if (CanStop()) {
	    HRESULT hr;
	    IMediaControl *pMC;
	    hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph, 
	         &IID_IMediaControl, (void **) &pMC);
	    if (hr == S_OK) {
	        hr = pMC->lpVtbl->Stop( pMC );
	        pMC->lpVtbl->Release( pMC );
	        if (hr == S_OK)	ChangeStateTo(Stopped);
	    };
	};
}
/*-----------------------------------------------------------------*/
void OnMediaStop(void)
{
    if (CanStop()) {
	    HRESULT hr;
	    IMediaControl *pMC;
	    hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph, 
	         &IID_IMediaControl, (void **) &pMC);
	    if(hr == S_OK) {
	        pMC->lpVtbl->Stop(pMC);
	        pMC->lpVtbl->Release(pMC);
	        ChangeStateTo(Stopped);
	    };
    };
}
/*-----------------------------------------------------------------*/
HANDLE GetGraphEventHandle(void)
{
    return(media.hGraphNotifyEvent);
}
/*-----------------------------------------------------------------*/
unsigned long OnGraphNotify(void)
{
    IMediaEvent *pME;
    long lEventCode, lParam1, lParam2;
    HRESULT hr;
    unsigned long retval = FALSE;

    if (media.hGraphNotifyEvent != NULL) {
        hr = media.pGraph->lpVtbl->QueryInterface(media.pGraph, 
             &IID_IMediaEvent, (void **) &pME);
        if (hr == S_OK) {
	        hr = pME->lpVtbl->GetEvent(pME, &lEventCode, &lParam1, &lParam2, 0);
            if (hr == S_OK) {
	            if (lEventCode == EC_COMPLETE) {
		            OnMediaStop();
                    retval = TRUE;
	            } else if ((lEventCode == EC_USERABORT) ||
		                   (lEventCode == EC_ERRORABORT)) 
		        {
		            OnMediaAbortStop();
                    retval = TRUE;
                };
            };
          	pME->lpVtbl->Release(pME);
        };
    };
    return(retval);
}

/*-------------------------------------------------------*/
BOOL EventLoop(void)
{
    MSG msg;
    HANDLE  ahObjects[1];
    const int cObjects = 1;

    while (TRUE) {
        ahObjects[0] = GetGraphEventHandle();
        if (ahObjects[0] == NULL) WaitMessage();
        else {
            DWORD Result = MsgWaitForMultipleObjects(cObjects,
                                                     ahObjects,
                                                     FALSE,
                                                     INFINITE,
                                                     QS_ALLINPUT);
            if(Result != (WAIT_OBJECT_0+cObjects)) {
                if(Result == WAIT_OBJECT_0) {
                    // FIXME: bei STOP zurueckkehren?
                    if (OnGraphNotify()) return(TRUE);
                };
            }
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                // FIXME: offensichtlich, wenn komplett quit!?!
                return(FALSE);
            };
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        };
    };

}

/*-----------------------------------------------------------------**
**                                                                 **
**  dshow.c Interface Routinen                                     **
**                                                                 **
**-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
unsigned long dshow_PlayMovie(char *fname)
/*
**  FUNCTION
**
**  INPUTS
**
**	RESULTS
**
**  CHANGED
*/
{
    unsigned long result = 0;
    if (fname) {
        CoInitialize(NULL);
        if (InitMedia()) {
            MessageBox(NULL,"Nach InitMedia()","Bla",MB_OK); 
            if (RenderFile(fname)) {
                MessageBox(NULL,"Nach RenderFile()","Bla",MB_OK); 
                ChangeStateTo(Stopped);
                OnMediaPlay();
                result = EventLoop();
                OnMediaStop();
            };
            DeleteContents();
        };
        CoUninitialize();
    };
    return(result);
}








