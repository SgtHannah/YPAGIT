/*
**  $Source: PRG:VFM/Classes/_IWimpClass/iw_main.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/03/03 17:32:11 $
**  $Locker:  $
**  $Author: floh $
**
**  Die iwimp.class ist Subklasse der idev.class und erweitert
**  diese um ein "GUI-Zeigergerät" und Support für 
**  GUI-Input-Handling.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "input/iwimpclass.h"

/*-------------------------------------------------------------------
**  Prototypes
*/
_dispatcher(Object *, iwimp_OM_NEW, struct TagItem *);
_dispatcher(ULONG, iwimp_IWIMPM_HASFOCUS, void *);
_dispatcher(void, iwimp_IWIMPM_ADDCLICKBOX, struct iwimp_clickbox_msg *);
_dispatcher(void, iwimp_IWIMPM_REMCLICKBOX, struct iwimp_clickbox_msg *);
_dispatcher(void, iwimp_IWIMPM_GETCLICKINFO, struct ClickInfo *);

/*-------------------------------------------------------------------
**  Class Header
*/
_use_nucleus

#ifdef AMIGA
__far ULONG iwimp_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG iwimp_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo iwimp_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeIWimpClass(ULONG id,...);
__geta4 BOOL FreeIWimpClass(void);
#else
struct ClassInfo *MakeIWimpClass(ULONG id,...);
BOOL FreeIWimpClass(void);
#endif

struct GET_Class iwimp_GET = {
    &MakeIWimpClass,      /* MakeExternClass() */
    &FreeIWimpClass,      /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&iwimp_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *iwimp_Entry(void)
{
    return(&iwimp_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo iwimp_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:iwimp.class"  /* der Segment-Name */
    },
    iwimp_Entry,                 /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeIWimpClass(ULONG id,...)
#else
struct ClassInfo *MakeIWimpClass(ULONG id,...)
#endif
/*
**  CHANGES
**      19-Feb-96   floh    created
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    memset(iwimp_Methods,0,sizeof(iwimp_Methods));

    iwimp_Methods[OM_NEW]               = (ULONG) iwimp_OM_NEW;
    iwimp_Methods[IWIMPM_HASFOCUS]      = (ULONG) iwimp_IWIMPM_HASFOCUS;
    iwimp_Methods[IWIMPM_ADDCLICKBOX]   = (ULONG) iwimp_IWIMPM_ADDCLICKBOX;
    iwimp_Methods[IWIMPM_REMCLICKBOX]   = (ULONG) iwimp_IWIMPM_REMCLICKBOX;
    iwimp_Methods[IWIMPM_GETCLICKINFO]  = (ULONG) iwimp_IWIMPM_GETCLICKINFO;

    iwimp_clinfo.superclassid = IDEV_CLASSID;
    iwimp_clinfo.methods      = iwimp_Methods;
    iwimp_clinfo.instsize     = sizeof(struct iwimp_data);
    iwimp_clinfo.flags        = 0;

    /* und das war's */
    return(&iwimp_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeIWimpClass(void)
#else
BOOL FreeIWimpClass(void)
#endif
/*
**  CHANGED
**      19-Feb-96   floh    created
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(Object *, iwimp_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      21-Feb-96   floh    created
**      23-Feb-96   floh    debugging...
*/
{
    Object *newo;
    struct iwimp_data *iwd;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    iwd = INST_DATA(cl,newo);

    /*** ClickBox-Liste initialisieren ***/
    _NewList((struct List *) &(iwd->cbox_list));

    /*** Ende ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, iwimp_IWIMPM_HASFOCUS, void *ignored)
/*
**  CHANGED
**      23-Feb-96   floh    created
*/
{
    return(TRUE);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, iwimp_IWIMPM_ADDCLICKBOX, struct iwimp_clickbox_msg *msg)
/*
**  CHANGED
**      21-Feb-96   floh    created
*/
{
    struct iwimp_data *iwd = INST_DATA(cl,o);

    if (msg->flags & IWIMPF_AddHead) {
        _AddHead((struct List *) &(iwd->cbox_list),
                 (struct Node *) msg->cb);
    } else {
        _AddTail((struct List *) &(iwd->cbox_list),
                 (struct Node *) msg->cb);
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, iwimp_IWIMPM_REMCLICKBOX, struct iwimp_clickbox_msg *msg)
/*
**  CHANGED
**      21-Feb-96   floh    created
*/
{
    _Remove((struct Node *) msg->cb);
}

/*-----------------------------------------------------------------*/
void iwimp_GetClickInfo(struct iwimp_data *iwd,
                        struct ClickInfo *ci,
                        struct MouseCoords *mc)
/*
**  FUNCTION
**      Versucht, den Mauskoordinaten in [mc->scrx,mc->scry]
**      einen ClickBox und/oder Button-Treffer zuzuordnen,
**      indem die Mausposition gegen die ClickBox-Liste
**      des Objekts verglichen wird.
**
**      Das Ergebnis wird in die ClickInfo-Struktur geschrieben.
**
**      <mc> muß auf eine der in <ci> eingebetteten <struct MouseCoords>
**      zeigen und wird mit den relativen Treffer-Koordinaten
**      ausgefüllt.
**
**  INPUTS
**      ci  -> Click-Info-Struktur, die mit dem Ergebnis ausgefüllt
**             wird:
**
**              ci->box     Ptr auf getroffene ClickBox, oder NULL
**              ci->btn     Index des getroffenen Buttons in <ci->box>, oder -1
**
**      mc  -> eins von &(ci->act), &(ci->down), &(ci->up),
**             wird mit den relativen Mouse-Koords ausgefüllt
**
**  RESULTS
**      ausgefüllt ClickInfo-Struktur
**
**  CHANGED
**      12-Aug-95   floh    created (InputEngine)
**      21-Feb-96   floh    übernommen nach iwimp.class
*/
{
    struct MinList *ls;
    struct MinNode *nd;

    /*** die Screen-Koordinaten müssen bereits ausgefüllt sein! ***/
    WORD x = mc->scrx;
    WORD y = mc->scry;

    /*** der Mausposition ein Click-Element zuordnen ***/
    ls = (struct MinList *) &(iwd->cbox_list);
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct ClickBox *cbox = (struct ClickBox *)nd;

        mc->boxx = x - cbox->rect.x;
        mc->boxy = y - cbox->rect.y;
        if ((mc->boxx >= 0) && (mc->boxx < cbox->rect.w) &&
            (mc->boxy >= 0) && (mc->boxy < cbox->rect.h))
        {
            ULONG i;

            /*** ClickBox getroffen! ***/
            ci->box = cbox;

            /*** ClickBox auf Button-Treffer untersuchen ***/
            for (i=0; i<cbox->num_buttons; i++) {

                struct ClickButton *btn = cbox->buttons[i];

                mc->btnx = mc->boxx - btn->rect.x;
                mc->btny = mc->boxy - btn->rect.y;
                if ((mc->btnx >= 0) && (mc->btnx < btn->rect.w) &&
                    (mc->btny >= 0) && (mc->btny < btn->rect.h))
                {
                    /*** Button-Treffer! (hier abbrechen) ***/
                    ci->btn = i;
                    break;
                };
            };
            /* falls ClickBox-Treffer, hier abbrechen */
            break;
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, iwimp_IWIMPM_GETCLICKINFO, struct ClickInfo *ci)
/*
**
**  IMPORTANT
**      Folgende Felder müssen von der Subklasse bereitgestellt
**      werden:
**
**      Mauskoordinaten:
**      ci->act.scrx,scry
**      ci->down.scrx,scry
**      ci->up.scrx,scry
**
**      Mausbutton-Information:
**      ci->flags (CIF_MOUSEDOWN|CIF_MOUSEHOLD|CIF_MOUSEUP)
**
**  CHANGED
**      21-Feb-96   floh    created
*/
{
    struct iwimp_data *iwd = INST_DATA(cl,o);

    /*** ClickInfo initialisieren und gültig machen initialisieren ***/
    ci->box   = NULL;
    ci->btn   = -1;
    ci->flags |= CIF_VALID;

    /*** Test gegen aktuelle Mausposition ***/
    iwimp_GetClickInfo(iwd, ci, &(ci->act));

    /*** BUTTON_HOLD? ***/
    if ((ci->flags & CIF_MOUSEHOLD) &&
        (ci->btn >= 0) &&
        (ci->box == iwd->hitbox_store) &&
        (ci->btn == iwd->hitbtn_store))
    {
        ci->flags |= CIF_BUTTONHOLD;
    };

    /*** BUTTON_DOWN? ***/
    if (ci->flags & CIF_MOUSEDOWN) {

        iwimp_GetClickInfo(iwd, ci, &(ci->down));

        if (ci->btn != -1) {
            iwd->hitbox_store = ci->box;
            iwd->hitbtn_store = ci->btn;
            ci->flags |= CIF_BUTTONDOWN;
        } else {
            iwd->hitbox_store = NULL;
            iwd->hitbtn_store = -1;
        };
    };

    /*** BUTTON_UP? ***/
    if (ci->flags & CIF_MOUSEUP) {

        iwimp_GetClickInfo(iwd, ci, &(ci->up));

        if ((ci->box != NULL) &&
            (ci->box == iwd->hitbox_store) &&
            (ci->btn == iwd->hitbtn_store))
        {
            ci->flags |= CIF_BUTTONUP;
        };
        iwd->hitbox_store = NULL;
        iwd->hitbtn_store = -1;
    };

    /*** Ende ***/
}
