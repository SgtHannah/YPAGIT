/*
**  $Source: PRG:VFM/Nucleus2/pc/iff.c,v $
**  $Revision: 38.3 $
**  $Date: 1996/11/08 01:53:19 $
**  $Locker:  $
**  $Author: floh $
**
**  Nucleus-"Emulation" einer Untermenge der iffparse.library.
**  Einschränkungen gegenüber der Original iffparse.library:
**
**      1) alle File-Zugriffe sind auf die Nucleus-IO-Routinen
**         nc_FRead(), nc_FWrite() und nc_FSeek() hardgecoded, es ist also
**         keine Handler-Einbindung möglich
**      2) es wird nur IFFPARSE_RAWSTEP unterstützt (also auch
**         keine >CollectionChunks< oder >PropChunks<)
**      3) von den IFF-Basis-Chunktypen "FORM", "LIST" und "CAT "
**         wird nur "FORM" unterstützt
**      4) mit (1) fällt auch der Clipboard-Support weg
**      5) Chunk-Entry/Exit-Handler sind nicht implementiert
**      6) IFF-Syntax-Prüfung nicht 100% exakt
**      7) es ist nur eine (sehr) begrenztes Set an Routinen
**         vorhanden
**
**  Ansonsten ist der gesamte Code ANSI-kompatibel, allerdings wird
**  von Funktionen anderer Nucleus-Kernel-Module Gebrauch gemacht,
**  namentlich <io.c> und <lists.c> und <memory.c>
**
**  (C) Copyright 1994 by A.Weissflog
*/
/*** Amiga [Emul] Stuff ***/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>

/*** ANSI-Zeugs ***/
#include <stdio.h>

/*** eigenes Zeugs ***/
#define IFF_IFFPARSE_H          // libraries/iffparse.h NICHT includen
#include "nucleus/nucleus2.h"
#include "nucleus/iff.h"
#include "nucleus/sys.h"

/*-----------------------------------------------------------------*/
struct ContextNode *allocContextNode(void)
/*
**  FUNCTION    ***PRIVATE***PRIVATE***PRIVATE
**      Allokiert und initialisiert eine neue ContextNode-Struktur.
**
**  INPUTS
**      ---
**
**  RESULTS
**      Pointer auf neue ContextNode oder NULL, falls Out Of Mem.
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{
    /* Node allokieren... */
    struct ContextNode *cn = (struct ContextNode *)
        nc_AllocVec(sizeof(struct ContextNode), MEMF_PUBLIC|MEMF_CLEAR);

    /* ...und zurück */
    return(cn);
};

/*-----------------------------------------------------------------*/
void freeContextNode(struct ContextNode *cn)
/*
**  FUNCTION    ***PRIVATE***PRIVATE***PRIVATE
**      Entfernt eine ContextNode-Struktur aus dem Speicher.
**      Die Node muß bereits aus dem ContextNode-Stack
**      entfernt worden sein!
**
**  INPUTS
**      cn - Pointer auf zu killende ContextNode
**
**  RESULTS
**      ---
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{
    if (cn) {
        /* dann die ContextNode selbst freigeben */
        nc_FreeVec(cn);
    };
};

/*-----------------------------------------------------------------*/
struct IFFHandle *nc_AllocIFF(void)
/*
**  FUNCTION
**      Erzeugt eine neue IFFHandle-Struktur
**
**  INPUTS
**      ---
**
**  RESULTS
**      Pointer auf IFFHandle, oder NULL, falls kein Mem.
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{   
    struct ContextNode *defaultnode;
    struct IFFHandle *iff;

    /* per nc_AllocVec() ein IFFHandle allokieren */
    iff = (struct IFFHandle *)
        nc_AllocVec(sizeof(struct IFFHandle),MEMF_PUBLIC|MEMF_CLEAR);

    if (iff) {
        /* initialisiere ContextNode-Stack... */
        nc_NewList(&(iff->iff_Stack));

        /* erzeuge die versteckte Default-ContextNode als "Master-FORM" */
        defaultnode = allocContextNode();
        if (!defaultnode) {
            nc_FreeVec(iff); return(NULL);
        };

        /* kennzeichne Default-Node als FORM NULL */
        defaultnode->cn_ID   = ID_FORM;
        defaultnode->cn_Type = ID_NULL;
        defaultnode->cn_Size = 1L<<31;   /* irgendein utopischer Wert */

        /* trage ContextNode als ContextNode-Stack-Head ein */
        nc_AddHead(&(iff->iff_Stack), defaultnode);
    };

    /* und zurück */
    return(iff);
};

/*-----------------------------------------------------------------*/
void nc_FreeIFF(struct IFFHandle *iff)
/*
**  FUNCTION
**      Gibt ein mit nc_AllocIFF() erzeugtes IFFHandle frei.
**
**  INPUTS
**      iff -> Pointer auf IFFHandle
**
**  RESULTS
**      ---
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
**
**  NOTE
**      Der ContextNode-Stack wird vollständig aufgelöst, egal
**      in welchem Zustand er ist.
*/
{
    if (iff) {

        /* gebe ContextNode-Stack vollständig frei */
        struct ContextNode *cn;
        while (cn = (struct ContextNode *) nc_RemHead(&(iff->iff_Stack)))
            freeContextNode(cn);

        /* dann das IFFHandle selbst freigeben */
        nc_FreeVec(iff);

    };
};

/*-----------------------------------------------------------------*/
LONG nc_OpenIFF(struct IFFHandle *iff, LONG rwMode)
/*
**  FUNCTION
**      Initialisiert ein IFFHandle für ein erneutes Lesen/Schreiben
**      (spezifiziert durch rwMode).
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**      rwMode  -> IFFF_READ oder IFFF_WRITE
**
**  RESULTS
**      0   -> alles OK, sonst ErrorCode
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{
    /* Flag-Wort initialisieren... */
    iff->iff_Flags = rwMode;

    return(0);
};

/*-----------------------------------------------------------------*/
void nc_CloseIFF(struct IFFHandle *iff)
/*
**  FUNCTION
**      Schließt einen mit nc_OpenIFF() erzeugten IFF context.
**
**  INPUTS
**      iff -> offenenes IFFHandle
**
**  RESULTS
**      ---
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
**
**  NOTE
**      Ich wüßte im Augenblick nicht, womit ich diese Funktion
**      füllen soll ;-)
*/
{ };

/*-----------------------------------------------------------------*/
struct ContextNode *nc_CurrentChunk(struct IFFHandle *iff)
/*
**  FUNCTION
**      siehe "iffparse.library/CurrentChunk" Autodocs
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**
**  RESULTS
**      Pointer auf aktuelle ContextNode oder NULL, falls keine vorhanden.
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{
    /* aktuelle ContextNode holen */
    struct ContextNode *cn = (struct ContextNode *) iff->iff_Stack.mlh_Head;

    /* falls das die Default-Node ist, NULL zurück! */
    if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_NULL)) {
        cn = NULL;
    };

    return(cn);
};

/*-----------------------------------------------------------------*/
void nc_InitIFFasNucleus(struct IFFHandle *iff)
/*
**  FUNCTION
**      Weil diese Implementierung Modul sowieso auf die 
**      Nucleus-IO-Routinen hardgecodet ist, ist diese Routine leer!
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**
**  RESULTS
**      ---
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{ };

/*-----------------------------------------------------------------*/
LONG nc_PushChunk(struct IFFHandle *iff, LONG type, LONG id, LONG size)
/*
**  FUNCTION
**      Push't neue ContextNode auf den ContextStack.
**      Siehe "iffparse.library/PushChunk" Autodocs
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**      type    -> Chunk Typ (z.B. ILBM) (wird ignoriert wenn Leaf-Chunk).
**      id      -> Chunk ID (z.B. CMAP) (wird ignoriert in ReadMode)
**      size    -> Byte-Größe des Chunks oder IFFSIZE_UNKNOWN
**
**  RESULTS
**      0L  -> falls erfolgreich, andernfalls ein IFFERR_#?-Code
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{
    LONG result;

    /*-- WRITE MODE -----------------------------------------------*/
    if (iff->iff_Flags & IFFF_WRITE) {

        LONG scan;
        ULONG w_id,w_size;
        struct ContextNode *cn;

        /* Chunks dürfen nur innerhalb FORMs geschrieben werden */
        struct ContextNode *cur_cn = (struct ContextNode *) iff->iff_Stack.mlh_Head;
        if (cur_cn->cn_ID != ID_FORM) return(IFFERR_SYNTAX);

        scan = 0;

        /* erzeuge einen neuen Chunk-Header im File */
        w_id   = id; n2vl(&w_id);       /* Native-Endian -> VFM-Endian */
        w_size = size; n2vl(&w_size);   /* Native-Endian -> VFM-Endian */

        result = nc_FWrite(&w_id, sizeof(w_id), 1, iff->iff_Stream);
        if (result != 1) return(IFFERR_WRITE);
        result = nc_FWrite(&w_size, sizeof(w_size), 1, iff->iff_Stream);
        if (result != 1) return(IFFERR_WRITE);

        /* falls FORM-Chunk, Type an Header anhängen */
        if (id == ID_FORM) {

            ULONG w_type=type; n2vl(&w_type); /* Native-Endian -> VFM-Endian */

            result = nc_FWrite(&w_type, sizeof(w_type), 1, iff->iff_Stream);
            if (result != 1) return(IFFERR_WRITE);
            scan += sizeof(type);

        } else {

            /* falls kein FORM, Type = Type des Parent-Chunk */
            type = cur_cn->cn_Type;
        };

        /* neue ContextNode erzeugen und ausfüllen */
        cn = allocContextNode();
        if (!cn) return(IFFERR_NOMEM);

        cn->cn_ID   = id;
        cn->cn_Type = type;
        cn->cn_Size = size;
        cn->cn_Scan = scan;

        /* neue ContextNode auf Context-Stack legen */
        nc_AddHead(&(iff->iff_Stack), cn);

        /* iff_Depth abhandeln */
        iff->iff_Depth += 1;

        /* und erfolgreich zurück... */
        return(0);

    } else {

    /*-- READ MODE ------------------------------------------------*/
        struct ContextNode *new_cn;
        ULONG new_id, new_size, new_type, new_scan;

        /* aktuellen Chunk untersuchen */
        struct ContextNode *cur_cn = (struct ContextNode *) iff->iff_Stack.mlh_Head;

        /* falls KEIN FORM, sofort mit "End Of Chunk" zurück */
        if (cur_cn->cn_ID != ID_FORM) return(IFFERR_EOC);

        /* wenn FORM, nur mit "EOC" zurück, wenn voll durchgescannt */
        if (cur_cn->cn_Scan == cur_cn->cn_Size) return(IFFERR_EOC);

        /* ansonsten MUSS noch ein Sub-Chunk kommen... */

        new_type = cur_cn->cn_Type;
        new_scan = 0;

        /* lese ID und Size */
        result = nc_FRead(&new_id, sizeof(new_id), 1, iff->iff_Stream);
        if (result != 1) return(IFFERR_READ);
        result = nc_FRead(&new_size, sizeof(new_size), 1, iff->iff_Stream);
        if (result != 1) return(IFFERR_READ);

        v2nl(&new_id);  /* VFM-Endian -> Native-Endian */
        v2nl(&new_size);  /* VFM-Endian -> Native-Endian */

        /* falls es eine FORM ist, noch Type lesen... */
        if (new_id == ID_FORM) {

            result = nc_FRead(&new_type, sizeof(new_type), 1, iff->iff_Stream);
            if (result != 1) return(IFFERR_READ);

            v2nl(&new_type);  /* VFM-Endian -> Native-Endian */

            new_scan += sizeof(new_type);
        };

        /* erzeuge neue ContextNode und fülle sie mit gelesenen Zeug */
        new_cn = allocContextNode();
        if (!new_cn) return(IFFERR_NOMEM);

        new_cn->cn_ID   = new_id;
        new_cn->cn_Type = new_type;
        new_cn->cn_Size = new_size;
        new_cn->cn_Scan = new_scan;

        /* lege neue ContextNode auf Context-Stack */
        nc_AddHead(&(iff->iff_Stack), new_cn);

        /* iff_Depth abhandeln */
        iff->iff_Depth += 1;

        /* ...und erfolgreich zurück */
        return(0);
    };

    /* Cant' happen... */
};

/*-----------------------------------------------------------------*/
LONG nc_PopChunk(struct IFFHandle *iff)
/*
**  FUNCTION
**      Siehe "iffparse.library/PopChunk" Autodocs
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**
**  RESULTS
**      0L  -> falls erfolgreich, andernfalls ein IFFERR_#?-Code
**
**  CHANGED
**      23-Dec-94   floh    konvertiert aus <iffp_emu.c>
*/
{
    LONG result;
    ULONG id, type, size, scan;

    struct ContextNode *cn = (struct ContextNode *) iff->iff_Stack.mlh_Head;
    id   = cn->cn_ID;
    type = cn->cn_Type;
    size = cn->cn_Size;
    scan = cn->cn_Scan;

    /* falls keine Node auf Stack gepusht ist, mit Fehler zurück! */
    if ((id == ID_FORM) && (type == ID_NULL))
        return(IFFERR_SYNTAX);

    /*-- WRITE MODE -----------------------------------------------*/
    if (iff->iff_Flags & IFFF_WRITE) {

        struct ContextNode *new_cn;

        /* IFFSIZE_UNKNOWN abhandeln... */
        if (size == IFFSIZE_UNKNOWN) {

            ULONG w_size;
            size = scan;

            /* seeke zurück auf ChunkHeader.Size */
            result = nc_FSeek(iff->iff_Stream, -size-sizeof(ULONG),SEEK_CUR);
            if (result != 0) return(IFFERR_SEEK);

            /* schreibe wirkliche Size nach ChunkHeader.Size */
            w_size=size; n2vl(&w_size);  /* Native-Endian -> VFM-Endian */

            result = nc_FWrite(&w_size, sizeof(w_size), 1, iff->iff_Stream);
            if (result != 1) return(IFFERR_WRITE);

            /* seeke wieder auf Ausgangs-Position */
            result = nc_FSeek(iff->iff_Stream, size, SEEK_CUR);
            if (result != 0) return(IFFERR_SEEK);
        };

        /* falls nicht genügend Bytes nach Chunk geschrieben wurden... */
        if (scan < size) return(IFFERR_MANGLED);

        /* ungerade Chunkgröße abhandeln... */
        if (size & 1L) {
            /* 1 Pad-Byte nach Chunk schreiben */
            UBYTE pad = 0;
            result = nc_FWrite(&pad, sizeof(pad), 1, iff->iff_Stream);
            if (result != 1) return(IFFERR_WRITE);

            /* size und scan auch "padden" */
            size++; scan++;
        };

        /* entferne ContextNode vom iff_Stack */
        nc_RemHead(&(iff->iff_Stack));
        freeContextNode(cn);

        /* der gepoppte Chunk beeinflußt cn_Scan der neuen CurNode */
        new_cn = (struct ContextNode *) iff->iff_Stack.mlh_Head;
        new_cn->cn_Scan += size+2*sizeof(ULONG);

        /* ^^^ die 2*sizeof(ULONG) sind für den Chunk-Header */

        /* noch iff_Depth abhandeln... */
        iff->iff_Depth -= 1;

        /* und erfolgreich zurück */
        return(0);

    } else {

    /*-- READ MODE ------------------------------------------------*/

        struct ContextNode *new_cn;

        /* Leaf-Chunks bedürfen einer extensiveren Behandlung: */
        if (id != ID_FORM) {

            /* falls unvollständig gelesen, ans Ende seeken */
            if (scan < size) {
                result = nc_FSeek(iff->iff_Stream, size-scan, SEEK_CUR);
                if (result != 0) return(IFFERR_SEEK);
                scan = size;
            };

            /* ungerade Chunk-Größe abhandeln (1 Byte vorwärts seeken) */
            if (size & 1L) {
                result = nc_FSeek(iff->iff_Stream, 1, SEEK_CUR);
                if (result != 0) return(IFFERR_SEEK);
                size++; scan++;
            };
        };

        /* oberste Node vom Stack holen und killen */
        nc_RemHead(&(iff->iff_Stack));
        freeContextNode(cn);

        /* neue CurrentNode holen */
        new_cn = (struct ContextNode *) iff->iff_Stack.mlh_Head;

        /* falls Stack jetzt leer, mit EOF zurück */
        if ((new_cn->cn_ID == ID_FORM) && (new_cn->cn_Type == ID_NULL))
            return(IFFERR_EOF);

        /* die gepoppte Node beeinflußt cn_Scan der Parent-Node */
        new_cn->cn_Scan += size+2*sizeof(ULONG);
        /* ^^^ die 2*sizeof(ULONG) sind für den Chunk-Header */

        /* noch iff_Depth abhandeln... */
        iff->iff_Depth -= 1;

        /* und erfolgreich zurück */
        return(0);
    };

    /* Can't happen... */
};

/*-----------------------------------------------------------------*/
LONG nc_ParseIFF(struct IFFHandle *iff, LONG control)
/*
**  FUNCTION
**      siehe "iffparse.library/ParseIFF" Autodocs.
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**      control -> IFFPARSE_RAWSTEP 
**                 (andere Modi werden in Nucleus z.Zt. nicht unterstützt!)
**
**  RESULTS
**      0L  -> alles OK, sonst IFFERR_#?-Code.
**
**  CHANGED
**      23-Dec-94   floh    'created'
*/
{
    LONG result = IFFERR_SYNTAX;

    /* bisher nur mit IFFPARSE_RAWSTEP */
    if (control == IFFPARSE_RAWSTEP) {

        /* wenn IFFF_POPME-Flag gesetzt - poppen */
        if (iff->iff_Flags & IFFF_POPME) {

            result = nc_PopChunk(iff);

            /* IFFF_POPME-Flag löschen */
            iff->iff_Flags &= ~IFFF_POPME;

            /* "End Of File" erreicht oder anderer Fehler ? */
            if (result != 0) return(result);
        };

        /* nächsten Chunk [read]-pushen */
        result = nc_PushChunk(iff, ID_NULL, ID_NULL, IFFSIZE_UNKNOWN);

        /* falls mit EndOfChunk zurück, IFFF_POPME-Flag setzen */
        if (result == IFFERR_EOC)  iff->iff_Flags |= IFFF_POPME;
    };

    /* und zurück an Applikation */
    return(result);
};

/*-----------------------------------------------------------------*/
LONG nc_ReadChunkBytes(struct IFFHandle *iff, APTR buf, LONG numBytes)
/*
**  FUNCTION
**      siehe "iffparse.library/ReadChunkBytes" Autodocs
**
**  INPUTS
**      iff         -> Pointer auf IFFHandle
**      buf         -> die gelesenen Daten hierhin, bitte
**      numBytes    -> soviele Bytes lesen
**
**  RESULTS
**      >=0  -> Anzahl der gelesenen Bytes
**      <0   -> IFFERR_#? Code
**
**  CHANGED
**      23-Dec-94   floh    'created'
*/
{
    LONG result;

    /* hole CurrentChunk */
    struct ContextNode *cn = nc_CurrentChunk(iff);
    if (!cn) return(IFFERR_SYNTAX);

    /* man kann nur von einem Leaf-Chunk direkt lesen ! */
    if (cn->cn_ID == ID_FORM) return(IFFERR_SYNTAX);

    /* >numBytes< evtl. beschnippeln */
    if ((cn->cn_Scan + numBytes) > cn->cn_Size) {
        numBytes = cn->cn_Size - cn->cn_Scan;
        if (numBytes == 0) return(0);
    };

    /* cn_Scan aktualisieren */
    cn->cn_Scan += numBytes;

    /* und konkret einlesen, das Zeugs */
    result = nc_FRead(buf, 1, numBytes, iff->iff_Stream);
    if (result != numBytes) return(IFFERR_READ);

    return(result);
};

/*-----------------------------------------------------------------*/
LONG nc_WriteChunkBytes(struct IFFHandle *iff, APTR buf, LONG numBytes)
/*
**  FUNCTION
**      siehe "iffparse.library/WriteChunkBytes" Autodocs
**
**  INPUTS
**      iff         -> Pointer auf IFFHandle
**      buf         -> von hier die zu schreibenden Daten holen
**      numBytes    -> soviele Bytes schreiben
**
**  RESULTS
**      >=0     -> Anzahl tatsächlich geschriebener Bytes
**      <0      -> IFFERR_#? Code
**
**  CHANGED
**      23-Dec-94   floh    created
*/
{
    LONG result;

    /* hole CurrentChunk */
    struct ContextNode *cn = nc_CurrentChunk(iff);
    if (!cn) return(IFFERR_SYNTAX);

    /* man kann nur in einen Leaf-Chunk direkt schreiben ! */
    if (cn->cn_ID == ID_FORM) return(IFFERR_SYNTAX);

    /* >numBytes< evtl. beschnippeln */
    if (cn->cn_Size != IFFSIZE_UNKNOWN) {
        if ((cn->cn_Scan + numBytes) > cn->cn_Size) {
            numBytes = cn->cn_Size - cn->cn_Scan;
            if (numBytes == 0) return(0);
        };
    };

    /* cn_Scan aktualisieren */
    cn->cn_Scan += numBytes;

    /* und konkret schreiben, das Zeugs */
    result = nc_FWrite(buf, 1, numBytes, iff->iff_Stream);
    if (result != numBytes) return(IFFERR_WRITE);

    return(result);
};

/*-----------------------------------------------------------------*/
BOOL nc_SkipChunk(struct IFFHandle *iff)
/*
**  FUNCTION
**      Überspringt den aktuellen Context (und seine Subkontexte).
**
**  INPUTS
**      iff     -> Pointer auf IFFHandle
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> Ooosps, Fehler
**
**  CHANGED
**      01-Jan-95   floh    created
*/
{
    LONG error;
    while ((error = nc_ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        if (error) return(FALSE);
        if (!nc_SkipChunk(iff)) return(FALSE);
    };
    return(TRUE);
};

