/*
**  $Source: PRG:VFM/Nucleus2/pc/memory.c,v $
**  $Revision: 38.5 $
**  $Date: 1997/02/24 21:58:36 $
**  $Locker:  $
**  $Author: floh $
**
**  Memory-Funktionen für PC (ist allerdings alles ANSI-Stuff,
**  also durchaus wiederverwendbar).
**
**  (C) Copyright 1994 by A.Weissflog
*/
#include <exec/types.h>     /* Amiga-Emulation */
#include <exec/memory.h>    /* Amiga-Emulation */

#include <stdlib.h>         /* ANSI */
#include <string.h>

#include "nucleus/nucleus2.h"

extern struct NucleusBase NBase;

/*-----------------------------------------------------------------*/
APTR nc_AllocVec(ULONG byteSize, ULONG attributes)
/*
**  FUNCTION
**      Speicher-Allokierung mit <size tracking>.
**
**  INPUTS
**      byteSize    -> Größe des angeforderten Speicherblocks in Bytes
**      attributes  -> Speicherattribute
**                          MEMF_CHIP   -> ignoriert auf Non-Amigas
**                          MEMF_FAST   -> ignoriert auf Non-Amigas
**                          MEMF_PUBLIC -> ignoriert auf Non-Amigas
**                          MEMF_CLEAR  -> fülle Speicherblock mit NULL
**
**  RESULTS
**      Pointer auf Speicher-Block oder NULL bei Mißerfolg.
**
**      Layout des Blocks:
**
**      "ABAD","CAFE","byteSize","Start",...,"FOOD"
**
**  CHANGED
**      22-Dec-94   floh    created
**      04-Nov-95   floh    jetzt mit Fehler-Meldung
**      26-Apr-96   floh    Debug-Variable __AllocCount
**      04-Aug-96   floh    + __AllocSize, __AllocMax
**                          + Size wird in MemBlock selbst aufgehoben
**      24-Feb-97   floh    + Diagnose-Variablen jetzt in NBase eingebettet
**      07-May-98   floh    + Mungwall/Boundschecker Tests drin (am Anfang und am
**                            am Ende wird zusaetzlich Platz mit besonderen
**                            Variablen reserviert, der dann bei der Freigabe
**                            auf Ueberschreiben getestet wird. 
*/
{
    ULONG *mem_block;
    UBYTE *end_ptr;

    /* Speicher allokieren */
    if (attributes & MEMF_CLEAR) {
        mem_block = (APTR) calloc(byteSize+16,1);
    } else {
        mem_block = (APTR) malloc(byteSize+16);
        /*** FIXME ***/
        memset(mem_block, 0, byteSize+16);
    };

    if (mem_block) {
        *mem_block++ = 0xABADCAFE;
        *mem_block++ = byteSize;
        *mem_block++ = 0xABADCAFE;
        end_ptr = ((UBYTE *)mem_block) + byteSize;
        *((ULONG *)end_ptr) = 0xABADCAFE;        
        NBase.AllocCount++;
        NBase.AllocSize += byteSize;
        if (NBase.AllocSize > NBase.AllocMax) NBase.AllocMax = NBase.AllocSize;
    } else {
        nc_LogMsg("_AllocVec(%d, %d) failed (Out of mem)!\n",
                  byteSize, attributes);
    };

    return(mem_block);
};

/*-----------------------------------------------------------------*/
void nc_FreeVec(APTR memoryBlock)
/*
**  FUNCTION
**      Gibt einen mit nc_AllocVec() allokierten Speicherblock
**      frei.
**      Macht Integrity-Check, ob der Speicherblock am Anfang oder am
**      Ende ueberschrieben wurde, und loescht den gesamten Block
**      mit 0xfc. 
**
**  INPUTS
**      memoryBlock -> Pointer auf freizugebenden Block
**
**  RESULTS
**      ---
**
**  CHANGED
**      22-Dec-94   floh    created
**      26-Apr-96   floh    Debug-Variable __AllocCount
**      04-Aug-96   floh    __AllocSize, __AllocMax
**      24-Feb-97   floh    + Diagnose-Variablen jetzt in NBase eingebettet
**      07-May-98   floh    + macht jetzt Mungwall-Check, ob was ueberschrieben wurde
*/
{
    ULONG *m = ((ULONG *)memoryBlock)-3;
    LONG val1,val2,size;
    UBYTE *end_ptr;
    val1 = m[0];
    size = m[1];
    val2 = m[2];
    end_ptr = ((UBYTE *)memoryBlock)+size; 
         
    /*** Anfangsbereich checken ***/
    if ((val1!=0xABADCAFE) || (val2!=0xABADCAFE)) {
        _LogMsg("########\nRED ALERT! RED ALERT! RED ALERT!\nIntegrity Check (START of mem block) failed in nc_FreeMem()\n########\n");
    };
    if (*((ULONG *)end_ptr)!=0xABADCAFE) {     
        _LogMsg("########\nRED ALERT! RED ALERT! RED ALERT!\nIntegrity Check (END of mem block) failed in nc_FreeMem()\n########\n");
    };
    NBase.AllocCount--;
    NBase.AllocSize -= size;
    memset(m,0xfc,size+16);
    free(m);
};

