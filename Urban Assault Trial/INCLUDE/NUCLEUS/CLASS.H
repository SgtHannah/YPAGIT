#ifndef NUCLEUS_CLASS_H
#define NUCLEUS_CLASS_H
/*
**  $Source: PRG:VFM/Include/nucleus/class.h,v $
**  $Revision: 38.11 $
**  $Date: 1998/01/06 14:01:47 $
**  $Locker:  $
**  $Author: floh $
**
**  27-Aug-93   floh    INST_DATA(cl,o) Macro
**  28-Aug-93   floh    NCTAG_DONE umbenannt nach OVERLOAD_DONE
**  21-Sep-93   floh    - GET_Class eingeführt, das ist die
**                        Global Entry Table eines Class-Dispatchers,
**                        da dieser ja als Executable-Modul dynamisch
**                        eingelinkt werden muß.
**                      - Class-Struktur erweitert, um einige NukeDOS-
**                        Parameter aufzunehmen.
**  02-Oct-93   floh    Revisionskontrolle an RCS übergeben.
**
**
**  (C) Copyright 1993 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef NUCLEUS_SYS_H
#include "nucleus/sys.h"
#endif

#ifndef NUCLEUS_SEGMENT_H
#include "nucleus/segment.h"
#endif

/*----------------------------------------------------------------------*/
#define NUCLEUS_NUMMETHODS  1024    /* Anzahl der max. Anzahl Methoden*/
                                    /* pro Klasse */
#define METHOD_DISTANCE     0x40    /* max. Anzahl Methoden pro Subklasse */
#define ATTRIB_DISTANCE     0x1000  /* max. Anzahl Attribute pro Subklasse */

/*----------------------------------------------------------------------
** ein paar ziemlich abstrakte TypeDefs...
*/
#ifndef NUCLEUS_BOOPSI_FRIENDLY
    typedef struct NClass Class;
    typedef void Msg;
    typedef ULONG Object;
    typedef UBYTE *ClassID;
#endif

/*----------------------------------------------------------------------
**  Die neue Methoden-Array-Eintrag-Struktur (vom 14-Nov-93).
*/
struct MethodArrayEntry {
    #ifdef AMIGA
        ULONG __asm (*dispatcher) (__a0 Object *, __a1 Class *, __a2 void *);
    #else
        ULONG (*dispatcher) (Object *, Class *, void *);
    #endif
    Class *trueclass;
};

/*------------------------------------------------------------------------
**  Die GET-Funktion "MakeExternClass()" eines Class-Dispatchers
**  gibt einen Pointer auf eine ClassInfo-Struktur zurück.
*/
struct ClassInfo {
    UBYTE *superclassid;
    ULONG *methods;         /* die Methoden-Dispatcher */
    UWORD instsize;         /* die Größe der LID */
    ULONG flags;            /* wird nach Class->flags kopiert */
};

/*------------------------------------------------------------------------
**  Die Nucleus-Klassen-Struktur:
*/
#ifndef NUCLEUS_BOOPSI_FRIENDLY
struct NClass {
    struct Node node;           /* ln_Name enthält ClassID */

    Class *superclass;          /* Pointer auf SuperClass */

    UWORD instoffset;           /* von Object-Anfang auf LID */
    UWORD instsize;             /* Locale Instance Data Größe */

    APTR userdata;

    ULONG classcount;           /* so viele MakeClass()-es */
    ULONG subclasscount;        /* so viele SubClasses existieren */
    ULONG objectcount;          /* so viele Objects existieren */

    ULONG flags;                /* noch nicht definiert */

    struct MethodArrayEntry *methods;

    struct SegmentHandle seg_handle;    /* für nc_Open/CloseSegment() */
};
#endif

/*----------------------------------------------------------------------*/
#ifndef NUCLEUS_BOOPSI_FRIENDLY
#define INST_DATA(cl,o)     ((VOID *) (((UBYTE *)o)+cl->instoffset))
#endif

/*----------------------------------------------------------------------*/
#endif

