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


_dispatcher( BOOL, rq_RQM_NEWBUTTONOBJECT, struct newbuttonobject_msg *bo )
{
/*
**  FUNCTION    fügt dem Requester ein ButtonObject hinzu.
**
**  INPUT       Das Objekt höchstselbst
**
**  RESULT      TRUE, wenn wir es noch aufnehmen konnten
**
**  CHANGED     8100000C created in march 96
*/

    struct requester_data *rqd;

    rqd = INST_DATA( cl, o );

    /*** alles ok? ***/
    if( !bo ) return( FALSE );
    if( rqd->number == NUM_BUTTONOBJECTS ) return( FALSE );

    rqd->button[ rqd->number ].o  = bo->bo;
    rqd->button[ rqd->number ].ID = bo->ID;
    rqd->number++;
    
    return( TRUE );
}


_dispatcher( BOOL, rq_RQM_REMOVEBUTTONOBJECT, struct selectbo_msg *bo)
{
/*
**  FUNCTION    Nimmt ein ButtonObject wieder weg. Achtung! Evtl. müssen
**              wir es ausklinken!
**
**  INPUT       Nummer
**
**  RESULT      TRUE, wenn alles ok war
**
**  CHANGED     8100 000C march 96
**
*/

    struct requester_data *rqd;
    struct switchpublish_msg sp;
    int    i;
    WORD   pos;

    rqd = INST_DATA( cl, o);
    pos = _methoda( o, RQM_GETOFFSET, bo );

    /*** Alles ok? ***/
    if( (pos > NUM_BUTTONOBJECTS) || (pos < 0) ||
        (rqd->button[ pos ].o == NULL ) )
        return( FALSE );

    /*** Wegschalten ***/
    sp.modus = SP_NOPUBLISH;
    _methoda( rqd->button[ pos ].o, BTM_SWITCHPUBLISH, &sp );

    /*** Nachrutschen lassen ***/
    for( i = pos; i < (NUM_BUTTONOBJECTS-1); i++ )
        rqd->button[ i ] = rqd->button[ i+1 ];

    rqd->button[ NUM_BUTTONOBJECTS - 1 ].o  = NULL;
    rqd->button[ NUM_BUTTONOBJECTS - 1 ].ID = 0L;
    rqd->number--;

    return( TRUE );
}

