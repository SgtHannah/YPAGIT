/*
**  $Source: PRG:VFM/Classes/_BmpAnimClass/bmpanimclass.c,v $
**  $Revision: 38.8 $
**  $Date: 1996/01/22 22:17:54 $
**  $Locker:  $
**  $Author: floh $
**
**  Die bmpanim.class integriert Bitmap- und Outline-Animation
**  mit Timing-Kontrolle in ein normal ansprechbares
**  Bitmap-Objekt.
**
**  (C) Copyright 1994 by A.Weissflog
*/
/*** Amiga Includes ***/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

/*** ANSI Includes ***/
#include <stdlib.h>
#include <string.h>

/*** VFM Includes ***/
#include "types.h"
#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "bitmap/bmpanimclass.h"


/*-------------------------------------------------------------------
**  Moduleigene Prototypen
*/
_dispatcher(Object *, bmpanim_OM_NEW, struct TagItem *attrs);
_dispatcher(BOOL, bmpanim_OM_DISPOSE, void *ignored);
_dispatcher(void, bmpanim_OM_GET, struct TagItem *attrs);
_dispatcher(void, bmpanim_OM_SET, struct TagItem *attrs);
_dispatcher(Object *, bmpanim_OM_NEWFROMIFF, struct iff_msg *iffmsg);
_dispatcher(BOOL, bmpanim_OM_SAVETOIFF, struct iff_msg *iffmsg);

_dispatcher(void, bmpanim_BMM_BMPOBTAIN, struct bmpobtain_msg *msg);

_dispatcher(struct RsrcNode *, bmpanim_RSM_CREATE, struct TagItem *tlist);
_dispatcher(void, bmpanim_RSM_FREE, struct rsrc_free_msg *msg);
_dispatcher(ULONG, bmpanim_RSM_SAVE, struct rsrc_save_msg *msg);

/*-------------------------------------------------------------------
**  aus bmpanim_support.c
*/
extern void killBmpanimResource(struct sequence_header *sh);
extern struct sequence_header *createBmpanimResource(UBYTE *loaderclass, UBYTE **fn_pool_array, Pixel2D **ol_pool_array, UWORD num_frames, struct BmpAnimFrame *sequence);
extern struct sequence_header *loadBmpanimResource(UBYTE *rsrc_id, struct IFFHandle *iff);
extern BOOL saveBmpanimResource(struct sequence_header *sh, UBYTE *rsrc_id, struct IFFHandle *iff);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG bmpanim_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG bmpanim_Methods[NUCLEUS_NUMMETHODS];
#endif


struct ClassInfo bmpanim_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeBmpanimClass(ULONG id,...);
__geta4 BOOL FreeBmpanimClass(void);
#else
struct ClassInfo *MakeBmpanimClass(ULONG id,...);
BOOL FreeBmpanimClass(void);
#endif

struct GET_Class bmpanim_GET = {
    &MakeBmpanimClass,
    &FreeBmpanimClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&bmpanim_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *bmpanim_Entry(void)
{
    return(&bmpanim_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo bmpanim_class_seg = {
    { NULL, NULL,                   /* ln_Succ, ln_Pred */
      0, 0,                         /* ln_Type, ln_Pri  */
      "MC2classes:bmpanim.class"    /* der Segment-Name */
    },
    bmpanim_Entry,                  /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeBmpanimClass(ULONG id,...)
#else
struct ClassInfo *MakeBmpanimClass(ULONG id,...)
#endif
/*
**  FUNCTION
**      MakeExternClass()-Einsprung für bmpanim.class
**
**  INPUTS
**      Folgende Tags werden gescannt:
**          MID_NUCLEUS
**
**  RESULTS
**      Class * -> Pointer auf ausgefüllte ClassInfo-Struktur
**
**  CHANGED
**      25-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Methoden-Array initialisieren */
    memset(bmpanim_Methods,0,sizeof(bmpanim_Methods));

    bmpanim_Methods[OM_NEW]         = (ULONG) bmpanim_OM_NEW;
    bmpanim_Methods[OM_DISPOSE]     = (ULONG) bmpanim_OM_DISPOSE;
    bmpanim_Methods[OM_GET]         = (ULONG) bmpanim_OM_GET;
    bmpanim_Methods[OM_SET]         = (ULONG) bmpanim_OM_SET;
    bmpanim_Methods[OM_NEWFROMIFF]  = (ULONG) bmpanim_OM_NEWFROMIFF;
    bmpanim_Methods[OM_SAVETOIFF]   = (ULONG) bmpanim_OM_SAVETOIFF;

    bmpanim_Methods[RSM_CREATE]     = (ULONG) bmpanim_RSM_CREATE;
    bmpanim_Methods[RSM_FREE]       = (ULONG) bmpanim_RSM_FREE;
    bmpanim_Methods[RSM_SAVE]       = (ULONG) bmpanim_RSM_SAVE;

    bmpanim_Methods[BMM_BMPOBTAIN]  = (ULONG) bmpanim_BMM_BMPOBTAIN;

    /* ClassInfo-Struktur ausfüllen */
    bmpanim_clinfo.superclassid = BITMAP_CLASSID;
    bmpanim_clinfo.methods      = bmpanim_Methods;
    bmpanim_clinfo.instsize     = sizeof(struct bmpanim_data);
    bmpanim_clinfo.flags        = 0;

    /* und das war's... */
    return(&bmpanim_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeBmpanimClass(void)
#else
BOOL FreeBmpanimClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**      Zur Zeit immer TRUE.
**
**  CHANGED
**      25-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
*/
{
    return(TRUE);
}

/*******************************************************************/
/***    AB HIER METHODEN DISPATCHER     ****************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, bmpanim_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      25-Oct-94   floh    created + debugged
**      28-Oct-94   floh    Falls das neue resource.class Attribut
**                          RA_FromDiskRsrc TRUE ist, wird der
**                          Sequence-Header nicht aus den übergebenen
**                          Attributen erzeugt, sondern mittels
**                          loadBmpanimResource(BANIMA_AnimID) versucht,
**                          diese aus einem Diskresource-File zu laden.
**                          Der OM_NEWFROMIFF-Dispatcher wendet diese
**                          Methode zum Beispiel an!
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    Object *newo;
    struct bmpanim_data *bmad;
    struct TagItem *ti;

    /*** BANIMA_AnimID wandeln in RSA_Name ***/
    ti = _FindTagItem(BANIMA_AnimID, attrs);
    if (ti) ti->ti_Tag = RSA_Name;

    /*** Objekt erzeugen, dabei die bitmap.class ignorieren ***/
    newo = (Object *) _supermethoda(cl->superclass,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    bmad = INST_DATA(cl,newo);

    /*** LID Initialisierung ***/
    _get(newo, RSA_Handle, &(bmad->SeqHeader));
    bmad->AnimType = (WORD) _GetTagData(BANIMA_AnimType,ANIMTYPE_CYCLE,attrs);
    bmad->FrameAdder        = 1;
    bmad->FrameTimeOverflow = 0;
    bmad->ActFrame          = bmad->SeqHeader->sequence;

    /*** das war's ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, bmpanim_OM_DISPOSE, void *ignored)
/*
**  CHANGED
**      14-Jan-96   floh    created
*/
{
    /*** die bitmap.class wird geflissentlich ignoriert ***/
    return((BOOL)_supermethoda(cl->superclass,o,OM_DISPOSE,0));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bmpanim_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      29-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    BMA_Outline liefert jetzt immer NULL
**                          zurück, wegen neuem internen VFMOutline-Format
**      12-Jan-96           revised & updated
*/
{
    struct bmpanim_data *bmad = INST_DATA(cl,o);
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

            /* bitmap.class Attribute */
            switch(tag) {
                /*** Bitmap-Sachen müssen "vertuscht" werden ***/
                case BMA_Bitmap:    
                    *value = (ULONG) bmad->ActFrame->bitmap;
                    break;

                case BMA_Outline:
                case BMA_Width:
                case BMA_Height:
                case BMA_Type:
                case BMA_Body:
                    *value = NULL; break;
            };

            /* bmpanim.class Attribute */
            switch(tag) {
                case BANIMA_AnimID:
                    _get(o, RSA_Name, value);
                    break;

                case BANIMA_LoaderClass:
                    *value = (ULONG) bmad->SeqHeader->loader_class;
                    break;

                case BANIMA_NumFrames:
                    *value = (ULONG) bmad->SeqHeader->num_frames;
                    break;

                case BANIMA_AnimType:
                    *value = (ULONG) bmad->AnimType;
                    break;
            };
        };
    };

    /* bitmap.class überspringen!!! */
    _supermethoda(cl->superclass,o,OM_GET,store_attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bmpanim_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      29-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**                          war auch noch ein kleiner Bug drin
**      12-Jan-96   floh    revised & updated
*/
{
    struct bmpanim_data *bmad = INST_DATA(cl,o);
    struct TagItem *ti;

    /* BANIMA_AnimType */
    if (ti = _FindTagItem(BANIMA_AnimType,attrs)) {
        bmad->AnimType = ti->ti_Data;
    };

    /* bitmap.class überspringen! */
    _supermethoda(cl->superclass,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, bmpanim_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      28-Oct-94   floh    created
**      30-Oct-94   floh    BANIMA_AnimType wurde zwar korrekt
**                          gelesen, aber nicht in die
**                          OM_NEW-Attribut-Liste eingebaut
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;
    struct ContextNode *cn;
    ULONG ifferror;
    Object *newo = NULL;

    /* Variablen die beim Laden initialisiert werden... */
    struct banim_IFFAttr *iffattr;

    UWORD iffattr_version = 0;
    UBYTE iffattr_chunk[256];
    UWORD anim_type;
    UBYTE *animID;

    /* I am parsing... */
    while ((ifferror = _ParseIFF(iff,IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* anderen Fehler als EndOfChunk abfangen */
        if (ifferror) return(NULL);

        cn = _CurrentChunk(iff);

        /* IFFAttr-Chunk? */
        if (cn->cn_ID == BANIMIFF_IFFAttr) {

            /* lese IFFAttr-Chunk */
            _ReadChunkBytes(iff,iffattr_chunk,sizeof(iffattr_chunk));

            iffattr = (struct banim_IFFAttr *) iffattr_chunk;

            /* Endian-Konvertierung */
            v2nw(&iffattr->version);
            v2nw(&iffattr->animID_offset);
            v2nw(&iffattr->anim_type);

            iffattr_version = iffattr->version;

            /* lese einzelne Attribute aus Chunk */
            if (iffattr_version >= 1) {
                animID =    &iffattr_chunk[iffattr->animID_offset];
                anim_type = iffattr->anim_type;
            };

            /* EndOfChunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);

    };

    /* erzeuge ein Bmpanim-Object... */
    if (iffattr_version != 0) { /* Chunk wurde geladen... */

        struct TagItem new_attrs[4];
        struct TagItem *na_ptr = new_attrs;

        /* baue (I)-Attribut-Liste für OM_NEW auf */
        na_ptr->ti_Tag    = BANIMA_AnimID;
        na_ptr++->ti_Data = (ULONG) animID;
        na_ptr->ti_Tag    = RSA_FromDiskRsrc;
        na_ptr++->ti_Data = (ULONG) TRUE;
        na_ptr->ti_Tag    = BANIMA_AnimType;
        na_ptr++->ti_Data = (ULONG) anim_type;

        na_ptr->ti_Tag = TAG_DONE;

        /* OM_NEW direkt anwenden, damit Nucleus nicht durcheinanderkommt */
        newo = bmpanim_OM_NEW((Object *)cl,cl,new_attrs);
    };

    /* Ende */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, bmpanim_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  CHANGED
**      28-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
**      12-Jan-96   floh    revised & updated
*/
{
    struct IFFHandle *iff;
    struct bmpanim_data *bmad;
    UBYTE *animID;
    struct banim_IFFAttr iffattr;
    LONG save_res;

    iff  = iffmsg->iffhandle;
    bmad = INST_DATA(cl,o);

    _get(o, RSA_Name, &animID);
    if (!animID) return(FALSE);

    /*** Anim-Resource jedesmal in Standalone-File schreiben ***/
    save_res = _method(o, RSM_SAVE, (ULONG) animID, NULL, RSRC_SAVE_STANDALONE);
    if (save_res != RSRC_SAVE_STANDALONE) {
        _LogMsg("bmpanim.class/OM_SAVETOIFF: couldn't save resource.\n");
        return(FALSE);
    };

    /*** eigenen FORM schreiben ***/
    if (_PushChunk(iff, BANIMIFF_FORMID, ID_FORM, IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /*** Methode NICHT an Superclass hochreichen */

    /* IFFAttr-Chunk schreiben */
    _PushChunk(iff, 0, BANIMIFF_IFFAttr, IFFSIZE_UNKNOWN);

    /* IFFAttr-Struktur ausfüllen, endian-konv. und nach IFFAttrs-Chunk */
    iffattr.version       = BANIMIFF_VERSION;
    iffattr.animID_offset = sizeof(struct banim_IFFAttr);
    iffattr.anim_type     = bmad->AnimType;

    n2vw(&iffattr.version);
    n2vw(&iffattr.animID_offset);
    n2vw(&iffattr.anim_type);

    _WriteChunkBytes(iff, &iffattr, sizeof(iffattr));

    /* hänge AnimID-String an IFFAttr-Chunk */
    _WriteChunkBytes(iff,animID,strlen(animID)+1);

    _PopChunk(iff);

    /* BANIMIFF_FORMID Chunk schließen */
    if (_PopChunk(iff) != 0) return(FALSE);

    /* Ende */
    return(TRUE);
}

//=================================================================//
//                                                                 //
//  MODIFIZIERTE rsrc.class METHODEN                               //
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                               //
//=================================================================//

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, bmpanim_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**      Falls zumindest BANIMA_LoaderClass angegeben ist, wird
**      versucht, die Sequenz aus den übergebenen Attributen
**      neu zu erzeugen, andernfalls wird die Resource von
**      Disk geladen erzeugt. Dabei wird wie üblich RSA_IFFHandle
**      ausgewertet, ob aus einem Standalone-File, oder aus einem
**      offenen IFF-Stream gelesen werden soll. Bei Standalone-
**      Files wird auch das "alte" Nicht-IFF Format akzeptiert.
**
**  CHANGED
**      12-Jan-96   floh    created
**      22-Jan-96   floh    die BmpAnim-Resource wird jetzt
**                          mit RSA_Weight = 1 erzeugt, damit
**                          sie garantiert hinter den benutzten
**                          Bitmap-Objects in die Shared Resource Liste
**                          eingeklinkt wird (wegen dem Problem
**                          mit der embed.class, wo die bmpanim.class
**                          Objects aus dem IFF-Chunk geladen werden
**                          sollten, ohne daß die benutzten Bitmap-
**                          Objects bereits vorhanden waren)
*/
{
    struct RsrcNode *rnode;
    struct TagItem more[2];

    /*** RSA_Weight an die Resource-Liste anhängen ***/
    more[0].ti_Tag  = RSA_Weight;
    more[0].ti_Data = 1;
    more[1].ti_Tag  = TAG_MORE;
    more[1].ti_Data = (ULONG) tlist;

    /*** Resource-Node direkt von rsrc.class erzeugen lassen ***/
    rnode = (struct RsrcNode *) _supermethoda(cl->superclass,o,RSM_CREATE,more);
    if (rnode) {

        /*** von Disk laden, oder neu erzeugen? ***/
        UBYTE *loader_class;
        loader_class = (UBYTE *) _GetTagData(BANIMA_LoaderClass, NULL, tlist);
        if (loader_class) {

            /*** neu erzeugen! ***/
            UBYTE **fn_pool_array;
            Pixel2D **ol_pool_array;
            UWORD num_frames;
            struct BmpAnimFrame *sequence;

            fn_pool_array = (UBYTE **)  _GetTagData(BANIMA_FilenamePool,NULL,tlist);
            ol_pool_array = (Pixel2D **)_GetTagData(BANIMA_OutlinePool,NULL,tlist);
            num_frames    = (UWORD)     _GetTagData(BANIMA_NumFrames,0,tlist);
            sequence      = (struct BmpAnimFrame *) _GetTagData(BANIMA_Sequence,NULL,tlist);

            /*** alles da, was so gebraucht wird? ***/
            if (fn_pool_array && ol_pool_array && num_frames && sequence) {

                rnode->Handle = createBmpanimResource(loader_class,
                                    fn_pool_array, ol_pool_array, 
                                    num_frames, sequence);
            };

        } else {

            /*** von Disk laden ***/
            UBYTE *name;
            struct IFFHandle *iff;

            name = (UBYTE *)            _GetTagData(RSA_Name, NULL, tlist);
            iff  = (struct IFFHandle *) _GetTagData(RSA_IFFHandle, NULL, tlist);

            /*** zumindest eine Resource-ID MUSS da sein! ***/
            if (name) {
                rnode->Handle = loadBmpanimResource(name,iff);
            };
        };

        /*** trat ein Fehler auf? ***/
        if (NULL == rnode->Handle) {
            _method(o,RSM_FREE,(ULONG)rnode);
            return(NULL);
        };
    };

    /*** Ende ***/
    return(rnode);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bmpanim_RSM_FREE, struct rsrc_free_msg *msg)
/*
**  CHANGED
**      14-Jan-96   floh    created
*/
{
    struct RsrcNode *rnode = msg->rnode;
    struct sequence_header *sh;

    if (sh = rnode->Handle) {

        killBmpanimResource(sh);
        rnode->Handle = NULL;
    };

    /*** bitmap.class ignorieren ***/
    _supermethoda(cl->superclass,o,RSM_FREE,msg);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, bmpanim_RSM_SAVE, struct rsrc_save_msg *msg)
/*
**  CHANGED
**      14-Jan-96   floh    created
*/
{
    struct bmpanim_data *bmad;

    bmad = INST_DATA(cl,o);

    if (bmad->SeqHeader) {

        UBYTE *name = NULL;
        struct IFFHandle *iff = NULL;

        if (msg->type == RSRC_SAVE_STANDALONE) {
            name = msg->name;
            if (!name) return(RSRC_SAVE_ERROR);
        } else {
            iff = msg->iff;
            if (!iff) return(RSRC_SAVE_ERROR);
        };

        if (saveBmpanimResource(bmad->SeqHeader, name, iff)) return(msg->type);
    };
    return(RSRC_SAVE_ERROR);
}

/*******************************************************************/
/***    MODIFIZIERTE BITMAP.CLASS METHODEN *************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
struct internal_frame *forceNextFrame(struct bmpanim_data *bmad)
/*
**  FUNCTION
**      Diese Routine ermittelt pauschal den nächsten
**      Frame in der Sequence (ohne irgendwelche Timing-Sachen)
**      und wird zum Beispiel von BMM_GETBITMAP aufgerufen,
**      falls <frametime_msg.frame_time == -1> ist.
**
**  INPUTS
**      bmad    -> Pointer auf LID des Objects
**
**  RESULTS
**      Pointer auf nöchsten Frame, Sequence-Overflows
**      werden korrekt gehandhabt.
**
**  CHANGED
**      29-Oct-94   floh    created
**      21-Jan-95   floh    Nucleus2-Revision
*/
{
    /* hole ActFrame und schalte um einen Frame weiter */
    struct internal_frame *actframe = bmad->ActFrame;
    actframe += bmad->FrameAdder;

    /* Sequence-Over/Underflow? */
    if (actframe == bmad->SeqHeader->endof_sequence) {

        /* Overflow */
        if (bmad->AnimType == ANIMTYPE_CYCLE) {
            actframe = bmad->SeqHeader->sequence;
        } else {
            actframe--;
            bmad->FrameAdder = -1;
        };
    } else if (actframe < bmad->SeqHeader->sequence) {

        /* Underflow */
        actframe++;
        bmad->FrameAdder = +1;
    };

    /* schreibe neuen ActFrame nach LID zurück */
    bmad->ActFrame = actframe;

    /* ...und fertig */
    return(actframe);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, bmpanim_BMM_BMPOBTAIN, struct bmpobtain_msg *msg)
/*
**  FUNCTION
**      Neue Standard-Funktion zur Abfrage von aktueller VFMBitmap
**      und VFMOutline.
**
**  INPUTS
**      msg.time_stamp  -> globaler TimeStamp des aktuellen Frames
**      msg.frame_time  -> FrameTime des aktuellen Frames
**
**      Sonderfälle:
**          msg.frame_time == 0  -> aktueller Frame bleibt erhalten
**          msg.frame_time == -1 -> nächster Frame wird aktiviert, ohne
**                                  Time-Sync
**
**  RESULTS
**      msg.bitmap      -> Ptr auf aktuelle VFMBitmap-Struktur
**      msg.outline     -> optionale aktuelle Outline
**
**  CHANGED
**      07-Jul-95   floh    created
*/
{
    struct bmpanim_data *bmad;
    LONG overflow, new_overflow;
    struct sequence_header *sh;
    struct internal_frame *actframe;

    bmad = INST_DATA(cl,o);

    /*** ForceNextFrame? ***/
    if (msg->frame_time == -1) {
        actframe = forceNextFrame(bmad);
        msg->bitmap  = actframe->bitmap;
        msg->outline = actframe->outline;

    } else {

        /*** hiermit wird verhindert, daß Sync-Probleme auftauchen, ***/
        /*** wenn BMM_BMPOBTAIN pro Frame mehrmals auf ein Object   ***/
        /*** angewendet wird. ***/
        if (msg->time_stamp == bmad->TimeStamp) {

            msg->bitmap  = bmad->ActFrame->bitmap;
            msg->outline = bmad->ActFrame->outline;

        } else {

            /*** sonst normal den nächsten Frame ermitteln... ***/
            bmad->TimeStamp = msg->time_stamp;

            overflow = bmad->FrameTimeOverflow + msg->frame_time;
            sh       = bmad->SeqHeader;
            actframe = bmad->ActFrame;

            while ((new_overflow = overflow - actframe->frame_time) >= 0) {

                actframe += bmad->FrameAdder;
                overflow  = new_overflow;

                /* Sequence-Over/Underflow? */
                if (actframe == sh->endof_sequence) {

                    /* Overflow */
                    if (bmad->AnimType == ANIMTYPE_CYCLE) {
                        actframe = sh->sequence;
                    } else {
                        actframe--;
                        bmad->FrameAdder = -1;
                    };

                } else if (actframe < sh->sequence) {

                    /* Underflow */
                    actframe++;
                    bmad->FrameAdder = +1;
                };
            };

            /* neue Frame-Parameter nach LID */
            bmad->ActFrame          = actframe;
            bmad->FrameTimeOverflow = overflow;

            /* msg ausfüllen */
            msg->bitmap  = actframe->bitmap;
            msg->outline = actframe->outline;
        };
    };

    /*** Ende ***/
}

