/*

**  $Source: PRG:VFM/Engines/GfxEngine/gfx_main.c,v $

**  $Revision: 38.8 $

**  $Date: 1998/01/06 16:39:04 $

**  $Locker: floh $

**  $Author: floh $

**

**  Universelle Gfx-Engine mit Display-Treiber-Object.

**

**  (C) Copyright 1996 by A.Weissflog

*/

/*** Amiga Stuff ***/

#include <exec/types.h>

#include <exec/memory.h>

#include <utility/tagitem.h>



#include <stdlib.h>

#include <string.h>

#include <math.h>



/*** VFM Includes ***/

#include "nucleus/nucleus2.h"

#include "engine/engine.h"

#include "modules.h"

#include "visualstuff/ov_engine.h"

#include "polygon/polygonflags.h"

#include "bitmap/displayclass.h"



/*-----------------------------------------------------------------

**  interne Prototypes

*/

BOOL ove_Open(ULONG id,...);

void ove_Close(void);

void ove_SetAttrs(ULONG tags,...);

void ove_GetAttrs(ULONG tags,...);



/*-------------------------------------------------------------------

**  Global Entry Tables

*/

struct GET_Engine ove_OVE_GET = {

    ove_Open,

    ove_Close,

    ove_SetAttrs,

    ove_GetAttrs,

};



#ifdef DYNAMIC_LINKING

struct ove_GET_Specific ove_GET_SPEC = {

    NULL,

    NULL,

    NULL,

    ove_ToggleDisplays,

    ove_ClearDisplay,

    ove_LoadColorMap,

    NULL,

    NULL,

    NULL,

    ove_DrawPolygon,

    ove_SetFont,

    ove_DrawText,

    ove_TextLen,

    ove_GetFont,

    ove_FlushGfx,

    ove_OccupyRegion,

    ove_DrawLine,

    ove_DrawClippedLine,

};

#endif



/*-----------------------------------------------------------------*/

_use_nucleus



Object *DisplayObject = NULL;

UBYTE DisplayDriver[256];           // Klassenname



/*** Config-Item-Handling ***/

UBYTE PaletteName[CONFIG_MAX_STRING_LEN];

UBYTE DisplayName[CONFIG_MAX_STRING_LEN];

UBYTE Display2Name[CONFIG_MAX_STRING_LEN];



struct ConfigItem ove_ConfigItems[] = {

    { OVECONF_Mode, CONFIG_INTEGER, 0 },    /* DisplayID */

    { OVECONF_XRes, CONFIG_INTEGER, 0 },  /* X-Auflösung */

    { OVECONF_YRes, CONFIG_INTEGER, 0 },  /* Y-Auflösung */

    { OVECONF_ColorMap, CONFIG_STRING, (ULONG) PaletteName },

    { OVECONF_Display,  CONFIG_STRING, (ULONG) DisplayName },

    { OVECONF_Display2, CONFIG_STRING, (ULONG) Display2Name },

};



#define OVE_NUM_CONFIG_ITEMS (6)



/*-------------------------------------------------------------------

**  *** CODE SEGMENT HEADER ***

*/

#ifdef DYNAMIC_LINKING

#ifdef AMIGA

__geta4 struct GET_Engine *start(void)

{

    return(&ove_OVE_GET);

}

#endif

#endif



#ifdef STATIC_LINKING

struct GET_Engine *ove_entry(void)

{

    return(&ove_OVE_GET);

}



struct SegInfo ove_engine_seg = {

    { NULL, NULL,

      0, 0,

      "MC2engines:gfx.engine",

    },

    ove_entry,

};

#endif



/*-----------------------------------------------------------------*/

#ifdef DYNAMIC_LINKING

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

#endif



/*-----------------------------------------------------------------*/

BOOL ove_CreateDisplayObject(UBYTE *prim_class, UBYTE *sec_class,

                             ULONG display_id)

/*

**  FUNCTION

**      Erzeugt Display-Objekt, probiert dabei zwei Klassen

**      aus.

**

**  CHANGED

**      13-Mar-98   floh    created

*/

{

    Object *o;



    /*** Display-Object erzeugen ***/

    if (prim_class[0]) {

        strcpy(DisplayDriver,"drivers/gfx/");

        strcat(DisplayDriver,prim_class);

        DisplayObject = _new(DisplayDriver,

                             RSA_Name,   "display",

                             RSA_Access, ACCESS_EXCLUSIVE,

                             DISPA_DisplayID, display_id,

                             TAG_DONE);



        /*** falls primäres Display schiefging, sekundäres versuchen ***/

        if ((!DisplayObject) && (sec_class[0])) {

            strcpy(DisplayDriver,"drivers/gfx/");

            strcat(DisplayDriver,sec_class);

            DisplayObject = _new(DisplayDriver,

                                 RSA_Name,   "display",

                                 RSA_Access, ACCESS_EXCLUSIVE,

                                 DISPA_DisplayID, display_id,

                                 TAG_DONE);

        };

        if (!DisplayObject) {

            _LogMsg("gfx.engine: display driver init failed!\n");

        };

    } else {

        _LogMsg("gfx.engine: no display driver name given!\n");

    };

    if (DisplayObject) return(TRUE);

    else               return(FALSE);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 BOOL ove_Open(ULONG id,...)

#else

BOOL ove_Open(ULONG id,...)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

**      17-Jun-96   floh    + [width,height] jetzt Default-Höhe

**                            NULL, damit die Display-Treiber-Klassen

**                            selbst ergänzen können.

**      28-Jul-96   floh    + Bugfix: Falscher Rückgabewert, wenn

**                            Display-Init schief ging

**      21-Oct-97   floh    + versucht jetzt ein sekundäres Display

**                            Object zu öffnen, falls das 1.

**                            schiefgeht <gfx.display2>

**      18-Dec-97   floh    + Display-Treiber-Object bekommt keine

**                            IDNode mehr uebergeben!

*/

{

    BOOL retval = FALSE;

    UBYTE *cmap_name;

    UBYTE *dsp_class;

    UBYTE *dsp2_class;

    ULONG width,height;



    #ifdef DYNAMIC_LINKING

    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(FALSE);

    #endif



    memset(PaletteName,0,sizeof(PaletteName));

    memset(DisplayName,0,sizeof(DisplayName));

    memset(Display2Name,0,sizeof(Display2Name));



    /*** Konfiguration lesen ***/

    _GetConfigItems(NULL, ove_ConfigItems, OVE_NUM_CONFIG_ITEMS);

    width      = (UWORD)   ove_ConfigItems[1].data;

    height     = (UWORD)   ove_ConfigItems[2].data;

    cmap_name  = (UBYTE *) ove_ConfigItems[3].data;

    dsp_class  = (UBYTE *) ove_ConfigItems[4].data;

    dsp2_class = (UBYTE *) ove_ConfigItems[5].data;

    if (ove_CreateDisplayObject(dsp_class,dsp2_class,0)) {

        /*** lade Colormap ***/

        ove_LoadColorMap(cmap_name);

        retval = TRUE;

    };

    return(retval);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void ove_Close(void)

#else

void ove_Close(void)

#endif

/*

**  CHANGED

**      06-Jun-96   floh    created

*/

{

    if (DisplayObject) {

        _dispose(DisplayObject);

        DisplayObject = NULL;

    };

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void ove_SetAttrs(ULONG tags,...)

#else

void ove_SetAttrs(ULONG tags,...)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

**      18-Jun-96   floh    OVET_ModeInfo settable

*/

{

    struct TagItem *tlist = (struct TagItem *) &tags;

    struct TagItem *ti;

    ULONG map;



    if (ti = _FindTagItem(OVET_ToFront, tlist)) {

        _methoda(DisplayObject, DISPM_Show, NULL);

    };



    if (ti = _FindTagItem(OVET_ToBack, tlist)) {

        _methoda(DisplayObject, DISPM_Hide, NULL);

    };



    map = _GetTagData(OVET_TracyRemap, NULL, tlist);

    if (map) _set(DisplayObject, RASTA_TracyLUM, map);



    map = _GetTagData(OVET_ShadeRemap, NULL, tlist);

    if (map) _set(DisplayObject, RASTA_ShadeLUM, map);



    /*** neuer DisplayMode ***/

    if (ti = _FindTagItem(OVET_ModeInfo, tlist)) {



        UBYTE cm_buf[256*3];

        UBYTE *cm;



        /*** Colormap retten ***/

        _get(DisplayObject, BMA_ColorMap, &cm);

        if (cm) memcpy(cm_buf, cm, sizeof(cm_buf));

        _methoda(DisplayObject,DISPM_End,NULL);

        _dispose(DisplayObject);



        /*** erzeuge Display-Object ***/

        if (ove_CreateDisplayObject(DisplayName,Display2Name,ti->ti_Data)) {

            /*** Colormap restaurieren ***/

            _methoda(DisplayObject,DISPM_Begin,NULL);

            _set(DisplayObject, BMA_ColorMap, cm);

        };

    };



    /* Ende */

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void ove_GetAttrs(ULONG tags,...)

#else

void ove_GetAttrs(ULONG tags,...)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

**      17-Jun-96   floh    + OVET_Object

**      20-Jun-96   floh    + OVET_Palette Handling

*/

{

    ULONG *value;

    struct TagItem *tlist = (struct TagItem *) &tags;



    #ifdef AMIGA

    value = (ULONG *) _GetTagData(OVET_GET_SPEC, NULL, tlist);

    if (value) *value = (ULONG) &ove_GET_SPEC;

    #endif



    value = (ULONG *) _GetTagData(OVET_XRes, NULL, tlist);

    if (value) _get(DisplayObject, BMA_Width, value);



    value = (ULONG *) _GetTagData(OVET_YRes, NULL, tlist);

    if (value) _get(DisplayObject, BMA_Height, value);



    value = (ULONG *) _GetTagData(OVET_ModeInfo, NULL, tlist);

    if (value) _get(DisplayObject, DISPA_DisplayHandle, value);



    value = (ULONG *) _GetTagData(OVET_Object, NULL, tlist);

    if (value) *value = (ULONG) DisplayObject;



    value = (ULONG *) _GetTagData(OVET_Palette, NULL, tlist);

    if (value) _get(DisplayObject, BMA_ColorMap, value);



    value = (ULONG *) _GetTagData(OVET_Display, NULL, tlist);

    if (value) {

        struct VFMBitmap *bmp;

        _get(DisplayObject, BMA_Bitmap, &bmp);

        *value = (ULONG) bmp->Data;

    };



    /* Ende */

}



/*===================================================================

**  COLORMAP HANDLING

**=================================================================*/



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 BOOL __asm ove_LoadColorMap(__a0 UBYTE *name)

#else

BOOL ove_LoadColorMap(UBYTE *name)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    Object *cmo;

    cmo = _new("ilbm.class",

               RSA_Name, name,

               BMA_HasColorMap, TRUE,

               TAG_DONE);

    if (cmo) {

        UBYTE *cm;

        _get(cmo, BMA_ColorMap, &cm);

        _set(DisplayObject, BMA_ColorMap, cm);

        _dispose(cmo);

        return(TRUE);

    };

    return(FALSE);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_ToggleDisplays(void)

#else

void ove_ToggleDisplays(void)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    _methoda(DisplayObject, DISPM_End, NULL);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_ClearDisplay(__d0 ULONG color)

#else

void ove_ClearDisplay(ULONG color)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    _set(DisplayObject, RASTA_BGPen, color);

    _methoda(DisplayObject, RASTM_Clear, NULL);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_DrawPolygon(__a0 struct rast_poly *poly)

#else

void ove_DrawPolygon(struct rast_poly *poly)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created (provisorisch!)

**      11-Jun-96   floh    umgebogen auf <struct rast_poly *>

*/

{

    _methoda(DisplayObject, RASTM_Poly, poly);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_SetFont(__a0 struct VFMFont *font, __d0 UBYTE id)

#else

void ove_SetFont(struct VFMFont *font, UBYTE id)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    struct rast_font rf;

    rf.font = font;

    rf.id   = id;

    _methoda(DisplayObject, RASTM_SetFont, (Msg *)&rf);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 __asm struct VFMFont *ove_GetFont(__d0 UBYTE id)

#else

struct VFMFont *ove_GetFont(UBYTE id)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    struct rast_font rf;

    rf.font = NULL;

    rf.id   = id;

    _methoda(DisplayObject, RASTM_GetFont, (Msg *)&rf);

    return(rf.font);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 ULONG __asm ove_TextLen(__d0 UBYTE id, __a0 UBYTE *str, __d1 ULONG cnt)

#else

ULONG ove_TextLen(UBYTE id, UBYTE *str, ULONG cnt)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{ return(0); }



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_DrawText(__a0 struct drawtext_args *args)

#else

void ove_DrawText(struct drawtext_args *args)

#endif

/*

**  CHANGED

**      06-Jun-96   floh    created

*/

{

    struct rast_text rt;

    rt.string = args->string;

    rt.clips  = args->clips;

    _methoda(DisplayObject, RASTM_Text, (Msg *)&rt);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_DrawLine(__d0 LONG x0, __d1 LONG y0,

                                __d2 LONG x1, __d3 LONG y1,

                                __d4 UBYTE color)

#else

void ove_DrawLine(LONG x0, LONG y0, LONG x1, LONG y1, UBYTE color)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    struct rast_intline ril;

    ril.x0 = x0;

    ril.y0 = y0;

    ril.x1 = x1;

    ril.y1 = y1;

    _set(DisplayObject, RASTA_FGPen, color);

    _methoda(DisplayObject, RASTM_IntLine, (Msg *)&ril);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_DrawClippedLine(__d0 LONG x0, __d1 LONG y0,

                                       __d2 LONG x1, __d3 LONG y1,

                                       __d4 UBYTE color,

                                       __a0 struct VFMRect *clip)

#else

void ove_DrawClippedLine(LONG x0, LONG y0, LONG x1, LONG y1,

                         UBYTE color, struct VFMRect *clip)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    struct rast_intline ril;

    struct rast_intrect rir;

    ril.x0 = x0;

    ril.y0 = y0;

    ril.x1 = x1;

    ril.y1 = y1;

    rir.xmin = clip->xmin;

    rir.ymin = clip->ymin;

    rir.xmax = clip->xmax;

    rir.ymax = clip->ymax;

    _set(DisplayObject, RASTA_FGPen, color);

    _methoda(DisplayObject, RASTM_IntClipRegion, (Msg *)&rir);

    _methoda(DisplayObject, RASTM_IntClippedLine, (Msg *)&ril);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_FlushGfx(void)

#else

void ove_FlushGfx(void)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{

    /*** OBSOLETE ***/

    // _methoda(DisplayObject, RASTM_End3D, NULL);

}



/*-----------------------------------------------------------------*/

#ifdef AMIGA

__geta4 void __asm ove_OccupyRegion(__d0 WORD x, __d1 WORD y, __d2 WORD dx, __d3 WORD dy)

#else

void ove_OccupyRegion(WORD x, WORD y, WORD dx, WORD dy)

#endif

/*

**  CHANGED

**      08-Jun-96   floh    created

*/

{ }



