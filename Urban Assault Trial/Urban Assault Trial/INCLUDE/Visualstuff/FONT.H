#ifndef VISUALSTUFF_FONT_H
#define VISUALSTUFF_FONT_H
/*
**  $Source: PRG:VFM/Include/visualstuff/font.h,v $
**  $Revision: 38.9 $
**  $Date: 1998/01/06 14:11:01 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen für Text-Drawing-Funktionen der gfx.engine.
**
**  (C) Copyright 1995 by A.Weissflog
*/
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef NUCLEUS_NUCLEUS2_H
#include "nucleus/nucleus2.h"
#endif

#ifndef VISUALSTUFF_BITMAP_H
#include "visualstuff/bitmap.h"
#endif

#ifdef __DBCS__
	#ifndef MISC_DBCS_H
	#include "misc/dbcs.h"
	#endif
#endif

/*-------------------------------------------------------------------
**  Background
**  ~~~~~~~~~~
**  Die gfx.engine bietet folgende Funktionen zur Textausgabe
**  an:
**
**      void _SetFont(struct VFMFont *font, UBYTE id);
**      void _DrawText(struct drawfont_args);
**
**  'Textausgabe' beschränkt sich dabei nicht auf die klassische
**  Bedeutung "einfarbige Buchstaben auf einfarbigen Hintergrund",
**  sondern auf die schnelle Ausgabe von farbigen Images, deren
**  Anordnung durch einen Kontrollstring beliebig definierbar ist.
**
**  Innerhalb eines Strings sind Kontroll-Sequenzen eingestreut,
**  mit deren Hilfe externe Substrings (clips)  eingefügt werden können,
**  der aktuelle Font gewechselt, oder die Breite des nächsten
**  Buchstaben explizit vorgegeben werden kann etc etc...
**
**  Die Textausgabe basiert ausschließlich auf nichtproportionalen Fonts.
*/

/*-------------------------------------------------------------------
**  Definition eines Buchstaben im Font.
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**  Die <struct VFMFont> enthält ein Array, auf das per ASCII-Code
**  als Index direkt zugegriffen werden kann, um so schnell wie
**  möglich an die relevanten Daten für jeden Buchstaben zu kommen.
*/
struct FontChar {
    UWORD offset;       // Start des Buchstaben in Fontpage als Direkt-Offset
    UWORD width;        // Breite des Buchstaben in Pixel, [width < 256]
};

struct VFMFont {
    Object *page_master;        // 'Besitzer' der Fontpage, wird ignoriert
    struct VFMBitmap *page_bmp; // Ptr auf reguläre Page-Bitmap
    UBYTE *page;                // die "Font-Page" für fast access
    struct FontChar *fchars;    // Ptr auf Array mit 255 Einträgen
    UWORD height;               // Höhe des Fonts in Pixel, [height < 256]
};

struct drawtext_args {
    UBYTE *string;
    UBYTE **clips;              // NULL, wenn <string> keine Clips enthält
};

/*--------------------------------------------------------------------
**  Definition der Kontroll-Codes.
**  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**  Die Font-Engine behandelt einen einzigen Byte-Wert nicht
**  als Buchstabe, sondern als Control-Sequence-Introducer.
**  Dieser Byte-Wert ist CTRL_SEQ (0), die nachfolgenden
**  Bytes bestimmen die genaue Bedeutung der Kontroll-Sequenz.
**
**  Das Byte unmittelbar nach CTRL_SEQ ist der Selektor. Damit
**  wird definiert, um *welche* Kontroll-Sequenz es sich handelt.
**  Nach dem Selektor folgen optional beliebig viele "Daten-Bytes"
**  für die Kontroll-Sequenz.
*/
#define CTRL_SEQ    (0)

/*-------------------------------------------------------------------
**  Selektor-Definition:
**  ~~~~~~~~~~~~~~~~~~~~
**
**  00       -> End Of String
**  01 nn nn -> setze X-Position direkt, WORD nn ist Display-X-Koordinate,
**              Nullpunkt in Screen-Mitte. High Order Byte first (!!!),
**              ACHTUNG, ENDIAN ABHÄNGIG!!!
**  02 nn nn -> setze Y-Position direkt, siehe <01>
**  03 nn nn -> setze X-Position Rand-relativ, wenn <nn nn> positiv, dann
**              relativ zu linken Rand, sonst zu rechten Rand.
**  04 nn nn -> setze Y-Position Rand-relativ, siehe <03>
**  05 nn nn -> setze X-Position relativ zu aktueller Position
**  06 nn nn -> setze Y-Position relativ zu aktueller Position
**
**  07       -> New Line, Position wird an Anfang der aktuellen
**              Zeile gesetzt, um <font_height> nach unten verschoben
**  08 nn    -> Neuer Font, <nn> ist Font-ID, wie mit _AddFont() definiert.
**              Die aktuellen Zeilen-Parameter (siehe 0e und 0f werden
**              *nicht* modifiziert!)
**  09 nn    -> Insert Clip, der Input-Stream wird auf den mittels
**              <nn> definierten Clip umgebogen, damit lassen sich
**              dynamische Werte in sonst statische Strings einbinden.
**              Clips sind beliebig schachtelbar.
**
**  0a nn    -> Stretch, der nächste Buchstabe wird <nn> Pixel breit.
**              Dabei wird nur die erste Spalte des Buchstaben <nn>
**              mal hintereinander gezeichnet! Sinnvoll als Füllstück.
**
**  0b nn    -> Stretch to, der nächste Buchstabe wird solange gestretcht,
**              bis die Position <nn> relativ zum Zeilen-Anfang erreicht
**              ist. Ebenfalls sinnvoll als Füller für Zeilen mit
**              dynamischen Inhalt. Funktioniert sonst wie <0a>
**
**  0c nn    -> Offset Hori, die ersten <nn> Spalten des nächsten Buchstaben
**              werden ignoriert.
**  0d nn    -> Length Hori, zeichne nur die ersten <nn> Spalten des
**              nächsten Buchstaben
**  0e nn    -> Offset Vert, die ersten <nn> Zeilen *aller* folgenden
**              Buchstaben bis zum Zeilenende werden ignoriert.
**  0f nn    -> Length Vert, zeichne nur die ersten <nn> Zeilen der
**              aktuellen Zeile.
**  10 nn    -> New Font & Flush, wie NewFont, die Zeilen-
**              Parameter werden aber sofort modifiziert.
**  11 nn nn -> Stretch To Long (mit 16 Bit Stretcher)
*/

/*** die folgenden Macros sind zum Einfügen in ein ***/
/*** vorinitialisiertes UBYTE[] gedacht...         ***/

#define EOS             0x0,0x0       // End Of String

/* Position absolut setzen */
#define XPOS_ABS(x)     0x0,0x1,(x>>8),(x)
#define YPOS_ABS(y)     0x0,0x2,(y>>8),(y)
#define POS_ABS(x,y)    XPOS_ABS(x),YPOS_ABS(y)

/* Position rand-relativ setzen */
#define XPOS_BREL(x)    0x0,0x3,(x>>8),(x)
#define YPOS_BREL(y)    0x0,0x4,(y>>8),(y)
#define POS_BREL(x,y)   XPOS_BREL(x),YPOS_BREL(x)

/* Position positions-relativ setzen */
#define XPOS_REL(x)     0x0,0x5,(x>>8),(x)
#define YPOS_REL(y)     0x0,0x6,(y>>8),(y)
#define POS_REL(x,y)    XPOS_REL(x),YPOS_REL(y)

#define NEW_LINE            0x0,0x7
#define NEW_FONT(x)         0x0,0x8,(x)
#define NEW_FONTFLUSH(x)    0x0,0x10,(x)
#define CLIP(x)             0x0,0x9,(x)
#define STRETCH(x)          0x0,0xa,(x)
#define STRETCH_TO(x)       0x0,0xb,(x)
#define OFF_HORI(x)         0x0,0xc,(x)
#define LEN_HORI(x)         0x0,0xd,(x)
#define OFF_VERT(x)         0x0,0xe,(x)
#define LEN_VERT(x)         0x0,0xf,(x)
#define LSTRETCH_TO(x)      0x0,0x11,(x>>8),(x)

/*** die folgenden Macros können zum dynamischen Aufbau benutzt werden ***/
#define put_word(str,w)		*str++=(UBYTE)((WORD)w>>8);*str++=(UBYTE)w;

#define put(str,c)          *str++=(UBYTE)c;
#define eos(str)            *str++=0x0;*str++=0x0;
#define xpos_abs(str,x)     *str++=0x0;*str++=0x1;*str++=(UBYTE)((WORD)x>>8);*str++=(UBYTE)x;
#define ypos_abs(str,y)     *str++=0x0;*str++=0x2;*str++=(UBYTE)((WORD)y>>8);*str++=(UBYTE)y;
#define pos_abs(str,x,y)    xpos_abs(str,x);ypos_abs(str,y);

#define xpos_brel(str,x)    *str++=0x0;*str++=0x3;*str++=(UBYTE)((WORD)x>>8);*str++=(UBYTE)x;
#define ypos_brel(str,y)    *str++=0x0;*str++=0x4;*str++=(UBYTE)((WORD)y>>8);*str++=(UBYTE)y;
#define pos_brel(str,x,y)   xpos_brel(str,x);ypos_brel(str,y);

#define xpos_rel(str,x)     *str++=0x0;*str++=0x5;*str++=(UBYTE)((WORD)x>>8);*str++=(UBYTE)x;
#define ypos_rel(str,y)     *str++=0x0;*str++=0x6;*str++=(UBYTE)((WORD)y>>8);*str++=(UBYTE)y;
#define pos_rel(str,x,y)    xpos_rel(str,x);ypos_rel(str,y);

#define new_line(str)        *str++=0x0;*str++=0x7;
#define new_font(str,f)      *str++=0x0;*str++=0x8;*str++=(UBYTE)f;
#define newfont_flush(str,f) *str++=0x0;*str++=0x10;*str++=(UBYTE)f;

#define clip(str,c)          *str++=0x0;*str++=0x9;*str++=(UBYTE)c;

#define stretch(str,x)      *str++=0x0;*str++=0xa;*str++=(UBYTE)x;
#define stretch_to(str,x)   *str++=0x0;*str++=0xb;*str++=(UBYTE)x;
#define off_hori(str,x)     *str++=0x0;*str++=0xc;*str++=(UBYTE)x;
#define len_hori(str,x)     *str++=0x0;*str++=0xd;*str++=(UBYTE)x;
#define off_vert(str,y)     *str++=0x0;*str++=0xe;*str++=(UBYTE)y;
#define len_vert(str,y)     *str++=0x0;*str++=0xf;*str++=(UBYTE)y;

#define lstretch_to(str,x)  *str++=0x0;*str++=0x11;*str++=(UBYTE)((WORD)x>>8);*str++=(UBYTE)x;

#ifdef __DBCS__
#define put_dbcs(str,w,align,txt)	*str++=0x0;*str++=0x12;put_word(str,w);put_word(str,align);strcpy(str,txt);str+=strlen(txt);*str++=0;
#define	freeze_dbcs_pos(str)		*str++=0x0;*str++=0x13;
#define set_dbcs_flags(str,f) 		*str++=0x0;*str++=0x14;put_word(str,f);
#define unset_dbcs_flags(str,f)		*str++=0x0;*str++=0x15;put_word(str,f);
#define dbcs_color(str,r,g,b)		*str++=0x0;*str++=0x16;put_word(str,r);put_word(str,g);put_word(str,b);
#endif 

/*-----------------------------------------------------------------*/
#endif
