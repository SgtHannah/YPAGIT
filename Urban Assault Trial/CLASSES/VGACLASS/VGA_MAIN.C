/*
**  $Source: PRG:VFM/Classes/_VgaClass/vga_main.c,v $
**  $Revision: 38.8 $
**  $Date: 1997/01/20 22:42:43 $
**  $Locker: floh $
**  $Author: floh $
**
**  Display-Treiber-Klasse für VGA und SVGA mit VESA2.x linearen
**  Framebuffer-Support.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <i86.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "bitmap/vgaclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(Object *, vga_OM_NEW, struct TagItem *attrs);
_dispatcher(ULONG, vga_OM_DISPOSE, void *);
_dispatcher(void, vga_OM_GET, struct TagItem *);

_dispatcher(ULONG, vga_DISPM_Query, struct disp_query_msg *);
_dispatcher(void, vga_DISPM_Begin, void *);
_dispatcher(void, vga_DISPM_End, void *);
_dispatcher(void, vga_DISPM_Show, void *);
_dispatcher(void, vga_DISPM_Hide, void *);
_dispatcher(void, vga_DISPM_MixPalette, struct disp_mixpal_msg *);
_dispatcher(void, vga_DISPM_SetPalette, struct disp_setpal_msg *);

_dispatcher(void, vga_DISPM_SetPointer, struct disp_pointer_msg *);
_dispatcher(void, vga_DISPM_ShowPointer, void *);
_dispatcher(void, vga_DISPM_HidePointer, void *);

struct MinList vga_IDList;
struct vesa_VbeInfoBlock vga_VIB;
ULONG vga_Vesa2;

/*-------------------------------------------------------------------
**  Class Header
*/
ULONG vga_Methods[NUCLEUS_NUMMETHODS];

struct ClassInfo vga_clinfo;

/*-------------------------------------------------------------------
**  Global Entry Table
*/
struct ClassInfo *vga_MakeClass(ULONG id,...);
BOOL vga_FreeClass(void);

struct GET_Class vga_GET = {
    &vga_MakeClass,
    &vga_FreeClass,
};

/*===================================================================
**  *** CODE SEGMENT HEADER ***
*/
struct GET_Class *vga_Entry(void)
{
    return(&vga_GET);
};

struct SegInfo vga_class_seg = {
    { NULL, NULL,
      0, 0,
      "MC2classes:drivers/gfx/vga.class"
    },
    vga_Entry,
};

/*-----------------------------------------------------------------*/
BOOL vga_AddMode(UBYTE *pfix, ULONG id, ULONG w, ULONG h, ULONG bpg)
/*
**  FUNCTION
**      Erzeugt <id_node> und hängt sie an die Mode-Liste.
**
**  INPUT
**      pfix        - Mode-Name-Prefix (VGA, oder VESA)
**      id          - Mode-ID
**      w           - Width
**      h           - Height
**      bpg         - Bits Per Gun
**
**  CHANGED
**      26-Nov-96   floh    created
*/
{
    struct disp_idnode *dnd;
    ULONG retval = FALSE;

    dnd = (struct disp_idnode *)
          _AllocVec(sizeof(struct disp_idnode),
          MEMF_PUBLIC|MEMF_CLEAR);
    if (dnd) {
        dnd->id = id;
        dnd->w  = w;
        dnd->h  = h;
        dnd->data[0] = bpg;       // Bit Per Gun
        sprintf(dnd->name,"%s: %dx%d (%d BPG)",pfix,w,h,bpg);
        _AddTail((struct List *)&vga_IDList,(struct Node *)dnd);
        retval = TRUE;
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
struct ClassInfo *vga_MakeClass(ULONG id,...)
/*
**  CHANGED
**      09-Jun-96   floh    created
**      13-Aug-96   floh    + 8-Bit-DAC Support, globale
**                            Variable vga_Vesa2, sowie
**                            erweiterte Modi-Namen
**      17-Aug-96   floh    - OM_SET, BMA_ColorMap-Handling
**                            übernimmt die Superklasse
**      19-Aug-96   floh    - Support für 8-Bit-DAC entfernt,
**                            entweder ich, oder die VBE-Specs,
**                            oder meine Gfx-Karte spinnt,
**                            das Thema ist vielleicht auch bissel
**                            zu esoterisch...
*/
{ 
    struct disp_idnode *dnd;

    /*** DisplayMode-ID Liste aufbauen ***/
    _NewList((struct List *) &vga_IDList);
    vga_Vesa2 = vesa_InitVESAInfo(&vga_VIB);

    /*** Mode 0x13 geht immer ***/
    vga_AddMode("VGA",0x13,320,200,6);

    /*** die restlichen Modi sind optional ***/
    if (vga_Vesa2) {

        WORD id;
        WORD *modes;
        struct vesa_ModeInfoBlock mib;

        modes = vga_VIB.VideoModePtr;
        while ((id = *modes++) != -1) {

            /*** Eigenschaften des aktuellen Modes ermitteln ***/
            if (vesa_GetModeInfo(id,&vga_VIB,&mib)) {
                /*** geeignet? ***/
                if ((mib.BitsPerPixel == 8) &&
                    (mib.MemoryModel  == VBEMM_PACKED))
                {
                    /*** der übliche 6 bpg Modus ***/
                    vga_AddMode("VESA",id,mib.XResolution,mib.YResolution,6);

                    /*** falls möglich, 8 bpg Modus ***/
                    if (vga_VIB.Capabilities & VBECAPF_DAC8Bittable) {
                        vga_AddMode("VESA",0x8000|id,
                        mib.XResolution,mib.YResolution,8);
                    };
                };
            };
        };
    };

    /*** Klasse initialisieren ***/
    memset(vga_Methods, 0, sizeof(vga_Methods));

    vga_Methods[OM_NEW]      = (ULONG) vga_OM_NEW;
    vga_Methods[OM_DISPOSE]  = (ULONG) vga_OM_DISPOSE;
    vga_Methods[OM_GET]      = (ULONG) vga_OM_GET;

    vga_Methods[DISPM_Query]       = (ULONG) vga_DISPM_Query;
    vga_Methods[DISPM_Begin]       = (ULONG) vga_DISPM_Begin;
    vga_Methods[DISPM_End]         = (ULONG) vga_DISPM_End;
    vga_Methods[DISPM_Show]        = (ULONG) vga_DISPM_Show;
    vga_Methods[DISPM_Hide]        = (ULONG) vga_DISPM_Hide;
    vga_Methods[DISPM_SetPalette]  = (ULONG) vga_DISPM_SetPalette;
    vga_Methods[DISPM_MixPalette]  = (ULONG) vga_DISPM_MixPalette;
    vga_Methods[DISPM_SetPointer]  = (ULONG) vga_DISPM_SetPointer;
    vga_Methods[DISPM_ShowPointer] = (ULONG) vga_DISPM_ShowPointer;
    vga_Methods[DISPM_HidePointer] = (ULONG) vga_DISPM_HidePointer;

    vga_clinfo.superclassid = DISPLAY_CLASSID;
    vga_clinfo.methods      = vga_Methods;
    vga_clinfo.instsize     = sizeof(struct vga_data);
    vga_clinfo.flags        = 0;

    return(&vga_clinfo);
}

/*-----------------------------------------------------------------*/
BOOL vga_FreeClass(void)
/*
**  CHANGED
**      09-Jun-96   floh    created
*/
{
    /*** Display-ID-Liste killen ***/
    struct Node *nd;
    while (nd = _RemHead((struct List *) &vga_IDList)) _FreeVec(nd);
    vesa_KillVESAInfo(&vga_VIB);
    return(TRUE);
}

/*=================================================================**
**  SUPPORT FUNKTIONEN                                             **
**=================================================================*/

/*-----------------------------------------------------------------*/
struct disp_idnode *vga_GetIDNode(ULONG id)
/*
**  FUNCTION
**      Sucht ID-Node der gegebenen Mode-ID in der
**      ID-Liste, und returniert Pointer darauf.
**
**  CHANGED
**      09-Jun-96   floh    created
*/
{
    struct MinNode *nd;
    struct MinList *ls;

    ls = &vga_IDList;
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct disp_idnode *ind = (struct disp_idnode *) nd;
        if (id == ind->id) return(ind);
    };
    return(NULL);
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(Object *, vga_OM_NEW, struct TagItem *attrs)
/*
**  CHANGED
**      09-Jun-96   floh    created
**      13-Aug-96   floh    + 8 bpg Support
**      19-Aug-96   floh    - 8 bpg Support :-)
**      26-Nov-96   floh    + modespezifischer 8 bpg Support
**                            wieder drin
*/
{
    Object *newo;
    struct vga_data *vd;
    ULONG id,w,h;
    UBYTE *name;
    struct TagItem ti[8];
    ULONG t=0;
    struct disp_idnode *idnode = NULL;

    memset(ti,0,sizeof(ti));

    /*** suche Display-ID, Höhe und Breite ***/
    name = (UBYTE *) _GetTagData(RSA_Name,0,attrs);
    w    = _GetTagData(BMA_Width,0,attrs);
    h    = _GetTagData(BMA_Height,0,attrs);
    id   = _GetTagData(DISPA_DisplayID,0,attrs);

    /*** fehlende Parameter setzen ***/
    if (id != 0) {
        idnode = vga_GetIDNode(id);
        if (idnode) {
            /*** evtl. Höhe und/oder Breite ergänzen ***/
            if (0 == w) { 
                w = idnode->w;
                ti[t].ti_Tag  = BMA_Width;
                ti[t].ti_Data = idnode->w;
                t++;
            };
            if (0 == h) {
                h = idnode->h;
                ti[t].ti_Tag  = BMA_Height;
                ti[t].ti_Data = idnode->h;
                t++;
            };
        } else {
            /*** unbekannte Display-ID ***/
            _LogMsg("vga.class: Unsupported display id!\n");
            return(NULL);
        };
        /*** ColorMap erzwingen ***/
        ti[t].ti_Tag  = BMA_HasColorMap;
        ti[t].ti_Data = TRUE;
        t++;
    };

    /*** erzeuge Objekt ***/
    ti[t].ti_Tag  = TAG_MORE;
    ti[t].ti_Data = (ULONG) attrs;
    newo = (Object *) _supermethoda(cl,o,OM_NEW,ti);
    if (!newo) return(NULL);
    vd = INST_DATA(cl,newo);

    /*** Display-Parameter initialisieren ***/
    vd->id = id;
    if (vga_Vesa2) vd->flags |= VGAF_Vesa2;
    if (id & 0x8000) {
        /*** ein 8-bpg-Modus ***/
        vd->flags |= VGAF_8BitDAC;
        id &= ~0x8000;
    };

    /*** falls DisplayID gegeben, Display initialisieren ***/
    if (idnode) {

        UBYTE *cm;

        if (id == 0x13) {

            /*** Spezialfall: Mode 0x13 ***/
            union REGS regs;

            vd->d.Data   = 0xa000<<4;    // DOS/4GW-spezifisch
            vd->d.Width  = 320;
            vd->d.Height = 200;

            regs.h.ah = 0x0;
            regs.h.al = 0x13;
            int386(0x10, &regs, &regs);

        } else {

            /*** ein VESA-Modus ***/
            vesa_GetModeInfo(id,&vga_VIB,&(vd->mib));
            vd->d.Data   = vd->mib.PhysBasePtr;
            vd->d.Width  = vd->mib.XResolution;
            vd->d.Height = vd->mib.YResolution;
            vesa_SetLinearMode(id);

            /*** falls 8 bpg, DAC manipulieren ***/
            if (vd->flags & VGAF_8BitDAC) vesa_Set8BitDAC(&vga_VIB);
        };

        /*** ColorMap einstellen, sowie Slot0 auf Default-Map ***/
        cm = (UBYTE *) _GetTagData(BMA_ColorMap, NULL, attrs);
        if (cm) {
            struct disp_setpal_msg dsm;
            struct disp_mixpal_msg dmm;
            ULONG slot[1],weight[1];

            dsm.slot  = 0;
            dsm.first = 0;
            dsm.num   = 256;
            dsm.pal   = cm;
            _methoda(newo, DISPM_SetPalette, &dsm);

            slot[0]    = 0;
            weight[0]  = 256;
            dmm.num    = 1;
            dmm.slot   = &slot;
            dmm.weight = &weight;
            _methoda(newo, DISPM_MixPalette, &dmm);
        };
    };

    /*** Direkt-Pointer auf Raster-Bitmap ***/
    _get(newo, RSA_Handle, &(vd->r));

    /*** Export-Daten ausfüllen ***/
    vd->export.x_size = vd->d.Width;
    vd->export.y_size = vd->d.Height;
    vd->export.data   = vd;
    vd->export.mouse_callback = vga_MCallBack;

    /*** ... und fertig ***/
    return(newo);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, vga_OM_DISPOSE, void *nil)
/*
**  CHANGED
**      09-Jun-96   floh    created
*/
{
    struct vga_data *vd = INST_DATA(cl,o);
    if (vd->id != 0) {
        /*** 80x25x16 Textmodus einstellen ***/
        union REGS regs;
        regs.h.ah = 0x0;
        regs.h.al = 0x3;
        int386(0x10, &regs, &regs);
    };
    return((BOOL)_supermethoda(cl,o,OM_DISPOSE,nil));
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_OM_GET, struct TagItem *attrs)
/*
**  CHANGED
**      09-Jun-96   floh    created
**      25-Nov-96   floh    DISPA_DisplayHandle ist jetzt ein
**                          <struct msdos_DispEnv *>
*/
{
    struct vga_data *vd = INST_DATA(cl,o);
    ULONG tag;
    struct TagItem *ti = attrs;

    /* Attribut-Liste scannen */
    while ((tag = ti->ti_Tag) != TAG_DONE) {

        register ULONG *value = (ULONG *) ti++->ti_Data;

        /* erstmal die System-Tags... */
        switch(tag) {

            case TAG_IGNORE:    continue;
            case TAG_MORE:      ti = (struct TagItem *) value; break;
            case TAG_SKIP:      ti += (ULONG) value; break;
            default:

                switch(tag) {
                    case DISPA_DisplayID:
                        *value = vd->id;
                        break;
                    case DISPA_DisplayHandle:
                        *value = (ULONG) &(vd->export);
                        break;
                };
        };
    };
    _supermethoda(cl,o,OM_GET,attrs);
}

/*-----------------------------------------------------------------*/
_dispatcher(ULONG, vga_DISPM_Query, struct disp_query_msg *msg)
/*
**  CHANGED
**      08-Jun-96   floh    created
**      20-Jul-96   floh    Bugfix: die gefundene <id> wurde
**                          nicht in die Message zurückgeschrieben,
**                          das gab Probleme beim Suchen der ersten
**                          Node, welche mit <msg->id==0> definiert ist.
*/
{
    ULONG id = msg->id;
    struct disp_idnode *ind;

    if (id != 0) {
        ind = vga_GetIDNode(id);
    } else {
        ind = (struct disp_idnode *) vga_IDList.mlh_Head;
    };

    if (ind) {
        struct disp_idnode *next;
        msg->id = ind->id;
        msg->w  = ind->w;
        msg->h  = ind->h;
        strncpy(msg->name,ind->name,32);
        next = (struct disp_idnode *) ind->nd.mln_Succ;
        if (next->nd.mln_Succ) return(next->id);
    };

    return(0);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_Begin, void *nil)
/*
**  CHANGED
**      09-Jun-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_End, void *nil)
/*
**  CHANGED
**      08-Jun-96   floh    created
**      25-Nov-96   floh    sorgt jetzt intern dafür,
**                          daß der Mauszeiger korrekt bleibt
**      26-Nov-96   floh    kopieren passiert jetzt normalerweise
**                          in 3 Chunks: über dem Mauszeiger, auf
**                          dem Mauszeiger, unter dem Mauszeiger.
**      05-Dec-96   floh    weniger konservativ beim Interrupt-Disablen
*/
{
    struct vga_data *vd = INST_DATA(cl,o);
    if (vd->id) {

        /*** Mauszeiger aktiv? ***/
        if (vd->ptr) {

            UBYTE *from = vd->r->Data;
            UBYTE *to   = vd->d.Data;
            LONG y0,y1,y2,y3;
            LONG i0,i1,i2,i3;

            /*** Y-Koordinaten ***/
            _disable();
            y0 = 0;             // oberer Rand Display
            y3 = vd->d.Height;  // unterer Rand Display
            y1 = vd->m_y;       // oberer Rand Mauspointer
            if (y1 < y0) y1=y0;
            y2 = y1+vd->ptr->Height;    // unterer Rand Mauspointer
            if (y2 > y3) y2=y3;
            _enable();

            /*** Offsets ***/
            i0 = y0*vd->d.Width;
            i1 = y1*vd->d.Width;
            i2 = y2*vd->d.Width;
            i3 = y3*vd->d.Width;

            /*** Chunk über,auf und unter Mauspointer ***/
            if (i0 != i1) memcpy(to+i0,from+i0,i1-i0);
            if (i1 != i2) {
                _disable();
                vga_MFlush(vd);
                _enable();
                memcpy(to+i1,from+i1,i2-i1);
                _disable();
                vga_MShow(vd);
                _enable();
            };
            if (i2 != i3) memcpy(to+i2,from+i2,i3-i2);

        } else {

            /*** wenn die Maus eh nicht sichtbar ist... ***/
            _disable();
            vga_MFlush(vd);
            _enable();
            memcpy(vd->d.Data,vd->r->Data,vd->r->Width*vd->r->Height);
            _disable();
            vga_MShow(vd);
            _enable();
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_Show, void *nil)
/*
**  CHANGED
**      08-Jun-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_Hide, void *nil)
/*
**  CHANGED
**      08-Jun-96   floh    created
*/
{ }

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_SetPalette, struct disp_setpal_msg *msg)
/*
**  CHANGED
**      14-Apr-97   floh    created
*/
{
    /*** HACK: Farbe 0 auf schwarz patchen ***/
    if ((msg->slot == 0) && (msg->first == 0)) {
        msg->pal[0] = msg->pal[1] = msg->pal[2] = 0;
    };

    /*** Superklasse übernimmt das Mixing... ***/
    _supermethoda(cl,o,DISPM_SetPalette,msg);
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_MixPalette, struct disp_mixpal_msg *msg)
/*
**  CHANGED
**      13-Aug-96   floh    created
*/
{
    struct vga_data *vd     = INST_DATA(cl,o);
    struct display_data *dd = INST_DATA((cl->superclass),o);

    /*** Superklasse übernimmt das Mixing... ***/
    _supermethoda(cl,o,DISPM_MixPalette,msg);

    /*** das Resultat sichtbar machen ***/
    vesa_SetPalette(&vga_VIB,
                    (vd->flags & VGAF_Vesa2),
                    (vd->flags & VGAF_8BitDAC),
                    (UBYTE *) &(dd->pal));

    /*** das war's ***/
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_SetPointer, struct disp_pointer_msg *msg)
/*
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    struct vga_data *vd = INST_DATA(cl,o);
    _disable();
    vga_MHide(vd);
    vd->ptr = msg->pointer;
    vga_MShow(vd);
    _enable();
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_ShowPointer, void *nil)
/*
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    struct vga_data *vd = INST_DATA(cl,o);
    _disable();
    vga_MShow(vd);
    _enable();
}

/*-----------------------------------------------------------------*/
_dispatcher(void, vga_DISPM_HidePointer, void *nil)
/*
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    struct vga_data *vd = INST_DATA(cl,o);
    _disable();
    vga_MHide(vd);
    _enable();
}

