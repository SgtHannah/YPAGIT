/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  Zambastelroutinen für button.class.
**
**  (C) Copyright 1996 by Andreas Flemming
*/
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include <math.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "requester/buttonclass.h"
#include "input/clickbox.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_input_engine
    
char   output[ 5000 ];
WORD bt_GetStringLen( struct VFMFont *font, char *text );
void bt_DoVFMRendering( struct button_data *btd, struct bt_button *bt, char **ptr );
#ifdef __DBCS__
void bt_DoTrueTypeRendering( struct button_data *btd, struct bt_button *bt, char **ptr );
#endif

_dispatcher( void, bt_BTM_SWITCHPUBLISH, struct switchpublish_msg *sp )
{
/*
**  FUNCTION    Schaltet den Requester, also die Buttonleiste ein oder
**              aus. Wenn die Darstellung unterdrückt werden soll,
**              muß das Flag ausgeschaltet werden und die ClickBox der
**              inputengine entrissen werden.
**              weil anfangs alles gelöscht ist (somit auch das Publish-
**              Flag) machen wir es so, daß vor der darstellung
**              immer ein switchpublish aufgerufen werden muß.
*/

    struct button_data *btd;

    btd = INST_DATA( cl, o);

    switch( sp->modus ) {

        case SP_PUBLISH:

            if( !(btd->flags & BF_PUBLISH)) {

                btd->flags |= BF_PUBLISH;

                /*** Anmelden ***/
                _AddClickBox( &(btd->click), IE_CBX_ADDHEAD);
                }
            break;

        case SP_NOPUBLISH:

            if( btd->flags & BF_PUBLISH ) {

                btd->flags &= ~BF_PUBLISH;

                /*** Abmelden ***/
                _RemClickBox( &(btd->click));
                }
            break;
        }
}


void bt_DoVFMRendering( struct button_data *btd, struct bt_button *bt, char **ptr )
{
    /* ------------------------------------------------------------
    ** Font
    ** jeder Button hat seinen eigenen Font, somit ist die Unter-
    ** scheidung nach der Art sinnlos, weil diese im allgmeinen 
    ** sowieso nicht umgestellt wird. Lediglich eine Unterscheidung 
    ** in enabled/disabled ist notwendig. 
    ** ----------------------------------------------------------*/
    WORD   slen, fontnr, j, pixel_count, breite, rest, pos;
    struct VFMFont *font; 
    char   *str;
    
    str = *ptr;
            
    pixel_count = 0;  // zählt Text(!)breite
        
    if( bt->flags & BT_DISABLED )
        fontnr = bt->disabled_font;
    else {

        /*** Gedrückt? ***/
        if( bt->flags & BT_PRESSED )
            fontnr = bt->pressed_font;
        else
            fontnr = bt->unpressed_font;
        }

    font = _GetFont( fontnr );

    /*** Länge des Strings für das Zentrieren ***/
    if( bt->flags & BT_PRESSED )
        slen = bt_GetStringLen( font, bt->pressed_text );
    else
        slen = bt_GetStringLen( font, bt->unpressed_text );

    /*** was müssen wir platz lassen? ***/
    slen = (bt->w - slen - font->fchars[ btd->ch_bopen  ].width ) / 2;

    /* ------------------------
    ** Now lets bastel e string 
    ** ----------------------*/

    /*** Der Font ***/
    new_font( str, fontnr );
                
    /*** Die Position ***/
    pos = btd->click.rect.x + bt->x - btd->screen_x/2;
    xpos_abs( str, pos );
                
    pos = btd->click.rect.y + bt->y - btd->screen_y/2; 
    ypos_abs( str, pos );

    /*** der String höchstselbst ***/
    if( bt->modus == BM_SLIDER ) {

        /*** Dann wurde der String schon vorbereitet ***/
        j = 0;

        /* ---------------------------------------------------
        ** wir erkennen das Ende an 2 Nullen, übernehmen diese
        ** aber nicht, weil hier der Abschluß gemacht wird
        ** -------------------------------------------------*/
        while( bt->unpressed_text[ j ] || bt->unpressed_text[ j + 1 ] ) {
            put( str, bt->unpressed_text[ j++ ] );
            }
        }
    else {

        /*** Die Gadgeteröffnung und Breite ***/
        breite = bt->w;
        if( (bt->modus != BM_STRING) && (bt->flags & BT_BORDER) ) {

            put( str, btd->ch_bopen );

            /*** Breite wird um Rand reduziert ***/
            breite -= font->fchars[ btd->ch_bopen  ].width;
            breite -= font->fchars[ btd->ch_bclose ].width;
            }

        /*** Platz lassen ? ***/
        if( (slen > 0) && (bt->flags & BT_CENTRE) ) {

            stretch( str, slen );
            put( str, btd->ch_bfill );

            pixel_count += slen;
            }

        /*** Vielleicht gedrückt? ***/
        if( bt->flags & BT_PRESSED ) {

            j = 0;
            while( bt->pressed_text[ j ] ) {

                /*** geteilter Buchstabe und 1 Pixel Rand ***/
                if( (pixel_count + font->fchars[ bt->pressed_text[ j ] ].width)
                    > breite ) {

                    rest = breite - pixel_count;

                    /*** ist vom Buchstabe überhaupt was zu sehen? ***/
                    if( rest > 2 ) {
                        
                        len_hori( str, rest );
                        put( str, bt->pressed_text[ j++ ] );
                        pixel_count      += rest;
                        }
                    break;
                    }

                pixel_count += font->fchars[ bt->pressed_text[ j ] ].width;
                put( str, bt->pressed_text[ j++ ]);
                }
            }
        else {

            /*** Ungedrückt, egal ob disabled oder nich' ***/

            j = 0;
            while( bt->unpressed_text[ j ] )  {
                        
                /*** geteilter Buchstabe und 1 Pixel Rand ***/
                if( (pixel_count + font->fchars[ bt->unpressed_text[ j ] ].width)
                    > breite ) {
                     rest = breite - pixel_count;

                    /*** ist vom Buchstabe überhaupt was zu sehen? ***/
                    if( rest > 2 ) {
                        
                        len_hori( str, rest );
                        put( str, bt->unpressed_text[ j++ ]);
                        pixel_count      += rest;
                        }
                    break;
                    }

                pixel_count += font->fchars[ bt->unpressed_text[ j ] ].width;
                put( str,  bt->unpressed_text[ j++ ] );
                }
            }

        /*** Auffüllen, wenn notwendig  ***/
        if( pixel_count < breite ) { 
        
            if( (bt->modus == BM_STRING) || (!(bt->flags & BT_BORDER)) ) {                
                lstretch_to( str, bt->w ); 
                }
            else  {
                lstretch_to( str, (bt->w - font->fchars[ btd->ch_bclose ].width ));
                } 
                
            put( str, btd->ch_bfill );    
            }

        /*** Der GadgetAbschluss ***/
        if( (bt->modus != BM_STRING) && (bt->flags & BT_BORDER) ) {
            put( str, btd->ch_bclose );
            }
        }      
        
    /*** Wert wieder zurueckschreiben ***/    
    *ptr = str;
}                

#ifdef __DBCS__
void bt_DoTrueTypeRendering( struct button_data *btd, struct bt_button *bt, char **ptr )
{
    /*
    ** Hier laeufts e wingl annersch. Also. ich zeichne zuerst den
    ** Button je nach Wunsch leer. So dass nur ein rahmen entsteht.
    ** Dann uebergebe ich das alles an die DBCS-Routine, die dann drueber
    ** zeichnet.
    **
    ** Ich kann eine Menge Vereinfachungen machen, weil es nur Gadgets
    ** mit Text sind.
    */
    
    UBYTE *str, *txt;
    WORD   fontnr, pos, breite, flags;
    struct VFMFont *font;
    
    if( bt->flags & BT_DISABLED )
        fontnr = bt->disabled_font;
    else {

        /*** Gedrückt? ***/
        if( (bt->flags & BT_PRESSED) && 
            (!(bt->flags & BT_NOPRESS)) )
            fontnr = bt->pressed_font;
        else
            fontnr = bt->unpressed_font;
        }

    font = _GetFont( fontnr );

    str = *ptr;

    /*** Der Font ***/
    new_font( str, fontnr );
                
    /*** Die Position ***/
    pos = btd->click.rect.x + bt->x - btd->screen_x/2;
    xpos_abs( str, pos );
    
    pos = btd->click.rect.y + bt->y - btd->screen_y/2;
    ypos_abs( str, pos );
    
    /*** Die Gadgeteröffnung und Breite ***/
    breite = bt->w;
    if( (bt->modus != BM_STRING) && (bt->flags & BT_BORDER) ) {

        put( str, btd->ch_bopen );

        /*** Breite wird um Rand reduziert ***/
        breite -= font->fchars[ btd->ch_bopen  ].width;
        breite -= font->fchars[ btd->ch_bclose ].width;
        } 
        
    /*** Jetzt Position aufheben ***/
    freeze_dbcs_pos( str );    
        
    /*** jetzt Breite stretchen ***/
    lstretch_to( str, breite );
    put( str, btd->ch_bfill );
    
    /*** abschluss ***/    
    if( (bt->modus != BM_STRING) && (bt->flags & BT_BORDER) ) {
        put( str, btd->ch_bclose ); 
        }
        
    /*** Jetzt text schreiben ***/
    if( bt->flags & BT_PRESSED ) {
        
        if( bt->flags & BT_NOPRESS )
            flags = 0;
        else    
            flags = DBCSF_PRESSED;
        txt   = bt->pressed_text;
        }
    else {
    
        flags = 0;
        txt   = bt->unpressed_text;
        }
        
    if( bt->flags & BT_CENTRE )
        flags |= DBCSF_CENTER;
    else    
        if( bt->flags & BT_RIGHTALIGN)
            flags |= DBCSF_RIGHTALIGN;
        else
            flags |= DBCSF_LEFTALIGN;
    
    dbcs_color(str,bt->red, bt->green, bt->blue);
    put_dbcs( str, breite, flags, txt ); 
    
    /*** Wert wieder zurueckschreiben ***/
    *ptr = str;                  
              
}
#endif

_dispatcher( void, bt_BTM_PUBLISH, void *nix)
{
/*
**  FUNCTION    Aufforderung an die Buttonliste, sich darzustellen
**              natürlich nur, wenn das Publish-Flag gesetzt ist
**
**              Neu: Sieht sich jetzt die textart an und verzweigt,
**              sofern es sich um einen DBCS-Text handelt. Bei grafik-
**              Buttons bleibt alles beim alten.
**
**  INPUT
**
**  RESULTS
**
**  CHANGED     8100 000C created march 96
*/

    struct button_data *btd;
    WORD   i;
    char   *ptr;
    struct drawtext_args dt;

    btd = INST_DATA( cl, o);
    ptr = output;


    if( btd->flags & BF_PUBLISH ) {

        /* ----------------------------------------------------------------
        ** Nun werden alle Button dargestellt. Es muß also der Font gesetzt 
        ** werden und dann muß (aus name oder inactiv) ein String gebastelt
        ** werden.
        ** --------------------------------------------------------------*/

        i=0;
        while( (btd->button[ i ]) && (i<MAXNUM_CLICKBUTTONS) ) {

            struct bt_button *bt;

            bt = btd->button[ i ];
            if( bt->flags & BT_PUBLISH ) { 
            
                /* ---------------------------------------------------------
                ** Neu: Verzweigen, wenn es sich um einen DBCS Text handelt.
                ** -------------------------------------------------------*/
                #ifdef __DBCS__
                if( bt->flags & BT_TEXT ) 
                    bt_DoTrueTypeRendering( btd, bt, &ptr );
                else
                    bt_DoVFMRendering( btd, bt, &ptr );
                #else
                bt_DoVFMRendering( btd, bt, &ptr );
                #endif         
                }
            i++;
            }     
            
        /*** und zeichnen ***/    
        eos( ptr);
        dt.string = output;
        dt.clips  = NULL;
        _DrawText( &dt );
        }
}


WORD bt_GetStringLen( struct VFMFont *font, char *text )
{
    /*** Berechnet die Länge eines Strings ***/
    WORD laenge = 0, i = 0;

    while( text[ i ] ) {

        laenge += font->fchars[ text[ i ] ].width;
        i++;
        }

    return( laenge );
}
