#ifndef RESOURCES_EMBEDCLASS_H
#define RESOURCES_EMBEDCLASS_H
/*
**  $Source: PRG:VFM/Include/resources/embedclass.h,v $
**  $Revision: 38.1 $
**  $Date: 1996/01/22 22:14:25 $
**  $Locker:  $
**  $Author: floh $
**
**  Die embed.class bettet Resourcen in der Shared Resource
**  Liste in ein Diskobject ein.
**
**  (C) Copyright 1996 by A.Weissflog
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

#ifndef RESOURCES_RSRCCLASS_H
#include "resources/rsrcclass.h"
#endif

/*-------------------------------------------------------------------
**  Die ClassID der embed.class
*/
#define EMBED_CLASSID   "embed.class"

/*-------------------------------------------------------------------
**  NAME
**      embed.class - bettet Resources in Diskobjects ein
**
**  SUPERCLASS
**      nucleus.class
**
**  FUNCTION
**      Innerhalb OM_NEW erzeugt ein embed.class Object eine
**      Liste von Objects, die jeweils eine Resource
**      in der globalen Shared Resource Liste der rsrc.class
**      einbetten. Bei einem OM_SAVETOIFF sichert sich dann
**      das embed.class Object als "normales" Disk-Object,
**      für jedes Object in der Resourcen-Liste wird
**      die Klasse und die Resource-ID gesichert, danach
**      wird das jeweilige Object per RSM_SAVE aufgefordert,
**      seine Resource in denselben IFF-Stream zu sichern.
**
**      Bei einem OM_NEWFROMIFF werden dann für jedes
**      Object im Object-Chunk ein neues Resource-Object
**      erzeugt, welches seine Resource jeweils aus dem
**      Object-IFF-Stream restauriert.
**
**      That's it. Damit kann man (mit etwas Hilfe von der
**      base.class ALLE benötigten Resourcen zusammen mit
**      dem Objekt selbst in EINEM File haben.
**
**      Bitte beachten, daß die embed.class zwar Resourcen
**      "bearbeitet", aber deshalb noch lange nicht Siubklasse
**      der rsrc.class ist.
**
**  METHODS
**      OM_NEW
**      OM_DISPOSE
**      OM_SAVETOIFF
**      OM_NEWFROMIFF
**
**      keine eigenen Methoden
**
**  ATTRIBUTES
**
**      keine eigenen Attribute
*/

/*-----------------------------------------------------------------*/
#define EBM_BASE            (OM_BASE+METHOD_DISTANCE)
/*-----------------------------------------------------------------*/
#define EBA_BASE            (OMA_BASE+METHOD_DISTANCE)

/*-------------------------------------------------------------------
**  Ein embed.class Object erzeugt eine interne "gepufferte"
**  Liste mit Platz für 32 Resource-Objects pro Node:
*/
#define EBC_NUM_NODEOBJECTS   (32)

struct EmbedNode {
    struct MinNode nd;
    ULONG num_objects;
    Object *o_array[EBC_NUM_NODEOBJECTS];
};

/*-------------------------------------------------------------------
**  LID eines embed.class Objects
*/
struct embed_data {
    struct MinList object_list;
};

/*-------------------------------------------------------------------
**  IFF-Stuff
**  ~~~~~~~~~
**  Ein embed.class Object Chunk besteht jeweils aus einem
**  Resource Description Chunk und einem nachfolgenden
**  Resource-Class-spezifischen Daten-Chunk oder -Form.
**  Falls die jeweilige Resource-Class nicht in der Lage
**  ist, sich in einen IFF-Stream zu sichern, kann der
**  Daten-Chunk auch wegfallen.
**  Im Resource Description Chunk steht der Name der
**  Handler-Klasse und die Resource-ID, das sind
**  alle benötigten Informationen zum Erzeugen des
**  Resource-Objects.
*/
#define EMBEDIFF_FORMID     MAKE_ID('E','M','B','D')
#define EMBEDIFF_Rsrc       MAKE_ID('E','M','R','S')

/*
**  Der EMBEDIFF_Rsrc Chunk besteht aus zwei
**  normalen C-Strings, durch eine NULL getrennt,
**  der letzte String ist mit einer Doppel-Null
**  abgeschlossen:
**
**  char RsrcChunk[n] = {
**      "handler.class",0,
**      "resource_id",0,0
**  };
*/

/*-----------------------------------------------------------------*/
#endif

