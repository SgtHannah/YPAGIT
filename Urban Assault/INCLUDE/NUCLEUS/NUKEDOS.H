#ifndef NUCLEUS_NUKEDOS_H
#define NUCLEUS_NUKEDOS_H
/*
**  $Source: PRG:VFM/Include/nucleus/nukedos.h,v $
**  $Revision: 38.7 $
**  $Date: 1996/11/10 20:50:26 $
**  $Locker:  $
**  $Author: floh $
**
**  Allgemeine Includes für NukeDOS-Modul.
**
**  24-Sep-93   floh    37.01   iff_msg definiert
**  02-Oct-93   floh    38.1    Revisionskontrolle jetzt bei RCS
**
**  (C) Copyright 1993 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

/*-------------------------------------------------------------------
**  FORM-ID's
*/
#define FORM_MC2            MAKE_ID ('M','C','2',' ')
#define FORM_Object         MAKE_ID ('O','B','J','T')

/*-------------------------------------------------------------------
**  Chunk-IDs
*/
#define CHUNK_ClassID       MAKE_ID ('C','L','I','D')

/*-------------------------------------------------------------------
**  Standard-Assigns (die im statischen Link-Modell nur als
**  Prefix für die Code-Segment-Namen zu verstehen sind!);
**  (BITTE DIE SYSTEM PATHS VERWENDEN).
*/
#define ASSIGN_classes      "MC2classes:"
#define ASSIGN_engines      "MC2engines:"

#ifdef __DOS__
#define ASSIGN_resources    "MC2RES\\"
#endif

#ifdef __WINDOWS__
#define ASSIGN_resources    "MC2RES\\"
#endif

#ifdef AMIGA
#define ASSIGN_resources    "MC2resources:"
#endif

/*-------------------------------------------------------------------
**  Fehlercodes von NukeDOS (nicht mehr unterstützt)
*/
#define NDOSerr_NoError         0L  /* Sinnlos, ich weiß! */
#define NDOSerr_AllocIFF        1L  /* Fehler bei AllocIFF() */
#define NDOSerr_Open            2L  /* Fehler beim File-öffnen */
#define NDOSerr_IFFerror        3L  /* allgemeiner IFF-Error, dieser wird */
                                    /* in >IFFerror< abgelegt (GetIFFError()) */
#define NDOSerr_NoMC2file       4L  /* keine MC2-FORM */
#define NDOSerr_NoObjectFile    5L  /* kein OBJT-FORM */
#define NDOSerr_UnknownClassID  6L  /* geforderte Klasse noch nicht */
                                    /* angemeldet */
#define NDOSerr_PushChunk       7L  /* Fehler bei PushChunk() */
#define NDOSerr_InsideObject    8L  /* Fehler innerhalb Methoden-Ausführung */
                                    /* nur bei OM_SAVETOIFF */
#define NDOSerr_Write           9L  /* allgemeiner Fehler bei Write() */

/*-------------------------------------------------------------------
**  private Message-Strukturen für Methoden-Dispatcher
*/
struct iff_msg {
    struct IFFHandle *iffhandle;
};

/*-------------------------------------------------------------------
**  Die System-Path-Identifiers, siehe _GetSysPath() und _SetSysPath()
*/
#define NC_NUM_SYSPATHS     (3)

#define SYSPATH_RESOURCES   (0)
#define SYSPATH_CLASSES     (1)
#define SYSPATH_ENGINES     (2)

#endif
