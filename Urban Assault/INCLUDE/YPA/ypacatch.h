#ifndef YPA_YPACATCH_H
#define YPA_YPACATCH_H
/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  ypacatch.h -- Event Catching System fuer Tutorial Levels.
**
**  (C) Copyright 1998 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Background
**  ==========
**  Hauptziel ist die Definition einer linearen Message-Schleife,
**  jede Message kann durch ein Event "blockiert" sein, die
**  Nichterfuellung des Events bewirkt, dass die Message-Schleife
**  "haengenbleibt", und die Message zyklisch wiederholt wird,
**  bis das Event eintritt. Eine Message darf mehrere Variationen
**  beinhalten, die dann zufaellig ausgewaehlt werden.
*/
#define MAXNUM_EVENTMSG_VARS (8)
struct ypa_EventLoopItem {
    ULONG type;             // siehe unten 
    BOOL (*event_func)(struct ypaworld_data *); // fuer YPAEVENTTYPE_COMPLEX
    LONG timestamp;         // fuer YPAEVENTTYPE_COUNTDOWN
    ULONG first_hit;        // fuer YPAEVENTTYPE_FIRSTHIT
    ULONG delay;            // solange zwischen Wiederholungen warten
    ULONG act_logmsg;
    ULONG num_logmsg;
    ULONG logmsg[MAXNUM_EVENTMSG_VARS];
};

/*** Event Types ***/
#define YPAEVENTTYPE_NONE               (0)
#define YPAEVENTTYPE_FIRSTHIT           (1)
#define YPAEVENTTYPE_DELAYED_CYCLIC     (2)
#define YPAEVENTTYPE_COMPLEX            (3)
#define YPAEVENTTYPE_CYCLIC             (4)
#define YPAEVENTTYPE_DELAYED_FIRSTHIT   (5)

/*** Returnvalues fuer CheckEvent() ***/
#define YPAEVENTRES_MSG     (1)
#define YPAEVENTRES_BLOCK   (2)
#define YPAEVENTRES_SKIP    (3)

#define MAXNUM_EVENTS (16)
struct ypa_EventCatcher {
    ULONG event_loop_id;            // siehe unten
    ULONG last_check_timestamp;     // Timestamp des letzten Checks
    LONG  last_msg_event;           // Event-Nummer des letzten Events das ausgegeben wurde
    ULONG last_msg_timestamp;       // Timestamp des letzten Events
    LONG num_events;                // Anzahl Events in der Event-Loop
    struct ypa_EventLoopItem events[MAXNUM_EVENTS];
};

/*-------------------------------------------------------------------
**  Vordefinierte Event-Loops in ypa_CatchEvent.event_loop_id
*/
#define EVENTLOOP_NONE          (0)
#define EVENTLOOP_TUTORIAL1     (1)
#define EVENTLOOP_TUTORIAL2     (2)
#define EVENTLOOP_TUTORIAL3     (3)

/*-----------------------------------------------------------------*/
#endif
