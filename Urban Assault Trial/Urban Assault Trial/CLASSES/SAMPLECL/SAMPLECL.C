/*
**  $Source: PRG:VFM/Classes/_SampleClass/sampleclass.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/04/18 23:31:02 $
**  $Locker:  $
**  $Author: floh $
**
**  (C) Copyright 1996 by A.Weissflog
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
#include "audio/sampleclass.h"

/*-------------------------------------------------------------------
**  Moduleigene Prototypen
*/
_dispatcher(Object *, smp_OM_NEW, struct TagItem *attrs);
_dispatcher(void, smp_OM_GET, struct TagItem *attrs);

_dispatcher(struct RsrcNode *, smp_RSM_CREATE, struct TagItem *tlist);
_dispatcher(void, smp_RSM_FREE, struct rsrc_free_msg *msg);

_dispatcher(void, smp_SMPM_OBTAIN, struct smpobtain_msg *msg);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG smp_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG smp_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo smp_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeSampleClass(ULONG id,...);
__geta4 BOOL FreeSampleClass(void);
#else
struct ClassInfo *MakeSampleClass(ULONG id,...);
BOOL FreeSampleClass(void);
#endif

struct GET_Class sample_GET = {
    &MakeSampleClass,
    &FreeSampleClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&sample_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *sample_Entry(void)
{
    return(&sample_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo sample_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:sample.class" /* der Segment-Name */
    },
    sample_Entry,               /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeSampleClass(ULONG id,...)
#else
struct ClassInfo *MakeSampleClass(ULONG id,...)
#endif
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Methoden-Array initialisieren */
    memset(smp_Methods,0,sizeof(smp_Methods));

    smp_Methods[OM_NEW]      = (ULONG) smp_OM_NEW;
    smp_Methods[OM_GET]      = (ULONG) smp_OM_GET;

    smp_Methods[RSM_CREATE]  = (ULONG) smp_RSM_CREATE;
    smp_Methods[RSM_FREE]    = (ULONG) smp_RSM_FREE;

    smp_Methods[SMPM_OBTAIN] = (ULONG) smp_SMPM_OBTAIN;

    /* ClassInfo-Struktur ausfüllen */
    smp_clinfo.superclassid = RSRC_CLASSID;
    smp_clinfo.methods      = smp_Methods;
    smp_clinfo.instsize     = sizeof(struct sample_data);
    smp_clinfo.flags        = 0;

    /* und das war's... */
    return(&smp_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeSampleClass(void)
#else
BOOL FreeSampleClass(void)
#endif
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, smp_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    Object *newo;
    struct sample_data *smd;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);
    smd = INST_DATA(cl,newo);

    _get(newo, RSA_Handle, &(smd->sample));
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, smp_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    struct sample_data *smd = INST_DATA(cl,o);
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
                case SMPA_Sample:   *value = (ULONG) smd->sample; break;

                case SMPA_Type:
                    if (smd->sample) *value = (ULONG) smd->sample->Type;
                    else             *value = 0;
                    break;

                case SMPA_Length:
                    if (smd->sample) *value = (ULONG) smd->sample->Length;
                    else             *value = 0;
                    break;

                case SMPA_Body:
                    if (smd->sample) *value = (ULONG) smd->sample->Data;
                    else             *value = 0;
                    break;
            };
        };
    };

    /* Methode nach oben reichen */
    _supermethoda(cl,o,OM_GET,store_attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, smp_SMPM_OBTAIN, struct smpobtain_msg *msg)
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    struct sample_data *smd = INST_DATA(cl,o);
    msg->sample = smd->sample;
}

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, smp_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**      Falls SMPA_Length *und* SMPA_Type angegeben ist, 
**      wird eine VFMSample-Struktur allokiert und initialisiert.
**      In diesem Fall kann optional ein "fremder" Sample-Body
**      verwendet werden, wenn dieser per SMPA_Body gegeben
**      ist.
**
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    struct RsrcNode *rnode;

    /*** Resource-Node erzeugen lassen ***/
    rnode = (struct RsrcNode *) _supermethoda(cl,o,RSM_CREATE,tlist);
    if (rnode) {

        /*** genug Tags, um eigenes VFMSample zu allokieren? ***/
        ULONG l,t;

        l = _GetTagData(SMPA_Length, 0, tlist);
        t = _GetTagData(SMPA_Type,   0xffff, tlist);
        if ((l > 0) && (t != 0xffff)) {

            struct VFMSample *smp;
            smp = (struct VFMSample *)
                  _AllocVec(sizeof(struct VFMSample), MEMF_PUBLIC|MEMF_CLEAR);
            if (smp) {

                UBYTE *body;

                /*** Sample initialisieren ***/
                smp->Length = l;
                smp->Type   = t;

                body = (UBYTE *) _GetTagData(SMPA_Body, NULL, tlist);
                if (body) {
                    /*** extern bereitgestellter Body ***/
                    smp->Data   = body;
                    smp->Flags |= VSMPF_AlienData;
                } else {
                    /*** Body selbst allokieren ***/
                    smp->Data = (UBYTE *) _AllocVec(l, MEMF_PUBLIC|MEMF_CLEAR);
                    if (NULL == smp->Data) {
                        _FreeVec(smp);
                        return(rnode);
                    };
                };
            } else return(rnode);

            /*** VFMSample ist komplett, also RNode-Handle gültig machen ***/
            rnode->Handle = (void *) smp;
        };
    };
    return(rnode);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, smp_RSM_FREE, struct rsrc_free_msg *msg)
/*
**  FUNCTION
**      siehe bitmap.class/RSM_CREATE!
**
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    struct RsrcNode *rnode = msg->rnode;
    struct VFMSample *smp;

    /*** existiert ein gültiges Handle? ***/
    if (smp = rnode->Handle) {

        /*** dann dieses als VFMSample interpretieren ***/
        if (!(smp->Flags & VSMPF_AlienData)) {
            if (smp->Data) _FreeVec(smp->Data);
        };
        _FreeVec(smp);
        rnode->Handle = NULL;
    };

    /*** die rsrc.class erledigt den Rest ***/
    _supermethoda(cl,o,RSM_FREE,msg);
}

