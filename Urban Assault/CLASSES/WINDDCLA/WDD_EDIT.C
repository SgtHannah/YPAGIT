/*
**  $Source: PRG:VFM/Classes/_WinDDClass/wdd_edit.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/08 19:12:32 $
**  $Locker: floh $
**  $Author: floh $
**
**  wdd_edit.c -- Implementiert eine Text-Edit-Box für
**                systemfreundliche Texteingaben.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include "stdlib.h"
#include "string.h"
#include "memory.h"

#define WIN3D_WINBOX
#include "bitmap/win3dclass.h"
#include "imm.h"

/*** externe Prototypes ***/
extern void wdd_FailMsg(char *, char *, unsigned long);
extern HINSTANCE win_Instance;
extern HINSTANCE win_LangDllInst;

#define WDD_EDIT_MAXSTRLEN (1024)
#define WDD_TIMERID (1)

char wdd_EditBoxBuf[WDD_EDIT_MAXSTRLEN];
char *wdd_TitleText   = NULL;
char *wdd_OkText      = NULL;
char *wdd_CancelText  = NULL;
char *wdd_DefaultText = NULL;
long wdd_TimerVal     = 0;
void (*wdd_TimerFunc)(void *) = NULL;
void *wdd_TimerArg    = NULL;

/*-----------------------------------------------------------------*/
BOOL FAR PASCAL wdd_EditDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
/*
**  CHANGED
**      12-Jan-98   floh    created
*/
{
    switch(msg) {
        case WM_INITDIALOG:
            {
                HWND ebox_hwnd,ok_hwnd,cancel_hwnd;

                ebox_hwnd   = GetDlgItem(hDlg,101);
                ok_hwnd     = GetDlgItem(hDlg,IDOK);
                cancel_hwnd = GetDlgItem(hDlg,IDCANCEL);

                if (wdd_TitleText) SetWindowText(hDlg,wdd_TitleText);
                if (ok_hwnd && wdd_OkText) {
                    SetWindowText(ok_hwnd,wdd_OkText);
                };
                if (cancel_hwnd && wdd_CancelText) {
                    SetWindowText(cancel_hwnd,wdd_CancelText);
                };
                if (ebox_hwnd) {
                    if (wdd_DefaultText) SetWindowText(ebox_hwnd,wdd_DefaultText);
                    SetFocus(ebox_hwnd);
                };

                /*** Callback-Timer initialisieren... ***/
                if (wdd_TimerFunc) {
                    SetTimer(hDlg,WDD_TIMERID,wdd_TimerVal,NULL);
                };
            };
            break;

        case WM_TIMER:
            if (wdd_TimerFunc) {
                /*** Timer-Callback aufrufen ***/
                KillTimer(hDlg,WDD_TIMERID);
                wdd_TimerFunc(wdd_TimerArg);
                SetTimer(hDlg,WDD_TIMERID,wdd_TimerVal,NULL);
            };
            break;

        case WM_COMMAND:

            /*** IME Window flushen ***/
    		if ((HIWORD(wParam) == EN_KILLFOCUS) && (GetDlgItem(hDlg,101)==(HWND)lParam)) {
    			HIMC hIMC = ImmGetContext((HWND)lParam);
    			if (hIMC) {
    				ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0 );
    				ImmReleaseContext((HWND)lParam,hIMC);
    			}
    			break;
    		};
        
            /*** Kommando auswerten ***/        
            switch (wParam) {
                case IDOK:
                    {
                        HWND hEdit;
                        int len;

                        /*** Inhalt der EditBox holen ***/
                        hEdit = GetDlgItem(hDlg,101);
                        len = GetWindowText(hEdit,wdd_EditBoxBuf,sizeof(wdd_EditBoxBuf-1));
                        if (len == 0) wdd_EditBoxBuf[0] = 0;

                        /*** und Dialog killen ***/
                        if (wdd_TimerFunc) KillTimer(hDlg,WDD_TIMERID);
                        EndDialog(hDlg,1);
                        return(TRUE);
                    };
                    break;

                case IDCANCEL:
                    if (wdd_TimerFunc) KillTimer(hDlg,WDD_TIMERID);
                    EndDialog(hDlg,0);
                    return(TRUE);

                case EN_CHANGE:
                    {
                        /*** Inhalt der EditBox rüberkopieren ***/
                        HWND hEdit = (HWND) lParam;
                        int len;
                        len = GetWindowText(hEdit,wdd_EditBoxBuf,sizeof(wdd_EditBoxBuf-1));
                        if (len == 0) wdd_EditBoxBuf[0] = 0;
                    };
                    break;
            };
            break;
    };
    return(FALSE);
}

/*-----------------------------------------------------------------*/
void wdd_ClipText(char *str, long num_bytes)
/*
**  FUNCTION
**      Clippt den Text auf num_bytes, double-byte-clean.
**
**  CHANGED
**      15-Jun-98   floh    created   
*/
{
    char *p = str;
    long i = 0;
    while ((p = CharNext(p)) && (*p) && ((i=p-str)<num_bytes));
    if (p && (*p)) {
        p = CharPrev(str,p);
        if (p) *p=0;
    };
}

/*-----------------------------------------------------------------*/
void wdd_FilterPathName(char *str)
/*
**  FUNCTION
**      Ersetzt alle fuer Filenamen ungueltige Zeichen
**      durch ein Leerzeichen.
**
**  CHANGED
**      15-Jun-98   floh    created
*/
{
    char *p;
    for (p=str; *p; p=CharNext(p)) {
        char c = *p;
        if (!IsDBCSLeadByte(c)) {
            switch (c) {
                case '\\':
                case '/':
                case ':':
                case '*':
                case '?':
                case '"':
                case '<':
                case '>':
                case '|':
                case ',':
                    *p = ' ';
                    break;
            };
        };
    };
}     

/*-----------------------------------------------------------------*/
char *wdd_GetText(struct windd_data *wdd,
                  char *title_text,
                  char *ok_text,
                  char *cancel_text,
                  char *default_text,
                  long timer_val,
                  void (*timer_func)(void *),
                  void *timer_arg,
                  unsigned long flags,
                  unsigned long max_text_len) 
/*
**  FUNCTION
**      Erzeugt eine TextEditBox und wartet auf eine
**      gültige Texteingabe. Returniert Zeiger auf
**      einen statischen Textbuffer mit Eingabe,
**      oder NULL bei Abbruch.
**      Die Routine MUSS zwischen EnableGDI/DisableGDI
**      aufgerufen werden!
**
**  CHANGED
**      08-Jan-98   floh    created
**      19-Mar-98   floh    + lädt die DlgBox Resource jetzt aus der
**                            Locale-Dll
**      15-Jun-98   floh    + <flags> und <max_text_len> Parameter
*/
{
    if (wdd->hWnd && win_LangDllInst) {

        int retval = 0;
        HRSRC hRsrc;

        /*** Resource laden ***/
        hRsrc = FindResource(win_LangDllInst,"EDITBOX_DLG",RT_DIALOG);
        if (hRsrc) {
            LPDLGTEMPLATE hDlgBox;
            hDlgBox = (LPDLGTEMPLATE) LoadResource(win_LangDllInst,hRsrc);
            if (hDlgBox) {
                wdd_TitleText   = title_text;
                wdd_OkText      = ok_text;
                wdd_CancelText  = cancel_text;
                wdd_DefaultText = default_text;
                wdd_TimerVal    = timer_val;
                wdd_TimerFunc   = timer_func;
                wdd_TimerArg    = timer_arg;
                retval = DialogBoxIndirect(win_Instance,hDlgBox,wdd->hWnd,(DLGPROC)wdd_EditDialogProc);
                if (retval == 1) { 
                   if (max_text_len > 0) wdd_ClipText(wdd_EditBoxBuf,max_text_len);
                   if (flags & (1<<0)) wdd_FilterPathName(wdd_EditBoxBuf); 
                   return(wdd_EditBoxBuf);
                } else return(NULL);
            };
        };
    };
    return(NULL);
}




