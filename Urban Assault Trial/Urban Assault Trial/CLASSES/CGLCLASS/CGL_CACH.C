/*
**  $Source: PRG:VFM/Classes/_CglClass/cgl_cache.c,v $
**  $Revision: 38.2 $
**  $Date: 1997/02/26 17:21:18 $
**  $Locker: floh $
**  $Author: floh $
**
**  Textur-Cache-Manager für cgl.class.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap/cglclass.h"

/*** Inline-Assembler für Texture-Download ***/
extern void _download(UBYTE *from, UWORD far *to, UWORD *table, ULONG size);
#pragma aux _download parm [esi][gs edi][edx][ecx] = \
            "xor eax,eax"        \
            "add edi,ecx"        \
            "add edi,ecx"        \
            "add esi,ecx"        \
            "neg ecx"            \
      "cycl: mov al,[esi+ecx]"   \
            "mov bx,[edx+eax*2]" \
            "mov gs:[edi+ecx*2],bx" \
            "inc ecx" \
            "jne cycl" \
            modify [eax ebx ecx edi esi] ;

/*-----------------------------------------------------------------*/
BOOL cgl_InitTxtCache(struct cgl_data *cd, ULONG txtbuf_size)
/*
**  FUNCTION
**      Initialisiert den Texture-Cache-Manager.
**
**  INPUTS
**      cd          - LID
**      txtbuf_size - Speicherplatz für Texturen im RAM der
**                    3D-Blaster (wird von cglInitScreen
**                    zurückgegeben).
**
**  RESULTS
**      TRUE    - alles OK
**      FALSE   - irgendwas ging barbarisch schief
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    BOOL retval = FALSE;
    ULONG num_slots;
    ULONG txtsize = 256*256*2;

    /*** Anzahl Textur-Slots (256x256xRGB5551) ***/
    cd->num_slots = (txtbuf_size / txtsize) - 1;
    if (cd->num_slots > 0) {

        // FIXME
        _LogMsg("Num txt slots: %d.\n",cd->num_slots);

        cd->slot = (struct TextureSlot *)
                   _AllocVec(cd->num_slots * sizeof(struct TextureSlot),
                   MEMF_PUBLIC|MEMF_CLEAR);
        if (cd->slot) {

            ULONG i;
            CGL_TEXTURELD_ST cgl_txt;
            ULONG txt_start;

            /*** interne Start-Adresse des Txt-Buffers... ***/
            memset(&cgl_txt,0,sizeof(cgl_txt));
            cgl_txt.wColorFormat = CGL_RGB5551;
            cgl_txt.pHostBuffer  = &cgl_txt;
            cglLoadTextureMap(&cgl_txt);
            txt_start = cgl_txt.dwTextureAddress;

            for (i=0; i<cd->num_slots; i++) {
                cd->slot[i].flags      = CGLF_FLUSHME;
                cd->slot[i].cache_hits = 0;
                cd->slot[i].offset     = i*(txtsize>>1);
                cd->slot[i].bmp        = NULL;
                cd->slot[i].txt_handle = txt_start + i*txtsize;
            };

            retval = TRUE;
        };
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void cgl_KillTxtCache(struct cgl_data *cd)
/*
**  FUNCTION
**      Killt den Textur-Cache-Manager.
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    if (cd->slot) {
        _FreeVec(cd->slot);
        cd->slot = NULL;
        cd->num_slots = 0;
    };
}

/*-----------------------------------------------------------------*/
BOOL cgl_LoadTexture(struct cgl_data *cd, struct VFMBitmap *bmp, ULONG slot)
/*
**  FUNCTION
**      Schreibt die in <bmp> definierte Textur
**      in per <slot> definierten Textur-Cache-Slot.
**      Der Slot muß frei oder geflusht sein. Die
**      Routine konvertiert die Textur vom 8-Bit-Indexed
**      Format nach RGB5551, für das Remapping wird die
**      globale Tabelle <cd->rgb5551_table> benutzt. Die Textur
**      muß exakt 256 Pixel breit und maximal 256 Pixel hoch sein.
**      Der TextureSlot WIRD NICHT MODIFIZIERT!!!
**
**          slot.flags      = 0
**          slot.cache_hits = 0
**          slot.bmp        = bmp
**
**  INPUTS
**      cd      - LID des cgl.class Objects
**      bmp     - VFMBitmap mit Textur
**      slot    - Nummer des Slots, der beschrieben werden soll
**
**  CHANGED
**      18-Aug-96   floh    created
**      19-Aug-96   floh    Peephole-Optimizing
**      28-Nov-96   floh    jetzt mit Inline-Assembler-Funktion
*/
{
    CGL_TEXTURELD_ST cgl_txt;
    cgl_txt.dwFreeByteAddress = cd->slot[slot].offset<<1;
    cgl_txt.wBufferWidth      = 256;
    cgl_txt.wWidth            = 256;
    cgl_txt.wHeight           = 256;
    cgl_txt.wColorFormat      = CGL_RGB5551;
    cgl_txt.pHostBuffer       = bmp->Data;
    cglLoadTextureMap(&cgl_txt);
    return(TRUE);

    //---------------------------------------------------------------
    // UWORD far *to;
    // BOOL retval = FALSE;
    //
    // if (cglLockBuffer(CGL_TEXTURE_BUFFER,&to) == CGL_SUCCESS) {
    //
    //     UBYTE *from  = bmp->Data;
    //     UWORD *table = &(cd->rgb5551_table);
    //     ULONG size   = bmp->Width*bmp->Height;
    //
    //     to += cd->slot[slot].offset;
    //     _download(from,to,table,size);
    //     retval = TRUE;
    // };
    // return(retval);
}

/*-----------------------------------------------------------------*/
void cgl_TxtCacheBeginFrame(struct cgl_data *cd)
/*
**  FUNCTION
**      Löscht das Used-Flag bei allen Texturen im
**      Textur-Cache. Das Used-Flag wird gesetzt,
**      sobald im aktuellen Frame der Textur-Slot benötigt
**      wird (touch).
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    ULONG i;
    for (i=0; i<cd->num_slots; i++) cd->slot[i].flags &= ~CGLF_USED;
}

/*-----------------------------------------------------------------*/
void cgl_TxtCacheEndFrame(struct cgl_data *cd)
/*
**  FUNCTION
**      Alle Texturen im Cache, die in diesem Frame
**      nicht benötigt wurden, werden als CGLF_FLUSHME
**      markiert (zum Überschreiben im nächsten Frame
**      freigegeben.
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    ULONG i;
    for (i=0; i<cd->num_slots; i++) {
        if (!(cd->slot[i].flags&CGLF_USED)) cd->slot[i].flags|=CGLF_FLUSHME;
    };
}

/*-----------------------------------------------------------------*/
ULONG cgl_ValidateTexture(struct cgl_data *cd, struct VFMBitmap *bmp, BOOL force)
/*
**  FUNCTION
**      Kernstück des Texture-Managers. Stellt eine
**      CGL-interne Textur-Adresse bereit, wie sie
**      zum Rendern benötigt wird. Zuerst wird
**      geguckt, ob die Textur bereits im Cache ist.
**      Wenn ja, kehrt die Routine sofort erfolgreich
**      zurück.
**
**      Ist die Textur nicht im Cache, aber ein Slot
**      frei, wird die VFMBitmap in den Textur-Cache
**      downgeloadet (dabei konvertiert) und
**      die Routine kommt ebenfalls erfolgreich zurück.
**
**      Ansonsten (Cachemiss bei vollem Cache) gibt
**      es zwei Modi:
**      (1) Im Force-Mode (force == TRUE) wird die
**          in letzter Zeit am wenigsten benötigte
**          Textur zwangsgeflusht, und die neue Textur
**          in den freigewordenen Slot geladen.
**      (2) Im Delay-Mode (force == FALSE) kehrt
**          die Routine mit einem NULL-Pointer zurück,
**          der Cache bleibt unverändert. Für den
**          Polygon-Renderer ist das das Zeichnen,
**          den Polygon an die Delay-Liste zu hängen
**          (die Delay-Liste wird bei DISPM_End abgearbeitet,
**          dabei wird sichergestellt, daß keine Textur
**          mehr als einmal pro Frame in den Cache geladen
**          werden muß).
**
**  INPUTS
**      cd      - LID des cgl.class Objects
**      bmp     - VFMBitmap Pointer der benötigten Textur
**      force   - TRUE -> Force-Modus an, sonst Delay-Modus an
**
**  RESULTS
**      <ULONG> ist das CGL-spezifische Textur-Handle,
**      oder NULL, wenn Delay-Modus und CacheMiss.
**
**  CHANGED
**      18-Aug-96   floh    created
*/
{
    ULONG i;
    LONG free = -1;

    for (i=0; i<cd->num_slots; i++) {

        struct TextureSlot *s = &(cd->slot[i]);

        if (s->bmp == bmp) {

            /*** Textur ist schon im Cache ***/
            s->flags &= ~CGLF_FLUSHME; // FlushMe löschen
            s->flags |= CGLF_USED;     // Used setzen
            s->cache_hits++;
            return(s->txt_handle);

        } else if (s->flags & CGLF_FLUSHME) {
            /*** weil gerade dabei, einen leeren Slot merken ***/
            free = i;
        };
    };

    /*** ab hier CacheMiss, war ein Slot frei? ***/
    if (free != -1) {

        /*** yo, dorthin downloaden ***/
        if (cgl_LoadTexture(cd,bmp,free)) {

            /*** TextureSlot initialisieren ***/
            struct TextureSlot *s = &(cd->slot[free]);
            s->flags     &= ~CGLF_FLUSHME;
            s->flags     |= CGLF_USED;
            s->cache_hits = 1;
            s->bmp        = bmp;
            return(s->txt_handle);
        };

    } else {

        /*** kritisch, CacheMiss bei vollem Cache ***/
        if (force) {

            /*** suche Slot mit niedrigstem UsedCount ***/
            ULONG min_slot = 0;
            for (i=0; i<cd->num_slots; i++) {
                if (cd->slot[i].cache_hits < cd->slot[min_slot].cache_hits) {
                    min_slot = i;
                };
            };

            /*** diesen Slot zwangsflushen ***/
            if (cgl_LoadTexture(cd,bmp,min_slot)) {

                /*** TextureSlot initialisieren ***/
                struct TextureSlot *s = &(cd->slot[min_slot]);
                s->flags     &= ~CGLF_FLUSHME;
                s->flags     |= CGLF_USED;
                s->bmp        = bmp;
                s->cache_hits = 1;
                return(s->txt_handle);
            };
        };
    };

    /*** ab hier Delay ***/
    return(NULL);
}

