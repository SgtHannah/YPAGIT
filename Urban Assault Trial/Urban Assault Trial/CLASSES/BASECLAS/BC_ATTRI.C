/*
**  $Source: PRG:VFM/Classes/_BaseClass/bc_attrib.c,v $
**  $Revision: 38.18 $
**  $Date: 1996/11/10 21:02:30 $
**  $Locker:  $
**  $Author: floh $
**
**  Routinen zum (schnellen) Initialisieren, Setzen und
**  Getten der base.class Attribute.
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include "nucleus/nucleus2.h"
#include "baseclass/baseclass.h"
#include "ade/ade_class.h"
#include "skeleton/skeletonclass.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus

/*-------------------------------------------------------------------
**  aus bc_trigger.c
*/
#ifdef AMIGA
extern __far struct pubstack_entry PubStack[];
extern __far UBYTE ArgStack[];
#else
extern struct pubstack_entry PubStack[];
extern UBYTE ArgStack[];
#endif

extern UBYTE *EndOf_ArgStack;

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/
/*-----------------------------------------------------------------*/
BOOL base_initAttributes(Object *o, struct base_data *bd, struct TagItem *attrs)
/*
**  FUNCTION
**      Initialisiert LID mit den Default-Attribut-Values und
**      scannt danach die übergebene TagList für "Overriders".
**      Dabei wird nicht mit GetTagData() gearbeitet, sondern
**      eine eigene (viel schnellere) Methode verwendet.
**
**  INPUTS
**      o    -> Pointer auf Object
**      bd   -> Pointer auf dessen LID
**      attr -> Pointer auf (I) Attribut-Liste
**
**  RESULTS
**      TRUE  -> alles OK
**      FALSE -> Ooops...
**
**  CHANGED
**      11-Dec-94   floh    created
**      12-Dec-94   floh    neu: BSA_HandleInput
**      21-Dec-94   floh    Ooops, Default von BSA_HandleInput wurde
**                          nicht richtig initialisiert.
**      19-Jan-95   floh    Nucleus2-Revision
**      30-Jun-95   floh    Neues DFade- und Ambient-Light-Handling,
**                          allerdings backwards-kompatibel. Neue Attribute
**                          täten not, nur zur Zeit keine solche... ;-)
**      11-Jul-95   floh    + BSA_VisLimit
**                          + BSA_DFadeLength
**      24-Sep-95   floh    + BSA_Static
**                          - BSA_TerminateCollision
**                          - BSA_CollisionMode
**      17-Jan-96   floh    revised & updated
**      20-Jan-96   floh    Ooops, bd->VisLimit wurde nicht mit
**                          Default-Wert initialisiert...
**      22-Jan-96   floh    + BSA_EmbedRsrc
*/
{
    register ULONG tag;
    Object *ade;

    /* zuerst mal die Default-Attribute eintragen */
    if (BSA_FollowMother_DEF)       bd->TForm.flags |= TFF_FollowMother;
    if (BSA_PublishAll_DEF)         bd->Flags |= BSF_PublishAll;
    if (BSA_HandleInput_DEF)        bd->Flags |= BSF_HandleInput;

    bd->VisLimit = BSA_VisLimit_DEF;

    bd->PubMsg.dfade_start   = BSA_VisLimit_DEF - BSA_DFadeLength_DEF;
    bd->PubMsg.dfade_length  = BSA_DFadeLength_DEF;
    bd->PubMsg.ambient_light = BSA_AmbientLight_DEF;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* dann die Attribute */
                switch (tag) {
                    case BSA_Skeleton:
                        if (data) {
                            if (bd->Skeleton) _dispose(bd->Skeleton);
                            bd->Skeleton = (Object *) data;
                            _get(bd->Skeleton, SKLA_Skeleton, &(bd->PubMsg.sklt));
                        };
                        break;

                    case BSA_ADE:
                        /* ADE ankoppeln, weiteres bei PostModify... */
                        if (ade = (Object *) data) {
                            _method(ade, ADEM_CONNECT, (ULONG) &(bd->ADElist));
                        };
                        break;

                    case BSA_FollowMother:
                        if (data) bd->TForm.flags |= TFF_FollowMother;
                        else      bd->TForm.flags &= ~TFF_FollowMother;
                        break;

                    case BSA_VisLimit:
                        bd->VisLimit = data;
                        bd->PubMsg.dfade_start  = data - bd->PubMsg.dfade_length;
                        break;

                    case BSA_AmbientLight:
                        bd->PubMsg.ambient_light = data;
                        break;

                    case BSA_PublishAll:
                        if (data) bd->Flags |= BSF_PublishAll;
                        else      bd->Flags &= ~BSF_PublishAll;
                        break;

                    case BSA_HandleInput:
                        if (data) bd->Flags |= BSF_HandleInput;
                        else      bd->Flags &= ~BSF_HandleInput;
                        break;

                    case BSA_DFadeLength:
                        bd->PubMsg.dfade_length = data;
                        bd->PubMsg.dfade_start  = bd->VisLimit - data;
                        break;

                    case BSA_Static:
                        if (data) bd->TForm.flags |= TFF_GlobalizeNoRot;
                        else      bd->TForm.flags &= ~TFF_GlobalizeNoRot;
                        break;

                    case BSA_EmbedRsrc:
                        if (data) bd->Flags |= BSF_EmbedRsrc;
                        else      bd->Flags &= ~BSF_EmbedRsrc;
                        break;

                }; /* switch(tag) */
        }; /* switch(tag) */
    }; /* while(attrs->ti_Tag) */

    /* das war's */
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void base_setAttributes(Object *o, struct base_data *bd, struct TagItem *attrs)
/*
**  FUNCTION
**      Setzt die in der Attributliste übergebenen (S)-Attribute.
**      Schneller als frühere Lösungen mit GetTagData().
**
**  INPUTS
**      o    -> Pointer auf Object
**      bd   -> Pointer auf dessen LID
**      attr -> Pointer auf (I) Attribut-Liste
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Dec-94   floh    created
**      12-Dec-94   floh    neu: BSA_HandleInput
**      19-Jan-95   floh    Nucleus2-Revision
**      30-Jun-95   floh    Neues DFade- und Ambient-Light-Handling,
**                          allerdings backwards-kompatibel. Neue Attribute
**                          täten not, nur zur Zeit keine solche... ;-)
**      11-Jul-95   floh    + BSA_VisLimit
**                          + BSA_DFadeLength
**      24-Sep-95   floh    + BSA_Static
**                          - BSA_TerminateCollision
**                          - BSA_CollisionMode
**      17-Jan-96   floh    revised & updated
**      22-Jan-96   floh    + BSA_EmbedRsrc
*/
{
    register ULONG tag;
    Object *ade;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG data = attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs = (struct TagItem *) data; break;
            case TAG_SKIP:      attrs += data; break;
            default:

                /* dann die eigentlichen Attribute, schön nacheinander */
                switch (tag) {

                    case BSA_Skeleton:
                        if (data) {
                            if (bd->Skeleton) _dispose(bd->Skeleton);
                            bd->Skeleton = (Object *) data;
                            _get(bd->Skeleton, SKLA_Skeleton, &(bd->PubMsg.sklt));
                        };
                        break;

                    case BSA_ADE:
                        if (ade = (Object *) data) {
                            /* ADE ankoppeln.. */
                            _method(ade, ADEM_CONNECT, (ULONG) &(bd->ADElist));
                        };
                        break;

                    case BSA_FollowMother:
                        if (data) bd->TForm.flags |= TFF_FollowMother;
                        else      bd->TForm.flags &= ~TFF_FollowMother;
                        break;

                    case BSA_VisLimit:
                        bd->VisLimit = data;
                        bd->PubMsg.dfade_start  = data - bd->PubMsg.dfade_length;
                        break;

                    case BSA_AmbientLight:
                        bd->PubMsg.ambient_light = data;
                        break;

                    case BSA_PublishAll:
                        if (data) bd->Flags |= BSF_PublishAll;
                        else      bd->Flags &= ~BSF_PublishAll;
                        break;

                    case BSA_HandleInput:
                        if (data) bd->Flags |= BSF_HandleInput;
                        else      bd->Flags &= ~BSF_HandleInput;
                        break;

                    case BSA_DFadeLength:
                        bd->PubMsg.dfade_length = data;
                        bd->PubMsg.dfade_start  = bd->VisLimit - data;
                        break;

                    case BSA_Static:
                        if (data) bd->TForm.flags |= TFF_GlobalizeNoRot;
                        else      bd->TForm.flags &= ~TFF_GlobalizeNoRot;
                        break;

                    case BSA_EmbedRsrc:
                        if (data) bd->Flags |= BSF_EmbedRsrc;
                        else      bd->Flags &= ~BSF_EmbedRsrc;
                        break;

                }; /* switch(tag) */
        }; /* switch(tag) */
    }; /* while(attrs->ti_Tag) */

    /* Ende */
}

/*-----------------------------------------------------------------*/
void base_getAttributes(Object *o, struct base_data *bd, struct TagItem *attrs)
/*
**  FUNCTION
**      Schnelle Routine zum Abfragen einer Liste
**      von (G)-Attributen.
**
**  INPUTS
**      o    -> Pointer auf Object
**      bd   -> Pointer auf dessen LID
**      attr -> Pointer auf (I) Attribut-Liste
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Dec-94   floh    created
**      12-Dec-94   floh    neu: BSA_HandleInput
**                               BSA_ADEList
**                               BSA_TForm
**                               BSA_ChildList
**                               BSA_ChildNode
**                               BSA_ColMsg
**                               BSA_PubMsg
**      21-Dec-94   floh    neu: BSA_MainChild  <BOOL>
**                               BSA_MainObject <BOOL>
**      19-Jan-95   floh    Nucleus2-Revision
**      09-May-95   floh    neu: BSA_PubStack
**                               BSA_ArgStack
**                               BSA_EOArgStack
**      30-Jun-95   floh    Neues DFade- und Ambient-Light-Handling,
**                          allerdings backwards-kompatibel. Neue Attribute
**                          täten not, nur zur Zeit keine solche... ;-)
**      11-Jul-95   floh    + BSA_VisLimit
**                          + BSA_DFadeLength
**      24-Sep-95   floh    + BSA_Static
**                          - BSA_CollisionMode
**                          - BSA_TerminateCollision
**                          - BSA_ColMsg
**      09-Nov-95   floh    - BSA_PubStack
**                          + BSA_BTFPubStack
**                          + BSA_FTBPubStack
**      12-Nov-95   floh    - BSA_BTFPubStack
**                          - BSA_FTBPubStack
**                          + BSA_PubStack
**      22-Jan-96   floh    + BSA_EmbedRsrc
*/
{
    register ULONG tag;

    /* Attribut-Liste scannen... */
    while ((tag = attrs->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) attrs++->ti_Data;

        switch (tag) {

            /* erstmal die Sonderfälle... */
            case TAG_IGNORE:    continue;
            case TAG_MORE:      attrs  = (struct TagItem *) value; break;
            case TAG_SKIP:      attrs += (ULONG) value; break;
            default:

                /* dann die eigentlichen Attribute, schön nacheinander */
                switch (tag) {

                    case BSA_Skeleton:
                        *value = (ULONG) bd->Skeleton;
                        break;

                    case BSA_FollowMother:
                        if ((bd->TForm.flags) & TFF_FollowMother)
                            *value = TRUE;
                        else
                            *value = FALSE;
                        break;

                    case BSA_CollisionMode:
                        *value = COLLISION_NONE;
                        break;

                    case BSA_VisLimit:
                        *value = bd->VisLimit;
                        break;

                    case BSA_AmbientLight:
                        *value = bd->PubMsg.ambient_light;
                        break;

                    case BSA_PublishAll:
                        if ((bd->Flags) & BSF_PublishAll)  *value = TRUE;
                        else                               *value = FALSE;
                        break;

                    case BSA_TerminateCollision:
                        *value = FALSE;
                        break;

                    case BSA_HandleInput:
                        if ((bd->Flags) & BSF_HandleInput)
                            *value = TRUE;
                        else
                            *value = FALSE;
                        break;

                    case BSA_X:
                        *(FLOAT *)value = bd->TForm.loc.x;
                        break;

                    case BSA_Y:
                        *(FLOAT *)value = bd->TForm.loc.y;
                        break;

                    case BSA_Z:
                        *(FLOAT *)value = bd->TForm.loc.z;
                        break;

                    case BSA_VX:
                        *(FLOAT *)value = bd->TForm.vec.x;
                        break;

                    case BSA_VY:
                        *(FLOAT *)value = bd->TForm.vec.y;
                        break;

                    case BSA_VZ:
                        *(FLOAT *)value = bd->TForm.vec.z;
                        break;

                    case BSA_AX:
                        *value = (bd->TForm.ang.x) >> 16;
                        break;

                    case BSA_AY:
                        *value = (bd->TForm.ang.y) >> 16;
                        break;

                    case BSA_AZ:
                        *value = (bd->TForm.ang.z) >> 16;
                        break;

                    case BSA_RX:
                        *value = (bd->TForm.rot.x) >> 6;
                        break;

                    case BSA_RY:
                        *value = (bd->TForm.rot.y) >> 6;
                        break;

                    case BSA_RZ:
                        *value = (bd->TForm.rot.z) >> 6;
                        break;

                    case BSA_SX:
                        *(FLOAT *)value = bd->TForm.scl.x;
                        break;

                    case BSA_SY:
                        *(FLOAT *)value = bd->TForm.scl.y;
                        break;

                    case BSA_SZ:
                        *(FLOAT *)value = bd->TForm.scl.z;
                        break;

                    case BSA_ADEList:
                        *value = (ULONG) &(bd->ADElist);
                        break;

                    case BSA_TForm:
                        *value = (ULONG) &(bd->TForm);
                        break;

                    case BSA_ChildList:
                        *value = (ULONG) &(bd->ChildList);
                        break;

                    case BSA_ChildNode:
                        *value = (ULONG) &(bd->ChildNode);
                        break;

                    case BSA_PubMsg:
                        *value = (ULONG) &(bd->PubMsg);
                        break;

                    case BSA_MainChild:
                        *value = (bd->Flags & BSF_MainChild) ? TRUE:FALSE;
                        break;

                    case BSA_MainObject:
                        *value = (bd->Flags & BSF_MainObject) ? TRUE:FALSE;
                        break;

                    case BSA_PubStack:
                        *value = (ULONG) PubStack;
                        break;

                    case BSA_ArgStack:
                        *value = (ULONG) ArgStack;
                        break;

                    case BSA_EOArgStack:
                        *value = (ULONG) EndOf_ArgStack;
                        break;

                    case BSA_DFadeLength:
                        *value = bd->PubMsg.dfade_length;
                        break;

                    case BSA_Static:
                        if ((bd->TForm.flags) & TFF_GlobalizeNoRot)
                            *value = TRUE;
                        else
                            *value = FALSE;
                        break;

                    case BSA_EmbedRsrc:
                        if (bd->Flags & BSF_EmbedRsrc)
                            *value = TRUE;
                        else
                            *value = FALSE;
                        break;

                }; /* switch(tag) */
        }; /* switch(tag) */
    }; /* while(attrs->ti_Tag) */

    /* Ende */
}

