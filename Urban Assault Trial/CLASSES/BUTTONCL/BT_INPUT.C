/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  input-auswertung für button.class.
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


_dispatcher( LONG, bt_BTM_HANDLEINPUT, struct VFMInput *input )
{
/*
**  FUNCTION    wertet die Eingabe aus. Es schaltet die Pressed-Flags
**              und gibt die pressed- oder released-Werte zurück. Wenn
**              es nur ein String war, so ist es schon ausgeschaltet
**              durch die nicht vorhandene Höhe.
**  RESULT
**
**  INPUT       eine VFM-Input-Struktur
**
**  CHANGED     8100 000C march 96
*/

    struct button_data *btd;
    struct bt_button *bt;
    LONG   retvalue;
    int    g;
    BOOL   switch_off;
    struct bt_propinfo *prop;

    btd = INST_DATA( cl, o);
    switch_off = FALSE;
    retvalue   = 0;

    /*** Ein Mausereignis ? ***/
    if( input->ClickInfo.flags & CIF_VALID ) {
            
        /*** jedes MDOWN schaltet bei anderen das Flags, weil es ein neues ist ***/
        if( input->ClickInfo.flags & CIF_MOUSEDOWN ) {

            for( g=0; g<MAXNUM_CLICKBUTTONS; g++ )
                if( btd->button[ g ] )
                    btd->button[ g ]->flags &= ~BT_DOWN;
            }
            
        /*** Slider sind auch aktiv, wenn wir nicht drüber sind-->up alles aus ***/
        if( input->ClickInfo.flags & CIF_MOUSEUP ) {

            for( g=0; g<MAXNUM_CLICKBUTTONS; g++ ) {
                if( btd->button[ g ] ) {
                    if( BM_SLIDER == btd->button[ g ]->modus ) {
                        if( BT_DOWN & btd->button[ g ]->flags ) {

                            prop = (struct bt_propinfo *)
                                   btd->button[ g ]->specialinfo;
                            btd->button[ g ]->flags &= ~BT_DOWN;
                            btd->button[ g ]->flags &= ~BT_PRESSED;
                            prop->what = HIT_NOTHING;

                            /* --------------------------------------------
                            ** Weil es nur einen Slider geben kann, den wir
                            ** loslassen, gehen wir hier gleich raus!
                            ** ------------------------------------------*/
                            return( (LONG) ((btd->button[ g ]->ID << 16) |
                                             btd->button[ g ]->user_released));
                            }
                        }
                    }
                }
            }

        /* ------------------------------------------------------------------
        ** Spezielle Sachen für Gadgets. wenn wir nicht mehr über dem Gadget,
        ** welches down hatte, sind, schalten wir bei dem Gadget PRESSED aus,
        ** sonst ein. 
        ** ----------------------------------------------------------------*/
        if( input->ClickInfo.flags & CIF_MOUSEHOLD ) {

            if( (input->ClickInfo.box == &(btd->click)) &&
                (input->ClickInfo.btn != -1) &&
                (btd->button[ input->ClickInfo.btn ]->flags & BT_DOWN)) {

                if( btd->button[ input->ClickInfo.btn ]->modus == BM_GADGET )
                    btd->button[ input->ClickInfo.btn ]->flags |= BT_PRESSED;
                }
            else {

                /*** gedrückt, aber nicht über DOWN. Allen Gadgets Pressed weg ***/
                for( g=0; g<MAXNUM_CLICKBUTTONS; g++ ) {

                    if( btd->button[ g ] )
                        if( btd->button[ g ]->modus == BM_GADGET )
                            btd->button[ g ]->flags &= ~BT_PRESSED;
                    }
                }

            /* -------------------------------------------------------------
            ** Egal, wo wir sind, Slider funktionieren immer, deshalb müssen
            ** wir sie global bearbeiten
            ** -----------------------------------------------------------*/
            for( g=0; g<MAXNUM_CLICKBUTTONS; g++ ) {

                if( btd->button[ g ] ) {
                    if( BM_SLIDER == btd->button[ g ]->modus ) {

                        prop = (struct bt_propinfo *) btd->button[ g ]->specialinfo;

                        if( (btd->button[ g ]->flags & BT_DOWN) &&
                            (HIT_SLIDER == prop->what) ) {

                            LONG dv, moved_x;

                            /*** hold nur senden, wenn pessed war ***/
                            if( btd->button[ g ]->flags & BT_PRESSED )
                                retvalue = (btd->button[ g ]->ID << 16) | 
                                            btd->button[ g ]->user_hold;

                            /* ---------------------------------------------
                            ** Wenn vorher ein HitSlider war, dann wirkt die
                            ** relative x-Bewegung
                            ** -------------------------------------------*/
                            moved_x = input->ClickInfo.act.scrx - prop->down_x;

                            dv = moved_x * (prop->max - prop->min + 1) / 
                                 btd->button[ g ]->w;
                            prop->value = prop->mvalue + dv;
                            if( prop->value < prop->min ) 
                                prop->value = prop->min;
                            if( prop->value > prop->max )
                                prop->value = prop->max;
                            
                            /*** Grenzen neu berechnen ***/
                            bt_CalculateProp( btd, btd->button[g] );
                            }
                        }
                    }
                }
            }


        if( input->ClickInfo.box == &(btd->click) ) {

            /*** erstmal die Box getroffen ***/
            if( input->ClickInfo.flags & CIF_MOUSEDOWN )
                retvalue = PRA_CBOXHIT; // darf überlagert werden

            if( input->ClickInfo.btn != -1 ) {

                /* ------------------------------------------------------
                ** Wir haben was gültiges reinbekommen. Nun holen wir den
                ** Button, zu dem das Ereignis gehört und bearbeiten ihn
                ** entsprechend Status und Modus. 
                ** ----------------------------------------------------*/

                bt = btd->button[ input->ClickInfo.btn ];

                /* ----------------------------------------------------
                ** Es kann passieren, daß die Nummer nicht zur ClickBox
                ** paßt. Für den fall ein test
                ** --------------------------------------------------*/
                if( bt == NULL )
                    return( retvalue );

                /* -----------------------------------------------------------
                ** wir müssen auch ein "Maus-über" zurückliefern. Das machen
                ** wir hier: wir haben (noch) kein Ereignis, aber einen
                ** Button. somit füllen wir retvalue mit dem Sender, aber
                ** ohne Ereignis aus, auch für den Fall, daß wir CBOXHIT
                ** überschreiben. Jeder bearbeitete Button schreibt ja sowieso
                ** sein Ereignis mit Sender rein. Außerhalb heißt das dann:
                ** Sender ohne Ereignis( keine oberen 16 Bit): Maus über!
                ** Ereignis mit oder ohne Sender(immer unt 16B): Req->active!
                ** ---------------------------------------------------------*/
                retvalue = (bt->ID << 16);

                switch( bt->modus ) {

                    case BM_MXBUTTON:

                        /* -------------------------------------------------------
                        ** so etwas sollte nie allein vorkommen, denn diese lassen
                        ** sich nur durch andere mx-buttons ausschalten.
                        ** Wir schalten alle MX-Buttons aus
                        ** -----------------------------------------------------*/

                        if( input->ClickInfo.flags & CIF_BUTTONDOWN ) {

                            /*** alle anderen Button ausschalten (VORHER!) ***/
                            for( g=0; g<MAXNUM_CLICKBUTTONS; g++ ) {

                                if( btd->button[ g ] )
                                    if( btd->button[ g ]->modus == BM_MXBUTTON )
                                        btd->button[ g ]->flags &= ~(BT_PRESSED|BT_DOWN);
                                }

                            bt->flags |= (BT_PRESSED | BT_DOWN);

                            retvalue = (bt->ID << 16) | bt->user_pressed;
                            }
                        break;

                    case BM_SWITCH:

                        /* ----------------------------------------------------
                        ** Schalter. Bei einem MUP nach MDOWN wird umgeschalten
                        ** sonst nur merken
                        ** --------------------------------------------------*/

                        if( input->ClickInfo.flags & CIF_BUTTONDOWN ) {

                            /* --------------------------------------
                            ** wir dürfen was zurückmelden. Aber was?
                            ** Das BT_ON-Flag entscheidet.
                            ** ------------------------------------*/

                            if( bt->flags & BT_PRESSED ) {

                                /*** ausschalten ***/
                                retvalue = (bt->ID << 16) | bt->user_released;
                                bt->flags &= ~BT_PRESSED;
                                }
                            else {
                                retvalue = (bt->ID << 16) | bt->user_pressed;
                                bt->flags |= BT_PRESSED;
                                }
                            }

                        break;

                    case BM_GADGET:

                        /* --------------------------------------------------
                        ** Gadget, da brauchen wir nur Pressed zu merken. Der
                        ** Vollständigkeit halber mache ich beides.
                        ** ------------------------------------------------*/

                        if( input->ClickInfo.flags & CIF_BUTTONDOWN ) {

                            bt->flags |= BT_PRESSED;
                            bt->flags |= BT_DOWN;
                            retvalue = (bt->ID << 16) | bt->user_pressed;
                            }

                        if( input->ClickInfo.flags & CIF_BUTTONHOLD ) {

                            /*** hold nur senden, wenn pressed war ***/
                            if( bt->flags & BT_PRESSED )
                                retvalue = (bt->ID << 16) | bt->user_hold;
                            }

                        if( input->ClickInfo.flags & CIF_BUTTONUP ) {

                            /*** war vorher ein down? ***/
                            if( bt->flags & BT_DOWN ) {

                                /*** Soso, korrekt losgelassen ***/
                                bt->flags &= ~BT_PRESSED;
                                bt->flags &= ~BT_DOWN;
                                retvalue = (bt->ID << 16) | bt->user_released;
                                }
                            }

                        break;

                    case BM_SLIDER:

                        /* ------------------------------------------------------
                        ** funktioniert erstmal wie ein normales Gadget, nur
                        ** müssen wir unterscheiden, wo das Gadget getroffen 
                        ** wurde. Dazu merke ich mir Anfang und Ende des beweg-
                        ** baren Stückes relativ zum Button. Diese Werte aktuali-
                        ** siere ich immer wieder.
                        ** Move hat natürlich nur Sinn, wenn der mittlere Teil
                        ** getroffen wurde, auch das muß ich mir merken. Aus
                        ** den Werten bastele ich jedesmal einen neuen String
                        ** (einen Namen hat das Ding sowieso nicht).
                        **
                        ** Änderung: für das AmigaFeeling werden MouseHold und 
                        ** Mouseup global erledigt.
                        ** ----------------------------------------------------*/

                        prop = (struct bt_propinfo *) bt->specialinfo;

                        if( input->ClickInfo.flags & CIF_BUTTONUP ) {

                            /*** Wird global für alle erledigt ***/
                            }
                        else {
                            
                            if( input->ClickInfo.flags & CIF_BUTTONDOWN ) {

                                /*** Welcher teil wurde denn getroffen? ***/
                                if( prop->start > input->ClickInfo.down.btnx ) {

                                    prop->what = HIT_LESS;
                                    if( prop->value > prop->min ) prop->value--;
                                    }
                                else {
                                    if( prop->end < input->ClickInfo.down.btnx ) {

                                        prop->what = HIT_MORE;
                                        if( prop->value < prop->max ) prop->value++;
                                        }
                                    else {

                                        /*** hier wirkt erst die Bewegung ***/
                                        prop->what   = HIT_SLIDER;
                                        prop->mvalue = prop->value;

                                        /*** down wird seltsamerweise gelöscht ***/
                                        prop->down_x = input->ClickInfo.down.scrx;
                                        }
                                    }

                                bt->flags |= BT_PRESSED;
                                bt->flags |= BT_DOWN;
                                retvalue = (bt->ID << 16) | bt->user_pressed;
                                }
                            else {

                                if( input->ClickInfo.flags & CIF_BUTTONHOLD ) {

                                    retvalue = (bt->ID << 16) | bt->user_hold;
                                    }
                                }
                            }

                        /*** Grenzen neu berechnen ***/
                        bt_CalculateProp( btd, bt );

                        break;
                    }
                }
            }
        }

    /*** Ein Tastaturereignis ? ***/

    return( retvalue );
}


_dispatcher( LONG, bt_BTM_GETOFFSET, struct selectbutton_msg *sb )
{
/*
**  FUNCTION    Liefert offset zu einer ButtonID. Es ist von jetzt an nicht
**              mehr erlaubt, Button über ihre Nummer anzusprechen, weil
**              sich das ständig ändern kann.
**
**  RESULT      nummer oder -1, wenn es nicht gefunden wird
**
**  INPUT       ID
**
**  CHANGED     8100 000C created march 96
*/

    struct button_data *btd;
    WORD   i;

    btd = INST_DATA( cl, o);

    for( i=0; i<MAXNUM_CLICKBUTTONS; i++ ) {

        if( btd->button[ i ] ) {
            if( btd->button[i]->ID == sb->number )
                return( (LONG) i );
            }
        }

    return( -1L );
}

_dispatcher( LONG *, bt_BTM_GETSPECIALINFO, struct selectbutton_msg *sb )
{
/*
**  FUNCTION    liefert Specialinfo oder NULL, wenn diese bei dieser
**              Art Button nicht unterstützt wird.
**
**  INPUT       ID
**
**  CHANGED     heute
*/

    struct button_data *btd = INST_DATA( cl, o );
    LONG   offset;

    if( -1 != ( offset = (LONG) _methoda( o, BTM_GETOFFSET, sb )) ) {

        if( btd->button[ offset ]->modus == BM_SLIDER )
            return( btd->button[ offset ]->specialinfo );
        else
            return( NULL );

        }
    else
        return( NULL );
}


void bt_CalculateProp( struct button_data *btd, struct bt_button *bt )
{

    WORD c, i, breite, sliderpos;
    struct bt_propinfo *prop = (struct bt_propinfo *) bt->specialinfo;

    /*** Sicherheitstest ***/
    if( prop->value > prop->max ) prop->value = prop->max;
    if( prop->value < prop->min ) prop->value = prop->min;

    /*** Positionen neu ermitteln ***/
    breite = 1 + bt->w / ( prop->max - prop->min + 1);
    if( breite < MIN_SLIDER_WIDTH ) breite = MIN_SLIDER_WIDTH;

    sliderpos = 0.5 + ( bt->w - breite ) * (prop->value - prop->min) /
                      ( prop->max - prop->min );
    prop->start = sliderpos;
    if( prop->start < 0 ) prop->start = 0;
    prop->end   = prop->start + breite;
    if( prop->end > bt->w ) prop->end = bt->w;

    /*** String neu ermitteln ***/
    c = 0;
    if( prop->start > 0 ) {

        if( bt->flags & BT_BORDER ) {
            
            /*** mit Eröffnung ***/
            bt->unpressed_text[ c++ ] = btd->ch_bslideropen;

            if( prop->start > 1 ) {

                bt->unpressed_text[ c++ ] = CTRL_SEQ;
                bt->unpressed_text[ c++ ] = 0x11;
                bt->unpressed_text[ c++ ] = (UBYTE) (prop->start >> 8);
                bt->unpressed_text[ c++ ] = (UBYTE) (prop->start);
                bt->unpressed_text[ c++ ] = (UBYTE) btd->ch_bsliderback;
                }
            }
        else {

            bt->unpressed_text[ c++ ] = CTRL_SEQ;
            bt->unpressed_text[ c++ ] = 0x11;
            bt->unpressed_text[ c++ ] = (UBYTE) (prop->start >> 8);
            bt->unpressed_text[ c++ ] = (UBYTE) (prop->start);
            bt->unpressed_text[ c++ ] = (UBYTE) btd->ch_bsliderback;
            }
        }

    bt->unpressed_text[ c++ ] = btd->ch_bopen;

    bt->unpressed_text[ c++ ] = CTRL_SEQ;
    bt->unpressed_text[ c++ ] = 0x11;
    bt->unpressed_text[ c++ ] = (UBYTE) ((prop->end - 1) >> 8);
    bt->unpressed_text[ c++ ] = (UBYTE) (prop->end - 1);
    bt->unpressed_text[ c++ ] = (UBYTE) btd->ch_bfill;

    bt->unpressed_text[ c++ ] = btd->ch_bclose;

    if( bt->flags & BT_BORDER ) {

        if( prop->end < (bt->w - 1) ) {

            bt->unpressed_text[ c++ ] = CTRL_SEQ;
            bt->unpressed_text[ c++ ] = 0x11;
            bt->unpressed_text[ c++ ] = (UBYTE) ((bt->w - 1) >> 8);
            bt->unpressed_text[ c++ ] = (UBYTE) (bt->w - 1);
            bt->unpressed_text[ c++ ] = (UBYTE) btd->ch_bsliderback;
            
            /*** mit Abschluß ***/
            bt->unpressed_text[ c++ ] = btd->ch_bsliderclose;
            }
        }
    else {

        if( prop->end < bt->w ) {

            bt->unpressed_text[ c++ ] = CTRL_SEQ;
            bt->unpressed_text[ c++ ] = 0x11;
            bt->unpressed_text[ c++ ] = (UBYTE) ((bt->w) >> 8);
            bt->unpressed_text[ c++ ] = (UBYTE) (bt->w);
            bt->unpressed_text[ c++ ] = (UBYTE) btd->ch_bsliderback;
            }
        }

    bt->unpressed_text[ c++ ] = CTRL_SEQ;
    bt->unpressed_text[ c++ ] = 0;

}
