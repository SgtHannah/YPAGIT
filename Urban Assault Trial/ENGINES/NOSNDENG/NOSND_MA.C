/*

**  $Source$

**  $Revision$

**  $Date$

**  $Locker$

**  $Author$

**

**  Dummy-Sound-Engine, ohne Audio, aber mit Shake- und

**  Paletten-FX.

**

**  (C) Copyright 1996 by A.Weissflog

*/

#include <exec/types.h>

#include <exec/memory.h>

#include <utility/tagitem.h>



#include <stdlib.h>



#include <sys/types.h>  // für Directory-Change

#include <direct.h>



#include "nucleus/nucleus2.h"

#include "nucleus/math.h"

#include "engine/engine.h"

#include "modules.h"

#include "visualstuff/ov_engine.h"

#include "bitmap/displayclass.h"



#include "audio/nosoundengine.h"



/*-------------------------------------------------------------------

**  Engine-Interface

*/

BOOL audio_Open(ULONG id,...);

void audio_Close(void);

void audio_SetAttrs(ULONG tags,...);

void audio_GetAttrs(ULONG tags,...);



struct GET_Engine audio_AE_GET = {

    audio_Open,

    audio_Close,

    audio_SetAttrs,

    audio_GetAttrs,

};



/*-------------------------------------------------------------------

**  Globals

*/



_use_ov_engine



struct NoSoundBase NoSoundBase;



/*** Config-Items ***/

#define AUDIO_NUM_CONFIG_ITEMS (4)

struct ConfigItem audio_ConfigItems[AUDIO_NUM_CONFIG_ITEMS] = {

    { AECONF_Channels,      CONFIG_INTEGER, 4 },

    { AECONF_MasterVolume,  CONFIG_INTEGER, 127 },

    { AECONF_PaletteFX,     CONFIG_INTEGER, 4 },

    { AECONF_ReverseStereo, CONFIG_BOOL, FALSE }

};



/*-------------------------------------------------------------------

**  *** CODE SEGMENT HANDLING ***

**

**  !!!NUR STATIC_LINKING MODELL!!!

*/

/* Einsprung-Routine */

struct GET_Engine *audio_entry(void)

{

    return(&audio_AE_GET);

};



/* und die SegmentInfo-Struktur */

struct SegInfo audio_nosound_engine_seg = {

    { NULL, NULL,

      0, 0,

      "MC2engines:nosound.engine"

    },

    audio_entry,

};



/*-----------------------------------------------------------------*/

BOOL audio_Open(ULONG id,...)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;

    UBYTE old_path[256];



    /*** Gfx-Engine-Handle besorgen ***/

    _get_ov_engine(&id);



    memset(&NoSoundBase, 0, sizeof(NoSoundBase));



    /*** lese Audio-Konfiguration ***/

    _GetConfigItems(NULL, &audio_ConfigItems, AUDIO_NUM_CONFIG_ITEMS);

    NoSoundBase.num_palfx       = (ULONG) audio_ConfigItems[2].data;



    /*** Konfig-Data evtl korrigieren ***/

    if (NoSoundBase.num_palfx > NOSND_NUMPALFX) NoSoundBase.num_palfx=NOSND_NUMPALFX;



    /*** Shake-Parameter ausfüllen ***/

    NoSoundBase.num_shakefx = NOSND_NUMSHAKEFX;

    NoSoundBase.act_rnd = 0;

    for (i=0; i<NOSND_RNDTABLE_ENTRIES; i++) {

        LONG ir = (RAND_MAX/2) - rand();

        FLOAT fr = ((float)ir) / ((float)(RAND_MAX/2));

        NoSoundBase.rnd_table[i] = fr;

    };



    /*** das war's prinzipiell ***/

    return(TRUE);

}



/*-----------------------------------------------------------------*/

void audio_Close(void)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{ }



/*-----------------------------------------------------------------*/

void audio_SetAttrs(ULONG tags,...)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    struct TagItem *tlist = (struct TagItem *) &tags;

    struct TagItem *ti;



    /*** AET_NumPalFX ***/

    if (ti = _FindTagItem(AET_NumPalFX,tlist)) {

        NoSoundBase.num_palfx = ti->ti_Data;

        if (NoSoundBase.num_palfx > NOSND_NUMPALFX) {

            NoSoundBase.num_palfx = NOSND_NUMPALFX;

        };

    };



    /*** Ende ***/

}



/*-----------------------------------------------------------------*/

void audio_GetAttrs(ULONG tags,...)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG *value;

    struct TagList *tlist = (struct TagItem *) &tags;



    /*** AET_MasterVolume ***/

    value = (ULONG *) _GetTagData(AET_MasterVolume, 0, tlist);

    if (value) *value = 20;



    /*** AET_Channels ***/

    value = (ULONG *) _GetTagData(AET_Channels, 0, tlist);

    if (value) *value = 4;



    /*** AET_NumPalFX ***/

    value = (ULONG *) _GetTagData(AET_NumPalFX, 0, tlist);

    if (value) *value = (ULONG) NoSoundBase.num_palfx;



    /*** AET_ReverseStereo ***/

    value = (ULONG *) _GetTagData(AET_ReverseStereo, 0, tlist);

    if (value) *value = FALSE;

}



/*=================================================================**

**  SUPPORT ROUTINEN                                               **

**=================================================================*/



/*-----------------------------------------------------------------*/

void audio_InsertPalFX(struct SoundSource *src)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;

    LONG min_mag = 1000.0;   // BigNum

    LONG min_i;



    /*** suche die kleinste top_palfx-Magnitude ***/

    for (i=0; i < NoSoundBase.num_palfx; i++) {

        if (NULL == NoSoundBase.top_palfx[i]) {

            /*** ein leerer Slot -> abbrechen ***/

            min_mag = 0.0;

            min_i   = i;

            i = NoSoundBase.num_palfx;

        } else {

            if (NoSoundBase.top_palfx[i]->pfx_actmag < min_mag) {

                min_mag = NoSoundBase.top_palfx[i]->pfx_actmag;

                min_i   = i;

            };

        };

    };



    /*** kleinste Magnitude klein genug? ***/

    if (min_mag < src->pfx_actmag) NoSoundBase.top_palfx[min_i] = src;

}



/*-----------------------------------------------------------------*/

void audio_InsertShakeFX(struct SoundSource *src)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;

    LONG min_mag = 1000.0;   // BigNum

    LONG min_i;



    /*** suche die kleinste top_shakefx-Magnitude ***/

    for (i=0; i < NoSoundBase.num_shakefx; i++) {

        if (NULL == NoSoundBase.top_shakefx[i]) {

            /*** ein leerer Slot -> abbrechen ***/

            min_mag = 0.0;

            min_i   = i;

            i = NoSoundBase.num_shakefx;

        } else {

            if (NoSoundBase.top_shakefx[i]->shk_actmag < min_mag) {

                min_mag = NoSoundBase.top_shakefx[i]->shk_actmag;

                min_i   = i;

            };

        };

    };



    /*** kleinste Magnitude klein genug? ***/

    if (min_mag < src->shk_actmag) NoSoundBase.top_shakefx[min_i] = src;

}



/*-----------------------------------------------------------------*/

FLOAT audio_Rand(void)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    FLOAT r = NoSoundBase.rnd_table[NoSoundBase.act_rnd++];

    if (NoSoundBase.act_rnd >= NOSND_RNDTABLE_ENTRIES) NoSoundBase.act_rnd=0;

    return(r);

}



/*=================================================================**

**  INTERFACE ROUTINEN                                             **

**=================================================================*/



/*-----------------------------------------------------------------*/

void audio_InitSoundCarrier(struct SoundCarrier *sc)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;

    memset(sc, 0, sizeof(struct SoundCarrier));

    for (i=0; i<MAX_SOUNDSOURCES; i++) sc->src[i].sc = sc;

}



/*-----------------------------------------------------------------*/

void audio_KillSoundCarrier(struct SoundCarrier *sc)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{ }



/*-----------------------------------------------------------------*/

void audio_StartAudioFrame(ULONG frame_time, 

                           struct flt_triple *vwr_pos,

                           struct flt_triple *vwr_vec,

                           struct flt_m3x3   *vwr_mx)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;



    /*** Frame-Data übernehmen ***/

    NoSoundBase.time_stamp += frame_time;

    NoSoundBase.vwr_pos = *vwr_pos;

    NoSoundBase.vwr_vec = *vwr_vec;

    NoSoundBase.vwr_mx  = *vwr_mx;



    /*** die Top-N zurücksetzen ***/

    memset(NoSoundBase.top_palfx,0,sizeof(NoSoundBase.top_palfx));

    memset(NoSoundBase.top_shakefx,0,sizeof(NoSoundBase.top_shakefx));

}



/*-----------------------------------------------------------------*/

void audio_StartSoundSource(struct SoundCarrier *sc, ULONG num)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    struct SoundSource *src = &(sc->src[num]);



    src->start_time = NoSoundBase.time_stamp;



    /*** Sound ***/

    if (src->sample) {

        src->flags |= AUDIOF_ACTIVE;

        src->flags &= ~AUDIOF_PLAYING;

        src->res_volume = 127;

        src->res_pitch  = src->sample->SamplesPerSec + src->pitch;

        src->res_pan    = 64;   // Stereo-Position auf Mitte

    };



    /*** Pal-FX ***/

    if (src->flags & AUDIOF_HASPFX) {

        src->flags |= AUDIOF_PFXACTIVE;

        src->flags &= ~AUDIOF_PFXPLAYING;

        src->pfx_actmag = src->pfx->mag0;

    };



    /*** Shake-FX ***/

    if (src->flags & AUDIOF_HASSHAKE) {

        src->flags |= AUDIOF_SHKACTIVE;

        src->flags &= ~AUDIOF_SHKPLAYING;

        src->shk_actmag = src->shk->mag0;

    };

}



/*-----------------------------------------------------------------*/

void audio_EndSoundSource(struct SoundCarrier *sc, ULONG num)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    struct SoundSource *src = &(sc->src[num]);

    src->flags &= ~(AUDIOF_ACTIVE | AUDIOF_PFXACTIVE | AUDIOF_SHKACTIVE);

}



/*-----------------------------------------------------------------*/

void audio_RefreshPalFX(struct SoundCarrier *sc, FLOAT dist)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;

    FLOAT divider = dist / 300.0;



    for (i=0; i<MAX_SOUNDSOURCES; i++) {



        struct SoundSource *src = &(sc->src[i]);

        if (src->flags & AUDIOF_HASPFX) {



            FLOAT dt;   // Position in Timeline (0.0 .. 1.0)



            /*** TimeOut bei OneShots? ***/

            if (src->flags & AUDIOF_PFXACTIVE) {

                if (!(src->flags & AUDIOF_LOOPDALOOP)) {

                    ULONG stop_time = src->start_time + src->pfx->time;

                    if (stop_time < NoSoundBase.time_stamp) {

                        src->flags &= ~AUDIOF_PFXACTIVE;

                    };

                };

            };



            /*** inaktive FX ignorieren ***/

            if (!(src->flags & AUDIOF_PFXACTIVE)) continue;



            /*** aktuelle Magnitude berechnen (nur OneShots) ***/

            if (!(src->flags & AUDIOF_LOOPDALOOP)) {

                dt = ((FLOAT)(NoSoundBase.time_stamp - src->start_time))/

                     ((FLOAT)src->pfx->time);

                src->pfx_actmag = src->pfx->mag0 +

                     ((src->pfx->mag1 - src->pfx->mag0)*dt);

            } else {

                src->pfx_actmag = src->pfx->mag0;

            };



            /*** Entfernung zum Viewer ! ***/

            if (divider >= 1.0) src->pfx_actmag /= divider;



            /*** in das top_palfx[] Array einsortieren ***/

            audio_InsertPalFX(src);

        };

    };

}



/*-----------------------------------------------------------------*/

void audio_RefreshShakeFX(struct SoundCarrier *sc, FLOAT dist)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    ULONG i;



    for (i=0; i<MAX_SOUNDSOURCES; i++) {



        struct SoundSource *src = &(sc->src[i]);

        if (src->flags & AUDIOF_HASSHAKE) {



            FLOAT divider;



            /*** Timeout bei One-Shots? ***/

            if (src->flags & AUDIOF_SHKACTIVE) {

                if (!(src->flags & AUDIOF_LOOPDALOOP)) {

                    ULONG stop_time = src->start_time + src->shk->time;

                    if (stop_time < NoSoundBase.time_stamp) {

                        src->flags &= ~AUDIOF_SHKACTIVE;

                    };

                };

            };



            /*** inaktive FX ignorieren ***/

            if (!(src->flags & AUDIOF_SHKACTIVE)) continue;



            /*** aktuelle Magnitude berechnen (nur OneShots) ***/

            if (!(src->flags & AUDIOF_LOOPDALOOP)) {

                FLOAT dt;

                dt = ((FLOAT)(NoSoundBase.time_stamp - src->start_time))/

                    ((FLOAT)src->shk->time);

                src->shk_actmag = src->shk->mag0 +

                    ((src->shk->mag1 - src->shk->mag0)*dt);

            } else {

                src->shk_actmag = src->shk->mag0;

            };



            /*** Entfernung zum Viewer ***/

            divider = dist * src->shk->mute;

            if (divider >= 1.0) src->shk_actmag /= divider;



            /*** in das top_shakefx-Array einsortieren ***/

            audio_InsertShakeFX(src);

        };

    };

}



/*-----------------------------------------------------------------*/

void audio_RefreshSoundCarrier(struct SoundCarrier *sc)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    FLOAT dist;

    FLOAT dx,dy,dz;



    /*** berechne Entfernung zum Hörer ***/

    dx = sc->pos.x - NoSoundBase.vwr_pos.x;

    dy = sc->pos.y - NoSoundBase.vwr_pos.y;

    dz = sc->pos.z - NoSoundBase.vwr_pos.z;



    dist = nc_sqrt(dx*dx + dy*dy + dz*dz);

    if (dist < MAX_AUDIO_DISTANCE) {

        if (dist < 2400.0) {

            if (NoSoundBase.num_palfx > 0) audio_RefreshPalFX(sc,dist);

            audio_RefreshShakeFX(sc,dist);

        };

    };

}



/*-----------------------------------------------------------------*/

void audio_PlayFinalPaletteFX(void)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    LONG i = 0;

    LONG slot[NOSND_NUMPALFX+1];

    LONG weight[NOSND_NUMPALFX+1];

    LONG mag = 256;

    Object *gfxo;

    struct SoundSource *src;



    while ((src = NoSoundBase.top_palfx[i]) && (i<NoSoundBase.num_palfx)) {

        mag -= src->pfx_actmag;

        slot[i]   = src->pfx->slot;

        weight[i] = (ULONG) (src->pfx_actmag * 256.0);

        mag -= weight[i];

        i++;

    };



    /*** wenn keine Paletten-FX und Default bereits an, Abbruch ***/

    if (i == 0) {

        if (NoSoundBase.palfx_clean) return;

        else NoSoundBase.palfx_clean = TRUE;

    } else   NoSoundBase.palfx_clean = FALSE;



    /*** mag ist die "Rest-Magnitude" für die Default-Palette ***/

    if (mag > 0) {

        /*** von der Default-Palette ist noch was übrig ***/

        slot[i]   = 0;    // Default-Palette

        weight[i] = mag;

        i++;

    };



    /*** mixen... ***/

    _OVE_GetAttrs(OVET_Object, &gfxo, TAG_DONE);

    if (gfxo) {

        struct disp_mixpal_msg dmm;

        dmm.num    = i;

        dmm.slot   = &(slot[0]);

        dmm.weight = &(weight[0]);

        _methoda(gfxo, DISPM_MixPalette, &dmm);

    };

}



/*-----------------------------------------------------------------*/

void audio_PlayFinalShakeFX(void)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    LONG i=0;

    struct SoundSource *src;

    FLOAT shake_x,shake_y,shake_z;

    struct flt_m3x3 *m = &(NoSoundBase.shk_mx);



    shake_x = 0.0;

    shake_y = 0.0;

    shake_z = 0.0;

    while ((src = NoSoundBase.top_shakefx[i]) && (i<NoSoundBase.num_shakefx)) {

        shake_x += audio_Rand() * src->shk_actmag * src->shk->x;

        shake_y += audio_Rand() * src->shk_actmag * src->shk->y;

        shake_z += audio_Rand() * src->shk_actmag * src->shk->z;

        i++;

    };



    /*** falls mindestens eine Shake-Source ... ***/

    if (i>0) {



        LONG dtime;

        FLOAT sin_x,cos_x,sin_y,cos_y,sin_z,cos_z;



        /*** Rotations-Matrix aufbauen ***/

        sin_x=sin(shake_x); sin_y=sin(shake_y); sin_z=sin(shake_z);

        cos_x=cos(shake_x); cos_y=cos(shake_y); cos_z=cos(shake_z);



        m->m11 = cos_z*cos_y - sin_z*sin_x*sin_y;

        m->m12 = -sin_z*cos_x;

        m->m13 = cos_z*sin_y + sin_z*sin_x*cos_y;



        m->m21 = sin_z*cos_y + cos_z*sin_x*sin_y;

        m->m22 = cos_z*cos_x;

        m->m23 = sin_z*sin_y - cos_z*sin_x*cos_y;



        m->m31 = -cos_x*sin_y;

        m->m32 = sin_x;

        m->m33 = cos_x*cos_y;



    } else {

        memset(m,0,sizeof(struct flt_m3x3));

        m->m11 = 1.0;

        m->m22 = 1.0;

        m->m33 = 1.0;

    };

}



/*-----------------------------------------------------------------*/

struct flt_m3x3 *audio_EndAudioFrame(void)

/*

**  CHANGED

**      21-Nov-96   floh    created

*/

{

    /*** Palette-FX-Handling ***/

    if (NoSoundBase.num_palfx > 0) audio_PlayFinalPaletteFX();



    /*** Shake-FX-Handling (zerstört NoSoundBase.vwr_mx!) ***/

    audio_PlayFinalShakeFX();



    /*** das war's... ***/

    return(&(NoSoundBase.shk_mx));

}



/*-----------------------------------------------------------------*/

void audio_FlushAudio(void)

/*

**  CHANGED

**      24-Jan-97   floh    created

*/

{ }





