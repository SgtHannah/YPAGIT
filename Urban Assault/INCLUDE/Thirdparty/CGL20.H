/*
 * Copyright (c) Creative Technology Ltd, 1996. All Rights Reserved
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * cgl20.h
 *   - CGL 2.x structures
 *
 * 24 Apr 96 -    First version (tclee)
 */

#ifndef __CGL_20__
#define __CGL_20__

#pragma pack(push, 4);

// CGL 2.x surface structure (subject to change)
typedef struct CGL_SURFACE {
   CGL_UINT32  dwPitch;
   CGL_UINT32  dwWidth;
   CGL_UINT32  dwHeight;
   CGL_UINT32  dwFormat;
   CGL_UINT32  dwSurfaceType;
   CGL_UINT8LP lpAddress;
} CGL_SURFACE_ST, *CGL_SURFACE_PST;

// Event notification
typedef void (CGLAPI *CGL_EVENTPROC)(CGL_UINT32 dwEventID);

typedef enum {
   CGL_EVENT_DISPLAYSW   = 1,
   CGL_EVENT_VRETRACE    = 2,
   CGL_EVENT_SCANLINE    = 3,
   CGL_EVENT_TEXLOADED   = 4,
} CGL_EVENT_ID;

// Device capability class IDs
typedef enum {
   CGL_DEVCAP_TEXTURE    = 1,
   CGL_DEVCAP_PRIMITIVES = 2,
   CGL_DEVCAP_LOGICOPS   = 3,
   CGL_DEVCAP_SURFACES   = 4,
   CGL_DEVCAP_CURSOR     = 5,
   CGL_DEVCAP_EVENT      = 6,

   // various engines
   CGL_DEVCAP_RENDER     = 7,
   CGL_DEVCAP_GEOMETRY   = 8,
} CGL_DEVCAP_ID;

// Rendering class capability bits
typedef struct {
   CGL_UINT32 dwSize;

   CGL_UINT32 fShade     : 1;
   CGL_UINT32 fTexture   : 1;
   CGL_UINT32 fAlpha     : 1;
   CGL_UINT32 fFog       : 1;
   CGL_UINT32 fPattern   : 1;
   CGL_UINT32 fStipple   : 1;

   CGL_UINT32 fSpecular  : 1;           // Permedia/FX ready
} CGL_DEVCAP_RENDER_ST;

// Texture class capability bits
typedef struct {
   CGL_UINT32 dwSize;

   // formats (1 = hardware support, 2 = software emulated)
   CGL_UINT32 fRGB5551   : 2;
   CGL_UINT32 fRGB4444   : 2;
   CGL_UINT32 fRGB8888   : 2;
   CGL_UINT32 fRGB565    : 2;
   CGL_UINT32 fRGB2321   : 2;
   CGL_UINT32 fRGB332    : 2;
   CGL_UINT32 fRGB232    : 2;
   CGL_UINT32 fCI8       : 2;
   CGL_UINT32 f4I5551    : 2;
   CGL_UINT32 f4I4444    : 2;
   CGL_UINT32 f4I565     : 2;
   CGL_UINT32 fYUV444    : 2;           // Permedia/FX ready
   CGL_UINT32 fYUV422    : 2;
   CGL_UINT32 fYUV411    : 2;           // Permedia/FX ready

   // mapping modes
   CGL_UINT32 fChroma    : 1;
   CGL_UINT32 fOverlay   : 1;

   // filters
   CGL_UINT32 fBilinear  : 1;
   CGL_UINT32 fTrilinear : 1;           // Cirrus/M2 ready

   // wrap modes
   CGL_UINT32 fRepeat    : 1;
   CGL_UINT32 fClamp     : 1;
   CGL_UINT32 fMirror    : 1;

} CGL_DEVCAP_TEXTURE_ST, *CGL_DEVCAP_TEXTURE_PST;

// Rendering primitives
typedef struct {
   CGL_UINT32 dwSize;

   CGL_UINT32 fPOINT     : 1;
   CGL_UINT32 fLINE      : 1;
   CGL_UINT32 fLINESTRIP : 1;
   CGL_UINT32 fLINELOOP  : 1;

   CGL_UINT32 fTRI       : 1;
   CGL_UINT32 fTRIFAN    : 1;
   CGL_UINT32 fTRISTRIP  : 1;

   CGL_UINT32 fRECT      : 1;
   CGL_UINT32 fQUAD      : 1;
   CGL_UINT32 fQUADSTRIP : 1;

   CGL_UINT32 fAAEDGE    : 1;
   CGL_UINT32 fAAEDGESTRIP : 1;
} CGL_DEVCAP_PRIMITIVES_ST, *CGL_DEVCAP_PRIMITIVES_PST;

// Logical Ops capability bits
typedef struct {
   CGL_UINT32 dwSize;

   CGL_UINT32 fCLEAR     : 1;
   CGL_UINT32 fSET       : 1;
   CGL_UINT32 fCOPY      : 1;
   CGL_UINT32 fINVERT    : 1;
   CGL_UINT32 fNOP       : 1;
   CGL_UINT32 fAND       : 1;
   CGL_UINT32 fOR        : 1;
   CGL_UINT32 fXOR       : 1;
   CGL_UINT32 fNOT       : 1;
   CGL_UINT32 fNAND      : 1;
   CGL_UINT32 fNOR       : 1;
   CGL_UINT32 fNOT_XOR   : 1;
   CGL_UINT32 fNOT_AND   : 1;
   CGL_UINT32 fNOT_OR    : 1;
   CGL_UINT32 fAND_NOT   : 1;
   CGL_UINT32 fOR_NOT    : 1;
} CGL_DEVCAP_LOGICOPS_ST;

// Hardware Surface capability bits
typedef struct {
   CGL_UINT32 dwSize;

   CGL_UINT32 fDepth     : 1;
   CGL_UINT32 fStencil   : 1;
} CGL_DEVCAP_SURFACES_ST;

// Cursor class capability bits
typedef struct {
   CGL_UINT32 dwSize;

   CGL_UINT32 f32x32x2   : 1;
   CGL_UINT32 f32x32x2HL : 1;
   CGL_UINT32 f32x32x3   : 1;
   CGL_UINT32 f64x64x2   : 1;
   CGL_UINT32 f64x64x2HL : 1;
   CGL_UINT32 f64x64x3   : 1;
} CGL_DEVCAP_CURSOR_ST;

// Event class capability bits
typedef struct {
   CGL_UINT32 dwSize;

   CGL_UINT32 fDisplaySw : 1;
   CGL_UINT32 fVRetrace  : 1;
   CGL_UINT32 fScanline  : 1;
   CGL_UINT32 fDMADone   : 1;
} CGL_DEVCAP_EVENT_ST;

#pragma pack(pop);

#endif /* __CGL_20__ */