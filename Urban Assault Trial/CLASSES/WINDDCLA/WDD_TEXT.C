/*
**  $Source: PRG:VFM/Classes/_WinDDClass/wdd_text.c,v $
**  $Revision: 38.1 $
**  $Date: 1998/01/06 15:04:57 $
**  $Locker:  $
**  $Author: floh $
**
**  wdd_text.c -- Text-Engine (Gummi-Zelle)
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include "stdlib.h"
#include "string.h"

#define WIN3D_WINBOX
#include "bitmap/win3dclass.h"

/*** importiert aus windd.class ***/
extern LPDIRECTDRAW lpDD;
extern LPDIRECT3D2  lpD3D2;
extern struct wdd_Data wdd_Data;
extern void wdd_FailMsg(char *title, char *msg, unsigned long code);

#ifdef _MSC_VER
#define wdd_GetWord(in) wdd_GetWordFunc(&(in))

__inline int wdd_GetWordFunc(unsigned char **in)
{
	unsigned int retval;
	retval = (*in)[0];
	retval <<= 8;
	retval |= (*in)[1];

	(*in) += 2;

	return (int)retval;
}
#else
#define wdd_GetWord(in) ((unsigned short)(((((unsigned char)(*in++)))<<8)|(*in++)))
#endif

/*-----------------------------------------------------------------*/
void wdd_DrawText(struct windd_data *wdd,
                  struct wdd_VFMFont **fonts,
                  unsigned char *string,
                  unsigned char **clips)

/*
**  FUNCTION
**		Dies ist eine 1:1 Version des Textrenderers der raster.class,
**		nur eben in C und nicht in Assembler. Grund dafuer war die
**		neue put_dbcs() Funktion.
**
**  CHANGED
**		20-Nov-97	floh	created
**							+ neuer Instruktions-Handler: put_dbcs()
**      22-Nov-97   floh    + fuer DBCS-Strings wird die Hoehe der 
**                            Clipbox auf die Fonthoehe gesetzt.
**                          + fuer DBCS-Strings gelten jetzt die "ueblichen"
**                            Koordinatenangaben.
**      01-Dec-97   floh    + set_dbcs_flags(), unset_dbcs_flags()
**      09-Dec-97   floh    + dbcs_color()
*/
{
    if (wdd->back_ptr) {

        short xpos,ypos,linex,liney;
        short dbcs_x = 0;
        short dbcs_y = 0;
        unsigned short override_dbcs_flags = 0;
        unsigned long off_hori,len_hori;
        unsigned long off_vert,len_vert;
        struct wdd_VFMFont *act_font = NULL;

        unsigned char *in = string;
        unsigned char c;
        long c_addx;
        unsigned char *stack[64];
        unsigned long index = 0;
        unsigned long pitch = wdd->back_pitch;
        
        long back_w,back_h;

        if (wdd->flags & WINDDF_IsFullScrHalf) {
            back_w=wdd->back_w>>1; back_h=wdd->back_h>>1;
        } else {
            back_w=wdd->back_w; back_h=wdd->back_h;
        };

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
                struct wdd_FontChar *c_desc = &(act_font->fchars[c]);
                long c_w,c_h;
                long c_addy;
                unsigned long out_addy;
                unsigned char *c_from;
                unsigned char *out;

                /*** Breite und Höhe des Chars ***/
                if (len_hori != 0) c_w = len_hori - off_hori;
                else               c_w = c_desc->width - off_hori;
                c_h = len_vert - off_vert;

                /*** Anfang des Chars in Font-Bmp ***/
                if (c_addx) c_addy = act_font->page_bmp->Width - c_w;
                else        c_addy = act_font->page_bmp->Width;

                /*** Anfang des Chars in Display ***/
                out_addy = pitch - c_w;

                /*** Char-Render-Loop ***/
                c_from = ((unsigned char *)act_font->page_bmp->Data) +
                         c_desc->offset +
                         off_hori +
                         off_vert * act_font->page_bmp->Width;
                out = ((unsigned char *)wdd->back_ptr)+ypos*pitch+xpos;
                do {
                    unsigned long count_w = c_w;
                    do {
                        unsigned char pix = *c_from;
                        if (pix) *out = pix;
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
                    short r,g,b;
                    case 0: /*** eos() ***/
                        in = stack[--index];
                        if (in == NULL) {
                            #ifdef __DBCS__
                            DDSURFACEDESC ddsd;
                            if (dbcs_Flush(&ddsd)) {
                                wdd->back_ptr   = ddsd.lpSurface;
                                wdd->back_pitch = ddsd.lPitch;
                            };
                            #endif
                            return;
                        };
                        break;

                    case 1: /*** xpos_abs() ***/
                        xpos  = wdd_GetWord(in) + (back_w>>1);
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 2: /*** ypos_abs() ***/
                        ypos  = wdd_GetWord(in) + (back_h>>1);
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 3: /*** xpos_brel() ***/
                        xpos = wdd_GetWord(in);
                        if (xpos < 0) xpos += pitch;
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 4: /*** ypos_brel() ***/
                        ypos = wdd_GetWord(in);
                        if (ypos < 0) ypos += back_h;
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 5: /*** xpos_rel() ***/
                        xpos += wdd_GetWord(in);
                        break;

                    case 6: /*** ypos_rel() ***/
                        ypos += wdd_GetWord(in);
                        break;

                    case 7: /*** new_line() ***/
                        liney += (len_vert-off_vert);
                        xpos   = linex;
                        ypos   = liney;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 8: /*** new_font() ***/
                        act_font = fonts[*in++];
                        break;

                    case 9: /*** clip() ***/
                        {
                            unsigned char clip = *in++;
                            stack[index++] = in;
                            in = clips[clip];
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
                        act_font = fonts[*in++];
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 17:    /*** stretch_long() ***/
                        len_hori  = wdd_GetWord(in);
                        len_hori -= (xpos-linex);
                        off_hori  = 0;
                        c_addx    = 0;
                        break;

					case 18:	/*** put_dbcs() ***/
						{
							#ifdef __DBCS__
							char *dbcs_text;
                            short dbcs_w,dbcs_h;
                            short dbcs_flags;
							dbcs_w = wdd_GetWord(in);
                            dbcs_flags = wdd_GetWord(in) | override_dbcs_flags;
                            dbcs_text  = in;
                            in += (lstrlen(dbcs_text)+1);  
							dbcs_h = act_font->height;
							dbcs_Add(dbcs_text,dbcs_x,dbcs_y,dbcs_w,dbcs_h,dbcs_flags);
							#endif
						};
						break;

                    case 19:    /*** freeze_dbcs_pos() ***/
                        #ifdef __DBCS__
                        dbcs_x = xpos;
                        dbcs_y = ypos;
                        #endif
                        break;

                    case 20:    /*** set_dbcs_flags() ***/
                        #ifdef __DBCS__
                        override_dbcs_flags |= wdd_GetWord(in); 
                        #endif
                        break;

                    case 21:    /*** unset_dbcs_flags() ***/
                        #ifdef __DBCS__
                        override_dbcs_flags &= ~wdd_GetWord(in);
                        #endif
                        break;

                    case 22:    /*** dbcs_color() ***/
                        #ifdef __DBCS__
                        r = wdd_GetWord(in);
                        g = wdd_GetWord(in);
                        b = wdd_GetWord(in);
                        dbcs_Add(NULL,r,g,b,0,DBCSF_COLOR);
                        #endif __DBCS__
                        break;
                };
            };
        };  // for(;;)
    };

    /*** Ende ***/
}
