;
; $Source: PRG:VFM/Classes/_YPAWorldClass/yw_floodx86.asm,v $
; $Revision: 38.3 $
; $Date: 1996/02/04 23:13:02 $
; $Locker:  $
; $Author: floh $
;
;   x86-Version des EnergyMap Floodfillers (für x= 3..n)
;
; (C) Copyright 1995 by A.Weissflog
;
_TEXT   segment byte public 'CODE'
        assume CS:_TEXT
        assume DS:DGROUP

;-----------------------------------------------------------------**
;  NAME
;   yw_FloodFill
;
;  SYNOPSIS
;   void yw_FloodFill(struct EMapElm *emap, ULONG x, ULONG y, ULONG factor)
;
;  FUNCTION
;   Der Floodfiller halt. Siehe <_YPAWorldClass/yw_floodm68k.s>
;   für mehr Info. Die Motorola-Version war halt eher da :-)
;
;  INPUTS
;       eax [UBYTE *emap]   -> Pointer auf UBYTE-EMap in 256*256 Organisation
;       edx [ULONG x]       -> X-Start-Position
;       ebx [ULONG y]       -> Y-Start-Position
;       ecx [ULONG factor]  -> Energie-Factor, != 0!!!
;
;  RESULTS
;       ---
;
;  TRASHES
;       eax
;
;  CHANGED
;       23-Jul-95   floh    created
;       24-Jul-95   floh    debugging... jetzt leider etwas mehr
;                           Stack-Useage...
;       16-Oct-95   floh    Umgeschrieben, wegen neuer Energie-Philosophie
;                           und EMap-Aufbau
;
;  SEE ALSO
;       _YPAWorldClass/yw_floodm68k.s/_yw_FloodFill()
;
        public yw_FloodFill_
yw_FloodFill_   proc near
        push ebx
        push ecx
        push edx
        push edi

        mov  edi,eax        ;edi = EnergyMap
        shl  bx,6           ;bx  = y*64
        or   dx,bx          ;dx  = Start-Index in Energie-Map

        mov bl,[1+edi+edx*2]    ; Overflow-Check!
        add bl,cl
        jnc no_ovl

        mov cl,255          ; Overflow -> EFactor-Adder angleichen
        sub cl,[1+edi+edx*2]
        je cmback           ; Restlos voll...

no_ovl:
        mov bx,[edi+edx*2]  ;bx ist Konglomerat aus Owner/EFactor

        call flood

cmback: pop  edi
        pop  edx
        pop  ecx
        pop  ebx
        ret
;--------------------------------------------------------------------
flood:  cmp  bx,[edi+edx*2] ;Füll-würdig?
        jne  quit           ;nö.
        add  [1+edi+edx*2],cl   ;doch -> EFactor aufaddieren

        push dx             ;aktuelles (16-Bit-)Offset auf Stack

        add  dx,0040H       ;y++
        call flood

        sub  dx,0080H       ;y--
        call flood

        add  dx,0041H       ;x++
        call flood

        sub  dx,0002H       ;x--
        call flood

        pop dx              ;[x,y] Offset zurück

quit:   ret

yw_FloodFill_ endp

;--------------------------------------------------------------------
_TEXT   ends
        end

