/*
**  $Source: PRG:VFM/Classes/_DosTimer/dtm_main.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/03/12 23:27:29 $
**  $Locker:  $
**  $Author: floh $
**
**  Time-Source-Klasse für DOS-PCs.
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
#include <conio.h>

#include "nucleus/nucleus2.h"
#include "input/dtimerclass.h"

/*-------------------------------------------------------------------
**  Prototypes
*/
void __interrupt __far dtimer_timer_int(void);
_dispatcher(ULONG, dtimer_ITIM_ELAPSEDTIME, void *);

/*-------------------------------------------------------------------
**  Global Data
*/
ULONG dtimer_CountInt   = 0;
ULONG dtimer_TimerDelta = 0;
void (__interrupt __far *dtimer_prev_timer_int)() = NULL;

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG dtimer_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo dtimer_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *MakeDTimerClass(ULONG id,...);
BOOL FreeDTimerClass(void);

struct GET_Class dtimer_GET = {
    &MakeDTimerClass,
    &FreeDTimerClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *dtimer_Entry(void)
{
    return(&dtimer_GET);
};

struct SegInfo dtimer_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/input/dtimer.class"
    },
    dtimer_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *MakeDTimerClass(ULONG id,...)
/*
**  CHANGED
**      11-Mar-96   floh    created
*/
{
    /*** Global Data initialisieren ***/
    dtimer_CountInt   = 0;
    dtimer_TimerDelta = 0;

    /*** Timer-Interrupt verbiegen ***/
    dtimer_prev_timer_int = _dos_getvect(0x8);
    _dos_setvect(0x8, dtimer_timer_int);

    /*** Timer reprogrammieren (Speed-Factor 11) (Kochbuch-Programmierung) ***/
    _disable();
    outp(0x43,0x36);    /* Channel 0 mode 3 (??) */
    outp(0x40,0x45);    /* Low Byte of 65536/11  */
    outp(0x40,0x17);    /* High Byte of 65536/11 */
    _enable();

    /*** Timer läuft jetzt mit 200Hz ***/

    /*** Klasse initialisieren ***/
    memset(dtimer_Methods, 0, sizeof(dtimer_Methods));

    dtimer_Methods[ITIM_ELAPSEDTIME] = (ULONG) dtimer_ITIM_ELAPSEDTIME;

    dtimer_clinfo.superclassid = ITIMER_CLASSID;
    dtimer_clinfo.methods      = dtimer_Methods;
    dtimer_clinfo.instsize     = sizeof(struct dtimer_data);
    dtimer_clinfo.flags        = 0;

    /*** Success-Ende ***/
    return(&dtimer_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL FreeDTimerClass(void)
/*
**  CHANGED
**      11-Mar-96   floh    created
*/
{
    if (dtimer_prev_timer_int) {

        /*** Timer abbremsen ***/
        _disable();
        outp(0x43,0x36);
        outp(0x40,0);       // Low Byte = 65536
        outp(0x40,0);       // High Byte = 65536
        _enable();

        /*** Interrupt Vektor restaurieren ***/
        _dos_setvect(0x8, dtimer_prev_timer_int);
    };
}

/*=================================================================**
**  SUPPORT FUNKTIONEN                                             **
**=================================================================*/

/*-----------------------------------------------------------------*/
void __interrupt __far dtimer_timer_int(void)
/*
**  FUNCTION
**      Mein eigener Timer-Interrupt 0x8. Durch die Timer-
**      Beschleunigung kommt etwa alle 1/200 sec ein Timer-Interrupt
**      an. Ich erhöhe dann erstmal die globale Timer-Variable
**      dtimer_TimeDelta um 5 (die damit quasi auf 1 millsec Auflösung
**      kommt) und gebe alle 11 Interrupts einmal die Kontrolle
**      an den ursprünglichen Timer-Interrupt, damit die Systemzeit
**      und so net durcheinander kommt.
**
**  CHANGED
**      28-Jan-95   floh    created (input_ibm.engine)
**      11-Mar-96   floh    übernommen nach dtimer.class
*/
{
    /* globalen Zeitzähler erhöhen */
    dtimer_TimerDelta += 5;

    /* muß originaler Interrupt aufgerufen werden? */
    dtimer_CountInt++;
    if (dtimer_CountInt == 11) {
        dtimer_CountInt = 0;
        _chain_intr(dtimer_prev_timer_int);
    };

    /* ansonsten ist hier Schluß (Lower Level Ints wieder enablen(???) */
    outp(0x20,0x20);
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, dtimer_ITIM_ELAPSEDTIME, void *nul)
/*
**  CHANGED
**      11-Mar-96   floh    created
*/
{
    struct dtimer_data *dtd = INST_DATA(cl,o);
    ULONG elapsed;
    ULONG act_time;

    /*** lese aktuelle globale Zeit ***/
    act_time = dtimer_TimerDelta;

    /*** Elapsed Time ermitteln (auf 1, falls 1.Aufruf) ***/
    if (dtd->timer_store == 0) elapsed = 1;
    else {
        elapsed = act_time - dtd->timer_store;
        if (elapsed == 0) elapsed = 1;
    };
    dtd->timer_store = act_time;

    /*** Ende ***/
    return(elapsed);
}

