#ifndef NUCLEUS_SEGMENT_H
#define NUCLEUS_SEGMENT_H
/*
**  $Source: PRG:VFM/Include/nucleus/segment.h,v $
**  $Revision: 38.3 $
**  $Date: 1995/05/16 22:46:16 $
**  $Locker:  $
**  $Author: floh $
**
**  Definition für Nucleus-Code-Segmente.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifdef AMIGA
#ifndef DOS_DOS_H
#include <dos/dos.h>    /* für BCPL Definition */
#endif
#endif

#ifndef NUCLEUS_SYS_H
#include "nucleus/sys.h"
#endif

/*-------------------------------------------------------------------
**  Background
**  ~~~~~~~~~~
**  Die Nucleus-Runtime-Umgebung besteht aus einzelnen Code-Modulen,
**  zur Zeit sind das die einzelnen Klassen-Dispatcher und
**  die Engines. Diese Code-Module sind die 'Segmente'. Dabei ist
**  es "von außen gesehen" egal, ob die Nucleus-Runtime-Umgebung
**  dynamisch oder statisch gelinkt ist, sämtliche systemspezifischen
**  Aspekte sind durch die internen Nucleus-Routinen nc_OpenSegment()
**  und nc_CloseSegment() gekapselt.
**
**  Eine dynamische Umgebung (z.B. auf dem Amiga) lädt innerhalb von
**  nc_OpenSegment() das entsprechende Code-Segment aus dem Filesystem,
**  initialisiert dieses auf irgendeine "magische" Art und Weise
**  und gibt einen Pointer auf die "Global Entry Table" des neu
**  geladenene Segments zurück.
**
**  Eine statische Umgebung (z.B. unter MS-DOS) durchsucht die
**  interne Segment-Liste (die während nc_OpenNucleus() angelegt wurde)
**  nach dem entsprechenden Segment-Eintrag, der Rest läuft genauso
**  ab, wie in einer dynamischen Umgebung.
**
**  Die "Global Entry Table" ist eine normale Einsprungtabelle und
**  ist für die beiden Segment-Typen "Class-Dispatcher" und
**  "Engine" standardisiert. Auf der Ebene von nc_OpenSegment() und
**  nc_CloseSegment() ist diese Unterscheidung allerdings unerheblich,
**  hier ist es einfach ein normales ULONG[].
*/

/*-------------------------------------------------------------------
**  Statisch gelinkte "Nucleusse" haben eine interne Liste aller
**  Code-Segmente. Diese Liste muß innerhalb von nc_OpenNucleus()
**  mittels nc_AddSegment() aufgebaut werden. Die Liste besteht
**  aus einzelnen <struct SegInfo>'s.
*/
#ifdef STATIC_LINKING
struct SegInfo {
    struct Node node; /* zum Aufbau der Segment-Liste, ln_Name = SegName! */
    ULONG *(*entry) (void);  /* der Einsprung-Punkt dieses Segments */
                             /* liefert Pointer auf GET */
};
#endif

/*-------------------------------------------------------------------
**  Das "SegmentHandle" ist eine systemspezifische Struktur und
**  hält spezielle Parameter, die zwischen nc_OpenSegment() und
**  nc_CloseSegment() aufbewahrt werden müssen.
*/
#ifdef AMIGA
struct SegmentHandle {
    BPTR dosseg;        /* BCPL-Pointer, wie von Amiga-DOS verlangt */
    ULONG *GET;         /* die Global-Entry-Table dieses Moduls */
};
#endif

#ifdef STATIC_LINKING
struct SegmentHandle {
    struct SegInfo *seginfo;    /* Pointer auf Element in Segment-Liste */
    ULONG *GET;                 /* die Global-Entry-Table dieses Moduls */
};
#endif

/*-------------------------------------------------------------------
**  Hier die zwei Standard-GET's, für Class-Dispatcher-Segmente
**  und für Engine-Segmente.
*/
struct GET_Class {
    struct ClassInfo *(*MakeExternClass) (ULONG,...);
    BOOL (*FreeExternClass) (void);
};

struct GET_Engine {
    BOOL (*Open) (ULONG id,...);
    void (*Close) (void);
    void (*SetAttrs) (ULONG attrs,...);
    void (*GetAttrs) (ULONG attrs,...);
};

/*-----------------------------------------------------------------*/
#endif



