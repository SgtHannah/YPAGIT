/*
**  $Source: PRG:VFM/Classes/_BitmapClass/bitmapclass.c,v $
**  $Revision: 38.16 $
**  $Date: 1998/01/06 14:45:02 $
**  $Locker:  $
**  $Author: floh $
**
**  (C) Copyright 1995,1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "bitmap/bitmapclass.h"
#include "bitmap/displayclass.h"

struct NucleusBase *bm_NBase;

/*-------------------------------------------------------------------
**  Moduleigene Prototypen
*/
_dispatcher(Object *, bm_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, bm_OM_DISPOSE, void *ignored);
_dispatcher(void, bm_OM_SET, struct TagItem *attrs);
_dispatcher(void, bm_OM_GET, struct TagItem *attrs);
_dispatcher(Object *, bm_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, bm_OM_SAVETOIFF, struct iff_msg *iffmsg);

_dispatcher(struct RsrcNode *, bm_RSM_CREATE, struct TagItem *tlist);
_dispatcher(void, bm_RSM_FREE, struct rsrc_free_msg *msg);

_dispatcher(struct VFMBitmap *, bm_BMM_GETBITMAP, struct frametime_msg *msg);
_dispatcher(Pixel2D *, bm_BMM_GETOUTLINE, void *ignored);
_dispatcher(void, bm_BMM_BMPOBTAIN, struct bmpobtain_msg *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG bm_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG bm_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo bm_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeBitmapClass(ULONG id,...);
__geta4 BOOL FreeBitmapClass(void);
#else
struct ClassInfo *MakeBitmapClass(ULONG id,...);
BOOL FreeBitmapClass(void);
#endif

struct GET_Class bitmap_GET = {
    &MakeBitmapClass,
    &FreeBitmapClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&bitmap_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *bitmap_Entry(void)
{
    return(&bitmap_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo bitmap_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:bitmap.class" /* der Segment-Name */
    },
    bitmap_Entry,               /* Entry()-Adresse */
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
        };
        tagList++;
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeBitmapClass(ULONG id,...)
#else
struct ClassInfo *MakeBitmapClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      MakeExternClass()-Einsprung für BitmapClass...
**
**  INPUTS
**      Folgende Tags werden gescannt:
**          MID_NUCLEUS
**
**  RESULTS
**      Class * -> Pointer auf ausgefüllte Class-Info-Struktur
**
**  CHANGED
**      10-Mar-94   floh    created
**      13-Mar-94   floh    die bitmap.class ist jetzt subclass der
**                          resource.class -> ClassInfo-Struktur dahingehend
**                          geändert.
**      20-Apr-94   floh    Neue Methode: BMM_GETOUTLINE
**      03-Jan-95   floh    Nucleus2-Revision
**      10-Jan-96   floh    - kein OM_SAVETOIFF, OM_NEWFROMIFF mehr
**                          + jetzt Subklasse der rsrc.class
**      25-Feb-97   floh    + importiert NucleusBase
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    _get_nbase((struct TagItem *)&id,&bm_NBase);

    /* Methoden-Array initialisieren */
    memset(bm_Methods,0,sizeof(bm_Methods));

    bm_Methods[OM_NEW]         = (ULONG) bm_OM_NEW;
    bm_Methods[OM_DISPOSE]     = (ULONG) bm_OM_DISPOSE;
    bm_Methods[OM_GET]         = (ULONG) bm_OM_GET;
    bm_Methods[OM_SET]         = (ULONG) bm_OM_SET;

    bm_Methods[RSM_CREATE]     = (ULONG) bm_RSM_CREATE;
    bm_Methods[RSM_FREE]       = (ULONG) bm_RSM_FREE;

    bm_Methods[BMM_GETBITMAP]  = (ULONG) bm_BMM_GETBITMAP;
    bm_Methods[BMM_GETOUTLINE] = (ULONG) bm_BMM_GETOUTLINE;
    bm_Methods[BMM_BMPOBTAIN]  = (ULONG) bm_BMM_BMPOBTAIN;

    /* ClassInfo-Struktur ausfüllen */
    bm_clinfo.superclassid = RSRC_CLASSID;
    bm_clinfo.methods      = bm_Methods;
    bm_clinfo.instsize     = sizeof(struct bitmap_data);
    bm_clinfo.flags        = 0;

    /* und das war's... */
    return(&bm_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeBitmapClass(void)
#else
BOOL FreeBitmapClass(void)
#endif
/*
**  FUNCTION
**      ...
**
**  INPUTS
**      ---
**
**  RESULTS
**      Zur Zeit immer TRUE.
**
**  CHANGED
**      10-Mar-94   floh    created
**      02-Jan-95   floh    Nucleus2-Revision
*/
{
    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
BOOL createOutline(Object *o,
                   struct bitmap_data *bmd,
                   Pixel2D *source)
/*
**  FUNCTION
**      Initialisiert die Outline-Definition des Bitmap-Objekts.
**      Wenn bereits eine Outline-Definition existiert,
**      wird diese korrekt gekillt.
**
**  INPUTS
**      o       -> Pointer auf "mich"
**      bmd     -> Pointer auf Locale Instance Data
**      source  -> Pixel2D * mit Outline-Definition. Dieses
**                 Array wird in einen eigenen Puffer kopiert.
**                 Falls das Object mit RSA_DontCopy==TRUE erzeugt
**                 wurde, muß die "Source" allerdings bereits
**                 im VFMOutline-Format (FLOATings) vorliegen!
**
**  RESULTS
**      TRUE  -> alles OK
**      FALSE -> Not Enough Mem
**
**  CHANGED
**      20-Apr-94   floh    created
**      22-Oct-94   floh    Beim Zählen der Outline wurde
**                          falsch gegen Ende getestet.
**      23-Oct-94   floh    neu: <o>,<cl>-Parameter, neu: fragt jetzt
**                          den RSA_DontCopy-Status ab, um zu ermitteln,
**                          ob die Outline in einen eigenen Puffer
**                          kopiert werden oder einfach nur der Pointer
**                          übernommen werden soll.
**      03-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    neu: aus der Pixel2D-Source wird ein
**                          Array aus <struct VFMOutline> (FLOAT-Format)
**                          erzeugt!
**                          Das heißt, im Falle von RSA_DontCopy==TRUE
**                          wird die Source nicht im Pixel2D-Format,
**                          sondern bereits im VFMOutline-Format erwartet.
*/
{
    /* frage Status von RSA_DontCopy ab */
    ULONG dont_copy;
    _get(o, RSA_DontCopy, &dont_copy);

    /* falls bereits eine [eigene] Outline existiert, diese killen */
    if (bmd->outln) {
        if (!dont_copy) {
            /* wurde kopiert, also alte Outline killen */
            _FreeVec(bmd->outln);
        };
    };

    if (dont_copy) {
        /* Outline-Pointer direkt übernehmen */
        bmd->outln = (struct VFMOutline *) source;

    } else {
        /* sonst Outline in einen eigenen Puffer kopieren */

        /* Anzahl Pixel2Ds in Outline-Definition */
        ULONG i;
        ULONG numelm = 1;

        struct VFMOutline *ol_buf;
        Pixel2D *actptr = source;

        while (actptr->flags >= 0) {
            actptr++;
            numelm++;
        };

        /* eigenen Puffer allokieren */
        ol_buf = (struct VFMOutline *) 
                 _AllocVec(numelm*sizeof(struct VFMOutline),MEMF_PUBLIC);
        if (!ol_buf) return(FALSE);

        /* Pixel2D-Source nach <struct VFMOutline> konvertieren... */
        for (i=0; i<(numelm-1); i++) {
            ol_buf[i].x = ((FLOAT)source[i].x) / 256.0;
            ol_buf[i].y = ((FLOAT)source[i].y) / 256.0;
        };
        ol_buf[i].x = -1.0;     // Outline begrenzen
        ol_buf[i].y = -1.0;

        /* neuen Pointer eintragen... */
        bmd->outln = ol_buf;
    };

    /* ...fertig! */
    return(TRUE);
}

/*******************************************************************/
/***    AB HIER METHODEN DISPATCHER    *****************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, bm_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**      Konstruktor der bitmap.class. Innerhalb des
**      Konstruktors selbst wird nur die Erzeugung der
**      optionalen Outline (übergeben mit BMA_Outline)
**      erzeugt, der Rest wird von bm_RSM_CREATE bzw.
**      einer Subklasse übernommen.
**
**  CHANGED
**      10-Mar-94   floh    created
**      13-Mar-94   floh    umgeschrieben, wegen neuer Definition
**                          der bitmap.class
**      20-Apr-94   floh    jetzt mit Outline-Definition
**      23-Oct-94   floh    Falls (RSA_DontCopy==TRUE), wird die
**                          Outline NICHT in einen eigenen Buffer
**                          kopiert, sondern der per BMA_OUTLINE
**                          direkt in die LID eingetragen.
**      03-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    interne Outline liegt jetzt im 
**                          <struct VFMOutline> Format vor.
**      10-Jan-96   floh    angepaßt an rsrc.class Philosphie
*/
{
    Object *newo;
    struct bitmap_data *bmd;
    Pixel2D *outline;

    /* Methode an Superclass... */
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /* Locale Instance Data Pointer ermitteln */
    bmd = INST_DATA(cl,newo);

    /* TagList scannen */
    outline = (Pixel2D *) _GetTagData(BMA_Outline,NULL,attrs);
    if (outline) {
        /* es kann zwar sein, das es schiefgeht, dann ist aber eh alles */
        /* zu spät, weil der Memory alle ist */
        createOutline(newo, bmd, outline);
    };

    /*** hole statischen Ptr auf VFMBitmap, Subklassen, die keine ***/
    /*** VFMBitmap erzeugen, müssen RSA_Handle abfangen und NULL  ***/
    /*** zurückliefern! ***/
    _get(newo, RSA_Handle, &(bmd->bitmap));

    /* das war alles... */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, bm_OM_DISPOSE, void *ignored)
/*
**  FUNCTION
**
**  CHANGED
**      10-Mar-94   floh    created
**      20-Apr-94   floh    Falls vorhanden, wird das Outline-Array
**                          freigegeben.
**      23-Oct-94   floh    falls die (evtl. vorhandene) Outline in einem
**                          eigenen Puffer steht, wird sie freigegeben
**      03-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    interne Outline liegt jetzt im 
**                          <struct VFMOutline> Format vor.
**      10-Jan-96   floh    angepaßt an rsrc.class Philosophie
*/
{
    struct bitmap_data *bmd = INST_DATA(cl,o);

    /* Outline-Array killen */
    if (bmd->outln) {

        /* wurde diese kopiert? */
        ULONG not_copied;
        _get(o, RSA_DontCopy, &not_copied);

        if (!not_copied) {
            /* wurde kopiert, also alte Outline killen */
            _FreeVec(bmd->outln);
        };
    };

    /* Kill me softly... */
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bm_OM_SET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**      scannt folgende Attribute:
**
**          BMA_Outline     <Pixel2D *>
**          BMA_ColorMap    <UBYTE [3] *>
**
**  RESULTS
**      ---
**
**  CHANGED
**      20-Apr-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    interne Outline liegt jetzt im 
**                          <struct VFMOutline> Format vor.
**      10-Jan-96   floh    angepaßt an rsrc.class Philosophie
**      29-May-96   floh    + BMA_ColorMap
*/
{
    struct bitmap_data *bmd = INST_DATA(cl,o);
    struct TagItem *ti;

    /* BMA_Outline */
    if (ti = _FindTagItem(BMA_Outline,attrs)) {
        Pixel2D *outline = (Pixel2D *) ti->ti_Data;
        createOutline(o,bmd,outline);
    };

    /* BMA_ColorMap */
    if (ti = _FindTagItem(BMA_ColorMap,attrs)) {
        UBYTE *cm = (UBYTE *) ti->ti_Data;
        if (bmd->bitmap->ColorMap) {
            memcpy(bmd->bitmap->ColorMap, cm, 256*3);
        };
    };

    /* Methode an Superclass weiterreichen */
    _supermethoda(cl,o,OM_SET,attrs);

    /* ENDE */
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bm_OM_GET, struct TagItem *attrs)
/*
**  FUNCTION
**
**  INPUTS
**      Folgende Attribute sind gettable:
**          BMA_Bitmap      <struct VFMBitmap *>
**          BMA_Outline     OBSOLETE!
**          BMA_Width       <ULONG>
**          BMA_Height      <ULONG>
**          BMA_Type        <ULONG>
**          BMA_Body        <UBYTE *>
**          BMA_HasColorMap <BOOL>
**          BMA_ColorMap    <UBYTE *>
**
**  CHANGED
**      10-Mar-94   floh    created
**      13-Mar-94   floh    Die Methode wird jetzt korrekt an den
**                          resource.class-Dispatcher weitergereicht
**      20-Apr-94   floh    neu: BMA_Outline
**      03-Jan-95   floh    Nucleus2-Revision
**      04-Jan-95   floh    Oh no, böser Bug, "attrs" wurde modifiziert
**                          an die Superclass weitergegeben.
**      07-Jul-95   floh    Interne Outline liegt jetzt im
**                          <struct VFMOutline> Format vor. Aus diesem
**                          Grund liefert ein _get(BMA_Outline) vorerst
**                          generell NULL zurück, weil ich nicht ins
**                          Pixel2D-Format zurückkonvertieren will...
**                          Falls Probleme auftauchen, mach ich's noch...
**      10-Jan-96   floh    + rsrc.class Philosophie
**                          + BMA_Width, BMA_Height, BMA_Type, BMA_Height
**      29-May-96   floh    + BMA_HasColorMap, BMA_ColorMap
**      25-Feb-97   floh    + BMA_Type obsolete
*/
{
    struct bitmap_data *bmd = INST_DATA(cl,o);
    ULONG *value;
    ULONG tag;
    struct TagItem *store_attrs = attrs;

    /* TagList scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        value = (ULONG *) attrs++->ti_Data;
        switch(tag) {
            /* die System-Tags */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

            /* dann die "echten" Attribute */
            switch(tag) {
                case BMA_Bitmap:  *value = (ULONG) bmd->bitmap; break;
                case BMA_Outline: *value = (ULONG) NULL; break;

                case BMA_Width:
                    if (bmd->bitmap) *value = bmd->bitmap->Width;
                    else             *value = 0;
                    break;

                case BMA_Height:
                    if (bmd->bitmap) *value = bmd->bitmap->Height;
                    else             *value = 0;
                    break;

                case BMA_Type:
                    /*** OBSOLETE *** OBSOLETE *** OBSOLETE ***/
                    break;

                case BMA_Body:
                    if (bmd->bitmap) *value = (ULONG) bmd->bitmap->Data;
                    else             *value = 0;
                    break;

                case BMA_HasColorMap:
                    *value = (bmd->bitmap->ColorMap) ? TRUE:FALSE;
                    break;

                case BMA_ColorMap:
                    *value = (ULONG) bmd->bitmap->ColorMap;
                    break;
            };
        };
    };

    /* Methode nach oben reichen */
    _supermethoda(cl,o,OM_GET,store_attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(struct VFMBitmap *, bm_BMM_GETBITMAP, struct frametime_msg *msg)
/*
**  FUNCTION
**      *** OBSOLETE *** OBSOLETE *** OBSOLETE ***
*/
{
    return(NULL);
}

/*-----------------------------------------------------------------*/
_dispatcher(Pixel2D *, bm_BMM_GETOUTLINE, void *ignore)
/*
**  FUNCTION
**      *** OBSOLETE *** OBSOLETE *** OBSOLETE ***
*/
{
    return(NULL);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bm_BMM_BMPOBTAIN, struct bmpobtain_msg *msg)
/*
**  FUNCTION
**      Liefert eingebettete Bitmap- und (falls vorhanden)
**      Outline-Information zurück.
**
**  INPUTS
**      msg.time_stamp      - globaler TimeStamp des Frames
**      msg.frame_time      - Länge dieses Frames
**
**  RESULTS
**      msg.bitmap          - Pointer auf eingebettete Bitmap-Struktur
**      msg.outline         - Pointer auf eingebettete Outline im
**                            <struct VFMOutline> Format.
**
**  CHANGED
**      07-Jul-95   floh    created
*/
{
    struct bitmap_data *bd = INST_DATA(cl,o);

    msg->bitmap  = bd->bitmap;
    msg->outline = bd->outln;
}

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, bm_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**      Falls BMA_Width *und* BMA_Height angegeben ist,
**      wird eine eigene VFMBitmap allokiert und
**      initialisiert. In diesem Fall kann optional ein
**      "fremdes" Pixel-Array verwendet werden, wenn
**      dieses per BMA_Body angegeben wurde.
**
**      Falls BMA_HasColorMap TRUE ist, wird Platz für
**      eine ColorMap allokiert, und diese mit NULL
**      gefüllt (in diesem Fall muß BMA_Width/BMA_Height
**      NICHT angegeben sein, das Bitmap-Object besteht
**      dann nur aus einer Colormap).
**
**      Andernfalls wird NICHTS gemacht (in diesem Fall
**      sollte eine Subklasse das Teil erzeugen, zum Beispiel
**      durch Reinladen) -> muß aber immer zuerst gucken,
**      ob das Result->Handle bereits ausgefüllt wurde!!!
**
**  INPUTS
**      BMA_Width
**      BMA_Height
**      BMA_Body
**      BMA_HasColorMap
**      BMA_Texture
**      BMA_TxtBlittable
**
**      alle optional...
**
**  CHANGED
**      10-Jan-96   floh    created
**      29-May-96   floh    + BMA_HasColorMap
**      25-Feb-97   floh    + BMA_Texture Handling
**      25-Mar-97   floh    + BMA_TxtBlittable Handling
*/
{
    struct RsrcNode *rnode;

    /*** Resource-Node erzeugen lassen... ***/
    rnode = (struct RsrcNode *) _supermethoda(cl,o,RSM_CREATE,tlist);
    if (rnode) {

        /*** sind genug Daten da, um eigene Bitmap zu allokieren? ***/
        ULONG w,h,has_cm,is_texture,is_txtblittable;

        w               = _GetTagData(BMA_Width,  0, tlist);
        h               = _GetTagData(BMA_Height, 0, tlist);
        has_cm          = _GetTagData(BMA_HasColorMap, FALSE, tlist);
        is_texture      = _GetTagData(BMA_Texture, FALSE, tlist);
        is_txtblittable = _GetTagData(BMA_TxtBlittable, FALSE, tlist);
        if ((w && h) || has_cm) {

            struct VFMBitmap *bmp;

            bmp = (struct VFMBitmap *) 
                  _AllocVec(sizeof(struct VFMBitmap), MEMF_PUBLIC|MEMF_CLEAR);

            if (bmp) {

                /*** Colormap? ***/
                if (has_cm) {
                    bmp->Flags |= VBF_HasColorMap;
                    bmp->ColorMap = (UBYTE *) _AllocVec(256*3,MEMF_PUBLIC|MEMF_CLEAR);
                    if (NULL == bmp->ColorMap) {
                        _FreeVec(bmp);
                        return(rnode);
                    };
                };

                /*** Bitmap-Data nur wenn Höhe/Breite angegeben ***/
                if (w && h) {

                    bmp->Width  = w;
                    bmp->Height = h;

                    /*** Textur? ***/
                    if (is_texture && bm_NBase->GfxObject) {
                        struct disp_texture dt;
                        bmp->Width   = w;
                        bmp->Height  = h;
                        bmp->Flags  |= VBF_Texture;
                        if (is_txtblittable) bmp->Flags |= VBF_TxtBlittable;
                        dt.texture = bmp;
                        if (!_methoda(bm_NBase->GfxObject,DISPM_ObtainTexture,&dt)) {
                            _FreeVec(bmp);
                            return(rnode);
                        };

                    } else {
                        /*** "normale" Bitmap ***/
                        UBYTE *body = (UBYTE *) _GetTagData(BMA_Body, NULL, tlist);
                        bmp->BytesPerRow = w;
                        if (body) {
                            /*** extern bereitgestellter Body ***/
                            bmp->Data    = body;
                            bmp->Flags  |= VBF_AlienData;
                        } else {
                            /*** Body selbst allokieren ***/
                            bmp->Data = (UBYTE *) _AllocVec(w*h,MEMF_PUBLIC|MEMF_CLEAR);
                            if (NULL == bmp->Data) {
                                /*** rnode->Handle nach wie vor ungültig!!! ***/
                                _FreeVec(bmp);
                                return(rnode);
                            };
                        };
                    };
                };

            } else return(rnode);

            /*** Bitmap wurde komplett erzeugt, also RsrcNode komplettieren ***/
            rnode->Handle = (void *) bmp;
        };
    };

    return(rnode);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bm_RSM_FREE, struct rsrc_free_msg *msg)
/*
**  FUNCTION
**      Die übergebene RsrcNode wird auf ein gültiges
**      Handle untersucht. Ist eins da, wird dieses
**      als VFMBitmap interpretiert und freigegeben.
**
**      Das ist wichtig für Subklassen, die evtl. keine
**      VFMBitmaps als Handle verwenden (z.B. bmpanim.class).
**      Diese müssen dann <msg->rnode->Handle> vor der
**      bitmap.class "verstecken", indem sie es auf NULL
**      setzen.
**
**  CHANGED
**      10-Jan-96   floh    created
**      11-Jan-96   floh    debugging: Methode wurde nicht nach oben
**                          gereicht
**      29-May-96   floh    + ColorMap-Freigabe, falls existent
**      25-Feb-97   floh    + VBF_Texture Handling
*/
{
    struct RsrcNode *rnode = msg->rnode;
    struct VFMBitmap *bmp;

    /*** existiert ein gültiges Handle? ***/
    if (bmp = rnode->Handle) {

        /*** dann dieses als VFMBitmap interpretieren ***/
        if ((bmp->Flags & VBF_Texture) && bm_NBase->GfxObject) {
            struct disp_texture dt;
            dt.texture = bmp;
            _methoda(bm_NBase->GfxObject,DISPM_ReleaseTexture,&dt);
        }else if (!(bmp->Flags & VBF_AlienData)){
            if (bmp->Data) _FreeVec(bmp->Data);
        };
        if (bmp->ColorMap) _FreeVec(bmp->ColorMap);
        _FreeVec(bmp);

        rnode->Handle = NULL;
    };

    /*** die rsrc.class erledigt den Rest ***/
    _supermethoda(cl,o,RSM_FREE,msg);
}

