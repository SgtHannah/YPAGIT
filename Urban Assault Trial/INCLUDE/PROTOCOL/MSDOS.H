#ifndef PROTOCOL_MSDOS_H
#define PROTOCOL_MSDOS_H
/*
**  $Source: PRG:VFM/Include/protocol/msdos.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/11/26 01:53:20 $
**  $Locker:  $
**  $Author: floh $
**
**  Klassen-übergreifendes Kommunikations-Protokolle
**  unter MS-DOS.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-----------------------------------------------------------------*/
struct msdos_DispEnv {
    LONG x_size,y_size;     // Ausdehnung des Displays
    void *data;             // Daten-Ptr für Callback-Aufruf
    void (*mouse_callback)(void *data,LONG x,LONG y);  // ss!=ds(!!!)
};
/*-----------------------------------------------------------------*/
#endif





