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

#ifdef _MSC_VER

#ifndef HANDLE
typedef void * HANDLE;
#endif

#define NAME_MAX        255             /* maximum filename for HPFS or NTFS */
typedef struct dirent {
        char    d_dta[ 21 ];            /* disk transfer area */
        char    d_attr;                         /* file's attribute */
        unsigned short int d_time;      /* file's time */
        unsigned short int d_date;      /* file's date */
        long    d_size;                         /* file's size */
        char    d_name[ NAME_MAX + 1 ]; /* file's name */
        unsigned short d_ino;           /* serial number (not used) */
        char    d_first;                        /* flag for 1st time */

        HANDLE  hFind;
} DIR;

/* File attribute constants for d_attr field */

#define _A_NORMAL       0x00    /* Normal file - read/write permitted */
#define _A_RDONLY       0x01    /* Read-only file */
#define _A_HIDDEN       0x02    /* Hidden file */
#define _A_SYSTEM       0x04    /* System file */
#define _A_VOLID        0x08    /* Volume-ID entry */
#define _A_SUBDIR       0x10    /* Subdirectory */
#define _A_ARCH         0x20    /* Archive file */

extern DIR      *opendir( const char * );
extern int      closedir( DIR * );
extern struct dirent *readdir( DIR * );

extern DIR *opendir( const char *s );
extern int closedir( DIR *p );
extern struct dirent *readdir( DIR *p);

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


