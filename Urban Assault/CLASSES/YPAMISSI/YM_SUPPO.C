/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  Schnickschnack u den Raketen
**
**  (C) Copyright 1995 by Andreas Flemming
*/
#include <exec/types.h>

#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"
#include "ypa/ypamissileclass.h"
#include "input/input.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine


_dispatcher( void, ym_YMM_RESETVIEWER, void *nix)
{
    /*
    ** Wenn wir Viewer waren, dann schalten wir den RifleMan auf Viewer
    */

    ULONG VIEW;
    struct ypamissile_data *ymd;

    ymd = INST_DATA(cl, o );

    _get( ymd->bact->BactObject, YBA_Viewer, &VIEW );

    if( VIEW ) {

        _set( ymd->bact->BactObject, YBA_Viewer, FALSE );
        _set( ymd->bact->BactObject, YBA_UserInput, FALSE );
        
        if( (ymd->rifle_man->MainState == ACTION_DEAD) &&
            ((ULONG)ymd->rifle_man->master > 3L ) ) {

            _set( ymd->rifle_man->master, YBA_Viewer, TRUE);
            _set( ymd->rifle_man->master, YBA_UserInput, TRUE);
            }
        else {

            _set( ymd->rifle_man->BactObject , YBA_Viewer, TRUE);
            _set( ymd->rifle_man->BactObject , YBA_UserInput, TRUE);
            }
        }
}


_dispatcher( void, ym_YMM_DOIMPULSE, void *nix )
{
/*
**  FUNCTION    Wendet auf alle Objekte im Sektor einen Impulse an.
**              Den Impulse selbst macht die ImpulseMethode.
**
**  INPUT/RESULT ---
**
**  CHANGED     8100 000C   15-Nov-95
*/

    struct ypamissile_data *ymd;
    struct Bacterium *kandidat;
    struct impulse_msg imp;

    #ifdef __NETWORK__
    struct ypaworld_data *ywd;
    struct ypamessage_impulse im;
    struct sendmessage_msg sm;
    #endif

    ymd = INST_DATA(cl, o );

    #ifdef __NETWORK__
    ywd = INST_DATA( ((struct nucleusdata *)(ymd->world))->o_Class,
                     ymd->world );
    #endif

    imp.pos       = ymd->bact->pos;
    imp.impulse   = ymd->bact->Energy;
    imp.miss      = ymd->bact->dof;
    imp.miss_mass = ymd->bact->mass;

    kandidat = (struct Bacterium *) ymd->bact->Sector->BactList.mlh_Head;
    while( kandidat->SectorNode.mln_Succ ) {

        /* ------------------------------------------------------------------
        ** Wenn es eine Sonderbehandlung für den Viewer gibt, dann evtl.hier.
        ** Weiterhin wird im Netzwerkspiel kein Impuls auf Schatten
        ** angewendet, sondern eine Message verschickt, die dann auch
        ** wieder etwas "YMM_DOIMPULSE-mäßiges" anwendet. So kann man dann
        ** sagen, im Netzwerkspiel wird DOIMPULSE nur auf Objekte angewendet,
        ** die den Owner des Spielers haben.
        ** ----------------------------------------------------------------*/

        /*** Impuls nur auf Luftfahrzeuge ***/
        if( (kandidat->BactClassID != BCLID_YPAMISSY) &&
            (kandidat->BactClassID != BCLID_YPAROBO) &&
            (kandidat->BactClassID != BCLID_YPATANK) &&
            (kandidat->BactClassID != BCLID_YPACAR)  &&
            (kandidat->BactClassID != BCLID_YPAGUN)  &&
            (kandidat->BactClassID != BCLID_YPAHOVERCRAFT) &&
            (!(kandidat->ExtraState & EXTRA_MEGADETH)) ) {

            BOOL   really = TRUE;

            #ifdef __NETWORK__
            if( ywd->playing_network && (ymd->bact->Owner != kandidat->Owner))
                really = FALSE;
            #endif

            if( really )
                _methoda( kandidat->BactObject, YBM_IMPULSE, &imp );
            }

        kandidat = (struct Bacterium *) kandidat->SectorNode.mln_Succ;
        }

    /*** Nun noch Message verschicken ***/
    #ifdef __NETWORK__
    if( ywd->playing_network ) {

        im.generic.message_id = YPAM_IMPULSE;
        im.generic.owner      = ymd->bact->Owner;
        im.ident              = ymd->bact->ident;
        im.pos                = ymd->bact->pos;
        im.impulse            = ymd->bact->Energy;
        im.miss               = ymd->bact->dof;
        im.miss_mass          = ymd->bact->mass;

        sm.receiver_id         = NULL;
        sm.receiver_kind       = MSG_ALL;
        sm.data                = &im;
        sm.data_size           = sizeof( im );
        sm.guaranteed          = TRUE;
        _methoda( ymd->world, YWM_SENDMESSAGE, &sm );
        }
    #endif
}


_dispatcher( void, ym_YBM_REINCARNATE, void *nix )
{
/*
**  FUNCTION    Uffräume
**
*/

    struct ypamissile_data *ymd;

    ymd = INST_DATA(cl, o);

    /*** nach oben ***/
    _supermethoda( cl, o, YBM_REINCARNATE, NULL);

    /*** auch sinnvolle Voreinstellungen ***/
    ymd->flags = 0;
    ymd->delay = 0;

    /*** Your LastSeconds ist anders ***/
    _set( o, YBA_YourLS, (LONG) YBA_YourLastSeconds_Missy_DEF );
}
