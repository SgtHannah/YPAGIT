**
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_floodm68k.s,v $
**  $Revision: 38.4 $
**  $Date: 1996/02/04 23:12:36 $
**  $Locker:  $
**  $Author: floh $
**
**  m68k Version des EnergyMap Floodfillers.
**
**  (C) Copyright 1995 by A.Weissflog
**
        MC68020
        CODE

**-----------------------------------------------------------------**
**  NAME
**      yw_FloodFill
**
**  SYNOPSIS
**      void yw_FloodFill(__a0 UBYTE *emap,
**                        __d0 ULONG x,
**                        __d1 ULONG y,
**                        __d2 ULONG factor)
**
**  FUNCTION
**      Gigantisch genial-multi-assoziativ-quasi-rekursionaler
**      Floodfiller für die Energy-Map. Vom Punkt [x,y] aus
**      wird jedes Pixel mit dem Wert <owner> durch 0xff
**      überschrieben - nach 4-Stage-Rekursiv-Floodfill-Regel.
**
**      max. Stack-Verbrauch: (mapsize_x * mapsize_y * 4 Bytes)/4
**
**      Die EnergyMap muß in einen 256*256 großen "Framebuffer"
**      eingebettet sein, und muß durch einen 1 Pixel breiten 0-Rand
**      "abgesichert" sein.
**
**      Ahemm... extra care has been taken to keep the stack
**      usage as low as possible... unne suu.
**
**  INPUTS
**      __a0 emap   -> Pointer auf UBYTE-Buffer in 256*256 Organisation
**      __d0 x      -> X-Start-Position
**      __d1 y      -> Y-Start-Position
**      __d2 factor -> "Energie-Stärke" des Kraftwerks
**
**  *** WICHTIG *** WICHTIG *** WICHTIG ***
**      Die Routine darf NIEMALS mit einem (factor == 0) aufgerufen
**      werden!!! Sonst: Stackoverflow und sudden death!
**
**  RESULTS
**      ---
**
**  TRASHES
**      d0-d1/a0-a1
**
**  CHANGED
**      19-Jul-95   floh    created
**      14-Oct-95   floh    neu: factor, Energie-Map-Size ist jetzt
**                          hardgecodet auf 64x64!!!
**      15-Oct-95   floh    Ooops, dummerweise wurden nach dem
**                          neuen Verfahren bereits gefüllte Pixel wieder
**                          gefüllt... und wieder, und wieder, und wieder...
**                          bis zum Overflow.
**      30-Jan-96   floh    EFactor-Overflow-Verhinderung
**
        XDEF _yw_FloodFill
_yw_FloodFill:
        movem.l d2-d4,-(a7)
        lsl.w #6,d1         ;y-Koord als Index
        or.w d1,d0          ;d0.l ist Index in EnergyMap

        move.b (1,a0,d0.l*2),d1 ;Overflow-Check
        add.b d2,d1
        bcc.s no_ovl

        moveq #(0-1),d1      ; Overflow -> EFactor-Adder angleichen
        sub.b (1,a0,d0.l*2),d1
        beq.s cmback        ;ist bereits restlos voll...

no_ovl
        move.w (a0,d0.l*2),d1   ;das ist eine Kombination aus Owner und
                                ;aktuellem Energie-Faktor

        move.w #(cmback-return),-(a7)
        bra.s flood
cmback  movem.l (a7)+,d2-d4
        rts

flood   lea (a0,d0.l*2),a1  ;a1 ist Pointer auf emap[x,y]
        cmp.w (a1),d1       ;Owner-Check + bereits gefüllt Check
        bne.s quit          ;rekursiv up.
        add.b d2,1(a1)      ;doch: Factor aufaddieren

        move.w d0,-(a7)     ;rette [x,y]

        add.w #((1<<6)*1+0),d0  ;orig_y++
        move.w #(cback1-return),-(a7)
        bra.s flood         ;quasi rekursiv...

cback1  sub.w #((1<<6)*2+0),d0  ;orig_y--
        move.w #(cback2-return),-(a7)
        bra.s flood

cback2  add.w #((1<<6)*1+1),d0  ;orig_x++
        move.w #(cback3-return),-(a7)
        bra.s flood

cback3  sub.w #((1<<6)*0+2),d0   ;orig_x--
        move.w #(cback4-return),-(a7)
        bra.s flood

cback4  move.w (a7)+,d0     ;[x,y]-Position zurück

quit    move.w (a7)+,d4     ;Return-Adresse, PC-relativ
return  jmp (return,d4.w)

**-----------------------------------------------------------------**
        END

