#ifndef YPA_GUIABORT_H
#define YPA_GUIABORT_H
/*
**  $Source$
**  $Revision$
**  $Date$
**  $Locker$
**  $Author$
**
**  (C) 1998 A.Weissflog
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
struct YPAAbortReq {
    struct YPAListReq l;    // eingebettete Listen-Requester-Struktur
    ULONG action;
    ULONG entry_height;
    ULONG edge_width;
    ULONG entry_space;
    ULONG btn_width;
};

#define AMR_BTN_CANCEL  (LV_NUM_STD_BTN)
#define AMR_BTN_SAVE    (LV_NUM_STD_BTN+1)
#define AMR_BTN_LOAD    (LV_NUM_STD_BTN+2)
#define AMR_BTN_RESTART (LV_NUM_STD_BTN+3)
#define AMR_BTN_RESUME  (LV_NUM_STD_BTN+4)

/*-----------------------------------------------------------------*/
#endif
