#ifndef RESOURCES_RESOURCES_H
#define RESOURCES_RESOURCES_H
/*
**  $Source: PRG:VFM/Include/resources/resources.h,v $
**  $Revision: 38.7 $
**  $Date: 1994/02/28 14:23:50 $
**  $Locker:  $
**  $Author: floh $
**
**  Allgemeine LowLevel-Definitionen für Resourcen.
**
**  (C) Copyright 1993 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include "exec/types.h"
#endif

#ifndef EXEC_NODES_H
#include "exec/nodes.h"
#endif

#ifndef UTILITY_TAGITEM_H
#include "utility/tagitem.h"
#endif

#ifndef TYPES_H
#include "types.h"
#endif

/*-------------------------------------------------------------------
**  Die Resource-Tags, diese werden z.B. von der
**  ResourceEngine-Routine OpenResource() akzeptiert.
*/
#define RESOURCE_BASE	  (TAG_USER)
#define RESOURCE_NAME	  (RESOURCE_BASE+1)
#define RESOURCE_TYPE	  (RESOURCE_BASE+2)
#define RESOURCE_ACCESS   (RESOURCE_BASE+3)

/*-------------------------------------------------------------------
**  Resource-Types
*/
#define RES_BASE	(TAG_USER)
#define RES_BITMAP	(RES_BASE+1)
#define RES_3DPOOL	(RES_BASE+2)
#define RES_SAMPLE	(RES_BASE+3)

/*-------------------------------------------------------------------
**  Die Zugriffsarten auf Resourcen
*/
#define ACCESS_SHARED	    1L
#define ACCESS_EXCLUSIVE    2L

/*-------------------------------------------------------------------
**  Dies ResourceNode ist Element der ResourceList. In dieser werden
**  alle geöffneten Resources (shared und exclusive) gehalten.
*/
struct ResourceNode {
    struct Node Node;	/* ln_Name = Name der Resource */
    ULONG Type; 	/* eins von RES_BITMAP, RES_3DPOOL, RES_SAMPLE */
    ULONG Access;	/* eins von ACCESS_SHARED, ACCESS_EXCLUSIVE */
    ULONG OpenCount;	/* wichtig für Shared Resources */
    struct ResourceDummy *Resource;  /* Pointer auf Resource-Struktur */
};

/*-------------------------------------------------------------------
**  Dummy-Resource für Zugriff auf ResourceNode von allgemeiner
**  Resource aus.
*/
struct ResourceDummy {
    struct ResourceNode *ResNode;   /* TABU */
    APTR Data;			    /* allgemeiner Pointer auf Daten-Body */
};

/*-------------------------------------------------------------------
**  ResourceNode->Resource zeigt auf eins von:
**  PS: alle Resource-Strukturen müssen als erstes Feld
**	>struct ResourceNode *ResNode< haben !!!
**	(für allgemeinen Zugriff von außen).
*/
struct ResourceBitmap {
    struct ResourceNode *ResNode;   /* TAAABUUU! */
    APTR Data;		/* Pointer auf Chunky Pixelmap */
    UWORD Width;	/* Breite in Pixel */
    UWORD Height;	/* Höhe in Pixel */
    ULONG ColorModel;	/* eins von CM_LOWCOLOR, CM_HICOLOR, CM_TRUECOLOR */
    ULONG BytesPerRow;	/* NUR bei CM_LOWCOLOR == Width */
    UWORD *LineOffsets; /* Pointer auf UWORD-Offsets der einzelnen Zeilen */
			/* relativ zu >Data< */
};

/*
**  ResourceBitmap->Data zeigt auf den Bitmap-Body, also die eigentlichen
**  Bilddaten. Der Body sieht je nach ColorModel anders aus:
**	CM_LOWCOLOR -> 1 Pixel = 1 UBYTE, in dem die Register-Nummer
**		       der Color Lookup Table steht. 0 ist
**		       durchsichtig (1 Bit Alphachannel bzw. Maske)
**	CM_HICOLOR  -> 1 Pixel = 1 UWORD, ohne CLUT. Bit 15 ist
**		       Maske bzw. 1 Bit Alphachannel
**	CM_TRUECOLOR-> 1 Pixel = 1 ULONG, ohne CLUT. Bits 24-31 sind für
**				 8-Bit-Alphachannel-Support reserviert.
*/

struct Resource3DPool {
    struct ResourceNode *ResNode;   /* TABU !!! */
    Pixel3D *Data;	/* Pixel3D-Array von 3D-Koordinaten */
    ULONG NumElements;	/* einschließlich Ende-Element */
};

struct ResourceSample {
    struct ResourceNode *ResNode;   /* TABU !!! */
    APTR Data;		/* Pointer auf 8-Bit-Sampledaten */
    ULONG NumBytes;	/* Länge des Samples in Bytes */
};

/*-----------------------------------------------------------------*/
#endif


