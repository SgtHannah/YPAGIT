;
;  $Source: PRG:VFM/Classes/_RasterClass/text_x86.asm,v $
;  $Revision: 38.3 $
;  $Date: 1998/01/06 14:59:06 $
;  $Locker:  $
;  $Author: floh $
;
;   x86 Font-Engine.
;
;  (C) Copyright 1995,1996 by A.Weissflog
;
ifdef MASM
.386
DGROUP=_DATA
rst_DrawText_ EQU _rst_DrawText
ENDIF

            assume CS:_TEXT
            assume DS:DGROUP

; Importe aus rst_text.c
;-----------------------
        extrn _dt_BytesPerRow:NEAR          ;<ULONG>
        extrn _dt_Width:NEAR                ;<UWORD>
        extrn _dt_Height:NEAR               ;<UWORD>
        extrn _dt_XOffset:NEAR              ;<UWORD>
        extrn _dt_YOffset:NEAR              ;<UWORD>
        extrn _dt_Display:NEAR              ;<UBYTE *>
        extrn _dt_TracyRemapData:NEAR       ;<UBYTE *>
        extrn _dt_Fonts:NEAR                ;<struct VFMFont *>

; Struktur-Definitionen
;-----------------------

FONTCHAR    struc
        FC_OFFSET   DW ?
        FC_WIDTH    DW ?
FONTCHAR    ends

VFMFONT     struc
        VFNT_PAGE_MASTER    DD ?
        VFNT_PAGE_BMP       DD ?
        VFNT_PAGE           DD ?
        VFNT_FCHARS         DD ?
        VFNT_HEIGHT         DW ?
VFMFONT     ends

RAST_TEXT   struc
        RT_STRING           DD ?
        RT_CLIPS            DD ?
RAST_TEXT   ends

VFMBITMAP   struc
        VBMP_DATA           DD ?
        VBMP_WIDTH          DW ?
        VBMP_HEIGHT         DW ?
VFMBITMAP   ends        ;Rest ist hier nicht interessant...

;--------------------------------------------------------------------
;   Lokaler Variablenbereich
;
LOCAL_DATA  struc
        LD_XPOS             DD 0
        LD_YPOS             DD 0

        LD_LINEX            DD 0
        LD_LINEY            DD 0

        LD_PAGE_WIDTH       DD 0
        LD_FONT             DD 0
        LD_CLIPS            DD 0

        LD_LINE_IGNORE      DD 0
        LD_LINE_HEIGHT      DB 0
LOCAL_DATA  ends

;--------------------------------------------------------------------
_DATA       segment byte public 'DATA'

prms    LOCAL_DATA <>

jump_table:
        DD  sel_00
        DD  sel_01
        DD  sel_02
        DD  sel_03
        DD  sel_04
        DD  sel_05
        DD  sel_06
        DD  sel_07
        DD  sel_08
        DD  sel_09
        DD  sel_0a
        DD  sel_0b
        DD  sel_0c
        DD  sel_0d
        DD  sel_0e
        DD  sel_0f
        DD  sel_10
        DD  sel_11

;--- DATA SECTION ENDS HERE ------------------------------------------
_DATA   ends
;---------------------------------------------------------------------
_TEXT       segment byte public 'CODE'

;=====================================================================
;   SUPPORT MACROS
;=====================================================================

GetPixelAddress MACRO
;   in:  eax -> xpos
;   in:  ebx -> ypos
;   out: edi -> result

        mov edi,_dt_Display
        push eax
        push edx
        xor eax,eax
        mov ax,WORD PTR _dt_BytesPerRow
        xor edx,edx
        mul ebx
        pop edx
        add edi,eax
        pop eax
        add edi,eax
        ENDM

ResetLineParams MACRO
        mov prms.LD_LINE_IGNORE,0
        mov ebx,prms.LD_FONT
        mov bx,[ebx+VFNT_HEIGHT]
        mov prms.LD_LINE_HEIGHT,bl
        ENDM

GetX    MACRO
        xor eax,eax
        mov ah,[ebp]
        inc ebp
        mov al,[ebp]
        inc ebp
        ENDM

GetY    MACRO
        xor ebx,ebx
        mov bh,[ebp]
        inc ebp
        mov bl,[ebp]
        inc ebp
        ENDM

ModifyX MACRO
        mov prms.LD_XPOS,eax
        mov prms.LD_LINEX,eax
        mov ebx,prms.LD_YPOS
        mov prms.LD_LINEY,ebx
        ENDM

ModifyY MACRO
        mov prms.LD_YPOS,ebx
        mov prms.LD_LINEY,ebx
        mov eax,prms.LD_XPOS
        mov prms.LD_LINEX,eax
        ENDM

;-- ---------------------------------------------------------------**
;   NAME
;       rst_DrawText -- Text-Engine für raster.class
;
;   SYNOPSIS
;       void rst_DrawText([eax] struct rast_test *args)
;
;   INPUTS
;       args->string        -> UBYTE * char stream
;       args->clips         -> Array von UBYTE *'s auf Clip-Substrings
;
;   TRASHES
;       eax
;
;   CHANGED
;       05-Aug-95   floh    created aus m68k-Version
;       07-Aug-95   floh    debugging
;       31-Oct-95   floh    neuer Selektor 0x10: New Font & Flush
;       07-Nov-95   floh    debugging in sel_0c
;       14-Dec-95   floh    Transparenz per Remap-Tabellen
;       21-Mar-96   floh    XPosRel und YPosRel resetten nicht
;                           mehr die Line-Parameter, damit bleiben
;                           Ignore-Sachen etc. weiter gültig
;       05-Jun-96   floh    übernommen nach raster.class, mit den
;                           notwendigen Änderungen.
;       15-Aug-97   floh    + Transparenz rausgeflogen
;
        public rst_DrawText_
rst_DrawText_   proc near

        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp

        mov ebx,[eax+RT_CLIPS]
        mov prms.LD_CLIPS,ebx       ;Clips-Ptr nach LD_CLIPS

        mov ebp,[eax+RT_STRING]     ;Ptr auf Input-Stream nach ebp

        xor eax,eax
        push eax                    ; NULL-Ptr auf Stack

read:   xor eax,eax                 ;eax 32 bit clean machen
        mov al,[ebp]                ;aktuelles Stream-Byte holen
        inc ebp
        cmp al,0                    ;Control-Sequenz?
        je do_selektor

        mov esi,prms.LD_FONT        ;aktuell gültige Font-Struktur
        mov esi,[esi+VFNT_FCHARS]   ;Anfang des Char-Arrays
        lea esi,[esi+eax*4]         ;esi jetzt auf aktuelles Char

        mov ax,[esi+FC_WIDTH]       ;ax = char width
        add prms.LD_XPOS,eax        ;<xpos> updaten

sel_return_1:
        xor ecx,ecx                 ;ecx 32 bit clean
        mov cx,[esi+FC_OFFSET]      ;ecx = Offset in Font-Page
        add ecx,prms.LD_LINE_IGNORE ;Ignore Line Offset aufaddieren

        mov esi,prms.LD_FONT
        mov esi,[esi+VFNT_PAGE]     ;Ptr auf Font-Page
        add esi,ecx                 ;esi = Pointer in Font-Page

sel_return_2:
        mov ebx,_dt_BytesPerRow
        sub ebx,eax                 ;ebx = Display-Line-Adder

        mov edx,prms.LD_PAGE_WIDTH
        sub edx,eax                 ;edx = Font-Page-Line-Adder

        push edi                    ;Display-Ptr -> Stack

        mov ch,prms.LD_LINE_HEIGHT ;ycount initialisieren

        sub esp,12          ;lokaler Variablen-Bereich
        mov [4+esp],ebx     ;Display-Zeilen-Adder
        mov [8+esp],eax     ;der xcount
        xor ebx,ebx

ycycl:  mov cl,[8+esp]      ;xcount laden (***HACK***)
xcycl:  mov bl,[esi]        ;Fontpage Pixel laden
        test bl,bl
        je skip
        mov [edi],bl        ;resultierender Farbcode nach Display
skip:   inc esi             ;Fontpage++
        inc edi             ;Display++
        dec cl              ;xcount--
        jne xcycl           ;und nächstes Pixel
        add esi,edx         ;Fontpage += FontpageAdder
        add edi,[4+esp]     ;Display  += Display-Adder
        dec ch
        jne ycycl           ;nächste Zeile

        mov eax,[8+esp]     ;Char-Breite wiederherstellen
        add esp,12          ;Stack wiederherstellen

        pop edi                     ;Display-Ptr <- Stack
        add edi,eax                 ;Display-Ptr auf Start nächstes Char
        jmp read
;--------------------------------------------------------------------
do_selektor:
        mov al,[ebp]                ;lese Selektor
        inc ebp
        lea esi,jump_table          ;jump_table...
        jmp dword ptr [esi+eax*4]   ;Selektor via Jump-Table bespringen

;--------------------------------------------------------------------
;   Introducing: Die Selektoren.
;--------------------------------------------------------------------
sel_00:         ;END OF STRING
        pop ebp                     ;aktuelle Return-Stream-Position
        cmp ebp,0                   ; == 0??? (Endekennzeichen)
        jne read                    ;nein -> weiter mit nächstem Char

        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx
        ret
;--------------------------------------------------------------------
sel_01:         ;setze X-Pos absolut
        GetX
        add ax,WORD PTR _dt_XOffset          ;neue X-Pos normalisieren

        ModifyX
        GetPixelAddress
        ResetLineParams
        jmp read
;--------------------------------------------------------------------
sel_02:         ;setze Y-Pos absolut
        GetY
        add bx,WORD PTR _dt_YOffset

        ModifyY
        GetPixelAddress
        ResetLineParams
        jmp read
;--------------------------------------------------------------------
sel_03:         ;setze X-Pos Rand-relativ
        GetX
        cmp ax,0                    ;positiv oder negativ?
        jge left_03
        add ax,WORD PTR _dt_Width
left_03:
        ModifyX
        GetPixelAddress
        ResetLineParams
        jmp read
;--------------------------------------------------------------------
sel_04:         ;setze Y-Pos Rand-relativ
        GetY
        cmp bx,0                    ;positiv oder negativ?
        jge top_04
        add bx,WORD PTR _dt_Height
top_04:
        ModifyY
        GetPixelAddress
        ResetLineParams
        jmp read
;--------------------------------------------------------------------
sel_05:         ;setze X-Pos relativ zu aktueller Position
        GetX
        add prms.LD_XPOS,eax
        add edi,eax             ;Display-Ptr direkt updaten
        jmp read
;--------------------------------------------------------------------
sel_06:         ;setze Y-Pos relativ zu aktueller Position
        GetY
        add prms.LD_YPOS,ebx
        mov eax,_dt_BytesPerRow
        mul ebx                     ;Display-Ptr-Modifikator
        add edi,eax
        jmp read
;--------------------------------------------------------------------
sel_07:         ;New Line
        mov eax,prms.LD_LINEX
        mov ebx,prms.LD_LINEY
        mov esi,prms.LD_FONT
        movzx cx,prms.LD_LINE_HEIGHT
        add bx,cx
        mov prms.LD_LINEY,ebx
        mov prms.LD_XPOS,eax
        mov prms.LD_YPOS,ebx
        GetPixelAddress
        ResetLineParams
        jmp read
;--------------------------------------------------------------------
sel_08:         ;New Font
        xor eax,eax
        mov al,[ebp]            ;Font-ID
        inc ebp
        mov esi,_dt_Fonts
        mov esi,[esi+eax*4]     ;esi jetzt auf neue Font-Struktur
        mov prms.LD_FONT,esi
        mov esi,[esi+VFNT_PAGE_BMP] ;Ptr auf Font-Page-Bitmap
        mov ax,[esi+VBMP_WIDTH]     ;ax = Breite der Font-Page
        mov prms.LD_PAGE_WIDTH,eax  ;und nach LD_PAGE_WIDTH...
        jmp read
;--------------------------------------------------------------------
sel_09:         ;Insert Clip
        xor eax,eax
        mov al,[ebp]            ;al = ClipID
        inc ebp
        push ebp                ;aktuellen Input-Stream-Ptr auf Stack retten
        mov esi,prms.LD_CLIPS   ;Ptr auf Clips-Array
        mov ebp,[esi+eax*4]     ;Clip-Ptr nach Input-Stream-Ptr
        jmp read
;--------------------------------------------------------------------
sel_0a:         ;Stretch Char
        xor eax,eax
        mov al,[ebp]            ;Stretch-Width
        inc ebp

sel_0a_jumpin:
        add prms.LD_XPOS,eax    ;<xpos> updaten

        xor ecx,ecx
        mov cl,[ebp]            ;nächstes Char
        inc ebp
        cmp cl,0                ;Selektor? sollte eigentlich nicht vorkommen
        je do_selektor

        mov esi,prms.LD_FONT        ;aktuell gültige Font-Struktur
        mov esi,[esi+VFNT_FCHARS]   ;Anfang des Char-Arrays
        lea esi,[esi+ecx*4]         ;esi jetzt auf aktuelles Char

        xor ecx,ecx                 ;ecx 32 bit clean
        mov cx,[esi+FC_OFFSET]      ;ecx = Offset in Font-Page
        add ecx,prms.LD_LINE_IGNORE ;Ignore Line Offset aufaddieren

        mov esi,prms.LD_FONT
        mov esi,[esi+VFNT_PAGE]     ;Ptr auf Font-Page
        add esi,ecx                 ;esi = Pointer in Font-Page

        mov ebx,_dt_BytesPerRow
        sub ebx,eax                 ;ebx = Display-Line-Adder

        mov edx,prms.LD_PAGE_WIDTH  ;edx = FontPage-Adder

        push edi                    ;Display-Ptr -> Stack
        sub esp,12                  ;Platz für Locals

        mov [4+esp],ebx         ;Display-Line-Adder
        mov [8+esp],eax         ;X-Counter (max. 16 bit)

        mov al,prms.LD_LINE_HEIGHT  ; Y-Counter

ycycl_0a:
        mov ecx,[8+esp]         ;lade X-Counter
        mov bl,[esi]            ;lade statisches Font-Pixel
        test bl,bl
        je skip_0a
xcycl_0a:
        mov [edi],bl            ;Pixel nach Display
        inc edi                 ;Display++
        dec ecx                 ;xcount--
        jne xcycl_0a

        add esi,edx             ;Fontpage += FontpageLineAdder
        add edi,[4+esp]         ;Display  += DisplayLineAdder
        dec al                  ;ycount--
        jne ycycl_0a
skip_0a:
        mov eax,[8+esp]         ;Char-Width wiederherstellen
        add esp,12              ;Stack wiederherstellen
        pop edi                 ;Display-Ptr <- Stack
        add edi,eax             ;Display-Ptr auf Start nächstes Char
        jmp read

;--------------------------------------------------------------------
sel_0b:         ;Stretch To
        xor eax,eax
        mov al,[ebp]            ;al==Endpunkt rel. zu Zeilen-Anfang
        inc ebp

        mov ebx,prms.LD_XPOS    ;aktuelle X-Pos
        sub ebx,prms.LD_LINEX   ;soviele Pixel ab Zeilen-Anf. gezeichnet
        sub eax,ebx             ;in eax jetzt Ziel-Breite
        jmp sel_0a_jumpin
;--------------------------------------------------------------------
sel_0c:             ;Ignore Hori
        xor ecx,ecx
        mov cl,[ebp]            ;soviele Pixel vom nächsten Char ignorieren
        inc ebp
        xor eax,eax
        mov al,[ebp]            ;nächstes Char nach al
        inc ebp

        mov esi,prms.LD_FONT        ;aktuell gültige Font-Struktur
        mov esi,[esi+VFNT_FCHARS]   ;Anfang des Char-Arrays
        lea esi,[esi+eax*4]         ;esi jetzt auf aktuelles Char

        mov ax,[esi+FC_WIDTH]       ;ax ist wahre Breite des Chars
        sub ax,cx                   ;Rest-Breite in ax
        add prms.LD_XPOS,eax        ;<xpos> updaten

        xor ebx,ebx
        mov bx,[esi+FC_OFFSET]      ;Offset des Char in FontPage
        add ebx,ecx                 ;Hori-Ignore-Wert adden
        add ebx,prms.LD_LINE_IGNORE ;Line-Ignore-Wert adden

        mov esi,prms.LD_FONT
        mov esi,[esi+VFNT_PAGE]     ;Ptr auf Font-Page
        add esi,ebx                 ;esi = Pointer in Font-Page
        jmp sel_return_2
;--------------------------------------------------------------------
sel_0d:             ;Cutoff Hori
        xor eax,eax
        mov al,[ebp]            ;nächsten Char so breit
        inc ebp
        xor ecx,ecx
        mov cl,[ebp]            ;nächster Char
        inc ebp

        mov esi,prms.LD_FONT        ;aktuell gültige Font-Struktur
        mov esi,[esi+VFNT_FCHARS]   ;Anfang des Char-Arrays
        lea esi,[esi+ecx*4]         ;esi jetzt auf aktuelles Char

        add prms.LD_XPOS,eax        ;<xpos> updaten
        jmp sel_return_1
;--------------------------------------------------------------------
sel_0e:             ;Ignore Vert
        xor eax,eax
        mov al,[ebp]            ;soviele Pixel-Zeilen ignorieren
        inc ebp

        mov esi,[prms.LD_FONT]
        mov cx,[esi+VFNT_HEIGHT]
        sub cx,ax                   ;neue Gesamt-Höhe dieser Zeile in cx
        mov prms.LD_LINE_HEIGHT,cl
        mul prms.LD_PAGE_WIDTH      ;eax ist LineIgnore-Wert
        mov prms.LD_LINE_IGNORE,eax
        jmp read                    ;nächstes Char...
;--------------------------------------------------------------------
sel_0f:             ;Cutoff Vert
        xor eax,eax
        mov al,[ebp]            ;soviele Pixel-Zeilen zeichnen
        inc ebp

        mov prms.LD_LINE_HEIGHT,al  ;neue Gesamt-Höhe der Zeile
        mov prms.LD_LINE_IGNORE,0   ;DL_LINE_IGNORE löschen
        jmp read                    ;nächstes Char...
;--------------------------------------------------------------------
sel_10:             ;New Font & Flush
        xor eax,eax
        mov al,[ebp]            ;Font-ID
        inc ebp
        mov esi,_dt_Fonts
        mov esi,[esi+eax*4]     ;esi jetzt auf neue Font-Struktur
        mov prms.LD_FONT,esi
        mov esi,[esi+VFNT_PAGE_BMP] ;Ptr auf Font-Page-Bitmap
        mov ax,[esi+VBMP_WIDTH]     ;ax = Breite der Font-Page
        mov prms.LD_PAGE_WIDTH,eax  ;und nach LD_PAGE_WIDTH...
        ResetLineParams
        jmp read

;--------------------------------------------------------------------
sel_11:         ;Stretch To Long
        GetX
        mov ebx,prms.LD_XPOS    ;aktuelle X-Pos
        sub ebx,prms.LD_LINEX   ;soviele Pixel ab Zeilen-Anf. gezeichnet
        sub eax,ebx             ;in eax jetzt Ziel-Breite
        jmp sel_0a_jumpin
;--------------------------------------------------------------------

rst_DrawText_   endp
;--------------------------------------------------------------------
_TEXT   ends
        end
