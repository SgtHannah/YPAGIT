#ifndef MODULES_H
#define MODULES_H
/*
@NODE ModulesInclude "modules.h"
**
**  $Source: PRG:VFM/Include/modules.h,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 13:59:51 $
**  $Locker:  $
**  $Author: floh $
**
**  23-Sep-93   floh    37.01   Sämtliche Modul-ID's redefiniert, da sich
**                              dummerweise ein paar Überschneidungen
**                              mit Kontroll-Tags der Tag-Definition
**                              in <utility/tagitem.h> ergeben haben.
**  02-Oct-93   floh    38.1    RCS hat Revisions-Kontrolle übernommen.
**
**  (C) 1993 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-------------------------------------------------------------------
**  Jedes einzelne MovingCubes2-Modul bekommt eine eigene
**  ID-Nummer, außerdem werden die gebräuchlichsten
**  Amiga-Libraries mit einer ID versehen. Diese
**  IDs werden beim dynamischen Linken der einzelnen
**  Module in TagLists zur Übergabe der Einsprungtabellen
**  zwischen den Modulen benutzt.
*/
#define MID_NUCLEUS                 0x80000001L
#define MID_NucleusBase             0x80000002L

#define MID_ENGINE_OUTPUT_VISUAL    0x80000101L
#define MID_ENGINE_OUTPUT_AUDIO     0x80000102L
#define MID_ENGINE_OUTPUT_IC        0x80000103L

#define MID_ENGINE_RESOURCE         0x80000201L

#define MID_ENGINE_INPUT            0x80000301L

#define MID_ENGINE_TRANSFORM        0x80000401L

#define MID_SYSLIB_EXEC             0xC0000001L // OBSOLETE
#define MID_SYSLIB_DOS              0xC0000002L // OBSOLETE
#define MID_SYSLIB_UTILITY          0xC0000003L // OBSOLETE
#define MID_SYSLIB_IFFPARSE         0xC0000004L // OBSOLETE

/*-------------------------------------------------------------------
@ENDNODE
*/
#endif
