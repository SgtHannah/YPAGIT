/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_voiceover.c,v $
**  $Revision: 38.1 $
**  $Date: 1997/10/11 15:51:00 $
**  $Locker: floh $
**  $Author: floh $
**
**  yw_voiceover.c -- Handelt die VoiceOver-Messages ab.
**
**  (C) Copyright 1997 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "engine/engine.h"
#include "ypa/ypaworldclass.h"

#include "yw_protos.h"

_extern_use_nucleus
_extern_use_audio_engine

/*-----------------------------------------------------------------*/
void yw_VOFreeSlot(struct ypaworld_data *ywd, struct vo_Slot *s)
/*
**  FUNCTION
**      Macht den angegebenen Slot frei, spielende Sounds
**      werden gestoppt, das Sample-Object wird freigegeben.
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    _KillSoundCarrier(&(s->sc));
    if (s->so) {
        _dispose(s->so);
    };
    memset(s,0,sizeof(struct vo_Slot));
    s->pri = -1;
}

/*-----------------------------------------------------------------*/
void yw_VOInitSlot(struct ypaworld_data *ywd,
                   struct vo_Slot *s,
                   LONG pri,
                   Object *so,
                   struct Bacterium *sender)
/*
**  FUNCTION
**      Initialisiert den angegebenen Slot mit dem
**      übergebenen Sample-Object. Das Sample wird
**      NICHT gestartet. Der Slot sollte vorher
**      yw_VOFreeSlot()'ed sein.
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    struct SoundSource *src = &(s->sc.src[0]);
    _InitSoundCarrier(&(s->sc));
    src->volume = 500;
    src->pitch  = 0;
    if (so) {
        _get(so,SMPA_Sample,&(src->sample));
    } else {
        src->sample = NULL;
    };
    s->pri = pri;
    s->so  = so;
    s->sender = sender;
}

/*-----------------------------------------------------------------*/
void yw_VOGetPos(struct ypaworld_data *ywd,
                 struct Bacterium *sender,
                 struct flt_triple *pos)
/*
**  FUNCTION
**      Ordnet der hereinkommenden Message von <sender> eine
**      Position im 3D-Raum zu. Die Messages werden korrekt
**      im Stereoraum positioniert, aber alle auf eine
**      Entfernung normalisiert.
**      Rückgabe der Position in <pos>.
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    if (sender == ywd->URBact) {
        /*** Hoststation-Meldungen immer direkt "im Kopf" ***/
        pos->x = ywd->UVBact->pos.x;
        pos->y = ywd->UVBact->pos.y;
        pos->z = ywd->UVBact->pos.z;
    } else {
        /*** normale Meldungen Richtungs-verteilen ***/
        FLOAT x = sender->pos.x - ywd->UVBact->pos.x;
        FLOAT y = sender->pos.y - ywd->UVBact->pos.y;
        FLOAT z = sender->pos.z - ywd->UVBact->pos.z;
        FLOAT l;

        /*** Richtungs-Vektor normalisieren... ***/
        l = nc_sqrt(x*x + y*y + z*z);
        if (l > 0) {
            x = (x/l)*100.0;
            y = (y/l)*100.0;
            z = (z/l)*100.0;
        };
        /*** ...und wieder eine globale Position draus machen... ***/
        pos->x = x + ywd->UVBact->pos.x;
        pos->y = y + ywd->UVBact->pos.y;
        pos->z = z + ywd->UVBact->pos.z;
    };
}

/*-----------------------------------------------------------------*/
void yw_VOLoadStartSlot(struct ypaworld_data *ywd,
                        struct vo_Slot *s,
                        UBYTE *sname,
                        struct Bacterium *sender,
                        LONG pri)
/*
**  FUNCTION
**      Lädt und startet einen Voice-Over-Effekt.
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    UBYTE fname[255];
    UBYTE old_path[255];
    Object *so;

    /*** Slot deinitialisieren ***/
    yw_VOFreeSlot(ywd,s);

    /*** Name des WAV-Files zusammenstückeln ***/
    strcpy(fname,"sounds/speech/");
    if (ywd->LocaleLang[0]) {
        strcat(fname,ywd->LocaleLang);
        strcat(fname,"/");
    } else {
        strcat(fname,"language/");
    };
    strcat(fname,sname);

    /*** Sample laden ***/
    strcpy(old_path,_GetAssign("rsrc"));
    _SetAssign("rsrc","data:");
    so = _new("wav.class", RSA_Name, fname, TAG_DONE);
    _SetAssign("rsrc",old_path);
    if (so) {

        /*** Slot initialisieren ***/
        yw_VOInitSlot(ywd,s,pri,so,sender);

        /*** Position initialisieren ***/
        yw_VOGetPos(ywd,sender,&(s->sc.pos));
        s->sc.vec.x = ywd->UVBact->dof.x * ywd->UVBact->dof.v;
        s->sc.vec.y = ywd->UVBact->dof.y * ywd->UVBact->dof.v;
        s->sc.vec.z = ywd->UVBact->dof.z * ywd->UVBact->dof.v;

        /*** Playback! ***/
        _StartSoundSource(&(s->sc),0);
    };
}

/*-----------------------------------------------------------------*/
void yw_KillVoiceOverSystem(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Killt das VoiceOver-System.
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    ULONG i;
    if (ywd->VoiceOver) {
        for (i=0; i<MAXNUM_VOICEOVERS; i++) {
            /*** alle SoundCarriers killen ***/
            struct vo_Slot *s = &(ywd->VoiceOver->slot[i]);
            yw_VOFreeSlot(ywd,s);
        };
        _FreeVec(ywd->VoiceOver);
        ywd->VoiceOver = 0;
    };
}

/*-----------------------------------------------------------------*/
ULONG yw_InitVoiceOverSystem(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert das VoiceOver-System.
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    ULONG retval = FALSE;
    ywd->VoiceOver = _AllocVec(sizeof(struct vo_VoiceOver),MEMF_PUBLIC|MEMF_CLEAR);
    if (ywd->VoiceOver) {
        ULONG i;
        for (i=0; i<MAXNUM_VOICEOVERS; i++) {
            struct vo_Slot *s = &(ywd->VoiceOver->slot[i]);
            s->pri = -1;
            _InitSoundCarrier(&(s->sc));
        };
        retval = TRUE;
    };
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_StartVoiceOver(struct ypaworld_data *ywd,
                       struct Bacterium *sender,
                       LONG pri,
                       ULONG code)
/*
**  FUNCTION
**      Initialisiert und startet einen VoiceOver_Effekt.
**      Dazu wird ein freier oder niedrig priorisierter
**      VoiceOverSlot gesucht, initialisiert und
**      ein einmaliges Abspielen gestartet. In den
**      folgenden yw_TriggerVoiceOver()'s wird dann
**      Position, Lautstärke, etc... angepaßt, sowie
**      entschieden, ob sich der VO-Effekt beenden soll.
**
**  INPUTS
**      ywd
**      sender  - Sender-Bakterium, kann auch Hoststation sein!
**      pri     - Priorität des VoiceOvers
**      code    - LOGMSG_#? Code
**
**  CHANGED
**      24-Sep-97   floh    created
**      07-Oct-97   floh    + "PowerStationCaptured"
**      30-Apr-98   floh    + Power/Radar/Flak-Station Lost
**                          + Superbomb/Superwave Messages
**                          + Beamgate Full
**      31-May-98   floh    + LOGMSG_CHAT 
*/
{
    if (ywd->VoiceOver) {

        LONG free_slot = -1;
        ULONG i;

        /*** suche einen freien, oder niedrig priorisierten Slot ***/
        for (i=0; i<MAXNUM_VOICEOVERS; i++) {
            struct vo_Slot *s = &(ywd->VoiceOver->slot[i]);
            if ((s->pri < 0)||(s->pri < pri)) {
                free_slot = i;
                break;
            };
        };
        if (free_slot != -1) {

            struct vo_Slot *s = &(ywd->VoiceOver->slot[free_slot]);
            UBYTE sname[32];
            ULONG group = 0;
            ULONG line = 0;
            ULONG col1 = 0;
            ULONG col2 = 0;
            ULONG num_vars = 0;
            ULONG var = 0;

            /*** Msg Code ***/
            switch(code) {

                /*** Squadrons, Feedback ***/
                case LOGMSG_CREATED:
                    group=1; col1=1; col2=1; num_vars=2; break;
                case LOGMSG_DONE:
                    group=1; col1=1; col2=2; num_vars=2; break;
                case LOGMSG_ORDERMOVE:
                    group=1; col1=1; col2=3; num_vars=2; break;
                case LOGMSG_ORDERATTACK:
                    group=1; col1=1; col2=4; num_vars=2; break;
                case LOGMSG_CONTROL:
                    group=1; col1=1; col2=5; num_vars=1; break;
                case LOGMSG_ENEMYSECTOR:
                    group=1; col1=1; col2=6; num_vars=1; break;

                /*** Squadrons, Critical ***/
                case LOGMSG_RELAXED:
                    group=1; col1=2; col2=1; num_vars=1; break;
                case LOGMSG_FOUNDENEMY:
                    group=1; col1=2; col2=2; num_vars=2; break;
                case LOGMSG_FOUNDROBO:
                    group=1; col1=2; col2=3; num_vars=1; break;
                case LOGMSG_REQUESTSUPPORT:
                    group=1; col1=2; col2=4; num_vars=1; break;
                case LOGMSG_ESCAPE:
                    group=1; col1=2; col2=5; num_vars=1; break;
                case LOGMSG_LASTDIED:
                    group=1; col1=2; col2=6; num_vars=2; break;

                /*** Squadrons, Success ***/
                case LOGMSG_ENEMYESCAPES:
                    group=1; col1=3; col2=1; num_vars=1; break;
                case LOGMSG_ENEMYDESTROYED:
                    group=1; col1=3; col2=2; num_vars=2; break;
                case LOGMSG_ROBODEAD:
                    group=1; col1=3; col2=3; num_vars=1; break;
                case LOGMSG_POWER_CAPTURED:
                    group=1; col1=3; col2=4; num_vars=1;
                    if (sender==0) return;  // can happen, wenn Station
                    break;

                /*** Station ***/
                case LOGMSG_GATEOPENED:
                    group=2; line=1; col1=1; col2=1; num_vars=1; break;
                case LOGMSG_GATECLOSED:
                    group=2; line=1; col1=1; col2=2; num_vars=1; break;
                case LOGMSG_TECH_WEAPON:
                    group=2; line=1; col1=1; col2=3; num_vars=1; break;
                case LOGMSG_TECH_ARMOR:
                    group=2; line=1; col1=1; col2=4; num_vars=1; break;
                case LOGMSG_TECH_VEHICLE:
                    group=2; line=1; col1=1; col2=5; num_vars=1; break;
                case LOGMSG_TECH_BUILDING:
                    group=2; line=1; col1=1; col2=6; num_vars=1; break;
                case LOGMSG_TECH_RADAR:
                    group=2; line=1; col1=1; col2=0xc; num_vars=1; break;
                case LOGMSG_TECH_BUILDANDVEHICLE:
                    group=2; line=4; col1=1; col2=2; num_vars=1; break;
                case LOGMSG_TECH_GENERIC:
                    group=2; line=1; col1=1; col2=0xd; num_vars=1; break;
                case LOGMSG_TECH_LOST:
                    group=2; line=1; col1=1; col2=7; num_vars=1; break;
                case LOGMSG_POWER_DESTROYED:
                    group=2; line=1; col1=1; col2=8; num_vars=1; break;
                case LOGMSG_FLAK_DESTROYED:
                    group=2; line=2; col1=1; col2=8; num_vars=1; break;
                case LOGMSG_RADAR_DESTROYED:
                    group=2; line=3; col1=1; col2=8; num_vars=1; break;
                case LOGMSG_POWER_ATTACK:
                    group=2; line=1; col1=1; col2=9; num_vars=1; break;
                case LOGMSG_FLAK_ATTACK:
                    group=2; line=2; col1=1; col2=9; num_vars=1; break;
                case LOGMSG_RADAR_ATTACK:
                    group=2; line=3; col1=1; col2=9; num_vars=1; break;
                case LOGMSG_POWER_CREATED:
                    group=2; line=1; col1=1; col2=0xa; num_vars=1; break;
                case LOGMSG_FLAK_CREATED:
                    group=2; line=2; col1=1; col2=0xa; num_vars=1; break;
                case LOGMSG_RADAR_CREATED:
                    group=2; line=3; col1=1; col2=0xa; num_vars=1; break;
                case LOGMSG_USERROBODANGER:
                    group=2; line=1; col1=2; col2=1; num_vars=1; break;
                case LOGMSG_HOST_WELCOMEBACK:
                    group=2; line=1; col1=2; col2=2; num_vars=1; break;
                case LOGMSG_HOST_ENERGY_CRITICAL:
                    group=2; line=1; col1=2; col2=3; num_vars=1; break;
                case LOGMSG_HOST_ONLINE:
                    group=2; line=1; col1=2; col2=4; num_vars=2; break;
                case LOGMSG_HOST_SHUTDOWN:
                    group=2; line=1; col1=2; col2=5; num_vars=2; break;
                case LOGMSG_USERROBODEAD:
                    group=2; line=1; col1=2; col2=6; num_vars=2; break;
                case LOGMSG_ANALYSIS_AVAILABLE:
                    group=2; line=1; col1=2; col2=7; num_vars=3; break;
                case LOGMSG_ANALYSIS_WORKING:
                    group=2; line=1; col1=2; col2=8; num_vars=2; break;
                case LOGMSG_POWER_LOST:
                    group=2; line=1; col1=1; col2=0xb; num_vars=1; break;
                case LOGMSG_FLAK_LOST:
                    group=2; line=2; col1=1; col2=8; num_vars=1; break;  
                case LOGMSG_RADAR_LOST:
                    group=2; line=3; col1=1; col2=8; num_vars=1; break;
                    
                case LOGMSG_EVENTMSG_CREATE:
                    group=3; line=0; col1=0; col2=1; num_vars=1; break;
                case LOGMSG_EVENTMSG_CONTROL:
                    group=3; line=0; col1=0; col2=2; num_vars=2; break;
                case LOGMSG_EVENTMSG_DESTROYROBO:
                    group=3; line=0; col1=0; col2=3; num_vars=3; break;
                case LOGMSG_EVENTMSG_COMPLETE_1:
                    group=3; line=0; col1=0; col2=4; num_vars=1; break;
                    
                case LOGMSG_EVENTMSG_MAP:
                    group=3; line=0; col1=0; col2=5; num_vars=1; break;
                case LOGMSG_EVENTMSG_COMMAND:
                    group=3; line=0; col1=0; col2=6; num_vars=1; break;                    
                case LOGMSG_EVENTMSG_OPT_CONTROL:
                    group=3; line=0; col1=0; col2=7; num_vars=1; break;
                case LOGMSG_EVENTMSG_OPT_AGGR:
                    group=3; line=0; col1=0; col2=8; num_vars=1; break;
                case LOGMSG_EVENTMSG_DESTROYALL:
                    group=3; line=0; col1=0; col2=9; num_vars=1; break;
                case LOGMSG_EVENTMSG_COMPLETE_2:
                    group=3; line=0; col1=1; col2=0; num_vars=1; break;

                case LOGMSG_EVENTMSG_POWERSTATION:
                    group=3; line=0; col1=1; col2=1; num_vars=1; break;
                case LOGMSG_EVENTMSG_KEYSECTORS:
                    group=3; line=0; col1=1; col2=2; num_vars=1; break;
                case LOGMSG_EVENTMSG_BEAMGATE:
                    group=3; line=0; col1=1; col2=3; num_vars=1; break;
                case LOGMSG_EVENTMSG_HOWTOBEAM:
                    group=3; line=0; col1=1; col2=4; num_vars=1; break;
                case LOGMSG_EVENTMSG_COMPLETE_3:
                    group=3; line=0; col1=1; col2=5; num_vars=1; break;
                case LOGMSG_EVENTMSG_READYFORWAR:
                    group=3; line=0; col1=1; col2=6; num_vars=1; break;

                case LOGMSG_SUPERBOMB_ACTIVATED:
                    group=2; line=4; col1=1; col2=5; num_vars=1; break;                
                case LOMGSG_SUPERBOMB_TRIGGERED:
                    group=2; line=4; col1=1; col2=8; num_vars=1; break;                
                case LOGMSG_SUPERBOMB_FROZEN:
                    group=2; line=4; col1=1; col2=6; num_vars=1; break;                
                case LOGMSG_SUPERBOMB_DEACTIVATED:
                    group=2; line=4; col1=1; col2=7; num_vars=1; break;                
                case LOGMSG_SUPERWAVE_ACTIVATED:
                    group=2; line=4; col1=1; col2=9; num_vars=1; break;                
                case LOGMSG_SUPERWAVE_TRIGGERED:
                    group=2; line=4; col1=1; col2=0xc; num_vars=1; break;                
                case LOGMSG_SUPERWAVE_FROZEN:
                    group=2; line=4; col1=1; col2=0xa; num_vars=1; break;                
                case LOGMSG_SUPERWAVE_DEACTIVATED:
                    group=2; line=4; col1=1; col2=0xb; num_vars=1; break;
                    
                case LOGMSG_BEAMGATE_FULL:
                    group=2; line=4; col1=1; col2=1; num_vars=1; break;                    
                case LOGMSG_KEYSECTOR_CAPTURED:
                    group=2; line=4; col1=1; col2=4; num_vars=1; break;
                case LOGMSG_KEYSECTOR_LOST:
                    group=2; line=4; col1=1; col2=3; num_vars=1; break;

                case LOGMSG_NETWORK_USER_LEFT:
                    group=2; line=5; col1=1; col2=2; num_vars=1; break;                    
                case LOGMSG_NETWORK_KYT_LEFT:
                    group=2; line=5; col1=1; col2=1; num_vars=1; break;                    
                case LOGMSG_NETWORK_TAER_LEFT:
                    group=2; line=5; col1=1; col2=3; num_vars=1; break;                    
                case LOGMSG_NETWORK_MYK_LEFT:
                    group=2; line=5; col1=1; col2=4; num_vars=1; break;                    
                case LOGMSG_NETWORK_USER_KILLED:
                    group=2; line=5; col1=1; col2=7; num_vars=1; break;                    
                case LOGMSG_NETWORK_KYT_KILLED:
                    group=2; line=5; col1=1; col2=6; num_vars=1; break;                    
                case LOGMSG_NETWORK_TAER_KILLED:
                    group=2; line=5; col1=1; col2=8; num_vars=1; break;                    
                case LOGMSG_NETWORK_MYK_KILLED:
                    group=2; line=5; col1=1; col2=9; num_vars=1; break;                    
                case LOGMSG_NETWORK_YOUWIN:
                    group=2; line=5; col1=1; col2=5; num_vars=1; break;                    
                case LOGMSG_NETWORK_PARASITE_STOPPED:
                    group=2; line=5; col1=1; col2=0xa; num_vars=1; break;
                
                case LOGMSG_CHAT:
                    group=4; line=1; col1=1; col2=1; num_vars=1; break;                     
            };

            /*** nicht unterstützte abfangen ***/
            if (group == 0) return;

            /*** falls kein Sender angegeben, Hoststation annehmen ***/
            if (!sender) sender=ywd->URBact;

            /*** Squad-Meldung: Commander Vehicle-Type ***/
            if (group == 1) {
                line = ywd->VP_Array[sender->TypeID].VOType;
                if (line == 0) line = 0xb;
            };

            /*** Variations? ***/
            if (num_vars>1) {
                var = (rand() % num_vars)+1;
            } else {
                var = 1;
            };

            /*** eine Squadron-Meldung ***/
            sprintf(sname,"%x%x%x%x%x.wav",group,line,col1,col2,var);

            /*** Slot initialisieren und starten ***/
            yw_VOLoadStartSlot(ywd,s,sname,sender,pri);
        };
    };
}

/*-----------------------------------------------------------------*/
void yw_TriggerVoiceOver(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Geht alle momentan aktiven VoiceOvers durch, paßt
**      (bei noch lebendem Sender) die Position an, beendet
**      nicht mehr aktive VoiceOvers und macht ein
**      RefreshSoundCarrier().
**
**  CHANGED
**      24-Sep-97   floh    created
*/
{
    ULONG i;
    for (i=0; i<MAXNUM_VOICEOVERS; i++) {

        struct vo_Slot *s = &(ywd->VoiceOver->slot[i]);

        /*** aktiviert? ***/
        if (s->pri >= 0) {
            /*** lebt der Spender noch? ***/
            if (s->sender->MainState != ACTION_DEAD) {
                /*** dann Position anpassen... ***/
                yw_VOGetPos(ywd,s->sender,&(s->sc.pos));
                s->sc.vec.x = ywd->UVBact->dof.x * ywd->UVBact->dof.v;
                s->sc.vec.y = ywd->UVBact->dof.y * ywd->UVBact->dof.v;
                s->sc.vec.z = ywd->UVBact->dof.z * ywd->UVBact->dof.v;
            };
            /*** VoiceOver noch aktiv? ***/
            if (s->sc.src[0].flags & AUDIOF_ACTIVE) {
                _RefreshSoundCarrier(&(s->sc));
            } else {
                yw_VOFreeSlot(ywd,s);
            };
        };
    };
}

