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
#include "nucleus/math.h"
#include "engine/engine.h"
#include "transform/tform.h"
#include "input/input.h"
#include "audio/audioengine.h"

#include "ypa/ypaworldclass.h"
#include "ypa/ypabactclass.h"

#ifdef __NETWORK__
#include "network/networkclass.h"
#include "ypa/ypamessages.h"
#endif


/*-----------------------------------------------------------------*/
_extern_use_nucleus
_extern_use_tform_engine
_extern_use_audio_engine


/* Brot-O-Typen */
void yb_rot_round_lokal_y( struct Bacterium *b, FLOAT angle );
void yb_RunEffect( struct ypabact_data *wirt, struct DestEffect *fx );


_dispatcher( void, yb_YBM_SETSTATE, struct setstate_msg *msg)
{
    /* Die Methode scheint auf den ersten Blick sinnlos, ist aber
    ** dann ganz brauchbar, wenn mit dem Umschalten eine Änderung
    ** des VisProtos notwendig wird 
    **
    ** Man sollte extra_on und extra_off nur dann gleichzeitig verwenden,
    ** wenn sicher ist, daß es keine Probleme mit den VP gibt. Am saubersten
    ** ist es, immer nur eine der 3 Sachen zu schalten. Für die Zustands-
    ** änderung ohne Visprotos braucht man die Methode ja nicht...
    **
    ** Weiterhin wird beim Umschalten von VisProtos getestet, ob diese
    ** schon gesetzt sind, damit Anims etc. nicht von vorn anfangen. 
    **
    ** Achtung, wir setzen die Sounds nur, wenn wir keine Rakete sind.
    ** Wir überlagern die Methode durch die MissileClass
    */
    struct ypabact_data *ybd;
    struct ypaworld_data *ywd;
    BOOL   ret;

    ybd = INST_DATA( cl, o );

    /*** Bodenfahrzeug-Hack (wegen Kamikaze) ***/
    if( ((BCLID_YPATANK == ybd->bact.BactClassID) ||
         (BCLID_YPACAR  == ybd->bact.BactClassID)) &&
        (msg->main_state == ACTION_DEAD) ) {

        struct setstate_msg state;

        state.main_state = state.extra_off = 0;
        state.extra_on   = EXTRA_MEGADETH;
        _methoda( o, YBM_SETSTATE, &state );
        return;
        }
    
    ret = _methoda( o, YBM_SETSTATE_I, msg );

    /* --------------------------------------------------------------------
    ** Dazu geht was übers Netz, wenn es kein Brocken ist und keine Rakete
    ** ist, denn Raketen werden nun auf der Schattenmaschine normal berech-
    ** net
    ** ------------------------------------------------------------------*/
    ywd = INST_DATA( ((struct nucleusdata *)ybd->world)->o_Class, ybd->world);
    if( ywd->playing_network && ret && (ybd->bact.Owner != 0) &&
        (BCLID_YPAMISSY != ybd->bact.BactClassID) ) {

        struct ypamessage_setstate ssm;
        struct sendmessage_msg sm;

        ssm.generic.message_id = YPAM_SETSTATE;
        ssm.generic.owner      = ybd->bact.Owner;
        ssm.ident              = ybd->bact.ident;
        ssm.main_state         = msg->main_state;
        ssm.extra_on           = msg->extra_on;
        ssm.extra_off          = msg->extra_off;
        ssm.class              = ybd->bact.BactClassID;

        sm.receiver_id         = NULL;
        sm.receiver_kind       = MSG_ALL;
        sm.data                = &ssm;
        sm.data_size           = sizeof( ssm );
        sm.guaranteed          = TRUE;
        _methoda( ywd->world, YWM_SENDMESSAGE, &sm );
        }

}


_dispatcher( BOOL, yb_YBM_SETSTATE_I, struct setstate_msg *msg)
{
    /* ---------------------------------------------------------------------
    ** Abgekapselte interne Version ohne Message verschicken, die von den
    ** Netzwerksachen direkt verwendet werden kann. So tritt keine rekursion
    ** auf.
    ** Wenn wirklich was gemacht wurde, geben wir TRUE zurück!!!
    ** -------------------------------------------------------------------*/
    struct ypabact_data *ybd;
    BOOL   ret = FALSE;
    struct runeffects_msg re;

    ybd = INST_DATA( cl, o );
    

    if( msg->main_state != 0 )    ybd->bact.MainState = (UBYTE) msg->main_state;
    if( msg->extra_on   != 0 )    ybd->bact.ExtraState |=  msg->extra_on;
    if( msg->extra_off  != 0 )    ybd->bact.ExtraState &= ~(msg->extra_off);

    /*** Nun folgt eine Endlose Reihe von Ausnahmebehandlungen ***/

    if( msg->main_state == ACTION_DEAD ) {

        /*** darf nur gesetzt werden, wenn noch nicht Megadeth oder death ***/
        if( !(( ybd->bact.vpactive == VP_JUSTDEATH ) ||
              ( ybd->bact.vpactive == VP_JUSTMEGADETH )) ) {

            /*** Visproto ***/
            ybd->vis_proto     = ybd->bact.vis_proto_dead;
            ybd->vp_tform      = ybd->bact.vp_tform_dead;
            ybd->bact.vpactive = VP_JUSTDEATH;

            /*** Wechen Netzwerch... ***/
            ybd->bact.Energy     = -10000;

            /*** Vorherigen ContSound ausschalten ***/
            if( ybd->bact.sound & (1L << VP_NOISE_FIRE) ) {

                if( ybd->flags & YBF_UserInput ) {

                    struct yw_forcefeedback_msg yffm;
                    yffm.type = YW_FORCE_MG_OFF;
                    _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
                    }

                _EndSoundSource(&(ybd->bact.sc), VP_NOISE_FIRE);
                ybd->bact.sound &= ~(1L << VP_NOISE_FIRE);
                }

            /*** Cockpit geräusch ausschalten ***/
            if( ybd->flags & YBF_UserInput )
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_COCKPIT );

            if( ybd->bact.sound & (1L << VP_NOISE_NORMAL) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_NORMAL);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_NORMAL);
                }

            if( ybd->bact.sound & (1L << VP_NOISE_GENESIS) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GENESIS);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GENESIS);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_WAIT) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_WAIT);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_WAIT);
                }

            /*** Sound ist abschmiergeln, Expl. kommt von Rakete ***/
            _StartSoundSource(&(ybd->bact.sc), VP_NOISE_GOINGDOWN );
            ybd->bact.sound |= (1L << VP_NOISE_GOINGDOWN);

            /*** Effekte abspielen ***/
            re.effects = DEF_Death;
            _methoda( o, YBM_RUNEFFECTS, &re );

            ret = TRUE;
            }
        }

    if( msg->main_state == ACTION_NORMAL ) {

        if( ybd->bact.vpactive != VP_JUSTNORMAL ) {

            /*** Der Visproto ***/
            ybd->vis_proto     = ybd->bact.vis_proto_normal;
            ybd->vp_tform      = ybd->bact.vp_tform_normal;
            ybd->bact.vpactive = VP_JUSTNORMAL;

            /*** Vorherigen ContSound ausschalten ***/
            if( ybd->bact.sound & (1L << VP_NOISE_GENESIS) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GENESIS);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GENESIS);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_WAIT) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_WAIT);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_WAIT);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_GOINGDOWN) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GOINGDOWN);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GOINGDOWN);
                }
            
            /*** Der Sound, ist kontinuierlich ***/
            if( !(ybd->bact.sound & (1L << VP_NOISE_NORMAL)) ) {

                ybd->bact.sound |= ( 1L << VP_NOISE_NORMAL);
                _StartSoundSource(&(ybd->bact.sc), VP_NOISE_NORMAL);
                }

            ret = TRUE;
            }
        }

    if( msg->main_state == ACTION_BEAM ) {

        /*** Wir beginnen diesen Zustand mit dem Normal-Visproto ***/
        if( ybd->bact.vpactive != VP_JUSTBEAM ) {

            /*** Der Visproto ***/
            ybd->vis_proto     = ybd->bact.vis_proto_create;
            ybd->vp_tform      = ybd->bact.vp_tform_create;
            ybd->bact.vpactive = VP_JUSTBEAM;

            /*** Vorherigen ContSound ausschalten ***/
            if( ybd->bact.sound & (1L << VP_NOISE_GENESIS) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GENESIS);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GENESIS);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_WAIT) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_WAIT);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_WAIT);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_GOINGDOWN) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GOINGDOWN);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GOINGDOWN);
                }
            
            /*** Der Sound, ist kontinuierlich (Genesis ist Notlösung) ***/
            if( !(ybd->bact.sound & (1L << VP_NOISE_BEAMIN)) ) {

                ybd->bact.sound |= ( 1L << VP_NOISE_BEAMIN);
                _StartSoundSource(&(ybd->bact.sc), VP_NOISE_BEAMIN);
                }

            /*** Effekte abspielen ***/
            re.effects = DEF_Beam;
            _methoda( o, YBM_RUNEFFECTS, &re );

            ret = TRUE;
            }
        }

    if( msg->main_state == ACTION_WAIT ) {

        if( ybd->bact.vpactive != VP_JUSTWAIT ) {
            
            /*** dr vp, ne ***/
            ybd->vis_proto     = ybd->bact.vis_proto_wait;
            ybd->vp_tform      = ybd->bact.vp_tform_wait;
            ybd->bact.vpactive = VP_JUSTWAIT;
            
            /*** Vorherigen ContSound ausschalten ***/
            if( ybd->bact.sound & (1L << VP_NOISE_NORMAL) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_NORMAL);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_NORMAL);
                }

            if( ybd->bact.sound & (1L << VP_NOISE_GENESIS) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GENESIS);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GENESIS);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_GOINGDOWN) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GOINGDOWN);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GOINGDOWN);
                }
            
            /*** Der Sound, ist kontinuierlich ***/
            if( !(ybd->bact.sound & (1L << VP_NOISE_WAIT)) ) {

                ybd->bact.sound |= (1L << VP_NOISE_WAIT);
                _StartSoundSource(&(ybd->bact.sc), VP_NOISE_WAIT);
                }

            ret = TRUE;
            }
        }

    if( msg->main_state == ACTION_CREATE ) {

        if( ybd->bact.vpactive != VP_JUSTCREATE ) {
            
            /*** eto visProto ***/
            ybd->vis_proto     = ybd->bact.vis_proto_create;
            ybd->vp_tform      = ybd->bact.vp_tform_create;
            ybd->bact.vpactive = VP_JUSTCREATE;

            /*** Vorherigen ContSound ausschalten ***/
            if( ybd->bact.sound & (1L << VP_NOISE_FIRE) ) {

                if( ybd->flags & YBF_UserInput ) {

                    struct yw_forcefeedback_msg yffm;
                    yffm.type = YW_FORCE_MG_OFF;
                    _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
                    }

                _EndSoundSource(&(ybd->bact.sc), VP_NOISE_FIRE);
                ybd->bact.sound &= ~(1L << VP_NOISE_FIRE);
                }

            if( ybd->bact.sound & (1L << VP_NOISE_NORMAL) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_NORMAL);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_NORMAL);
                }

            if( ybd->bact.sound & (1L << VP_NOISE_WAIT) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_WAIT);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_WAIT);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_GOINGDOWN) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GOINGDOWN);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GOINGDOWN);
                }
            
            /*** Der Sound, ist kontinuierlich ***/
            if( !( ybd->bact.sound & (1L << VP_NOISE_GENESIS)) ) {

                ybd->bact.sound |= (1L << VP_NOISE_GENESIS);
                _StartSoundSource(&(ybd->bact.sc), VP_NOISE_GENESIS);
                }

            /*** Effekte abspielen ***/
            re.effects = DEF_Create;
            _methoda( o, YBM_RUNEFFECTS, &re );

            ret = TRUE;
            }
        }

    /* 
    ** Alle ExtraOff vor (!!!) ExtraOn, denn VP werden sonst überschrieben.
    ** Also zuerst die Ausschalter ...  
    */
    
    if( msg->extra_off == EXTRA_FIRE ) {

        if( ybd->bact.vpactive == VP_JUSTFIRE ) {

            if( ybd->flags & YBF_UserInput ) {

                struct yw_forcefeedback_msg yffm;
                yffm.type = YW_FORCE_MG_OFF;
                _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
                }

            ybd->vis_proto     = ybd->bact.vis_proto_normal;
            ybd->vp_tform      = ybd->bact.vp_tform_normal;
            ybd->bact.vpactive = VP_JUSTNORMAL;

            /*** Sound ausschalten ***/
            _EndSoundSource(&(ybd->bact.sc), VP_NOISE_FIRE);
            ybd->bact.sound &= ~(1L << VP_NOISE_FIRE);

            ret = TRUE;
            }
        }

    if( msg->extra_off == EXTRA_MEGADETH ) {

        /*** Nur der Form halber, kann nicht passieren ***/
        if( ybd->bact.vpactive == VP_JUSTMEGADETH ) {

            ybd->vis_proto     = ybd->bact.vis_proto_normal;
            ybd->vp_tform      = ybd->bact.vp_tform_normal;
            ybd->bact.vpactive = VP_JUSTNORMAL;
            
            /*** kontinuierlicher Feuer-Sound ***/

            ret = TRUE;
            }
        }

    /*** ... dann die Einschalter ***/

    if( msg->extra_on == EXTRA_FIRE ) {

        if( ybd->bact.vpactive != VP_JUSTFIRE ) {

            /*** VisProto ***/
            ybd->vis_proto     = ybd->bact.vis_proto_fire;
            ybd->vp_tform      = ybd->bact.vp_tform_fire;
            ybd->bact.vpactive = VP_JUSTFIRE;
            
            /*** kontinuierlicher Feuer-Sound ***/
            if( !(ybd->bact.sound & (1L << VP_NOISE_FIRE)) ) {

                if( ybd->flags & YBF_UserInput ) {

                    struct yw_forcefeedback_msg yffm;
                    yffm.type = YW_FORCE_MG_ON;
                    _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
                    }

                ybd->bact.sound |= (1L << VP_NOISE_FIRE);
                _StartSoundSource(&(ybd->bact.sc), VP_NOISE_FIRE);
                }

            ret = TRUE;
            }
        }

    if( msg->extra_on == EXTRA_MEGADETH ) {

        /*
        ** Läßt sich nur einschalten, wenn vorher Tot (normal) war
        ** Quark! Immer ! 
        ** Sound ist Kollision mit Welt
        **
        ** Schaltet auf jeden Fall auch auf Tot, weil megadeth ohne
        ** dead gibt es nicht!
        */

        ybd->bact.MainState = ACTION_DEAD;

        if( ybd->bact.vpactive != VP_JUSTMEGADETH ) {

            ybd->vis_proto     = ybd->bact.vis_proto_megadeth;
            ybd->vp_tform      = ybd->bact.vp_tform_megadeth;
            ybd->bact.vpactive = VP_JUSTMEGADETH;

            /*** Vorherigen ContSound ausschalten ***/
            if( ybd->bact.sound & (1L << VP_NOISE_FIRE) ) {

                if( ybd->flags & YBF_UserInput ) {

                    struct yw_forcefeedback_msg yffm;
                    yffm.type = YW_FORCE_MG_OFF;
                    _methoda(ybd->world,YWM_FORCEFEEDBACK,&yffm);
                    }

                _EndSoundSource(&(ybd->bact.sc), VP_NOISE_FIRE);
                ybd->bact.sound &= ~(1L << VP_NOISE_FIRE);
                }

            /*** Ebenfalls CockpitSound aus ***/
            if( ybd->flags & YBF_UserInput )
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_COCKPIT );

            if( ybd->bact.sound & (1L << VP_NOISE_NORMAL) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_NORMAL);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_NORMAL);
                }

            if( ybd->bact.sound & (1L << VP_NOISE_GENESIS) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GENESIS);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GENESIS);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_WAIT) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_WAIT);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_WAIT);
                }
            
            if( ybd->bact.sound & (1L << VP_NOISE_GOINGDOWN) ) {

                ybd->bact.sound &= ~(1L << VP_NOISE_GOINGDOWN);
                _EndSoundSource( &(ybd->bact.sc), VP_NOISE_GOINGDOWN);
                }
            
            /*** einmaliger Crash-Sound ***/
            _StartSoundSource(&(ybd->bact.sc), VP_NOISE_EXPLODE);

            /*** Effekte abspielen ***/
            re.effects = DEF_Megadeth;
            _methoda( o, YBM_RUNEFFECTS, &re );

            /*** Wegen Sound? ***/
            ybd->bact.dof.v = 0.0;

            ret = TRUE;
            }
        }

    return( ret );
}



_dispatcher( void, yb_YBM_SUMPARAMETER, struct sumparameter_msg *sum)
{
    /*
    ** FUNCTION     Addiert verschiedene Werte des eigenen Objektes
    **              und seiner Untergebenen. Dient dazu, sich schnell
    **              einen Überblick über den Zustand eines Geschwaders
    **              zu verschaffen.
    **
    ** INPUT        sum->para       Was soll addiert/vermischt werden?
    **              sum->value = 0 vor erstem Aufruf!!!!
    **
    ** RESULTS      sum->value      Die Sache an sich als LONG
    **
    ** CHANGES      1-Oct-95     created af
    **              28-Mai-96    wenn ich tot bin, mißachte ich mich!!
    **
    */

    struct ypabact_data *ybd;
    struct OBNode *Untertan;
    struct OBNode *attacker;

    ybd = INST_DATA(cl, o );

    /* Zuerst an alle Untergebenen weitergeben, dann gebe ich selbst
    ** meinen Senf dazu. Ich gehe davon aus, daß das sum->value-Feld
    ** vorher richtig initialisiert  wurde (i.a. 0 ), so daß ich einfach
    ** aufaddieren kann 
    */


    Untertan = (struct OBNode *) ybd->bact.slave_list.mlh_Head;
    while( Untertan->nd.mln_Succ ) {

        _methoda( Untertan->o, YBM_SUMPARAMETER, sum );
        Untertan = (struct OBNode *) Untertan->nd.mln_Succ;
        }
    
    if( ybd->bact.MainState != ACTION_DEAD ) {

        switch( sum->para )  {

            case PARA_ENERGY:

                sum->value += (LONG) ybd->bact.Energy;
                break;

            case PARA_NUMBER:

                sum->value++;
                break;

            case PARA_SHIELD:

                sum->value += (LONG) ybd->bact.Shield;
                break;

            case PARA_MAXENERGY:

                sum->value += (LONG) ybd->bact.Maximum;
                break;

            case PARA_ATTACKER:

                attacker = (struct OBNode *) ybd->attck_list.mlh_Head;
                while( attacker->nd.mln_Succ ) {
                    sum->value++;
                    attacker = (struct OBNode *) attacker->nd.mln_Succ;
                    }
                break;
            }
        }
}




_dispatcher( BOOL, yb_YBM_TESTDESTROY, void *nix )
{
/*
**  FUNCTION    Testet, ob etwas zerstört ist. Das Problem ist doch folgendes:
**              Es stehen tote leute rum, die energetisch runter sind,
**              obwohl sie nie erschossen wurden. Das muß eine rakete testen!
**              Wenn der VP des zu testenden Objektes dead oder megadeth
**              ist, dann isser zerstört und wir können TRUE zurückgeben.
**
**  RESULTS     TRUE, wenn zerstört
**
**  INPUT       absolut nüscht, rein gar nix
**
**  CHANGED     23-12-95    8100 000C created   (bald is' Weihnachten)
**              september neu: kaputte können auch im createVisproto sein,
**                             nur tot müssen sie eben sein!
*/

    struct ypabact_data *ybd = INST_DATA(cl, o);

    if( ((ybd->vis_proto == ybd->bact.vis_proto_dead)   ||
         (ybd->vis_proto == ybd->bact.vis_proto_create) ||
         (ybd->vis_proto == ybd->bact.vis_proto_megadeth)) &&
        (ybd->bact.MainState == ACTION_DEAD) )
        return( TRUE );
    else
        return( FALSE );
}


_dispatcher( void, yb_YBM_BREAKFREE, struct trigger_logic_msg *msg)
{
/*
**  FUNCTION    Wenn sich jemand festgefressen hat, muß er irgendwie
**              freikommen. Deshalb schalten wir mal zuerst die
**              Bacterienkollision aus und wenn das nix genützt hat,
**              schleudern wir ihn etwas zurück.
**
**  INPUT       Zeiten mit der trigger_logic_msg
**
**  RESULT      nichts, hat ja auch keiner was erwartet!
**
**  CHANGED     ich, ganz alleine
*/

    FLOAT  distance;
    struct intersect_msg inter;
    struct ypabact_data *ybd = INST_DATA( cl, o );

    /* ---------------------------------------------------------------
    ** Wenn wir hierher gekommen sind, dann ist eine gewisse Zeit ver-
    ** gangen und wir setzen unabhängig vom Zustand die Flags zurück
    ** -------------------------------------------------------------*/
    if( ybd->bact.bfb_time > 0 ) {

        /*** runterzählen der "BactColl-Verboten-Zeit" ***/
        ybd->bact.bfb_time -= msg->frame_time;
        }

    /*** Für einfache Tests auf 0 biegen ***/
    if( ybd->bact.bfb_time < 0 ) ybd->bact.bfb_time = 0;

    if( (ybd->bact.backup_flags & YBF_BactCollision) && !ybd->bact.bfb_time)
        ybd->flags |= YBF_BactCollision;

    if( (ybd->bact.MainState == ACTION_NORMAL) &&
        (!(ybd->flags & YBF_UserInput)) ) {

        /*** Action_normal heißt, daß wir ein Ziel haben ***/

        distance = nc_sqrt( (ybd->bact.mpos.x - ybd->bact.pos.x) *
                            (ybd->bact.mpos.x - ybd->bact.pos.x) +
                            (ybd->bact.mpos.y - ybd->bact.pos.y) *
                            (ybd->bact.mpos.y - ybd->bact.pos.y) +
                            (ybd->bact.mpos.z - ybd->bact.pos.z) *
                            (ybd->bact.mpos.z - ybd->bact.pos.z));

        if( distance < 12 ) {

            /*** Nicht bewegt. Zuerst schalten wir die BactCollision aus ***/
            if( ybd->flags & YBF_BactCollision ) 
                ybd->bact.backup_flags |= YBF_BactCollision;
            
            if( (ybd->bact.internal_time - ybd->bact.bf2_time) > (2 * BF_TIME) ) {

                /*** Nun je nach Klasse ***/
                switch( ybd->bact.BactClassID ) {

                    case BCLID_YPATANK:
                    case BCLID_YPACAR:

                        /*** Speed hat keinen Sinn, einfach e bissel wegsetzen ***/
                        ybd->bact.old_pos = ybd->bact.pos;
                        ybd->bact.pos.x += -ybd->bact.dir.m31 * 10;
                        ybd->bact.pos.y += -ybd->bact.dir.m32 * 10;
                        ybd->bact.pos.z += -ybd->bact.dir.m33 * 10;
                        _methoda( o, YBM_CORRECTPOSITION, NULL );

                        /*** etwas drehen ***/
                        yb_rot_round_lokal_y( &(ybd->bact), (FLOAT) 0.1 );
                        
                        /*** trotzdem ein test ***/
                        inter.pnt.x = ybd->bact.old_pos.x;
                        inter.pnt.y = ybd->bact.old_pos.y;
                        inter.pnt.z = ybd->bact.old_pos.z;
                        inter.vec.x = ybd->bact.pos.x - ybd->bact.old_pos.x;
                        inter.vec.y = ybd->bact.pos.y - ybd->bact.old_pos.y;
                        inter.vec.z = ybd->bact.pos.z - ybd->bact.old_pos.z;
                        inter.flags = INTERSECTF_CHEAT; // da Bodenfahrzeug
                        _methoda( ybd->world, YWM_INTERSECT, &inter );
                        if( inter.insect ) {

                            /*** etwas über ipnt setzen ***/
                            ybd->bact.pos.x = inter.ipnt.x;
                            ybd->bact.pos.y = inter.ipnt.y - 5;
                            ybd->bact.pos.z = inter.ipnt.z;
                            }
                        break;
                    }
                }


            return;
            }
        }

    /*** is alles ok ***/
    ybd->bact.mpos     = ybd->bact.pos;
    ybd->bact.bf2_time = ybd->bact.internal_time;
}


_dispatcher( BOOL, yb_YBM_HOWDOYOUDO, struct howdoyoudo_msg *how )
{
/*
**  FUNCTION    Die sache ist folgende: Ob wir flüchten sollen oder nicht,
**              hängt nicht von den Kräfteverhältnissen, sondern von unserer
**              Verfassung ab. Wir ermitteln somit die Prozentuale energetische
**              Sättigung und entscheiden dann. Ich gehe davon aus, daß die
**              Methode nur auf Commander angewendet wird.
**              Weiterhin bauen wir einen Triggereffekt ein (wenn ESCAPE schon
**              gesetzt ist)
**
**  RESULT      TRUE - uns geht es gut
**
**  CREATED     heute
*/

    struct ypabact_data *ybd;
    struct sumparameter_msg sum;
    FLOAT  energy, maximum, aggr_factor;

    ybd = INST_DATA( cl, o);

    /*** Gleich raus ?? ***/
    if( ybd->bact.Aggression == 100 ) return( TRUE );

    /*** Erstmal durchschnittliche Energie berechnen ***/
    sum.value = 0;
    sum.para  = PARA_ENERGY;
    _methoda( o, YBM_SUMPARAMETER, &sum );
    energy    = (FLOAT) sum.value;

    sum.value = 0;
    sum.para  = PARA_MAXENERGY;
    _methoda( o, YBM_SUMPARAMETER, &sum );
    maximum   = (FLOAT) sum.value;

    /*** durch Maximum dividieren, wir wollen ja was prozentuales ***/
    energy /= maximum;

    /*** Die Aggression verändert diesen Wert ***/
    aggr_factor = ((FLOAT) ybd->bact.Aggression) / 60.0;
    energy *= aggr_factor;

    /*** Für exaktere Auswertung außerhalb ***/
    if( how ) how->value = energy;

    if( ybd->bact.ExtraState & EXTRA_ESCAPE ) {

        /*** Wir flüchten schon und haben uns vielleicht erholt ***/
        if( energy > 0.5 )
            return( TRUE );
        else
            return( FALSE );
        }
    else {

        if( energy > 0.2 )
            return( TRUE );
        else
            return( FALSE );
        }
}


_dispatcher( void, yb_YBM_RUNEFFECTS, struct runeffects_msg *re )
{
/*
**  FUNCTION    spielt alle Effekte ab. Diese sind Zustandsabhängig.
**              Da alle Informationen dazu in der bact-Struktur stehen
**              brauche ich keine weiteren Informationen
**
**              ACHTUNG: Zustände müssen vorher korrekt gesetzt sein!
**                 "   : Zustände werden wie Einschüsse tot geboren!
**                 "   : Effekte sollten keine Effekte enthalten!
**
**  INPUT       steht alles in der Bactstruktur
**
**  OUTPUT      nüscht
*/

    int    i;
    ULONG  numdestfx;
    struct ypabact_data *ybd = INST_DATA( cl, o );

    if( !_methoda( ybd->world, YWM_ISVISIBLE, &(ybd->bact)) ) return;

    /*** Begrenzung holen ***/
    _get( ybd->world, YWA_NumDestFX, &numdestfx );

    /*** alle Effekte durchgehen ***/
    for( i = 0; (i < NUM_DESTFX) && (i < numdestfx); i++ ) {

        /*** gibt es den? ***/
        if( ybd->bact.DestFX[ i ].proto ) {

            struct DestEffect *fx = &( ybd->bact.DestFX[ i ] );

            /*** Nun Zustände und Flags vergleichen ***/
            if( (fx->flags    & DEF_Megadeth)  &&
                (re->effects == DEF_Megadeth) ) {

                yb_RunEffect( ybd, fx );
                }
            else {

                if( (fx->flags    & DEF_Death) &&
                    (re->effects == DEF_Death) ) {

                    yb_RunEffect( ybd, fx );
                    }
                else {

                    if( (fx->flags    & DEF_Create) &&
                        (re->effects == DEF_Create) ) {

                        yb_RunEffect( ybd, fx );
                        }
                    else {

                        if( (fx->flags    & DEF_Beam) &&
                            (re->effects == DEF_Beam) ) {

                            yb_RunEffect( ybd, fx );
                            }
                        }
                    }
                }
            }
        }
}


void yb_RunEffect( struct ypabact_data *wirt, struct DestEffect *fx )
{
    /*** Hier wird der eigentliche Effekt abgespielt ***/
    struct createvehicle_msg cv;
    Object *effect;

    cv.x    = wirt->bact.pos.x;
    cv.y    = wirt->bact.pos.y;
    cv.z    = wirt->bact.pos.z;
    cv.vp   = fx->proto;

    /*** Wenn es sich lohnt, dann Effect etwas nach außen setzen ***/
    if( wirt->bact.radius > 31 ) {

        FLOAT d = nc_sqrt( fx->relspeed.x * fx->relspeed.x + fx->relspeed.y *
                           fx->relspeed.y + fx->relspeed.z * fx->relspeed.z );
        if( d > 0.1 ) {

            struct flt_triple versatz;

            /*** Richtung ermitteln ***/
            d = 1 / d;
            versatz.x = fx->relspeed.x * d * wirt->bact.radius;
            versatz.y = fx->relspeed.y * d * wirt->bact.radius;
            versatz.z = fx->relspeed.z * d * wirt->bact.radius;

            /*** Aufaddieren ***/
            cv.x += versatz.x * wirt->bact.dir.m11 +
                    versatz.y * wirt->bact.dir.m12 +
                    versatz.z * wirt->bact.dir.m13;
            cv.y += versatz.x * wirt->bact.dir.m21 +
                    versatz.y * wirt->bact.dir.m22 +
                    versatz.z * wirt->bact.dir.m23;
            cv.z += versatz.x * wirt->bact.dir.m31 +
                    versatz.y * wirt->bact.dir.m32 +
                    versatz.z * wirt->bact.dir.m33;
            }
        }

    if( effect = (Object *)_methoda( wirt->world, YWM_CREATEVEHICLE, &cv ) ) {

        /* -----------------------------------------------------------
        ** einklinken in die Slaveliste des Robos, weil der garantiert
        ** noch existiert. Unsere Slavelist ist ja in den meisten
        ** Fällen schon aufgeräumt und getriggert werden muß das zeug
        ** trotzdem.
        ** Raketen haben auch einen gültigen Robopointer!
        ** ---------------------------------------------------------*/
        struct setstate_msg state;
        struct Bacterium *fbact;
        FLOAT  speed;

        /*** egal, alles an Welt wie beim Floh ***/
        _methoda( wirt->world, YWM_ADDCOMMANDER, effect );

        /*** Abtöten ***/
        state.main_state = ACTION_DEAD;
        state.extra_off  = state.extra_on = 0L;
        _methoda( effect, YBM_SETSTATE_I, &state ); // läuft ja auf jeder Maschine extra

        /*** Geschwindigkeit verpassen ***/
        _get( effect, YBA_Bacterium, &fbact );

        fbact->dof.x = wirt->bact.dir.m11 * fx->relspeed.x +
                       wirt->bact.dir.m12 * fx->relspeed.y +
                       wirt->bact.dir.m13 * fx->relspeed.z;
        fbact->dof.y = wirt->bact.dir.m21 * fx->relspeed.x +
                       wirt->bact.dir.m22 * fx->relspeed.y +
                       wirt->bact.dir.m23 * fx->relspeed.z;
        fbact->dof.z = wirt->bact.dir.m31 * fx->relspeed.x +
                       wirt->bact.dir.m32 * fx->relspeed.y +
                       wirt->bact.dir.m33 * fx->relspeed.z;

        if( fx->flags & DEF_Relative ) {

            /*** Objektspeed noch aufaddieren ***/
            fbact->dof.x += wirt->bact.dof.x * wirt->bact.dof.v;
            fbact->dof.y += wirt->bact.dof.y * wirt->bact.dof.v;
            fbact->dof.z += wirt->bact.dof.z * wirt->bact.dof.v;
            }

        /*** normieren ***/
        speed = nc_sqrt( fbact->dof.x * fbact->dof.x +
                         fbact->dof.y * fbact->dof.y +
                         fbact->dof.z * fbact->dof.z);

        if( speed > 0.001 ) {

            fbact->dof.v  = speed;
            fbact->dof.x /= speed;
            fbact->dof.y /= speed;
            fbact->dof.z /= speed;
            }
        }

}
