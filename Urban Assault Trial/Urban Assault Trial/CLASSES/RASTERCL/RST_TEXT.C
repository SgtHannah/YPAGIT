/*
**  $Source: PRG:VFM/Classes/_RasterClass/rst_text.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 14:58:21 $
**  $Locker:  $
**  $Author: floh $
**
**  Font- und Text-Handling der raster.class
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>

#include "nucleus/nucleus2.h"
#include "bitmap/rasterclass.h"

_extern_use_nucleus

/*-------------------------------------------------------------------
**  folgende globalen Variablen werden von der Text-Render-Engine
**  benötigt, damit sich die Portierung aus der Gfx-Engine
**  etwas vereinfacht!
*/

ULONG dt_BytesPerRow;
UWORD dt_Width;
UWORD dt_Height;
UWORD dt_XOffset;
UWORD dt_YOffset;
UBYTE *dt_Display;
UBYTE *dt_TracyRemapData;
struct VFMFont **dt_Fonts;

#ifdef AMIGA
extern __asm void rst_DrawText(__a0 struct rast_text *);
#else
#ifdef _MSC_VER
#define rst_DrawText(x)
#else
extern void rst_DrawText(struct rast_text *);
#endif
#endif

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_SetFont, struct rast_font *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rd->fonts[msg->id] = msg->font;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_GetFont, struct rast_font *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    if (msg->font) {
        ULONG i;
        for (i=0; i<256; i++) {
            if (msg->font == rd->fonts[i]) {
                msg->id = i;
                return;
            };
        };
        msg->id = -1;
    } else {
        msg->font = rd->fonts[msg->id];
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_Text, struct rast_text *msg)
/*
**  CHANGED
**      04-Jun-96   floh    created
**      12-Feb-97   floh    Pitch-Fix
*/
{
    struct raster_data *rd = INST_DATA(cl,o);

    /*** globale Parameter-Variablen für Text-Engine ausfüllen ***/
    dt_Width   = rd->r->Width;
    dt_Height  = rd->r->Height;
    dt_XOffset = rd->ioff_x;
    dt_YOffset = rd->ioff_y;
    dt_Display = rd->r->Data;
    dt_TracyRemapData = rd->tracy_body;
    dt_BytesPerRow    = rd->r->BytesPerRow;
    dt_Fonts   = &(rd->fonts[0]);

    /*** Text-Engine zünden! ***/
    rst_DrawText(msg);
}

