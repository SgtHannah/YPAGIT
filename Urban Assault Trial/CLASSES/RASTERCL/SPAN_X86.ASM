;
;  $Source: PRG:VFM/Classes/_RasterClass/span_x86.asm,v $
;  $Revision: 38.5 $
;  $Date: 1998/01/06 14:58:48 $
;  $Locker:  $
;  $Author: floh $
;
;   All die PC-Spandrawer...
;
;  (C) Copyright 1994,1995,1996 by A.Weissflog
;
ifdef MASM
.386
DGROUP=_DATA
span_lnf_ EQU @span_lnf@12
span_nnn_ EQU @span_nnn@12
		
span_lnn_ EQU @span_lnn@12
span_lnc_ EQU @span_lnc@12
span_lgn_ EQU @span_lgn@12
span_lgc_ EQU @span_lgc@12
		
span_znn_ EQU @span_znn@12
span_znc_ EQU @span_znc@12
span_zgn_ EQU @span_zgn@12
span_zgc_ EQU @span_zgc@12

rst_GenClusterStack_ EQU _rst_GenClusterStack
ENDIF

        assume CS:_TEXT
        assume DS:DGROUP

;--------------------------------------------------------------------
        extrn rst_GenClusterStack_:NEAR

;--------------------------------------------------------------------
;   ein paar Struktur-Definitionen
;

; *** struct rast_scanline ***
RS_DRAWSPAN equ 0       ; <APTR>
RS_FLAGS    equ 4       ; <WORD>
RS_Y        equ 6       ; <WORD>
RS_X0       equ 8       ; <WORD>
RS_DX       equ 10      ; <WORD>
RS_B0       equ 12      ; <ULONG>
RS_Z0       equ 16      ; <ULONG>
RS_U0       equ 20      ; <ULONG>
RS_V0       equ 24      ; <ULONG>
RS_DB       equ 28      ; <ULONG>
RS_DZ       equ 32      ; <ULONG>
RS_DU       equ 36      ; <ULONG>
RS_DV       equ 40      ; <ULONG>
RS_MAP      equ 44      ; <struct VFMBitmap *>

; *** struct rast_cluster (modified!) ***
cl_count    equ 0   ; LONG

; für gleichzeitigen WORD/LONG Zugriff!
cl_v        equ 4   ; WORD
cl_u        equ 6   ; WORD
cl_uv       equ 4   ;  LONG!    (u<<16|v)

cl_dv       equ 8   ; WORD
cl_du       equ 10  ; WORD
cl_duv      equ 8   ;  LONG!    (du<<16|dv)

cl_size     equ 12

; *** struct raster_data ***
RD_R            equ 0   ; <struct VFMBitmap *>
RD_FGPEN        equ 4   ; <ULONG>
RD_FGAPEN       equ 8   ; <ULONG>
RD_BGPEN        equ 12   ; <ULONG>
RD_SHADEBMP     equ 16  ; <struct VFMBitmap *>
RD_SHADEBODY    equ 20  ; <UBYTE *>
RD_TRACYBMP     equ 24  ; <struct VFMBitmap *>
RD_TRACYBODY    equ 28  ; <UBYTE *>
RD_CLUSTERSTACK equ 32  ; <struct rast_cluster *>
; Rest ist uninteressant

; *** lokaler Stack-Variablen-Bereich für Perspektiv-Mapper ***/
gcs_size         equ 8*4

gcs_count        equ 0   ; ULONG
gcs_dz           equ 4   ; ULONG
gcs_du           equ 8   ; ULONG
gcs_dv           equ 12  ; ULONG
gcs_end_u        equ 16  ; LONG
gcs_end_v        equ 20  ; LONG
gcs_clsize       equ 24  ; LONG
gcs_clshift      equ 28  ; LONG

;--------------------------------------------------------------------
;   Struktur-Definition für lokalen Variablen-Bereich
;   im Depth-Mapper.
;
LOCAL_SIZE      equ 16  ;4*LONG

LOC_Count       equ 0   ;ULONG
LOC_DitherErr   equ 4   ;ULONG
LOC_ClusterPtr  equ 8   ;ULONG
LOC_BytesPerRow equ 12  ;ULONG

;--------------------------------------------------------------------
;   Definition für Cluster-Size-Lookup-Table
;
_DATA   segment byte public 'DATA'
ClShiftArray    DD  6, 5, 5, 5, 4, 4, 4, 4      ; 8 Einträge!
ClSizeArray     DD 64,32,32,32,16,16,16,16
_DATA   ends

;--------------------------------------------------------------------
_TEXT   segment dword public 'CODE'

;====================================================================
;   SUPPORT MACROS
;====================================================================
_Prolog     MACRO
        push ebx
        push ecx
        push edx
        push edi
        push esi
        push ebp
        ENDM

_Epilog     MACRO
        pop ebp
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx
ifdef MASM
        ret 4
else
        ret
endif
        ENDM

;--------------------------------------------------------------------
;   _GenClusterStack
;
;   In:
;       edx - Pointer auf Scanline-Data
;       ebx - Pointer auf LID des raster.class Objects
;
;       folgende Festkomma-Formate werden erwartet:
;           z0 -> 2.30
;           u0 -> 4.28
;           v0 -> 4.28
;
;   Out:
;       initialisierter Cluster-Stack in rd->cluster_stack
;
;   Trash:
;       ALLES!
;
_GenClusterStack MACRO
        LOCAL l1,do_chunk,do_rest,done

        sub esp,gcs_size    ; Platz für lokale Variablen
        mov edi,[ebx+RD_CLUSTERSTACK]
        mov esi,edx

    ;
    ; ermittle optimale ClusterSize
    ;-------------------------------
    ; Out:
    ;   [esp+gcs_clsize]
    ;   [esp+gcs_clshift]
    ;
        mov eax,[esi+RS_DZ]

        mov ecx,eax             ; eax = abs(eax)
        sar ecx,31
        xor eax,ecx
        sub eax,ecx

        shr eax,13              ; DZ auf Tabellen-Lookup zurechtstutzen
        cmp eax,7
        jle l1
        mov eax,7
l1:
        mov ecx,ClShiftArray[eax*4]
        mov eax,ClSizeArray[eax*4]
        mov [esp+gcs_clshift],ecx
        mov [esp+gcs_clsize],eax

    ; Berechne letztes u|v Pair vornweg
    ;-----------------------------------
    ; In:
    ;   edi - Pointer auf Clusterstack
    ;   esi - Pointer auf Scanline-Data (struct rast_scanline *)
    ; Out:
    ;   [esp+gcs_count]  initialisiert
    ;   [esp+gcs_end_uv] initialisiert
    ;
        xor ecx,ecx                 ; Counter initialisieren
        mov cx,[esi+RS_DX]
        mov [esp+gcs_count],ecx

        mov eax,[esi+RS_DZ]         ; ez = (z0 + dz*dx)>>4
        imul ecx
        add eax,[esi+RS_Z0]
        mov ebp,eax
        shr ebp,10

        mov eax,[esi+RS_DU]         ; eu = (u0 + du*dx)/ez
        imul ecx
        add eax,[esi+RS_U0]
        xor edx,edx
        div ebp
        mov [esp+gcs_end_u],eax

        mov eax,[esi+RS_DV]         ; ev = (v0 + dv*dx)/ez
        imul ecx
        add eax,[esi+RS_V0]
        xor edx,edx
        div ebp
        mov [esp+gcs_end_v],eax

    ; initialisiere Parameter und erstes [u,v] Paar
    ;-----------------------------------------------
    ;
        mov ecx,[esp+gcs_clshift]
        mov eax,[esi+RS_DU]
        mov ebx,[esi+RS_DV]
        mov edx,[esi+RS_DZ]

        shl eax,cl      ; du<<=clust_shift
        shl edx,cl      ; dz<<=clust_shift
        shl ebx,cl      ; dv<<=clust_shift
        sar edx,10      ; Festkomma-Korrektur

        mov [esp+gcs_du],eax
        mov [esp+gcs_dv],ebx
        mov [esp+gcs_dz],edx

        mov ebx,[esi+RS_Z0]         ; ebx = z0
        mov ecx,[esi+RS_U0]         ; ecx = u0
        mov esi,[esi+RS_V0]         ; esi = v0
        shr ebx,10                  ; z0 >>= 10 Festkomma-Korrektur

        xor edx,edx             ; u = u0/z0
        mov eax,ecx
        div ebx
        mov [edi+cl_u],ax

        xor edx,edx             ; v = v0/z0
        mov eax,esi
        div ebx
        mov [edi+cl_v],ax

    ;
    ; Full Cluster Schleife, jeweils 16 Pixel am Stück
    ;--------------------------------------------------
    ; eax - scratch
    ; edx - scratch
    ;
    ; ebx - z0
    ; ecx - u0
    ; esi - v0
    ; edi - Cluster-Stack-Ptr
    ;
    ; [esp + gcs_count]
    ; [esp + gcs_dz]
    ; [esp + gcs_du]
    ; [esp + gcs_dv]
    ;

        mov edx,[esp+gcs_clsize]
do_chunk:
        sub dword ptr [esp+gcs_count],edx
        jl do_rest

        add ecx,[esp+gcs_du]    ; u0 += du
        xor edx,edx
        add ebx,[esp+gcs_dz]    ; z0 += dz

        mov eax,ecx             ; u0 -> eax
        div ebx                 ; next_u = u0/z0
        add esi,[esp+gcs_dv]    ; v0 += dv
        mov ebp,eax             ; next_u -> ebp

        xor edx,edx
        mov eax,esi             ; v0 -> eax
        div ebx                 ; next_v = v0/z0

        push ecx            ; alle Stack-Refs + 4!

        mov [edi+cl_size+cl_v],ax       ; store v
        xor edx,edx
        mov [edi+cl_size+cl_u],bp       ; store u
        mov dx,[edi+cl_u]
        sub ebp,edx         ; cluster_du = next_u - act_u
        mov dx,[edi+cl_v]
        mov ecx,[4+esp+gcs_clshift]
        sub eax,edx         ; cluster_dv = next_v - act_v
        sar ebp,cl
        sar eax,cl
        shl ebp,16
        pop ecx
        mov bp,ax           ; ebp jetzt (du<<16|dv)

        mov edx,[esp+gcs_clsize]
        mov [edi+cl_duv],ebp        ; store (du<<16|dv)
        mov dword ptr [edi+cl_count],edx    ; volle Cluster-Größe
        add edi,cl_size                     ; nächstes Cluster-Stack-Element
        jmp do_chunk

    ;
    ; Rest Pixel Code, falls weniger als 16 Rest-Pixel
    ;--------------------------------------------------
    ;

do_rest:
        mov ebp,[esp+gcs_count]
        add ebp,[esp+gcs_clsize]
        jle done                ; endgültig fertig
        xor esi,esi
        mov [edi+cl_count],ebp

        mov eax,[esp+gcs_end_u] ; du = (end_u - act_u)/num_tail
        mov si,[edi+cl_u]
        sub eax,esi
        mov edx,eax
        sar edx,31
        idiv ebp
        mov [edi+cl_du],ax

        mov eax,[esp+gcs_end_v] ; dv = (end_u - act_u)/num_tail
        mov si,[edi+cl_v]
        sub eax,esi
        mov edx,eax
        sar edx,31
        idiv ebp
        mov [edi+cl_dv],ax

        add edi,cl_size
done:
        mov dword ptr [edi+cl_count],0      ; Clusterstack terminieren
        add esp,gcs_size        ; lokalen Variablen-Space killen
        ENDM

;--------------------------------------------------------------------
;   _GetDisplay
;
;   In:
;       eax -> Pointer auf Zeilen-Anfang im Display
;       edx -> <struct rast_scanline *>
;
;   Out:
;       edi -> Direkt-Pointer ins Display
;       ecx -> Pixel-Counter
;       eax -> == edx (<struct rast_scanline *>)
;
;   Trash:
;       ---
;
_GetDisplay MACRO
        xor ecx,ecx
        mov edi,eax
        mov cx,[RS_X0+edx]
        add edi,ecx
        mov cx,[RS_DX+edx]
        mov eax,edx
        ENDM

;--------------------------------------------------------------------
;   _ShadeInit
;
;   In:
;       eax -> <struct rast_scanline *>
;
;   Output Regs:
;       ebx -> shade_act [8.8 fixed point]
;       esi -> shade_add [8.8 fixed point]
;
;   Trash:
;       ---
;
_ShadeInit MACRO
        mov ebx,[RS_B0+eax]
        mov esi,[RS_DB+eax]
        ENDM

;--------------------------------------------------------------------
;   _MapInit (nur lineares Mapping!)
;
;   In:
;       eax -> <struct rast_scanline *>
;
;   Out:
;       edx -> uv  [u0|v0]
;       ebp -> duv [du|dv]
;
;   Note:
;       Der Pointer auf die Txt-Data wird *NICHT* initialisiert!
;
_MapInit MACRO
        push ebx

        mov edx,[RS_U0+eax]
        mov ebx,[RS_V0+eax]
        shl edx,16
        mov dx,bx

        mov ebp,[RS_DU+eax]
        mov ebx,[RS_DV+eax]
        shl ebp,16
        mov bp,bx

        pop ebx
        ENDM

;--------------------------------------------------------------------
;   MACRO
;       ZeroSpan_macro
;   IN
;       eax - Pointer auf Start der Display-Zeile
;       edx - <struct rast_scanline *>
;       ebx - <struct raster_data *>
;
;   CHANGED
;       01-Apr-96   floh    created
;       05-Jun-96   floh    übernommen nach raster.class
;       30-Oct-96   floh    more Optimizing
;
ZeroSpan_macro MACRO
        LOCAL cycl1,dorest,cycl2,yppsn

        _Prolog
        _GetDisplay     ; edi = display, ecx = count

        xor eax,eax     ; eax als Filler
        cmp ebx,[edi]   ; Cacheline lesen
        mov edx,ecx
        shr ecx,2       ; Anzahl LONGs
        and edx,3       ; Anzahl Rest-Bytes
        rep stosd
        mov ecx,edx
        rep stosb

yppsn:  _Epilog
        ENDM

;--------------------------------------------------------------------
;   MACRO
;       TracyTxtSpan_macro
;
;   IN
;       eax - Pointer auf Start der Display-Zeile
;       edx - <struct rast_scanline *>
;       ebx - <struct raster_data *>
;
;   CHANGED
;       30-Mar-96   floh    created
;       05-Jun-96   floh    übernommen nach raster.class
;
TracyTxtSpan_macro  MACRO
        LOCAL cycl

        _Prolog
        _GetDisplay ;edi = display, ecx = count, eax = <struct rast_scanline *>
        _MapInit    ;edx = map_act, ebp = map_add

        mov esi,[RS_MAP+eax]        ;esi = Txt-Pointer
        xor eax,eax
        mov ebx,[RD_TRACYBODY+ebx]  ;ebx = Remap-Tabelle

        add edi,ecx             ; wegen Counter == Offset
        mov ah,dh               ;Preloop Part 1 Map Offset Ermittlung
        neg ecx
        rol edx,8               ;Preloop Part 2 Map Offset Ermittlung

        align 4
cycl:   mov al,dl               ;Part 3 Map Offset ermitteln
        ror edx,8               ;Part 4 Map Offset
        mov al,[esi+eax]        ;hole Textur-Pixel
        mov ah,[edi+ecx]        ;hole Hintergrund-Pixel aus Display
        add edx,ebp             ;map_act += map_add
        mov al,[ebx+eax]        ;resultierendes (geremapptes) Pixel
        mov ah,dh               ;Part 1 Map Offset ermitteln
        mov [edi+ecx],al        ;*Display = Pixel
        inc ecx                 ;Counter
        rol edx,8               ;Part 2 Map Offset ermitteln
        jne cycl

        _Epilog
        ENDM

;--------------------------------------------------------------------
;   MACRO
;       TxtSpan_macro map,shd,trc
;
;   INPUTS
;       map = L
;       shd = N
;       trc = [N or C]
;
;       eax - Pointer auf Start der Display-Zeile
;       edx - <struct rast_scanline *>
;       ebx - <struct raster_data *>
;
;   CHANGED
;       20-Sep-95   floh    rewrite
;       30-Sep-95   floh    Skip Pixel Schleife entfernt, ist nicht
;                           mehr notwendig, wegen Span-Clipper!
;       05-Jun-96   floh    übernommen nach raster.class
;       30-Oct-96   floh    more Optimizing
;
TxtSpan_macro   MACRO map,shd,trc
        LOCAL cycl,ignore_pixel

        _Prolog
        _GetDisplay ;edi = display, ecx = count, eax = <struct rast_scanline *>
        _MapInit    ;edx = map_act, ebp = map_add

        add edi,ecx             ; wegen Counter == Offset
        mov esi,[RS_MAP+eax]    ;esi = Txt-Pointer
        neg ecx                 ; cnt = -cnt
        xor eax,eax
        cmp ebx,[edi+ecx]       ; Preread Cacheline

        mov ah,dh               ;Preloop Part 1 Map Offset Ermittlung
        rol edx,8               ;Preloop Part 2 Map Offset Ermittlung

        align 4
cycl:   mov al,dl               ;Part 3 Map Offset ermitteln
        ror edx,8               ;Part 4 Map Offset ermitteln

        mov bl,[esi+eax]        ;Texel holen
        add edx,ebp             ;map_act += map_add
        IFIDN <trc>,<C>     ;Falls Clear Transparenz ON
            test bl,bl          ; (Texel == 0)???
            je  ignore_pixel    ; dann Pixel ignorieren
        ENDIF

        mov [edi+ecx],bl            ;*Display = Pixel

ignore_pixel:
        mov ah,dh               ;Part 1 Map Offset ermitteln
        inc ecx
        rol edx,8               ;Part 2 Map Offset ermitteln
        jne cycl

        _Epilog
        ENDM

;--------------------------------------------------------------------
;   MACRO
;       TxtShadeSpan_macro map,shd,trc
;
;   INPUTS
;       map = L
;       shd = G
;       trc = [N or C]
;       eax - Pointer auf Start der Display-Zeile
;       edx - <struct rast_scanline *>
;       ebx - <struct raster_data *>
;
;   CHANGED
;       29-Sep-95   floh    rewrite
;       30-Sep-95   floh    Skip Pixel Schleife entfernt, ist nicht
;                           mehr notwendig, wegen Span-Clipper!
;       25-Dec-95   floh    Shading jetzt per Remap-Tabelle.
;       29-Dec-95   floh    debugging... (Map-Offset im Transparenz-Teil)
;       05-Jun-96   floh    übernommen nach raster.class, sowie das
;                           <shade_add>,<count> Merge rausgenommen,
;                           weil es ja die bekannten Probleme verursacht
;                           hat...
;       30-Oct-96   floh    + more Optimizing
;
TxtShadeSpan_macro   MACRO map,shd,trc
        LOCAL cycl,ignore_pixel,done

        _Prolog
        push ebx    ;<struct raster_data *> in Sicherheit
        _GetDisplay ;edi = display, ecx = count, eax = <struct rast_scanline *>
        _ShadeInit  ;esi = shade_add, ebx = shade_act
        _MapInit    ;ebp = map_add, edx = map_act

        push esi    ; shade_add auf [4+esp]
        push ebp    ; map_add auf [esp]

        mov esi,[8+esp]     ; esi = <struct raster_data *>
        add edi,ecx         ; wegen Counter==Offset
        mov ebp,[RD_SHADEBODY+esi]  ; ebp = Ptr auf ShadeRemap
        neg ecx
        mov esi,[RS_MAP+eax]        ; esi = Ptr auf TextureMap

        xor eax,eax
        cmp ebx,[edi+ecx]       ; Preread Cacheline
        mov ah,dh               ;Preloop Part 1 Map Offset Ermittlung
        rol edx,8               ;Preloop Part 2 Map Offset Ermittlung

        align 4
cycl:
        mov al,dl               ;Part 3 Map Offset ermitteln
        ror edx,8               ;Part 4 Map Offset ermitteln
        mov al,[esi+eax]        ;Texel holen
        IFIDN <trc>,<C>
            test al,al
            je ignore_pixel
        ENDIF

        mov ah,bh               ;ax jetzt Offset in Remap-Data
        add edx,[esp]           ;map_act   += map_add
        mov al,[ebp+eax]        ;al ist resultierendes Pixel
        mov ah,dh               ;Part 1 Map Offset ermitteln
        add ebx,[4+esp]         ;shade_act += shade_add
        rol edx,8               ;Part 2 Map Offset ermitteln
        mov [edi+ecx],al        ;*display = pixel
        inc ecx
        jne cycl
        add esp,12              ;lokalen Variablen-Bereich killen

        IFIDN <trc>,<C>
        jmp done                ;ignore_pixel Part überspringen
ignore_pixel:
        mov ah,dh               ;Part 1 Map Offset ermitteln
        add edx,[esp]           ;map_act += map_add
        add ebx,[4+esp]         ;shade_act += shade_add, pix_count--
        inc ecx
        rol edx,8               ;Part 2 Map Offset ermitteln
        jne cycl
        add esp,12              ;lokalen Variablen-Bereich killen
        ENDIF

done:
        _Epilog
        ENDM

;--------------------------------------------------------------------
;   MACRO
;       DepthSpan_macro map,shd,trc
;
;   INPUTS
;       map = Z
;       shd = N
;       trc = [N or C]
;
;       eax - Pointer auf Start der Display-Zeile
;       edx - <struct rast_scanline *>
;       ebx - <struct raster_data *>
;
;   CHANGED
;       29-Sep-95   floh    rewrite
;       30-Sep-95   floh    Skip Pixel Schleife entfernt, ist nicht
;                           mehr notwendig, wegen Span-Clipper!
;       05-Jun-96   floh    übernommen nach raster.class
;       09-Jun-96   floh    + debugging...
;       30-Oct-96   floh    + more Optimizing
;
DepthSpan_macro   MACRO map,shd,trc
        LOCAL cycl,new_cluster,ignore_pixel

        _Prolog

; *** benutzt inlined Macro ***/
        push eax
        push ebx
        push edx
        _GenClusterStack
        pop edx
        pop ebx
        pop eax

; *** benutzt externen C-Call ***/
;        push eax            ;rette SpanElm *
;        push ebx
;        push edx            ;rette Y-Koord.
;        mov eax,[RD_CLUSTERSTACK+ebx]   ;Args sind [eax,edx]
;        call rst_GenClusterStack_       ;ClusterStack erzeugen
;        pop edx
;        pop ebx
;        pop eax

        _GetDisplay ;edi = display, ecx = count, eax = <struct rast_scanline *>

        mov ebx,[RD_CLUSTERSTACK+ebx]   ; ebx ist ClusterStack-Ptr
        mov esi,[RS_MAP+eax]            ;esi = Txtmap Pointer
        xor eax,eax                     ;eax 32 bit clean
        jmp new_cluster

        align 4
cycl:   mov al,dl               ;Part 3 Map-Offset ermitteln
        ror edx,8

        mov al,[esi+eax]        ;aktuelles Texel nach <al>
        add edx,ebp             ;map_act += map_add

        IFIDN <trc>,<C>     ;Falls Clear Transparenz ON
            test al,al          ; (Texel == 0)???
            je  ignore_pixel    ; dann Pixel ignorieren
        ENDIF
        mov [edi+ecx],al        ;Pixel nach Display!

ignore_pixel:
        mov ax,dx               ;Part 1 Map Offset ermitteln...
        inc ecx
        rol edx,8               ;Part 2 Map Offset ermitteln..
        jne cycl

new_cluster:
        mov ecx,[ebx]           ; neuen Counter laden
        mov edx,4[ebx]          ; edx = map_act
        add edi,ecx             ;  wegen Counter==Offset
        mov ebp,8[ebx]          ; ebp = map_add
        neg ecx                 ;  Negativ-Counter
        mov ax,dx               ;Pre Part1 Map Offset ermitteln
        add ebx,12              ; ebx zeigt auf nächsten Cluster
        cmp eax,[edi+ecx]       ;  Preread Cacheline
        rol edx,8               ;Pre Part2 Map Offset ermitteln
        test ecx,ecx
        jne cycl

        _Epilog
        ENDM

;--------------------------------------------------------------------
;   MACRO
;       DepthShadeSpan_macro map,shd,trc
;
;   INPUTS
;       map = Z
;       shd = G
;       trc = [N or C]
;
;   RESULTS
;
;   CHANGED
;       29-Sep-95   floh    rewrite
;       30-Sep-95   floh    Skip Pixel Schleife entfernt, ist nicht
;                           mehr notwendig, wegen Span-Clipper!
;       25-Dec-95   floh    Shading jetzt mit Remap-Tabelle
;       07-Jan-96   floh    Fehler in Texture-Map-Offset-Berechnung
;                           bei IgnorePixel behoben
;       09-Jun-96   floh    + debugging
;
DepthShadeSpan_macro   MACRO map,shd,trc
	LOCAL cycl
	LOCAL new_cluster
	LOCAL ignore_pixel
	LOCAL ignore_span
	LOCAL end2
	LOCAL sadd_neg

        _Prolog

; *** benutzt inlined Macro ***
        push ebx                    ;[0+esp] = <struct raster_data *>
        push eax
        push edx
        _GenClusterStack
        pop edx
        pop eax

;        push ebx
;        push eax                    ;rette SpanElm *
;        push edx                    ;rette y-koord
;        mov eax,[RD_CLUSTERSTACK+ebx]   ;Args sind [eax,edx]
;        call rst_GenClusterStack_   ;ClusterStack erzeugen
;        pop edx
;        pop eax

        _GetDisplay ;edi = display, ecx = count, eax = <struct rast_scanline *>
        _ShadeInit  ;esi = shade_add, ebx = shade_act

        push esi                        ;[0+esp] = shade_add
        mov esi,[4+esp]                 ;[4+esp] -> <struct raster_data *>
        mov edx,[RD_CLUSTERSTACK+esi]
        mov ebp,[RD_SHADEBODY+esi]
        push edx                        ;[0+esp] = cluster_stack *
                                        ;[4+esp] = shade_add
                                        ;[8+esp] = <struct raster_data *>

        sub esp,4               ;Platz für <map_add>
        mov esi,[RS_MAP+eax]    ;esi = Txtmap-Pointer
        xor eax,eax             ;32 bit clean
        jmp new_cluster

; Stack: [0+esp]  = map_add
;        [4+esp]  = cluster_stack *
;        [8+esp]  = shade_add
;        [12+esp] = <struct raster_data *>

        align 4
cycl:   ror edx,8               ;Part4 Map Offset ermitteln

        mov al,[esi+eax]        ;aktuelles Texel jetzt in <al>
        IFIDN <trc>,<C>     ;Falls Clear Transparenz ON
            test al,al          ; (Texel == 0)???
            je  ignore_pixel    ; dann Pixel ignorieren
        ENDIF

        mov ah,bh               ;ax ist jetzt Offset auf Shading-Tabelle
        add edx,[esp]           ;map_act += map_add
        mov al,[ebp+eax]        ;al ist resultierendes Pixel
        mov ah,dh               ;Part 1 Map Offset ermitteln
        add ebx,[8+esp]         ;shade_act += shade_add
        rol edx,8               ;Part 2 Map Offset ermitteln
        mov [edi+ecx],al        ;*display = pixel
        inc ecx                 ;count--
        mov al,dl               ;Part3 Map Offset ermitteln
        jne cycl

new_cluster:
        mov eax,[esp+4]         ;ClusterStack-Pointer laden
        mov edx,[eax+8]         ; edx=map_add
        mov ecx,[eax]           ; ecx=count
        mov [esp],edx           ; store map_add
        add edi,ecx             ; wegen Counter=Offset
        mov edx,[eax+4]         ; edx=map_act
        add eax,12              ; nächstes ClusterStack-Element
        neg ecx                 ; count=-count
        mov [esp+4],eax         ;store ClusterStack Pointer
        xor eax,eax             ; 32 Bit clean
        cmp ebx,[edi+ecx]       ; Preload Cache Line
        mov ah,dh               ; Pre Part 1 Map Offset
        rol edx,8               ; Pre Part 2 Map Offset
        test ecx,ecx            ; Abbruch?
        mov al,dl               ; Pre Part 3 Map Offset
        jne cycl

end2:   add esp,16              ;lokalen Variablen-Bereich killen
        _Epilog

        IFIDN <trc>,<C>     ;Ignore-Pixel-Part für Clear-Transparenz
ignore_pixel:
            add edx,[esp]       ;map_act += map_add
            mov ah,dh           ;Part 1 Map Offset ermitteln
            add ebx,[8+esp]     ;shade_act += shade_add
            inc ecx
            rol edx,8           ;Part 2 Map Offset ermitteln
            mov al,dl           ;Part 3 Map Offset ermitteln
            jne cycl
            jmp new_cluster     ;neuer Cluster
        ENDIF

        ENDM

;--------------------------------------------------------------------
;   void span_xxx([eax] UBYTE *start,
;                 [edx] struct rast_scanline *rs
;                 [ebx] struct raster_data *rd)
;
;  FUNCTION
;       Alle Spandrawer in der x86-Version-
;
;  INPUTS
;       eax - Pointer auf Anfang der Zeile in der VFMBitmap
;       edx - Pointer auf Scanline-Data
;       ebx - Pointer auf LID des raster.class Objects
;
;  CHANGED
;       29-Sep-95   floh    created
;       30-Mar-96   floh    neu DrawSpan_LNF_ (Linear, No Shade, Flat-Tracy)
;       01-Apr-96   floh    + DrawSpan_NNN_ (Zero-Flat)
;       05-Jun-96   floh    + übernommen nach raster.class, neue Namen
;
        public span_lnf_
        public span_nnn_

        public span_lnn_
        public span_lnc_
        public span_lgn_
        public span_lgc_

        public span_znn_
        public span_znc_
        public span_zgn_
        public span_zgc_

ifdef MASM
;*** flat, zero color
span_nnn_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        ZeroSpan_macro
span_nnn_   endp

;*** linear mapped, no shade, flat tracy (remapped)
        align 4
span_lnf_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        TracyTxtSpan_macro
span_lnf_   endp

;*** linear mapped, no shade, no tracy
        align 4
span_lnn_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        TxtSpan_macro L,N,N
span_lnn_   endp

;*** linear mapped, no shade, clear tracy
        align 4
span_lnc_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        TxtSpan_macro L,N,C
span_lnc_   endp

;*** linear mapped, gshade, no tracy
        align 4
span_lgn_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        TxtShadeSpan_macro L,G,N
span_lgn_   endp

;*** linear mapped, gshade, clear tracy
        align 4
span_lgc_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        TxtShadeSpan_macro L,G,C
span_lgc_   endp

;*** depth mapped, no shade, no tracy
        align 4
span_znn_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        DepthSpan_macro Z,N,N
span_znn_   endp

;*** depth mapped, no shade, clear tracy
        align 4
span_znc_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        DepthSpan_macro Z,N,C
span_znc_   endp

;*** depth mapped, gshade, no tracy
        align 4
span_zgn_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        DepthShadeSpan_macro Z,G,N
span_zgn_   endp

;*** depth mapped, gshade, clear tracy
        align 4
span_zgc_   proc near
		mov eax, ecx;
		mov	ebx, [esp+4];
        DepthShadeSpan_macro Z,G,C
span_zgc_   endp


else ; MASM
span_nnn_   proc near
        ZeroSpan_macro
span_nnn_   endp

;*** linear mapped, no shade, flat tracy (remapped)
        align 4
span_lnf_   proc near
        TracyTxtSpan_macro
span_lnf_   endp

;*** linear mapped, no shade, no tracy
        align 4
span_lnn_   proc near
        TxtSpan_macro L,N,N
span_lnn_   endp

;*** linear mapped, no shade, clear tracy
        align 4
span_lnc_   proc near
        TxtSpan_macro L,N,C
span_lnc_   endp

;*** linear mapped, gshade, no tracy
        align 4
span_lgn_   proc near
        TxtShadeSpan_macro L,G,N
span_lgn_   endp

;*** linear mapped, gshade, clear tracy
        align 4
span_lgc_   proc near
        TxtShadeSpan_macro L,G,C
span_lgc_   endp

;*** depth mapped, no shade, no tracy
        align 4
span_znn_   proc near
        DepthSpan_macro Z,N,N
span_znn_   endp

;*** depth mapped, no shade, clear tracy
        align 4
span_znc_   proc near
        DepthSpan_macro Z,N,C
span_znc_   endp

;*** depth mapped, gshade, no tracy
        align 4
span_zgn_   proc near
        DepthShadeSpan_macro Z,G,N
span_zgn_   endp

;*** depth mapped, gshade, clear tracy
        align 4
span_zgc_   proc near
        DepthShadeSpan_macro Z,G,C
span_zgc_   endp

endif ; MASM

;====================================================================
_TEXT   ends
        end

