#ifndef VISUALSTUFF_VESAKIT_H
#define VISUALSTUFF_VESAKIT_H
/*
**  $Source: PRG:VFM/Include/visualstuff/vesakit.h,v $
**  $Revision: 38.6 $
**  $Date: 1998/01/06 14:11:34 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für mein "funky little vesa kit", das
**  ein relativ einfach zu benutzendes API zur Ansteuerung
**  von VESA-Karten aus dem Protected Mode heraus sein soll.
**  Das ganze ist eingebettet in die VGA-gfx.engine, und nicht
**  von außen zugänglich, die Quelltexte sind aber einigermaßen
**  universell gehalten.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/*** ein "Realmode-Pointer" ***/
struct RMPtr {
    UWORD off;
    UWORD seg;
};

// Macro konvertiert Realmode-Pointer in <struct RMPtr> in
// Flat-Model-Near-Pointer (DOS/4GW-spezifisch!)
#define RM2PM(x) (((x)->seg<<4)+((x)->off))

/*** Info-Struktur für DPMI-Calls ***/
struct rminfo {
    long EDI;
    long ESI;
    long EBP;
    long reserved_by_system;
    long EBX;
    long EDX;
    long ECX;
    long EAX;
    short flags;
    short ES,DS,FS,GS,IP,CS,SP,SS;
};

/*** Return-Struktur Func00 - Realmode ***/
struct vesa_PrivVbeInfoBlock {

    /*** VBE 1.x ***/
    BYTE VbeSignature[4];
    WORD VbeVersion;
    struct RMPtr OemStringPtr;
    ULONG Capabilities;
    struct RMPtr VideoModePtr;
    UWORD TotalMemory;              // in 64kb Blöcken

    /*** VBE 2.0 ***/
    WORD OemSoftwareRev;
    struct RMPtr OemVendorNamePtr;
    struct RMPtr OemProductNamePtr;
    struct RMPtr OemProductRevPtr;
    UBYTE Reserved[222];
    UBYTE OemData[256];
};

/*** Protected Mode Interface Struktur ***/
struct vesa_PMInfo {
    UWORD setWindow;        // Offset für Set Window Call
    UWORD setDisplayStart;  // Offset für Set Display Start
    UWORD setPalette;       // Offset für Set Palette
    UWORD IOPrivInfo;       // I/O Privileg Info
    // ... ab hier Code und Tabellen
};

/*** Return-Struktur Func00 - für Flat Memory Model mit Near Ptrn ***/
struct vesa_VbeInfoBlock {

    /*** VBE x.x ***/
    UBYTE VbeSignature[4];
    WORD VbeVersion;
    UBYTE *OemStringPtr;
    ULONG Capabilities;
    WORD *VideoModePtr;
    UWORD TotalMemory;               // in 64kb Blöcken

    /*** VBE 2.0 ***/
    WORD OemSoftwareRev;
    UBYTE *OemVendorNamePtr;
    UBYTE *OemProductNamePtr;
    UBYTE *OemProductRevPtr;
    UBYTE Reserved[222];
    UBYTE OemData[256];

    /*** Protected Mode Interface ***/
    struct vesa_PMInfo *PMInfo;
    void (*pm_SetPalette) (ULONG,ULONG,ULONG,UBYTE *);  // aux Definieren!
#pragma aux pm_SetPalette parm [EBX] [ECX] [EDX] [EDI];
};

/*** VbeInfoBlock.Capabilities Flags ***/
#define VBECAPF_DAC8Bittable (1<<0) // DAC umschaltbar auf 8 bit per gun
#define VBECAPF_VGAComp      (1<<1) // Controller ist VGA kompatibel
#define VBECAPF_BlitBlankBit (1<<2) // VMem-Copy -> BlankBit setzen


/*** Return-Struktur Func01 - Realmode ***/
struct vesa_PrivModeInfoBlock {

    /*** VBE x.x ***/
    UWORD ModeAttributes;
    UBYTE WinAAttributes;
    UBYTE WinBAttributes;
    UWORD WinGranularity;
    UWORD WinSize;
    UWORD WinASegment;
    UWORD WinBSegment;
    struct RMPtr WinFuncPtr;
    UWORD BytesPerScanline;

    /*** VBE 1.2 ***/
    UWORD XResolution;
    UWORD YResolution;
    UBYTE XCharSize;
    UBYTE YCharSize;
    UBYTE NumberOfPlanes;
    UBYTE BitsPerPixel;
    UBYTE NumberOfBanks;
    UBYTE MemoryModel;
    UBYTE BankSize;
    UBYTE NumberOfImagePages;
    UBYTE Reserved;

    /*** Direct Color Fields ***/
    UBYTE RedMaskSize;
    UBYTE RedFieldPosition;
    UBYTE GreenMaskSize;
    UBYTE GreenFieldPosition;
    UBYTE BlueMaskSize;
    UBYTE BlueFieldPosition;
    UBYTE RsvdMaskSize;
    UBYTE RsvdFieldPosition;
    UBYTE DirectColorModeInfo;

    /*** VBE 2.0 ***/
    ULONG PhysBasePtr;
    ULONG OffScreenMemOffset;
    UWORD OffScreenMemSize;
    UBYTE Reserved_1[206];
};

/*** Die folgende Public Version des ModeInfoBlocks ***/
/*** gibt nur einen Bruchteil der von VBE2.x zurück-***/
/*** gelieferten Information. Halt nur das, was für ***/
/*** das Linear Frame Buffer Model benötigt wird.   ***/

struct vesa_ModeInfoBlock {
    UWORD ModeAttributes;

    UWORD XResolution;
    UWORD YResolution;
    UWORD reserved1;

    UBYTE BitsPerPixel;
    UBYTE MemoryModel;
    UBYTE NumberOfImagePages;
    UBYTE reserved2;

    UBYTE *PhysBasePtr;
    ULONG OffScreenMemOffset;
    UWORD OffScreenMemSize;
};

/*** vesa_ModeInfoBlock.ModeAttributes ***/
#define VBEMIF_SuppInHardware   (1<<0)
#define VBEMIF_Reserved         (1<<1)
#define VBEMIF_TTYOutput        (1<<2)
#define VBEMIF_ColorMode        (1<<3)
#define VBEMIF_GraphicsMode     (1<<4)
#define VBEMIF_NotVGACompatible (1<<5)
#define VBEMIF_NotWindowed      (1<<6)
#define VBEMIF_LinearAvail      (1<<7)

/*** vesa_ModeInfoBlock.MemoryModel ***/
#define VBEMM_TEXTMODE      (0x0)
#define VBEMM_CGA           (0x1)
#define VBEMM_HERCULES      (0x2)
#define VBEMM_PLANAR        (0x3)
#define VBEMM_PACKED        (0x4)
#define VBEMM_NONCHAIN4     (0x5)       // ???
#define VBEMM_DIRECTCOLOR   (0x6)
#define VBEMM_YUV           (0x7)

/*** VBE Paletten Entry ***/
struct vesa_PalEntry {
    UBYTE blue;
    UBYTE green;
    UBYTE red;
    UBYTE alpha;
};

/*-------------------------------------------------------------------
**  Protos des VESAkits
*/
BOOL vesa_InitVESAInfo(struct vesa_VbeInfoBlock *);
void vesa_KillVESAInfo(struct vesa_VbeInfoBlock *);
BOOL vesa_GetModeInfo(ULONG, struct vesa_VbeInfoBlock *, struct vesa_ModeInfoBlock *);
void vesa_SetLinearMode(ULONG);
void vesa_SetPalette(struct vesa_VbeInfoBlock *,ULONG,ULONG,UBYTE *);
void vesa_Set8BitDAC(struct vesa_VbeInfoBlock *);
/*-----------------------------------------------------------------*/
#endif



