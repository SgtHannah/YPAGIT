/*
**  $Source: PRG:VFM/Classes/_AreaClass/areaclass.c,v $
**  $Revision: 38.29 $
**  $Date: 1996/11/10 20:59:43 $
**  $Locker:  $
**  $Author: floh $
**
**  Die area.class kann Flächen aller Art darstellen...
**
**  (C) Copyright 1994-1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "engine/engine.h"
#include "polygon/polygonflags.h"
#include "polygon/polygon.h"

#include "visualstuff/ov_engine.h"
#include "transform/te.h"

#include "ade/areaclass.h"
#include "skeleton/skeletonclass.h"
#include "bitmap/bitmapclass.h"

/*-------------------------------------------------------------------
**  Moduleigene Prototypen
*/
_dispatcher(Object *, area_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, area_OM_DISPOSE, void *ignored);
_dispatcher(void, area_OM_SET, struct TagItem *attrs);
_dispatcher(void, area_OM_GET, struct TagItem *attrs);
_dispatcher(Object *, area_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, area_OM_SAVETOIFF, struct iff_msg *iffmsg);

_dispatcher(void, area_ADEM_PUBLISH, struct publish_msg *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus
_use_ov_engine
_use_tform_engine

#ifdef AMIGA
__far ULONG area_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG area_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo area_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes.
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeAreaClass(ULONG id,...);
__geta4 BOOL FreeAreaClass(void);
#else
struct ClassInfo *MakeAreaClass(ULONG id,...);
BOOL FreeAreaClass(void);
#endif

struct GET_Class area_GET = {
    &MakeAreaClass,             /* MakeExternClass() */
    &FreeAreaClass,             /* FreeVASAClass()   */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&area_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *area_Entry(void)
{
    return(&area_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo area_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:area.class"       /* der Segment-Name */
    },
    area_Entry,  /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeAreaClass(ULONG id,...)
#else
struct ClassInfo *MakeAreaClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      Meldet die area.class im Nucleus-System an.
**
**  INPUTS
**      Folgende Tags werden ausgefiltert:
**          MID_ENGINE_OUTPUT_VISUAL
**          MID_ENGINE_TRANSFORM
**          MID_NUCLEUS
**
**  RESULTS
**      Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      08-Apr-94   floh    created
**      21-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
*/
{
    struct TagItem *tlist = (struct TagItem *) &id;

    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray fischen */
    if (!(NUC_GET = local_GetNucleus(tlist))) return(NULL);
    #endif

    _get_ov_engine(tlist);
    _get_tform_engine(tlist);

    /* Methoden-Array initialisieren */
    memset(area_Methods,0,sizeof(area_Methods));

    area_Methods[OM_NEW]         = (ULONG) area_OM_NEW;
    area_Methods[OM_DISPOSE]     = (ULONG) area_OM_DISPOSE;
    area_Methods[OM_SET]         = (ULONG) area_OM_SET;
    area_Methods[OM_GET]         = (ULONG) area_OM_GET;
    area_Methods[OM_NEWFROMIFF]  = (ULONG) area_OM_NEWFROMIFF;
    area_Methods[OM_SAVETOIFF]   = (ULONG) area_OM_SAVETOIFF;
    area_Methods[ADEM_PUBLISH]   = (ULONG) area_ADEM_PUBLISH;

    /* ClassInfo Struktur ausfüllen */
    area_clinfo.superclassid = ADE_CLASSID;
    area_clinfo.methods      = area_Methods;
    area_clinfo.instsize     = sizeof(struct area_data);
    area_clinfo.flags        = 0;

    /* und das war alles... */
    return(&area_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeAreaClass(void)
#else
BOOL FreeAreaClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**      ---
**
**  RESULTS
**      TRUE -> alles OK
**      FALSE-> Klasse konnte nicht freigegeben werden (sollte eigentlich
**              nie auftreten).
**
**  CHANGED
**      08-Apr-94   floh    created
**      21-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
*/
{
    return(TRUE);
}

/*===================================================================
**  SUPPORT ROUTINEN
*/

/*-----------------------------------------------------------------*/
void bindTxtBitmapObject(struct area_data *ad, Object *bmpo)
/*
**  FUNCTION
**      Übernimmt alles, was beim Anbinden des TxtBitmap-Objects
**      so anfällt:
**          1) bei Bedarf altes TxtBitmap-Objekt disposen
**          2) neues TxtBitmap-Objekt nach ad->TxtBitmap
**
**  INPUTS
**      ad   - Pointer auf LID
**      bmpo - anzubindendes TxtBitmap-Object [instance of bitmap.class]
**
**  RESULTS
**      ---
**
**  CHANGED
**      21-Apr-94   floh    created
**      04-May-94   floh    Neues Flag AREAF_TexturePossible wird
**                          jetzt mit abgehandelt (weil
**                          AREAF_TextureMapped jetzt über OM_SET ein-/
**                          ausgeschaltet werden kann).
**      22-Sep-94   floh    1) die Outline wird jetzt online abgefragt,
**                          das Eintragen derselbigen in die LID
**                          entfällt also
**                          2) No more Texture-Flags, jetzt existieren
**                          ja die Polygon-Flags, deren Handhabung
**                          etwas komplexer ist, deshalb nix mehr
**                          internes Error-Handling. (Bin gerade auf
**                          einem Minimalisierungs-Trip...)
**                          3) Es gibt jetzt zwei Routinen dieser Art,
**                          eine fürs TxtBitmap-Object und eine
**                          fürs TracyBitmap-Object.
**      21-Nov-94   floh    serial debug code
**      16-Jan-95   floh    Nucleus2-Revision
*/
{
    /* existiert ein altes TxtBitmap-Object ? */
    if (ad->TxtBitmap) _dispose(ad->TxtBitmap);
    ad->TxtBitmap = bmpo;
}

/*-----------------------------------------------------------------*/
void bindTracyBitmapObject(struct area_data *ad, Object *bmpo)
/*
**  FUNCTION
**      siehe <bindTxtBitmapObject()>
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      22-Sep-94   floh    created
**      21-Nov-94   floh    serial debug code
**      16-Jan-95   floh    Nucleus2-Revision
*/
{
    /* existiert ein altes TracyBitmap-Object ? */
    if (ad->TracyBitmap) _dispose(ad->TracyBitmap);
    ad->TracyBitmap = bmpo;
}

/*-----------------------------------------------------------------*/
BOOL area_initAttributes(struct area_data *ad, struct TagItem *attrs)
/*
**  FUNCTION
**      Übernimmt Auswertung der (I)-Attribute.
**
**  INPUTS
**      ad      - Pointer auf LID
**      attrs   - Pointer auf (I) Attribut-Liste
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> schwerwiegender Fehler
**
**  CHANGED
**      11-Dec-94   floh    created
**      16-Jan-95   floh    Nucleus2-Revision
**      14-Mar-95   floh    massive Vereinfachungen bei PolyInfo-Init
**                          (die Routine initSkeletonStuff() existiert
**                          überhaupt nicht mehr!)
**      17-Mar-95   floh    ADEA_ExtSkeleton wird jetzt auch mit
**                          einem NULL-Pointer akzeptiert.
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* Default-Attribut-Values initialisieren */
    register UWORD poly_flags = DEF_PolyFlags;

    if (ADEA_DepthFade_DEF) ad->Flags |= AREAF_DepthFade;

    ad->ColorValue = AREAA_ColorValue_DEF;
    ad->TracyValue = AREAA_TracyValue_DEF;
    ad->ShadeValue = AREAA_ShadeValue_DEF;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* die ade.class Attribute */
                switch(tag) {

                    case ADEA_DepthFade:
                        if (data)  ad->Flags |= AREAF_DepthFade;
                        else       ad->Flags &= ~AREAF_DepthFade;
                        break;

                    case ADEA_Polygon:
                        ad->PolyInfo.PolyNum = data;
                        break;
                };

                /* die area.class Attribute */
                switch(tag) {

                    case AREAA_TxtBitmap:
                        if (data) bindTxtBitmapObject(ad,(Object *)data);
                        break;

                    case AREAA_ColorValue:
                        ad->ColorValue = data;
                        break;

                    case AREAA_Map:
                        poly_flags &= ~(PLGF_MAPBIT1|PLGF_MAPBIT2);
                        switch (data) {
                            case MAP_NONE:      poly_flags |= PLGF_NONMAPPED; break;
                            case MAP_LINEAR:    poly_flags |= PLGF_LINEARMAPPED; break;
                            case MAP_DEPTH:     poly_flags |= PLGF_DEPTHMAPPED; break;
                        };
                        break;

                    case AREAA_Txt:
                        poly_flags &= ~(PLGF_TXTBIT1);
                        switch (data) {
                            case TXT_NONE:      poly_flags |= PLGF_NOTEXTURE; break;
                            case TXT_MAPPED:    poly_flags |= PLGF_TEXTUREMAPPED; break;
                        };
                        break;

                    case AREAA_Shade:
                        poly_flags &= ~(PLGF_SHADEBIT1|PLGF_SHADEBIT2);
                        switch (data) {
                            case SHADE_NONE:    poly_flags |= PLGF_NOSHADE; break;
                            case SHADE_FLAT:    poly_flags |= PLGF_FLATSHADE; break;
                            case SHADE_LINE:    poly_flags |= PLGF_LINESHADE; break;
                            case SHADE_GRADIENT: poly_flags |= PLGF_GRADSHADE; break;
                        };
                        break;

                    case AREAA_Tracy:
                        poly_flags &= ~(PLGF_TRACYBIT1|PLGF_TRACYBIT2);
                        switch (data) {
                            case TRACY_NONE:    poly_flags |= PLGF_NOTRACY; break;
                            case TRACY_CLEAR:   poly_flags |= PLGF_CLEARTRACY; break;
                            case TRACY_FLAT:    poly_flags |= PLGF_FLATTRACY; break;
                            case TRACY_MAPPED:  poly_flags |= PLGF_TRACYMAPPED; break;
                        };
                        break;

                    case AREAA_TracyMode:
                        poly_flags &= ~(PLGF_TRACYBIT3);
                        switch (data) {
                            case TRACYMODE_DARKEN:  poly_flags |= PLGF_DARKEN; break;
                            case TRACYMODE_LIGHTEN: poly_flags |= PLGF_LIGHTEN; break;
                        };
                        break;

                    case AREAA_TracyBitmap:
                        if (data) bindTracyBitmapObject(ad,(Object *)data);
                        break;

                    case AREAA_ShadeValue:
                        ad->ShadeValue = data;
                        break;

                    case AREAA_TracyValue:
                        ad->TracyValue = data;
                        break;

                };
        };
    };


    /* finally müssen noch die Polygon-Flags eingetragen werden */
    ad->PolyInfo.Flags = poly_flags;

    /* das war's dann */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void area_setAttributes(Object *o, struct area_data *ad, struct TagItem *attrs)
/*
**  FUNCTION
**      Setzt (S)-Attribute
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Dec-94   floh    created
**      16-Jan-95   floh    Nucleus2-Revision
**      14-Mar-94   floh    massive Vereinfachung bei den Attributen
**                          ADEA_ExtSkeleton und ADEA_Polygon
**      17-Mar-95   floh    ADEA_ExtSkeleton wird jetzt auch mit einem
**                          NULL-Pointer akzeptiert.
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;

    /* ein paar lokale Variablen... */
    UWORD poly_flags  = ad->PolyInfo.Flags;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* die ade.class Attribute */
                switch(tag) {

                    case ADEA_DepthFade:
                        if (data) ad->Flags |= AREAF_DepthFade;
                        else      ad->Flags &= ~AREAF_DepthFade;
                        break;

                    case ADEA_Polygon:
                        ad->PolyInfo.PolyNum = data;
                        break;
                };

                /* die area.class Attribute */
                switch(tag) {

                    case AREAA_TxtBitmap:
                        if (data) bindTxtBitmapObject(ad,(Object *)data);
                        break;

                    case AREAA_ColorValue:
                        ad->ColorValue = data;
                        break;

                    case AREAA_Map:
                        poly_flags &= ~(PLGF_MAPBIT1|PLGF_MAPBIT2);
                        switch (data) {
                            case MAP_NONE:      poly_flags |= PLGF_NONMAPPED; break;
                            case MAP_LINEAR:    poly_flags |= PLGF_LINEARMAPPED; break;
                            case MAP_DEPTH:     poly_flags |= PLGF_DEPTHMAPPED; break;
                        };
                        break;

                    case AREAA_Txt:
                        poly_flags &= ~(PLGF_TXTBIT1);
                        switch (data) {
                            case TXT_NONE:      poly_flags |= PLGF_NOTEXTURE; break;
                            case TXT_MAPPED:    poly_flags |= PLGF_TEXTUREMAPPED; break;
                        };
                        break;

                    case AREAA_Shade:
                        poly_flags &= ~(PLGF_SHADEBIT1|PLGF_SHADEBIT2);
                        switch (data) {
                            case SHADE_NONE:    poly_flags |= PLGF_NOSHADE; break;
                            case SHADE_FLAT:    poly_flags |= PLGF_FLATSHADE; break;
                            case SHADE_LINE:    poly_flags |= PLGF_LINESHADE; break;
                            case SHADE_GRADIENT: poly_flags |= PLGF_GRADSHADE; break;
                        };
                        break;

                    case AREAA_Tracy:
                        poly_flags &= ~(PLGF_TRACYBIT1|PLGF_TRACYBIT2);
                        switch (data) {
                            case TRACY_NONE:    poly_flags |= PLGF_NOTRACY; break;
                            case TRACY_CLEAR:   poly_flags |= PLGF_CLEARTRACY; break;
                            case TRACY_FLAT:    poly_flags |= PLGF_FLATTRACY; break;
                            case TRACY_MAPPED:  poly_flags |= PLGF_TRACYMAPPED; break;
                        };
                        break;

                    case AREAA_TracyMode:
                        poly_flags &= ~(PLGF_TRACYBIT3);
                        switch (data) {
                            case TRACYMODE_DARKEN:  poly_flags |= PLGF_DARKEN; break;
                            case TRACYMODE_LIGHTEN: poly_flags |= PLGF_LIGHTEN; break;
                        };
                        break;

                    case AREAA_TracyBitmap:
                        if (data) bindTracyBitmapObject(ad,(Object *)data);
                        break;

                    case AREAA_ShadeValue:
                        ad->ShadeValue = data;
                        break;

                    case AREAA_TracyValue:
                        ad->TracyValue = data;
                        break;

                    case AREAA_Blob1:
                        poly_flags = (UWORD) data;
                        ad->Flags  = (UWORD) (data>>16);
                        break;

                    case AREAA_Blob2:
                        ad->ShadeValue = data;
                        ad->TracyValue = data>>8;
                        ad->ColorValue = data>>16;
                        break;

                };
        };
    };

    /* Polygonflags wieder zurückschreiben */
    ad->PolyInfo.Flags = poly_flags;

    /* das war's dann... */
}

/*-----------------------------------------------------------------*/
void area_getAttributes(struct area_data *ad, struct TagItem *attrs)
/*
**  FUNCTION
**      Fragt (G)-Attribute ab.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      11-Dec-94   floh    created
**      16-Jan-95   floh    Nucleus2-Revision
**      14-Mar-95   floh    neu: AREAA_PolygonInfo
**      17-Jan-96   floh    revised & updated
*/
{
    register ULONG tag;
    UWORD poly_flags = ad->PolyInfo.Flags;

    /* Attribut-Liste scannen */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch(tag) {

            /* die System-Tags */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

                /* dann die "echten" Attribute */
                switch (tag) {

                    case AREAA_TxtBitmap:
                        *value = (ULONG) ad->TxtBitmap;
                        break;

                    case AREAA_ColorValue:
                        *value = ad->ColorValue;
                        break;

                    case AREAA_Map:
                        switch (poly_flags & (PLGF_MAPBIT1|PLGF_MAPBIT2)) {
                            case PLGF_NONMAPPED:    *value = MAP_NONE; break;
                            case PLGF_LINEARMAPPED: *value = MAP_LINEAR; break;
                            case PLGF_DEPTHMAPPED:  *value = MAP_DEPTH; break;
                        };
                        break;

                    case AREAA_Txt:
                        switch (poly_flags & (PLGF_TXTBIT1)) {
                            case PLGF_NOTEXTURE:     *value = TXT_NONE; break;
                            case PLGF_TEXTUREMAPPED: *value = TXT_MAPPED; break;
                        };
                        break;

                    case AREAA_Shade:
                        switch (poly_flags & (PLGF_SHADEBIT1|PLGF_SHADEBIT2)) {
                            case PLGF_NOSHADE:      *value = SHADE_NONE; break;
                            case PLGF_FLATSHADE:    *value = SHADE_FLAT; break;
                            case PLGF_LINESHADE:    *value = SHADE_LINE; break;
                            case PLGF_GRADSHADE:    *value = SHADE_GRADIENT; break;
                        };
                        break;

                    case AREAA_Tracy:
                        switch (poly_flags & (PLGF_TRACYBIT1|PLGF_TRACYBIT2)) {
                            case PLGF_NOTRACY:      *value = TRACY_NONE; break;
                            case PLGF_CLEARTRACY:   *value = TRACY_CLEAR; break;
                            case PLGF_FLATTRACY:    *value = TRACY_FLAT; break;
                            case PLGF_TRACYMAPPED:  *value = TRACY_MAPPED; break;
                        };
                        break;

                    case AREAA_TracyMode:
                        switch (poly_flags & (PLGF_TRACYBIT3)) {
                            case PLGF_DARKEN:   *value = TRACYMODE_DARKEN; break;
                            case PLGF_LIGHTEN:  *value = TRACYMODE_LIGHTEN; break;
                        };
                        break;

                    case AREAA_TracyBitmap:
                        *value = (ULONG) ad->TracyBitmap;
                        break;

                    case AREAA_ShadeValue:
                        *value = ad->ShadeValue;
                        break;

                    case AREAA_TracyValue:
                        *value = ad->TracyValue;
                        break;

                    case AREAA_PolygonInfo:
                        *value = (ULONG) &(ad->PolyInfo);
                        break;

                };
        };
    };

    /* Ende */
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, area_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      08-Apr-94   floh    created
**      21-Apr-94   floh    Kein Support für AREAA_TxtDef mehr. Die
**                          Umriß-Definition wurde in die bitmap.class
**                          ausgelagert.
**                          Allerdings wird das Bitmap-Object probeweise
**                          nach der Outline gefragt. Wenn eine
**                          zurückgegeben wird, wird das TextureMapped-Flag
**                          gesetzt, sonst gelöscht.
**                          --> Der gesamte Bitmap/Outline-Stuff wurde
**                              in die lokale Support-Routine
**                              bindBitmapObject() ausgelagert.
**      04-May-94   floh    neu: AREAA_Texture-Handling.
**      08-May-94   floh    neu: ADEA_DetailLimit-Handling
**      23-Sep-94   floh    Neues Attribut-Set. Bisherige area.class-
**                          Attribute sind <obsolete>, werden
**                          aber trotzdem noch unterstützt.
**                          -> Attribut-Scanning wurde in die Prozedur
**                             initAttributes() ausgelagert.
**      21-Nov-94   floh    (1) ein paar Tags werden ignoriert, siehe
**                              initAttributes()
**                          (2) serial debug code
**                          (3) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    Object *newo;
    struct area_data *ad;

    /* Methode an Superclass hochreichen */
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    /* Locale Instance Data initialisieren */
    ad = INST_DATA(cl,newo);

    /* Init-Attribute extern auswerten... */
    if (!area_initAttributes(ad,attrs)) {
        /* oops, da war ein Speicher-Problem... */
        _methoda(newo,OM_DISPOSE,0);
        return(NULL);
    };

    /* ...und tschüß */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, area_OM_DISPOSE, void *ignored)
/*
**  CHANGED
**      09-Apr-94   floh    created
**      21-Apr-94   floh    ad->TxtPoints wird nicht mehr FreeVec()'ed,
**                          weil es nicht mehr existiert (Outline-Def
**                          ist jetzt Sache des Bitmap-Objects).
**      23-Apr-94   floh    Weil jetzt evtl. zwei Bitmap-Objects existieren,
**                          werden logischerweise beide auch evtl.
**                          disposed.
**      18-Oct-94   floh    leicht modifiziert wegen neuer in LID
**                          eingebundener PolygonInfo-Struktur
**      21-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
**      18-Jan-95   floh    Die beiden Zwischen-Puffer in der
**                          PolyInfo-Struktur sind durch Änderungen
**                          im Polygon-Handling überflüssig geworden,
**                          deshalb werden sie auch nicht mehr freigegeben.
**      17-Jan-96   floh    revised & updated
*/
{
    struct area_data *ad = INST_DATA(cl,o);

    /* einverleibte Objects */
    if (ad->TxtBitmap)   _dispose(ad->TxtBitmap);
    if (ad->TracyBitmap) _dispose(ad->TracyBitmap);

    /* OM_DISPOSE hochreichen */
    return((BOOL) _supermethoda(cl,o,OM_DISPOSE,(Msg *)ignored));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, area_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      09-Apr-94   floh    created
**      10-Apr-94   floh    kleinerer Bug beim Scannen des
**                          ADEA_ExtSkeleton-Attributs
**      22-Apr-94   floh    1) AREAA_TxtDef-Attribut existiert nicht mehr,
**                             Outline-Definition ist jetzt Sache des
**                             Bitmap-Objects.
**                          2) Neue Bitmap-Objects werden getestet, ob sie
**                             eine Outline-Definition besitzen. Wenn nein,
**                             wird das TextureMapped-Flag gelöscht, sonst
**                             gesetzt.
**                          3) --> Das gesamte Bitmap/Outline-Handling
**                             wurde in die lokale Support-Routine
**                             bindBitmapObject() ausgelagert.
**      04-May-94   floh    Neu: AREAA_Texture-Handling
**      08-May-94   floh    Neu: ADEA_Detail
**                               ADEA_DetailLimit
**      23-Sep-94   floh    Praktisch komplett umgeschrieben,
**                          wegen massig neuer Attribute + der
**                          "Blob-Attribut-Strategie".
**                          Die eigentliche Set-Attributes-Routine
**                          ist jetzt extern (for no good reasons) ;-)
**      21-Nov-94   floh    (1) ein paar Attribute werden ignoriert,
**                              siehe setAttributes()
**                          (2) serial debug code
**                          (3) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    struct area_data *ad = INST_DATA(cl,o);

    /* rufe externen "Attribut-Handler" auf */
    area_setAttributes(o,ad,attrs);

    /* Methode an Superclass weiterreichen */
    _supermethoda(cl,o,OM_SET,(Msg *)attrs);

    /* ENDE */
}

/*-----------------------------------------------------------------*/
_dispatcher(void, area_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      09-Apr-94   floh    created
**      21-Apr-94   floh    keine Unterstützung mehr für AREAA_TxtKoords-
**                          Attribut
**      04-May-94   floh    neu: AREAA_Texture-Handling
**      23-Sep-94   floh    Kompletter Rewrite mit externer
**                          getAttributes()-Prozedur.
**      21-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    struct area_data *ad = INST_DATA(cl,o);

    /* the easy way... */
    area_getAttributes(ad,attrs);

    /* Methode an SuperClass */
    _supermethoda(cl,o,OM_GET,(Msg *)attrs);
}

/*=================================================================**
**  SUPPORT ROUTINEN FÜR OM_NEWFROMIFF                             **
**=================================================================*/
BOOL handle_STRC(Object *newo, struct area_data *ad, struct IFFHandle *iff)
{
    if (newo) {

        /* IFFAttr-Struktur und zu initialisierende TagList */
        struct area_IFFAttr iffattr;
        struct TagItem tags[5]; /* bei Erweiterung anpassen! */

        /* lese und endian-konvertiere Chunk-Inhalt */
        _ReadChunkBytes(iff, &iffattr, sizeof(iffattr));
        v2nw(&(iffattr.version));
        v2nl(&(iffattr.blob1));
        v2nl(&(iffattr.blob2));

        /* initialisiere TagList */
        if (iffattr.version >= 1) {
            tags[0].ti_Tag  = AREAA_Blob1;
            tags[0].ti_Data = iffattr.blob1;
            tags[1].ti_Tag  = AREAA_Blob2;
            tags[1].ti_Data = iffattr.blob2;
        };

        /* begrenze Tag-Array */
        tags[2].ti_Tag = TAG_DONE;

        /* gelesene Attribute setzen */
        _seta(newo,(Msg *)tags);

        return(TRUE);

    } else return(FALSE);
}
/*-----------------------------------------------------------------*/
BOOL handle_OBJT(Object *newo, struct area_data *ad, struct IFFHandle *iff)
{
    if (newo) {

        Object *bmpo;
        ULONG txtmask, tracymask, masked;

        /* interessierende Flags ausmaskieren... */
        txtmask   = ad->PolyInfo.Flags & (PLGF_TXTBIT1);
        tracymask = ad->PolyInfo.Flags & (PLGF_TRACYBIT1|PLGF_TRACYBIT2);

        /* Mapping-Maske korrekt aufbauen */
        masked = txtmask;
        if (tracymask == PLGF_TRACYMAPPED) masked |= tracymask;

        /* Object lesen */
        bmpo = (Object *) _LoadInnerObject(iff);
        if (!bmpo) return(FALSE);

        /* aber welches ??? */
        switch (masked) {
            case PLGF_TEXTUREMAPPED:    /* only txt-mapped */
                _set(newo,AREAA_TxtBitmap,bmpo);
                break;
            case PLGF_TRACYMAPPED:      /* only tracy-mapped */
                _set(newo,AREAA_TracyBitmap,bmpo);
                break;
            case (PLGF_TEXTUREMAPPED | PLGF_TRACYMAPPED):
                /* beides kombiniert... */
                if (!(ad->TxtBitmap)) _set(newo,AREAA_TxtBitmap,bmpo);
                else                  _set(newo,AREAA_TracyBitmap,bmpo);
                break;
        };
        return(TRUE);

    } else return(FALSE);
}

// /*-----------------------------------------------------------------*/
// /* DAS ALTE ZEUG AKZEPTIEREN WIR NUR NOCH AUF DEM AMIGA...         */
// /*-----------------------------------------------------------------*/
// #ifdef AMIGA
// BOOL handle_ATTR(Object *newo, struct area_data *ad, struct IFFHandle *iff)
// {
//     if (newo) {
//
//         /* Chunk-Puffer init... */
//         struct TagItem orig_tags[8];
//         struct TagItem new_tags[16];
//
//         /* Chunk-Puffer lesen... */
//         _ReadChunkBytes(iff,orig_tags,sizeof(orig_tags));
//
//         /* uralte Version oder alte Version? */
//         if (orig_tags[0].ti_Tag == 0x8000000D) {
//             /* altes AREAA_BaseColor */
//             new_tags[0].ti_Tag = AREAA_ColorValue;
//             new_tags[0].ti_Data = orig_tags[0].ti_Data;
//             /* altes AREAA_Stamp */
//             new_tags[1].ti_Tag = AREAA_Tracy;
//             new_tags[1].ti_Data = (orig_tags[1].ti_Data) ? TRACY_NONE : TRACY_CLEAR;
//             /* alte ADEs waren immer texture-gemapped */
//             new_tags[2].ti_Tag = AREAA_Map;
//             new_tags[2].ti_Data = MAP_DEPTH;
//             new_tags[3].ti_Tag = AREAA_Txt;
//             new_tags[3].ti_Data = TXT_MAPPED;
//             new_tags[4].ti_Tag = TAG_DONE;
//         } else {
//             /* die etwas neuere Version mit AREAA_BLOB1/AREAA_BLOB2 */
//             new_tags[0].ti_Tag = AREAA_Blob1;
//             new_tags[0].ti_Data = orig_tags[0].ti_Data;
//             new_tags[1].ti_Tag = AREAA_Blob2;
//             new_tags[1].ti_Data = orig_tags[1].ti_Data;
//             new_tags[2].ti_Tag = TAG_DONE;
//         };
//
//         /* Attribute direkt setzen (nicht via OM_SET)... */
//         _seta(newo,(Msg *)new_tags);
//
//         return(TRUE);
//
//     } else return(FALSE);
// }
// #endif /* AMIGA */
//
/*-----------------------------------------------------------------*/
_dispatcher(Object *, area_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      09-Apr-94   floh    created
**      10-Apr-94   floh    Bug: es wurde ein "TRUE" zurückgegeben,
**                          anstatt ein Pointer auf das neue Object.
**      21-Apr-94   floh    AREAIFF_TextureChunk-Stuff komplett entfernt,
**                          Outline-Definition ist jetzt Sache der
**                          bitmap.class
**      23-Sep-94   floh    Komplett überarbeitet und optimiert.
**                          1) Subklassen hören nix mehr von den
**                             area.class-Attributen, das bedeutet,
**                             es wird nicht empfohlen, eine
**                             Subklasse der area.class zu schreiben...
**                          2) Subklassen hören auch nix mehr von den
**                             eingebetteten Bitmap-Object für Textur-
**                             und Tracy-Map.
**                          (1) und (2) dürften einen signifikanten
**                          SpeedUp bringen und "so weit unten" (bei den
**                          ADEs ist das wichtiger als Luxus!
**      18-Oct-94   floh    leichte Modifikationen wegen neuer
**                          in LID eingebetteter PolygonInfo-Struktur
**      20-Oct-94   floh    - neues Attributes-Laden per STRC-Chunk
**                          - höchstmögliche Kompatibilität, akzeptiert
**                            jetzt beide bisherige Versionen des
**                            ATTR-Chunks und konvertiert deren
**                            Attribute in "moderne" Attribut-Listen.
**                          - alles was ging nach außen verlagert, weil
**                            DICE wieder mit seinen "Too many Redos (9)"
**                            rumgesponnen hat
**      02-Nov-94   floh    Oops, in handleOBJT() war ja noch ein dermaßen
**                          blöder Fehler, der dazu geführt hat, daß
**                          keine Objects geladen wurden, sobald
**                          IRGENDEIN Tracy-Flag gesetzt war...
**                          Das Transparenz-Handling bei "alten"
**                          Objects ist etwas mystisch geraten...
**                          Falls bei alten Objects TextureMapping
**                          eingeschaltet war, wird dieses jetzt
**                          als "MAP_LINEAR" emuliert.
**      21-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      16-Jan-95   floh    (1) Nucleus2-Revision
**                          (2) veralteter ATTR-Chunk wird vorerst nur
**                              noch auf dem AMIGA akzeptiert.
**      18-Jan-95   floh    debugging auf PC...
**      15-Jan-95   floh    diverse geladene Attribute werden jetzt
**                          per _set() gesetzt, das macht die Sache
**                          vererbungsfreundlicher
**      17-Jan-96   floh    revised & updated
*/
{
    /* zuerst den Pointer auf das IFF-Handle aus der Msg holen */
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* File parsen */
    Object *newo = NULL;
    struct area_data *ad = NULL;
    ULONG ifferror;
    struct ContextNode *cn;

    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror) {
            if (newo) _methoda(newo,OM_DISPOSE,0);
            return(NULL);
        };

        cn = _CurrentChunk(iff);

        /* FORM der SuperClass ? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == ADEIFF_FORMID)) {

            /* dann Kontrolle an Superclass */
            newo = (Object *) _supermethoda(cl,o,OM_NEWFROMIFF,(Msg *)iffmsg);
            if (!newo) return(NULL);

            ad = INST_DATA(cl,newo);

            continue;
        };

        /* STRC-Chunk (Attribute) */
        if (cn->cn_ID == AREAIFF_IFFAttr) {

            if (!handle_STRC(newo,ad,iff)) {
                _methoda(newo,OM_DISPOSE,0);  return(NULL);
            };
            /* End of Chunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* Object-Chunk ? (heavy magic begins here...) */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == FORM_Object)) {

            if (!handle_OBJT(newo,ad,iff)) {
                _methoda(newo,OM_DISPOSE,0);  return(NULL);
            };
            /* KEIN EndOfChunk... */
            continue;
        };

        // #ifdef AMIGA
        // /* Emulation der alten Attributes-Chunks (nur auf AMIGA) */
        // if (cn->cn_ID == AREAIFF_Attributes) {
        //
        //     if (!handle_ATTR(newo,ad,iff)) {
        //         _methoda(newo,OM_DISPOSE,0);  return(NULL);
        //     };
        //     _ParseIFF(iff,IFFPARSE_RAWSTEP);
        //     continue;
        // };
        // #endif /* AMIGA */

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);

        /* Ende der Parse-Schleife */
    };
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, area_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      09-Apr-94   floh    created
**      21-Apr-94   floh    AREAIFF_Texture-Chunk-Stuff komplett entfernt,
**                          Outline-Definition wurde nach bitmap.class
**                          verlagert.
**      23-Sep-94   floh    1) neuer Aufbau des Attributes-Chunks
**                             (Blob-Attribute)
**                          2) Enhanced bitmap object handling.
**      18-Oct-94   floh    leichte Modifikationen wegen neuer in
**                          LID eingebetteter PolygonInfo-Struktur
**      19-Oct-94   floh    dummerweise wurde noch ad->Flags statt
**                          ad->PolyInfo.Flags ausgewertet.
**      20-Oct-94   floh    der neue ATTR-Chunk existiert schon nicht mehr,
**                          jetzt wird ein erweiterungs-freundlicheres
**                          Schema verwendet (STRC-Chunk).
**      21-Nov-94   floh    (1) serial debug code
**                          (2) __geta4
**      16-Jan-95   floh    Nucleus2-Revision
**      17-Jan-96   floh    revised & updated
*/
{
    struct area_data *ad  = INST_DATA(cl,o);
    struct IFFHandle *iff = iffmsg->iffhandle;
    struct area_IFFAttr iffattr;

    /* eigenen FORM schreiben */
    if (_PushChunk(iff,AREAIFF_FORMID,ID_FORM,IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /* Methode an SuperClass hochreichen */
    if (!_supermethoda(cl,o,OM_SAVETOIFF,(Msg *)iffmsg)) return(FALSE);

    /* AREAIFF_IFFAttr-Chunk erzeugen .. endian-konv .. schreiben */
    _PushChunk(iff,0,AREAIFF_IFFAttr,IFFSIZE_UNKNOWN);

    iffattr.version = AREAIFF_VERSION;
    iffattr.blob1   = (ULONG) (((ad->Flags)<<16)|(ad->PolyInfo.Flags));
    iffattr.blob2   = (ULONG) ad->ColorValue<<16|
                              ad->TracyValue<<8|
                              ad->ShadeValue;
    n2vl(&(iffattr.blob1));
    n2vl(&(iffattr.blob2));

    _WriteChunkBytes(iff,&iffattr,sizeof(iffattr));
    _PopChunk(iff);


    /*** Texture-Bitmap-Object ***/
    if ((ad->PolyInfo.Flags & (PLGF_TXTBIT1)) == PLGF_TEXTUREMAPPED) {

        /* Texture-Mapping ist eingeschaltet, also muß ein */
        /* TxtBitmap-Object vorhanden sein! */
        if (!ad->TxtBitmap)  return(FALSE);
        else if (!_SaveInnerObject(ad->TxtBitmap,iff)) return(FALSE);
    };

    /*** Tracy-Bitmap-Object ***/
    if ((ad->PolyInfo.Flags & (PLGF_TRACYBIT1|PLGF_TRACYBIT2)) == PLGF_TRACYMAPPED) {

        /* Tracymapping ist eingeschaltet, also muß auch ein */
        /* TracyBitmap-Object vorhanden sein! */
        if (!ad->TracyBitmap) return(FALSE);
        else if (!_SaveInnerObject(ad->TracyBitmap,iff)) return(FALSE);
    };

    /* AREAIFF_FORMID-Chunk poppen */
    if (_PopChunk(iff) != 0) return(FALSE);

    /* Ende */
    return(TRUE);
}

