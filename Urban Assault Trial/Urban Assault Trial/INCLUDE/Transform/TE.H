#ifndef TRANSFORM_TE_H
#define TRANSFORM_TE_H
/*
**  $Source: PRG:VFM/Include/transform/te.h,v $
**  $Revision: 38.3 $
**  $Date: 1995/02/05 21:40:34 $
**  $Locker:  $
**  $Author: floh $
**
**  Standard-Definitionen für Transformer-Engines.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*-------------------------------------------------------------------
**  Base-Tags
**
**  C -> Configurable
**  S -> Settable
**  G -> Gettable
*/
#define TET_BASE    (TAG_USER+0x2000)

#define TET_GET_SPEC    (TET_BASE+1)    /* (G) */

#define TET_ZoomX   (TET_BASE+2)    /* (CSG) */
#define TET_ZoomY   (TET_BASE+3)    /* (CSG) */
#define TET_MaxZ    (TET_BASE+4)    /* (CSG) */
#define TET_MinZ    (TET_BASE+5)    /* (CSG) */

/*-------------------------------------------------------------------
**  Config-Stuff
*/
#define TECONF_MaxZ     "tform.backplane"   /* INTEGER */
#define TECONF_MinZ     "tform.frontplane"  /* INTEGER */
#define TECONF_ZoomX    "tform.zoomx"       /* INTEGER */
#define TECONF_ZoomY    "tform.zoomy"       /* INTEGER */

/*-----------------------------------------------------------------*/
#endif

