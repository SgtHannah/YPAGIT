/*
**  $Source: PRG:VFM/Classes/_BmpAnimClass/banio.c,v $
**  $Revision: 38.4 $
**  $Date: 1996/03/20 18:41:52 $
**  $Locker:  $
**  $Author: floh $
**
**  Spezielle IO-Funktionen zum transparenten Lesen und
**  Schreiben von bmpanim-Resourcefiles als "Plain File"
**  und als IFF-File.
**
**  Vorsicht! Massive Hacks...
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <libraries/iffparse.h>

#include <string.h>
#include <stdio.h>

#include "nucleus/nucleus2.h"
#include "bitmap/bmpanimclass.h"

//-------------------------------------------------------------------
_extern_use_nucleus

BOOL IsIFF      = FALSE;    // globaler IFF-Indikator, internal use only.
BOOL IFFWasOpen = FALSE;    // IFF-File war bereits offen.
ULONG IFFMode;

/*-----------------------------------------------------------------*/
APTR bmpanim_FOpen(UBYTE *name, UBYTE *mode)
/*
**  FUNCTION
**      Öffnet einen File entweder als "normalen" Datenfile
**      oder als IFF-Stream. Auf diesen File darf
**      ausschließlich über die banim_#? IO-Routinen zugegriffen
**      werden!!!
**
**      Falls IsIFF beim Aufruf von banim_FOpen() bereits TRUE
**      ist, geht die Routine davon aus, daß bereits ein
**      offener IFF-Stream vorliegt, in <name> wird dann
**      kein String-Pointer, sondern ein IFFHandle-Pointer
**      erwartet (gelobt sei C). Ansonsten wird im Lese-
**      Modus der per <name> definiert File geöffnet, und
**      auf IFF getestet. Je nach Ergebnis wird ein IFF-Stream
**      daraus gemacht, oder nicht.
**
**      Im Schreib-Modus wird IMMER ein IFF-Stream geöffnet.
**
**  INPUTS
**      name    - if (IsIFF == FALSE) ABSOLUTER(!) Filename
**                oder offenes IFF-Handle
**      mode    - herkömmlicher ANSI-IO-Mode-String
**
**  RESULTS
**      Ein abstraktes Handle, welches an die restlichen bmpanim_#?
**      Routinen übergeben werden muß.
**
**  CHANGED
**      12-Jan-96   floh    created
**      14-Jan-96   floh    debugging...
**                          + Man kann ja gar nicht einfach
**                            Daten in einen FORM schreiben,
**                            dazu muss schon ein Chunk her!
**                          + Ich muss doch unterscheiden zwischen
**                            Lese- und Schreib-Modus...
**      22-Jan-96   floh    + Bugfix: im Collection-Modus muß ich
**                            irgendwie besoffen gewesen sein :-/
**      20-Mar-96   floh    Long standing Bug: auf PCs wurden IFF-Files
**                          nicht erkannt, weil ich die Endian-Konvertierung
**                          auf FORM vergessen hatte.
*/
{
    if (strchr(mode,'r')) IFFMode = IFFF_READ;
    else                  IFFMode = IFFF_WRITE;

    if (!IsIFF) {

        APTR fp;

        /*** das ist wichtig für bmpanim_FClose() ***/
        IFFWasOpen = FALSE;

        /*** IFF-Test (4 Zeichen lesen und gegen 'FORM' testen) ***/
        if (IFFMode == IFFF_READ) {

            ULONG form;

            if (fp = _FOpen(name, mode)) {
                _FRead(&form,sizeof(form),1,fp);
                v2nl(&form);
                if (form == MAKE_ID('F','O','R','M')) {
                    /*** ein IFF-File ***/
                    IsIFF = TRUE;
                    _FSeek(fp,0,SEEK_SET);
                } else {
                    /*** ein "normaler" File ***/
                    IsIFF = FALSE;
                    _FSeek(fp,0,SEEK_SET);
                    return(fp);
                };
            } else return(NULL);

        } else {

            /*** Schreib-Modus (immer im IFF-Modus) ***/
            IsIFF = TRUE;
            fp = _FOpen(name, mode);
            if (!fp) return(NULL);
        };

        /*** IFF-Stream initialisieren, falls notwendig ***/
        if (IsIFF) {

            struct IFFHandle *iff;
            LONG error;

            if (iff = _AllocIFF()) {

                /*** fp ist jetzt garantiert gültig ***/
                iff->iff_Stream = (ULONG) fp;
                _InitIFFasNucleus(iff);

                error = _OpenIFF(iff, IFFMode);
                if (error == 0) {
                    if (IFFMode == IFFF_READ) {
                        /*** Lese-Modus, FORM VANM und DATA lesen (dirty) ***/
                        error = _ParseIFF(iff, IFFPARSE_RAWSTEP);
                        error |= _ParseIFF(iff, IFFPARSE_RAWSTEP);
                    } else {
                        /*** Schreib-Modus, FORM VANM und DATA pushen ***/
                        error = _PushChunk(iff, BMPANIM_VANM, ID_FORM, IFFSIZE_UNKNOWN);
                        error |= _PushChunk(iff, 0, BMPANIM_DATA, IFFSIZE_UNKNOWN);
                    };
                    if (error == 0) return((APTR)iff);

                    /*** Fehler... Queue abbauen ***/
                    _CloseIFF(iff);
                };
                _FClose(fp);
                _FreeIFF(iff);
            };
            /*** irgend ein Fehler ***/
            return(NULL);
        };

    } else {

        /*** IFF-Chunk war schon offen ***/
        struct IFFHandle *iff = (struct IFFHandle *) name;
        LONG error;

        IFFWasOpen = TRUE;

        if (IFFMode == IFFF_READ) {
            /*** Lese-Modus, FORM VANM und DATA lesen (dirty) ***/
            error = _ParseIFF(iff, IFFPARSE_RAWSTEP);
            error |= _ParseIFF(iff, IFFPARSE_RAWSTEP);
        } else {
            /*** Schreib-Modus, FORM VANM und DATA pushen ***/
            error = _PushChunk(iff, BMPANIM_VANM, ID_FORM, IFFSIZE_UNKNOWN);
            error |= _PushChunk(iff, 0, BMPANIM_DATA, IFFSIZE_UNKNOWN);
        };
        if (error == 0) return((APTR)iff);
    };

    /*** irgend ein Fehler ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
LONG bmpanim_FClose(APTR handle)
/*
**  FUNCTION
**      Gegenstück zu bmpanim_FOpen(). Falls es sich um einen
**      IFF-Handle handelt (IfIFF == TRUE), wird ein _PopChunk()
**      gemacht, falls der IFF-Stream selbst geöffnet wurde,
**      (IFFWasOpen == FALSE) wird der IFFHandle deinitialisiert.
**
**      Bei einem normalen File wird dieser einfach geschlossen.
**
**  INPUTS
**      handle  - ist entweder ein normaler Filepointer, oder
**                ein struct IFFHandle.
**
**  RESULTS
**      immer 0
**
**  CHANGED
**      12-Jan-96   floh    created
**      14-Jan-96   floh    + der neue Data-Chunk muss auch gepoppt
**                            werden...
**                          + Unterscheidung zwischen Lese- und
**                            Schreib-Modus
*/
{
    if (handle) {

        /*** ein IFF-File? ***/
        if (IsIFF) {

            struct IFFHandle *iff = (struct IFFHandle *) handle;

            if (IFFMode == IFFF_READ) {
                /* 2x EndOfChunk holen */
                _ParseIFF(iff,IFFPARSE_RAWSTEP);
                _ParseIFF(iff,IFFPARSE_RAWSTEP);
            } else {
                /*** FORM VANM und DATA-Chunk poppen ***/
                _PopChunk(iff);
                _PopChunk(iff);
            };

            if (!IFFWasOpen) {

                /*** IFF-Stream deinitialisieren ***/
                _CloseIFF(iff);
                _FClose((APTR)iff->iff_Stream);
                _FreeIFF(iff);
            };

        } else {
            /*** ein normaler File ***/
            _FClose(handle);
        };
    };

    return(0);
}

/*-----------------------------------------------------------------*/
LONG bmpanim_FRead(APTR b, ULONG bsize, ULONG n, APTR fp)
/*
**  FUNCTION
**      Universelle FRead() Funktion, arbeitet sowohl auf einem
**      IFF-Stream, als auch auf einem "normalen" Datenstrom.
**
**  INPUTS
**      full ANSI
**
**  RESULTS
**      Anzahl der gelesenen "Blöcke".
**
**  CHANGED
**      12-Jan-96   floh    created
*/
{
    LONG res = 0;

    if (IsIFF) {

        struct IFFHandle *iff = (struct IFFHandle *) fp;
        res = _ReadChunkBytes(iff, b, bsize*n);
        if (res < 0) res=0;
        else res /= bsize;

    } else {
        res = _FRead(b,bsize,n,fp);
    };
    return(res);
}

/*-----------------------------------------------------------------*/
LONG bmpanim_FWrite(APTR b, ULONG bsize, ULONG n, APTR fp)
/*
**  FUNCTION
**      siehe bmpanim_FRead()
**
**  CHANGED
**      12-Jan-96   floh    created
*/
{
    LONG res = 0;

    if (IsIFF) {

        struct IFFHandle *iff = (struct IFFHandle *) fp;
        res = _WriteChunkBytes(iff, b, bsize*n);
        if (res < 0) res=0;
        else res /= bsize;

    } else {
        res = _FWrite(b,bsize,n,fp);
    };
    return(res);
}


