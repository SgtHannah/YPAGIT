#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H
/*
**  $Source: PRG:VFM/Include/engine/engine.h,v $
**  $Revision: 38.70 $
**  $Date: 1998/01/06 12:56:30 $
**  $Locker:  $
**  $Author: floh $
**
**  Für alle Engines geltenden Definitionen.
**  (C) Copyright 1993,1994,1995,1996 by a.weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef TYPES_H
#include "types.h"
#endif

#ifndef NUCLEUS_NUCLEUS2_H
#include "nucleus/nucleus2.h"
#endif

#ifndef INPUT_IE_H
#include "input/ie.h"
#endif

#ifndef POLYGON_POLYGON_H
#include "polygon/polygon.h"
#endif

#ifndef TRANSFORM_TFORM_H
#include "transform/tform.h"
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"
#endif

#ifndef VISUALSTUFF_FONT_H
#include "visualstuff/font.h"
#endif

#ifndef AUDIO_AUDIOENGINE_H
#include "audio/audioengine.h"
#endif

#ifndef BITMAP_RASTERCLASS_H
#include "bitmap/rasterclass.h"
#endif

/*===================================================================
**  Die GET der OutputVisual-Engine als C-Struktur
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
struct ove_GET_Specific {
    void __asm (*SetDisplay) (__d0 UWORD);       /* not yet implemented */
    void __asm (*LoadDisplay) (__d0 UWORD);      /* not yet implemented */
    void __asm (*WaitTOF) (void);                /* not yet implemented */
    void __asm (*ToggleDisplays) (void);
    void __asm (*ClearDisplay) (__d0 ULONG);
    BOOL __asm (*LoadColorMap) (__a0 UBYTE *name);
    void __asm (*DrawBitmap) (__a0 struct VFMBitmap *bmp, __d0 WORD x, __d1 WORD y);
    void __asm (*DrawScaledBitmap) (__a0 struct VFMBitmap *bmp, __d0 WORD x, __d1 WORD y, __d2 UWORD sizex, __d3 UWORD sizey, __d4 UWORD fade, __d5 ULONG flags);
    void __asm (*SetBlurMode) (__d0 ULONG blurmode);
    void __asm (*DrawPolygon) (__a0 struct rast_poly *poly);
    void __asm (*SetFont) (__a0 struct VFMFont *font, __d0 UBYTE id);
    void __asm (*DrawText) (__a0 struct drawtext_args *args);
    ULONG __asm (*TextLen) (__d0 UBYTE id, __a0 UBYTE *str, __d1 ULONG cnt);
    struct VFMFont * __asm (*GetFont) (__d0 UBYTE id);
    void __asm (*FlushGfx) (void);
    void __asm (*OccupyRegion) (__d0 WORD x, __d1 WORD y, __d2 WORD dx, __d3 WORD dy);
    void __asm (*DrawLine) (__d0 LONG x0, __d1 LONG y0, __d2 LONG x1, __d3 LONG y1, __d4 UBYTE color);
    void __asm (*DrawClippedLine) (__d0 LONG x0, __d1 LONG y0, __d2 LONG x1, __d3 LONG y1, __d4 UBYTE color, __a0 struct VFMRect *clip);

};
#endif
#endif

/* ...und als normale Prototypen */
#ifdef AMIGA
void __asm ove_SetDisplay(__d0 UWORD);       /* not yet implemented */
void __asm ove_LoadDisplay(__d0 UWORD);      /* not yet implemented */
void __asm ove_WaitTOF(void);                /* not yet implemented */
void __asm ove_ToggleDisplays(void);
void __asm ove_ClearDisplay(__d0 ULONG);
BOOL __asm ove_LoadColorMap(__a0 UBYTE *name);
void __asm ove_DrawBitmap(__a0 struct VFMBitmap *bmp, __d0 WORD x, __d1 WORD y);
void __asm ove_DrawScaledBitmap(__a0 struct VFMBitmap *bmp, __d0 WORD x, __d1 WORD y, __d2 UWORD sizex, __d3 UWORD sizey, __d4 UWORD fade, __d5 ULONG flags);
void __asm ove_SetBlurMode(__d0 ULONG blurmode);
void __asm ove_DrawPolygon(__a0 struct rast_poly *poly);
void __asm ove_SetFont(__a0 struct VFMFont *font, __d0 UBYTE id);
void __asm ove_DrawText(__a0 struct drawtext_args *args);
ULONG __asm ove_TextLen(__d0 UBYTE id, __a0 UBYTE *str, __d1 ULONG cnt);
struct VFMFont * __asm ove_GetFont(__d0 UBYTE id);
void __asm ove_FlushGfx(void);
void __asm ove_OccupyRegion(__d0 WORD x, __d1 WORD y, __d2 WORD dx, __d3 WORD dy);
void __asm ove_DrawLine(__d0 LONG x0, __d1 LONG y0, __d2 LONG x1, __d3 LONG y1, __d4 UBYTE color);
void __asm ove_DrawClippedLine(__d0 LONG x0, __d1 LONG y0, __d2 LONG x1, __d3 LONG y1, __d4 UBYTE color, __a0 struct VFMRect *clip);
#else
void ove_SetDisplay(UWORD);       /* not yet implemented */
void ove_LoadDisplay(UWORD);      /* not yet implemented */
void ove_WaitTOF(void);           /* not yet implemented */
void ove_ToggleDisplays(void);
void ove_ClearDisplay(ULONG);
BOOL ove_LoadColorMap(UBYTE *name);
void ove_DrawBitmap(struct VFMBitmap *bmp, WORD x, WORD y);
void ove_DrawScaledBitmap(struct VFMBitmap *bmp, WORD x, WORD y, UWORD sizex, UWORD sizey, UWORD fade, ULONG flags);
void ove_SetBlurMode(ULONG blurmode);
void ove_DrawPolygon(struct rast_poly *poly);
void ove_SetFont(struct VFMFont *font, UBYTE id);
void ove_DrawText(struct drawtext_args *args);
ULONG ove_TextLen(UBYTE id, UBYTE *str, ULONG cnt);
struct VFMFont *ove_GetFont(UBYTE id);
void ove_FlushGfx(void);
void ove_OccupyRegion(WORD x, WORD y, WORD dx, WORD dy);
void ove_DrawLine(LONG x0, LONG y0, LONG x1, LONG y1, UBYTE color);
void ove_DrawClippedLine(LONG x0, LONG y0, LONG x1, LONG y1, UBYTE color, struct VFMRect *clip);
#endif

/*-------------------------------------------------------------------
**  Die Compatibilty-Macros für die Gfx-Engine
*/
#ifdef DYNAMIC_LINKING
#define _SetDisplay       OVE_GET_SPEC->SetDisplay
#define _LoadDisplay      OVE_GET_SPEC->LoadDisplay
#define _WaitTOF          OVE_GET_SPEC->WaitTOF
#define _ToggleDisplays   OVE_GET_SPEC->ToggleDisplays
#define _ClearDisplay     OVE_GET_SPEC->ClearDisplay
#define _LoadColorMap     OVE_GET_SPEC->LoadColorMap
#define _DrawBitmap       OVE_GET_SPEC->DrawBitmap
#define _DrawScaledBitmap OVE_GET_SPEC->DrawScaledBitmap
#define _SetBlurMode      OVE_GET_SPEC->SetBlurMode
#define _DrawPolygon      OVE_GET_SPEC->DrawPolygon
#define _SetFont          OVE_GET_SPEC->SetFont
#define _DrawText         OVE_GET_SPEC->DrawText
#define _TextLen          OVE_GET_SPEC->TextLen
#define _GetFont          OVE_GET_SPEC->GetFont
#define _FlushGfx         OVE_GET_SPEC->FlushGfx
#define _OccupyRegion     OVE_GET_SPEC->OccupyRegion
#define _DrawLine         OVE_GET_SPEC->DrawLine
#define _DrawClippedLine  OVE_GET_SPEC->DrawClippedLine
#endif
#ifdef STATIC_LINKING
#define _SetDisplay       ove_SetDisplay
#define _LoadDisplay      ove_LoadDisplay
#define _WaitTOF          ove_WaitTOF
#define _ToggleDisplays   ove_ToggleDisplays
#define _ClearDisplay     ove_ClearDisplay
#define _LoadColorMap     ove_LoadColorMap
#define _DrawBitmap       ove_DrawBitmap
#define _DrawScaledBitmap ove_DrawScaledBitmap
#define _SetBlurMode      ove_SetBlurMode
#define _DrawPolygon      ove_DrawPolygon
#define _SetFont          ove_SetFont
#define _DrawText         ove_DrawText
#define _TextLen          ove_TextLen
#define _GetFont          ove_GetFont
#define _FlushGfx         ove_FlushGfx
#define _OccupyRegion     ove_OccupyRegion
#define _DrawLine         ove_DrawLine
#define _DrawClippedLine  ove_DrawClippedLine
#endif

/*===================================================================
**  die (nicht mehr) experimentelle NG-Transformer-Engine
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
struct teng_GET_Specific {
    void (*Obsolete_1) (void);
    void (*Obsolete_2) (void);
    void __asm (*SetViewer) (__a0 tform *viewer);
    tform *__asm (*GetViewer) (void);
    void (*Obsolete_3) (void);
    void (*Obsolete_4) (void);
    void __asm (*ViewerToDisplay) (__a0 fp3d *viewerpool, __a1 Point2D *displaypool);
    LONG __asm (*Get3DClipCodes) (__a0 fp3d *pool3D);
    void __asm (*Clip3DPolygon) (__a0 fp3d *psource, __a1 fp3d *pclipped, __d0 struct VFMOutline *mapsource, __d1 struct VFMOutline *mapclipped, __d3 ULONG orclipcode);
    void (*Obsolete_5) (void);
    BOOL __asm (*PolygonPipeline) (__a0 struct PolygonInfo *poly_info);
    ULONG __asm (*Isolate3DPolygon) (__a0 fp3d *pool, __a1 fp3d *polygon, __a2 WORD *point_numbers, __a3 fp3d *norm_vec);
    void __asm (*RefreshTForm) (__a0 tform *tf);
    void __asm (*TFormToGlobal) (__a0 tform *tf);
    void (*Obsolete_6) (void);
    void (*Obsolete_7) (void);
};
#endif
#endif

/* die "normalen" Prototypen */
#ifdef AMIGA
void __asm te_SetViewer(__a0 tform *viewer);
tform *__asm te_GetViewer(void);
void __asm te_RefreshTForm(__a0 tform *tf);
void __asm te_TFormToGlobal(__a0 tform *tf);
#else
void te_SetViewer(tform *viewer);
tform *te_GetViewer(void);
void te_RefreshTForm(tform *tf);
void te_TFormToGlobal(tform *tf);
#endif

/*-------------------------------------------------------------------
**  Die Compatibilty-Macros für die TForm-Engine
*/
#ifdef DYNAMIC_LINKING
#define _SetViewer          TE_GET_SPEC->SetViewer
#define _GetViewer          TE_GET_SPEC->GetViewer
#define _RefreshTForm       TE_GET_SPEC->RefreshTForm
#define _TFormToGlobal      TE_GET_SPEC->TFormToGlobal
#endif  /* DYNAMIC_LINKING */

#ifdef STATIC_LINKING
#define _SetViewer          te_SetViewer
#define _GetViewer          te_GetViewer
#define _RefreshTForm       te_RefreshTForm
#define _TFormToGlobal      te_TFormToGlobal
#endif  /* STATIC_LINKING */

/*===================================================================
**  Specific-GET-Struktur der Input-Engine
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
struct ie_GET_Specific {
    void __asm (*GetInput) (__a0 struct VFMInput *input);
    void __asm (*EnableGUI) (void);
    void __asm (*DisableGUI) (void);
    void __asm (*AddClickBox) (__a0 struct ClickBox *cbox, __d0 ULONG flg);
    void __asm (*RemClickBox) (__a0 struct ClickBox *cbox);
    void __asm (*BeginRefresh) (void);
    void __asm (*EndRefresh) (void);
};
#endif
#endif

#ifdef AMIGA
void __asm ie_GetInput(__a0 struct VFMInput *input);
void __asm ie_EnableGUI(void);
void __asm ie_DisableGUI(void);
void __asm ie_AddClickBox(__a0 struct ClickBox *cbox, __d0 ULONG flg);
void __asm ie_RemClickBox(__a0 struct ClickBox *cbox);
void __asm ie_BeginRefresh(void);
void __asm ie_EndRefresh(void);
#else
void ie_GetInput(struct VFMInput *input);
void ie_EnableGUI(void);
void ie_DisableGUI(void);
void ie_AddClickBox(struct ClickBox *cbox, ULONG flg);
void ie_RemClickBox(struct ClickBox *cbox);
void ie_BeginRefresh(void);
void ie_EndRefresh(void);
#endif

/*-------------------------------------------------------------------
**  Die Compatibilty-Macros für die Input-Engine
*/
#ifdef DYNAMIC_LINKING
#define _GetInput       IE_GET_SPEC->GetInput
#define _EnableGUI      IE_GET_SPEC->EnableGUI
#define _DisableGUI     IE_GET_SPEC->DisableGUI
#define _AddClickBox    IE_GET_SPEC->AddClickBox
#define _RemClickBox    IE_GET_SPEC->RemClickBox
#define _BeginRefresh   IE_GET_SPEC->BeginRefresh
#define _EndRefresh     IE_GET_SPEC->EndRefresh
#endif

#ifdef STATIC_LINKING
#define _GetInput       ie_GetInput
#define _EnableGUI      ie_EnableGUI
#define _DisableGUI     ie_DisableGUI
#define _AddClickBox    ie_AddClickBox
#define _RemClickBox    ie_RemClickBox
#define _BeginRefresh   ie_BeginRefresh
#define _EndRefresh     ie_EndRefresh
#endif

/*=================================================================**
**  AUDIO ENGINE                                                   **
**=================================================================*/

#ifdef DYNAMIC_LINKING
#ifdef AMIGA
struct audio_GET_Specific {
    void __asm (*InitSoundCarrier) (__a0 struct SoundCarrier *);
    void __asm (*KillSoundCarrier) (__a0 struct SoundCarrier *);
    void __asm (*StartAudioFrame) (__d0 ULONG frame_time, __a0 struct flt_triple *, __a1 struct flt_triple *, __a2 struct flt_m3x3 *);
    struct flt_m3x3 *__asm (*EndAudioFrame) (void);
    void __asm (*StartSoundSource) (__a0 struct SoundCarrier *, __d0 ULONG);
    void __asm (*EndSoundSource) (__a0 struct SoundCarrier *, __d0 ULONG);
    void __asm (*RefreshSoundCarrier) (__a0 struct SoundCarrier *);
    void __asm (*FlushAudio) (void);
    BOOL __asm (*InitCDPlayer) (void);
    BOOL __asm (*ControlCDPlayer) (__a0 struct snd_cdcontrol_msg *);
    BOOL __asm (*SetCDVolume) ( __a0 struct snd_cdvolume_msg *);
    void __asm (*FreeCDPlayer) (void);
};
#endif
#endif

#ifdef AMIGA
void __asm audio_InitSoundCarrier(__a0 struct SoundCarrier *);
void __asm audio_KillSoundCarrier(__a0 struct SoundCarrier *);
void __asm audio_StartAudioFrame(__d0 ULONG frame_time, __a0 struct flt_triple *, __a1 struct flt_triple *, __a2 struct flt_m3x3 *);
struct flt_m3x3 *__asm audio_EndAudioFrame(void);
void __asm audio_StartSoundSource(__a0 struct SoundCarrier *, __d0 ULONG);
void __asm audio_EndSoundSource(__a0 struct SoundCarrier *, __d0 ULONG);
void __asm audio_RefreshSoundCarrier(__a0 struct SoundCarrier *);
void __asm audio_FlushAudio(void);
unsigned long __asm audio_InitCDPlayer(void);
unsigned long __asm audio_ControlCDPlayer(__a0 struct snd_cdcontrol_msg *);
unsigned long __asm audio_TriggerCDPlayer(__a0 struct snd_cdtrigger_msg *);
unsigned long __asm audio_SetCDVolume( __a0 struct snd_cdvolume_msg *);
void __asm audio_FreeCDPlayer(void);
#else
void audio_InitSoundCarrier(struct SoundCarrier *);
void audio_KillSoundCarrier(struct SoundCarrier *);
void audio_StartAudioFrame(ULONG frame_time, struct flt_triple *, struct flt_triple *, struct flt_m3x3 *);
struct flt_m3x3 *audio_EndAudioFrame(void);
void audio_StartSoundSource(struct SoundCarrier *, ULONG);
void audio_EndSoundSource(struct SoundCarrier *, ULONG);
void audio_RefreshSoundCarrier(struct SoundCarrier *);
void audio_FlushAudio(void);
unsigned long audio_InitCDPlayer(void);
unsigned long audio_ControlCDPlayer( struct snd_cdcontrol_msg *);
unsigned long audio_TriggerCDPlayer( struct snd_cdtrigger_msg *);
unsigned long audio_SetCDVolume( struct snd_cdvolume_msg *);
void audio_FreeCDPlayer(void);
#endif

#ifdef DYNAMIC_LINKING
#define _InitSoundCarrier       AE_GET_SPEC->InitSoundCarrier
#define _KillSoundCarrier       AE_GET_SPEC->KillSoundCarrier
#define _StartAudioFrame        AE_GET_SPEC->StartAudioFrame
#define _EndAudioFrame          AE_GET_SPEC->EndAudioFrame
#define _StartSoundSource       AE_GET_SPEC->StartSoundSource
#define _EndSoundSource         AE_GET_SPEC->EndSoundSource
#define _RefreshSoundCarrier    AE_GET_SPEC->RefreshSoundCarrier
#define _FlushAudio             AE_GET_SPEC->FlushAudio
#define _InitCDPlayer           AE_GET_SPEC->InitCDPlayer
#define _ControlCDPlayer        AE_GET_SPEC->ControlCDPlayer
#define _TriggerCDPlayer        AE_GET_SPEC->TriggerCDPlayer
#define _SetCDVolume            AE_GET_SPEC->SetCDVolume
#define _FreeCDPlayer           AE_GET_SPEC->FreeCDPlayer
#endif

#ifdef STATIC_LINKING
#define _InitSoundCarrier    audio_InitSoundCarrier
#define _KillSoundCarrier    audio_KillSoundCarrier
#define _StartAudioFrame     audio_StartAudioFrame
#define _EndAudioFrame       audio_EndAudioFrame
#define _StartSoundSource    audio_StartSoundSource
#define _EndSoundSource      audio_EndSoundSource
#define _RefreshSoundCarrier audio_RefreshSoundCarrier
#define _FlushAudio          audio_FlushAudio
#define _InitCDPlayer        audio_InitCDPlayer
#define _ControlCDPlayer     audio_ControlCDPlayer
#define _TriggerCDPlayer     audio_TriggerCDPlayer
#define _SetCDVolume         audio_SetCDVolume
#define _FreeCDPlayer        audio_FreeCDPlayer
#endif
/*-----------------------------------------------------------------*/
#endif

