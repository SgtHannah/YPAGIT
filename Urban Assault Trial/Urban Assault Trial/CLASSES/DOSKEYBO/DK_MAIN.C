/*
**  $Source: PRG:VFM/Classes/_DosKeyboard/dk_main.c,v $
**  $Revision: 38.7 $
**  $Date: 1996/09/29 20:05:06 $
**  $Locker: floh $
**  $Author: floh $
**
**  Keyboard-Treiber für DOS.
**
**  ACHTUNG:
**  ~~~~~~~~
**  Muß unterm Watcom mit ss!=ds compiliert werden.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>

#include <dos.h>
#include <i86.h>
#include <bios.h>

#include "nucleus/nucleus2.h"
#include "input/dkeyclass.h"

/*-------------------------------------------------------------------
**  Prototypes
*/
void __interrupt __far vfm_kbd_int(void);

_dispatcher(Object *, dkey_OM_NEW, struct TagItem *);
_dispatcher(ULONG, dkey_IDEVM_SETID, struct idev_setid_msg *);
_dispatcher(void, dkey_IDEVM_GETBUTTON, struct idev_status_msg *);
_dispatcher(void, dkey_IDEVM_GETSLIDER, struct idev_status_msg *);
_dispatcher(void, dkey_IDEVM_GETKEY, struct idev_getkey_msg *);
_dispatcher(ULONG, dkey_IDEVM_SETHOTKEY, struct idev_sethotkey_msg *);
_dispatcher(void, dkey_IDEVM_QUERYHOTKEY, struct idev_queryhotkey_msg *);

/*-------------------------------------------------------------------
**  Global Data
*/

void (__interrupt __far *prev_kbd_int)() = NULL;   /* originaler Handler */

struct idev_remap dkey_RemapTable[] = {
    {"rshift", IDEVTYPE_BUTTON, KEYCODE_RSHIFT,        0},
    {"lshift", IDEVTYPE_BUTTON, KEYCODE_LSHIFT,        0},
    {"alt",    IDEVTYPE_BUTTON, KEYCODE_LALT,          0},
    {"enter",  IDEVTYPE_BUTTON, KEYCODE_RETURN,        0},
    {"left",   IDEVTYPE_BUTTON, KEYCODE_CURSOR_LEFT,   0},
    {"right",  IDEVTYPE_BUTTON, KEYCODE_CURSOR_RIGHT,  0},
    {"up",     IDEVTYPE_BUTTON, KEYCODE_CURSOR_UP,     0},
    {"down",   IDEVTYPE_BUTTON, KEYCODE_CURSOR_DOWN,   0},
    {"ctrl",   IDEVTYPE_BUTTON, KEYCODE_CTRL,          0},
    {"space",  IDEVTYPE_BUTTON, KEYCODE_SPACEBAR,      0},
    {"tab",    IDEVTYPE_BUTTON, KEYCODE_TAB,           0},
    {"del",    IDEVTYPE_BUTTON, KEYCODE_DEL,           0},
    {"esc",    IDEVTYPE_BUTTON, KEYCODE_ESCAPE,        0},
    {"bs",     IDEVTYPE_BUTTON, KEYCODE_BS,            0},
    {"f1",     IDEVTYPE_BUTTON, KEYCODE_F1,            0},
    {"f2",     IDEVTYPE_BUTTON, KEYCODE_F2,            0},
    {"f3",     IDEVTYPE_BUTTON, KEYCODE_F3,            0},
    {"f4",     IDEVTYPE_BUTTON, KEYCODE_F4,            0},
    {"f5",     IDEVTYPE_BUTTON, KEYCODE_F5,            0},
    {"f6",     IDEVTYPE_BUTTON, KEYCODE_F6,            0},
    {"f7",     IDEVTYPE_BUTTON, KEYCODE_F7,            0},
    {"f8",     IDEVTYPE_BUTTON, KEYCODE_F8,            0},
    {"f9",     IDEVTYPE_BUTTON, KEYCODE_F9,            0},
    {"f10",    IDEVTYPE_BUTTON, KEYCODE_F10,           0},
    {"1",      IDEVTYPE_BUTTON, KEYCODE_1,             0},
    {"2",      IDEVTYPE_BUTTON, KEYCODE_2,             0},
    {"3",      IDEVTYPE_BUTTON, KEYCODE_3,             0},
    {"4",      IDEVTYPE_BUTTON, KEYCODE_4,             0},
    {"5",      IDEVTYPE_BUTTON, KEYCODE_5,             0},
    {"6",      IDEVTYPE_BUTTON, KEYCODE_6,             0},
    {"7",      IDEVTYPE_BUTTON, KEYCODE_7,             0},
    {"8",      IDEVTYPE_BUTTON, KEYCODE_8,             0},
    {"9",      IDEVTYPE_BUTTON, KEYCODE_9,             0},
    {"0",      IDEVTYPE_BUTTON, KEYCODE_0,             0},
    {"a",      IDEVTYPE_BUTTON, KEYCODE_A,             0},
    {"b",      IDEVTYPE_BUTTON, KEYCODE_B,             0},
    {"c",      IDEVTYPE_BUTTON, KEYCODE_C,             0},
    {"d",      IDEVTYPE_BUTTON, KEYCODE_D,             0},
    {"e",      IDEVTYPE_BUTTON, KEYCODE_E,             0},
    {"f",      IDEVTYPE_BUTTON, KEYCODE_F,             0},
    {"g",      IDEVTYPE_BUTTON, KEYCODE_G,             0},
    {"h",      IDEVTYPE_BUTTON, KEYCODE_H,             0},
    {"i",      IDEVTYPE_BUTTON, KEYCODE_I,             0},
    {"j",      IDEVTYPE_BUTTON, KEYCODE_J,             0},
    {"k",      IDEVTYPE_BUTTON, KEYCODE_K,             0},
    {"l",      IDEVTYPE_BUTTON, KEYCODE_L,             0},
    {"m",      IDEVTYPE_BUTTON, KEYCODE_M,             0},
    {"n",      IDEVTYPE_BUTTON, KEYCODE_N,             0},
    {"o",      IDEVTYPE_BUTTON, KEYCODE_O,             0},
    {"p",      IDEVTYPE_BUTTON, KEYCODE_P,             0},
    {"q",      IDEVTYPE_BUTTON, KEYCODE_Q,             0},
    {"r",      IDEVTYPE_BUTTON, KEYCODE_R,             0},
    {"s",      IDEVTYPE_BUTTON, KEYCODE_S,             0},
    {"t",      IDEVTYPE_BUTTON, KEYCODE_T,             0},
    {"u",      IDEVTYPE_BUTTON, KEYCODE_U,             0},
    {"v",      IDEVTYPE_BUTTON, KEYCODE_V,             0},
    {"w",      IDEVTYPE_BUTTON, KEYCODE_W,             0},
    {"x",      IDEVTYPE_BUTTON, KEYCODE_X,             0},
    {"y",      IDEVTYPE_BUTTON, KEYCODE_YZ,            0},
    {"z",      IDEVTYPE_BUTTON, KEYCODE_ZY,            0},
    {"num0",   IDEVTYPE_BUTTON, KEYCODE_NUM_0,         0},
    {"num1",   IDEVTYPE_BUTTON, KEYCODE_NUM_1,         0 },
    {"num2",   IDEVTYPE_BUTTON, KEYCODE_NUM_2,         0 },
    {"num3",   IDEVTYPE_BUTTON, KEYCODE_NUM_3,         0 },
    {"num4",   IDEVTYPE_BUTTON, KEYCODE_NUM_4,         0 },
    {"num5",   IDEVTYPE_BUTTON, KEYCODE_NUM_5,         0 },
    {"num6",   IDEVTYPE_BUTTON, KEYCODE_NUM_6,         0 },
    {"num7",   IDEVTYPE_BUTTON, KEYCODE_NUM_7,         0 },
    {"num8",   IDEVTYPE_BUTTON, KEYCODE_NUM_8,         0 },
    {"num9",   IDEVTYPE_BUTTON, KEYCODE_NUM_9,         0 },

    /*** Terminator ***/
    { NULL, IDEVTYPE_NONE, -1, 0, },
};

/*** Lookup-Table für [RawKeyCode -> RemapTableIndex] ***/
UBYTE dkey_RK2IX[0x80];

LONG dkey_ContKey;
LONG dkey_NormKey;

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG dkey_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo dkey_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *MakeDKeyClass(ULONG id,...);
BOOL FreeDKeyClass(void);

struct GET_Class dkey_GET = {
    &MakeDKeyClass,
    &FreeDKeyClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *dkey_Entry(void)
{
    return(&dkey_GET);
};

struct SegInfo dkey_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/dkey.class"
    },
    dkey_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *MakeDKeyClass(ULONG id,...)
/*
**  CHANGED
**      10-Mar-96   floh    created
**                          + oops, die RawKey-2-Remap-Sache funktioniert
**                            nicht, wenn auf einem RawKey-Code
**                            mehrere Tasten liegen... Korrektur bei
**                            der Remap-Tabelle...
**      23-Mar-96   floh    + IDEVM_SETHOTKEY
**      29-Sep-96   floh    + IDEVM_GETSLIDER
*/
{
    LONG i,j;

    /*** Global Data initialisieren ***/
    dkey_ContKey = 0;
    dkey_NormKey = 0;

    /*** RawKey2Index-Lookup-Table ausfüllen ***/
    memset(dkey_RK2IX, 0xff, sizeof(dkey_RK2IX));
    for (i=0; i<0x80; i++) {

        /*** RAWKEY-Code in Remap-Tabelle suchen ***/
        j = 0;
        while (dkey_RemapTable[j].type != IDEVTYPE_NONE) {
            if (dkey_RemapTable[j].code == i) {
                /*** Slot noch frei? ***/
                if (dkey_RK2IX[i] == 0xff) {
                    dkey_RK2IX[i] = j;
                };
            };
            j++;
        };
    };

    /*** Interrupt 0x9 verbiegen ***/
    prev_kbd_int = _dos_getvect(0x9);
    _dos_setvect(0x9, vfm_kbd_int);

    /*** Klasse initialisieren ***/
    memset(dkey_Methods, 0, sizeof(dkey_Methods));

    dkey_Methods[OM_NEW]            = (ULONG) dkey_OM_NEW;
    dkey_Methods[IDEVM_SETID]       = (ULONG) dkey_IDEVM_SETID;
    dkey_Methods[IDEVM_GETBUTTON]   = (ULONG) dkey_IDEVM_GETBUTTON;
    dkey_Methods[IDEVM_GETSLIDER]   = (ULONG) dkey_IDEVM_GETSLIDER;
    dkey_Methods[IDEVM_GETKEY]      = (ULONG) dkey_IDEVM_GETKEY;
    dkey_Methods[IDEVM_SETHOTKEY]   = (ULONG) dkey_IDEVM_SETHOTKEY;
    dkey_Methods[IDEVM_QUERYHOTKEY] = (ULONG) dkey_IDEVM_QUERYHOTKEY;

    dkey_clinfo.superclassid = IDEV_CLASSID;
    dkey_clinfo.methods      = dkey_Methods;
    dkey_clinfo.instsize     = sizeof(struct dkey_data);
    dkey_clinfo.flags        = 0;

    return(&dkey_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL FreeDKeyClass(void)
/*
**  CHANGED
**      10-Mar-96   floh    created
*/
{
    /*** Keyboard-Interrupt wieder auf Original ***/
    if (prev_kbd_int) {
        _dos_setvect(0x9,prev_kbd_int);
    };

    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
void __interrupt __far vfm_kbd_int(void)
/*
**  FUNCTION
**      Das ist mein Keyboard-Interrupt-Handler. Der Keyboard-
**      Interrupt kommt jeweils, wenn eine Taste gedrückt oder
**      losgelassen wurde. Wenn eine Taste gedrückt wurde,
**      steht auf Port 0x60 deren Scancode, Bit 7 gelöscht.
**      Wenn eine Taste losgelassen wurde, steht auf Port 0x60
**      ebenfalls der Scancode, aber mit Bit 7 gesetzt...
**
**  INPUTS
**      Port 0x60 hält Tastatur-Scancode
**
**  CHANGED
**      27-Jan-95   floh    created (für input_ibm.engine)
**      10-Mar-96   floh    übernommen nach dkey.class
*/
{
    UBYTE scan_code;
    UBYTE rawkey;
    BOOL pressed;

    /*** was liegt an? ***/
    scan_code = inp(0x60);

    /*** Taste gedrückt, oder losgelassen? ***/
    if (scan_code & 0x80) pressed = FALSE;
    else                  pressed = TRUE;

    /*** "reinen" Tastencode rausfiltern ***/
    rawkey = scan_code & 0x7f;

    /*** Button-Status per Lookup-Table in Remap-Table schreiben ***/
    if (dkey_RK2IX[rawkey] != 0xff) {
        dkey_RemapTable[dkey_RK2IX[rawkey]].status = pressed;
    };

    /*** normales Tasten-Handling ***/
    if (pressed) {
        dkey_ContKey = rawkey;
        dkey_NormKey = rawkey;
    } else {
        if (dkey_ContKey == rawkey) dkey_ContKey = 0;
    };

    /* das war's, also Interrupts wieder enablen... (Voodoo Ray) */
    outp(0x20,0x20);
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, dkey_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      10-Mar-96   floh    created
*/
{
    Object *newo;
    struct dkey_data *dkd;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    dkd = INST_DATA(cl,newo);

    dkd->remap_index = -1;

    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, dkey_IDEVM_SETID, struct idev_setid_msg *msg)
/*
**  CHANGED
**      10-Mar-96   floh    created
*/
{
    struct dkey_data *dkd = INST_DATA(cl,o);
    ULONG i=0;

    while (dkey_RemapTable[i].id != NULL) {
        if (stricmp(dkey_RemapTable[i].id, msg->id) == 0) {
            dkd->remap_index = i;
            return(dkey_RemapTable[i].type);
        };
        i++;
    };

    /*** unbekannte ID ***/
    return(IDEVTYPE_NONE);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, dkey_IDEVM_SETHOTKEY, struct idev_sethotkey_msg *msg)
/*
**  CHANGED
**      23-Mar-96   floh    created
*/
{
    struct dkey_data *dkd = INST_DATA(cl,o);
    ULONG i=0;

    if (msg->hotkey >= IDEV_NUMHOTKEYS) return(IDEVTYPE_NONE);

    while (dkey_RemapTable[i].id != NULL) {
        if (stricmp(dkey_RemapTable[i].id, msg->id) == 0) {
            if (IDEVTYPE_BUTTON == dkey_RemapTable[i].type) {
                dkd->hotkeys[msg->hotkey] = dkey_RemapTable[i].code;
                return(IDEVTYPE_HOTKEY);
            };
        };
        i++;
    };

    /*** unbekannte ID ***/
    return(IDEVTYPE_NONE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dkey_IDEVM_GETBUTTON, struct idev_status_msg *msg)
/*
**  CHANGED
**      10-Mar-96   floh    created + debugging
**      08-Apr-96   floh    neu Msg für IDEVM_GETBUTTON,
**                          IDEVM_GETSTATUS.
*/
{
    struct dkey_data *dkd = INST_DATA(cl,o);
    LONG ix = dkd->remap_index;

    if (ix >= 0) msg->btn_status = dkey_RemapTable[ix].status;
    else         msg->btn_status = FALSE;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dkey_IDEVM_GETSLIDER, struct idev_status_msg *msg)
/*
**  FUNCTION
**      Slider-Simulation per Button.
**
**  CHANGED
**      29-Sep-96   floh    created
**      03-Oct-96   floh    Detail-Änderungen
*/
{
    struct dkey_data *dkd = INST_DATA(cl,o);
    LONG ix = dkd->remap_index;

    msg->sld_status = 0.0;

    if (ix >= 0) {
        BOOL down = dkey_RemapTable[ix].status;
        if (down) dkd->vpos += 0.1;     // Anstieg
        else      dkd->vpos *= 0.8;     // Gummiband-Effekt
        if (dkd->vpos > 1.0) dkd->vpos = 1.0;
        msg->sld_status = dkd->vpos;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dkey_IDEVM_GETKEY, struct idev_getkey_msg *msg)
/*
**  CHANGED
**      10-Mar-96   floh    created
**      23-Mar-96   floh    modifiziert für Hotkey-Support
*/
{
    struct dkey_data *dkd = INST_DATA(cl,o);

    ULONG i;
    UBYTE ckey = dkey_ContKey;
    UBYTE nkey = dkey_NormKey;

    /*** NormKey entprellen ***/
    dkey_NormKey = 0;

    msg->cont_key = ckey;
    msg->norm_key = nkey;
    msg->hot_key  = 0;

    /*** Hotkeying ***/
    if (nkey != 0) {
        for (i=0; i<IDEV_NUMHOTKEYS; i++) {
            if (dkd->hotkeys[i] == nkey) {
                msg->hot_key = (i|0x80);
                return;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dkey_IDEVM_QUERYHOTKEY, struct idev_queryhotkey_msg *msg)
/*
**  CHANGED
**      03-Jan-97   floh    created
*/
{
    struct dkey_data *dkd = INST_DATA(cl,o);
    ULONG i;

    /*** was wird gesucht? ***/
    if (msg->keycode != 0) {
        /*** Keycode -> Hotkey ***/
        for (i=0; i<IDEV_NUMHOTKEYS; i++) {
            if (dkd->hotkeys[i] == msg->keycode) {
                msg->hotkey = (i|0x80);
                return;
            };
        };
    } else if (msg->hotkey != 0) {
        /*** Hotkey -> Keycode ***/
        LONG hkey = msg->hotkey & ~0x80;
        if (hkey < IDEV_NUMHOTKEYS) msg->keycode = dkd->hotkeys[hkey];
    };
}

