#ifndef NUCLEUS_LOG_H
#define NUCLEUS_LOG_H
/*
**  $Source: PRG:VFM/Include/nucleus/log.h,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 14:02:17 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Nucleus-Log-System.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-----------------------------------------------------------------*/
#define NUM_LOGENTRIES  (64)

#define LOGMODE_PUFFERED    (0)
#define LOGMODE_IMMEDIATE   (1)
#define LOGMODE_FILE        (2)

/*-----------------------------------------------------------------*/
#endif
