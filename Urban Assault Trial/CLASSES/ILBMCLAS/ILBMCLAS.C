/*
**  $Source: PRG:VFM/Classes/_ILBMClass/ilbmclass.c,v $
**  $Revision: 38.14 $
**  $Date: 1998/01/06 14:51:30 $
**  $Locker: floh $
**  $Author: floh $
**
**  (C) Copyright 1994,1995,1996 by A.Weissflog
*/
#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "types.h"
#include "bitmap/ilbmclass.h"

struct NucleusBase *ilbm_NBase;

#ifdef __WINDOWS__
/*** HACK WARNING *** HACK WARNING *** HACK WARNING ***/
extern unsigned long wdd_DoDirect3D;
extern unsigned long wdd_CanDoAlpha;
extern unsigned long wdd_CanDoAdditiveBlend;
extern unsigned long wdd_CanDoStipple;
#endif

/*-------------------------------------------------------------------
**  Prototypen und Importe
*/
_dispatcher(Object *, ilbm_OM_NEW, struct TagItem *);
_dispatcher(void, ilbm_OM_SET, struct TagItem *);
_dispatcher(void, ilbm_OM_GET, struct TagItem *);
_dispatcher(Object *, ilbm_OM_NEWFROMIFF, struct iff_msg *);
_dispatcher(BOOL, ilbm_OM_SAVETOIFF, struct iff_msg *);
_dispatcher(struct RsrcNode *, ilbm_RSM_CREATE, struct TagItem *);
_dispatcher(ULONG, ilbm_RSM_SAVE, struct rsrc_save_msg *);

extern struct IFFHandle *ilbm_OpenIff(UBYTE *, ULONG);
struct RsrcNode *ilbm_CreateBitmap(Object *, Class *, struct TagItem *, struct IFFHandle *, BOOL);
extern void ilbm_CloseIff(struct IFFHandle *);

BOOL ilbm_SaveAsILBM(Object *o, struct IFFHandle *, struct VFMBitmap *);
BOOL ilbm_SaveAsVBMP(Object *o, struct IFFHandle *, struct VFMBitmap *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG ilbm_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG ilbm_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo ilbm_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeILBMClass(ULONG id,...);
__geta4 BOOL FreeILBMClass(void);
#else
struct ClassInfo *MakeILBMClass(ULONG id,...);
BOOL FreeILBMClass(void);
#endif

struct GET_Class ilbm_GET = {
    &MakeILBMClass,
    &FreeILBMClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
/* spezieller hocheffizienter DICE-Startup-Code */
__geta4 struct GET_Class *start(void)
{
    return(&ilbm_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *ilbm_Entry(void)
{
    return(&ilbm_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo ilbm_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:ilbm.class"   /* der Segment-Name */
    },
    ilbm_Entry,                 /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeILBMClass(ULONG id,...)
#else
struct ClassInfo *MakeILBMClass(ULONG id,...)
#endif
/*
**  CHANGED
**      13-Mar-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
**      10-Oct-96   floh    revised & updated
**      25-Feb-97   floh    + NucleusBase wird importiert
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    _get_nbase((struct TagItem *)&id,&ilbm_NBase);

    /* Methoden-Array initialisieren */
    memset(ilbm_Methods,0,sizeof(ilbm_Methods));

    ilbm_Methods[OM_NEW]        = (ULONG) ilbm_OM_NEW;
    ilbm_Methods[OM_SET]        = (ULONG) ilbm_OM_SET;
    ilbm_Methods[OM_GET]        = (ULONG) ilbm_OM_GET;
    ilbm_Methods[OM_NEWFROMIFF] = (ULONG) ilbm_OM_NEWFROMIFF;
    ilbm_Methods[OM_SAVETOIFF]  = (ULONG) ilbm_OM_SAVETOIFF;
    ilbm_Methods[RSM_CREATE]    = (ULONG) ilbm_RSM_CREATE;
    ilbm_Methods[RSM_SAVE]      = (ULONG) ilbm_RSM_SAVE;

    /* ClassInfo-Struktur ausfüllen */
    ilbm_clinfo.superclassid = BITMAP_CLASSID;
    ilbm_clinfo.methods      = ilbm_Methods;
    ilbm_clinfo.instsize     = sizeof(struct ilbm_data);
    ilbm_clinfo.flags        = 0;

    /* Ende */
    return(&ilbm_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeILBMClass(void)
#else
BOOL FreeILBMClass(void)
#endif
/*
**  FUNCTION
**
**  INPUTS
**      ---
**
**  RESULTS
**      Zur Zeit immer TRUE.
**
**  CHANGED
**      13-Mar-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
*/
{
    return(TRUE);
}

/*******************************************************************/
/***  METHODEN HANDLER  ********************************************/
/*******************************************************************/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ilbm_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      10-Oct-96   floh    created
*/
{
    Object *newo;
    struct ilbm_data *id;
    ULONG data;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);
    id = INST_DATA(cl,newo);

    /*** (I) Attribute ***/
    data = _GetTagData(ILBMA_SaveILBM,FALSE,attrs);
    if (data) id->flags |= ILBMF_SaveILBM;

    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ilbm_OM_SET, struct TagItem *attrs)
/*
**  CHANGED
**      10-Oct-96   floh    created
*/
{
    struct ilbm_data *id = INST_DATA(cl,o);
    struct TagItem *ti;

    if (ti = _FindTagItem(ILBMA_SaveILBM,attrs)) {
        if (ti->ti_Data) id->flags |= ILBMF_SaveILBM;
        else             id->flags &= ~ILBMF_SaveILBM;
    };
    _supermethoda(cl,o,OM_SET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, ilbm_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      10-Oct-96   floh    created
*/
{
    struct ilbm_data *id = INST_DATA(cl,o);
    ULONG *data;

    if (data = (ULONG *) _GetTagData(ILBMA_SaveILBM,NULL,attrs)) {
        *data = (id->flags & ILBMF_SaveILBM) ? TRUE : FALSE;
    };
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, ilbm_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**
**  CHANGED
**      11-Jan-96   floh    created
**      25-Feb-97   floh    BMA_Texture-Hack (alle in IFF-Streams
**                          eingebettete Daten werden auf BMA_Texture
**                          gezwungen, damit die embed.class
**                          Texturen erzeugt [das sind die einzigen
**                          persistenten Image-Objekt... hoffentlich :-)]
**      21-Mar-97   floh    + jetzt mit Filename-Trap-Hack, für
**                            wdd_Direct3D-HiColor-Hack
**      03-Jun-97   floh    + "mapcur64.ilbm" und "lego16.ilbm" getrappt
**      03-Jul-97   floh    + finder.ilbm getrappt
**      04-Mar-98   floh    + neuer Trap-Case: "gamma"
**                          + Hint an CreateBitmap(), daß es sich um eine
**                            getrappte Textur handelt
*/
{
    struct RsrcNode *rnode = NULL;
    UBYTE *name;

    /*** RSA_Name ***MUSS*** vorhanden sein ***/
    name = (UBYTE *) _GetTagData(RSA_Name, NULL, tlist);
    if (name) {

        struct IFFHandle *iff;
        BOOL stand_alone = FALSE;
        struct TagItem more_tags[2];
        UBYTE *trap_name = NULL;

        more_tags[0].ti_Tag  = TAG_MORE;
        more_tags[0].ti_Data = (ULONG) tlist;

        /*** HACK: Filename-Trap ***/
        #ifdef __WINDOWS__
        if (wdd_DoDirect3D) {
            /*** Textur-Traps ***/
            if (wdd_CanDoAdditiveBlend) {
                if      (stricmp(name,"fx1.ilbm")==0) trap_name="hi/alpha/fx1.ilbm";
                else if (stricmp(name,"fx2.ilbm")==0) trap_name="hi/alpha/fx2.ilbm";
                else if (stricmp(name,"fx3.ilbm")==0) trap_name="hi/alpha/fx3.ilbm";
            } else if (wdd_CanDoStipple) {
                if      (stricmp(name,"fx1.ilbm")==0) trap_name="hi/beta/fx1.ilbm";
                else if (stricmp(name,"fx2.ilbm")==0) trap_name="hi/beta/fx2.ilbm";
                else if (stricmp(name,"fx3.ilbm")==0) trap_name="hi/beta/fx3.ilbm";
            } else if (wdd_CanDoAlpha) {
                if      (stricmp(name,"fx1.ilbm")==0) trap_name="hi/gamma/fx1.ilbm";
                else if (stricmp(name,"fx2.ilbm")==0) trap_name="hi/gamma/fx2.ilbm";
                else if (stricmp(name,"fx3.ilbm")==0) trap_name="hi/gamma/fx3.ilbm";
            };
        };
        #endif

        /*** falls kein IFFHandle bereitgestellt, Standalone-File ***/
        iff = (struct IFFHandle *) _GetTagData(RSA_IFFHandle, NULL, tlist);

        /*** BEWARE HEAVY HACKS AREA ***/
        if (trap_name) {
            /*** Spezial-Behandlung für Traps... ***/
            if (iff) {
                /*** IFF-Streams sind immer BMA_Texture (HACK) ***/
                more_tags[0].ti_Tag  = BMA_Texture;
                more_tags[0].ti_Data = TRUE;
                more_tags[1].ti_Tag  = TAG_MORE;
                more_tags[1].ti_Data = (ULONG) tlist;
                /*** alter IFFStream-Chunk muß übersprungen werden...***/
                _ParseIFF(iff,IFFPARSE_RAWSTEP);
                _SkipChunk(iff);
            };
            /*** iff auf Standalone-File umbiegen... ***/
            iff = ilbm_OpenIff(trap_name,IFFF_READ);
            if (!iff) return(NULL);
            stand_alone = TRUE;

        } else if (!iff) {
            /*** normaler Standalone-File ***/
            iff = ilbm_OpenIff(name,IFFF_READ);
            if (!iff) return(NULL);
            stand_alone = TRUE;
        } else {
            /*** HACK: wenn IFF-Stream, auf BMA_Texture zwingen ***/
            more_tags[0].ti_Tag  = BMA_Texture;
            more_tags[0].ti_Data = TRUE;
            more_tags[1].ti_Tag  = TAG_MORE;
            more_tags[1].ti_Data = (ULONG) tlist;
        };

        /*** VFMBitmap erzeugen lassen und ausfüllen... ***/
        rnode = ilbm_CreateBitmap(o,cl,more_tags,iff,trap_name ? TRUE:FALSE);

        /*** falls Standalone-File, IFF-Stream schließen ***/
        if (stand_alone) ilbm_CloseIff(iff);
    };

    /*** das war's bereits ***/
    return(rnode);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, ilbm_RSM_SAVE, struct rsrc_save_msg *msg)
/*
**  FUNCTION
**      Sichert die eingebettete Bitmap-Resource als
**      VBMP IFF file in einen Standalone File oder
**      eine Resource-Collection.
**
**  INPUTS
**      msg->name       - Filename relativ zu mc2resources:
**                        Nur, wenn (type == RSRC_SAVE_STANDALONE)
**      msg->iff        - Offener IFF-Stream.
**                        Nur, wenn (type == RSRC_SAVE_COLLECTION)
**      msg->type       - Gibt an, ob sich das Objekt in einen eigenen
**                        File sichern soll, oder in eine Collection
**                        (RSRC_SAVE_STANDALONE oder RSRC_SAVE_COLLECTION)
**
**  RESULTS
**      RSRC_SAVE_STANDALONE, RSRC_SAVE_COLLECTION oder RSRC_SAVE_ERROR
**
**  CHANGED
**      11-Jan-96   floh    created
**      10-Oct-96   floh    verzweigt jetzt nach ilbm_SaveAsVBMP()
**                          oder ilbm_SaveAsILBM() (je nach Status
**                          von (id->flags & ILBMF_SaveILBM)
*/
{
    struct ilbm_data *id = INST_DATA(cl,o);
    struct IFFHandle *iff;
    struct bmpobtain_msg bob;
    BOOL save_ok;

    /*** IFF-Stream bereitstellen ***/
    if (RSRC_SAVE_STANDALONE == msg->type) {

        /*** einen neuen IFF-File anlegen ***/
        if (msg->name) {
            iff = ilbm_OpenIff(msg->name, IFFF_WRITE);
            if (!iff) return(RSRC_SAVE_ERROR);
        } else return(RSRC_SAVE_ERROR);

    } else {
        iff = msg->iff;
        if (!iff) return(RSRC_SAVE_ERROR);
    };

    /*** eingebettete VFMBitmap holen ***/
    bob.time_stamp = 1;
    bob.frame_time = 1;
    _methoda(o, BMM_BMPOBTAIN, &bob);
    if ((NULL==bob.bitmap)||(NULL==bob.bitmap->Data)) return(RSRC_SAVE_ERROR);

    /*** und saven... ***/
    if (id->flags & ILBMF_SaveILBM) save_ok=ilbm_SaveAsILBM(o,iff,bob.bitmap);
    else                            save_ok=ilbm_SaveAsVBMP(o,iff,bob.bitmap);
    if (!save_ok) return(RSRC_SAVE_ERROR);

    /*** falls Standalone-File, IFF-Stream schließen ***/
    if (RSRC_SAVE_STANDALONE == msg->type) ilbm_CloseIff(iff);

    /*** Ende ***/
    return(msg->type);
}

/*-----------------------------------------------------------------*/
Object *ilbm_readCIBO(Object *o, Class *cl, struct iff_msg *iffmsg)
/*
**  FUNCTION
**      OM_NEWFROMIFF-Handler für neues, platzsparendes
**      ILBM-Chunk-Format.
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      22-Oct-94   floh    created
**      03-Jan-95   floh    Nucleus2-Revision
**      11-Jan-96   floh    revised & updated :-)
**      25-Feb-97   floh    persistente ILBM-Objekte werden generell
**                          als BMA_Texture geladen!
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* Platz für Filename und Outline */
    UBYTE filename[256];
    Pixel2D outline[64];
    BOOL filename_exists = FALSE;
    BOOL outline_exists  = FALSE;
    Object *newo = NULL;

    /* die "üblichen" IFF-Scan-Variablen */
    ULONG ifferror;
    struct ContextNode *cn;

    /* Parse Chunk... */
    while ((ifferror = _ParseIFF(iff,IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror) return(NULL);

        cn = _CurrentChunk(iff);

        /* Name2-Chunk? */
        if (cn->cn_ID == ILBMIFF_NAME2) {
            /* Filename einlesen */
            _ReadChunkBytes(iff, filename, sizeof(filename));
            filename_exists = TRUE;

            /* EndOfChunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* OL2-Chunk? */
        if (cn->cn_ID == ILBMIFF_OL2) {

            /* ein paar temporäre Variablen und Buffer */
            struct ol2_piece temp_ol[64];
            WORD num_elements = (cn->cn_Size)/sizeof(struct ol2_piece);
            WORD i;

            struct ol2_piece *tol_ptr = temp_ol;
            struct Pixel2D *ol_ptr    = outline;

            /* komprimierte Outline einlesen */
            _ReadChunkBytes(iff, temp_ol, sizeof(temp_ol));

            /* komprimierte Outline in "echte" Outline umwandeln */
            for (i=0; i<num_elements; i++) {
                memset(ol_ptr, 0, sizeof(Pixel2D));
                ol_ptr->x = (UWORD) tol_ptr->x; /* von UBYTE */
                ol_ptr->y = (UWORD) tol_ptr->y; /* von UBYTE */
                ol_ptr++;
                tol_ptr++;
            };
            /* letztes Element... */
            memset(ol_ptr, 0, sizeof(Pixel2D));
            ol_ptr->flags   = (WORD) P2DF_Done;

            outline_exists = TRUE;

            /* hole EndOfChunk */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);

        /* Ende der Parse-Schleife */
    };

    /* erzeuge ein ILBM-Object (auf die simple Art :-))) */
    if (filename_exists) {

        struct TagItem new_attrs[5];
        struct TagItem *na_ptr = new_attrs;

        /* baue Attribut-Liste für neues Object */
        na_ptr->ti_Tag    = RSA_Name;
        na_ptr++->ti_Data = (ULONG) filename;
        na_ptr->ti_Tag    = RSA_Access;
        na_ptr++->ti_Data = ACCESS_SHARED;

        /*** HACK! ***/
        na_ptr->ti_Tag    = BMA_Texture;
        na_ptr++->ti_Data = TRUE;

        if (outline_exists) {
            na_ptr->ti_Tag    = BMA_Outline;
            na_ptr++->ti_Data = (ULONG) outline;
        };

        na_ptr->ti_Tag = TAG_DONE;

        //
        // OM_NEW muß direkt ausgeführt werden, damit u.a. Nucleus' Objekt-
        // Counter nicht ducheinander kommt. Weil die ilbm.class aber
        // keinen eigenen OM_NEW Handler hat, dieses:
        //
        newo = (Object *) _supermethoda(cl,o,OM_NEW,new_attrs);
    };

    /* Ende */
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ilbm_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      13-Mar-94   floh    created
**      28-Mar-94   floh    DisposeObject() ersetzt durch OM_DISPOSE
**      22-Oct-94   floh    Lädt jetzt sowohl das "herkömmliche"
**                          Format "IFIM", als auch das maximal
**                          komprimierte "CIBO"-Chunk-Format.
**      03-Jan-95   floh    Nucleus2-Revision
**      11-Jan-96   floh    revised & updated
**      30-May-96   floh    Support für IFIM removed.
*/
{
    /* welches Chunk-Format liegt denn an? */
    struct IFFHandle *iff  = iffmsg->iffhandle;
    struct ContextNode *cn = _CurrentChunk(iff);
    Object *newo = NULL;

    if (cn->cn_Type == ILBMIFF_FORMID2) {
        newo = ilbm_readCIBO(o,cl,iffmsg);
    };
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, ilbm_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      13-Mar-94   floh    created
**      22-Oct-94   floh    Erzeugt ab jetzt ausschließlich das
**                          neue maximal komprimierte CIBO-Format!
**      03-Jan-95   floh    Nucleus2-Revision
**      07-Jul-95   floh    Outline-Save-Code jetzt angepaßt an neues
**                          FLOAT-Outline-Format der bitmap.class
**                          [it's been a hack anyway :-)]
**      11-Jan-96   floh    revised & updated
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;
    UBYTE *filename       = NULL;
    struct VFMOutline *ol;
    struct bmpobtain_msg bob_msg;

    /* hole Outline und ILBM-Filename des Objects */
    _get(o, RSA_Name, &filename);
    bob_msg.time_stamp = 1;
    bob_msg.frame_time = 1;
    _methoda(o, BMM_BMPOBTAIN, &bob_msg);
    ol = bob_msg.outline;

    /*** eigenen FORM schreiben ***/
    if (_PushChunk(iff, ILBMIFF_FORMID2, ID_FORM, IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /*** Methode NICHT an Superclass hochreichen :-) */

    /*** ILBMIFF_NAME2 Chunk schreiben ***/
    _PushChunk(iff, 0, ILBMIFF_NAME2, IFFSIZE_UNKNOWN);

    /* Name nach Chunk schreiben und Chunk poppen */
    _WriteChunkBytes(iff,filename,strlen(filename)+1);
    _PopChunk(iff);


    /*** Outline-Chunk schreiben, falls eine existiert */
    if (ol) {

        /* konvertiere Outline in komprimiertes Format */
        LONG ol_size = 0;
        struct VFMOutline *ol_ptr = ol;
        struct ol2_piece ol2_array[64];
        struct ol2_piece *ol2_ptr = ol2_array;

        while (ol_ptr->x >= 0.0) {

            ol2_ptr->x = (BYTE) (ol_ptr->x * 256.0);
            ol2_ptr->y = (BYTE) (ol_ptr->y * 256.0);
            ol_ptr++;
            ol2_ptr++;
            ol_size += sizeof(struct ol2_piece);
        };

        /* erzeuge Chunk schreibe komprimierte Outline */
        _PushChunk(iff, 0, ILBMIFF_OL2, ol_size);
        _WriteChunkBytes(iff, ol2_array, ol_size);
        _PopChunk(iff);
    };

    /* ILBMIFF_FORMID-Chunk poppen */
    if (_PopChunk(iff) != 0) return(FALSE);

    /* Ende */
    return(TRUE);
}


