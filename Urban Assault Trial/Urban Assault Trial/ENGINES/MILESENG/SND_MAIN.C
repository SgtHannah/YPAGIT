/*
**  $Source: PRG:VFM/Engines/MilesEngine/snd_main.c,v $
**  $Revision: 38.3 $
**  $Date: 1998/01/06 16:40:19 $
**  $Locker:  $
**  $Author: floh $
**
**  Die auf dem Miles-Treiber-System aufbauende VFM-Audio-Engine.
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

#include "audio/milesengine.h"
#include "audio/cdplay.h"

void audio_PlaySampleFragment(struct snd_Channel *,struct SoundSource *,ULONG);

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

/*-----------------------------------------------------------------**
**  Die Miles-Engine funktioniert ausschließlich im statischen     **
**  Linkmodell, also keine Specific-GET.                           **
**-----------------------------------------------------------------*/

/*-------------------------------------------------------------------
**  Globals
*/

_use_ov_engine

struct MilesBase MilesBase1;
#define MilesBase MilesBase1
struct cd_data cd_data1;
#define cd_data cd_data1

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
struct SegInfo audio_miles_engine_seg = {
    { NULL, NULL,       /* ln_Succ, ln_Pred */
      0, 0,             /* ln_Type, ln_Pri  */
      "MC2engines:miles.engine"   /* der Segment-Name */
    },
    audio_entry,          /* Entry()-Adresse */
};

/*=================================================================**
**                                                                 **
**  END OF SAMPLE CALLBACK ROUTINE                                 **
**                                                                 **
**=================================================================*/

void nc_LogMsg(STRPTR string, ...);

/*-----------------------------------------------------------------*/
void audio_EOS_CallBack(snd_Channel *sample)
/*
**  FUNCTION
**      End-Of-Sample-Callback-Routine, löscht AUDIOF_ACTIVE
**      und kümmert sich um die korrekte Verkettung der
**      Extended Sample Definition Fragmente.
**
**  CHANGED
**      28-Dec-96   floh    created
**      18-Feb-97   floh    jetzt mit SoundKit-Kapselung
*/
{
    ULONG old_channel,new_channel;
    struct SoundSource *src=0;

    /*** zuerst mal die SoundSource auf diesem Kanal suchen ***/
    for (old_channel=0; old_channel<MilesBase.num_channels; old_channel++) {
        if (MilesBase.ch[old_channel].handle == sample) {
            src = MilesBase.ch[old_channel].src;
            break;
        };
    };

    if (src) {
        /*** diese noch aktiv? ***/
        if (src->flags & AUDIOF_ACTIVE) {
            if (src->flags & AUDIOF_HASEXTSMP) {
                /*** Extended Sample Definition: etwas komplizierter ***/
                src->frag_act++;
                if (src->frag_act >= src->ext_smp->count) {
                    if (src->flags & AUDIOF_LOOPDALOOP) {
                        src->frag_act = 0;
                    } else {
                        src->flags &= ~AUDIOF_ACTIVE;
                    };
                };
                if (src->flags & AUDIOF_ACTIVE) {

                    /*** wenn möglich das neue Fragment auf einem ***/
                    /*** freien Hardware-Kanal parallel starten,  ***/
                    /*** um Knackser zu vermeiden.                ***/
                    for (new_channel=0; new_channel<MilesBase.num_channels; new_channel++) {
                        if (NULL == MilesBase.ch[new_channel].src) {
                            /*** auf einen freien Kanal wechseln ***/
                            sample = MilesBase.ch[new_channel].handle;
                            MilesBase.ch[old_channel].src=NULL;
                            MilesBase.ch[new_channel].src=src;
                            break;
                        };
                        /*** wenn Schleife durchläuft, bleibt <sample> ***/
                    };
                    audio_PlaySampleFragment(sample,src,0);
                };
            } else {
                /*** konventionell: deaktivieren, und zurück ***/
                src->flags &= ~AUDIOF_ACTIVE;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
BOOL audio_Open(ULONG id,...)
/*
**  FUNCTION
**      Liest Konfiguration und initialisiert Miles-AIL.
**
**  INPUTS
**
**  RESULTS
**      TRUE    -> alles OK
**      FALSE   -> Miles-AIL-Init hat nicht geklappt
**
**  CHANGED
**      14-Apr-96   floh    created
**      28-Apr-96   floh    Feinschliff an den Preferences, zwecks
**                          besserer Performance.
**      05-Aug-96   floh    kein Abbruch mehr, wenn DIG_INI Driver
**                          nicht geladen wurde
**      14-Aug-96   floh    benötigt jetzt das Gfx-Engine-Handle
**      16-Aug-96   floh    + AECONF_ReverseStereo
**      25-Sep-96   floh    + Shake-FX
**      26-Sep-96   floh    + more Shake-FX...
**      04-Oct-96   floh    + wechselt vor der Initialisierung des
**                            Treibers nach "DRIVERS/"
**      18-Feb-97   floh    + jetzt mit SoundKit-Interface
*/
{
    ULONG i;
    UBYTE old_path[256];

    /*** Gfx-Engine-Handle besorgen ***/
    _get_ov_engine(&id);

    memset(&MilesBase, 0, sizeof(MilesBase));

    /*** lese Audio-Konfiguration ***/
    _GetConfigItems(NULL, &audio_ConfigItems, AUDIO_NUM_CONFIG_ITEMS);
    MilesBase.num_channels    = (ULONG) audio_ConfigItems[0].data;
    MilesBase.master_volume   = (ULONG) audio_ConfigItems[1].data;
    MilesBase.num_palfx       = (ULONG) audio_ConfigItems[2].data;
    MilesBase.rev_stereo      = (ULONG) audio_ConfigItems[3].data;

    /*** Konfig-Data evtl korrigieren ***/
    if (MilesBase.master_volume < 0)        MilesBase.master_volume = 0;
    else if (MilesBase.master_volume > 127) MilesBase.master_volume = 127;
    if (MilesBase.num_channels < 1) MilesBase.num_channels = 1;
    if (MilesBase.num_palfx > MILES_NUMPALFX) MilesBase.num_palfx=MILES_NUMPALFX;

    /*** Shake-Parameter ausfüllen ***/
    MilesBase.num_shakefx = MILES_NUMSHAKEFX;
    MilesBase.act_rnd = 0;
    for (i=0; i<MILES_RNDTABLE_ENTRIES; i++) {
        LONG ir = (RAND_MAX/2) - rand();
        FLOAT fr = ((float)ir) / ((float)(RAND_MAX/2));
        MilesBase.rnd_table[i] = fr;
    };

    /*** SoundKit initialisieren (kann auch schiefgehen ***/
    MilesBase.driver = snd_InitAudio(MilesBase.num_channels);
    if (NULL == MilesBase.driver) {
        _LogMsg("Couldn't initialize audio!\n");
        return(TRUE); // kein Bug
    };

    /*** Channels allokieren (kann weniger sein als angefordert!) ***/
    for (i=0; i<MilesBase.num_channels; i++) {
        MilesBase.ch[i].handle = snd_AllocChannel(MilesBase.driver);
        if (NULL == MilesBase.ch[i].handle) break;
    };
    MilesBase.num_channels = i;

    /*** Anfangs-Master-Volume setzen ***/
    snd_SetMasterVolume(MilesBase.driver,MilesBase.master_volume);

    /*** CD-Player initialisieren ***/
    _InitCDPlayer();

    /*** das war's prinzipiell ***/
    return(TRUE);
}

/*-----------------------------------------------------------------*/
void audio_Close(void)
/*
**  CHANGED
**      14-Apr-96   floh    created
**      18-Feb-97   floh    jetzt mit SoundKit-Interface
*/
{
    /*** CD-Player schließen ***/
    _FreeCDPlayer();

    snd_KillAudio(MilesBase.driver);
}

/*-----------------------------------------------------------------*/
void audio_SetAttrs(ULONG tags,...)
/*
**  CHANGED
**      17-Apr-96   floh    created
**      05-Aug-96   floh    Abbruch, wenn AIL nicht initialisiert
**      16-Aug-96   floh    + AET_Channels
**                          + AET_NumPalFX
**                          + AET_ReverseStereo
**      31-Oct-96   floh    + Bugfix: in AET_Channels wurde der
**                            neue Pfad "DRIVERS/" nicht beachtet
**      18-Feb-97   floh    + SoundKit-Interface
**      19-Feb-97   floh    + AET_Channels jetzt per snd_ModifyNumChannels()
*/
{
    struct TagItem *tlist = (struct TagItem *) &tags;
    struct TagItem *ti;
    ULONG master_volume;

    /*** AET_NumPalFX ***/
    if (ti = _FindTagItem(AET_NumPalFX,tlist)) {
        MilesBase.num_palfx = ti->ti_Data;
        if (MilesBase.num_palfx > MILES_NUMPALFX) {
            MilesBase.num_palfx = MILES_NUMPALFX;
        };
    };


    if (NULL != MilesBase.driver) {

        /*** AET_MasterVolume ***/
        master_volume = _GetTagData(AET_MasterVolume, 0xffff, tlist);
        if (master_volume != 0xffff) {
            MilesBase.master_volume = master_volume;
            snd_SetMasterVolume(MilesBase.driver,MilesBase.master_volume);
        };

        /*** AET_ReverseStereo ***/
        if (ti = _FindTagItem(AET_ReverseStereo,tlist)) {
            MilesBase.rev_stereo = ti->ti_Data;
        };

        /*** AET_Channels ***/
        if (ti = _FindTagItem(AET_Channels,tlist)) {

            //ULONG i;
            //UBYTE old_path[256];
            //MilesBase.num_channels = ti->ti_Data;
            //if (MilesBase.num_channels == 0) MilesBase.num_channels = 1;

            //MilesBase.driver = snd_ModifyNumChannels(MilesBase.driver,MilesBase.num_channels);
            //for (i=0; i<MilesBase.num_channels; i++) {
            //    MilesBase.ch[i].handle = snd_AllocChannel(MilesBase.driver);
            //    if (NULL == MilesBase.ch[i].handle) break;
            //};
            //MilesBase.num_channels = i;
            //snd_SetMasterVolume(MilesBase.driver,MilesBase.master_volume);
        };
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void audio_GetAttrs(ULONG tags,...)
/*
**  CHANGED
**      17-Apr-96   floh    created
**      16-Aug-96   floh    + AET_Channels
**                          + AET_NumPalFX
**                          + AET_ReverseStereo
*/
{
    ULONG *value;
    struct TagList *tlist = (struct TagItem *) &tags;

    /*** AET_MasterVolume ***/
    value = (ULONG *) _GetTagData(AET_MasterVolume, 0, tlist);
    if (value) *value = (ULONG) MilesBase.master_volume;

    /*** AET_Channels ***/
    value = (ULONG *) _GetTagData(AET_Channels, 0, tlist);
    if (value) *value = (ULONG) MilesBase.num_channels;

    /*** AET_NumPalFX ***/
    value = (ULONG *) _GetTagData(AET_NumPalFX, 0, tlist);
    if (value) *value = (ULONG) MilesBase.num_palfx;

    /*** AET_ReverseStereo ***/
    value = (ULONG *) _GetTagData(AET_ReverseStereo, 0, tlist);
    if (value) *value = (ULONG) MilesBase.rev_stereo;
}

/*=================================================================**
**  SUPPORT ROUTINEN                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
ULONG audio_GetExtSmpFrag(struct SoundSource *src, LONG age, LONG *rel_age)
/*
**  FUNCTION
**      Errechnet aus dem Alter einer aktiven Extended Sample
**      Definition (in Millisec) das momentan aktive Fragment.
**      Das ist nur eine Schätzung, eine exakte Berechnung
**      ist nicht ohne größeren Aufwand möglich (weil sich
**      die resultierende Tonhöhe unvorhersehbar ändern kann).
**
**  INPUTS
**      src     - die betroffene SoundSource (AUDIOF_HASEXTSMP
**                muß gesetzt sein
**      age     - Alter in Millisekunden (muß > 0 sein!).
**      rel_age - LONG Pointer, in den das relative Zeit-Offset
**                im gefundenen Fragment zurückgeschrieben wird.
**                Darf NULL sein, falls das nicht
**                interessiert.
**
**  RESULTS
**      frag    - Nummer des aktiven Fragments. Ist gleich
**                src->ext_smp->count, falls die SoundSource
**                bereits beendet sein müßte.
**                Falls ein unendlicher Loop in der Sample-
**                Definition existiert, wird mit diesem
**                Fragment abgebrochen.
**
**  CHANGED
**      28-Dec-96   floh    created
*/
{
    ULONG frag;

    for (frag=0; frag<src->ext_smp->count; frag++) {

        LONG freq;
        ULONG frag_lifetime;
        struct SampleFragment *act_sf = &(src->ext_smp->frag[0]);

        /*** unendlicher Loop, Fragment gefunden ***/
        if (act_sf->loop==0) break;

        /*** Fragment-Lifetime abziehen von wahrem Alter ***/
        freq = act_sf->sample->SamplesPerSec + act_sf->pitch + src->pitch;
        if (freq <= 0) freq=1;
        frag_lifetime = ((act_sf->real_len*act_sf->loop)<<10)/freq;

        /*** aktuelles Fragment gefunden? ***/
        if (age < frag_lifetime) break;
        age -= frag_lifetime;
    };

    /*** relatives Zeit-Offset im aktuellen Fragment zurückschreiben ***/
    if (rel_age) *rel_age=age;

    return(frag);
}

/*-----------------------------------------------------------------*/
void audio_InsertSoundSource(struct SoundSource *src)
/*
**  FUNCTION
**      Versucht, die übergebene SoundSource anhand ihrer
**      Priorität in das TopN-Array einzuordnen.
**
**  INPUTS
**      sc  -> SoundCarrier, in dem <src> eingebettet ist
**      src -> SoundSource, pri muß korrekt sein
**
**  RESULTS
**      ---
**
**  CHANGED
**      15-Apr-96   floh    created
**      05-Aug-96   floh    Abbruch, wenn AIL nicht initialisiert
**      06-Jul-98   floh    + min_i konnte offensichtlich uninitialisiert
**                            als Array-Index genommen werden.
*/
{
    ULONG i;
    LONG min_pri = 1000000;   // BigNum
    LONG min_i   = -1;

    if (NULL == MilesBase.driver) return;

    /*** suche die kleinste TopN-Priorität ***/
    for (i=0; i<MilesBase.num_channels; i++) {
        if (NULL == MilesBase.top_n[i]) {
            /*** ein leerer Slot -> abbrechen ***/
            min_pri = 0;
            min_i   = i;
            i = MilesBase.num_channels;
        } else {
            if (MilesBase.top_n[i]->pri < min_pri) {
                min_pri = MilesBase.top_n[i]->pri;
                min_i   = i;
            };
        };
    };

    /*** kleinste Pri klein genug? ***/
    if (min_i == -1) nc_LogMsg("-> audio_InsertSoundSource(): <min_i> not initialized.\n");
    if ((min_i != -1) && (min_pri < src->pri)) MilesBase.top_n[min_i] = src;
}

/*-----------------------------------------------------------------*/
void audio_InsertPalFX(struct SoundSource *src)
/*
**  FUNCTION
**      Versucht, den Paletten-FX der SoundSource anhand
**      seiner aktuellen Magnitude in das top_palfx
**      Array einzuordnen.
**
**      Voraussetzung: an der SoundSource muß auch
**      ein Paletten-FX attached sein!
**
**  CHANGED
**      14-Aug-96   floh    created
**      25-Sep-96   floh    ooops, kleiner Bugfix, ich habe einmal
**                          lesenderweise auf das top_n[] Array
**                          referenziert, anstatt auf das top_shakefx[],
**                          dürfte aber kaum aufgefallen sein...
**      06-Jul-98   floh    + min_i konnte offensichtlich uninitialisiert
**                            als Array-Index genommen werden.
*/
{
    ULONG i;
    LONG min_mag = 1000000.0;   // BigNum
    LONG min_i   = -1;

    /*** suche die kleinste top_palfx-Magnitude ***/
    for (i=0; i<MilesBase.num_palfx; i++) {
        if (NULL == MilesBase.top_palfx[i]) {
            /*** ein leerer Slot -> abbrechen ***/
            min_mag = 0.0;
            min_i   = i;
            i = MilesBase.num_palfx;
        } else {
            if (MilesBase.top_palfx[i]->pfx_actmag < min_mag) {
                min_mag = MilesBase.top_palfx[i]->pfx_actmag;
                min_i   = i;
            };
        };
    };

    /*** kleinste Magnitude klein genug? ***/
    if (min_i == -1) nc_LogMsg("-> audio_InsertPalFX(): <min_i> not initialized.\n");
    if ((min_i != -1) && (min_mag < src->pfx_actmag)) MilesBase.top_palfx[min_i] = src;
}

/*-----------------------------------------------------------------*/
void audio_InsertShakeFX(struct SoundSource *src)
/*
**  FUNCTION
**      Versucht, den Shake-FX der SoundSource anhand
**      seiner aktuellen Magnitude in das top_shakefx
**      Array einzuordnen.
**
**      Voraussetzung: an der SoundSource muß auch
**      ein Shake-FX attached sein!
**
**  CHANGED
**      25-Sep-96   floh    created
**      06-Jul-98   floh    + min_i konnte offensichtlich uninitialisiert
**                            als Array-Index genommen werden.
*/
{
    ULONG i;
    LONG min_mag = 1000.0;   // BigNum
    LONG min_i   = -1;

    /*** suche die kleinste top_shakefx-Magnitude ***/
    for (i=0; i < MilesBase.num_shakefx; i++) {
        if (NULL == MilesBase.top_shakefx[i]) {
            /*** ein leerer Slot -> abbrechen ***/
            min_mag = 0.0;
            min_i   = i;
            i = MilesBase.num_shakefx;
        } else {
            if (MilesBase.top_shakefx[i]->shk_actmag < min_mag) {
                min_mag = MilesBase.top_shakefx[i]->shk_actmag;
                min_i   = i;
            };
        };
    };

    /*** kleinste Magnitude klein genug? ***/
    if (min_i == -1) nc_LogMsg("-> audio_InsertShakeFX(): <min_i> not initialized.\n");
    if ((min_i != -1) && (min_mag < src->shk_actmag)) MilesBase.top_shakefx[min_i] = src;
}

/*-----------------------------------------------------------------*/
FLOAT audio_Rand(void)
/*
**  FUNCTION
**      Returniert eine Pseudo-Zufalls-Zahl zwischen
**      -1.0 und 1.0 per Table-Lookup (Periode MILES_RNDTABLE_ENTRIES).
**
**  CHANGED
**      26-Sep-96   floh    created
*/
{
    FLOAT r = MilesBase.rnd_table[MilesBase.act_rnd++];
    if (MilesBase.act_rnd >= MILES_RNDTABLE_ENTRIES) MilesBase.act_rnd=0;
    return(r);
}

/*=================================================================**
**  INTERFACE ROUTINEN                                             **
**=================================================================*/

/*-----------------------------------------------------------------*/
void audio_InitSoundCarrier(struct SoundCarrier *sc)
/*
**  FUNCTION
**      Initialisiert die übergebene SoundCarrier-Struktur,
**      das sollte nur einmal pro existierender Carrier-Struktur
**      passieren.
**
**  CHANGED
**      14-Apr-96   floh    created
*/
{
    ULONG i;
    memset(sc, 0, sizeof(struct SoundCarrier));
    for (i=0; i<MAX_SOUNDSOURCES; i++) sc->src[i].sc = sc;
}

/*-----------------------------------------------------------------*/
void audio_KillSoundCarrier(struct SoundCarrier *sc)
/*
**  FUNCTION
**      Checkt, ob eine SoundSource des Carriers gerade
**      ausgegeben wird, wenn dem so ist, wird sie
**      gecancelt und kann somit freigegeben werden.
**
**  CHANGED
**      14-Apr-96   floh    created
**      17-Apr-96   floh    Test aus SMP_DONE raus, hat Probleme
**                          mit Debugger verursacht (wahrscheinlich,
**                          weil der die Interrupts abwürgt).
**      05-Aug-96   floh    Abbruch, wenn AIL nicht initialisiert
**      06-Sep-96   floh    + Bugfix: falls ein "Treffer" im Mixer-Channel-
**                            Array auftritt, wurde der dortige Src-Pointer
**                            nicht auf NULL gesetzt -> unter gewissen
**                            Umständen konnten damit ungültige Pointer
**                            referenziert werden -> Crash.
**                          + Bugfix: Der Div-By-Zero-Fehler trat
**                            wahrscheinlich wegen einem ungenügenden
**                            Cleanup in _KillSoundCarrier() auf.
**                            Jetzt wird generell im ch[] UND im
**                            top_n[] Array geguckt, ob eine SoundSource
**                            aus dem Carrier dort referenziert wird,
**                            und löscht in diesem Fall den Pointer.
**      20-Jan-97   floh    + macht jetzt auf jede SoundSource
**                            pauschal ein audio_EndSoundSource()
**      18-Feb-97   floh    + SoundKit-Interface
**      05-Apr-97   floh    + Bugfix: ShakeFX und PalFX-Arrays wurden
**                            nicht korrekt aufgeräumt.
*/
{
    ULONG i,j;
    UBYTE *start = (UBYTE *) sc;
    UBYTE *end   = start + sizeof(struct SoundCarrier);

    /*** teste im ch[] und top_n[] Array, ob eine Src benutzt wird ***/
    for (i=0; i<MilesBase.num_channels; i++) {

        struct SoundSource *src;

        /*** Mixer-Channel-Test ***/
        if (NULL != MilesBase.driver) {
            src = MilesBase.ch[i].src;
            if ((((UBYTE *)src) > start) && (((UBYTE *)src) < end)) {
                /*** uggh, spielt gerade, also beenden und löschen ***/
                snd_Channel *act_smp = MilesBase.ch[i].handle;
                src->flags &= ~(AUDIOF_ACTIVE|AUDIOF_PLAYING);
                snd_EndChannel(MilesBase.driver,act_smp);
                MilesBase.ch[i].src = NULL;
            };

            /*** Top-N-Tests ***/
            src = MilesBase.top_n[i];
            if ((((UBYTE *)src) > start) && (((UBYTE *)src) < end)) {
                /*** ok, raus damit ***/
                MilesBase.top_n[i] = NULL;
                src->flags &= ~(AUDIOF_ACTIVE|AUDIOF_PLAYING);
            };
        };
    };

    /*** Top-Pal-FX aufräumen ***/
    for (i=0; i<MilesBase.num_palfx; i++) {
        struct SoundSource *src = MilesBase.top_palfx[i];
        if ((((UBYTE *)src) > start) && (((UBYTE *)src) < end)) {
            MilesBase.top_palfx[i] = NULL;
            src->flags &= ~(AUDIOF_PFXACTIVE|AUDIOF_PFXPLAYING);
        };
    };

    /*** Top-Shake-FX aufräumen ***/
    for (i=0; i<MilesBase.num_shakefx; i++) {
        struct SoundSource *src = MilesBase.top_shakefx[i];
        if ((((UBYTE *)src) > start) && (((UBYTE *)src) < end)) {
            MilesBase.top_shakefx[i] = NULL;
            src->flags &= ~(AUDIOF_SHKACTIVE|AUDIOF_SHKPLAYING);
        };
    };

    /*** auf jede SoundSource ein audio_EndSoundSource() ***/
    for (i=0; i<MAX_SOUNDSOURCES; i++) audio_EndSoundSource(sc,i);

    /*** der SoundCarrier ist jetzt clean ***/
}

/*-----------------------------------------------------------------*/
void audio_StartAudioFrame(ULONG frame_time,
                           struct flt_triple *vwr_pos,
                           struct flt_triple *vwr_vec,
                           struct flt_m3x3   *vwr_mx)
/*
**  FUNCTION
**      Startet einen neuen Audio-Frame, mit frischer
**      Information über den "Rest der Welt".
**
**  INPUTS
**      frame_time  -> die FrameTime in 1/1000 sec
**      vwr_pos     -> aktuelle Position des Viewers
**      vwr_vec     -> aktueller Geschw.-Vektor des Viewers
**      vwr_mx      -> aktuelle Rotations-Matrix des Viewers
**
**  CHANGED
**      14-Apr-96   floh    created
**      25-Sep-96   floh    top_shakefx Array wird zurückgesetzt
*/
{
    ULONG i;

    /*** Frame-Data übernehmen ***/
    MilesBase.time_stamp += frame_time;
    MilesBase.vwr_pos = *vwr_pos;
    MilesBase.vwr_vec = *vwr_vec;
    MilesBase.vwr_mx  = *vwr_mx;

    /*** die Top-N zurücksetzen ***/
    memset(MilesBase.top_n,0,sizeof(MilesBase.top_n));
    memset(MilesBase.top_palfx,0,sizeof(MilesBase.top_palfx));
    memset(MilesBase.top_shakefx,0,sizeof(MilesBase.top_shakefx));

    /*** Für den CDPlayer brauchen wir eine verbindliche Zeit ***/
    cd_data.timestamp = MilesBase.time_stamp;
}

/*-----------------------------------------------------------------*/
void audio_StartSoundSource(struct SoundCarrier *sc, ULONG num)
/*
**  FUNCTION
**      Macht eine SoundSource aktiv, spult sie an den
**      Anfang zurück. Beim nächsten _RefreshSoundCarrier()
**      wird dann der 3D-Effekt reingerechnet, wirklich
**      etwas hören kann man frühestens etwas beim nächsten
**      _EndAudioFrame() -> so die resultierende Priorität
**      der SoundSource hoch genug ist.
**
**  INPUTS
**      sc      -> SoundCarrier, zu dem die SoundSource gehört
**      num     -> Nummer der SoundSource
**
**  CHANGED
**      15-Apr-96   floh    created
**      27-Apr-96   floh    zuerst Test, ob ein Sample an der Source
**                          hängt.
**      14-Aug-96   floh    + attached palette fx
**      23-Sep-96   floh    + Bugfix: Paletten-FX wurden nur
**                            akzeptiert, wenn auch ein Sound-Sample
**                            dran hing
**                          + Bugfix: src->start_time wurde nicht
**                            für Paletten-FX initialisiert
**      25-Sep-96   floh    + updated für Shake-FX
**      28-Dec-96   floh    + Support für "Extended Sample Definition"
*/
{
    struct SoundSource *src = &(sc->src[num]);

    src->start_time = MilesBase.time_stamp;

    /*** Extended Sample Definition oder konventionelles Audio-Sample? ***/
    if ((src->flags & AUDIOF_HASEXTSMP) || (src->sample)) {
        src->flags |= AUDIOF_ACTIVE;
        src->flags &= ~AUDIOF_PLAYING;
        src->frag_act = 0;
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
**  FUNCTION
**      Beendet eine SoundSource explizit (nur notwendig bei
**      Loop-Da-Loop-Sources). Wirkliche beendet wird die
**      Source aber erst beim nächsten _EndAudioFrame().
**
**  INPUTS
**      sc      -> SoundCarrier, zu dem die SoundSource gehört
**      num     -> Nummer der SoundSource
**
**  CHANGED
**      15-Apr-96   floh    created
**      14-Aug-96   floh    + attached palette fx
**      25-Sep-96   floh    + attached shake fx
*/
{
    struct SoundSource *src = &(sc->src[num]);
    src->flags &= ~(AUDIOF_ACTIVE | AUDIOF_PFXACTIVE | AUDIOF_SHKACTIVE);
}

/*-----------------------------------------------------------------*/
void audio_RefreshSounds(struct SoundCarrier *sc, FLOAT dist)
/*
**  FUNCTION
**      Geht alle SoundSources des Carriers durch,
**      berechnet resultierende Priorität und
**      ordnet sie in das TopN-Array ein.
**
**  INPUTS
**      sc      - SoundCarrier
**      dist    - Abstand zum Beobachter
**
**  CHANGED
**      14-Aug-96   floh    created
**      28-Dec-96   floh    + Support für Extended Sample Definition
**      19-Jun-97   floh    + falls die resultierende Lautstärke
**                            <= AUDIO_MIN_VOL ist, wird die SoundSource
**                            ignoriert (um die Aliasing-Artefakte zu
**                            entschärfen
**      09-Sep-97   floh    + Divider-Divider experimentell von
**                            150.0 auf 200.0 gesetzt, das dürfte
**                            ein weniger extremes Abfallen der Lautstärke
**                            bewirken.
*/
{
    ULONG i;
    FLOAT divider = dist / 200.0;   // FIXME: war 150.0

    /*** für jede SoundSource... ***/
    for (i=0; i<MAX_SOUNDSOURCES; i++) {

        struct SoundSource *src = &(sc->src[i]);
        if (src->sample || (src->flags & AUDIOF_HASEXTSMP)) {

            ULONG frag=0;
            LONG age;

            /*** inaktive von vornherein ignorieren ***/
            if (!(src->flags & AUDIOF_ACTIVE)) continue;

            /*** ist aktiv, wie lange schon? ***/
            age = MilesBase.time_stamp - src->start_time;

            /*** falls Ext Smp Def, aktives Fragment ermitteln ***/
            if (src->flags & AUDIOF_HASEXTSMP) {
                if (src->flags & AUDIOF_PLAYING) {
                    frag = src->frag_act;
                } else {
                    frag = audio_GetExtSmpFrag(src,age,NULL);
                };
            };

            /*** TimeOut-Deaktivierung bei OneShots? ***/
            if (!(src->flags & AUDIOF_LOOPDALOOP)) {
                if ((src->flags & AUDIOF_PLAYING) == 0) {

                    /*** aktiv, aber nicht abgespielt, also schätzen ***/
                    if (age > 0) {
                        if (src->flags & AUDIOF_HASEXTSMP) {
                            /*** Fragment-Überlauf? ***/
                            if (frag >= src->ext_smp->count) {
                                /*** deaktivieren und ignorieren ***/
                                src->flags &= ~AUDIOF_ACTIVE;
                                continue;
                            };
                        } else {
                            /*** konventionell: Lifetime abschätzen ***/
                            LONG done;
                            done = src->sample->SamplesPerSec + src->pitch;
                            done *= age;
                            done >>= 10;

                            /*** <done> jetzt in etwa Anzahl fertiger Samples ***/
                            if (done > src->sample->Length) {
                                /*** deaktivieren und ignorieren ***/
                                src->flags &= ~AUDIOF_ACTIVE;
                                continue;
                            };
                        };
                    };
                };
            };

            /*** immer noch aktiv: resultierende Lautstärke berechnen ***/
            src->res_volume = src->volume;
            if (src->flags & AUDIOF_HASEXTSMP) {
                src->res_volume += src->ext_smp->frag[frag].volume;
            };

            if (divider >= 1.0) {
                FLOAT vol = ((FLOAT)(src->res_volume)) / divider;
                src->res_volume = (LONG) vol;
            };
            if (src->res_volume <= AUDIO_MIN_VOL) continue;

            /*** Priorität "berechnen" ***/
            src->pri = src->res_volume;

            /*** wenn's geht, in TopN-Array einordnen ***/
            audio_InsertSoundSource(src);
        };
    };
}

/*-----------------------------------------------------------------*/
void audio_RefreshPalFX(struct SoundCarrier *sc, FLOAT dist)
/*
**  FUNCTION
**      Geht alle SoundSources des Carriers mit attached
**      Paletten-FX durch, berechnet die resultierende
**      Magnitude der FX und ordnet sie in das
**      top_palfx[] Array ein.
**
**  CHANGED
**      14-Aug-96   floh    created
**      15-Aug-96   floh    da war noch eine Anomalie bei
**                          der act_mag Ermittlung von LoopDaLoops
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
                    if (stop_time < MilesBase.time_stamp) {
                        src->flags &= ~AUDIOF_PFXACTIVE;
                    };
                };
            };

            /*** inaktive FX ignorieren ***/
            if (!(src->flags & AUDIOF_PFXACTIVE)) continue;

            /*** aktuelle Magnitude berechnen (nur OneShots) ***/
            if (!(src->flags & AUDIOF_LOOPDALOOP)) {
                dt = ((FLOAT)(MilesBase.time_stamp - src->start_time))/
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
**  FUNCTION
**      Geht alle SoundSources des Carriers mit attached
**      Shake-FX durch, berechnet die resultierende
**      Magnitude des FX und ordnet sie in das
**      top_shakefx[] Array ein.
**
**  CHANGED
**      25-Sep-96   floh    created
**      27-Sep-96   floh    Mute-Wert eingebunden
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
                    if (stop_time < MilesBase.time_stamp) {
                        src->flags &= ~AUDIOF_SHKACTIVE;
                    };
                };
            };

            /*** inaktive FX ignorieren ***/
            if (!(src->flags & AUDIOF_SHKACTIVE)) continue;

            /*** aktuelle Magnitude berechnen (nur OneShots) ***/
            if (!(src->flags & AUDIOF_LOOPDALOOP)) {
                FLOAT dt;
                dt = ((FLOAT)(MilesBase.time_stamp - src->start_time))/
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
**  FUNCTION
**      Das Arbeitspferd der AudioEngine. Die Routine muß pro
**      Frame einmal auf jeden SoundCarrier angewendet werden,
**      mit aktualisierten Daten in:
**
**          sc->pos             [empfohlen]
**          sc->vec             [empfohlen]
**          sc->src[n].volume   [optional]
**          sc->src[n].pitch    [optional]
**
**      Die wichtigste Aufgabe von _RefreshSoundCarrier() ist,
**      die resultierende Priorität für jede SoundSource im
**      Carrier zu berechnen, und die jeweils N höchstpriorisierten
**      SoundSources in einer Top-N-Liste zu sammeln (N steht
**      dabei für die Anzahl paralleler Mixer-Kanäle).
**
**      Folgende Schritte werden unternommen:
**
**          (*) Abbruch, wenn SoundCarrier so weit entfernt, das
**              garantiert keine der SoundSources hörbar ist.
**          (*) Bei One-Shot-Samples Untersuchung, ob TimeOut.
**              Wenn dem so ist, wird AUDIOF_ACTIVE gelöscht.
**          (*) Ignoriere SoundSource, wenn AUDIOF_ACTIVE gelöscht
**              (entweder nie _StartSoundSource() angewendet, oder
**              Timed Out bei OneShots, oder _EndSoundSource() bei
**              LoopDaLoops.
**          (*) Je SoundSource berechne resultierende Lautstärke.
**              (aus Entfernung zum Viewer).
**          (*) Abbruch, wenn resultierende Lautstärke <= 0
**          (*) Je SoundSource berechne aus Lautstärke resultierende
**              Priorität.
**          (*) Ersetze niedrigst-priorisierte TopN-Soundsource
**              durch die aktuelle, wenn deren Priorität drüber
**              liegt.
**
**      Wenn also alle SoundCarrier ein audio_RefreshSoundCarrier()
**      hinter sich haben, verfügt die AudioEngine über eine
**      Liste der <num_channels> höchstpriorisierten SoundSources.
**      Den Rest erledigt dann _EndAudioFrame().
**
**  INPUTS
**      sc  -> Pointer auf zu bearbeitende SoundCarrier-Struktur
**
**  CHANGED
**      15-Apr-96   floh    created
**      19-Apr-96   floh    debugging...
**      29-Apr-96   floh    neue Lautstärke-Berechnung.
**      05-Aug-96   floh    Abbruch, wenn AIL nicht initialisiert
**      14-Aug-96   floh    + palette fx handling
**      25-Sep-96   floh    + ruft audio_RefreshShakeFX() auf (aber nur,
**                            wenn dist kleiner als 2400 ist, damit hier
**                            nicht zuviel Arbeit anfällt!)
*/
{
    FLOAT dist;
    FLOAT dx,dy,dz;

    /*** berechne Entfernung zum Hörer ***/
    dx = sc->pos.x - MilesBase.vwr_pos.x;
    dy = sc->pos.y - MilesBase.vwr_pos.y;
    dz = sc->pos.z - MilesBase.vwr_pos.z;

    dist = nc_sqrt(dx*dx + dy*dy + dz*dz);
    if (dist < MAX_AUDIO_DISTANCE) {
        if (MilesBase.driver) audio_RefreshSounds(sc,dist);
        if (dist < 2400.0) {
            if (MilesBase.num_palfx > 0) audio_RefreshPalFX(sc,dist);
            audio_RefreshShakeFX(sc,dist);
        };
    };
}

/*-----------------------------------------------------------------*/
void audio_RemoveSoundZombies(void)
/*
**  FUNCTION
**      Deaktiviert alle momentan spielenden Samples,
**      die beim nächsten Durchgang nicht mehr dabei
**      sein werden.
**
**  CHANGED
**      14-Aug-96   floh    created
**      18-Feb-97   floh    SoundKit-Interface
*/
{
    ULONG i;
    for (i=0; i<MilesBase.num_channels; i++) {

        struct SoundSource *playing = MilesBase.ch[i].src;
        if (playing) {

            ULONG j;
            BOOL cancel_me = TRUE;

            for (j=0; j<MilesBase.num_channels; j++) {
                if (MilesBase.top_n[j] == playing) {
                    /*** Abbruch, weil nach wie vor gültig ***/
                    cancel_me = FALSE;
                    j = MilesBase.num_channels;
                };
            };
            if (cancel_me) {
                /*** ungültig geworden, also raus ***/
                playing->flags &= ~AUDIOF_PLAYING;
                MilesBase.ch[i].src = NULL;
                snd_EndChannel(MilesBase.driver,MilesBase.ch[i].handle);
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void audio_InsertNewSounds(void)
/*
**  FUNCTION
**      Verteilt die "neuen" Sounds im TopN-Array
**      auf die echten Hardware-Mixer-Kanäle.
**
**  CHANGED
**      14-Aug-96   floh    created
**      28-Oct-97   floh    + ouch, übler Bug: wenn alle Channels schon
**                            besetzt waren, hangelte sich die Routine
**                            über das Channels-Array hinaus und machte
**                            den ersten folgenden NULL-Pointer kaputt!
*/
{
    ULONG free_ch = 0;
    ULONG i;
    for (i=0; i < MilesBase.num_channels; i++) {
        struct SoundSource *new_src = MilesBase.top_n[i];
        if (new_src) {
            if (!(new_src->flags & AUDIOF_PLAYING)) {
                for (free_ch; free_ch<MilesBase.num_channels; free_ch++)
                {
                    if (NULL == MilesBase.ch[free_ch].src) {
                        /*** ein freier Kanal! ***/
                        MilesBase.ch[free_ch].src = new_src;
                        break;
                    };
                };
                /*** Überlauf? ***/
                if (free_ch == MilesBase.num_channels) return;
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void audio_PlaySampleFragment(snd_Channel *h,
                              struct SoundSource *src,
                              LONG off)
/*
**  FUNCTION
**      Startet das aktuelle Fragment einer Extended Sample
**      Definition. Das Fragment ist definiert durch
**      <src->frag_act>.
**      Kann sowohl zum "Anstoßen", als auch aus der
**      EOS-Callback-Routine aufgerufen werden.
**      Ein zusätzlicher Offset-Wert in's aktuelle Fragment
**      ist unterstützt.
**
**      src->res_pitch, src->res_volume und src->res_pan
**      werden direkt benutzt, die entsprechenden Fragment-
**      Modifier müssen also bereits reingerechnet sein.
**
**  INPUTS
**      h       - der "Hardware-Kanal"
**      src     - SoundSource, die das Sample-Fragment
**                enthält
**      frag    - Nummer des Fragments
**      off     - Offset in die Sample-Data des Fragments.
**
**  CHANGED
**      28-Dec-96   floh    created
**      18-Feb-97   floh    + SoundKit-Interface
*/
{
    struct SampleFragment *sf = &(src->ext_smp->frag[src->frag_act]);
    UBYTE *data = (UBYTE *) sf->sample->Data;

    snd_StartChannel(MilesBase.driver,h,
                     audio_EOS_CallBack,
                     data+sf->real_offset,
                     sf->real_len,
                     src->res_pitch,
                     src->res_volume,
                     src->res_pan,
                     sf->loop,
                     off);
}

/*-----------------------------------------------------------------*/
void audio_PlaySample(ULONG ch, struct SoundSource *src)
/*
**  FUNCTION
**      Aktiviert SoundSource auf dem übergebenen "Kanal" (snd_Channel *).
**      Falls notwendig, wird per src->start_time ein Offset
**      in die Sample-Data errechnet.
**      Extended Sample Definition werden transparent und
**      bevorzugt abgehandelt.
**      Das Sample wird als Playing gekennzeichnet.
**
**      src->res_pitch, res_volume, res_pan muß gültig sein!
**
**  CHANGED
**      23-Dec-96   floh    created
*/
{
    snd_Channel *h = MilesBase.ch[ch].handle;

    /*** kennzeichne Sample als Playing ***/
    src->flags  |= AUDIOF_PLAYING;

    if (src->flags & AUDIOF_HASEXTSMP) {

        /*** Extended Sample Definition! ***/
        LONG off;
        LONG age;

        /*** Zeitpunkt im Gesamt-Konstrukt ***/
        age = MilesBase.time_stamp - src->start_time;

        /*** <frag> und <off> ermitteln... ***/
        if (age > 0) {

            LONG rel_age;
            ULONG frag;
            struct SampleFragment *sf;

            /*** wurde irgendwann in Vergangenheit gestartet... ***/
            /*** also über den Daumen peilen                    ***/
            frag = audio_GetExtSmpFrag(src,age,&rel_age);

            /*** aktuelles Fragment gefunden (clippen?) ***/
            if (frag == src->ext_smp->count) {
                frag=src->ext_smp->count-1;
            };
            sf = &(src->ext_smp->frag[frag]);

            /*** Offset ins Fragment (<rel_age> ist Zeit seit Frag-Start) ***/
            off   = rel_age * src->res_pitch;
            off >>= 10;
            off  %= sf->real_len;

            /*** aktuelles Fragment eintragen ***/
            src->frag_act = frag;

        } else {
            /*** nagelneu, ist einfacher, erstes SampleFragment auf Off 0 ! ***/
            src->frag_act = 0;
            off = 0;
        };

        /*** <sf> und <off> sind jetzt gültig ***/
        audio_PlaySampleFragment(h,src,off);

    } else {

        /*** konventionelle Sample-Definition ***/
        LONG off;
        UBYTE *smp_data = (UBYTE *) src->sample->Data;

        /*** etwaiges Offset in die Sample-Data ***/
        off = ((MilesBase.time_stamp - src->start_time) *
               (src->sample->SamplesPerSec + src->pitch));
        off >>= 10;
        off  %= src->sample->Length;

        /*** neues Sample initialisieren ***/
        snd_StartChannel(MilesBase.driver,h,
                         audio_EOS_CallBack,
                         smp_data,
                         src->sample->Length,
                         src->res_pitch,
                         src->res_volume,
                         src->res_pan,
                         (src->flags & AUDIOF_LOOPDALOOP) ? 0:1,
                         off);
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
void audio_CreateStereoDoppler(struct SoundSource *src)
/*
**  FUNCTION
**      Rechnet Stereo- und Doppler-Effekt in
**      eine SoundSource. Die SoundSource muß AUDIOF_ACTIVE
**      sein und AUDIOF_PLAYING sein (bzw. unmittelbar
**      AUDIOF_PLAYING werden).
**
**  CHANGED
**      14-Aug-96   floh    created
**      16-Aug-96   floh    beachtet jetzt das RevStereo-Flag
**      28-Dec-96   floh    + beachtet jetzt Ext Sample Defs
**                          + mit interner Fehlerkorrektur für
**                            res_pitch, res_pan und res_volume
**      19-Jun-97   floh    + Fehler-Korrektur des resultierenden
**                            Pitch jetzt mit Häfte und Doppelten
**                            der Original-Tonhöhe
*/
{
    FLOAT x;
    struct SoundCarrier *sc = src->sc;

    /*** Doppler-Effekt ***/

    /* Vektor von SoundSource auf Viewer*/
    FLOAT px = MilesBase.vwr_pos.x - sc->pos.x;
    FLOAT py = MilesBase.vwr_pos.y - sc->pos.y;
    FLOAT pz = MilesBase.vwr_pos.z - sc->pos.z;
    FLOAT len_px = nc_sqrt(px*px + py*py + pz*pz);

    /* resultierender Geschwindigkeits-Vektor */
    FLOAT vx = sc->vec.x - MilesBase.vwr_vec.x;
    FLOAT vy = sc->vec.y - MilesBase.vwr_vec.y;
    FLOAT vz = sc->vec.z - MilesBase.vwr_vec.z;
    FLOAT len_vx = nc_sqrt(vx*vx + vy*vy + vz*vz);

    FLOAT cos_ang;
    LONG u,f,df;

    /* Cos(< a,b) */
    if ((len_px * len_vx) > 0.0) {
        cos_ang = (px*vx + py*vy + pz*vz) / (len_px * len_vx);
    } else {
        cos_ang = 0;
    };

    /* echte Annäherungs-Geschwindigkeit (in Koords/sec) */
    u = (LONG) (cos_ang * len_vx);

    /*** wie beeinflusse ich jetzt die Tonhöhe??? ***/
    /*** Ok, AF weiß Rat: df = f * u/c            ***/
    /***    df -> Frequenz-Verschiebung           ***/
    /***     f -> Grundfrequenz                   ***/
    /***     u -> Objekt-Geschwindigkeit (+/-)    ***/
    /***     c -> Ausbreitungs-Geschwindigkeit    ***/

    if (src->flags & AUDIOF_HASEXTSMP) {
        struct SampleFragment *sf = &(src->ext_smp->frag[src->frag_act]);
        f = sf->sample->SamplesPerSec + sf->pitch + src->pitch;
    } else {
        f = src->sample->SamplesPerSec + src->pitch;
    };
    if (f < AUDIO_MIN_FREQ)       f = AUDIO_MIN_FREQ;
    else if (f > AUDIO_MAX_FREQ)  f = AUDIO_MAX_FREQ;
    df = (f * u) / 400;

    /*** resultierende Frequenz eintragen ***/
    src->res_pitch = f + df;

    /*** Stereo-Position ***/

    /* normalisierte Viewer-lokale X-Objekt-Position */
    x = MilesBase.vwr_mx.m11 * px +
        MilesBase.vwr_mx.m12 * py +
        MilesBase.vwr_mx.m13 * pz;
    if (len_px > 0.0) {
        if (MilesBase.rev_stereo) x /= len_px;
        else                      x /= -len_px;
    }else{
        x=0;
    };

    /*** Stereo-Position zwischen 0..127 ***/
    src->res_pan = (LONG) ((64.0 * x) + 64.0);

    /*** Fehler-Korrektur ***/
    if (src->res_volume < 0)          src->res_volume = 0;
    else if (src->res_volume > 127)   src->res_volume = 127;
    if (src->res_pan < 0)             src->res_pan    = 0;
    else if (src->res_pan > 127)      src->res_pan    = 127;
    if (src->res_pitch < (f>>1)) {
        src->res_pitch = f>>1;
    } else if (src->res_pitch > (f<<1)) {
        src->res_pitch = f<<1;
    };
}

/*-----------------------------------------------------------------*/
void audio_PlayFinalSounds(void)
/*
**  FUNCTION
**      Macht die "übriggebliebenen" Sounds hörbar.
**
**  KNOWN BUGS
**      Stereo- und Doppler-Effekt bei "neuen" (per
**      PlaySample() gestarteten) Extended Sample
**      Definitions für 1 Frame nicht korrekt.
**      Fix wäre ziemlich aufwendig.
**
**  CHANGED
**      14-Aug-96   floh    created
**      03-Sep-96   floh    Fix + Warnung des Div-by-Zero-Flags,
**                          zwecks Absturz-Verhinderung und
**                          Diagnose.
**      28-Dec-96   floh    + beachtet jetzt auch Extended Sample Defs
**                          + CreateStereoDoppler wird jetzt von
**                            hier aufgerufen.
*/
{
    ULONG i;

    /*** Final Loop: neue SoundSources initialis-, alte modifiz-ieren ***/
    for (i=0; i<MilesBase.num_channels; i++) {

        struct SoundSource *src = MilesBase.ch[i].src;
        snd_Channel        *handle = MilesBase.ch[i].handle;

        if (src) {
            if (src->sample || (src->flags & AUDIOF_HASEXTSMP)) {

                /*** Stereo- und Doppler-Effekt reinrechnen ***/
                audio_CreateStereoDoppler(src);

                if (src->flags & AUDIOF_PLAYING) {
                    /*** eine "alte" Source, die schon vorher gedudelt hat ***/
                    /*** diese wird nur modifiziert...                     ***/
                    snd_ModifyChannel(MilesBase.driver,handle,
                                      src->res_pitch,
                                      src->res_volume,
                                      src->res_pan);
                } else {
                    /*** sonst neues Sample starten (Stereo- und ***/
                    /*** Doppler wird intern                     ***/
                    audio_PlaySample(i,src);
                };

            } else {
                _LogMsg("AUDIO ENGINE ERROR #1!\n");
            };
        };
    };
}

/*-----------------------------------------------------------------*/
void audio_PlayFinalPaletteFX(void)
/*
**  FUNCTION
**      Guckt nach, wieviele Paletten-FX in der top_palfx[]
**      stehen, und mixt diese mit der Original-Palette.
**
**  CHANGED
**      14-Aug-96   floh    created
*/
{
    LONG i = 0;
    LONG slot[MILES_NUMPALFX+1];
    LONG weight[MILES_NUMPALFX+1];
    LONG mag = 256;
    Object *gfxo;
    struct SoundSource *src;

    while ((src = MilesBase.top_palfx[i]) && (i<MilesBase.num_palfx)) {
        mag -= src->pfx_actmag;
        slot[i]   = src->pfx->slot;
        weight[i] = (ULONG) (src->pfx_actmag * 256.0);
        mag -= weight[i];
        i++;
    };

    /*** wenn keine Paletten-FX und Default bereits an, Abbruch ***/
    if (i == 0) {
        if (MilesBase.palfx_clean) return;
        else MilesBase.palfx_clean = TRUE;
    } else   MilesBase.palfx_clean = FALSE;

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
**  FUNCTION
**      Vermischt die Top-N-Shakefx (Magnitude addiert sich
**      auf, Frequenz wird gemittelt), rechnet auf
**      jeder Raumachse einen Noise rein und rotiert
**      das Viewer-Koordinaten-System in der MilesBase
**      entsprechend.
**
**  CHANGED
**      26-Sep-96   floh    created
**      03-Oct-96   floh    der Rotations-Shake wird jetzt in die
**                          extra MilesBase.shk_mx eingetragen,
**                          anstatt die originale Viewer-Matrix
**                          direkt zu rotieren, das ist cleaner
**                          für die Outside-World
*/
{
    LONG i=0;
    struct SoundSource *src;
    FLOAT shake_x,shake_y,shake_z;
    struct flt_m3x3 *m = &(MilesBase.shk_mx);

    shake_x = 0.0;
    shake_y = 0.0;
    shake_z = 0.0;
    while ((src = MilesBase.top_shakefx[i]) && (i<MilesBase.num_shakefx)) {
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
**  FUNCTION
**      Berechnet für die Einträge im TopN-Array die
**      "restlichen" 3D-Effekte Stereo-Position und
**      Doppler-Verschiebung, und übergibt sie den
**      Miles-HSAMPLE's zum Mixen.
**
**      - SoundSources, die bereits gespielt wurden,
**        werden nur modifiziert und weiterdudeln
**        gelassen.
**      - Bei SoundSources, die "neu" sind, wird ein (mehr oder
**        weniger genaues) Offset in die Sample-Data berechnet,
**        damit angeschnittene oder unterbrochene Samples
**        einigermaßen richtig klingen, diese Sources
**        ersetzen dann "die anderen", die aus der Ausgabe
**        rausfallen.
**
**  CHANGED
**      15-Apr-96   floh    created
**      17-Apr-96   floh    debugging
**      19-Apr-96   floh    debugging
**      05-Aug-96   floh    Abbruch, wenn AIL nicht initialisiert
**      14-Aug-96   floh    + Paletten-FX-Handling
**      27-Sep-96   floh    + returniert jetzt Ptr auf modifizierte
**                            Viewer-Matrix (Shake-FX)
**
*/
{
    struct snd_cdtrigger_msg cdtrigger;

    /*** Sound-Handling ***/
    if (MilesBase.driver) {

        ULONG i;

        /*** SoundZombies deaktivieren ***/
        audio_RemoveSoundZombies();

        /*** neue Sounds auf freigewordene Kanäle verteilen ***/
        audio_InsertNewSounds();

        /*** schließlich das Resultat zu den Mixerkanälen raus ***/
        audio_PlayFinalSounds();
    };

    /*** Palette-FX-Handling ***/
    if (MilesBase.num_palfx > 0) audio_PlayFinalPaletteFX();

    /*** Shake-FX-Handling (zerstört MilesBase.vwr_mx!) ***/
    audio_PlayFinalShakeFX();

    /*** Triggern des CD-Players ***/
    cdtrigger.global_time    = MilesBase.time_stamp;
    cdtrigger.playitagainsam = 1;
    _TriggerCDPlayer( &cdtrigger );

    /*** das war's... ***/
    return(&(MilesBase.shk_mx));
}

/*-----------------------------------------------------------------*/
void audio_FlushAudio(void)
/*
**  FUNCTION
**      Räumt alle TopN-Arrays und die momentan aktiven
**      Audio-Channels auf.
**
**  CHANGED
**      02-Jan-97   floh    created
*/
{
    ULONG i;

    if (MilesBase.driver) {
        for (i=0; i<MilesBase.num_channels; i++) {
            /*** aktive SoundSource? ***/
            if (MilesBase.ch[i].src) {
                struct snd_Channel *smp = MilesBase.ch[i].handle;
                struct SoundSource *src = MilesBase.ch[i].src;
                src->flags &= ~(AUDIOF_ACTIVE|AUDIOF_PLAYING);
                snd_EndChannel(MilesBase.driver,smp);
                MilesBase.ch[i].src = NULL;
            };
        };
    };

    /*** Top-N-Arrays aufräumen ***/
    for (i=0; i<MilesBase.num_channels; i++) {
        struct SoundSource *src = MilesBase.top_n[i];
        if (src) {
            src->flags &= ~(AUDIOF_ACTIVE|AUDIOF_PLAYING);
            MilesBase.top_n[i] = NULL;
        };
    };
    for (i=0; i<MilesBase.num_palfx; i++) {
        struct SoundSource *src = MilesBase.top_palfx[i];
        if (src) {
            src->flags &= ~(AUDIOF_PFXACTIVE|AUDIOF_PFXPLAYING);
            MilesBase.top_palfx[i] = NULL;
        };
    };
    for (i=0; i<MilesBase.num_shakefx; i++) {
        struct SoundSource *src = MilesBase.top_shakefx[i];
        if (src) {
            src->flags &= ~(AUDIOF_SHKACTIVE|AUDIOF_SHKPLAYING);
            MilesBase.top_shakefx[i] = NULL;
        };
    };
}

