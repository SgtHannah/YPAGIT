**
**  $Source: PRG:VFM/Classes/_RasterClass/span_68k.s,v $
**  $Revision: 38.2 $
**  $Date: 1996/06/05 01:41:48 $
**  $Locker:  $
**  $Author: floh $
**
**  8bit Chunky Spandrawer für m68k.
**
**  (C) Copyright 1996 by A.Weissflog
**
        MC68020
        CODE

        include 'exec/types.i'

        XREF _rst_GenClusterStack

;--------------------------------------------------------------------
        STRUCTURE RAST_SCANLINE,0
        APTR  RS_DRAWSPAN
        WORD  RS_FLAGS
        WORD  RS_Y
        WORD  RS_X0
        WORD  RS_DX
        ULONG RS_B0
        ULONG RS_Z0
        ULONG RS_U0
        ULONG RS_V0
        LONG  RS_DB
        LONG  RS_DZ
        LONG  RS_DU
        LONG  RS_DV
        APTR  RS_MAP

        LABEL RS_SIZE
;--------------------------------------------------------------------
        STRUCTURE CLUSTER,0

        LONG CL_COUNT
        LONG CL_M0
        LONG CL_DM

        LABEL CLUSTER_SIZE
;--------------------------------------------------------------------
        STRUCTURE RASTER_DATA,0

        APTR RD_R           ; <struct VFMBitmap *>
        ULONG RD_FGPEN
        ULONG RD_BGPEN

        APTR RD_SHADEBMP    ; <struct VFMBitmap *>
        APTR RD_SHADEBODY   ; <UBYTE *>

        APTR RD_TRACYBMP    ; <struct VFMBitmap *>
        APTR RD_TRACYBODY   ; <UBYTE *>

        APTR RD_CLUSTERSTACK
        STRUCT RD_CLIP,16
        STRUCT RD_DRAWSPANLUT,256

        LABEL RD_SIZE

;--------------------------------------------------------------------

_Prolog macro
        movem.l d2-d7/a2-a3,-(a7)
        endm

_Epilog macro
        movem.l (a7)+,d2-d7/a2-a3
        rts
        endm

;
;   _GetDisplay
;
;   In:
;       a0 -> Pointer auf Zeilen-Anfang
;       a1 -> <struct rast_scanline *>
;
;   Out:
;       a0   -> Pointer ins Display
;       d0.l -> Pixel-Counter
;
_GetDisplay macro
        add.w (RS_X0,a1),a0
        moveq #0,d0
        move.w (RS_DX,a1),d0
        endm

;
;   _ShadeInit
;
;   In:
;       a1 -> <struct rast_scanline *>
;
;   Out:
;       d1.w -> shade_act [8.8]
;       d2.w -> shade_add [8.8]
;
_ShadeInit  macro
        move.l (RS_B0,a1),d1
        move.l (RS_DB,a1),d2
        endm

;
;   _MapInit (NUR LINEARES MAPPING!)
;
;   In:
;       a1  -> <struct rast_scanline *>
;
;   Out:
;       d3.l -> uv  [u0|v0]
;       d4.l -> duv [du|dv]
;       a3   -> Pointer auf rohe Txt-Data
;
_MapInit    macro
        move.l (RS_U0,a1),d3
        swap d3
        move.w (RS_V0+2,a1),d3

        move.l (RS_DU,a1),d4
        swap d4
        move.w (RS_DV+2,a1),d4
        move.l (RS_MAP,a1),a3
        endm

**-----------------------------------------------------------------**
**  MACRO
**      ZeroSpan_macro (für DrawSpan_NNN auf Farbe 0)
**
**  PARMS
**      a0 - Pointer auf Span-Start
**      a1 - <struct rast_scanline *>
**      a2 - <struct raster_data *>
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      01-Jun-96   floh    created
**
ZeroSpan_macro  macro
        _Prolog
        _GetDisplay     ; a0 = Display, d0 = count

        moveq.l #0,d1
        ror.l #2,d0     ; d0.w ist Anzahl LONGs
        subq.w #1,d0    ; weil dbf-Counter
        blt.s .dorest\@ ; weniger als 4 Bytes
.cycl1\@
        move.l d1,(a0)+ ; 4 Pixel schreiben
        dbf d0,.cycl1\@
.dorest\@
        clr.w d0        ; war ja 0xffff
        rol.l #2,d0     ; Anzahl Restbytes als Zähler
        subq.w #1,d0    ; weil dbf-Counter
        blt.s .end\@    ; keine Restbytes da
.cycl2\@
        move.b d1,(a0)+ ; 1 Pixel schreiben
        dbf d0,.cycl2\@
.end\@
        _Epilog
        ENDM

**-----------------------------------------------------------------**
**  MACRO
**      TracyTxtSpan_macro (für DrawSpan_LNF)
**
**  PARMS
**      a0 - Pointer auf Span-Start
**      a1 - <struct rast_scanline *>
**      a2 - <struct raster_data *>
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      01-Jun-96   floh    created
**
TracyTxtSpan_macro macro
        _Prolog
        _GetDisplay     ;a0 = Display, d0 = counter
        _MapInit        ;d3.l = map_act, d4.l = map_add, a3 = Txt-Pntr
        subq.w #1,d0    ;dbf Counter
        moveq #0,d6     ;32 Bit clean!
        moveq #0,d1     ;32 Bit clean!
        move.l (RD_TRACYBODY,a2),a2 ; a2 = Lookup-Table
.cycl\@
        move.w d3,d6            ; Part 1 Map-Offset ermitteln
        move.b (a0),d1          ; Background-Pixel aus Display
        rol.l #8,d3             ; Part 2 Map-Offset ermitteln
        lsl.w #8,d1             ; Background-Pix als Remap-Spalte
        move.b d3,d6            ; Part 3 Map-Offset ermitteln
        ror.l #8,d3             ; Part 4 Map-Offset ermitteln

        move.b (a3,d6.l),d1     ; d1.l ist Offset im TracyRemap-Tabelle
        add.l d4,d3             ; map_act += map_add
        move.b (a2,d1.l),(a0)+  ; *display++ = resultierendes Pixel
        dbf d0,.cycl\@          ; und nächstes Pixel
        _Epilog
        ENDM

**-----------------------------------------------------------------**
**  MACRO
**      TxtSpan_macro map,shd,trc
**
**  PARMS
**      map = L
**      shd = N
**      trc = [N or C]
**
**      a0 - Pointer auf Span-Start
**      a1 - <struct rast_scanline *>
**      a2 - <struct raster_data *>
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      01-Jun-96   floh    created
**
TxtSpan_macro   macro
        _Prolog
        _GetDisplay     ;a0 = Display, d0 = counter
        _MapInit        ;d3.l = map_act, d4.l = map_add, a3 = Txt-Pntr
        subq.w #1,d0    ;dbf Counter
        moveq #0,d6     ;32 Bit clean!
.cycl\@
        move.w d3,d6            ; ermittle Map-Offset
        rol.l #8,d3
        move.b d3,d6
        ror.l #8,d3

        IFC '\3','N'                ;-> no tracy
            move.b (a3,d6.l),(a0)+  ;Texel -> Display++
        ELSE                        ;-> Clear Tracy
            move.b (a3,d6.l),d6     ;hole Texel
            beq.s .no_write\@       ;== 0, also ignorieren
            move.b d6,(a0)          ;Texel nach Display
.no_write\@ addq.l #1,a0            ;display++
        ENDC
        add.l d4,d3                 ;map_act += map_add
        dbf d0,.cycl\@              ;und nächstes Pixel...

.ignore_span\@
        _Epilog
        ENDM

**-----------------------------------------------------------------**
**  MACRO
**      TxtShadeSpan_macro map,shd,trc
**
**  INPUTS
**      map = L
**      shd = G
**      trc = [N or C]
**
**      a0 - Pointer auf Span-Start
**      a1 - <struct rast_scanline *>
**      a2 - <struct raster_data *>
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      01-Jun-96   floh    created
**      02-Jun-96   floh    der Merge-Trick mußte leider wieder
**                          removed werden...
**
TxtShadeSpan_macro MACRO
        _Prolog
        _GetDisplay     ;a0 = Display, d0 = counter
        _ShadeInit      ;d1.w = shade_act, d2.w = shade_add
        _MapInit        ;d3.l = map_act, d4.l = map_add, a3=Txt-Ptr

        subq.w #1,d0    ; weil dbf-Counter
        move.l (RD_SHADEBODY,a2),a2     ; a2 = Lookup-Table
        moveq #0,d5     ;32 bit clean!
        moveq #0,d6     ;32 bit clean!
.cycl\@
        move.w d3,d6            ; ermittle Map-Offset
        rol.l #8,d3
        move.b d3,d6
        ror.l #8,d3

        move.w d1,d5            ; shade_act -> d1

        move.b (a3,d6.l),d5     ; d5.w -> Offset in Shading-Tabelle
        IFC '\3','C'    ;-> Clear Tracy
            beq .no_write\@
        ENDC

        add.l d4,d3         ;map_act += map_add

        IFC '\3','C'    ;-> Clear Tracy
            move.b (a2,d5.l),(a0)  ;*Display = Pixel
.no_write\@ addq.l #1,a0    ;Display++
        ELSE
            move.b (a2,d5.l),(a0)+ ;*Display++ = Pixel
        ENDC

        add.w d2,d1         ;shade_act += shade_add
        dbf d0,.cycl\@

.ignore_span\@
        _Epilog
        ENDM

**-----------------------------------------------------------------**
**  MACRO
**      DepthSpan_macro map,shd,trc
**
**  INPUTS
**      map = Z
**      shd = N
**      trc = [N or C]
**
**      a0 - Pointer auf Span-Start
**      a1 - <struct rast_scanline *>
**      a2 - <struct raster_data *>
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      01-Jun-96   floh    created
**
DepthSpan_macro macro
        _Prolog
        movem.l a0-a1,-(a7)
        move.l (RD_CLUSTERSTACK,a2),a0
        bsr _rst_GenClusterStack
        movem.l (a7)+,a0-a1
        _GetDisplay                 ;a0 = Display, d0 = counter

        move.l (RS_MAP,a1),a3          ; a3 = Ptr auf Txt-Date
        move.l (RD_CLUSTERSTACK,a2),a1 ; a1 = Ptr auf Clusterstack
        moveq #0,d6                 ;muß 32 Bit clean sein!
        bra.s .new_cluster\@

.cycl\@
        move.w d1,d6            ;Map-Offset in d6 ermitteln
        rol.l #8,d1
        move.b d1,d6            ;Offset in d6 fertig
        ror.l #8,d1             ;map_act wieder ok

        IFC '\3','0'    ;-> No Tracy
            move.b (a3,d6.l),(a0)+  ;Texel -> Display++
        ELSE            ;-> Clear Tracy
            move.b (a3,d6.l),d6     ;hole Texel
            beq.s .no_write\@       ;== 0, also ignorieren
            move.b d6,(a0)      ;Texel nach Display
.no_write\@ addq.l #1,a0        ;Display++
        ENDC

        add.l d2,d1         ;map_act += map_add
        dbf d0,.cycl\@      ;und nächstes Pixel...

.new_cluster\@              ;neuen Cluster "anschneiden"
        movem.l (a1)+,d0-d2     ;d0=count, d1=map_act, d2=map_add
        dbf d0,.cycl\@          ;weiter, wenn Cluster-Stack nicht am Ende

.ignore_span\@
        _Epilog
        ENDM

**-----------------------------------------------------------------**
**  MACRO
**      DepthShadeSpan_macro map,shd,trc
**
**  INPUTS
**      map = Z
**      shd = G
**      trc = [N or C]
**
**      a0 - Pointer auf Span-Start
**      a1 - <struct rast_scanline *>
**      a2 - <struct raster_data *>
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      01-Jun-96   floh    created
**
DepthShadeSpan_macro    MACRO
        _Prolog
        movem.l a0-a1,-(a7)
        move.l (RD_CLUSTERSTACK,a2),a0
        bsr _rst_GenClusterStack
        movem.l (a7)+,a0-a1
        _GetDisplay             ;a0 = Display, d0 = counter
        _ShadeInit              ;d1.w = shade_act, d2.w = shade_add

        move.l (RS_MAP,a1),a3          ; a3 = Ptr auf Txt-Data
        move.l (RD_CLUSTERSTACK,a2),a1 ; a1 = Ptr auf Clusterstack
        move.l (RD_SHADEBODY,a2),a2    ; a2 = Ptr auf Shade-LUT

        moveq #0,d6                 ;muß 32 Bit clean sein!
        moveq #0,d0                 ;das auch...
        bra.s .new_cluster\@

.cycl\@
        move.w d4,d6            ;Map-Offset in d6 ermitteln
        rol.l #8,d4
        move.b d4,d6            ;Offset in d6 fertig
        ror.l #8,d4             ;map_act wieder ok

        move.w d1,d0            ; shade_act -> d0

        move.b (a3,d6.l),d0     ; d0.w -> Offset auf Shading-Tabelle
        IFC '\3','C'    ;-> Clear Tracy
            beq .no_write\@
        ENDC

        add.l d5,d4             ;map_act   += map_add

        IFC '\3','C'
            move.b (a2,d0.l),(a0)   ;*display = resultierendes Pixel
.no_write\@ addq.l #1,a0        ;Display++
        ELSE
            move.b (a2,d0.l),(a0)+  ;*display = resultierendes Pixel
        ENDC

        add.w d2,d1             ;shade_act += shade_add
        dbf d3,.cycl\@

.new_cluster\@               ;neuen Cluster "anschneiden"
        movem.l (a1)+,d3-d5  ;d3=count, d4=map_act, d5=map_add
        dbf d3,.cycl\@       ;weiter, wenn Cluster-Stack nicht am Ende
.ignore_span\@
        _Epilog
        ENDM

**--------------------------------------------------------------------
**   void span_xxx(__a0 UBYTE *start,
**                 __a1 struct rast_scanline *rs,
**                 __a2 struct raster_data *rd)
**
**  FUNCTION
**      Die einzelnen Spandrawer in 68k Version.
**
**  INPUTS
**      a0  - Pointer auf Span-Start in der VFMBitmap
**      a1  - Pointer auf Scanline-Data
**      a2  - Pointer auf LID des raster.class Objects
**
**  CHANGED
**      01-Jun-96   floh    created
**
        XDEF _span_nnn
        XDEF _span_lnf
        XDEF _span_lnn
        XDEF _span_lnc
        XDEF _span_lgn
        XDEF _span_lgc
        XDEF _span_znn
        XDEF _span_znc
        XDEF _span_zgn
        XDEF _span_zgc

*** flat, Farbe Zero
_span_nnn ZeroSpan_macro

*** linear mapped, no shade, Remap-Tracy
_span_lnf TracyTxtSpan_macro

*** linear mapped, no shade, no tracy
_span_lnn TxtSpan_macro L,N,N

*** linear mapped, no shade, clear tracy
_span_lnc TxtSpan_macro L,N,C

*** linear mapped, gshade, no tracy
_span_lgn TxtShadeSpan_macro L,G,N

*** linear mapped, gshade, clear tracy
_span_lgc TxtShadeSpan_macro L,G,C

*** depth mapped, no shade, no tracy
_span_znn DepthSpan_macro Z,N,N

*** depth mapped, no shade, clear tracy
_span_znc DepthSpan_macro Z,N,C

*** depth mapped, gshade, no tracy
_span_zgn DepthShadeSpan_macro Z,G,N

*** depth mapped, gshade, clear tracy
_span_zgc DepthShadeSpan_macro Z,G,C

;--------------------------------------------------------------------
        END


