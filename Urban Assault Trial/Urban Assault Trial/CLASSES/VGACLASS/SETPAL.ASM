;
;  $Source: PRG:VFM/Classes/_VgaClass/setpal.asm,v $
;  $Revision: 38.2 $
;  $Date: 1997/01/20 22:42:00 $
;  $Locker:  $
;  $Author: floh $
;
;   Lowlevel VGA-Paletten-Setz-Funktion.
;
;  (C) Copyright 1996 by A.Weissflog
;
    assume CS:_TEXT
    assume DS:DGROUP

;--------------------------------------------------------------------
_TEXT   segment byte public 'CODE'

;--------------------------------------------------------------------
;   FUNCTION
;       vga_SetPal -- Lowlevel-Paletten-Setzen, synchronisiert
;                     mit V-Retrace
;
;   INPUTS
;       eax - Ptr auf Farbpalette mit 256 Einträgen, RGB,
;             6 bpg!
;
;   CHANGED
;       23-Sep-96   floh    created
;
        public vga_SetPal_
vga_SetPal_ proc near
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi

        mov esi,eax             ; Source Pointer nach esi
        mov ecx,256*3           ; 256 Entries x 3 Bytes Per Entry

        mov dx,03dah            ; CRT controller input status 1 register
cyc1:   in al,dx                ; warte auf Start des Vertical Retrace
        test al,08h
        jnz cyc1
cyc2:   in al,dx                ; warte auf Ende des Vertical Retrace
        test al,08
        jz cyc2

        xor al,al               ; 1.Farbnummer immer 0
        mov dx,03c8h            ; DAC palette index register
        out dx,al               ; 1.Farbindex
        inc dx                  ; DAC Color Entry
        rep outsb               ; und kontinuierlich schreiben...

        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        ret

vga_SetPal_ endp

;--------------------------------------------------------------------
;   FUNCTION
;       vbe_SetPal -- ruft VBE2.0 Set Palette über PMode-Interface
;                     auf
;
;   INPUTS
;       eax - Ptr auf VBE SetPalette Funktion
;       edx - UBYTE * auf Farbpalette (VBE-Format!)
;
;   CHANGED
;       26-Nov-96   floh    created
;
        public vbe_SetPal_
vbe_SetPal_ proc near
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push es

        mov si,ds
        mov es,si
        mov edi,edx     ; es:edi Pointer auf Palette
        mov ebx,80H     ; Set Palette Data, Wait for VRT
        mov ecx,100H    ; 256 Einträge
        xor edx,edx     ; ab Eintrag 0
        call eax        ; Set Palette

        pop es
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        ret
vbe_SetPal_ endp

;--------------------------------------------------------------------
_TEXT   ends
        end

