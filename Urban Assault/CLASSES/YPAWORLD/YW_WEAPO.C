/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_weapon.c,v $
**  $Revision: 38.14 $
**  $Date: 1998/01/06 16:30:14 $
**  $Locker: floh $
**  $Author: floh $
**
**  Definitionen etc. für WeaponTypes.
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

#include "ypa/ypamissileclass.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

_extern_use_nucleus
_extern_use_audio_engine

/*-----------------------------------------------------------------*/
_dispatcher(Object *, yw_YWM_CREATEWEAPON, struct createweapon_msg *msg)
/*
**  FUNCTION
**      siehe Specs in ypa/ypaworldclass.h
**
**  CHANGED
**      06-Dec-95   floh    created
**      20-Dec-95   floh    AF's latest diffs
**      17-Mar-96   floh    AF's latest diffs
**      26-Apr-96   floh    + Initialisiert die SoundCarrier-Struktur im
**                            Bakterium.
**      15-Aug-96   floh    + Paletten-FX-Initialisierung
**      27-Sep-96   floh    + Shake-FX eingebunden
**      01-Oct-96   floh    + neue Shake-Keywords
**      29-Oct-96   floh    + DestFX-Handling
**      20-Dec-96   floh    + FX-Initialisierung angepaßt an neues
**                            Snd-FX-Handling.
**      12-Apr-97   floh    + WP_Array jetzt global
**      19-Aug-97   floh    + First Hit Laden der Soundsamples dieser
**                            Waffe
*/
{
    struct ypaworld_data *ywd = INST_DATA(cl,o);

    struct setposition_msg sp_msg;
    struct setstate_msg st_msg;
    struct Bacterium *w_bact;

    struct WeaponProto *wproto;

    Object *w_obj = NULL;

    /*** Proto-Struktur ermitteln ***/
    if (msg->wp > NUM_WEAPONPROTOS) return(NULL);
    wproto = &(ywd->WP_Array[msg->wp]);

    /*** ist es überhaupt eine autonome Waffe? ***/
    if (wproto->Flags & WPF_Autonom) {

        /*** dann das Objekt erzeugen ***/
        if (w_obj = yw_PrivObtainBact(o, ywd, wproto->BactClassID)) {

            ULONG missy_handle;
            ULONG i;

            _get(w_obj, YBA_Bacterium, &w_bact);

            w_bact->Energy          = wproto->Energy;
            w_bact->Maximum         = wproto->Energy;
            w_bact->Shield          = 0;
            w_bact->mass            = wproto->Mass;
            w_bact->max_force       = wproto->MaxForce;
            w_bact->max_rot         = wproto->MaxRot;
            w_bact->pref_height     = wproto->PrefHeight;
            w_bact->radius          = wproto->Radius;
            w_bact->viewer_radius   = wproto->ViewerRadius;
            w_bact->over_eof        = wproto->OverEOF;
            w_bact->viewer_over_eof = wproto->ViewerOverEOF;
            w_bact->air_const       = wproto->AirConst;
            w_bact->bu_air_const    = wproto->AirConst;
            w_bact->adist_sector    = wproto->ADist_Sector;
            w_bact->adist_bact      = wproto->ADist_Bact;

            w_bact->TypeID  = msg->wp;
            w_bact->auto_ID = 0;

            w_bact->vis_proto_normal   = ywd->VisProtos[wproto->TypeNormal].o;
            w_bact->vp_tform_normal    = ywd->VisProtos[wproto->TypeNormal].tform;
            w_bact->vis_proto_fire     = ywd->VisProtos[wproto->TypeFire].o;
            w_bact->vp_tform_fire      = ywd->VisProtos[wproto->TypeFire].tform;
            w_bact->vis_proto_dead     = ywd->VisProtos[wproto->TypeDead].o;
            w_bact->vp_tform_dead      = ywd->VisProtos[wproto->TypeDead].tform;
            w_bact->vis_proto_wait     = ywd->VisProtos[wproto->TypeWait].o;
            w_bact->vp_tform_wait      = ywd->VisProtos[wproto->TypeWait].tform;
            w_bact->vis_proto_megadeth = ywd->VisProtos[wproto->TypeMegaDeth].o;
            w_bact->vp_tform_megadeth  = ywd->VisProtos[wproto->TypeMegaDeth].tform;
            w_bact->vis_proto_create   = ywd->VisProtos[wproto->TypeGenesis].o;
            w_bact->vp_tform_create    = ywd->VisProtos[wproto->TypeGenesis].tform;

            /*** DestFX ***/
            memcpy(w_bact->DestFX,wproto->DestFX,sizeof(w_bact->DestFX));


            /*** ein paar Missile-spezifische Attrs per OM_SET setzen ***/
            switch (wproto->Flags) {

                case (WPF_Autonom|WPF_Driven|WPF_Searching):
                    missy_handle = YMF_Search;
                    break;

                case (WPF_Autonom|WPF_Driven|WPF_FireAndForget):
                    missy_handle = YMF_Intelligent;
                    break;

                case (WPF_Autonom|WPF_Driven):
                    missy_handle = YMF_Simple;
                    break;

                case (WPF_Autonom|WPF_Impulse):
                    missy_handle = YMF_Grenade;
                    break;

                case (WPF_Autonom):
                    missy_handle = YMF_Bomb;
                    break;

                default:
                    missy_handle = YMF_Simple;
                    break;
            };

            _method(w_obj, OM_SET, YMA_LifeTime,      wproto->LifeTime,
                                   YMA_Delay,         wproto->DelayTime,
                                   YMA_DriveTime,     wproto->DriveTime,
                                   YMA_Handle,        missy_handle,     
                                   YMA_EnergyHeli,    (ULONG)(wproto->EnergyHeli  * 1000.0),
                                   YMA_EnergyTank,    (ULONG)(wproto->EnergyTank  * 1000.0),    
                                   YMA_EnergyFlyer,   (ULONG)(wproto->EnergyFlyer * 1000.0),    
                                   YMA_EnergyRobo,    (ULONG)(wproto->EnergyRobo  * 1000.0),
                                   YMA_RadiusHeli,    wproto->RadiusHeli,      
                                   YMA_RadiusTank,    wproto->RadiusTank,      
                                   YMA_RadiusFlyer,   wproto->RadiusFlyer,      
                                   YMA_RadiusRobo,    wproto->RadiusRobo,      
                                   TAG_DONE);

            /*** AUDIO ***/
            _InitSoundCarrier(&(w_bact->sc));

            /*** First-Hit-Laden der Samples dieser Waffe ***/
            for (i=0; i<WP_NUM_NOISES; i++) yw_LoadSISamples(&(wproto->Noise[i]));

            /*** übetrage VProto-Noises ins Bakterium ***/
            for (i=0; i<WP_NUM_NOISES; i++) {

                struct SoundSource *src = &(w_bact->sc.src[i]);

                /*** Basis-Parameter ***/
                src->volume = wproto->Noise[i].volume;
                src->pitch  = wproto->Noise[i].pitch;

                /*** LoopDaLoop-Flag setzen bei Loop-Noises ***/
                switch(i) {
                    case WP_NOISE_NORMAL:
                        src->flags |= AUDIOF_LOOPDALOOP;
                        break;
                };

                /*** konventionelle Sound-FX ***/
                if (wproto->Noise[i].smp_object) {
                    /*** übernehme Sample ***/
                    _get(wproto->Noise[i].smp_object, SMPA_Sample, &(src->sample));
                } else {
                    src->sample = NULL;
                };

                /*** Palette-FX ***/
                if (wproto->Noise[i].pfx.slot != 0) {
                    src->flags |= AUDIOF_HASPFX;
                    src->pfx    = &(wproto->Noise[i].pfx);
                } else {
                    src->flags &= ~AUDIOF_HASPFX;
                };

                /*** Shake-FX ***/
                if (wproto->Noise[i].shk.slot != 0) {
                    src->flags |= AUDIOF_HASSHAKE;
                    src->shk    = &(wproto->Noise[i].shk);
                } else {
                    src->flags &= ~AUDIOF_HASSHAKE;
                };

                /*** Extended Sample Definition ***/
                if (wproto->Noise[i].ext_smp.count > 0) {
                    src->flags  |= AUDIOF_HASEXTSMP;
                    src->ext_smp = &(wproto->Noise[i].ext_smp);
                } else {
                    src->flags &= ~AUDIOF_HASEXTSMP;
                };
            };

            /*** zusätzliche Attribute setzen ***/
            _methoda(w_obj, OM_SET, &(wproto->MoreAttrs[0]));

            /*** Position setzen, per YBM_SETPOSITION ***/
            sp_msg.x = msg->x;
            sp_msg.y = msg->y;
            sp_msg.z = msg->z;
            sp_msg.flags = YBFSP_SetGround;
            _methoda(w_obj, YBM_SETPOSITION, &sp_msg);

            /*** FIXME: auf Genesis-Zustand initialisieren? ***/
            st_msg.extra_on  = 0;
            st_msg.extra_off = 0;
            st_msg.main_state = ACTION_NORMAL;
            _methoda(w_obj, YBM_SETSTATE_I, &st_msg);
        };
    };

    /*** Ende ***/
    return(w_obj);
}

/*=================================================================**
**                                                                 **
**  WEAPON PROTOTYPE SCRIPT PARSER                                 **
**                                                                 **
**=================================================================*/

/*-----------------------------------------------------------------*/
void yw_InitWeapTags(struct WeaponProto *wp)
/*
**  FUNCTION
**      Initialisiert das WeaponProto TagArray neu.
**
**  CHANGED
**      13-Nov-96   floh    created
*/
{
    wp->LastTag = &(wp->MoreAttrs[0]);
    wp->LastTag->ti_Tag = TAG_DONE;
}

/*-----------------------------------------------------------------*/
void yw_AddWeapTag(struct WeaponProto *wp, ULONG tag, ULONG data)
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

    if (ti = _FindTagItem(tag,wp->MoreAttrs)) {
        /*** existiert bereits ***/
        ti->ti_Data = data;
    } else {
        /*** neues Tag anlegen ***/
        wp->LastTag->ti_Tag  = tag;
        wp->LastTag->ti_Data = data;
        wp->LastTag++;
        wp->LastTag->ti_Tag = TAG_DONE;
    };
}

/*-----------------------------------------------------------------*/
LONG yw_GetWeaponFXType(UBYTE *str)
/*
**  FUNCTION
**      Returniert den entsprechenden WP_NOISE_#? Typ
**      für den übergebenen String (nur für WeaponProtos!).
**      Oder -1, falls der String falsch ist.
**
**  CHANGED
**      20-Dec-96   floh    created.
*/
{
    LONG i;

    if      (stricmp(str,"normal")==0)    i=WP_NOISE_NORMAL;
    else if (stricmp(str,"launch")==0)    i=WP_NOISE_LAUNCH;
    else if (stricmp(str,"hit")==0)       i=WP_NOISE_HIT;
    else i = -1;

    return(i);
}

/*-----------------------------------------------------------------*/
ULONG yw_ParseWeaponPalFX(struct WeaponProto *wp, UBYTE *kw, UBYTE *data)
/*
**  FUNCTION
**      Siehe yw_ParseVhclPalFX().
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
        i = yw_GetWeaponFXType(str);
        if (i != -1) si = &(wp->Noise[i]);
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
ULONG yw_ParseWeaponShkFX(struct WeaponProto *wp, UBYTE *kw, UBYTE *data)
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
        i = yw_GetWeaponFXType(str);
        if (i != -1) si = &(wp->Noise[i]);
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
ULONG yw_ParseWeaponSndFX(struct WeaponProto *wp, UBYTE *kw, UBYTE *data)
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
        i = yw_GetWeaponFXType(str);
        if (i != -1) si = &(wp->Noise[i]);
        else return(PARSE_UNKNOWN_KEYWORD);

        /*** welcher Parameter? ***/
        str = strtok(NULL,"_");
        if (stricmp(str,"sample")==0) {
            if (strlen(data) >= (sizeof(si->smp_name)-1)) {
                _LogMsg("ParseWeaponSndFX(): Sample name too long!\n");
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
ULONG yw_WeaponProtoParser(struct ScriptParser *p)
/*
**  FUNCTION
**      Parser für WeaponProto Descriptions
**      (new_weapon, modifiy_weapon Blöcke)
**
**  CHANGED
**      03-May-96   floh    created
**      08-May-96   floh    Fix für Loop-Optimizer-Bug im Watcom (FUCK!)
**      15-Aug-96   floh    + Paletten-FX-Keywords (pal_#?)
**      21-Sep-96   floh    + Design-Mode: alle Weapons für User
**                            enabled
**      27-Sep-96   floh    + Shake-FX-Keyword (shk_#?)
**      01-Oct-96   floh    + neue Shake-Keywords
**      29-Oct-96   floh    + <dest_fx> Keyword
**      20-Dec-96   floh    + Pal-,Shk- und Pal-FX-Parsing aufgeräumt
**                            wie in yw_VhclProtoParser()
**      12-Apr-97   floh    + WP_Array nicht mehr global
**      24-Apr-97   floh    + "wireframe"
**      05-May-97   floh    + es gab ein Memory-Leak bei Wireframes
**      01-Oct-97   floh    + add_energy, add_shot_time, add_shot_time_user
**      16-Dec-97   floh    + Touchstone Struktur wird ausgefuellt
*/
{
    UBYTE *kw   = p->keyword;
    UBYTE *data = p->data;
    struct ypaworld_data *ywd = (struct ypaworld_data *) p->store[0];

    if (PARSESTAT_READY == p->status) {

        /*** momentan außerhalb eines Contexts ***/
        struct WeaponProto *wp = NULL;
        ULONG wp_num = 0;
        p->target = NULL;

        /*** NEW ***/
        if (stricmp(kw, "new_weapon") == 0) {

            /*** neuer VehicleProto ***/
            wp_num = strtol(data, NULL, 0);
            if (wp_num >= NUM_WEAPONPROTOS) return(PARSE_BOGUS_DATA);
            else {

                ULONG i;

                wp = &(ywd->WP_Array[wp_num]);

                /*** Weapon-Proto frisch initialisieren ***/
                if (wp->wireframe_object) {
                    _dispose(wp->wireframe_object);
                    wp->wireframe_object = NULL;
                };

                memset(wp, 0, sizeof(struct WeaponProto));

                /*** setspezische Attribute auf Default ***/
                wp->BactClassID = BCLID_YPAMISSY;
                memset(wp->Name, 0, sizeof(wp->Name));

                wp->Energy        = 10000;
                wp->Mass          = 50.0;
                wp->MaxForce      = 5000.0;
                wp->AirConst      = 50.0;
                wp->MaxRot        = 2.0;
                wp->Radius        = 20.0;
                wp->OverEOF       = 10.0;
                wp->ViewerRadius  = 20.0;
                wp->ViewerOverEOF = 20.0;
                
                /* ----------------------------------------------------------
                ** neues Zeug... weil ich nicht alle mit energy ausfuellen
                ** kann (wegen Wundersteinen und so...wuerde dann vorheriges
                ** ueberschreiben), setze ich sie was "nix besonderes" heisst
                ** und bedeutet, dass ich dann den Wert aus der bactStruktur 
                ** nehme. 
                ** --------------------------------------------------------*/
                wp->EnergyHeli    = 1.0;
                wp->EnergyTank    = 1.0;
                wp->EnergyFlyer   = 1.0;
                wp->EnergyRobo    = 1.0;
                wp->RadiusHeli    = 0.0;
                wp->RadiusTank    = 0.0;
                wp->RadiusFlyer   = 0.0;
                wp->RadiusRobo    = 0.0;

                wp->StartSpeed    = 70;
                wp->LifeTime      = 20000;
                wp->LifeTimeNT    = 0; // 0 heisst "nicht aendern", also LifeTime verwenden
                wp->DriveTime     = 7000;
                wp->ShotTime      = 3000;
                wp->ShotTime_User = 1000;
                wp->SalveDelay    = 0;
                wp->SalveShots    = 0;  // 0 heisst normales Verhalten

                wp->TypeNormal   = 0;
                wp->TypeFire     = 1;
                wp->TypeMegaDeth = 2;
                wp->TypeWait     = 3;
                wp->TypeDead     = 4;
                wp->TypeGenesis  = 5;
                wp->TypeIcon     = 'A';

                for (i=0; i<WP_NUM_NOISES; i++) {
                    wp->Noise[i].volume = 120;
                    wp->Noise[i].pfx.time = 1000;
                    wp->Noise[i].pfx.mag0 = 1.0;
                    wp->Noise[i].shk.time = 1000;
                    wp->Noise[i].shk.mag0 = 1.0;
                    wp->Noise[i].shk.mute = 0.02;
                    wp->Noise[i].shk.x    = 0.2;  // 11.4 Grad
                    wp->Noise[i].shk.y    = 0.2;
                    wp->Noise[i].shk.z    = 0.2;
                };
                yw_InitWeapTags(wp);
            };

        /*** MODIFY ***/
        } else if (stricmp(kw, "modify_weapon") == 0) {

            /*** existierenden WeaponProto modifizieren ***/
            wp_num = strtol(data, NULL, 0);
            if (wp_num >= NUM_WEAPONPROTOS) return(PARSE_BOGUS_DATA);
            else {
                wp = &(ywd->WP_Array[wp_num]);
                ywd->touch_stone.wp_num = wp_num;
            };

        } else return(PARSE_UNKNOWN_KEYWORD);

        /*** Script-Parser-Struktur für weitere Aufrufe initialisieren ***/
        p->target  = wp;
        p->status  = PARSESTAT_RUNNING;
        return(PARSE_ENTERED_CONTEXT);

    } else {

        /*** innerhalb eines Context-Block ***/
        struct WeaponProto *wp = p->target;
        ULONG res;

        /*** END ***/
        if (stricmp(kw, "end") == 0){

            #ifdef YPA_DESIGNMODE
            /*** für User enablen ***/
            wp->FootPrint |= (1<<1);
            #endif

            p->status = PARSESTAT_READY;
            p->target = NULL;
            return(PARSE_LEFT_CONTEXT);

        /*** Verhaltens-Modell ***/
        }else if (stricmp(kw, "model") == 0){

            if (stricmp(data, "grenade") == 0)
                wp->Flags = WPF_Autonom|WPF_Impulse;
            else if (stricmp(data, "rocket") == 0)
                wp->Flags = WPF_Autonom|WPF_Driven;
            else if (stricmp(data, "missile") == 0)
                wp->Flags = WPF_Autonom|WPF_Driven|WPF_Searching;
            else if (stricmp(data, "bomb") == 0)
                wp->Flags = WPF_Autonom;
            else if (stricmp(data, "special") == 0)
                wp->Flags = WPF_Autonom;
            else return(PARSE_BOGUS_DATA);

        /*** ENABLE ***/
        }else if (stricmp(kw, "enable") == 0){
            ULONG own = strtol(data, NULL, 0);
            wp->FootPrint |= (1<<own);

        /*** DISABLE ***/
        }else if (stricmp(kw, "disable") == 0){
            ULONG own = strtol(data, NULL, 0);
            wp->FootPrint &= ~(1<<own);

        /*** name ***/
        }else if (stricmp(kw, "name") == 0){
            /*** Name kopieren und '_' ersetzen durch ' ' ***/
            UBYTE *s;
            if (strlen(data) >= (sizeof(wp->Name)-1)) {
                _LogMsg("WeaponProtoParser(): Name too long!\n");
                return(PARSE_BOGUS_DATA);
            };
            strcpy(wp->Name, data);
            s = wp->Name;
            while (s = strchr(s,'_')) *s++ = ' ';

        /*** physikalische Attribute ***/
        } else if (stricmp(kw, "energy") == 0) 
            wp->Energy = strtol(data, NULL, 0);
        else if (stricmp(kw, "energy_heli") == 0)
            wp->EnergyHeli = atof(data);
        else if (stricmp(kw, "energy_tank") == 0)
            wp->EnergyTank = atof(data);
        else if (stricmp(kw, "energy_flyer") == 0)
            wp->EnergyFlyer = atof(data);
        else if (stricmp(kw, "energy_robo") == 0)
            wp->EnergyRobo = atof(data);
        else if (stricmp(kw, "mass") == 0)
            wp->Mass = atof(data);
        else if (stricmp(kw, "force") == 0)
            wp->MaxForce = atof(data);
        else if (stricmp(kw, "maxrot") == 0)
            wp->MaxRot = atof(data);
        else if (stricmp(kw, "airconst") == 0)
            wp->AirConst = atof(data);
        else if (stricmp(kw, "radius") == 0) 
            wp->Radius = atof(data);          
        else if (stricmp(kw, "radius_heli") == 0)
            wp->RadiusHeli = atof(data);
        else if (stricmp(kw, "radius_tank") == 0)
            wp->RadiusTank = atof(data);
        else if (stricmp(kw, "radius_flyer") == 0)
            wp->RadiusFlyer = atof(data);
        else if (stricmp(kw, "radius_robo") == 0)
            wp->RadiusRobo = atof(data);
        else if (stricmp(kw, "overeof") == 0)
            wp->OverEOF = atof(data);
        else if (stricmp(kw, "vwr_radius") == 0)
            wp->ViewerRadius = atof(data);
        else if (stricmp(kw, "vwr_overeof") == 0)
            wp->ViewerOverEOF = atof(data);
        else if (stricmp(kw, "start_speed") == 0)
            wp->StartSpeed = atof(data);
        else if (stricmp(kw, "life_time") == 0)
            wp->LifeTime = strtol(data, NULL, 0);
        else if (stricmp(kw, "life_time_nt") == 0)
            wp->LifeTimeNT = strtol(data, NULL, 0);
        else if (stricmp(kw, "drive_time") == 0)
            wp->DriveTime = strtol(data, NULL, 0);
        else if (stricmp(kw, "delay_time") == 0)
            wp->DelayTime = strtol(data, NULL, 0);
        else if (stricmp(kw, "shot_time") == 0)
            wp->ShotTime = strtol(data, NULL, 0);
        else if (stricmp(kw, "shot_time_user") == 0)
            wp->ShotTime_User = strtol(data, NULL, 0);
        else if (stricmp(kw, "salve_shots") == 0)
            wp->SalveShots = strtol(data, NULL, 0);
        else if (stricmp(kw, "salve_delay") == 0)
            wp->SalveDelay = strtol(data, NULL, 0);

        /*** additive Attribute ***/
        else if (stricmp(kw, "add_energy") == 0)
            wp->Energy += strtol(data,NULL,0);
        else if (stricmp(kw, "add_energy_heli") == 0)
            wp->EnergyHeli += strtol(data,NULL,0);
        else if (stricmp(kw, "add_energy_tank") == 0)
            wp->EnergyTank += strtol(data,NULL,0);
        else if (stricmp(kw, "add_energy_flyer") == 0)
            wp->EnergyFlyer += strtol(data,NULL,0);
        else if (stricmp(kw, "add_energy_Robo") == 0)
            wp->EnergyRobo += strtol(data,NULL,0);
        else if (stricmp(kw, "add_shot_time") == 0)
            wp->ShotTime += strtol(data,NULL,0);
        else if (stricmp(kw, "add_shot_time_user") == 0)
            wp->ShotTime_User += strtol(data,NULL,0);

        /*** Visuals ***/
        else if (stricmp(kw, "vp_normal") == 0)
            wp->TypeNormal = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_fire") == 0)
            wp->TypeFire   = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_megadeth") == 0)
            wp->TypeMegaDeth = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_wait") == 0)
            wp->TypeWait = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_dead") == 0)
            wp->TypeDead = strtol(data, NULL, 0);
        else if (stricmp(kw, "vp_genesis") == 0)
            wp->TypeGenesis = strtol(data, NULL, 0);
        else if (stricmp(kw, "type_icon") == 0)
            wp->TypeIcon = data[0];

        /*** Wireframe-Outline ***/
        else if (stricmp(kw, "wireframe") == 0){
            /*** vorher aufräumen... ***/
            if (wp->wireframe_object) {
                _dispose(wp->wireframe_object);
                wp->wireframe_object = NULL;
            };
            wp->wireframe_object = _new("sklt.class",RSA_Name,data,TAG_DONE);

        /*** Dest-FX ***/
        }else if (stricmp(kw, "dest_fx") == 0) {

            UBYTE *state_str = strtok(data," _");
            UBYTE *vp_str = strtok(NULL," _");
            UBYTE *x_str  = strtok(NULL," _");
            UBYTE *y_str  = strtok(NULL," _");
            UBYTE *z_str  = strtok(NULL," _");

            if (state_str && vp_str && x_str && y_str && z_str) {

                struct DestEffect *dfx = &(wp->DestFX[wp->NumDestFX]);

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

                wp->NumDestFX++;
                if (wp->NumDestFX >= NUM_DESTFX) wp->NumDestFX=NUM_DESTFX-1;
            } else return(PARSE_BOGUS_DATA);

        }else if ((res=yw_ParseWeaponSndFX(wp,kw,data))!=PARSE_UNKNOWN_KEYWORD){
            if (res == PARSE_BOGUS_DATA) return(res);

        /*** Paletten-FX ***/
        }else if ((res=yw_ParseWeaponPalFX(wp,kw,data))!=PARSE_UNKNOWN_KEYWORD){
            if (res == PARSE_BOGUS_DATA) return(res);

        /*** Shake-FX ***/
        }else if ((res=yw_ParseWeaponShkFX(wp,kw,data))!=PARSE_UNKNOWN_KEYWORD){
            if (res == PARSE_BOGUS_DATA) return(res);

        /*** Unknown Keyword ***/
        }else return(PARSE_UNKNOWN_KEYWORD);

        /*** Parse-Data updaten für nächsten Aufruf ***/
        return(PARSE_ALL_OK);
    };

    /*** can't happen ***/
    return(PARSE_UNKNOWN_KEYWORD);
}

