/*
**  $Source: PRG:VFM/Classes/_EmbedClass/embedclass.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/01/22 22:19:14 $
**  $Locker:  $
**  $Author: floh $
**
**  Die embed.class bettet Shared Resources in
**  Disk-Objects ein.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "resources/embedclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ebd_OM_NEW, struct TagItem *);
_dispatcher(BOOL, ebd_OM_DISPOSE, void *);
_dispatcher(Object *, ebd_OM_NEWFROMIFF, struct iff_msg *);
_dispatcher(BOOL, ebd_OM_SAVETOIFF, struct iff_msg *);

/*-------------------------------------------------------------------
**  Globals
*/
_use_nucleus

#ifdef AMIGA
__far ULONG ebd_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG ebd_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo ebd_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table des Klassen-Moduls, plus Prototypes
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeEmbedClass(ULONG id,...);
__geta4 BOOL FreeEmbedClass(void);
#else
struct ClassInfo *MakeEmbedClass(ULONG id,...);
BOOL FreeEmbedClass(void);
#endif

struct GET_Class ebd_GET = {
    &MakeEmbedClass,
    &FreeEmbedClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&ebd_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
struct GET_Class *ebd_Entry(void)
{
    return(&ebd_GET);
};

struct SegInfo ebd_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:embed.class"
    },
    ebd_Entry,
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
__geta4 struct ClassInfo *MakeEmbedClass(ULONG id,...)
#else
struct ClassInfo *MakeEmbedClass(ULONG id,...)
#endif
/*
**  CHANGED
**      21-Jan-96   floh    created
*/ 
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *) &id))) return(NULL);
    #endif 

    memset(ebd_Methods,0,sizeof(ebd_Methods));

    ebd_Methods[OM_NEW]         = (ULONG) ebd_OM_NEW;
    ebd_Methods[OM_DISPOSE]     = (ULONG) ebd_OM_DISPOSE;
    ebd_Methods[OM_NEWFROMIFF]  = (ULONG) ebd_OM_NEWFROMIFF;
    ebd_Methods[OM_SAVETOIFF]   = (ULONG) ebd_OM_SAVETOIFF;

    ebd_clinfo.superclassid = NUCLEUSCLASSID;
    ebd_clinfo.methods      = ebd_Methods;
    ebd_clinfo.instsize     = sizeof(struct embed_data);
    ebd_clinfo.flags        = 0;

    return(&ebd_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeEmbedClass(void)
#else
BOOL FreeEmbedClass(void)
#endif
/*
**  CHANGED
**      21-Jan-96   floh    created
*/
{
    return(TRUE);
}   

/*******************************************************************/

/*-----------------------------------------------------------------*/
BOOL ebd_AddObject(struct embed_data *ebd, Object *o)
/*
**  FUNCTION
**      Hängt ein neues Object an die Object-Liste des
**      embed.class Objects. Bei Bedarf wird eine
**      neue allokiert.
**
**  INPUTS
**      ebd     - Ptr auf LID des embed.class Objects
**      o       - Ptr auf ein beliebiges Resource-Object
**
**  RESULTS
**      TRUE    - alles OK
**      FALSE   - no mem
**
**  CHANGED
**      21-Jan-96   floh    created
*/
{
    struct MinList *ls;
    struct EmbedNode *last;
    struct EmbedNode *enode = NULL;

    /*** letzte Node in Liste untersuchen ***/
    ls = &(ebd->object_list);
    last = (struct EmbedNode *) (ls->mlh_TailPred);
    if (last->nd.mln_Pred) {
        /*** ist in letzter Node noch Platz? ***/
        if (last->num_objects < EBC_NUM_NODEOBJECTS) {
            /*** dann wird <enode> dorthin gesetzt ***/
            enode = last;
        };
    };

    /*** falls keine gültige Node, eine neue allokieren ***/
    if (!enode) {
        enode = _AllocVec(sizeof(struct EmbedNode),MEMF_PUBLIC|MEMF_CLEAR);
        if (enode) {
            _AddTail((struct List *)ls, (struct Node *)enode);
        } else {
            return(FALSE);
        };
    };

    /*** und endlich Object-Pointer eintragen ***/
    enode->o_array[enode->num_objects++] = o;

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ebd_OM_NEW, struct TagItem *attrs)
/*
**  FUNCTION
**      siehe "resources/embedclass.h"
**
**  CHANGED
**      21-Jan-96   floh    created
*/
{
    Object *newo;
    struct embed_data *ebd;
    struct MinList *rsrc_list = NULL;
    struct MinNode *nd;
    Object *ro;

    /*** Ptr auf statische Shared Rsrc List besorgen ***/
    ro = _new("rsrc.class", 
              RSA_Name,   "embed",
              RSA_Access, ACCESS_EXCLUSIVE,
              TAG_DONE);
    if (ro) {
        _get(ro, RSA_SharedRsrcList, &rsrc_list);
        _dispose(ro);
    } else return(NULL);

    /*** dann eigenes Objekt aufbauen ***/
    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    ebd = INST_DATA(cl,newo);

    /*** LID initialisieren ***/
    _NewList((struct List *) &(ebd->object_list));

    /*** Resourcen-Liste durchackern und Clients erzeugen ***/
    for (nd=rsrc_list->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct RsrcNode *rnode = (struct RsrcNode *) nd;
        Object *eo;

        eo = _new(rnode->HandlerClass,
                  RSA_Name,   rnode->Node.ln_Name,
                  RSA_Access, ACCESS_SHARED,
                  TAG_DONE);
        if (eo) {
            if (!ebd_AddObject(ebd, eo)) {
                _dispose(eo);
                _methoda(newo, OM_DISPOSE, NULL);
                return(NULL);
            };
        };
    };

    /*** das war's bereits ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, ebd_OM_DISPOSE, void *ignore)
/*
**  FUNCTION
**      siehe "resources/embedclass.h"
**
**  CHANGED
**      21-Jan-96   floh    created
*/
{
    struct embed_data *ebd = INST_DATA(cl,o);
    struct MinList *ls = &(ebd->object_list);

    /*** die gesamte EmbedNode-Liste freigeben ***/
    while (ls->mlh_Head->mln_Succ) {

        struct EmbedNode *enode = (struct EmbedNode *) ls->mlh_Head;
        ULONG i;

        /*** zuerst alle Objects in Node killen ***/
        for (i=0; i<(enode->num_objects); i++) {
            if (enode->o_array[i]) _dispose(enode->o_array[i]);
        };

        /*** dann Node selbst removen und killen ***/
        _Remove((struct Node *)enode);
        _FreeVec(enode);
    };

    /*** das war's ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignore));
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, ebd_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**      Scannt die Object-Liste des embed.class Objects
**      und erzeugt für jedes Objekt einen EMBEDIFF_Rsrc
**      Chunk mit ClassID und ResourceID, danach wird
**      das jeweilige Objekt angewiesen, seine Resource
**      in den DiskObject-IFF-Stream zu schreiben.
**
**  CHANGED
**      21-Jan-96   floh    created
*/
{
    struct embed_data *ebd = INST_DATA(cl,o);
    struct IFFHandle *iff = iffmsg->iffhandle;
    struct MinList *ls;
    struct MinNode *nd;

    /*** eigene FORM schreiben ***/
    if (_PushChunk(iff,EMBEDIFF_FORMID,ID_FORM,IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /*** Methode an SuperClass ***/
    if (!_supermethoda(cl,o,OM_SAVETOIFF,iffmsg)) return(FALSE);

    /*** für jedes Objekt in Embedded-Liste... ***/
    ls = &(ebd->object_list);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct EmbedNode *enode = (struct EmbedNode *) nd;
        ULONG i;

        for (i=0; i<(enode->num_objects); i++) {

            Object *eo = enode->o_array[i];
            if (eo) {

                UBYTE *class_id;
                UBYTE *rsrc_id;
                UBYTE terminator=0;
                struct rsrc_save_msg rs_msg;
                ULONG save_result;

                /*** schreibe EMBEDIFF_Rsrc-Chunk ***/
                _get(eo, OA_ClassName, &class_id);
                _get(eo, RSA_Name, &rsrc_id);

                _PushChunk(iff,0,EMBEDIFF_Rsrc,IFFSIZE_UNKNOWN);
                _WriteChunkBytes(iff,class_id,strlen(class_id)+1);
                _WriteChunkBytes(iff,rsrc_id,strlen(rsrc_id)+1);
                _WriteChunkBytes(iff,&terminator,sizeof(terminator));
                _PopChunk(iff);

                /*** Rsrc-Object, seine Resource in IFF-Stream zu sichern ***/
                rs_msg.name = NULL;
                rs_msg.iff  = iff;
                rs_msg.type = RSRC_SAVE_COLLECTION;
                save_result = _methoda(eo, RSM_SAVE, &rs_msg);
                if (save_result == RSRC_SAVE_ERROR) return(FALSE);
            };
        };
    };

    /*** EMBEDIFF_FORMID poppen ***/
    if (_PopChunk(iff) != 0) return(FALSE);

    /*** Ende ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, ebd_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      21-Jan-96   floh    created
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;

    Object *newo = NULL;
    struct embed_data *ebd = NULL;
    ULONG ifferror;
    struct ContextNode *cn;

    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror) {
            if (newo) _methoda(newo,OM_DISPOSE,NULL);
            return(NULL);
        };

        cn = _CurrentChunk(iff);

        /* FORM der SuperClass? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == OF_NUCLEUS)) {
            newo = (Object *) _supermethoda(cl,o,OM_NEWFROMIFF,iffmsg);
            if (!newo) return(NULL);

            ebd = INST_DATA(cl,newo);

            /* LID initialisieren */
            _NewList((struct List *) &(ebd->object_list));

            continue;
        };

        /* ein EMBEDIFF_Rsrc Chunk? */
        if (cn->cn_ID == EMBEDIFF_Rsrc) {

            UBYTE str_buf[255];
            UBYTE *class_id;
            UBYTE *rsrc_id;
            Object *eo;

            /*** lese den String-Chunk ***/
            _ReadChunkBytes(iff, str_buf, sizeof(str_buf));

            /*** Pointer zuweisen ***/
            class_id = &(str_buf[0]);
            rsrc_id  = &(str_buf[strlen(class_id)+1]);

            /*** hole EndOfChunk ***/
            _ParseIFF(iff,IFFPARSE_RAWSTEP);

            /*** Rsrc-Object aus IFF-Stream erzeugen und einklinken ***/
            eo = _new(class_id,
                      RSA_Name,      rsrc_id,
                      RSA_Access,    ACCESS_SHARED,
                      RSA_IFFHandle, iff,
                      TAG_DONE);
            if (eo) {
                if (!ebd_AddObject(ebd, eo)) {
                    _dispose(eo);
                    _methoda(newo, OM_DISPOSE, NULL);
                    return(NULL);
                };
            };
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);
    };

    /*** Ende ***/
    return(newo);
}

