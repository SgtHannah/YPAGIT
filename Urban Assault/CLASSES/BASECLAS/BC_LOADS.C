/*
**  $Source: PRG:VFM/Classes/_BaseClass/bc_loadsave.c,v $
**  $Revision: 38.10 $
**  $Date: 1996/01/22 22:17:13 $
**  $Locker:  $
**  $Author: floh $
**
**  OM_SAVETOIFF und OM_NEWFROMIFF-Dispatcher der base.class.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "engine/engine.h"
#include "types.h"

#include "resources/embedclass.h"
#include "baseclass/baseclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine

extern ULONG base_Id;

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, base_OM_SAVETOIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      13-Dec-94   floh    created
**      20-Jan-95   floh    Nucleus2-Revision
**      30-Jun-95   floh    neues Handling für AmbientLight und
**                          DFade-Stuff.
**      24-Sep-95   floh    alle Kollisions-Sachen unschädlich gemacht
**      22-Jan-96   floh    + Resource-Embedding
*/
{
    struct base_IFFAttr iffattr;    /* für BASEIFF_IFFAttr-Chunk */
    struct base_data *bd  = INST_DATA(cl,o);
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* eigenen FORM schreiben */
    if (_PushChunk(iff,BASEIFF_FORMID,ID_FORM,IFFSIZE_UNKNOWN)!=0)
        return(FALSE);

    /* Methode an SuperClass hochreichen */
    if (!_supermethoda(cl,o,OM_SAVETOIFF,(Msg *)iffmsg)) return(FALSE);

    /* Resource-Embedding? */
    if (bd->Flags & BSF_EmbedRsrc) {

        /* temporäres embed.class Object erzeugen */
        Object *eo = _new("embed.class", TAG_DONE);
        if (eo) {
            if (!_SaveInnerObject(eo,iff)) {
                _dispose(eo);
                return(FALSE);
            };
            _dispose(eo);
        };
    };

    /* BASEIFF_IFFAttr-Chunk erzeugen */
    _PushChunk(iff,0,BASEIFF_IFFAttr,IFFSIZE_UNKNOWN);

    /* iffattr-Struktur ausfüllen + Endian-Konvertierung */
    iffattr.version = BASEIFF_VERSION;
    n2vw(&iffattr.version);

    iffattr.attr_flags = 0;
    if (bd->Flags & BSF_MoveMe)             iffattr.attr_flags |= IAF_MoveMe;
    if (bd->Flags & BSF_RotateMe)           iffattr.attr_flags |= IAF_RotateMe;
    if (bd->Flags & BSF_MainObject)         iffattr.attr_flags |= IAF_MainObject;
    if (bd->Flags & BSF_PublishAll)         iffattr.attr_flags |= IAF_PublishAll;
    if (bd->Flags & BSF_TerminateCollision) iffattr.attr_flags |= IAF_TermCol;
    if (bd->Flags & BSF_HandleInput)        iffattr.attr_flags |= IAF_HandleInput;
    if (bd->TForm.flags & TFF_FollowMother) iffattr.attr_flags |= IAF_FollowMother;
    n2vw(&iffattr.attr_flags);

    iffattr.x = bd->TForm.loc.x;    n2vl(&iffattr.x);
    iffattr.y = bd->TForm.loc.y;    n2vl(&iffattr.y);
    iffattr.z = bd->TForm.loc.z;    n2vl(&iffattr.z);

    iffattr.vx = bd->TForm.vec.x;   n2vl(&iffattr.vx);
    iffattr.vy = bd->TForm.vec.y;   n2vl(&iffattr.vy);
    iffattr.vz = bd->TForm.vec.z;   n2vl(&iffattr.vz);

    iffattr.sx = bd->TForm.scl.x;   n2vl(&iffattr.sx);
    iffattr.sy = bd->TForm.scl.y;   n2vl(&iffattr.sy);
    iffattr.sz = bd->TForm.scl.z;   n2vl(&iffattr.sz);

    iffattr.ax = (WORD) (bd->TForm.ang.x >> 16);    n2vw(&iffattr.ax);
    iffattr.ay = (WORD) (bd->TForm.ang.y >> 16);    n2vw(&iffattr.ay);
    iffattr.az = (WORD) (bd->TForm.ang.z >> 16);    n2vw(&iffattr.az);

    iffattr.rx = (WORD) (bd->TForm.rot.x >> 6);     n2vw(&iffattr.rx);
    iffattr.ry = (WORD) (bd->TForm.rot.y >> 6);     n2vw(&iffattr.ry);
    iffattr.rz = (WORD) (bd->TForm.rot.z >> 6);     n2vw(&iffattr.rz);

    iffattr.col_mode  = 0;                        n2vw(&iffattr.col_mode);
    iffattr.vis_limit = bd->PubMsg.dfade_start +
                        bd->PubMsg.dfade_length;  n2vl(&iffattr.vis_limit);
    iffattr.ambience  = bd->PubMsg.ambient_light; n2vl(&iffattr.ambience);

    /* IFFAttr-Struktur komplett, also auf Disk damit... */
    _WriteChunkBytes(iff,&iffattr,sizeof(iffattr));
    _PopChunk(iff);

    /* Skeleton-Object vorhanden? */
    if (bd->Skeleton) {

        /* dann als eingebettetes Object schreiben... */
        if (!_SaveInnerObject(bd->Skeleton,iff)) return(FALSE);

        /* ADE FORM nur schreiben, wenn ADE-Liste nicht leer */
        if (bd->ADElist.mlh_Head->mln_Succ) {

            struct MinNode *nd;

            _PushChunk(iff,BASEIFF_ADE,ID_FORM,IFFSIZE_UNKNOWN);

            /* und die ganzen ADEs... */
            for (nd=bd->ADElist.mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
                Object *ade = ((struct ObjectNode *)nd)->Object;
                if (!_SaveInnerObject(ade,iff)) return(FALSE);
            };

            /* schließe BASEIFF_ADE-FORM */
            _PopChunk(iff);
        };
    };

    /* falls ChildList nicht leer, BASEIFF_Children FORM schreiben */
    if (bd->ChildList.mlh_Head->mln_Succ) {

        struct MinNode *nd;

        _PushChunk(iff,BASEIFF_Children,ID_FORM,IFFSIZE_UNKNOWN);

        /* here we go... */
        for (nd=bd->ChildList.mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            Object *child = ((struct ObjectNode *)nd)->Object;
            if (!_SaveInnerObject(child,iff)) return(FALSE);
        };

        /* schließe BASEIFF_Children FORM */
        _PopChunk(iff);
    };

    /* das war's, also äußerste FORM schließen */
    if (_PopChunk(iff) != 0) return(FALSE);

    /* Ende */
    return(TRUE);
}

/*=================================================================**
**  SUPPORT ROUTINEN FUER OM_NEWFROMIFF                            **
**=================================================================*/

BOOL handle_IFFAttr(Object *newo,
                    struct base_data *bd,
                    struct IFFHandle *iff)
{
    if (newo) {

        struct base_IFFAttr iffattr;
        struct flt_vector_msg flt_vec;
        struct lng_vector_msg lng_vec;
        struct TagItem tags[10];        /* VORSICHT! nicht überschreiben! */
        ULONG *tag_ptr = (ULONG *) tags;

        /* Chunk einlesen */
        if (!_ReadChunkBytes(iff, &iffattr, sizeof(iffattr))) return(FALSE);

        /* Endian-Konvertierung */
        v2nw(&iffattr.version);

        v2nl(&iffattr.x);  v2nl(&iffattr.y);  v2nl(&iffattr.z);
        v2nl(&iffattr.vx); v2nl(&iffattr.vy); v2nl(&iffattr.vz);
        v2nl(&iffattr.sx); v2nl(&iffattr.sy); v2nl(&iffattr.sz);
        v2nw(&iffattr.ax); v2nw(&iffattr.ay); v2nw(&iffattr.az);
        v2nw(&iffattr.rx); v2nw(&iffattr.ry); v2nw(&iffattr.rz);

        v2nw(&iffattr.attr_flags);
        v2nw(&iffattr.col_mode);
        v2nl(&iffattr.vis_limit);
        v2nl(&iffattr.ambience);

        /* TagList initialisieren */
        if (iffattr.version >= 1) {
            ULONG flg = iffattr.attr_flags;

            /* OM_SET-TagArray auffüllen */
            *tag_ptr++ = BSA_PublishAll;
            *tag_ptr++ = (ULONG) (flg & IAF_PublishAll) ? TRUE:FALSE;
            *tag_ptr++ = BSA_TerminateCollision;
            *tag_ptr++ = (ULONG) (flg & IAF_TermCol) ? TRUE:FALSE;
            *tag_ptr++ = BSA_HandleInput;
            *tag_ptr++ = (ULONG) (flg & IAF_HandleInput) ? TRUE:FALSE;
            *tag_ptr++ = BSA_FollowMother;
            *tag_ptr++ = (ULONG) (flg & IAF_FollowMother) ? TRUE:FALSE;
            *tag_ptr++ = BSA_CollisionMode;
            *tag_ptr++ = (ULONG) iffattr.col_mode;
            *tag_ptr++ = BSA_VisLimit;
            *tag_ptr++ = (ULONG) iffattr.vis_limit;
            *tag_ptr++ = BSA_AmbientLight;
            *tag_ptr++ = (ULONG) iffattr.ambience;

            /* bissel was muß per direkter Methode init. werden... */

            /* Am I the MainObject? */
            if (flg & IAF_MainObject) {
                _method(newo,BSM_MAIN,NULL,NULL);
            };

            /* Positions-/Richtungs-/Winkel-Daten etc. */
            flt_vec.mask = VEC_X|VEC_Y|VEC_Z;
            flt_vec.x = iffattr.x;
            flt_vec.y = iffattr.y;
            flt_vec.z = iffattr.z;
            _methoda(newo,BSM_POSITION,(Msg *)&flt_vec);
            flt_vec.x = iffattr.sx;
            flt_vec.y = iffattr.sy;
            flt_vec.z = iffattr.sz;
            _methoda(newo,BSM_SCALE,(Msg *)&flt_vec);
            if (flg & IAF_MoveMe) {
                flt_vec.x = iffattr.vx;
                flt_vec.y = iffattr.vy;
                flt_vec.z = iffattr.vz;
            } else {
                flt_vec.mask = VEC_FORCE_ZERO;
            };
            _methoda(newo,BSM_VECTOR,(Msg *)&flt_vec);

            lng_vec.mask = VEC_X|VEC_Y|VEC_Z;
            lng_vec.x = (LONG) iffattr.ax;
            lng_vec.y = (LONG) iffattr.ay;
            lng_vec.z = (LONG) iffattr.az;
            _methoda(newo,BSM_ANGLE,(Msg *)&lng_vec);
            if (flg & IAF_RotateMe) {
                lng_vec.x = iffattr.rx;
                lng_vec.y = iffattr.ry;
                lng_vec.z = iffattr.rz;
            } else {
                lng_vec.mask = VEC_FORCE_ZERO;
            };
            _methoda(newo,BSM_ROTATION,(Msg *)&lng_vec);

            /* Ufff, das war's hierfür... */
        }; /* if (iffattr.version >= 1) */

        /* OM_SET-TagArray begrenzen... */
        *tag_ptr = TAG_DONE;

        /* Attribute setzen... */
        _seta(newo,(Msg *)tags);

        return(TRUE);

    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL handle_ADE(Object *newo,
                struct base_data *bd,
                struct IFFHandle *iff)
{
    if (newo) {

        ULONG ifferror;
        struct ContextNode *cn;

        /* eine kleine TagList... */
        struct TagItem tags[2];
        tags[0].ti_Tag = BSA_ADE;
        tags[1].ti_Tag = TAG_DONE;

        /* parse BASEIFF_ADE-FORM-Innereien... */
        while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
            /* andere Fehler als EndOfChunk abfangen */
            if (ifferror) return(FALSE);

            cn = _CurrentChunk(iff);

            /* ADE-OBJT-FORM? */
            if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == FORM_Object)) {

                /* Object lesen und an base.class Object hängen */
                Object *adeo = (Object *) _LoadInnerObject(iff);
                if (!adeo) return(FALSE);

                tags[0].ti_Data = (ULONG) adeo;
                _seta(newo,(Msg *)tags);
                continue;
            };

            /* unbekannte Chunks überspringen */
            _SkipChunk(iff);
        };

        return(TRUE);
    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
BOOL handle_Children(Object *newo,
                     struct base_data *bd,
                     struct IFFHandle *iff)
{
    if (newo) {

        struct addchild_msg ac_msg;

        LONG ifferror;
        struct ContextNode *cn;

        /* parse ADEIFF_Children-FORM-Innereien */
        while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
            /* andere Fehler als EndOfChunk abfangen */
            if (ifferror) return(FALSE);

            cn = _CurrentChunk(iff);

            /* OBJT-FORM? */
            if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == FORM_Object)) {

                /* Object lesen und an base.class Object hängen */
                Object *childo = (Object *) _LoadInnerObject(iff);
                if (!childo) return(FALSE);

                ac_msg.child = childo;
                _methoda(newo,BSM_ADDCHILD,(Msg *)&ac_msg);
                continue;
            };

            /* unbekannte Chunks überspringen */
            _SkipChunk(iff);
        };

        return(TRUE);
    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, base_OM_NEWFROMIFF, struct iff_msg *iffmsg)
/*
**  FUNCTION
**
**  INPUTS
**
**  RESULTS
**
**  CHANGED
**      13-Dec-94   floh    created & debugged
**      21-Dec-94   floh    BSM_MAIN hat eine neue Msg-Struktur, fixed.
**      20-Jan-95   floh    Nucleus2-Revision
**      18-Mar-95   floh    Bug: Absturz, falls Transformer-Engine
**                          nicht geöffnet war... (Aufruf von _RefreshTForm)
**      11-Jul-95   floh    Depthfading-Parameter in <PubMsg> werden
**                          jetzt zuerst auf Default-Werte gesetzt.
**      12-Sep-95   floh    initialisiert jetzt <bd->Id>
**      24-Sep-95   floh    no more collisions
**      22-Jan-96   floh    akzeptiert jetzt optionales
**                          embed.class Object.
*/
{
    struct IFFHandle *iff = iffmsg->iffhandle;

    /* File parsen */
    Object *newo = NULL;
    struct base_data *bd = NULL;
    ULONG ifferror;
    struct ContextNode *cn;
    BOOL iffattrs_done = FALSE;     // siehe OBJT FORM Handling

    while ((ifferror = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {
        /* andere Fehler als EndOfChunk abfangen */
        if (ifferror) {
            if (newo) _methoda(newo,OM_DISPOSE,NULL);
            return(NULL);
        };

        cn = _CurrentChunk(iff);

        /* FORM der SuperClass ? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == OF_NUCLEUS)) {
            newo = (Object *) _supermethoda(cl,o,OM_NEWFROMIFF,(Msg *)iffmsg);
            if (!newo) return(NULL);

            bd = INST_DATA(cl,newo);

            /* Locale Instance Data initialisieren (siehe OM_NEW) */
            _NewList((struct List *) &(bd->ADElist));
            _NewList((struct List *) &(bd->ChildList));

            bd->Id = base_Id++;

            bd->ChildNode.Object = newo;

            bd->TForm.scl.x = 1.0;
            bd->TForm.scl.y = 1.0;
            bd->TForm.scl.z = 1.0;

            #ifdef DYNAMIC_LINKING
                if (TE_GET_SPEC) _RefreshTForm(&(bd->TForm));
            #else
                _RefreshTForm(&(bd->TForm));
            #endif

            bd->VisLimit = BSA_VisLimit_DEF;
            bd->PubMsg.dfade_start  = BSA_VisLimit_DEF - BSA_DFadeLength_DEF;
            bd->PubMsg.dfade_length = BSA_DFadeLength_DEF;

            continue;
        };

        /* BASEIFF_IFFAttr-Chunk? */
        if (cn->cn_ID == BASEIFF_IFFAttr) {

            iffattrs_done = TRUE;

            if (!handle_IFFAttr(newo,bd,iff)) {
                _methoda(newo,OM_DISPOSE,NULL);
                return(NULL);
            };

            /* End Of Chunk holen */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* OBJT-FORM ? vor dem IFFAttrs-Chunk ein embed.class Object, */
        /* sonst ein Skeleton-Object                                  */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == FORM_Object)) {

            if (newo) {
                if (iffattrs_done) {
                    /*** das Skeleton ***/
                    Object *sklto;
                    sklto = (Object *) _LoadInnerObject(iff);
                    if (!sklto) {
                        _methoda(newo,OM_DISPOSE,NULL);
                        return(NULL);
                    };
                    _set(newo, BSA_Skeleton, sklto);
                } else {
                    /*** das embed.class Object lebt bis zum Tod des ***/
                    /*** base.class Objects weiter...                ***/
                    bd->EmbedObj = _LoadInnerObject(iff);
                };
            };

            /* KEIN EndOfChunk... */
            continue;
        };

        /* BASEIFF_ADE-FORM? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == BASEIFF_ADE)) {

            if (!handle_ADE(newo,bd,iff)) {
                _methoda(newo,OM_DISPOSE,NULL);
                return(NULL);
            };

            /* KEIN EndOfChunk... */
            continue;
        };

        /* BASEIFF_Children-FORM? */
        if ((cn->cn_ID == ID_FORM) && (cn->cn_Type == BASEIFF_Children)) {

            if (!handle_Children(newo,bd,iff)) {
                _methoda(newo,OM_DISPOSE,NULL);
                return(NULL);
            };

            /* KEIN EndOfChunk... */
            continue;
        };

        /* unbekannte Chunks überspringen */
        _SkipChunk(iff);

        /* Ende der Parse-Schleife */
    };

    /* das war's auch schon */
    return(newo);
}

