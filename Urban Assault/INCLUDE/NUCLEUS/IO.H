#ifndef NUCLEUS_IO_H
#define NUCLEUS_IO_H
/*
**  $Source: PRG:VFM/Include/nucleus/io.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/04/30 19:00:01 $
**  $Locker: floh $
**  $Author: floh $
**
**  Allgemeine Definitionen für Nucleus-FileIO-Routinen.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

/*-------------------------------------------------------------------
**  Überblick über Nucleus File IO
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
**  Nucleus-Filenamen:
**  ~~~~~~~~~~~~~~~~~~
**  Nucleus unterstützt plattform-übergreifend die Trennzeichen
**  ':' und '/' (Amiga-Notation mit Device-Support).
**  Bei Plattformen, die andere Trennzeichen benutzen (z.B. DOS)
**  sind Filenamens-Filter eingebaut, die die Trennzeichen
**  so gut wie möglich umwandeln. ':' wird dabei in einen
**  normalen "Filenamens-Separator" umgewandelt -> das heißt
**  statt einem Device wird ein normales Subdirectory angesprochen.
**
**
**  *** File Manipulation Functions ***
**  APTR _FOpen(STRPTR filename, STRPTR modes);
**  LONG _FClose(APTR file);
**  LONG _FSeek(APTR file, ULONG offset, ULONG how);
**  LONG _FRead(APTR buf, ULONG objsize, ULONG nobjs, APTR file);
**  LONG _FWrite(APTR buf, ULONG objsize, ULONG nobjs, APTR file);
**  STRPTR _FGetS(STRPTR buf, ULONG maxlen, APTR file);
**  ULONG _FDelete(STRPTR filename);
**
**  *** Directory Scanning Functions ***
**  APTR _FOpenDir(STRPTR dirname);
**  LONG _FCloseDir(APTR dir);
**  struct ncDirEntry *_FReadDir(APTR dir, struct ncDirEntry *entry);
**
**  NOTE
**  ----
**  - Es existiert keine Funktion, mit der man ein neues Directory
**    erzeugen kann.
**  - _FReadDir() unterscheidet sich von der POSIX-Funktion
**    readdir()! Es muß ein Pointer auf eine ncDirEntry-Funktion
**    übergeben werden, welche von der Funktion ausgefüllt wird!
**    Derselbe Pointer wird zurückgegeben wenn alles glattging,
**    NULL, falls ein Fehler auftrat. Die Freigabe der
**    ncDirEntry-Struktur ist aber auf *jeden* Fall Sache des
**    Aufrufers!
**    Siehe auch amiga/io.c oder pc/io.c!
*/

/*** ncDirEntry -> wird von _FReadDir() zurückgegeben ***/
struct ncDirEntry {
    ULONG attrs;            // siehe unten
    ULONG size;             // Filegröße in Bytes
    UBYTE name[256];        // C-String, purer Name (ohne Pfad etc.)
};

#define NCDIR_DIRECTORY     (1<<0)  // das ist ein Subdirectory

/*** ncAssign ***/
struct ncAssign {
    struct MinNode nd;
    UBYTE assign[32];       // der Assign-Name
    UBYTE path[128];        // der Replacement-Pfad
};

/*-----------------------------------------------------------------*/
#endif


