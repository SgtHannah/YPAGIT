/*
**  $Source: PRG:VFM/Classes/_DosMouse/dm_main.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/11/26 01:43:20 $
**  $Locker:  $
**  $Author: floh $
**
**  BIOS-Mouse-Treiber.
**  Compile mit ss!=ds.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>

#include <i86.h>
#include <bios.h>

#include "nucleus/nucleus2.h"
#include "input/dmouseclass.h"

/*-------------------------------------------------------------------
**  Prototypes
*/
extern void dmouse_AsmMouseHandler(void);   // aus mhandler.asm

_dispatcher(Object *, dmouse_OM_NEW, struct TagItem *);
_dispatcher(BOOL, dmouse_OM_DISPOSE, void *);
_dispatcher(void, dmouse_OM_SET, struct TagItem *);
_dispatcher(void, dmouse_OM_GET, struct TagItem *);

_dispatcher(void, dmouse_IDEVM_GETBUTTON, struct idev_status_msg *);
_dispatcher(void, dmouse_IDEVM_GETSLIDER, struct idev_status_msg *);
_dispatcher(ULONG, dmouse_IDEVM_SETID, struct idev_setid_msg *);
_dispatcher(void, dmouse_IDEVM_RESET, struct idev_reset_msg *);

_dispatcher(ULONG, dmouse_IWIMPM_HASFOCUS, void *);
_dispatcher(void, dmouse_IWIMPM_GETCLICKINFO, struct ClickInfo *);

/*-------------------------------------------------------------------
**  Global Data
*/
static struct dmouse_data *ActLID;  // Focus-Pntr für den Mousehandler!
static struct idev_xy MickeyPos;    // für LowLevel-Slider-Watcher
ULONG MouseStatus;                  // Buttons etc...

struct idev_remap dmouse_RemapTable[] = {

    /*** Buttons ***/
    { "lmb", IDEVTYPE_BUTTON, DMOUSE_BTN_LMB,   0 },
    { "rmb", IDEVTYPE_BUTTON, DMOUSE_BTN_RMB,   0 },
    { "mmb", IDEVTYPE_BUTTON, DMOUSE_BTN_MMB,   0 },

    /*** Sliders ***/
    { "x",   IDEVTYPE_SLIDER, DMOUSE_SLD_X,     0 },
    { "y",   IDEVTYPE_SLIDER, DMOUSE_SLD_Y,     0 },

    /*** Terminator ***/
    { NULL, IDEVTYPE_NONE, -1, 0 },
};

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG dmouse_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo dmouse_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *MakeDMouseClass(ULONG id,...);
BOOL FreeDMouseClass(void);

struct GET_Class dmouse_GET = {
    &MakeDMouseClass,
    &FreeDMouseClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *dmouse_Entry(void)
{
    return(&dmouse_GET);
};

struct SegInfo dmouse_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/dmouse.class"
    },
    dmouse_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *MakeDMouseClass(ULONG id,...)
/*
**  CHANGED
**      01-Mar-96   floh    created
*/
{
    union REGS regs;
    struct SREGS segs;
    int (far *function_ptr)();

    /*** Mouse-Handler-Initialisierung ***/
    ActLID      = NULL;
    MickeyPos.x = 0;
    MickeyPos.y = 0;
    MouseStatus = 0;

    /*** BIOS-Maustreiber resetten ***/
    regs.w.ax = 0;
    int386(0x33,&regs,&regs);
    if (regs.w.ax == 0xffff) {

        /*** Maushandler installieren ***/
        function_ptr = dmouse_AsmMouseHandler;
        segread(&segs);
        regs.x.eax = 0xC;
        regs.x.ecx = 0xff;
        regs.x.edx = FP_OFF(function_ptr);
        segs.es    = FP_SEG(function_ptr);
        int386x(0x33, &regs, &regs, &segs);
    };

    /*** Class Init ***/
    memset(dmouse_Methods,0,sizeof(dmouse_Methods));

    dmouse_Methods[OM_NEW]              = (ULONG) dmouse_OM_NEW;
    dmouse_Methods[OM_DISPOSE]          = (ULONG) dmouse_OM_DISPOSE;
    dmouse_Methods[OM_SET]              = (ULONG) dmouse_OM_SET;
    dmouse_Methods[OM_GET]              = (ULONG) dmouse_OM_GET;

    dmouse_Methods[IDEVM_GETBUTTON]     = (ULONG) dmouse_IDEVM_GETBUTTON;
    dmouse_Methods[IDEVM_GETSLIDER]     = (ULONG) dmouse_IDEVM_GETSLIDER;
    dmouse_Methods[IDEVM_SETID]         = (ULONG) dmouse_IDEVM_SETID;
    dmouse_Methods[IDEVM_RESET]         = (ULONG) dmouse_IDEVM_RESET;

    dmouse_Methods[IWIMPM_HASFOCUS]     = (ULONG) dmouse_IWIMPM_HASFOCUS;
    dmouse_Methods[IWIMPM_GETCLICKINFO] = (ULONG) dmouse_IWIMPM_GETCLICKINFO;

    dmouse_clinfo.superclassid = IWIMP_CLASSID;
    dmouse_clinfo.methods      = dmouse_Methods;
    dmouse_clinfo.instsize     = sizeof(struct dmouse_data);
    dmouse_clinfo.flags        = 0;

    return(&dmouse_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL FreeDMouseClass(void)
/*
**  CHANGED
**      01-Mar-96
*/
{
    union REGS regs;

    /*** Mouse resetten (und damit Mousetreiber killen) ***/
    regs.w.ax = 0;
    int386(0x33, &regs, &regs);
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/

/*=================================================================**
**  FOLGENDE FUNKTIONEN LAUFEN IM MOUSE-HANDLER-CONTEXT            **
**=================================================================*/

/*-----------------------------------------------------------------*/
void dmouse_MouseHandler(LONG events, LONG buttons, LONG xpos, LONG ypos)
/*
**  FUNCTION
**      Highlevel-Mousehandler. Wird aufgerufen von der ASM-
**      Funktion dmouse_AsmMouseHandler in mhandler.asm.
**
**  INPUTS
**      event   - eax, Event Maske
**      buttons - edx, Status der Maus-Knöpfe
**      xpos    - ebx, aktuelle X-Position in Mickeys
**      ypos    - ecx, aktuelle Y-Position in Mickeys
**
**  RESULTS
**
**  CHANGED
**      05-Sep-95   floh    jetzt nach Watcom-Beispiel...
**      20-Sep-95   floh    jetzt wieder wie früher, es gab
**                          Schwierigkeiten mit Stack-Zugriffen.
**      21-Sep-95   floh    übernommen nach ie_handlers.c
**      02-Mar-96   floh    übernommen nach dmouse.class/main.c
**      25-Nov-96   floh    Pointer-Rendering eliminiert, dafür
**                          wird eine externe Callback-Funktion
**                          aufgerufen (falls importiert)
*/
{
    /*** kein Target-Object, keine Maus-Daten ***/
    struct dmouse_data *dmd = ActLID;

    if (dmd) {

        /*** aktueller Button-Status ***/
        MouseStatus = buttons;

        /*** wurde Maus bewegt? ***/
        if (events & (1<<0)) {

            LONG dx = xpos - MickeyPos.x;
            LONG dy = ypos - MickeyPos.y;

            MickeyPos.x = xpos;
            MickeyPos.y = ypos;

            dmd->raw_screen_pos.x += dx;
            dmd->raw_screen_pos.y += dy;

            /*** rohe Screenkoordinaten begrenzen ***/
            if (dmd->raw_screen_pos.x < 0) dmd->raw_screen_pos.x = 0;
            else if (dmd->raw_screen_pos.x > 639) dmd->raw_screen_pos.x = 639;

            if (dmd->raw_screen_pos.y < 0) dmd->raw_screen_pos.y = 0;
            else if (dmd->raw_screen_pos.y > 399) dmd->raw_screen_pos.y = 399;

            /*** raw_pos -> screen_pos, und Mouse-Callback aufrufen ***/
            if (dmd->import) {
                LONG w = dmd->import->x_size;
                LONG h = dmd->import->y_size;
                dmd->screen_pos.x = (dmd->raw_screen_pos.x * w)/640;
                dmd->screen_pos.y = (dmd->raw_screen_pos.y * h)/400;
                if (dmd->import->mouse_callback) {
                    dmd->import->mouse_callback(dmd->import->data,
                                                dmd->screen_pos.x,
                                                dmd->screen_pos.y);
                };
            }else{
                dmd->screen_pos.x = 0;
                dmd->screen_pos.y = 0;
            };
        };

        /*** Button-Ereignisse ***/
        if (events & (1<<1)) {
            dmd->lmb_down++;
            dmd->lmb_down_pos.x = dmd->screen_pos.x;
            dmd->lmb_down_pos.y = dmd->screen_pos.y;
        };
        if (events & (1<<2)) {
            dmd->lmb_up++;
            dmd->lmb_up_pos.x = dmd->screen_pos.x;
            dmd->lmb_up_pos.y = dmd->screen_pos.y;
        };
        if (events & (1<<3)) {
            dmd->rmb_down++;
            dmd->rmb_down_pos.x = dmd->screen_pos.x;
            dmd->rmb_down_pos.y = dmd->screen_pos.y;
        };
        if (events & (1<<4)) {
            dmd->rmb_up++;
            dmd->rmb_up_pos.x = dmd->screen_pos.x;
            dmd->rmb_up_pos.y = dmd->screen_pos.y;
        };
        if (events & (1<<5)) {
            dmd->mmb_down++;
            dmd->mmb_down_pos.x = dmd->screen_pos.x;
            dmd->mmb_down_pos.y = dmd->screen_pos.y;
        };
        if (events & (1<<6)) {
            dmd->mmb_up++;
            dmd->mmb_up_pos.x = dmd->screen_pos.x;
            dmd->mmb_up_pos.y = dmd->screen_pos.y;
        };
    };
}
/*=================================================================**
**  METHOD HANDLERS                                                **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, dmouse_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      01-Mar-96   floh    created
**      09-Mar-96   floh    revised & updated
**      25-Nov-96   floh    + IWIMPA_Environment jetzt
**                            <struct msdos_DispEnv *>
**                          - IWIMPA_Pointer gekillt
*/
{
    Object *newo;
    struct dmouse_data *dmd;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    dmd = INST_DATA(cl,newo);

    /*** das Object ist momentan auf kein Element gemappt ***/
    dmd->remap_index = -1;

    /*** IWIMPA_Environment wird interpretiert als VFMBitmap-Pntr ***/
    dmd->import = (struct msdos_DispEnv *)
                  _GetTagData(IWIMPA_Environment,NULL,attrs);

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, dmouse_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      01-Mar-96   floh    created
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);

    /*** MouseHandler verliert evtl. seinen Focus ***/
    if (dmd == ActLID) ActLID = NULL;

    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dmouse_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      01-Mar-96   floh    created
**      09-Mar-96   floh    revised & updated
**      15-Jun-96   floh    akzeptiert jetzt auch NULL-Mousepointer
**      19-Aug-96   floh    Beim Neusetzen von IWIMPA_Environment
**                          werden die Mauskoordinaten (geschützt
**                          durch ein _disable()/_enable()) auf
**                          [0,0] zurückgesetzt.
**      25-Nov-96   floh    + IWIMPA_Environment neu
**                          - IWIMPA_Pointer gekillt
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);
    struct TagItem *ti;

    /*** IWIMPA_Environment ***/
    if (ti = _FindTagItem(IWIMPA_Environment, attrs)) {
        _disable();
        dmd->import = (struct msdos_DispEnv *) ti->ti_Data;
        dmd->screen_pos.x = 0;
        dmd->screen_pos.y = 0;
        dmd->raw_screen_pos.x = 0;
        dmd->raw_screen_pos.y = 0;
        _enable();
    };

    /*** Ende ***/
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dmouse_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      01-Mar-96   floh    created
**      09-Mar-96   floh    revised & updated
**      25-Nov-96   floh    + IWIMPA_Environment neu
**                          - IWIMPA_Pointer gekillt
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);
    struct TagItem *ti;

    /*** IWIMPA_Environment ***/
    if (ti = _FindTagItem(IWIMPA_Environment, attrs)) {
        *(ULONG *)(ti->ti_Data) = (ULONG) dmd->import;
    };

    /*** Ende ***/
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, dmouse_IDEVM_SETID, struct idev_setid_msg *msg)
/*
**  CHANGED
**      09-Mar-96   floh    created
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);
    ULONG i = 0;

    while (dmouse_RemapTable[i].id != NULL) {
        if (stricmp(dmouse_RemapTable[i].id, msg->id) == 0) {
            dmd->remap_index = i;
            return(dmouse_RemapTable[i].type);
        };
        i++;
    };

    /*** unbekannte ID ***/
    return(IDEVTYPE_NONE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dmouse_IDEVM_GETBUTTON, struct idev_status_msg *msg)
/*
**  CHANGED
**      02-Mar-96   floh    created
**      09-Mar-96   floh    revised & updated
**      08-Apr-96   floh    neu Msg für IDEVM_GETBUTTON,
**                          IDEVM_GETSTATUS.
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);
    LONG ix = dmd->remap_index;
    LONG code;
    if (ix >= 0) code = dmouse_RemapTable[ix].code;
    else         code = 0;
    msg->btn_status = (code & MouseStatus) ? TRUE:FALSE;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dmouse_IDEVM_GETSLIDER, struct idev_status_msg *msg)
/*
**  CHANGED
**      02-Mar-96   floh    created
**      09-Mar-96   floh    revised & updated
**      08-Apr-96   floh    neu Msg für IDEVM_GETBUTTON,
**                          IDEVM_GETSTATUS.
**      15-Jun-96   floh    Verändertes Slider-Handling (nicht mehr
**                          relativ, sondern absolut.
**      17-Jun-96   floh    <virtual_pos> wird jetzt wie durch ein
**                          Gummiband langsam auf 0 zurückgezogen.
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);
    LONG ix = dmd->remap_index;
    LONG delta;

    msg->sld_status = 0.0;

    if (ix >= 0) {

        /*** Gummiband-Effekt ***/
        dmd->virtual_pos = (dmd->virtual_pos*9)/10;

        switch(dmouse_RemapTable[ix].code) {

            case DMOUSE_SLD_X:
                delta             = MickeyPos.x - dmd->mickey_pos;
                dmd->mickey_pos   = MickeyPos.x;
                dmd->virtual_pos += delta;
                break;

            case DMOUSE_SLD_Y:
                delta             = MickeyPos.y - dmd->mickey_pos;
                dmd->mickey_pos   = MickeyPos.y;
                dmd->virtual_pos += delta;
                break;
        };

        /*** Range-Korrektur ***/
        if (dmd->virtual_pos < (-DMOUSE_MAXVIRTUAL)) {
            dmd->virtual_pos = -DMOUSE_MAXVIRTUAL;
        } else if (dmd->virtual_pos > DMOUSE_MAXVIRTUAL) {
            dmd->virtual_pos = DMOUSE_MAXVIRTUAL;
        };

        /*** Umwandlung in Slider-Wert ***/
        msg->sld_status = ((FLOAT)(dmd->virtual_pos))/DMOUSE_MAXVIRTUAL;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dmouse_IDEVM_RESET, struct idev_reset_msg *msg)
/*
**  FUNCTION
**      Setzt die <virtual_pos> auf 0 zurück.
**
**  CHANGED
**      15-Jun-96   floh    created
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);
    if (msg->rtype == IRTYPE_CENTER) dmd->virtual_pos = 0;
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, dmouse_IWIMPM_HASFOCUS, void *nil)
/*
**  CHANGED
**      04-Mar-96   floh    created
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);

    /*** der globale ActLID Pointer wird auf das Objekt     ***/
    /*** gebogen, welches nach dem Input-Focus gefragt wird ***/
    ActLID = dmd;

    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dmouse_IWIMPM_GETCLICKINFO, struct ClickInfo *ci)
/*
**  FUNCTION
**      Ermittelt die Mouse-Events und -Positionen
**      für den ClickInfo-Check in der Superklasse.
**
**  CHANGED
**      02-Mar-96   floh    created
**      18-Mar-96   floh    + CIF_RMOUSEDOWN, CIF_RMOUSEUP, CIF_RMOUSEHOLD,
**                            CIF_MMOUSEDOWN, CIF_MMOUSEUP, CIF_MMOUSEHOLD
*/
{
    struct dmouse_data *dmd = INST_DATA(cl,o);

    /*** Mausbutton Flags initialisieren ***/
    ci->flags = 0;

    /*** aktuelle Mausposition ***/
    ci->act.scrx = dmd->screen_pos.x;
    ci->act.scry = dmd->screen_pos.y;

    /*** CIF_MOUSEHOLD? ***/
    if (MouseStatus & (1<<0)) ci->flags |= CIF_MOUSEHOLD;
    if (MouseStatus & (1<<1)) ci->flags |= CIF_RMOUSEHOLD;
    if (MouseStatus & (1<<2)) ci->flags |= CIF_MMOUSEHOLD;

    /*** CIF_MOUSEDOWN? ***/
    if (dmd->lmb_down > 0) {
        ci->flags    |= CIF_MOUSEDOWN;
        ci->down.scrx = dmd->lmb_down_pos.x;
        ci->down.scry = dmd->lmb_down_pos.y;
        dmd->lmb_down = 0;
    };

    /*** CIF_MOUSEUP? ***/
    if (dmd->lmb_up > 0) {
        ci->flags  |= CIF_MOUSEUP;
        ci->up.scrx = dmd->lmb_up_pos.x;
        ci->up.scry = dmd->lmb_up_pos.y;
        dmd->lmb_up = 0;
    };

    /*** RMB, MMB Up & Down Events ***/
    if (dmd->rmb_down > 0) {
        ci->flags    |= CIF_RMOUSEDOWN;
        dmd->rmb_down = 0;
    };
    if (dmd->rmb_up > 0) {
        ci->flags  |= CIF_RMOUSEUP;
        dmd->rmb_up = 0;
    };
    if (dmd->mmb_down > 0) {
        ci->flags    |= CIF_MMOUSEDOWN;
        dmd->mmb_down = 0;
    };
    if (dmd->mmb_up > 0) {
        ci->flags  |= CIF_MMOUSEUP;
        dmd->mmb_up = 0;
    };

    /*** den Rest macht die iwimp.class ***/
    _supermethoda(cl,o,IWIMPM_GETCLICKINFO,ci);
}

