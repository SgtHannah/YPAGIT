/*
**  $Source: PRG:VFM/Classes/_RasterClass/rst_prim.c,v $
**  $Revision: 38.5 $
**  $Date: 1998/01/06 14:57:33 $
**  $Locker:  $
**  $Author: floh $
**
**  Enthält Routinen und Methoden zum Rendern von
**  2D-Primitives.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include <stdlib.h>

#define NOMATH_FUNCTIONS 1

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/rasterclass.h"

_extern_use_nucleus

#define LINECLIP_LEFT   (1<<0)
#define LINECLIP_RIGHT  (1<<1)
#define LINECLIP_TOP    (1<<2)
#define LINECLIP_BOTTOM (1<<3)

/*-----------------------------------------------------------------*/
void rst_Line(struct raster_data *rd, LONG x0, LONG y0, LONG x1, LONG y1)
/*
**  FUNCTION
**      Zeichnet eine Linie von [x0,y0]->[x1,y1] in Farbe <color>.
**      Keinerlei Extras, die Linie muß bereits korrekt
**      geclippt sein.
**
**      Frei nach "CG - Principles & Pratice".
**
**      Hochgradig unoptimiert.
**
**  INPUTS
**      rd      - Pointer auf LID des Raster-Objects
**      x0,y0   - Start-Koordinaten, Nullpunkt in Screen-Mitte
**      x1,y1   - End-Koordinaten, Nullpunkt in Screen-Mitte
**      color   - Farbwert
**
**  CHANGED
**      06-Feb-96   floh    created
**      04-Jun-96   floh    + übernommen nach raster.class
**      14-Jun-96   floh    + wird jetzt generell per RemapTable
**                            gefiltert
**      12-Feb-97   floh    + Pitch korrekt
*/
{
    UBYTE *pnt;
    LONG i;
    LONG evenAdd, oddAdd;

    LONG dx = abs(x1-x0);
    LONG dy = abs(y1-y0);

    LONG d, incrE, incrNE;
    UBYTE *rm = rd->tracy_body;

    if (dx > dy) {
        /*** horizontal Slope ***/
        if (x1 > x0) evenAdd = 1;
        else         evenAdd = -1;
        if (y1 > y0) oddAdd  = rd->r->BytesPerRow;
        else         oddAdd  = -rd->r->BytesPerRow;
    } else {

        LONG swap;

        /*** vertikel Slope ***/
        if (y1 > y0) evenAdd = rd->r->BytesPerRow;
        else         evenAdd = -rd->r->BytesPerRow;
        if (x1 > x0) oddAdd = 1;
        else         oddAdd = -1;

        /*** dx und dy vertauschen ***/
        swap = dx;
        dx   = dy;
        dy   = swap;
    };

    /*** Bresenham-Parameter ***/
    d      = (dy<<1) - dx;
    incrE  = (dy<<1);
    incrNE = (dy-dx)<<1;

    /*** Start-Adresse ermitteln ***/
    pnt = ((UBYTE *)rd->r->Data) + y0*rd->r->BytesPerRow + x0;

    /*** Anfangs-Pixel zeichnen ***/
    *pnt = rm[(*pnt<<8)|rd->fg_pen];

    /*** Schleife... ***/
    for (i=0; i<dx; i++) {
        if (d <= 0)  d += incrE;
        else {
            d   += incrNE;
            pnt += oddAdd;
        };
        pnt += evenAdd;
        *pnt = rm[(*pnt<<8)|rd->fg_pen];
    };

    /*** fertig ***/
}

/*-----------------------------------------------------------------*/
LONG rst_CompOutCode(LONG x, LONG y, struct rast_intrect *clip)
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
**      20-May-96   floh    created
**      04-Jun-96   floh    übernommen nach raster.class
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
LONG rst_Clip(struct rast_intrect *clip, struct rast_intrect *line)
/*
**  FUNCTION
**      Clippt die Linie in <line> gegen <clip>, schreibt Ergebnis
**      nach <line> zurueck. Kehrt TRUE zurueck, falls Linie
**      sichtbar, sonst FALSE.
**
**  CHANGED
**      11-Dec-97   floh    + aus win3d.class uebernommen
*/
{
    ULONG outcode0,outcode1;
    LONG accept = 1;        // voll drin als Default 
    BOOL done   = FALSE;
    LONG x0 = line->xmin;
    LONG x1 = line->xmax;
    LONG y0 = line->ymin;
    LONG y1 = line->ymax;
    
    /*** LageCodes für beide EndPunkte ***/
    outcode0 = rst_CompOutCode(x0,y0,clip);
    outcode1 = rst_CompOutCode(x1,y1,clip);
    do {

        if ((outcode0 | outcode1) == 0) {
            /*** Linie (jetzt) innerhalb, konnte geclippt worden sein, muss aber nicht ***/
            done   = TRUE;
        } else if ((outcode0 & outcode1) != 0) {
            /*** Linie vollstaendig ausserhalb ***/
            accept = -1;
            done   = TRUE;
        } else {

            LONG x,y;
            LONG outcodeOut;

            /*** diese Linie wurde geclippt... ***/
            accept = 0;

            /*** mindestens einer der Punkte ist draussen, welcher? ***/
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
                x0 = x;
                y0 = y;
                outcode0 = rst_CompOutCode(x0,y0,clip);
            } else {
                x1 = x;
                y1 = y;
                outcode1 = rst_CompOutCode(x1,y1,clip);
            };
        };
    } while (!done);

    /*** Ergebnis zurueckschreiben, nur wenn wirklich geclippt ***/
    if (accept == 0) {
        line->xmin = x0;  line->xmax = x1;
        line->ymin = y0;  line->ymax = y1;
    };
    return(accept);
}

/*-----------------------------------------------------------------*/
void rst_ClippedLine(struct raster_data *rd, LONG x0, LONG y0, LONG x1, LONG y1)
/*
**  CHANGED
**      01-Apr-97   floh    created
**      31-Jul-97   floh    + Endpunkt-Farben eingetragen
**      11-Dec-97   floh    + umgebaut auf zusaetzliches Invers-Clippen
**      14-May-98   floh    + konnte unter manchen Umstaenden abstuerzen      
*/
{
    struct rast_intrect outer_r;
    outer_r.xmin=x0; outer_r.xmax=x1;
    outer_r.ymin=y0; outer_r.ymax=y1;
    /*** Clippen gegen Outside-Rectangle ***/
    if (rst_Clip(&(rd->clip),&outer_r) != -1) {
        /*** Clippen gegen Inside-Rectangle ***/
        LONG clip_code;
        struct rast_intrect inner_r;
        inner_r = outer_r;
        if (rd->inv_clip.xmin != rd->inv_clip.xmax) clip_code = rst_Clip(&(rd->inv_clip),&inner_r);
        else                                        clip_code = -1;
        if (clip_code == -1) {
            /*** vollstaendig sichtbar ***/
            rst_Line(rd,inner_r.xmin,inner_r.ymin,inner_r.xmax,inner_r.ymax);
        } else if (clip_code == 0) {
            /*** geclippt, einer von 3 Faellen ***/
            if ((inner_r.xmax == outer_r.xmax) && (inner_r.ymax == outer_r.ymax)) {
                rst_Line(rd,outer_r.xmin,outer_r.ymin,inner_r.xmin,inner_r.ymin);
            } else if ((inner_r.xmin == outer_r.xmin) && (inner_r.ymin == outer_r.ymin)) {
                rst_Line(rd,inner_r.xmax,inner_r.ymax,outer_r.xmax,outer_r.ymax);
            } else {
                rst_Line(rd,outer_r.xmin,outer_r.ymin,inner_r.xmin,inner_r.ymin);
                rst_Line(rd,inner_r.xmax,inner_r.ymax,outer_r.xmax,outer_r.ymax);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Line, struct rast_line *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    LONG x0 = FLOAT_TO_INT((msg->x0+1.0) * (rd->foff_x-1.0));
    LONG y0 = FLOAT_TO_INT((msg->y0+1.0) * (rd->foff_y-1.0));
    LONG x1 = FLOAT_TO_INT((msg->x1+1.0) * (rd->foff_x-1.0));
    LONG y1 = FLOAT_TO_INT((msg->y1+1.0) * (rd->foff_y-1.0));
    rst_Line(rd,x0,y0,x1,y1);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_ClippedLine, struct rast_line *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    LONG x0 = FLOAT_TO_INT((msg->x0+1.0) * (rd->foff_x-1.0));
    LONG y0 = FLOAT_TO_INT((msg->y0+1.0) * (rd->foff_y-1.0));
    LONG x1 = FLOAT_TO_INT((msg->x1+1.0) * (rd->foff_x-1.0));
    LONG y1 = FLOAT_TO_INT((msg->y1+1.0) * (rd->foff_y-1.0));
    rst_ClippedLine(rd,x0,y0,x1,y1);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_ClipRegion, struct rast_rect *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rd->clip.xmin = FLOAT_TO_INT((msg->xmin+1.0) * (rd->foff_x-1.0));
    rd->clip.ymin = FLOAT_TO_INT((msg->ymin+1.0) * (rd->foff_y-1.0));
    rd->clip.xmax = FLOAT_TO_INT((msg->xmax+1.0) * (rd->foff_x-1.0));
    rd->clip.ymax = FLOAT_TO_INT((msg->ymax+1.0) * (rd->foff_y-1.0));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_InvClipRegion, struct rast_rect *msg)
/*
**  CHANGED
**      11-Dec-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rd->inv_clip.xmin = FLOAT_TO_INT((msg->xmin+1.0) * (rd->foff_x-1.0));
    rd->inv_clip.ymin = FLOAT_TO_INT((msg->ymin+1.0) * (rd->foff_y-1.0));
    rd->inv_clip.xmax = FLOAT_TO_INT((msg->xmax+1.0) * (rd->foff_x-1.0));
    rd->inv_clip.ymax = FLOAT_TO_INT((msg->ymax+1.0) * (rd->foff_y-1.0));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntLine, struct rast_intline *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    LONG x0 = msg->x0 + rd->ioff_x;
    LONG y0 = msg->y0 + rd->ioff_y;
    LONG x1 = msg->x1 + rd->ioff_x;
    LONG y1 = msg->y1 + rd->ioff_y;
    rst_Line(rd,x0,y0,x1,y1);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntClippedLine, struct rast_intline *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    LONG x0 = msg->x0 + rd->ioff_x;
    LONG y0 = msg->y0 + rd->ioff_y;
    LONG x1 = msg->x1 + rd->ioff_x;
    LONG y1 = msg->y1 + rd->ioff_y;
    rst_ClippedLine(rd,x0,y0,x1,y1);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntClipRegion, struct rast_intrect *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rd->clip.xmin = msg->xmin + rd->ioff_x;
    rd->clip.ymin = msg->ymin + rd->ioff_y;
    rd->clip.xmax = msg->xmax + rd->ioff_x;
    rd->clip.ymax = msg->ymax + rd->ioff_y;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_IntInvClipRegion, struct rast_intrect *msg)
/*
**  CHANGED
**      11-Dec-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rd->inv_clip.xmin = msg->xmin + rd->ioff_x;
    rd->inv_clip.ymin = msg->ymin + rd->ioff_y;
    rd->inv_clip.xmax = msg->xmax + rd->ioff_x;
    rd->inv_clip.ymax = msg->ymax + rd->ioff_y;
}
