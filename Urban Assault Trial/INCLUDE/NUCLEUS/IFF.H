#ifndef IFFP_EMU_H
#define IFFP_EMU_H
/*
**  $Source: PRG:VFM/Include/nucleus/iff.h,v $
**  $Revision: 38.1 $
**  $Date: 1994/12/23 19:41:37 $
**  $Locker:  $
**  $Author: floh $
**
**  Header-File der ANSI-IFFParse-Emulation. Dieser File
**  enthält eigene Erweiterungen der öffentlichen
**  Strukturen der iffparse.library.
**  Für "normale" Applikationen bitte den Include-File
**  "libraries/iffparse.h" benutzen.
**
**  Dieser Header enthält nur Definitionen für die
**  zur Zeit realisierten Features.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include "exec/types.h"
#endif

#ifndef EXEC_LISTS_H
#include "exec/lists.h"
#endif

#ifndef EXEC_NODES_H
#include "exec/nodes.h"
#endif

#ifndef STDIO_H
#include "stdio.h"
#endif
/*-------------------------------------------------------------------
 * A node associated with a context on the iff_Stack. Each node
 * represents a chunk, the stack representing the current nesting
 * of chunks in the open IFF file. Each context node has associated
 * local context items in the (private) LocalItems list.  The ID, type,
 * size and scan values describe the chunk associated with this node.
 *
 * This structure can only be allocated by iffparse.library
**
**  Normalerweise darf ein Chunk Subchunks enthalten, wenn er
**  einem der drei Basis-Typen "FORM", "LIST" oder "CAT "
**  entspricht. Die derzeitige Implementation unterstützt
**  allerdings nur "FORM"s.
*/
struct ContextNode
{
    struct MinNode cn_Node;
    LONG           cn_ID;
    LONG           cn_Type;
    LONG           cn_Size;     /*  Size of this chunk             */
    LONG           cn_Scan;     /*  # of bytes read/written so far */
    /* ab hier privat */
};

/*-------------------------------------------------------------------
 * Structure associated with an active IFF stream.
 * "iff_Stream" is a value used by the client's read/write/seek functions -
 * it will not be accessed by the library itself and can have any value
 * (could even be a pointer or a BPTR).
 *
 * This structure can only be allocated by iffparse.library
**
**  Die Struktur wurde von mir um private Felder erweitert. Auf diese
**  Felder darf in der Applikation nie direkt zugegriffen werden
**  (das kann ohnehin nicht passieren, wenn man "libraries/iffparse.h"
**  included.
*/
struct IFFHandle
{
    void *iff_Stream;   /* das hier wurde korrigiert... */
    ULONG iff_Flags;
    LONG  iff_Depth;    /*  Depth of context stack */
    /* ab jetzt privat */
    struct MinList iff_Stack;   /* der Context-Stack als Liste */
};

/* bit masks for "iff_Flags" field */
#define IFFF_READ       0L                       /* read mode - default    */
#define IFFF_WRITE      1L                       /* write mode             */
#define IFFF_RWBITS     (IFFF_READ | IFFF_WRITE) /* read/write bits        */
#define IFFF_FSEEK      (1L<<1)          /* forward seek only      */
#define IFFF_RSEEK      (1L<<2)          /* random seek    */
#define IFFF_RESERVED   0xFFFF0000L              /* Don't touch these bits */

/* private Flag-Definitionen */
#define IFFF_POPME      (1L<<31)


/* IFF return codes. Most functions return either zero for success or
 * one of these codes. The exceptions are the read/write functions which
 * return positive values for number of bytes or records read or written,
 * or a negative error code. Some of these codes are not errors per sae,
 * but valid conditions such as EOF or EOC (End of Chunk).
 */
#define IFFERR_EOF        -1L   /* Reached logical end of file  */
#define IFFERR_EOC        -2L   /* About to leave context       */
#define IFFERR_NOSCOPE    -3L   /* No valid scope for property  */
#define IFFERR_NOMEM      -4L   /* Internal memory alloc failed */
#define IFFERR_READ       -5L   /* Stream read error            */
#define IFFERR_WRITE      -6L   /* Stream write error           */
#define IFFERR_SEEK       -7L   /* Stream seek error            */
#define IFFERR_MANGLED    -8L   /* Data in file is corrupt      */
#define IFFERR_SYNTAX     -9L   /* IFF syntax error             */
#define IFFERR_NOTIFF     -10L  /* Not an IFF file              */
#define IFFERR_NOHOOK     -11L  /* No call-back hook provided   */
#define IFF_RETURN2CLIENT -12L  /* Client handler normal return */

/*****************************************************************************/


#define MAKE_ID(a,b,c,d)        \
        ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))

/* Universal IFF identifiers */
#define ID_FORM         MAKE_ID('F','O','R','M')
#define ID_LIST         MAKE_ID('L','I','S','T')
#define ID_CAT                  MAKE_ID('C','A','T',' ')
#define ID_PROP         MAKE_ID('P','R','O','P')
#define ID_NULL         MAKE_ID(' ',' ',' ',' ')

/* Identifier codes for universally recognized local context items */
#define IFFLCI_PROP             MAKE_ID('p','r','o','p')
#define IFFLCI_COLLECTION       MAKE_ID('c','o','l','l')
#define IFFLCI_ENTRYHANDLER     MAKE_ID('e','n','h','d')
#define IFFLCI_EXITHANDLER      MAKE_ID('e','x','h','d')


/*****************************************************************************/


/* Control modes for ParseIFF() function */
#define IFFPARSE_SCAN    0L
#define IFFPARSE_STEP    1L
#define IFFPARSE_RAWSTEP 2L

/*****************************************************************************/


/* Magic value for writing functions. If you pass this value in as a size
 * to PushChunk() when writing a file, the parser will figure out the
 * size of the chunk for you. If you know the size, is it better to
 * provide as it makes things faster.
 */
#define IFFSIZE_UNKNOWN -1L

/*-----------------------------------------------------------------*/
#endif



