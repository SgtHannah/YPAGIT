#ifndef YPA_GUICONFIRM_H
#define YPA_GUICONFIRM_H
/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  (C) Copyright 1998 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INPUT_INPUT_H
#include "input/input.h"
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"
#endif

#ifndef YPA_GUILIST_H
#include "ypa/guilist.h"
#endif

/*-------------------------------------------------------------------
**  Defs fuer YPAConfirmReq
*/
struct YPAConfirmReq {
    struct YPAListReq l;    // eingebettete Listen-Requester-Struktur
    UBYTE *text;            // momentan eingestellter Mode (siehe unten)
    ULONG status;           // siehe unten
    LONG btn_width;         // Layout-Variable
    LONG btn_space;         // Layout-Variable
    LONG edge_width;        // Edge-Breite
};

#define YPACR_STATUS_CLOSED     (0) // ist zu
#define YPACR_STATUS_OK         (1) // mit OK verlassen (nur einmal)
#define YPACR_STATUS_CANCEL     (2) // mit Cancel verlassen (nur einmal)
#define YPACR_STATUS_OPEN       (3) // derzeit noch offen

/*-----------------------------------------------------------------*/
#endif

