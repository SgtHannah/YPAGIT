/*
**  $Source: PRG:VFM/Classes/_VgaClass/vga_mouse.c,v $
**  $Revision: 38.1 $
**  $Date: 1996/11/26 01:42:16 $
**  $Locker:  $
**  $Author: floh $
**
**  Mouse-Render-Modul unter VGA und VESA.
**  Compile mit ss!=ds!
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <stdlib.h>

#include <i86.h>
#include <bios.h>

#include "nucleus/nucleus2.h"
#include "bitmap/vgaclass.h"

/*-----------------------------------------------------------------*/
void vga_MRestore(struct vga_data *vd)
/*
**  FUNCTION
**      Mauspointer wird ausgeblendet, indem die
**      die vorher gespeicherte Information in
**      den Bildpuffer zurückgeschrieben wird.
**
**  CHANGED
**      23-Nov-96   floh    created
*/
{
    if (vd->m_restore.valid) {

        UBYTE *vmem;
        UBYTE *data;
        ULONG i,j;

        vmem = vd->m_restore.start;
        data = &(vd->m_restore.data);

        for (i=0; i<vd->m_restore.height; i++) {
            for (j=0; j<vd->m_restore.width; j++) {
                *vmem++ = *data++;
            };
            vmem += vd->m_restore.off_add;
        };
        vd->m_restore.valid = FALSE;
    };
}

/*-----------------------------------------------------------------*/
void vga_MDraw(struct vga_data *vd)
/*
**  FUNCTION
**      Zeichnet Mauspointer, inklusive Restaurierung des alten
**      Backgrounds.
**
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    /*** erstmal Background restaurieren ***/
    vga_MRestore(vd);

    if (vd->ptr && ((vd->flags & VGAF_MouseHidden) == 0)) {

        UBYTE *vmem;
        UBYTE *ptr_data;
        UBYTE *bg_data;

        LONG width,height,i,j;
        LONG clipx,clipy;
        LONG bpr;       // BytesPerRow
        LONG addx;      // Line-Adder Pointer-Source-Data, falls X-Clipping

        /*** Pointer auf Start des Framebuffers und BytesPerRow ***/
        vmem = vd->d.Data;
        bpr  = vd->d.Width;

        /*** Pointer ins VMem, Höhe, Breite geclippt ***/
        vmem += ((vd->m_y * bpr) + vd->m_x);

        addx  = 0;
        width = vd->ptr->Width;
        clipx = vd->m_x + width - vd->d.Width;
        if (clipx > 0) {
            width -= clipx;
            addx   = clipx;
        };

        height = vd->ptr->Height;
        clipy  = vd->m_y + height - vd->d.Height;
        if (clipy > 0) height -= clipy;

        /*** Vmem-Zeilen-Adder... ***/
        bpr -= width;

        /*** restliche Pointer, Bg-Store neu initialisieren ***/
        vd->m_restore.valid   = TRUE;
        vd->m_restore.start   = vmem;
        vd->m_restore.width   = width;
        vd->m_restore.height  = height;
        vd->m_restore.off_add = bpr;

        ptr_data = vd->ptr->Data;
        bg_data  = &(vd->m_restore.data);

        /*** Pointer malen und Hintergrund sichern ***/
        for (i=0; i<height; i++) {
            for (j=0; j<width; j++) {
                UBYTE pix;
                *bg_data++ = *vmem;
                if (pix=*ptr_data++) *vmem=pix;
                vmem++;
            };
            vmem     += bpr;
            ptr_data += addx;
        };
    };
}

/*-----------------------------------------------------------------*/
void vga_MShow(struct vga_data *vd)
/*
**  FUNCTION
**      Mousepointer wieder einblenden, nachdem Modifikation
**      des Displays beendet ist.
**      WICHTIG: Die Routine muß innerhalb eines _disable(),
**      _enable() Paars aufgerufen werden.
**
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    vd->flags &= ~VGAF_MouseHidden;
    vga_MDraw(vd);
}

/*-----------------------------------------------------------------*/
void vga_MHide(struct vga_data *vd)
/*
**  FUNCTION
**      Mousepointer ausblenden, bevor Display direkt
**      manipuliert wird.
**      WICHTIG: Die Routine muß innerhalb eines _disable(),
**      _enable() Paars aufgerufen werden.
**
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    vga_MRestore(vd);
    vd->flags |= VGAF_MouseHidden;
}

/*-----------------------------------------------------------------*/
void vga_MFlush(struct vga_data *vd)
/*
**  FUNCTION
**      Funktioniert wie vga_MHide(), markiert aber
**      nur die Restore-Struktur als ungültig, und
**      läßt das Mauszeiger-Abbild sichtbar (schaltet
**      den Maus-Status aber auf Hidden). Damit
**      läßt sich vielleicht der Flimmereffekt minimieren.
**      WICHTIG: Die Routine muß innerhalb eines _disable(),
**      _enable() Paars aufgerufen werden.
**
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    vd->m_restore.valid = FALSE;
    vd->flags |= VGAF_MouseHidden;
}

/*-----------------------------------------------------------------*/
void vga_MCallBack(struct vga_data *vd, LONG x, LONG y)
/*
**  FUNCTION
**      Callback-Routine für "Mouse-Moved-Event". Wird
**      normalerweise von der Mouse-Input-Treiberklasse
**      direkt aufgerufen, sobald Mouseevents reinkommen.
**      Kümmert sich um die Darstellung des Mauszeigers.
**
**      Vorsicht: die Routine lebt in einem Interrupt.
**
**  CHANGED
**      25-Nov-96   floh    created
*/
{
    vd->m_x = x;
    vd->m_y = y;
    vga_MDraw(vd);
}

