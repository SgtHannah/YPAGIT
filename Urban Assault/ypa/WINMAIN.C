/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  WinMain.c -- Gummizellen-Frontend für VFM-Windows-Apps.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <windows.h>
#include <windowsx.h>

extern unsigned long ypa_Init( void );
extern unsigned long ypa_DoFrame( void );
extern void ypa_Kill( void );

/*** globale Variablen allgemeinen Interesses ***/
HINSTANCE win_Instance = NULL;
int       win_CmdShow  = NULL;
HWND      win_HWnd     = NULL;
char     *win_CmdLine  = NULL;
char      CmdLineCopy[ 1000 ];
HINSTANCE win_LangDllInst = NULL;

/*-----------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HWND old_hwnd;

    win_Instance = hInstance;
    win_CmdShow  = nCmdShow;
    win_HWnd     = NULL;
    win_CmdLine  = lpCmdLine;

    /*** laufen wir schon? ***/
    old_hwnd = FindWindow("UA Window Class","Urban Assault");
    if (old_hwnd) {
        /*** jo, alte Instanz aktivieren, und selbst beenden ***/
        ShowWindow(old_hwnd,SW_RESTORE);
        SetForegroundWindow(old_hwnd);
        return(0);
    };

    /*** COM initialisieren ***/
    CoInitialize(NULL);

    /*** Priority boosten ***/
    // SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);

    /* --------------------------------------------
    ** Simulation einer C-Shell-Parameter-Übergabe,
    ** Windows übergibt aber keinen Programmnamen
    ** ------------------------------------------*/
    strcpy( CmdLineCopy, win_CmdLine );
    if (ypa_Init()) {
        while (1) {
            if (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) {
                if (!GetMessage(&msg,NULL,0,0)) {
                    ypa_Kill();
                    CoUninitialize();
                    return(msg.wParam);
                } else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                };
            } else if (GetActiveWindow()) {
                if (!ypa_DoFrame()) {
                    if (win_HWnd) DestroyWindow(win_HWnd);
                    ypa_Kill();
                    CoUninitialize();
                    return(msg.wParam);
                };
            } else WaitMessage();
        };
    };
    CoUninitialize();
    return(0);
}

