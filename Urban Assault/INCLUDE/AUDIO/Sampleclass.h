#ifndef AUDIO_SAMPLECLASS_H
#define AUDIO_SAMPLECLASS_H
/*
**  $Source: PRG:VFM/Include/audio/sampleclass.h,v $
**  $Revision: 38.2 $
**  $Date: 1996/04/18 23:35:01 $
**  $Locker:  $
**  $Author: floh $
**
**  Die sample.class ist die RootClass aller "Sample-Daten-
**  Container-Klassen".
**
**  (C) Copyright 1996 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef RESOURCES_RSRCCLASS_H
#include "resources/rsrcclass.h"
#endif

#ifndef AUDIO_SAMPLE_H
#include "audio/sample.h"
#endif

/*-------------------------------------------------------------------
**
**  NAME
**      sample.class -- Stammklasse aller Audio-"Container"-Klassen
**
**  FUNCTION
**      Die sample.class stellt eine einheitliches Interface
**      für Audio-Sample-Daten zur Verfügung.
**
**  METHODS
**      nucleus.class
**      ~~~~~~~~~~~~~
**      OM_NEW
**      OM_DISPOSE
**      OM_GET
**      OM_SAVETOIFF
**      OM_NEWFROMIFF
**
**      rsrc.class
**      ~~~~~~~~~~
**      RSM_CREATE
**      RSM_FREE
**
**      sample.class
**      ~~~~~~~~~~~~
**      SMPM_OBTAIN
**          Msg:    struct smpobtain_msg
**          Ret:    ausgefüllte Msg
**
**          Diese Methode ist ähnlich BMM_BMPOBTAIN der
**          bitmap.class und erlaubt die Abfrage von Daten
**          über das eingebettete Audio-Sample.
**
**          In:
**              msg.time_stamp  -> globaler Timestamp des aktuellen Frames
**              msg.frame_time  -> Zeitdauer des aktuellen Frames
**
**          Out:
**              msg.sample  -> Ptr auf VFMSample-Struktur
**
**
**  ATTRIBUTES
**      SMPA_Sample (G) - <struct VFMSample *>
**          Returniert einen Pointer auf die eingebettete
**          VFMSample-Struktur.
**
**      SMPA_Type (IG) - <SMPT_8BIT_MONO>
**          Typ-Information über das eingebettete
**          Sample. Bis jetzt wird nur SMPT_8BIT_MONO
**          unterstützt.
**
**      SMPA_Length (IG) - <ULONG>
**          Länge des rohen Sample-Buffers in BYTES!
**
**      SMPA_Body (IG) - <void *>
**          Pointer auf die rohen Sample-Daten selbst. Wird dieses
**
**  NOTE
**      Die sample.class unterstützt kein _load()/_save(),
**      das ist Sache der spezifischen Subklassen.
**-----------------------------------------------------------------*/
#define SMPM_BASE   (RSM_BASE+METHOD_DISTANCE)

#define SMPM_OBTAIN (SMPM_BASE)

/*-----------------------------------------------------------------*/
#define SMPA_BASE   (RSA_BASE+ATTRIB_DISTANCE)

#define SMPA_Sample (SMPA_BASE)     /* (G)  */
#define SMPA_Type   (SMPA_BASE+1)   /* (IG) */
#define SMPA_Length (SMPA_BASE+2)   /* (IG) */
#define SMPA_Body   (SMPA_BASE+3)   /* (IG) */

/*-----------------------------------------------------------------*/
#define SAMPLE_CLASSID "sample.class"

/*-----------------------------------------------------------------*/
struct sample_data {
    struct VFMSample *sample;   /* Ptr auf VFMSample */
};

/*-------------------------------------------------------------------
**  Msg-Strukturen
*/
struct smpobtain_msg {
    LONG time_stamp;            /* In */
    LONG frame_time;            /* In */
    struct VFMSample *sample;   /* Out */
};

/*-----------------------------------------------------------------*/
#endif

