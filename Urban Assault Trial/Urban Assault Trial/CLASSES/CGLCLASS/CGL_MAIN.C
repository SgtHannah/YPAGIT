/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_main.c,v $
**  $Revision: 38.2 $
**  $Date: 1997/02/26 17:21:39 $
**  $Locker: floh $
**  $Author: floh $
**
**  Display-Treiber-Klasse für "Creative Graphics Library (TM)"
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <i86.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "bitmap/cglclass.h"

struct NucleusBase *cgl_NBase;

/*-----------------------------------------------------------------*/
_dispatcher(Object *, cgl_OM_NEW, struct TagItem *);
_dispatcher(ULONG, cgl_OM_DISPOSE, void *);
_dispatcher(void, cgl_OM_GET, struct TagItem *);

/*** cgl_mouse.c ***/
void cgl_MCallBack(struct cgl_data *, LONG, LONG);

/*** cgl_disp.c ***/
_dispatcher(ULONG, cgl_DISPM_Query, struct disp_query_msg *);
_dispatcher(void, cgl_DISPM_Begin, void *);
_dispatcher(void, cgl_DISPM_End, void *);
_dispatcher(void, cgl_DISPM_Show, void *);
_dispatcher(void, cgl_DISPM_Hide, void *);
_dispatcher(void, cgl_DISPM_MixPalette, struct disp_mixpal_msg *);
_dispatcher(void, cgl_DISPM_SetPalette, struct disp_setpal_msg *);
_dispatcher(void, cgl_DISPM_SetPointer, struct disp_pointer_msg *);
_dispatcher(ULONG, cgl_DISPM_ObtainTexture, struct disp_texture *);
_dispatcher(void, cgl_DISPM_ReleaseTexture, struct disp_texture *);
_dispatcher(void, cgl_DISPM_MangleTexture, struct disp_texture *);
_dispatcher(ULONG, cgl_DISPM_LockTexture, struct disp_texture *);
_dispatcher(void, cgl_DISPM_UnlockTexture, struct disp_texture *);

/*** cgl_poly.c ***/
ULONG cgl_InitPolyEngine(struct cgl_data *);
void cgl_KillPolyEngine(struct cgl_data *);
_dispatcher(void, cgl_RASTM_Poly, struct rast_poly *);

/*** cgl_rast.c ***/
_dispatcher(void, cgl_RASTM_Clear, void *);
_dispatcher(void, cgl_RASTM_End3D, void *);
_dispatcher(void, cgl_RASTM_Line, struct rast_line *);
_dispatcher(void, cgl_RASTM_ClippedLine, struct rast_line *);
_dispatcher(void, cgl_RASTM_IntLine, struct rast_intline *);
_dispatcher(void, cgl_RASTM_IntClippedLine, struct rast_intline *);
_dispatcher(void, cgl_RASTM_ClipRegion, struct rast_rect *);
_dispatcher(void, cgl_RASTM_IntClipRegion, struct rast_intrect *);
_dispatcher(void, cgl_RASTM_IntBlit, struct rast_intblit *);
_dispatcher(void, cgl_RASTM_Blit, struct rast_blit *);
_dispatcher(void, cgl_RASTM_IntClippedBlit, struct rast_intblit *);
_dispatcher(void, cgl_RASTM_ClippedBlit, struct rast_blit *);
_dispatcher(void, cgl_RASTM_Copy, struct rast_copy_msg *);

/*** cgl_text.c ***/
_dispatcher(void, cgl_RASTM_Text, struct rast_text *);

/*** cgl_cache.c ***/
BOOL cgl_InitTxtCache(struct cgl_data *, ULONG);
void cgl_KillTxtCache(struct cgl_data *);

/*** GLOBALS ***/
struct MinList cgl_IDList;

#define CGL_NUM_CONFIG_ITEMS (2)
struct ConfigItem cgl_ConfigItems[CGL_NUM_CONFIG_ITEMS] = {
    { CGLCONF_Dither, CONFIG_BOOL, FALSE },
    { CGLCONF_Filter, CONFIG_BOOL, FALSE },
};

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG cgl_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo cgl_clinfo;

/*** alle CGL-ScreenModes als Array ***/
#define CGLCLASS_NUM_SCREENMODES (16)
static ULONG cgl_ScreenModes[CGLCLASS_NUM_SCREENMODES] = {
    CGL_M320x200x70Hz,
    CGL_M320x240x60Hz,
    CGL_M512x384x70Hz,
    CGL_M640x350x70Hz,
    CGL_M640x400x70Hz,
    CGL_M640x480x60Hz,
    CGL_M640x480x72Hz,
    CGL_M640x480x75Hz,
    CGL_M800x600x56Hz,
    CGL_M800x600x60Hz,
    CGL_M800x600x72Hz,
    CGL_M800x600x75Hz,
    CGL_M1024x768x60Hz,
    CGL_M1024x768x75Hz,
    CGL_M1280x1024x75Hz,
    CGL_M1280x1024x57Hz
};

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *cgl_MakeClass(ULONG id,...);
BOOL cgl_FreeClass(void);

struct GET_Class cgl_GET = {
    &cgl_MakeClass,
    &cgl_FreeClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *cgl_Entry(void)
{
    return(&cgl_GET);
};

struct SegInfo cgl_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/gfx/cgl.class"
    },
    cgl_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *cgl_MakeClass(ULONG id,...)
/*
**  CHANGED
**      16-Aug-96   floh    created
*/
{
    ULONG i,j;
    CGL_UINT32 err;

    _get_nbase((struct TagItem *)&id,&cgl_NBase);

    /*** DisplayMode-ID Liste aufbauen ***/
    _NewList((struct List *) &cgl_IDList);

    /*** CGL-Treiber laden ***/
    err = cglLoad("DRIVERS", CGL_BOARD_AUTO, CGL_CDL_FLOAT);
    if (err != CGL_SUCCESS) {
        _LogMsg("cgl.class: Error #%d loading CGL modules.\n",err);
        return(NULL);
    };

    /*** Query-Schleife ***/
    for (j=0; j<CGLCLASS_NUM_SCREENMODES; j++) {

        CGL_INT16 query;
        CGL_SCREEN_ST screen;

        ULONG sm = cgl_ScreenModes[j];

        memset(&screen,0,sizeof(screen));
        screen.wMode = sm;
        screen.wReference = CGL_TOP_LEFT;   // Origin oben links
        screen.wBuffers   = CGL_DOUBLE;     // Double-Buffering
        screen.wOption    = CGL_D16S0;      // 16-Bit-ZBuffer
        screen.wColorFormat = CGL_RGB5551;  // Pixelformat

        query = cglQueryScreen(&screen);
        if (query == CGL_SUCCESS) {

            struct disp_idnode *dnd;
            ULONG w,h;
            UBYTE *sm_str;

            /*** ScreenMode parameter zuordnen ***/
            switch(sm) {
                case CGL_M320x200x70Hz:   w=320; h=200; sm_str="CGL: 320X200X70HZ"; break;
                case CGL_M320x240x60Hz:   w=320; h=240; sm_str="CGL: 320X240X60HZ"; break;
                case CGL_M512x384x70Hz:   w=512; h=384; sm_str="CGL: 512X384X70HZ"; break;
                case CGL_M640x350x70Hz:   w=640; h=350; sm_str="CGL: 640X350X70HZ"; break;
                case CGL_M640x400x70Hz:   w=640; h=400; sm_str="CGL: 640X400X70HZ"; break;
                case CGL_M640x480x60Hz:   w=640; h=480; sm_str="CGL: 640X480X60HZ"; break;
                case CGL_M640x480x72Hz:   w=640; h=480; sm_str="CGL: 640X480X72HZ"; break;
                case CGL_M640x480x75Hz:   w=640; h=480; sm_str="CGL: 640X480X75HZ"; break;
                case CGL_M800x600x56Hz:   w=800; h=600; sm_str="CGL: 800X600X56HZ"; break;
                case CGL_M800x600x60Hz:   w=800; h=600; sm_str="CGL: 800X600X60HZ"; break;
                case CGL_M800x600x72Hz:   w=800; h=600; sm_str="CGL: 800X600X72HZ"; break;
                case CGL_M800x600x75Hz:   w=800; h=600; sm_str="CGL: 800X600X75HZ"; break;
                case CGL_M1024x768x60Hz:  w=1024; h=768; sm_str="CGL: 1024X768X60HZ"; break;
                case CGL_M1024x768x75Hz:  w=1024; h=768; sm_str="CGL: 1024X768X75HZ"; break;
                case CGL_M1280x1024x75Hz: w=1280; h=1024; sm_str="CGL: 1280X1024X75HZ"; break;
                case CGL_M1280x1024x57Hz: w=1280; h=1024; sm_str="CGL: 1280X1024X57HZ"; break;
                default: w=0; h=0; sm_str="CGL: UNKNOWN"; break;
            };

            /*** Display-Node allokieren und initialisieren ***/
            dnd = (struct disp_idnode *)
                  _AllocVec(sizeof(struct disp_idnode),
                  MEMF_PUBLIC|MEMF_CLEAR);
            if (dnd) {
                dnd->id = 0x100 | sm;
                dnd->w  = w;
                dnd->h  = h;
                strcpy(dnd->name,sm_str);
                _AddTail((struct List *)&cgl_IDList,(struct Node *)dnd);
                _LogMsg("cgl.class: 0x%lx -- %s\n",dnd->id,dnd->name);
            };
        };
    };

    /*** Klasse initialisieren ***/
    memset(cgl_Methods, 0, sizeof(cgl_Methods));

    cgl_Methods[OM_NEW]     = (ULONG) cgl_OM_NEW;
    cgl_Methods[OM_DISPOSE] = (ULONG) cgl_OM_DISPOSE;
    cgl_Methods[OM_GET]     = (ULONG) cgl_OM_GET;

    cgl_Methods[RASTM_Poly]           = (ULONG) cgl_RASTM_Poly;
    cgl_Methods[RASTM_Clear]          = (ULONG) cgl_RASTM_Clear;
    cgl_Methods[RASTM_End3D]          = (ULONG) cgl_RASTM_End3D;
    cgl_Methods[RASTM_Text]           = (ULONG) cgl_RASTM_Text;
    cgl_Methods[RASTM_Line]           = (ULONG) cgl_RASTM_Line;
    cgl_Methods[RASTM_ClippedLine]    = (ULONG) cgl_RASTM_ClippedLine;
    cgl_Methods[RASTM_IntLine]        = (ULONG) cgl_RASTM_IntLine;
    cgl_Methods[RASTM_IntClippedLine] = (ULONG) cgl_RASTM_IntClippedLine;
    cgl_Methods[RASTM_ClipRegion]     = (ULONG) cgl_RASTM_ClipRegion;
    cgl_Methods[RASTM_IntClipRegion]  = (ULONG) cgl_RASTM_IntClipRegion;
    cgl_Methods[RASTM_Copy]           = (ULONG) cgl_RASTM_Copy;
    cgl_Methods[RASTM_IntBlit]        = (ULONG) cgl_RASTM_IntBlit;
    cgl_Methods[RASTM_Blit]           = (ULONG) cgl_RASTM_Blit;
    cgl_Methods[RASTM_IntClippedBlit] = (ULONG) cgl_RASTM_IntClippedBlit;
    cgl_Methods[RASTM_ClippedBlit]    = (ULONG) cgl_RASTM_ClippedBlit;

    cgl_Methods[DISPM_Query]      = (ULONG) cgl_DISPM_Query;
    cgl_Methods[DISPM_Begin]      = (ULONG) cgl_DISPM_Begin;
    cgl_Methods[DISPM_End]        = (ULONG) cgl_DISPM_End;
    cgl_Methods[DISPM_Show]       = (ULONG) cgl_DISPM_Show;
    cgl_Methods[DISPM_Hide]       = (ULONG) cgl_DISPM_Hide;
    cgl_Methods[DISPM_MixPalette] = (ULONG) cgl_DISPM_MixPalette;
    cgl_Methods[DISPM_SetPalette] = (ULONG) cgl_DISPM_SetPalette;
    cgl_Methods[DISPM_SetPointer] = (ULONG) cgl_DISPM_SetPointer;
    cgl_Methods[DISPM_ObtainTexture]  = (ULONG) cgl_DISPM_ObtainTexture;
    cgl_Methods[DISPM_MangleTexture]  = (ULONG) cgl_DISPM_MangleTexture;
    cgl_Methods[DISPM_ReleaseTexture] = (ULONG) cgl_DISPM_ReleaseTexture;
    cgl_Methods[DISPM_LockTexture]    = (ULONG) cgl_DISPM_LockTexture;
    cgl_Methods[DISPM_UnlockTexture]  = (ULONG) cgl_DISPM_UnlockTexture;

    cgl_clinfo.superclassid = DISPLAY_CLASSID;
    cgl_clinfo.methods      = cgl_Methods;
    cgl_clinfo.instsize     = sizeof(struct cgl_data);
    cgl_clinfo.flags        = 0;

    return(&cgl_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL cgl_FreeClass(void)
/*
**  CHANGED
**      16-Aug-96   floh    created
**      17-Aug-96   floh    he... Textmodus muß von Hand wieder
**                          eingeschaltet werden...
*/
{
    union REGS regs;

    /*** Display-ID-Liste killen ***/
    struct Node *nd;
    while (nd = _RemHead((struct List *) &cgl_IDList)) _FreeVec(nd);

    /*** CGL aufräumen ***/
    cglCloseScreen();
    cglUnload();

    /*** Textmodus einstellen ***/
    memset(&regs,0,sizeof(regs));
    regs.h.ah = 0x0;
    regs.h.al = 0x3;
    int386(0x10, &regs, &regs);

    /*** Ende ***/
    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/
/*-----------------------------------------------------------------*/
struct disp_idnode *cgl_GetIDNode(ULONG id)
/*
**  FUNCTION
**      Sucht ID-Node der gegebenen Mode-ID in der
**      ID-Liste, und returniert Pointer darauf.
**
**  CHANGED
**      17-Aug-96   floh    created
*/
{
    struct MinNode *nd;
    struct MinList *ls;
    ls = &cgl_IDList;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct disp_idnode *ind = (struct disp_idnode *) nd;
        if (id == ind->id) return(ind);
    };
    return(NULL);
}

/*=================================================================**
**                                                                 **
**  METHODEN HANDLER                                               **
**  ~~~~~~~~~~~~~~~~                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, cgl_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      17-Aug-96   floh    created
**      18-Aug-96   floh    + Texture-Cache-Initialisierung
**                          + Poly-Engine-Initialisierung
**      02-Dec-96   floh    + eigenes Config-Handling
**      04-Dec-96   floh    + Line-Handling
**      26-Feb-97   floh    + Custom-Textur-Handling, trägt sich in
**                            NBase als globales Gfx-Objekt ein.
*/
{
    Object *newo;
    struct cgl_data *cd;
    ULONG id,w,h;
    UBYTE *name;
    struct TagItem ti[8];
    ULONG t = 0;
    struct disp_idnode *idnode = NULL;

    memset(ti,0,sizeof(ti));

    /*** suche Display-ID, Höhe und Breite ist fix ***/
    name = (UBYTE *) _GetTagData(RSA_Name,0,attrs);
    id   = _GetTagData(DISPA_DisplayID,0,attrs);

    /*** Höhe und Breite ergänzen ***/
    if (id != 0) {
        idnode = cgl_GetIDNode(id);
        if (idnode) {

            w = idnode->w;
            h = idnode->h;

            ti[t].ti_Tag  = BMA_Width;
            ti[t].ti_Data = idnode->w;
            t++;
            ti[t].ti_Tag  = BMA_Height;
            ti[t].ti_Data = idnode->h;
            t++;

        }else{
            /*** unbekannte Display-ID ***/
            _LogMsg("cgl.class: Unsupported display id!\n");
            return(NULL);
        };
        /*** ColorMap erzwingen ***/
        ti[t].ti_Tag  = BMA_HasColorMap;
        ti[t].ti_Data = TRUE;
        t++;
        /*** keine VFM-Render-Bitmap! ***/
        ti[t].ti_Tag  = BMA_Body;
        ti[t].ti_Data = 1;
        t++;
    };

    /*** erzeuge Object mit VFMBitmap ***/
    ti[t].ti_Tag  = TAG_MORE;
    ti[t].ti_Data = attrs;
    newo = (Object *) _supermethoda(cl,o,OM_NEW,ti);
    if (!newo) return(NULL);
    cgl_NBase->GfxObject = newo;
    cd = INST_DATA(cl,newo);
    cd->id = id;

    /*** lese zusätzliche Konfiguration ***/
    _GetConfigItems(NULL, cgl_ConfigItems, CGL_NUM_CONFIG_ITEMS);
    if (cgl_ConfigItems[0].data) cd->flags |= CGLF_Dither;
    if (cgl_ConfigItems[1].data) cd->flags |= CGLF_Filter;

    /*** falls Display-ID gegeben, Display initialisieren ***/
    if (idnode) {

        CGL_SCREEN_ST screen;
        CGL_INT16 err;
        ULONG sm = id & ~0x100;
        ULONG cm = CGL_RGB5551;

        memset(&screen,0,sizeof(screen));
        screen.wReference   = CGL_TOP_LEFT;   // Origin oben links
        screen.wBuffers     = CGL_DOUBLE;     // Double-Buffering
        screen.wOption      = CGL_D16S0;      // 16-Bit-ZBuffer
        screen.wMode        = sm;
        screen.wColorFormat = cm;
        err = cglInitScreen(&screen);
        if (CGL_SUCCESS == err) {

            CGL_COLOR_ST bg;

            /*** Success-Flag eintragen ***/
            cd->screen_mode = sm;
            cd->color_model = cm;
            cd->screen_ok   = TRUE;
            cd->x_scale     = (FLOAT) (idnode->w>>1);
            cd->y_scale     = (FLOAT) (idnode->h>>1);

            /*** Clip-Rect initialisieren ***/
            cd->clip.xmin   = -1.0;
            cd->clip.ymin   = -1.0;
            cd->clip.xmax   = +1.0;
            cd->clip.ymax   = +1.0;

            /*** initialisiere Background-Color ***/
            bg.bBlue  = 0;
            bg.bGreen = 0;
            bg.bRed   = 0;
            bg.bAlpha = 0;

            /*** konstante CGL Parameter setzen ***/
            cglSetDepthMode(CGL_LESS);          // Z-Buffer-Modus
            cglSetConstant(&bg,0x7fff,0);       // Buffer-Clear-Konstanten
            cglSetDepthCueColor(&bg);           // DepthFade zu BG-Color
            cglSelectRenderBuffer(CGL_BACK_BUFFER);

            if (cd->flags & CGLF_Dither) cglSetDitherMode(CGL_ENABLE);
            else                         cglSetDitherMode(CGL_DISABLE);

            cd->line.wOperation     = CGL_RENDER;
            cd->line.dwNoOfVertices = 2;
            cd->line.wPrimitiveType = CGL_LINE;
            cd->line.pColor         = &(cd->line_color);
            cd->line.pVertex        = &(cd->line_xy);
            cd->line.uPropertyEnableMask.i = 0;

            /*** Screen löschen ***/
            cglClearScreen();
            cglSwapBuffer();

            /*** soviel freier Texture-Cache-Speicher... ***/
            _LogMsg("cgl.class: %d bytes texture cache.\n",screen.dwUB_BufferSize);

            /*** Textur-Cache-Manager initialisieren ***/
            if (!cgl_InitTxtCache(cd,screen.dwUB_BufferSize)) {
                _LogMsg("cgl.class: init txt cache manager failed!\n");
                _methoda(newo,OM_DISPOSE,NULL);
                return(NULL);
            };

            /*** Polygon-Engine initialisieren ***/
            if (!cgl_InitPolyEngine(cd)) {
                _LogMsg("cgl.class: init poly engine failes!\n");
                _methoda(newo,OM_DISPOSE,NULL);
                return(NULL);
            };

            /*** ColorMap einstellen ***/
            cm = (UBYTE *) _GetTagData(BMA_ColorMap, NULL, attrs);
            if (cm) {
                struct disp_setpal_msg dsm;
                struct disp_mixpal_msg dmm;
                ULONG slot[1],weight[1];

                dsm.slot  = 0;
                dsm.first = 0;
                dsm.num   = 256;
                dsm.pal   = cm;
                _methoda(newo, DISPM_SetPalette, &dsm);

                slot[0]    = 0;
                weight[0]  = 256;
                dmm.num    = 1;
                dmm.slot   = &slot;
                dmm.weight = &weight;
                _methoda(newo, DISPM_MixPalette, &dmm);
            };
        };

        /*** Export-Daten ausfüllen ***/
        cd->export.x_size = idnode->w;
        cd->export.y_size = idnode->h;
        cd->export.data   = cd;
        cd->export.mouse_callback = cgl_MCallBack;
    };

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, cgl_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      17-Aug-96   floh    created
**      18-Aug-96   floh    + Texture-Cache-Manager-Cleanup
**      29-Nov-96   floh    + Textmode wird eingestellt
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    if (cd->screen_ok) {
        cgl_KillPolyEngine(cd);
        cgl_KillTxtCache(cd);
    };
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, cgl_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      17-Aug-96   floh    created
**      28-Nov-96   floh    + DISPA_DisplayHandle jetzt
**                            <struct msdos_DispEnv>
*/
{
    struct cgl_data *cd = INST_DATA(cl,o);
    ULONG tag;
    struct TagItem *ti = attrs;

    /* Attribut-Liste scannen */
    while ((tag = ti->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) ti++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) value; break;
            case TAG_SKIP:      ti += (ULONG) value; break;
            default:

                switch(tag) {
                    case DISPA_DisplayID:
                        *value = cd->id;
                        break;
                    case DISPA_DisplayHandle:
                        *value = (ULONG) &(cd->export);
                        break;
                };
        };
    };
    _supermethoda(cl,o,OM_GET,attrs);
}

