#ifndef TYPES_H
#define TYPES_H
/*
**  $Source: PRG:VFM/Include/types.h,v $
**  $Revision: 38.9 $
**  $Date: 1995/07/08 00:19:39 $
**  $Locker:  $
**  $Author: floh $
**
**  27-Aug-93   floh    37.01   ColorTag-Typedef eingeführt
**  07-Sep-93   floh    37.02   P2DF_BehindViewer eingeführt
**                              P2DF_Done eingeführt
**  11-Sep-93   floh    37.03   >types.h< befindet sich jetzt
**                              nicht im >Include/Resources<-Directory,
**                              sondern im >Include<-Directory. Alle
**                              relevanten Files wurden dementsprechend
**                              geändert.
**  02-Oct-93   floh    38.1    Revisionskontrolle an RCS übergeben.
**
**
**  Dieses File enthält elementare Datentyp-Definitionen
**  im Zusammenhang mit Resources.
**
**  (C) Copyright 1993 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*-----------------------------------------------------------------*/
typedef struct Pixel3D {    /* Obsolete */
    LONG flags;
    LONG x, y, z;
}Pixel3D;

typedef struct fp3d {
    LONG flags;
    FLOAT x,y,z;
}fp3d;

#define P3DF_DONE   0x80000000  /* = Bit 31 gesetzt, LONG also negativ */
#define P3DF_READY  P3DF_DONE
#define P3DF_END    P3DF_DONE

#define CLIP3D_LEFTOUT  (1<<0)  /* Die 3D-Clipflags kommen ebenfalls */
#define CLIP3D_RIGHTOUT (1<<1)  /* in das Flag-Feld */
#define CLIP3D_TOPOUT   (1<<2)
#define CLIP3D_BOTOUT   (1<<3)
#define CLIP3D_BEHIND   (1<<4)
#define CLIP3D_TOOFAR   (1<<5)

/*-----------------------------------------------------------------*/
typedef struct Pixel2D {
    WORD  x,y;
    LONG  zbuffer;
    ULONG color;
    UBYTE mapx, mapy;
    WORD  flags;
}Pixel2D;

/*
**  Die folgenden Flag-Definitionen gelten
**  für Pixel2D->flags
*/
#define P2DF_Done           0x8000  /* Ende des 2DPool, Element    */
                                    /* enthält KEINE gültigen      */
                                    /* Koordinaten mehr            */

/*-------------------------------------------------------------------
**  Die neue Point2D-Struktur, die mittelfristig die Pixel2D-
**  Struktur ersetzen wird.
*/
typedef struct Point2D {
    WORD x,y;           // Display-Position
    LONG zbuffer;       // Parallel-Entfernung des Punkts
    ULONG color;        // Helligkeits-Wert (0..255)
    UWORD mapx,mapy;    // Map-Koords (8.8)
}Point2D;

// Endekennzeichen: (Point2D.zbuffer < 0)

/*-----------------------------------------------------------------*/
#endif


