#ifndef OV_ENGINE_H
#define OV_ENGINE_H
/*
**  $Source: PRG:VFM/Include/visualstuff/ov_engine.h,v $
**  $Revision: 38.23 $
**  $Date: 1998/01/06 14:11:20 $
**  $Locker:  $
**  $Author: floh $
**
**  ALLGEMEINE OutputVisualEngine-Definitionen
**
**  (C) Copyright 1993,1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*-------------------------------------------------------------------
**  Engine-Tags
**
**  C -> Configurable
**  S -> Settable
**  G -> Gettable
*/
#define OVET_BASE       (TAG_USER+0x3000)

#define OVET_GET_SPEC   (OVET_BASE+1)

#define OVET_Mode       (OVET_BASE+2)   // (CG)
#define OVET_XRes       (OVET_BASE+3)   // (CG)
#define OVET_YRes       (OVET_BASE+4)   // (CG)
#define OVET_ToFront    (OVET_BASE+5)   // (S) (Amiga only)
#define OVET_ToBack     (OVET_BASE+6)   // (S) (Amiga only)
#define OVET_ModeInfo   (OVET_BASE+7)   // (G) Ptr auf interne ModeInfo-Struktur
                                        // IBM only!!!
#define OVET_Debug      (OVET_BASE+8)   /* (CSG) */

#define OVET_TracyRemap (OVET_BASE+9)   // <struct VFMBitmap *> auf 
                                        // Default-Transparenz-Remap-Table
#define OVET_ShadeRemap (OVET_BASE+10)  // <struct VFMBitmap *> auf
                                        // Default-Shader-Remap-Table
#define OVET_Palette    (OVET_BASE+11)  // (G) Ptr auf akt. Palette
#define OVET_Display    (OVET_BASE+12)  // (G) Ptr auf Frame-Buffer

#define OVET_Object     (OVET_BASE+13)  // (G) Ptr auf eingebettetes Object

/*-------------------------------------------------------------------
**  Config-Stuff
*/
#define OVECONF_Mode            "gfx.mode"              /* INTEGER */
#define OVECONF_XRes            "gfx.xres"              /* INTEGER */
#define OVECONF_YRes            "gfx.yres"              /* INTEGER */
#define OVECONF_Debug           "gfx.debug"             /* BOOL */
#define OVECONF_ColorMap        "gfx.palette"           /* STRING */
#define OVECONF_Depth           "gfx.depth"             /* INTEGER */
#define OVECONF_Interlace       "gfx.interlace"         /* BOOL */
#define OVECONF_SpansPerLine    "gfx.spans_per_line"    /* INTEGER */
#define OVECONF_Display         "gfx.display"           /* STRING */
#define OVECONF_Display2        "gfx.display2"          /* STRING */

/*-------------------------------------------------------------------
**  div. Strukturen
*/
struct RGB_Triplet {
    UBYTE r,g,b;
};

/*-----------------------------------------------------------------*/
#endif


