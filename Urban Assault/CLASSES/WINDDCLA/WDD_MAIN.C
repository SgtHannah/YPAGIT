/*
**  $Source: PRG:VFM/Classes/_WinDDClass/wdd_main.c,v $
**  $Revision: 38.6 $
**  $Date: 1998/01/06 15:04:46 $
**  $Locker: floh $
**  $Author: floh $
**
**  Display-Treiber-Klasse für Windows, benutzt DirectDraw.
**
**  *** WICHTIG ***
**
**  Die Klasse benötigt diverse Informationen, die einer
**  Windows-Applikation so mitgegeben werden. Dafür
**  definiert die windd.class eigene Attribute.
**  Das funktioniert aber nicht, wenn die windd.class
**  von der Gfx-Engine erzeugt wurde. Als Kludge
**  existieren für diesen Zweck globale Variablen,
**  die ausgelesen werden, wenn die entsprechenden
**  Attribute nicht initialisiert wurden.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>

#include "nucleus/nucleus2.h"
#include "bitmap/winddclass.h"  // Public Version

extern unsigned long wdd_DoDirect3D;

/*-----------------------------------------------------------------*/
_dispatcher(Object *, wdd_OM_NEW, struct TagItem *);
_dispatcher(ULONG, wdd_OM_DISPOSE, void *);
_dispatcher(void, wdd_OM_GET, struct TagItem *);
_dispatcher(void, wdd_OM_SET, struct TagItem *);

/*** es kann vorkommen, daß sich die Hintergrund-Map nicht locken ***/
/*** ließ, in diesem Fall dürfen keine Raster-Operationen darauf  ***/
/*** "durchkommen"                                                ***/
#define rast_method(m,id,name) _dispatcher(void,m,void *msg) {struct windd_data *wdd=INST_DATA(cl,o); ENTERED(name); if (wdd->r->Data) _supermethoda(cl,o,id,msg); LEFT(name); }

_dispatcher(void, wdd_RASTM_Clear, void *);
_dispatcher(void, wdd_RASTM_Copy, void *);
_dispatcher(void, wdd_RASTM_Rect, void *);
_dispatcher(void, wdd_RASTM_IntRect, void *);
_dispatcher(void, wdd_RASTM_ClippedRect, void *);
_dispatcher(void, wdd_RASTM_IntClippedRect, void *);
_dispatcher(void, wdd_RASTM_Line, void *);
_dispatcher(void, wdd_RASTM_IntLine, void *);
_dispatcher(void, wdd_RASTM_ClippedLine, void *);
_dispatcher(void, wdd_RASTM_IntClippedLine, void *);
_dispatcher(void, wdd_RASTM_Blit, void *);
_dispatcher(void, wdd_RASTM_IntBlit, void *);
_dispatcher(void, wdd_RASTM_MaskBlit, void *);
_dispatcher(void, wdd_RASTM_IntMaskBlit, void *);
_dispatcher(void, wdd_RASTM_ClippedBlit, void *);
_dispatcher(void, wdd_RASTM_IntClippedBlit, void *);
_dispatcher(void, wdd_RASTM_Poly, void *);
_dispatcher(void, wdd_RASTM_Begin2D, void *);
_dispatcher(void, wdd_RASTM_End2D, void *);
_dispatcher(void, wdd_RASTM_Begin3D, void *);
_dispatcher(void, wdd_RASTM_End3D, void *);

_dispatcher(void, wdd_RASTM_Text, struct rast_text *);

_dispatcher(ULONG, wdd_DISPM_Query, struct disp_query_msg *);
_dispatcher(void, wdd_DISPM_Begin, void *);
_dispatcher(void, wdd_DISPM_End, void *);
_dispatcher(void, wdd_DISPM_Show, void *);
_dispatcher(void, wdd_DISPM_Hide, void *);
_dispatcher(void, wdd_DISPM_SetPalette, struct disp_setpal_msg *);
_dispatcher(void, wdd_DISPM_MixPalette, struct disp_mixpal_msg *);
_dispatcher(void, wdd_DISPM_SetPointer, struct disp_pointer_msg *);

_dispatcher(void, wdd_WINDDM_EnableGDI, void *);
_dispatcher(void, wdd_WINDDM_DisableGDI, void *);
_dispatcher(void, wdd_WINDDM_GetText, struct windd_gettext *);
_dispatcher(void, wdd_WINDDM_PlayMovie, struct windd_playmovie *);
_dispatcher(void, wdd_WINDDM_QueryDevice, struct windd_device *);
_dispatcher(void, wdd_WINDDM_SetDevice, struct windd_device *);

/*** WinBox Prototypes ***/
ULONG wdd_DDrawCreate(ULONG);
void wdd_DDrawDestroy(void);
ULONG wdd_InitDDrawStuff(struct windd_data *wdd, UBYTE *cm, ULONG w, ULONG h, ULONG bpp);
void wdd_KillDDrawStuff(struct windd_data *);

void wdd_Begin(struct windd_data *wdd);
void wdd_End(struct windd_data *wdd);
void wdd_SetPalette(struct windd_data *wdd,UBYTE *pal);
void wdd_SetMouseImage(struct windd_data *wdd, unsigned long type, unsigned long flush);

void *wdd_Lock(struct windd_data *wdd);
void wdd_Unlock(struct windd_data *wdd);
void wdd_Clear(struct windd_data *wdd);
void wdd_EnableGDI(struct windd_data *wdd, unsigned long mode);
void wdd_DisableGDI(struct windd_data *wdd, unsigned long mode);
void wdd_WrongDisplayDepth(struct windd_data *wdd);

unsigned long wdd_DoSoftCursor(struct windd_data *wdd);
void wdd_GetMousePos(struct windd_data *, long *, long *);

void wdd_DrawText(struct windd_data *,struct wdd_VFMFont **,unsigned char *,unsigned char **);
char *wdd_GetText(struct windd_data *wdd, char *title_text, char *ok_text, char *cancel_text, char *default_text, long timer_val, void (*timer_func)(void *), void *timer_arg);
void wdd_PlayMovie(struct windd_data *wdd, char *fname);
void wdd_SetCursorMode(struct windd_data *wdd, unsigned long mode);
unsigned long wdd_WriteGUID(char *fname, void *guid, char *alt_text);

void wdd_InitLog(void);
void wdd_Log(char *string,...);

/*-------------------------------------------------------------------
**  Global Data
*/
struct MinList wdd_IDList;
struct MinList wdd_DevList;

#define WINDD_NUM_CONFIG_ITEMS (7)
struct ConfigItem wdd_ConfigItems[WINDD_NUM_CONFIG_ITEMS] = {
    {"gfx.force_soft_cursor",       CONFIG_BOOL, FALSE },
    {"gfx.all_modes",               CONFIG_BOOL, FALSE },
    {"gfx.movie_player",            CONFIG_BOOL, TRUE },
    {"gfx.force_alpha_textures",    CONFIG_BOOL, FALSE },
    {"gfx.use_draw_primitive",      CONFIG_BOOL, FALSE },
    {"gfx.disable_lowres",          CONFIG_BOOL, FALSE },
    {"gfx.export_window_mode",      CONFIG_BOOL, FALSE },
};

extern unsigned long wdd_ForceAlphaTextures;

#define ENV_NAME     ("env/vid.def")
#define ENV_16BITTXT ("env/txt16bit.def")
#define ENV_DPRIM    ("env/drawprim.def")

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG wdd_Methods[NUCLEUS_NUMMETHODS];
struct ClassInfo wdd_clinfo;

struct ClassInfo *wdd_MakeClass(ULONG id,...);
BOOL wdd_FreeClass(void);

struct GET_Class wdd_GET = {
    &wdd_MakeClass,
    &wdd_FreeClass,
};

struct GET_Class *wdd_Entry(void)
{
    return(&wdd_GET);
};

struct SegInfo windd_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/gfx/windd.class"
    },
    wdd_Entry,
};

/*-----------------------------------------------------------------*/
struct ClassInfo *wdd_MakeClass(ULONG id,...)
/*
**  CHANGED
**      07-Nov-96   floh    created
**      05-Nov-97   floh    + RASTM_MaskBlit, RASTM_IntMaskBlit
**      23-Feb-98   floh    + OM_SET
**      10-Mar-98   floh    + DDraw/D3D-Init nach OM_NEW verlegt
**      12-Mar-98   floh    + WINDDM_QueryDevice
*/
{
    ULONG i;

    /*** Klasse initialisieren ***/
    memset(wdd_Methods,0,sizeof(wdd_Methods));

    wdd_Methods[OM_NEW]     = (ULONG) wdd_OM_NEW;
    wdd_Methods[OM_DISPOSE] = (ULONG) wdd_OM_DISPOSE;
    wdd_Methods[OM_GET]     = (ULONG) wdd_OM_GET;
    wdd_Methods[OM_SET]     = (ULONG) wdd_OM_SET;

    wdd_Methods[RASTM_Clear]            = (ULONG) wdd_RASTM_Clear;
    wdd_Methods[RASTM_Copy]             = (ULONG) wdd_RASTM_Copy;
    wdd_Methods[RASTM_Rect]             = (ULONG) wdd_RASTM_Rect;
    wdd_Methods[RASTM_IntRect]          = (ULONG) wdd_RASTM_IntRect;
    wdd_Methods[RASTM_ClippedRect]      = (ULONG) wdd_RASTM_ClippedRect;
    wdd_Methods[RASTM_IntClippedRect]   = (ULONG) wdd_RASTM_IntClippedRect;
    wdd_Methods[RASTM_Line]             = (ULONG) wdd_RASTM_Line;
    wdd_Methods[RASTM_IntLine]          = (ULONG) wdd_RASTM_IntLine;
    wdd_Methods[RASTM_ClippedLine]      = (ULONG) wdd_RASTM_ClippedLine;
    wdd_Methods[RASTM_IntClippedLine]   = (ULONG) wdd_RASTM_IntClippedLine;
    wdd_Methods[RASTM_Blit]             = (ULONG) wdd_RASTM_Blit;
    wdd_Methods[RASTM_IntBlit]          = (ULONG) wdd_RASTM_IntBlit;
    wdd_Methods[RASTM_MaskBlit]         = (ULONG) wdd_RASTM_MaskBlit;
    wdd_Methods[RASTM_IntMaskBlit]      = (ULONG) wdd_RASTM_IntMaskBlit;
    wdd_Methods[RASTM_ClippedBlit]      = (ULONG) wdd_RASTM_ClippedBlit;
    wdd_Methods[RASTM_IntClippedBlit]   = (ULONG) wdd_RASTM_IntClippedBlit;
    wdd_Methods[RASTM_Poly]             = (ULONG) wdd_RASTM_Poly;
    wdd_Methods[RASTM_Text]             = (ULONG) wdd_RASTM_Text;
    wdd_Methods[RASTM_Begin2D]          = (ULONG) wdd_RASTM_Begin2D;
    wdd_Methods[RASTM_End2D]            = (ULONG) wdd_RASTM_End2D;
    wdd_Methods[RASTM_Begin3D]          = (ULONG) wdd_RASTM_Begin3D;
    wdd_Methods[RASTM_End3D]            = (ULONG) wdd_RASTM_End3D;

    wdd_Methods[DISPM_Query] = (ULONG) wdd_DISPM_Query;
    wdd_Methods[DISPM_Begin] = (ULONG) wdd_DISPM_Begin;
    wdd_Methods[DISPM_End]   = (ULONG) wdd_DISPM_End;
    wdd_Methods[DISPM_Show]  = (ULONG) wdd_DISPM_Show;
    wdd_Methods[DISPM_Hide]  = (ULONG) wdd_DISPM_Hide;
    wdd_Methods[DISPM_SetPalette] = (ULONG) wdd_DISPM_SetPalette;
    wdd_Methods[DISPM_MixPalette] = (ULONG) wdd_DISPM_MixPalette;
    wdd_Methods[DISPM_SetPointer] = (ULONG) wdd_DISPM_SetPointer;

    wdd_Methods[WINDDM_EnableGDI]    = (ULONG) wdd_WINDDM_EnableGDI;
    wdd_Methods[WINDDM_DisableGDI]   = (ULONG) wdd_WINDDM_DisableGDI;
    wdd_Methods[WINDDM_GetText]      = (ULONG) wdd_WINDDM_GetText;
    wdd_Methods[WINDDM_PlayMovie]    = (ULONG) wdd_WINDDM_PlayMovie;
    wdd_Methods[WINDDM_QueryDevice]  = (ULONG) wdd_WINDDM_QueryDevice;
    wdd_Methods[WINDDM_SetDevice]    = (ULONG) wdd_WINDDM_SetDevice;

    wdd_clinfo.superclassid = DISPLAY_CLASSID;
    wdd_clinfo.methods      = wdd_Methods;
    wdd_clinfo.instsize     = sizeof(struct windd_data);
    wdd_clinfo.flags        = 0;

    return(&wdd_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL wdd_FreeClass(void)
/*
**  CHANGED
**      07-Nov-96   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
void wdd_AddDisplayMode(ULONG w, ULONG h, ULONG bpp, ULONG flags)
/*
**  FUNCTION
**      Callback-Funktion, wird für jeden Display-Mode
**      einmal aufgerufen.
**
**  CHANGED
**      14-Nov-96   floh    created
**      19-Nov-96   floh    + <windowed>
**      21-Feb-97   floh    + Sortiert jetzt die Auflösungen
**                            nochmal extra in die Liste ein,
**                            um die halbierten Auflösungen als
**                            "echte" Auflösungen erscheinen zu lassen.
**      03-Mar-96   floh    + BitsPerPixel, wird in dnd->data[3]
**                            gehalten.
**      15-Oct-97   floh    + "redundante" Fullscreen-Half-Auflösungen
**                            werden jetzt ignoriert.
**      18-Mar-98   floh    + in Display-ID ist jetzt die Auflösung reincodiert,
**                            damit es keine zufälligen Übereinstimmungen mehr
**                            zwischen verschiedenen Devices geben kann.
**      04-Apr-98   floh    + Display-ID anders kodiert
*/
{
    LONG vis_w,vis_h;
    struct disp_idnode *dnd = NULL;

    if (flags & WINDDF_IsFullScrHalf) {
        vis_w = w>>1;
        vis_h = h>>1;
    } else {
        vis_w = w;
        vis_h = h;
    };

    /*** gibt's schon einen Nicht-Windowed-Modus dieser Auflösung? ***/
    if (!(flags & WINDDF_IsWindowed)) {
        struct MinList *ls;
        struct MinNode *nd;
        ls = &wdd_IDList;
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            struct disp_idnode *act_dnd = (struct disp_idnode *)nd;
            if ((act_dnd->data[1] == vis_w) &&
                (act_dnd->data[2] == vis_h) &&
                (act_dnd->data[3] == bpp))
            {
                /*** dann ignorieren ***/
                return;
            };
        };
    };

    dnd = (struct disp_idnode *)_AllocVec(sizeof(struct disp_idnode),MEMF_PUBLIC|MEMF_CLEAR);
    if (dnd) {
        dnd->id = (w<<12)|h;
        if (flags & WINDDF_IsWindowed)    dnd->id |= (1<<15);
        if (flags & WINDDF_IsFullScrHalf) dnd->id |= (1<<14);
        dnd->w  = w;
        dnd->h  = h;
        dnd->data[0] = flags;
        dnd->data[1] = vis_w;       // wahre Breite
        dnd->data[2] = vis_h;       // wahre Höhe
        dnd->data[3] = bpp;         // Bits Per Pixel
        dnd->name[0] = 0;
        switch(flags & ~WINDDF_IsDirect3D) {
            case (WINDDF_IsWindowed):
                sprintf(dnd->name,"WIN: %d x %d",dnd->w,dnd->h);
                break;

            case (WINDDF_IsWindowed|WINDDF_IsSysMem):
                sprintf(dnd->name,"WIN: %d x %d",dnd->w,dnd->h);
                break;

            case (WINDDF_IsFullScrHalf):
                sprintf(dnd->name,"%d x %d x %d",vis_w,vis_h,bpp);
                break;
                
            default:
                sprintf(dnd->name,"%d x %d x %d",dnd->w,dnd->h,bpp);
                break;
        };

        /*** in Mode-Liste eintragen ***/
        if (flags & WINDDF_IsWindowed) {
            /*** Windowed-Modes ans Ende der Liste ***/
            _AddTail((struct List *)&wdd_IDList,(struct Node *)dnd);
        } else {
            /*** Fullscreen-Modes einsortieren ***/
            struct MinList *ls;
            struct MinNode *nd;
            ls = &wdd_IDList;

            /*** Liste von HINTEN nach VORN durchwandern ***/
            for (nd=ls->mlh_TailPred; nd->mln_Pred; nd=nd->mln_Pred) {

                struct disp_idnode *dnd = (struct disp_idnode *) nd;
                ULONG l_flags = dnd->data[0];
                ULONG l_w     = dnd->data[1];
                ULONG l_h     = dnd->data[2];

                /*** Windowed-Modes ignorieren ***/
                if ((l_flags & WINDDF_IsWindowed) == 0) {
                    if ((vis_w >= l_w) && (vis_h >= l_h)) {
                        /*** _hinter_ diese Node einsortieren! ***/
                        break;
                    };
                };
            };
            /*** hinter <nd> eintragen ***/
            nd->mln_Succ->mln_Pred = dnd;
            dnd->nd.mln_Pred       = nd;
            dnd->nd.mln_Succ       = nd->mln_Succ;
            nd->mln_Succ           = dnd;
        };
    };
}

/*-----------------------------------------------------------------*/
void wdd_AddDevice(char *name, char *guid, unsigned long is_current)
/*
**  FUNCTION
**      Hängt ein 3D-Device an die Device-Liste.
**
**  CHANGED
**      12-Mar-98   floh    created
*/
{
    struct windd_devnode *dn;
    dn = (struct windd_devnode *) _AllocVec(sizeof(struct windd_devnode),MEMF_PUBLIC|MEMF_CLEAR);
    if (dn) {
        strcpy(&(dn->name),name);
        strcpy(&(dn->guid),guid);   // nach String konvertiert!!!
        dn->flags = is_current ? WINDDF_IsCurrentDevice : 0;
        _AddTail((struct List *)&wdd_DevList,(struct Node *)dn);
        wdd_Log("wdd_AddDevice(%s,%s,%s)\n",name,guid,is_current?"is_current":"not_current");
    };
}

/*-----------------------------------------------------------------*/
struct disp_idnode *wdd_GetFallbackIDNode(void)
/*
**  FUNCTION
**      Durchsucht Mode-Liste nach einer 640x480 Aufloesung,
**      oder, falls nicht vorhanden, returniert die 1.Node
**      in der Liste.
**
**  CHANGED
**      18-Dec-97    floh    created
*/
{
    struct MinNode *nd;
    struct MinList *ls;
    ls = &wdd_IDList;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct disp_idnode *ind = (struct disp_idnode *)nd;
        if ((ind->w == 640) && (ind->h == 480)) return(ind);
    };
    /*** nix gefunden... ***/
    return((struct disp_idnode *)ls->mlh_Head);
}

/*-----------------------------------------------------------------*/
struct disp_idnode *wdd_GetIDNode(ULONG id)
/*
**  FUNCTION
**      Sucht ID-Node der gegebenen Mode-ID in
**      der ID-Liste, returniert Pointer darauf.
**
**  CHANGED
**      08-Nov-96   floh    created
*/
{
    struct MinNode *nd;
    struct MinList *ls;

    ls = &wdd_IDList;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct disp_idnode *ind = (struct disp_idnode *) nd;
        if (id == ind->id) return(ind);
    };
    /*** nix gefunden, Fallback-Node returnieren ***/
    return(wdd_GetFallbackIDNode());
}

/*-----------------------------------------------------------------*/
struct disp_idnode *wdd_GetIDNodeByName(UBYTE *name)
/*
**  CHANGED
**      18-Dec-97   floh    created
*/
{
    struct MinNode *nd;
    struct MinList *ls;
    ls = &wdd_IDList;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
        struct disp_idnode *ind = (struct disp_idnode *) nd;
        if (stricmp(name,ind->name)==0) return(ind);
    };
    /*** nix gefunden, Fallback-Node returnieren ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
struct disp_idnode *wdd_GetIDNodeFromFile(UBYTE *fname)
/*
**  CHANGED
**      18-Dec-97   floh    created
**      02-Jun-98   floh    + oops, File wurde nicht geschlossen
*/
{
    APTR fp;
    struct disp_idnode *ind = NULL;
    if (fp = fopen(fname,"r")) {
        UBYTE buf[128];
        if (fgets(buf,sizeof(buf),fp)) {
            UBYTE *dike_out;
            /*** NewLine killen ***/
            if (dike_out = strpbrk(buf, "\n")) *dike_out = 0;
            ind = wdd_GetIDNodeByName(buf);
        };
        fclose(fp);
    };
    if (!ind) ind=wdd_GetFallbackIDNode();
    return(ind);
}

/*-----------------------------------------------------------------*/
ULONG wdd_ReadEnvStatus(UBYTE *fname, ULONG def_val)
/*
**  CHANGED
**      02-Jun-98   floh    created
*/
{
    APTR fp;
    ULONG retval = def_val;
    if (fp = fopen(fname,"r")) {
        UBYTE buf[128];
        if (fgets(buf,sizeof(buf),fp)) {
            UBYTE *dike_out;
            if (dike_out = strpbrk(buf, "; \n")) *dike_out=0;
            if (stricmp(buf,"yes")==0) retval = TRUE;
            else                       retval = FALSE;
        };
        fclose(fp);
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void wdd_WriteEnvStatus(UBYTE *fname, ULONG status)
/*
**  CHANGED
**      02-Jun-98   floh    created
*/
{
    APTR fp;
    if (fp = fopen(fname,"w")) {
        if (status) fputs("yes",fp);
        else        fputs("no",fp);
        fclose(fp);
    };
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, wdd_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      08-Nov-96   floh    created
**      17-Sep-97   floh    + liest ConfigItems
**      10-Mar-98   floh    + DDraw/D3D Init passiert jetzt hier,
**                            nicht mehr in MakeClass
**      26-May-98   floh    + disable_lowres Handling
**      02-Jun-98   floh    + liest AlphaTextureStatus jetzt aus
**                            einem ENV-File.
**      12-Jun-98   floh    + "export_window_mode" Keyword.
*/
{
    struct disp_idnode *idnode = NULL;
    struct windd_data  *wdd;
    struct TagItem     ti[8];
    Object *newo;
    UBYTE  *cm;
    ULONG  id,res,t=0;
    APTR fp;
    struct TagItem *txt_ti;
    ULONG force_alpha_textures;
    ULONG use_draw_primitive;
    ULONG export_window_mode;
    memset(ti,0,sizeof(ti));
    
    /*** Konfiguration auslesen ***/
    force_alpha_textures   = wdd_ReadEnvStatus(ENV_16BITTXT,TRUE);
    use_draw_primitive     = wdd_ReadEnvStatus(ENV_DPRIM,FALSE);    
    wdd_ForceAlphaTextures = force_alpha_textures;
    _GetConfigItems(NULL,wdd_ConfigItems,WINDD_NUM_CONFIG_ITEMS);
    _NewList((struct List *) &wdd_IDList);
    _NewList((struct List *) &wdd_DevList);
    export_window_mode     = wdd_ConfigItems[6].data;

    /*** DirectDraw Object erzeugen ***/
    if (!wdd_DDrawCreate(export_window_mode)) return(NULL);

    /*** (I) Attribute abfragen ***/
    id = (ULONG)   _GetTagData(DISPA_DisplayID,0,attrs);
    cm = (UBYTE *) _GetTagData(BMA_ColorMap,NULL,attrs);

    /*** hole <w> und <h> zu <id> ***/
    if (id == 0) idnode = wdd_GetIDNodeFromFile(ENV_NAME);
    else         idnode = wdd_GetIDNode(id);
    id = idnode->id;
    wdd_Log("ddraw init: picked mode %s\n",idnode->name);

    /*** erzeuge Bitmap-Object ohne eigenen Body ***/
    if (idnode->data[0] & WINDDF_IsFullScrHalf) {
        ti[t].ti_Tag=BMA_Width;  ti[t++].ti_Data=(idnode->w)>>1;
        ti[t].ti_Tag=BMA_Height; ti[t++].ti_Data=(idnode->h)>>1;
    } else {
        ti[t].ti_Tag=BMA_Width;  ti[t++].ti_Data=idnode->w;
        ti[t].ti_Tag=BMA_Height; ti[t++].ti_Data=idnode->h;
    };    
    ti[t].ti_Tag=BMA_Body;        ti[t++].ti_Data=1; // != 0!!!
    ti[t].ti_Tag=BMA_HasColorMap; ti[t++].ti_Data=TRUE;
    ti[t].ti_Tag=TAG_MORE;        ti[t].ti_Data=(ULONG)attrs;
    newo = (Object *) _supermethoda(cl,o,OM_NEW,ti);
    if (!newo) return(NULL);
    wdd = INST_DATA(cl,newo);
    wdd->id = id;

    wdd->forcesoftcursor    = FALSE;
    wdd->movieplayer        = wdd_ConfigItems[2].data;
    wdd->disablelowres      = wdd_ConfigItems[5].data;
    wdd->forcealphatextures = force_alpha_textures;
    wdd->usedrawprimitive   = use_draw_primitive;
    wdd->exportwindowmode   = export_window_mode;
    
    /*** Mode-Flags auswerten ***/
    if (idnode->data[0] & WINDDF_IsWindowed)    wdd->flags |= WINDDF_IsWindowed;
    if (idnode->data[0] & WINDDF_IsSysMem)      wdd->flags |= WINDDF_IsSysMem;
    if (idnode->data[0] & WINDDF_IsFullScrHalf) wdd->flags |= WINDDF_IsFullScrHalf;
    if (idnode->data[0] & WINDDF_IsDirect3D)    wdd->flags |= WINDDF_IsDirect3D;

    /*** restliche DirectDraw-Initialisierung ***/
    res = wdd_InitDDrawStuff(wdd,cm,idnode->w,idnode->h,idnode->data[3]);
    if (!res) {
        wdd_Log("wdd_InitDDrawStuff() failed.\n");
        _methoda(newo,OM_DISPOSE,NULL);
        return(NULL);
    };

    /*** schreibe Videomode in Env-Variable ***/
    fp = fopen(ENV_NAME,"w");
    if (fp) {
        fprintf(fp,"%s\n",idnode->name);
        fclose(fp);
    };

    /*** Direkt-Pointer auf Raster-VFMBitmap ***/
    _get(newo,RSA_Handle,&(wdd->r));

    /*** wow, das war's schon ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, wdd_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      09-Nov-96   floh    created
**      10-Mar-98   floh    + Teil der Deinitialisierung von
**                            FreeClass() hierher verlegt
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    struct Node *nd;
    wdd_KillDDrawStuff(wdd);

    /*** Display-ID-Liste und Device-Liste killen ***/
    while (nd = _RemHead((struct List *) &wdd_IDList)) _FreeVec(nd);
    while (nd = _RemHead((struct List *) &wdd_DevList)) _FreeVec(nd);

    /*** DirectDraw Object killen ***/
    wdd_DDrawDestroy();

    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      11-Nov-96   floh    created
**      14-May-97   floh    Bugfix: versetzter Mousepointer bei
**                          Fullscreen-Half-Modes
**      02-Jun-98   floh    + WINDDA_16BitTextures
**                          + WINDDA_DrawPrimitive
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    ULONG tag;
    struct TagItem *ti = attrs;

    static struct win_DispEnv disp;

    while ((tag = ti->ti_Tag) != TAG_DONE) {

        ULONG *value = (ULONG *) ti++->ti_Data;

        switch(tag) {
            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) value; break;
            case TAG_SKIP:      ti += (ULONG) value; break;
            default:

                switch(tag) {
                    case DISPA_DisplayID:
                        *value = wdd->id;
                        break;
                    case DISPA_DisplayHandle:
                        disp.hwnd   = wdd->hWnd;
                        if (wdd->flags & WINDDF_IsFullScrHalf) {
                            disp.x_size = wdd->back_w>>1;
                            disp.y_size = wdd->back_h>>1;
                        } else {
                            disp.x_size = wdd->back_w;
                            disp.y_size = wdd->back_h;
                        };
                        *value = (ULONG) &disp;
                        break;
                    case WINDDA_16BitTextures:
                        *value = wdd->forcealphatextures;
                        break;
                    case WINDDA_UseDrawPrimitive:
                        *value = wdd->usedrawprimitive;
                        break;                        
                };
        };
    };
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      23-Feb-98   floh    created
**      26-May-98   floh    + WINDDA_DisableLowres
**      02-Jun-98   floh    + WINDDA_16BitTextures
**                          + WINDDA_DrawPrimitive
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
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
                    case WINDDA_CursorMode:
                        wdd_SetCursorMode(wdd,data);
                        break;
                    case WINDDA_DisableLowres:
                        wdd->disablelowres = data;
                        break;    
                    case WINDDA_16BitTextures:
                        wdd_WriteEnvStatus(ENV_16BITTXT,data);
                        break;
                    case WINDDA_UseDrawPrimitive:
                        wdd_WriteEnvStatus(ENV_DPRIM,data);
                        break;
                };
        };
    };
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_RASTM_Clear, void *nil)
/*
**  FUNCTION
**      Das fangen wir ab, weil DISPM_Begin das automatisch
**      und schneller erledigt.
**
**  CHANGED
**      14-Nov-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_RASTM_Text, struct rast_text *msg) 
/*
**  CHANGED
**              20-Nov-97       floh    created
*/
{
        #ifdef __DBCS__
        struct raster_data *rd = INST_DATA(cl->superclass->superclass,o);
        struct windd_data *wdd = INST_DATA(cl,o);
        wdd_DrawText(wdd,&(rd->fonts),msg->string,msg->clips);
        #else
            struct windd_data *wdd=INST_DATA(cl,o); 
            if (wdd->r->Data) _supermethoda(cl,o,RASTM_Text,msg);
        #endif
}

/*=================================================================**
**  Folgende Methoden dürfen nur durchgeleitet werden, wenn        **
**  sich die Background-Map locken ließ.                           **
**=================================================================*/

rast_method(wdd_RASTM_Copy,RASTM_Copy,"RASTM_Copy");
rast_method(wdd_RASTM_Rect,RASTM_Rect,"RASTM_Rect");
rast_method(wdd_RASTM_IntRect,RASTM_IntRect,"RASTM_IntRect");
rast_method(wdd_RASTM_ClippedRect,RASTM_ClippedRect,"RASTM_ClippedRect");
rast_method(wdd_RASTM_IntClippedRect,RASTM_IntClippedRect,"RASTM_IntClippedRect");
rast_method(wdd_RASTM_Line,RASTM_Line,"RASTM_Line");
rast_method(wdd_RASTM_IntLine,RASTM_IntLine,"RASTM_IntLine");
rast_method(wdd_RASTM_ClippedLine,RASTM_ClippedLine,"RASTM_ClippedLine");
rast_method(wdd_RASTM_IntClippedLine,RASTM_IntClippedLine,"RASTM_IntClippedLined");
rast_method(wdd_RASTM_Blit,RASTM_Blit,"RASTM_Blit");
rast_method(wdd_RASTM_IntBlit,RASTM_IntBlit,"RASTM_IntBlit");
rast_method(wdd_RASTM_MaskBlit,RASTM_MaskBlit,"RASTM_MaskBlit");
rast_method(wdd_RASTM_IntMaskBlit,RASTM_IntMaskBlit,"RASTM_IntMaskBlit");
rast_method(wdd_RASTM_ClippedBlit,RASTM_ClippedBlit,"RASTM_ClippedBlit");
rast_method(wdd_RASTM_IntClippedBlit,RASTM_IntClippedBlit,"RASTM_IntClippedBlit");
rast_method(wdd_RASTM_Poly,RASTM_Poly,"RASTM_Poly");
rast_method(wdd_RASTM_Begin2D,RASTM_Begin2D,"RASTM_Begin2D");
rast_method(wdd_RASTM_End2D,RASTM_End2D,"RASTM_End2D");
rast_method(wdd_RASTM_Begin3D,RASTM_Begin3D,"RASTM_Begin3D");
rast_method(wdd_RASTM_End3D,RASTM_End3D,"RASTM_End3D");

/*-----------------------------------------------------------------*/
void wdd_PutSoftwarePointer(Object *o,
                            struct display_data *dd,
                            struct windd_data *wdd,
                            long x, long y)
/*
**  NOTE
**      KLAPPT NUR FÜR FULLSCREEN (weil die Mauskoordinaten
**      roh von Windows übernommen werden).
**
**  CHANGED
**      14-Apr-97   floh    created
**      15-Apr-97   floh    + Bugfix beim Clipping
*/
{
    if (dd->pointer) {

        long w = wdd->back_w;
        long h = wdd->back_h;

        /*** FullScreenHalf-Korrektur ***/
        if (wdd->flags & WINDDF_IsFullScrHalf) {
            x>>=1; y>>=1;
            w>>=1; h>>=1;
        };

        /*** innerhalb? ***/
        if ((x>=0)&&(y>=0)&&(x<w)&&(y<h)) {

            struct FontChar temp_chr[2];
            struct VFMFont  temp_font;
            struct rast_font rf;
            struct rast_text rt;
            UBYTE str_buf[64];
            UBYTE *str  = str_buf;
            long clip_x = 0;
            long clip_y = 0;

            /*** erzeuge einen temporären Font ***/
            temp_chr[1].offset = 0;
            temp_chr[1].width  = dd->pointer->Width;
            temp_font.page_master = NULL;
            temp_font.page_bmp    = dd->pointer;
            temp_font.page        = dd->pointer->Data;
            temp_font.fchars      = &(temp_chr[0]);
            temp_font.height      = dd->pointer->Height;
            rf.font = &temp_font;
            rf.id   = 127;
            _methoda(o,RASTM_SetFont,&rf);

            /*** Clipping? ***/
            if ((x+dd->pointer->Width) > w) {
                clip_x = w - x;
                if (clip_x == 0) return;
            };
            if ((y+dd->pointer->Height) > h) {
                clip_y = h - y;
                if (clip_y == 0) return;
            };

            /*** schreibe Pointer-String ***/
            new_font(str,127);
            pos_abs(str,(x-(w>>1)),(y-(h>>1)));
            /*** Clipping? ***/
            if (clip_x) {
                len_hori(str,clip_x);
            };
            if (clip_y) {
                len_vert(str,clip_y);
            };
            put(str,1);
            eos(str);

            /*** String rendern ***/
            rt.string = str_buf;
            rt.clips  = NULL;
            _methoda(o,RASTM_Text,&rt);
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, wdd_DISPM_Query, struct disp_query_msg *msg)
/*
**  CHANGED
**      11-Nov-96   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    ULONG id = msg->id;
    struct disp_idnode *ind;

    if (id != 0) {
        ind = wdd_GetIDNode(id);
    } else {
        if (wdd->disablelowres) {
            /*** skippe alle Aufloesungen unter 512 ***/
            struct MinList *ls = &wdd_IDList;
            struct MinNode *nd;
            for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                ind = (struct disp_idnode *) nd;
                /*** data[1] ist sichtbare Breite ***/
                if (ind->data[1] >= 512) break;
            };
            if (nd->mln_Succ == NULL) ind = NULL;
        } else {
            ind = wdd_IDList.mlh_Head;
        };
    };
    if (ind) {
        struct disp_idnode *next;
        msg->id = ind->id;
        msg->w  = ind->w;
        msg->h  = ind->h;
        strncpy(msg->name,ind->name,32);
        next = (struct disp_idnode *) ind->nd.mln_Succ;
        if (next->nd.mln_Succ) return(next->id);
    };
    return(0);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_Begin, void *nil)
/*
**  CHANGED
**      12-Nov-96   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    ENTERED("wdd_DISPM_Begin");
    wdd_Begin(wdd);
    wdd->r->Data        = wdd->back_ptr;
    wdd->r->BytesPerRow = wdd->back_pitch;
    LEFT("wdd_DISPM_Begin");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_End, void *nil)
/*
**  CHANGED
**      12-Nov-96   floh    created
*/
{
    struct windd_data *wdd  = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA(cl->superclass,o);

    ENTERED("wdd_DISPM_End");
    if (wdd_DoSoftCursor(wdd)) {
        /*** Pointer rendern ***/
        long x,y;
        wdd_GetMousePos(wdd,&x,&y);
        _methoda(o,RASTM_Begin2D,NULL);
        wdd_PutSoftwarePointer(o,dd,wdd,x,y);
        _methoda(o,RASTM_End2D,NULL);
    };
    wdd_End(wdd);
    LEFT("wdd_DISPM_End");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_Show, void *nil)
/*
**  CHANGED
**      12-Nov-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_Hide, void *nil)
/*
**  CHANGED
**      12-Nov-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_SetPalette, struct disp_setpal_msg *msg)
/*
**  CHANGED
**      14-Apr-97   floh    created
*/
{
    ENTERED("wdd_DISPM_SetPalette");
    
    /*** 8-Bit-HACK: Farbe 0 auf schwarz patchen ***/
    if (!wdd_DoDirect3D) {
        if ((msg->slot == 0) && (msg->first == 0)) {
            msg->pal[0] = msg->pal[1] = msg->pal[2] = 0;
        };
    };

    /*** Superklasse übernimmt das Mixing... ***/
    _supermethoda(cl,o,DISPM_SetPalette,msg);
    LEFT("wdd_DISPM_SetPalette");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_MixPalette, struct disp_mixpal_msg *msg)
/*
**  CHANGED
**      22-Nov-96   floh    created
*/
{
    struct windd_data *wdd  = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);

    ENTERED("wdd_DISPM_MixPalette");
    
    /*** display.class übernimmt Mixing ***/
    _supermethoda(cl,o,DISPM_MixPalette,msg);

    /*** Resultat realisieren ***/
    wdd_SetPalette(wdd,(UBYTE *)&(dd->pal));
    LEFT("wdd_DISPM_MixPalette");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_DISPM_SetPointer, struct disp_pointer_msg *msg)
/*
**  CHANGED
**      22-Nov-96   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    ENTERED("wdd_DISPM_SetPointer");
    wdd_SetMouseImage(wdd,msg->type,FALSE);
    _supermethoda(cl,o,DISPM_SetPointer,msg);
    LEFT("wdd_DISPM_SetPointer");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_WINDDM_EnableGDI, void *nil)
/*
**  CHANGED
**      08-Apr-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    ENTERED("wdd_WINDDM_EnableGDI");
    wdd_EnableGDI(wdd,WINDD_GDIMODE_WINDOW);
    LEFT("wdd_WINDDM_EnableGDI");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_WINDDM_DisableGDI, void *nil)
/*
**  CHANGED
**      08-Apr-97   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);
    ENTERED("wdd_WINDDM_DisableGDI");
    wdd_DisableGDI(wdd,WINDD_GDIMODE_WINDOW);
    wdd_SetPalette(wdd,(UBYTE *)&(dd->pal));
    LEFT("wdd_WINDDM_DisableGDI");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_WINDDM_GetText, struct windd_gettext *msg)
/*
**  CHANGED
**      08-Jan-98   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    ENTERED("wdd_WINDDM_GetText");
    _methoda(o,WINDDM_EnableGDI,NULL);
    msg->result = wdd_GetText(wdd,msg->title_text,msg->ok_text,msg->cancel_text,
                  msg->default_text,msg->timer_val,msg->timer_func,msg->timer_arg);
    _methoda(o,WINDDM_DisableGDI,NULL);
    LEFT("wdd_WINDDM_GetText");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_WINDDM_PlayMovie, struct windd_playmovie *msg)
/*
**  CHANGED
**      22-Jan-98   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);
    ENTERED("wdd_WINDDM_PlayMovie");
    wdd_PlayMovie(wdd,msg->fname);
    wdd_SetPalette(wdd,(UBYTE *)&(dd->pal));
    LEFT("wdd_WINDDM_PlayMovie");
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_WINDDM_QueryDevice, struct windd_device *msg)
/*
**  CHANGED
**      12-Mar-98   floh    created
*/
{
    struct windd_data *wdd = INST_DATA(cl,o);
    struct windd_devnode *dnd = NULL;

    if (msg->guid == NULL) {
        /*** das erste Device zurückgeben ***/
        dnd = wdd_DevList.mlh_Head;
    } else {
        /*** suche 1.Device, dessen Name übereinstimmt ***/
        struct MinList *ls;
        struct MinNode *nd;
        ls = &wdd_DevList;
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            dnd = (struct windd_devnode *)nd;
            if (strcmp(dnd->name,msg->name)==0) {
                /*** Treffer, Nachfolger nehmen ***/
                dnd = dnd->nd.mln_Succ;
                if (dnd->nd.mln_Succ) break;
                /*** sonst gab es keinen Nachfolger, durchrauschen ***/
            };
            dnd = NULL;
        };
    };

    if (dnd) {
        /*** gültigen Eintrag gefunden, neue Infos zurückschreiben ***/
        msg->name  = dnd->name;
        msg->guid  = dnd->guid;
        msg->flags = dnd->flags;
    } else {
        /*** keine weiteren Einträge ***/
        msg->name  = NULL;
        msg->guid  = NULL;
        msg->flags = 0;
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, wdd_WINDDM_SetDevice, struct windd_device *msg)
/*
**  CHANGED
**      12-Mar-98   floh    created
*/
{
    /*** ist es ein Sonderfall? ***/
    UBYTE *alt_text = "<error>";
    void  *guid     = msg->guid;
    if (msg->guid) {
        if (strcmp(msg->guid,"<primary>")==0) {
            alt_text = guid;
            guid     = NULL;
        };
        if (strcmp(msg->guid,"<software>")==0) {
            alt_text = guid;
            guid     = NULL;
        };
    };
    wdd_WriteGUID("env/guid3d.def",guid,alt_text);
}

