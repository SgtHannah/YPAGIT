/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Die Intelligenzen...
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


/* Brot-O-Typen */


_dispatcher( BOOL, yb_YBM_CANYOUDOIT, struct canyoudoit_msg *cydi)
{
/*
**  FUNCTION    Entscheidet, ob mein Geschwader (!!!) in der Lage ist,
**              etwas zu tun. Es gibt 2 Situationen, wo so etwas sinnvoll
**              sein kann: Vor und während eines Kampfes. Deshalb wird dies 
**              mit Hilfe des CYDI_WHILE_FIGHTING-Flags angegeben.
**
**              Zur Zeit ist das der Übersichtlichkeit wegen sehr simple
**
**  INPUT       Energy
**
**  RESULT      BOOL Ok, mach mr's (weiter)
**
**  CHANGED     13-Nov-95   created     8100 000C
*/

    struct ypabact_data *ybd;
    struct sumparameter_msg sum;
    WORD   agf;

    ybd = INST_DATA( cl, o );

    if( ybd->bact.Aggression >= 100 )   return( TRUE );     // klar

    /*** Aggressionfactor ***/
    agf = ybd->bact.Aggression / 15;

    sum.value = 0;
    sum.para  = PARA_ENERGY;
    _methoda( o, YBM_SUMPARAMETER, &sum );

    if( cydi->flags & CYDI_WHILE_FIGHTING ) {

        if( cydi->energy > ( agf * sum.value ) )
            return( FALSE );
        else
            return( TRUE );
        }
    else {

        if( cydi->energy > sum.value )
            return( FALSE );
        else
            return( TRUE );
        }
}

