/*
**  $Source: $
**  $Revision: $
**  $Date: $
**  $Locker: $
**  $Author: $
**
**  input-auswertung für requester.class.
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
#include "requester/requesterclass.h"
#include "input/clickbox.h"

/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_ov_engine
_extern_use_input_engine

void rq_MoveRequester( struct requester_data *rqd, Object *o, struct VFMInput *input );


_dispatcher( LONG, rq_RQM_HANDLEINPUT, struct VFMInput *input )
{
/*
**  FUNCTION    wertet die Eingabe aus. Dabei gibt es die VFM-Input-
**              Struktur an alle Objekte weiter. Die Messages, die
**              die internen Objekte liefern, werte ich hier aus.
**  RESULT
**
**  INPUT       eine VFM-Input-Struktur
**
**  CHANGED     8100 000C march 96
*/

    struct requester_data *rqd;
    LONG   i, ret, retvalue = 0L;
    struct reqswitchpublish_msg rsp;
    //LONG   width, height, posx, posy, movex, movey;

    rqd = INST_DATA( cl, o);

    /* ------------------------------------------------------------------------
    ** Nun an alle übergeben und gucken, ob der Rückkehrwert eine Requester-
    ** aktion ist. Dann auswerten, ansonsten nur zurückgeben. Falls wir irgend-
    ** was finden, brechen wir sofort ab...
    ** Wir fragen erst und werten dann gemeinschaftlich aus, weil auch fremde
    ** Buttons was internes melden können!
    ** ----------------------------------------------------------------------*/

    /*** Der requester? ***/
    retvalue = _methoda( rqd->requester, BTM_HANDLEINPUT, input);
    
    /*** Das Icon? ***/
    if( !retvalue && rqd->icon )
        retvalue = _methoda( rqd->icon, BTM_HANDLEINPUT, input);
    
    /*** die fremden? ***/
    if( !retvalue ) {

        for( i=0; i<NUM_BUTTONOBJECTS; i++ ) {

            if( rqd->button[i].o )
                retvalue = _methoda( rqd->button[i].o, BTM_HANDLEINPUT, input);

            if( retvalue )  break;
            }
        }


    /*** MOVE-Flag bei jedem MOUSEUP aus! ***/
    if( (input->ClickInfo.flags & CIF_VALID) &&
        (input->ClickInfo.flags & CIF_MOUSEUP) )
        rqd->flags &= ~RF_MOVE;

    /*** Es kann auch eine bewegung sein. Dann ist das MOVE-Flag gesetzt! ***/
    if( rqd->flags & RF_MOVE )
        rq_MoveRequester( rqd, o, input );


    /*** Nun alle möglichen Ereignisse auswerten ***/
    ret = ((retvalue << 16) >> 16);
    switch( ret ) {

        case PRA_RQ_CLOSE:

            /*** Den Requester schließen ***/
            rsp.modus = RSP_REQNOPUBLISH;
            _methoda( o, RQM_SWITCHPUBLISH, &rsp );
            break;

        case PRA_RQ_PRESSMOVE:

            /*** nur Flag erstmal ***/
            rqd->flags |= RF_MOVE;

            /*** mouse.down ist nicht so gut... ***/
            rqd->last_mousex = input->ClickInfo.down.scrx;
            rqd->last_mousey = input->ClickInfo.down.scry;

            /*** bringt auch den Requester nach vorn ***/

            /*** Nach vorn schieben in Abfragekette ***/
            rsp.modus = RSP_REQNOPUBLISH;
            _methoda( o, RQM_SWITCHPUBLISH, &rsp );
            rsp.modus = RSP_REQPUBLISH;
            _methoda( o, RQM_SWITCHPUBLISH, &rsp );
            break;

        case PRA_RQ_RELEASEMOVE:

            /*** nur Flag erstmal ***/
            rqd->flags &= ~RF_MOVE;
            break;

        case PRA_RQ_ICONIFY:

            if( rqd->icon ) {

                /*** Das Icon einschalten ***/
                rqd->flags |= RF_ICONIFIED;

                /*** Requester und alle anderen weg ***/
                rsp.modus = RSP_ICONIFY;
                _methoda( o, RQM_SWITCHPUBLISH, &rsp );
                }
            break;

        case PRA_RQ_ICONPRESSED:

            /* ----------------------------------------------------------
            ** wenn wir das Icon irgendwann mal mit Doppelklick aufmachen
            ** dann hat das Sinn, weil wir den Biutton dann auch bewegen
            ** können
            ** --------------------------------------------------------*/
            break;

        case PRA_RQ_ICONRELEASED:

            if( rqd->icon ) {

                rqd->flags &= ~RF_ICONIFIED;
                rsp.modus = RSP_REQPUBLISH;
                _methoda( o, RQM_SWITCHPUBLISH, &rsp );
                }
            break;
        }
    return( retvalue );
}


_dispatcher( LONG, rq_RQM_GETOFFSET, struct selectbo_msg *sb )
{
/*
**  FUNCTION    Liefert offset zu einer ButtonObjectID. Es ist von jetzt an nicht
**              mehr erlaubt, ButtonObjects über ihre Nummer anzusprechen, weil
**              sich das ständig ändern kann.
**
**  RESULT      nummer oder -1, wenn es nicht gefunden wird
**
**  INPUT       ID
**
**  CHANGED     8100 000C created march 96
*/

    struct requester_data *rqd;
    WORD   i;

    rqd = INST_DATA( cl, o);

    for( i=0; i<NUM_BUTTONOBJECTS; i++ ) {

        if( rqd->button[i].ID == sb->number )
            return( (LONG) i );
        }

    return( -1L );
}


void rq_MoveRequester( struct requester_data *rqd, Object *o, struct VFMInput *input )
{
    struct movereq_msg mrm;

    /*** Bewegungsstück ermitteln ***/
    mrm.x     = input->ClickInfo.act.scrx - rqd->last_mousex;
    mrm.y     = input->ClickInfo.act.scry - rqd->last_mousey;
    mrm.flags = 0;

    _methoda( o, RQM_MOVEREQUESTER, &mrm );
}


_dispatcher( void, rq_RQM_POSREQUESTER, struct movereq_msg *mrm )
{
/*
**  FUNCTION    positioniert einen Requester an eine gewisse Position
**              Wenn diese nicht möglich ist, erledige ich das Maximum.
**
**  INPUT       position.
**
**  CHANGED     das ist ein Scheißabend heute
*/

    struct VFMFont *sfont;
    LONG   posx, posy;
    struct movereq_msg move;

    struct requester_data *rqd = INST_DATA( cl, o );

    sfont = _GetFont( rqd->rq_font );

    if( (rqd->flags & RF_ICONIFIED) && !rqd->icon ) return;

    /*** Position und Ausdehnung holen ***/
    if( rqd->flags & RF_ICONIFIED )
        _method( rqd->icon, OM_GET, BTA_BoxX, &posx,  BTA_BoxY, &posy,
                                    TAG_DONE);
    else
        _method( rqd->requester, OM_GET, BTA_BoxX, &posx,  BTA_BoxY, &posy,
                                         TAG_DONE);

    /*** Differenz ermitteln ***/
    move.x     = mrm->x - posx;
    move.y     = mrm->y - posy;
    move.flags = 0;

    _methoda( o, RQM_MOVEREQUESTER, &move );
}


_dispatcher( void, rq_RQM_MOVEREQUESTER, struct movereq_msg *mrm )
{
/*
**  FUNCTION    Wie pos, lediglich wird eine Differenz angegeben
**
**  CHANGED     dschäjnsched
*/
    LONG   g, width, height, posx, posy, movex, movey;
    LONG   font;
    struct VFMFont *sfont;

    struct requester_data *rqd = INST_DATA( cl, o );

    sfont = _GetFont( rqd->rq_font );

    if( (rqd->flags & RF_ICONIFIED) && !rqd->icon ) return;

    /*** Position und Ausdehnung holen ***/
    if( rqd->flags & RF_ICONIFIED )
        _method( rqd->icon, OM_GET, BTA_BoxX, &posx,  BTA_BoxY, &posy,
                                    BTA_BoxW, &width, BTA_BoxH, &height,
                                    TAG_DONE);
    else
        _method( rqd->requester, OM_GET, BTA_BoxX, &posx,  BTA_BoxY, &posy,
                                         BTA_BoxW, &width, BTA_BoxH, &height,
                                         TAG_DONE);


    /*** Bewegungsstück ermitteln ***/
    movex = mrm->x;
    movey = mrm->y;

    /*** Bewegung, wenn möglich ( kein Rand mehr!) ***/
    if( (posx + movex) < rqd->moveborder_l ) movex = rqd->moveborder_l - posx;
    if( (posx + width + movex) >= rqd->screen_x - rqd->moveborder_r )
         movex = rqd->screen_x - posx - width - rqd->moveborder_r;
    
    rqd->last_mousex += movex;
    if( rqd->flags & RF_ICONIFIED )
        _set( rqd->icon, BTA_BoxX, (posx + movex) );
    else {

        /*** auch Untergebene ! ***/
        _set( rqd->requester, BTA_BoxX, (posx + movex) );
        for( g = 0; g < NUM_BUTTONOBJECTS; g++ ) {

            if( rqd->button[ g ].o ) {
                _get( rqd->button[ g ].o, BTA_BoxX, &posx );
                _set( rqd->button[ g ].o, BTA_BoxX, (posx + movex));
                }
            }
        }

    if( (posy + movey) < rqd->moveborder_u ) movey = rqd->moveborder_u - posy;
    if( (posy + height + movey) >= (rqd->screen_y - rqd->moveborder_d) )
         movey = rqd->screen_y - posy - rqd->moveborder_d - height;
    
    rqd->last_mousey += movey;
    if( rqd->flags & RF_ICONIFIED )
        _set( rqd->icon, BTA_BoxY, (posy + movey) );
    else {

        /*** auch Untergebene ***/
        _set( rqd->requester, BTA_BoxY, (posy + movey) );
        for( g = 0; g < NUM_BUTTONOBJECTS; g++ ) {

            if( rqd->button[ g ].o ) {
                _get( rqd->button[ g ].o, BTA_BoxY, &posy );
                _set( rqd->button[ g ].o, BTA_BoxY, (posy + movey));
                }
            }
        }
}
