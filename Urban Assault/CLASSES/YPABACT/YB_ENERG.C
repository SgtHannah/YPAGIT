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
#include "input/input.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine




_dispatcher(void, yb_YBM_GENERALENERGY, struct trigger_logic_msg *msg)
{

/*
**  FUNCTION    Realisiert den Energieabzug bei Bewegung. Hier nur
**              in der Luft. F¸r Bodentypen m¸ssen die Subklassen
**              das halt ¸berladen. Auch Sektorbeachtung. Allgemein eben.
**
**  INPUTS      move, wegen der frame_time
**
**  CHANGED     created heute abend af
*/

    struct ypabact_data *ybd;
    LONG   multi;
    FLOAT  time;
    struct getrldratio_msg gr;


    ybd = INST_DATA( cl, o);
    multi = ybd->bact.Sector->EnergyFactor;

    /* -----------------------------------------------------------------------
    ** Wenn wir schon tot sind, d¸rfen wir uns nicht mehr aufladen. Das bringt
    ** die Energiebilanz durcheinander. 
    ** ---------------------------------------------------------------------*/
    
    if( ybd->bact.MainState == ACTION_DEAD ) return;

    /*** Auf grund von Rundungsfehlern muﬂ time groﬂ sein ***/
    if( (ybd->bact.internal_time - ybd->bact.time_energy) < 1500 ) return;

    time = (FLOAT) (ybd->bact.internal_time - ybd->bact.time_energy) / 1000.0;
    ybd->bact.time_energy = ybd->bact.internal_time;

    /*** Subfactor (Mikrowirkung) abfragen ***/
    gr.owner = ybd->bact.Sector->Owner;
    _methoda( ybd->world, YWM_GETRLDRATIO, &gr );
    multi = (LONG)( ((FLOAT) multi) * gr.ratio );

    /*** Zuerst das normale Aufladen ***/
    if( ybd->bact.Owner == ybd->bact.Sector->Owner )
        ybd->bact.Energy += ((ybd->bact.Maximum * time * multi)/7000);
    else
        ybd->bact.Energy -= ((ybd->bact.Maximum * time * multi)/7000);

    if( ybd->bact.Energy < 0 )
        ybd->bact.Energy = 0;
    if( ybd->bact.Energy > ybd->bact.Maximum ) 
        ybd->bact.Energy = ybd->bact.Maximum;
}


_dispatcher(void, yb_YBM_MODVEHICLEENERGY, struct modvehicleenergy_msg *mve)
{

/*
**  FUNCTION    Auﬂerplanm‰ﬂige Energieaenderung. Achtung, Reduktion
**              muss dann natuerlich negative Energie uebergeben werden
**
**  INPUT       energy - klar
**              flags:
**              RE_SHOT --> durch Beschuﬂ -->unter 0 DeathVis einschalten!
**
**  CHANGED     10-Oct-95   8100 000C  created
*/

    struct ypabact_data *ybd = INST_DATA(cl, o );
    struct ypaworld_data *ywd;
    struct setstate_msg state;
    BOOL   really_network = FALSE;
    BOOL   own_machine;
    struct ypamessage_vehicleenergy re;
    struct sendmessage_msg sm;


    /*** Ein Netzwerkspiel? ***/
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    if( ywd->playing_network ) really_network = TRUE;

    /*** eigene Maschine oder fremde (dann uebers Netz) ***/
    if( mve->killer ) {

        if(ybd->bact.Owner == mve->killer->Owner)
            own_machine = TRUE;
        else
            own_machine = FALSE;
        }
    else
        own_machine = TRUE; // weil ich nicht weiss, wie entscheiden...

    if( really_network && (FALSE == own_machine) ) {

        /* -------------------------------------------
        ** Message verschicken, sofern es kein Eigener
        ** war, denn den verwalten wir ja selbst!
        ** -----------------------------------------*/
        re.generic.message_id  = YPAM_VEHICLEENERGY;
        re.generic.owner       = ybd->bact.Owner;
        re.ident               = ybd->bact.ident;
        re.energy              = mve->energy;
        if( mve->killer ) {
            re.killer          = mve->killer->ident;
            re.killerowner     = mve->killer->Owner;
            }
        else {
            re.killer          = NULL;
            re.killerowner     = 0;
            }
        sm.receiver_id         = NULL;
        sm.receiver_kind       = MSG_ALL;
        sm.data                = &re;
        sm.data_size           = sizeof( re );
        sm.guaranteed          = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
    else {

        /*** Energie richtig abziehen ***/
        ybd->bact.Energy += mve->energy;

        /*** ist der Kandidat unter 0, dann setzeb wir den VP ***/
        if( ybd->bact.Energy <= 0 ) {

            struct setstate_msg state;

            /* ------------------------------------------------------
            ** killer immer merken, denn der Robo kann ja kurz danach
            ** an Energiemangel sterben
            ** ----------------------------------------------------*/
            ybd->bact.killer = mve->killer;

            /*** erst hier fallend machen! ***/
            ybd->bact.ExtraState &= ~EXTRA_LANDED;  // !!!

            /*** auf tot setzen ***/
            state.extra_on   = state.extra_off = 0;
            state.main_state = ACTION_DEAD;
            //_methoda( o, YBM_SETSTATE_I, &state);
            _methoda( o, YBM_SETSTATE, &state);

            /*** Gleich alle Abmeldung machen ***/
            _methoda( o, YBM_DIE, NULL );
            }
        }
}


_dispatcher(void, yb_YBM_MODSECTORENERGY, struct energymod_msg *mse)
{
    /*** Messagebedingte Kapselung der Welt-Methode ***/
    struct ypabact_data  *ybd = INST_DATA( cl, o );
    struct ypaworld_data *ywd;
    struct getsectorinfo_msg gsi;
    UBYTE  owner;

    /*** im Original immer selbst‰ndige Eigent¸merermittlung ***/
    mse->owner = 255;
    _methoda( ybd->world, YWM_MODSECTORENERGY, mse );

    /*** Sectordaten wegen Owner ***/
    gsi.abspos_x = mse->pnt.x;
    gsi.abspos_z = mse->pnt.z;
    if( _methoda( ybd->world, YWM_GETSECTORINFO, &gsi ) )
        owner = gsi.sector->Owner;
    else
        owner = 0;

    #ifdef __NETWORK__
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    if( ywd->playing_network ) {

        struct ypamessage_sectorenergy msm;
        struct sendmessage_msg sm;

        msm.generic.message_id = YPAM_SECTORENERGY;
        msm.generic.owner      = ybd->bact.Owner;
        msm.pos.x              = mse->pnt.x;
        msm.pos.y              = mse->pnt.y;
        msm.pos.z              = mse->pnt.z;
        msm.energy             = mse->energy;
        msm.sectorowner        = owner;
        if( mse->hitman )
            msm.hitman         = mse->hitman->ident;
        else
            msm.hitman         = 0L;  

        sm.receiver_id         = NULL;
        sm.receiver_kind       = MSG_ALL;
        sm.data                = &msm;
        sm.data_size           = sizeof( msm );
        sm.guaranteed          = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }
    #endif
}
