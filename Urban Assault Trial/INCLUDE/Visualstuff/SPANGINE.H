#ifndef VISUALSTUFF_SPANGINE_H
#define VISUALSTUFF_SPANGINE_H
/*
**  $Source: PRG:VFM/Include/visualstuff/spangine.h,v $
**  $Revision: 38.3 $
**  $Date: 1995/11/12 21:57:55 $
**  $Locker:  $
**  $Author: floh $
**
**  Floh proudly presents: His Incredible Span Clipper (TM) :-)
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

/*-------------------------------------------------------------------
**  Folgende Struktur beschreibt einen Span, so wie er als
**  Input von der Spangine und von den Span-Drawern
**  akzeptiert wird.
*/
struct SpanElm {
    UWORD y;                // y-Koordinate des Spans
    UWORD flags;            // Span-Eigenschaften

    WORD sx, dx;            // Start und Länge des Spans

    WORD l0, ladd;          // Lighting

    UBYTE *mapdata;         // Mapping
    ULONG mx0;
    LONG  mxadd;

    ULONG my0;
    LONG  myadd;

    ULONG mz0;
    LONG  mzadd;
};

/*-------------------------------------------------------------------
**  MinSpans sind die interne Repräsentation eines Spans
**  in der Spangine.
*/
struct MinSpan {
    struct MinNode nd;
    WORD sx, dx;
};

/*-----------------------------------------------------------------*/
#define SF_RIGHT    (1<<0)      // private
#define SF_LEFT     (1<<1)      // private
#define SF_HIDDEN   (1<<2)      // private
#define SF_CLIPPED  (1<<3)      // private
#define SF_MERGE    (1<<4)      // private
#define SF_SOLID    (1<<15)     // public (siehe se_AddSpan())

/*-------------------------------------------------------------------
**  Spangine-Prototypes
*/
BOOL se_InitSpangine(ULONG, ULONG);
void se_KillSpangine(void);
void se_NewFrame(ULONG);
void se_AddSpan(struct SpanElm *);
void se_FlushSpangine(void);
/*-----------------------------------------------------------------*/
#endif


