;
; $Source: PRG:VFM/Classes/_DosMouse/mhandler.asm,v $
; $Revision: 38.1 $
; $Date: 1996/03/02 22:19:26 $
; $Locker:  $
; $Author: floh $
;
;   Der eigentliche MouseHandler, der aber nicht weiter
;   macht, als die C-Funktion dmouse_MouseHandler() mit den
;   richtigen Parametern aufzurufen.
;
; (C) Copyright 1995 by A.Weissflog
;
        assume CS:_TEXT
        assume DS:DGROUP

        extrn dmouse_MouseHandler_:NEAR
        extrn __GETDS:NEAR

DGROUP  segment byte public 'DATA'
busy        dw 0                    ;BUSY Flag
DGROUP  ends

_TEXT   segment byte public 'CODE'

        public dmouse_AsmMouseHandler_
dmouse_AsmMouseHandler_ proc far
        pushad
        push ds
        push es
        push fs
        push gs
        cld
        call __GETDS

        mov cx,ds
        mov es,cx

; Kaskadierung verhindern
        cmp word ptr busy,0
        jne donothing
        mov word ptr busy,1

        movzx eax,ax                ;Ereignis-Maske, 1.Parameter
        movzx edx,bx                ;Status der Maus-Knöpfe, 2.Parameter
        movsx ebx,si                ;horizontale Maus-Position
        movsx ecx,di                ;vertikale Maus-Position

        call dmouse_MouseHandler_   ;C-Mousehandler aufrufen

        mov word ptr busy,0
donothing:
        pop gs
        pop fs
        pop es
        pop ds
        popad
        retf

dmouse_AsmMouseHandler_ endp
_TEXT   ends
        end







