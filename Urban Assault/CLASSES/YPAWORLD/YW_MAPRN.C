/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_maprnd.c,v $
**  $Revision: 38.22 $
**  $Date: 1998/01/06 16:23:49 $
**  $Locker:  $
**  $Author: floh $
**
**  Ausgelagertes Map-Rendering, damit yw_mapreq.c nicht
**  überläuft.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"
#include "ypa/guimap.h"
#include "ypa/ypavehicles.h"

#include "yw_protos.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine

extern struct YPAMapReq MR;     // aus yw_mapreq.c

#ifdef AMIGA
extern __far UBYTE MR_ClipRealMap[];  // Char-Buffer für Map-Interior
#else
extern UBYTE MR_ClipRealMap[];  // Char-Buffer für Map-Interior
#endif

/*-----------------------------------------------------------------*/
UWORD yw_GetLandLego(struct ypaworld_data *ywd, LONG subx, LONG suby)
/*
**  FUNCTION
**      Ermittelt anhand den Subkoordinaten subx und suby
**      das richtige Font-Char für einen Lego-Modus. Das
**      Char wird in den Clip reingeschrieben, bei Bedarf
**      wird die richtige Font-Nummer vorher eingestellt.
**
**  INPUTS
**      ywd       - Pointer auf LID des Welt-Objekts
**      subx      - Sub-X-Koordinate (SubSector-Auflösung)
**      suby      - Sub-Y-Koordinate (SubSector-Auflösung)
**
**  RESULTS
**      (UBYTE) res      = Char-Num
**      (UBYTE) (res>>8) = FontPage-Nummer (0..3)
**
**  CHANGED
**      06-Nov-95   floh    created
**      21-Mar-96   floh    beachtet jetzt Sektor-Footprint und
**                          gibt bei Mißerfolg 0 als char zurück.
**      26-Jun-96   floh    Map-Revision: Test auf "außerhalb Welt",
**                          wird als (CharNum==0) returniert
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      07-Dec-96   floh    + Support für Höhenlinien.
**      12-Dec-96   floh    + bei Slurp-Auswahl wurde links ein
**                            Sektor zu weit nach innen geclippt
**      17-Jun-98   floh    + plus "which side is higher" Handling
*/
{
    UBYTE chr;
    UBYTE fnum;
    LONG secx = subx >> 2;
    LONG secy = suby >> 2;
    LONG inx  = subx & 3;
    LONG iny  = suby & 3;
    struct Cell *sec;

    /*** Clipping ***/
    if ((secx<0)||(secx>=ywd->MapSizeX)||(secy<0)||(secy>=ywd->MapSizeY)) {
        return(0);
    };

    sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);
    if (sec->FootPrint & MR.footprint) {

        if ((inx == 0) || (iny == 0)) {

            /*** ein Slurp (evtl. mit Höhenlinien) ***/
            if ((secx<1)||(secy<1)) {

                /*** Clipping am linken Map-Rand ***/
                fnum = 0;
                chr  = 1;

            } else {

                /*** Höhen-Linien-Check ist sicher ***/
                struct Cell *n[4];  // Nachbar-Sektoren
                LONG  code = 0;
                FLOAT diff = 500;   // ab hier schaffens Landfahrzeuge nicht

                /*-------------------------------------------------**
                **        |                                        **
                **   n[0] | n[1]                                   **
                **        |                                        **
                **  ------+------                                  **
                **        |                                        **
                **   n[2] | n[3]                                   **
                **        |                                        **
                **                                                 **
                **  Lagecode:                                      **
                **      (1<<0) -- n[0] <-> n[1]                    **
                **      (1<<1) -- n[2] <-> n[3]                    **
                **      (1<<2) -- n[0] <-> n[2]                    **
                **      (1<<3) -- n[1] <-> n[3]                    **
                **                                                 **
                **-------------------------------------------------*/

                if (inx != 0) {
                
                    /*** ein waagerechter Slurp ***/
                    n[0] = n[1] = sec - ywd->MapSizeX;
                    n[2] = n[3] = sec;
                    
                } else if (iny != 0) {
                
                    /*** ein vertikaler Slurp ***/
                    n[0] = n[2] = sec - 1;
                    n[1] = n[3] = sec;
                    
                } else {
                    /*** ein Cross-Slurp ***/
                    n[0] = sec - ywd->MapSizeX - 1;
                    n[1] = sec - ywd->MapSizeX;
                    n[2] = sec - 1;
                    n[3] = sec;
                };

                /*** Lagecode bauen ***/
                if (abs(n[0]->Height - n[1]->Height) >= diff) code |= (1<<0);
                if (abs(n[2]->Height - n[3]->Height) >= diff) code |= (1<<1);
                if (abs(n[0]->Height - n[2]->Height) >= diff) code |= (1<<2);
                if (abs(n[1]->Height - n[3]->Height) >= diff) code |= (1<<3);

                if (code == ((1<<2)|(1<<3))) {
                    if (n[0]->Height > n[2]->Height) code = -4;
                    else                             code = -3;
                } else if (code == ((1<<0)|(1<<1))) { 
                    if (n[0]->Height > n[1]->Height) code = -2;
                    else                             code = -1;
                };

                /*** char ermitteln (die letzten 16 sind reserviert ***/
                fnum = 0;
                chr  = 240 + code;
            };

        } else {

            /*** ein 3x3- oder ein Kompakt-Sektor ***/
            if (sec->SType != SECTYPE_COMPACT) {

                /*** ein 3x3 Sektor ***/
                LONG sx = inx - 1;
                LONG sy = 2 - (iny-1);
                LONG legonum = GET_LEGONUM(ywd,sec,sx,sy);
                chr  = ywd->Legos[legonum].chr;
                fnum = ywd->Legos[legonum].page;

            } else {

                /*** ein Kompakt-Sektor ***/
                LONG legonum = GET_LEGONUM(ywd,sec,0,0);
                chr  = ywd->Legos[legonum].chr;
                chr += ((iny-1)<<4) + (inx-1);
                fnum = ywd->Legos[legonum].page;
            };
        };

    } else {
        fnum = 0;
        chr  = 0;   // Sektor ist nicht sichtbar
    };

    return((UWORD) ((fnum<<8)|(chr)));
}

/*-----------------------------------------------------------------*/
UWORD yw_GetLandSector(struct ypaworld_data *ywd, LONG secx, LONG secy)
/*
**  FUNCTION
**      Funktioniert wie alle anderen Map-Supplier auch,
**      returniert das "All-In-One-Char" für einen einzelnen
**      Sektor. Die Font-Num ist immer auf 0 hardgecodet.
**
**  CHANGED
**      15-Nov-95   floh    created
**      05-Feb-96   floh    revised & updated
**      21-Mar-96   floh    beachtet jetzt Sektor-Footprint und
**                          gibt bei Mißerfolg 0 als char zurück.
**      26-Jun-96   floh    Map-Revision: Test auf "außerhalb Welt",
**                          wird als (CharNum==0) returniert
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    struct Cell *sec;
    UBYTE chr;

    /*** Clipping ***/
    if ((secx<0)||(secx>=ywd->MapSizeX)||(secy<0)||(secy>=ywd->MapSizeY)) {
        return(0);
    };
    sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);
    if (sec->FootPrint & MR.footprint) {
        chr = ywd->Sectors[sec->Type].Chr;
    } else {
        chr = 0;
    };

    return((UWORD)chr);
}

/*-----------------------------------------------------------------*/
UWORD yw_GetOwnerSector(struct ypaworld_data *ywd, LONG secx, LONG secy)
/*
**  FUNCTION
**      Returniert Owner des entsprechenden Sektors.
**      Die Font-Page-Nummer ist immer auf 0 hardgecodet.
**
**  CHANGED
**      05-Feb-96   floh    created
**      21-Mar-96   floh    beachtet jetzt Sektor-Footprint und
**                          gibt bei Mißerfolg 0 als char zurück.
**      23-May-96   floh    Im Design-Modus wird der Footprint
**                          ignoriert.
**      26-Jun-96   floh    Map-Revision: Test auf "außerhalb Welt",
**                          wird als (CharNum==0) returniert
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    struct Cell *sec;
    UBYTE chr;

    /*** Clipping ***/
    if ((secx<0)||(secx>=ywd->MapSizeX)||(secy<0)||(secy>=ywd->MapSizeY)) {
        return(0);
    };
    sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);
    if (sec->FootPrint & MR.footprint) {
        chr = sec->Owner;
        if (chr == 0) chr=8;
    } else {
        chr = 0;
    };

    return((UWORD)chr);
}

/*-----------------------------------------------------------------*/
UWORD yw_GetBGPattern(struct ypaworld_data *ywd, LONG secx, LONG secy)
/*
**  FUNCTION
**      Stellt die "Füllstücke" für den Map-Background-Layer
**      bereit, der alles ausfüllt, was sonst "leer" wäre.
**      Zurückgegeben wird 0, oder Char #9 in
**      FONTID_OWNER_4..32.
**
**  CHANGED
**      07-Sep-96   floh    created
*/
{
    struct Cell *sec;
    UBYTE chr;

    /*** Clipping ***/
    if ((secx<0)||(secx>=ywd->MapSizeX)||(secy<0)||(secy>=ywd->MapSizeY)) {
        return(9);
    };
    sec = &(ywd->CellArea[secy * ywd->MapSizeX + secx]);

    /*** ist einer der Layer angeschaltet? ***/
    if (MR.layers & (MAP_LAYER_LANDSCAPE|MAP_LAYER_OWNER)) {
        /*** Footprint beachten! ***/
        if (sec->FootPrint & MR.footprint) chr=0;   // leer
        else                               chr=9;
    } else chr=9;   // Hintergrund voll malen

    return((UWORD)chr);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_PutNewFont(UBYTE *clip, UBYTE fid)
/*
**  FUNCTION
**      Schreibt ein NEW_FONT in den Byte-Strom und returiert 
**      den neuen Clip-Pointer.
**
**  INPUTS
**      clip    - Pointer auf Byte-Strom
**      fid     - Font-ID
**
**  RESULTS
**      Modifizierter Pointer auf auf Byte-Strom
**
**  CHANGED
**      07-Nov-95   floh    created
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    new_font(clip,fid);
    return(clip);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_RMapCore(struct ypaworld_data *ywd,
                   LONG size,
                   UBYTE *clip,
                   LONG ps_x, LONG ps_y,
                   LONG pe_x, LONG pe_y,
                   UBYTE font_base,
                   UWORD (*supply_func)(struct ypaworld_data *,LONG,LONG),
                   WORD xpos, WORD ypos)
/*
**  FUNCTION
**      Render-Shell für die Map in allen Größen und Arten.
**
**  INPUTS
**      ywd         -> Ptr auf LID des Welt-Objekts
**      size        -> 64, 32, 16, 8 oder 4
**      clip        -> Pointer auf Output-Channel
**      ps_x,ps_y   -> Start-X/Y im Pixel-Koord-System der Map
**      pe_x,pe_y   -> End-X/Y im Pixel-Koord-System der Map
**      font_base   -> zu benutzende Fonts sind ab hier definiert
**      supply_func -> diese Funktion muß auf eine SubSektor-Koordinate
**                     ein Font-Offset und eine Char-Num zurückliefern
**
**  CHANGED
**      06-Nov-95   floh    created
**      07-Nov-95   floh    Font-Handling
**      08-Nov-95   floh    universell gemacht für alle Lego-Größen
**      16-Nov-95   floh    sogar NOCH UNIVERSELLER gemacht
**                          (supply_func()).
**      05-Feb-96   floh    jetzt auch die Größe 32
**      21-Mar-96   floh    + Footprint-Feature: wenn die <supply_func>
**                            als char 0 zurückgibt, wird das entsprechende
**                            Element übersprungen (realisiert durch
**                            ein XPOS_REL(dist) im Stream). Folgen
**                            mehrere NULLs hintereinander, wird für alle
**                            nur ein Skip verwendet (also clever wie Sau).
**                          - Multiple-Font-Handling erstmal removed,
**                            das würde das neue Skipping ziemlich erschweren
**                            und ist mit der neuen Map-Philosophie
**                            hoffentlich eh gestorben.
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
*/
{
    ULONG shift, and_code;
    UWORD res;

    LONG store_ps_x = ps_x;

    switch(size) {
        case 4:     shift = 2; break;
        case 8:     shift = 3; break;
        case 16:    shift = 4; break;
        case 32:    shift = 5; break;
        case 64:    shift = 6; break;
        default:
            /*** can't happen ***/
            _LogMsg("yw_maprnd.c/yw_RMapLego(): wrong size\n");
            eos(clip);
            return(clip);
            break;
    };
    and_code = size-1;

    /*** Positionierung + Font ***/
    pos_abs(clip,xpos,ypos);
    newfont_flush(clip,font_base);

    /*** Zeile für Zeile... ***/
    while (ps_y < pe_y) {

        /*** Sub-Y-Koordinate ***/
        LONG suby = ps_y >> shift;
        LONG skip = 0;

        if (ps_y & and_code) {

            /*** oberer Teil der ersten Zeile wurde angebrochen ***/
            LONG off = ps_y & and_code;
            off_vert(clip,off);
            ps_y += (size - off);

        } else if ((pe_y - ps_y) < size) {

            /*** unteren Teil der letzten Zeile ignorieren ***/
            LONG draw = pe_y - ps_y;
            len_vert(clip,draw);
            ps_y = pe_y;

        } else {
            /*** eine ganzzahlige Zeile! ***/
            ps_y += size;
        };

        /*** eine Zeile zeichnen ***/

        /*** 1.Zeichen angebrochen? ***/
        if (ps_x & and_code) {

            BYTE off = ps_x & and_code;
            res = (*supply_func)(ywd, ps_x>>shift, suby);

            if (((UBYTE)res)==0) {
                /*** ein Skip! (rel. easy am Zeilenanfang) ***/
                skip = (size-off);
                xpos_rel(clip,skip);
            } else {
                skip = 0;
                off_hori(clip,off);
                put(clip,res);
            };
            ps_x += (size - off);
        };

        /*** ganzzahligen Rest zeichnen ***/
        for (ps_x; ps_x < (pe_x-size); ps_x += size) {
            res = (*supply_func)(ywd, ps_x>>shift, suby);

            if (((UBYTE)res)==0) {
                /*** ein Skip! ***/
                if (skip>0) {
                    skip += size;
                    clip[-2] = (skip>>8);
                    clip[-1] = (UBYTE) (skip);
                } else {
                    /*** einen neuen Skip einfügen ***/
                    skip = size;
                    xpos_rel(clip,skip);
                };
            } else {
                skip = 0;
                put(clip,res);
            };
        };

        /*** evtl. den ungeraden Rest zeichnen ***/
        if ((pe_x - ps_x) > 0) {
            BYTE draw = pe_x - ps_x;
            res = (*supply_func)(ywd, ps_x>>shift, suby);

            /*** Skips am Zeilenende sind eh egal... ***/
            if (((UBYTE)res)!=0) {
                len_hori(clip,draw);
                put(clip,res);
            };
        };

        /*** NEW_LINE ***/
        new_line(clip);

        /*** und die nächste Zeile... ***/
        ps_x = store_ps_x;
    };

    /*** Ende ***/
    return(clip);
}

/*-----------------------------------------------------------------*/
UBYTE *yw_GeneralMapInterior(struct ypaworld_data *ywd,
                             UBYTE *clip,
                             WORD xpos, WORD ypos,
                             WORD pw_x, WORD pw_y)
/*
**  FUNCTION
**      Universelle Map-Interior-Render-Routine, alle
**      Parameter werden übergeben, anstatt aus
**      der MR-Struktur entnommen zu werden.
**
**      ***WICHTIG***
**      MR.midx, MR.midz, MR.x_aspect, MR.y_aspect, MR.layers
**      und MR.zoom müssen auf die korrekten Werte gepatcht worden
**      sein!!! (zu beachten beim Radar-Modus!).
**
**  INPUTS
**      ywd     -> Pointer auf LID des Welt-Objects
**      clip    -> zeigt auf UBYTE-String-Buffer für Map-Inneres,
**                 positioniert nach den Clip-Positionierungs-Codes!
**      xpos    -> Display-X-Position der linken oberen Map-Ecke
**      ypos    -> ditto für Display-Y-Pos (Mittelpunkt in Screenmitte)
**      pw_x,pw_h   -> Pixel-Größe des "Mal-Bereichs"
**      layers      -> die zu rendernden Layer
**      zoom        -> die Zoom-Stufe
**
**  RESULTS
**      str - Pointer auf Ende des Ausgabe-Stroms
**
**  CHANGED
**      14-Sep-96   floh    abgespaltet
*/
{
    LONG ps_x, ps_y;
    LONG pm_x, pm_y;
    LONG pe_x, pe_y;
    UBYTE *str = clip;

    /*** Pixel-Position des aktuellen Mittelpunkts ermitteln ***/
    pm_x = MR.midx / MR.x_aspect;
    pm_y = -MR.midz / MR.y_aspect;

    /*** Start- und End-Koordinaten (Pixel-Position) ***/
    ps_x = pm_x - (pw_x>>1);
    ps_y = pm_y - (pw_y>>1);
    pe_x = ps_x + pw_x;
    pe_y = ps_y + pw_y;

    /*** zuallererst der BG-Layer (in Sektor-Auflösung) ***/
    str = yw_RMapCore(ywd,(4<<MR.zoom),str,
                      ps_x,ps_y,pe_x,pe_y,
                      FONTID_OWNER_4 + MR.zoom,
                      &yw_GetBGPattern,
                      xpos, ypos);

    /*** ...der Owner-Layer (immer Sektor-Auflösung) ***/
    if (MR.layers & MAP_LAYER_OWNER) {

        ULONG size = (4 << MR.zoom);
        UBYTE font_base = FONTID_OWNER_4 + MR.zoom;

        /*** Owner-Layer zeichnen ***/
        str = yw_RMapCore(ywd, size, str,
                          ps_x, ps_y,
                          pe_x, pe_y,
                          font_base,
                          &yw_GetOwnerSector,
                          xpos, ypos);
    };

    /*** dann Landscape-Layer ***/
    if (MR.layers & MAP_LAYER_LANDSCAPE) {

        ULONG size;
        UBYTE font_base;
        UWORD (*supply_func)(struct ypaworld_data *ywd,LONG,LONG);

        LONG tps_x = ps_x;
        LONG tps_y = ps_y;
        LONG tpe_x = pe_x;
        LONG tpe_y = pe_y;

        switch (MR.zoom) {

            case 0:
                /*** 4x4, Sektor-Auflösung ***/
                size        = 4;
                font_base   = FONTID_SEC4X4;
                supply_func = &yw_GetLandSector;
                break;

            case 1:
                /*** 8x8, Sektor-Auflösung ***/
                size        = 8;
                font_base   = FONTID_SEC8X8;
                supply_func = &yw_GetLandSector;
                break;

            case 2:
                /*** 4x4, Subsektor-Auflösung ***/
                size        = 4;
                font_base   = FONTID_LEGO4X4_0;
                supply_func = &yw_GetLandLego;
                tps_x += 2;  tps_y += 2;
                tpe_x += 2;  tpe_y += 2;
                break;

            case 3:
                /*** 8x8, Subsektor-Auflösung ***/
                size        = 8;
                font_base   = FONTID_LEGO8X8_0;
                supply_func = &yw_GetLandLego;
                tps_x += 4;  tps_y += 4;
                tpe_x += 4;  tpe_y += 4;
                break;

            case 4:
                /*** 16x16, Subsektor-Auflösung ***/
                size = 16;
                font_base = FONTID_LEGO16X16_0;
                supply_func = &yw_GetLandLego;
                tps_x += 8; tps_y += 8;
                tpe_x += 8; tpe_y += 8;
                break;
        };

        /*** Landscape-Layer zeichnen ***/
        str = yw_RMapCore(ywd, size, str,
                          tps_x, tps_y,
                          tpe_x, tpe_y,
                          font_base,
                          supply_func,
                          xpos, ypos);
    };

    /*** und Ende ***/
    return(str);
}

/*-----------------------------------------------------------------*/
void yw_RenderMapInterior(struct ypaworld_data *ywd, 
                          UBYTE *clip,
                          WORD xpos, WORD ypos)
/*
**  FUNCTION
**      Rendert das komplette Map-Innere, abhängig von der
**      eingestellten Zoom-Stufe und den eingestellten
**      Layern.
**
**  INPUTS
**      ywd     -> Pointer auf LID des Welt-Objects
**      clip    -> zeigt auf UBYTE-String-Buffer für Map-Inneres,
**                 positioniert nach den Clip-Positionierungs-Codes!
**      xpos    -> Display-X-Position der linken oberen Map-Ecke
**      ypos    -> ditto für Display-Y-Pos (Mittelpunkt in Screenmitte)
**
**  CHANGED
**      06-Nov-95   floh    created
**      08-Nov-95   floh    die restlichen Lego-Landscape-Modi
**      21-Nov-95   floh    Die MapReq-Struktur enthält jetzt die Felder
**                          ps_x, ps_y, pw_x, pw_y, die vorher
**                          lokale Vars dieser Routine waren. Die
**                          Werte werden aber noch vom Vehikel-Render-Hook
**                          benötigt... alles klar?
**      05-Feb-96   floh    die einzelnen Map-Modi sind jetzt nicht
**                          mehr mutual-exclusive, sondern werden als
**                          transparente Layer übereinander gezeichnet.
**      13-Mar-96   floh    revised & updated (neues Map-Layout)
**                          - Energie-Layer wurde gekillt
**      26-Jun-96   floh    HeightLayer endgültig removed
**      30-Jul-96   floh    revised & updated (Dynamic Layout + Locale)
**      07-Sep-96   floh    neuer Pseudo-Layer, der den Hintegrund
**                          dort füllt, wo die anderen Layer nichts
**                          hinmalen (damit die Map endlich mal
**                          wie ein richtiges Fenster aussieht.
*/
{
    clip = yw_GeneralMapInterior(ywd,clip,xpos,ypos,
           MR.req.req_cbox.rect.w - MR.BorHori,
           MR.req.req_cbox.rect.h - MR.BorVert);

    /*** wichtig: EOS ***/
    eos(clip);

    /*** und Ende ***/
}


