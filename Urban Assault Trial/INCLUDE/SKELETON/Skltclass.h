#ifndef SKELETON_SKLTCLASS_H
#define SKELETON_SKLTCLASS_H
/*
**  $Source: PRG:VFM/Include/skeleton/skltclass.h,v $
**  $Revision: 38.6 $
**  $Date: 1996/11/10 20:55:02 $
**  $Locker:  $
**  $Author: floh $
**
**  Die sklt.class l�dt das VFM-eigene SKLT Format als
**  Inhalt eines normalen skeleton.class Objects.
**
**  (C) Copyright 1994,1995,1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef SKELETON_SKELETONCLASS_H
#include "skeleton/skeletonclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      sklt.class - Loader Klasse f�r IFF SKLT Format
**
**  FUNCTION
**      Erzeugt eine Skeleton-Struktur aus dem IFF-File, diese
**      Struktur wird in das sklt-Objekt als Resource eingekapselt.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OM_NEW
**          RSA_Name -
**              Definiert die Resource-ID des zu ladenden
**              Files. Wird das sklt.class Object aus einem
**              Standalone File erzeugt, idt RSA_Name
**              gleichzeitig der Filename relativ zu
**              MC2resources:
**
**          RSA_IFFHandle
**              Mu� definiert sein, wenn das Object aus
**              einem Collection-File erzeugt werden
**              soll.
**
**          Beachte: Die sklt.class ben�tigt kein eigenes
**          OM_NEW oder OM_DISPOSE, weil diese Aufgaben
**          von den Resource-Konstruktoren RSM_CREATE
**          und RSM_FREE �bernommen werden.
**
**      OM_SAVETOIFF
**          Kompatibel zum alten Format, allerdings wird keine
**          Superklasse mehr unterst�tzt, so da� ein eventuell
**          vorhandener Objekt-Name (OA_Name) der nucleus.class
**          verloren geht. Hier mu� mal noch eine g�ngige
**          und kompatible L�sung her (wie, eine Klasse �bergibt
**          alle unbekannten Chunks an die Superklasse z.B.)
**
**      OM_NEWFROMIFF
**          Wie gehabt.
**
**      RSM_SAVE
**          Wird voll unterst�tzt.
**
**  NOTE:
**      Die Klasse het keine eigene Locale Instance Data und keine
**      Methoden/Attribute. Sie �bernimmt nur das Laden eines
**      SKLT-Files, den Rest macht die Superklasse.
*/
/*-------------------------------------------------------------------
**  ClassID
*/
#define SKLT_CLASSID "sklt.class"
/*-----------------------------------------------------------------*/
#define SKLTM_BASE          (SKLM_BASE+METHOD_DISTANCE)
/*-----------------------------------------------------------------*/
#define SKLTA_BASE          (SKLA_BASE+ATTRIB_DISTANCE)
/*-------------------------------------------------------------------
**  IFF-Stuff
**  ~~~~~~~~~
**  Als einzige Information wird die Resource-ID persistent
**  gemacht.
*/
#define SKLTCLIFF_FORMID    MAKE_ID('S','K','L','C')
#define SKLTCLIFF_NAME      MAKE_ID('N','A','M','E')

/*-----------------------------------------------------------------*/
#endif
