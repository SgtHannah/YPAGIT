/*
**  $Source: PRG:VFM/Classes/_ILBMClass/getfile.c,v $
**  $Revision: 38.7 $
**  $Date: 1998/01/06 14:51:10 $
**  $Locker: floh $
**  $Author: floh $
**
**  Routinen zum Laden eines ILBM- oder VBMP-Files.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/iffparse.h>

#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"

#include "bitmap/ilbmclass.h"
#include "bitmap/displayclass.h"

_extern_use_nucleus

/*** aus ilbmclass.c ***/
extern struct NucleusBase *ilbm_NBase;

/*-----------------------------------------------------------------*/
struct IFFHandle *ilbm_OpenIff(UBYTE *name, ULONG how)
/*
**  FUNCTION
**      Öffnet den übergebenen Filename als IFF-Stream
**      und returniert bei Erfolg einen IFFHandle.
**
**  INPUTS
**      name    - Filename, relativ zu MC2resources: !!!
**      how     - IFFF_READ oder IFFF_WRITE
**
**  RESULTS
**      Pointer auf offenen IFFStream.
**
**  CHANGED
**      11-Jan-96   floh    created
*/
{
    static UBYTE filename[256];
    UBYTE *path_prefix;
    LONG error;
    struct IFFHandle *iff;
    UBYTE *dos_mode;

    if (how == IFFF_READ) dos_mode = "rb";
    else                  dos_mode = "wb";

    /* vollen Filenamen zusammenbasteln */
    path_prefix = _GetSysPath(SYSPATH_RESOURCES);
    strcpy(filename, path_prefix);
    strcat(filename, name);

    /* und los geht's */
    if (iff = _AllocIFF()) {

        iff->iff_Stream = (ULONG) _FOpen(filename, dos_mode);
        if (iff->iff_Stream) {

            _InitIFFasNucleus(iff);

            error = _OpenIFF(iff, how);
            if (error == 0) {

                /*** Success!!! ***/
                return(iff);
            };
            _FClose((APTR)iff->iff_Stream);
        };
        _FreeIFF(iff);
    };

    /*** Fehler ***/
    return(NULL);
}

/*-----------------------------------------------------------------*/
void ilbm_CloseIff(struct IFFHandle *iff)
/*
**  FUNCTION
**      Schließt einen mit ilbm_OpenIff() geöffneten
**      IFF-Stream wieder. ilbm_CloseIff() darf ***nur***
**      aufgerufen werden, wenn ilbm_OpenIff() erfolgreich
**      war!
**
**  INPUTS
**      iff - Ptr auf IFFHandle
**
**  RESULTS
**      ---
**
**  CHANGED
**      11-Jan-96   floh    created
*/
{
    if (iff) {
        _CloseIFF(iff);
        _FClose((APTR)iff->iff_Stream);
        _FreeIFF(iff);
    };
}

/*-----------------------------------------------------------------*/
void ilbm_DecodeILBMBody(BitMapHeader *BMHD, UBYTE *body_chunk, UBYTE *bmp_data)
/*
**  FUNCTION
**      Decodiert den ILBM <body_chunk> in eine Chunky8-Pixelmap (<bmp_data>).
**
**  INPUTS
**      BMHD    -> Pointer auf bereits eingelesene BitMapHeader-Struktur
**      body_chunk  -> Pointer auf (evtl. runtime-codierten) ILBM BODY
**      bmp_data    -> Pointer auf bereits reservierten Ziel-Chunky8-Buffer
**
**  CHANGED
**      03-Jan-95   floh    created
**      08-Aug-95   floh    Fuck!!! Bei der Berechnung von
**                          nNoBytersPerRow habe ich wohl die Höhe
**                          mit der Breite vertauscht... Erstaunlich,
**                          daß das bisher noch nicht aufgefallen ist (????).
**      11-Jan-96   floh    revised & updated
*/
{
    static UBYTE nMask[8] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
    };
    UBYTE nMask2;   /* Hilfsmaske für Bitselektion */
    ULONG i,j,nCol; /* Hilfszähler */
    LONG nPixelRow = 0;
    UBYTE nSteuer;  /* Steuerbyte aus BODY */
    UBYTE nPixel;   /* Farbinformation aus BODY */
    LONG nNoBytesPerRow;    /* Anzahl Bytes pro Zeile */
    UBYTE ILBMBuf[8][128];  /* Speicherblock für eine Bildzeile */

    nNoBytesPerRow = (BMHD->w+7)/8;          /* aufrunden */
    nNoBytesPerRow+= (nNoBytesPerRow%2);    /* gerade machen */

    while (nPixelRow < BMHD->h) {

        if (BMHD->compression == 0) {
            /* BODY nicht komprimiert */
            for (i=0; i<BMHD->nPlanes; i++) {
                memcpy(&ILBMBuf[i][0], body_chunk, nNoBytesPerRow);
                body_chunk += nNoBytesPerRow;
            };
        } else {
            /* BODY runtime encoded */
            for (i=0; i<BMHD->nPlanes; i++) {

                nCol = 0;
                while (nCol < nNoBytesPerRow) {

                    nSteuer = *body_chunk++;    /* Steuerbyte lesen */
                    if (nSteuer < 0x80) {       /* nSteuer+1 Bytes lesen */
                        memcpy(&ILBMBuf[i][nCol], body_chunk, ++nSteuer);
                        body_chunk += nSteuer;
                        nCol       += nSteuer;

                    } else if (nSteuer > 0x80) {    /* Runtime-Code! */

                        /* das nächste Byte ist 0x100-nSteuer+1 mal */
                        /* nach Output zu kopieren */
                        nSteuer = ~nSteuer+2;
                        nPixel  = *body_chunk++;
                        for (j=0; j<nSteuer; j++) 
                            ILBMBuf[i][nCol++]=nPixel;
                    };
                }; /* while */
            }; /* for */
        }; /* else */

        /*** PLANAR-2-CHUNKY-KONVERTIERUNG (für 1 Zeile) ***/
        for (i=0; i<BMHD->w; i++) {

            nMask2 = nMask[7-(i%8)];
            nCol   = i/8;
            nPixel = 0;

            for (j=0; j<BMHD->nPlanes; j++) {
                if ((ILBMBuf[j][nCol]&nMask2)!=0)  nPixel |= nMask[j];
            };

            /* Chunky-Pixel nach Chunky8-Pixelmap */
            *bmp_data++ = nPixel;
        };

        /* nächste Zeile */
        nPixelRow++;

    };  /* while */

    /*** ENDE ***/
}

/*-----------------------------------------------------------------*/
BOOL ilbm_GetILBMBody(struct IFFHandle *iff,
                      BitMapHeader *BMHD,
                      struct VFMBitmap *bmp)
/*
**  FUNCTION
**      Liest den BODY-Chunk des übergebenen IFF-ILBM Files
**      in das Pixel-Array der ebenfalls übergebenen VFMBitmap.
**
**  INPUTS
**      iff     - IFF-Stream, gestoppt auf ILBM BODY
**      BMHD    - der bereits eingelesene BMHD-Chunk
**      bmp     - richtig dimensionierte VFMBitmap (insbesondere
**                Width, Height und Data)
**
**  RESULTS
**      TRUE    - alles OK
**      FALSE   - ernsthafter Fehler
**
**  CHANGED
**      11-Jan-96   floh    teilweise übernommen aus getilbm.c
**                          der "alten" ilbm.class
*/
{
    BOOL retval = FALSE;

    if (bmp->Data) {

        struct ContextNode *cn;
        UBYTE *body_chunk;

        cn = _CurrentChunk(iff);

        /*** temporären Body-Chunk-Buffer allokieren ***/
        body_chunk = (UBYTE *) _AllocVec(cn->cn_Size, MEMF_PUBLIC);
        if (body_chunk) {

            /*** Body-Chunk komplett einlesen... ***/
            _ReadChunkBytes(iff, body_chunk, cn->cn_Size);

            /*** ... und decodieren ***/
            ilbm_DecodeILBMBody(BMHD, body_chunk, bmp->Data);

            /*** temp. Buffer freigeben ***/
            _FreeVec(body_chunk);
            retval = TRUE;
        };
    };

    return(retval);
}

/*-----------------------------------------------------------------*/
BOOL ilbm_GetVBMPBody(struct IFFHandle *iff,
                      struct VBMP_Header *VBMP,
                      struct VFMBitmap *bmp)
/*
**  FUNCTION
**      Liest den BODY-Chunk des übergebenen VBMP Files
**      in das Pixel-Array der ebenfalls übergebenen VFMBitmap.
**      Der VBMP File ***MUSS*** zur Zeit im Chunky8-Format vorliegen!
**
**  INPUTS
**      iff     - IFF-Stream, gestoppt auf ILBM BODY
**      VBMP    - der bereits eingelesene VBMP-Chunk
**      bmp     - richtig dimensionierte VFMBitmap (insbesondere
**                Width, Height und Data)
**
**  RESULTS
**      TRUE    - alles OK
**      FALSE   - ernsthafter Fehler
**
**  CHANGED
**      11-Jan-96   floh    created
**      19-Mar-97   floh    Größe des Puffers wurde mit
**                          BytesPerRow bestimmt, richtiger
**                          ist aber die Breite in Bytes.
*/
{
    if (bmp->Data) {

        /*** die Sache ist ausgesprochen simple... ***/
        _ReadChunkBytes(iff, bmp->Data, bmp->Width * bmp->Height);
        return(TRUE);

    } else return(FALSE);
}

/*-----------------------------------------------------------------*/
struct RsrcNode *ilbm_CreateBitmap(Object *o, Class *cl,
                                   struct TagItem *tlist,
                                   struct IFFHandle *iff,
                                   BOOL is_trapped)
/*
**  FUNCTION
**      Die Routine macht aus einem ILBM- oder VBMP-File
**      eine Resource-Node mit eingebetteter VFMBitmap-
**      Struktur. Die Routine ***MUSS*** aus dem RSM_CREATE
**      Handler aufgerufen werden, weil sie selbständig
**      RSM_CREATE der Superklasse ausführt!!!
**
**          - auswerten, ob es ein ILBM- oder VBMP-File
**            ist
**          - Breite, Höhe und Bitmap-Typ aus dem File
**            ermitteln
**          - mit diesen Informationen RSM_CREATE der
**            Superklasse (bitmap.class) invoken, woraufhin
**            eine VFMBitmap der benötigten Größe
**            zur Verfügung steht
**          - Diese Bitmap mit dem Bitmap-Body aus dem
**            File ausfüllen.
**          - die von _supermethoda() erzeugte RsrcNode
**            wird returniert.
**
**  INPUTS
**      o       - wie bei RSM_CREATE reingekommen
**      cl      - wie bei RSM_CREATE reingekommen
**      tlist   - Message von RSM_CREATE
**      iff     - offener IFFHandle, gestoppt auf
**                äußerstem FORM des zu lesenden Files
**      is_trapped - TRUE, wenn es sich um eine getrappte
**                   Alpha-Map handelte
**
**  RESULTS
**      Eine vollständige RsrcNode, oder NULL bei einem Fehler.
**
**  CHANGED
**      11-Jan-96   floh    created
**      30-May-96   floh    veränderte Vorgehensweise, weil jetzt
**                          auch CMAP akzeptiert wird (ein BODY
**                          muß dann nicht mehr zwingend vorhanden
**                          sein).
**      06-Dec-96   floh    falls CMAP-Chunk vorhanden, aber noch
**                          keine Colormap, wird diese so oder so
**                          allokiert und ausgefüllt
**      25-Feb-97   floh    + Handling für VBF_Texture Bitmaps
**                            Das Display-Treiber-Objekt (so existent)
**                            erhält Gelegenheit, die Bitmap-Daten
**                            nach Belieben zu konvertieren. Das passiert
**                            im BODY Chunk Block.
**                          + Falls CMAP nachträglich allokiert werden
**                            muß, wird das VBF_Texture Flag gesetzt
**      21-Mar-97   floh    + Texturen, welche mit "FX" anfangen, werden
**                            mit einem Paletten-Slot statt der lokalen
**                            Farbpalette "overridden" (wird nur ausgewertet
**                            bei Hi/Truecolor-Display-Treibern).
**      04-Mar-98   floh    + is_trapped Arg
*/
{
    LONG error;
    struct ContextNode *cn;

    BOOL is_ilbm;
    struct RsrcNode *rnode = NULL;
    struct VFMBitmap *bmp  = NULL;

    BitMapHeader BMHD;
    struct VBMP_Header VBMP;
    struct TagItem add_tags[4];

    /*** zuerst entscheiden, ob ILBM, VBMP, oder was anderes ***/
    error = _ParseIFF(iff, IFFPARSE_RAWSTEP);
    if (0 == error) {
        cn = _CurrentChunk(iff);
        if (ID_FORM == cn->cn_ID) {

            if (ILBM_ID == cn->cn_Type)           is_ilbm = TRUE;
            else if (VBMP_FORM_ID == cn->cn_Type) is_ilbm = FALSE;

            else {
                _LogMsg("ilbm.class: Not an ILBM or VBMP file!\n");
                return(NULL);
            };
        } else {
            _LogMsg("ilbm.class: Not an IFF FORM chunk!\n");
            return(NULL);
        };
    } else {
        _LogMsg("ilbm.class: Not an IFF file!\n");
        return(NULL);
    };


    /*** IFF-File weiterparsen... ***/
    while ((error = _ParseIFF(iff, IFFPARSE_RAWSTEP)) != IFFERR_EOC) {

        /*** andere Fehler als EndOfChunk abfangen ***/
        if (error) {
            /*** falls RsrcNode bereits erzeugt, diese killen ***/
            if (rnode) _method(o, RSM_FREE, (ULONG) rnode);
            return(NULL);
        };

        /*** ContextNode holen ***/
        cn = _CurrentChunk(iff);

        /*** ILBM-Header-Chunk? ***/
        if (cn->cn_ID == BMHD_ID) {

            /* reinladen */
            _ReadChunkBytes(iff, &BMHD, sizeof(BMHD));

            /* Endian-Anpassung */
            v2nw(&(BMHD.w)); v2nw(&(BMHD.h));

            /*** VFMBitmap erzeugen ***/
            add_tags[0].ti_Tag  = BMA_Width;
            add_tags[0].ti_Data = BMHD.w;
            add_tags[1].ti_Tag  = BMA_Height;
            add_tags[1].ti_Data = BMHD.h;
            add_tags[2].ti_Tag  = TAG_MORE;
            add_tags[2].ti_Data = (ULONG) tlist;

            rnode = (struct RsrcNode *) _supermethoda(cl,o,RSM_CREATE,add_tags);
            if (rnode) {
                bmp = (struct VFMBitmap *) rnode->Handle;
                if (!bmp) {
                    _method(o, RSM_FREE, (ULONG) rnode);
                    return(NULL);
                };
            };

            /* EOC */
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** VBMP-Header-Chunk ***/
        if (cn->cn_ID == VBMP_HEADER_ID) {

            /* reinladen */
            _ReadChunkBytes(iff, &VBMP, sizeof(VBMP));

            /* Endian-Anpassung */
            v2nw(&(VBMP.w)); v2nw(&(VBMP.h));

            /*** VFMBitmap erzeugen ***/
            add_tags[0].ti_Tag  = BMA_Width;
            add_tags[0].ti_Data = VBMP.w;
            add_tags[1].ti_Tag  = BMA_Height;
            add_tags[1].ti_Data = VBMP.h;
            add_tags[2].ti_Tag  = TAG_MORE;
            add_tags[2].ti_Data = (ULONG) tlist;

            rnode = (struct RsrcNode *) _supermethoda(cl,o,RSM_CREATE,add_tags);
            if (rnode) {
                bmp = (struct VFMBitmap *) rnode->Handle;
                if (!bmp) {
                    _method(o, RSM_FREE, (ULONG) rnode);
                    return(NULL);
                };
            };

            /* EOC */
            _ParseIFF(iff, IFFPARSE_RAWSTEP);
            continue;
        };

        /*** CMAP-Chunk ***/
        if (cn->cn_ID == CMAP_ID) {
            /*** identisch für ILBM und VBMP! ***/
            if (bmp) {
                /*** keine Colormap -> eine allokieren ***/
                if (NULL == bmp->ColorMap) {
                    bmp->ColorMap = (UBYTE *) _AllocVec(256*3,MEMF_PUBLIC|MEMF_CLEAR);
                };
                /*** ... und ausfüllen ***/
                if (bmp->ColorMap) {
                    _ReadChunkBytes(iff,bmp->ColorMap,(256*3));
                    bmp->Flags |= VBF_HasColorMap;
                };
            };
            /* EOC */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /*** BODY-Chunk (identisch für ILBM und VBMP) ***/
        if (cn->cn_ID == BODY_ID) {

            BOOL success = FALSE;

            if (bmp) {

                Object *gfxo    = ilbm_NBase->GfxObject;
                BOOL do_body    = TRUE;
                BOOL is_texture = (gfxo && (bmp->Flags & VBF_Texture));

                /*** Textur? Dann muß gelockt werden! ***/
                if (is_texture) {
                    struct disp_texture dt;
                    dt.texture = bmp;
                    if (!_methoda(gfxo,DISPM_LockTexture,&dt)) do_body=FALSE;
                };

                if (do_body) {

                    /*** Body nach <bmp->Data> laden ***/
                    if (is_ilbm) success = ilbm_GetILBMBody(iff, &BMHD, bmp);
                    else         success = ilbm_GetVBMPBody(iff, &VBMP, bmp);

                    /*** Textur? Dann konvertieren lassen und unlocken ***/
                    if (is_texture) {
                        struct disp_texture dt;
                        dt.texture       = bmp;
                        _methoda(gfxo,DISPM_UnlockTexture,&dt);
                        if (is_trapped) bmp->Flags |= VBF_AlphaHint;
                        _methoda(gfxo,DISPM_MangleTexture,&dt);
                    };
                };
            };

            if (!success) {
                _method(o, RSM_FREE, (ULONG) rnode);
                return(NULL);
            };

            /* EOC */
            _ParseIFF(iff,IFFPARSE_RAWSTEP);
            continue;
        };

        /* ungewollte Chunks überspringen */
        _SkipChunk(iff);
    };

    /*** Ende (rnode KANN 0 sein!) ***/
    return(rnode);
}

