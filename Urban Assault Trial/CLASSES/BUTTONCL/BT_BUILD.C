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

void bt_CalculateProp( struct button_data *btd, struct bt_button *bt );


_dispatcher( BOOL, bt_BTM_NEWBUTTON, struct newbutton_msg *nb )
{
/*
**  FUNCTION    fügt einen neuen Button hinzu, wenn das möglich ist.
**              übergeben wird eine struktur mit den notwendigen Daten
**              Parallel dazu sollte man die Fonts initialisieren.
**              Der ClickButton muß alloziert werden!
**
**  INPUT       struct newbutton *
**
**  RESULT      TRUE, wenn alles ok war
**
**  CHANGED     created 8100 000C March 96
*/

    struct button_data *btd;
    struct bt_button *button;
    WORD   pos;

    btd = INST_DATA( cl, o);


    /*** Gibt es noch freie Plätze? ***/
    if( btd->number >= MAXNUM_CLICKBUTTONS ) return( FALSE );

    /* --------------------------------------------------------------------
    ** Nun Buttonstruktur ausfüllen. Das number-Feld wird erst hochgesetzt, 
    ** wenn sucher ist, daß alle Informationen korrekt waren und ein Button
    ** angemeldet werden konnte
    ** ------------------------------------------------------------------*/

    pos = btd->number;

    /*** Button für ClickInfo allozieren ***/
    if( !(btd->click.buttons[ pos ] = _AllocVec( sizeof( struct ClickButton),
                                                 MEMF_CLEAR|MEMF_PUBLIC)))
        return( FALSE );

    /*** Button für mich allozieren ***/
    if( !(btd->button[ pos ] = _AllocVec( sizeof( struct bt_button),
                                          MEMF_CLEAR|MEMF_PUBLIC)))
        return( FALSE );

    button = btd->button[ pos ];

    /*** Der Modus ***/
    if( (nb->modus != BM_GADGET) && (nb->modus != BM_SWITCH)   &&
        (nb->modus != BM_STRING) && (nb->modus != BM_MXBUTTON) &&
        (nb->modus != BM_SLIDER) )
        return( FALSE );
    else
        button->modus = nb->modus;


    /*** Der Text ***/
    if( button->modus != BM_SLIDER ) {

        /*** Text ist sinnvoll ***/
        if( nb->unpressed_text == NULL )
            return( FALSE );
        else
            strncpy( button->unpressed_text, nb->unpressed_text, BT_NAMELEN-1 );


        /*** PressedText, darf NULL sein ***/
        if( nb->pressed_text == NULL ) {

            /*** wir nehmen den normalen Namen ***/
            strcpy( button->pressed_text, button->unpressed_text );
            }
        else
            strncpy( button->pressed_text, nb->pressed_text, BT_NAMELEN-1 );
        }


    /*** Die Größe ***/
    button->x = nb->x;
    button->y = nb->y;
    button->w = nb->w;
    
    if( (button->x < 0) || (button->y < 0) || (button->w < 0) ||
        ((button->x + button->w) > btd->click.rect.w) )
        return( FALSE );


    /*** Shortcut ***/
    button->shortcut = nb->shortcut;

    /*** Aktionen ***/
    button->user_pressed  = nb->user_pressed;
    button->user_released = nb->user_released;
    button->user_hold     = nb->user_hold;

    /*** Color immer, wird aber nur bei DBCS ausgewertet ***/
    button->red   = nb->red;
    button->green = nb->green;
    button->blue  = nb->blue;

    /*** war alles ok ***/
    btd->number++;

    /* ----------------------------------------------------------------
    ** Nun müssen die Informatiomnen soweit wie notwendig in die Click-
    ** BoxStruktur übertragen werden. Das betrifft vor allem die Größe
    ** --------------------------------------------------------------*/

    btd->click.buttons[ pos ]->rect.x = button->x;
    btd->click.buttons[ pos ]->rect.y = button->y;
    btd->click.buttons[ pos ]->rect.w = button->w;
    if( button->modus == BM_STRING )
        btd->click.buttons[ pos ]->rect.h = 0;
    else {

        /*** Fonthöhe ist nicht fest ***/
        struct VFMFont *fn = _GetFont( nb->unpressed_font );
        btd->click.buttons[ pos ]->rect.h = fn->height;
        }
    
    btd->click.num_buttons++;
    button->ID = nb->ID;

    /* --------------------------------------------------------
    ** Noch die Fonts schalten, Da ja jetzt jeder Button eigene
    ** Fonts hat.
    ** ------------------------------------------------------*/
    button->pressed_font    = nb->pressed_font;
    button->unpressed_font  = nb->unpressed_font;
    button->disabled_font   = nb->disabled_font;


    /*** Publish schalten ***/
    button->flags  = (WORD) nb->flags;
    button->flags |= BT_PUBLISH;

    /*** erwarten wir Special Infos? ***/
    if( button->modus == BM_SLIDER ) {

        struct bt_propinfo *prop_my;
        struct bt_propinfo *prop_in = (struct bt_propinfo *) nb->specialinfo;
        if( !prop_in ) return( FALSE );

        prop_my = (struct bt_propinfo *) _AllocVec( sizeof( struct bt_propinfo ),
                                                    MEMF_PUBLIC | MEMF_CLEAR );
        if( !prop_my ) return( FALSE );

        *prop_my = *prop_in;
        button->specialinfo = (LONG *) prop_my;
        
        bt_CalculateProp( btd, button );
        }


    return ( TRUE );
}


_dispatcher( BOOL, bt_BTM_REMOVEBUTTON, struct selectbutton_msg *rm )
{
/*
**  FUNCTION    Removed einen BUtton. Das heißt natürlich
**              auch, daß die anderen nachrutschen müssen
**
**  INPUT       number , beginnend  bei 0
**
**  RESULT      TRUE, wenn es den Button gab
**
**  CHANGED     created 8100 000C March '96
*/

    struct button_data *btd;
    WORD   pos;

    btd = INST_DATA( cl, o);
    pos = _methoda( o, BTM_GETOFFSET, rm );

    /*** Button erlaubt ? ***/
    if( (btd->number < (pos + 1)) || (pos < 0) ) return( FALSE );

    /*** Freigeben der Buttonspeicher (2x!!) ***/
    _FreeVec( btd->button[ pos ]);
    _FreeVec( btd->click.buttons[ pos ]);

    /* --------------------------------------------------------------
    ** von hier an müssen alle nachfolgenden Button "runtergeschoben"
    ** werden. Das muß ich natürlich auch in der ClickBox tun!
    ** Nur die Pointer werden versetzt, der letzte muß aber gelöscht
    ** werden!
    ** ------------------------------------------------------------*/

    while( pos < (MAXNUM_CLICKBUTTONS-1) ) {

        /*** button[pos] = button[pos+1] ***/
        btd->button[ pos ] = btd->button[ pos + 1 ];

        btd->click.buttons[ pos ] = btd->click.buttons[ pos + 1 ];
        pos++;
        }

    /*** die anderen NULLen wurden schon runtergeschoben ***/
    btd->button[MAXNUM_CLICKBUTTONS-1]        = NULL;
    btd->click.buttons[MAXNUM_CLICKBUTTONS-1] = NULL;

    btd->number--;
    btd->click.num_buttons--;

    return( TRUE );
}



_dispatcher( BOOL, bt_BTM_ENABLEBUTTON, struct switchbutton_msg *sb)
{
/*
**  FUNCTION    Schaltet einen Button anwählbar, löscht also das Disable-Flag
**              und schreibt die Ausdehnung neu
**
**  RESULT      TRUE, wenn es den button gab
**
**  INPUT       nummer
**
**  CHANGED     8100 000C March 96
*/

    struct button_data *btd;
    WORD   pos;
    struct VFMFont *fnt;

    btd = INST_DATA( cl, o);

    pos = _methoda( o, BTM_GETOFFSET, sb );
    fnt = _GetFont(btd->button[pos]->pressed_font);

    /*** Button erlaubt? ***/
    if( (btd->number < (pos + 1)) || (pos < 0) ) return( FALSE );

    if( btd->button[ pos ]->modus != BM_STRING ) {

        /*** Ausdehnung ***/
        btd->click.buttons[ pos ]->rect.w = btd->button[ pos ]->w;
        btd->click.buttons[ pos ]->rect.h = fnt->height;
        }

    /*** Flag ***/
    btd->button[ pos ]->flags &= ~BT_DISABLED;
    btd->button[ pos ]->flags |= BT_PUBLISH;

    return( TRUE );
}



_dispatcher( BOOL, bt_BTM_DISABLEBUTTON, struct switchbutton_msg *sb)
{
/*
**  FUNCTION    Schaltet einen Button unwählbar, setzt also das Disable-Flag
**              und die Ausdehnung
**
**  RESULT      TRUE, wenn es den button gab
**
**  INPUT       nummer
**
**  CHANGED     8100 000C March 96
*/

    struct button_data *btd;
    WORD   pos;

    btd = INST_DATA( cl, o);

    pos = _methoda( o, BTM_GETOFFSET, sb );

    /*** Button erlaubt? ***/
    if( (btd->number < (pos + 1)) || (pos < 0)) return( FALSE );

    /*** Ausdehnung ***/
    btd->click.buttons[ pos ]->rect.w = 0;
    btd->click.buttons[ pos ]->rect.h = 0;

    /*** Flag ***/
    btd->button[ pos ]->flags |= BT_DISABLED;

    /*** evtl. unsichtbar schalten ***/
    if( !(sb->visible) )
        btd->button[ pos ]->flags &= ~BT_PUBLISH;

    return( TRUE );
}


_dispatcher( BOOL, bt_BTM_SETSTRING, struct setstring_msg *ss)
{
/*
**  FUNCTION    Setzt alle Strings neu.Sind Pointer NULL, werden die
**              üblichen Dfefaultwerte genommen.
**
**  RESULT      TRUE, wenn der name nicht NULL war
**
**  INPUT       mindestens der name, aber auch inaktiv und pressed
**
**  CHANGED     8100 000c march 96
*/

    struct button_data *btd;
    struct bt_button *bt;
    struct selectbutton_msg sb;
    WORD   pos;

    btd = INST_DATA( cl, o);

    sb.number = ss->number;
    pos = _methoda( o, BTM_GETOFFSET, &sb );

    if( (btd->number < (pos + 1)) || (pos < 0)) return( FALSE );

    bt = btd->button[ pos ];

    if( ss->unpressed_text ) {

        strncpy( bt->unpressed_text, ss->unpressed_text, BT_NAMELEN-1 );
    
        /*** PressedText, darf NULL sein ***/
        if( ss->pressed_text == NULL ) {

            /*** wir nehmen den normalen Namen ***/
            strcpy( bt->pressed_text, bt->unpressed_text );
            }
        else
            strncpy( bt->pressed_text, ss->pressed_text, BT_NAMELEN-1 );

        return( TRUE );
        }
    else {

        return( FALSE );
        }
}


_dispatcher( void, bt_BTM_SETSTATE, struct setbuttonstate_msg *sbs)
{
/*
**  FUNCTION    setzt einen Button auf gedrückt oder losgelassen.
**              STRING: gar nix
**              SWITCH: beides
**              GADGET: beides, auch bei MX, für Katastrophen bzgl. des
**              Sinns solcher Aktionen ist der Programmierer verantwortlich
**
**  INPUT       wer und was
**
**  CHANGED     ein Produkt aus dem Hause Flemming 17. 4. 96
*/

    struct button_data *btd;
    struct selectbutton_msg sb;
    LONG   g, pos;
    struct bt_button *bt;

    btd = INST_DATA( cl, o);

    /*** erstmal das Offset holen ***/
    sb.number = sbs->who;
    pos = _methoda( o, BTM_GETOFFSET, &sb );

    if( (btd->number < (pos + 1)) || (pos < 0)) return;

    bt = btd->button[ pos ];

    switch( bt->modus ) {

        case BM_MXBUTTON:

            /*** erstmal alle anderen ausschalten ***/
            for( g=0; g<MAXNUM_CLICKBUTTONS; g++ ) {

                if( btd->button[ g ] )
                    if( btd->button[ g ]->modus == BM_MXBUTTON )
                        btd->button[ g ]->flags &= ~(BT_PRESSED|BT_DOWN);
                }

        case BM_GADGET:
        case BM_SWITCH:

            /*** Nun mal gucken, was der jenige will. ***/
            switch( sbs->state ) {

                case SBS_PRESSED:

                    bt->flags |= (BT_PRESSED | BT_DOWN);
                    break;

                case SBS_UNPRESSED:

                    bt->flags &= ~(BT_PRESSED | BT_DOWN);
                    break;
                }
            
            break;

        default:

            /*** gar nix ***/
            break;
        }
}



_dispatcher( void, bt_BTM_REFRESH, struct selectbutton_msg *sb )
{
/*
**  FUNCTION    Sofern sinnvoll, wird der übergebene Button "neu durchgerechnet"
**
**  INPUT       ID
**
**  CHANGED     jdghdxiuehiudsng.,dbkjsygyiu fuzjrdrdjh7esyizr98ewjrewlörs
*/

    struct button_data *btd = INST_DATA( cl, o);
    LONG   offset;

    offset = _methoda( o, BTM_GETOFFSET, sb );

    if( -1 != offset ) {

        if( BM_SLIDER == btd->button[ offset ]->modus ) {

            /*** Das ist alles ***/
            bt_CalculateProp( btd, btd->button[ offset] );
            }
        }
}


_dispatcher( BOOL, bt_BTM_SETBUTTONPOS, struct setbuttonpos_msg *sbp)
{
/*
**  FUNCTION    veraendert eine Buttonposition bzw. Breite. Hoehe ist Font-
**              abhaengig und wird nicht geaendert. wird eine -1 uebergeben,
**              so wird der Wert nicht geaendert.
**
**  RESULT      TRUE, wenn es den button gab
**
**  INPUT       nummer
**
**  CHANGED     8100 000C March 96
*/

    struct button_data *btd;
    struct selectbutton_msg sb;
    WORD   pos;

    btd = INST_DATA( cl, o);
    
    sb.number = sbp->number;
    pos = _methoda( o, BTM_GETOFFSET, sbp );

    /*** Button erlaubt? ***/
    if( (btd->number < (pos + 1)) || (pos < 0)) return( FALSE );

    /*** Ausdehnung und Position aendern, wenn gewuenscht ***/
    if( sbp->x != -1 ) {
        btd->button[ pos ]->x = sbp->x;
        btd->click.buttons[ pos ]->rect.x = btd->button[pos]->x;
        }
        
    if( sbp->y != -1 ) {
        btd->button[ pos ]->y = sbp->y;
        btd->click.buttons[ pos ]->rect.y = btd->button[ pos ]->y;
        }
        
    if( sbp->w != -1 ) {
        btd->button[ pos ]->w = sbp->w;
        btd->click.buttons[ pos ]->rect.w = btd->button[ pos ]->w;
        }

    return( TRUE );
}


