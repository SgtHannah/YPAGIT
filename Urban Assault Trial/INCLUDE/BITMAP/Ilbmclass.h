#ifndef BITMAP_ILBMCLASS_H
#define BITMAP_ILBMCLASS_H
/*
**  $Source: PRG:VFM/Include/bitmap/ilbmclass.h,v $
**  $Revision: 38.9 $
**  $Date: 1998/01/06 12:54:22 $
**  $Locker:  $
**  $Author: floh $
**
**  Die ilbm.class ist die konkrete Loader-Klasse für das
**  IFF-ILBM-Formatbis 256 Farben. Sie realisiert außerdem ein 
**  vollständiges Resource-Sharing. Die ilbm.class ist eine 
**  Subklasse der bitmap.class.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef BITMAP_BITMAPCLASS_H
#include "bitmap/bitmapclass.h"
#endif

/*-------------------------------------------------------------------
**  NAME
**      ilbm.class - Loader Klasse für IFF ILBM Format.
**
**  FUNCTION
**      Die ilbm.class erzeugt ein Bitmap-Objekt aus einem
**      IFF-ILBM-File, unterstützt aber auch das von der
**      rsrc.class geerbte Resource-Sharing (so daß, wenn
**      nicht anders angefordert, ein Bild nur genau
**      einmal von Disk geladen wird).
**
**      Außer dem ILBM-Format unterstützt die ilbm.class
**      transparent ein zweites, privates Fileformat
**      namens VBMP, das direkter auf die "Bedürfnisse"
**      von VFM zugeschnitten ist (schnelles Laden, weil
**      ohne Kompression und im Chunky-Format).
**
**      Die ilbm.class unterstützt außerdem die RSM_SAVE
**      Methode der rsrc.class und kann damit die eingebettete
**      Bitmap-Resource als Standalone-File oder als Teil
**      einer Resource-Collection sichern.
**
**      Außerdem kann sich ein ilbm.class Objekt per OM_SAVETOIFF
**      persistent machen (als erstes in der Vererbungs-Hierarchie).
**
**      Die ilbm.class erzeugt unabhängig vom Status des
**      BMA_HasColorMap Attrs eine ColorMap, sobald der
**      File Colormap Informationen enthält.
**
**  METHODS
**      Im folgenden nur ein paar Worte zu ausgewählten Methoden,
**      die unterstützt werden, ansonsten verhält sich ein ilbm.class
**      Objekt auch wie ein normales rsrc.class bzw. bitmap.class
**      Objekt [Wunder der Vererbung ;-)].
**
**      OM_NEW
**          RSA_Name - 
**              Definiert die Resource-ID des geladenen
**              Files. Wird das ilbm.class Objekt aus einem
**              Standalone-File erzeugt (ILBM oder VBMP Format),
**              ist RSA_Name gleichzeitig der Filename relativ
**              zu MC2resources:
**
**          RSA_IFFHandle -
**              Wenn vorhanden, wird das Objekt nicht aus einem
**              Standalone-File erzeugt, sondern aus dem
**              übergebenen IFF-Stream als Teil eines Collection-
**              Files. Erlaubt sind hier sowohl ILBM, als auch
**              VBMP Format, RSM_SAVE unterstützt aber nur
**              VBMP (und anders kann man eine Collection bisher
**              nicht erzeugen).
**
**          BMA_Outline -
**              Wird durchgereicht an bitmap.class, die ilbm.class
**              beachtet die Outline aber beim persistent machen.
**
**          BMA_HasColorMap -
**              Wenn der File Paletten-Information besitzt,
**              wird so oder so eine Colormap erzeugt.
**
**      OM_SAVETOIFF -
**          Macht die Resource-ID und falls vorhanden die Outline
**          "haltbar". Die Resource selbst (also die Bitmap) wird
**          NICHT gesichert, das passiert nur explizit mit
**          RSM_SAVE.
**
**  ATTRS
**      ILBMA_SaveILBM  (ISG) [BOOL] [def=FALSE]
**          Wenn TRUE, wird ein FORM ILBM in den IFF-Stream
**          geschrieben, statt einem FORM VBMP.
*/
/*-------------------------------------------------------------------
**  Die ClassID
*/
#define ILBM_CLASSID    "ilbm.class"

/*-----------------------------------------------------------------*/
#define ILBMM_BASE          (BMM_BASE+METHOD_DISTANCE)
/*-----------------------------------------------------------------*/
#define ILBMA_BASE          (BMA_BASE+ATTRIB_DISTANCE)
#define ILBMA_SaveILBM      (ILBMA_BASE)

/*-----------------------------------------------------------------*/
struct ilbm_data {
    ULONG flags;
};

#define ILBMF_SaveILBM  (1<<0)

/*-------------------------------------------------------------------
**  IFF-Stuff
*/
/*-- OBSOLETE -- OBSOLETE -- OBSOLETE -- OBSOLETE -- OBSOLETE --*/
#define ILBMIFF_FORMID  MAKE_ID('I','F','I','M')
#define ILBMIFF_NAME    MAKE_ID('N','A','M','E')
/*-- OBSOLETE -- OBSOLETE -- OBSOLETE -- OBSOLETE -- OBSOLETE --*/

#define ILBMIFF_FORMID2 MAKE_ID('C','I','B','O')    /* Compressed ILBM Bitmap Object */
#define ILBMIFF_NAME2   MAKE_ID('N','A','M','2')
#define ILBMIFF_OL2     MAKE_ID('O','T','L','2')
/*
**  Die optionale Outline wird in einem komprimierten
**  Array folgender Struktur abgelegt:
*/
struct ol2_piece {
    UBYTE x,y;
};

/*-------------------------------------------------------------------
**  Definitionen für ILBM-Format
*/
typedef struct {
    UWORD w, h;
    WORD x, y;
    UBYTE nPlanes;
    UBYTE masking;
    UBYTE compression;
    UBYTE flags;
    UWORD transparentColor;
    UBYTE xAspect, yAspect;
    WORD pageWidth, pageHeight;
} BitMapHeader; 

#define ILBM_ID     MAKE_ID('I','L','B','M')
#define BMHD_ID     MAKE_ID('B','M','H','D')
#define BODY_ID     MAKE_ID('B','O','D','Y')
#define CMAP_ID     MAKE_ID('C','M','A','P')

/*-------------------------------------------------------------------
**  Definitionen für VBMP-Format
*/
struct VBMP_Header {
    UWORD w,h;
    UBYTE type;     // Format des nachfolgenden Bodys, siehe unten
    UBYTE flags;    // noch nicht definiert
};

//
// Definitionen für VBMP_Header.type
//
#define VBMP_Chunky8    (0)     // 1 Byte -> 1 Pixel, externe CLUT
#define VBMP_Chunky16   (1)     // 1 Word -> 1 Pixel, Direct Color
#define VBMP_Chunky32   (2)     // 8 bit R,G,B,A, Direct Color

#define VBMP_FORM_ID    MAKE_ID('V','B','M','P')
#define VBMP_HEADER_ID  MAKE_ID('H','E','A','D')
#define VBMP_BODY_ID    MAKE_ID('B','O','D','Y')

/*-----------------------------------------------------------------*/
#endif

