/*
**  $Source: PRG:VFM/Classes/_WavClass/wavclass.c,v $
**  $Revision: 38.2 $
**  $Date: 1996/04/23 00:20:46 $
**  $Locker:  $
**  $Author: floh $
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nucleus/nucleus2.h"
#include "nucleus/nukedos.h"
#include "modules.h"
#include "types.h"
#include "audio/wavclass.h"

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, wav_RSM_CREATE, struct TagItem *tlist);

/*-----------------------------------------------------------------*/
_use_nucleus

#ifdef AMIGA
__far ULONG wav_Methods[NUCLEUS_NUMMETHODS];
#else
ULONG wav_Methods[NUCLEUS_NUMMETHODS];
#endif

struct ClassInfo wav_clinfo;

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeWavClass(ULONG id,...);
__geta4 BOOL FreeWavClass(void);
#else
struct ClassInfo *MakeWavClass(ULONG id,...);
BOOL FreeWavClass(void);
#endif

struct GET_Class wav_GET = {
    &MakeWavClass,
    &FreeWavClass,
};

/*===================================================================
**  *** CODESEGMENT HEADER ***
*/
#ifdef DYNAMIC_LINKING
#ifdef AMIGA
__geta4 struct GET_Class *start(void)
{
    return(&wav_GET);
}
#endif
#endif

#ifdef STATIC_LINKING
/* die Einsprung-Routine */
struct GET_Class *wav_Entry(void)
{
    return(&wav_GET);
};

/* und die zugehörige SegmentInfo-Struktur */
struct SegInfo wav_class_seg = {
    { NULL, NULL,               /* ln_Succ, ln_Pred */
      0, 0,                     /* ln_Type, ln_Pri  */
      "MC2classes:wav.class"    /* der Segment-Name */
    },
    wav_Entry,                  /* Entry()-Adresse */
};
#endif

/*-----------------------------------------------------------------*/
#ifdef DYNAMIC_LINKING
/*-------------------------------------------------------------------
**  Logischerweise kann der NUC_GET-Pointer nicht mit einem
**  _GetTagData() aus dem Nucleus-Kernel ermittelt werden,
**  weil NUC_GET noch nicht initialisiert ist! Deshalb hier eine
**  handgestrickte Routine.
*/
struct GET_Nucleus *local_GetNucleus(struct TagItem *tagList)
{
    register ULONG act_tag;

    while ((act_tag = tagList->ti_Tag) != MID_NUCLEUS) {
        switch (act_tag) {
            case TAG_DONE:  return(NULL); break;
            case TAG_MORE:  tagList = (struct TagItem *) tagList->ti_Data; break;
            case TAG_SKIP:  tagList += tagList->ti_Data; break;
        };
        tagList++;
    };
    return((struct GET_Nucleus *)tagList->ti_Data);
}
#endif /* DYNAMIC_LINKING */

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 struct ClassInfo *MakeWavClass(ULONG id,...)
#else
struct ClassInfo *MakeWavClass(ULONG id,...)
#endif
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    #ifdef DYNAMIC_LINKING
    /* GET-Pointer aus TagArray holen */
    if (!(NUC_GET = local_GetNucleus((struct TagItem *)&id))) return(NULL);
    #endif

    /* Methoden-Array initialisieren */
    memset(wav_Methods,0,sizeof(wav_Methods));

    wav_Methods[RSM_CREATE] = (ULONG) wav_RSM_CREATE;

    /* ClassInfo-Struktur ausfüllen */
    wav_clinfo.superclassid = SAMPLE_CLASSID;
    wav_clinfo.methods      = wav_Methods;
    wav_clinfo.instsize     = 0;     /* KEINE EIGENE INSTANCE-DATA ! */
    wav_clinfo.flags        = 0;

    /* Ende */
    return(&wav_clinfo);
}

/*-----------------------------------------------------------------*/
#ifdef AMIGA
__geta4 BOOL FreeWavClass(void)
#else
BOOL FreeWavClass(void)
#endif
/*
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    return(TRUE);
}

/*=================================================================**
**  SUPPORT FUNKTIONEN                                             **
**=================================================================*/

#ifdef _BigEndian_
#define SWAPW(a)        (WORD)(((UWORD)a>>8)+((((UWORD)a&0xff)<<8)))
#define SWAPU(a)        (UWORD)(((UWORD)a>>8)+((((UWORD)a&0xff)<<8)))
#define SWAPL(a)        (LONG)(((ULONG)a>>24)+(((ULONG)a&0xff0000)>>8)+(((ULONG)a&0xff00)<<8)+(((ULONG)a&0xff)<<24))
#else
#define SWAPW(a) (a)
#define SWAPU(a) (a)
#define SWAPL(a) (a)
#endif

#define riff_ID MAKE_ID('R','I','F','F')
#define wave_ID MAKE_ID('W','A','V','E')
#define fmt_ID  MAKE_ID('f','m','t',' ')
#define data_ID MAKE_ID('d','a','t','a')

/*-----------------------------------------------------------------*/
struct RsrcNode *wav_LoadWav(Object *o, Class *cl, 
                             struct TagItem *tlist, 
                             UBYTE *name)
/*
**  FUNCTION
**      Macht aus einem WAV-File eine komplette Resource-Node
**      mit eingebetteter VFMSample-Struktur und returniert 
**      Pointer darauf.
**
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    APTR file;
    UBYTE buf[256];
    UBYTE *path_prefix;
    struct RsrcNode *rnode = NULL;

    path_prefix = _GetSysPath(SYSPATH_RESOURCES);
    strcpy(buf, path_prefix);
    strcat(buf, name);

    if (file = _FOpen(buf, "rb")) {

        struct RiffHeader *rh;

        /*** lese Header ***/
        _FRead(buf, sizeof(struct RiffHeader), 1, file);
        rh = (struct RiffHeader *) buf;
        v2nl(&(rh->rh_id));
        v2nl(&(rh->rh_type));

        /*** ist es überhaupt ein WAV-File ***/
        if ((rh->rh_id == riff_ID) && (rh->rh_type == wave_ID)) {

            struct RiffChunk *rc;
            struct WavFormat *wf;
            ULONG pb_freq;
            struct TagItem add_tags[4];
            BOOL next_chunk = TRUE;

            while (next_chunk) {

                ULONG chunk_len;
                ULONG chunk_id;
                ULONG read_res;

                /*** nächsten Chunk-Header lesen ***/
                read_res = _FRead(buf, sizeof(struct RiffChunk), 1, file);
                if (read_res != 1) {
                    next_chunk = FALSE;
                    continue;
                };

                rc = (struct RiffChunk *) buf;
                v2nl(&(rc->rc_id));
                chunk_id  = rc->rc_id;
                chunk_len = SWAPL(rc->rc_len);

                switch(chunk_id) {

                    /*** Format-Chunk ***/
                    case fmt_ID:
                        /*** Frequenz interessiert ***/
                        _FRead(buf, chunk_len, 1, file);
                        wf = (struct WavFormat *) buf;
                        pb_freq = SWAPL(wf->nSamplesPerSec);
                        break;

                    /*** Data-Chunk ***/
                    case data_ID:
                        /*** beauftrage Superklasse, VFMSample zu bauen ***/
                        add_tags[0].ti_Tag  = SMPA_Length;
                        add_tags[0].ti_Data = chunk_len;
                        add_tags[1].ti_Tag  = SMPA_Type;
                        add_tags[1].ti_Data = SMPT_8BIT_MONO;
                        add_tags[2].ti_Tag  = TAG_MORE;
                        add_tags[2].ti_Data = (ULONG) tlist;

                        rnode = (struct RsrcNode *) 
                                _supermethoda(cl,o,RSM_CREATE,add_tags);
                        if (rnode) {
                            struct VFMSample *smp;
                            if (smp = (struct VFMSample *) rnode->Handle) {

                                /*** VFMSample fertigmachen ***/
                                _FRead(smp->Data, chunk_len, 1, file);
                                smp->SamplesPerSec = pb_freq;

                            } else {
                                _method(o, RSM_FREE, (ULONG) rnode);
                                _FClose(file);
                                return(NULL);
                            };
                        };
                        break;

                    /*** irgendein anderer Chunk -> überspringen ***/
                    default:
                        _FSeek(file,chunk_len,SEEK_CUR);
                        break;
                };
            };

        } else {
            _LogMsg("wav.class: Not a wav file.\n");
        };
        _FClose(file);
    };

    /*** Ende ***/
    return(rnode);
}

/*=================================================================**
**  METHODEN HANDLER                                               **
**=================================================================*/

/*-----------------------------------------------------------------*/
_dispatcher(struct RsrcNode *, wav_RSM_CREATE, struct TagItem *tlist)
/*
**  FUNCTION
**      Erzeugt eine VFMSample-Resource aus einem #?.wav
**      File. Bisher werden nur Standalone-Files unterstützt,
**      keine IFF-Streams.
**
**  CHANGED
**      18-Apr-96   floh    created
*/
{
    struct RsrcNode *rnode = NULL;
    UBYTE *name;

    /*** RSA_Name muß vorhanden sein ***/
    name = (UBYTE *) _GetTagData(RSA_Name, NULL, tlist);
    if (name) {

        /*** IFF-Streams werden ignoriert! ***/
        rnode = wav_LoadWav(o,cl,tlist,name);

    };

    /*** das war's ***/
    return(rnode);
}

