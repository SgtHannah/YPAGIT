/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_vehicle.c,v $
**  $Revision: 38.20 $
**  $Date: 1998/01/06 16:29:53 $
**  $Locker:  $
**  $Author: floh $
**
**  Definitionen etc. für vordefinierte VehicleTypes.
**  Siehe auch deathcache.c
**
**  (C) Copyright 1995 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "engine/engine.h"
#include "audio/sampleclass.h"

#include "ypa/ypabactclass.h"
#include "ypa/ypaworldclass.h"
#include "ypa/yparoboclass.h"
#include "ypa/ypaflyerclass.h"
#include "ypa/ypagunclass.h"
#include "ypa/ypacarclass.h"

#include "yw_protos.h"

_extern_use_nucleus
_extern_use_audio_engine

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yw_YWM_CREATEVEHICLE, struct createvehicle_msg *msg)
/*
**  FUNCTION
**      siehe ypaworldclass.h
**
**  CHANGED
**      06-Dec-95   floh    created
**      12-Feb-96   floh    Andreas Weapon-Info-Zeug
**      19-Feb-96   heute hat die "Hüpf" Geburtstag, aber die kennt wieder keiner...
**                  trotzdem habe ich (8100 000C) die ViewerPos nachgerüstet.
**      01-Apr-96   floh    AF's letztes Update übernommen (ViewerX..Z raus)
**      26-Apr-96   floh    + Initialisiert die SoundCarrier-Struktur im
**                            Bakterium.
**      05-May-96   floh    + AF's Sound-Pitch und Sound-Volume-Backup
**                            übernommen
**      10-May-96   floh    + neue Noises: VP_NOISE_GOINGDOWN und
**                            VP_NOISE_COCKPIT
**      02-Aug-96   floh    + sdist_sector, sdist_bact
**      15-Aug-96   floh    + Paletten-FX-Initialisierung
**      10-Sep-96   floh    + Update von AF (gun_angle_user)
**      27-Sep-96   floh    + Shake-FX eingebunden
**      01-Oct-96   floh    + neue Shake-Parameter [x,y,z]
**      29-Oct-96   floh    + DestFX-Übergabe
**      20-Dec-96   floh    + FX-Initialisierung angepaßt an
**                            neues Sound-FX-Handling
**      02-Jan-96   floh    + Bugfix: falls Ext Smp Def, aber
**                            kein "konventionelles" Sample
**                            definiert war, wurden Vol, Pitch
**                            und LOOPDALOOP Flag nicht korrekt
**                            initialisiert
**      09-Apr-97   floh    + "num_weapons" für Mehrfachabschuß
**                            von Raketen
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + WP_Array nicht mehr global
**      21-Apr-97   floh    + Explode-FX (aufskalierende Todes-Effekte)
**      12-Jul-97   floh    + Scale-FX-Initialisierung hatte noch ne
**                            Macke.
**      21-Apr-98   floh    + RoboReloadConst
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    struct setposition_msg sp_msg;
    struct setstate_msg st_msg;
    Object *v_obj;
    struct Bacterium *v_bact;

    struct VehicleProto *vproto;

    if (msg->vp > NUM_VEHICLEPROTOS) return(NULL);
    vproto = &(ywd->VP_Array[msg->vp]);

    /*** Object erzeugen ***/
    if (v_obj = yw_PrivObtainBact(o, ywd, vproto->BactClassID)) {

        ULONG i;

        _get(v_obj, YBA_Bacterium, &v_bact);

        v_bact->Energy          = vproto->Energy;
        v_bact->Maximum         = vproto->Energy;
        v_bact->Shield          = vproto->Shield;
        v_bact->mass            = vproto->Mass;
        v_bact->max_force       = vproto->MaxForce;
        v_bact->max_rot         = vproto->MaxRot;
        v_bact->pref_height     = vproto->PrefHeight;
        v_bact->radius          = vproto->Radius;
        v_bact->viewer_radius   = vproto->ViewerRadius;
        v_bact->over_eof        = vproto->OverEOF;
        v_bact->viewer_over_eof = vproto->ViewerOverEOF;
        v_bact->air_const       = vproto->AirConst;
        v_bact->bu_air_const    = vproto->AirConst;
        v_bact->adist_sector    = vproto->ADist_Sector;
        v_bact->adist_bact      = vproto->ADist_Bact;
        v_bact->sdist_sector    = vproto->SDist_Sector;
        v_bact->sdist_bact      = vproto->SDist_Bact;
        v_bact->View            = vproto->View;
        v_bact->gun_radius      = vproto->GunRadius;
        v_bact->gun_power       = vproto->GunPower;
        v_bact->max_pitch       = vproto->MaxPitch;

        v_bact->TypeID    = msg->vp;
        v_bact->auto_ID   = vproto->Weapons[0];
        if (vproto->Weapons[0] != -1) {
            v_bact->auto_info = ywd->WP_Array[vproto->Weapons[0]].Flags;
        } else {
            v_bact->auto_info = 0;
        };
        v_bact->mg_shot   = vproto->MG_Shot;

        v_bact->firepos.x       = vproto->FireRelX;
        v_bact->firepos.y       = vproto->FireRelY;
        v_bact->firepos.z       = vproto->FireRelZ;
        v_bact->gun_angle       = vproto->GunAngle;
        v_bact->gun_angle_user  = vproto->GunAngle;
        v_bact->num_weapons     = vproto->NumWeapons;
        v_bact->kill_after_shot = vproto->KillAfterShot;

        v_bact->vis_proto_normal   = ywd->VisProtos[vproto->TypeNormal].o;
        v_bact->vp_tform_normal    = ywd->VisProtos[vproto->TypeNormal].tform;
        v_bact->vis_proto_fire     = ywd->VisProtos[vproto->TypeFire].o;
        v_bact->vp_tform_fire      = ywd->VisProtos[vproto->TypeFire].tform;
        v_bact->vis_proto_dead     = ywd->VisProtos[vproto->TypeDead].o;
        v_bact->vp_tform_dead      = ywd->VisProtos[vproto->TypeDead].tform;
        v_bact->vis_proto_wait     = ywd->VisProtos[vproto->TypeWait].o;
        v_bact->vp_tform_wait      = ywd->VisProtos[vproto->TypeWait].tform;
        v_bact->vis_proto_megadeth = ywd->VisProtos[vproto->TypeMegaDeth].o;
        v_bact->vp_tform_megadeth  = ywd->VisProtos[vproto->TypeMegaDeth].tform;
        v_bact->vis_proto_create   = ywd->VisProtos[vproto->TypeGenesis].o;
        v_bact->vp_tform_create    = ywd->VisProtos[vproto->TypeGenesis].tform;

        /*** DestFX ***/
        memcpy(v_bact->DestFX,vproto->DestFX,sizeof(v_bact->DestFX));

        /*** Explode-Special-Effects ***/
        memset(v_bact->vp_proto_fx,0,sizeof(v_bact->vp_proto_fx));
        memset(v_bact->vp_tform_fx,0,sizeof(v_bact->vp_tform_fx));
        v_bact->scale_start    = vproto->scale_start;
        v_bact->scale_speed    = vproto->scale_speed;
        v_bact->scale_accel    = vproto->scale_accel;
        v_bact->scale_duration = vproto->scale_duration;
        v_bact->scale_count    = 0;
        i=0;
        while (vproto->scale_type[i]) {
            v_bact->vp_proto_fx[i] = ywd->VisProtos[vproto->scale_type[i]].o;
            v_bact->vp_tform_fx[i] = ywd->VisProtos[vproto->scale_type[i]].tform;
            /*** falls mindestens 1 gegeben, Explode-Special-FX aktivieren ***/
            v_bact->ExtraState |= EXTRA_SPECIALEFFECT;
            i++;
        };

        /*** AUDIO ***/
        _InitSoundCarrier(&(v_bact->sc));

        /*** First-Hit-Laden der Samples dieses Vehicles ***/
        for (i=0; i<VP_NUM_NOISES; i++) yw_LoadSISamples(&(vproto->Noise[i]));

        /*** übetrage VProto-Noises ins Bakterium ***/
        for (i=0; i<VP_NUM_NOISES; i++) {

            struct SoundSource *src = &(v_bact->sc.src[i]);

            /*** Basis-Parameter ***/
            src->volume = vproto->Noise[i].volume;
            src->pitch  = vproto->Noise[i].pitch;

            /*** LoopDaLoop-Flag setzen bei Loop-Noises ***/
            switch(i) {
                case VP_NOISE_NORMAL:
                case VP_NOISE_FIRE:
                case VP_NOISE_WAIT:
                case VP_NOISE_GENESIS:
                case VP_NOISE_GOINGDOWN:
                case VP_NOISE_COCKPIT:
                    src->flags |= AUDIOF_LOOPDALOOP;
                    break;
            };

            /*** konventionelle Sound-FX ***/
            if (vproto->Noise[i].smp_object) {
                /*** übernehme Sample ***/
                _get(vproto->Noise[i].smp_object, SMPA_Sample, &(src->sample));
            } else {
                src->sample = NULL;
            };

            /*** Palette-FX ***/
            if (vproto->Noise[i].pfx.slot != 0) {
                src->flags |= AUDIOF_HASPFX;
                src->pfx    = &(vproto->Noise[i].pfx);
            } else {
                src->flags &= ~AUDIOF_HASPFX;
            };

            /*** Shake-FX ***/
            if (vproto->Noise[i].shk.slot != 0) {
                src->flags |= AUDIOF_HASSHAKE;
                src->shk    = &(vproto->Noise[i].shk);
            } else {
                src->flags &= ~AUDIOF_HASSHAKE;
            };

            /*** Extended Sample Definition ***/
            if (vproto->Noise[i].ext_smp.count > 0) {
                src->flags  |= AUDIOF_HASEXTSMP;
                src->ext_smp = &(vproto->Noise[i].ext_smp);
            } else {
                src->flags &= ~AUDIOF_HASEXTSMP;
            };
        };

        /*** Merken der Normallautstärke und -tonhöhe ***/
        v_bact->bu_pitch  = v_bact->sc.src[ VP_NOISE_NORMAL ].pitch;
        v_bact->bu_volume = v_bact->sc.src[ VP_NOISE_NORMAL ].volume;

        /*** zusätzliche Attribute setzen ***/
        _methoda(v_obj, OM_SET, &(vproto->MoreAttrs[0]));

        /*** Position setzen, per YBM_SETPOSITION ***/
        sp_msg.x = msg->x;
        sp_msg.y = msg->y;
        sp_msg.z = msg->z;
        sp_msg.flags = 0;
        _methoda(v_obj, YBM_SETPOSITION, &sp_msg);

        /*** FIXME: auf Genesis-Zustand initialisieren? ***/
        st_msg.extra_on  = 0;
        st_msg.extra_off = 0;
        st_msg.main_state = ACTION_NORMAL;
        _methoda(v_obj, YBM_SETSTATE_I, &st_msg);
    };

    /*** Ende ***/
    return(v_obj);
}

/*=================================================================**
**                                                                 **
**  VEHICLE PROTOTYPE SCRIPT PARSER                                **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
void yw_InitVhclTags(struct VehicleProto *vp)
/*
**  FUNCTION
**      Initialisiert das VehicleProto TagArray neu.
**
**  CHANGED
**      13-Nov-96   floh    created
*/
{
    vp->LastTag = &(vp->MoreAttrs[0]);
    vp->LastTag->ti_Tag = TAG_DONE;
}

/*-----------------------------------------------------------------*/
void yw_AddVhclTag(struct VehicleProto *vp, ULONG tag, ULONG data)
/*
**  FUNCTION
**      Fügt ein neues TagItem in das MoreAttrs Array
**      ein. Sollte bereits ein identisches
**      Tag existieren, wird dessen Data-Feld
**      modifiziert.
**      Das TagArray wird immer korrekt abgeschlossen.
**
**  CHANGED
**      13-Nov-96   floh    created
*/
{
    struct TagItem *ti;

    if (ti = _FindTagItem(tag,vp->MoreAttrs)) {
        /*** existiert bereits ***/
        ti->ti_Data = data;
    } else {
        /*** neues Tag anlegen ***/
        vp->LastTag->ti_Tag  = tag;
        vp->LastTag->ti_Data = data;
        vp->LastTag++;
        vp->LastTag->ti_Tag = TAG_DONE;
    };
}

/*-----------------------------------------------------------------*/
LONG yw_GetVhclFXType(UBYTE *str)
/*
**  FUNCTION
**      Returniert den entsprechenden VP_NOISE_#? Typ
**      für den übergebenen String (nur für VehicleProtos!).
**      Oder -1, falls der String falsch ist.
**
**  CHANGED
**      20-Dec-96   floh    created.
*/
{
    LONG i;

    if      (stricmp(str,"normal")==0)    i=VP_NOISE_NORMAL;
    else if (stricmp(str,"fire")==0)      i=VP_NOISE_FIRE;
    else if (stricmp(str,"wait")==0)      i=VP_NOISE_WAIT;
    else if (stricmp(str,"genesis")==0)   i=VP_NOISE_GENESIS;
    else if (stricmp(str,"explode")==0)   i=VP_NOISE_EXPLODE;
    else if (stricmp(str,"crashland")==0) i=VP_NOISE_CRASHLAND;
    else if (stricmp(str,"crashvhcl")==0) i=VP_NOISE_CRASHVHCL;
    else if (stricmp(str,"goingdown")==0) i=VP_NOISE_GOINGDOWN;
    else if (stricmp(str,"cockpit")==0)   i=VP_NOISE_COCKPIT;
    else if (stricmp(str,"beamin")==0)    i=VP_NOISE_BEAMIN;
    else if (stricmp(str,"beamout")==0)   i=VP_NOISE_BEAMOUT;
    else if (stricmp(str,"build")==0)     i=VP_NOISE_BUILD;
    else i = -1;

    return(i);
}

/*-----------------------------------------------------------------*/
ULONG yw_ParseVhclPalFX(struct VehicleProto *vp, UBYTE *kw, UBYTE *data)
/*
**  FUNCTION
**      Zerlegt einen Paletten-FX-Keyword-String (startet
**      mit pal_) in seine Komponenten und initialisiert
**      entsprechend den Paletten-FX.
**
**  CHANGED
**      20-Dec-96   floh    created
*/
{
    UBYTE temp_kw[32];
    UBYTE *str;

    /*** Sicherheits-Kopie des Keywords ***/
    strcpy(temp_kw,kw);
    str = strtok(temp_kw,"_");
    if (stricmp(str,"pal") == 0) {

        struct SoundInit *si;
        LONG i;

        /*** welcher Typ ? ***/
        str = strtok(NULL,"_");
        i = yw_GetVhclFXType(str);
        if (i != -1) si = &(vp->Noise[i]);
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** welcher Parameter? ***/
        str = strtok(NULL,"_");
        if      (stricmp(str,"slot")==0) si->pfx.slot = strtol(data,NULL,0);
        else if (stricmp(str,"mag0")==0) si->pfx.mag0 = atof(data);
        else if (stricmp(str,"mag1")==0) si->pfx.mag1 = atof(data);
        else if (stricmp(str,"time")==0) si->pfx.time = strtol(data,NULL,0);
        else return(PARSE_UNKNOWN_KEYWORD);

    } else return(PARSE_UNKNOWN_KEYWORD);

    /*** wenn hier angekommen, alles OK ***/
    return(PARSE_ALL_OK);
}

/*-----------------------------------------------------------------*/
ULONG yw_ParseVhclShkFX(struct VehicleProto *vp, UBYTE *kw, UBYTE *data)
/*
**  FUNCTION
**      Siehe yw_ParseVhclPalFX()
**
**  CHANGED
**      20-Dec-96   floh    created
*/
{
    UBYTE temp_kw[32];
    UBYTE *str;

    /*** Sicherheits-Kopie des Keywords ***/
    strcpy(temp_kw,kw);
    str = strtok(temp_kw,"_");
    if (stricmp(str,"shk") == 0) {

        struct SoundInit *si;
        LONG i;

        /*** welcher Typ ? ***/
        str = strtok(NULL,"_");
        i = yw_GetVhclFXType(str);
        if (i != -1) si = &(vp->Noise[i]);
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** welcher Parameter? ***/
        str = strtok(NULL,"_");
        if      (stricmp(str,"slot")==0) si->shk.slot = strtol(data,NULL,0);
        else if (stricmp(str,"mag0")==0) si->shk.mag0 = atof(data);
        else if (stricmp(str,"mag1")==0) si->shk.mag1 = atof(data);
        else if (stricmp(str,"time")==0) si->shk.time = strtol(data,NULL,0);
        else if (stricmp(str,"mute")==0) si->shk.mute = atof(data);
        else if (stricmp(str,"x")==0)    si->shk.x    = atof(data);
        else if (stricmp(str,"y")==0)    si->shk.y    = atof(data);
        else if (stricmp(str,"z")==0)    si->shk.z    = atof(data);
        else return(PARSE_UNKNOWN_KEYWORD);

    } else return(PARSE_UNKNOWN_KEYWORD);

    /*** wenn hier angekommen, alles OK ***/
    return(PARSE_ALL_OK);
}

/*-----------------------------------------------------------------*/
BOOL yw_ParseVhclSndFX(struct VehicleProto *vp, UBYTE *kw, UBYTE *data)
/*
**  FUNCTION
**      Siehe yw_ParseVhclPalFX()
**
**  CHANGED
**      20-Dec-96   floh    created
*/
{
    UBYTE temp_kw[32];
    UBYTE *str;

    /*** Sicherheits-Kopie des Keywords ***/
    strcpy(temp_kw,kw);
    str = strtok(temp_kw,"_");
    if (stricmp(str,"snd") == 0) {

        struct SoundInit *si;
        LONG i;

        /*** welcher Typ ? ***/
        str = strtok(NULL,"_");
        i = yw_GetVhclFXType(str);
        if (i != -1) si = &(vp->Noise[i]);
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** welcher Parameter? ***/
        str = strtok(NULL,"_");
        if (stricmp(str,"sample")==0){
            if (strlen(data) >= (sizeof(si->smp_name)-1)) {
                _LogMsg("ParseVhclSndFX(): Sample name too long!\n");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(si->smp_name,data);
        }else if (stricmp(str,"volume")==0) si->volume = strtol(data,NULL,0);
        else if (stricmp(str,"pitch")==0)  si->pitch  = strtol(data,NULL,0);
        else if (stricmp(str,"ext")==0) {
            /*** eine extended Sample-Definition ***/
            if (!yw_ParseExtSampleDef(si,data)) return(PARSE_BOGUS_DATA);
        } else return(PARSE_UNKNOWN_KEYWORD);

    } else return(PARSE_UNKNOWN_KEYWORD);

    /*** wenn hier angekommen, alles OK ***/
    return(PARSE_ALL_OK);
}

/*-----------------------------------------------------------------*/
ULONG yw_VhclProtoParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser für VehicleProto Descriptions
**      (new_vehicle, modifiy_vehicle Blöcke)
**
**  CHANGED
**      03-May-96   floh    created
**      04-May-96   floh    Änderungen bei Robo-VProto-Init:
**                          Der zu verwendende EXT_Array-Slot muß jetzt
**                          explizit per <robo_data_slot> Keyword
**                          definiert werden.
**      08-May-96   floh    Fix für Loop-Optimizer-Bug im Watcom (FUCK!)
**      10-May-96   floh    neue Geräusch-Keywords für die Geräusche
**                              VP_NOISE_GOINGDOWN
**                              VP_NOISE_COCKPIT
**                          Beide kontinuierlich, GoindDown ist der
**                          eigentliche Absturz (vor Megadeth), Cockpit
**                          ist eine alternatives Geräusch, wenn der
**                          User im Fahrzeug sitzt.
**      17-May-96   floh    Andreas' neues Robokollisions-Zeuch
**                          übernommen
**      17-Jun-96   floh    + <gun_type> [mg,flak,dummy] Keyword
**      20-Jul-96   floh    - robo_gun_weapon
**                          + robo_gun_type
**                          + robo_gun_name
**                          + robo_viewer_x..z
**                          + robo_viewer_max_#?
**      21-Jul-96   floh    + robo_does_twist
**                          + robo_does_flux
**      02-Aug-96   floh    + sdist_sector
**                          + sdist_bact
**      15-Aug-96   floh    + Paletten-FX-Init (pal_#?)
**      21-Sep-96   floh    + im Design-Modus werden alle
**                            abgehandelten Vehikel für den
**                            User enabled
**      27-Sep-96   floh    + Shake-FX-Init (shk_#?)
**      01-Oct-96   floh    + neue Shake-Keywords
**      29-Oct-96   floh    + <dest_fx> Keyword
**      31-Oct-96   floh    + alle Keywords für die FX BeamIn,
**                            BeamOut, Build
**      13-Nov-96   floh    + das gesamte MoreAttrs-Handling
**                            war bei modify_vehicle mehr oder
**                            weniger broken, also ist dieses
**                            jetzt etwas mehr advanced
**      20-Dec-96   floh    + Initialisierung der Snd-,Pal- und
**                            Shk-FX ausgelagert und modularer
**                            gemacht (etliche Zeilen Code gespart).
**      09-Apr-97   floh    + <num_weapons> für Mehrfachabschuß
**                            von Waffen
**      12-Apr-97   floh    + VP_Array nicht mehr global
**                          + EXT_Array nicht mehr global
**      21-Apr-97   floh    + Special-FX-Keywords (scale_fx)
**                          + "kamikaze"
**      24-Apr-97   floh    + "wireframe"
**      05-May-97   floh    + es gab ein Memory-Leak bei den
**                            Wireframe-Objects
**      13-Sep-97   floh    + es gab einen Zugriff aufs WP_Array
**                            mit Index -1 (vproto->Weapons[0] konnte
**                            auf -1 sein).
**      24-Sep-97   floh    + <vo_type> Keyword
**      01-Oct-97   floh    + <add_energy>, <add_shield>, <add_radar>
**      16-Dec-97   floh    + touch_stone Struktur wird ausgefuellt
**      29-Jun-98   floh    + zusaetzliche Vehikel-Wireframes
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;
    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        struct VehicleProto *vp = NULL;
        ULONG vp_num = 0;
        p->target = NULL;

        /*** NEW ***/
        if (stricmp(kw, "new_vehicle") == 0) {

            /*** neuer VehicleProto ***/
            vp_num = strtol(data, NULL, 0);
            if (vp_num >= NUM_VEHICLEPROTOS) return(PARSE_BOGUS_DATA);
            else {

                ULONG i;
                vp = &(ywd->VP_Array[vp_num]);

                /*** VehicleProto frisch initialisieren ***/
                if (vp->wireframe_object) {
                    _dispose(vp->wireframe_object);
                    vp->wireframe_object = NULL;
                };
                if (vp->hud_wf_object) {
                    _dispose(vp->hud_wf_object);
                    vp->hud_wf_object = NULL;
                };
                if (vp->mg_wf_object) {
                    _dispose(vp->mg_wf_object);
                    vp->mg_wf_object = NULL;
                };
                if (vp->weapon_wf_object_1) {
                    _dispose(vp->weapon_wf_object_1);
                    vp->weapon_wf_object_1 = NULL;
                };
                if (vp->weapon_wf_object_2) {
                    _dispose(vp->weapon_wf_object_2);
                    vp->weapon_wf_object_2 = NULL;
                };
                memset(vp, 0, sizeof(struct VehicleProto));

                /*** Attribute auf Default ***/
                vp->BactClassID  = BCLID_YPATANK;
                vp->Weapons[0]   = -1;
                vp->MG_Shot      = -1;
                vp->TypeIcon     = 'A';
                vp->TypeNormal   = 0;
                vp->TypeFire     = 1;
                vp->TypeMegaDeth = 2;
                vp->TypeWait     = 3;
                vp->TypeDead     = 4;
                vp->TypeGenesis  = 5;

                vp->Shield       = 50;
                vp->Energy       = 10000;
                vp->ADist_Sector = YBA_ADist_Sector_DEF;
                vp->ADist_Bact   = YBA_ADist_Bact_DEF;
                vp->SDist_Sector = YBA_SDist_Sector_DEF;
                vp->SDist_Bact   = YBA_SDist_Bact_DEF;
                vp->View         = 1;
                vp->KillAfterShot= FALSE;

                vp->Mass          = 400.0;
                vp->MaxForce      = 5000.0;
                vp->AirConst      = 80.0;
                vp->MaxRot        = 0.8;
                vp->PrefHeight    = 150.0;
                vp->Radius        = 25.0;
                vp->OverEOF       = 25.0;
                vp->ViewerRadius  = 30.0;
                vp->ViewerOverEOF = 30.0;
                vp->GunPower      = YBA_GunPower_DEF;
                vp->GunRadius     = YBA_GunRadius_DEF;
                vp->MaxPitch      = -1.0;   // default wert: Nimm Standard

                vp->JobFightFlyer      = 0;
                vp->JobFightHelicopter = 0;
                vp->JobFightRobo       = 0;
                vp->JobFightTank       = 0;
                vp->JobReconnoitre     = 0;
                vp->JobConquer         = 0;

                for (i=0; i<VP_NUM_NOISES; i++) {
                    vp->Noise[i].volume     = 120;
                    vp->Noise[i].pfx.time   = 1000;
                    vp->Noise[i].pfx.mag0   = 1.0;
                    vp->Noise[i].shk.time = 1000;
                    vp->Noise[i].shk.mag0 = 1.0;
                    vp->Noise[i].shk.mute = 0.02;
                    vp->Noise[i].shk.x    = 0.2;  // 11.4 Grad
                    vp->Noise[i].shk.y    = 0.2;
                    vp->Noise[i].shk.z    = 0.2;
                };
                yw_InitVhclTags(vp);
            };

        /*** MODIFY ***/
        } else if (stricmp(kw, "modify_vehicle") == 0) {

            /*** existierenden VehicleProto modifizieren ***/
            vp_num = strtol(data, NULL, 0);
            if (vp_num >= NUM_VEHICLEPROTOS) return(PARSE_BOGUS_DATA);
            else {
                vp = &(ywd->VP_Array[vp_num]);
                ywd->touch_stone.vp_num = vp_num;
            };

        } else return(PARSE_UNKNOWN_KEYWORD);

        /*** Script-Parser-Struktur für weitere Aufrufe initialisieren ***/
        p->target  = vp;
        p->status  = PARSESTAT_RUNNING;
        p->store[1] = 0;    // act_gun
        p->store[2] = 0;    // act_coll
        p->store[3] = vp_num % MAXNUM_DEFINEDROBOS;    // robo_slot
        return(PARSE_ENTERED_CONTEXT);

    } else {

        /*** innerhalb eines Context-Block ***/
        struct VehicleProto *vp = p->target;
        ULONG act_gun   = p->store[1];
        ULONG act_coll  = p->store[2];
        ULONG robo_slot = p->store[3];
        ULONG res;
        struct RoboExtension *robo_ext = &(ywd->EXT_Array[robo_slot]);

        /*** END ***/
        if (stricmp(kw,"end")==0){

            /*** Robo-Sachen ***/
            if (BCLID_YPAROBO == vp->BactClassID) {
                yw_AddVhclTag(vp,YRA_Extension,(ULONG)robo_ext);
            };

            /*** berechne Range und Speed ***/
            if (BCLID_YPABACT == vp->BactClassID)
                /*** Helicopter -> Sonderbehandlung ***/
                vp->Speed = (ULONG) ((vp->MaxForce*0.6)/vp->AirConst);
            else
                vp->Speed = (ULONG) (vp->MaxForce / vp->AirConst);
            vp->Range = (ULONG) (vp->Speed / 10) * SECTOR_SIZE;

            #ifdef YPA_DESIGNMODE
            /*** für User enablen ***/
            vp->FootPrint |= (1<<1);
            #endif

            p->status   = PARSESTAT_READY;
            p->target   = NULL;
            return(PARSE_LEFT_CONTEXT);

        /*** Flug/Fahr-Modell ***/
        }else if (stricmp(kw, "model") == 0){

            if (stricmp(data, "heli") == 0)
                vp->BactClassID = BCLID_YPABACT;
            else if (stricmp(data, "tank") == 0)
                vp->BactClassID = BCLID_YPATANK;
            else if (stricmp(data, "robo") == 0){
                vp->BactClassID = BCLID_YPAROBO;
                /*** Robo-Extensions initialisieren ***/
                memset(robo_ext,0,sizeof(struct RoboExtension));
                robo_ext->Viewer.dir.m11 = 1.0;
                robo_ext->Viewer.dir.m22 = 1.0;
                robo_ext->Viewer.dir.m33 = 1.0;
            }else if (stricmp(data, "ufo") == 0)
                vp->BactClassID = BCLID_YPAUFO;
            else if (stricmp(data, "car") == 0)
                vp->BactClassID = BCLID_YPACAR;
            else if (stricmp(data, "gun") == 0)
                vp->BactClassID = BCLID_YPAGUN;
            else if (stricmp(data, "hover") == 0)
                vp->BactClassID = BCLID_YPAHOVERCRAFT;
            else if (stricmp(data, "plane") == 0){
                vp->BactClassID = BCLID_YPAFLYER;
                yw_AddVhclTag(vp,YFA_FlightType,YFF_AirPlane);
            }else if (stricmp(data, "glider") == 0){
                vp->BactClassID = BCLID_YPAFLYER;
                yw_AddVhclTag(vp,YFA_FlightType,YFF_Glider);
            }else if (stricmp(data, "zeppelin") == 0){
                vp->BactClassID = BCLID_YPAFLYER;
                yw_AddVhclTag(vp,YFA_FlightType,YFF_Zepp);

            }else return(PARSE_BOGUS_DATA);

        /*** ENABLE ***/
        }else if (stricmp(kw, "enable") == 0){
            ULONG own = strtol(data, NULL, 0);
            vp->FootPrint |= (1<<own);

        /*** DISABLE ***/
        }else if (stricmp(kw,"disable")==0){
            ULONG own = strtol(data, NULL, 0);
            vp->FootPrint &= ~(1<<own);

        /*** name ***/
        }else if (stricmp(kw, "name") == 0){
            /*** Name kopieren und '_' ersetzen durch ' ' ***/
            UBYTE *s;
            if (strlen(data) >= (sizeof(vp->Name)-1)) {
                _LogMsg("VhclProtoParser(): Name too long!\n");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(vp->Name,data);
            s = vp->Name;
            while (s = strchr(s,'_')) *s++ = ' ';

        /*** physikalische Attribute ***/
        }else if (stricmp(kw, "energy") == 0)
            vp->Energy     = strtol(data, NULL, 0);
        else if (stricmp(kw, "shield") == 0)
            vp->Shield     = strtol(data, NULL, 0);
        else if (stricmp(kw, "mass") == 0)
            vp->Mass       = atof(data);
        else if (stricmp(kw, "force") == 0)
            vp->MaxForce   = atof(data);
        else if (stricmp(kw, "maxrot") == 0)
            vp->MaxRot     = atof(data);
        else if (stricmp(kw, "airconst") == 0)
            vp->AirConst   = atof(data);
        else if (stricmp(kw, "height") == 0)
            vp->PrefHeight = atof(data);
        else if (stricmp(kw, "radius") == 0)
            vp->Radius     = atof(data);
        else if (stricmp(kw, "overeof") == 0)
            vp->OverEOF    = atof(data);
        else if (stricmp(kw, "vwr_radius") == 0)
            vp->ViewerRadius = atof(data);
        else if (stricmp(kw, "vwr_overeof") == 0)
            vp->ViewerOverEOF = atof(data);
        else if (stricmp(kw, "adist_sector") == 0)
            vp->ADist_Sector = atof(data);
        else if (stricmp(kw, "adist_bact") == 0)
            vp->ADist_Bact   = atof(data);
        else if (stricmp(kw, "sdist_sector") == 0)
            vp->SDist_Sector = atof(data);
        else if (stricmp(kw, "sdist_bact") == 0)
            vp->SDist_Bact   = atof(data);
        else if (stricmp(kw, "radar") == 0)
            vp->View = strtol(data, NULL, 0);

        /*** additive Attribute ***/
        else if (stricmp(kw, "add_energy") == 0)
            vp->Energy += strtol(data,NULL,0);
        else if (stricmp(kw, "add_shield") == 0)
            vp->Shield += strtol(data,NULL,0);
        else if (stricmp(kw, "add_radar")== 0)
            vp->View += strtol(data,NULL,0);

        /*** Visuals ***/
        else if (stricmp(kw, "vp_normal") == 0)
            vp->TypeNormal = strtol(data, NULL, 0);
        else if (stricmp(kw,"vp_fire")==0)
            vp->TypeFire   = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_megadeth") == 0)
            vp->TypeMegaDeth = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_wait") == 0)
            vp->TypeWait = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_dead") == 0)
            vp->TypeDead = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_genesis") == 0)
            vp->TypeGenesis = strtol(data, NULL, 0);
        else if (stricmp(kw, "type_icon") == 0)
            vp->TypeIcon = data[0];

        /*** Dest-FX ***/
        else if (stricmp(kw, "dest_fx") == 0) {

            UBYTE *state_str = strtok(data," _");
            UBYTE *vp_str = strtok(NULL," _");
            UBYTE *x_str  = strtok(NULL," _");
            UBYTE *y_str  = strtok(NULL," _");
            UBYTE *z_str  = strtok(NULL," _");

            if (state_str && vp_str && x_str && y_str && z_str) {

                struct DestEffect *dfx = &(vp->DestFX[vp->NumDestFX]);

                dfx->flags = 0;
                if (stricmp(state_str,"death")==0)
                    dfx->flags |= DEF_Death;
                else if (stricmp(state_str,"megadeth")==0)
                    dfx->flags |= DEF_Megadeth;
                else if (stricmp(state_str,"create")==0)
                    dfx->flags |= DEF_Create;
                else if (stricmp(state_str,"beam")==0)
                    dfx->flags |= DEF_Beam;
                else return(PARSE_BOGUS_DATA);

                dfx->proto      = strtol(vp_str,NULL,0);
                dfx->relspeed.x = atof(x_str);
                dfx->relspeed.y = atof(y_str);
                dfx->relspeed.z = atof(z_str);

                vp->NumDestFX++;
                if (vp->NumDestFX >= NUM_DESTFX) vp->NumDestFX=NUM_DESTFX-1;
            } else return(PARSE_BOGUS_DATA);

        /*** Weapons ***/
        }else if (stricmp(kw, "weapon") == 0){
            vp->Weapons[0] = strtol(data, NULL, 0);
            vp->Weapons[1] = -1;
        }else if (stricmp(kw, "mgun") == 0)
            vp->MG_Shot = strtol(data, NULL, 0);
        else if (stricmp(kw, "fire_x") == 0)
            vp->FireRelX = atof(data);
        else if (stricmp(kw,"fire_y")==0)
            vp->FireRelY = atof(data);
        else if (stricmp(kw, "fire_z") == 0)
            vp->FireRelZ = atof(data);
        else if (stricmp(kw, "gun_radius") == 0)
            vp->GunRadius = atof(data);
        else if (stricmp(kw, "gun_power") == 0)
            vp->GunPower = atof(data);
        else if (stricmp(kw, "gun_angle") == 0)
            vp->GunAngle = atof(data);
        else if (stricmp(kw, "num_weapons") == 0)
            vp->NumWeapons = strtol(data, NULL, 0);
        else if (stricmp(kw, "kill_after_shot") == 0)
            vp->KillAfterShot = strtol(data, NULL, 0);  // 0: FALSE

        /*** Eignungsparameter für diverse Aufgaben ***/
        else if (stricmp(kw, "job_fighthelicopter") == 0 )
            vp->JobFightHelicopter = (UBYTE) atoi( data );
        else if (stricmp(kw, "job_fightflyer") == 0 )
            vp->JobFightFlyer = (UBYTE) atoi( data );
        else if (stricmp(kw, "job_fighttank") == 0 )
            vp->JobFightTank = (UBYTE) atoi( data );
        else if (stricmp(kw, "job_fightrobo") == 0 )
            vp->JobFightRobo = (UBYTE) atoi( data );
        else if (stricmp(kw, "job_reconnoitre") == 0 )
            vp->JobReconnoitre = (UBYTE) atoi( data );
        else if (stricmp(kw, "job_conquer") == 0 )
            vp->JobConquer = (UBYTE) atoi( data );

        /*** SPECIALS ***/
        else if (stricmp(kw, "gun_side_angle") == 0)
            yw_AddVhclTag(vp,YGA_SideAngle,strtol(data,NULL,0));
        else if (stricmp(kw, "gun_up_angle") == 0)
            yw_AddVhclTag(vp,YGA_UpAngle,strtol(data,NULL,0));
        else if (stricmp(kw, "gun_down_angle") == 0)
            yw_AddVhclTag(vp,YGA_DownAngle,strtol(data,NULL,0));
        else if (stricmp(kw, "gun_type") == 0) {
            ULONG type = 0;
            if (stricmp(data, "flak") == 0)       type=GF_Real;
            else if (stricmp(data, "mg") == 0)    type=GF_Proto;
            else if (stricmp(data, "dummy") == 0) type=GF_None;
            if (type != 0) yw_AddVhclTag(vp,YGA_FireType,type);
        }else if (stricmp(kw, "kamikaze") == 0) {
            yw_AddVhclTag(vp,YCA_Kamikaze,TRUE);
            yw_AddVhclTag(vp,YCA_Blast,strtol(data,NULL,0));
        
        /*** Wireframe Objekte ***/
        }else if (stricmp(kw, "wireframe") == 0){
            if (vp->wireframe_object) {
                _dispose(vp->wireframe_object);
                vp->wireframe_object = NULL;
            };
            vp->wireframe_object = _new("sklt.class",RSA_Name,data,TAG_DONE);

        }else if (stricmp(kw, "hud_wireframe") == 0){
            if (vp->hud_wf_object) {
                _dispose(vp->hud_wf_object);
                vp->hud_wf_object = NULL;
            };
            vp->hud_wf_object = _new("sklt.class",RSA_Name,data,TAG_DONE);
        }else if (stricmp(kw, "mg_wireframe") == 0){
            if (vp->mg_wf_object) {
                _dispose(vp->mg_wf_object);
                vp->mg_wf_object = NULL;
            };
            vp->mg_wf_object = _new("sklt.class",RSA_Name,data,TAG_DONE);
        }else if (stricmp(kw, "wpn_wireframe_1") == 0){
            if (vp->weapon_wf_object_1) {
                _dispose(vp->weapon_wf_object_1);
                vp->weapon_wf_object_1 = NULL;
            };
            vp->weapon_wf_object_1 = _new("sklt.class", RSA_Name, data, TAG_DONE);
        }else if (stricmp(kw, "wpn_wireframe_2") == 0){
            if (vp->weapon_wf_object_2) {
                _dispose(vp->weapon_wf_object_2);
                vp->weapon_wf_object_2 = NULL;
            };
            vp->weapon_wf_object_2 = _new("sklt.class", RSA_Name, data, TAG_DONE);

        /*** Sound-FX ***/
        }else if (stricmp(kw, "vo_type") == 0) {
            vp->VOType = strtol(data,NULL,16);
        }else if (stricmp(kw, "max_pitch") == 0) {
            vp->MaxPitch = atof(data);
        }else if ((res=yw_ParseVhclSndFX(vp,kw,data))!=PARSE_UNKNOWN_KEYWORD){
            if (res == PARSE_BOGUS_DATA) return(res);

        /*** Paletten-FX ***/
        }else if ((res=yw_ParseVhclPalFX(vp,kw,data))!=PARSE_UNKNOWN_KEYWORD){
            if (res == PARSE_BOGUS_DATA) return(res);

        /*** Shake-FX ***/
        }else if ((res=yw_ParseVhclShkFX(vp,kw,data))!=PARSE_UNKNOWN_KEYWORD){
            if (res == PARSE_BOGUS_DATA) return(res);

        /*** Special-Explode-FX ***/
        }else if (stricmp(kw, "scale_fx")==0) {
            UBYTE *start_str = strtok(data,"_");
            UBYTE *speed_str = strtok(NULL,"_");
            UBYTE *accel_str = strtok(NULL,"_");
            UBYTE *dur_str   = strtok(NULL,"_");
            if (start_str&&speed_str&&accel_str&&dur_str) {
                UBYTE *type_str;
                ULONG i;
                vp->scale_start    = atof(start_str);
                vp->scale_speed    = atof(speed_str);
                vp->scale_accel    = atof(accel_str);
                vp->scale_duration = strtol(dur_str,NULL,0);
                i=0;
                while (type_str=strtok(NULL,"_")) {
                    vp->scale_type[i] = strtol(type_str,NULL,0);
                    i++;
                };
            };

        /*** ROBO EXTENSION ZEUGS ***/
        }else if (stricmp(kw, "robo_data_slot") == 0) {
            /*** OBSOLETE!!! ***/

        } else if (stricmp(kw, "robo_num_guns") == 0)
            robo_ext->number_of_guns = strtol(data, NULL, 0);
        else if (stricmp(kw, "robo_act_gun") == 0)
            act_gun = strtol(data, NULL, 0);
        else if (stricmp(kw,"robo_gun_pos_x")==0)
            robo_ext->gun[act_gun].pos.x = atof(data);
        else if (stricmp(kw, "robo_gun_pos_y") == 0)
            robo_ext->gun[act_gun].pos.y = atof(data);
        else if (stricmp(kw, "robo_gun_pos_z") == 0)
            robo_ext->gun[act_gun].pos.z = atof(data);
        else if (stricmp(kw, "robo_gun_dir_x") == 0)
            robo_ext->gun[act_gun].dir.x = atof(data);
        else if (stricmp(kw, "robo_gun_dir_y") == 0)
            robo_ext->gun[act_gun].dir.y = atof(data);
        else if (stricmp(kw,"robo_gun_dir_z")==0)
            robo_ext->gun[act_gun].dir.z = atof(data);
        else if (stricmp(kw, "robo_gun_type") == 0)
            robo_ext->gun[act_gun].gun = strtol(data, NULL, 0);
        else if (stricmp(kw, "robo_gun_name") == 0)
            strcpy((UBYTE *)&(robo_ext->gun[act_gun].name),data);

        else if (stricmp(kw, "robo_dock_x") == 0)
            robo_ext->dock_pos.x = atof(data);
        else if (stricmp(kw, "robo_dock_y") == 0)
            robo_ext->dock_pos.y = atof(data);
        else if (stricmp(kw, "robo_dock_z") == 0)
            robo_ext->dock_pos.z = atof(data);
        else if (stricmp(kw, "robo_coll_num") == 0)
            robo_ext->coll.number = strtol(data, NULL, 0);
        else if (stricmp(kw, "robo_coll_radius") == 0)
            robo_ext->coll.points[act_coll].radius = atof(data);
        else if (stricmp(kw, "robo_coll_act") == 0)
            act_coll = strtol(data, NULL, 0);
        else if (stricmp(kw, "robo_coll_x") == 0)
            robo_ext->coll.points[act_coll].point.x = atof(data);
        else if (stricmp(kw, "robo_coll_y") == 0)
            robo_ext->coll.points[act_coll].point.y = atof(data);
        else if (stricmp(kw, "robo_coll_z") == 0)
            robo_ext->coll.points[act_coll].point.z = atof(data);
        else if (stricmp(kw, "robo_viewer_x") == 0)
            robo_ext->Viewer.relpos.x = atof(data);
        else if (stricmp(kw, "robo_viewer_y") == 0)
            robo_ext->Viewer.relpos.y = atof(data);
        else if (stricmp(kw, "robo_viewer_z") == 0)
            robo_ext->Viewer.relpos.z = atof(data);
        else if (stricmp(kw, "robo_viewer_max_up") == 0)
            robo_ext->Viewer.max_up = atof(data);
        else if (stricmp(kw, "robo_viewer_max_down") == 0)
            robo_ext->Viewer.max_down = atof(data);
        else if (stricmp(kw, "robo_viewer_max_side") == 0)
            robo_ext->Viewer.max_side = atof(data);
        else if (stricmp(kw, "robo_does_twist") == 0)
            yw_AddVhclTag(vp,YRA_WaitRotate,TRUE);
        else if (stricmp(kw, "robo_does_flux") == 0)
            yw_AddVhclTag(vp,YRA_WaitSway,TRUE);

        /*** Unknown Keyword ***/
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** Parse-Data updaten für nächsten Aufruf ***/
        p->store[1] = act_gun;
        p->store[2] = act_coll;
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

