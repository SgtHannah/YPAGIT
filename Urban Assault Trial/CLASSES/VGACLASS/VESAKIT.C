/*
**  $Source: PRG:VFM/Classes/_VgaClass/vesakit.c,v $
**  $Revision: 38.5 $
**  $Date: 1997/01/20 22:42:18 $
**  $Locker:  $
**  $Author: floh $
**
**  Ein paar klitzekleine VESA-Support-Funktionen...
**
**  (C) Copyright 1995 by A.Weissflog
*/
/*** Amiga Includes ***/
#include <exec/types.h>
#include <string.h>

/*** x86 Zeugs ***/
#include <i86.h>

/*** VESA Kit Includes ***/
#include "visualstuff/vesakit.h"

/*** aus setpal.asm ***/
extern void vga_SetPal(UBYTE *);
extern void vbe_SetPal(void(*)(),UBYTE *);

/*-----------------------------------------------------------------*/
ULONG vesa_InitPModeInterface(struct vesa_VbeInfoBlock *vib)
/*
**  FUNCTION
**      Initialisiert das VESA2-Protected-Mode-Interface,
**      und initialisiert einen Pointer auf die
**      "Set/Get Primary Palette" Funktion.
**
**  INPUTS
**      vib     - Ptr auf teilweise initialisierten VIB
**
**  RESULTS
**      TRUE: alles OK, und:
**          vib->PMInfo       - 32-Bit-Near-Pointer auf PMInfo-Block
**          vib->pmSetPalette - Funktions-Pointer auf Set Palette
**      FALSE:  Fehler aufgetreten
**
**  CHANGED
**      26-Nov-96   floh    created
*/
{
    union REGS regs;
    struct SREGS sregs;
    struct rminfo rmi;

    /*** sim RealMode Interrupt: VESA Get Protected Mode Interface ***/
    memset(&rmi,0,sizeof(rmi));
    memset(&regs,0,sizeof(regs));
    memset(&sregs,0,sizeof(sregs));
    rmi.EAX = 0x4f0A;
    rmi.EBX = 0;

    regs.w.ax = 0x0300;     // DPMI: simulate RealMode interrupt
    regs.h.bl = 0x10;
    regs.h.bh = 0;
    regs.w.cx = 0;
    sregs.es   = FP_SEG(&rmi);
    regs.x.edi = FP_OFF(&rmi);
    int386x(0x31, &regs, &regs, &sregs);
    if ((rmi.EAX & 0xffff) == 0x004f) {
        ULONG pm_size = rmi.ECX;
        if (vib->PMInfo = malloc(pm_size)) {

            struct RMPtr real_ptr;
            void *real_pminfo;

            real_ptr.off = rmi.EDI;
            real_ptr.seg = rmi.ES;
            real_pminfo  = RM2PM(&real_ptr);
            memcpy(vib->PMInfo,real_pminfo,pm_size);
            vib->pm_SetPalette = (void(*)())
                                 (((UBYTE *)vib->PMInfo) + vib->PMInfo->setPalette);

            return(TRUE);
        };
    };

    return(FALSE);
}

/*-----------------------------------------------------------------*/
void vesa_KillPModeInterface(struct vesa_VbeInfoBlock *vib)
/*
**  FUNCTION
**      Gegenstück zu vesa_InitPModeInterface().
**
**  CHANGED
**      26-Nov-96   floh    created
*/
{
    if (vib->PMInfo) {
        free(vib->PMInfo);
        vib->PMInfo = NULL;
    };
}

/*-----------------------------------------------------------------*/
BOOL vesa_InitVESAInfo(struct vesa_VbeInfoBlock *vib)
/*
**  FUNCTION
**      Ermittelt, ob VESA 2.x präsent ist und füllt die übergebene
**      vesa_VbeInfoBlock-Struktur aus.
**
**  INPUTS
**      vib -> Ist ein Pointer auf eine vesa_VbeInfoBlock-Struktur,
**             die mit dem Resultat des VESA-Calls 0x0 ausgefüllt
**             ist. Alle Pointer sind in gültige 32-Bit-NEAR-Pointer
**             umgewandelt.
**
**  RESULTS
**      TRUE    -> VESA 2.x API wird unterstützt
**      FALSE   -> kein VESA, oder nur VESA 1.x verfügbar. Die übergebene
**                 Struktur ist nicht, oder nur teilweise ausgefüllt,
**                 also Finger wech...
**
**  CHANGED
**      17-Aug-95   floh    created
**      18-Aug-95   floh    neu
**      30-Aug-95   floh    jetzt auf VESA 2.x umgemodelt
**      26-Nov-96   floh    + initialisiert PMode-Interface
*/
{
    union REGS regs;
    struct SREGS sregs;
    struct rminfo rmi;
    UWORD sel, seg;
    BOOL vesa2 = FALSE;
    struct vesa_PrivVbeInfoBlock priv;

    UBYTE far *dosblock;

    /*** DPMI-Call 0x100: allokiere DOS-Mem für VESA-Info-Block ***/
    memset(&sregs,0,sizeof(sregs));
    regs.w.ax = 0x0100;
    regs.w.bx = 32;     // 512 Bytes
    int386x(0x31, &regs, &regs, &sregs);
    seg = regs.w.ax;
    sel = regs.w.dx;

    /*** wir wollen VBE2 Support... ***/
    dosblock = MK_FP(sel,0);
    dosblock[0] = 'V';
    dosblock[1] = 'B';
    dosblock[2] = 'E';
    dosblock[3] = '2';

    /*** simuliere RealMode Interrupt via DPMI ***/
    memset(&rmi,0,sizeof(rmi));
    rmi.EAX = 0x4f00;
    rmi.EDI = 0;        // Offset des Info-Blocks
    rmi.ES  = seg;      // Segment des Info-Blocks

    regs.w.ax = 0x0300;     // DPMI: simulate RealMode interrupt
    regs.h.bl = 0x10;
    regs.h.bh = 0;
    regs.w.cx = 0;
    sregs.es   = FP_SEG(&rmi);
    regs.x.edi = FP_OFF(&rmi);
    int386x(0x31, &regs, &regs, &sregs);

    /*** VESA unterstützt? ***/
    if ((rmi.EAX & 0xffff) == 0x004f) {

        /*** das Resultat in Sicherheit bringen ***/
        UBYTE far *to;

        to = MK_FP(FP_SEG(&priv), FP_OFF(&priv));
        _fmemcpy(to, dosblock, 512);

        /*** public VbeInfoBlock ausfüllen (+ Pointer konvertieren) ***/
        memcpy(vib, &priv, 512);
        vib->OemStringPtr = (UBYTE *) RM2PM(&(priv.OemStringPtr));
        vib->VideoModePtr = (WORD *)  RM2PM(&(priv.VideoModePtr));

        /*** VESA 2.0 unterstützt? ***/
        if (vib->VbeVersion >= 0x200) {
            vib->OemVendorNamePtr  = (UBYTE *) RM2PM(&(priv.OemVendorNamePtr));
            vib->OemProductNamePtr = (UBYTE *) RM2PM(&(priv.OemProductNamePtr));
            vib->OemProductRevPtr  = (UBYTE *) RM2PM(&(priv.OemProductRevPtr));
            vesa_InitPModeInterface(vib);
            vesa2 = TRUE;
        };
    };

    /*** Dos-Mem freigeben per DPMI call 0x101 ***/
    memset(&sregs,0,sizeof(sregs));
    regs.w.ax = 0x0101;
    regs.w.dx = sel;
    int386x(0x31, &regs, &regs, &sregs);

    /*** :-))) ***/
    return(vesa2);
}

/*-----------------------------------------------------------------*/
void vesa_KillVESAInfo(struct vesa_VbeInfoBlock *vib)
/*
**  CHANGED
**      26-Nov-96   floh    created
*/
{
    vesa_KillPModeInterface(vib);
}

/*-----------------------------------------------------------------*/
BOOL vesa_GetModeInfo(ULONG vm,
                      struct vesa_VbeInfoBlock *vib,
                      struct vesa_ModeInfoBlock *mib)
/*
**  FUNCTION
**      Erfragt Infos über einen VESA-VideoModus. Es wird nicht
**      getestet, inwieweit der Modus Sinn hat.
**      Also Vorsicht...
**
**      *** WICHTIG *** die Routine darf nur angewendet werden,
**      wenn vesa_GetVESAInfo() mit TRUE zurückkam!
**
**  INPUTS
**      vm  -> VESA(!)-Video-Modus
**      vib -> die mittels vesa_GetVESAInfo() ausgefüllte
**             vesa_VbeInfoBlock-Struktur
**      mib -> Pointer auf eine leere <struct vesa_ModeInfoBlock>,
**             die mit den Eigenschaften des aktivierten Modus
**             ausgefüllt wird (nur wenn die Routine mit TRUE
**             zurückkommt).
**
**  RESULTS
**      TRUE    -> alles OK, <mib> ist gültig
**      FALSE   -> Modus nicht verfügbar, oder kein linearer
**                 Framebuffer unterstützt
**
**  CHANGED
**      17-Aug-95   floh    created
**      18-Aug-95   floh    VGA Mode 0x13 ab jetzt nicht mehr unterstützt.
**                          + veränderte Parameter-Übergabe
**                          + gesplittet in vesa_GetModeInfo() und
**                            vesa_SetMode()
**      30-Aug-95   floh    für VESA2.x umgemodelt
**      01-Sep-95   floh    mib->PhysBasePtr wird jetzt korrekt per
**                          DPMI-Call 0x800 in den linearen Adressraum
**                          gemappt. Das heißt, in mib->PhysBasePtr
**                          ist ein normaler 32bit Near-Pointer zu finden.
*/
{
    union REGS regs;
    struct SREGS sregs;
    struct rminfo rmi;
    UWORD sel, seg;
    BOOL exists;
    WORD *modes;
    UBYTE far *dosblock;
    UBYTE far *to;

    struct vesa_PrivModeInfoBlock priv;

    BOOL retval = FALSE;

    /*** checken, ob der benötigte Mode unterstützt wird ***/
    modes  = vib->VideoModePtr;
    exists = FALSE;
    while (*modes != -1) {
        if (vm == *modes++) {
            exists=TRUE;
            break;
        };
    };
    if (!exists) return(FALSE);

    /*** Mode-Eigenschaften ermitteln ***/

    /* per DPMI call 0x100 DosMem für ModeInfoBlock allokieren */
    memset(&sregs,0,sizeof(sregs));
    regs.w.ax = 0x0100;
    regs.w.bx = 16;         // 256 Bytes
    int386x(0x31, &regs, &regs, &sregs);
    seg = regs.w.ax;
    sel = regs.w.dx;
    dosblock = MK_FP(sel,0);

    /*** ModeInfoBlock ausfüllen lassen ***/
    memset(&rmi,0,sizeof(rmi));
    rmi.EAX = 0x4f01;
    rmi.ECX = vm;
    rmi.EDI = 0;
    rmi.ES  = seg;

    regs.w.ax = 0x0300;     // DPMI: simulate RealMode Interrupt
    regs.h.bl = 0x10;
    regs.h.bh = 0;
    regs.w.cx = 0;
    sregs.es   = FP_SEG(&rmi);
    regs.x.edi = FP_OFF(&rmi);
    int386x(0x31, &regs, &regs, &sregs);

    /*** oki doki... jetzt ModeInfoBlock in sicheren Bereich kopieren ***/
    to = MK_FP(FP_SEG(&priv), FP_OFF(&priv));
    _fmemcpy(to, dosblock, 256);

    /*** public vesa_ModeInfoBlock ausfüllen ***/
    mib->ModeAttributes = priv.ModeAttributes;
    mib->XResolution    = priv.XResolution;
    mib->YResolution    = priv.YResolution;
    mib->BitsPerPixel   = priv.BitsPerPixel;
    mib->MemoryModel    = priv.MemoryModel;
    mib->NumberOfImagePages = priv.NumberOfImagePages;
    mib->PhysBasePtr    = (UBYTE *) priv.PhysBasePtr;
    mib->OffScreenMemOffset = priv.OffScreenMemOffset;
    mib->OffScreenMemSize   = priv.OffScreenMemSize;

    /*** DOS-Mem freigeben ***/
    memset(&sregs,0,sizeof(sregs));
    regs.w.ax = 0x0101;
    regs.w.dx = sel;
    int386x(0x31, &regs, &regs, &sregs);

    /*** werden die geforderten Eigenschaften unterstützt? ***/
    if ((mib->PhysBasePtr) && (mib->ModeAttributes & VBEMIF_LinearAvail)) {

        ULONG phys = (ULONG) mib->PhysBasePtr;
        ULONG size = (1<<22)-1;         // max. 4MB

        /*** den PhysBasePtr ummappen ***/
        regs.w.ax = 0x800;      // DPMI-Call 0x800
        regs.w.bx = (UWORD) (phys>>16);
        regs.w.cx = (UWORD) (phys);
        regs.w.si = (UWORD) (size>>16);
        regs.w.di = (UWORD) (size);
        int386(0x31, &regs, &regs);
        mib->PhysBasePtr = (UBYTE *) ((regs.w.bx<<16)|(regs.w.cx));
        retval = TRUE;
    };

    /*** That's it! ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
void vesa_SetLinearMode(ULONG vm)
/*
**  FUNCTION
**      Aktiviert den gewünschten VESA(!)-Modus. Vorher
**      sollte mittels vesa_GetModeInfo() Informationen
**      über den gewünschten Modus ermittelt und ausgewertet
**      worden sein. Darf nur angewendet werden, wenn
**      vesa_GetModeInfo() auf diesen Mode mit TRUE zurückkam.
**
**      Das Linear Frame Buffer Flag wird gesetzt...
**
**  INPUTS
**      vm  -> der gewünschte VESA(!)-Videomode.
**
**  RESULTS
**      ---
**
**  CHANGED
**      18-Aug-95   floh    created
**      30-Aug-95   floh    Aufruf jetzt per DPMI Simulate RealMode Int,
**                          Linear Frame Buffer Model Flag wird gesetzt
*/
{
    struct rminfo rmi;
    union REGS regs;
    struct SREGS sregs;

    memset(&rmi,0,sizeof(rmi));
    rmi.EAX = 0x4f02;
    rmi.EBX = (vm | (1<<14));

    memset(&sregs,0,sizeof(sregs));
    regs.w.ax = 0x0300;     // DPMI: simulate RealMode Interrupt
    regs.h.bl = 0x10;
    regs.h.bh = 0;
    regs.w.cx = 0;
    sregs.es   = FP_SEG(&rmi);
    regs.x.edi = FP_OFF(&rmi);
    int386x(0x31, &regs, &regs, &sregs);
}

/*-----------------------------------------------------------------*/
void vesa_SetPalette(struct vesa_VbeInfoBlock *vib,
                     ULONG is_vesa2, ULONG is_8bpg,
                     UBYTE *pal)
/*
**  FUNCTION
**      Modifiziert Palette, wartet auf v-blank.
**
**  INPUT
**      vib         - initialisierter VbeInfoBlock
**      is_vesa2    - TRUE, falls VESA-2 initialisiert
**      is_8bpg     - TRUE, falls DAC auf 8bpg
**      pal         - Ptr auf Palette, 256 * (UBYTE) RGB
**
**  CHANGED
**      26-Nov-96   floh    created
*/
{
    ULONG i;
    if (is_vesa2 && vib->pm_SetPalette) {

        struct vesa_PalEntry vbe_pal[256];

        if (is_8bpg) {
            /*** 8bpg Palette konvertieren ***/
            ULONG vbe_params[2];
            for (i=0; i<256; i++) {
                vbe_pal[i].red   = *pal++;
                vbe_pal[i].green = *pal++;
                vbe_pal[i].blue  = *pal++;
                vbe_pal[i].alpha = 0;
            };
        } else {
            /*** 6bpg Palette konvertieren ***/
            for (i=0; i<256; i++) {
                vbe_pal[i].red   = *pal++ >> 2;
                vbe_pal[i].green = *pal++ >> 2;
                vbe_pal[i].blue  = *pal++ >> 2;
                vbe_pal[i].alpha = 0;
            };
        };
        vbe_SetPal(vib->pm_SetPalette,vbe_pal);

    } else {

        UBYTE vga_pal[256*3];
        UBYTE *act = vga_pal;

        /*** Fallback auf Standard-VGA-Verfahren ***/
        for (i=0; i<256; i++) {
            *act++ = *pal++ >> 2;
            *act++ = *pal++ >> 2;
            *act++ = *pal++ >> 2;
        };
        vga_SetPal(vga_pal);
    };
}

/*-----------------------------------------------------------------*/
void vesa_Set8BitDAC(struct vesa_VbeInfoBlock *vib)
/*
**  FUNCTION
**      Setzt DAC auf 8 bpg Modus.
**
**  CHANGED
**      26-Nov-96   floh    created
*/
{
    if (vib->Capabilities & VBECAPF_DAC8Bittable) {

        struct rminfo rmi;
        union REGS regs;
        struct SREGS sregs;

        memset(&rmi,0,sizeof(rmi));
        memset(&regs,0,sizeof(regs));
        memset(&sregs,0,sizeof(sregs));

        rmi.EAX = 0x4f08;
        rmi.EBX = 0x0800;   // bl=Set DAC Palette Format, bh=8 Bits Per Gun

        memset(&sregs,0,sizeof(sregs));
        regs.w.ax = 0x0300;     // DPMI: simulate RealMode Interrupt
        regs.h.bl = 0x10;
        regs.h.bh = 0;
        regs.w.cx = 0;
        sregs.es   = FP_SEG(&rmi);
        regs.x.edi = FP_OFF(&rmi);
        int386x(0x31, &regs, &regs, &sregs);
    };
}
