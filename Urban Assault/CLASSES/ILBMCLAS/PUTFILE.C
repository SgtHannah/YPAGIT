/*
**  $Source: PRG:VFM/Classes/_ILBMClass/putfile.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 14:51:40 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen zum Speichern eines ILBM- oder VBMP-Files.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "bitmap/ilbmclass.h"

_extern_use_nucleus

/*-----------------------------------------------------------------*/
void ilbm_SaveCMAP(struct IFFHandle *iff, struct VFMBitmap *bmp)
/*
**  FUNCTION
**      Falls das Object eine Colormap besitzt, wird
**      ein CMAP Chunk in den IFF-Stream geschrieben.
**
**  CHANGED
**      10-Oct-96   floh    created
*/
{
    if (bmp->ColorMap) {
        _PushChunk(iff,0,CMAP_ID,(256*3));
        _WriteChunkBytes(iff,bmp->ColorMap,(256*3));
        _PopChunk(iff);
    };
}

/*-----------------------------------------------------------------*/
BOOL ilbm_SaveAsVBMP(Object *o, struct IFFHandle *iff, struct VFMBitmap *bmp)
/*
**  FUNCTION
**      Sichert VFMBitmap als VBMP File.
**
**  CHANGED
**      10-Oct-96   floh    created
**      06-Dec-96   floh    + Standard-CMAP-Chunk (identisch mit ILBM)
**      25-Feb-97   floh    + VFMBitmap.Type obsolete, das VBMP_Header
**                            Type-Feld wird mit 0 initialisiert
*/
{
    struct VBMP_Header header;
    LONG body_size = bmp->Width * bmp->Height;

    /*** FORM VBMP ***/
    if (_PushChunk(iff,VBMP_FORM_ID,ID_FORM,IFFSIZE_UNKNOWN)!=0) return(FALSE);

    /*** Header Chunk ***/
    _PushChunk(iff,0,VBMP_HEADER_ID,sizeof(struct VBMP_Header));
    header.w = bmp->Width;
    header.h = bmp->Height;
    header.type = 0;
    header.flags = 0;
    n2vw(&(header.w));
    n2vw(&(header.h));
    _WriteChunkBytes(iff,&header,sizeof(header));
    _PopChunk(iff);

    /*** CMAP Chunk ***/
    ilbm_SaveCMAP(iff,bmp);

    /*** Body Chunk ***/
    _PushChunk(iff,0,VBMP_BODY_ID,body_size);
    _WriteChunkBytes(iff,bmp->Data,body_size);
    _PopChunk(iff);

    /*** FORM VBMP poppen ***/
    if (_PopChunk(iff) != 0) return(FALSE);

    /*** Success ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void ilbm_SaveBMHD(struct IFFHandle *iff, struct VFMBitmap *bmp)
/*
**  FUNCTION
**      Saved BMHD Chunk in IFF-Stream.
**
**  CHANGED
**      10-Oct-96   floh    created
*/
{
    BitMapHeader bmhd;

    bmhd.w = bmp->Width;
    bmhd.h = bmp->Height;
    bmhd.x = bmhd.y = 0;
    bmhd.nPlanes = 8;
    bmhd.masking = 0;  // mskNone
    bmhd.compression = 0;  // cmpNone
    bmhd.flags = (1<<7);   // == BMHDF_CMPAPOK (je 8 RGB Bits in Colormap)
    bmhd.transparentColor = 0;
    bmhd.pageWidth  = bmp->Width;
    bmhd.pageHeight = bmp->Height;
    bmhd.xAspect = 22; // Magic Numbers (siehe newiff39)
    bmhd.yAspect = 22;

    n2vw(&(bmhd.w)); n2vw(&(bmhd.h));
    n2vw(&(bmhd.x)); n2vw(&(bmhd.y));
    n2vw(&(bmhd.transparentColor));
    n2vw(&(bmhd.pageWidth));
    n2vw(&(bmhd.pageHeight));

    _PushChunk(iff,0,BMHD_ID,sizeof(bmhd));
    _WriteChunkBytes(iff,&bmhd,sizeof(bmhd));
    _PopChunk(iff);
}

/*-----------------------------------------------------------------*/
BOOL ilbm_SaveBODY(struct IFFHandle *iff, struct VFMBitmap *bmp)

/*
**  FUNCTION
**      Sichert ***nicht-komprimierten*** BODY Chunk in IFF-Stream.
**      Die Chunky-2-Planar-Konvertierung ist in keinster
**      Weise optimiert, die Sache ist also trotzdem
**      brechlangsam!
**
**  CHANGED
**      10-Oct-96   floh    created
**      11-Oct-96   floh    debugging
*/
{
    LONG FileRowBytes = ((bmp->Width+15)>>4)<<1; // muß gerade sein
    UBYTE *row = (UBYTE *) _AllocVec(FileRowBytes,MEMF_PUBLIC);

    if (row) {

        LONG planeCnt = 8;
        LONG iRow,iPlane;
        UBYTE *src = bmp->Data;

        /*** BODY Chunk ***/
        _PushChunk(iff,0,BODY_ID,(bmp->Height*planeCnt*FileRowBytes));

        /*** für jede Zeile... ***/
        for (iRow=bmp->Height; iRow>0; iRow--,src+=bmp->Width) {
            for (iPlane=0; iPlane<planeCnt; iPlane++) {

                LONG i;
                UBYTE msk = (1<<iPlane);

                /*** fülle Plane-Zeilen-Buffer (c2p-Konvertierung) ***/
                memset(row,0,FileRowBytes);
                for (i=0; i<bmp->Width; i++) {
                    LONG off_byte = i>>3;
                    LONG off_bit  = (i&7)^7;
                    if (src[i] & msk) row[off_byte] |= (1<<off_bit);
                };

                /*** schreibe Plane-Zeilen-Buffer, unkomprimiert ***/
                _WriteChunkBytes(iff,row,FileRowBytes);
            };
        };

        /*** poppe BODY, Ressourcen freigeben und zurück ***/
        _PopChunk(iff);
        _FreeVec(row);
        return(TRUE);
    };

    /*** No mem ***/
    return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL ilbm_SaveAsILBM(Object *o, struct IFFHandle *iff, struct VFMBitmap *bmp)
/*
**  FUNCTION
**      Sichert VFMBitmap als FORM ILBM File.
**
**  CHANGED
**      10-Oct-96   floh    created
*/
{
    /*** FORM ILBM ***/
    if (_PushChunk(iff,ILBM_ID,ID_FORM,IFFSIZE_UNKNOWN)!=0) return(FALSE);

    /*** BMHD Chunk ***/
    ilbm_SaveBMHD(iff,bmp);

    /*** CMAP Chunk ***/
    ilbm_SaveCMAP(iff,bmp);

    /*** encodeter BODY Chunk ***/
    if (!ilbm_SaveBODY(iff,bmp)) return(FALSE);

    /*** FORM ILBM poppen ***/
    if (_PopChunk(iff) != 0) return(FALSE);

    /*** Success ***/
    return(TRUE);
}

