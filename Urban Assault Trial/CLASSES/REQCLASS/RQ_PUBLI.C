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

void rq_PaintRect( struct requester_data *rqd );

char buffer[ 2048 ];

_dispatcher( void, rq_RQM_PUBLISH, void *nix )
{
/*
**  FUNCTION    An alle kommt eine Darstellungsmiiteilung. Ob die das dann
**              auch machen, hängt von ihrem Zustand ab.
**
**  RESULT
**
**  INPUT
**
**  CHANGED     8100 000C march 96
*/

    struct requester_data *rqd;
    int    g;

    rqd = INST_DATA( cl, o);

    if( rqd->flags & RF_PUBLISH ) {

        /*** FensterHintergrund malen ***/
        if( !(rqd->flags & RF_ICONIFIED)) {
            
            rq_PaintRect( rqd );

            /*** Die internen ***/
            _methoda( rqd->requester, BTM_PUBLISH, NULL );

            for( g=0; g < rqd->number; g++ ) {

                /*** trotzdem nochmal Sicherheit ***/
                if( rqd->button[ g ].o )
                    _methoda( rqd->button[ g ].o, BTM_PUBLISH, NULL );
                }
            }
        else {

            if( rqd->icon )
                _methoda( rqd->icon, BTM_PUBLISH, NULL );
            }
        }

    return;
}


_dispatcher( void, rq_RQM_SWITCHPUBLISH, struct reqswitchpublish_msg *rsp)
{
/*
**  FUNCTION    Schaltet den Requester oder Teile davon (in)aktiv
**
**  INPUT       was und wie
**
**
*/

    struct requester_data *rqd;
    LONG   i, pos, mask;
    struct switchpublish_msg sp;
    struct selectbo_msg sbo;

    rqd = INST_DATA( cl, o);

    switch( rsp->modus ) {

        case RSP_REQPUBLISH:

            /* --------------------------------------------------------------
            ** Requester Sichtbar schalten. Dabei removen wir ihn und klinken
            ** ihn neu ein. Danach setzen wir ihn auf die erste Position. 
            ** Achtung! Der Requester kann sich im Iconify-Status befinden!
            ** ------------------------------------------------------------*/
            rqd->flags |= RF_PUBLISH;

            /*** Requester an sich? ***/
            if( rqd->flags & RF_ICONIFIED ) {

                sp.modus = SP_NOPUBLISH;
                _methoda( rqd->requester, BTM_SWITCHPUBLISH, &sp);
                if( rqd->icon )
                     _methoda( rqd->icon, BTM_SWITCHPUBLISH, &sp);

                if( rqd->icon ) {
                    
                    sp.modus = SP_PUBLISH;
                    _methoda( rqd->icon, BTM_SWITCHPUBLISH, &sp);
                    }
                }
            else {

                sp.modus = SP_NOPUBLISH;
                _methoda( rqd->requester, BTM_SWITCHPUBLISH, &sp);
                if( rqd->icon )
                     _methoda( rqd->icon, BTM_SWITCHPUBLISH, &sp);
                
                sp.modus = SP_PUBLISH;
                _methoda( rqd->requester, BTM_SWITCHPUBLISH, &sp);

                /*** Alle gemerkten Button aufklappen ***/
                for( i = 0; i < NUM_BUTTONOBJECTS; i++ ) {

                    mask = 1L << i;

                    /*** war der mal offen? ***/
                    if( mask & rqd->iconify_backup ) {

                        /*** Gibts den auch ? ***/
                        if( rqd->button[ i ].o ) {
                            
                            sp.modus = SP_NOPUBLISH;
                            _methoda(rqd->button[ i ].o, BTM_SWITCHPUBLISH, &sp );
                            
                            sp.modus = SP_PUBLISH;
                            _methoda(rqd->button[ i ].o, BTM_SWITCHPUBLISH, &sp );
                            }
                        }
                    }
                }
            break;

        case RSP_REQNOPUBLISH:

            /*** alles wegschalten ***/
            rqd->flags &= ~RF_PUBLISH;
            sp.modus = SP_NOPUBLISH;

            _methoda( rqd->requester, BTM_SWITCHPUBLISH, &sp);
            if( rqd->icon )
                 _methoda( rqd->icon, BTM_SWITCHPUBLISH, &sp);

            for( i = 0; i < NUM_BUTTONOBJECTS; i++ ) {

                /*** Gibts den ? ***/
                if( rqd->button[ i ].o )
                    _methoda( rqd->button[ i ].o, BTM_SWITCHPUBLISH, &sp);
                }

            // rqd->iconify_backup = 0L;
            break;

        case RSP_ICONIFY:

            if( rqd->icon ) {

                /*** Alles aus und Icon an ***/
                sp.modus = SP_PUBLISH;
                _methoda( rqd->icon     , BTM_SWITCHPUBLISH, &sp);
                
                sp.modus = SP_NOPUBLISH;
                _methoda( rqd->requester, BTM_SWITCHPUBLISH, &sp);

                for( i = 0; i < NUM_BUTTONOBJECTS; i++ ) {

                    /*** Gibts den ? ***/
                    if( rqd->button[ i ].o )
                        _methoda( rqd->button[ i ].o, BTM_SWITCHPUBLISH, &sp);
                    }
                }
            break;

        case RSP_BTPUBLISH:

            /*** Buttonobjekt anknipsen ***/
            sbo.number = rsp->who;
            pos = _methoda( o, RQM_GETOFFSET, &sbo );

            if( (pos < 0) || (pos >= NUM_BUTTONOBJECTS) ||
                (rqd->button[ pos ].o == NULL ) )
                return;

            sp.modus = SP_PUBLISH;
            _methoda( rqd->button[ pos ].o, BTM_SWITCHPUBLISH, &sp );
            
            /*** merken ***/
            mask = 1L << pos;    // ID kann zu groß sein!
            rqd->iconify_backup |= mask;

            break;

        case RSP_BTNOPUBLISH:

            /*** Buttonobjekt ausknipsen ***/
            sbo.number = rsp->who;
            pos = _methoda( o, RQM_GETOFFSET, &sbo );

            if( (pos < 0) || (pos >= NUM_BUTTONOBJECTS) ||
                (rqd->button[ pos ].o == NULL ) )
                return;

            sp.modus = SP_NOPUBLISH;
            _methoda( rqd->button[ pos ].o, BTM_SWITCHPUBLISH, &sp );
            
            /*** merken ***/
            mask = 1L << pos;
            rqd->iconify_backup &= ~mask;

            break;
        }
}


void rq_PaintRect( struct requester_data *rqd )
{
/*
**  zeichnet ein Rechteck mit dem BTC_STRING_FONT.
*/

    WORD   value, count = 0;
    LONG   x, y, w, h;
    struct drawtext_args dt;
    BOOL   first;
    struct VFMFont *sfont;

    _method( rqd->requester, OM_GET, BTA_BoxX, &x, BTA_BoxY, &y,
                                     BTA_BoxW, &w, BTA_BoxH, &h,
                                     TAG_DONE);

    sfont = _GetFont( rqd->rq_font );

    /*** Korrekturen ***/
    //y += STAT_HEIGHT;
    //h -= STAT_HEIGHT;


    buffer[ count++ ] = CTRL_SEQ;
    buffer[ count++ ] = 8;
    buffer[ count++ ] = (char) rqd->rq_font;

    buffer[ count++ ] = CTRL_SEQ;
    value = x - rqd->screen_x/2;
    buffer[ count++ ] = 1;
    buffer[ count++ ] = (char) (value >> 8);
    buffer[ count++ ] = (char) value;
    
    buffer[ count++ ] = CTRL_SEQ;
    value = y - rqd->screen_y/2;
    buffer[ count++ ] = 2;
    buffer[ count++ ] = (char) (value >> 8);
    buffer[ count++ ] = (char) value;

    first      = TRUE;

    while( h > sfont->height ) {  // Fonthöhe immer gleich...

        /*** Eröffnung der Zeile ***/
        if( rqd->flags & RF_ACTIVE ) {
            
            if( first )
                buffer[ count++ ] = rqd->ac_rupleft;    // links oben
            else
                buffer[ count++ ] = rqd->ac_rleft;      // rand
            }
        else {
            
            if( first )
                buffer[ count++ ] = rqd->ch_rupleft;    // links oben
            else
                buffer[ count++ ] = rqd->ch_rleft;      // rand
            }

        /*** ZwischenStücke ***/
        buffer[ count++ ] = CTRL_SEQ;
        buffer[ count++ ] = 0x11;
        value = w - sfont->fchars[ rqd->ch_rright ].width;
        buffer[ count++ ] = (char) (value >> 8);
        buffer[ count++ ] = (char) (value);
        if( rqd->flags & RF_ACTIVE ) {

            if( first )
                buffer[ count++ ] = rqd->ac_rup;        // rand oben
            else
                buffer[ count++ ] = rqd->ac_rbackround; // frei
            }
        else {

            if( first )
                buffer[ count++ ] = rqd->ch_rup;        // rand oben
            else
                buffer[ count++ ] = rqd->ch_rbackround; // frei
            }

        if( rqd->flags & RF_ACTIVE ) {

            if( first )
                buffer[ count++ ] = rqd->ac_rupright;   // oben rechts
            else
                buffer[ count++ ] = rqd->ac_rright;     // rand
            }
        else {

            if( first )
                buffer[ count++ ] = rqd->ch_rupright;   // oben rechts
            else
                buffer[ count++ ] = rqd->ch_rright;     // rand
            }

        /*** new line ***/
        buffer[ count++ ] = CTRL_SEQ;
        buffer[ count++ ] = 7;
        
        first = FALSE;
        h -= sfont->height;
        }

    /*** Nun noch Rand der Rest-Höhe h ***/
    buffer[ count++ ] = CTRL_SEQ;
    buffer[ count++ ] = 0xe;
    buffer[ count++ ] = sfont->height - h;
    if( rqd->flags & RF_ACTIVE )
        buffer[ count++ ] = rqd->ac_rdownleft;          // unten links
    else
        buffer[ count++ ] = rqd->ch_rdownleft;          // unten links

    buffer[ count++ ] = CTRL_SEQ;
    buffer[ count++ ] = 0x11;
    value = w - sfont->fchars[ rqd->ch_rright ].width;
    buffer[ count++ ] = (char) (value >> 8);
    buffer[ count++ ] = (char) value;
    if( rqd->flags & RF_ACTIVE )
        buffer[ count++ ] = rqd->ac_rdown;              // Rand unten
    else
        buffer[ count++ ] = rqd->ch_rdown;              // Rand unten

    if( rqd->flags & RF_ACTIVE )
        buffer[ count++ ] = rqd->ac_rdownright;         // rechts unten
    else
        buffer[ count++ ] = rqd->ch_rdownright;         // rechts unten

    /*** Ende ***/
    buffer[ count++ ] = CTRL_SEQ;
    buffer[ count++ ] = 0;

    /*** Zeichnen ***/
    dt.string = buffer;
    dt.clips  = NULL;
    _DrawText( &dt );
}
