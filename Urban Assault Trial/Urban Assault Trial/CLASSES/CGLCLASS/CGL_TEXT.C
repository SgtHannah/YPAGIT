/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_text.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/12/02 21:14:45 $
**  $Locker: floh $
**  $Author: floh $
**
**  Text-Engine für cgl.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <stdlib.h>
#include <string.h>

#include "bitmap/cglclass.h"

#define cgl_GetWord(in) ((WORD)(((((UBYTE)(*in++)))<<8)|(*in++)))

/*-----------------------------------------------------------------*/
void cgl_DrawText(struct cgl_data *cd,
                  struct raster_data *rd,
                  UWORD far *disp, UWORD disp_w, UWORD disp_h,
                  struct rast_text *rt)
/*
**  FUNCTION
**      Rendert Text in eine RGB5551-Bitmap.
**
**  CHANGED
**      01-Dec-96   floh    created
**      26-Feb-97   floh    Fontpage muß jetzt eine "gemangelte"
**                          Textur-Bitmap sein (mit BMA_Texture
**                          erzeugt). Damit entfällt der
**                          Remap-Prozeß in der inneren Schleife,
**                          weil die Fontpage schon im richtigen
**                          Format vorliegt.
*/
{
    WORD xpos,ypos,linex,liney;
    ULONG off_hori,len_hori;
    ULONG off_vert,len_vert;
    struct VFMFont *font = NULL;

    UBYTE *in = rt->string;
    UBYTE c;
    LONG c_addx;

    UBYTE *stack[64];
    ULONG index = 0;

    /*** NULL Ptr auf Stack ***/
    stack[index++] = NULL;

    /*** aktuelles Stream-Byte holen ***/
    off_hori = 0;
    len_hori = 0;
    off_vert = 0;
    len_vert = 0;
    c_addx   = 1;
    for (;;) {
        c = *in++;
        if (c != 0) {

            /*** Char rendern ***/
            struct FontChar *c_desc = &(font->fchars[c]);
            LONG c_w,c_h;
            LONG c_addy;
            UWORD *c_from;

            UWORD far *out;
            LONG out_addy;

            /*** Breite und Höhe des Chars ***/
            if (len_hori != 0) c_w = len_hori - off_hori;
            else               c_w = c_desc->width - off_hori;
            c_h = len_vert - off_vert;

            /*** Anfang des Chars in Font-Bmp ***/
            c_from = ((UWORD *)font->page_bmp->Data) +
                     c_desc->offset +
                     off_hori +
                     off_vert * font->page_bmp->Width;
            if (c_addx) c_addy = font->page_bmp->Width - c_w;
            else        c_addy = font->page_bmp->Width;

            /*** Anfang des Chars in Display ***/
            out      = disp + ypos*disp_w + xpos;
            out_addy = disp_w - c_w;

            /*** Char-Render-Loop ***/
            do {
                LONG count_w = c_w;
                do {
                    UWORD pix = *c_from;
                    if (pix & (1<<15)) *out = pix;
                    c_from += c_addx;
                    out++;
                } while (--count_w);
                c_from += c_addy;
                out    += out_addy;
            } while (--c_h);

            /*** Per-Char-Parameter updaten ***/
            xpos    += c_w;
            len_hori = 0;
            off_hori = 0;
            c_addx   = 1;

        }else{

            /*** Control Sequenz ***/
            switch(*in++) {
                case 0: /*** eos() ***/
                    in = stack[--index];
                    if (in == NULL) return;
                    break;

                case 1: /*** xpos_abs() ***/
                    xpos  = cgl_GetWord(in) + rd->ioff_x;
                    linex = xpos;
                    liney = ypos;
                    off_vert = 0;
                    len_vert = font->height;
                    break;

                case 2: /*** ypos_abs() ***/
                    ypos  = cgl_GetWord(in) + rd->ioff_y;
                    linex = xpos;
                    liney = ypos;
                    off_vert = 0;
                    len_vert = font->height;
                    break;

                case 3: /*** xpos_brel() ***/
                    xpos = cgl_GetWord(in);
                    if (xpos < 0) xpos += rd->r->Width;
                    linex = xpos;
                    liney = ypos;
                    off_vert = 0;
                    len_vert = font->height;
                    break;

                case 4: /*** ypos_brel() ***/
                    ypos = cgl_GetWord(in);
                    if (ypos < 0) ypos += rd->r->Height;
                    linex = xpos;
                    liney = ypos;
                    off_vert = 0;
                    len_vert = font->height;
                    break;

                case 5: /*** xpos_rel() ***/
                    xpos += cgl_GetWord(in);
                    break;

                case 6: /*** ypos_rel() ***/
                    ypos += cgl_GetWord(in);
                    break;

                case 7: /*** new_line() ***/
                    liney += (len_vert-off_vert);
                    xpos   = linex;
                    ypos   = liney;
                    off_vert = 0;
                    len_vert = font->height;
                    break;

                case 8: /*** new_font() ***/
                    font = rd->fonts[*in++];
                    break;

                case 9: /*** clip() ***/
                    {
                        UBYTE clip = *in++;
                        stack[index++] = in;
                        in = rt->clips[clip];
                    };
                    break;

                case 10:    /*** stretch() ***/
                    len_hori = *in++;
                    off_hori = 0;
                    c_addx   = 0;
                    break;

                case 11:    /*** stretch_to() ***/
                    len_hori  = *in++;
                    len_hori -= (xpos-linex);
                    off_hori  = 0;
                    c_addx    = 0;
                    break;

                case 12:    /*** off_hori() ***/
                    off_hori = *in++;
                    break;

                case 13:    /*** len_hori() ***/
                    len_hori = *in++;
                    break;

                case 14:    /*** off_vert() ***/
                    off_vert = *in++;
                    break;

                case 15:    /*** len_vert() ***/
                    len_vert = *in++;
                    break;

                case 16:    /*** newfont_flush() ***/
                    font = rd->fonts[*in++];
                    off_vert = 0;
                    len_vert = font->height;
                    break;

                case 17:    /*** stretch_long() ***/
                    len_hori  = cgl_GetWord(in);
                    len_hori -= (xpos-linex);
                    off_hori  = 0;
                    c_addx    = 0;
                    break;
            };
        };
    };  // for(;;)

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_RASTM_Text, struct rast_text *msg)
/*
**  CHANGED
**      01-Dec-96   floh    created
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    struct raster_data *rd = INST_DATA((cl->superclass->superclass),o);
    UWORD far *disp;

    /*** Back-Buffer für Direkt-Zugriff locken ***/
    if (cglLockBuffer(CGL_BACK_BUFFER,&disp) == CGL_SUCCESS) {
        cgl_DrawText(cd,rd,disp,cd->export.x_size,cd->export.y_size,msg);
    };
}

