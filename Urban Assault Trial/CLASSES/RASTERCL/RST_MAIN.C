/*
**  $Source: PRG:VFM/Classes/_RasterClass/rst_main.c,v $
**  $Revision: 38.8 $
**  $Date: 1998/01/06 14:57:18 $
**  $Locker:  $
**  $Author: floh $
**
**  raster.class Main-Modul.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "bitmap/rasterclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(Object *, rst_OM_NEW, struct TagItem *);
_dispatcher(BOOL, rst_OM_DISPOSE, void *);
_dispatcher(void, rst_OM_SET, struct TagItem *);
_dispatcher(void, rst_OM_GET, struct TagItem *);

_dispatcher(void, rst_RASTM_Clear, void *);
_dispatcher(void, rst_RASTM_Copy, struct rast_copy_msg *);
_dispatcher(void, rst_RASTM_IntBlit, struct rast_intblit *);
_dispatcher(void, rst_RASTM_Blit, struct rast_blit *);
_dispatcher(void, rst_RASTM_IntClippedBlit, struct rast_intblit *);
_dispatcher(void, rst_RASTM_ClippedBlit, struct rast_blit *);
_dispatcher(void, rst_RASTM_IntMaskBlit, struct rast_intmaskblit *);
_dispatcher(void, rst_RASTM_MaskBlit, struct rast_maskblit *);

_dispatcher(void, rst_RASTM_Poly, struct rast_poly *);
_dispatcher(void, rst_RASTM_Begin3D, void *);
_dispatcher(void, rst_RASTM_End3D, void *);
_dispatcher(void, rst_RASTM_Begin2D, void *);
_dispatcher(void, rst_RASTM_End2D, void *);

_dispatcher(void, rst_RASTM_Line, struct rast_line *);
_dispatcher(void, rst_RASTM_ClippedLine, struct rast_line *);
_dispatcher(void, rst_RASTM_ClipRegion, struct rast_rect *);
_dispatcher(void, rst_RASTM_IntLine, struct rast_intline *);
_dispatcher(void, rst_RASTM_IntClippedLine, struct rast_intline *);
_dispatcher(void, rst_RASTM_ClipRegion, struct rast_rect *);
_dispatcher(void, rst_RASTM_IntClipRegion, struct rast_rect *);
_dispatcher(void, rst_RASTM_InvClipRegion, struct rast_rect *);
_dispatcher(void, rst_RASTM_IntInvClipRegion, struct rast_rect *);

_dispatcher(void, rst_RASTM_SetFont, struct rast_font *);
_dispatcher(void, rst_RASTM_GetFont, struct rast_font *);
_dispatcher(void, rst_RASTM_Text, struct rast_text *);

_dispatcher(void, rst_RASTM_SetFog, struct rast_fog *);
_dispatcher(void, rst_RASTM_SetPens, struct rast_pens *);

/*-----------------------------------------------------------------*/
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_lnn(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_lnc(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_lgn(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_lgc(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_znn(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_znc(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_zgn(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_zgc(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_lnf(DRAWSPAN_ARGS);
extern DRAWSPAN_PREFIX DRAWSPAN_CALL span_nnn(DRAWSPAN_ARGS);

/*-------------------------------------------------------------------
**  Class Header
*/
_use_nucleus

#ifdef AMIGA
__far ULONG rst_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG rst_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo rst_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeRasterClass(ULONG id,...);
__geta4 BOOL FreeRasterClass(void);
#else
struct ClassInfo *MakeRasterClass(ULONG id,...);
BOOL FreeRasterClass(void);
#endif

struct GET_Class rst_GET = {
    &MakeRasterClass,   /* MakeExternClass() */
    &FreeRasterClass,   /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&rst_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *rst_Entry(void)
{
    return(&rst_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo rst_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:raster.class" /* der Segment-Name */
    },
    rst_Entry,                  /* Entry()-Adresse */
};
#endif

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
/*-------------------------------------------------------------------
**  Logischerweise kann der NUC_GET-Pointer nicht mit einem
**  _GetTagData() aus dem Nucleus-Kernel ermittelt werden,
**  weil NUC_GET noch nicht initialisiert ist! Deshalb hier eine
**  handgestrickte Routine.
*/
struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)
{
    register ULONG act_tag;

    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {
        switch (act_tag) {
            case TAG_DONE:  return(NULL); break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;
            case TAG_SKIP:  tagList += tagList->ti_Data; break;
            default:        tagList++;
        };
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeRasterClass(ULONG id,...)
#else
struct ClassInfo *MakeRasterClass(ULONG id,...)
#endif
/*
**  CHANGES
**      30-May-96   floh    created
**      05-Nov-97   floh    + RASTM_MaskBlit, RASTM_IntMaskBlit
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    memset(rst_Methods,0,sizeof(rst_Methods));

    rst_Methods[OM_NEW]             = (ULONG) rst_OM_NEW;
    rst_Methods[OM_DISPOSE]         = (ULONG) rst_OM_DISPOSE;
    rst_Methods[OM_SET]             = (ULONG) rst_OM_SET;
    rst_Methods[OM_GET]             = (ULONG) rst_OM_GET;

    rst_Methods[RASTM_Clear]          = (ULONG) rst_RASTM_Clear;
    rst_Methods[RASTM_Copy]           = (ULONG) rst_RASTM_Copy;
    rst_Methods[RASTM_Blit]           = (ULONG) rst_RASTM_Blit;
    rst_Methods[RASTM_IntBlit]        = (ULONG) rst_RASTM_IntBlit;
    rst_Methods[RASTM_ClippedBlit]    = (ULONG) rst_RASTM_ClippedBlit;
    rst_Methods[RASTM_IntClippedBlit] = (ULONG) rst_RASTM_IntClippedBlit;
    rst_Methods[RASTM_Line]           = (ULONG) rst_RASTM_Line;
    rst_Methods[RASTM_IntLine]        = (ULONG) rst_RASTM_IntLine;
    rst_Methods[RASTM_ClippedLine]    = (ULONG) rst_RASTM_ClippedLine;
    rst_Methods[RASTM_IntClippedLine] = (ULONG) rst_RASTM_IntClippedLine;
    rst_Methods[RASTM_Poly]           = (ULONG) rst_RASTM_Poly;
    rst_Methods[RASTM_ClipRegion]     = (ULONG) rst_RASTM_ClipRegion;
    rst_Methods[RASTM_IntClipRegion]  = (ULONG) rst_RASTM_IntClipRegion;
    rst_Methods[RASTM_SetFont]        = (ULONG) rst_RASTM_SetFont;
    rst_Methods[RASTM_GetFont]        = (ULONG) rst_RASTM_GetFont;
    rst_Methods[RASTM_Text]           = (ULONG) rst_RASTM_Text;
    rst_Methods[RASTM_SetFog]         = (ULONG) rst_RASTM_SetFog;
    rst_Methods[RASTM_Begin3D]        = (ULONG) rst_RASTM_Begin3D;
    rst_Methods[RASTM_End3D]          = (ULONG) rst_RASTM_End3D;
    rst_Methods[RASTM_Begin2D]        = (ULONG) rst_RASTM_Begin2D;
    rst_Methods[RASTM_End2D]          = (ULONG) rst_RASTM_End2D;
    rst_Methods[RASTM_SetPens]        = (ULONG) rst_RASTM_SetPens;
    rst_Methods[RASTM_IntMaskBlit]    = (ULONG) rst_RASTM_IntMaskBlit;
    rst_Methods[RASTM_MaskBlit]       = (ULONG) rst_RASTM_MaskBlit;
    rst_Methods[RASTM_InvClipRegion]  = (ULONG) rst_RASTM_InvClipRegion;
    rst_Methods[RASTM_IntInvClipRegion] = (ULONG) rst_RASTM_IntInvClipRegion;

    rst_clinfo.superclassid = BITMAP_CLASSID;
    rst_clinfo.methods      = rst_Methods;
    rst_clinfo.instsize     = sizeof(struct raster_data);
    rst_clinfo.flags        = 0;

    /* und das war's */
    return(&rst_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeRasterClass(void)
#else
BOOL FreeRasterClass(void)
#endif
/*
**  CHANGED
**      30-May-96   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  SUPPORT                                                        **
**=================================================================*/

/*-----------------------------------------------------------------*/
void rst_ISAttrs(struct raster_data *rd, struct TagItem *ti)
/*
**  FUNCTION
**      Handelt (I) und (S) Attribute ab.
**
**  CHANGED
**      02-Jun-96   floh    created
**      31-Jul-96   floh    RASTA_FGAPen
*/
{
    ULONG tag;

    /* Attribut-Liste scannen */
    while ((tag = ti->ti_Tag) != TAG_DONE) {

        register ULONG data = ti++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) data; break;
            case TAG_SKIP:      ti += data; break;
            default:

                switch(tag) {
                    case RASTA_FGPen:   rd->fg_pen=data; break;
                    case RASTA_FGAPen:  rd->fg_apen=data; break;
                    case RASTA_BGPen:   rd->bg_pen=data; break;
                    case RASTA_ShadeLUM:
                        if (rd->shade_bmp  = (struct VFMBitmap *) data) {
                            rd->shade_body = rd->shade_bmp->Data;
                        };
                        break;

                    case RASTA_TracyLUM:
                        if (rd->tracy_bmp  = (struct VFMBitmap *) data) {
                            rd->tracy_body = rd->tracy_bmp->Data;
                        };
                        break;
                };
        };
    };
}

/*=================================================================**
**                                                                 **
**      METHODEN HANDLER                                           **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, rst_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      30-May-96   floh    created
**      31-May-96   floh    + Ausfüllen der drawspan_lut[]
**                          - Edge-Table gekillt
**      01-Jun-96   floh    + Clusterstack-Initialisierung
**      02-Jun-96   floh    + Aargh, hatte vergessen, die (I) Attribute
**                            zu initialisieren, das passiert jetzt
**                            per rst_ISAttrs().
**      04-Jun-96   floh    + ClipRect wird auf Größe der Bitmap
**                            initialisiert.
**      06-Jun-96   floh    + Edge-Tables werden allokiert
**                          + Spangine-Initialisierung
**      12-Jun-96   floh    + AAAAAAAAAARGGGGHHHH!!!
**                            Weil ich genau 1 Cluster-Element zuwenig
**                            allokiert habe, manifestierte sich ein
**                            SEHR ÜBLER BUG. Jetzt genug Platz für
**                            minimale Cluster-Größe == 8 Pixel (+1)
*/
{
    Object *newo;
    struct raster_data *rd;
    BOOL all_ok;

    /*** Bitmap-Object erzeugen ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);
    rd = INST_DATA(cl,newo);

    all_ok = FALSE;

    /*** Pointer auf eingebettete VFMBitmap als Malpapier ***/
    _get(newo, RSA_Handle, &(rd->r));
    if (rd->r) {

        /*** ClusterStack allokieren ***/
        rd->cluster_stack = (struct rast_cluster *)
            _AllocVec(((rd->r->Width >> 3)+1) * sizeof(struct rast_cluster),
            MEMF_PUBLIC|MEMF_CLEAR);

        /*** Edge-Tables allokieren ***/
        rd->left_edge = (struct rast_ppoint *)
            _AllocVec(rd->r->Height * sizeof(struct rast_ppoint),
            MEMF_PUBLIC|MEMF_CLEAR);
        rd->right_edge = (struct rast_ppoint *)
            _AllocVec(rd->r->Height * sizeof(struct rast_ppoint),
            MEMF_PUBLIC|MEMF_CLEAR);

        if (rd->cluster_stack &&
            rd->left_edge &&
            rd->right_edge)
        {
            /*** alle Resourcen konnten initialisiert werden ***/
            all_ok = TRUE;
        };
    };
    if (!all_ok) {
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };

    /*** Spangine initialisieren ***/
    if (!rst_seInitSpangine(rd,16*rd->r->Height,rd->r->Height)) {
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };
    rst_seNewFrame(rd,rd->r->Height);

    /*** diverse Variablen initialisieren ***/
    rd->clip.xmin = 0;
    rd->clip.ymin = 0;
    rd->clip.xmax = rd->r->Width-1;
    rd->clip.ymax = rd->r->Height-1;

    rd->ioff_x = rd->r->Width>>1;
    rd->ioff_y = rd->r->Height>>1;
    rd->foff_x = (FLOAT) (rd->r->Width>>1);
    rd->foff_y = (FLOAT) (rd->r->Height>>1);

    rd->fog.enable = FALSE;
    rd->fog.start  = 0.0;
    rd->fog.end    = 0.0;
    rd->fog.r      = 0.0;
    rd->fog.g      = 0.0;
    rd->fog.b      = 0.0;

    /*** unterstützte Spandrawer in die Lookup-Table eintragen ***/
    rd->drawspan_lut[0] = span_nnn;

    rd->drawspan_lut[RPF_LinMap]                             = span_lnn;
    rd->drawspan_lut[RPF_LinMap+RPF_ZeroTracy]               = span_lnc;
    rd->drawspan_lut[RPF_LinMap+RPF_GradShade]               = span_lgn;
    rd->drawspan_lut[RPF_LinMap+RPF_GradShade+RPF_ZeroTracy] = span_lgc;

    rd->drawspan_lut[RPF_PerspMap]                             = span_znn;
    rd->drawspan_lut[RPF_PerspMap+RPF_ZeroTracy]               = span_znc;
    rd->drawspan_lut[RPF_PerspMap+RPF_GradShade]               = span_zgn;
    rd->drawspan_lut[RPF_PerspMap+RPF_GradShade+RPF_ZeroTracy] = span_zgc;

    rd->drawspan_lut[RPF_LinMap+RPF_LUMTracy] = span_lnf;

    /*** (I) Attribute setzen ***/
    rst_ISAttrs(rd,attrs);

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, rst_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      30-May-96   floh    created
**      31-May-96   floh    - Edge-Table gekillt
**      01-Jun-96   floh    + cluster_stack
**      06-Jun-96   floh    + Edge-Tables
**      07-Jun-96   floh    + Spangine-Support
*/
{
    struct raster_data *rd = INST_DATA(cl,o);

    rst_seKillSpangine(rd);
    if (rd->cluster_stack) _FreeVec(rd->cluster_stack);
    if (rd->left_edge)     _FreeVec(rd->left_edge);
    if (rd->right_edge)    _FreeVec(rd->right_edge);

    /*** Fuck Off ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      30-May-96   floh    created
**      02-Jun-96   floh    + ruft jetzt Routine rst_ISAttrs(),
**                            welche (I) und (S) Attribute abhandelt
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rst_ISAttrs(rd,attrs);
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      30-May-96   floh    created
**      31-Jul-97   floh    + RASTA_FGAPen
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
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
                    case RASTA_FGPen:    *value = rd->fg_pen; break;
                    case RASTA_FGAPen:   *value = rd->fg_apen; break;
                    case RASTA_BGPen:    *value = rd->bg_pen; break;
                    case RASTA_ShadeLUM: *value = (ULONG) rd->shade_bmp; break;
                    case RASTA_TracyLUM: *value = (ULONG) rd->tracy_bmp; break;
                };
        };
    };
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_SetFog, struct rast_fog *msg)
/*
**  CHANGED
**      24-Mar-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    rd->fog = *msg;
}

/*-----------------------------------------------------------------*/
_dispatcher(void, rst_RASTM_SetPens, struct rast_pens *msg)
/*
**  CHANGED
**      31-Jul-97   floh    created
*/
{
    struct raster_data *rd = INST_DATA(cl,o);
    if (msg->fg_pen  != -1) rd->fg_pen  = msg->fg_pen;
    if (msg->fg_apen != -1) rd->fg_apen = msg->fg_apen;
    if (msg->bg_pen  != -1) rd->bg_pen  = msg->bg_pen;
}

