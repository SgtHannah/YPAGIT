/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_text.c,v $
**  $Revision: 38.4 $
**  $Date: 1998/01/06 15:03:12 $
**  $Locker:  $
**  $Author: floh $
**
**  w3d_text.c -- Text-Engine (Gummi-Zelle)
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
#define w3d_GetWord(in) w3d_GetWordFunc(&(in))

__inline int w3d_GetWordFunc(unsigned char **in)
{
	unsigned int retval;
	retval = (*in)[0];
	retval <<= 8;
	retval |= (*in)[1];

	(*in) += 2;

	return (int)retval;
}
#else
#define w3d_GetWord(in) ((unsigned short)(((((unsigned char)(*in++)))<<8)|(*in++)))
#endif

/*-----------------------------------------------------------------*/
void w3d_DrawText(struct windd_data *wdd,
                  struct win3d_data *w3d,
                  struct w3d_VFMFont **fonts,
                  unsigned char *string,
                  unsigned char **clips)

/*
**  FUNCTION
**      Rendert Text in den Backbuffer. Dieser muß gelockt sein,
**      ein Pointer auf den Surface-Mem muß in <wdd->back_ptr>
**      stehen!
**      Die Fontpages müssen als (BMA_Texture,TRUE) && (BMA_TxtBlittable)
**      sein!
**
**  CHANGED
**      25-Mar-97   floh    created
**      01-Dec-97   floh    + set_dbcs_flags(), unset_dbcs_flags()
**      08-Dec-97   floh    + dbcs_color()
*/
{
    if (wdd->back_ptr) {

        short xpos,ypos,linex,liney;
        short dbcs_x = 0;
        short dbcs_y = 0;
        unsigned short override_dbcs_flags = 0;
        unsigned long off_hori,len_hori;
        unsigned long off_vert,len_vert;
        struct w3d_VFMFont *act_font = NULL;

        unsigned short s_colorkey = w3d->p->disp_pfmt.color_key;
        unsigned char *in = string;
        unsigned char c;
        long c_addx;
        unsigned long pix_size = w3d->p->disp_pfmt.byte_size;

        unsigned char *stack[64];
        unsigned long index = 0;
        unsigned long pitch = wdd->back_pitch / pix_size;

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
                struct w3d_FontChar *c_desc = &(act_font->fchars[c]);
                long c_w,c_h;
                long c_addy;
                unsigned long out_addy;

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
                if (pix_size == 2) {
                    unsigned short *c_from;
                    unsigned short *out;
                    c_from = ((unsigned short *)act_font->page_bmp->Data) +
                             c_desc->offset +
                             off_hori +
                             off_vert * act_font->page_bmp->Width;
                    out = ((unsigned short *)wdd->back_ptr) + ypos*pitch + xpos;
                    do {
                        unsigned long count_w = c_w;
                        do {
                            unsigned short pix = *c_from;
                            if (pix!=s_colorkey) *out = pix;
                            c_from += c_addx;
                            out++;
                        } while (--count_w);
                        c_from += c_addy;
                        out    += out_addy;
                    } while (--c_h);
                };

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
                        xpos  = w3d_GetWord(in) + (wdd->back_w>>1);
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 2: /*** ypos_abs() ***/
                        ypos  = w3d_GetWord(in) + (wdd->back_h>>1);
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 3: /*** xpos_brel() ***/
                        xpos = w3d_GetWord(in);
                        if (xpos < 0) xpos += pitch;
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 4: /*** ypos_brel() ***/
                        ypos = w3d_GetWord(in);
                        if (ypos < 0) ypos += wdd->back_h;
                        linex = xpos;
                        liney = ypos;
                        off_vert = 0;
                        len_vert = act_font->height;
                        break;

                    case 5: /*** xpos_rel() ***/
                        xpos += w3d_GetWord(in);
                        break;

                    case 6: /*** ypos_rel() ***/
                        ypos += w3d_GetWord(in);
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
                        len_hori  = w3d_GetWord(in);
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
							dbcs_w = w3d_GetWord(in);
                            dbcs_flags = w3d_GetWord(in) | override_dbcs_flags;
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
                        override_dbcs_flags |= w3d_GetWord(in); 
                        #endif
                        break;

                    case 21:    /*** unset_dbcs_flags() ***/
                        #ifdef __DBCS__
                        override_dbcs_flags &= ~w3d_GetWord(in);
                        #endif
                        break;

                    case 22:    /*** dbcs_color() ***/
                        #ifdef __DBCS__
                        r = w3d_GetWord(in);
                        g = w3d_GetWord(in);
                        b = w3d_GetWord(in);
                        dbcs_Add(NULL,r,g,b,0,DBCSF_COLOR);
                        #endif __DBCS__
                        break;
                };
            };
        };  // for(;;)
    };

    /*** Ende ***/
}

