/*
**  $Source: PRG:VFM/Nucleus2/pc/io.c,v $
**  $Revision: 38.11 $
**  $Date: 1998/01/06 12:45:55 $
**  $Locker: floh $
**  $Author: floh $
**
**  File-IO-Funktionen des PC-Nucleus-Kernels. Diese orientieren
**  sich stark an den ANSI-C-Filefunktionen, diese Implementierung
**  ist in der Tat nichts weiter als eine Umlenkung in die
**  ANSI-Funktionen. Trotzdem auf KEINEN FALL Nucleus-File-IO
**  mit ANSI-File-IO mischen!!!
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifdef _MSC_VER
#include <windows.h>
#define WORD_DEFINED
#endif

#include <exec/types.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include <sys/types.h>
#include <direct.h>
#include <errno.h>

extern struct NucleusBase NBase;

#include <sys/types.h>
#ifdef _MSC_VER

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

DIR     *opendir( const char *s )
{
        DIR *temp;

        temp=(DIR *)malloc(sizeof(DIR));
        if (!temp)
                return NULL;

        strcpy(temp->d_name, s);

        temp->d_first = 1;
        temp->hFind=NULL;
        
        return temp;
}

int     closedir( DIR *p )
{
        if (p)
        {
                if (p->hFind)
                {
                        FindClose(p->hFind);
            p->hFind=NULL;
                }
                free(p);
        }

        return 0;
}

struct dirent *readdir( DIR *p)
{
        WIN32_FIND_DATA fd;
        char name[NAME_MAX + 1];

        if (p->d_first)
        {
                strcpy(name, p->d_name);
                if (p->d_name[strlen(p->d_name)-1]=='\\')
                        strcat(name, "\\*.*");
                else
                        strcat(name, "\\*.*");
        
                p->hFind = FindFirstFile(name, &fd      );
                if (!p->hFind)
                {
                        return NULL;
                }
        
        p->d_first = 0;

        }
        else
        {
                if (!FindNextFile(p->hFind, &fd))
                        return NULL;
        }
        
        //
        // attributes
        //
        p->d_attr=0;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                p->d_attr |= _A_RDONLY;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
                p->d_attr |= _A_SYSTEM;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                p->d_attr |= _A_SUBDIR;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                p->d_attr |= _A_HIDDEN;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                p->d_attr |= _A_ARCH;

        //
        // time and date are not used
        //

        //
        // size and name
        //
        p->d_size = (long)fd.nFileSizeLow;
        strcpy(p->d_name, fd.cFileName);

        return p;
}

#endif

/*-----------------------------------------------------------------*/
struct ncAssign *nc_FindAssign(UBYTE *assign)
/*
**  FUNCTION
**      Durchsucht die Assignliste nach <assign>
**      und returniert Pointer auf Assign-Node, oder NULL.
**
**  CHANGED
**      23-Mar-98   floh    created
*/
{
    struct MinList *ls;
    struct MinNode *nd;
    struct ncAssign *agn = NULL;
    ls = &(NBase.AssignList);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        agn = (struct ncAssign *)nd;
        if (stricmp(agn->assign,assign)==0) return(agn);
    };
    return(NULL);
}

/*-----------------------------------------------------------------*/
ULONG nc_SetAssign(STRPTR assign, STRPTR path)
/*
**  FUNCTION
**      Definiert ein Assign.
**
**  CHANGED
**      23-Mar-98   floh    created
*/
{
    struct ncAssign *agn = nc_FindAssign(assign);
    if (!agn) {
        agn = (struct ncAssign *)
              nc_AllocVec(sizeof(struct ncAssign),MEMF_PUBLIC|MEMF_CLEAR);
        if (agn) {
            nc_AddHead((struct List *)&(NBase.AssignList),(struct Node *)agn);
        };
    };
    if (agn) {
        strcpy(&(agn->assign),assign);
        strcpy(&(agn->path),path);
    };
    return(agn ? TRUE:FALSE);
}

/*-----------------------------------------------------------------*/
STRPTR nc_GetAssign(STRPTR assign)
/*
**  FUNCTION
**      Returniert den Pfad zum Assign.
**
**  CHANGED
**      23-Mar-98   floh    created
*/
{
    struct ncAssign *agn = nc_FindAssign(assign);
    if (agn) return(agn->path);
    else     return(NULL);
}

/*-----------------------------------------------------------------*/
void ncFilterFilename(UBYTE *fn, UBYTE *buf, ULONG buf_len)
/*
**  FUNCTION
**      MS-DOS-Filenamen-Filter.
**
**      ':'     -> '\'
**      '/'     -> '\'
**
**      Der Pfad wird 3 Zeichen nach dem ***ersten*** Punkt
**      abgetrennt.
**
**  INPUTS
**      fn      -> Original-Filename
**      buf     -> Puffer für konvertierten Filename
**      buf_len -> Größe des Ziel-Puffers
**
**  RESULTS
**      ---
**
**  CHANGED
**      30-Apr-96   floh    created (vorher _FOpen() intern)
**      01-Apr-98   floh    + Doppelpunkt wird jetzt ignoriert
*/
{
    UBYTE *str = buf;
    strncpy(buf, fn, buf_len-1);
    while (str = strchr(str,'/')) *str++='\\';
    // while (str = strpbrk(str,":/")) *str++='\\';
    if (str = strchr(buf,'.')) if (strlen(++str)>3) str[3] = (UBYTE) 0;
}

/*-----------------------------------------------------------------*/
void ncResolveAssigns(UBYTE *fn, UBYTE *buf, ULONG buf_len)
/*
**  FUNCTION
**      Prüft, ob <fn> ein gültiges Assign enthält und ersetzt
**      dieses durch den Replacement-Pfad. Ruft sich solange
**      rekursiv auf, bis kein gültiges Assign
**      mehr ersetzt werden kann.
**      Assigns, welche NICHT gefunden werden, werden
**      unverändert gelassen (nc_FilterFilename ersetzt
**      dann die Doppelpunkte durch Backslashes).
**
**  CHANGED
**      23-Mar-98   floh    created
**      01-Apr-98   floh    einbuchstabige Assign werden als
**                          DOS-Laufwerks-Buchstabe interpretiert
**                          und nicht verändert!
*/
{
    UBYTE tmp_src[256];
    UBYTE tmp_buf[512];
    UBYTE *post_assign;
    buf[buf_len-1] = 0;
    strcpy(tmp_src,fn);

    /*** Sonderfall Pfad beginnt mit DOS-Laufwerks-Buchstabe ***/
    if ((strlen(tmp_src) > 2) && (tmp_src[1]==':')) {
        /*** Lazfwerks-Definition ignorieren ***/
        post_assign = strchr(&(tmp_src[2]),':');
    } else {
        /*** sonst Standard-Suche nach Assign-Terminator ***/
        post_assign = strchr(tmp_src,':');
    };
    if (post_assign) {
        /*** ein Assign im Original-Pfad ***/
        struct ncAssign *agn;
        *post_assign = 0;
        agn = nc_FindAssign(tmp_src);
        *post_assign = '/';
        if (agn) {
            /*** das Assign ist gültig ***/
            strcpy(tmp_buf,agn->path);
            /*** falls ein Assign durch ein Assign ersetzt wurde, ***/
            /*** ist der / als Trennzeichen überflüssig, weil das ***/
            /*** Assign bereits ein : als Postfix hat             ***/
            if (tmp_buf[strlen(tmp_buf)-1] == ':') post_assign++;
            strcat(tmp_buf,post_assign);
            strncpy(buf,tmp_buf,buf_len-1);
            ncResolveAssigns(tmp_buf,buf,buf_len);
            return;
        };
        /*** ab hier ungültiges Assign ***/
    };
    /*** Original-Pfad nach <buf> kopieren ***/
    strncpy(buf,tmp_src,buf_len-1);
}

/*-----------------------------------------------------------------*/
APTR nc_FOpen(STRPTR filename, STRPTR modes)
/*
**  FUNCTION
**      Nucleus-Equivalent zu ANSI's fopen(). Der zurückgegebene
**      Pointer ist allerdings strikt als abstraktes Handle
**      zu betrachten, nicht als (FILE *)!
**
**  INPUTS
**      filename    -> Filename des zu öffnenden Files.
**      modes       -> wie in ANSI-C:
**                          r   -> read
**                          w   -> write
**                          a   -> append
**                          +   -> read/write
**                          b   -> Binär-File
**
**  RESULTS
**      Ein magisches File-Handle, das als Argument für alle anderen
**      Fileoperationen dient. Direktzugriff oder MitschMatsch mit
**      den C-File-Routinen sind TÖDLICH!
**
**  CHANGED
**      22-Dec-94   floh    created
**      04-Jan-95   floh    Jetzt mit MSDOS-Pfadnamen-Filter,
**                          vorsichtshalber wird der Pfad dupliziert,
**                          bevor er manipuliert wird.
**      04-Nov-95   floh    Gibt jetzt eine Fehler-Meldung aus,
**                          falls File nicht geöffnet werden konnte.
**      26-Apr-96   floh    Debugvariable __FOpenCount
**      30-Apr-96   floh    Benutzt jetzt externen Filenamen-Filter.
**      10-Nov-96   floh    Filenamen-Filter wird aktiviert, wenn
**                          _DOS_PATH_STYLE_ definiert ist.
**      24-Feb-97   floh    FOpenCount jetzt Bestandteil von NBase
**      23-Mar-98   floh    + Assign-Handling
*/
{
    APTR result;
    UBYTE buf1[256];
    UBYTE buf2[256];
    ncResolveAssigns(filename,buf1,sizeof(buf1));
    ncFilterFilename(buf1,buf2,sizeof(buf2));
    filename = buf2;
    result = (APTR) fopen((char *)filename,(char *)modes);
    if (!result) {
        nc_LogMsg("_FOpen('%s','%s') failed!\n", filename, modes);
    } else {
        NBase.FOpenCount++;
    };
    return(result);
};

/*-----------------------------------------------------------------*/
LONG nc_FClose(APTR file)
/*
**  FUNCTION
**      Schließt einen mit nc_FOpen() geöffneten Filehandle.
**
**  INPUTS
**      file    -> der von nc_FOpen zurückgelieferte Filehandle
**
**  RESULTS
**      0  -> alles OK, sonst Fehler
**
**  CHANGED
**      22-Dec-94   floh    created
**      26-Apr-96   floh    Debugvariable __FOpenCount
**      24-Feb-97   floh    FOpenCount jetzt Bestandteil von NBase
*/
{
    NBase.FOpenCount--;
    return((LONG)fclose((FILE *)file));
};

/*-----------------------------------------------------------------*/
LONG nc_FSeek(APTR file, ULONG offset, ULONG how)
/*
**  FUNCTION
**      ANSI-like fseek()-Funktion.
**
**  INPUTS
**      file    -> Filehandle, wie von nc_FOpen() zurückgegeben.
**      offset  -> seek-offset
**      how     -> ein der <stdio.h> Konstanten:
**                      SEEK_SET    (0)
**                      SEEK_CUR    (1)
**                      SEEK_END    (2)
**
**  RESULTS
**      0 -> alles OK, sonst Fehler
**
**  CHANGED
**      22-Dec-94   floh    created
**
**  NOTE
**      Es gibt Probleme, wenn der C-Compiler nicht 0, 1 und 2
**      für SEEK_SET, SEEK_CUR und SEEK_END benutzt, ist aber
**      unwahrscheinlich.
*/
{
    LONG result;

    result = (LONG) fseek((FILE *)file,(long int)offset,(int)how);

    if (result != 0) {
        nc_LogMsg("_FSeek(0x%x, %d, %d) failed!\n", file, offset, how);
    };
    return(result);
};

/*-----------------------------------------------------------------*/
LONG nc_FRead(APTR buf, ULONG objsize, ULONG nobjs, APTR file)
/*
**  FUNCTION
**      fread()-Equivalent in Nucleus.
**
**  INPUTS
**      buf     -> hierhin lesen
**      objsize -> Größe eines "Objects"
**      nobjs   -> Anzahl der zu lesenden Objects
**      file    -> Pointer auf Filehandle, wie von nc_FOpen() geliefert
**
**  RESULTS
**      Anzahl der gelesenen "Objects", falls diese nicht mit der
**      Anzahl der geforderten übereinstimmt, trat ein Fehler auf,
**      oder einfach EOF.
**
**  CHANGED
**      22-Dec-94   floh    created
*/
{
    return((LONG) fread((void *) buf,
                        (size_t) objsize,
                        (size_t) nobjs,
                        (FILE *) file));
};

/*-----------------------------------------------------------------*/
LONG nc_FWrite(APTR buf, ULONG objsize, ULONG nobjs, APTR file)
/*
**  FUNCTION
**      fwrite()-Equivalent in Nucleus.
**
**  INPUTS
**      buf     -> von hier Daten holen
**      objsize -> Größe eines "Objects"
**      nobjs   -> Anzahl der zu schreibenden Objects
**      file    -> Pointer auf Filehandle, wie von nc_FOpen() geliefert
**
**  RESULTS
**      Anzahl der geschriebenen "Objects", falls diese kleiner ist,
**      als "nobjs", trat ein Fehler auf.
**
**  CHANGED
**      22-Dec-94   floh    created
*/
{
    return((LONG) fwrite((void *) buf,
                         (size_t) objsize,
                         (size_t) nobjs,
                         (FILE *) file));
};

/*-----------------------------------------------------------------*/
STRPTR nc_FGetS(STRPTR buf, ULONG maxlen, APTR file)
/*
**  FUNCTION
**      Exaktes Äquivalent zur ANSI fgets() Funktion.
**
**  INPUTS
**      buf     -> Mem Buffer
**      maxlen  -> maximale Buffer Größe
**      file    -> Pointer auf File-Handle
**
**  RESULTS
**      NULL, falls Fehler oder EOF, sonst == buf
**
**  CHANGED
**      01-May-95   floh    created
*/
{
    return((STRPTR)fgets((char *) buf,
                         (int)    maxlen,
                         (FILE *) file));
}

/*-----------------------------------------------------------------*/
ULONG nc_FDelete(STRPTR filename)
/*
**  FUNCTION
**      siehe amiga/io.c/nc_FDelete().
**
**  CHANGED
**      30-Apr-96   floh    created
*/
{
    UBYTE buf1[256];
    UBYTE buf2[256];
    ncResolveAssigns(filename,buf1,sizeof(buf1));
    ncFilterFilename(buf1,buf2,sizeof(buf2));
    filename = buf2;
    if (remove(filename) == 0) return(TRUE);
    else                       return(FALSE);
}

/*=================================================================**
**                                                                 **
**  DIRECTORY SCANNER FUNCTIONS                                    **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
APTR nc_FOpenDir(STRPTR dirname)
/*
**  FUNCTION
**      siehe amiga/io.c/nc_FOpenDir()
**
**  CHANGED
**      30-Apr-96   floh    created
*/
{
    UBYTE buf1[256];
    UBYTE buf2[256];
    ncResolveAssigns(dirname,buf1,sizeof(buf1));
    ncFilterFilename(buf1,buf2,sizeof(buf2));
    dirname = buf2;
    return((APTR)opendir(dirname));
}

/*-----------------------------------------------------------------*/
void nc_FCloseDir(APTR handle)
/*
**  FUNCTION
**      siehe amiga/io.c/nc_FCloseDir()
**
**  CHANGED
**      30-Apr-96   floh    created
*/
{
    DIR *dir = (DIR *) handle;
    closedir(dir);
}

/*-----------------------------------------------------------------*/
struct ncDirEntry *nc_FReadDir(APTR handle, struct ncDirEntry *entry)
/*
**  FUNCTION
**      siehe amiga/io.c/nc_FReadDir()
**
**  CHANGED
**      30-Apr-96   floh    created
**      13-Feb-97   floh    Änderungen für VisualC, welcher in der
**                          Standard-C-Lib keine Dir-Handling-Routinen
**                          kennt (also benutze ich die Posix-Lib)
*/
{
    DIR *dir = (DIR *) handle;
    struct dirent *d;

    memset(entry, 0, sizeof(struct ncDirEntry));
    if (d = readdir(dir)) {

        if (d->d_attr & _A_SUBDIR) {
            /*** das ist ein Subdirectory ***/
            entry->attrs |= NCDIR_DIRECTORY;
        } else {
            /*** das ist ein File ***/
            entry->size = d->d_size;
        };

        /*** Name übernehmen ***/
        strcpy(entry->name, d->d_name);

        /*** und zurück ***/
        return(entry);

    };

    /*** Fehler, oder Ende erreicht ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
ULONG nc_FMakeDir(UBYTE *path)
/*
**  FUNCTION
**      Entspricht dem POSIX mkdir().
**
**  INPUTS
**      Relativer oder absoluter Pfadname.
**
**  RESULT
**      TRUE    -> erfolgreich
**      FALSE   -> Fehler (falscher Pfadname)
**
**  CHANGED
**      23-Sep-97   floh    created
*/
{
    int result;
    UBYTE buf1[256];
    UBYTE buf2[256];
    ncResolveAssigns(path,buf1,sizeof(buf1));
    ncFilterFilename(buf1,buf2,sizeof(buf2));
    path = buf2;
    result = mkdir(path);
    if (result == 0) return(TRUE);
    else             return(FALSE);
}

/*-----------------------------------------------------------------*/
ULONG nc_FRemDir(UBYTE *path)
/*
**  FUNCTION
**      Entspricht dem POSIX rmdir(), außer beim Rückgabe-Wert.
**
**  INPUTS
**      Relativer oder absoluter Pfadname.
**
**  RESULT
**      TRUE    -> erfolgreich
**      FALSE   -> Fehler (falscher Pfadname)
**
**  CHANGED
**      23-Sep-97   floh    created
*/
{
    int result;
    UBYTE buf1[256];
    UBYTE buf2[256];
    ncResolveAssigns(path,buf1,sizeof(buf1));
    ncFilterFilename(buf1,buf2,sizeof(buf2));
    path = buf2;
    result = rmdir(path);
    if (result == 0) return(TRUE);
    else             return(FALSE);
}

/*-----------------------------------------------------------------*/
void nc_ManglePath(UBYTE *fn, UBYTE *buf, ULONG buf_len)
/*
**  FUNCTION
**      Ersetzt Assigns und macht Pfad DOS-kompatibel
**
**  CHANGED
**      24-Mar-98   floh    created
*/
{
    UBYTE tmp_buf[256];
    ncResolveAssigns(fn,tmp_buf,sizeof(tmp_buf));
    ncFilterFilename(tmp_buf,buf,buf_len);
}


