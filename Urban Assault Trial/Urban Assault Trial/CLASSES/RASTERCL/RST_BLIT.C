/*
**  $Source: PRG:VFM/Classes/_RasterClass/rst_blit.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 14:57:04 $
**  $Locker:  $
**  $Author: floh $
**
**  Blit- und ähnliche Methoden für raster.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <stdlib.h>
#include <string.h>

#ifdef AMIGA
#undef memset
#endif

#define NOMATH_FUNCTIONS 1

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/rasterclass.h"

_extern_use_nucleus

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Clear, void *nil)
/*
**  CHANGED
**      30-May-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    ULONG *buf = rd->r->Data;
    ULONG size = rd->r->Width * rd->r->Height;
    ULONG val  = rd->bg_pen;
    memset(buf,val,size);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Copy, struct rast_copy_msg *msg)
/*
**  CHANGED
**      30-May-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    memcpy(msg->to->Data, rd->r->Data, rd->r->Width * rd->r->Height);
}

/*-----------------------------------------------------------------*/
void rst_BltInt2Coord(struct raster_data *rd,
                      struct rast_intblit *from,
                      struct rast_intblit *to)
/*
**  FUNCTION
**      Integer-Koordinaten-Konverter für Blit-Parameter-Struktur.
**
**  CHANGED
**      30-Aug-96   floh    created
*/
{
    LONG tar_offx = rd->ioff_x;
    LONG tar_offy = rd->ioff_y;
    LONG src_offx = from->src->Width >> 1;
    LONG src_offy = from->src->Height >> 1;
    to->src = from->src;
    to->from.xmin = from->from.xmin + src_offx;
    to->from.xmax = from->from.xmax + src_offx;
    to->from.ymin = from->from.ymin + src_offy;
    to->from.ymax = from->from.ymax + src_offy;
    to->to.xmin   = from->to.xmin + tar_offx;
    to->to.xmax   = from->to.xmax + tar_offx;
    to->to.ymin   = from->to.ymin + tar_offy;
    to->to.ymax   = from->to.ymax + tar_offy;
}

/*-----------------------------------------------------------------*/
void rst_BltFlt2Coord(struct raster_data *rd,
                      struct rast_blit *from,
                      struct rast_intblit *to)
/*
**  FUNCTION
**      Float-Koordinaten-Konverter für Blit-Parameter-Struktur.
**
**  CHANGED
**      30-Aug-96   floh    created
*/
{
    FLOAT tar_mulx = rd->foff_x;
    FLOAT tar_muly = rd->foff_y;
    FLOAT src_mulx = (FLOAT)(from->src->Width>>1);
    FLOAT src_muly = (FLOAT)(from->src->Height>>1);
    to->src = from->src;
    to->from.xmin = FLOAT_TO_INT((from->from.xmin+1.0) * src_mulx);
    to->from.xmax = FLOAT_TO_INT((from->from.xmax+1.0) * src_mulx);
    to->from.ymin = FLOAT_TO_INT((from->from.ymin+1.0) * src_muly);
    to->from.ymax = FLOAT_TO_INT((from->from.ymax+1.0) * src_muly);
    to->to.xmin   = FLOAT_TO_INT((from->to.xmin+1.0) * tar_mulx);
    to->to.xmax   = FLOAT_TO_INT((from->to.xmax+1.0) * tar_mulx);
    to->to.ymin   = FLOAT_TO_INT((from->to.ymin+1.0) * tar_muly);
    to->to.ymax   = FLOAT_TO_INT((from->to.ymax+1.0) * tar_muly);
}

/*-----------------------------------------------------------------*/
void rst_StretchBlit(struct raster_data *rd,
                     struct rast_intblit *blt)
/*
**  FUNCTION
**      Einfacher ungeclippter skalierender Blitter.
**      Ziemlich unoptimiert.
**
**  CHANGED
**      29-Aug-96   floh    created
**      12-Feb-97   floh    Pitch korrekt
**      08-Nov-97   floh    + unter Umständen konnte ein DivByZero passieren
*/
{
    ULONG src_x,src_y;
    ULONG tar_y;
    LONG src_dx,src_dy,tar_dx,tar_dy;
    UBYTE *src,*tar;

    tar_dx = blt->to.xmax - blt->to.xmin;
    tar_dy = blt->to.ymax - blt->to.ymin;
    if ((tar_dx==0)||(tar_dy==0)) return;

    src   = (UBYTE *) blt->src->Data;
    tar   = (UBYTE *) rd->r->Data;
    src_x = blt->from.xmin;
    src_y = blt->from.ymin;
    src_dx = ((blt->from.xmax - src_x)<<16) / tar_dx;
    src_dy = ((blt->from.ymax - src_y)<<16) / tar_dy;

    src_y <<= 16;
    for (tar_y = blt->to.ymin; tar_y < blt->to.ymax; tar_y++) {

        ULONG tar_x;
        UBYTE *tar_line = tar + (tar_y * rd->r->BytesPerRow);
        UBYTE *src_line = src + ((src_y>>16) * blt->src->BytesPerRow);

        src_y += src_dy;
        src_x  = blt->from.xmin<<16;
        for (tar_x = blt->to.xmin; tar_x < blt->to.xmax; tar_x++) {
            tar_line[tar_x] = src_line[src_x>>16];
            src_x += src_dx;
        };
    };
}

/*-----------------------------------------------------------------*/
void rst_MaskStretchBlit(struct raster_data *rd,
                         struct rast_intmaskblit *blt)
/*
**  FUNCTION
**      Stretch-Blitter mit 8-Bit-Source-Maske.
**
**  CHANGED
**      05-Nov-97   floh    created
**      08-Nov-97   floh    + DivByZero Check
*/
{
    ULONG src_x,src_y;
    ULONG tar_y;
    LONG src_dx,src_dy,tar_dx,tar_dy;
    UBYTE *src,*tar,*mask;
    UBYTE mask_key = (UBYTE) blt->mask_key;

    tar_dx = blt->to.xmax - blt->to.xmin;
    tar_dy = blt->to.ymax - blt->to.ymin;
    if ((tar_dx==0)||(tar_dy==0)) return;

    src   = (UBYTE *) blt->src->Data;
    mask  = (UBYTE *) blt->mask->Data;
    tar   = (UBYTE *) rd->r->Data;
    src_x = blt->from.xmin;
    src_y = blt->from.ymin;
    src_dx = ((blt->from.xmax - src_x)<<16) / tar_dx;
    src_dy = ((blt->from.ymax - src_y)<<16) / tar_dy;

    src_y <<= 16;
    for (tar_y = blt->to.ymin; tar_y<blt->to.ymax; tar_y++,src_y+=src_dy) {
        ULONG tar_x;
        ULONG src_off = (src_y>>16) * blt->src->BytesPerRow;
        UBYTE *tar_line  = tar + (tar_y * rd->r->BytesPerRow);
        UBYTE *src_line  = src + src_off;
        UBYTE *mask_line = mask + src_off;
        src_x = blt->from.xmin<<16;
        for (tar_x = blt->to.xmin; tar_x < blt->to.xmax; tar_x++,src_x+=src_dx) {
            UBYTE mask_pix = mask_line[src_x>>16];
            if (mask_pix == mask_key) tar_line[tar_x] = src_line[src_x>>16];
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL rst_BltClip(struct raster_data *rd,
                 struct rast_intblit *blt)
/*
**  FUNCTION
**      Clippt die Koordinaten in der <blt>-Struktur.
**      Nur das Target-Rechteck darf geclippt sein!
**
**      Die Routine kehrt FALSE zurück, wenn das
**      Resultat vollständig außerhalb liegt.
**
**  CHANGED
**      29-Aug-96   floh    created
*/
{
    LONG tar_x0 = blt->to.xmin;
    LONG tar_x1 = blt->to.xmax;
    LONG tar_y0 = blt->to.ymin;
    LONG tar_y1 = blt->to.ymax;

    LONG src_x0 = blt->from.xmin;
    LONG src_x1 = blt->from.xmax;
    LONG src_y0 = blt->from.ymin;
    LONG src_y1 = blt->from.ymax;

    /*** voll raus? ***/
    if (tar_x1 < rd->clip.xmin)      return(FALSE);
    else if (tar_x0 > rd->clip.xmax) return(FALSE);
    if (tar_y1 < rd->clip.ymin)      return(FALSE);
    else if (tar_y0 > rd->clip.ymax) return(FALSE);

    /*** X clippen ***/
    if (tar_x0 < rd->clip.xmin) {
        src_x0 = src_x0+(((src_x1-src_x0)*(rd->clip.xmin-tar_x0))/(tar_x1-tar_x0));
        tar_x0 = rd->clip.xmin;
    };
    if (tar_x1 > rd->clip.xmax) {
        src_x1 = src_x1 + (((src_x1-src_x0)*(rd->clip.xmax-tar_x1))/(tar_x1-tar_x0));
        tar_x1 = rd->clip.xmax;
    };

    /*** Y clippen ***/
    if (tar_y0 < rd->clip.ymin) {
        src_y0 = src_y0+(((src_y1-src_y0)*(rd->clip.ymin-tar_y0))/(tar_y1-tar_y0));
        tar_y0 = rd->clip.ymin;
    };
    if (tar_y1 > rd->clip.ymax) {
        src_y1 = src_y1 + (((src_y1-src_y0)*(rd->clip.ymax-tar_y1))/(tar_y1-tar_y0));
        tar_y1 = rd->clip.ymax;
    };

    /*** Ergebnis zurückschreiben ***/
    blt->to.xmin = tar_x0;
    blt->to.xmax = tar_x1;
    blt->to.ymin = tar_y0;
    blt->to.ymax = tar_y1;
    blt->from.xmin = src_x0;
    blt->from.xmax = src_x1;
    blt->from.ymin = src_y0;
    blt->from.ymax = src_y1;

    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntBlit, struct rast_intblit *msg)
/*
**  CHANGED
**      29-Aug-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_intblit blt;
    rst_BltInt2Coord(rd,msg,&blt);
    rst_StretchBlit(rd,&blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Blit, struct rast_blit *msg)
/*
**  CHANGED
**      29-Aug-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_intblit blt;
    rst_BltFlt2Coord(rd,msg,&blt);
    rst_StretchBlit(rd,&blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntMaskBlit, struct rast_intmaskblit *msg)
/*
**  CHANGED
**      05-Nov-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_intblit in_blt;
    struct rast_intblit out_blt;
    struct rast_intmaskblit mask_blt;
    in_blt.src  = msg->src;
    in_blt.from = msg->from;
    in_blt.to   = msg->to;
    rst_BltInt2Coord(rd,&in_blt,&out_blt);
    mask_blt.src      = msg->src;
    mask_blt.mask     = msg->mask;
    mask_blt.mask_key = msg->mask_key;
    mask_blt.from     = out_blt.from;
    mask_blt.to       = out_blt.to;
    rst_MaskStretchBlit(rd,&mask_blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_MaskBlit, struct rast_maskblit *msg)
/*
**  CHANGED
**      05-Nov-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_blit in_blt;
    struct rast_intblit out_blt;
    struct rast_intmaskblit mask_blt;
    in_blt.src  = msg->src;
    in_blt.from = msg->from;
    in_blt.to   = msg->to;
    rst_BltFlt2Coord(rd,&in_blt,&out_blt);
    mask_blt.src      = msg->src;
    mask_blt.mask     = msg->mask;
    mask_blt.mask_key = msg->mask_key;
    mask_blt.from     = out_blt.from;
    mask_blt.to       = out_blt.to;
    rst_MaskStretchBlit(rd,&mask_blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntClippedBlit, struct rast_intblit *msg)
/*
**  CHANGED
**      30-Aug-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_intblit blt;
    rst_BltInt2Coord(rd,msg,&blt);
    if (rst_BltClip(rd,&blt)) rst_StretchBlit(rd,&blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_ClippedBlit, struct rast_blit *msg)
/*
**  CHANGED
**      30-Aug-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    struct rast_intblit blt;
    rst_BltFlt2Coord(rd,msg,&blt);
    if (rst_BltClip(rd,&blt)) rst_StretchBlit(rd,&blt);
}

