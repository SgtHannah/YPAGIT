/*
**  $Source: PRG:VFM/Classes/_InputClass/in_main.c,v $
**  $Revision: 38.11 $
**  $Date: 1998/01/06 14:52:22 $
**  $Locker:  $
**  $Author: floh $
**
**  Die zentrale Input-Collector-Klasse.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "modules.h"
#include "input/idevclass.h"
#include "input/iwimpclass.h"
#include "input/itimerclass.h"

#include "input/inputclass.h"

/*-------------------------------------------------------------------
**  Prototypes
*/
_dispatcher(Object *, inp_OM_NEW, struct TagItem *);
_dispatcher(BOOL, inp_OM_DISPOSE, void *);
_dispatcher(ULONG, inp_IM_SETHANDLER, struct inp_handler_msg *);
_dispatcher(void, inp_IM_GETINPUT, struct VFMInput *);
_dispatcher(ULONG, inp_IM_DELEGATE, struct inp_delegate_msg *);

/*-------------------------------------------------------------------
**  Class Header
*/
_use_nucleus

#ifdef AMIGA
__far ULONG inp_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG inp_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo inp_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeInpClass(ULONG id,...);
__geta4 BOOL FreeInpClass(void);
#else
struct ClassInfo *MakeInpClass(ULONG id,...);
BOOL FreeInpClass(void);
#endif

struct GET_Class inp_GET = {
    &MakeInpClass,      /* MakeExternClass() */
    &FreeInpClass,      /* FreeExternClass() */
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&inp_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *inp_Entry(void)
{
    return(&inp_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo inp_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:input.class"  /* der Segment-Name */
    },
    inp_Entry,                  /* Entry()-Adresse */
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
__geta4 struct ClassInfo *MakeInpClass(ULONG id,...)
#else
struct ClassInfo *MakeInpClass(ULONG id,...)
#endif
/*
**  CHANGES
**      25-Feb-96   floh    created
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    memset(inp_Methods,0,sizeof(inp_Methods));

    inp_Methods[OM_NEW]        = (ULONG) inp_OM_NEW;
    inp_Methods[OM_DISPOSE]    = (ULONG) inp_OM_DISPOSE;
    inp_Methods[IM_SETHANDLER] = (ULONG) inp_IM_SETHANDLER;
    inp_Methods[IM_GETINPUT]   = (ULONG) inp_IM_GETINPUT;
    inp_Methods[IM_DELEGATE]   = (ULONG) inp_IM_DELEGATE;

    inp_clinfo.superclassid = NUCLEUSCLASSID;
    inp_clinfo.methods      = inp_Methods;
    inp_clinfo.instsize     = sizeof(struct input_data);
    inp_clinfo.flags        = 0;

    /* und das war's */
    return(&inp_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeInpClass(void)
#else
BOOL FreeInpClass(void)
#endif
/*
**  CHANGED
**      19-Feb-96   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  SUPPORT FUNKTIONEN                                             **
**=================================================================*/

/*-----------------------------------------------------------------*/
struct IXToken *inp_CreateIXToken(struct MinList *ls)
/*
**  FUNCTION
**      Allokiert ein neues Input-Expression-Token und
**      hängt es an das Ende der übergebenen IX-Liste.
**
**  INPUTS
**      ls  - Ptr auf Listenheader der IX-Liste
**
**  RESULTS
**      ix - Pointer auf allokiertes und eingeklinktes Token,
**           NULL bei Mißerfolg
**
**  CHANGED
**      07-Mar-96   floh    created
*/
{
    struct IXToken *ix;

    if (ix = _AllocVec(sizeof(struct IXToken), MEMF_PUBLIC|MEMF_CLEAR)) {
        _AddTail((struct List *)ls, (struct Node *)ix);
    };

    return(ix);
}

/*-----------------------------------------------------------------*/
void inp_DeleteIXToken(struct IXToken *ix)
/*
**  FUNCTION
**      Entfernt das übergebene IXToken aus seiner Liste
**      und gibt den Speicher frei.
**
**  INPUTS
**      ix  - Ptr auf IXToken, wie mit inp_CreateIXToken allokiert
**
**  CHANGED
**      07-Mar-96   floh    created
**      10-Mar-96   floh    _Remove() entfernt, das hat sich
**                          nämlich mit dem _RemHead() in OM_DISPOSE
**                          gebissen.
*/
{
    _FreeVec(ix);
}

/*-----------------------------------------------------------------*/
UBYTE *inp_NextIXToken(UBYTE *str, struct MinList *ls)
/*
**  FUNCTION
**      Scannt den String <str> nach dem nächsten
**      Input-Expression-Token, wertet dieses aus und
**      erzeugt daraus eine ausgefüllte IXToken-Struktur,
**      welche an die Liste <ls> gehängt wird.
**
**  INPUTS
**      str - ein Input-Expression-String, bestehend aus
**            einer beliebigen Menge folgender Elemente:
**
**              Input-Descriptor:   
**                  "driver:id"
**
**              Verknüpfer:         
**                  " " Trennzeichen (optional)
**                  "&" AND-Verknüfer
**                  "|" OR-Verknüfer
**
**              Modifikator:        
**                  "~" Inverter
**                  "#" Button-2-Slider-Konverter
**
**  RESULTS
**      str - Pointer auf Anfang der nächsten Input-Expression,
**            NULL wenn String-Ende erreicht oder ein
**            Fehler festgestellt wurde (in diesem Fall wird
**            eine Warnung ausgegeben).
**
**      Das erzeugte IXToken wird mit folgenden Infos ausgefüllt:
**
**          ix.o     -> bleibt NULL
**          ix.flags -> vorangestellte Tokens, als Flags:
**                      IXF_And | IXF_Or | IXF_Invert | IXF_ForceSlider
**
**  CHANGED
**      07-Mar-96   floh    created
**      08-Mar-96   floh    Stark veränderte Parsing-Schleife,
**                          sehr viel einfacher und etwas
**                          cleverer.
*/
{
    struct IXToken *ix;

    /*** Allokiere eine IXToken-Struktur ***/
    ix = inp_CreateIXToken(ls);
    if (ix) {

        UBYTE c;
        UBYTE *copy_to   = ix->id;
        BOOL  last_token = FALSE;

        while (c = *str) {

            /*** aktuelles Zeichen auswerten ***/
            switch (c) {

                /*** Leerzeichen und Tabs generell ignorieren ***/
                case ' ':
                case '\t':
                    if (last_token) return(str);
                    break;

                case '&':  
                    if (last_token) return(str);
                    else            ix->flags |= IXF_And;
                    break;
                case '|':  
                    if (last_token) return(str);
                    else            ix->flags |= IXF_Or;
                    break;
                case '~':  
                    if (last_token) return(str);
                    else            ix->flags |= IXF_Invert; 
                    break;
                case '#':
                    if (last_token) return(str);
                    else            ix->flags |= IXF_ForceSlider; 
                    break;
                case ':':
                    /*** id-Kopie ist fertig, umschalten auf sub_id ***/
                    copy_to    = ix->sub_id;
                    last_token = TRUE;  // Ende bei nächstem "Trennzeichen"
                    break;

                default:
                    /*** Teil der ID, oder SubID, also kopieren... ***/
                    *copy_to++ = c;
                    break;
            };

            /*** nächstes Zeichen ***/
            str++;
        };
    };

    /*** Fehler, oder String komplett ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
BOOL inp_CreateIXL(struct input_data *id, struct inp_handler_msg *ih_msg)
/*
**  FUNCTION
**      Erzeugt eine komplette Input-Expression-Liste, inklusive
**      den Input-Provider-Objects. Dazu wird der String
**      <msg->id> als eine Kette von Input-Expression-Tokens
**      geparsed und "binarisiert".
**
**  INPUTS
**      id      - Ptr auf LID des Objects
**      ih_msg  - Input-Handler-Message, wie an IM_SETHANDLER übergeben
**
**  RESULTS
**      TRUE    - alles OK
**      FALSE   - oops, ein Fehler
**
**  CHANGED
**      07-Mar-96   floh    created
**      05-Jun-96   floh    Bugfix: die entsprechende IX-Liste
**                          wird zuerst geleert!
**
**  BUGS
**      Die Routine gibt im Fehlerfalle nur Warnungen per _LogMsg()
**      aus, kehrt aber in diesem Fall mit einer unvollständigen
**      Expression-Liste TRUE zurück. Das sollte keine Abstürze
**      provozieren, kann aber Anormalitäten im Input-Handling
**      nach sich ziehen.
*/
{
    struct MinList *ls;
    UBYTE *str = ih_msg->id;

    /*** richtige Liste ermitteln ***/
    if (ih_msg->type == ITYPE_BUTTON) ls = &(id->btn_ixl[ih_msg->num]);
    else                              ls = &(id->sld_ixl[ih_msg->num]);

    if (str) {

        struct MinNode *nd;
        struct IXToken *old_ix;

        /*** IXToken-Liste leermachen ***/
        while (old_ix = (struct IXToken *) _RemHead((struct List *)ls)) {
            if (old_ix->o) _dispose(old_ix->o);
            inp_DeleteIXToken(old_ix);
        };

        /*** IXToken-Liste erzeugen, durch Parsen von msg->id ***/
        while (str = inp_NextIXToken(str,ls));

        /*** für jedes IXToken ein Watcher-Object erzeugen ***/
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

            UBYTE strbuf[128];
            struct IXToken *ix = (struct IXToken *) nd;

            /*** <id> und <sub_id> gültig? ***/
            if ((ix->id[0] != 0) && (ix->sub_id[0] != 0)) {

                /*** erzeuge Object ***/
                strcpy(strbuf,"drivers/input/");
                strcat(strbuf,ix->id);
                strcat(strbuf,".class");
                ix->o = _new(strbuf, TAG_DONE);

                if (ix->o) {

                    /*** teile dem Object seine sub_id mit ***/
                    struct idev_setid_msg sim;
                    sim.id   = ix->sub_id;
                    ix->type = _methoda(ix->o, IDEVM_SETID, &sim);

                    /*** hat das Obj die SubID akzeptiert? ***/
                    if (IDEVTYPE_NONE == ix->type) {
                        _LogMsg("input.class, WARN: Driver '%s' did not accept id '%s'.\n",
                                ix->id, ix->sub_id);
                    };
                } else {
                    _LogMsg("input.class, WARN: Unknown driver '%s'.\n",ix->id);
                };
            };
        };
    };

    /*** Ende (Hack) ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void inp_EvaluateIXL(struct MinList *ls, struct IXResult *res, ULONG ts)
/*
**  FUNCTION
**      Evaluiert die übergebene Input-Expression-Liste,
**      indem jedes Token-Object in der Liste nach seinem
**      Status gefragt wird und die Resultate danach
**      miteinander verknüpft werden.
**
**      Dabei gilt:
**
**          - die IXResult Struktur MUSS selbst initialisiert
**            werden
**          - die Liste wird von links nach rechts aufgelöst
**          - Slider werden generell addiert
**          - ein Inverter-Token '~' invertiert den boolschen
**            Status eines Buttons und negiert den numerischen
**            Wert eines Sliders
**          - ein # Token macht den nachfolgenden Button
**            zu einem "Pseudo-Slider" mit den Werten
**            0.0 (nicht gedrückt) oder 1.0 (gedrückt)
**
**  INPUTS
**      ls  - Ptr auf Input-Xpression-Liste
**      res - hierhin wird das Ergebnis geschrieben
**              res.bool_res - Gesamt-Resultat aller Buttons
**              res.flt_res  - Gesamt-Resultat aller Slider
**      ts  - timestamp für dieses Frames
**
**  RESULTS
**      siehe oben
**
**  CHANGED
**      07-Mar-96   floh    created
**      08-Mar-96   floh    Debugging: Default-Verknüpfer jetzt AND
**                          statt OR
**      08-Apr-96   floh    angepaßt an neues IDEVM_GETBUTTON und
**                          IDEVM_GETSLIDER
**      29-Sep-96   floh    ForceSlider-Buttons werden jetzt wegen
**                          dem komplexeren Slider-Handling (Gummiband)
**                          per IDEVM_GETSLIDER abgefragt.
*/
{
    struct MinNode *nd;

    /*** los geht's... ***/
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct IXToken *ix = (struct IXToken *) nd;

        if (ix->o) {

            struct idev_status_msg sm;
            sm.time_stamp = ts;

            /*** es gibt zwei IX-Object-Type, Buttons und Sliders ***/
            switch(ix->type) {

                case IDEVTYPE_BUTTON:

                    if (ix->flags & IXF_ForceSlider) {
                        /*** ForceSlider-Button ***/
                        _methoda(ix->o, IDEVM_GETSLIDER, &sm);
                        if (ix->flags & IXF_Invert) res->flt_res -= sm.sld_status;
                        else                        res->flt_res += sm.sld_status;
                    } else {
                        /*** ein "echter" Button ***/
                        BOOL btn_res;
                        _methoda(ix->o, IDEVM_GETBUTTON, &sm);
                        btn_res = sm.btn_status;
                        if (ix->flags & IXF_Invert) btn_res = !btn_res;
                        if (ix->flags & IXF_Or) res->bool_res |= btn_res;
                        else                    res->bool_res &= btn_res;
                    };
                    break;

                case IDEVTYPE_SLIDER:

                    /*** aktuelles Object ist ein Slider-Watcher ***/
                    _methoda(ix->o, IDEVM_GETSLIDER, &sm);
                    if (ix->flags & IXF_Invert) res->flt_res -= sm.sld_status;
                    else                        res->flt_res += sm.sld_status;
                    break;
            };
        };
    };

    /*** Ende ***/
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, inp_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      07-Mar-96   floh    created
*/
{
    Object *newo;
    struct input_data *id;
    ULONG i;

    newo = (Object *) _supermethoda(cl,o,OM_NEW,attrs);
    if (!newo) return(NULL);

    id = INST_DATA(cl,newo);

    /*** Input Expression Lists initialisieren ***/
    for (i=0; i<INUM_BUTTONS; i++) {
        _NewList((struct List *) &(id->btn_ixl[i]));
    };
    for (i=0; i<INUM_SLIDERS; i++) {
        _NewList((struct List *) &(id->sld_ixl[i]));
    };

    /*** fertig ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(BOOL, inp_OM_DISPOSE, void *ignore)
/*
**  CHANGED
**      25-Feb-96   floh    created
**      07-Mar-96   floh    angepaßt an neue Input-Expression-Listen
*/
{
    struct input_data *id = INST_DATA(cl,o);
    ULONG i;

    /*** alle eingebetteten Objects killen ***/
    if (id->timer) _dispose(id->timer);
    if (id->wimp)  _dispose(id->wimp);
    if (id->keyb)  _dispose(id->keyb);

    /*** Button Input-Expression-Lists ***/
    for (i=0; i<INUM_BUTTONS; i++) {

        /*** alle IXTokens in der Liste killen ***/
        struct MinList *ls = &(id->btn_ixl[i]);
        struct IXToken *ix;
        while (ix = (struct IXToken *) _RemHead((struct List *)ls)) {
            if (ix->o) _dispose(ix->o);
            inp_DeleteIXToken(ix);
        };
    };

    /*** Slider Input-Expresseion-Lists ***/
    for (i=0; i<INUM_SLIDERS; i++) {

        /*** alle IXTokens in der Liste killen ***/
        struct MinList *ls = &(id->sld_ixl[i]);
        struct IXToken *ix;
        while (ix = (struct IXToken *) _RemHead((struct List *)ls)) {
            if (ix->o) _dispose(ix->o);
            inp_DeleteIXToken(ix);
        };
    };

    /*** Ende ***/
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,ignore));
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, inp_IM_SETHANDLER, struct inp_handler_msg *msg)
/*
**  CHANGED
**      25-Feb-96   floh    created
*/
{
    struct input_data *id = INST_DATA(cl,o);
    ULONG retval = FALSE;
    UBYTE strbuf[128];

    switch(msg->type) {

        case ITYPE_WIMP:
            /*** das GUI-Handler-Object ***/
            if (id->wimp) _dispose(id->wimp);
            strcpy(strbuf,"drivers/input/");
            strcat(strbuf,msg->id);
            strcat(strbuf,".class");
            id->wimp = _new(strbuf, TAG_DONE);
            if (id->wimp) retval = TRUE;
            break;

        case ITYPE_TIMER:
            /*** das Time-Source-Object ***/
            if (id->timer) _dispose(id->timer);
            strcpy(strbuf,"drivers/input/");
            strcat(strbuf,msg->id);
            strcat(strbuf,".class");
            id->timer = _new(strbuf, TAG_DONE);
            if (id->timer) retval = TRUE;
            break;

        case ITYPE_KEYBOARD:
            /*** das Keyboard-Handler-Object ***/
            if (id->keyb) _dispose(id->keyb);
            strcpy(strbuf,"drivers/input/");
            strcat(strbuf,msg->id);
            strcat(strbuf,".class");
            id->keyb  = _new(strbuf, TAG_DONE);
            if (id->keyb) retval = TRUE;
            break;

        case ITYPE_BUTTON:
            if (inp_CreateIXL(id,msg)) retval = TRUE;
            break;

        case ITYPE_SLIDER:
            if (inp_CreateIXL(id,msg)) retval = TRUE;
            break;
    };

    /*** das war's ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, inp_IM_GETINPUT, struct VFMInput *ip)
/*
**  CHANGED
**      25-Feb-96   floh    created
**      23-Mar-96   floh    Hotkey-Support eingebaut.
**      17-Jun-96   floh    frame_id -> time_stamp
**      16-Sep-97   floh    + IDEVM_GETKEY Abfrage handelt jetzt auch die
**                            neue ASCII Key ab.
*/
{
    struct input_data *id = INST_DATA(cl,o);

    /*** nuke'em ***/
    memset(ip, 0, sizeof(struct VFMInput));

    /*** Timing ***/
    if (id->timer) {
        ip->FrameTime = _methoda(id->timer, ITIM_ELAPSEDTIME, NULL);
    } else {
        ip->FrameTime = 20;     // Fixed Timing
    };

    id->time_stamp += ip->FrameTime;

    /*** nur loslegen, wenn wir den InputFocus haben ***/
    if (id->wimp && _methoda(id->wimp, IWIMPM_HASFOCUS, NULL)) {

        ULONG i;

        /*** Key-Status ***/
        if (id->keyb) {
            struct idev_getkey_msg gk;
            memset(&gk,0,sizeof(gk));
            _methoda(id->keyb, IDEVM_GETKEY, &gk);
            ip->ContKey  = gk.cont_key;
            ip->NormKey  = gk.norm_key;
            ip->HotKey   = gk.hot_key;
            ip->AsciiKey = gk.ascii_key;
        };

        /*** GUI-Status ***/
        _methoda(id->wimp, IWIMPM_GETCLICKINFO, &(ip->ClickInfo));

        /*** Input-Xpression-Listen aller Buttons auflösen ***/
        for (i=0; i<INUM_BUTTONS; i++) {
            struct IXResult res;
            res.bool_res = TRUE;
            res.flt_res  = 0.0;
            inp_EvaluateIXL(&(id->btn_ixl[i]), &res, id->time_stamp);
            if (res.bool_res) ip->Buttons |= (1<<i);
        };

        /*** Input-Xpression-Listen aller Slider auflösen ***/
        for (i=0; i<INUM_SLIDERS; i++) {
            struct IXResult res;
            res.bool_res = TRUE;
            res.flt_res  = 0.0;
            inp_EvaluateIXL(&(id->sld_ixl[i]), &res, id->time_stamp);
            if (res.bool_res) ip->Slider[i] = res.flt_res;
        };

        /*** FIXME: Backward compatibilty only ***/
        ip->SliderX = (WORD) (ip->Slider[0] * 32.0);
        ip->SliderY = (WORD) (ip->Slider[1] * 32.0);

    } else {

        /*** die Slider müssen auch gerefresht werden,  ***/
        /*** wenn das Objekt nicht den Input-Focus hat, ***/
        /*** das Ergebnis wird aber verworfen           ***/
        ULONG i;
        for (i=0; i<INUM_SLIDERS; i++) {
            struct IXResult res;
            inp_EvaluateIXL(&(id->sld_ixl[i]), &res, id->time_stamp);
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, inp_IM_DELEGATE, struct inp_delegate_msg *msg)
/*
**  CHANGED
**      25-Feb-96   floh    created
**      15-Jun-96   floh    + ITYPE_BUTTON und ITYPE_SLIDER:
**                            die übergebene Methode wird an alle
**                            Objekte in der Input-Expression-Liste
**                            des übergebenen Buttons oder Sliders
**                            übergeben. In dem Fall wird als
**                            Retval allerdings immer 0 zurückgegeben
*/
{
    struct input_data *id = INST_DATA(cl,o);
    Object *target;
    ULONG retval = 0;

    /*** an welches Object delegieren? ***/
    switch(msg->type) {
        case ITYPE_WIMP:        target = id->wimp; break;
        case ITYPE_TIMER:       target = id->timer; break;
        case ITYPE_KEYBOARD:    target = id->keyb; break;
        default:                target = NULL;
    };

    if (target) retval = _methoda(target, msg->method, msg->msg);
    else if ((msg->type==ITYPE_BUTTON) || (msg->type==ITYPE_SLIDER)) {

        /*** Button, oder Slider: Methode distributen ***/
        struct MinList *ls;
        struct MinNode *nd;
        if (msg->type == ITYPE_BUTTON) ls=(struct MinList *)&(id->btn_ixl[msg->num]);
        else                           ls=(struct MinList *)&(id->sld_ixl[msg->num]);
        for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {
            struct IXToken *ix = (struct IXToken *) nd;
            if (ix->o) _methoda(ix->o, msg->method, msg->msg);
        };
        retval = 0;
    };

    /*** das war's auch schon ***/
    return(retval);
}

