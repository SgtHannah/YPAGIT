/*
**  $Source: PRG:VFM/Classes/_YPAWorldClass/yw_record.c,v $
**  $Revision: 38.5 $
**  $Date: 1997/03/22 18:59:12 $
**  $Locker: floh $
**  $Author: floh $
**
**  Scene-Recorder für YPA.
**
**  (C) Copyright 1996 by A.Weissflog
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <libraries/iffparse.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "nucleus/nucleus2.h"
#include "nucleus/math.h"
#include "ypa/ypaworldclass.h"

#include "thirdparty/eulerangles.h"

#include "yw_protos.h"

_extern_use_nucleus

/*** max. Anzahl Elemente in den Recorder-Puffern ***/
#define RC_NUMELMS   (1024)

/*-----------------------------------------------------------------**
**  Mathe Support Funktionen                                       **
**-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/*
**  FUNCTION
**      Matrix-2-Euler-Funktion.
**      (C) Ken Shoemake, veröffentlicht in Graphics Gems IV.
**
**  CHANGED
**      13-Jan-97   floh
*/
/* Convert matrix to Euler angles (in radians). */
EulerAngles Eul_FromHMatrix(HMatrix M, int order)
{
    EulerAngles ea;
    int i,j,k,h,n,s,f;
    EulGetOrd(order,i,j,k,h,n,s,f);
    if (s==EulRepYes) {
        double sy = sqrt(M[i][j]*M[i][j] + M[i][k]*M[i][k]);
        if (sy > 16*FLT_EPSILON) {
            ea.x = atan2(M[i][j], M[i][k]);
            ea.y = atan2(sy, M[i][i]);
            ea.z = atan2(M[j][i], -M[k][i]);
        } else {
            ea.x = atan2(-M[j][k], M[j][j]);
            ea.y = atan2(sy, M[i][i]);
            ea.z = 0;
        }
    } else {
        double cy = sqrt(M[i][i]*M[i][i] + M[j][i]*M[j][i]);
        if (cy > 16*FLT_EPSILON) {
            ea.x = atan2(M[k][j], M[k][k]);
            ea.y = atan2(-M[k][i], cy);
            ea.z = atan2(M[j][i], M[i][i]);
        } else {
            ea.x = atan2(-M[j][k], M[j][j]);
            ea.y = atan2(-M[k][i], cy);
            ea.z = 0;
        }
    }
    if (n==EulParOdd) {ea.x = -ea.x; ea.y = - ea.y; ea.z = -ea.z;}
    if (f==EulFrmR) {float t = ea.x; ea.x = ea.z; ea.z = t;}
    ea.w = order;
    return (ea);
}

/*-----------------------------------------------------------------*/
void yw_FreeSequenceData(struct YPASeqData *s)
/*
**  FUNCTION
**      Gibt Sequence-Data-Struktur mit allen
**      Puffern frei.
**
**  CHANGED
**      13-Jul-96   floh    created
*/
{
    if (s->buf_store)  _FreeVec(s->buf_store);
    if (s->buf_objinf) _FreeVec(s->buf_objinf);
    if (s->buf_audinf) _FreeVec(s->buf_audinf);
    if (s->buf_modeng) _FreeVec(s->buf_modeng);
    if (s->buf_audcod) _FreeVec(s->buf_audcod);
    _FreeVec(s);
}

/*-----------------------------------------------------------------*/
struct YPASeqData *yw_AllocSequenceData(void)
/*
**  FUNCTION
**      Allokiert und initialisiert eine <struct YPASeqData>.
**
**  CHANGED
**      13-Jul-96   floh    created
*/
{
    struct YPASeqData *s;

    s = (struct YPASeqData *) _AllocVec(sizeof(struct YPASeqData),MEMF_PUBLIC|MEMF_CLEAR);
    if (s) {

        /*** Mitschnitt-Puffer allokieren ***/
        s->buf_store  = (struct Bacterium **)
            _AllocVec(RC_NUMELMS * sizeof(APTR),MEMF_PUBLIC);
        s->buf_objinf = (struct rc_ObjectInfo *)
            _AllocVec(RC_NUMELMS * sizeof(struct rc_ObjectInfo),MEMF_PUBLIC);
        s->buf_audinf = (struct rc_AudioInfo *)
            _AllocVec(RC_NUMELMS * sizeof(struct rc_AudioInfo),MEMF_PUBLIC);
        s->buf_modeng = (struct rc_ModEnergy *)
            _AllocVec(RC_NUMELMS * sizeof(struct rc_ModEnergy),MEMF_PUBLIC);
        s->buf_audcod = (UBYTE *)
            _AllocVec(RC_NUMELMS * 2 * sizeof(struct rc_AudioInfo),MEMF_PUBLIC);

        if (s->buf_store && s->buf_objinf && s->buf_audinf &&
            s->buf_modeng && s->buf_audcod)
        {
            /*** Begrenzer für die Event-Stream-Puffer ***/
            s->max_store  = RC_NUMELMS;
            s->max_modeng = RC_NUMELMS;
            return(s);
        } else {
            yw_FreeSequenceData(s);
        };
    };
    return(NULL);
}

/*-----------------------------------------------------------------*/
void yw_KillSceneRecorder(struct ypaworld_data *ywd)
/*
**  CHANGED
**      06-Jul-96   floh    created
**      13-Jul-96   floh    Revision, damit Player vollkommen parallel
**                          arbeiten kann.
*/
{
    if (ywd->out_seq) {
        if (ywd->out_seq->active) yw_RCEndScene(ywd);
        yw_FreeSequenceData(ywd->out_seq);
    };
}

/*-----------------------------------------------------------------*/
BOOL yw_InitSceneRecorder(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Initialisiert (ywd->out_seq), den Datenbereich
**      des Scene-Recorders.
**
**      Wird nur innerhalb OM_NEW aufgerufen.
**
**  CHANGED
**      06-Jul-96   floh    created
*/
{
    if (ywd->out_seq = yw_AllocSequenceData()) return(TRUE);
    else return(FALSE);
}

/*-----------------------------------------------------------------*/
void yw_RCParseObjectList(struct ypaworld_data *ywd,
                          struct YPASeqData *s,
                          struct MinList *ls)
/*
**  FUNCTION
**      Scannt die angegebene Liste rekursiv, und
**      schreibt die Bact-Pointer nach
**      <s->buf_store>. Die evtl. vorhandenen autonomen Waffen 
**      werden einfach mit reingeschrieben.
**
**  CHANGED
**      06-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
**      15-Jul-96   floh    Object wird ignoriert, wenn
**                          (b->ident < 65535) (kommt nur bei
**                          Kameras vor), UND (b != ywd->URBact) ist,
**                          das alles als Support für den Player, wenn
**                          eine einmal mitgeschnittene Szene nochmal
**                          aufgenommen werden soll.
*/
{
    struct MinNode *nd;

    /*** Geschwader-Liste auf dieser Hierarchie ***/
    for (nd=ls->mlh_Head; nd->mln_Succ; nd=nd->mln_Succ) {

        struct Bacterium *b = ((struct OBNode *)nd)->bact;

        if (!((b->ident < 65535) && (b != ywd->URBact))) {

            /*** aktuelles Vehicle mitschneiden ***/
            if (s->act_store < s->max_store) {
                s->buf_store[s->act_store++] = b;
            };

            /*** dessen Waffen-Liste ***/
            yw_RCParseObjectList(ywd,s,&(b->auto_list));

            /*** dessen Slave-Liste ***/
            yw_RCParseObjectList(ywd,s,&(b->slave_list));
        };
    };
}

/*-----------------------------------------------------------------*/
int __CDECL yw_RCSortHook(struct Bacterium **arg1, struct Bacterium **arg2)
/*
**  FUNCTION
**      QSort-Hook für qsort() auf den <buf_store>. Sortiert
**      wird nach <bact->ident>.
**
**  CHANGED
**      06-Jul-96   floh    created
*/
{
    return(((int)(*arg1)->ident) - ((int)(*arg2)->ident));
}

/*-----------------------------------------------------------------*/
void yw_RCGetObjectInfo(struct ypaworld_data *ywd, 
                        struct YPASeqData *s)
/*
**  FUNCTION
**      Scannt die Geschwader-Liste, sowie alle
**      autonomen Waffen der Objekte der Liste,
**      füllt damit letztenendes ObjectInfo und
**      AudioInfo-Puffer komplett.
**
**  CHANGED
**      06-Jul-96   floh    created
**      08-Jul-96   floh    setzt jetzt zuerst immer <act_store>
**                          zurück.
**      13-Jan-97   floh    Rotation wird jetzt als Euler-Angles
**                          gespeichert.
**      14-Jan-97   floh    dummer Bugfix (hat komischerweise nur
**                          unter Windows zum Absturz geführt),
**                          in AudioInfo-Schleife wurde [i] und
**                          nicht [j] als Index genommen, [i] ist
**                          aber bereits der Index in der äußeren
**                          Schleife
**      20-Jan-97   floh    + verändertes Audio-Format
*/
{
    ULONG i;

    /*** <act_store> zurücksetzen ***/
    s->act_store = 0;

    /*** alle momentan aktiven Objects durchrattern ***/
    yw_RCParseObjectList(ywd,s,&(ywd->CmdList));

    /*** die mitgeschnittenen Bact-Pointer nach <b->ident> sortieren ***/
    qsort(s->buf_store, s->act_store, sizeof(APTR), yw_RCSortHook);

    /*** ObjectInfo und AudioInfo-Puffer ausfüllen ***/
    for (i=0; i<s->act_store; i++) {

        struct Bacterium *b = s->buf_store[i];
        struct rc_ObjectInfo *oi = &(s->buf_objinf[i]);
        struct rc_AudioInfo  *ai = &(s->buf_audinf[i]);
        Object *vproto;
        ULONG j;

        EulerAngles ea;
        HMatrix hm;

        /*** ObjectInfo ausfüllen ***/
        oi->id = b->ident;
        oi->p  = b->pos;

        /*** Matrix->Euler ***/
        hm[0][0]=b->dir.m11; hm[0][1]=b->dir.m12; hm[0][2]=b->dir.m13; hm[0][3]=0.0;
        hm[1][0]=b->dir.m21; hm[1][1]=b->dir.m22; hm[1][2]=b->dir.m23; hm[1][3]=0.0;
        hm[2][0]=b->dir.m31; hm[2][1]=b->dir.m32; hm[2][2]=b->dir.m33; hm[2][3]=0.0;
        hm[3][0]=0.0;        hm[3][1]=0.0;        hm[3][2]=0.0;        hm[3][3]=1.0;
        ea = Eul_FromHMatrix(hm,EulOrdXYZs);
        oi->roll  = (UBYTE) FLOAT_TO_INT(((ea.x/(2*PI))*127.0));
        oi->pitch = (UBYTE) FLOAT_TO_INT(((ea.y/(2*PI))*127.0));
        oi->yaw   = (UBYTE) FLOAT_TO_INT(((ea.z/(2*PI))*127.0));

        _get(b->BactObject, YBA_VisProto, &vproto);
        if (vproto == b->vis_proto_normal)        oi->state = RCSTATE_Normal;
        else if (vproto == b->vis_proto_fire)     oi->state = RCSTATE_Fire;
        else if (vproto == b->vis_proto_wait)     oi->state = RCSTATE_Wait;
        else if (vproto == b->vis_proto_dead)     oi->state = RCSTATE_Dead;
        else if (vproto == b->vis_proto_megadeth) oi->state = RCSTATE_MegaDeth;
        else if (vproto == b->vis_proto_create)   oi->state = RCSTATE_Create;
        else oi->state = RCSTATE_None;

        if (b->BactClassID == BCLID_YPAMISSY) oi->type = RCTYPE_Missy;
        else                                  oi->type = RCTYPE_Vehicle;
        oi->proto = b->TypeID;  // Weapon- oder VehicleProto-Index

        /*** AudioInfo ausfüllen ***/
        ai->active = 0;
        for (j=0; j<MAX_SOUNDSOURCES; j++) {
            ULONG f = b->sc.src[j].flags;
            if (f & (AUDIOF_ACTIVE|AUDIOF_PFXACTIVE|AUDIOF_SHKACTIVE)) {
                ai->active |= (1<<j);
            };
        };
        ai->norm_pitch = b->sc.src[0].pitch;
    };

    /*** Ende ***/
}

/*-----------------------------------------------------------------*/
BOOL yw_RCNewScene(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Eröffnet eine neue Szene.
**
**  CHANGED
**      06-Jul-96   floh    created
**      13-Jul-96   floh    revised & updated
**      15-Jul-96   floh    jetzt etwas cleverere Bezeichnung
**                          für den Filename, sowie Unterscheidung,
**                          ob im Cuttermodus oder nicht (Endung).
**      18-Jul-96   floh    oops, Fehler bei Filenamens-Findung...
**      31-Mar-98   floh    + Ergebnis kommt jetzt nach env/snaps
**
*/
{
    struct YPASeqData *s = ywd->out_seq;
    BOOL retval = FALSE;
    UBYTE fname[256];
    APTR tst_file;
    ULONG serial;

    /*** neue Sequenz, Framenummer zurückspulen ***/
    s->active = FALSE;  // wird bei Erfolg auf TRUE gesetzt!
    s->delay  = 0;      // damit 1. NewFrame einen Snap liefert!
    s->seq.id++;
    s->seq.level = ywd->Level->Num;
    s->frame.id  = 0;
    s->frame.time_stamp = 0;    // TimeStamp rücksetzen

    s->act_store  = 0;
    s->act_modeng = 0;
    s->act_audcod = 0;

    /*** IFF-File zum Schreiben öffnen ***/
    #ifdef YPA_CUTTERMODE
        /*** finde einen Filename, der noch frei ist ***/
        serial = s->seq.id;
        do {
            sprintf(fname, "env/snaps/m%02d%04d.cut", ywd->Level->Num, serial++);
            if (tst_file = _FOpen(fname, "rb")) _FClose(tst_file);
        } while (tst_file);
    #else
        sprintf(fname, "env/snaps/m%02d%04d.raw",ywd->Level->Num,s->seq.id);
    #endif
    if (s->iff = _AllocIFF()) {
        s->iff->iff_Stream = (ULONG) _FOpen(fname,"wb");
        if (s->iff->iff_Stream) {
            _InitIFFasNucleus(s->iff);
            if (_OpenIFF(s->iff,IFFF_WRITE) == 0) {

                struct rc_Sequence seq = s->seq;

                /*** Sequence-Header schreiben ***/
                _PushChunk(s->iff,RCIFF_SeqHeader,ID_FORM,IFFSIZE_UNKNOWN);
                _PushChunk(s->iff,0,RCIFF_SeqInfo,sizeof(struct rc_Sequence));
                _WriteChunkBytes(s->iff,&seq,sizeof(struct rc_Sequence));
                _PopChunk(s->iff);
                retval = TRUE;
                s->active = TRUE;
            } else {
                _FClose((APTR)s->iff->iff_Stream);
                _FreeIFF(s->iff);
                s->iff=NULL;
            };
        } else {
            _FreeIFF(s->iff);
            s->iff=NULL;
        };
    };

    /*** fertig ***/
    return(retval);
}

/*-----------------------------------------------------------------*/
void yw_RCEndScene(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Beendet eine mit yw_RCNewScene() angefangene
**      Sequenz.
**
**  CHANGED
**      06-Jul-96   floh    created
**      07-Jul-96   floh    ooops, _PopChunk() vergessen!
**      13-Jul-96   floh    revised & updated
*/
{
    struct YPASeqData *s = ywd->out_seq;

    s->active = FALSE;
    if (s->iff) {
        _PopChunk(s->iff);
        _CloseIFF(s->iff);
        _FClose((APTR)s->iff->iff_Stream);
        _FreeIFF(s->iff);
        s->iff=NULL;
    };
}

/*-----------------------------------------------------------------*/
void yw_RCNewFrame(struct ypaworld_data *ywd, UWORD ftime)
/*
**  FUNCTION
**      Leitet einen neuen Frame ein.
**
**  CHANGED
**      06-Jul-96   floh    created
**      08-Jul-96   floh    nicht mehr jedes yw_RCNewFrame()
**                          fängt jetzt auch wirklich einen Frame
**                          an, das passiert nur, wenn die Delay-Time
**                          zwischen zwei Snaps verstrichen ist!
**      13-Jul-96   floh    revised & updated
*/
{
    struct YPASeqData *s = ywd->out_seq;

    /*** Timer aktualisieren ***/
    s->frame.time_stamp += ftime;
    s->delay -= ftime;
}

/*-----------------------------------------------------------------*/
void yw_RCEncodeAudioInfo(struct YPASeqData *s)
/*
**  FUNCTION
**      Runlength-encodet <buf_audinf> nach <buf_aufcod>.
**      Resultierende Länge in Bytes kommt nach <act_audcod>.
**
**  NOTE
**      Abgeleitet aus NetPBM, ppmtoilbm.c/runbyte1(),
**      by Robert A. Knop (rknop@mop.caltech.edu).
**
**  CHANGED
**      07-Jul-96   floh    created
*/
{
    LONG in,out,count,hold;
    UBYTE *inbuf  = (UBYTE *) s->buf_audinf;
    UBYTE *outbuf = s->buf_audcod;
    LONG size = s->act_store * sizeof(struct rc_AudioInfo);

    /*** Runlength-Encoder ***/
    in = out = 0;
    while (in<size) {

        if ((in<size-1) && (inbuf[in]==inbuf[in+1])) {
            /* replicate run (Schleife nur als Replikations-Count) */
            for (count=0,hold=in; 
                 (in<size) && (inbuf[in]==inbuf[hold]) && (count<128);
                 in++,count++);
            /* Ergebnis schreiben (nicht in Schleife!) */
            outbuf[out++] = (UBYTE)(BYTE)(-count+1);
            outbuf[out++] = inbuf[hold];
        } else {
            /* literal run */
            hold=out; out++; count=0;
            while (((in>=size-2)&&(in<size))||
                   ((in<size-2)&&
                   ((inbuf[in]!=inbuf[in+1])||(inbuf[in]!=inbuf[in+2]))))
            {
                outbuf[out++]=inbuf[in++];
                if (++count>=128) break;
            };
            outbuf[hold] = count-1;
        };
    };

    /*** Länge Output-Buffer schreiben ***/
    s->act_audcod = out;
}

/*-----------------------------------------------------------------*/
void yw_RCEndFrame(struct ypaworld_data *ywd)
/*
**  FUNCTION
**      Liest Object- und Audio-Informationen aller
**      momentan existierenden Objects, sortiert
**      nach <bact->ident> und schreibt den Frame
**      komplett in den IFF-File.
**
**  CHANGED
**      06-Jul-96   floh    created
**      07-Jul-96   floh    + Ungerade-Chunk-Größen-Korrektur
**                            in der Größen-Berechnung beim
**                            Frame-Header-Chunk
**                          + FORMs waren Kleinbuchstaben, das ist
**                            nach EA-IFF-85 verboten (deshalb hat das
**                            Amiga-Sift gesponnen).
**      13-Jul-96   floh    revised & updated
*/
{
    struct YPASeqData *s = ywd->out_seq;

    /*** Frame nur im 1/4 Sekunden-Takt sichern! ***/
    if (s->delay < 0) {

        ULONG frame_size;       // Gesamt-Frame-FORM-Größe
        ULONG objinf_size;
        ULONG modeng_size;

        /*** lese Object-Info ***/
        yw_RCGetObjectInfo(ywd,s);

        /*** Viewer des Frames als BactID ***/
        s->frame.viewer = ywd->UVBact->ident;

        /*** Audio-Info-Puffer run-length-encoden ***/
        yw_RCEncodeAudioInfo(s);

        /*** berechne Gesamt-Größe des RCIFF_FrameHeader FORM ***/
        objinf_size = s->act_store * sizeof(struct rc_ObjectInfo);
        modeng_size = s->act_modeng * sizeof(struct rc_ModEnergy);

        frame_size = 4+8+sizeof(struct rc_Frame);
        if (objinf_size > 0) {
            frame_size += 8+objinf_size;
            if (frame_size & 1) frame_size++;
        };
        if (s->act_audcod > 0) {
            frame_size += 8+s->act_audcod;
            if (frame_size & 1) frame_size++;
        };
        if (modeng_size > 0) {
            frame_size += 8+modeng_size;
            if (frame_size & 1) frame_size++;
        };

        /*** IFF-Chunks rausschreiben ***/
        _PushChunk(s->iff, RCIFF_FrameHeader, ID_FORM, frame_size);

        _PushChunk(s->iff, 0, RCIFF_FrameInfo, sizeof(struct rc_Frame));
        _WriteChunkBytes(s->iff, &(s->frame), sizeof(struct rc_Frame));
        _PopChunk(s->iff);

        if (objinf_size > 0) {
            _PushChunk(s->iff, 0, RCIFF_ObjectInfo, objinf_size);
            _WriteChunkBytes(s->iff, s->buf_objinf, objinf_size);
            _PopChunk(s->iff);
        };

        if (s->act_audcod > 0) {
            _PushChunk(s->iff, 0, RCIFF_AudioInfo, s->act_audcod);
            _WriteChunkBytes(s->iff, s->buf_audcod, s->act_audcod);
            _PopChunk(s->iff);
        };

        if (modeng_size > 0) {
            _PushChunk(s->iff, 0, RCIFF_ModEnergy, modeng_size);
            _WriteChunkBytes(s->iff, s->buf_modeng, modeng_size);
            _PopChunk(s->iff);
        };

        /*** Frame schließen ***/
        _PopChunk(s->iff);

        /*** diverse Parameter rücksetzen ***/
        s->delay += RC_DELAY_TIME;
        s->act_modeng = 0;
        s->frame.id++;
    };

    /*** Ende ***/
}

