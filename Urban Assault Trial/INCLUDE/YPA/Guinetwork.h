#ifndef YPA_GUINETWIN_H
#define YPA_GUINETWIN_H
/*
**  $Source: PRG:VFM/Include/ypa/guinetwork.h,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 14:23:34 $
**  $Locker:  $
**  $Author: floh $
**
**
**  (C) Copyright 1996 by A.Weissflog
*/

#ifdef __NETWORK__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef YPA_GUILIST_H
#include "ypa/guilist.h"
#endif

#ifndef NETWORK_NETWORKCLASS_H
#include "network/networkclass.h"
#endif


/*-----------------------------------------------------------------*/

/* ---------------------------------------------------------------
** Dies ist der Requester für das Verschicken von Messages. In der
** Titelleiste steht, an wen es geht und dann folgt nur noch eine
** Zeile, die den text aufnehmen kann.
** -------------------------------------------------------------*/

#define CHATMSG_LEN (STANDARD_NAMELEN)   // weil ueberall diese Groesse...

struct YPAMessageWin {

    struct YPAReq Req;      // beginnend mit Requester-Struktur

    LONG   min_width;
    LONG   min_height;
    LONG   max_width;
    LONG   max_height;

    /*** Buffer für die Message ***/
    char   msg[ CHATMSG_LEN ];
    LONG   cursorpos;
};

#define MWBTN_ICONIFY   (0)
#define MWBTN_DRAGBAR   (1)
/*-----------------------------------------------------------------*/
#endif

#endif
