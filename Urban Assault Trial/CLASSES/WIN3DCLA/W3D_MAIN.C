/*
**  $Source: PRG:VFM/Classes/_Win3DClass/w3d_main.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 15:02:32 $
**  $Locker: floh $
**  $Author: floh $
**
**  Direct3D-Treiber-Klasse (Immediate Mode).
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include "stdlib.h"
#include "string.h"

#define NOMATH_FUNCTIONS
#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "bitmap/win3dclass.h"

extern ULONG wdd_DoDirect3D;
extern ULONG wdd_ZBufWhenTracy;
extern ULONG wdd_CanDoAlpha;

#define LINECLIP_LEFT   (1<<0)
#define LINECLIP_RIGHT  (1<<1)
#define LINECLIP_TOP    (1<<2)
#define LINECLIP_BOTTOM (1<<3)

/*-----------------------------------------------------------------*/
_dispatcher(Object *, w3d_OM_NEW, struct TagItem *);
_dispatcher(ULONG, w3d_OM_DISPOSE, void *);
_dispatcher(void, w3d_OM_SET, struct TagItem *);

_dispatcher(ULONG, w3d_DISPM_ObtainTexture, struct disp_texture *);
_dispatcher(void, w3d_DISPM_ReleaseTexture, struct disp_texture *);
_dispatcher(void, w3d_DISPM_MangleTexture, struct disp_texture *);
_dispatcher(ULONG, w3d_DISPM_LockTexture, struct disp_texture *);
_dispatcher(void, w3d_DISPM_UnlockTexture, struct disp_texture *);
_dispatcher(void, w3d_DISPM_BeginSession, void *);
_dispatcher(void, w3d_DISPM_EndSession, void *);
_dispatcher(void, w3d_DISPM_MixPalette, struct disp_mixpal_msg *);
_dispatcher(void, w3d_DISPM_ScreenShot, struct disp_screenshot_msg *);

_dispatcher(void, w3d_RASTM_Flush, void *);
_dispatcher(void, w3d_RASTM_Poly, struct rast_poly *);
_dispatcher(void, w3d_RASTM_Line, struct rast_line *);
_dispatcher(void, w3d_RASTM_IntLine, struct rast_intline *);
_dispatcher(void, w3d_RASTM_ClippedLine, struct rast_line *);
_dispatcher(void, w3d_RASTM_IntClippedLine, struct rast_intline *);
_dispatcher(void, w3d_RASTM_Blit, struct rast_blit *);
_dispatcher(void, w3d_RASTM_MaskBlit, struct rast_maskblit *);
_dispatcher(void, w3d_RASTM_ClippedBlit, struct rast_blit *);
_dispatcher(void, w3d_RASTM_Begin3D, void *);
_dispatcher(void, w3d_RASTM_End3D, void *);
_dispatcher(void, w3d_RASTM_Begin2D, void *);
_dispatcher(void, w3d_RASTM_End2D, void *);
_dispatcher(void, w3d_RASTM_Text, struct rast_text *);

/*** w3d_winbox.c ***/
ULONG w3d_InitWin3D(struct windd_data *, struct win3d_data *);
void  w3d_KillWin3D(struct windd_data *, struct win3d_data *);
unsigned long w3d_LockBackBuffer(struct windd_data *, struct win3d_data *);
void w3d_UnlockBackBuffer(struct windd_data *, struct win3d_data *);
void w3d_MixPalette(struct win3d_data *, unsigned long, unsigned long *, unsigned long *);
void w3d_FrameSnap(struct windd_data *, struct win3d_data *, void *);

/*** w3d_pixfmt.c ***/
ULONG w3d_InitPixFormats(struct windd_data *, struct win3d_data *);
void w3d_KillPixFormats(struct windd_data *, struct win3d_data *);

/*** w3d_poly.c ***/
ULONG w3d_InitPolyEngine(struct windd_data *, struct win3d_data *);
void w3d_KillPolyEngine(struct windd_data *, struct win3d_data *);
unsigned long w3d_BeginRender(struct windd_data *, struct win3d_data *);
void w3d_EndRender(struct windd_data *wdd, struct win3d_data *);
void w3d_DrawPoly(struct windd_data *, struct win3d_data *, struct w3d_RastPoly *, struct w3d_BmpAttach *, unsigned long, unsigned long);
void w3d_BeginScene(struct windd_data *, struct win3d_data *);
void w3d_EndScene(struct windd_data *, struct win3d_data *);
void w3d_FlushDelayed(struct windd_data *,struct win3d_data *);
void w3d_FlushTracy(struct windd_data *, struct win3d_data *);

/*** w3d_txtcache.c ***/
ULONG w3d_InitTxtCache(struct windd_data *, struct win3d_data *);
void w3d_KillTxtCache(struct windd_data *, struct win3d_data *);
ULONG w3d_ObtainTexture(struct windd_data *,struct win3d_data *,ULONG,ULONG,struct w3d_BmpAttach **);
void w3d_ReleaseTexture(struct windd_data *,struct win3d_data *,struct w3d_BmpAttach *);
void w3d_MangleTexture(struct windd_data *,struct win3d_data *,char *,char *,struct w3d_BmpAttach *,ULONG);
ULONG w3d_LockTexture(struct windd_data *,struct win3d_data *,void **,struct w3d_BmpAttach *);
void w3d_UnlockTexture(struct windd_data *,struct win3d_data *,void *,struct w3d_BmpAttach *);
void w3d_TxtCacheBeginFrame(struct windd_data *,struct win3d_data *);
void w3d_TxtCacheEndFrame(struct windd_data *,struct win3d_data *);
unsigned long w3d_ObtainTxtBlt(struct windd_data *,struct win3d_data *,unsigned long,unsigned long,void **);
void w3d_ReleaseTxtBlt(struct windd_data *,struct win3d_data *,void *);
void w3d_MangleTxtBlt(struct windd_data *,struct win3d_data *,char *,char *,unsigned long, unsigned long, void *);
void w3d_TxtCacheBeginSession(struct windd_data *, struct win3d_data *);

/*** w3d_text.c ***/
void w3d_DrawText(struct windd_data *,struct win3d_data *,struct w3d_VFMFont *,unsigned char *,unsigned char **);

/*** w3d_prim.c ***/
void w3d_DrawLine(struct windd_data *wdd, struct win3d_data *w3d,
                  long x0, long y0, long x1, long y1,
                  long r0, long g0, long b0, long r1, long g1, long b1);
void w3d_StretchBlit(struct windd_data *wdd, struct win3d_data *w3d,
                  void *from_data, long from_w,
                  long from_xmin, long from_ymin,
                  long from_xmax, long from_ymax,
                  long to_xmin,   long to_ymin,
                  long to_xmax,   long to_ymax);
void w3d_MaskStretchBlit(struct windd_data *wdd, struct win3d_data *w3d,
                         void *from_data, long from_w,
                         void *mask_data, unsigned char mask_key,
                         long from_xmin, long from_ymin,
                         long from_xmax, long from_ymax,
                         long to_xmin,   long to_ymin,
                         long to_xmax,   long to_ymax);

#define WIN3D_NUM_CONFIG_ITEMS (7)
struct ConfigItem w3d_ConfigItems[WIN3D_NUM_CONFIG_ITEMS] = {
    {"gfx.dither",     CONFIG_BOOL,    FALSE},
    {"gfx.filter",     CONFIG_BOOL,    FALSE},
    {"gfx.antialias",  CONFIG_BOOL,    FALSE},
    {"gfx.alpha",      CONFIG_INTEGER, 192},
    {"gfx.zbuf_when_tracy", CONFIG_BOOL, FALSE},
    {"gfx.colorkey",   CONFIG_BOOL,    FALSE},
    {"gfx.force_emul", CONFIG_BOOL,   FALSE},
};

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG w3d_Methods[NUCLEUS_NUMMETHODS];
struct ClassInfo w3d_clinfo;

struct ClassInfo *w3d_MakeClass(ULONG id,...);
BOOL w3d_FreeClass(void);

struct GET_Class w3d_GET = {
    &w3d_MakeClass,
    &w3d_FreeClass,
};

struct GET_Class *w3d_Entry(void)
{
    return(&w3d_GET);
};



struct SegInfo win3d_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/gfx/win3d.class"
    },
    w3d_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *w3d_MakeClass(ULONG id,...)
/*
**  CHANGED
**      10-Mar-97   floh    created
**      05-Mar-97   floh    + RASTM_MaskBlit
*/
{
    ULONG i;

    /*** Klasse initialisieren ***/
    memset(w3d_Methods,0,sizeof(w3d_Methods));

    w3d_Methods[OM_NEW]      = (ULONG) w3d_OM_NEW;
    w3d_Methods[OM_DISPOSE]  = (ULONG) w3d_OM_DISPOSE;
    w3d_Methods[OM_SET]      = (ULONG) w3d_OM_SET;

    w3d_Methods[DISPM_ObtainTexture]  = (ULONG) w3d_DISPM_ObtainTexture;
    w3d_Methods[DISPM_ReleaseTexture] = (ULONG) w3d_DISPM_ReleaseTexture;
    w3d_Methods[DISPM_MangleTexture]  = (ULONG) w3d_DISPM_MangleTexture;
    w3d_Methods[DISPM_LockTexture]    = (ULONG) w3d_DISPM_LockTexture;
    w3d_Methods[DISPM_UnlockTexture]  = (ULONG) w3d_DISPM_UnlockTexture;
    w3d_Methods[DISPM_BeginSession]   = (ULONG) w3d_DISPM_BeginSession;
    w3d_Methods[DISPM_EndSession]     = (ULONG) w3d_DISPM_EndSession;
    w3d_Methods[DISPM_MixPalette]     = (ULONG) w3d_DISPM_MixPalette;
    w3d_Methods[DISPM_ScreenShot]     = (ULONG) w3d_DISPM_ScreenShot;

    w3d_Methods[RASTM_Poly]           = (ULONG) w3d_RASTM_Poly;
    w3d_Methods[RASTM_Begin3D]        = (ULONG) w3d_RASTM_Begin3D;
    w3d_Methods[RASTM_End3D]          = (ULONG) w3d_RASTM_End3D;
    w3d_Methods[RASTM_Begin2D]        = (ULONG) w3d_RASTM_Begin2D;
    w3d_Methods[RASTM_End2D]          = (ULONG) w3d_RASTM_End2D;
    w3d_Methods[RASTM_Text]           = (ULONG) w3d_RASTM_Text;
    w3d_Methods[RASTM_Line]           = (ULONG) w3d_RASTM_Line;
    w3d_Methods[RASTM_IntLine]        = (ULONG) w3d_RASTM_IntLine;
    w3d_Methods[RASTM_ClippedLine]    = (ULONG) w3d_RASTM_ClippedLine;
    w3d_Methods[RASTM_IntClippedLine] = (ULONG) w3d_RASTM_IntClippedLine;
    w3d_Methods[RASTM_Blit]           = (ULONG) w3d_RASTM_Blit;
    w3d_Methods[RASTM_ClippedBlit]    = (ULONG) w3d_RASTM_ClippedBlit;
    w3d_Methods[RASTM_MaskBlit]       = (ULONG) w3d_RASTM_MaskBlit;

    w3d_clinfo.superclassid = WINDD_CLASSID;
    w3d_clinfo.methods      = w3d_Methods;
    w3d_clinfo.instsize     = sizeof(struct win3d_data);
    w3d_clinfo.flags        = 0;

    return(&w3d_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL w3d_FreeClass(void)
/*
**  CHANGED
**      10-Mar-97   floh    created
*/
{
    wdd_DoDirect3D = FALSE;
    return(TRUE);
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, w3d_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      10-Mar-97   floh    created
**      20-Aug-97   floh    + gfx.colorkey ConfigItem
**      10-Mar-98   floh    + Initialisierung teilweise aus
**                            MakeClass nach OM_NEW verlegt
*/
{
    struct windd_data *wdd;
    struct win3d_data *w3d;
    Object *newo;

    /*** Force-Emul??? dann gehen wir hier schief! ***/
    _GetConfigItems(NULL,w3d_ConfigItems,WIN3D_NUM_CONFIG_ITEMS);
    if (w3d_ConfigItems[6].data) return(NULL);

    /*** 3D-Modus für windd.class aktivieren ***/
    wdd_DoDirect3D = TRUE;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);
    w3d = INST_DATA(cl,newo);
    wdd = INST_DATA(cl->superclass,newo);

    /*** Konfiguration auslesen ***/
    w3d->dither        = w3d_ConfigItems[0].data;
    w3d->filter        = w3d_ConfigItems[1].data;
    w3d->antialias     = w3d_ConfigItems[2].data;
    w3d->zbufwhentracy = w3d_ConfigItems[4].data;
    w3d->colorkey      = w3d_ConfigItems[5].data;

    /*** Alpha-Wert hardgecodet für Stipple und Alpha ***/
    if (wdd_CanDoAlpha) w3d->alpha = 192;
    else                w3d->alpha = 128;

    /*** allgemeines Zeuch initialisieren ***/
    if (!w3d_InitWin3D(wdd,w3d)) {
        _LogMsg("win3d.class: Initialization failed.\n");
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };

    /*** Pixelformate und Lookup-Tables ***/
    if (!w3d_InitPixFormats(wdd,w3d)) {
        _LogMsg("win3d.class: Pixelformat problems.\n");
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };

    /*** Textur Cache initialisieren ***/
    if (!w3d_InitTxtCache(wdd,w3d)) {
        _LogMsg("win3d.class: Failed to initialize texture cache.\n");
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };

    /*** Polygon-Engine initialisieren ***/
    if (!w3d_InitPolyEngine(wdd,w3d)) {
        _LogMsg("win3d.class: Failed to initialize polygon engine.\n");
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };

    /*** schon fertig !?! ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, w3d_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      10-Mar-97   floh    created
*/
{
    struct win3d_data *w3d = INST_DATA(cl,o);
    struct windd_data *wdd = INST_DATA(cl->superclass,o);

    w3d_KillPolyEngine(wdd,w3d);
    w3d_KillTxtCache(wdd,w3d);
    w3d_KillPixFormats(wdd,w3d);
    w3d_KillWin3D(wdd,w3d);

    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      23-Feb-98   floh    created
*/
{
    struct win3d_data *w3d = INST_DATA(cl,o);
    register ULONG tag;
    struct TagItem *ti = attrs;

    /* Attribut-Liste scannen... */
    while ((tag = ti->ti_Tag) != TAG_DONE) {

        register ULONG data = ti++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) data; break;
            case TAG_SKIP:      ti += data; break;
            default:

                /* dann die eigentlichen Attribute, schön nacheinander */
                switch (tag) {
                    case WINDDA_TextureFilter:
                        w3d->filter = data;
                        break;
                };
        };
    };
    _supermethoda(cl,o,OM_SET,attrs);
}


/*=================================================================**
**  Abgeleitete raster.class Methoden                              **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_Begin3D, void *nil)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    ENTERED("w3d_RASTM_Begin3D");
    w3d_Begin(wdd,w3d);
    LEFT("w3d_RASTM_Begin3D");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_End3D, void *nil)
/*
**  CHANGED
**      12-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    ENTERED("w3d_RASTM_End3D");
    w3d_FlushDelayed(wdd,w3d);
    w3d_FlushTracy(wdd,w3d);
    w3d_EndScene(wdd,w3d);
    w3d_TxtCacheEndFrame(wdd,w3d);
    LEFT("w3d_RASTM_End3D");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_Begin2D, void *nil)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    ENTERED("w3d_RASTM_Begin2D");
    w3d_LockBackBuffer(wdd,w3d);
    LEFT("w3d_RASTM_Begin2D");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_End2D, void *nil)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    ENTERED("w3d_RASTM_End2D");
    w3d_UnlockBackBuffer(wdd,w3d);
    LEFT("w3d_RASTM_End2D");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_Poly, struct rast_poly *msg)
/*
**  CHANGED
**      13-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    void *bmp_attach;
    if (msg->map[0]) bmp_attach = msg->map[0]->TxtHandle;
    else             bmp_attach = NULL;
    w3d_DrawPoly(wdd,w3d,msg,bmp_attach,FALSE,FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_Text, struct rast_text *msg)
/*
**  CHANGED
**      26-Mar-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl->superclass->superclass->superclass,o);
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    ENTERED("w3d_RASTM_Text");
    w3d_DrawText(wdd,w3d,&(rd->fonts),msg->string,msg->clips);
    LEFT("w3d_RASTM_Text");
}

/*=================================================================**
**  Abgeleitete display.class Methoden                             **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, w3d_DISPM_ObtainTexture, struct disp_texture *msg)
/*
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    struct VFMBitmap *bmp = msg->texture;
    ULONG retval;
    if (bmp->Flags & VBF_TxtBlittable) {
        bmp->BytesPerRow = w3d_ObtainTxtBlt(wdd,w3d,bmp->Width,bmp->Height,&(bmp->Data));
        if (bmp->BytesPerRow > 0) retval = TRUE;
        else                      retval = FALSE;
    } else {
        retval = w3d_ObtainTexture(wdd,w3d,bmp->Width,bmp->Height,&(bmp->TxtHandle));
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_ReleaseTexture, struct disp_texture *msg)
/*
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    struct VFMBitmap *bmp = msg->texture;
    if (bmp->Flags & VBF_TxtBlittable) w3d_ReleaseTxtBlt(wdd,w3d,bmp->Data);
    else                               w3d_ReleaseTexture(wdd,w3d,bmp->TxtHandle);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_MangleTexture, struct disp_texture *msg)
/*
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    struct display_data *dd   = INST_DATA(cl->superclass->superclass,o);
    struct windd_data *wdd    = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d    = INST_DATA(cl,o);
    struct VFMBitmap *bmp     = msg->texture;

    /*** einen Paletten-Slot zur Pixelkonvertierung verwenden? ***/
    if (bmp->Flags & VBF_TxtBlittable) {
        w3d_MangleTxtBlt(wdd, w3d, bmp->ColorMap, &(dd->pal),
                         bmp->Width, bmp->Height, bmp->Data);
    } else {
        w3d_MangleTexture(wdd, w3d, bmp->ColorMap, &(dd->pal),
                          bmp->TxtHandle, (bmp->Flags & VBF_AlphaHint));
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, w3d_DISPM_LockTexture, struct disp_texture *msg)
/*
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    struct VFMBitmap *bmp = msg->texture;
    if (bmp->Flags & VBF_TxtBlittable) return(TRUE);
    else return(w3d_LockTexture(wdd,w3d,&(bmp->Data),bmp->TxtHandle));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_UnlockTexture, struct disp_texture *msg)
/*
**  CHANGED
**      11-Mar-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    struct VFMBitmap *bmp = msg->texture;
    if (!(bmp->Flags & VBF_TxtBlittable)) {
        w3d_UnlockTexture(wdd,w3d,bmp->Data,bmp->TxtHandle);
        bmp->Data = NULL;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_BeginSession, void *msg)
/*
**  CHANGED
**      03-Apr-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d = INST_DATA(cl,o);
    /*** flushe den Texturcache komplett ***/
    w3d_TxtCacheBeginSession(wdd,w3d);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_EndSession, void *msg)
/*
**  CHANGED
**      03-Apr-97   floh    created
*/
{ }

/*=================================================================**
**  Diverse Primitive-Render-Support-Routinen.                     **
**=================================================================*/

/*-----------------------------------------------------------------*/
LONG w3d_CompOutCode(LONG x, LONG y, struct rast_intrect *clip)
/*
**  FUNCTION
**      Ermittelt Lagecode fuer [x,y] im Verhaeltnis zum
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
**      04-Jun-96   floh    uebernommen nach raster.class
**      11-Dec-97   floh    + inverse Flag
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
LONG w3d_Clip(struct rast_intrect *clip, struct rast_intrect *line)
/*
**  FUNCTION
**      Clippt die Linie in <line> gegen <clip>, schreibt Ergebnis
**      nach <line> zurueck. Kehrt TRUE zurueck, falls Linie
**      sichtbar, sonst FALSE.
**
**  CHANGED
**      11-Dec-97   floh    + aus raster.class uebernommen
**                          + [inverse] Flag
**                          + die Routine gibt jetzt 3 Ergebnisse zurueck:
**                            -1 -> Linie ist ausserhalb des Cliprects
**                             0 -> Linie ist geclippt worden
**                            +1 -> Linie war von Anfang an innerhalb
*/
{
    ULONG outcode0,outcode1;
    LONG accept = 1;        // voll drin als 
    BOOL done   = FALSE;
    LONG x0 = line->xmin;
    LONG x1 = line->xmax;
    LONG y0 = line->ymin;
    LONG y1 = line->ymax;
    
    /*** LageCodes für beide EndPunkte ***/
    outcode0 = w3d_CompOutCode(x0,y0,clip);
    outcode1 = w3d_CompOutCode(x1,y1,clip);
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
                outcode0 = w3d_CompOutCode(x0,y0,clip);
            } else {
                x1 = x;
                y1 = y;
                outcode1 = w3d_CompOutCode(x1,y1,clip);
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
void w3d_ClippedLine(struct raster_data *rd,
                     struct windd_data *wdd,
                     struct win3d_data *w3d,
                     LONG x0, LONG y0, LONG x1, LONG y1,
                     ULONG r0, ULONG g0, ULONG b0,
                     ULONG r1, ULONG g1, ULONG b1)
/*
**  CHANGED
**      01-Apr-97   floh    created
**      31-Jul-97   floh    + Endpunkt-Farben eingetragen
*/
{
    struct rast_intrect outer_r;
    outer_r.xmin=x0; outer_r.xmax=x1;
    outer_r.ymin=y0; outer_r.ymax=y1;
    /*** Clippen gegen Outside-Rectangle ***/
    if (w3d_Clip(&(rd->clip),&outer_r) != -1) {
        /*** Clippen gegen Inside-Rectangle ***/
        LONG clip_code;
        struct rast_intrect inner_r;
        inner_r = outer_r;
        if (rd->inv_clip.xmin != rd->inv_clip.xmax) clip_code = w3d_Clip(&(rd->inv_clip),&inner_r);
        else                                        clip_code = -1;
        if (clip_code == -1) {
            /*** vollstaendig sichtbar ***/
            w3d_DrawLine(wdd,w3d,inner_r.xmin,inner_r.ymin,inner_r.xmax,inner_r.ymax,r0,g0,b0,r1,g1,b1);

        } else if (clip_code == 0) {
            /*** geclippt, einer von 3 Faellen ***/
            if ((inner_r.xmax == outer_r.xmax) && (inner_r.ymax == outer_r.ymax)) {
                w3d_DrawLine(wdd,w3d,outer_r.xmin,outer_r.ymin,inner_r.xmin,inner_r.ymin,r0,g0,b0,r1,g1,b1);
            } else if ((inner_r.xmin == outer_r.xmin) && (inner_r.ymin == outer_r.ymin)) {
                w3d_DrawLine(wdd,w3d,inner_r.xmax,inner_r.ymax,outer_r.xmax,outer_r.ymax,r0,g0,b0,r1,g1,b1);
            } else {
                w3d_DrawLine(wdd,w3d,outer_r.xmin,outer_r.ymin,inner_r.xmin,inner_r.ymin,r0,g0,b0,r1,g1,b1);
                w3d_DrawLine(wdd,w3d,inner_r.xmax,inner_r.ymax,outer_r.xmax,outer_r.ymax,r0,g0,b0,r1,g1,b1);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL w3d_BltClip(struct raster_data *rd, struct rast_intblit *blt)
/*
**  FUNCTION
**      Clippt die Koordinaten in der <blt>-Struktur.
**      Nur das Target-Rechteck darf geclippt sein!
**
**      Die Routine kehrt FALSE zurück, wenn das
**      Resultat vollständig außerhalb liegt.
**
**  CHANGED
**      02-Apr-97   floh    übernommen aus raster.class
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
_dispatcher(void, w3d_RASTM_Line, struct rast_line *msg)
/*
**  CHANGED
**      01-Apr-97   floh    created
**      31-Jul-97   floh    + Endpunkt-Farben
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct display_data *dd = INST_DATA(cl->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);
    LONG x0 = FLOAT_TO_INT((msg->x0+1.0)*(rd->foff_x-1.0));
    LONG y0 = FLOAT_TO_INT((msg->y0+1.0)*(rd->foff_y-1.0));
    LONG x1 = FLOAT_TO_INT((msg->x1+1.0)*(rd->foff_x-1.0));
    LONG y1 = FLOAT_TO_INT((msg->y1+1.0)*(rd->foff_y-1.0));
    ENTERED("w3d_RASTM_Line");
    w3d_DrawLine(wdd,w3d,x0,y0,x1,y1,
                (rd->fg_pen>>16) & 0xff,
                (rd->fg_pen>>8)  & 0xff,
                (rd->fg_pen)     & 0xff,
                (rd->fg_apen>>16)& 0xff,
                (rd->fg_apen>>8) & 0xff,
                (rd->fg_apen)    & 0xff);
    LEFT("w3d_RASTM_Line");            
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_IntLine, struct rast_intline *msg)
/*
**  CHANGED
**      01-APr-97   floh    created
**      31-Jul-97   floh    + Endpunkt-Farben
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct display_data *dd = INST_DATA(cl->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);
    LONG x0 = msg->x0 + rd->ioff_x;
    LONG y0 = msg->y0 + rd->ioff_y;
    LONG x1 = msg->x1 + rd->ioff_x;
    LONG y1 = msg->y1 + rd->ioff_y;
    ENTERED("w3d_RASTM_IntLine");
    w3d_DrawLine(wdd,w3d,x0,y0,x1,y1,
                (rd->fg_pen>>16) & 0xff,
                (rd->fg_pen>>8)  & 0xff,
                (rd->fg_pen)     & 0xff,
                (rd->fg_apen>>16)& 0xff,
                (rd->fg_apen>>8) & 0xff,
                (rd->fg_apen)    & 0xff);
    LEFT("w3d_RASTM_IntLine");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_ClippedLine, struct rast_line *msg)
/*
**  CHANGED
**      01-APr-97   floh    created
**      31-Jul-97   floh    + Endpunkt-Farben
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct display_data *dd = INST_DATA(cl->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);
    LONG x0,y0,x1,y1;
    ENTERED("w3d_RASTM_ClippedLine");
    x0 = FLOAT_TO_INT((msg->x0+1.0)*(rd->foff_x-1.0));
    y0 = FLOAT_TO_INT((msg->y0+1.0)*(rd->foff_y-1.0));
    x1 = FLOAT_TO_INT((msg->x1+1.0)*(rd->foff_x-1.0));
    y1 = FLOAT_TO_INT((msg->y1+1.0)*(rd->foff_y-1.0));
    w3d_ClippedLine(rd,wdd,w3d,x0,y0,x1,y1,
                   (rd->fg_pen>>16) & 0xff,
                   (rd->fg_pen>>8)  & 0xff,
                   (rd->fg_pen)     & 0xff,
                   (rd->fg_apen>>16)& 0xff,
                   (rd->fg_apen>>8) & 0xff,
                   (rd->fg_apen)    & 0xff);
    LEFT("w3d_RASTM_ClippedLine");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_IntClippedLine, struct rast_intline *msg)
/*
**  CHANGED
**      01-APr-97   floh    created
**      31-Jul-97   floh    + Endpunkt-Farben
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct display_data *dd = INST_DATA(cl->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);
    LONG x0,y0,x1,y1;
    ENTERED("w3d_RASTM_IntClippedLine");
    x0 = msg->x0 + rd->ioff_x;
    y0 = msg->y0 + rd->ioff_y;
    x1 = msg->x1 + rd->ioff_x;
    y1 = msg->y1 + rd->ioff_y;
    w3d_ClippedLine(rd,wdd,w3d,x0,y0,x1,y1,
                   (rd->fg_pen>>16) & 0xff,
                   (rd->fg_pen>>8)  & 0xff,
                   (rd->fg_pen)     & 0xff,
                   (rd->fg_apen>>16)& 0xff,
                   (rd->fg_apen>>8) & 0xff,
                   (rd->fg_apen)    & 0xff);
    LEFT("w3d_RASTM_IntClippedLine");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_Blit, struct rast_blit *msg)
/*
**  CHANGED
**      02-Apr-97   floh    created
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);

    FLOAT to_mulx,to_muly,src_mulx,src_muly;
    LONG from_xmin,from_xmax,from_ymin,from_ymax;
    LONG to_xmin,to_xmax,to_ymin,to_ymax;
    void *from_data;
    ULONG from_w;

    ENTERED("w3d_RASTM_Blit");
    to_mulx   = rd->foff_x;
    to_muly   = rd->foff_y;
    src_mulx  = (FLOAT)(msg->src->Width>>1);
    src_muly  = (FLOAT)(msg->src->Height>>1);
    from_xmin = FLOAT_TO_INT((msg->from.xmin+1.0)*src_mulx);
    from_xmax = FLOAT_TO_INT((msg->from.xmax+1.0)*src_mulx);
    from_ymin = FLOAT_TO_INT((msg->from.ymin+1.0)*src_muly);
    from_ymax = FLOAT_TO_INT((msg->from.ymax+1.0)*src_muly);
    to_xmin   = FLOAT_TO_INT((msg->to.xmin+1.0)*to_mulx);
    to_xmax   = FLOAT_TO_INT((msg->to.xmax+1.0)*to_mulx);
    to_ymin   = FLOAT_TO_INT((msg->to.ymin+1.0)*to_muly);
    to_ymax   = FLOAT_TO_INT((msg->to.ymax+1.0)*to_muly);
    from_data = msg->src->Data;
    from_w    = msg->src->Width;
    w3d_StretchBlit(wdd,w3d,from_data,from_w,
                    from_xmin, from_ymin, from_xmax, from_ymax,
                    to_xmin, to_ymin, to_xmax, to_ymax);
    LEFT("w3d_RASTM_Blit");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_MaskBlit, struct rast_maskblit *msg)
/*
**  CHANGED
**      05-Nov-97   floh    created
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);

    FLOAT to_mulx,to_muly,src_mulx,src_muly;
    LONG from_xmin,from_xmax,from_ymin,from_ymax;
    LONG to_xmin,to_xmax,to_ymin,to_ymax;
    void *from_data;
    ULONG from_w;

    ENTERED("w3d_RASTM_MaskBlit");
    to_mulx   = rd->foff_x;
    to_muly   = rd->foff_y;
    src_mulx  = (FLOAT)(msg->src->Width>>1);
    src_muly  = (FLOAT)(msg->src->Height>>1);
    from_xmin = FLOAT_TO_INT((msg->from.xmin+1.0)*src_mulx);
    from_xmax = FLOAT_TO_INT((msg->from.xmax+1.0)*src_mulx);
    from_ymin = FLOAT_TO_INT((msg->from.ymin+1.0)*src_muly);
    from_ymax = FLOAT_TO_INT((msg->from.ymax+1.0)*src_muly);
    to_xmin   = FLOAT_TO_INT((msg->to.xmin+1.0)*to_mulx);
    to_xmax   = FLOAT_TO_INT((msg->to.xmax+1.0)*to_mulx);
    to_ymin   = FLOAT_TO_INT((msg->to.ymin+1.0)*to_muly);
    to_ymax   = FLOAT_TO_INT((msg->to.ymax+1.0)*to_muly);
    from_data = msg->src->Data;
    from_w    = msg->src->Width;
    w3d_MaskStretchBlit(wdd, w3d, from_data, from_w, msg->mask->Data, msg->mask_key,
                        from_xmin, from_ymin, from_xmax, from_ymax,
                        to_xmin, to_ymin, to_xmax, to_ymax);
    LEFT("w3d_RASTM_MaskBlit");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_RASTM_ClippedBlit, struct rast_blit *msg)
/*
**  CHANGED
**      02-Apr-97   floh    created
*/
{
    struct raster_data *rd  = INST_DATA(cl->superclass->superclass->superclass,o);
    struct windd_data *wdd  = INST_DATA(cl->superclass,o);
    struct win3d_data *w3d  = INST_DATA(cl,o);
    struct rast_intblit blt;
    FLOAT to_mulx,to_muly,src_mulx,src_muly;

    ENTERED("w3d_RASTM_ClippedBlit");    
    to_mulx   = rd->foff_x;
    to_muly   = rd->foff_y;
    src_mulx  = (FLOAT)(msg->src->Width>>1);
    src_muly  = (FLOAT)(msg->src->Height>>1);
    blt.src       = msg->src;
    blt.from.xmin = FLOAT_TO_INT((msg->from.xmin+1.0)*src_mulx);
    blt.from.xmax = FLOAT_TO_INT((msg->from.xmax+1.0)*src_mulx);
    blt.from.ymin = FLOAT_TO_INT((msg->from.ymin+1.0)*src_muly);
    blt.from.ymax = FLOAT_TO_INT((msg->from.ymax+1.0)*src_muly);
    blt.to.xmin   = FLOAT_TO_INT((msg->to.xmin+1.0)*to_mulx);
    blt.to.xmax   = FLOAT_TO_INT((msg->to.xmax+1.0)*to_mulx);
    blt.to.ymin   = FLOAT_TO_INT((msg->to.ymin+1.0)*to_muly);
    blt.to.ymax   = FLOAT_TO_INT((msg->to.ymax+1.0)*to_muly);
    if (w3d_BltClip(rd,&blt)) {
        w3d_StretchBlit(wdd,w3d,blt.src->Data,blt.src->Width,
                        blt.from.xmin, blt.from.ymin, blt.from.xmax, blt.from.ymax,
                        blt.to.xmin, blt.to.ymin, blt.to.xmax, blt.to.ymax);
    };
    LEFT("w3d_RASTM_ClippedBlit");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_MixPalette, struct disp_mixpal_msg *msg)
/*
**  CHANGED
**      04-Apr-97   floh    created
*/
{
    struct win3d_data *w3d = INST_DATA(cl,o);
    ENTERED("w3d_DISPM_MixPalette");
    w3d_MixPalette(w3d,msg->num,msg->slot,msg->weight);
    _supermethoda(cl,o,DISPM_MixPalette,msg);
    LEFT("w3d_DISPM_MixPalette");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, w3d_DISPM_ScreenShot, struct disp_screenshot_msg *msg)
/*
**  CHANGED
**      18-Jun-97   floh    created
*/
{
    struct win3d_data *w3d = INST_DATA(cl,o);
    struct windd_data *wdd = INST_DATA(cl->superclass,o);
    void *fp;
    UBYTE fname[128];

    strcpy(fname,msg->filename);
    strcat(fname,".ppm");

    /*** öffne Filestream ***/
    if (fp = _FOpen(fname,"wb")) {
        _methoda(o,RASTM_Begin2D,NULL);
        w3d_FrameSnap(wdd,w3d,fp);
        _methoda(o,RASTM_End2D,NULL);
        _FClose(fp);
    };
}

