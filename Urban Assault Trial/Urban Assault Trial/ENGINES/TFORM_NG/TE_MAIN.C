/*
**  $Source: PRG:VFM/Engines/TransEngineNG/te_ng_main.c,v $
**  $Revision: 38.15 $
**  $Date: 1996/06/16 02:00:00 $
**  $Locker:  $
**  $Author: floh $
**
**  ANSI-konforme Transformer-Engine mit Standard-FLOAT's
**  als internes Pixel3D-Format.
**
**  (C) Copyright 1994 by A.Weissflog
*/
/*** Amiga Includes ***/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include <math.h>

/*** VFM Includes ***/
#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "modules.h"
#include "types.h"
#include "transform/te_ng.h"

/*-------------------------------------------------------------------
**  interne Prototypen
*/
BOOL te_Open(ULONG id,...);
void te_Close(void);
void te_SetAttrs(ULONG tags,...);
void te_GetAttrs(ULONG tags,...);

/*-------------------------------------------------------------------
**  Global Entry Table der Transformer-Engine.
*/
struct GET_Engine te_TE_GET = {
    te_Open,
    te_Close,
    te_SetAttrs,
    te_GetAttrs,
};

/* Einsprung-Tabelle nur für Dynamic Linking */
#ifdef DYNAMIC_LINKING
struct teng_GET_Specific te_Specific = {
    NULL,               // OBSOLETE
    NULL,               // OBSOLETE
    te_SetViewer,
    te_GetViewer,
    NULL,               // OBSOLETE
    NULL,               // OBSOLETE
    NULL,               // OBSOLETE: ViewerToDisplay()
    NULL,               // OBSOLETE: Get3DClipCodes()
    NULL,               // OBSOLETE: Clip3DPolygon()
    NULL,
    NULL,               // OBSOLETE: PolygonPipeline()
    NULL,               // OBSOLETE: Isolate3DPolygon()
    te_RefreshTForm,
    te_TFormToGlobal,
    NULL,               // OBSOLETE
    NULL,               // OBSOLETE
};
#endif

/*-----------------------------------------------------------------*/
_use_nucleus

struct TE_Base TEBase;

/*** Config-Handling ***/
struct ConfigItem te_ConfigItems[] = {
    { TECONF_MaxZ,  CONFIG_INTEGER, 4096 },
    { TECONF_MinZ,  CONFIG_INTEGER, 16 },
    { TECONF_ZoomX, CONFIG_INTEGER, 320 },
    { TECONF_ZoomY, CONFIG_INTEGER, 200 },
};

#define TE_NUM_CONFIG_ITEMS (4)

/*-------------------------------------------------------------------
**  *** CODE SEGMENT HANDLING ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/*** spezieller minimaler Startup-Code ***/
__geta4 struct GET_Engine *start(void)
{
    return(&te_TE_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* der Einsprung... */
struct GET_Engine *te_entry(void)
{
    return(&te_TE_GET);
};

/* und die SegmentInfo-Struktur */
struct SegInfo tform_ng_engine_seg = {
    { NULL, NULL,       /* ln_Succ, ln_Pred */
      0, 0,             /* ln_Type, ln_Pri  */
      "MC2engines:tform_NG.engine"  /* der Segment-Name */
    },
    te_entry,   /* Entry()-Adresse */
};
#endif

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
/*-------------------------------------------------------------------
**  Support-Routine, um Nucleus-GET aus Initialisierungs-Tagliste
**  zu holen, kann logischerweise nicht mit _GetTagData() arbeiten,
**  weil noch kein NUC_GET existiert!
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
__geta4 BOOL te_Open(ULONG id,...)
#else
BOOL te_Open(ULONG id,...)
#endif
/*
**  FUNCTION
**      Öffnet Transformer-Engine.
**
**  INPUTS
**      Folgende Tags werden akzeptiert:
**          MID_NUCLEUS
**
**  RESULTS
**      TRUE -> alles ok, sonst Fehler
**
**  CHANGED
**      15-Oct-94   floh    created
**      02-Jan-95   floh    jetzt Nucleus2-kompatibel
**      05-Feb-95   floh    enthält jetzt durch neues Config-Handling
**                          die gesamte Öffnungs-Prozedur für die
**                          Engine
**      05-Jun-95   floh    jetzt mit Profiling Code,
**                          nein... doch nicht
*/
{
    /* DYNAMIC-LINK-SPECIFICS */
    #ifdef DYNAMIC_LINKING
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(FALSE);
    #endif

    /*** Konfiguration auslesen ***/
    _GetConfigItems(NULL, te_ConfigItems, TE_NUM_CONFIG_ITEMS);
    TEBase.MaxZ  = (FLOAT) te_ConfigItems[0].data;
    TEBase.MinZ  = (FLOAT) te_ConfigItems[1].data;
    TEBase.ZoomX = (FLOAT) te_ConfigItems[2].data;
    TEBase.ZoomY = (FLOAT) te_ConfigItems[3].data;

    /*** SinCosTable allokieren und ausfüllen ***/
    TEBase.Sincos_Table = (struct Sincos_atom *)
        _AllocVec(361*sizeof(struct Sincos_atom),MEMF_PUBLIC);
    if (TEBase.Sincos_Table) {
        struct Sincos_atom *table = TEBase.Sincos_Table;
        WORD degree;
        FLOAT rad;
        for (degree = 0; degree<361; degree++) {
            rad = ((FLOAT)degree)/180.0 * M_PI;
            table->sinus   = (FLOAT) sin(rad);
            table->cosinus = (FLOAT) cos(rad);
            table++;
        };
    } else {
        return(FALSE);
    };

    /* fertig */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 void te_Close(void)
#else
void te_Close(void)
#endif
/*
**  FUNCTION
**      Schließt Transformer-Engine komplett.
**
**  INPUTS
**      ---
**  RESULTS
**      ---
**  CHANGED
**      15-Oct-94   floh    created
**      02-Jan-95   floh    jetzt Nucleus2-kompatibel
**      05-Jan-95   floh    enthält jetzt komplette Close-Prozedur
**                          (durch neues Config-Handling).
**      05-Jun-95   floh    Jetzt mit Profiling Code,
**                          nein... doch nicht
*/
{
    /* gebe Sincos_Table frei, falls vorhanden */
    if (TEBase.Sincos_Table) _FreeVec(TEBase.Sincos_Table);

    /* Ende */
}
/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 void te_SetAttrs(ULONG tags,...)
#else
void te_SetAttrs(ULONG tags,...)
#endif
/*
**  FUNCTION
**      Folgende Attribute sind settable:
**          TET_ZoomX
**          TET_ZoomY
**          TET_MaxZ
**          TET_MinZ
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      16-Oct-94   floh    created
**      02-Jan-94   floh    jetzt Nucleus2-kompatibel
**      05-Feb-95   floh    heißt jetzt te_SetAttrs()
*/
{
    struct TagItem *ti;

    if (ti = _FindTagItem(TET_ZoomX, (struct TagItem *) &tags)) {
        TEBase.ZoomX = (FLOAT) ti->ti_Data;
    };

    if (ti = _FindTagItem(TET_ZoomY, (struct TagItem *) &tags)) {
        TEBase.ZoomY = (FLOAT) ti->ti_Data;
    };

    if (ti = _FindTagItem(TET_MaxZ, (struct TagItem *) &tags)) {
        TEBase.MaxZ = (FLOAT) ti->ti_Data;
    };

    if (ti = _FindTagItem(TET_MinZ, (struct TagItem *) &tags)) {
        TEBase.MinZ = (FLOAT) ti->ti_Data;
    };

    /* Ende */
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 void te_GetAttrs(ULONG tags,...)
#else
void te_GetAttrs(ULONG tags,...)
#endif
/*
**  FUNCTION
**      Folgende Tags sind settable:
**          TET_GET_SPEC    (nur DYNAMIC_LINKING)
**          TET_ZoomX
**          TET_ZoomY
**          TET_MaxZ
**          TET_MinZ
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      05-Feb-95   floh    created
**      11-Feb-95   floh    etwas "Advanced Typecasting", weil
**                          irgendwie beim Umwandeln einer Float-Zahl
**                          in eine LONG-Zahl eine externe Routine
**                          benötigt wird... deshalb hab ichs mit
**                          Umweg über WORD gemacht, ist dann zwar
**                          nur 16 Bit genau, aber was solls...
*/
{
    ULONG *value;
    struct TagItem *tlist = (struct TagItem *) &tags;

    #ifdef DYNAMIC_LINKING
    value = (ULONG *) _GetTagData(TET_GET_SPEC, NULL, tlist);
    if (value) *value = (ULONG) &te_Specific;
    #endif

    value = (ULONG *) _GetTagData(TET_ZoomX, NULL, tlist);
    if (value) *value = (ULONG) ((WORD)TEBase.ZoomX);

    value = (ULONG *) _GetTagData(TET_ZoomY, NULL, tlist);
    if (value) *value = (ULONG) ((WORD)TEBase.ZoomY);

    value = (ULONG *) _GetTagData(TET_MaxZ, NULL, tlist);
    if (value) *value = (ULONG) ((WORD)TEBase.MaxZ);

    value = (ULONG *) _GetTagData(TET_MinZ, NULL, tlist);
    if (value) *value = (ULONG) ((WORD)TEBase.MinZ);

    /* Ende */
}

