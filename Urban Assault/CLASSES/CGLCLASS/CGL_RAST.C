/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_rast.c,v $
**  $Revision: 38.2 $
**  $Date: 1997/02/26 17:22:20 $
**  $Locker: floh $
**  $Author: floh $
**
**  Diverse abgeleitete raster.class Methoden der
**  cgl.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NOMATH_FUNCTIONS 1

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/cglclass.h"

#define LINECLIP_LEFT   (1<<0)
#define LINECLIP_RIGHT  (1<<1)
#define LINECLIP_TOP    (1<<2)
#define LINECLIP_BOTTOM (1<<3)

/*** aus cgl_poly.c ***/
void cgl_FlushPolyBuf(struct cgl_data *);
void cgl_FlushDelayed(struct cgl_data *);
void cgl_FlushTracy(struct cgl_data *);

/*-----------------------------------------------------------------*/
LONG cgl_CompOutCode(FLOAT x, FLOAT y, struct rast_rect *clip)
/*
**  FUNCTION
**      Ermittelt Lagecode für [x,y] im Verhältnis zum
**      ClipRect [clip].
**      Returniert eine Kombination aus
**
**          LINECLIP_LEFT
**          LINECLIP_RIGHT
**          LINECLIP_TOP
**          LINECLIP_BOTTOM
**
**  CHANGED
**      04-Dec-96   floh    übernommen aus raster.class
*/
{
    LONG code = 0;
    if (y < clip->ymin)      code  = LINECLIP_TOP;
    else if (y > clip->ymax) code  = LINECLIP_BOTTOM;
    if (x < clip->xmin)      code |= LINECLIP_LEFT;
    else if (x > clip->xmax) code |= LINECLIP_RIGHT;
    return(code);
}

/*-----------------------------------------------------------------*/
BOOL cgl_ClipLine(struct rast_line *l, struct rast_rect *clip)
/*
**  FUNCTION
**      Clippt die Linie in <rast_line> gegen das ClipRect
**      <rast_rect>. Das Ergebnis wird zurückgeschrieben.
**
**  INPUT
**      l     - zu clippende Linie
**      clip  - das ClipRect
**
**  RETURN
**      TRUE    - Linie sichtbar
**      FALSE   - Linie nicht sichtbar
**
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    ULONG outcode0,outcode1;
    BOOL accept = FALSE;
    BOOL done   = FALSE;

    /*** LageCodes für beide EndPunkte ***/
    outcode0 = cgl_CompOutCode(l->x0,l->y0,clip);
    outcode1 = cgl_CompOutCode(l->x1,l->y1,clip);

    do {

        if ((outcode0 | outcode1) == 0) {

            /*** Linie vollständig innerhalb ***/
            accept = TRUE;
            done   = TRUE;

        } else if ((outcode0 & outcode1) != 0) {

            /*** Linie vollständig außerhalb ***/
            done = TRUE;

        } else {

            FLOAT x,y;
            LONG outcodeOut;

            FLOAT x0 = l->x0;
            FLOAT x1 = l->x1;
            FLOAT y0 = l->y0;
            FLOAT y1 = l->y1;

            /*** mindestens einer der Punkte ist draußen, welcher? ***/
            if (outcode0 != 0) outcodeOut = outcode0;
            else               outcodeOut = outcode1;

            /*** jetzt den Intersection-Punkt [x,y] finden ***/
            if (LINECLIP_TOP & outcodeOut){
                x = x0 + (((x1-x0)*(clip->ymin-y0))/(y1-y0));
                y = clip->ymin;
            }else if (LINECLIP_BOTTOM & outcodeOut){
                x = x0 + (((x1-x0)*(clip->ymax-y0))/(y1-y0));
                y = clip->ymax;
            }else if (LINECLIP_LEFT & outcodeOut){
                x = clip->xmin;
                y = y0 + (((y1-y0)*(clip->xmin-x0))/(x1-x0));
            }else if (LINECLIP_RIGHT & outcodeOut){
                x = clip->xmax;
                y = y0 + (((y1-y0)*(clip->xmax-x0))/(x1-x0));
            };

            /*** geclippten Punkt Start- oder Endpunkt zuordnen ***/
            if (outcodeOut == outcode0) {
                l->x0 = x;
                l->y0 = y;
                outcode0 = cgl_CompOutCode(l->x0,l->y0,clip);
            } else {
                l->x1 = x;
                l->y1 = y;
                outcode1 = cgl_CompOutCode(l->x1,l->y1,clip);
            };
        };
    } while (!done);

    /*** fertig ***/
    return(accept);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_Clear, void *nil)
/*
**  CHANGED
**      19-Aug-96   floh    created
*/
{
    cglClearScreen();
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_End3D, void *nil)
/*
**  CHANGED
**      19-Aug-96   floh    created
**      02-Dec-96   floh    delayed Polys werden geflusht
**      26-Mar-97   floh    -> vorher RASTM_Flush, jetzt RASTM_End3D
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    cgl_FlushDelayed(cd);
    cgl_FlushTracy(cd);
    cgl_FlushPolyBuf(cd);
    cgl_TxtCacheEndFrame(cd);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_Line, struct rast_line *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd     = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);
    struct raster_data *rd  = INST_DATA((cl->superclass->superclass),o);
    ULONG fg_pen;
    CGL_PTR streamPtr[1];
    UBYTE r,g,b;

    /*** Eckpunkte konvertieren ***/
    cd->line_xy[0].x = (msg->x0 + 1.0) * cd->x_scale;
    cd->line_xy[0].y = (msg->y0 + 1.0) * cd->y_scale;
    cd->line_xy[1].x = (msg->x1 + 1.0) * cd->x_scale;
    cd->line_xy[1].y = (msg->y1 + 1.0) * cd->y_scale;

    /*** Farbe ermitteln ***/
    r = dd->slot[0].rgb[rd->fg_pen].r;
    g = dd->slot[0].rgb[rd->fg_pen].g;
    b = dd->slot[0].rgb[rd->fg_pen].b;

    cd->line_color[0].bRed   = r;
    cd->line_color[0].bGreen = g;
    cd->line_color[0].bBlue  = b;
    cd->line_color[0].bAlpha = 0;

    /*** und los... ***/
    streamPtr[0] = &(cd->line);
    cglSendStream(streamPtr,1);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_ClippedLine, struct rast_line *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    struct rast_line l;

    /*** kopieren, clippen, rendern ***/
    l = *msg;
    if (cgl_ClipLine(&l,&(cd->clip))) _methoda(o,RASTM_Line,&l);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_IntLine, struct rast_intline *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    struct rast_line l;
    l.x0 = ((FLOAT)(msg->x0)) / cd->x_scale;
    l.y0 = ((FLOAT)(msg->y0)) / cd->y_scale;
    l.x1 = ((FLOAT)(msg->x1)) / cd->x_scale;
    l.y1 = ((FLOAT)(msg->y1)) / cd->y_scale;
    _methoda(o,RASTM_Line,&l);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_IntClippedLine, struct rast_intline *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    struct rast_line l;
    l.x0 = ((FLOAT)(msg->x0)) / cd->x_scale;
    l.y0 = ((FLOAT)(msg->y0)) / cd->y_scale;
    l.x1 = ((FLOAT)(msg->x1)) / cd->x_scale;
    l.y1 = ((FLOAT)(msg->y1)) / cd->y_scale;
    _methoda(o,RASTM_ClippedLine,&l);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_ClipRegion, struct rast_rect *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    cd->clip = *msg;
    _supermethoda(cl,o,RASTM_ClipRegion,msg);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_IntClipRegion, struct rast_intrect *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    cd->clip.xmin = ((FLOAT)msg->xmin) / cd->x_scale;
    cd->clip.ymin = ((FLOAT)msg->ymin) / cd->y_scale;
    cd->clip.xmax = ((FLOAT)msg->xmax) / cd->x_scale;
    cd->clip.ymax = ((FLOAT)msg->ymax) / cd->y_scale;
    _supermethoda(cl,o,RASTM_IntClipRegion,msg);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_Copy, struct rast_copy_msg *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
void cgl_BltInt2Coord(struct raster_data *rd,
                      struct rast_intblit *from,
                      struct rast_intblit *to)
/*
**  FUNCTION
**      Integer-Koordinaten-Konverter für Blit-Parameter-Struktur.
**
**  CHANGED
**      04-Dec-96   floh    übernommen aus raster.class
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
void cgl_BltFlt2Coord(struct raster_data *rd,
                      struct rast_blit *from,
                      struct rast_intblit *to)
/*
**  FUNCTION
**      Float-Koordinaten-Konverter für Blit-Parameter-Struktur.
**
**  CHANGED
**      04-Dec-96   floh    übernommen aus raster.class
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
void cgl_StretchBlit(struct cgl_data *cd,
                     struct raster_data *rd,
                     struct rast_intblit *blt)
/*
**  FUNCTION
**      Byte-2-Word Stretchblitter für CGL.
**
**  INPUT
**      cd        - cgl.class LID
**      rd        - raster.class LID
**      blt       - Blit-Struktur
**      b2w_table - 8bit-2-RGB5551 CLUT
**
**  CHANGED
**      04-Dec-96   floh    mit Modifikationen übernommen
**                          aus raster.class
**      26-Feb-97   floh    arbeitet jetzt ohne Remap-Tabelle,
**                          sondern erwartet Source-Bitmap
**                          als BMA_Texture
*/
{
    ULONG src_x,src_y;
    ULONG tar_y;
    ULONG src_dx,src_dy;
    UWORD *src;
    UWORD far *tar;

    src   = (UWORD *) blt->src->Data;
    src_x = blt->from.xmin;
    src_y = blt->from.ymin;
    src_dx = ((blt->from.xmax - src_x)<<16) / (blt->to.xmax - blt->to.xmin);
    src_dy = ((blt->from.ymax - src_y)<<16) / (blt->to.ymax - blt->to.ymin);

    if (cglLockBuffer(CGL_BACK_BUFFER,&tar) == CGL_SUCCESS) {
        for (tar_y = blt->to.ymin; tar_y < blt->to.ymax; tar_y++) {
            ULONG tar_x;
            UWORD far *tar_line = tar + (tar_y * rd->r->Width);
            UWORD *src_line     = src + ((src_y>>16) * blt->src->Width);

            src_y += src_dy;
            src_x  = blt->from.xmin<<16;
            for (tar_x = blt->to.xmin; tar_x < blt->to.xmax; tar_x++) {
                tar_line[tar_x] = src_line[src_x>>16];
                src_x += src_dx;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL cgl_BltClip(struct raster_data *rd,
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
**      04-Dec-96   floh    übernommen nach cgl.class
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
_dispatcher(void, cgl_RASTM_IntBlit, struct rast_intblit *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data    *cd = INST_DATA(cl,o);
    struct raster_data *rd = INST_DATA(cl->superclass->superclass,o);
    struct rast_intblit blt;

    cgl_BltInt2Coord(rd,msg,&blt);
    cgl_StretchBlit(cd,rd,&blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_Blit, struct rast_blit *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data    *cd = INST_DATA(cl,o);
    struct raster_data *rd = INST_DATA(cl->superclass->superclass,o);
    struct rast_intblit blt;

    cgl_BltFlt2Coord(rd,msg,&blt);
    cgl_StretchBlit(cd,rd,&blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_IntClippedBlit, struct rast_intblit *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data    *cd = INST_DATA(cl,o);
    struct raster_data *rd = INST_DATA(cl->superclass->superclass,o);
    struct rast_intblit blt;

    cgl_BltInt2Coord(rd,msg,&blt);
    if (cgl_BltClip(rd,&blt)) cgl_StretchBlit(cd,rd,&blt);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_ClippedBlit, struct rast_blit *msg)
/*
**  CHANGED
**      04-Dec-96   floh    created
*/
{
    struct cgl_data *cd    = INST_DATA(cl,o);
    struct raster_data *rd = INST_DATA(cl->superclass->superclass,o);
    struct rast_intblit blt;

    cgl_BltFlt2Coord(rd,msg,&blt);
    if (cgl_BltClip(rd,&blt)) cgl_StretchBlit(cd,rd,&blt);
}

