/*
**  $Source: $
**  $Revision: 38.1 $
**  $Date: 1995/06/12 18:24:47 $
**  $Locker: $
**  $Author: $
**
**  SetState für Raketen
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
#include "audio/audioengine.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine

_dispatcher( void, ym_YBM_SETSTATE, struct setstate_msg *msg)
{
    
    struct ypamissile_data *ymd;
    BOOL   ret;

    ymd = INST_DATA( cl, o );

    ret = _methoda( o, YBM_SETSTATE_I, msg );

    /*** Raketen-SETSTATES gehen nicht mehr uebers Netz ***/
}


_dispatcher( BOOL, ym_YBM_SETSTATE_I, struct setstate_msg *msg)
{
    /* 
    ** Wir überlagern SetState, weil wir für raketen ander geräusche nehmen.
    ** Außerdem kommen CREATE und WAIT bei Raketen nicht vor, also nehmen
    ** wir das Zeug gleich raus
    ** dead und megadeth erfordern die gleichen Geräusche, 's sind ja Explosionen
    */


    struct ypamissile_data *ymd;
    BOOL   ret = FALSE;

    ymd = INST_DATA( cl, o );
    

    /*** alles ausschalten, denn RaketenSounds überlagern sich nicht ***/
    _EndSoundSource(&(ymd->bact->sc), WP_NOISE_HIT);
    _EndSoundSource(&(ymd->bact->sc), WP_NOISE_NORMAL);
    _EndSoundSource(&(ymd->bact->sc), WP_NOISE_LAUNCH);


    if( msg->main_state != 0 )    ymd->bact->MainState = (UBYTE) msg->main_state;
    if( msg->extra_on   != 0 )    ymd->bact->ExtraState |=  msg->extra_on;
    if( msg->extra_off  != 0 )    ymd->bact->ExtraState &= ~(msg->extra_off);

    /*** Nun folgt eine Endlose Reihe von Ausnahmebehandlungen ***/
    if( msg->main_state == ACTION_DEAD ) {

        struct runeffects_msg re;

        /*** Der Visproto ***/
        _set( o, YBA_VisProto, ymd->bact->vis_proto_dead);
        _set( o, YBA_VPTForm,  ymd->bact->vp_tform_dead);
        
        /*** Der Sound, ist ein Einzelkracher ***/
        _StartSoundSource(&(ymd->bact->sc), WP_NOISE_HIT);
        re.effects = DEF_Death;
        _methoda( o, YBM_RUNEFFECTS, &re );

        /*** Wegen Sounds ***/
        ymd->bact->dof.v = 0.0;

        ret = TRUE;
        }

    if( msg->main_state == ACTION_NORMAL ) {

        /*** Der Visproto ***/
        _set( o, YBA_VisProto, ymd->bact->vis_proto_normal);
        _set( o, YBA_VPTForm,  ymd->bact->vp_tform_normal);
        
        /*** Der Sound, ist kontinuierlich ***/
        _StartSoundSource(&(ymd->bact->sc), WP_NOISE_NORMAL);

        ret = TRUE;
        }



    /* 
    ** Alle ExtraOff vor (!!!) ExtraOn, denn VP werden sonst überschrieben.
    ** Also zuerst die Ausschalter ...  
    */

    /*** Hat ja keinen Sinn ***/
    if( msg->extra_off == EXTRA_MEGADETH ) {

        /*** Der Visproto ***/
        _set( o, YBA_VisProto, ymd->bact->vis_proto_normal);
        _set( o, YBA_VPTForm,  ymd->bact->vp_tform_normal);
        
        /*** Der Sound, ist kontinuierlich ***/
        _StartSoundSource(&(ymd->bact->sc), WP_NOISE_NORMAL);

        ret = TRUE;
        }

    /*** ... dann die Einschalter ***/


    if( msg->extra_on == EXTRA_MEGADETH ) {

        struct runeffects_msg re;

        ymd->bact->MainState = ACTION_DEAD;

        /*** Der Visproto ***/
        _set( o, YBA_VisProto, ymd->bact->vis_proto_megadeth);
        _set( o, YBA_VPTForm,  ymd->bact->vp_tform_megadeth);
        
        /*** Der Sound, ist kontinuierlich ***/
        _StartSoundSource(&(ymd->bact->sc), WP_NOISE_HIT);
        re.effects = DEF_Megadeth;
        _methoda( o, YBM_RUNEFFECTS, &re );

        /*** Wegen Sounds ***/
        ymd->bact->dof.v = 0.0;

        ret = TRUE;
        }

    return( TRUE );
}




