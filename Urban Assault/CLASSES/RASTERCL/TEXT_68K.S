**
**  $Source: PRG:VFM/Classes/_RasterClass/text_68k.s,v $
**  $Revision: 38.1 $
**  $Date: 1996/06/05 01:42:36 $
**  $Locker:  $
**  $Author: floh $
**
**  m68k Font-Engine.
**
**  (C) Copyright 1994,1995,1996 by A.Weissflog
**
        MC68020
        CODE

        include 'exec/types.i'

; Importe aus rst_text.c
;-----------------------
        XREF _dt_Width          ;<UWORD>
        XREF _dt_Height         ;<UWORD>
        XREF _dt_XOffset        ;<UWORD>
        XREF _dt_YOffset        ;<UWORD>
        XREF _dt_LineTable      ;<UBYTE **>
        XREF _dt_Display        ;<UBYTE *>
        XREF _dt_Fonts          ;<struct VFMFont **>

;--------------------------------------------------------------------
;   Struktur-Definitionen aus <visualstuff/font.h>
;
        STRUCTURE FONTCHAR,0
        UWORD   FC_OFFSET
        UWORD   FC_WIDTH
        LABEL   FC_SIZE

        STRUCTURE VFMFONT,0
        APTR    VFNT_PAGE_MASTER        ; <Object *>
        APTR    VFNT_PAGE_BMP           ; <struct VFMBitmap *>
        APTR    VFNT_PAGE               ; <UBYTE *> !!! (kein VFMBitmap *)
        APTR    VFNT_FCHARS             ; <struct FontChar *>
        UWORD   VFNT_HEIGHT
        LABEL   VFNT_SIZE

        STRUCTURE RAST_TEXT,0
        APTR    RT_STRING               ; <UBYTE *>
        APTR    RT_CLIPS                ; <UBYTE **>
        LABEL   RT_SIZE

        STRUCTURE VFMBITMAP,0
        APTR    VBMP_DATA
        UWORD   VBMP_WIDTH
        UWORD   VBMP_HEIGHT
        LABEL   VBMP_END                ; der Rest ist hier irrelevant...

;--------------------------------------------------------------------
;   Lokale Variablen, pc-relativ adressierbar.
;
xpos    dc.w 0              ;aktuelle X-Display-Position
ypos    dc.w 0              ;aktuelle Y-Display-Position
linex   dc.w 0              ;Display-Start-X aktuelle Zeile
liney   dc.w 0              ;Display-Start-Y aktuelle Zeile
line_ignore dc.w 0
line_height dc.w 0
page_width  dc.w 0          ;Breite der Font-Page

jump_table:
        dc.l sel_00
        dc.l sel_01
        dc.l sel_02
        dc.l sel_03
        dc.l sel_04
        dc.l sel_05
        dc.l sel_06
        dc.l sel_07
        dc.l sel_08
        dc.l sel_09
        dc.l sel_0a
        dc.l sel_0b
        dc.l sel_0c
        dc.l sel_0d
        dc.l sel_0e
        dc.l sel_0f
        dc.l sel_10
        dc.l sel_11

;--------------------------------------------------------------------
;   MACRO: GetPixelAddress xpos,ypos,result
;
;   Ermittelt mit Hilfe der ove_LineTable die per <xpos,ypos>
;   definierte Display-Adresse.
;   xpos und ypos werden zerstört!
;
GetPixelAddress MACRO
        mulu   _dt_Width,\2
        move.l _dt_Display,\3
        add.l  \1,\2
        add.l \2,\3
        ENDM

;--------------------------------------------------------------------
;   MACRO:  ResetLineParams
;
;   line_ignore = 0
;   line_height = (VFNT_HEIGHT,a2)
;
ResetLineParams MACRO
        clr.w line_ignore
        move.w (VFNT_HEIGHT,a2),line_height
        ENDM

**-----------------------------------------------------------------**
**  NAME
**      rst_DrawText -- Text-Engine für raster.class
**
**  SYNOPSIS
**      void __asm ove_DrawText(__a0 struct rast_text *args)
**
**  INPUTS
**      args->string    -> UBYTE * char stream
**      args->clips     -> Array von UBYTE *'s auf Clip-Substrings
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      03-Aug-95   floh    created
**      04-Aug-95   floh    debugging...
**      05-Aug-95   floh    jetzt mit beliebig breiten Font-Page-Bitmaps
**      08-Aug-95   floh    debugging...
**      30-Oct-95   floh    Fontpage durfte nur max. 32KB groß
**                          sein, jetzt 64KB
**      31-Oct-95   floh    neuer Selektor: 0x10
**      07-Nov-95   floh    Bugfix in sel_0e
**      21-Mar-96   floh    XPosRel und YPosRel resetten nicht mehr
**                          alle Lineparameter (so daß Ignore-Stuff
**                          erhalten bleibt).
**      04-Jun-96   floh    übernommen in raster.class
**
        XDEF _rst_DrawText
_rst_DrawText:
        movem.l d2-d7/a2-a6,-(a7)
        move.l (RT_CLIPS,a0),a5
        move.l (RT_STRING,a0),a0
        moveq #0,d0                     ;d0 32 bit clean machen
        move.l d0,-(a7)                 ; 0 auf Stack als Gosubstring-Terminator

read    moveq #0,d7
        move.b (a0)+,d7                 ;lese nächstes Stream-Byte
        beq.s do_selektor               ;Selektor???

        ;a0 -> zeigt auf Byte-Stream
        ;a2 -> zeigt auf aktuelle VFMFont-Struktur
        ;a3 -> zeigt direkt in Display-Buffer
        ;a4 -> zeigt direkt in Font-Page
        ;a5 -> zeigt auf Clips-Array
        ;d1.b -> Char-Width  (char-spezifisch)
        ;d2.b -> Char-Height (konstant für gesamten Font)
        ;d6.w -> Display Adder neue Pixel-Line
        ;d7.w -> Font-Page-Adder neue Pixel-Line

        lea ([VFNT_FCHARS,a2],d7.w*4),a6    ;Ptr auf aktuelle <struct FontChar>
        move.w (FC_WIDTH,a6),d1             ;Breite aktuelles Char
        add.w d1,xpos                       ;<xpos> updaten

sel_return_1
        moveq #0,d0
        move.w (FC_OFFSET,a6),d0        ;Offset in Font-Page
        move.l (VFNT_PAGE,a2),a4
        add.w line_ignore,d0            ;Ignore-Zeilen mit beachten
        add.l d0,a4                     ;a4 zeigt direkt auf akt. Char

sel_return_2
        move.w _dt_Width,d7
        sub.w d1,d7                 ;d7 ist Display-Adder neue Pix-Line
        move.w page_width,d6
        sub.w d1,d6                 ;d6 ist FontPage-Offset-Adder neue Pix-Line

        move.l a3,-(a7)                 ;aktuellen Display-Ptr pushen

        move.w line_height,d2           ;d2 ist y-Count
        moveq #(0-1),d3                 ;d3 ist -1
        subq.w #1,d2
ycycl   add.w d1,d3                     ;d3 ist dbf-x-Count
xcycl   move.b (a4)+,(a3)+              ;aktuelles Font-Pixel nach Display++
        dbf d3,xcycl
        add.w d7,a3                     ;neue Display-Position
        add.w d6,a4                     ;neue Font-Page-Position
        dbf d2,ycycl

        move.l (a7)+,a3             ;Display-Ptr char top-left zurück
        add.w d1,a3                 ;Start-Display-Ptr nächstes char
        bra.s read                  ;...und neues Char holen!

;--------------------------------------------------------------------
do_selektor:
        move.b (a0)+,d7             ;hole Selektor-Byte
        lea jump_table,a6
        jmp ([a6,d7.w*4])           ;Selektor direkt bespringen
;--------------------------------------------------------------------
;   Einlauf der Selektoren!
;--------------------------------------------------------------------
sel_00:             ;END OF STRING
        move.l (a7)+,d7             ;aktuelle "Return-Position"
        beq.s .end                  ; == 0 -> endültiges Ende!
        move.l d7,a0                ;sonst -> an alter Position weitermachen
        bra read
.end    movem.l (a7)+,d2-d7/a2-a6   ;zurück...
        rts
;--------------------------------------------------------------------
sel_01:             ;setze X-Pos absolut
        move.w (a0)+,d6
        add.w _dt_XOffset,d6
        move.w d6,xpos
        move.w d6,linex
        move.w ypos,d7
        move.w d7,liney
        GetPixelAddress d6,d7,a3
        ResetLineParams
        bra read
;--------------------------------------------------------------------
sel_02:             ;setze Y-Pos absolut
        move.w (a0)+,d7
        add.w _dt_YOffset,d7
        move.w d7,ypos
        move.w d7,liney
        move.w xpos,d6
        move.w d6,linex
        GetPixelAddress d6,d7,a3
        ResetLineParams
        bra read
;--------------------------------------------------------------------
sel_03:             ;setze X-Pos Rand-relativ
        move.w (a0)+,d6
        bge.s .left             ;war links-relativ
        add.w _dt_Width,d6      ;war rechts-relativ
.left   move.w d6,xpos
        move.w d6,linex
        move.w ypos,d7
        move.w d7,liney
        GetPixelAddress d6,d7,a3
        ResetLineParams
        bra read
;--------------------------------------------------------------------
sel_04              ;setze Y-Pos Rand-relativ
        move.w (a0)+,d7
        bge.s .top
        add.w _dt_Height,d7     ;war lower-relativ
.top    move.w d7,ypos
        move.w d7,liney
        move.w xpos,d6
        move.w d6,linex
        GetPixelAddress d6,d7,a3
        ResetLineParams
        bra read
;--------------------------------------------------------------------
sel_05              ;setze X-Pos relativ zu aktueller Position
        move.w (a0)+,d6
        add.w d6,xpos
        add.w d6,a3
        bra read
;--------------------------------------------------------------------
sel_06              ;setze Y-Pos relativ zu aktueller Position
        move.w (a0)+,d7
        add.w d7,ypos
        move.w _dt_Width,d6
        muls d7,d6
        add.l d6,a3
        bra read
;--------------------------------------------------------------------
sel_07              ;New Line
        move.w linex,d6
        move.w liney,d7
        add.w line_height,d7
        move.w d7,liney
        move.w d6,xpos
        move.w d7,ypos
        GetPixelAddress d6,d7,a3
        ResetLineParams
        bra read
;--------------------------------------------------------------------
sel_08              ;Neuer Font
        move.b (a0)+,d7         ;Font-ID
        move.l _dt_Fonts,a2
        move.l (a2,d7.w*4),a2       ;Ptr auf jetzt gültige VFMFont-Struktur
        move.w ([VFNT_PAGE_BMP,a2],VBMP_WIDTH),page_width
        bra read

;--------------------------------------------------------------------
sel_09              ;Insert Clip
        move.b (a0)+,d7         ;hole Clip-ID
        move.l a0,-(a7)         ;sichere Return-Position
        move.l (a5,d7.w*4),a0   ;Sub-Clip als neuer Byte-Stream
        bra read

;--------------------------------------------------------------------
sel_0a              ;Stretch Char
        moveq #0,d1
        move.b (a0)+,d1         ;forced Breite des nächsten Buchstaben

;*** ab hier Spezial-Version der Canonical-Read-Schleife! ***
sel_0a_jumpin
        add.w d1,xpos           ;<xpos> updaten
        move.b (a0)+,d7         ;nächster Buchstabe
        beq do_selektor         ;darf eigentlich nicht vorkommen!

        lea ([VFNT_FCHARS,a2],d7.w*4),a6    ;Ptr auf aktuelle <struct FontChar>
                                            ;Breite des Chars ignorieren!

        moveq #0,d0
        move.w (FC_OFFSET,a6),d0        ;Offset in Font-Page
        move.l (VFNT_PAGE,a2),a4
        add.w line_ignore,d0            ;Ignore-Zeilen beachten
        add.l d0,a4                     ;a4 zeigt direkt auf akt. Char

        move.w _dt_Width,d7
        sub.w d1,d7                     ;d7 ist Display-Adder neue Pix-Line
        move.w page_width,d6            ;kein(!) X-Offset-Modifikator

        move.l a3,-(a7)                 ;aktuellen Display-Ptr pushen

        move.w line_height,d2           ;d2 ist y-Count
        moveq #(0-1),d3                 ;d3 ist -1
        subq.w #1,d2
ycycl_1 add.w d1,d3                     ;d3 ist dbf-x-Count
        move.b (a4),d0                  ;statisches Font-Pixel holen
xcycl_1 move.b d0,(a3)+                 ;statisches Font-Pixel nach Display++
        dbf d3,xcycl_1
        add.w d7,a3                     ;neue Display-Position
        add.w d6,a4                     ;neue Font-Page-Position
        dbf d2,ycycl_1

        move.l (a7)+,a3             ;Display-Ptr char top-left zurück
        add.w d1,a3                 ;Start-Display-Ptr nächstes char
        bra read                    ;...und neues Char holen!

;--------------------------------------------------------------------
sel_0b:             ;Stretch To
        moveq #0,d1
        move.b (a0)+,d1         ;Endpunkt relativ zu Zeilen-Anfang
        move.w xpos,d6          ;aktuelle X-Position
        sub.w linex,d6          ;soviele Pixel ab Zeilenanfang gezeichnet
        sub.w d6,d1             ;in d1 jetzt Ziel-Breite
        bra.s sel_0a_jumpin     ;weiter wie in <sel_0a>
;--------------------------------------------------------------------
sel_0c:             ;Ignore Hori
        moveq #0,d6
        move.b (a0)+,d6         ;soviele Pixel vom nächsten Buchstaben ign.
        move.b (a0)+,d7         ;nächster Buchstabe
        beq do_selektor         ;sollte eigentlich nicht vorkommen

        lea ([VFNT_FCHARS,a2],d7.w*4),a6
        move.w (FC_WIDTH,a6),d1     ;Gesamt-Breite...
        sub.w d6,d1                 ;neue Breite...
        add.w d1,xpos               ;<xpos> updaten

        moveq #0,d0
        move.w (FC_OFFSET,a6),d0    ;Offset in Font-Page
        add.w d6,d0                 ;<nn> Pixel überspringen
        move.l (VFNT_PAGE,a2),a4
        add.w line_ignore,d0        ;Line-Ignore-Wert adden
        add.l d0,a4
        bra sel_return_2            ;...und nach Hauptschleife verzweigen...
;--------------------------------------------------------------------
sel_0d:             ;Cutoff Hori
        moveq #0,d6
        move.b (a0)+,d1         ;nächsten Buchstaben nur so breit
        move.b (a0)+,d7         ;nächster Buchstabe

        lea ([VFNT_FCHARS,a2],d7.w*4),a6
        add.w d1,xpos           ;<xpos> updaten
        bra sel_return_1        ;...und nach Hauptschleife verzweigen...
;--------------------------------------------------------------------
sel_0e:             ;Ignore Vert
        move.b (a0)+,d7         ;soviele Pixel-Zeilen ignorieren...
        move.w (VFNT_HEIGHT,a2),d6
        sub.w d7,d6             ;neue Gesamt-Höhe dieser Zeile
        move.w d6,line_height
        muls page_width,d7      ;soviele Byte in Fontpage ignorieren
        move.w d7,line_ignore
        bra read                ;und weiter mit nächstem Char
;--------------------------------------------------------------------
sel_0f:             ;Cutoff Vert
        move.b (a0)+,d7         ;soviele Pixel-Zeilen zeichnen
        clr.w line_ignore       ;<line_ignore> auf "von Anfang an"
        move.w d7,line_height   ;und <line_height> auf neue Höhe
        bra read                ;und weiter...
;--------------------------------------------------------------------
sel_10:             ;New Font & Flush
        move.b (a0)+,d7         ;Font-ID
        move.l _dt_Fonts,a2
        move.l (a2,d7.w*4),a2       ;Ptr auf jetzt gültige VFMFont-Struktur
        move.w ([VFNT_PAGE_BMP,a2],VBMP_WIDTH),page_width
        ResetLineParams
        bra read
;--------------------------------------------------------------------
sel_11:             ;Stretch To Long
        moveq #0,d1
        move.w (a0)+,d1         ;Endpunkt relativ zu Zeilen-Anfang
        move.w xpos,d6          ;aktuelle X-Position
        sub.w linex,d6          ;soviele Pixel ab Zeilenanfang gezeichnet
        sub.w d6,d1             ;in d1 jetzt Ziel-Breite
        bra sel_0a_jumpin       ;weiter wie in <sel_0a>

;====================================================================
        END

