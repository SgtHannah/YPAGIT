/*
**  $Source: PRG:VFM/Classes/_DosDBGMouseClass/dbgm_main.c,v $
**  $Revision: 38.2 $
**  $Date: 1996/04/23 00:21:40 $
**  $Locker:  $
**  $Author: floh $
**
**  DOS-Debug-Mouse-Treiber.
**  Bitte beachten, daﬂ die Klasse hochgradig anti-optimiert ist :-)
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>
#include <bios.h>
#include <i86.h>
#include <dos.h>

#include "nucleus/nucleus2.h"
#include "input/dbgmclass.h"

/*-------------------------------------------------------------------
**  Prototypes
*/
_dispatcher(Object *, dbgm_OM_NEW, struct TagItem *);
_dispatcher(ULONG, dbgm_IDEVM_SETID, struct idev_setid_msg *);
_dispatcher(void, dbgm_IDEVM_GETBUTTON, struct idev_status_msg *);
_dispatcher(void, dbgm_IDEVM_GETSLIDER, struct idev_status_msg *);
_dispatcher(void, dbgm_IWIMPM_GETCLICKINFO, struct ClickInfo *);
_dispatcher(void, dbgm_IWIMPM_BEGINREFRESH, void *);
_dispatcher(void, dbgm_IWIMPM_ENDREFRESH, void *);

/*-------------------------------------------------------------------
**  Global Data
*/
struct idev_remap dbgm_RemapTable[] = {

    /*** Buttons ***/
    { "lmb", IDEVTYPE_BUTTON, DBGM_BTN_LMB,   0 },
    { "rmb", IDEVTYPE_BUTTON, DBGM_BTN_RMB,   0 },
    { "mmb", IDEVTYPE_BUTTON, DBGM_BTN_MMB,   0 },

    /*** Sliders ***/
    { "x",   IDEVTYPE_SLIDER, DBGM_SLD_X,     0 },
    { "y",   IDEVTYPE_SLIDER, DBGM_SLD_Y,     0 },

    /*** Terminator ***/
    { NULL, IDEVTYPE_NONE, -1, 0 },
};

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG dbgm_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo dbgm_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *MakeDbgMouseClass(ULONG id,...);
BOOL FreeDbgMouseClass(void);

struct GET_Class dbgm_GET = {
    &MakeDbgMouseClass,
    &FreeDbgMouseClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *dbgm_Entry(void)
{
    return(&dbgm_GET);
};

struct SegInfo dbgm_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/dbgm.class"
    },
    dbgm_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *MakeDbgMouseClass(ULONG id,...)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    union REGS regs;

    /*** BIOS-Maustreiber resetten ***/
    regs.w.ax = 0;
    int386(0x33,&regs,&regs);

    /*** Maus-Treiber einblenden ***/
    regs.w.ax = 0x1;
    int386(0x33,&regs,&regs);

    /*** Class Init ***/
    memset(dbgm_Methods,0,sizeof(dbgm_Methods));

    dbgm_Methods[OM_NEW]              = (ULONG) dbgm_OM_NEW;
    dbgm_Methods[IDEVM_SETID]         = (ULONG) dbgm_IDEVM_SETID;
    dbgm_Methods[IDEVM_GETBUTTON]     = (ULONG) dbgm_IDEVM_GETBUTTON;
    dbgm_Methods[IDEVM_GETSLIDER]     = (ULONG) dbgm_IDEVM_GETSLIDER;
    dbgm_Methods[IWIMPM_GETCLICKINFO] = (ULONG) dbgm_IWIMPM_GETCLICKINFO;
    dbgm_Methods[IWIMPM_BEGINREFRESH] = (ULONG) dbgm_IWIMPM_BEGINREFRESH;
    dbgm_Methods[IWIMPM_ENDREFRESH]   = (ULONG) dbgm_IWIMPM_ENDREFRESH;

    dbgm_clinfo.superclassid = IWIMP_CLASSID;
    dbgm_clinfo.methods      = dbgm_Methods;
    dbgm_clinfo.instsize     = sizeof(struct dbgm_data);
    dbgm_clinfo.flags        = 0;

    return(&dbgm_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL FreeDbgMouseClass(void)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    union REGS regs;

    /*** Mouse resetten ***/
    regs.w.ax = 0;
    int386(0x33, &regs, &regs);
}

/*=================================================================**
**  METHOD HANDLERS                                                **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, dbgm_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    Object *newo;
    struct dbgm_data *dmd;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    dmd = INST_DATA(cl,newo);

    /*** das Object ist momentan auf kein Element gemappt ***/
    dmd->remap_index = -1;

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, dbgm_IDEVM_SETID, struct idev_setid_msg *msg)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    struct dbgm_data *dmd = INST_DATA(cl,o);
    ULONG i = 0;

    while (dbgm_RemapTable[i].id != NULL) {
        if (stricmp(dbgm_RemapTable[i].id, msg->id) == 0) {
            dmd->remap_index = i;
            return(dbgm_RemapTable[i].type);
        };
        i++;
    };

    /*** unbekannte ID ***/
    return(IDEVTYPE_NONE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dbgm_IDEVM_GETBUTTON, struct idev_status_msg *msg)
/*
**  CHANGED
**      12-Mar-96   floh    created
**      08-Apr-96   floh    neu Msg f¸r IDEVM_GETBUTTON,
**                          IDEVM_GETSTATUS.
*/
{
    struct dbgm_data *dmd = INST_DATA(cl,o);
    LONG ix = dmd->remap_index;
    LONG code;

    msg->btn_status = FALSE;

    if (ix >= 0) {
        union REGS regs;
        regs.w.ax = 0x3;    // Maus-Status abfragen
        int386(0x33,&regs,&regs);
        if (regs.w.bx & dbgm_RemapTable[ix].code) msg->btn_status = TRUE;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dbgm_IDEVM_GETSLIDER, struct idev_status_msg *msg)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    struct dbgm_data *dmd = INST_DATA(cl,o);
    LONG ix = dmd->remap_index;

    msg->sld_status = 0.0;

    if (ix >= 0) {

        LONG delta;
        WORD x,y;
        union REGS regs;

        /*** Maus-Position abfragen... ***/
        regs.w.ax = 0x3;    // Maus-Status abfragen
        int386(0x33,&regs,&regs);
        x = regs.w.cx;
        y = regs.w.dx;

        switch(dbgm_RemapTable[ix].code) {

            case DBGM_SLD_X:
                delta       = x - dmd->store;
                dmd->store  = x;
                msg->sld_status = ((FLOAT)delta) / 32.0;
                break;

            case DBGM_SLD_Y:
                delta       = y - dmd->store;
                dmd->store  = y;
                msg->sld_status = ((FLOAT)delta) / 16.0;
                break;
        };

        /*** Range-Korrektur ***/
        if      (msg->sld_status < -1.0) msg->sld_status = -1.0;
        else if (msg->sld_status > +1.0) msg->sld_status = +1.0;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dbgm_IWIMPM_GETCLICKINFO, struct ClickInfo *ci)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    struct dbgm_data *dmd = INST_DATA(cl,o);
    union REGS regs;

    /*** Mausbutton Flags initialisieren ***/
    ci->flags = 0;

    /*** CIF_MOUSEHOLD, aktuelle Mouse-Position ***/
    regs.w.ax = 0x3;
    int386(0x33,&regs,&regs);
    if (regs.w.bx & (1<<0)) ci->flags |= CIF_MOUSEHOLD;
    ci->act.scrx = regs.w.cx >> 1;
    ci->act.scry = regs.w.dx;

    /*** CIF_MOUSEDOWN? ***/
    regs.w.ax = 0x5;
    regs.w.bx = 0;      // nur linker Mausbutton interessiert
    int386(0x33,&regs,&regs);
    if (regs.w.bx > 0) {
        ci->flags    |= CIF_MOUSEDOWN;
        ci->down.scrx = regs.w.cx >> 1;
        ci->down.scry = regs.w.dx;
    };

    /*** CIF_MOUSEUP? ***/
    regs.w.ax = 0x6;
    regs.w.bx = 0;      // nur linker Mausbutton interessiert
    int386(0x33,&regs,&regs);
    if (regs.w.bx > 0) {
        ci->flags  |= CIF_MOUSEUP;
        ci->up.scrx = regs.w.cx >> 1;
        ci->up.scry = regs.w.dx;
    };

    /*** den Rest macht die iwimp.class ***/
    _supermethoda(cl,o,IWIMPM_GETCLICKINFO,ci);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dbgm_IWIMPM_BEGINREFRESH, void *nil)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    union REGS regs;
    regs.w.ax = 0x2;    // Mauscursor ausblenden
    int386(0x33,&regs,&regs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, dbgm_IWIMPM_ENDREFRESH, void *nil)
/*
**  CHANGED
**      12-Mar-96   floh    created
*/
{
    union REGS regs;
    regs.w.ax = 0x1;    // Mauscursor einblenden
    int386(0x33,&regs,&regs);
}

