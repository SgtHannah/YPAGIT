/*
**  $Source: PRG:VFM/Classes/_WinInpClass/winp_main.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 15:10:42 $
**  $Locker: floh $
**  $Author: floh $
**
**  Input-Treiber für Windows.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include "nucleus/nucleus2.h"
#include "bitmap/winddclass.h"  // wegen <struct win_DispEnv>
#include "input/winpclass.h"

extern ULONG winp_InitDirectInput(void);
extern void winp_KillDirectInput(void);
extern void winp_GetAbsMousePos(void);
extern void winp_GetJoystickState(void);

extern void winp_FFEngineTank(unsigned long mode, float power, float rpm);
extern void winp_FFEngineJet(unsigned long mode, float power, float rpm);
extern void winp_FFEngineHeli(unsigned long mode, float power, float rpm);
extern void winp_FFMaxRot(unsigned long mode, float power);
extern void winp_FFMGun(unsigned long mode);
extern void winp_FFMissLaunch(unsigned long mode, float power, float speed);
extern void winp_FFGrenLaunch(unsigned long mode, float power, float speed);
extern void winp_FFBombLaunch(unsigned long mode, float power, float speed);
extern void winp_FFCollission(unsigned long mode, float power, float dir_x, float dir_y);
extern void winp_FFShake(unsigned long mode, float power, float dur, float dir_x, float dir_y);
extern void winp_FFStopAll(void);

/*-----------------------------------------------------------------*/
_dispatcher(Object *, winp_OM_NEW, struct TagItem *);
_dispatcher(ULONG, winp_OM_DISPOSE, void *);
_dispatcher(void, winp_OM_SET, struct TagItem *);
_dispatcher(void, winp_OM_GET, struct TagItem *);

_dispatcher(void, winp_IDEVM_GETBUTTON, struct idev_status_msg *);
_dispatcher(void, winp_IDEVM_GETSLIDER, struct idev_status_msg *);
_dispatcher(void, winp_IDEVM_GETKEY, struct idev_getkey_msg *);
_dispatcher(ULONG, winp_IDEVM_SETID, struct idev_setid_msg *);
_dispatcher(ULONG, winp_IDEVM_SETHOTKEY, struct idev_sethotkey_msg *);
_dispatcher(void, winp_IDEVM_RESET, struct idev_reset_msg *);
_dispatcher(void, winp_IDEVM_QUERYHOTKEY, struct idev_queryhotkey_msg *);
_dispatcher(void, winp_IDEVM_FFCONTROL, struct idev_ffcontrol_msg *);

_dispatcher(ULONG, winp_IWIMPM_HASFOCUS, void *);
_dispatcher(void, winp_IWIMPM_GETCLICKINFO, struct ClickInfo *);

/*-----------------------------------------------------------------*/
struct idev_remap winp_RemapTable[] = {
    {"nop",     IDEVTYPE_BUTTON, KEYCODE_NOP,           0},
    {"esc",     IDEVTYPE_BUTTON, KEYCODE_ESCAPE,        0},
    {"space",   IDEVTYPE_BUTTON, KEYCODE_SPACEBAR,      0},
    {"up",      IDEVTYPE_BUTTON, KEYCODE_CURSOR_UP,     0},
    {"down",    IDEVTYPE_BUTTON, KEYCODE_CURSOR_DOWN,   0},
    {"left",    IDEVTYPE_BUTTON, KEYCODE_CURSOR_LEFT,   0},
    {"right",   IDEVTYPE_BUTTON, KEYCODE_CURSOR_RIGHT,  0},
    {"f1",      IDEVTYPE_BUTTON, KEYCODE_F1,            0},
    {"f2",      IDEVTYPE_BUTTON, KEYCODE_F2,            0},
    {"f3",      IDEVTYPE_BUTTON, KEYCODE_F3,            0},
    {"f4",      IDEVTYPE_BUTTON, KEYCODE_F4,            0},
    {"f5",      IDEVTYPE_BUTTON, KEYCODE_F5,            0},
    {"f6",      IDEVTYPE_BUTTON, KEYCODE_F6,            0},
    {"f7",      IDEVTYPE_BUTTON, KEYCODE_F7,            0},
    {"f8",      IDEVTYPE_BUTTON, KEYCODE_F8,            0},
    {"f9",      IDEVTYPE_BUTTON, KEYCODE_F9,            0},
    {"f10",     IDEVTYPE_BUTTON, KEYCODE_F10,           0},
    {"f11",     IDEVTYPE_BUTTON, KEYCODE_F11,           0},
    {"f12",     IDEVTYPE_BUTTON, KEYCODE_F12,           0},
    {"bs",      IDEVTYPE_BUTTON, KEYCODE_BS,            0},
    {"tab",     IDEVTYPE_BUTTON, KEYCODE_TAB,           0},
    {"clear",   IDEVTYPE_BUTTON, KEYCODE_CLEAR,         0},
    {"return",  IDEVTYPE_BUTTON, KEYCODE_RETURN,        0},
    {"ctrl",    IDEVTYPE_BUTTON, KEYCODE_CTRL,          0},
    {"rshift",  IDEVTYPE_BUTTON, KEYCODE_RSHIFT,        0},
    {"lshift",  IDEVTYPE_BUTTON, KEYCODE_LSHIFT,        0},
    {"shift",   IDEVTYPE_BUTTON, KEYCODE_LSHIFT,        0},
    {"alt",     IDEVTYPE_BUTTON, KEYCODE_LALT,          0},
    {"pause",   IDEVTYPE_BUTTON, KEYCODE_PAUSE,         0},
    {"pageup",  IDEVTYPE_BUTTON, KEYCODE_PAGEUP,        0},
    {"pagedown",IDEVTYPE_BUTTON, KEYCODE_PAGEDOWN,      0},
    {"end",     IDEVTYPE_BUTTON, KEYCODE_END,           0},
    {"home",    IDEVTYPE_BUTTON, KEYCODE_HOME,          0},
    {"select",  IDEVTYPE_BUTTON, KEYCODE_SELECT,        0},
    {"execute", IDEVTYPE_BUTTON, KEYCODE_EXECUTE,       0},
    {"snapshot",IDEVTYPE_BUTTON, KEYCODE_SNAPSHOT,      0},
    {"ins",     IDEVTYPE_BUTTON, KEYCODE_INS,           0},
    {"del",     IDEVTYPE_BUTTON, KEYCODE_DEL,           0},
    {"help",    IDEVTYPE_BUTTON, KEYCODE_HELP,          0},
    {"1",       IDEVTYPE_BUTTON, KEYCODE_1,             0},
    {"2",       IDEVTYPE_BUTTON, KEYCODE_2,             0},
    {"3",       IDEVTYPE_BUTTON, KEYCODE_3,             0},
    {"4",       IDEVTYPE_BUTTON, KEYCODE_4,             0},
    {"5",       IDEVTYPE_BUTTON, KEYCODE_5,             0},
    {"6",       IDEVTYPE_BUTTON, KEYCODE_6,             0},
    {"7",       IDEVTYPE_BUTTON, KEYCODE_7,             0},
    {"8",       IDEVTYPE_BUTTON, KEYCODE_8,             0},
    {"9",       IDEVTYPE_BUTTON, KEYCODE_9,             0},
    {"0",       IDEVTYPE_BUTTON, KEYCODE_0,             0},
    {"a",       IDEVTYPE_BUTTON, KEYCODE_A,             0},
    {"b",       IDEVTYPE_BUTTON, KEYCODE_B,             0},
    {"c",       IDEVTYPE_BUTTON, KEYCODE_C,             0},
    {"d",       IDEVTYPE_BUTTON, KEYCODE_D,             0},
    {"e",       IDEVTYPE_BUTTON, KEYCODE_E,             0},
    {"f",       IDEVTYPE_BUTTON, KEYCODE_F,             0},
    {"g",       IDEVTYPE_BUTTON, KEYCODE_G,             0},
    {"h",       IDEVTYPE_BUTTON, KEYCODE_H,             0},
    {"i",       IDEVTYPE_BUTTON, KEYCODE_I,             0},
    {"j",       IDEVTYPE_BUTTON, KEYCODE_J,             0},
    {"k",       IDEVTYPE_BUTTON, KEYCODE_K,             0},
    {"l",       IDEVTYPE_BUTTON, KEYCODE_L,             0},
    {"m",       IDEVTYPE_BUTTON, KEYCODE_M,             0},
    {"n",       IDEVTYPE_BUTTON, KEYCODE_N,             0},
    {"o",       IDEVTYPE_BUTTON, KEYCODE_O,             0},
    {"p",       IDEVTYPE_BUTTON, KEYCODE_P,             0},
    {"q",       IDEVTYPE_BUTTON, KEYCODE_Q,             0},
    {"r",       IDEVTYPE_BUTTON, KEYCODE_R,             0},
    {"s",       IDEVTYPE_BUTTON, KEYCODE_S,             0},
    {"t",       IDEVTYPE_BUTTON, KEYCODE_T,             0},
    {"u",       IDEVTYPE_BUTTON, KEYCODE_U,             0},
    {"v",       IDEVTYPE_BUTTON, KEYCODE_V,             0},
    {"w",       IDEVTYPE_BUTTON, KEYCODE_W,             0},
    {"x",       IDEVTYPE_BUTTON, KEYCODE_X,             0},
    {"y",       IDEVTYPE_BUTTON, KEYCODE_Y,             0},
    {"z",       IDEVTYPE_BUTTON, KEYCODE_Z,             0},
    {"num0",    IDEVTYPE_BUTTON, KEYCODE_NUM_0,         0},
    {"num1",    IDEVTYPE_BUTTON, KEYCODE_NUM_1,         0},
    {"num2",    IDEVTYPE_BUTTON, KEYCODE_NUM_2,         0},
    {"num3",    IDEVTYPE_BUTTON, KEYCODE_NUM_3,         0},
    {"num4",    IDEVTYPE_BUTTON, KEYCODE_NUM_4,         0},
    {"num5",    IDEVTYPE_BUTTON, KEYCODE_NUM_5,         0},
    {"num6",    IDEVTYPE_BUTTON, KEYCODE_NUM_6,         0},
    {"num7",    IDEVTYPE_BUTTON, KEYCODE_NUM_7,         0},
    {"num8",    IDEVTYPE_BUTTON, KEYCODE_NUM_8,         0},
    {"num9",    IDEVTYPE_BUTTON, KEYCODE_NUM_9,         0},
    {"nummul",  IDEVTYPE_BUTTON, KEYCODE_NUM_MUL,       0},
    {"numplus", IDEVTYPE_BUTTON, KEYCODE_NUM_PLUS,      0},
    {"numdot",  IDEVTYPE_BUTTON, KEYCODE_NUM_DOT,       0},
    {"numminus",IDEVTYPE_BUTTON, KEYCODE_NUM_MINUS,     0},
    {"enter",   IDEVTYPE_BUTTON, KEYCODE_ENTER,         0},
    {"numdiv",  IDEVTYPE_BUTTON, KEYCODE_NUM_DIV,       0},
    {"extra1",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_1,       0},
    {"extra2",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_2,       0},
    {"extra3",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_3,       0},
    {"extra4",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_4,       0},
    {"extra5",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_5,       0},
    {"extra6",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_6,       0},
    {"extra7",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_7,       0},
    {"extra8",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_8,       0},
    {"extra9",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_9,       0},
    {"extra10",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_10,     0},
    {"extra11",  IDEVTYPE_BUTTON, KEYCODE_EXTRA_11,     0},

    /*** Mouse-Buttons ***/
    {"lmb",    IDEVTYPE_BUTTON, WINP_CODE_LMB, 0},
    {"rmb",    IDEVTYPE_BUTTON, WINP_CODE_RMB, 0},
    {"mmb",    IDEVTYPE_BUTTON, WINP_CODE_MMB, 0},

    /*** Mouse-Sliders ***/
    {"mousex", IDEVTYPE_SLIDER, WINP_CODE_MX, 0},
    {"mousey", IDEVTYPE_SLIDER, WINP_CODE_MY, 0},

    /*** Joystick-Buttons ***/
    {"joyb0", IDEVTYPE_BUTTON, WINP_CODE_JB0, 0},
    {"joyb1", IDEVTYPE_BUTTON, WINP_CODE_JB1, 0},
    {"joyb2", IDEVTYPE_BUTTON, WINP_CODE_JB2, 0},
    {"joyb3", IDEVTYPE_BUTTON, WINP_CODE_JB3, 0},
    {"joyb4", IDEVTYPE_BUTTON, WINP_CODE_JB4, 0},
    {"joyb5", IDEVTYPE_BUTTON, WINP_CODE_JB5, 0},
    {"joyb6", IDEVTYPE_BUTTON, WINP_CODE_JB6, 0},
    {"joyb7", IDEVTYPE_BUTTON, WINP_CODE_JB7, 0},

    /*** Mouse-Achsen ***/
    {"joyx",        IDEVTYPE_SLIDER, WINP_CODE_JX,       0},
    {"joyy",        IDEVTYPE_SLIDER, WINP_CODE_JY,       0},
    {"joythrottle", IDEVTYPE_SLIDER, WINP_CODE_THROTTLE, 0},
    {"joyhatx",     IDEVTYPE_SLIDER, WINP_CODE_HATX,     0},
    {"joyhaty",     IDEVTYPE_SLIDER, WINP_CODE_HATY,     0},
    {"joyrudder",   IDEVTYPE_SLIDER, WINP_CODE_RUDDER,   0},

    /*** Terminator ***/
    {NULL, IDEVTYPE_NONE, -1, 0, },
};

/*** Lookup-Table für [RawKeyCode -> RemapTableIndex] ***/
UBYTE winp_RK2IX[256];

/*** Original-Größe des Back-Buffers ***/
LONG winp_SizeX;
LONG winp_SizeY;

/*** Input Status Global Data ***/
#define SIZEOF_ASCIIKEYBUF (8)

ULONG winp_HasFocus;
ULONG winp_ContKey;
ULONG winp_NormKey;
ULONG winp_AsciiKey[SIZEOF_ASCIIKEYBUF];
ULONG winp_MaxAsciiKeyIndex;
ULONG winp_ActAsciiKeyIndex;

ULONG winp_LmbIsDown, winp_RmbIsDown, winp_MmbIsDown;
WORD winp_LmbDownCnt, winp_RmbDownCnt, winp_MmbDownCnt;
WORD winp_LmbUpCnt, winp_RmbUpCnt, winp_MmbUpCnt;
ULONG winp_LmbDblClck;

struct winp_xy winp_ScreenMousePos;
struct winp_xy winp_WinMousePos;
struct winp_xy winp_LmbDownPos;
struct winp_xy winp_RmbDownPos;
struct winp_xy winp_MmbDownPos;
struct winp_xy winp_LmbUpPos;
struct winp_xy winp_RmbUpPos;
struct winp_xy winp_MmbUpPos;
struct winp_xy winp_JoyPrimAxes;
struct winp_xy winp_JoySecAxes;
struct winp_xy winp_JoyHatAxes;
ULONG winp_JoyBtn;

/*-----------------------------------------------------------------*/
ULONG winp_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo winp_clinfo;

struct ClassInfo *winp_MakeClass(ULONG id,...);
BOOL winp_FreeClass(void);

struct GET_Class winp_GET = {
    winp_MakeClass,
    winp_FreeClass,
};

struct GET_Class *winp_Entry(void)
{
    return(&winp_GET);
}

struct SegInfo winp_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/winp.class"
    },
    winp_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *winp_MakeClass(ULONG id,...)
/*
**  CHANGED
**      15-Nov-96   floh    created
**      04-Feb-97   floh    + benutzt DirectInput fuer Delta-MouseMove-Handling
**      18-May-97   floh    + IDEVM_FFCONTROL
*/
{
    ULONG i;

    /*** DirectInput initialisieren ***/
    if (!winp_InitDirectInput()) return(NULL);

    /*** RawKey2Index-Lookup-Table ausfüllen ***/
    memset(winp_RK2IX,0xff,sizeof(winp_RK2IX));
    for (i=0; i<256; i++) {
        ULONG j=0;
        while (winp_RemapTable[j].type != IDEVTYPE_NONE) {
            if (winp_RemapTable[j].code == i) {
                /*** wegen möglicher Doppelbelegung... ***/
                if (winp_RK2IX[i] == 0xff) {
                    /*** Slot war noch frei... ***/
                    winp_RK2IX[i] = j;
                };
            };
            j++;
        };
    };

    /*** Input Status Data initialisieren ***/
    winp_SizeX = 0;
    winp_SizeY = 0;

    winp_ContKey = 0;
    winp_NormKey = 0;
    winp_JoyBtn  = 0;
    memset(winp_AsciiKey,0,sizeof(winp_AsciiKey));
    winp_ActAsciiKeyIndex = 0;
    winp_MaxAsciiKeyIndex = SIZEOF_ASCIIKEYBUF;

    winp_LmbIsDown  = FALSE;
    winp_RmbIsDown  = FALSE;
    winp_MmbIsDown  = FALSE;
    winp_LmbDblClck = FALSE;

    winp_LmbDownCnt = winp_LmbUpCnt = 0;
    winp_RmbDownCnt = winp_RmbUpCnt = 0;
    winp_MmbDownCnt = winp_MmbUpCnt = 0;

    memset(&winp_ScreenMousePos,0,sizeof(struct winp_xy));
    memset(&winp_WinMousePos,0,sizeof(struct winp_xy));
    memset(&winp_LmbDownPos,0,sizeof(struct winp_xy));
    memset(&winp_RmbDownPos,0,sizeof(struct winp_xy));
    memset(&winp_MmbDownPos,0,sizeof(struct winp_xy));
    memset(&winp_LmbUpPos,0,sizeof(struct winp_xy));
    memset(&winp_RmbUpPos,0,sizeof(struct winp_xy));
    memset(&winp_MmbUpPos,0,sizeof(struct winp_xy));

    /*** Klasse initialisieren ***/
    memset(winp_Methods,0,sizeof(winp_Methods));

    winp_Methods[OM_NEW]     = (ULONG) winp_OM_NEW;
    winp_Methods[OM_DISPOSE] = (ULONG) winp_OM_DISPOSE;
    winp_Methods[OM_SET]     = (ULONG) winp_OM_SET;
    winp_Methods[OM_GET]     = (ULONG) winp_OM_GET;

    winp_Methods[IDEVM_GETBUTTON]   = (ULONG) winp_IDEVM_GETBUTTON;
    winp_Methods[IDEVM_GETSLIDER]   = (ULONG) winp_IDEVM_GETSLIDER;
    winp_Methods[IDEVM_GETKEY]      = (ULONG) winp_IDEVM_GETKEY;
    winp_Methods[IDEVM_SETID]       = (ULONG) winp_IDEVM_SETID;
    winp_Methods[IDEVM_SETHOTKEY]   = (ULONG) winp_IDEVM_SETHOTKEY;
    winp_Methods[IDEVM_RESET]       = (ULONG) winp_IDEVM_RESET;
    winp_Methods[IDEVM_QUERYHOTKEY] = (ULONG) winp_IDEVM_QUERYHOTKEY;
    winp_Methods[IDEVM_FFCONTROL]   = (ULONG) winp_IDEVM_FFCONTROL;

    winp_Methods[IWIMPM_HASFOCUS]     = (ULONG) winp_IWIMPM_HASFOCUS;
    winp_Methods[IWIMPM_GETCLICKINFO] = (ULONG) winp_IWIMPM_GETCLICKINFO;

    winp_clinfo.superclassid = IWIMP_CLASSID;
    winp_clinfo.methods      = winp_Methods;
    winp_clinfo.instsize     = sizeof(struct winp_data);
    winp_clinfo.flags        = 0;

    return(&winp_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL winp_FreeClass(void)
/*
**  CHANGED
**      15-Nov-96   floh    created
**      04-Feb-97   floh    killt DirectInput
*/
{
    winp_KillDirectInput();
    return(TRUE);
}

/*=================================================================**
**  SUPPORT FUNKTIONEN                                             **
**=================================================================*/
/*-----------------------------------------------------------------*/
void winp_KeyDown(ULONG key)
/*
**  FUNCTION
**      Updated den Button-Status der Key in der
**      winp_RemapTable[] bei KeyDown.
**
**  CHANGED
**      18-Nov-96   floh    created
**      18-Feb-98   floh    + übernimmt jetzt auch winp_ContKey
**                            und winp_UpKey Handling
*/
{
    if (winp_RK2IX[key] != 0xff) {
        winp_RemapTable[winp_RK2IX[key]].status = TRUE;
    };
    winp_ContKey = key;
    winp_NormKey = key;
}

/*-----------------------------------------------------------------*/
void winp_KeyUp(ULONG key)
/*
**  FUNCTION
**      Updated den Button-Status der Key in der
**      winp_RemapTable[] bei KeyUp.
**
**  CHANGED
**      18-Nov-96   floh    created
**      18-Feb-98   floh    + übernimmt jetzt auch winp_ContKey
**                            und winp_UpKey Handling
*/
{
    if (winp_RK2IX[key] != 0xff) {
        winp_RemapTable[winp_RK2IX[key]].status = FALSE;
    };
    if (winp_ContKey == key) winp_ContKey = 0;
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, winp_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      15-Nov-96   floh    created
*/
{
    Object *newo;
    struct winp_data *wid;
    struct win_DispEnv *wde;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);
    wid = INST_DATA(cl,newo);

    /*** momentan auf kein Element gemappt ***/
    wid->remap_index = -1;

    /*** IWIMPA_Environment ist <HWND> ***/
    wde = (struct win_DispEnv *) _GetTagData(IWIMPA_Environment,NULL,attrs);
    if (wde) {
        wid->hwnd  = wde->hwnd;
        winp_SizeX = wde->x_size;
        winp_SizeY = wde->y_size;
        winp_PlugIn(wid->hwnd);
    };

    /*** das war's schon ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, winp_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      18-Nov-96   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    if (wid->hwnd) winp_UnPlug(wid->hwnd);
    return(_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      15-Nov-96   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    ULONG data;

    /*** IWIMPA_Environment ***/
    data = _GetTagData(IWIMPA_Environment,NULL,attrs);
    if (data) {
        struct win_DispEnv *wde = (struct win_DispEnv *) data;
        if (wid->hwnd) winp_UnPlug(wid->hwnd);
        wid->hwnd  = wde->hwnd;
        winp_SizeX = wde->x_size;
        winp_SizeY = wde->y_size;
        winp_PlugIn(wid->hwnd);
    };

    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      15-Nov-96   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    struct TagItem *ti;

    /*** IWIMPA_Environment ***/
    if (ti = _FindTagItem(IWIMPA_Environment,attrs)) {
        *((ULONG *)ti->ti_Data) = wid->hwnd;
    };

    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, winp_IDEVM_SETID, struct idev_setid_msg *msg)
/*
**  CHANGED
**      15-Nov-96   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    ULONG i=0;

    /*** suche ID in Remap-Tabelle ***/
    while (winp_RemapTable[i].id != NULL) {
        if (stricmp(winp_RemapTable[i].id,msg->id)==0) {
            wid->remap_index = i;
            return(winp_RemapTable[i].type);
        };
        i++;
    };

    /*** keine passende ID gefunden ***/
    return(IDEVTYPE_NONE);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, winp_IDEVM_SETHOTKEY, struct idev_sethotkey_msg *msg)
/*
**  CHANGED
**      22-Nov-96   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    ULONG i=0;

    if (msg->hotkey >= IDEV_NUMHOTKEYS) return(IDEVTYPE_NONE);

    while (winp_RemapTable[i].id != NULL) {
        if (stricmp(winp_RemapTable[i].id, msg->id) == 0) {
            if (IDEVTYPE_BUTTON == winp_RemapTable[i].type) {
                wid->hotkeys[msg->hotkey] = winp_RemapTable[i].code;
                return(IDEVTYPE_HOTKEY);
            };
        };
        i++;
    };

    /*** unbekannte ID ***/
    return(IDEVTYPE_NONE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IDEVM_GETBUTTON, struct idev_status_msg *msg)
/*
**  CHANGED
**      15-Nov-96   floh    created
**      13-May-97   floh    + Joystick-Buttons
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    LONG ix = wid->remap_index;

    /*** Object bereits auf ein Element gemappt? ***/
    if (ix >= 0) {
        struct idev_remap *rm_slot = &(winp_RemapTable[ix]);

        /*** Non-Key-Buttons ***/
        switch(rm_slot->code) {
            case WINP_CODE_LMB:
                msg->btn_status = winp_LmbIsDown;
                break;
            case WINP_CODE_MMB:
                msg->btn_status = winp_MmbIsDown;
                break;
            case WINP_CODE_RMB:
                msg->btn_status = winp_RmbIsDown;
                break;
            case WINP_CODE_JB0:
            case WINP_CODE_JB1:
            case WINP_CODE_JB2:
            case WINP_CODE_JB3:
            case WINP_CODE_JB4:
            case WINP_CODE_JB5:
            case WINP_CODE_JB6:
            case WINP_CODE_JB7:
                {
                    ULONG jcode = rm_slot->code - WINP_CODE_JB0;
                    if (winp_JoyBtn & (1<<jcode)) {
                        msg->btn_status = TRUE;
                    } else {
                        msg->btn_status = FALSE;
                    };
                };
                break;
            default:
                msg->btn_status = rm_slot->status;
                break;
        };

    } else {
        msg->btn_status = FALSE;
    };
}

/*-----------------------------------------------------------------*/
LONG winp_GetSlider(LONG act_slider, LONG new_slider)
/*
**  CHANGED
**      15-May-98   floh    created
*/
{
    if (act_slider > new_slider) {
        act_slider -= WINP_MAX_SLIDER/8;
        if (act_slider < new_slider) act_slider=new_slider;
    } else if (act_slider < new_slider) {
        act_slider += WINP_MAX_SLIDER/8;
        if (act_slider > new_slider) act_slider=new_slider;
    };
    return(act_slider);
};

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IDEVM_GETSLIDER, struct idev_status_msg *msg)
/*
**  FUNCTION
**      Fragt einen der eingebauten Slider ab. Der interne Update
**      der Slider passiert einmalig pro Frame innerhalb
**      IWIMPM_GETCLICKINFO.
**
**  CHANGED
**      15-Nov-96   floh    created
**      13-May-97   floh    + Joystick-Achsen
**      03-Jun-97   floh    + Joystick-Rudder-Achse
**      15-May-98   floh    + Joystick-Sliders haben jetzt einen
**                            Gummiband-Effekt
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    LONG ix = wid->remap_index;

    /*** Object bereits auf ein Element gemappt? ***/
    if (ix >= 0) {

        LONG delta;
        BOOL mouse_changed = FALSE;
        ULONG rubber_band = TRUE;
        
        switch(winp_RemapTable[ix].code) {

            case WINP_CODE_MX:
                delta = winp_ScreenMousePos.x - wid->mouse_store;
                wid->mouse_store   = winp_ScreenMousePos.x;
                wid->mouse_slider += delta;
                mouse_changed = TRUE;
                if (delta != 0) rubber_band = FALSE; 
                break;

            case WINP_CODE_MY:
                delta = winp_ScreenMousePos.y - wid->mouse_store;
                wid->mouse_store   = winp_ScreenMousePos.y;
                wid->mouse_slider += delta;
                mouse_changed = TRUE;
                if (delta != 0) rubber_band = FALSE;
                break;

            case WINP_CODE_JX:
                wid->mouse_slider = winp_GetSlider(wid->mouse_slider,winp_JoyPrimAxes.x);
                rubber_band = FALSE;
                break;

            case WINP_CODE_JY:
                wid->mouse_slider = winp_JoyPrimAxes.y; // winp_GetSlider(wid->mouse_slider,winp_JoyPrimAxes.y);
                rubber_band = FALSE;
                break;

            case WINP_CODE_THROTTLE:
                wid->mouse_slider = winp_GetSlider(wid->mouse_slider,-winp_JoySecAxes.x);
                rubber_band = FALSE;
                break;

            case WINP_CODE_RUDDER:
                wid->mouse_slider = winp_GetSlider(wid->mouse_slider,winp_JoySecAxes.y);
                rubber_band = FALSE;
                break;

            case WINP_CODE_HATX:
                wid->mouse_slider = winp_JoyHatAxes.x; // winp_GetSlider(wid->mouse_slider,winp_JoyHatAxes.x);
                rubber_band = FALSE;
                break;

            case WINP_CODE_HATY:
                wid->mouse_slider = winp_JoyHatAxes.y; // winp_GetSlider(wid->mouse_slider,winp_JoyHatAxes.y);
                rubber_band = FALSE;
                break;

            /*** ein Button? ***/
            default:
                {
                    BOOL down = winp_RemapTable[ix].status;
                    if (down) {
                        wid->mouse_slider += (WINP_MAX_SLIDER/8);
                        rubber_band = FALSE;
                    };
                };
                break;
        };
        
        /*** Gummiband-Effekt ***/
        if (rubber_band) wid->mouse_slider = (wid->mouse_slider*8)/10;

        /*** Range-Check ***/
        if (wid->mouse_slider < (-WINP_MAX_SLIDER)) {
            wid->mouse_slider = -WINP_MAX_SLIDER;
        } else if (wid->mouse_slider > WINP_MAX_SLIDER) {
            wid->mouse_slider = WINP_MAX_SLIDER;
        };

        /*** Umwandlung in Slider-Wert ***/
        msg->sld_status = ((FLOAT)(wid->mouse_slider))/WINP_MAX_SLIDER;

    } else {
        msg->sld_status = 0.0;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IDEVM_RESET, struct idev_reset_msg *msg)
/*
**  CHANGED
**      15-Nov-96   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    if (msg->rtype == IRTYPE_CENTER) wid->mouse_slider=0;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IDEVM_GETKEY, struct idev_getkey_msg *msg)
/*
**  CHANGED
**      15-Nov-96   floh    created
**      22-Nov-96   floh    + Hotkey-Handling
*/
{
    struct winp_data *wid = INST_DATA(cl,o);

    msg->cont_key  = winp_ContKey;
    msg->norm_key  = winp_NormKey;
    msg->hot_key   = 0;
    msg->ascii_key = winp_AsciiKey[0];

    /*** AsciiKey-Puffer nachrücken ***/
    if (winp_ActAsciiKeyIndex > 0) {
        ULONG i;
        winp_ActAsciiKeyIndex--;
        for (i=0; i<winp_ActAsciiKeyIndex; i++) {
            winp_AsciiKey[i] = winp_AsciiKey[i+1];
        };
    };

    /*** Keys entprellen ***/
    winp_NormKey = 0;
    winp_AsciiKey[winp_ActAsciiKeyIndex] = 0;

    /*** Hotkey? ***/
    if (msg->norm_key != 0) {
        ULONG i;
        for (i=0; i<IDEV_NUMHOTKEYS; i++) {
            if (wid->hotkeys[i] == msg->norm_key) {
                msg->hot_key = (i|0x80);
                return;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IDEVM_QUERYHOTKEY, struct idev_queryhotkey_msg *msg)
/*
**  CHANGED
**      01-Apr-97   floh    created
*/
{
    struct winp_data *wid = INST_DATA(cl,o);
    ULONG i;

    if (msg->keycode != 0) {
        /*** Keycode -> Hotkey ***/
        for (i=0; i<IDEV_NUMHOTKEYS; i++) {
            if (wid->hotkeys[i] == msg->keycode) {
                msg->hotkey = (i|0x80);
                return;
            };
        };
    } else if (msg->hotkey != 0) {
        /*** Hotkey -> Keycode ***/
        LONG hkey = msg->hotkey & ~0x80;
        if (hkey < IDEV_NUMHOTKEYS) msg->keycode = wid->hotkeys[hkey];
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IDEVM_FFCONTROL, struct idev_ffcontrol_msg *msg)
/*
**  CHANGED
**      18-May-97   floh    created
*/
{
    switch(msg->type) {
        case IDEV_FFTYPE_ENGINE_TANK:
            winp_FFEngineTank(msg->mode,msg->power,msg->pitch);
            break;
        case IDEV_FFTYPE_ENGINE_PLANE:
            winp_FFEngineJet(msg->mode,msg->power,msg->pitch);
            break;
        case IDEV_FFTYPE_ENGINE_HELI:
            winp_FFEngineHeli(msg->mode,msg->power,msg->pitch);
            break;
        case IDEV_FFTYPE_MAXROT:
            winp_FFMaxRot(msg->mode, msg->power);
            break;
        case IDEV_FFTYPE_MGUN:
            winp_FFMGun(msg->mode);
            break;
        case IDEV_FFTYPE_MISSLAUNCH:
            winp_FFMissLaunch(msg->mode,msg->power,msg->pitch);
            break;
        case IDEV_FFTYPE_GRENLAUNCH:
            winp_FFGrenLaunch(msg->mode,msg->power,msg->pitch);
            break;
        case IDEV_FFTYPE_BOMBLAUNCH:
            winp_FFBombLaunch(msg->mode,msg->power,msg->pitch);
            break;
        case IDEV_FFTYPE_COLLISSION:
            winp_FFCollission(msg->mode,msg->power,msg->dir_x,msg->dir_y);
            break;
        case IDEV_FFTYPE_SHAKE:
            winp_FFShake(msg->mode,msg->power,msg->pitch,msg->dir_x,msg->dir_y);
            break;
        case IDEV_FFTYPE_ALL:
            winp_FFStopAll();
            break;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, winp_IWIMPM_HASFOCUS, void *nil)
/*
**  CHANGED
**      15-Nov-96   floh    created
**      21-Nov-96   floh    liefert vorerst immer TRUE zurück
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, winp_IWIMPM_GETCLICKINFO, struct ClickInfo *ci)
/*
**  CHANGED
**      15-Nov-96   floh    created
**      04-Feb-97   floh    winp_GetAbsMousePos() (fragt die absolute
**                          Mausposition fuer die Slider einmalig
**                          pro Frame ab (sonst muesste das jeder
**                          Mausslider fuer sich selbst tun.
**      01-Dec-97   floh    + zusaetzlicher Bounds-Check, ob die Mauskoordinaten
**                            ausserhalb des zulaessigen Bereichs liegen
**      19-Mar-98   floh    + Support für Doubleclicks
*/
{
    struct winp_data *wid = INST_DATA(cl,o);

    /*** lösche Mausbutton-Status ***/
    ci->flags = 0;

    if (wid->hwnd) {

        winp_GetAbsMousePos();
        winp_GetJoystickState();

        ci->act.scrx = winp_WinMousePos.x;
        ci->act.scry = winp_WinMousePos.y;

        if (winp_LmbIsDown)  ci->flags |= CIF_MOUSEHOLD;
        if (winp_MmbIsDown)  ci->flags |= CIF_MMOUSEHOLD;
        if (winp_RmbIsDown)  ci->flags |= CIF_RMOUSEHOLD;
        if (winp_LmbDblClck) ci->flags |= CIF_MOUSEDBLCLCK;
        winp_LmbDblClck = FALSE;

        /*** Mouse Up/Down Events ***/
        if (winp_LmbDownCnt > 0) {
            ci->flags      |= CIF_MOUSEDOWN;
            ci->down.scrx   = winp_LmbDownPos.x;
            ci->down.scry   = winp_LmbDownPos.y;
            winp_LmbDownCnt = 0;
        };
        if (winp_LmbUpCnt > 0) {
            ci->flags    |= CIF_MOUSEUP;
            ci->up.scrx   = winp_LmbUpPos.x;
            ci->up.scry   = winp_LmbUpPos.y;
            winp_LmbUpCnt = 0;
        };
        if (winp_RmbDownCnt > 0) {
            ci->flags |= CIF_RMOUSEDOWN;
            winp_RmbDownCnt = 0;
        };
        if (winp_RmbUpCnt > 0) {
            ci->flags |= CIF_RMOUSEUP;
            winp_RmbUpCnt = 0;
        };
        if (winp_MmbDownCnt > 0) {
            ci->flags |= CIF_MMOUSEDOWN;
            winp_MmbDownCnt = 0;
        };
        if (winp_MmbUpCnt > 0) {
            ci->flags |= CIF_MMOUSEUP;
            winp_MmbUpCnt = 0;
        };

        /*** Bound Checks ***/
        if (ci->act.scrx < 0)                 ci->act.scrx  = 0;
        else if (ci->act.scrx  >= winp_SizeX) ci->act.scrx  = winp_SizeX-1;
        if (ci->act.scry < 0)                 ci->act.scry  = 0;
        else if (ci->act.scry  >= winp_SizeY) ci->act.scry  = winp_SizeY-1;
        if (ci->down.scrx < 0)                ci->down.scrx = 0;
        else if (ci->down.scrx >= winp_SizeX) ci->down.scrx = winp_SizeX-1;
        if (ci->down.scry < 0)                ci->down.scry = 0;
        else if (ci->down.scry >= winp_SizeY) ci->down.scry = winp_SizeY-1;
        if (ci->up.scrx < 0)                  ci->up.scrx   = 0;
        else if (ci->up.scrx   >= winp_SizeX) ci->up.scrx   = winp_SizeX-1;
        if (ci->up.scry < 0)                  ci->up.scry   = 0;
        else if (ci->up.scry   >= winp_SizeY) ci->up.scry   = winp_SizeY-1;

        /*** nur, wenn alles OK, Methode weitergeben ***/
        _supermethoda(cl,o,IWIMPM_GETCLICKINFO,ci);
    };
}

