/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_prim.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 15:03:04 $
**  $Locker: floh $
**  $Author: floh $
**
**  w3d_prim.c -- Primitive-Render-Routinen.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <stdlib.h>

#define WIN3D_WINBOX
#include "bitmap/win3dclass.h"

/*** importiert aus windd.class ***/
extern LPDIRECTDRAW lpDD;
extern LPDIRECT3D2  lpD3D2;
extern struct wdd_Data wdd_Data;
extern void wdd_FailMsg(char *title, char *msg, unsigned long code);

unsigned long w3d_ColorConvert(unsigned long r, unsigned long g, unsigned long b, unsigned long a,
                               long r_shift, long g_shift, long b_shift, long a_shift,
                               long r_mask, long g_mask, long b_mask, long a_mask);


/*-----------------------------------------------------------------*/
void w3d_DrawLine(struct windd_data *wdd, struct win3d_data *w3d,
                  long x0, long y0, long x1, long y1,
                  long r0, long g0, long b0,
                  long r1, long g1, long b1)
/*
**  FUNCTION
**      Zeichnet eine Linie in 16 Bit, in Farbe <color>.
**
**  CHANGED
**      01-Apr-97   floh    created
**      31-Jul-97   floh    + jetzt als Gradient-Lines (Start- und
**                            Endpunkt dürfen verschiedene Farben
**                            haben)
*/
{
    if (wdd->back_ptr) {

        struct w3d_PixelFormat *px = &(w3d->p->disp_pfmt);
        unsigned long color;
        unsigned long pix_size = px->byte_size;
        unsigned long pitch    = wdd->back_pitch / pix_size;
        long i, evenAdd, oddAdd;

        long dx = abs(x1-x0);
        long dy = abs(y1-y0);
        long d, incrE, incrNE;

        unsigned long mask;

        mask = w3d_ColorConvert(0x80, 0x80, 0x80, 0x0,
                                px->r_shift, px->g_shift, px->b_shift, px->a_shift,
                                px->r_mask, px->g_mask, px->b_mask, px->a_mask);
        mask = ~((mask<<1)|1);
        color = w3d_ColorConvert(r0, g0, b0, 0x0,
                                 px->r_shift, px->g_shift, px->b_shift, px->a_shift,
                                 px->r_mask, px->g_mask, px->b_mask, px->a_mask);
        if (dx > dy) {
            /*** horizontal Slope ***/
            if (x1 > x0) evenAdd = 1;
            else         evenAdd = -1;
            if (y1 > y0) oddAdd  = pitch;
            else         oddAdd  = -pitch;
        } else {
            LONG swap;
            /*** vertikal Slope ***/
            if (y1 > y0) evenAdd = pitch;
            else         evenAdd = -pitch;
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

        /*** Rendering ***/
        color &= mask;
        if (pix_size == 2) {

            unsigned short *pnt;
            unsigned long src_col;

            pnt = ((unsigned short *)wdd->back_ptr) + y0*pitch + x0;
            src_col = (unsigned long) (*pnt);
            *pnt    = ((src_col & mask) + color)>>1;

            for (i=0; i<dx; i++) {
                if (d<=0) d+=incrE;
                else {
                    d   += incrNE;
                    pnt += oddAdd;
                };
                pnt    += evenAdd;
                src_col = (unsigned long) (*pnt);
                *pnt    = ((src_col & mask) + color)>>1;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void w3d_StretchBlit(struct windd_data *wdd, struct win3d_data *w3d,
                     void *from_data, long from_w,
                     long from_xmin, long from_ymin,
                     long from_xmax, long from_ymax,
                     long to_xmin,   long to_ymin,
                     long to_xmax,   long to_ymax)
/*
**  CHANGED
**      02-Apr-97   floh    created
**      14-Apr-97   floh    Bugfix, unsauberer unterer Rand
**      05-Nov-97   floh    + 32-Bit-Support entfernt
**      08-Nov-97   floh    + DivByZero Check
*/
{
    if (wdd->back_ptr) {
        unsigned long src_x,src_y;
        unsigned long tar_y;
        long src_dx,src_dy,tar_dx,tar_dy;
        void *src = from_data;
        void *tar = wdd->back_ptr;

        tar_dx = to_xmax - to_xmin;
        tar_dy = to_ymax - to_ymin;
        if ((tar_dx==0)||(tar_dy==0)) return;
        src_x = from_xmin;
        src_y = from_ymin;
        src_dx = ((from_xmax - src_x)<<16) / tar_dx;
        src_dy = ((from_ymax - src_y)<<16) / tar_dy;
        src_y <<= 16;
        for (tar_y = to_ymin; tar_y < to_ymax; tar_y++,src_y+=src_dy) {
            unsigned long pix_size = w3d->p->disp_pfmt.byte_size;
            unsigned long pitch    = wdd->back_pitch / pix_size;
            unsigned long tar_x;
            unsigned short *tar_line = ((unsigned short *)tar)+(tar_y*pitch);
            unsigned short *src_line = ((unsigned short *)src)+((src_y>>16)*from_w);
            src_x  = from_xmin<<16;
            for (tar_x = to_xmin; tar_x < to_xmax; tar_x++,src_x+=src_dx) {
                tar_line[tar_x] = src_line[src_x>>16];
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void w3d_MaskStretchBlit(struct windd_data *wdd, struct win3d_data *w3d,
                         void *from_data, long from_w,
                         void *mask_data, unsigned char mask_key,
                         long from_xmin, long from_ymin,
                         long from_xmax, long from_ymax,
                         long to_xmin,   long to_ymin,
                         long to_xmax,   long to_ymax)
/*
**  CHANGED
**      05-Nov-97   floh    abgeleitet von w3d_StretchBlit()
**      08-Nov-97   floh    DivByZero Check
*/
{
    if (wdd->back_ptr) {
        unsigned long src_x,src_y;
        unsigned long tar_y;
        long src_dx,src_dy,tar_dx,tar_dy;
        void *src = from_data;
        void *tar = wdd->back_ptr;
        unsigned char *mask = mask_data;
        unsigned long pix_size = w3d->p->disp_pfmt.byte_size;
        unsigned long pitch    = wdd->back_pitch / pix_size;

        tar_dx = to_xmax - to_xmin;
        tar_dy = to_ymax - to_ymin;
        if ((tar_dx==0)||(tar_dy==0)) return;

        src_x = from_xmin;
        src_y = from_ymin;
        src_dx = ((from_xmax - src_x)<<16) / tar_dx;
        src_dy = ((from_ymax - src_y)<<16) / tar_dy;
        src_y <<= 16;
        for (tar_y = to_ymin; tar_y < to_ymax; tar_y++,src_y+=src_dy) {
            unsigned long tar_x;
            unsigned short *tar_line  = ((unsigned short *)tar)+(tar_y*pitch);
            unsigned short *src_line  = ((unsigned short *)src)+((src_y>>16)*from_w);
            unsigned char  *mask_line = mask + ((src_y>>16)*from_w);
            src_x = from_xmin<<16;
            for (tar_x = to_xmin; tar_x < to_xmax; tar_x++,src_x+=src_dx) {
                unsigned char mask_pix = mask_line[src_x>>16];
                if (mask_pix==mask_key) tar_line[tar_x]=src_line[src_x>>16];
            };
        };
    };
}

