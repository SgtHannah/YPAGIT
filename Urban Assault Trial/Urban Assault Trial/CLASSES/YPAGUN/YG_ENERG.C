/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Das planm‰ﬂige und auﬂerplanm‰ﬂige Energieger¸mpel
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>
#include <stdlib.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypagunclass.h"
#include "input/input.h"


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine




_dispatcher(void, yg_YBM_GENERALENERGY, struct trigger_logic_msg *msg)
{

/*
**  FUNCTION    Guns laden sich nur auf, denn sie sterben mit dem Sektor
**              oder dem Robo
**
**  INPUTS
**
**  CHANGED     created heute abend af
*/

    struct ypagun_data *ygd;
    LONG  multi;
    FLOAT time;


    ygd = INST_DATA( cl, o);
    multi = ygd->bact->Sector->EnergyFactor;
    time = (FLOAT) msg->frame_time / 1000.0;

    /*
    ** Wenn wir schon tot sind, d¸rfen wir uns nicht mehr aufladen. Das bringt
    ** die Energiebilanz durcheinander. 
    */
    
    if( ygd->bact->MainState == ACTION_DEAD ) return;

    /*** Zuerst das normale Aufladen ***/
    if( ygd->bact->Owner == ygd->bact->Sector->Owner )
        ygd->bact->Energy += ((ygd->bact->Maximum * time * multi)/40000);
    
    if( ygd->bact->Energy > ygd->bact->Maximum )
        ygd->bact->Energy = ygd->bact->Maximum;
}


