/*
**  $Source: PRG:VFM/Classes/_WinInpClass/winp_wbox.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 15:10:55 $
**  $Locker: floh $
**  $Author: floh $
**
**  Windows-Gummizelle für die winp.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#define MMNOJOY
#define INITGUID

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "dinput.h"
#include "thirdparty/sw_guid.hpp"

/*** siehe winpclass.h ***/
#define WINP_CODE_LMB       (0x81)  // linke Maustaste
#define WINP_CODE_RMB       (0x82)  // rechte Maustaste
#define WINP_CODE_MMB       (0x83)  // mittlere Maustaste

#define WINP_CODE_MX        (0x84)  // Maus horizontal
#define WINP_CODE_MY        (0x85)  // Maus vertikal

#define WINP_CODE_JB0       (0x86)  // max. 8 Joystick-Buttons
#define WINP_CODE_JB1       (0x87)
#define WINP_CODE_JB2       (0x88)
#define WINP_CODE_JB3       (0x89)
#define WINP_CODE_JB4       (0x8a)
#define WINP_CODE_JB5       (0x8b)
#define WINP_CODE_JB6       (0x8c)
#define WINP_CODE_JB7       (0x8d)

#define WINP_CODE_JX        (0x8e)  // Joystick-X-Achse
#define WINP_CODE_JY        (0x8f)  // Joystick-Y-Achse
#define WINP_CODE_THROTTLE  (0x90)  // Joystick-Throttle
#define WINP_CODE_HATX      (0x91)  // HatView-X-Komponente
#define WINP_CODE_HATY      (0x92)  // HatView-Y-Komponente
#define WINP_CODE_RUDDER    (0x93)  // Joystick-Ruder

/*** hier wird der Original-Window-Handler aufgehoben ***/
WNDPROC winp_OldWinProc = NULL;

/*** Gummizellen-Version von "input/winpclass.h" ***/
struct winp_xy {
    short x,y;
};

/*** aus win_main.c ***/
extern HINSTANCE win_Instance;
extern HWND win_HWnd;

/*** DirectInput Data ***/
#define WINP_BUFFERSIZE (16)

/*** Prototypes ***/
void winp_FFCreateEffects(void);
void winp_FFKillEffects(void);

/*** Strukturen ***/
struct winp_DIForceData {
    LPDIRECTINPUTEFFECT TankEngine;     // Motor-Vibration
    LPDIRECTINPUTEFFECT JetEngine;      // Plane-Engine
    LPDIRECTINPUTEFFECT HeliEngine;     // Heli-Effect
    LPDIRECTINPUTEFFECT MaxRot;         // Behinderung durch Vehicle-Maxrot
    LPDIRECTINPUTEFFECT MGun;           // Machine-Gun-Effect
    LPDIRECTINPUTEFFECT MissLaunch;     // Weapon-Launch
    LPDIRECTINPUTEFFECT GrenLaunch;     // Granaten-Abschuß
    LPDIRECTINPUTEFFECT BombLaunch;     // Bomben-Abschuß
    LPDIRECTINPUTEFFECT Collission;     // Kollision
    LPDIRECTINPUTEFFECT Shake1;         // ein Random-Shake
    LPDIRECTINPUTEFFECT Shake2;         // wird mit Shake1 überlagert

    DIPERIODIC      PeriodicTankEngine;
    DIPERIODIC      PeriodicJetEngine;
    DIPERIODIC      PeriodicHeliEngine;
    DICONDITION     CondMaxRot[2];
    DIPERIODIC      PeriodicMGun;
    DIPERIODIC      PeriodicMissLaunch;
    DIRAMPFORCE     RampGrenLaunch;
    DIRAMPFORCE     RampBombLaunch;
    DIRAMPFORCE     RampCollission;
    DIPERIODIC      PeriodicShake1;
    DIPERIODIC      PeriodicShake2;
};

struct winp_DIData {
    HINSTANCE hinst;
    HWND hwnd;
    LPDIRECTINPUT DI;

    /*** Mouse specifics ***/
    LPDIRECTINPUTDEVICE Mouse;

    /*** Joystick-Specifics ***/
    LPDIRECTINPUTDEVICE2 Joyst2;
    LPDIRECTINPUTDEVICE2 FFJoyst;   // init. falls ForceFeedback Joystick
    DIDEVCAPS            JCaps;     // Joystick-Caps.
    struct winp_DIForceData FFData;

    DIDEVICEOBJECTDATA Data[WINP_BUFFERSIZE];      // wird ausgefuellt
} winp_DI;

DIPROPDWORD winp_DipDw =
{
    {
        sizeof(DIPROPDWORD),        // diph.dwSize
        sizeof(DIPROPHEADER),       // diph.dwHeaderSize
        0,                          // diph.dwObj
        DIPH_DEVICE,                // diph.dwHow
    },
    0,                // dwData
};

/*** aus winp_main.c ***/
extern void winp_KeyDown(unsigned long);
extern void winp_KeyUp(unsigned long);

/*** aus winp_log.c ***/
void winp_Log(char *string,...);

/*** Größe des Backbuffers (für Maus-Rückrechnung) ***/
extern long winp_SizeX;
extern long winp_SizeY;

/*** Input Status Global Data ***/
extern unsigned long winp_HasFocus;
extern unsigned long winp_ContKey;
extern unsigned long winp_NormKey;
extern unsigned long winp_AsciiKey[];
extern unsigned long winp_MaxAsciiKeyIndex;
extern unsigned long winp_ActAsciiKeyIndex;

extern unsigned long winp_LmbIsDown, winp_RmbIsDown, winp_MmbIsDown;
extern short winp_LmbDownCnt, winp_RmbDownCnt, winp_MmbDownCnt;
extern short winp_LmbUpCnt, winp_RmbUpCnt, winp_MmbUpCnt;
extern unsigned long winp_LmbDblClck;

extern struct winp_xy winp_ScreenMousePos;
extern struct winp_xy winp_WinMousePos;
extern struct winp_xy winp_LmbDownPos;
extern struct winp_xy winp_RmbDownPos;
extern struct winp_xy winp_MmbDownPos;
extern struct winp_xy winp_LmbUpPos;
extern struct winp_xy winp_RmbUpPos;
extern struct winp_xy winp_MmbUpPos;
extern struct winp_xy winp_JoyPrimAxes;
extern struct winp_xy winp_JoySecAxes;
extern struct winp_xy winp_JoyHatAxes;
extern unsigned long winp_JoyBtn;

/*-----------------------------------------------------------------*/
void winp_FailMsg(char *title, char *msg, unsigned long code)
/*
**  CHANGED
**      04-Feb-97   floh    created
*/
{
    char buf[128];
    char *err;

    if (code == DIERR_ACQUIRED)                    err = "Acquired";
    // else if (code == DIERR_ALREADYINITIALIZED)     err = "AlreadyInitialized";
    else if (code == DIERR_BADDRIVERVER)           err = "BadDriverVer";
    // else if (code == DIERR_BETADIRECTINPUTVERSION) err = "BetaDirectInputVersion";
    else if (code == DIERR_DEVICEFULL)             err = "DeviceFull";
    else if (code == DIERR_DEVICENOTREG)           err = "DeviceNotReg";
    else if (code == DIERR_EFFECTPLAYING)          err = "EffectPlaying";
    else if (code == DIERR_HASEFFECTS)             err = "HasEffects";
    else if (code == DIERR_GENERIC)                err = "Generic";
    else if (code == DIERR_READONLY)               err = "HandleExists or ReadOnly";
    else if (code == DIERR_INPUTLOST)              err = "InputLost";
    else if (code == DIERR_INVALIDPARAM)           err = "InvalidParam";
    else if (code == DIERR_NOAGGREGATION)          err = "NoAggregation";
    else if (code == DIERR_NOTACQUIRED)            err = "NotAcquired";
    else if (code == DIERR_NOTEXCLUSIVEACQUIRED)   err = "NotExclusiveAcquired";
    else if (code == DIERR_MOREDATA)               err = "MoreData";
    else if (code == DIERR_NOINTERFACE)            err = "NoInterface";
    else if (code == DIERR_NOTDOWNLOADED)          err = "NotDownloaded";
    else if (code == DIERR_NOTBUFFERED)            err = "NotBuffered";
    else if (code == DIERR_NOTFOUND)               err = "NotFound";
    else if (code == DIERR_NOTINITIALIZED)         err = "NotInitialized";
    else if (code == DIERR_OBJECTNOTFOUND)         err = "ObjectNotFound";
    // else if (code == DIERR_OLDDIRECTINPUTVERSION)  err = "OldDirectInputVersion";
    else if (code == DIERR_OTHERAPPHASPRIO)        err = "OtherAppHasPrio";
    else if (code == DIERR_OUTOFMEMORY)            err = "OutOfMemory";
    else if (code == DIERR_UNSUPPORTED)            err = "Unsupported";
    else                
    winp_Log("FAIL MSG: title=%s, msg=%s, err=%s\n",title,msg,err);
}    

/*-----------------------------------------------------------------*/
void winp_KillDirectInput(void)
/*
**  FUNCTION
**      DirectInput Cleanup
**
**  CHANGED
**      04-Feb-97   floh    created
*/
{
    HRESULT dival;

    /*** Mouse killen ***/
    if (winp_DI.Mouse) {
        dival = winp_DI.Mouse->lpVtbl->Unacquire(winp_DI.Mouse);
        winp_DI.Mouse->lpVtbl->Release(winp_DI.Mouse);
        winp_DI.Mouse = NULL;
    };    

    /*** Joystick killen ***/
    if (winp_DI.Joyst2) {
        dival = winp_DI.Joyst2->lpVtbl->Unacquire(winp_DI.Joyst2);
        winp_DI.Joyst2->lpVtbl->Release(winp_DI.Joyst2);
        winp_DI.Joyst2 = NULL;
    };
    /*** optionales ForceFeedback-Handle killen ***/
    if (winp_DI.FFJoyst) {
        /*** alle Effekte killen ***/
        winp_FFKillEffects();
        dival = winp_DI.FFJoyst->lpVtbl->Release(winp_DI.FFJoyst);
        winp_DI.FFJoyst = NULL;
    };

    /*** DirectInput killen ***/
    if (winp_DI.DI) winp_DI.DI->lpVtbl->Release(winp_DI.DI);
}

/*-----------------------------------------------------------------*/
unsigned long winp_InitMouse(void)
/*
**  FUNCTION
**      Initialisiert die System-Maus als DirectInput-Device.
**
**  CHANGED
**      13-May-97   floh    created
**      01-May-98   floh    macht jetzt zuallererst das App-Window aktiv,
**                          weil sonst Acuire schiefgehen kann.
*/
{
    HRESULT dival;

    dival = winp_DI.DI->lpVtbl->CreateDevice(winp_DI.DI,&GUID_SysMouse,&(winp_DI.Mouse), NULL);
    if (dival != DI_OK) {
        winp_FailMsg("CreateDevice()","DirectInput",dival);
        winp_KillDirectInput();
        return(FALSE);
    };

    dival = winp_DI.Mouse->lpVtbl->SetDataFormat(winp_DI.Mouse,&c_dfDIMouse);
    if (dival != DI_OK) {
        winp_FailMsg("SetDataFormat()","DirectInputDevice",dival);
        winp_KillDirectInput();
        return(FALSE);
    };

    dival = winp_DI.Mouse->lpVtbl->SetCooperativeLevel(winp_DI.Mouse,
            winp_DI.hwnd,
            DISCL_NONEXCLUSIVE|DISCL_BACKGROUND);
//            DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
    if (dival != DI_OK) {
        winp_FailMsg("SetCooperativeLevel()","DirectInputDevice",dival);
        winp_KillDirectInput();
        return(FALSE);
    };

    winp_DipDw.dwData = WINP_BUFFERSIZE;
    dival = winp_DI.Mouse->lpVtbl->SetProperty(winp_DI.Mouse,DIPROP_BUFFERSIZE,&winp_DipDw);
    if (dival != DI_OK) {
        winp_FailMsg("SetProperty(BUFFERSIZE)","DirectInputDevice",dival);
        winp_KillDirectInput();
        return(FALSE);
    };

    winp_DipDw.dwData = DIPROPAXISMODE_ABS;
    dival = winp_DI.Mouse->lpVtbl->SetProperty(winp_DI.Mouse,DIPROP_AXISMODE,&winp_DipDw);
    if (dival != DI_OK) {
        winp_FailMsg("SetProperty(AXISMODE)","DirectInputDevice",dival);
        winp_KillDirectInput();
        return(FALSE);
    };

    if (win_HWnd) ShowWindow(win_HWnd,SW_SHOW);
    dival = winp_DI.Mouse->lpVtbl->Acquire(winp_DI.Mouse);
    if (dival != DI_OK) {
        winp_FailMsg("Acquire(Mouse)","DirectInputDevice",dival);
        winp_KillDirectInput();
        return(FALSE);
    };

    /*** alles klaa ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
BOOL FAR PASCAL winp_EnumJoystCallback(
                    LPDIDEVICEINSTANCE lpddi,
                    LPVOID pvRef)
/*
**  FUNCTION
**      Callback-Hook für EnumDevices() auf Joystick-Typen.
**      Man beachte, das gefundene Joysticks sofort komplett
**      initialisiert werden. Nach dem ersten vollständig
**      initialisierten Joystick wird die Enumeration
**      abgebrochen.
**
**  CHANGED
**      13-May-97   floh    created
**      17-May-97   floh    + ForceFeedback-Initialisierung
**      03-Jun-97   floh    + Support für R-Achse (Rudder)
**      04-Jun-97   floh    + Z- und R-Achse sind jetzt optional
*/
{
    HRESULT dival;
    LPDIRECTINPUTDEVICE joy;

    winp_DI.Joyst2 = NULL;

    /*** erzeuge das Joystick-Device ***/
    dival = winp_DI.DI->lpVtbl->CreateDevice(winp_DI.DI,
                &(lpddi->guidInstance), &joy, NULL);
    if (dival == DI_OK) {

        DIPROPRANGE diprg;
        DIPROPDWORD dipdw;

        memset(&diprg,0,sizeof(diprg));
        memset(&dipdw,0,sizeof(dipdw));
        memset(&(winp_DI.JCaps.dwSize),0,sizeof(winp_DI.JCaps));

        /*** DirectInputDevice2 Interface abfragen ***/
        dival = joy->lpVtbl->QueryInterface(joy,
                &IID_IDirectInputDevice2,&(winp_DI.Joyst2));
        joy->lpVtbl->Release(joy);
        if (dival != DI_OK) goto fail_escape;

        /*** dann seine Caps abfragen ***/
        winp_DI.JCaps.dwSize = sizeof(winp_DI.JCaps);
        dival = winp_DI.Joyst2->lpVtbl->GetCapabilities(winp_DI.Joyst2,
                &(winp_DI.JCaps));
        if (dival != DI_OK) goto fail_escape;

        /*** ForceFeedback-Device? ***/
        if (winp_DI.JCaps.dwFlags & DIDC_FORCEFEEDBACK) {
            dival = winp_DI.Joyst2->lpVtbl->QueryInterface(winp_DI.Joyst2,
                    &IID_IDirectInputDevice2,&(winp_DI.FFJoyst));
            if (dival != DI_OK) {
                /*** kein Abbruch, sondern nur Pointer ungültig machen ***/
                winp_DI.FFJoyst = NULL;
            };
        };

        /*** Joystick initialisieren ***/
        dival = winp_DI.Joyst2->lpVtbl->SetDataFormat(winp_DI.Joyst2,&c_dfDIJoystick);
        if (dival != DI_OK) goto fail_escape;
        dival = winp_DI.Joyst2->lpVtbl->SetCooperativeLevel(winp_DI.Joyst2,
                    winp_DI.hwnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND);
        if (dival != DI_OK) goto fail_escape;

        /*** Achsen-Range setzen ***/
        diprg.diph.dwSize       = sizeof(diprg);
        diprg.diph.dwHeaderSize = sizeof(diprg.diph);
        diprg.diph.dwObj        = DIJOFS_X;
        diprg.diph.dwHow        = DIPH_BYOFFSET;
        diprg.lMin = -300;  // identisch WINP_MAX_SLIDER!
        diprg.lMax = +300;
        dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                DIPROP_RANGE, &diprg.diph);
        if (dival != DI_OK) {
            winp_FailMsg("SetProperty() 1","DirectInputDevice",dival);
        };
        diprg.diph.dwObj = DIJOFS_Y;
        dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                DIPROP_RANGE, &diprg.diph);
        if (dival != DI_OK) {
            winp_FailMsg("SetProperty() 2","DirectInputDevice",dival);
        };
        /*** Z-Achse (Throttle) ***/
        if (winp_DI.JCaps.dwAxes > 2) {
            diprg.diph.dwObj = DIJOFS_Z;
            dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                    DIPROP_RANGE, &diprg.diph);
            if (dival != DI_OK) {
                winp_FailMsg("SetProperty() 3","DirectInputDevice",dival);
            };
        };

        /*** R-Achse (Ruder) ***/
        if (winp_DI.JCaps.dwAxes > 3) {
            diprg.diph.dwObj = DIJOFS_RZ;
            dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                    DIPROP_RANGE, &diprg.diph);
            if (dival != DI_OK) {
                winp_FailMsg("SetProperty() 4","DirectInputDevice",dival);
            };
        };

        /*** Deadzone setzen ***/
        dipdw.diph.dwSize       = sizeof(dipdw);
        dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
        dipdw.diph.dwObj        = DIJOFS_X;
        dipdw.diph.dwHow        = DIPH_BYOFFSET;
        dipdw.dwData            = 2000; // 15% Deadzone
        dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                DIPROP_DEADZONE, &dipdw.diph);
        if (dival != DI_OK) {
            winp_FailMsg("SetProperty() 5","DirectInputDevice",dival);
        };

        dipdw.diph.dwObj = DIJOFS_Y;
        dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                DIPROP_DEADZONE, &dipdw.diph);
        if (dival != DI_OK) {
            winp_FailMsg("SetProperty() 6","DirectInputDevice",dival);
        };

        if (winp_DI.JCaps.dwAxes > 3) {
            dipdw.diph.dwObj = DIJOFS_RZ;
            dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                    DIPROP_DEADZONE, &dipdw.diph);
            if (dival != DI_OK) {
                winp_FailMsg("SetProperty() 7","DirectInputDevice",dival);
            };
        };

        /*** falls ForceFeedback-Device, Autocenter on ***/
        if (winp_DI.JCaps.dwFlags & DIDC_FORCEFEEDBACK) {
            dipdw.diph.dwSize       = sizeof(dipdw);
            dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
            dipdw.diph.dwObj  = 0;
            dipdw.diph.dwHow  = DIPH_DEVICE;
            dipdw.dwData      = DIPROPAUTOCENTER_OFF;
            dival = winp_DI.Joyst2->lpVtbl->SetProperty(winp_DI.Joyst2,
                    DIPROP_AUTOCENTER, &dipdw.diph);
            if (dival != DI_OK) {
                winp_FailMsg("SetProperty() 8","DirectInputDevice",dival);
            };
        };

        /*** Treffer! ***/
        return(DIENUM_STOP);
    };

fail_escape:
    winp_DI.Joyst2->lpVtbl->Release(winp_DI.Joyst2);
    winp_DI.Joyst2 = NULL;
    return(DIENUM_CONTINUE);
}

/*-----------------------------------------------------------------*/
unsigned long winp_InitJoystick(void)
/*
**  FUNCTION
**      Initialisiert (wenn vorhanden) einen angeschlossenen
**      Joystick (auch ForceFeedback-Devices).
**
**  CHANGED
**      13-May-97   floh    created
*/
{
    HRESULT dival;

    /*** Joystick-Devices müssen enumeriert werden ***/
    dival = winp_DI.DI->lpVtbl->EnumDevices(winp_DI.DI,
            DIDEVTYPE_JOYSTICK, winp_EnumJoystCallback,
            NULL,
            DIEDFL_ATTACHEDONLY);
    if (dival != DI_OK) {
        winp_FailMsg("EnumDevices()","DirectInput",dival);
        return(FALSE);
    };

    /*** wurde ein Joystick gefunden? ***/
    if (winp_DI.Joyst2) {

        /*** diesen "Acquirieren" ***/
        dival = winp_DI.Joyst2->lpVtbl->Acquire(winp_DI.Joyst2);
        if (dival != DI_OK) {
            winp_FailMsg("Acquire(Joystick)","DirectInputDevice",dival);
            winp_KillDirectInput();
            return(FALSE);
        };

        /*** falls Forcefeedback-Joystick vorhanden, Effekte erzeugen ***/
        if (winp_DI.FFJoyst) winp_FFCreateEffects();

    }; // kein Joystick ist auch nicht schlimm

    /*** Success ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
unsigned long winp_InitDirectInput(void)
/*
**  FUNCTION
**      Initialisiert DirectInput und erzeugt ein
**      Device, welches die Maus repraesentiert.
**
**  CHANGED
**      04-Feb-97   floh    created
*/
{
    HRESULT dival;

    memset(&winp_DI,0,sizeof(winp_DI));
    if (!win_HWnd) return(FALSE);
    if (!win_Instance) return(FALSE);
    winp_DI.hinst = win_Instance;
    winp_DI.hwnd  = win_HWnd;
    dival = DirectInputCreate(winp_DI.hinst, DIRECTINPUT_VERSION, &(winp_DI.DI), NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputCreate()","DirectInput",dival);
        winp_KillDirectInput();
        return(FALSE);
    };    

    /*** Maus initialisieren ***/
    if (!winp_InitMouse()) return(FALSE);

    /*** Joystick initialisieren (optional) ***/
    if (!winp_InitJoystick()) return(FALSE);
        
    /*** Success ***/
    return(TRUE);
}               

/*-----------------------------------------------------------------*/
void winp_GetMPos(HWND hwnd, POINTS *p, struct winp_xy *xy)
/*
**  FUNCTION
**      Macht aus der Screen-relativen 2D-Koordinate
**      in <p> die Window-relativen Koordinaten in <xy>.
**      Wenn die Koordinate außerhalb des Fensters liegt,
**      wird sie entsprechend geclippt.
**
**  CHANGED
**      18-Nov-96   floh    created
*/
{
    RECT r;
    long w,h;
    GetClientRect(hwnd,&r);

    w = r.right - r.left;
    h = r.bottom - r.top;

    xy->x = ((p->x - r.left) * winp_SizeX) / w;
    xy->y = ((p->y - r.top) * winp_SizeY) / h;
    if (xy->x < 0) xy->x=0;
    else if (xy->x >= winp_SizeX) xy->x=winp_SizeX-1;
    if (xy->y < 0) xy->y=0;
    else if (xy->y >= winp_SizeY) xy->y=winp_SizeY-1;
}

/*-----------------------------------------------------------------*/
long FAR PASCAL winp_WinProc(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam)
/*
**  FUNCTION
**      Plugin-Message-Handler für die winp.class.
**
**  CHANGED
**      18-Nov-96   floh    created
**      21-Feb-97   floh    die MouseButton-Ereignisse updaten jetzt
**                          die "normale" Maus-Position, damit funktioniert
**                          hoffentlich das automatische Verschicken von
**                          Maus-Events an VFM.
**      16-Sep-97   floh    + akzeptiert jetzt auch WM_CHAR Messages
**                            für nach ASCII gemappte Keycodes.
**      18-Feb-98   floh    + schickt jetzt auch für MouseButtons ein
**                            Keydown und Keyup los
**      19-Mar-98   floh    + Support für WM_LBUTTONDBLCLK
*/
{
    switch(message) {

        unsigned long key;
        POINTS p;

        case WM_KEYDOWN:
            key = wParam;
            winp_KeyDown(key);  // siehe winp_main.c
            break;

        case WM_KEYUP:
            key = wParam;
            winp_KeyUp(key);    // siehe winp_main.c
            break;

        case WM_CHAR:
            if (winp_ActAsciiKeyIndex < winp_MaxAsciiKeyIndex) {
                winp_AsciiKey[winp_ActAsciiKeyIndex++] = wParam;
            };
            break;

        case WM_LBUTTONDOWN:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_LmbDownPos);
            winp_LmbIsDown = TRUE;
            winp_LmbDownCnt++;
            winp_WinMousePos = winp_LmbDownPos;
            winp_KeyDown(WINP_CODE_LMB);
            break;

        case WM_LBUTTONUP:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_LmbUpPos);
            winp_LmbIsDown = FALSE;
            winp_LmbUpCnt++;
            winp_WinMousePos = winp_LmbUpPos;
            winp_KeyUp(WINP_CODE_LMB);
            break;

        case WM_LBUTTONDBLCLK:
            winp_LmbDblClck = TRUE;
            break;

        case WM_MBUTTONDOWN:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_MmbDownPos);
            winp_MmbIsDown = TRUE;
            winp_MmbDownCnt++;
            winp_WinMousePos = winp_MmbDownPos;
            winp_KeyDown(WINP_CODE_MMB);
            break;

        case WM_MBUTTONUP:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_MmbUpPos);
            winp_MmbIsDown = FALSE;
            winp_MmbUpCnt++;
            winp_WinMousePos = winp_MmbUpPos;
            winp_KeyUp(WINP_CODE_MMB);
            break;

        case WM_RBUTTONDOWN:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_RmbDownPos);
            winp_RmbIsDown = TRUE;
            winp_RmbDownCnt++;
            winp_WinMousePos = winp_RmbDownPos;
            winp_KeyDown(WINP_CODE_RMB);
            break;

        case WM_RBUTTONUP:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_RmbUpPos);
            winp_RmbIsDown = FALSE;
            winp_RmbUpCnt++;
            winp_WinMousePos = winp_RmbUpPos;
            winp_KeyUp(WINP_CODE_RMB);
            break;

        case WM_MOUSEMOVE:
            p = MAKEPOINTS(lParam);
            winp_GetMPos(hWnd,&p,&winp_WinMousePos);
            break;
    };

    /*** den Original-Handler aufrufen ***/
    return(CallWindowProc(winp_OldWinProc,hWnd,message,wParam,lParam));
}

/*-----------------------------------------------------------------*/
void winp_PlugIn(HWND hwnd)
/*
**  FUNCTION
**      Hängt den Message-Handler der wininp.class
**      an das übergebene Fenster. Der Message-
**      Handler filtert alle vorbeikommenden
**      Input-Messages und übergibt dann die Kontrolle
**      an den "Original-Handler" des Fensters
**      (welches der Handler der Display-Treiber-Klasse
**      sein sollte).
**
**  CHANGED
**      18-Nov-96   floh    created
*/
{
    /*** rette alte WinProc, setze neue WinProc ***/
    winp_OldWinProc = SetWindowLong(hwnd,GWL_WNDPROC,winp_WinProc);
}

/*-----------------------------------------------------------------*/
void winp_UnPlug(HWND hwnd)
/*
**  FUNCTION
**      Entfernt den winp.class Message-Handler aus der
**      Handler-Chain und klinkt den Original-Handler
**      wieder ein.
**
**  CHANGED
**      18-Nov-96   floh    created
*/
{
    if (winp_OldWinProc) {
        SetWindowLong(hwnd,GWL_WNDPROC,winp_OldWinProc);
        winp_OldWinProc = NULL;
    };
}

/*-----------------------------------------------------------------*/
void winp_GetAbsMousePos(void)
/*
**  FUNCTION
**      Holt die "absolute" Mausposition vom DirectInput
**      Standard-Maus-Device.
**      Updated winp_ScreenMousePos direkt, muss 1x pro Frame
**      oder oefter aufgerufen werden.
**
**  CHANGED
**      18-Nov-96   floh    created
*/
{
    if (winp_DI.Mouse) {
        
        HRESULT dival;
        DIMOUSESTATE ms;

        dival = winp_DI.Mouse->lpVtbl->GetDeviceState(winp_DI.Mouse,sizeof(ms),&ms);
        if (dival == DIERR_INPUTLOST) {
            dival = winp_DI.Mouse->lpVtbl->Acquire(winp_DI.Mouse);
            dival = winp_DI.Mouse->lpVtbl->GetDeviceState(winp_DI.Mouse,sizeof(ms),&ms);
        };
        if (dival == DI_OK) {
            winp_ScreenMousePos.x = ms.lX<<3;
            winp_ScreenMousePos.y = ms.lY<<3;
        };
    };
}       

/*-----------------------------------------------------------------*/
void winp_GetJoystickState(void)
/*
**  FUNCTION
**      Fragt Joystick-Status ab und schreibt diesen
**      in die globalen Variablen:
**
**          winp_JoystPrimAxes      (X/Y Achsen)
**          winp_JoystSecAxes       (Throttle/(Rudder) Achsen)
**          winp_JoystButtons       (gedrückte Buttons)
**
**  CHANGED
**      13-May-97   floh    created
**      03-Jun-97   floh    + R-Achse (Rudder)
**      04-Jun-97   floh    + Z- und R-Achse jetzt optional
**      10-Jun-97   floh    DirectX5 Beta2 Anpassungen
**      18-Feb-98   floh    JoyBtn hält jetzt den letzten
**                          Zustand, um KeyDown und KeyUp
**                          Events zu generieren.
**      16-May-98   floh    hat nicht abgefragt, ob der Joystick
**                          tatsaechlich einen Hatswitch hat      
*/
{
    if (winp_DI.Joyst2) {

        HRESULT dival;
        DIJOYSTATE js;
        unsigned long old_joybtn = winp_JoyBtn;
        winp_JoyBtn = 0;

        memset(&winp_JoyPrimAxes,0,sizeof(winp_JoyPrimAxes));
        memset(&winp_JoySecAxes,0,sizeof(winp_JoySecAxes));

        dival = winp_DI.Joyst2->lpVtbl->Poll(winp_DI.Joyst2);
        if ((dival==DIERR_INPUTLOST) || (dival==DIERR_NOTACQUIRED)) {
            dival = winp_DI.Joyst2->lpVtbl->Acquire(winp_DI.Joyst2);
            dival = winp_DI.Joyst2->lpVtbl->Poll(winp_DI.Joyst2);
        };
        dival = winp_DI.Joyst2->lpVtbl->GetDeviceState(winp_DI.Joyst2,sizeof(js),&js);

        if (dival == DI_OK) {

            unsigned long i;

            /*** Joystick-Achsen ***/
            winp_JoyPrimAxes.x = js.lX;     // X-Achse
            winp_JoyPrimAxes.y = js.lY;     // Y-Achse
            if (winp_DI.JCaps.dwAxes > 2) winp_JoySecAxes.x=js.lZ;
            else {
                /*** falls kein Throttle, diesen immer auf 1 ***/
                winp_JoySecAxes.x=-300;
            };
            if (winp_DI.JCaps.dwAxes > 3) winp_JoySecAxes.y=js.lRz;
            else {
                /*** falls kein Ruder, dieses auf 0 ***/
                winp_JoySecAxes.y=0;
            };

            /*** Joystick-Buttons ***/
            for (i=0; i<8; i++) {
                if (js.rgbButtons[i] & 0x80) {
                    /*** Button ist zu ***/
                    winp_JoyBtn |= (1<<i);
                    if ((old_joybtn & (1<<i)) == 0) winp_KeyDown(WINP_CODE_JB0+i);
                } else {
                    /*** Button ist offen ***/
                    if (old_joybtn & (1<<i)) winp_KeyUp(WINP_CODE_JB0+i);
                };
            };

            /*** POVs ***/
            if (winp_DI.JCaps.dwPOVs > 0) {
                if (LOWORD(js.rgdwPOV[0]) == 0xFFFF) {            
                    winp_JoyHatAxes.x = 0;
                    winp_JoyHatAxes.y = 0;
                }else{
                    /*** nur 4 Stellungen auswerten ***/
                    if ((js.rgdwPOV[0]>=31500) || (js.rgdwPOV[0]<4500)){
                        winp_JoyHatAxes.x = 0;
                        winp_JoyHatAxes.y = 300;
                    }else if ((js.rgdwPOV[0]>=4500) && (js.rgdwPOV[0]<13500)){
                        winp_JoyHatAxes.x = 300;
                        winp_JoyHatAxes.y = 0;
                    }else if ((js.rgdwPOV[0]>=13500) && (js.rgdwPOV[0]<22500)){
                        winp_JoyHatAxes.x = 0;
                        winp_JoyHatAxes.y = -300;
                    }else if ((js.rgdwPOV[0]>=22500) && (js.rgdwPOV[0]<31500)) {
                        winp_JoyHatAxes.x = -300;
                        winp_JoyHatAxes.y = 0;
                    };
                };
            } else {
                /*** kein POV ***/
                winp_JoyHatAxes.x = 0;
                winp_JoyHatAxes.y = 0;
            };
        };
    };
}

/*=================================================================**
**                                                                 **
**  ForceFeedback-Zeuch                                            **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
void winp_FFInitDefaultEffect(DIEFFECT   *diEffect,
                              DIENVELOPE *diEnvelope,
                              DWORD *rgdwAxes,
                              LONG  *rglDirection)
/*
**  FUNCTION
**      Initialisiert die übergebene Effects-Struktur mit
**      Default-Werten.
**
**  CHANGED
**      18-May-97   floh    created
*/
{
    diEnvelope->dwSize        = sizeof(DIENVELOPE);
    diEnvelope->dwAttackTime  = 50000;
    diEnvelope->dwAttackLevel = 0;
    diEnvelope->dwFadeTime    = 50000;
    diEnvelope->dwFadeLevel   = 0;

    rgdwAxes[0]  = DIJOFS_X;
    rgdwAxes[1]  = DIJOFS_Y;
    rglDirection[0] = 0;
    rglDirection[1] = 0;

    diEffect->dwSize         = sizeof(DIEFFECT);
    diEffect->dwFlags        = DIEFF_OBJECTOFFSETS | DIEFF_POLAR;
    diEffect->dwDuration     = 1000000;
    diEffect->dwSamplePeriod = 10000;
    diEffect->dwGain         = 10000;

    diEffect->dwTriggerButton         = DIEB_NOTRIGGER;
    diEffect->dwTriggerRepeatInterval = 0;

    diEffect->cAxes        = 2;
    diEffect->rgdwAxes     = rgdwAxes;
    diEffect->rglDirection = rglDirection;
    diEffect->lpEnvelope   = diEnvelope;
    diEffect->cbTypeSpecificParams  = 0;
    diEffect->lpvTypeSpecificParams = NULL;
}

/*-----------------------------------------------------------------*/
void winp_FFCreateTankEngine(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Tank-Engine ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.PeriodicTankEngine.dwMagnitude = 3000;
    winp_DI.FFData.PeriodicTankEngine.lOffset     = 0;
    winp_DI.FFData.PeriodicTankEngine.dwPhase     = 0;
    winp_DI.FFData.PeriodicTankEngine.dwPeriod    = 200000;    // Idle Engine (5 Hz)

    diEffect.dwDuration = INFINITE;
    diEffect.rglDirection[0] = 9000;
    diEffect.cbTypeSpecificParams  = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicTankEngine);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Sine, &diEffect, &(winp_DI.FFData.TankEngine), NULL);
}

/*-----------------------------------------------------------------*/
void winp_FFCreateJetEngine(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Jet-Engine ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.PeriodicJetEngine.dwMagnitude = 2300;
    winp_DI.FFData.PeriodicJetEngine.lOffset     = 0;
    winp_DI.FFData.PeriodicJetEngine.dwPhase     = 0;
    winp_DI.FFData.PeriodicJetEngine.dwPeriod    = 71500;

    diEffect.dwDuration     = INFINITE;
    diEffect.rglDirection[0] = 9000;
    diEffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicJetEngine);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Sine, &diEffect, &(winp_DI.FFData.JetEngine), NULL);
}

/*-----------------------------------------------------------------*/
void winp_FFCreateHeliEngine(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Heli-Engine ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.PeriodicHeliEngine.dwMagnitude = 4500;
    winp_DI.FFData.PeriodicHeliEngine.lOffset     = 0;
    winp_DI.FFData.PeriodicHeliEngine.dwPhase     = 0;
    winp_DI.FFData.PeriodicHeliEngine.dwPeriod    = 166666;

    diEffect.dwDuration     = INFINITE;
    diEffect.rglDirection[0] = 0;
    diEffect.cbTypeSpecificParams  = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicHeliEngine);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_SawtoothUp, &diEffect, &(winp_DI.FFData.HeliEngine), NULL);
}

/*-----------------------------------------------------------------*/
void winp_FFCreateMaxRot(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** MaxRot-Damper ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.CondMaxRot[0].lOffset              = 0;
    winp_DI.FFData.CondMaxRot[0].lPositiveCoefficient = 10000;
    winp_DI.FFData.CondMaxRot[0].lNegativeCoefficient = 10000;
    winp_DI.FFData.CondMaxRot[0].dwPositiveSaturation = 0;
    winp_DI.FFData.CondMaxRot[0].dwNegativeSaturation = 0;
    winp_DI.FFData.CondMaxRot[0].lDeadBand            = 0;
    winp_DI.FFData.CondMaxRot[1].lOffset              = 0;
    winp_DI.FFData.CondMaxRot[1].lPositiveCoefficient = 10000;
    winp_DI.FFData.CondMaxRot[1].lNegativeCoefficient = 10000;
    winp_DI.FFData.CondMaxRot[1].dwPositiveSaturation = 0;
    winp_DI.FFData.CondMaxRot[1].dwNegativeSaturation = 0;
    winp_DI.FFData.CondMaxRot[1].lDeadBand            = 0;

    diEffect.dwDuration = INFINITE;
    diEffect.lpEnvelope = NULL;
    diEffect.cbTypeSpecificParams  = sizeof(winp_DI.FFData.CondMaxRot);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.CondMaxRot);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Damper, &diEffect, &(winp_DI.FFData.MaxRot), NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputEffect","GetParameters()",dival);
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCreateMGun(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Machine Gun ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.PeriodicMGun.dwMagnitude = 5000;
    winp_DI.FFData.PeriodicMGun.lOffset     = 0;
    winp_DI.FFData.PeriodicMGun.dwPhase     = 0;
    winp_DI.FFData.PeriodicMGun.dwPeriod    = 83333;

    diEffect.dwDuration      = INFINITE;
    diEffect.rglDirection[0] = 0;
    diEffect.cbTypeSpecificParams  = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicMGun);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Square, &diEffect, &(winp_DI.FFData.MGun), NULL);
}

/*-----------------------------------------------------------------*/
void winp_FFCreateMissLaunch(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Missile Launch ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);

    winp_DI.FFData.PeriodicMissLaunch.dwMagnitude = 10000;
    winp_DI.FFData.PeriodicMissLaunch.lOffset     = 0;
    winp_DI.FFData.PeriodicMissLaunch.dwPhase     = 9000;
    winp_DI.FFData.PeriodicMissLaunch.dwPeriod    = 1000000; // 1 Hz

    diEnvelope.dwAttackTime  = 0;
    diEnvelope.dwAttackLevel = 10000;
    diEnvelope.dwFadeTime    = 264000;  // 44% bei 600ms Dauer
    diEnvelope.dwFadeLevel   = 0;

    diEffect.dwDuration = 600000;
    diEffect.cbTypeSpecificParams  = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicMissLaunch);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Sine,&diEffect,&(winp_DI.FFData.MissLaunch),NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputDevice2","CreateEffect()",dival);
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCreateGrenLaunch(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Grenade Launch ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);

    winp_DI.FFData.RampGrenLaunch.lStart = 10000;
    winp_DI.FFData.RampGrenLaunch.lEnd   = -10000;

    diEnvelope.dwAttackTime  = 0;
    diEnvelope.dwAttackLevel = 10000;
    diEnvelope.dwFadeTime    = 57000;  // 19% bei Duration 300 millisec
    diEnvelope.dwFadeLevel   = 0;

    diEffect.dwDuration = 300000;
    diEffect.cbTypeSpecificParams  = sizeof(DIRAMPFORCE);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.RampGrenLaunch);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_RampForce,&diEffect,&(winp_DI.FFData.GrenLaunch),NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputDevice2","CreateEffect()",dival);
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCreateBombLaunch(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Bomb Launch ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);

    winp_DI.FFData.RampBombLaunch.lStart = 8000;
    winp_DI.FFData.RampBombLaunch.lEnd   = -8000;

    diEffect.dwDuration = 400000;
    diEffect.lpEnvelope = NULL;
    diEffect.cbTypeSpecificParams  = sizeof(DIRAMPFORCE);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.RampBombLaunch);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_RampForce,&diEffect,&(winp_DI.FFData.BombLaunch),NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputDevice2","CreateEffect()",dival);
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCreateCollission(void)
/*
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Collission ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);

    winp_DI.FFData.RampCollission.lStart = 10000;
    winp_DI.FFData.RampCollission.lEnd   = -10000;

    diEffect.dwFlags    = DIEFF_OBJECTOFFSETS | DIEFF_CARTESIAN;
    diEffect.dwDuration = 95000;
    diEffect.lpEnvelope = NULL;
    diEffect.cbTypeSpecificParams  = sizeof(DIRAMPFORCE);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.RampCollission);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_RampForce,&diEffect,&(winp_DI.FFData.Collission),NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputDevice2","CreateEffect()",dival);
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCreateShake(void)
/*
**  FUNCTION
**      Der Shake-Effect besteht aus 2 sich überlagernden
**      Effekten.
**
**  CHANGED
**      21-May-97   floh    created
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    /*** Shake1: Square-Wave ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.PeriodicShake1.dwMagnitude = 10000;
    winp_DI.FFData.PeriodicShake1.lOffset     = 0;
    winp_DI.FFData.PeriodicShake1.dwPhase     = 0;
    winp_DI.FFData.PeriodicShake1.dwPeriod    = 71428;  // 14 Hz

    diEnvelope.dwAttackTime  = 0;
    diEnvelope.dwAttackLevel = 10000;
    diEnvelope.dwFadeTime    = 370000;  // 37% bei 1 sec Duration
    diEnvelope.dwFadeLevel   = 0;

    diEffect.dwFlags    = DIEFF_OBJECTOFFSETS | DIEFF_CARTESIAN;
    diEffect.dwDuration = 1000000;
    diEffect.cbTypeSpecificParams  = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicShake1);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Square, &diEffect, &(winp_DI.FFData.Shake1), NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputDevice2","CreateEffect()",dival);
    };

    /*** Shake2: Sine-Wave ***/
    winp_FFInitDefaultEffect(&diEffect,&diEnvelope,&rgdwAxes,&rglDirection);
    winp_DI.FFData.PeriodicShake2.dwMagnitude = 10000;  // variabel
    winp_DI.FFData.PeriodicShake2.lOffset     = 0;
    winp_DI.FFData.PeriodicShake2.dwPhase     = 0;
    winp_DI.FFData.PeriodicShake2.dwPeriod    = 166666; // 6 Hz

    diEnvelope.dwAttackTime  = 0;
    diEnvelope.dwAttackLevel = 10000;
    diEnvelope.dwFadeTime    = 630000;  // 63% bei 1 sec Duration
    diEnvelope.dwFadeLevel   = 0;

    diEffect.dwFlags = DIEFF_OBJECTOFFSETS | DIEFF_CARTESIAN;
    diEffect.dwDuration = 1000000;
    diEffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &(winp_DI.FFData.PeriodicShake2);
    dival = winp_DI.FFJoyst->lpVtbl->CreateEffect(winp_DI.FFJoyst,
            &GUID_Sine, &diEffect, &(winp_DI.FFData.Shake2), NULL);
    if (dival != DI_OK) {
        winp_FailMsg("DirectInputDevice2","CreateEffect()",dival);
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCreateEffects(void)
/*
**  FUNCTION
**      Erzeugt alle verwendeten Forcefeedback-Effekte.
**
**  CHANGED
**      19-May-97   floh    created
**      20-May-97   floh    + Damper-Effect
*/
{
    HRESULT    dival;
    DIEFFECT   diEffect;
    DIENVELOPE diEnvelope;
    DWORD      rgdwAxes[2];
    LONG       rglDirection[2];

    memset(&(winp_DI.FFData),0,sizeof(winp_DI.FFData));
    winp_FFCreateTankEngine();
    winp_FFCreateJetEngine();
    winp_FFCreateHeliEngine();
    winp_FFCreateMaxRot();
    winp_FFCreateMGun();
    winp_FFCreateMissLaunch();
    winp_FFCreateGrenLaunch();
    winp_FFCreateBombLaunch();
    winp_FFCreateCollission();
    winp_FFCreateShake();
}

/*-----------------------------------------------------------------*/
/*
**  FUNCTION
**      Destroy-Funktionen für die einzelnen Effekt-Objekte.
**
**  CHANGED
**      20-May-97   floh    created
*/
void winp_FFDestroyTankEngine(void)
{
    if (winp_DI.FFData.TankEngine) {
        winp_DI.FFData.TankEngine->lpVtbl->Release(winp_DI.FFData.TankEngine);
        winp_DI.FFData.TankEngine = NULL;
    };
}
void winp_FFDestroyJetEngine(void)
{
    if (winp_DI.FFData.JetEngine) {
        winp_DI.FFData.JetEngine->lpVtbl->Release(winp_DI.FFData.JetEngine);
        winp_DI.FFData.JetEngine = NULL;
    };
}
void winp_FFDestroyHeliEngine(void)
{
    if (winp_DI.FFData.HeliEngine) {
        winp_DI.FFData.HeliEngine->lpVtbl->Release(winp_DI.FFData.HeliEngine);
        winp_DI.FFData.HeliEngine = NULL;
    };
}
void winp_FFDestroyMaxRot(void)
{
    if (winp_DI.FFData.MaxRot) {
        winp_DI.FFData.MaxRot->lpVtbl->Release(winp_DI.FFData.MaxRot);
        winp_DI.FFData.MaxRot = NULL;
    };
}
void winp_FFDestroyMGun(void)
{
    if (winp_DI.FFData.MGun) {
        winp_DI.FFData.MGun->lpVtbl->Release(winp_DI.FFData.MGun);
        winp_DI.FFData.MGun = NULL;
    };
}
void winp_FFDestroyMissLaunch(void)
{
    if (winp_DI.FFData.MissLaunch) {
        winp_DI.FFData.MissLaunch->lpVtbl->Release(winp_DI.FFData.MissLaunch);
        winp_DI.FFData.MissLaunch = NULL;
    };
}
void winp_FFDestroyGrenLaunch(void)
{
    if (winp_DI.FFData.GrenLaunch) {
        winp_DI.FFData.GrenLaunch->lpVtbl->Release(winp_DI.FFData.GrenLaunch);
        winp_DI.FFData.GrenLaunch = NULL;
    };
}
void winp_FFDestroyBombLaunch(void)
{
    if (winp_DI.FFData.BombLaunch) {
        winp_DI.FFData.BombLaunch->lpVtbl->Release(winp_DI.FFData.BombLaunch);
        winp_DI.FFData.BombLaunch = NULL;
    };
}
void winp_FFDestroyCollission(void)
{
    if (winp_DI.FFData.Collission) {
        winp_DI.FFData.Collission->lpVtbl->Release(winp_DI.FFData.Collission);
        winp_DI.FFData.Collission = NULL;
    };
}
void winp_FFDestroyShake(void)
{
    if (winp_DI.FFData.Shake1) {
        winp_DI.FFData.Shake1->lpVtbl->Release(winp_DI.FFData.Shake1);
        winp_DI.FFData.Shake1 = NULL;
    };
    if (winp_DI.FFData.Shake2) {
        winp_DI.FFData.Shake2->lpVtbl->Release(winp_DI.FFData.Shake2);
        winp_DI.FFData.Shake2 = NULL;
    };
}

/*-----------------------------------------------------------------*/
void winp_FFKillEffects(void)
/*
**  FUNCTION
**      Killt alle in winp_FFCreateEffects() erzeugten
**      ForceFeedback-Effekte.
**
**  CHANGED
**      19-May-97   floh    created
*/
{
    winp_FFDestroyTankEngine();
    winp_FFDestroyJetEngine();
    winp_FFDestroyHeliEngine();
    winp_FFDestroyMaxRot();
    winp_FFDestroyMGun();
    winp_FFDestroyMissLaunch();
    winp_FFDestroyGrenLaunch();
    winp_FFDestroyBombLaunch();
    winp_FFDestroyCollission();
    winp_FFDestroyShake();
    memset(&(winp_DI.FFData),0,sizeof(winp_DI.FFData));
}

/*-----------------------------------------------------------------*/
void winp_FFModifyMagnitudePeriod(LPDIRECTINPUTEFFECT eff,
                                  float mag, float period)
/*
**  FUNCTION
**      Modifiziert Magnitude und Periode eines Periodischen
**      Effekts.
**
**  CHANGED
**      19-May-97   floh    created
*/
{
    HRESULT dival = DI_OK;
    if (eff) {
        DIEFFECT   diEffect   = {sizeof(diEffect)};
        DIPERIODIC diPeriodic = {sizeof(diPeriodic)};
        diEffect.dwSize = sizeof(diEffect);
        diEffect.cbTypeSpecificParams  = sizeof(diPeriodic);
        diEffect.lpvTypeSpecificParams = &diPeriodic;
        dival = eff->lpVtbl->GetParameters(eff,&diEffect,DIEP_TYPESPECIFICPARAMS);
        if (dival == DI_OK) {
            diPeriodic.dwMagnitude = (DWORD) mag;
            diPeriodic.dwPeriod    = (DWORD) period;
            dival = eff->lpVtbl->SetParameters(eff, &diEffect, DIEP_TYPESPECIFICPARAMS);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","SetParameters()",dival);
            };
        } else {
            winp_FailMsg("DirectInputEffect","GetParameters()",dival);
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFModifyDamper(LPDIRECTINPUTEFFECT eff, float power)
/*
**  FUNCTION
**      Modifiziert den MaxRot-Damper.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    HRESULT dival;
    if (eff) {
        DIEFFECT diEffect          = {sizeof(diEffect)};
        DICONDITION diCondition[2] = {sizeof(diCondition)};
        diEffect.dwSize = sizeof(diEffect);
        diEffect.cbTypeSpecificParams  = sizeof(diCondition);
        diEffect.lpvTypeSpecificParams = &diCondition;
        dival = eff->lpVtbl->GetParameters(eff,&diEffect,DIEP_TYPESPECIFICPARAMS);
        if (dival == DI_OK) {
            diCondition[0].lPositiveCoefficient = (LONG) power;
            diCondition[0].lNegativeCoefficient = (LONG) power;
            diCondition[1].lPositiveCoefficient = (LONG) power;
            diCondition[1].lNegativeCoefficient = (LONG) power;
            dival = eff->lpVtbl->SetParameters(eff,&diEffect,DIEP_TYPESPECIFICPARAMS);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","SetParameters()",dival);
            };
        } else {
            winp_FailMsg("DirectInputEffect","GetParameters()",dival);
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFModifyCollission(LPDIRECTINPUTEFFECT eff,
                             float power, float dir_x, float dir_y)
/*
**  CHANGED
**      21-May-97   floh    created
*/
{
    if (eff) {
        HRESULT dival;
        DIEFFECT diEffect = {sizeof(diEffect)};
        LONG rglDirection[2];
        DIRAMPFORCE diRampForce = {sizeof(diRampForce)};

        diEffect.cbTypeSpecificParams  = sizeof(diRampForce);
        diEffect.lpvTypeSpecificParams = &diRampForce;
        dival = eff->lpVtbl->GetParameters(eff,&diEffect,DIEP_TYPESPECIFICPARAMS);
        if (dival == DI_OK) {
            diRampForce.lStart    = (LONG) (10000.0 * power);
            diRampForce.lEnd      = (LONG) (-10000.0 * power);
            diEffect.dwFlags      = DIEFF_OBJECTOFFSETS | DIEFF_CARTESIAN;
            diEffect.cAxes        = 2;
            diEffect.rgdwAxes     = 0;
            diEffect.rglDirection = rglDirection;
            rglDirection[0] = (LONG) (dir_x*1000.0);
            rglDirection[1] = (LONG) (dir_y*1000.0);
            dival = eff->lpVtbl->SetParameters(eff,&diEffect,DIEP_TYPESPECIFICPARAMS|DIEP_DIRECTION);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","SetParameters()",dival);
            };
        } else {
            winp_FailMsg("DirectInputEffect","GetParameters()",dival);
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFModifyShake(float power,float dur,float dir_x,float dir_y)
/*
**  CHANGED
**      21-May-97   floh    created
*/
{
    LPDIRECTINPUTEFFECT eff1 = winp_DI.FFData.Shake1;
    LPDIRECTINPUTEFFECT eff2 = winp_DI.FFData.Shake2;

    if (eff1 && eff2) {
        HRESULT dival;
        DIEFFECT   diEffect   = {sizeof(diEffect)};
        DIPERIODIC diPeriodic = {sizeof(diPeriodic)};
        DIENVELOPE diEnvelope = {sizeof(diEnvelope)};
        LONG rglDirection[2];

        /*** Shake1 ***/
        diEffect.cbTypeSpecificParams  = sizeof(diPeriodic);
        diEffect.lpvTypeSpecificParams = &diPeriodic;
        dival = eff1->lpVtbl->GetParameters(eff1,&diEffect,DIEP_TYPESPECIFICPARAMS);
        if (dival == DI_OK) {

            diPeriodic.dwMagnitude = (DWORD) (10000.0 * power);
            diPeriodic.lOffset     = 0;
            diPeriodic.dwPhase     = 0;
            diPeriodic.dwPeriod    = 71428;  // 14 Hz

            diEnvelope.dwAttackTime  = 0;
            diEnvelope.dwAttackLevel = (DWORD) (10000.0 * power);
            diEnvelope.dwFadeTime    = (DWORD) (dur * 0.37 * 1000.0);
            diEnvelope.dwFadeLevel   = 0;

            diEffect.dwFlags      = DIEFF_OBJECTOFFSETS | DIEFF_CARTESIAN;
            diEffect.dwDuration   = (DWORD) (dur * 0.63 * 1000.0);
            diEffect.cAxes        = 2;
            diEffect.rgdwAxes     = 0;
            diEffect.lpEnvelope   = &diEnvelope;
            diEffect.rglDirection = rglDirection;
            rglDirection[0] = (LONG) (dir_x*1000.0);
            rglDirection[1] = (LONG) (dir_y*1000.0);
            dival = eff1->lpVtbl->SetParameters(eff1,&diEffect,
                                                DIEP_TYPESPECIFICPARAMS|
                                                DIEP_DIRECTION|
                                                DIEP_DURATION|
                                                DIEP_ENVELOPE);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","SetParameters(Shake1)",dival);
            };
        };

        /*** Shake2 ***/
        // diEffect.cbTypeSpecificParams  = sizeof(diPeriodic);
        // diEffect.lpvTypeSpecificParams = &diPeriodic;
        // dival = eff2->lpVtbl->GetParameters(eff2,&diEffect,DIEP_TYPESPECIFICPARAMS);
        // if (dival == DI_OK) {
        //
        //     diPeriodic.dwMagnitude = (DWORD) (10000.0 * power);
        //     diPeriodic.lOffset     = 0;
        //     diPeriodic.dwPhase     = 0;
        //     diPeriodic.dwPeriod    = 166666;
        //
        //     diEnvelope.dwAttackTime  = 0;
        //     diEnvelope.dwAttackLevel = (DWORD) (10000.0 * power);
        //     diEnvelope.dwFadeTime    = (DWORD) (dur * 0.63 * 1000.0);  // 63%
        //     diEnvelope.dwFadeLevel   = 0;
        //
        //     diEffect.dwFlags      = DIEFF_OBJECTOFFSETS | DIEFF_CARTESIAN;
        //     diEffect.dwDuration   = (DWORD) (dur * 0.37 * 1000.0);
        //     diEffect.cAxes        = 2;
        //     diEffect.rgdwAxes     = 0;
        //     diEffect.lpEnvelope   = &diEnvelope;
        //     diEffect.rglDirection = rglDirection;
        //
        //     rglDirection[0] = (LONG) (-dir_y*1000.0);
        //     rglDirection[1] = (LONG) (dir_x*1000.0);
        //     dival = eff2->lpVtbl->SetParameters(eff2,&diEffect,
        //                                         DIEP_TYPESPECIFICPARAMS|
        //                                         DIEP_DIRECTION|
        //                                         DIEP_DURATION|
        //                                         DIEP_ENVELOPE);
        //     if (dival != DI_OK) {
        //         winp_FailMsg("DirectInputEffect","SetParameters(Shake2)",dival);
        //     };
        // };
    };
}

/*-----------------------------------------------------------------*/
HRESULT winp_FFStart(LPDIRECTINPUTEFFECT eff)
/*
**  FUNCTION
**      Startet einen Effekt. Wenn nicht downgeloadet, wird
**      dies transparent erledigt.
**
**  CHANGED
**      19-May-97   floh    created
*/
{
    HRESULT dival = DI_OK;
    if (eff) {
        dival = eff->lpVtbl->Start(eff,1,0);
        if (dival == DIERR_NOTDOWNLOADED) {
            dival = eff->lpVtbl->Download(eff);
            dival = eff->lpVtbl->Start(eff,1,0);
        };
    };
    return(dival);
}

/*-----------------------------------------------------------------*/
HRESULT winp_FFStop(LPDIRECTINPUTEFFECT eff)
/*
**  FUNCTION
**      Stoppt einen Effekt
**
**  CHANGED
**      19-May-97   floh    created
*/
{
    HRESULT dival = DI_OK;
    if (eff) dival = eff->lpVtbl->Stop(eff);
    return(dival);
}

/*-----------------------------------------------------------------*/
void winp_FFEngineTank(unsigned long mode, float power, float rpm)
/*
**  FUNCTION
**      Erzeugt, killt oder modifiziert den Tank-Engine-Forcefeedback-
**      Effekt. Der Effekt wird sofort gestartet, bzw. sofort
**      modifiziert. Ein vorher laufender Effekt muß NICHT
**      explizit beendet werden!
**
**  INPUTS
**      mode    -> 0 -> Effekt wird gestartet
**                 1 -> Effekt wird beendet
**                 2 -> Effekt wird modifiziert
**      power   -> wie "stark" ist der Motor (0.0 .. 1.0)
**      rpm     -> wie hoch dreht der Motor (0.0 .. 1.0)
**
**  CHANGED
**      18-May-97   floh    created
*/
{
    if (winp_DI.FFData.TankEngine) {
        HRESULT dival;
        if (mode == 0) {

            float magnitude,period;

            /*** Effekt starten (vorher andere Engine-Effekte ausschalten!) ***/
            winp_FFStop(winp_DI.FFData.TankEngine);
            winp_FFStop(winp_DI.FFData.JetEngine);
            winp_FFStop(winp_DI.FFData.HeliEngine);
            dival = winp_FFStart(winp_DI.FFData.TankEngine);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
            /*** und sofort modifizieren... ***/
            magnitude = 5000.0 * power;
            period    = 1000000.0 / (5.0 + ((18.0-5.0) * rpm));
            winp_FFModifyMagnitudePeriod(winp_DI.FFData.TankEngine,
                                         magnitude, period);
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.TankEngine);
        } else {
            /*** Effekt modifizieren ***/
            float magnitude = 6000.0 * power;
            float period    = 1000000.0 / (5.0 + ((18.0-5.0) * rpm));
            winp_FFModifyMagnitudePeriod(winp_DI.FFData.TankEngine,
                                         magnitude, period);

        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFEngineJet(unsigned long mode, float power, float rpm)
/*
**  FUNCTION
**      Siehe winp_FFEngineTank()
**
**  CHANGED
**      18-May-97   floh    created
*/
{
    if (winp_DI.FFData.JetEngine) {
        HRESULT dival;
        if (mode == 0) {

            float magnitude,period;

            /*** Effekt starten (vorher andere Engine-Effekte ausschalten!) ***/
            winp_FFStop(winp_DI.FFData.TankEngine);
            winp_FFStop(winp_DI.FFData.JetEngine);
            winp_FFStop(winp_DI.FFData.HeliEngine);
            dival = winp_FFStart(winp_DI.FFData.JetEngine);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
            /*** und sofort modifizieren... ***/
            magnitude = 6000.0 * power;
            period    = 1000000.0 / (14.0 + ((26.0-14.0) * rpm));
            winp_FFModifyMagnitudePeriod(winp_DI.FFData.JetEngine,
                                         magnitude, period);
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.JetEngine);
        } else {
            /*** Effekt modifizieren ***/
            float magnitude = 6000.0 * power;
            float period    = 1000000.0 / (14.0 + ((26.0-14.0) * rpm));
            winp_FFModifyMagnitudePeriod(winp_DI.FFData.JetEngine,
                                         magnitude, period);

        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFEngineHeli(unsigned long mode, float power, float rpm)
/*
**  FUNCTION
**      Siehe winp_FFEngineTank()
**
**  CHANGED
**      18-May-97   floh    created
*/
{
    if (winp_DI.FFData.HeliEngine) {
        HRESULT dival;
        if (mode == 0) {

            float magnitude,period;

            /*** Effekt starten (vorher andere Engine-Effekte ausschalten!) ***/
            winp_FFStop(winp_DI.FFData.TankEngine);
            winp_FFStop(winp_DI.FFData.JetEngine);
            winp_FFStop(winp_DI.FFData.HeliEngine);
            dival = winp_FFStart(winp_DI.FFData.HeliEngine);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
            /*** und sofort modifizieren... ***/
            magnitude = 8000.0 * power;
            period    = 1000000.0 / (6.0 + ((18.0-6.0) * rpm));
            winp_FFModifyMagnitudePeriod(winp_DI.FFData.HeliEngine,
                                         magnitude, period);
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.HeliEngine);
        } else {
            /*** Effekt modifizieren ***/
            float magnitude = 8000.0 * power;
            float period    = 1000000.0 / (6.0 + ((18.0-6.0) * rpm));
            winp_FFModifyMagnitudePeriod(winp_DI.FFData.HeliEngine,
                                         magnitude, period);

        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFMaxRot(unsigned long mode, float power)
/*
**  FUNCTION
**      Der "MaxRot-Damper", macht die Bewegung des Joysticks
**      schwerer oder einfacher, je nach Vehikel-Wendigkeit.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.MaxRot) {
        HRESULT dival;
        if (mode == 0) {

            /*** Effekt starten (vorherigen Effekt auschalten!) ***/
            winp_FFStop(winp_DI.FFData.MaxRot);
            winp_FFModifyDamper(winp_DI.FFData.MaxRot, 10000.0*power);
            dival = winp_FFStart(winp_DI.FFData.MaxRot);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.MaxRot);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFMGun(unsigned long mode)
/*
**  FUNCTION
**      Das Machine-Gun gestaltet sich relativ einfach.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.MGun) {
        HRESULT dival;
        if (mode == 0) {
            /*** Effekt starten (vorherigen Effekt auschalten!) ***/
            winp_FFStop(winp_DI.FFData.MGun);
            dival = winp_FFStart(winp_DI.FFData.MGun);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.MGun);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFMissLaunch(unsigned long mode, float power, float speed)
/*
**  FUNCTION
**      Stärkere (power) Raketen ergeben stärkere Magnitude,
**      schnellere Raketen ergeben eine kürzere Dauer
**      (wird allerdings nicht ausgewertet hähä)
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.MissLaunch) {
        HRESULT dival;
        if (mode == 0) {
            /*** Missile starten, alle Waffen-Effekte aus ***/
            winp_FFStop(winp_DI.FFData.MissLaunch);
            winp_FFStop(winp_DI.FFData.GrenLaunch);
            winp_FFStop(winp_DI.FFData.BombLaunch);
            dival = winp_FFStart(winp_DI.FFData.MissLaunch);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.MissLaunch);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFGrenLaunch(unsigned long mode, float power, float speed)
/*
**  FUNCTION
**      Granaten-Abschuß: <power> und <speed> wird nicht ausgewertet.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.GrenLaunch) {
        HRESULT dival;
        if (mode == 0) {
            /*** Grenade starten, alle Waffen-Effekte aus ***/
            winp_FFStop(winp_DI.FFData.MissLaunch);
            winp_FFStop(winp_DI.FFData.GrenLaunch);
            winp_FFStop(winp_DI.FFData.BombLaunch);
            dival = winp_FFStart(winp_DI.FFData.GrenLaunch);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.GrenLaunch);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFBombLaunch(unsigned long mode, float power, float speed)
/*
**  FUNCTION
**      Bomben-Abschuß: <power> und <speed> wird nicht ausgewertet.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.BombLaunch) {
        HRESULT dival;
        if (mode == 0) {
            /*** Grenade starten, alle Waffen-Effekte aus ***/
            winp_FFStop(winp_DI.FFData.MissLaunch);
            winp_FFStop(winp_DI.FFData.GrenLaunch);
            winp_FFStop(winp_DI.FFData.BombLaunch);
            dival = winp_FFStart(winp_DI.FFData.BombLaunch);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.BombLaunch);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFCollission(unsigned long mode, float power, float dir_x, float dir_y)
/*
**  FUNCTION
**      Kollission, <power> definiert Stärke, <dir_x> und <dir_y>
**      die Richtung.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.Collission) {
        HRESULT dival;
        if (mode == 0) {
            /*** Kollissions-Effekt starten ***/
            winp_FFStop(winp_DI.FFData.Collission);
            winp_FFModifyCollission(winp_DI.FFData.Collission,power,dir_x,dir_y);
            dival = winp_FFStart(winp_DI.FFData.Collission);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart()",dival);
            };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.Collission);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFShake(unsigned long mode, float power, float dur, float dir_x, float dir_y)
/*
**  FUNCTION
**      Kollission, <power> definiert Stärke, <duration> Dauer in
**      Millisec(!) , <dir_x> und <dir_y> die Richtung.
**
**  CHANGED
**      20-May-97   floh    created
*/
{
    if (winp_DI.FFData.Shake1 && winp_DI.FFData.Shake2) {
        HRESULT dival;
        if (mode == 0) {
            /*** Kollissions-Effekt starten ***/
            winp_FFStop(winp_DI.FFData.Shake1);
            winp_FFStop(winp_DI.FFData.Shake2);
            winp_FFModifyShake(power,dur,dir_x,dir_y);
            dival = winp_FFStart(winp_DI.FFData.Shake1);
            if (dival != DI_OK) {
                winp_FailMsg("DirectInputEffect","winp_FFStart(Shake1)",dival);
            };
            // dival = winp_FFStart(winp_DI.FFData.Shake2);
            // if (dival != DI_OK) {
            //     winp_FailMsg("DirectInputEffect","winp_FFStart(Shake2)",dival);
            // };
        } else if (mode == 1) {
            /*** Effekt beenden ***/
            dival = winp_FFStop(winp_DI.FFData.Shake1);
            // dival = winp_FFStop(winp_DI.FFData.Shake2);
        } else {
            // MODIFY NOT SUPPORTED
        };
    };
}

/*-----------------------------------------------------------------*/
void winp_FFStopAll(void)
/*
**  FUNCTION
**      Stoppt alle Effekte.
**
**  CHANGED
**      21-May-97   floh    created
*/
{
    winp_FFStop(winp_DI.FFData.TankEngine);
    winp_FFStop(winp_DI.FFData.JetEngine);
    winp_FFStop(winp_DI.FFData.HeliEngine);
    winp_FFStop(winp_DI.FFData.MaxRot);
    winp_FFStop(winp_DI.FFData.MGun);
    winp_FFStop(winp_DI.FFData.MissLaunch);
    winp_FFStop(winp_DI.FFData.GrenLaunch);
    winp_FFStop(winp_DI.FFData.BombLaunch);
    winp_FFStop(winp_DI.FFData.Collission);
    winp_FFStop(winp_DI.FFData.Shake1);
    winp_FFStop(winp_DI.FFData.Shake2);
}

/*-----------------------------------------------------------------*/

