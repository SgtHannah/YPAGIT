/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_font.c,v $
**  $Revision: 38.18 $
**  $Date: 1998/01/06 16:19:27 $
**  $Locker: floh $
**  $Author: floh $
**
**  Font-Handling für ypaworld.class.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include "stdlib.h"
#include "string.h"

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "engine/engine.h"
#include "bitmap/ilbmclass.h"
#include "ypa/ypaworldclass.h"

_extern_use_nucleus
_extern_use_ov_engine       /* für _SetFont() */

/*** die "Default-Fonts", für die Font-Description-Files existieren ***/
#define NUM_YPAFONTS    (32)
UBYTE *YPAFontNames[NUM_YPAFONTS] = {
    "default.font",
    "maprobo.font",
    "icondown.font",
    "defblue.font",
    "defwhite.font",
    "menublue.font",
    "menugray.font",
    "menuw.font",
    "gadget.font",
    "menudown.font",
    "mapbtns.font",
    "maphorz.font",
    "mapvert.font",
    "mapvert1.font",
    "gauge4.font",
    "tracy.font",
    "mapcur4.font",
    "mapcur8.font",
    "mapcur16.font",
    "mapcur32.font",
    "mapcur64.font",
    "icon_nb.font",
    "icon_pb.font",
    "icon_db.font",
    "icon_ns.font",
    "icon_ps.font",
    "type_nb.font",
    "type_pb.font",
    "type_ns.font",
    "type_ps.font",
    "energy.font",
    "ltracy.font",
};

/*-----------------------------------------------------------------*/
void yw_FreeFont(struct VFMFont *fnt)
/*
**  FUNCTION
**      Entfernt eine mit yw_LoadFont() erzeugte
**      VFMFont-Struktur aus dem System.
**
**  INPUTS
**      fnt -> Pointer auf VFMFont-Struktur
**
**  RESULTS
**      ---
**
**  CHANGED
**      07-Aug-95   floh    created
*/
{
    if (fnt) {
        if (fnt->page_master) _dispose(fnt->page_master);
        if (fnt->fchars)      _FreeVec(fnt->fchars);
        _FreeVec(fnt);
    };
}

/*-----------------------------------------------------------------*/
struct VFMFont *yw_AllocFont(void)
/*
**  FUNCTION
**      Allokiert eine VFMFont-Struktur und ein FChars-Array
**      für max. 256 Buchstaben-Definitionen.
**
**  INPUTS
**      ---
**
**  RESULTS
**      Pointer auf leere VFMFont-Struktur. *Muß* vor Verwendung
**      erst initialisiert werden!!! NULL bei Mißerfolg (not enough mem).
**
**  CHANGED
**      07-Aug-95   floh    created
**
**  SEE ALSO
**      yw_LoadFont()
*/
{
    struct VFMFont *fnt;

    fnt = (struct VFMFont *) 
          _AllocVec(sizeof(struct VFMFont),
          MEMF_PUBLIC|MEMF_CLEAR);
    if (!fnt) return(NULL);

    fnt->fchars = (struct FontChar *) 
                  _AllocVec(sizeof(struct FontChar)*256,
                  MEMF_PUBLIC|MEMF_CLEAR);
    if (!fnt->fchars) {
        yw_FreeFont(fnt); return(NULL);
    };

    return(fnt);
}

/*-----------------------------------------------------------------*/
struct VFMFont *yw_BuildFont(UBYTE *fpage,
                             UWORD fnum,
                             UWORD x_size, UWORD y_size,
                             UWORD x_add, UWORD y_add,
                             UWORD x_num, UWORD y_num,
                             UWORD x_offset, UWORD y_offset)
/*
**  FUNCTION
**      "Baut" einen Font nicht aus einem Font-Description-
**      File, sondern "von Hand". Die Buchstaben-Positionen werden
**      dabei einfach "berechnet"... :-/
**
**      Die Buchstaben werden nacheinander initialisiert,
**      auch der Buchstabe Nummer 0, der ja useless ist.
**      Man sollte also an Position [0,0] in der Fontpage
**      keinen benutzbaren Buchstaben unterbringen...
**
**  INPUTS
**      fpage   - Filename der Fontpage, relativ zu mc2resources:
**      fnum    - Nummer des Fonts im Font-Array
**      x_size, y_size  - Größe eines Buchstaben in Pixel
**      x_add, y_add    - Adder für X- und Y-Koordinate
**      x_num, y_num    - Anzahl Buchstaben pro Zeile/Spalte
**      x_offset, y_offset - Start-Offset in Font-Page
**
**  RESULTS
**      Pointer auf Font-Struktur, oder NULL bei Fehler.
**
**  CHANGED
**      05-Nov-95   floh    created
**      16-Nov-95   floh    x_add und y_add eingeführt
**      05-Feb-96   floh    falls ein Zeichen irgendwohin übersteht,
**                          wird es zurechtgerückt.
**      26-Feb-97   floh    Fontpages werden mit dem BMA_Texture
**                          Attribut geladen, das GfxObjekt darf
**                          also drin rumfummeln.
**      25-Mar-97   floh    Fontpages erhalten das Modifier-Attr
**                          BMA_TxtBlittable
*/
{
    struct VFMFont *fnt = NULL;

    if (fnt = yw_AllocFont()) {

        ULONG i,x,y;

        /*** Fontpage laden ***/
        fnt->page_master = _new("ilbm.class",
                                RSA_Name,         fpage,
                                BMA_Texture,      TRUE,
                                BMA_TxtBlittable, TRUE,
                                TAG_DONE);
        if (fnt->page_master) {

            _get(fnt->page_master, BMA_Bitmap, &(fnt->page_bmp));
            fnt->page   = fnt->page_bmp->Data;
            fnt->height = y_size;

            /*** Char-Definitionen ausfüllen ***/
            for (i=0, y=0; y<y_num; y++) {
                for (x=0; x<x_num; x++, i++) {

                    UWORD act_x = x_offset + x * x_add;
                    UWORD act_y = y_offset + y * y_add;

                    /*** Rand-Korrektur ***/
                    if ((act_x + x_size) > (fnt->page_bmp->Width)) {
                        act_x = fnt->page_bmp->Width - x_size;
                    };
                    if ((act_y + y_size) > (fnt->page_bmp->Height)) {
                        act_y = fnt->page_bmp->Height - y_size;
                    };

                    fnt->fchars[i].offset = act_y * fnt->page_bmp->Width + act_x;
                    fnt->fchars[i].width  = x_size;
                };
            };
            _LogMsg("yw_BuildFont(): font #%d created from %s ok.\n",fnum,fpage);            
        } else {
            _LogMsg("yw_BuildFont(): font #%d created from %s FAILED.\n",fnum,fpage);            
            yw_FreeFont(fnt);
            fnt = NULL;
        };
    };

    return(fnt);
}

/*-----------------------------------------------------------------*/
struct VFMFont *yw_LoadFont(struct ypaworld_data *ywd, UBYTE *fontname)
/*
**  FUNCTION
**      Lädt den mittels <fontname> definierten Font und
**      gibt einen Pointer auf die erzeugte VFMFont-Struktur
**      zurück, oder NULL, falls ein Fehler auftrat.
**
**      <fontname> muß der Filename eines Font-Description-Files
**      sein, *relativ* zu MC2resources:
**      Siehe "ypa/ypworldclass.h" für Specs.
**
**  INPUTS
**      fontname    - Filename, ohne Pfadangabe
**
**  RESULTS
**      Ausgefüllte VFMFont-Struktur, oder NULL, falls Fehler.
**
**  CHANGED
**      07-Aug-95   floh    created
**      26-Jan-96   floh    oops, kam mit Ergebnis zurück, obwohl
**                          Font nicht geladen werden konnte...
**      03-Aug-96   floh    je nach Status von DspXRes (Display-X-Auflösung)
**                          wird der Font aus
**                          "set/fonts/#?.font" (DspXRes < 640) oder
**                          "set/hfonts/#?.font" geladen
**      26-Feb-97   floh    Fontpages jetzt mit BMA_Texture(TRUE)
**      25-Mar-97   floh    Fontpages erhalten das Modifier-Attr
**                          BMA_TxtBlittable
**      27-May-97   floh    + Buchstaben können jetzt auch mit
**                            einem #NUMMER eingeleitet werden!
**      18-Sep-97   floh    + Alle nicht initialisierten Zeichen
**                            werden ein Clone des 1.initialisierten
**                            Zeichens. Damit ist die Sache etwas
**                            absturzsicherer.
*/
{
    APTR file;
    UBYTE filename[255];
    struct VFMFont *fnt = NULL;
    UBYTE *path_prefix;

    /*** Filename zusammenbasteln... ***/
    strcpy(filename,"rsrc:");
    if (ywd->DspXRes < 512) strcat(filename,"fonts/");
    else                    strcat(filename,"hfonts/");
    strcat(filename,fontname);

    /*** File parsen ***/
    if (file = _FOpen(filename, "r")) {

        ULONG first_hit;
        ULONG i;

        /*** leere VFMFont-Struktur allokieren ***/
        if (fnt = yw_AllocFont()) {

            UBYTE line[128];

            /*** 1.Zeile ist FontPage-Name und Höhe des Fonts ***/
            if (_FGetS(line, sizeof(line), file)) {

                /*** Newline eliminieren ***/
                UBYTE *dike_out, *tok, *dummy;
                if (dike_out = strchr(line, '\n')) *dike_out = 0;

                /*** FontPage laden ***/
                if (tok = strtok(line, " \t")) {
                    fnt->page_master = _new("ilbm.class",
                                            RSA_Name,         tok,
                                            BMA_Texture,      TRUE,
                                            BMA_TxtBlittable, TRUE,
                                            TAG_DONE);
                    if (fnt->page_master) {
                        _get(fnt->page_master, BMA_Bitmap, &(fnt->page_bmp));
                        fnt->page = fnt->page_bmp->Data;
                    } else {
                        /*** konnte FontPage nicht laden ***/
                        yw_FreeFont(fnt);
                        _LogMsg("yw_LoadFont(): font %s, couldn't load fontpage %s.\n",fontname,tok);
                        return(NULL);
                    };
                } else {
                    /*** Font-Description-File mangled ***/
                    yw_FreeFont(fnt);
                    _LogMsg("yw_LoadFont(): font %s, font definition file corrupt.\n",fontname);
                    return(NULL);
                };

                /*** Font-Höhe lesen ***/
                if (tok = strtok(NULL, " \t")) {
                    fnt->height = strtol(tok, &dummy, 0);
                } else {
                    /*** Font-Description-File mangled ***/
                    _LogMsg("yw_LoadFont(): font %s, font definition file corrupt.\n",fontname);
                    yw_FreeFont(fnt);
                    return(NULL);
                };

            } else {
                /*** unexpected EOF ***/
                _LogMsg("yw_LoadFont(): font %s, font definition file corrupt.\n",fontname);
                yw_FreeFont(fnt);
                return(NULL);
            };

            /*** alle Buchstaben-Definitionen laden ***/
            while (_FGetS(line, sizeof(line), file)) {

                /*** Newline eliminieren ***/
                UBYTE *dike_out, *tok, *dummy, *x_pnt;
                UBYTE chr;
                UWORD x,y,w;
                if (dike_out = strchr(line, '\n')) *dike_out = 0;

                /*** Char-Code laden ***/
                if (line[0] == ' ') {
                    /*** Sonderfall Leerzeichen ***/
                    chr   = line[0];
                    x_pnt = &(line[1]);
                } else if (tok = strtok(line," \t")) {
                    if ((tok[0] == '#') && tok[1]) {
                        /*** eine Nummern-Definition ***/
                        chr   = (UBYTE) strtol(&(tok[1]),NULL,0);
                    } else {
                        chr = tok[0];
                    };
                    x_pnt = NULL;
                } else continue;

                /*** X,Y-Pos und Breite laden ***/
                if (tok = strtok(x_pnt, " \t")) {
                    x = (UWORD) strtol(tok, &dummy, 0);
                } else continue;

                if (tok = strtok(NULL, " \t")) {
                    y = (UWORD) strtol(tok, &dummy, 0);
                } else continue;

                if (tok = strtok(NULL, " \t")) {
                    w = (UWORD) strtol(tok, &dummy, 0);
                } else continue;

                /*** entsprechende Font-Char-Struktur ausfüllen ***/
                fnt->fchars[chr].offset = y*fnt->page_bmp->Width + x;
                fnt->fchars[chr].width  = w;
            };

            /*** ungültige Buchstaben gültig machen ***/
            first_hit = 0;
            for (i=0; i<256; i++) {
                /*** Pass 1 findet den 1.initialisierten Buchstaben ***/
                if (fnt->fchars[i].width != 0) {
                    first_hit = i;
                    break;
                };
            };
            for (i=0; i<256; i++) {
                /*** Pass 2 überschreibt nicht initialisierte Buchstaben ***/
                if (fnt->fchars[i].width == 0) {
                    fnt->fchars[i] = fnt->fchars[first_hit];
                };
            };
        };

        /*** File schließen ***/
        _FClose(file);
        _LogMsg("yw_LoadFont(): font %s loaded ok.\n",fontname);
    };
    return(fnt);
}

/*-----------------------------------------------------------------*/
BOOL yw_LoadFontSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Lädt alle Fonts (welche ja jetzt setspezifisch
**      sein dürfen).
**
**  INPUTS
**      ywd -> Ptr auf LID des Welt-Objects
**
**  RESULTS
**      TRUE/FALSE als Erfolgs-Indikator.
**
**  CHANGED
**      05-Nov-95   floh    created
**      07-Nov-95   floh    FontIDs angepaßt
**      16-Nov-95   floh    x_add und y_add in yw_BuildFont() beachtet
**      21-Nov-95   floh    + neue Fonts für Vehikel-Rendering in
**                            2D-Karte
**      24-Jan-96   floh    all new...
**      05-Feb-96   floh    alte Fonts rausgehauen, neue rein
**      10-Feb-96   floh    wieder mehrere Map-Vehikel-Fonts
**      19-Mar-96   floh    Vehikel-Font-Offsets nach BB's Fontpage-
**                          Aufräuming übernommen.
**      20-May-96   floh    Map-Vehikel-Fonts jetzt etwas komplexer,
**                          werden deshalb per normalem Font-Description-
**                          File geladen!
**      31-Jul-96   floh    Initialisiert jetzt die globalen
**                          Layout-Variablen, weil die ja
**                          ausschließlich von den Fonts abhängig sind.
**      03-Aug-96   floh    falls (DspXRes >= 640) werden die
**                          Fonts aus dem Verzeichnis "hfonts/" statt
**                          "fonts" geladen!
**      06-Aug-96   floh    neue Layout-Variablen ywd->IconBW, ywd->IconBH,
**                          ywd->IconSW, ywd->IconSH
**      07-Sep-96   floh    Der "Owner-Font" hat ein neues Char #9,
**                          in der normalen Window-BG-Farbe, als Füllstück
**                          für die Map. Das heißt: an mapmisc.ilbm
**                          muß "oben rechts" ein neues 32-Pixel-Char ran.
**      11-Apr-97   floh    + Brightness-Fonts gefixt, Owner-Fonts
**                            haben neues Fontpage-Handling
**      03-Jun-97   floh    + neue Fonts "lego16.font", "mapcur64.font",
**                            "mapvhcl9.font"
**      11-Oct-97   floh    + ywd->UpperTabu und ywd->LowerTabu wird
**                            initialisiert
**      20-May-98   floh    + UpperTabu nimmt jetzt Hoehe des Energy-Font
*/
{
    struct VFMFont *fnt;
    LONG id;
    
    _LogMsg("yw_LoadFontSet() entered.\n");    

    /*** erstmal die Font-Description-File-Fonts ***/
    for (id=0; id<NUM_YPAFONTS; id++) {
        ywd->Fonts[id] = yw_LoadFont(ywd,YPAFontNames[id]);
        if (ywd->Fonts[id]) _SetFont(ywd->Fonts[id],id);
        else {
            _LogMsg("Could not load font (%s)",YPAFontNames[id]);
            return(FALSE);
        };
    };

    /*** 16x16 Landscape Map Font ***/
    id  = FONTID_LEGO16X16_0;
    fnt = yw_BuildFont("lego16.ilbm", id,
                       16, 16,      // x_size, y_size
                       16, 16,      // x_add, y_add
                       16, 16,      // x_num, y_num
                        0,  0);     // x_offset, y_offset
    if (fnt) {
        ywd->Fonts[id] = fnt;
        _SetFont(fnt, id);
    } else return(FALSE);

    /*** 8x8 Landscape Map Font ***/
    id  = FONTID_LEGO8X8_0;
    fnt = yw_BuildFont("lego8.ilbm", id, 
                        8, 8,       // x_size, y_size
                        8, 8,       // x_add, y_add
                        16, 16,     // x_num, y_num
                        0, 0);      // x_offset, y_offset
    if (fnt) {
        ywd->Fonts[id] = fnt;
        _SetFont(fnt, id);
    } else return(FALSE);

    /*** 4x4 Landscape Map Font ***/
    id = FONTID_LEGO4X4_0;
    fnt = yw_BuildFont("lego4.ilbm", id, 
                       4, 4,        // x_size, y_size
                       4, 4,        // x_add, y_add
                       16, 16,      // x_num, y_num
                       0, 0);       // x_offset, y_offset
    if (fnt) {
        ywd->Fonts[id] = fnt;
        _SetFont(fnt, id);
    } else return(FALSE);

    /*** 8x8 Landscape Sektor Font ***/
    id = FONTID_SEC8X8;
    fnt = yw_BuildFont("sec8.ilbm", id, 
                       8, 8,        // x_size, y_size
                       8, 8,        // x_add, y_add
                       16, 16,      // x_num, y_num
                       0, 0);       // x_offset, y_offset
    if (fnt) {
        ywd->Fonts[id] = fnt;
        _SetFont(fnt, id);
    } else return(FALSE);

    /*** 4x4 Landscape Sektor Font ***/
    id = FONTID_SEC4X4;
    fnt = yw_BuildFont("sec4.ilbm", id, 
                       4, 4,        // x_size, y_size
                       4, 4,        // x_add, y_add
                       16, 16,      // x_num, y_num
                       0, 0);       // x_offset, y_offset
    if (fnt) {
        ywd->Fonts[id] = fnt;
        _SetFont(fnt, id);
    } else return(FALSE);

    /*** die 4 Owner-Color-Fonts in verschiedenen Größen ***/
    for (id=0; id<5; id++) {

        ULONG x_size  = (4<<id);
        ULONG y_size  = x_size;
        ULONG x_start,y_start,x_add;
        ULONG real_id = id + FONTID_OWNER_4;

        switch(id) {
            case 0: x_start=9*32+9*16+9*8; x_add=4;  y_start=64; break;
            case 1: x_start=9*32+9*16;     x_add=8;  y_start=64; break;
            case 2: x_start=9*32;          x_add=16; y_start=64; break;
            case 3: x_start=0;             x_add=32; y_start=64; break;
            case 4: x_start=0;             x_add=64; y_start=0;  break;
        };

        fnt = yw_BuildFont("mapmisc.ilbm", real_id,
                           x_size, y_size,
                           x_add, 0,               // x_add, y_add
                           9, 1,                   // x_num, y_num
                           x_start, y_start);      // x_offset, y_offset

        /*** Window-BG nach Char 9 patchen ***/
        fnt->fchars[9].offset = fnt->fchars[8].offset;
        fnt->fchars[9].width  = fnt->fchars[8].width;

        /*** unerlaubtes Char 0 (neutraler Owner) auf Char 8 patchen ***/
        fnt->fchars[8].offset = fnt->fchars[0].offset;
        fnt->fchars[8].width  = fnt->fchars[0].width;

        if (fnt) {
            ywd->Fonts[real_id] = fnt;
            _SetFont(fnt, real_id);
        } else return(FALSE);
    };

    /*** Map-Vehicle-Fonts ***/
    ywd->Fonts[FONTID_BACT3X3] = yw_LoadFont(ywd,"mapvhcl3.font");
    if (ywd->Fonts[FONTID_BACT3X3]) {
        _SetFont(ywd->Fonts[FONTID_BACT3X3], FONTID_BACT3X3);
    } else return(FALSE);

    ywd->Fonts[FONTID_BACT5X5] = yw_LoadFont(ywd,"mapvhcl5.font");
    if (ywd->Fonts[FONTID_BACT5X5]) {
        _SetFont(ywd->Fonts[FONTID_BACT5X5], FONTID_BACT5X5);
    } else return(FALSE);

    ywd->Fonts[FONTID_BACT7X7] = yw_LoadFont(ywd,"mapvhcl7.font");
    if (ywd->Fonts[FONTID_BACT7X7]) {
        _SetFont(ywd->Fonts[FONTID_BACT7X7], FONTID_BACT7X7);
    } else return(FALSE);

    ywd->Fonts[FONTID_BACT9X9] = yw_LoadFont(ywd,"mapvhcl9.font");
    if (ywd->Fonts[FONTID_BACT9X9]) {
        _SetFont(ywd->Fonts[FONTID_BACT9X9], FONTID_BACT9X9);
    } else return(FALSE);

    /*** initialisiere Layout-Variablen ***/
    ywd->FontH  = ywd->Fonts[FONTID_DEFAULT]->height;
    ywd->CloseW = ywd->Fonts[FONTID_DEFAULT]->fchars['a'].width;
    ywd->PropW  = ywd->Fonts[FONTID_MAPVERT]->fchars['B'].width;
    ywd->PropH  = ywd->Fonts[FONTID_MAPHORZ]->height;
    ywd->EdgeW  = ywd->Fonts[FONTID_DEFAULT]->fchars['b'].width;
    ywd->EdgeH  = 2;
    ywd->VPropH = ywd->Fonts[FONTID_MAPVERT]->height;
    ywd->IconBW = ywd->Fonts[FONTID_ICON_NB]->fchars['A'].width;
    ywd->IconBH = ywd->Fonts[FONTID_ICON_NB]->height;
    ywd->IconSW = ywd->Fonts[FONTID_ICON_NS]->fchars['A'].width;
    ywd->IconSH = ywd->Fonts[FONTID_ICON_NS]->height;
    ywd->UpperTabu = ywd->Fonts[FONTID_ENERGY]->height;
    ywd->LowerTabu = ywd->IconBH;

    /*** Phew... ***/
    _LogMsg("yw_LoadFontSet() left.\n");    
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillFontSet(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Entfernt *ALLE* geladenen Fonts...
**
**  INPUTS
**      ywd - Pointer auf LID des Welt-Objects
**
**  CHANGED
**      05-Nov-95   floh    created
**      24-Jan-96   floh    neu...
*/
{
    ULONG i;
    _LogMsg("yw_KillFontSet() entered.\n");    
    for (i=0; i<MAXNUM_FONTS; i++) {
        if (ywd->Fonts[i]) yw_FreeFont(ywd->Fonts[i]);
    };
    memset(ywd->Fonts,0,sizeof(ywd->Fonts));
    _LogMsg("yw_KillFontSet() left.\n");    
}


