/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_force.c,v $
**  $Revision: 38.2 $
**  $Date: 1998/01/06 16:19:39 $
**  $Locker:  $
**  $Author: floh $
**
**  yw_force.c -- Forcefeedback-Modul für YPA.
**                Windows ONLY.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "input/ie.h"
#include "input/inputclass.h"
#include "input/idevclass.h"
#include "ypa/ypaworldclass.h"

_extern_use_input_engine

#ifdef __WINDOWS__

/*** HACK!!! ***/
#include "audio/milesengine.h"
extern struct MilesBase MilesBase1;
#define MilesBase MilesBase1

/*-----------------------------------------------------------------*/
void yw_FFControl(struct ypaworld_data *ywd, struct idev_ffcontrol_msg *ffm)
/*
**  FUNCTION
**      Wrapper-Funktion für die input.class Delegate-Methode,
**      mit der Force-Feedback-Methoden an das Wimp-Objekt
**      weitergegeben werden.
**
**  CHANGED
**      22-May-97   floh    created
*/
{
    if (ywd->InputObject) {
        struct inp_delegate_msg idm;
        idm.type   = ITYPE_WIMP;
        idm.num    = 0;
        idm.method = IDEVM_FFCONTROL;
        idm.msg    = ffm;
        _methoda(ywd->InputObject, IM_DELEGATE, &idm);
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_InitForceFeedback(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert die YPA-Parts des FF-Support (welch
**      ein Deutsch).
**
**  CHANGED
**      21-May-97   floh    created
*/
{
    /*** e paar Object-Pointer holen ***/
    _IE_GetAttrs(IET_Object,&(ywd->InputObject),TAG_DONE);
    ywd->FFTimeStamp  = 0;
    ywd->FFEngineType = -1;
    ywd->FFPeriod     = 0.0;
    ywd->FFMagnitude  = 0.0;
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void yw_KillForceFeedback(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt das ForceFeedback-Modul.
**
**  CHANGED
**      22-May-97   floh    created
*/
{
    struct idev_ffcontrol_msg ffm;
    ffm.type = IDEV_FFTYPE_ALL;
    ffm.mode = IDEV_FFMODE_END;
    ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
    yw_FFControl(ywd,&ffm);
}

/*-----------------------------------------------------------------*/
void yw_FFVehicleChanged(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Muß aufgerufen werden, wenn der User das Vehicle
**      wechselt, es werden dann passende Parameter
**      für die Engine-Vibration und die MaxRot-Trägheit
**      berechnet und gestartet.
**
**  CHANGED
**      22-May-97   floh    created
*/
{
    if (ywd->Prefs.valid) {
        if (ywd->Prefs.Flags & YPA_PREFS_JOYDISABLE) return;
        if (ywd->Prefs.Flags & YPA_PREFS_FFDISABLE)  return;
    };

    if (ywd->UVBact) {
        struct idev_ffcontrol_msg ffm;
        LONG type;
        float mass0,mass1;
        float per0,per1;
        float damp0,damp1;
        float mag0,mag1;
        float mrot0,mrot1;

        /*** TimeStamp mitschreiben ***/
        ywd->FFTimeStamp = ywd->TimeStamp;

        /*** alles Stop ***/
        ffm.type = IDEV_FFTYPE_ALL;
        ffm.mode = IDEV_FFMODE_END;
        ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
        yw_FFControl(ywd,&ffm);

        /*** welcher Effect-Typ? ***/
        switch(ywd->UVBact->BactClassID) {
            case BCLID_YPATANK:
            case BCLID_YPACAR:
                type = IDEV_FFTYPE_ENGINE_TANK;
                mass0 = 200.0;  mass1 = 500.0;
                mrot0 = 0.6;    mrot1 = 1.0;
                damp0 = 1.0;    damp1 = 0.1;
                mag0  = 0.3;    mag1  = 0.4;
                per0  = 1.0;    per1  = 0.0;
                break;

            case BCLID_YPABACT:
                type = IDEV_FFTYPE_ENGINE_HELI;
                mass0 = 300.0;  mass1 = 800.0;
                mrot0 = 1.0;    mrot1 = 2.0;
                damp0 = 1.0;    damp1 = 0.7;
                mag0  = 0.3;    mag1  = 0.5;
                per0  = 1.0;    per1  = 0.0;
                break;

            case BCLID_YPAFLYER:
                type = IDEV_FFTYPE_ENGINE_PLANE;
                mass0 = 200.0;  mass1 = 500.0;
                mrot0 = 1.0;    mrot1 = 2.0;
                damp0 = 1.0;    damp1 = 0.1;
                mag0  = 0.3;    mag1  = 0.75;
                per0  = 1.0;    per1  = 0.0;
                break;

            default:
                type = -1;
                break;
        };

        /*** neuen Force-Effekt einschalten ***/
        if (type != -1) {

            float mass_scaled;
            float mrot_scaled;
            float mag, per, damp;

            /*** Masse -> Engine-Magnitude ***/
            mass_scaled = (ywd->UVBact->mass - mass0) / (mass1-mass0);
            mag = (mass_scaled * (mag1-mag0)) + mag0;

            /*** Masse -> maximale Drehzahl ***/
            per = (mass_scaled * (per1-per0)) + per0;

            /*** MaxRot -> Damper-Wert ***/
            mrot_scaled = (ywd->UVBact->max_rot-mrot0) / (mrot1-mrot0);
            damp = (mrot_scaled * (damp1-damp0)) + damp0;

            /*** Bereichs-Überschreitung korrigieren ***/
            if      (mag  < mag0)  mag=mag0;
            else if (mag  > mag1)  mag=mag1;
            if      (per  < per1)  per=per1;
            else if (per  > per0)  per=per0;
            if      (damp < damp1) damp=damp1;
            else if (damp > damp0) damp=damp0;

            ywd->FFEngineType = type;
            ywd->FFPeriod     = per;
            ywd->FFMagnitude  = mag;

            ffm.type  = type;
            ffm.mode  = IDEV_FFMODE_START;
            ffm.power = mag;
            ffm.pitch = per;
            ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);

            ffm.type  = IDEV_FFTYPE_MAXROT;
            ffm.mode  = IDEV_FFMODE_START;
            ffm.power = damp;
            ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_FFTrigger(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Paßt die Motordrehzahl an.
**
**  CHANGED
**      22-May-97   floh    created
*/
{
    if (ywd->Prefs.valid) {
        if (ywd->Prefs.Flags & YPA_PREFS_JOYDISABLE) return;
        if (ywd->Prefs.Flags & YPA_PREFS_FFDISABLE)  return;
    };

    if (ywd->UVBact) {

        /*** Engine-Update passiert nur alle 1/4 Sekunden ***/
        if ((ywd->TimeStamp - ywd->FFTimeStamp) > 250) {

            ywd->FFTimeStamp = ywd->TimeStamp;

            /*** Fahrzeug tot? ***/
            if (ACTION_DEAD == ywd->UVBact->MainState) {
                struct idev_ffcontrol_msg ffm;
                ffm.type = ywd->FFEngineType;
                ffm.mode = IDEV_FFMODE_MODIFY;
                ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
                yw_FFControl(ywd,&ffm);
            } else {

                /*** Engine-Vibration speed-relativ ***/
                if (ywd->FFEngineType != -1) {

                    float rel_speed;
                    struct Bacterium *b;
                    struct idev_ffcontrol_msg ffm;

                    /*** relative Geschwindigkeit (zur Höchstgeschwindigkeit) ***/
                    b = ywd->UVBact;
                    rel_speed = abs(b->dof.v)/
                                (nc_sqrt(b->max_force*b->max_force-100*b->mass*b->mass)/
                                b->bu_air_const);
                    if (rel_speed < 0.0)      rel_speed=0.0;
                    else if (rel_speed > 1.0) rel_speed=1.0;

                    ffm.type  = ywd->FFEngineType;
                    ffm.mode  = IDEV_FFMODE_MODIFY;
                    ffm.power = ywd->FFMagnitude;
                    ffm.pitch = ywd->FFPeriod * rel_speed;
                    ffm.dir_x = ffm.dir_y = 0.0;
                    yw_FFControl(ywd,&ffm);
                };
            };
        };

        /*** Shake-FX-Update passiert in jedem Frame ***/
        if (MilesBase.top_shakefx[0]) {

            struct SoundSource *shk = MilesBase.top_shakefx[0];
            ywd->ActShakeFX = shk;

            /*** hat der Shake in diesem Frame gestartet? ***/
            if (shk->start_time == MilesBase.time_stamp) {

                /*** dann einen Shake-Force-Feedback-FX generieren ***/
                struct idev_ffcontrol_msg ffm;
                struct Bacterium *vb = ywd->UVBact;
                struct flt_m3x3 *m = &(vb->dir);

                ffm.type  = IDEV_FFTYPE_SHAKE;
                ffm.mode  = IDEV_FFMODE_START;
                ffm.power = shk->shk_actmag;
                if (ffm.power > 1.0) ffm.power = 1.0;
                ffm.pitch = shk->shk->time;      // LifeTime!
                ffm.dir_x = m->m11 * (shk->sc->pos.x-vb->pos.x) +
                            m->m12 * (shk->sc->pos.y-vb->pos.y) +
                            m->m13 * (shk->sc->pos.z-vb->pos.z);
                ffm.dir_y = -(m->m31 * (shk->sc->pos.x-vb->pos.x)+
                              m->m32 * (shk->sc->pos.y-vb->pos.y)+
                              m->m33 * (shk->sc->pos.z-vb->pos.z));
                if (ffm.pitch > 0.0) {
                    yw_FFControl(ywd,&ffm);
                };
            };
        } else {
            ywd->ActShakeFX = NULL;
        };
    };
}

/*-----------------------------------------------------------------*/
_dispatcher(void, yw_YWM_FORCEFEEDBACK, struct yw_forcefeedback_msg *msg)
/*
**  CHANGED
**      22-May-97   floh    created
**      26-May-97   floh    + Prefs-Handling
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);
    struct idev_ffcontrol_msg ffm;

    if (ywd->Prefs.valid) {
        if (ywd->Prefs.Flags & YPA_PREFS_JOYDISABLE) return;
        if (ywd->Prefs.Flags & YPA_PREFS_FFDISABLE)  return;
    };

    switch (msg->type) {
        case YW_FORCE_MG_ON:
            ffm.type = IDEV_FFTYPE_MGUN;
            ffm.mode = IDEV_FFMODE_START;
            ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);
            break;

        case YW_FORCE_MG_OFF:
            ffm.type = IDEV_FFTYPE_MGUN;
            ffm.mode = IDEV_FFMODE_END;
            ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);
            break;

        case YW_FORCE_MISSILE_LAUNCH:
            ffm.type = IDEV_FFTYPE_MISSLAUNCH;
            ffm.mode = IDEV_FFMODE_START;
            ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);
            break;

        case YW_FORCE_GRENADE_LAUNCH:
            ffm.type = IDEV_FFTYPE_GRENLAUNCH;
            ffm.mode = IDEV_FFMODE_START;
            ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);
            break;

        case YW_FORCE_BOMB_LAUNCH:
            ffm.type = IDEV_FFTYPE_BOMBLAUNCH;
            ffm.mode = IDEV_FFMODE_START;
            ffm.power = ffm.pitch = ffm.dir_x = ffm.dir_y = 0.0;
            yw_FFControl(ywd,&ffm);
            break;

        case YW_FORCE_COLLISSION:
            {
                struct idev_ffcontrol_msg ffm;
                struct Bacterium *vb = ywd->UVBact;
                struct flt_m3x3 *m = &(vb->dir);

                ffm.type  = IDEV_FFTYPE_COLLISSION;
                ffm.mode  = IDEV_FFMODE_START;
                ffm.power = msg->power;
                ffm.pitch = 0.0;
                /*** man beachte: in dir_x/dir_y wird NICHT der Richtungs- ***/
                /*** Vektor, sondern einfach X/Z-Position des Events       ***/
                ffm.dir_x = m->m11 * (msg->dir_x - vb->pos.x) +
                            m->m13 * (msg->dir_y - vb->pos.z);
                ffm.dir_y = -(m->m31 * (msg->dir_x - vb->pos.x)+
                              m->m33 * (msg->dir_y - vb->pos.z));
                              yw_FFControl(ywd,&ffm);
            };
            break;
    };
}

/*-----------------------------------------------------------------*/
#endif

